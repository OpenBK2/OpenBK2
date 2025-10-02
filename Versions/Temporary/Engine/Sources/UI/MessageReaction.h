// Reaction.h: interface for the CReaction class.
//


#if !defined(AFX_REACTION_H__C9D8977B_A116_4A9B_93A3_8EBE426CA74D__INCLUDED_)
#define AFX_REACTION_H__C9D8977B_A116_4A9B_93A3_8EBE426CA74D__INCLUDED_

#include "IMessageReaction.h"

typedef vector< CPtr<IMessageReactionB2> > CMessageSequence;
typedef vector< CDBPtr<NDb::SUIDesc> > CMessageSequienceDesc;


// 1 check (branches) and sequience of atim reactions for each branch
class CMessageReactionB2 : public IMessageReactionB2
{
	OBJECT_BASIC_METHODS( CMessageReactionB2 );
	typedef hash_map<int/*custom check return*/, CMessageSequence> CMessageSequences;

	CPtr<IMessageCheck> pCheck;
	CMessageSequences branches;
	CMessageSequence commonBefore;					// always run and before any branch
	CMessageSequence commonAfter;						// always run and after any branch

	bool Execute( const CMessageSequence *pToExecute, interface IScreen *pScreen, interface IScriptWrapper *pScript, interface IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags ) const;
	void InitSequienceByDesc( CMessageSequence *pCreate, const CMessageSequienceDesc &src );

public:
	CMessageReactionB2() {  }
	virtual bool Execute( interface IScreen *pScreen, interface IScriptWrapper *pScript, interface IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags ) const;
	int operator&( IBinSaver &ss );
	void AddCommonBefore( IMessageReactionB2 *pReaction ) { commonBefore.push_back( pReaction ); }
	virtual void InitByDesc( const struct NDb::SUIDesc *_pDesc );
};

// Atom EMART_SET_GLOBAL_VAR

class CARSetGlobalVar : public IMessageReactionB2
{
	OBJECT_BASIC_METHODS( CARSetGlobalVar );
	CDBPtr<NDb::SARSetGlobalVar> pDesc;
public:
	int operator&( IBinSaver &ss );
	virtual bool Execute( interface IScreen *pScreen, interface IScriptWrapper *pScript, interface IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags ) const;
	virtual void InitByDesc( const struct NDb::SUIDesc *_pDesc );
};

// Atom EMART_REMOVE_GLOBAL_VAR

class CARRemoveGlobalVar : public IMessageReactionB2
{
	OBJECT_BASIC_METHODS( CARRemoveGlobalVar );
	CDBPtr<NDb::SARRemoveGlobalVar> pDesc;
public:
	int operator&( IBinSaver &ss );
	virtual bool Execute( interface IScreen *pScreen, interface IScriptWrapper *pScript, interface IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags  ) const;
	virtual void InitByDesc( const struct NDb::SUIDesc *_pDesc );
};

class CARSendUIMessage : public IMessageReactionB2 
{
	OBJECT_BASIC_METHODS( CARSendUIMessage );
	CDBPtr<NDb::SARSendUIMessage> pDesc;
public:
	int operator&( IBinSaver &ss );
	virtual bool Execute( interface IScreen *pScreen, interface IScriptWrapper *pScript, interface IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags ) const;	
	virtual void InitByDesc( const struct NDb::SUIDesc *_pDesc );
};

class CARSendGameMessage : public IMessageReactionB2 
{
	OBJECT_BASIC_METHODS( CARSendGameMessage );
	CDBPtr<NDb::SARSendGameMessage> pDesc;
public:
	int operator&( IBinSaver &ss );
	virtual bool Execute( interface IScreen *pScreen, interface IScriptWrapper *pScript, interface IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags ) const;	
	virtual void InitByDesc( const struct NDb::SUIDesc *_pDesc );
};

class CARSiwtchTab : public IMessageReactionB2 
{
	OBJECT_BASIC_METHODS( CARSiwtchTab );
	CDBPtr<NDb::SARSwitchTab> pDesc;
public:
	int operator&( IBinSaver &ss );
	virtual bool Execute( interface IScreen *pScreen, interface IScriptWrapper *pScript, interface IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags ) const;	
	virtual void InitByDesc( const struct NDb::SUIDesc *_pDesc );
};



#endif // !defined(AFX_REACTION_H__C9D8977B_A116_4A9B_93A3_8EBE426CA74D__INCLUDED_)

