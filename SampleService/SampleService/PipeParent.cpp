#include "pch.h"
#include "PipeParent.h"

#include <windows.h>
#include <tchar.h>
#include <cstdio> 
#include <strsafe.h>

constexpr int BUFSIZE = 4096;
 
HANDLE g_hChildStd_IN_Rd = nullptr;
HANDLE g_hChildStd_IN_Wr = nullptr;
HANDLE g_hChildStd_OUT_Rd = nullptr;
HANDLE g_hChildStd_OUT_Wr = nullptr;

HANDLE g_hInputFile = nullptr;
 
void CreateChildProcess(void); 
void WriteToPipe(void); 
void ReadFromPipe(void); 
void ErrorExit(PCTSTR); 
 
int PipeParent::RunParent(int argc, TCHAR *argv[]) { 
	SECURITY_ATTRIBUTES saAttr; 
 
	printf("\n->Start of PipeParent execution.\n");

	// Set the bInheritHandle flag so pipe handles are inherited. 
 
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE; 
	saAttr.lpSecurityDescriptor = nullptr; 

	// Create a pipe for the child process's STDOUT. 
 
	if ( ! CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0) ) 
		ErrorExit(TEXT("StdoutRd CreatePipe")); 

	// Ensure the read handle to the pipe for STDOUT is not inherited.

	if ( ! SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0) )
		ErrorExit(TEXT("Stdout SetHandleInformation")); 

	// Create a pipe for the child process's STDIN. 
 
	if (! CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0)) 
		ErrorExit(TEXT("Stdin CreatePipe")); 

	// Ensure the write handle to the pipe for STDIN is not inherited. 
 
	if ( ! SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) )
		ErrorExit(TEXT("Stdin SetHandleInformation")); 
	 
	// Create the child process. 
   
	CreateChildProcess();
	 
	// Write to the pipe that is the standard input for a child process. 
	// Data is written to the pipe's buffers, so it is not necessary to wait
	// until the child process is running before writing data.
 
	WriteToPipe(); 
	printf( "\n->Contents of %s written to child STDIN pipe.\n", argv[1]);
	 
	// Read from pipe that is the standard output for child process. 
 
	printf( "\n->Contents of child process STDOUT:\n\n");
	ReadFromPipe(); 

	printf("\n->End of PipeParent execution.\n");

	// The remaining open handles are cleaned up when this process terminates. 
	// To avoid resource leaks in a larger application, close handles explicitly. 

	return 0; 
} 

// Create a child process that uses the previously created pipes for STDIN and STDOUT.
void CreateChildProcess() { 
	TCHAR szCmdline[]=TEXT("C:\\Windows\\System32\\cmd.exe");
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
 
	// Create the child process. 
    
	bSuccess = CreateProcess(NULL, 
		szCmdline,     // command line 
		NULL,          // process security attributes 
		NULL,          // primary thread security attributes 
		TRUE,          // handles are inherited 
		0,             // creation flags 
		NULL,          // use PipeParent's environment 
		NULL,          // use PipeParent's current directory 
		&siStartInfo,  // STARTUPINFO pointer 
		&piProcInfo);  // receives PROCESS_INFORMATION 
   
	// If an error occurs, exit the application. 
	if ( ! bSuccess ) 
		ErrorExit(TEXT("CreateProcess"));
	else {
		// Close handles to the child process and its primary thread.
		// Some applications might keep these handles to monitor the status
		// of the child process, for example. 

		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
      
		// Close handles to the stdin and stdout pipes no longer needed by the child process.
		// If they are not explicitly closed, there is no way to recognize that the child process has ended.
      
		CloseHandle(g_hChildStd_OUT_Wr);
		CloseHandle(g_hChildStd_IN_Rd);
	}
}

// Read from a file and write its contents to the pipe for the child's STDIN.
// Stop when there is no more data. 
void WriteToPipe(void) { 
	DWORD dwRead, dwWritten; 
	CHAR chBuf[BUFSIZE];
	BOOL bSuccess = FALSE;
 
	for (;;) { 
		bSuccess = ReadFile(g_hInputFile, chBuf, BUFSIZE, &dwRead, NULL);
		if ( ! bSuccess || dwRead == 0 ) break; 
      
		bSuccess = WriteFile(g_hChildStd_IN_Wr, chBuf, dwRead, &dwWritten, NULL);
		if ( ! bSuccess ) break; 
	} 
	 
	// Close the pipe handle so the child process stops reading. 
 
	if ( ! CloseHandle(g_hChildStd_IN_Wr) ) 
		ErrorExit(TEXT("StdInWr CloseHandle")); 
} 

// Read output from the child process's pipe for STDOUT
// and write to the PipeParent process's pipe for STDOUT. 
// Stop when there is no more data. 
void ReadFromPipe(void) { 
	DWORD dwRead, dwWritten; 
	CHAR chBuf[BUFSIZE]; 
	BOOL bSuccess = FALSE;
	HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	for (;;) { 
		bSuccess = ReadFile( g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
		if( ! bSuccess || dwRead == 0 ) break; 

		bSuccess = WriteFile(hParentStdOut, chBuf, 
                           dwRead, &dwWritten, NULL);
		if (! bSuccess ) break; 
	} 
} 

// Format a readable error message, display a message box, 
// and exit from the application.
void ErrorExit(PCTSTR lpszFunction) { 
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(1);
}