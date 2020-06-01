#include "pch.h"
#include "CServer.h"

bool CServer::MyInit() {
	// Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0)
        return false;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(nullptr, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        WSACleanup();
        return false;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        freeaddrinfo(result);
        WSACleanup();
        return false;
    }

    // Setup the TCP listening socket
    iResult = bind( ListenSocket, result->ai_addr, int(result->ai_addrlen));
    if (iResult == SOCKET_ERROR) {
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return false;
    }

    freeaddrinfo(result);
	return true;
}

int __cdecl CServer::AwaitConnection(void) 
{
    // Listen for incoming connections on default port
    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Accept a client socket
    ClientSocket = accept(ListenSocket, nullptr, nullptr);
    if (ClientSocket == INVALID_SOCKET) {
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // No longer need server socket
    closesocket(ListenSocket);
    return 0;
}

const char* CServer::Recv()
{
	do {
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult <= 0) 
        {
            closesocket(ClientSocket);
            WSACleanup();
            return nullptr;
        }
	} while (iResult > 0);
	return recvbuf;
}

bool CServer::Send(const char* buffer)
{
	iSendResult = send(ClientSocket, buffer, iResult, 0);
    if (iSendResult == SOCKET_ERROR) {
        closesocket(ClientSocket);
        WSACleanup();
        return false;
    }
	return true;
}

void CServer::MyDone()
{
	// Shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        closesocket(ClientSocket);
        WSACleanup();
        return;
    }

    // Cleanup
    closesocket(ClientSocket);
    WSACleanup();
}
