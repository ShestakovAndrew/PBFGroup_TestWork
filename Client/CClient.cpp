#include <iostream>
#include <chrono>
#include <thread>

#include "date.h"
#include "CClient.h"

CClient::CClient(std::string clientName, uint16_t serverPort, size_t connectionTimeout)
	: m_clientName(clientName), m_serverPort(serverPort), m_connectionTimeout(connectionTimeout)
{
}

CClient::~CClient()
{
	Disconnect();
}

int CClient::Start() noexcept
{
	addrinfo* connectionAddr = ResolveConnectionAddress();
	if (!connectionAddr) return -1;

	m_clientSocket = CreateConnectionSocket(connectionAddr);
	CHECK(m_clientSocket, "Error create connection socket");
	
	CHECK(
		connect(m_clientSocket, connectionAddr->ai_addr, connectionAddr->ai_addrlen), 
		"Error connect to connection address"
	);

	freeaddrinfo(connectionAddr);

	return HandleConnection();
}

void CClient::Disconnect() noexcept
{
	close(m_clientSocket);
}

addrinfo* CClient::ResolveConnectionAddress() noexcept
{
	addrinfo hints, *connectionAddress;
	memset(&hints, 0x00, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo("127.0.0.1", std::to_string(m_serverPort).c_str(), &hints, &connectionAddress) == -1)
	{
		return nullptr;
	}

	return connectionAddress;
}

SOCKET CClient::CreateConnectionSocket(addrinfo* connectionAddress) noexcept
{
	if (!connectionAddress) return -1;

	SOCKET newConnectionSocket = socket(
		connectionAddress->ai_family, 
		connectionAddress->ai_socktype, 
		connectionAddress->ai_protocol
	);

	CHECK(newConnectionSocket, "Error create socket");

	return newConnectionSocket;
}

int CClient::SendMessage(std::string const& message) noexcept
{
	std::string assembledMsg(message);
	PrependMessageLength(assembledMsg);

	size_t totalBytes = assembledMsg.size();
	size_t sentBytes = 0;
	size_t sentN;

	while (totalBytes > sentBytes)
	{
		sentN = send(m_clientSocket, assembledMsg.data() + sentBytes, totalBytes - sentBytes, 0);
		if (sentN == -1) return sentN;
		sentBytes += sentN;
	}

	return sentBytes;
}

int CClient::InputHandler() noexcept
{
	using namespace date;

	while (true)
	{
		sleep(m_connectionTimeout);

		std::ostringstream oss;
		oss << "[" << std::chrono::system_clock::now() << "] " << m_clientName << std::endl;
		if (SendMessage(oss.str()) == -1) std::exit(1);
	}
}

int CClient::HandleConnection() noexcept
{
	std::thread inputWorkerThread(&CClient::InputHandler, this);
	inputWorkerThread.detach();

	while (true) {}
}

void CClient::PrependMessageLength(std::string& message) noexcept
{
	std::string messageSizeStr = std::to_string(message.size());
	while (messageSizeStr.size() < 4)
	{
		messageSizeStr = "0" + messageSizeStr;
	}
	message = messageSizeStr + message;
}