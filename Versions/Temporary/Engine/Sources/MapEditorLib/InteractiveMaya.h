#pragma once

#include "InteractiveProcess.h"
#include "Interface_Logger.h"

#include "MapEditorLib_export.h"

class MAPEDITORLIB_EXPORT CInteractiveMaya : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CInteractiveMaya );

	static const std::string INTERACTIVE_MAYA_PROMPT;
	static const std::string INTERACTIVE_MAYA_RESULT_MARK;
	const std::string INTERACTIVE_MAYA_INVOKE;
	const std::string INTERACTIVE_MAYA_QUIT_DIRECTIVE;

	CInteractiveProcess process;
	int nExecutionQuota;
	int nExecutionCounter;

	CInteractiveMaya();
public:
	static CInteractiveMaya * Get();
	static bool ExtractResult( std::string *pszResult, const std::string &szOutput );

	void SetResponseTimeout( int nResponseWaitTimeout );
	void SetExecutionQuota( int n );

	bool IsStarted()
	{
		return process.IsStarted();
	}
	bool Start();
	bool Execute( const std::string &szScript, std::string *pszOutput, std::string *pszErrorOutput );
	bool Stop();

	// helpers
	bool TransactCommand( const std::string &szScript, const std::string &szExpectedResult );
	bool TransactQuery( const std::string &szScript, std::string *pszResult );
};


