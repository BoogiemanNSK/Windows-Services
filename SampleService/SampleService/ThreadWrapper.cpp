#include "pch.h"
#include "ThreadWrapper.h"

// Constructor
ThreadWrapper::ThreadWrapper() {
	DWORD tid;
	
	// TODO CreateEvent();
	::CreateThread(nullptr, 0, ThreadWrapperStartRoutine, this, 0, &tid);

	// TODO WaitForMultipleObject(hThread, hThreadStartedEvent, 10sec);
	// TODO if timeout - TerminateThread
}

// Destructor
ThreadWrapper::~ThreadWrapper() {
	// TODO Close handle	
}

DWORD WINAPI ThreadWrapper::ThreadWrapperStartRoutine( LPVOID param ) {
	auto p = static_cast<ThreadWrapper*>(param);
	return p->ThreadInternalWorker();
}

// Initialize thread, set event and return its working function
DWORD ThreadWrapper::ThreadInternalWorker() {
	if (!ThreadInit())
        return -1;
    
    SetEvent(hThreadStartedEvent);
	return ThreadWorker();
}

// Fabric Pattern
ThreadWrapper* ThreadWrapper::CreateThread() {
	auto p = new ThreadWrapper();

	if (p->bThreadStatus) {
		return p;
	}

	delete p;
	return nullptr;
}

// "Soft"/"Hard" methods to destroy thread
bool ThreadWrapper::StopThread(bool bTerminate) {
	// TODO SetEvent(hThreadStopEvent)
	// TODO TerminateThread
	return false;
}

// Initialize thread function
bool ThreadWrapper::ThreadInit() {
	// TODO
	return false;
}

// Thread main working function
DWORD ThreadWrapper::ThreadWorker() {
	// TODO
	return false;
}
