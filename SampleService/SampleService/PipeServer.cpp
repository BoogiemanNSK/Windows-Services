#include "pch.h"
#include "PipeServer.h"
#include "ServiceWrapper.h"

#define BUFSIZE 512

DWORD WINAPI instanceThread(LPVOID);
DWORD WINAPI ThreadProc(_In_ LPVOID lpParameter);
VOID getAnswerToRequest(LPTSTR, LPTSTR, LPDWORD);

int PipeServer::RunServer() {
	BOOL   fConnected = FALSE; 
	DWORD  dwThreadId = 0; 
	HANDLE hPipe = INVALID_HANDLE_VALUE, hThread = nullptr;
	const LPCTSTR lpszPipename = TEXT("\\\\.\\pipe\\mynamedpipe"); 
 
	// The main loop creates an instance of the named pipe and 
	// then waits for a PipeClient to connect to it. When the PipeClient 
	// connects, a thread is created to handle communications 
	// with that PipeClient, and this loop is free to wait for the
	// next PipeClient connect request. It is an infinite loop.
 
	for (;;) 
	{ 
		_tprintf( TEXT("\nPipe Server: Main thread awaiting PipeClient connection on %s\n"), lpszPipename);
		hPipe = CreateNamedPipe( 
			lpszPipename,             // pipe name 
			PIPE_ACCESS_DUPLEX,       // read/write access 
			PIPE_TYPE_MESSAGE |       // message type pipe 
			PIPE_READMODE_MESSAGE |   // message-read mode 
			PIPE_WAIT,                // blocking mode 
			PIPE_UNLIMITED_INSTANCES, // max. instances  
			BUFSIZE,                  // output buffer size 
			BUFSIZE,                  // input buffer size 
			0,                        // PipeClient time-out 
			nullptr);                    // default security attribute 

		if (hPipe == INVALID_HANDLE_VALUE) 
		{
			_tprintf(TEXT("CreateNamedPipe failed, GLE=%d.\n"), GetLastError()); 
			return -1;
		}

		// Wait for the PipeClient to connect; if it succeeds, 
		// the function returns a nonzero value. If the function
		// returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 

		fConnected = ConnectNamedPipe(hPipe, NULL) ? 
            TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 

		if (fConnected) 
		{ 
			printf("Client connected, creating a processing thread.\n"); 

			// Create a thread for this PipeClient. 
			hThread = CreateThread( 
				NULL,              // no security attribute 
				0,                 // default stack size 
				instanceThread,    // thread proc
				static_cast<LPVOID>(hPipe),    // thread parameter 
				0,                 // not suspended 
				&dwThreadId);      // returns thread ID 

			if (hThread == NULL) {
				_tprintf(TEXT("CreateThread failed, GLE=%d.\n"), GetLastError()); 
				return -1;
			} else CloseHandle(hThread); 
		} else CloseHandle(hPipe); 
	} 

	return 0;
}

// This routine is a thread processing function to read from and reply to a PipeClient
// via the open pipe connection passed from the main loop. Note this allows
// the main loop to continue executing, potentially creating more threads of
// of this procedure to run concurrently, depending on the number of incoming
// PipeClient connections.
DWORD WINAPI instanceThread(LPVOID lpvParam) { 
	HANDLE hHeap = GetProcessHeap();
	const auto pchRequest = static_cast<TCHAR*>(HeapAlloc(hHeap, 0, BUFSIZE * sizeof(TCHAR)));
	const auto pchReply   = static_cast<TCHAR*>(HeapAlloc(hHeap, 0, BUFSIZE * sizeof(TCHAR)));

	DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0; 
	BOOL fSuccess = FALSE;
	HANDLE hPipe  = nullptr;

	// Do some extra error checking since the app will keep running even if this
	// thread fails.

	if (lpvParam == nullptr) {
		printf( "\nERROR - Pipe Server Failure:\n");
		printf( "   InstanceThread got an unexpected NULL value in lpvParam.\n");
		printf( "   InstanceThread exitting.\n");
		if (pchReply != nullptr) HeapFree(hHeap, 0, pchReply);
		if (pchRequest != nullptr) HeapFree(hHeap, 0, pchRequest);
		return static_cast<DWORD>(-1);
	}

	if (pchRequest == nullptr) {
		printf( "\nERROR - Pipe Server Failure:\n");
		printf( "   InstanceThread got an unexpected NULL heap allocation.\n");
		printf( "   InstanceThread exitting.\n");
		if (pchReply != nullptr) HeapFree(hHeap, 0, pchReply);
		return static_cast<DWORD>(-1);
	}

	if (pchReply == nullptr) {
		printf( "\nERROR - Pipe Server Failure:\n");
		printf( "   InstanceThread got an unexpected NULL heap allocation.\n");
		printf( "   InstanceThread exitting.\n");
		if (pchRequest != nullptr) HeapFree(hHeap, 0, pchRequest);
		return static_cast<DWORD>(-1);
	}

	// Print verbose messages. In production code, this should be for debugging only.
	printf("InstanceThread created, receiving and processing messages.\n");

	// The thread's parameter is a handle to a pipe object instance. 

	hPipe = static_cast<HANDLE>(lpvParam); 

	// Loop until done reading
	while (true) { 
		// Read PipeClient requests from the pipe. This simplistic code only allows messages
		// up to BUFSIZE characters in length.
		fSuccess = ReadFile( 
			hPipe,					// handle to pipe 
			pchRequest,		// buffer to receive data 
			BUFSIZE * sizeof(TCHAR), // size of buffer 
			&cbBytesRead, // number of bytes read 
			nullptr);        // not overlapped I/O 

		if (!fSuccess || cbBytesRead == 0) {   
			if (GetLastError() == ERROR_BROKEN_PIPE) {
			  _tprintf(TEXT("InstanceThread: PipeClient disconnected.\n")); 
			} else {
			  _tprintf(TEXT("InstanceThread ReadFile failed, GLE=%d.\n"), GetLastError()); 
			}
			break;
		}

		// Process the incoming message.
		getAnswerToRequest(pchRequest, pchReply, &cbReplyBytes); 

		// Write the reply to the pipe. 
		fSuccess = WriteFile( 
			hPipe,        // handle to pipe 
			pchReply,     // buffer to write from 
			cbReplyBytes, // number of bytes to write 
			&cbWritten,   // number of bytes written 
			NULL);        // not overlapped I/O 

		if (!fSuccess || cbReplyBytes != cbWritten) {   
			_tprintf(TEXT("InstanceThread WriteFile failed, GLE=%d.\n"), GetLastError()); 
			break;
		}
	}

	// Flush the pipe to allow the PipeClient to read the pipe's contents 
	// before disconnecting. Then disconnect the pipe, and close the 
	// handle to this pipe instance. 

	FlushFileBuffers(hPipe); 
	DisconnectNamedPipe(hPipe); 
	CloseHandle(hPipe); 

	HeapFree(hHeap, 0, pchRequest);
	HeapFree(hHeap, 0, pchReply);

	printf("InstanceThread exiting.\n");
	return 1;
}

// This routine is a simple function to print the PipeClient request to the console
// and populate the reply buffer with a default data string. This is where you
// would put the actual PipeClient request processing code that runs in the context
// of an instance thread. Keep in mind the main thread will continue to wait for
// and receive other PipeClient connections while the instance thread is working.
VOID getAnswerToRequest(LPTSTR pchRequest, LPTSTR pchReply, LPDWORD pchBytes) {
    _tprintf( TEXT("Client Request String:\"%s\"\n"), pchRequest );

    // Check the outgoing message to make sure it's not too long for the buffer.
    if (FAILED(StringCchCopy( pchReply, BUFSIZE, TEXT("default answer from PipeServer") ))) {
        *pchBytes = 0;
        pchReply[0] = 0;
        printf("StringCchCopy failed, no outgoing message.\n");
        return;
    }
    *pchBytes = (lstrlen(pchReply)+1)*sizeof(TCHAR);
}

DWORD WINAPI ThreadProc( _In_ LPVOID lpParameter ) {
	const auto hEvent = static_cast<HANDLE>(lpParameter);
	SetEvent(hEvent);
	
	auto serviceObj = new ServiceWrapper();
	serviceObj->RunService(0, nullptr);
	
	return 0;
}

bool PipeServer::MyInit() {
	DWORD tid;
	
	hThreadStartedEvent = CreateEvent(nullptr, true, false, nullptr);
	if (nullptr == hThreadStartedEvent) {
		// TODO Error
		CloseHandle(hThreadStartedEvent);
		return false;
	}
	
	hThread = CreateThread(nullptr, 0, ThreadProc, hThreadStartedEvent, 0, &tid);
	if (nullptr == hThread) {
		// TODO Error
		CloseHandle(hThread);
		return false;
	}

	HANDLE handles[2] = {hThreadStartedEvent, hThread};
	const auto res = WaitForMultipleObjects(2, handles, false, 60 * 1000);

	switch (res) {
		case 0:
			return true;
		case 1:
			return false;
		case WAIT_TIMEOUT:
			// TODO Error / Cleanup
			return false;
		default:
			break;
	}
	
	return true;
}

void PipeServer::MyDone() {
	// Check if handles were initialized
	SetEvent(hStopThreadEvent);
	const auto res = WaitForSingleObject(hThread, 60 * 1000);

	switch (res) {
		case 0:
			return;
		default:
			// TODO Error / Cleanup
			TerminateThread(hThread, 0);
	}
}