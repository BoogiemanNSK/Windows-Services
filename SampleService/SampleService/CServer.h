#pragma once
#include "pch.h"

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

class CServer
{
	
public:
	bool Send(const char* buffer);
	const char* Recv();
	bool MyInit();
	void MyDone();
	int __cdecl AwaitConnection(void);

private:
	WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
	
};