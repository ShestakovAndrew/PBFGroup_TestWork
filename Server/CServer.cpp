#include <iostream>
#include <cstring>

#include "CServer.h"

CServer::CServer(uint16_t serverPort)
	: m_serverPort(serverPort)
{
}

CServer::~CServer()
{

}

int CServer::Start()
{
	addrinfo* serverAddr = GetServerLocalAddress();
	CreateServerSocket(serverAddr);
	freeaddrinfo(serverAddr);

	FD_ZERO(&m_pollingSocketSet);
	FD_SET(m_serverSocket, &m_pollingSocketSet);
	m_maxSocket = m_serverSocket;

	return HandleConnections();
}

void CServer::Shutdown()
{
	close(m_serverSocket);
}

void CServer::CreateServerSocket(addrinfo* bindAddress)
{
	if (!bindAddress)
	{
		throw std::logic_error("bindAddress is NULL.");
	}

	m_serverSocket = socket(
		bindAddress->ai_family, 
		bindAddress->ai_socktype, 
		bindAddress->ai_protocol
	);

	if (m_serverSocket == -1)
	{
		throw std::logic_error("Failed to create server socket.");
	}

	if (bind(m_serverSocket, bindAddress->ai_addr, bindAddress->ai_addrlen) == -1)
	{
		throw std::logic_error("Failed to bind server socket to address.");
	}

	ConfigureServerSocket();
}

void CServer::ConfigureServerSocket()
{
	int yes = 1;

	if (setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
	{
		throw std::logic_error("Failed to set socket options.");
	}

	if (listen(m_serverSocket, SOMAXCONN) == -1)
	{
		throw std::logic_error("Failed to activate socket listener.");
	}
}

addrinfo* CServer::GetServerLocalAddress()
{
	addrinfo hints, * bindAddress;
	memset(&hints, 0x00, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo("127.0.0.1", std::to_string(m_serverPort).c_str(), &hints, &bindAddress) != 0)
	{
		throw std::logic_error("Failed to resolve server's local address.");
	}

	return bindAddress;
}

void CServer::AcceptConnection()
{
	sockaddr_storage connectionAddress;
	socklen_t connectionLength = sizeof(connectionAddress);

	SOCKET newConnection = accept(m_serverSocket, reinterpret_cast<sockaddr*>(&connectionAddress), &connectionLength);
	ConnectionInfo newConnectionInfo = GetConnectionInfo(&connectionAddress);
	
	if (newConnection == -1) 
	{
		throw std::logic_error("Failed to accept new connection.");
	}

	if (newConnection > m_maxSocket) 
	{
		m_maxSocket = newConnection;
	}

	FD_SET(newConnection, &m_pollingSocketSet);
	m_connectedClients[newConnection] = newConnectionInfo;
}

void CServer::DisconnectClient(SOCKET socket) noexcept
{
	ConnectionInfo conn_info = m_connectedClients.at(socket);
	close(socket);
	m_connectedClients.erase(socket);
	FD_CLR(socket, &m_pollingSocketSet);
}

int CServer::HandleConnections()
{
	while (true) 
	{
		fd_set pollingSetCopy = m_pollingSocketSet;

		if (select(m_maxSocket + 1, &pollingSetCopy, NULL, NULL, NULL) < 0)
		{
			throw std::logic_error("Failed to fetch data on the server socket.");
		}

		for (SOCKET socket = 1; socket <= m_maxSocket + 1; ++socket)
		{
			if (FD_ISSET(socket, &pollingSetCopy))
			{
				if (socket == m_serverSocket)
				{
					AcceptConnection();
				}
				else 
				{
					char msgBuffer[MAX_DATA_BUFFER_SIZE];
					memset(msgBuffer, 0x00, MAX_DATA_BUFFER_SIZE);
					int recvBytes = ReceiveMessage(socket, msgBuffer);

					if (recvBytes <= 0)
					{
						DisconnectClient(socket);
						continue;
					}
					std::string msgStr(msgBuffer);
					BroadcastMessage(msgStr, m_connectedClients.at(socket));

				}
			}
		}
	}
	return 0;
}

int CServer::SendMessage(SOCKET receipientSocket, std::string const& message) noexcept
{
	std::string assembledMsg = message;
	PrependMessageLength(assembledMsg);

	size_t totalBytes = assembledMsg.size();
	size_t sentBytes = 0;
	size_t sentN;

	ConnectionInfo const& conn_info = m_connectedClients.at(receipientSocket);
	while (sentBytes < totalBytes)
	{
		sentN = send(receipientSocket, assembledMsg.data() + sentBytes, totalBytes - sentBytes, 0);
		if (sentN == -1) return sentN;
		sentBytes += sentN;
	}

	return sentBytes;
}

int CServer::BroadcastMessage(ConnectionInfo const& connectionFrom, std::string const& message) noexcept
{
	for (const auto& [socket, clientInfo] : m_connectedClients) 
	{
		if (SendMessage(socket, message) == -1)
		{
			DisconnectClient(socket);
			return -1;
		}
	}
	return 0;
}

int CServer::ReceiveMessage(SOCKET senderSocket, char* writableBuffer) noexcept
{
	if (!writableBuffer) return -1;

	ConnectionInfo const& conn_info = m_connectedClients.at(senderSocket);

	char messageSizeStr[5];
	memset(messageSizeStr, 0x00, sizeof(messageSizeStr));
	messageSizeStr[4] = '\0';

	int recvBytes = recv(senderSocket, messageSizeStr, sizeof(messageSizeStr) - 1, 0);
	if (recvBytes <= 0) return recvBytes;

	for (const char c : std::string(messageSizeStr))
	{
		if (!std::isdigit(c)) return -1;
	}

	int packetLength = std::atoi(messageSizeStr);
	recvBytes = recv(senderSocket, writableBuffer, packetLength, 0);

	return recvBytes;
}
