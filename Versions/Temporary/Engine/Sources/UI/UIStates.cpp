#include "StdAfx.h"
#include "uistates.h"
#include "UIScreen.h"



// CStates::


CStates::CStates( const NDb::SUIStateSequence &seq, const string &_szCmdName, const bool _bReversable, WORD _wKeyboardFlags ) 
: szCmdName( _szCmdName ), nCurIndex( 0 ), bForward( true ), bReversable( _bReversable ), bEnd( true ) ,
	wKeyboardFlags( _wKeyboardFlags )
{
	Reserve( seq.commands.size() );
	for ( vector<CDBPtr<NDb::SUIStateBase> >::const_iterator it	= seq.commands.begin(); it != seq.commands.end(); ++it )
		Add( *it );
	CheckEnd();
}

int CStates::operator&( IBinSaver &saver )
{
	saver.Add( 1, &states );
	saver.Add( 2, &nCurIndex );												// currently running effect
	saver.Add( 3, &bForward );												// effect direction 
	saver.Add( 4, &bEnd );														// all effects are finished
	saver.Add( 5, &pNotifySink );						// window that must be notified after
	saver.Add( 6, &bReversable );												// effect can be undone
	saver.Add( 7, &szCmdName );
	saver.Add( 8, &pContext );
	saver.Add( 9, &wKeyboardFlags );
	return 0;
}

void CStates::FastForward( const int timeDiff, class CWindowScreen *pScreen )
{
	Play( timeDiff, pScreen, true );
}

void CStates::NotifyParent()
{
	if ( IsValid(pNotifySink) )
		pNotifySink->NotifyStateSequenceFinished();
}

void CStates::CheckEnd()
{
	if ( bForward && nCurIndex >= states.size() )
	{
		nCurIndex = states.size() -1;
		bEnd = true;
		NotifyParent();
	}
	else if ( !bForward && nCurIndex < 0 )
	{
		nCurIndex = 0;
		bEnd = true;
		NotifyParent();
	}
	else if ( !states.empty() )
		bEnd = false;
}

void CStates::Add( const NDb::SUIStateBase *_pCmd )
{
//	NI_VERIFY( _pCmd, "No command", return );
	states.push_back( SUIState( _pCmd ) );
	CheckEnd();
}

void CStates::Advance() 
{ 
	NI_ASSERT( states.size() != 0, "no states" );
	bForward ? ++nCurIndex : --nCurIndex;
	CheckEnd();
}

const bool CStates::IsToBeDeleted() const 
{ 
	return IsEnd() &&	(!bForward || !bReversable );
}

void CStates::Reverse() 
{
	bForward = !bForward;
	CheckEnd();
	if ( !IsEnd() )													// notify every effect about reverce
	{
		for ( int i = 0; i < states.size(); ++i )
			if ( states[i].pEffect )
				states[i].pEffect->Reverse();
	}
}

void CStates::Segment( const int timeDiff, class CWindowScreen *pScreen )
{
	NI_ASSERT( !IsEnd(), "ended states, but segment called" );

	Play( timeDiff, pScreen, false );
}

void CStates::Play( const int timeDiff, class CWindowScreen *pScreen, const bool bFastForward )
{
	int timeAllowed = timeDiff;

	while( !IsEnd() && timeAllowed != 0 )
	{
		if ( GetCur().pEffect == 0 ) // command isn't launched yet
		{
			IUIEffector *pEff = CreateEffect( GetCur().pCmd, pScreen, pContext );
			if ( 0 == pEff )
				Advance();
			else
				GetCur().pEffect = pEff;
		}
		else if ( GetCur().pEffect->IsFinished() ) // effect is finished, advance to next command
			Advance();
		else if ( timeAllowed <= 0 )
			break;
		else
		{
			const int timeConsumed = GetCur().pEffect->Segment( timeAllowed, pScreen, bFastForward );
			NI_ASSERT( timeConsumed <= timeAllowed, "effect consumed more than was allowed" );
			timeAllowed -= timeConsumed;
		}
	}
}

IUIEffector *CStates::CreateEffect( const NDb::SUIStateBase *pCmd, class CWindowScreen *pScreen, SWindowContext *pContext )
{
	IUIEffector * pEff = CUIFactory::MakeEffect( pCmd, pScreen, pContext, id.second ? id.second->GetName() : 
		(pNotifySink ? pNotifySink->GetName() : "") );
//	NI_ASSERT( pEff != 0, "effect not created" );
	if ( pEff != 0 && pEff->IsFinished() )
	{
		delete pEff;
		return 0;
	}
	return pEff;
}

void CStates::SetAnimatedWindow( const int nID, class CWindow * pWindow )
{ 
	id.first = nID;
	id.second = pWindow; 
}


