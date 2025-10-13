#pragma once

#include <cstdint>

namespace NDb
{
	struct SUIDesc;
}

// atom message reaction
struct IMessageReactionB2 : public CObjectBase
{
	virtual bool Execute( struct IScreen *pScreen, struct IScriptWrapper *pScript, struct IProgrammedReactionsAndChecks *pProg, uint16_t wKeyboardFlags ) const = 0;
	virtual int operator&( IBinSaver &ss ) = 0;
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc ) = 0;
};

// custom check 
struct IMessageCheck : public CObjectBase
{
	// returns check result
	virtual int Check( struct IScreen *pScreen, struct IScriptWrapper *pScript, struct IProgrammedReactionsAndChecks *pProg, uint16_t wKeyboardFlags ) const = 0;
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc ) = 0;
};


