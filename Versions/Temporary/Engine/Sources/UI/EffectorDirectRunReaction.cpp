#include "stdAfx.h"
#include "EffectorDirectRunReaction.h"

REGISTER_SAVELOAD_CLASS(0x210C5AC0,CEffectorDirectRunReaction)

bool CEffectorDirectRunReaction::IsFinished() const 
{ 
	return bFinished; 
}

void CEffectorDirectRunReaction::Configure( const NDb::SUIStateBase *_pCmd, struct IScreen *pScreen, SWindowContext *pContext, const string &_szAnimatedWindow )
{ 
	szAnimatedWindow = _szAnimatedWindow;
	const NDb::SUISDirectRunReaction *pCmd( checked_cast<const NDb::SUISDirectRunReaction*>( _pCmd ) );
	pReactionForward = pCmd->pReactionForward;
	pReactionBackward = pCmd->pReactionBackward;
	bFinished = false;
	bForward = true;
}

const int CEffectorDirectRunReaction::Segment( const int timeDiff, struct IScreen *pScreen, const bool bFastForward )
{ 
	// to do RUN COMMAND cmd.szParam
	// in messageReaction
	// this effect is instant
	if ( bForward )
	{
		if ( pReactionForward )
		{
			pScreen->RunReaction( szAnimatedWindow, pReactionForward );
			//pReactionForward->Execute()
		}
		//const string &szReactionName = bForward ? szFwd : szBack;
		//pScreen->RunReaction( szAnimatedWindow, szReactionName );
	}
	else
	{
		if ( pReactionBackward )
		{
			pScreen->RunReaction( szAnimatedWindow, pReactionBackward );
		}
	}
	bFinished = true;
	return 0;
}

void CEffectorDirectRunReaction::Reverse()
{
	bForward = !bForward;
	bFinished = false;
}


