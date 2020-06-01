#include "pch.h"
#include "CService.h"
#include "CLogger.h"
#include "CPipe.h"
#include "CServer.h"

#pragma comment(lib, "advapi32.lib")

#define SVCNAME TEXT("AAA_Task1_Service")

CPipe *pipedCmd;
CLogger *logger;
CServer *serverObj;

BOOL bWorking;
SERVICE_STATUS          gSvcStatus; 
SERVICE_STATUS_HANDLE   gSvcStatusHandle; 
HANDLE                  ghSvcStopEvent = nullptr;

DWORD WINAPI PipeWriter( LPVOID params );
DWORD WINAPI PipeReader( LPVOID params );
VOID WINAPI SvcMain( DWORD dwArgc, LPTSTR *lpszArgv );
VOID SvcInit( DWORD, LPTSTR * );
VOID ReportSvcStatus( DWORD, DWORD, DWORD );
VOID WINAPI SvcCtrlHandler( DWORD );


//
// Purpose: 
//   Entry point for the process
//
// Parameters:
//   None
// 
// Return value:
//   None
//
void __cdecl CService::RunService(int argc, TCHAR *argv[]) 
{
	// Initialize working classes
	pipedCmd = new CPipe();
	serverObj = new CServer();
	logger = new CLogger("D:\\Task1.log");
	
    SERVICE_TABLE_ENTRY dispatchTable[] = 
    { 
        { LPTSTR(SVCNAME), (LPSERVICE_MAIN_FUNCTION)SvcMain }, 
        {nullptr, nullptr } 
    }; 

	// This call returns when the CService has stopped. 
    // The process should simply terminate when the call returns.
    logger->Log("Starting Service...");
	if (!StartServiceCtrlDispatcher(dispatchTable))
    	logger->Log("StartServiceCtrlDispatcher - Failed!!!");
}

//
// Purpose: 
//   Entry point for the service
//
// Parameters:
//   dwArgc - Number of arguments in the lpszArgv array
//   lpszArgv - Array of strings. The first string is the name of
//     the service and subsequent strings are passed by the process
//     that called the StartService function to start the service.
// 
// Return value:
//   None.
//
VOID WINAPI SvcMain( DWORD dwArgc, LPTSTR *lpszArgv )
{
    // Register the handler function for the service
    gSvcStatusHandle = RegisterServiceCtrlHandler( 
        SVCNAME, 
        SvcCtrlHandler);

    if( !gSvcStatusHandle )
    { 
        logger->Log("RegisterServiceCtrlHandler - Failed!!!"); 
        return; 
    }
    logger->Log("RegisterServiceCtrlHandler - OK");
	
    // These SERVICE_STATUS members remain as set here
    gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS; 
    gSvcStatus.dwServiceSpecificExitCode = 0;

    // Report initial status to the SCM
    ReportSvcStatus( SERVICE_START_PENDING, NO_ERROR, 3000 );
	
    // Perform service-specific initialization and work.
    SvcInit( dwArgc, lpszArgv );
}


//
// Purpose: 
//   The CService code
//
// Parameters:
//   dwArgc - Number of arguments in the lpszArgv array
//   lpszArgv - Array of strings. The first string is the name of
//     the CService and subsequent strings are passed by the process
//     that called the StartService function to start the CService.
// 
// Return value:
//   None
//
VOID SvcInit( DWORD dwArgc, LPTSTR *lpszArgv )
{
    if (!serverObj->MyInit()) {
    	logger->Log("Server initialization - Failed!!!");
	    ReportSvcStatus( SERVICE_STOP_PENDING, NO_ERROR, 0 );
        return;
    }

	if (!pipedCmd->MyInit()) {
		logger->Log("Piped cmd initialization - Failed!!!");
		ReportSvcStatus( SERVICE_STOP_PENDING, NO_ERROR, 0 );
        return;
	}
	
    // Create an event. The control handler function, SvcCtrlHandler,
    // signals this event when it receives the stop control code.
    ghSvcStopEvent = CreateEvent(
                         nullptr,    // default security attributes
                         TRUE,    // manual reset event
                         FALSE,   // not signaled
                         nullptr);   // no name

    if (ghSvcStopEvent == nullptr)
    {
    	logger->Log("Event creation - Failed!!!");
        ReportSvcStatus( SERVICE_STOPPED, NO_ERROR, 0 );
        return;
    }

    // Report running status when initialization is complete.
    ReportSvcStatus( SERVICE_RUNNING, NO_ERROR, 0 );
	logger->Log("Service Started - OK");

    // Wait for a client to connect
	if (serverObj->AwaitConnection() != 0) 
    {
		logger->Log("Client connection - Failed!!!");
		ReportSvcStatus( SERVICE_STOP_PENDING, NO_ERROR, 0 );
        return;
	}
	logger->Log("Client Connected - OK");
	
    // Create cmd with connected pipes here
	if (pipedCmd->CreatePipedCmd() != 0) 
    {
		logger->Log("Piped cmd creation - Failed!!!");
		ReportSvcStatus( SERVICE_STOP_PENDING, NO_ERROR, 0 );
        return;
	}
	logger->Log("Piped Cmd created - OK");

	bWorking = TRUE;
	DWORD readerId, writerId;
	CreateThread(nullptr, 0, PipeReader, nullptr, 0, &readerId);
	CreateThread(nullptr, 0, PipeWriter, nullptr, 0, &writerId);
	logger->Log("Threads initialized - OK");

    while(true)
    {
        // Check whether to stop the CService.
        WaitForSingleObject(ghSvcStopEvent, INFINITE);
        ReportSvcStatus( SERVICE_STOPPED, NO_ERROR, 0 );
        return;
    }
}

//
// Purpose: 
//   Sets the current CService status and reports it to the SCM.
//
// Parameters:
//   dwCurrentState - The current state (see SERVICE_STATUS)
//   dwWin32ExitCode - The system error code
//   dwWaitHint - Estimated time for pending operation, 
//     in milliseconds
// 
// Return value:
//   None
//
VOID ReportSvcStatus( DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;

    // Fill in the SERVICE_STATUS structure.

    gSvcStatus.dwCurrentState = dwCurrentState;
    gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
    gSvcStatus.dwWaitHint = dwWaitHint;

    if (dwCurrentState == SERVICE_START_PENDING)
        gSvcStatus.dwControlsAccepted = 0;
    else gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    if ( (dwCurrentState == SERVICE_RUNNING) ||
           (dwCurrentState == SERVICE_STOPPED) )
        gSvcStatus.dwCheckPoint = 0;
    else gSvcStatus.dwCheckPoint = dwCheckPoint++;

    // Report the status of the CService to the SCM.
    SetServiceStatus( gSvcStatusHandle, &gSvcStatus );
}

//
// Purpose: 
//   Called by SCM whenever a control code is sent to the CService
//   using the ControlService function.
//
// Parameters:
//   dwCtrl - control code
// 
// Return value:
//   None
//
VOID WINAPI SvcCtrlHandler( DWORD dwCtrl ) {
	// Handle the requested control
	switch(dwCtrl) {  
		case SERVICE_CONTROL_STOP: 
			ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
			bWorking = FALSE;
			serverObj->MyDone();
			pipedCmd->MyDone();
			logger->CloseLog();
		
			// TODO: Signal the service to stop.
			// 1) Wait for all threads to stop (with timeout)
			// 2) If timeout - terminate non-stopped threads

			delete logger;
			delete pipedCmd;
			delete serverObj;
			SetEvent(ghSvcStopEvent);
			ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);

		case SERVICE_CONTROL_INTERROGATE: 
			break; 

		default: 
			break;
	}
}

DWORD WINAPI PipeWriter( LPVOID params )
{
	while (bWorking) 
    {
		// Read from socket...
		const char* tempBuf = serverObj->Recv();
		if (tempBuf == nullptr) { continue; }

		// ...and write to pipe
		pipedCmd->WriteToPipe(tempBuf);
	}
	return bWorking;
}

DWORD WINAPI PipeReader( LPVOID params )
{
	while (bWorking) 
    {
		// Read from pipe...
		LPCVOID tempBuf = pipedCmd->ReadFromPipe();
		if (tempBuf == nullptr) { continue; }

		// ...and write to socket
		serverObj->Send(static_cast<const char*>(tempBuf));
	}
	return bWorking;
}
