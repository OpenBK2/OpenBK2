#pragma once

#include "port/cdecl.h"

struct SCallStackEntry; 
namespace NBSU
{
	EBSUReport PORT_CDECL ShowExceptionDlg( HINSTANCE hInstance, HWND hWnd,
		const char *pszCondition, const char *pszDescription, 
		const std::vector<SCallStackEntry> &entries, const char *pszExtInfo );
}
