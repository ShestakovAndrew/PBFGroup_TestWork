#pragma once
#include <string>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

class CClient
{
public:
	CClient(std::string clientName, uint16_t serverPort);

	void SetConnectionTimeout(size_t connectionTimeout);
	size_t GetConnectionTimeout();
	void Connect();
	void StartSending();

private:
	void CreateSocket();
	void SetSocketAddress();

	std::string m_clientName;
	uint16_t m_serverPort;
	size_t m_connectionTimeout;

	int m_socketDesc;
	sockaddr_in m_socketAddr;
};