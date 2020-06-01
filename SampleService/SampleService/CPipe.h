#pragma once
#include "pch.h"

#include "CLogger.h"

constexpr int BUFSIZE = 4096;
#define SESSION_ID 1 //// IMORTANT PART - UNIQUE FOR SESSION ////

class CPipe {
	HANDLE hThread = nullptr;
	HANDLE hThreadStartedEvent = nullptr;
	HANDLE hStopThreadEvent = nullptr;
	
public:
	int CreatePipedCmd();
	bool MyInit();
	void MyDone();
	void WriteToPipe(LPCVOID buffer); 
	LPCVOID ReadFromPipe(); 

private:
	CHAR ReadBuffer[BUFSIZE];
	CLogger *logger;
	
};
