#if !defined(__INTERACTIVE_MAYA__)
#define __INTERACTIVE_MAYA__
#pragma once

#include "InteractiveProcess.h"
#include "Interface_Logger.h"


class CInteractiveMaya : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CInteractiveMaya );

	static const string INTERACTIVE_MAYA_PROMPT;
	static const string INTERACTIVE_MAYA_RESULT_MARK;
	const string INTERACTIVE_MAYA_INVOKE;
	const string INTERACTIVE_MAYA_QUIT_DIRECTIVE;

	CInteractiveProcess process;
	int nExecutionQuota;
	int nExecutionCounter;

	CInteractiveMaya();
public:
	static CInteractiveMaya * Get();
	static bool ExtractResult( string *pszResult, const string &szOutput );

	void SetResponseTimeout( int nResponseWaitTimeout );
	void SetExecutionQuota( int n );

	bool IsStarted()
	{
		return process.IsStarted();
	}
	bool Start();
	bool Execute( const string &szScript, string *pszOutput, string *pszErrorOutput );
	bool Stop();

	// helpers
	bool TransactCommand( const string &szScript, const string &szExpectedResult );
	bool TransactQuery( const string &szScript, string *pszResult );
};

#endif //#define __INTERACTIVE_MAYA__
