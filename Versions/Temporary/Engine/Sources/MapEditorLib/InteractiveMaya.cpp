#include "stdafx.h"

#include <algorithm>

//#include "../MapEditorLib/Interface_Logger.h"
#include "InteractiveMaya.h"

namespace
{
	CObj<CInteractiveMaya> pInteractiveMayaInstance;
}


CInteractiveMaya * CInteractiveMaya::Get()
{
	if ( !pInteractiveMayaInstance )
	{
		pInteractiveMayaInstance = new CInteractiveMaya;
	}
	return pInteractiveMayaInstance;
}


const string CInteractiveMaya::INTERACTIVE_MAYA_PROMPT("mel: ");         // leading space is important
const string CInteractiveMaya::INTERACTIVE_MAYA_RESULT_MARK("Result: "); // leading space is important


CInteractiveMaya::CInteractiveMaya()
	: INTERACTIVE_MAYA_INVOKE("mayabatch -prompt"),
	  INTERACTIVE_MAYA_QUIT_DIRECTIVE("quit -f\n"), // leading \n is important
	  nExecutionQuota(0),
	  nExecutionCounter(0)
{
}


void CInteractiveMaya::SetResponseTimeout( int nResponseWaitTimeout )
{
	process.SetResponseTimeout( nResponseWaitTimeout );
}


void CInteractiveMaya::SetExecutionQuota( int n )
{
	nExecutionCounter = nExecutionQuota = max(0, n);
}


bool CInteractiveMaya::Start()
{
	string szErrorMessage;
	ILogger *pLogger = NLog::GetLogger();
	pLogger->Log( LT_IMPORTANT, "Starting Maya interactive session...\n" );
	if ( process.Start( INTERACTIVE_MAYA_INVOKE, &szErrorMessage ) )
	{
		string szOutput;
		string szErrorOutput;
		if ( process.Execute( "", INTERACTIVE_MAYA_PROMPT, &szOutput, &szErrorOutput, &szErrorMessage ) )
		{
			pLogger->Log( LT_NORMAL, szOutput );
			pLogger->Log( LT_ERROR, szErrorOutput );
			nExecutionCounter = nExecutionQuota;
			return true;
		}
	}
	pLogger->Log( LT_ERROR, "Maya session start failed: " );
	pLogger->Log( LT_ERROR, szErrorMessage );

	return false;
}


bool CInteractiveMaya::Execute( const string &szScript, string *pszOutput, string *pszErrorOutput )
{
	ILogger *pLogger = NLog::GetLogger();
	pLogger->Log( LT_IMPORTANT, szScript );
	string szErrorMessage;
	if ( process.Execute( szScript, INTERACTIVE_MAYA_PROMPT, pszOutput, pszErrorOutput, &szErrorMessage ) )
	{
		pLogger->Log( LT_NORMAL, *pszOutput );
		pLogger->Log( LT_ERROR, *pszErrorOutput );

		if ( nExecutionQuota && (--nExecutionCounter) == 0 )
		{
			pLogger->Log( LT_IMPORTANT, INTERACTIVE_MAYA_QUIT_DIRECTIVE );
			if ( process.Stop( INTERACTIVE_MAYA_QUIT_DIRECTIVE ) )
			{
				pLogger->Log( LT_IMPORTANT, "Maya interactive session has ended due to exceeding execution quota.\n" );
				return true;
			}
			return false;
		}

		return true;
	}
	pLogger->Log( LT_ERROR, "Maya session exec operation failed: " );
	pLogger->Log( LT_ERROR, szErrorMessage );
	return false;
}


bool CInteractiveMaya::Stop()
{
	ILogger *pLogger = NLog::GetLogger();
	if ( process.IsStarted() )
	{
		pLogger->Log( LT_IMPORTANT, INTERACTIVE_MAYA_QUIT_DIRECTIVE );
		if ( process.Stop( INTERACTIVE_MAYA_QUIT_DIRECTIVE ) )
		{
			pLogger->Log( LT_IMPORTANT, "Maya interactive session has ended.\n" );
			return true;
		}
		return false;
	}
	pLogger->Log( LT_ERROR, "Maya interactive session was terminated unexpectedly.\n" );
	return true;
}


bool CInteractiveMaya::ExtractResult( string *pszResult, const string &szOutput )
{
	const size_t promptPos = szOutput.find( INTERACTIVE_MAYA_PROMPT, (szOutput.size() - INTERACTIVE_MAYA_PROMPT.size()) );
	if ( promptPos != string::npos )
	{
		const char *pBlockFirst = szOutput.c_str();
		const char *pBlockLast = pBlockFirst + promptPos;
		const char *pWhatFirst = INTERACTIVE_MAYA_RESULT_MARK.c_str();
		const char *pWhatLast = pWhatFirst + INTERACTIVE_MAYA_RESULT_MARK.size();
		const char *pResultStart = std::find_end( pBlockFirst, pBlockLast, pWhatFirst, pWhatLast );
		if ( pResultStart != pBlockLast )
		{
			// need to remove \r\n always placed by Maya between "Result: <>" and following "mel: "
			const char *pResultEnd = pBlockLast;
			if ( 0 == strncmp( pResultEnd - 2, "\r\n", 2 ) )
			{
				pResultEnd -= 2;
			}
			pResultStart += INTERACTIVE_MAYA_RESULT_MARK.size();
			pszResult->assign( pResultStart, pResultEnd - pResultStart );
			return true;
		}
		else
		{
			pszResult->clear();
			return true;
		}
	}
	else
	{
		// if this is the case, then supplied szOutput is invalid
		// (e.g comes from failed invocation of a maya command).
	}
	return false;
}


bool CInteractiveMaya::TransactCommand( const string &szScript, const string &szExpectedResult )
{
	string szOutput;
	string szErrorOutput;
	ILogger *pLogger = NLog::GetLogger();
	if ( Execute( szScript, &szOutput, &szErrorOutput ) )
	{
		string szResult;
		if ( ExtractResult( &szResult, szOutput ) )
		{
			if ( szResult == szExpectedResult )
			{
				return true;
			}
			else
			{
				pLogger->Log( LT_ERROR,
						StrFmt("\nCommand FAILED: result must be \"%s\" (actual: \"%s\")\n", szExpectedResult.c_str(), szResult.c_str())
						);
			}
		}
	}
	return false;
}


bool CInteractiveMaya::TransactQuery( const string &szScript, string *pszResult )
{
	string szOutput;
	string szErrorOutput;
	if ( Execute( szScript, &szOutput, &szErrorOutput ) )
	{
		if ( ExtractResult( pszResult, szOutput ) )
		{
			return true;
		}
	}
	return false;
}

