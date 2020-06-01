#include "pch.h"
#include "CSocket.h"

int __cdecl CSocket::OpenConnection() 
{
    // Attempt to connect to an address until one succeeds
    for(ptr = result; ptr != nullptr; ptr = ptr->ai_next) 
    {
        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) 
        {
            printf("Socket creation failed with error: %d\n", WSAGetLastError());
        	MyDone();
            return 1;
        }

        // Connect to server
        iResult = connect(ConnectSocket, ptr->ai_addr, int(ptr->ai_addrlen));
        if (iResult == SOCKET_ERROR) 
        {
        	ConnectSocket = INVALID_SOCKET;
            continue;
        }
    	
        break;
    }

	if (ConnectSocket == INVALID_SOCKET) {
        printf("Connection to server failed with error: %d\n", WSAGetLastError());
        MyDone();
        return 1;
    }
	
    freeaddrinfo(result);
    return 0;
}

bool CSocket::MyInit(const char* serverName)
{
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
    	MyDone();
        return false;
    }

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(serverName, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
    	MyDone();
        return false;
    }
	
	return true;
}

int CSocket::SendData(const char* buffer)
{
	iResult = send( ConnectSocket, buffer, int(strlen(buffer)), 0 );
    if (iResult == SOCKET_ERROR) {
        printf("Send failed with error: %d\n", WSAGetLastError());
        MyDone();
        return 1;
    }
	return 0;
}

const char* CSocket::RecvData()
{
	iResult = recv( ConnectSocket, recvbuf, DEFAULT_BUFLEN, 0 );
    if ( iResult <= 0 ) 
    {
        printf("Recv failed with error: %d\n", WSAGetLastError());
    	MyDone();
    }
	return recvbuf;
}

void CSocket::CloseConnection()
{
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("Shutdown failed with error: %d\n", WSAGetLastError());
    	MyDone();
    }
	WSACleanup();
}

void CSocket::MyDone() const {
	delete &hints;
	if (ConnectSocket != INVALID_SOCKET)
		closesocket(ConnectSocket);
	WSACleanup();
}
