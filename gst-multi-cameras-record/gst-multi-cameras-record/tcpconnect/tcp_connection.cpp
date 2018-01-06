#include "tcp_connection.h"
#include <errno.h>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;
TcpConnection::TcpConnection()
{
	socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	int socket_buf_size = 3700000;
	int ret = setsockopt(socket_, SOL_SOCKET, SO_SNDBUF,
			(char*) &socket_buf_size, sizeof(socket_buf_size));
	if (ret < 0)
	{
		printf("set socket error\n");
	}
	port_ = 0;
}
TcpConnection::TcpConnection(int socket, sockaddr_in& addr)
{
	socket_ = socket;
	ip_ = inet_ntoa(addr.sin_addr);
	port_ = ntohl(addr.sin_port);
}

TcpConnection::~TcpConnection()
{
	Close();
}
bool TcpConnection::Bind(char *ip, short port)
{
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	if (!ip)
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
    else if(strcmp(ip,"127.0.0.1")==0)
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
    else
        sin.sin_addr.s_addr = inet_addr(ip);
    if (bind(socket_, (sockaddr*)&sin, sizeof(sin)))
	{
    	 cout<<"Bind bind failed ";
    	return false;
	}
    if (listen(socket_, 5))
	{
    	  cout<<"Bind listen failed ";
    	return false;
	}
    cout<<"Bind success";
    return true;
}

TcpConnection* TcpConnection::Accept()
{
    int sClient;
	sockaddr_in remote_addr;
    socklen_t addr_len = sizeof(remote_addr);
    sClient = accept(socket_, (sockaddr*)&remote_addr, &addr_len);
    if (sClient <0)
	{
    	cout<<"sClient < 0";
    	return NULL;
	}
	else
	{
		cout<<"Accept";
		TcpConnection* new_socket = new TcpConnection(sClient, remote_addr);
		return new_socket;
	}
}
int  TcpConnection::ConnectTo(char* ip,short Port)
{
    sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons(Port);
    serAddr.sin_addr.s_addr = inet_addr(ip);
    socklen_t len=sizeof(serAddr);

    if (connect(socket_, (sockaddr*)&serAddr, len)<0)
    {
		return -1;
	}

    return 0;
}
int TcpConnection::Write(void* buff, size_t len)
{
    int err=::send(socket_, (char*)buff, len,0);
    return err < 0 ? -1:err;
}
int TcpConnection::Read(void* buff, size_t len)
{
    int err = ::recv(socket_, (char*)buff, len, 0);
    return err < 0 ? -1:err;
}
void TcpConnection::Close()
{
	if (socket_ >= 0)
		close(socket_);
	socket_ = -1;
}
