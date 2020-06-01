#include "pch.h"
#include "CPipe.h"
#include "CLogger.h"

#include <strsafe.h>

#include "CTcbHelper.h"

HANDLE g_hChildStd_IN_Rd = nullptr;
HANDLE g_hChildStd_IN_Wr = nullptr;
HANDLE g_hChildStd_OUT_Rd = nullptr;
HANDLE g_hChildStd_OUT_Wr = nullptr;
HANDLE g_hInputFile = nullptr;

bool CPipe::MyInit()
{
	logger = new CLogger("D:\\CPipeLog.log");
	
	SECURITY_ATTRIBUTES saAttr; 
	logger->Log("Start of CPipe execution...");
	
	// Set the bInheritHandle flag so pipe handles are inherited. 
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE; 
	saAttr.lpSecurityDescriptor = nullptr; 

	// Create a pipe for the child process's STDOUT. 
	if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0) )
		logger->Log("StdoutRd CreatePipe - Failed!!!");

	// Ensure the read handle to the pipe for STDOUT is not inherited.
	if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0) )
		logger->Log("Stdout SetHandleInformation - Failed!!!");

	// Create a pipe for the child process's STDIN. 
	if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
		logger->Log("Stdin CreatePipe - Failed!!!");

	// Ensure the write handle to the pipe for STDIN is not inherited.  
	if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) )
		logger->Log("Stdin SetHandleInformation - Failed!!!");

	return true;
}

void CPipe::MyDone()
{
	// TODO Perfrom a proper cleanup
	// Check if handles were initialized
	SetEvent(hStopThreadEvent);
	const auto res = WaitForSingleObject(hThread, 60 * 1000);

	// Close the pipe handle so the child process stops reading. 
	if (!CloseHandle(g_hChildStd_IN_Wr))
		logger->Log("StdInWr CloseHandle - Failed!!!");
      
	// Close handles to the stdin and stdout pipes no longer needed by the child process.
	// If they are not explicitly closed, there is no way to recognize that the child process has ended.
	CloseHandle(g_hChildStd_OUT_Wr);
	CloseHandle(g_hChildStd_IN_Rd);
	
	switch (res) {
		case 0:
			return;
		default:
			// TODO Error / Cleanup
			TerminateThread(hThread, 0);
	}
}

// Create a child process that uses the previously created pipes for STDIN and STDOUT.
int CPipe::CreatePipedCmd()
{
	HANDLE hProcessToken, hNewProcessToken;
	DWORD dwSessionId = SESSION_ID;
	
	if (!OpenProcessToken (GetCurrentProcess(), MAXIMUM_ALLOWED, &hProcessToken))
    {
		logger->Log("OpenProcessToken - Failed!!!");
		return FALSE;
	}
	logger->Log("OpenProcessToken - OK");

	if (!CTcbHelper::EnableTcb(hProcessToken))
	{
		logger->Log("EnableTcb - Failed!!!");
		return FALSE;
	}
	logger->Log("EnableTcb - OK");

	if (!DuplicateTokenEx (hProcessToken, MAXIMUM_ALLOWED, nullptr, SecurityImpersonation, TokenPrimary, &hNewProcessToken)) 
    {
		logger->Log("DuplicateTokenEx - Failed!!!");
		return FALSE;
	}
	logger->Log("DuplicateTokenEx - OK");

	if (!SetTokenInformation (hNewProcessToken, TokenSessionId, &dwSessionId, sizeof(DWORD)))
    {
    	logger->Log("SetTokenInformation - Failed!!!");
		return FALSE;
	}
	logger->Log("SetTokenInformation - OK");
	
	TCHAR szCmdline[] = TEXT("C:\\Windows\\System32\\cmd.exe");
	PROCESS_INFORMATION piProcInfo; 
	STARTUPINFO siStartInfo;
	BOOL bSuccess = FALSE; 
	 
	// Set up members of the PROCESS_INFORMATION structure. 
	ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
	 
	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDIN and STDOUT handles for redirection.
	ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
	siStartInfo.cb = sizeof(STARTUPINFO); 
	siStartInfo.hStdError = g_hChildStd_OUT_Wr;
	siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
	siStartInfo.hStdInput = g_hChildStd_IN_Rd;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	bSuccess = CreateProcessAsUserA (hNewProcessToken,
									nullptr,
									szCmdline,
							        nullptr,
									nullptr,
									TRUE,
							        0,
									nullptr,
									nullptr,
									&siStartInfo,
									&piProcInfo);
    
	if (!bSuccess )
	{
		logger->Log("Create process - Failed!!!");
		return 1;
	}
	logger->Log("Create process - OK");

	// Close handles to the child process and its primary thread.
	// Some applications might keep these handles to monitor the status
	// of the child process, for example. 
	CloseHandle(piProcInfo.hProcess);
	CloseHandle(piProcInfo.hThread);
	
	return 0;
}

// Read from a file and write its contents to the pipe for the child's STDIN.
// Stop when there is no more data. 
void CPipe::WriteToPipe(LPCVOID buffer)
{ 
	DWORD dwWritten;
	BOOL bSuccess = WriteFile(g_hChildStd_IN_Wr, buffer, BUFSIZE, &dwWritten, nullptr);
	if (!bSuccess) logger->Log("Write to child stdin - Failed!!!");
} 

// Read output from the child process's pipe for STDOUT
// and write to the CPipe process's pipe for STDOUT. 
// Stop when there is no more data. 
LPCVOID CPipe::ReadFromPipe()
{ 
	DWORD dwRead;
	BOOL bSuccess = ReadFile(g_hChildStd_OUT_Rd, ReadBuffer, BUFSIZE, &dwRead, nullptr);
	if (!bSuccess || dwRead == 0) 
	{
		logger->Log("Read from child stdout - Failed!!!");
		return nullptr;
	}
	return ReadBuffer;
}
