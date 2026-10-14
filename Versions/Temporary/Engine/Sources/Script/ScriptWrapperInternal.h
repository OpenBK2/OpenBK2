#pragma once
//
#include "Script/Script.h"
#include "ScriptWrapper.h"

//

namespace NScript
{
Script *GetScript();

class CScriptWrapper : public IScriptWrapper
{
	Script script;
	OBJECT_NOCOPY_METHODS( CScriptWrapper );

	//{ implement common files loading here.
	int RunCommonFiles()
	{
		return true;
	}

	//}
	Script *GetScript()
	{
		return &script;
	}

public:
	void Init() override;
	void AddRegFunctions( const SRegFunction *pRegList ) override;
	void Segment() override;
	int CallScriptFunction( const char *pszFunction, const bool bLogToConsole = false ) override;
	int RunScriptFile( const char *szFileName ) override;
	int RunScriptByID( int nID ) override;
	int RunScript( const char *pszScriptText ) override;

	int operator&( IBinSaver &saver ) override
	{
		saver.Add( 1, &script );
		return 0;
	}

	friend Script *GetScript();
};

} // namespace NScript
