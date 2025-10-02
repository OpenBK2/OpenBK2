#ifndef _IReaction_h_included_
#define _IReaction_h_included_

namespace NDb
{
	struct SUIDesc;
}

// atom message reaction
interface IMessageReactionB2 : public CObjectBase
{
	virtual bool Execute( interface IScreen *pScreen, interface IScriptWrapper *pScript, interface IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags ) const = 0;
	virtual int operator&( IBinSaver &ss ) = 0;
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc ) = 0;
};

// custom check 
interface IMessageCheck : public CObjectBase
{
	// returns check result
	virtual int Check( interface IScreen *pScreen, interface IScriptWrapper *pScript, interface IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags ) const = 0;
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc ) = 0;
};

#endif //_IReaction_h_included_
