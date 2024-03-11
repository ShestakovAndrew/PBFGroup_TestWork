#include <iostream>

#include <cstring>
#include <netdb.h>
#include <stdexcept>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>

#include "CServer.h"

CServer::CServer(uint16_t serverPort)
	: m_serverPort(serverPort)
{
	CreateSocket();
	PrepareSocketStructure();
	BindSocket();
}

void CServer::CreateSocket()
{
	m_socketDesc = socket(AF_INET, SOCK_STREAM, 0);
	if (m_socketDesc == -1)
	{
		throw std::logic_error("Could not create socket");
	}
}

void CServer::PrepareSocketStructure()
{
	m_socketAddr.sin_family = AF_INET;
	m_socketAddr.sin_addr.s_addr = INADDR_ANY;
	m_socketAddr.sin_port = htons(m_serverPort);
}

void CServer::BindSocket()
{
	if (bind(m_socketDesc, (sockaddr*)&m_socketAddr, sizeof(m_socketAddr)) == -1)
	{
		throw std::logic_error("bind failed. Error");
	}
}

void CServer::StartListen()
{
	if (listen(m_socketDesc, SOMAXCONN) == -1)
	{
		throw std::logic_error("Listen failed.");
	}

	sockaddr_in client;
	socklen_t clientSize = sizeof(client);

	char host[NI_MAXHOST];
	char svc[NI_MAXSERV];

	int clientSocket = accept(m_socketDesc, (sockaddr*)&client, &clientSize);
	if (clientSocket == -1)
	{
		throw std::logic_error("Problem with client connection.");
	}

	close(m_socketDesc);

	memset(host, 0, NI_MAXHOST);
	memset(svc, 0, NI_MAXSERV);

	int result = getnameinfo(
		(sockaddr*)&client,
		sizeof(client),
		host,
		NI_MAXHOST,
		svc,
		NI_MAXSERV,
		0
	);

	if (result)
	{
		std::cout << host << " connected on " << svc << std::endl;
	}
	else
	{
		inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
		std::cout << host << " connected on " << ntohs(client.sin_port) << std::endl;
	}

	char buf[4096];
	while (true)
	{
		memset(buf, 0, 4096);

		int bytesRecv = recv(clientSocket, buf, 4096, 0);
		if (bytesRecv == -1)
		{
			throw std::logic_error("There was a connection issue.");
		}

		if (bytesRecv == 0)
		{
			throw std::logic_error("The client disconnected.");
		}

		std::cout << "Received: " << std::string(buf, 0, bytesRecv) << std::endl;

		send(clientSocket, buf, bytesRecv + 1, 0);
	}

	close(clientSocket);
}