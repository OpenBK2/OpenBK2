#pragma once

#include "RegFunction.h"

#include "Script_export.h"

struct IScriptWrapper : public CObjectBase
{
	virtual void Init() = 0;
	virtual void AddRegFunctions( const SRegFunction *pRegList ) = 0;
	virtual void Segment() = 0;
	virtual int CallScriptFunction( const char *pszFunction, const bool bLogToConsole = false ) = 0;
	virtual int RunScriptFile( const char *szFileName ) = 0;
	virtual int RunScriptByID( int nID ) = 0;
	virtual int RunScript( const char *pszScriptText ) = 0;
	int operator&( IBinSaver &saver ) override = 0;
};

SCRIPT_EXPORT IScriptWrapper *CreateScriptWrapper();

//
namespace NScript
{
SCRIPT_EXPORT void RegisterCommonFunctionsToSaveLoad();
SCRIPT_EXPORT void AddScriptFunctionsToSaveLoad( const SRegFunction *pRegList );
} // namespace NScript
