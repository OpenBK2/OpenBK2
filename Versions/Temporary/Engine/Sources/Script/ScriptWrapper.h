#pragma once

#include "Script_export.h"

#include "RegFunction.h"

interface IScriptWrapper : public CObjectBase
{
	virtual void Init() = 0;
	virtual void AddRegFunctions( const SRegFunction *pRegList ) = 0;
	virtual void Segment() = 0;
	virtual int CallScriptFunction( const char *pszFunction, const bool bLogToConsole = false ) = 0;
	virtual int RunScriptFile( const char *szFileName ) = 0;
	virtual int RunScriptByID( int nID ) = 0;
	virtual int RunScript( const char *pszScriptText ) = 0;
	virtual int operator&( IBinSaver &saver ) = 0;
};

SCRIPT_EXPORT IScriptWrapper* CreateScriptWrapper();
//
namespace NScript
{
SCRIPT_EXPORT void RegisterCommonFunctionsToSaveLoad();
SCRIPT_EXPORT void AddScriptFunctionsToSaveLoad( const SRegFunction *pRegList );
}

