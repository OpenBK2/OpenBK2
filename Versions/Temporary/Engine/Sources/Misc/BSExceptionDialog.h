#pragma once

struct SCallStackEntry; 
namespace NBSU
{
	EBSUReport __cdecl ShowExceptionDlg( HINSTANCE hInstance, HWND hWnd,
		const char *pszCondition, const char *pszDescription, 
		const std::vector<SCallStackEntry> &entries, const char *pszExtInfo );
}
