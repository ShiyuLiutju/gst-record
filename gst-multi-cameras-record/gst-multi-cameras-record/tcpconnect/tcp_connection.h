#pragma once
#include <unistd.h>
#include <arpa/inet.h>
#include <string>

class TcpConnection
{
public:
	TcpConnection();
	~TcpConnection();
	TcpConnection* Accept();
	int ConnectTo(char* ip,short port);
	int Write(void* buff,size_t len);
	int Read(void* buff, size_t len);
	void Close();
	bool Bind(char *ip,short port);
private:
	TcpConnection(int socket,sockaddr_in& addr);
	TcpConnection(const TcpConnection&);
	TcpConnection& operator =(const TcpConnection&);

    int socket_;
	std::string ip_;
	short port_;
	
};

