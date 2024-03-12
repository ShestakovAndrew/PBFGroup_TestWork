#include <iostream>
#include <stdexcept>

#include "CServer.h"

void ValidateCountArguments(int argc)
{
	if (argc != 2)
	{
		throw std::invalid_argument("Error count arguments.");
	}
}

uint16_t GetServerPort(std::string const& serverPortStr)
{
	return static_cast<uint16_t>(std::stoul(serverPortStr));
}

int main(int argc, char* argv[])
{
	try
	{
		ValidateCountArguments(argc);
		uint16_t serverPort = GetServerPort(argv[1]);

		CServer server(serverPort);

		if (server.Start() == -1) 
		{
			std::cout << "Server close." << std::endl;
		}
	}
	catch (std::exception const& error)
	{
		std::cout << error.what() << std::endl;
	}

	return 0;
}