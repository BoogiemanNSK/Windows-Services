#pragma once
#include "pch.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define WIN32_LEAN_AND_MEAN
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

class CSocket
{
	
public:
	bool MyInit(const char* serverName);
	void MyDone() const;
	int __cdecl OpenConnection();
	int SendData(const char* buffer);
	const char* RecvData();
	void CloseConnection();

private:
	WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = nullptr, *ptr = nullptr, hints;
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
	
};
