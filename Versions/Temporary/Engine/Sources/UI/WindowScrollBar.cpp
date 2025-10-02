// WindowScrollBar.cpp: implementation of the CWindowScrollBar class.
//


#include "stdafx.h"
#include "WindowScrollBar.h"
#include "WindowSlider.h"
#include "WindowMSButton.h"
#include "InterfaceConsts.h"

REGISTER_SAVELOAD_CLASS(0x11075B89, CWindowScrollBar)

// Construction/Destruction

int CWindowScrollBar::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CWindow*>( this ) );
	saver.Add( 2, &pButtonLower );
	saver.Add( 3, &pButtonGreater );
	saver.Add( 4, &pSlider );
	saver.Add( 5, &bScrollGreater );													// timed scrolling up
	saver.Add( 6, &animTime );	
	saver.Add( 7, &bFirstTime );
	saver.Add( 8, &pShared );
	saver.Add( 9, &pInstance );

	return 0;
}

void CWindowScrollBar::AfterLoad()
{
	CWindow::AfterLoad();
	if ( IsValid( pButtonLower ) )
		pButtonLower->SetNotifySink( this );
	if ( IsValid( pButtonGreater ) )
		pButtonGreater->SetNotifySink( this );
}

void CWindowScrollBar::AllowMouseScrolling( const bool _bAllow )
{
	pSlider->AllowMouseScrolling( _bAllow );
}

void CWindowScrollBar::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	const NDb::SWindowScrollBar *pDesc( checked_cast<const NDb::SWindowScrollBar*>( _pDesc ) );
	pInstance = pDesc->Duplicate();
	CWindow::InitByDesc( _pDesc );

	pShared = checked_cast_ptr<const NDb::SWindowScrollBarShared *>( pDesc->pShared );
	pButtonLower = dynamic_cast<CWindowMSButton*>( CUIFactory::MakeWindow( pShared->pButtonLower ) );
	AddChild( pButtonLower, false );
	pButtonGreater = dynamic_cast<CWindowMSButton*>( CUIFactory::MakeWindow( pShared->pButtonGreater) );
	AddChild( pButtonGreater, false );
	pSlider = dynamic_cast<CWindowSlider*>( CUIFactory::MakeWindow( pShared->pSlider ) );
	AddChild( pSlider, false );
	// by default slider notifyes us about changes:
	pSlider->SetNotifySink( this );
}

bool CWindowScrollBar::IsHorisontal() const 
{ 
	return pSlider->IsHorisontal(); 
}

void CWindowScrollBar::Init()
{
	GetScreen()->RegisterToSegment( this, false );
	CWindow::Init();
}

bool CWindowScrollBar::IsLeverVisible() const
{
	return pSlider->IsLeverVisible();
}

void CWindowScrollBar::Reposition( const CTRect<float> &parentRect )
{
	//
	if ( !pSlider )
		pSlider = dynamic_cast<CWindowSlider*>( GetChild( "Slider", false ) );
	NI_ASSERT( pSlider != 0, "scrollbar must have \"Slider\"" );

	if ( !IsValid( pButtonGreater ) )
		pButtonGreater = dynamic_cast<CWindowMSButton*>( GetChild( "ButtonGreater", false ) );

	if ( !IsValid( pButtonLower ) )
		pButtonLower = dynamic_cast<CWindowMSButton*>( GetChild( "ButtonLower", false ) );

	CWindow::Reposition( parentRect );

	if ( IsValid( pButtonGreater ) )
		pButtonGreater->SetNotifySink( this );
	if ( IsValid( pButtonLower ) )
		pButtonLower->SetNotifySink( this );
	UpdateButtons();
}

void CWindowScrollBar::Segment( const int timeDiff )
{
	animTime += timeDiff;
	if ( animTime > (bFirstTime ? CInterfaceConsts::SCROLLER_ANIM_TIME_1() :
															  CInterfaceConsts::SCROLLER_ANIM_TIME()) )
	{
		bFirstTime = false;
		Scroll( pShared->fSpeed * animTime / 1000 );
		animTime = 0;
	}
}

void CWindowScrollBar::Scroll( const float fDist )
{
	pSlider->SetPos( pSlider->GetPos() + ( bScrollGreater ? fDist : -fDist ) );
}

void CWindowScrollBar::Released( class CWindow *pWho )
{
	GetScreen()->RegisterToSegment( this, false );
}

void CWindowScrollBar::Entered( class CWindow *pWho )
{
	if ( !GetScreen()->IsRegisteredToSegment( this ) )
	{
		IButton *pB = dynamic_cast<IButton*>( pWho );
		if ( pB && pB->IsPushed() )
		{
			GetScreen()->RegisterToSegment( this, true );
			Scroll();
		}
	}
}

void CWindowScrollBar::Leaved( class CWindow *pWho )
{
	GetScreen()->RegisterToSegment( this, false );
}

void CWindowScrollBar::Pushed( class CWindow *pWho )
{
	if ( pSlider->GetNSpecialPositions() > 1 )
	{
		bScrollGreater  = (pWho == pButtonGreater);
		// discrete slider, move discrete
		pSlider->SetSpecialPosition( pSlider->GetCurrentSpecialPosition() + (bScrollGreater ? 1 : -1) );
	}
	else
	{
		bFirstTime = true;
		animTime = 0;
		GetScreen()->RegisterToSegment( this, true );
		bScrollGreater  = (pWho == pButtonGreater);
		Scroll();
	}
}

void CWindowScrollBar::SetNotifySink( interface ISliderNotify *_pNotifySink ) 
{ 
	pSlider->SetNotifySink( _pNotifySink ); 
}

void CWindowScrollBar::SetRange( const float fMin, const float fMax, const float fPageSize )
{
	pSlider->SetRange( fMin, fMax, fPageSize );
	UpdateButtons();
}

void CWindowScrollBar::UpdateButtons()
{
	const bool bLever = pSlider->IsLeverVisible();
	if ( IsValid( pButtonGreater ) )
		pButtonGreater->Enable( bLever );
	if ( IsValid( pButtonLower ) )
		pButtonLower->Enable( bLever );
}

void CWindowScrollBar::GetRange( float *pMax, float *pMin ) const
{
	pSlider->GetRange( pMax, pMin );
}

void CWindowScrollBar::SetPos( const float fCur )
{
	pSlider->SetPos( fCur );
}

float CWindowScrollBar::GetPos() const
{
	return pSlider->GetPos();
}

void CWindowScrollBar::SliderPosition( const float fPosition, class CWindow *pWho )
{
	RunAnimationAndCommands( pInstance->effects, "", false, false );
}

int CWindowScrollBar::GetNSpecialPositions()
{
	return pSlider->GetNSpecialPositions();
}

int CWindowScrollBar::GetCurrentSpecialPosition() const
{
	return pSlider->GetCurrentSpecialPosition();
}

void CWindowScrollBar::SetNSpecialPositions( int nPositions )
{
	pSlider->SetNSpecialPositions( nPositions );
}

void CWindowScrollBar::SetSpecialPosition( int nPosition )
{
	pSlider->SetSpecialPosition( nPosition );
}


