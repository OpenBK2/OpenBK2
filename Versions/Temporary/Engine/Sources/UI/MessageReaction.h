#pragma once
// Reaction.h: interface for the CReaction class.
//

#include "IMessageReaction.h"

#include <cstdint>

typedef std::vector<CPtr<IMessageReactionB2>> CMessageSequence;
typedef std::vector<CDBPtr<NDb::SUIDesc>> CMessageSequienceDesc;

// 1 check (branches) and sequience of atim reactions for each branch
class CMessageReactionB2 : public IMessageReactionB2
{
	OBJECT_BASIC_METHODS( CMessageReactionB2 );
	typedef std::unordered_map<int /*custom check return*/, CMessageSequence> CMessageSequences;

	CPtr<IMessageCheck> pCheck;
	CMessageSequences branches;
	CMessageSequence commonBefore; // always run and before any branch
	CMessageSequence commonAfter;  // always run and after any branch

	bool Execute( const CMessageSequence *pToExecute,
	              struct IScreen *pScreen,
	              struct IScriptWrapper *pScript,
	              struct IProgrammedReactionsAndChecks *pProg,
	              uint16_t wKeyboardFlags ) const;
	void InitSequienceByDesc( CMessageSequence *pCreate, const CMessageSequienceDesc &src );

public:
	CMessageReactionB2() {}

	bool Execute( struct IScreen *pScreen,
	              struct IScriptWrapper *pScript,
	              struct IProgrammedReactionsAndChecks *pProg,
	              uint16_t wKeyboardFlags ) const override;
	int operator&( IBinSaver &ss ) override;

	void AddCommonBefore( IMessageReactionB2 *pReaction )
	{
		commonBefore.push_back( pReaction );
	}

	void InitByDesc( const struct NDb::SUIDesc *_pDesc ) override;
};

// Atom EMART_SET_GLOBAL_VAR

class CARSetGlobalVar : public IMessageReactionB2
{
	OBJECT_BASIC_METHODS( CARSetGlobalVar );
	CDBPtr<NDb::SARSetGlobalVar> pDesc;

public:
	int operator&( IBinSaver &ss ) override;
	bool Execute( struct IScreen *pScreen,
	              struct IScriptWrapper *pScript,
	              struct IProgrammedReactionsAndChecks *pProg,
	              uint16_t wKeyboardFlags ) const override;
	void InitByDesc( const struct NDb::SUIDesc *_pDesc ) override;
};

// Atom EMART_REMOVE_GLOBAL_VAR

class CARRemoveGlobalVar : public IMessageReactionB2
{
	OBJECT_BASIC_METHODS( CARRemoveGlobalVar );
	CDBPtr<NDb::SARRemoveGlobalVar> pDesc;

public:
	int operator&( IBinSaver &ss ) override;
	bool Execute( struct IScreen *pScreen,
	              struct IScriptWrapper *pScript,
	              struct IProgrammedReactionsAndChecks *pProg,
	              uint16_t wKeyboardFlags ) const override;
	void InitByDesc( const struct NDb::SUIDesc *_pDesc ) override;
};

class CARSendUIMessage : public IMessageReactionB2
{
	OBJECT_BASIC_METHODS( CARSendUIMessage );
	CDBPtr<NDb::SARSendUIMessage> pDesc;

public:
	int operator&( IBinSaver &ss ) override;
	bool Execute( struct IScreen *pScreen,
	              struct IScriptWrapper *pScript,
	              struct IProgrammedReactionsAndChecks *pProg,
	              uint16_t wKeyboardFlags ) const override;
	void InitByDesc( const struct NDb::SUIDesc *_pDesc ) override;
};

class CARSendGameMessage : public IMessageReactionB2
{
	OBJECT_BASIC_METHODS( CARSendGameMessage );
	CDBPtr<NDb::SARSendGameMessage> pDesc;

public:
	int operator&( IBinSaver &ss ) override;
	bool Execute( struct IScreen *pScreen,
	              struct IScriptWrapper *pScript,
	              struct IProgrammedReactionsAndChecks *pProg,
	              uint16_t wKeyboardFlags ) const override;
	void InitByDesc( const struct NDb::SUIDesc *_pDesc ) override;
};

class CARSiwtchTab : public IMessageReactionB2
{
	OBJECT_BASIC_METHODS( CARSiwtchTab );
	CDBPtr<NDb::SARSwitchTab> pDesc;

public:
	int operator&( IBinSaver &ss ) override;
	bool Execute( struct IScreen *pScreen,
	              struct IScriptWrapper *pScript,
	              struct IProgrammedReactionsAndChecks *pProg,
	              uint16_t wKeyboardFlags ) const override;
	void InitByDesc( const struct NDb::SUIDesc *_pDesc ) override;
};
