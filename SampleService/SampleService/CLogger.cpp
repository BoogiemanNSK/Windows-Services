#include "pch.h"
#include "CLogger.h"

// Log Initialization
CLogger::CLogger(const char* logPath)
{
	GLog = nullptr;
    fopen_s(&GLog, logPath, "a+");
}

// Write to Log
VOID CLogger::Log(const LPCTSTR szFunction) const
{
	TCHAR buffer[BUFF_SIZE];
	StringCchPrintf(buffer, BUFF_SIZE, "%s [Code = %d]", szFunction, GetLastError());
	if (GLog)
		fprintf(GLog, "%s\n", buffer);
}

VOID CLogger::CloseLog() const
{
	fclose(GLog);	
}

