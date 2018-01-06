#include "NetTcp.h"

NetTcp::NetTcp()
{
	socket__ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (socket__ <0)
	{
        //LOG(INFO)<<"Can not create new socket";
		throw NetException("Can not create new socket");
	}
	int socket_buf_size=3700000;
	int ret=setsockopt(socket__,SOL_SOCKET,SO_SNDBUF,(char*)&socket_buf_size,sizeof(socket_buf_size));
	if(ret<0)
	{
		printf("set socket error\n");
	}
    //LOG(ERROR)<<"socket:"<<socket__;
}
NetTcp::NetTcp(SOCKET fd, sockaddr_in& addr)
{
	socket__ = fd;
	this->IP_Address = inet_ntoa(addr.sin_addr);
	this->Port = ntohl(addr.sin_port);
}

NetTcp::~NetTcp()
{
	close();
}

void NetTcp::Init()
{
#ifdef WIN32
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	WSAStartup(sockVersion, &wsaData);
#endif
}


void NetTcp::bind(char *ip, short port)
{
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	if (!ip)
#ifndef WIN32
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
    else if(strcmp(ip,"127.0.0.1")==0)
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
    else
        sin.sin_addr.s_addr = inet_addr(ip);
    if (::bind(socket__, (sockaddr*)&sin, sizeof(sin)))
#else
sin.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	else
		sin.sin_addr.S_un.S_addr = inet_addr(ip);
	if (::bind(socket__, (LPSOCKADDR)&sin, sizeof(sin))==SOCKET_ERROR)
#endif
	{
		throw NetException("bind errror");
	}
    if (::listen(socket__, 5))
	{
		throw NetException("listen error");
	}
}
void NetTcp::listen(char* ip, short port)
{
	bind(ip, port);
}
NetTcp* NetTcp::Accept()
{
	SOCKET sClient;
	sockaddr_in remote_addr;
#ifdef WIN32
    int addr_len = sizeof(remote_addr);
#else
    unsigned int addr_len = sizeof(remote_addr);
#endif
    sClient = accept(socket__, (SOCKADDR*)&remote_addr, &addr_len);
	if (sClient == INVALID_SOCKET)
	{
		return NULL;
	}
	else
	{
		NetTcp* new_socket = new NetTcp(sClient, remote_addr);
		return new_socket;
	}
}
void NetTcp::ConnectTo(char* ip,short Port)
{
    int nNetTimeout= 20000;
    setsockopt(socket__,SOL_SOCKET, SO_RCVTIMEO, (char*)&(nNetTimeout),sizeof(int));
	sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(Port);
#ifndef WIN32
	serAddr.sin_addr.s_addr = inet_addr(ip);
#else
	serAddr.sin_addr.S_un.S_addr = inet_addr(ip);
#endif
	if (connect(socket__, (sockaddr*)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
	{
        //LOG(ERROR)<<"connect error";
		throw NetException("connect error");
	}
}
int NetTcp::Write(void* buff, size_t len)
{
	int err=send(socket__, (char*)buff, len,0);
	if (err == SOCKET_ERROR)
	{
        //LOG(ERROR)<<"SOCKET_ERROR:send error";
       #ifdef WIN32
        //LOG(ERROR)<<"WSALastError:"<<WSAGetLastError();
        #endif
	}
	else
	{
		return err;
	}
}
int NetTcp::Read(void* buff, size_t len)
{
	int err = recv(socket__, (char*)buff, len, 0);
	if (err == SOCKET_ERROR)
    {
       // //LOG(ERROR)<<"SOCKET_ERROR:recv error";
       // //LOG(ERROR)<<"SOCKET_ERROR CODE:"+err;
       // //LOG(ERROR)<<"socket:"<<socket__;
#ifdef WIN32
        ////LOG(ERROR)<<"WSALastError:"<<WSAGetLastError();
#endif
		return err;
	}
	else
	{
		return err;
	}
}

void NetTcp::close()
{    
	if (socket__ != INVALID_SOCKET)
	{
#ifndef WIN32
        ::close(socket__);
#else
	closesocket(socket__);
#endif
		socket__ = INVALID_SOCKET;
	}
}
