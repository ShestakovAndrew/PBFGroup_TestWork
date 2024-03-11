#pragma once
#include <cstdint>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

class CServer
{
public:
	CServer(uint16_t serverPort);

	void StartListen();

private:
	void CreateSocket();
	void PrepareSocketStructure();
	void BindSocket();

	uint16_t m_serverPort;
	int m_socketDesc;
	sockaddr_in m_socketAddr;
};