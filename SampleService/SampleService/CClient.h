#pragma once
#include "pch.h"

#include "CSocket.h"

#define SERVER_NAME "localhost"

class CClient
{
	
public:
	bool MyInit();
	void ConnectToRemoteCmd() const;

private:
	CSocket *serviceSocket;
	
};