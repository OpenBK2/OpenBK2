#ifndef __B2Script_H_
#define __B2Script_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
//
#include "ScriptWrapper.h"
#include "..\Script\Script.h"
//

namespace NScript
{
Script * GetScript();

class CScriptWrapper : public IScriptWrapper 
{
	Script script;
	OBJECT_NOCOPY_METHODS(CScriptWrapper);
	//{ implement common files loading here.
	int RunCommonFiles()
	{
		return true;
	}
	//}
	Script *GetScript() { return &script; }

public:
	void Init();
	void AddRegFunctions( const SRegFunction *pRegList );
	void Segment();
	int CallScriptFunction( const char *pszFunction, const bool bLogToConsole = false );
	int RunScriptFile( const char *szFileName );
	int RunScriptByID( int nID );
	int RunScript( const char *pszScriptText );
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &script );
		return 0;
	}

	friend Script * GetScript();
};

} // namespace

#endif

