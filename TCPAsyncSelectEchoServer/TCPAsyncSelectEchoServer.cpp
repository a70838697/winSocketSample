#include <winsock.h>
#include <tchar.h>

#define MSGSIZE   1024
#define WM_SOCKET WM_USER+0

#pragma comment(lib, "ws2_32.lib")

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	static TCHAR szAppName[] = _T("AsyncSelect Model");
	HWND         hwnd ;
	MSG          msg ;
	WNDCLASS     wndclass ;

	wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
	wndclass.lpfnWndProc   = WndProc ;
	wndclass.cbClsExtra    = 0 ;
	wndclass.cbWndExtra    = 0 ;
	wndclass.hInstance     = hInstance ;
	wndclass.hIcon         = LoadIcon (NULL, IDI_APPLICATION) ;
	wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
	wndclass.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH) ;
	wndclass.lpszMenuName  = NULL ;
	wndclass.lpszClassName = szAppName ;

	if (!RegisterClass(&wndclass))
	{
		MessageBox (NULL, TEXT ("This program requires Windows NT!"), szAppName, MB_ICONERROR) ;
		return 0 ;
	}

	hwnd = CreateWindow (szAppName,                  // window class name
		TEXT ("AsyncSelect Model"), // window caption
		WS_OVERLAPPEDWINDOW,        // window style
		CW_USEDEFAULT,              // initial x position
		CW_USEDEFAULT,              // initial y position
		CW_USEDEFAULT,              // initial x size
		CW_USEDEFAULT,              // initial y size
		NULL,                       // parent window handle
		NULL,                       // window menu handle
		hInstance,                  // program instance handle
		NULL) ;                     // creation parameters

	ShowWindow(hwnd, iCmdShow);
	UpdateWindow(hwnd);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg) ;
		DispatchMessage(&msg) ;
	}

	return msg.wParam;
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WSADATA       wsd;
	static SOCKET sListen;
	SOCKET        sClient;
	SOCKADDR_IN   local, client;
	int           ret, iAddrSize = sizeof(client);
	char          szMessage[MSGSIZE];

	switch (message)
	{
	case WM_CREATE:
		// Initialize Windows Socket library
		WSAStartup(0x0202, &wsd);

		// Create listening socket
		sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		// Bind
		local.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
		local.sin_family = AF_INET;
		local.sin_port = htons(20131);
		bind(sListen, (struct sockaddr *)&local, sizeof(local));

		// Listen
		listen(sListen, 3);

		// Associate listening socket with FD_ACCEPT event
		WSAAsyncSelect(sListen, hwnd, WM_SOCKET, FD_ACCEPT);
		return 0;

	case WM_DESTROY:
		closesocket(sListen);
		WSACleanup();
		PostQuitMessage(0);
		return 0;

	case WM_SOCKET:
		if (WSAGETSELECTERROR(lParam))
		{
			closesocket(wParam);
			break;
		}

		switch (WSAGETSELECTEVENT(lParam))
		{
		case FD_ACCEPT:
			// Accept a connection from client
			sClient = accept(wParam, (struct sockaddr *)&client, &iAddrSize);

			// Associate client socket with FD_READ and FD_CLOSE event
			WSAAsyncSelect(sClient, hwnd, WM_SOCKET, FD_READ | FD_CLOSE);
			break;

		case FD_READ:
			ret = recv(wParam, szMessage, MSGSIZE, 0);

			if (ret == 0 || ret == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET)
			{
				closesocket(wParam);
			}
			else
			{
				int sendCount,currentPosition=0,nRecv=ret;
				//write 也可以select的 此处简化处理
				while( nRecv>0 && (sendCount=send(wParam ,szMessage+currentPosition,nRecv,0))!=SOCKET_ERROR)
				{
					nRecv-=sendCount;
					currentPosition+=sendCount;
				}

			}
			break;

		case FD_CLOSE:
			closesocket(wParam);     
			break;
		}
		return 0;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}