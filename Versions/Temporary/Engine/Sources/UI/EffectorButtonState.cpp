#include "stdafx.h"
#include "EffectorButtonState.h"
#include "WindowMSButton.h"

REGISTER_SAVELOAD_CLASS(0x170AE341,CEffectorButtonState)

int CEffectorButtonState::operator&( IBinSaver &saver )
{
	saver.Add( 1, &bFinished );
	saver.Add( 2, &bForward );
	saver.Add( 3, &fWaitTime );
	saver.Add( 4, &fElapsedTime );
	saver.Add( 5, &pWindow );
	saver.Add( 7, &eSubstate );
	saver.Add( 8, &bStarted );
	return 0;
}

bool CEffectorButtonState::IsFinished() const 
{ 
	return bFinished; 
}

void CEffectorButtonState::Configure( const NDb::SUIStateBase *_pCmd, struct IScreen *pScreen, SWindowContext *pContext, const std::string &szAnimatedWindow )
{ 
	const NDb::SUISButtonSubstate *pCmd( checked_cast<const NDb::SUISButtonSubstate*>( _pCmd ) );
	eSubstate = pCmd->eSubstate;
	fWaitTime = pCmd->fWaitTime;
	fElapsedTime = 0;
	pWindow = dynamic_cast<CWindow*>( pScreen->GetVisibleElement( 
		szAnimatedWindow.empty() ? pCmd->szButton.first : szAnimatedWindow, true ) );
	bStarted = false;

	// NI_ASSERT( pWindow != 0, StrFmt( "no window (button) \"%s\"", pCmd->szButton.first.c_str() ) );
	if ( pWindow == 0 )
	{
		bFinished = true;
		bForward = true;
		return;
	}

	NI_ASSERT( pCmd->szButton.IsValid(), "button is invalid" );

	bFinished = !pWindow->IsEnabled();
	bForward = true;
}

const int CEffectorButtonState::Segment( const int timeDiff, struct IScreen *pScreen, const bool bFastForward )
{
	if ( !bStarted )
	{
		CWindowMSButton *pButton = dynamic_cast_ptr<CWindowMSButton*>( pWindow );
		NI_ASSERT( pButton != 0, "window is not a button" );
		pButton->SetEffectSubState( eSubstate, true );
		bStarted = true;
	}
	
	int nConsumedTime = timeDiff;

	fElapsedTime += timeDiff;
	if ( fElapsedTime >= fWaitTime )
	{
		nConsumedTime -= fElapsedTime - fWaitTime;
		CWindowMSButton *pButton = dynamic_cast_ptr<CWindowMSButton*>( pWindow );
		NI_ASSERT( pButton != 0, "window is not a button" );
		pButton->SetEffectSubState( eSubstate, false );
		bFinished = true;
	}

	return nConsumedTime;
}

void CEffectorButtonState::Reverse()
{
	bForward = !bForward;
	bFinished = false;
}


