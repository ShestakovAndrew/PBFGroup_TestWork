#include <iomanip>
#include <iostream>
#include <chrono>

#include <arpa/inet.h>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>
#include <semaphore.h>

#include "CClient.h"

CClient::CClient(std::string clientName, uint16_t serverPort)
	: m_clientName(clientName), m_serverPort(serverPort), m_connectionTimeout(0)
{
	CreateSocket();
	SetSocketAddress();
}

void CClient::SetConnectionTimeout(size_t connectionTimeout)
{
	if (m_connectionTimeout != connectionTimeout)
	{
		m_connectionTimeout = connectionTimeout;
	}
}

size_t CClient::GetConnectionTimeout()
{
	return m_connectionTimeout;
}

void CClient::CreateSocket()
{
	m_socketDesc = socket(AF_INET, SOCK_STREAM, 0);
	if (m_socketDesc == -1)
	{
		throw std::logic_error("Error create socket.");
	}
}

void CClient::SetSocketAddress()
{
	m_socketAddr.sin_family = AF_INET;
	m_socketAddr.sin_port = htons(m_serverPort);
	inet_pton(AF_INET, "127.0.0.1", &m_socketAddr.sin_addr);
}

void CClient::Connect()
{
	int connectRes = connect(m_socketDesc, (sockaddr*)&m_socketAddr, sizeof(m_socketAddr));
	if (connectRes == -1)
	{
		throw std::logic_error("Error connect to server.");
	}
}

void CClient::StartSending()
{
	auto now = std::chrono::system_clock::now();
	auto time = std::chrono::system_clock::to_time_t(now);

	std::stringstream ss;
	ss << std::put_time(std::localtime(&time), "[%Y %M %D %H h %M m %S s] ") << m_clientName << std::endl;
	std::string message = ss.str();

	do
	{
		int sendRes = send(m_socketDesc, message.c_str(), message.size() + 1, 0);
		if (sendRes == -1)
		{
			std::cout << "Error send to server.";
		}

	} while (true);

	close(m_socketDesc);
}