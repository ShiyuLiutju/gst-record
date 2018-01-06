#pragma once

#ifdef WIN32
#include <WinSock2.h>
#include <string>
#include <exception>
#else
#include <sys/types.h>
#include <string>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <exception>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#endif
#include "APTBaseDefine.h"



class NetTcp
{
public:
#ifndef WIN32
    typedef int SOCKET;
    typedef sockaddr SOCKADDR;
#define INVALID_SOCKET 0
#define SOCKET_ERROR -1
#endif
	NetTcp();
	~NetTcp();
	void static Init();
	void listen(char* ip, short port);
	NetTcp* Accept();
	void ConnectTo(char* ip,short port);
	int Write(void* buff,size_t len);
    int Read(void* buff, size_t len);
	void close();
	std::string IP_Address;
	short Port;
private:
	NetTcp(SOCKET fd,sockaddr_in& addr);
	NetTcp(const NetTcp&);
	NetTcp& operator =(const NetTcp&);
	void bind(char *ip,short port);
	SOCKET socket__;
	
};

