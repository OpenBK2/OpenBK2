#pragma once

#include "UI/IMessageReaction.h"
#include "DBUISpecificB2.h"


class CARSetForcedAction : public IMessageReactionB2 
{
	OBJECT_BASIC_METHODS( CARSetForcedAction );
	CDBPtr<NDb::SARSetForcedAction> pDesc;
public:       
	int operator&( IBinSaver &ss );
	virtual bool Execute( struct IScreen *pScreen, struct IScriptWrapper *pScript, struct IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags ) const;
	virtual void InitByDesc( const struct NDb::SUIDesc *_pDesc );
};

class CARSetSpecialAbility : public IMessageReactionB2 
{
	OBJECT_BASIC_METHODS( CARSetSpecialAbility );
	CDBPtr<NDb::SARSetSpecialAbility> pDesc;
public:       
	int operator&( IBinSaver &ss );
	virtual bool Execute( struct IScreen *pScreen, struct IScriptWrapper *pScript, struct IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags ) const;
	virtual void InitByDesc( const struct NDb::SUIDesc *_pDesc );
};


