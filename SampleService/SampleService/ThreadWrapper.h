	#pragma once
#include "pch.h"

class ThreadWrapper {
	
	HANDLE hThread;
	HANDLE hThreadStartedEvent;
	HANDLE hThreadStopEvent;
	// HANDLE hGlobalStopEvent; // use g_hGlobalStopEvent

	static DWORD WINAPI ThreadWrapperStartRoutine( LPVOID param );
	DWORD ThreadInternalWorker();
	~ThreadWrapper();

public:
	bool bThreadStatus; // if false - everything is bad
	
	ThreadWrapper();
	static ThreadWrapper* CreateThread();
	bool StopThread( bool bTerminate );

protected:
	virtual bool ThreadInit();
	virtual DWORD ThreadWorker();
	// TODO Consider to work for static polymorphism (aka templates)
	
};