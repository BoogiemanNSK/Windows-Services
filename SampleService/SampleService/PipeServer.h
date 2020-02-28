#pragma once
#include "pch.h"

class PipeServer {
	
	HANDLE hThread = nullptr;
	HANDLE hThreadStartedEvent = nullptr;
	HANDLE hStopThreadEvent = nullptr;
	
public:
	int RunServer();
	bool MyInit();
	void MyDone();
	
};
