// WindowSlider.cpp: implementation of the CWindowSlider class.
//


#include "stdafx.h"
#include "WindowSlider.h"
#include "WindowMSButton.h"
#include "InterfaceConsts.h"

REGISTER_SAVELOAD_CLASS(UI, 0x11075B8A, CWindowSlider)

// CWindowSlider

int CWindowSlider::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CWindow*>( this ) );
	saver.Add( 2, &pShared );
	saver.Add( 3, &fMin );
	saver.Add( 4, &fMax );
	saver.Add( 5, &fPageSize );														// 
	saver.Add( 6, &fCur );
	saver.Add( 7, &fPickOffset );												// when slider is picked no on center
	saver.Add( 8, &bManualScrolling );
	saver.Add( 9, &bPressed );													// remember pressed state, scrolls by timer
	saver.Add( 10, &bFirstTime );												// first scroll should will wait for a longer time
	saver.Add( 11, &animTime );
	saver.Add( 12, &vPressedPos );
	saver.Add( 13, &bFastScrollForward );
	saver.Add( 14, &bFastScrolling );
	saver.Add( 15, &pLever );
	saver.Add( 16, &pShared );
	saver.Add( 17, &pInstance );
	saver.Add( 18, &pNotifySink );
	saver.Add( 19, &bMouseScrollingAllowed  );
	saver.Add( 20, &bLeverOn );
	return 0;
}

void CWindowSlider::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	const NDb::SWindowSlider *pDesc ( checked_cast<const NDb::SWindowSlider*>( _pDesc ) );
	pInstance = pDesc->Duplicate();

	CWindow::InitByDesc( _pDesc );
	pShared = checked_cast_ptr<const NDb::SWindowSliderShared *>( pDesc->pShared );

	//{ CRAP - temporary
	IWindow *pWnd = GetChild( "Lever", false );
	if ( pWnd )
		pWnd->ShowWindow( false );
	//}

	pLever = dynamic_cast<CWindowMSButton*>( CUIFactory::MakeWindow( pShared->pLever ) );
	NI_ASSERT( pLever, StrFmt( "Lever not found for slider: \"%s\"", pInstance->szName.c_str() ) );
	AddChild( pLever, false );	
}

void CWindowSlider::UpdatePos()
{
	fCur = VerifyPos( fCur );
	
	int nX, nY, nW, nH;
	GetPlacement( &nX, &nY, &nW, &nH );

	// reposition lever according to page size
	int nLeverX, nLeverY, nLeverW, nLeverH;

	if ( pShared->bHorisontal )
	{
		nLeverW = Clamp( fPageSize * nW / ( fMax - fMin + fPageSize ), pShared->fMinLeverSize, pShared->fMaxLeverSize == 0 ? nW : pShared->fMaxLeverSize );
		if ( fMax - fMin >= 1.0f )
			nLeverX = ( nW - nLeverW ) * fCur / ( fMax - fMin );
		else
			nLeverX = 0;
		pLever->SetPlacement( nLeverX, 0, nLeverW, 0,  EWPF_POS_X|EWPF_SIZE_X );
	}
	else
	{
		nLeverH = Clamp( fPageSize * nH / ( fMax - fMin + fPageSize ), pShared->fMinLeverSize, pShared->fMaxLeverSize == 0 ? nH : pShared->fMaxLeverSize );
		if ( fMax - fMin >= 1.0f )
			nLeverY = ( nH - nLeverH ) * fCur / ( fMax - fMin );
		else
			nLeverY = 0;
		pLever->SetPlacement( 0, nLeverY, 0, nLeverH,  EWPF_POS_Y|EWPF_SIZE_Y );
	}
	// notify parent about position change
	if ( pNotifySink )
		pNotifySink->SliderPosition( fCur, this );
}

void CWindowSlider::SetRange( const float _fMin, const float _fMax, const float _fPageSize  )
{
	fMin = (std::min)( _fMin, _fMax );
	fMax = (std::max)( _fMax, _fMin );

	fPageSize = _fPageSize > 0 ? _fPageSize : fMax - fMin;

	if ( fMin == fMax )
	{
		fMin = 0;
		fMax = 100;
		fPageSize = 100;
		fCur = 0;
	}
	if ( fPageSize >= fMax - fMin )
	{
		fPageSize = fMax - fMin;
		pLever->ShowWindow( false );
		bLeverOn = false;
	}
	else
	{
		pLever->ShowWindow( true );
		bLeverOn = true;
	}
	fMax -= fPageSize;

	IScreen * pScreen = GetScreen();
	if ( pScreen )
		pScreen->RegisterToSegment( this, bMouseScrollingAllowed && fMax > fMin );

	UpdatePos();
}

void CWindowSlider::AllowMouseScrolling( const bool _bAllow )
{ 
	if ( bMouseScrollingAllowed != _bAllow )
	{
		bMouseScrollingAllowed = _bAllow; 
		GetScreen()->RegisterToSegment( this, bMouseScrollingAllowed && fMax > fMin );
	}
}

bool CWindowSlider::NeedHandleMouseScrolling() const
{
	return bMouseScrollingAllowed && fMax > fMin;
}

bool CWindowSlider::IsLeverVisible() const
{
	return bLeverOn;
}

void CWindowSlider::GetRange( float *pMax, float *pMin ) const
{
	*pMin = fMin;
	*pMax = fMax;
}

void CWindowSlider::SetPos( const float _fCur )
{
	fCur = _fCur;
	UpdatePos();
}

float CWindowSlider::GetPos() const
{
	return fCur;
}

void CWindowSlider::Reposition( const CTRect<float> &parentRect )
{
	if ( !pLever )
		pLever = dynamic_cast<CWindowMSButton*>( GetChild( "Lever", false ) );
	NI_ASSERT( pLever != 0, "slider without lever" );
	
	int nW, nH;
	GetPlacement( 0, 0, &nW, &nH );
	if ( pShared->bHorisontal )
		pLever->SetPlacement( 0, 0, 0, nH, EWPF_SIZE_Y );
	else
		pLever->SetPlacement( 0, 0, nW, 0, EWPF_SIZE_X );

	// set lever width or height
	CWindow::Reposition( parentRect );
	SetRange( fMin, fMax + fPageSize, fPageSize );
	SetPos( fCur );
}

void CWindowSlider::OnKeyUp( const struct SGameMessage &msg )
{
	if ( !pShared->bHorisontal )
	{
		if ( pInstance->nSpecialPositions > 1 )
			fCur -= ( fMax - fMin ) / ( pInstance->nSpecialPositions );
		else
			--fCur;
		UpdatePos();
	}
}

void CWindowSlider::OnKeyDown( const struct SGameMessage &msg )
{
	if ( !pShared->bHorisontal )
	{
		if ( pInstance->nSpecialPositions > 1 )
			fCur += ( fMax - fMin ) / ( pInstance->nSpecialPositions );
		else
			++fCur;
		UpdatePos();
	}
}

void CWindowSlider::OnKeyPgDn( const struct SGameMessage &msg )
{
	fCur += ( fMax - fMin ) / ( pInstance->nSpecialPositions );
	UpdatePos();
}

void CWindowSlider::OnKeyPgUp( const struct SGameMessage &msg )
{
	fCur -= ( fMax - fMin ) / ( pInstance->nSpecialPositions );
	UpdatePos();
}

void CWindowSlider::OnKeyHome( const struct SGameMessage &msg )
{
	fCur = fMin;
	UpdatePos();
}

void CWindowSlider::OnKeyEnd( const struct SGameMessage &msg )
{
	fCur = fMax - 1;
	UpdatePos();
}

void CWindowSlider::OnKeyRight( const struct SGameMessage &msg )
{
	if ( pShared->bHorisontal )
	{
		if ( pInstance->nSpecialPositions > 1 )
			fCur += ( fMax - fMin ) / ( pInstance->nSpecialPositions );
		else
			++fCur;
		UpdatePos();
	}
}

void CWindowSlider::OnKeyLeft( const struct SGameMessage &msg )
{
	if ( pShared->bHorisontal )
	{
		if ( pInstance->nSpecialPositions > 1 )
			fCur -= ( fMax - fMin ) / ( pInstance->nSpecialPositions );
		else
			--fCur;
		UpdatePos();
	}
}

bool CWindowSlider::OnMouseMove( const CVec2 &vPos, const int nButton )
{
	CWindow::OnMouseMove( vPos, nButton );

	if ( nButton & MSTATE_BUTTON1 )
	{
		// move lever with mouse
		const char *pszPressed = GetPressedName( MSTATE_BUTTON1 );
		if ( bManualScrolling )
		{
			SetPos( CalcPressedPos( vPos, fPickOffset ) );
		}
		else if ( pszPressed && pLever->GetName() == pszPressed )
		{
			// update position
			SetPos( CalcPressedPos( vPos ) );
		}
		else if ( Pick( vPos, false ) == pLever )			// lever is under cursor
		{
			// stop fast scrolling
			bFastScrolling = false;
		}
		else if ( IsInside( vPos ) )
		{
			vPressedPos = vPos;
			bFastScrolling = true;
		}
	}
	return false;
}

float CWindowSlider::VerifyPos( const float _fCur ) const
{
	float fTriedCur = _fCur;
	if ( fTriedCur < fMin ) 
		return fMin;
	if ( fTriedCur > fMax ) 
		return fMax;
	return fTriedCur;
}

void CWindowSlider::SetNSpecialPositions( int nPositions )
{
//	NI_VERIFY( nPositions > 1, "positions must be > 1", return );
	pInstance->nSpecialPositions = (nPositions > 1) ? nPositions : 0;
	SetPos( 0.0f );
}

int CWindowSlider::GetNSpecialPositions()
{
	return pInstance->nSpecialPositions;
}

int CWindowSlider::GetCurrentSpecialPosition() const
{
	return ( fCur / ( fMax - fMin ) * ( pInstance->nSpecialPositions -1  ) );
}

void CWindowSlider::SetSpecialPosition( int nPosition )
{
	if ( pInstance->nSpecialPositions > 1 )
		SetPos( nPosition * ( fMax - fMin ) / ( pInstance->nSpecialPositions -1 ) );
}

float CWindowSlider::CalcPressedPos( const CVec2 &vPos, const float fPosOffset ) const
{
	CTRect<float> rect;
	FillWindowRect( &rect );
	
	int nLeverX, nLeverY, nLeverW, nLeverH;
	pLever->GetPlacement( &nLeverX, &nLeverY, &nLeverW, &nLeverH );
	
	const float fOffset = pShared->bHorisontal ? vPos.x - fPosOffset - rect.left: vPos.y - fPosOffset - rect.top;
	const float fSize = pShared->bHorisontal ? rect.Width() - nLeverW : rect.Height() - nLeverH;
	int &nSpecialPositions = pInstance->nSpecialPositions;


	if ( nSpecialPositions > 1 )
	{		
		float fPortion = fOffset / fSize;
		if ( fPortion < 0 )
			fPortion = 0;
		if ( fPortion > 1 )
			fPortion = 1;
		int nPortion = ( nSpecialPositions - 1 ) * fPortion;
		float fRest = fPortion * ( nSpecialPositions - 1 ) - nPortion;
		if ( fRest > 0.5 )
			nPortion++;
		return nPortion * ( fMax - fMin ) / ( nSpecialPositions - 1 );		
	}
	return fOffset / fSize * ( fMax - fMin );
}

float CWindowSlider::CalcPickOffset( const CVec2 &vPos ) const
{
	CTRect<float> rect;
	FillWindowRect( &rect );

	int nLeverX, nLeverY;	
	pLever->GetPlacement( &nLeverX, &nLeverY, 0, 0 );

	return (pShared->bHorisontal ? vPos.x - nLeverX - rect.left : vPos.y - nLeverY - rect.top );
}

bool CWindowSlider::OnButtonDown( const CVec2 &vPos, const int nButton )
{
	CWindow::OnButtonDown( vPos, nButton );
	
	const char *pszPressed = GetPressedName( MSTATE_BUTTON1 );
	if ( pszPressed && pLever->GetName() == pszPressed )
	{
		bManualScrolling = true;
		fPickOffset = CalcPickOffset( vPos );
	}
	else if ( IsInside( vPos ) )
	{
		bManualScrolling = false;
		bFirstTime = true;
		bPressed = true;
		animTime = 0;
		vPressedPos = vPos;
		bFastScrollForward = VerifyPos( CalcPressedPos( vPos ) ) > fCur;
		bFastScrolling = true;
		ScrollFast();
		GetScreen()->RegisterToSegment( this, true );
	}
	else
		return false;

	return true;
}

void CWindowSlider::ScrollFast()
{
	if ( ( VerifyPos( CalcPressedPos( vPressedPos ) ) > fCur ) == bFastScrollForward )
	{
		if ( pInstance->nSpecialPositions > 1 )
			SetSpecialPosition( GetCurrentSpecialPosition() + (bFastScrollForward ? 1 : -1) );
		else
		{
			fCur += bFastScrollForward ? fPageSize : -fPageSize;
			UpdatePos();
		}
	}
}

bool CWindowSlider::ProcessEvent( const struct SGameMessage &msg )
{
	NInput::CBind b("mouse_wheel_scroll");
	if( b.ProcessEvent(msg) )
	{
		int nFunk;
		nFunk = 0;
	}
	if ( NeedHandleMouseScrolling() && inputScroll.ProcessEvent( msg ) )
		return true;
	return CWindow::ProcessEvent( msg );
}

void CWindowSlider::Segment( const int timeDiff )
{
	const float fDelta = inputScroll.GetDelta();
	if ( pInstance->nSpecialPositions > 1 )
		return;
	if ( fDelta != 0 )
	{
		fCur += fPageSize * fDelta;
		UpdatePos();
	}
	if ( bPressed && bFastScrolling )
	{
		animTime += timeDiff;
		
		if ( Pick( vPressedPos, false ) == pLever )
		{
			animTime = 0;
		}
		else if ( animTime > (bFirstTime ? CInterfaceConsts::SCROLLER_ANIM_TIME_1() : CInterfaceConsts::SCROLLER_ANIM_TIME()) )
		{
			ScrollFast();	
			animTime = 0;
			bFirstTime = false;
		}
	}
}

bool CWindowSlider::OnButtonUp( const CVec2 &vPos, const int nButton )
{
	CWindow::OnButtonUp( vPos, nButton );
	GetScreen()->RegisterToSegment( this, NeedHandleMouseScrolling() );
	bManualScrolling = false;
	return false;
}


