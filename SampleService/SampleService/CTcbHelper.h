#pragma once
#include "pch.h"

class CTcbHelper {
	
public:
	static BOOL EnableTcb(HANDLE hToken);
	static BOOL SetPrivilege( HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege );
	
};