#include "stdafx.h"
#include "BSAssertDialog.h"
#include "BSExceptionDialog.h"
#include "MemoryLib/SymAccess.h"
#include "BSUtil.h"
#include "BSUInit.h"

#include "port/stdcall.h"

#include <cinttypes>
#include <cstdint>

static void TypeDebugTrace( const char *buff, const char *pszWhat, const std::vector<SCallStackEntry> &entries )
{
	OutputDebugString( "*********************************************************************************************************\n" );
	OutputDebugString( buff );
	OutputDebugString( "\n" );
	OutputDebugString( pszWhat );
	OutputDebugString( "\n" );
	OutputDebugString( "CallStack entries dump:\n" );
	for ( int i = 0; i < entries.size(); ++i )
	{
		const SCallStackEntry &e = entries[i];
		char buff[1024];
		sprintf( buff, "%s(%d): %s\n", e.szFile.szStr, e.nLine, e.szFunc.szStr );
		OutputDebugString( buff );
	}
	OutputDebugString( "CallStack entries dump done\n" );
	OutputDebugString( "*********************************************************************************************************\n" );
}

namespace NBSU
{

static SIgnoresList ignores;
EBSUReport PORT_STDCALL ReportAssert( const char *pszCondition, const char *pszDescription,
	const char *pszFileName, int nLineNumber )
{
	// first, check for ignore
	if ( IsIgnore( ignores, pszFileName, nLineNumber ) )
		return BSU_IGNORE;
	//
	std::vector<SCallStackEntry> entries;
	{
		entries.resize( 1000 );
		int nEntries = CollectCallStack( &entries[0], entries.size() );
		entries.resize( nEntries );
		if ( entries.size() >= 2 )
			entries.erase( entries.begin(), entries.begin() + 2 );
	}
	TypeDebugTrace( pszCondition, pszDescription, entries );

	EBSUReport eRetCode = ShowAssertionDlg( GetBSUInstance(), 0, pszFileName, nLineNumber, pszCondition, pszDescription, entries, ignores, 0 );

	return eRetCode;
}
}

