#pragma once
#include "pch.h"

constexpr int BUFF_SIZE = 160;

class CLogger
{
	
public:
	CLogger() = delete;
	CLogger(const char* logPath);
	VOID Log(LPCTSTR szFunction) const;
	VOID CloseLog() const;

private:
	FILE* GLog;
};
