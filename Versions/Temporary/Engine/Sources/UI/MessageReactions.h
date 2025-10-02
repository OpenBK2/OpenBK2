// MessageReactions.h: interface for the CMessageReactions class.
//



#pragma once
#include "../Script/ScriptWrapper.h"

interface IMessageReactionB2;

// class contains message reactions
// that may be launched.
// Message Reaction is a set basic actions 
class CMessageReactions 
{
	typedef hash_map<string, CPtr<IMessageReactionB2> > CReactions;
	CReactions reactions;

	// script that does all complex checks and complex behaviour
	CPtr<IScriptWrapper> pScript;

	void InitScript( const string &szScriptFileName );
	void RunScriptText( const string &szScriptBody );
public:
	CMessageReactions() {  }
	void InitByDesc( const NDb::SMessageReactionsDesc &instance );
	bool Execute( const string &szSender, const string &szReactionKey, interface IScreen *pScreen, interface IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags );
	bool Execute( const string &szSender, const NDb::SUIDesc *pReaction, interface IScreen *pScreen, interface IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags );
	void Register( const string &szReactionKey, IMessageReactionB2 *pReaction );
	int operator&( IBinSaver &saver );
};



