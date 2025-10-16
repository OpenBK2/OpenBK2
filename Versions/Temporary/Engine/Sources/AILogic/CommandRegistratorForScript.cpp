#include "stdafx.h"
#include "commandregistratorforscript.h"
#include "Scripts.h"

#include <fmt/format.h>

extern CScripts* pScripts;
CCommandRegistratorForScript theCommandTrackerForScript;

void CCommandRegistratorForScript::Register( int nCommand )
{
	registeredCommands[nCommand] = true;
}

void CCommandRegistratorForScript::Called( int nCommand ) {
	if ( !registeredCommands.empty() && registeredCommands.find( nCommand ) != registeredCommands.end() ) {
		const auto function = fmt::format( "OnRegisteredCommand( {} )", nCommand );
		pScripts->CallScriptFunction(function.c_str());
	}
}


