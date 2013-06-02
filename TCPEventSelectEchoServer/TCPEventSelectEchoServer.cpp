#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define PORT     5150
#define MSGSIZE 1024

int                     g_iTotalConn = 0;
SOCKET                  g_CliSocketArr[MAXIMUM_WAIT_OBJECTS];
WSAEVENT                g_CliEventArr[MAXIMUM_WAIT_OBJECTS];
void Cleanup(int);

DWORD WINAPI ThreadProc(
	__in  LPVOID lpParameter
	)
{
	int   ret, index;
	DWORD cbTransferred;
	WSANETWORKEVENTS NetworkEvents;
	char             szMessage[MSGSIZE];

	while (TRUE)
	{
		ret = WSAWaitForMultipleEvents(g_iTotalConn, g_CliEventArr, FALSE, 1000, FALSE);
		if (ret == WSA_WAIT_FAILED || ret == WSA_WAIT_TIMEOUT)
		{
			continue;
		}

		index = ret - WSA_WAIT_EVENT_0;
		WSAEnumNetworkEvents(g_CliSocketArr[index], g_CliEventArr[index], &NetworkEvents);

		if (NetworkEvents.lNetworkEvents & FD_READ)
		{
			// Receive message from client
			ret = recv(g_CliSocketArr[index], szMessage, MSGSIZE, 0);
			if (ret == 0 || (ret == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET))
			{
				Cleanup(index);
			}
			else
			{
				// szMessage contains the received data
				szMessage[ret] = '/0';

				//将接收到的消息返回
				int sendCount,currentPosition=0,count=ret;
				while( count>0 && (sendCount=send(g_CliSocketArr[index] , szMessage+currentPosition,count,0))!=SOCKET_ERROR)
				{
					count-=sendCount;
					currentPosition+=sendCount;
				}

				if (NetworkEvents.lNetworkEvents & FD_CLOSE)
				{
					Cleanup(index);
				}
			}
		}
	}

	return 0;
}

int main(int argc, char* argv[])
{
	//----------------------
	// Initialize Winsock.
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		wprintf(L"WSAStartup failed with error: %ld\n", iResult);
		return 1;
	}
	//----------------------
	// Create a SOCKET for listening for
	// incoming connection requests.
	SOCKET ListenSocket=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket == INVALID_SOCKET) {
		wprintf(L"socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	//----------------------
	// The sockaddr_in structure specifies the address family,
	// IP address, and port for the socket that is being bound.
	sockaddr_in addrServer;
	addrServer.sin_family = AF_INET;
	addrServer.sin_addr.s_addr = htonl(INADDR_ANY); //实际上是0
	addrServer.sin_port = htons(20131);


	//绑定套接字到一个IP地址和一个端口上
	if (bind(ListenSocket,(SOCKADDR *) & addrServer, sizeof (addrServer)) == SOCKET_ERROR) {
		wprintf(L"bind failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	//将套接字设置为监听模式等待连接请求
	//----------------------
	// Listen for incoming connection requests.
	// on the created socket
	if (listen(ListenSocket, 5) == SOCKET_ERROR) {
		wprintf(L"listen failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}


	DWORD dwThread;
	HANDLE hThread = CreateThread(NULL,0,ThreadProc,NULL,0,&dwThread);
	if(hThread==NULL)
	{
		wprintf(L"Thread Creat Failed!\n");
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	CloseHandle(hThread);


	SOCKADDR_IN addrClient;
	int len=sizeof(SOCKADDR);
	//以一个无限循环的方式，不停地接收客户端socket连接
	while(1)
	{
		//请求到来后，接受连接请求，返回一个新的对应于此次连接的套接字
		SOCKET AcceptSocket=accept(ListenSocket,(SOCKADDR*)&addrClient,&len);
		if(AcceptSocket  == INVALID_SOCKET)break; //出错

		g_CliSocketArr[g_iTotalConn] = AcceptSocket;

		g_CliEventArr[g_iTotalConn] =  WSACreateEvent();

		// Launch an asynchronous operation
		WSAEventSelect(g_CliSocketArr[g_iTotalConn],
			g_CliEventArr[g_iTotalConn],
			FD_READ | FD_CLOSE);
		g_iTotalConn++;

	}

	closesocket(ListenSocket);
	WSACleanup();
	return 0;
}

void Cleanup(int index)
{
	closesocket(g_CliSocketArr[index]);
	WSACloseEvent(g_CliEventArr[index]);

	if (index < g_iTotalConn - 1)
	{
		g_CliSocketArr[index] = g_CliSocketArr[g_iTotalConn - 1];
		g_CliEventArr[index] = g_CliEventArr[g_iTotalConn - 1];
	}

	g_iTotalConn--;
}