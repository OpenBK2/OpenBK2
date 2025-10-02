#pragma once

namespace NBSU
{

// ignores
const DWORD IGNORE_THIS     = 0x00000001;
const DWORD IGNORE_NON_THIS = 0x00000002;
const DWORD IGNORE_FILE     = 0x00000004;
const DWORD IGNORE_NON_FILE = 0x00000008;
const DWORD IGNORE_ALL      = 0x00000010;
const DWORD IGNORE_LOG      = 0x00000020;
struct SIgnoresEntry
{
	string szCondition;
	string szFunctionName;
	string szFileName;
	int nLineNumber;
	DWORD dwFlags;
	//
	bool operator==( const SIgnoresEntry &ig ) const
	{
		return ( ( szCondition == ig.szCondition ) && 
			( szFileName == ig.szFileName   ) && 
			( nLineNumber == ig.nLineNumber ) && 
			( dwFlags == ig.dwFlags         ) );
	}
};
typedef list<SIgnoresEntry> SIgnoresList;

bool IsIgnore( const SIgnoresList &ignores, const char *pszFileName, int nLineNumber );
}
