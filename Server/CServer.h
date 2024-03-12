#pragma once
#include <unordered_map>
#include <fstream>
#include <cstdint>
#include <memory>

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
}

class CServer
{
public:
	CServer(uint16_t serverPort);

	CServer(const CServer& other) = delete;
	CServer& operator=(const CServer& other) = delete;

	~CServer();

	struct ConnectionInfo
	{
		bool isSuccessConnection;
		std::string address;
		std::string port;
	};

	int Start() noexcept;
	void Shutdown() noexcept;

private:
	SOCKET CreateServerSocket(addrinfo* bind_address) noexcept;
	addrinfo* GetServerLocalAddress() noexcept;
	int ConfigureServerSocket(SOCKET serverSocket) noexcept;

	int ReceiveMessage(SOCKET senderSocket, char* writableBuffer) noexcept;

	int AcceptConnection() noexcept;
	void DisconnectClient(SOCKET socket) noexcept;
	int HandleConnections() noexcept;

	void PrependMessageLength(std::string& message) noexcept;
	ConnectionInfo GetConnectionInfo(sockaddr_storage* addr) noexcept;

	uint16_t m_serverPort;
	std::ofstream m_logFile;
	SOCKET m_serverSocket;
	SOCKET m_maxSocket;
	fd_set m_pollingSocketSet;

	std::unordered_map<SOCKET, ConnectionInfo> m_connectedClients;
};