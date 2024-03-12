#include <cstddef>
#include <iostream>
#include <stdexcept>

#include "CClient.h"

void ValidateCountArguments(int argc)
{
	if (argc != 4)
	{
		throw std::invalid_argument("Error count arguments.");
	}
}

uint16_t GetServerPort(std::string const& serverPortStr)
{
	return static_cast<uint16_t>(std::stoul(serverPortStr));
}

size_t GetConnectionTimeout(std::string const& connectionTimeoutStr)
{
	return static_cast<size_t>(std::stoul(connectionTimeoutStr));
}

int main(int argc, char* argv[])
{
	try
	{
		ValidateCountArguments(argc);

		uint16_t serverPort = GetServerPort(argv[2]);
		size_t connectionTimeout = GetConnectionTimeout(argv[3]);
		std::string clientName(argv[1]);

		CClient client(clientName, serverPort, connectionTimeout);
		if (client.Start() == -1) 
		{
			std::cout << "Client " << clientName << " disconnected." << std::endl;
		}
	}
	catch (std::invalid_argument const& error)
	{
		std::cout << error.what() << std::endl;
	}

	return 0;
}