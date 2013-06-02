#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
#include <io.h>
// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

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
	// Create a SOCKET for connecting to server
	SOCKET senderSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (senderSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError() );
		WSACleanup();
		return 1;
	}

	BOOL bBoardcast = TRUE;  
	if(SOCKET_ERROR == setsockopt(senderSocket, SOL_SOCKET, SO_BROADCAST, (char*)&bBoardcast, sizeof(bBoardcast)))  
	{  
		printf("setsockopt failed with error code: %d\n", WSAGetLastError());  
		closesocket(senderSocket);  
		WSACleanup();  
	}   
	//----------------------
	// The sockaddr_in structure specifies the address family,
	// IP address, and port for the socket that is being bound.
	sockaddr_in addrServer;
	addrServer.sin_family = AF_INET;
	addrServer.sin_addr.s_addr = htonl(INADDR_BROADCAST); //255.255.255.255
	addrServer.sin_port = htons(20131);

	char buf[2048+1];
	int srvAddrLen = sizeof(addrServer);
	//以一个无限循环的方式，不停地接收输入，发送到server
	while(gets_s(buf,2048)!=NULL)
	{
		int count = strlen(buf);//从标准输入读入
		if(sendto(senderSocket, buf,count,0,(struct sockaddr *)&addrServer,srvAddrLen)<count)
			break;
	}
	//结束连接
	closesocket(senderSocket);
	WSACleanup();
	return 0;
}

