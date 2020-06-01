#pragma once
#include "pch.h"

class CThread {
	
	HANDLE hThread;
	HANDLE hThreadStartedEvent;
	HANDLE hThreadStopEvent;

	static DWORD WINAPI ThreadWrapperStartRoutine( LPVOID param );
	DWORD ThreadInternalWorker();
	~CThread();

public:
	bool bThreadStatus; // if false - everything is bad
	CThread();
	bool StopThread( bool bTerminate );
	virtual CThread* CreateThread();

protected:
	virtual bool ThreadInit();
	virtual DWORD ThreadWorker();
	
};