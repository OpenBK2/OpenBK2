#pragma once
#include "BSUtil.h"

struct SCallStackEntry;
namespace NBSU
{
EBSUReport ShowAssertionDlg( HINSTANCE hInstance, HWND hWnd,
	const char *pszFileName, int nLineNumber,
	const char *_pszCondition, const char *_pszDescription, 
	const std::vector<SCallStackEntry> &entries, SIgnoresList &ignores,
	const char *pszExtInfo );
}

