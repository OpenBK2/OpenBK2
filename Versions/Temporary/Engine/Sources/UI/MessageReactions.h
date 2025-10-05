// MessageReactions.h: interface for the CMessageReactions class.
//



#pragma once
#include "Script/ScriptWrapper.h"

struct IMessageReactionB2;

// class contains message reactions
// that may be launched.
// Message Reaction is a set basic actions 
class CMessageReactions 
{
	typedef std::unordered_map<std::string, CPtr<IMessageReactionB2> > CReactions;
	CReactions reactions;

	// script that does all complex checks and complex behaviour
	CPtr<IScriptWrapper> pScript;

	void InitScript( const std::string &szScriptFileName );
	void RunScriptText( const std::string &szScriptBody );
public:
	CMessageReactions() {  }
	void InitByDesc( const NDb::SMessageReactionsDesc &instance );
	bool Execute( const std::string &szSender, const std::string &szReactionKey, struct IScreen *pScreen, struct IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags );
	bool Execute( const std::string &szSender, const NDb::SUIDesc *pReaction, struct IScreen *pScreen, struct IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags );
	void Register( const std::string &szReactionKey, IMessageReactionB2 *pReaction );
	int operator&( IBinSaver &saver );
};



