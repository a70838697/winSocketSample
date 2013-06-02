#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define PORT     5150
#define MSGSIZE 1024
typedef enum
{
	RECV_POSTED
}OPERATION_TYPE;       //枚举,表示状态

typedef struct
{
	WSAOVERLAPPED   overlap;      
	WSABUF          Buffer;        
	char            szMessage[MSGSIZE];
	DWORD           NumberOfBytesRecvd;
	DWORD           Flags;
	OPERATION_TYPE OperationType;
}PER_IO_OPERATION_DATA, *LPPER_IO_OPERATION_DATA;    //定义一个结构体保存IO数据

DWORD WINAPI ThreadProc(
	__in  LPVOID lpParameter
	)
{
	HANDLE                   CompletionPort=(HANDLE)lpParameter;
	DWORD                    dwBytesTransferred;
	SOCKET                   sClient=INVALID_SOCKET;
	LPPER_IO_OPERATION_DATA lpPerIOData = NULL;

	while (TRUE)
	{
		if(!GetQueuedCompletionStatus( //遇到可以接收数据则返回，否则等待
			CompletionPort,
			&dwBytesTransferred, //返回的字数
			(PULONG_PTR) &sClient,           //是响应的哪个客户套接字？
			(LPOVERLAPPED *)&lpPerIOData, //得到该套接字保存的IO信息
			INFINITE)){						//无限等待咯。不超时的那种。
				if(WSAGetLastError()==64){
					// Connection was closed by client
					//下面三句可以注释掉，因为还会走到下面关闭的流程
					closesocket(sClient);
					HeapFree(GetProcessHeap(), 0, lpPerIOData);        //释放结构体
					continue;
				}
				else
				{
				wprintf(L"GetQueuedCompletionStatus: %ld\n", WSAGetLastError());
				return -1;
				}
		}
		if (dwBytesTransferred == 0xFFFFFFFF)
		{
			return 0;
		}

		if (lpPerIOData->OperationType == RECV_POSTED) //如果受到数据
		{
			if (dwBytesTransferred == 0)
			{
				// Connection was closed by client
				closesocket(sClient);
				HeapFree(GetProcessHeap(), 0, lpPerIOData);        //释放结构体
			}
			else
			{
				lpPerIOData->szMessage[dwBytesTransferred] = '\0';
				
				//将接收到的消息返回
				int sendCount,currentPosition=0,count=dwBytesTransferred;
				while( count>0 && (sendCount=send(sClient ,lpPerIOData->szMessage+currentPosition,count,0))!=SOCKET_ERROR)
				{
					count-=sendCount;
					currentPosition+=sendCount;
				}
				//------------------------------------------------------
				//if(sendCount==SOCKET_ERROR)break; there is error here


				// Launch another asynchronous operation for sClient
				memset(lpPerIOData, 0, sizeof(PER_IO_OPERATION_DATA));
				lpPerIOData->Buffer.len = MSGSIZE;
				lpPerIOData->Buffer.buf = lpPerIOData->szMessage;
				lpPerIOData->OperationType = RECV_POSTED;
				WSARecv(sClient,               //循环接收
					&lpPerIOData->Buffer,
					1,
					&lpPerIOData->NumberOfBytesRecvd,
					&lpPerIOData->Flags,
					&lpPerIOData->overlap,
					NULL);
			}
		}
	}
	return 0;
}

int main(int argc, char* argv[])
{
	// 初始化完成端口
	HANDLE CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if(CompletionPort==NULL){
		wprintf(L"CreateIoCompletionPort failed with error: %ld\n", WSAGetLastError());
		return 1;
	}

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


	// 有几个CPU就创建几个工作者线程
	SYSTEM_INFO              systeminfo;
	GetSystemInfo(&systeminfo);

	for (unsigned i = 0; i < systeminfo.dwNumberOfProcessors; i++)
	{
		DWORD dwThread;
		HANDLE hThread = CreateThread(NULL,0,ThreadProc,(LPVOID)CompletionPort,0,&dwThread);
		if(hThread==NULL)
		{
			wprintf(L"Thread Creat Failed!\n");
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}
		CloseHandle(hThread);
	}


	SOCKADDR_IN addrClient;
	int len=sizeof(SOCKADDR);
	//以一个无限循环的方式，不停地接收客户端socket连接
	while(1)
	{
		//请求到来后，接受连接请求，返回一个新的对应于此次连接的套接字
		SOCKET AcceptSocket=accept(ListenSocket,(SOCKADDR*)&addrClient,&len);
		if(AcceptSocket  == INVALID_SOCKET)break; //出错

		//将这个最新到来的客户套接字和完成端口绑定到一起。
		CreateIoCompletionPort((HANDLE)AcceptSocket, CompletionPort, ( ULONG_PTR)AcceptSocket, 0);
		//第三个参数表示传递的参数，这里就传递的客户套接字地址。最后一个参数为0 表示有和CPU一样的进程数。即1个CPU一个线程

		// 初始化结构体
		LPPER_IO_OPERATION_DATA lpPerIOData = (LPPER_IO_OPERATION_DATA)HeapAlloc(
			GetProcessHeap(),
			HEAP_ZERO_MEMORY,
			sizeof(PER_IO_OPERATION_DATA));
		lpPerIOData->Buffer.len = MSGSIZE; // len=1024
		lpPerIOData->Buffer.buf = lpPerIOData->szMessage;
		lpPerIOData->OperationType = RECV_POSTED; //操作类型
		WSARecv(AcceptSocket,         //异步接收消息，立刻返回。
			&lpPerIOData->Buffer, //获得接收的数据
			1,       //The number of WSABUF structures in the lpBuffers array.
			&lpPerIOData->NumberOfBytesRecvd, //接收到的字节数，如果错误返回0
			&lpPerIOData->Flags,       //参数，先不管
			&lpPerIOData->overlap,     //输入这个结构体咯。
			NULL);
	}

	//posts an I/O completion packet to an I/O completion port.
	PostQueuedCompletionStatus(CompletionPort, 0xFFFFFFFF, 0, NULL);
	CloseHandle(CompletionPort);
	closesocket(ListenSocket);
	WSACleanup();
	return 0;
}

