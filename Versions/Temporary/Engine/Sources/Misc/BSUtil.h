#pragma once

#include <cstdint>

namespace NBSU
{

// ignores
const uint32_t IGNORE_THIS     = 0x00000001;
const uint32_t IGNORE_NON_THIS = 0x00000002;
const uint32_t IGNORE_FILE     = 0x00000004;
const uint32_t IGNORE_NON_FILE = 0x00000008;
const uint32_t IGNORE_ALL      = 0x00000010;
const uint32_t IGNORE_LOG      = 0x00000020;
struct SIgnoresEntry
{
	std::string szCondition;
	std::string szFunctionName;
	std::string szFileName;
	int nLineNumber;
	uint32_t dwFlags;
	//
	bool operator==( const SIgnoresEntry &ig ) const
	{
		return ( ( szCondition == ig.szCondition ) && 
			( szFileName == ig.szFileName   ) && 
			( nLineNumber == ig.nLineNumber ) && 
			( dwFlags == ig.dwFlags         ) );
	}
};
typedef std::list<SIgnoresEntry> SIgnoresList;

bool IsIgnore( const SIgnoresList &ignores, const char *pszFileName, int nLineNumber );
}
