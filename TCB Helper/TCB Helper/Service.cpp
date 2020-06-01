#include "pch.h"
#include "Service.h"
#include "PrivilegesManager.h"

#pragma comment(lib, "advapi32.lib")

#define SVCNAME TEXT("AAA_TcbHelper")
#define LOG_PATH "D:\\TcbHelper.log"
#define SESSION_ID 1 //// IMPORTANT PART - UNIQUE FOR SESSION ////

SERVICE_STATUS          gSvcStatus; 
SERVICE_STATUS_HANDLE   gSvcStatusHandle; 
HANDLE                  ghSvcStopEvent = nullptr;
FILE*                   gLog;

BOOL CreateCmdOnInteractiveDesktop();
BOOL EnableTcb();

VOID WINAPI SvcCtrlHandler( DWORD );
VOID ReportSvcStatus( DWORD, DWORD, DWORD );
VOID SvcInit( DWORD, LPTSTR * ); 
VOID WINAPI SvcMain( DWORD dwArgc, LPTSTR *lpszArgv );
VOID LogError( LPCTSTR szFunction );


void __cdecl Service::RunService(int argc, char *argv[])
{
	// Log Initialization
    fopen_s(&gLog, LOG_PATH, "a+");
	
    SERVICE_TABLE_ENTRY DispatchTable[] = 
	{ 
        { LPTSTR(SVCNAME), (LPSERVICE_MAIN_FUNCTION) SvcMain }, 
        { NULL, NULL } 
    }; 
 
    LogError(L"Starting Service...");
	
    if (!StartServiceCtrlDispatcher( DispatchTable )) 
	{
    	LogError(L"StartServiceCtrlDispatcher - Failed!!!");
    }
}


VOID WINAPI SvcMain( DWORD dwArgc, LPTSTR *lpszArgv )
{
    // Register the handler function for the Service

    gSvcStatusHandle = RegisterServiceCtrlHandler( 
        SVCNAME, 
        SvcCtrlHandler);

    if( !gSvcStatusHandle )
    { 
        LogError(TEXT("RegisterServiceCtrlHandler - Failed!!!")); 
        return; 
    }
	else 
    {
		LogError(TEXT("RegisterServiceCtrlHandler - OK")); 
    }

    gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS; 
    gSvcStatus.dwServiceSpecificExitCode = 0;    

    // Report initial status to the SCM
    ReportSvcStatus( SERVICE_START_PENDING, NO_ERROR, 3000 );
	
    // Perform Service-specific initialization and work
    SvcInit( dwArgc, lpszArgv );
}


VOID SvcInit( DWORD dwArgc, LPTSTR *lpszArgv)
{
    ghSvcStopEvent = CreateEvent(
                         NULL,    // default security attributes
                         TRUE,    // manual reset event
                         FALSE,   // not signaled
                         NULL);   // no name

    if ( ghSvcStopEvent == NULL )
    {
        ReportSvcStatus( SERVICE_STOPPED, NO_ERROR, 0 );
        return;
    }

    // Report running status when initialization is complete.
    ReportSvcStatus( SERVICE_RUNNING, NO_ERROR, 0 );

    // Perform work until Service stops.
	CreateCmdOnInteractiveDesktop();

    while(1)
    {
        // Check whether to stop the Service.

        WaitForSingleObject(ghSvcStopEvent, INFINITE);

        ReportSvcStatus( SERVICE_STOPPED, NO_ERROR, 0 );
        return;
    }
}


VOID ReportSvcStatus( DWORD dwCurrentState,
                      DWORD dwWin32ExitCode,
                      DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;

    // Fill in the SERVICE_STATUS structure
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

    // Report the status of the Service to the SCM.
    SetServiceStatus( gSvcStatusHandle, &gSvcStatus );
}


// Handle the requested control code. 
VOID WINAPI SvcCtrlHandler( DWORD dwCtrl )
{
	switch(dwCtrl)
	{
		case SERVICE_CONTROL_STOP: 
			ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

			// Signal the Service to stop.

			// TODO: In normal Service
			// 1) Wait for all threads to stop (with timeout)
			// 2) If timeout - terminate non-stopped threads

			SetEvent(ghSvcStopEvent);
			ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);

		case SERVICE_CONTROL_INTERROGATE: 
			break; 

		default: 
			break;
	}
}


BOOL EnableTcb(HANDLE hToken)
{
	const auto bSuccess = SetPrivilege(hToken, SE_TCB_NAME, true);
	return bSuccess;
}


BOOL CreateCmdOnInteractiveDesktop()
{
	HANDLE hProcessToken, hNewProcessToken;
	DWORD dwSessionId = SESSION_ID, cbReturnLength;
	
	if (!OpenProcessToken (GetCurrentProcess(), MAXIMUM_ALLOWED, &hProcessToken))
    {
		LogError(TEXT("OpenProcessToken - Failed!!!"));
		return FALSE;
	}
	else
    {
		LogError(TEXT("OpenProcessToken - OK"));
	}

	/*if (!EnableTcb(hProcessToken))
	{
		LogError(TEXT("EnableTcb - Failed!!!"));
		return FALSE;
	}
	else
    {
		LogError(TEXT("EnableTcb - OK"));
	}*/
	
	if (!DuplicateTokenEx (hProcessToken, MAXIMUM_ALLOWED, nullptr, SecurityImpersonation, TokenPrimary, &hNewProcessToken)) 
    {
		LogError(TEXT("DuplicateTokenEx - Failed!!!"));
		return FALSE;
	}
	else 
    {
		LogError(TEXT("DuplicateTokenEx - OK"));
	}

    if (!SetTokenInformation (hNewProcessToken, TokenSessionId, &dwSessionId, sizeof(DWORD)))
    {
		LogError(TEXT("SetTokenInformation - Failed!!!"));
		return FALSE;
	}
	else
    {
		LogError(TEXT("SetTokenInformation - OK"));
	}

	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	memset(&si, 0, sizeof(pi));

	if (!CreateProcessAsUserA (hNewProcessToken, NULL, LPSTR("C:\\Windows\\System32\\cmd.exe"),
        NULL, NULL, FALSE,
        0, NULL, NULL, &si, &pi))
    {
		LogError(TEXT("CreateProcessAsUser - Failed!!!"));
		return FALSE;
	}
	else
    {
		LogError(TEXT("CreateProcessAsUser - OK"));
	}

	return TRUE;
}

VOID LogError(LPCTSTR szFunction)
{
	#define buffsize 160
	wchar_t buffer[buffsize];
	StringCchPrintf(buffer, buffsize, L"%s [Code = %d]", szFunction, GetLastError());
	if (gLog)
	{
		fprintf(gLog, "%ls \n", buffer);
	}
}
