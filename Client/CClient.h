#pragma once
#include <cstring>

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

class CClient
{
public:
	CClient(std::string clientName, uint16_t serverPort);

	CClient(CClient const& other) = delete;
	CClient& operator=(CClient const& other) = delete;

	~CClient();

	int Start() noexcept;
	void Disconnect() noexcept;

	void SetConnectionTimeout(size_t connectionTimeout) noexcept;
	size_t GetConnectionTimeout() noexcept;

private:
	addrinfo* ResolveConnectionAddress() noexcept;
	SOCKET CreateConnectionSocket(addrinfo* conn_addr) noexcept;
	
	void PrintInputPrompt() const noexcept;

	int SendMessage(const std::string& message) noexcept;
	int ReceiveMessage(char* writable_buff) noexcept;

	int InputHandler();
	int HandleConnection() noexcept;

	void PrependMessageLength(std::string& message) noexcept;

	std::string m_clientName;
	uint16_t m_serverPort;
	size_t m_connectionTimeout;

	SOCKET m_clientSocket;
};