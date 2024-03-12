#pragma once
#include <cstdint>
#include <memory>
#include <unordered_map>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define CHECK(val, msg) if (val == -1) { std::cout << msg << std::endl;	return -1; }

namespace
{
	using SOCKET = int;

	const int MAX_DATA_BUFFER_SIZE = 4096;

	void PrependMessageLength(std::string& message) 
	{
		std::string messageSizeStr = std::to_string(message.size());
		while (messageSizeStr.size() < 4)
		{
			messageSizeStr = "0" + messageSizeStr;
		}
		message = messageSizeStr + message;
	}

	struct ConnectionInfo
	{
		bool success;
		std::string address;
		std::string port;
	};

	inline static ConnectionInfo GetConnectionInfo(sockaddr_storage* addr)
	{
		sockaddr_in* connectionAddress = reinterpret_cast<sockaddr_in*>(addr);

		ConnectionInfo retConn;
		retConn.success = false;

		char ipAddress[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &connectionAddress->sin_addr, ipAddress, INET_ADDRSTRLEN);
		retConn.address = std::string(ipAddress);
		retConn.port = std::to_string(connectionAddress->sin_port);
		retConn.success = true;

		return retConn;
	}
}

class CServer
{
public:
	CServer(uint16_t serverPort);

	CServer(const CServer& other) = delete;
	CServer& operator=(const CServer& other) = delete;

	~CServer();

	int Start() noexcept;
	void Shutdown() noexcept;

private:
	SOCKET  CreateServerSocket(addrinfo* bind_address) noexcept;
	addrinfo* GetServerLocalAddress() noexcept;
	int ConfigureServerSocket(SOCKET serverSocket) noexcept;

	int SendMessage(SOCKET receipientSocket, std::string const& message) noexcept;
	int BroadcastMessage(ConnectionInfo const& connectionFrom, std::string const& message) noexcept;
	int ReceiveMessage(SOCKET senderSocket, char* writableBuffer) noexcept;

	int AcceptConnection() noexcept;
	void DisconnectClient(SOCKET socket) noexcept;
	int HandleConnections() noexcept;

	uint16_t m_serverPort;

	SOCKET m_serverSocket;
	SOCKET m_maxSocket;
	fd_set m_pollingSocketSet;

	std::unordered_map<SOCKET, ConnectionInfo> m_connectedClients;
};