#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
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
    // Create a SOCKET for listening for
    // incoming connection requests.
    SOCKET updReceiverSocket;
    updReceiverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (updReceiverSocket == INVALID_SOCKET) {
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
    if (bind(updReceiverSocket,(SOCKADDR *) & addrServer, sizeof (addrServer)) == SOCKET_ERROR) {
        wprintf(L"bind failed with error: %ld\n", WSAGetLastError());
        closesocket(updReceiverSocket);
        WSACleanup();
        return 1;
    }

	//以一个无限循环的方式，不停地接收客户端socket连接
	while(1)
	{
		//接收缓冲区的大小是1024个字符
		char recvBuf[2048+1];
		     
        struct sockaddr_in cliAddr;          //定义IPV4地址变量
        int cliAddrLen = sizeof(cliAddr);
        int count = recvfrom(updReceiverSocket,recvBuf,2048,0,(struct sockaddr *)&cliAddr, &cliAddrLen);
		if(count==0)break;//被对方关闭
		if(count==SOCKET_ERROR)break;//错误count<0
		recvBuf[count]='\0';
		printf("client IP = %s:%s\n",inet_ntoa(cliAddr.sin_addr),recvBuf);  //显示发送数据的IP地址
		//对于udp包只能一次成功
        //if(sendto(SrvSocket, recvBuf,count,0,(struct sockaddr *)&cliAddr,cliAddrLen)<count)
		//	break;
	}

	closesocket(updReceiverSocket);
	WSACleanup();
	return 0;
}

