#include "stdafx.h"
#include "commandregistratorforscript.h"
#include "Scripts.h"

extern CScripts* pScripts;
CCommandRegistratorForScript theCommandTrackerForScript;

void CCommandRegistratorForScript::Register( int nCommand )
{
	registeredCommands[nCommand] = true;
}

void CCommandRegistratorForScript::Called( int nCommand )
{
	if ( !registeredCommands.empty() && registeredCommands.find( nCommand ) != registeredCommands.end() )
		pScripts->CallScriptFunction( StrFmt( "OnRegisteredCommand( %i )", nCommand ) );
}


