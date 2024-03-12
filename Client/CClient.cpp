#include <iomanip>
#include <iostream>
#include <thread>

#include "CClient.h"

CClient::CClient(std::string clientName, uint16_t serverPort)
	: m_clientName(clientName), m_serverPort(serverPort), m_connectionTimeout(0)
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

void CClient::SetConnectionTimeout(size_t connectionTimeout) noexcept
{
	if (m_connectionTimeout != connectionTimeout)
	{
		m_connectionTimeout = connectionTimeout;
	}
}

size_t CClient::GetConnectionTimeout() noexcept
{
	return m_connectionTimeout;
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

void CClient::PrintInputPrompt() const noexcept
{
	std::cin.clear();
	std::cout << " >>> ";
	std::cout.flush();
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

int CClient::ReceiveMessage(char* writableBuff) noexcept
{
	char msgLengthStr[5];
	memset(msgLengthStr, 0x00, sizeof(msgLengthStr));
	msgLengthStr[4] = '\0';

	int recvBytes = recv(m_clientSocket, msgLengthStr, sizeof(msgLengthStr) - 1, 0);
	if (recvBytes <= 0) return recvBytes;

	for (const char ch : std::string(msgLengthStr))
	{
		if (!std::isdigit(ch)) return -1;
	}

	int packetLength = std::atoi(msgLengthStr);
	recvBytes = recv(m_clientSocket, writableBuff, packetLength, 0);
	if (recvBytes <= 0) return recvBytes;

	return recvBytes;
}

int CClient::InputHandler()
{
	while (true) 
	{
		char msgBuffer[MAX_DATA_BUFFER_SIZE];
		PrintInputPrompt();

		std::fgets(msgBuffer, MAX_DATA_BUFFER_SIZE, stdin);

		std::string message_str(msgBuffer);
		message_str.pop_back();
		if (SendMessage(message_str) == -1) std::exit(1);

		memset(msgBuffer, 0x00, MAX_DATA_BUFFER_SIZE);
	}
}

int CClient::HandleConnection() noexcept
{
	std::thread inputWorkerThread(&CClient::InputHandler, this);
	inputWorkerThread.detach();

	while (true) 
	{
		char msgBuffer[MAX_DATA_BUFFER_SIZE];
		memset(msgBuffer, 0x00, sizeof(msgBuffer));

		int recvBytes = ReceiveMessage(msgBuffer);
		if (recvBytes <= 0) std::exit(1);
		std::cout << msgBuffer << std::endl;

		PrintInputPrompt();
	}
}