#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
// Need to link with Ws2_32.lib
/*http://blog.csdn.net/mlite/article/details/699340*/
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
    SOCKET ListenSocket;
    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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

	// select模型处理过程  
    // 1）初始化一个套节字集合fdSocket，添加监听套节字句柄到这个集合  
    fd_set fdSocket;        // 所有可用套节字集合  
    FD_ZERO(&fdSocket);  
    FD_SET(ListenSocket, &fdSocket); 

	//以一个无限循环的方式，不停地检查是否还有集合里元素没处理
	while(fdSocket.fd_count>0)
	{
		// 2）将fdSocket集合的一个拷贝fdRead传递给select函数，  
        // 当有事件发生时，select函数移除fdRead集合中没有未决I/O操作的套节字句柄，然后返回。  
		fd_set fdRead = fdSocket;  
        int nRet = ::select(0, &fdRead, NULL, NULL, NULL);  
        if(nRet > 0)  
        {  
            // 3）通过将原来fdSocket集合与select处理过的fdRead集合比较，  
            // 确定都有哪些套节字有未决I/O，并进一步处理这些I/O。  
            for(int i=0; i<(int)fdSocket.fd_count; i++)  
            {  
                if(FD_ISSET(fdSocket.fd_array[i], &fdRead))  
                {  
                    if(fdSocket.fd_array[i] == ListenSocket)     // （1）监听套节字接收到新连接  
                    {  
                        if(fdSocket.fd_count < FD_SETSIZE)  
                        {  
                            sockaddr_in addrRemote;  
                            int nAddrLen = sizeof(addrRemote);  
                            SOCKET sNew = ::accept(ListenSocket, (SOCKADDR*)&addrRemote, &nAddrLen);  
							if(sNew  == INVALID_SOCKET)//error
							{
								FD_CLR(ListenSocket, &fdSocket);
							}
							else{
								FD_SET(sNew, &fdSocket);  
								printf("接收到连接（%s）\n", ::inet_ntoa(addrRemote.sin_addr));  
							}
                        }  
                        else  
                        {  
                            printf(" Too much connections! \n");  
                            continue;  
                        }  
                    }  
                    else  
                    {  
                        char szText[256];  
                        int nRecv = ::recv(fdSocket.fd_array[i], szText, 255, 0);
						szText[nRecv] = '\0'; 
                        if(nRecv > 0)                        // （2）可读  
                        {
							int sendCount,currentPosition=0;
							//write 也可以select的 此处简化处理
							while( nRecv>0 && (sendCount=send(fdSocket.fd_array[i] ,szText+currentPosition,nRecv,0))!=SOCKET_ERROR)
							{
								nRecv-=sendCount;
								currentPosition+=sendCount;
							}
							if(sendCount==SOCKET_ERROR){
								::closesocket(fdSocket.fd_array[i]);  
								FD_CLR(fdSocket.fd_array[i], &fdSocket);
							}								;
                            printf("接收到数据：%s \n", szText);  
                        }  
                        else                                // （3）连接关闭、重启或者中断  =0 <0 SOCKET_ERROR
                        {
							printf("close a client\n"); 
                            ::closesocket(fdSocket.fd_array[i]);  
                            FD_CLR(fdSocket.fd_array[i], &fdSocket);  
                        }  
                    }  
                }  
            }  
        }  
        else  
        {  
            printf(" Failed select() \n");  
            break;  
        }  
	}

	closesocket(ListenSocket);
	WSACleanup();
	return 0;
}

