#include "pch.h"
#include "CThread.h"

CThread::CThread() {
	DWORD tid;
	::CreateThread(nullptr, 0, ThreadWrapperStartRoutine, this, 0, &tid);
}


// Destructor
CThread::~CThread() {
	CloseHandle(hThread);
	CloseHandle(hThreadStartedEvent);
	CloseHandle(hThreadStopEvent);
}

DWORD WINAPI CThread::ThreadWrapperStartRoutine( LPVOID param ) {
	auto p = static_cast<CThread*>(param);
	return p->ThreadInternalWorker();
}

// Initialize thread, set event and return its working function
DWORD CThread::ThreadInternalWorker() {
	if (!ThreadInit())
        return -1;
    
    SetEvent(hThreadStartedEvent);
	return ThreadWorker();
}

// Fabric Pattern
CThread* CThread::CreateThread() {
	auto p = new CThread();

	if (p->bThreadStatus) {
		return p;
	}

	delete p;
	return nullptr;
}


// "Soft"/"Hard" methods to destroy thread
bool CThread::StopThread(bool bTerminate) {
	SetEvent(hThreadStopEvent);
	return false;
}

// Initialize thread function
bool CThread::ThreadInit() {
	return false;
}

// Thread main working function
DWORD CThread::ThreadWorker() {
	return false;
}
