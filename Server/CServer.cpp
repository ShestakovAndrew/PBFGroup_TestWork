#include <iostream>
#include <cstring>

#include "CServer.h"

CServer::CServer(uint16_t serverPort)
	: m_serverPort(serverPort)
{
}

CServer::~CServer()
{
	Shutdown();
}

int CServer::Start() noexcept
{
	addrinfo* serverAddr = GetServerLocalAddress();
	if (!serverAddr) return -1;

	m_serverSocket = CreateServerSocket(serverAddr);
	CHECK(m_serverSocket, "Error create socket");
	freeaddrinfo(serverAddr);

	FD_ZERO(&m_pollingSocketSet);
	FD_SET(m_serverSocket, &m_pollingSocketSet);
	m_maxSocket = m_serverSocket;

	return HandleConnections();
}

void CServer::Shutdown() noexcept
{
	close(m_serverSocket);
}

SOCKET CServer::CreateServerSocket(addrinfo* bindAddress) noexcept
{
	if (!bindAddress) return -1;

	SOCKET serverSocket = socket(
		bindAddress->ai_family, 
		bindAddress->ai_socktype, 
		bindAddress->ai_protocol
	);

	CHECK(serverSocket, "Failed to create server socket.");

	CHECK(
		bind(serverSocket, bindAddress->ai_addr, bindAddress->ai_addrlen),
		"Failed to bind server socket to address."
	);

	CHECK(ConfigureServerSocket(serverSocket), "Error configure server socket.");

	return serverSocket;
}

int CServer::ConfigureServerSocket(SOCKET serverSocket) noexcept
{
	int settingVariable = 1;

	CHECK(
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &settingVariable, sizeof(settingVariable)),
		"Failed to set socket options."
	);

	CHECK(listen(serverSocket, SOMAXCONN), "Failed to activate socket listener.");

	return 0;
}

addrinfo* CServer::GetServerLocalAddress() noexcept
{
	addrinfo hints, *bindAddress;
	memset(&hints, 0x00, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo("127.0.0.1", std::to_string(m_serverPort).c_str(), &hints, &bindAddress) != 0)
	{
		std::cout << "Failed to resolve server's local address." << std::endl;
		return nullptr;
	}

	return bindAddress;
}

int CServer::AcceptConnection() noexcept
{
	sockaddr_storage connectionAddress;
	socklen_t connectionLength = sizeof(connectionAddress);

	SOCKET newConnection = accept(m_serverSocket, reinterpret_cast<sockaddr*>(&connectionAddress), &connectionLength);
	ConnectionInfo newConnectionInfo = GetConnectionInfo(&connectionAddress);
	
	CHECK(newConnection, "Failed to accept new connection.");

	if (newConnection > m_maxSocket) 
	{
		m_maxSocket = newConnection;
	}

	FD_SET(newConnection, &m_pollingSocketSet);
	m_connectedClients[newConnection] = newConnectionInfo;

	return 0;
}

void CServer::DisconnectClient(SOCKET socket) noexcept
{
	close(socket);
	m_connectedClients.erase(socket);
	FD_CLR(socket, &m_pollingSocketSet);
}

int CServer::HandleConnections() noexcept
{
	while (true) 
	{
		fd_set pollingSetCopy = m_pollingSocketSet;

		CHECK(
			select(m_maxSocket + 1, &pollingSetCopy, NULL, NULL, NULL),
			"Failed to fetch data on the server socket."
		);

		for (SOCKET socket = 1; socket <= m_maxSocket + 1; ++socket)
		{
			if (FD_ISSET(socket, &pollingSetCopy))
			{
				if (socket == m_serverSocket)
				{
					CHECK(AcceptConnection(), "Error accept connection");
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
					BroadcastMessage(m_connectedClients.at(socket), msgStr);
				}
			}
		}
	}
	return 0;
}

void CServer::PrependMessageLength(std::string& message) noexcept
{
	std::string messageSizeStr = std::to_string(message.size());
	while (messageSizeStr.size() < 4)
	{
		messageSizeStr = "0" + messageSizeStr;
	}
	message = messageSizeStr + message;
}


CServer::ConnectionInfo CServer::GetConnectionInfo(sockaddr_storage* addr) noexcept
{
	sockaddr_in* connectionAddress = reinterpret_cast<sockaddr_in*>(addr);

	ConnectionInfo retConn;

	retConn.isSuccessConnection = false;
	char ipAddress[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &connectionAddress->sin_addr, ipAddress, INET_ADDRSTRLEN);
	retConn.address = std::string(ipAddress);
	retConn.port = std::to_string(connectionAddress->sin_port);
	retConn.isSuccessConnection = true;

	return retConn;
}

int CServer::SendMessage(SOCKET recipientSocket, std::string const& message) noexcept
{
	std::string assembledMsg(message);
	PrependMessageLength(assembledMsg);

	size_t totalBytes = assembledMsg.size();
	size_t sentBytes = 0;
	size_t sentN;

	ConnectionInfo const& conn_info = m_connectedClients.at(recipientSocket);
	while (sentBytes < totalBytes)
	{
		sentN = send(recipientSocket, assembledMsg.data() + sentBytes, totalBytes - sentBytes, 0);
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
