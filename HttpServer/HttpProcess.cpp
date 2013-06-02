#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
#pragma warning (disable: 4786) 
#include<map>
#include<string>
#include<ctime>
using namespace std;

bool ParseRequest(string szRequest, string &szResponse, bool &bKeepAlive);

BOOL writeToSocket(SOCKET s,const char *buff, int count)
{
	int sendCount,currentPosition=0;
	while( count>0 && (sendCount=send(s ,buff+currentPosition,count,0))!=SOCKET_ERROR)
	{
		count-=sendCount;
		currentPosition+=sendCount;
	}
	if(sendCount==SOCKET_ERROR)return FALSE;
	return TRUE;
}
BOOL writeToSocket(SOCKET s,char *pszBuff)
{
	return writeToSocket(s,pszBuff,strlen(pszBuff));
}

DWORD WINAPI HttpProcess(
	__in  LPVOID lpParameter
	)
{
	SOCKET clientSocket=(SOCKET) lpParameter;
	//接收缓冲区的大小是20480个字符
	char recvBuf[20480];
	bool bkeepalive=true;
	// Loop as long as the connection is opened.
	while( bkeepalive )
	{
		int count =recv(clientSocket ,recvBuf,20480,0);
		if(count==0)break;//被对方关闭
		if(count==SOCKET_ERROR)break;//错误count<0
		//if(count>0)
		recvBuf[count]=0;

		string szRequest(recvBuf);
		string szResponse;
		bkeepalive=false;
		ParseRequest(szRequest,szResponse,bkeepalive);
		writeToSocket(clientSocket,szResponse.c_str(),(int)szResponse.length());
	}


	//结束连接
	closesocket(clientSocket);
	return 0;
}

typedef map<string, string>				MIMETYPES;
MIMETYPES  MimeTypes;
string m_HomeDir;
string m_DefIndex;
#define ERROR404 "404.html"
#define ERROR501 "501.html"
#define SERVERNAME "localhost"


void Init()
{
	//初始化 设置虚拟目录
	m_HomeDir		= ".";
	m_DefIndex		= "index.html";

	if(m_HomeDir.substr(m_HomeDir.size() - 1, 1) != "\\")
		m_HomeDir += "\\";	
	//
	// Init MIME Types
	//
	MimeTypes["doc"]	= "application/msword";
	MimeTypes["bin"]	= "application/octet-stream";
	MimeTypes["dll"]	= "application/octet-stream";
	MimeTypes["exe"]	= "application/octet-stream";
	MimeTypes["pdf"]	= "application/pdf";
	MimeTypes["p7c"]	= "application/pkcs7-mime";
	MimeTypes["ai"]		= "application/postscript";
	MimeTypes["eps"]	= "application/postscript";
	MimeTypes["ps"]		= "application/postscript";
	MimeTypes["rtf"]	= "application/rtf";
	MimeTypes["fdf"]	= "application/vnd.fdf";
	MimeTypes["arj"]	= "application/x-arj";
	MimeTypes["rar"]	= "application/x-compressed";
	MimeTypes["gz"]		= "application/x-gzip";
	MimeTypes["class"]	= "application/x-java-class";
	MimeTypes["js"]		= "application/x-javascript";
	MimeTypes["lzh"]	= "application/x-lzh";
	MimeTypes["lnk"]	= "application/x-ms-shortcut";
	MimeTypes["tar"]	= "application/x-tar";
	MimeTypes["hlp"]	= "application/x-winhelp";
	MimeTypes["cert"]	= "application/x-x509-ca-cert";
	MimeTypes["zip"]	= "application/zip";
	MimeTypes["cab"]	= "application/x-compressed";
	MimeTypes["arj"]	= "application/x-compressed";
	MimeTypes["aif"]	= "audio/aiff";
	MimeTypes["aifc"]	= "audio/aiff";
	MimeTypes["aiff"]	= "audio/aiff";
	MimeTypes["au"]		= "audio/basic";
	MimeTypes["snd"]	= "audio/basic";
	MimeTypes["mid"]	= "audio/midi";
	MimeTypes["rmi"]	= "audio/midi";
	MimeTypes["mp3"]	= "audio/mpeg";
	MimeTypes["vox"]	= "audio/voxware";
	MimeTypes["wav"]	= "audio/wav";
	MimeTypes["ra"]		= "audio/x-pn-realaudio";
	MimeTypes["ram"]	= "audio/x-pn-realaudio";
	MimeTypes["bmp"]	= "image/bmp";
	MimeTypes["gif"]	= "image/gif";
	MimeTypes["jpeg"]	= "image/jpeg";
	MimeTypes["jpg"]	= "image/jpeg";
	MimeTypes["tif"]	= "image/tiff";
	MimeTypes["tiff"]	= "image/tiff";
	MimeTypes["xbm"]	= "image/xbm";
	MimeTypes["wrl"]	= "model/vrml";
	MimeTypes["htm"]	= "text/html";
	MimeTypes["html"]	= "text/html";
	MimeTypes["c"]		= "text/plain";
	MimeTypes["cpp"]	= "text/plain";
	MimeTypes["def"]	= "text/plain";
	MimeTypes["h"]		= "text/plain";
	MimeTypes["txt"]	= "text/plain";
	MimeTypes["rtx"]	= "text/richtext";
	MimeTypes["rtf"]	= "text/richtext";
	MimeTypes["java"]	= "text/x-java-source";
	MimeTypes["css"]	= "text/css";
	MimeTypes["mpeg"]	= "video/mpeg";
	MimeTypes["mpg"]	= "video/mpeg";
	MimeTypes["mpe"]	= "video/mpeg";
	MimeTypes["avi"]	= "video/msvideo";
	MimeTypes["mov"]	= "video/quicktime";
	MimeTypes["qt"]		= "video/quicktime";
	MimeTypes["shtml"]	= "wwwserver/html-ssi";
	MimeTypes["asa"]	= "wwwserver/isapi";
	MimeTypes["asp"]	= "wwwserver/isapi";
	MimeTypes["cfm"]	= "wwwserver/isapi";
	MimeTypes["dbm"]	= "wwwserver/isapi";
	MimeTypes["isa"]	= "wwwserver/isapi";
	MimeTypes["plx"]	= "wwwserver/isapi";
	MimeTypes["url"]	= "wwwserver/isapi";
	MimeTypes["cgi"]	= "wwwserver/isapi";
	MimeTypes["php"]	= "wwwserver/isapi";
	MimeTypes["wcgi"]	= "wwwserver/isapi";
}


//解析数据并且将文件中的内容读出来
//szRequest 用户发过来的数据
//szResponse 用来保存要发送回给客户的数据
bool ParseRequest(string szRequest, string &szResponse, bool &bKeepAlive)
{
	//
	// Simple Parsing of Request
	//
	string szMethod;
	string szFileName;
	string szFileExt;
	string szStatusCode("200 OK");
	string szContentType("text/html");
	string szConnectionType("close");
	string szNotFoundMessage;
	string szDateTime;
	char pResponseHeader[2048];
	fpos_t lengthActual = 0, length = 0;
	char *pBuf = NULL;
	int n;

	//
	// Check Method
	//
	n = szRequest.find(" ", 0);
	if(n != string::npos)
	{
		szMethod = szRequest.substr(0, n);
		if(szMethod == "GET")
		{
			//
			// Get file name
			// 
			int n1 = szRequest.find(" ", n + 1);
			if(n != string::npos)
			{
				szFileName = szRequest.substr(n + 1, n1 - n - 1);
				if(szFileName == "/")
				{
					szFileName = m_DefIndex;
				}
			}
			else
			{
				return false;
			}
		}
		else
		{
			szStatusCode = "501 Not Implemented";
			szFileName = ERROR501;
		}
	}
	else
	{
		return false;
	}

	//
	// Determine Connection type
	//
	n = szRequest.find("\nConnection: Keep-Alive", 0);
	if(n != string::npos)
		bKeepAlive = true;

	//
	// Figure out content type
	//
	int nPointPos = szFileName.rfind(".");
	if(nPointPos != string::npos)
	{
		szFileExt = szFileName.substr(nPointPos + 1, szFileName.size());
		strlwr((char*)szFileExt.c_str());
		MIMETYPES::iterator it;
		it = MimeTypes.find(szFileExt);
		if(it != MimeTypes.end())
			szContentType = (*it).second;
	}

	//
	// Obtain current GMT date/time
	//
	char szDT[128];
	struct tm *newtime;
	time_t ltime;

	time(&ltime);
	newtime = gmtime(&ltime);
	strftime(szDT, 128,
		"%a, %d %b %Y %H:%M:%S GMT", newtime);

	//
	// Read the file
	//
	FILE *f;
	f = fopen((m_HomeDir + szFileName).c_str(), "r+b");
	if(f != NULL)				
	{
		// Retrive file size
		fseek(f, 0, SEEK_END);
		fgetpos(f, &lengthActual);
		fseek(f, 0, SEEK_SET);

		pBuf = new char[lengthActual + 1];

		length = fread(pBuf, 1, lengthActual, f);
		fclose(f);

		//
		// Make Response
		//
		sprintf(pResponseHeader, "HTTP/1.0 %s\r\nDate: %s\r\nServer: %s\r\nAccept-Ranges: bytes\r\nContent-Length: %d\r\nConnection: %s\r\nContent-Type: %s\r\n\r\n",
			szStatusCode.c_str(), szDT, SERVERNAME, (int)length, bKeepAlive ? "Keep-Alive" : "close", szContentType.c_str());
	}
	else
	{
		//
		// In case of file not found	   
		//
		if(szFileName==ERROR501)
		{
			szNotFoundMessage="<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"></head>"
			"<body><h2>Casper Simple Web Server</h2><div>501 - Method Not Implemented</div></body></html>";
		}
		else
		{
		f = fopen((m_HomeDir + ERROR404).c_str(), "r+b");
		if(f != NULL)				
		{
			// Retrive file size
			fseek(f, 0, SEEK_END);
			fgetpos(f, &lengthActual);
			fseek(f, 0, SEEK_SET);
			pBuf = new char[lengthActual + 1];
			length = fread(pBuf, 1, lengthActual, f);
			fclose(f);
			szNotFoundMessage = string(pBuf, length);
			delete pBuf;
			pBuf = NULL;
		}else szNotFoundMessage="<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"></head>"
		"<body><h2>Casper Simple Web Server</h2><div>404 - Not Found</div></body></html>";
		szStatusCode = "404 Resource not found";
		}

		sprintf(pResponseHeader, "HTTP/1.0 %s\r\nContent-Length: %d\r\nContent-Type: text/html\r\nDate: %s\r\nServer: %s\r\n\r\n%s",
			szStatusCode.c_str(), szNotFoundMessage.size(), szDT, SERVERNAME, szNotFoundMessage.c_str());
		bKeepAlive = false;  
	}

	szResponse = string(pResponseHeader);
	if(pBuf)
		szResponse += string(pBuf, length);
	delete pBuf;
	pBuf = NULL;


	return false;
}
