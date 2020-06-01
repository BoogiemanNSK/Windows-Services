#include "pch.h"
#include "CClient.h"

bool CClient::MyInit()
{
	serviceSocket = new CSocket();

	if (!serviceSocket->MyInit(SERVER_NAME)) 
		printf("Error while creating socket\n");
	
	return true;
}

void CClient::ConnectToRemoteCmd() const
{
	if (serviceSocket->OpenConnection() != 0) 
	{
		printf("Error while opening connection\n");
		exit(-1);
	}

	// Send some test data
	serviceSocket->SendData("Hello, this is a test by Peter.\n");
	Sleep(1000);
	
	// Receive some test data
	auto received = serviceSocket->RecvData();
	printf("$T %s\n", received);
	Sleep(1000);

	// Send dir and receive everything from child
	serviceSocket->SendData("dir\n");
	while(received) {
		Sleep(1000);
		received = serviceSocket->RecvData();
		printf("$W %s\n", received);
	}
	
	serviceSocket->CloseConnection();
}
