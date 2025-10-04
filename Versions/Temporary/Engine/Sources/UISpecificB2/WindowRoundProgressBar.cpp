#include "stdafx.h"
#include "UI/commandparam.h"
#include "UI/dbuserinterface.h"
#include "UI/UI.h"
#include "Input/gamemessage.h"
#include "Misc/2Darray.h"
#include "System/Dg.h"
#include "WindowRoundProgressBar.h"

#include <zconf.h>

// CWindowRoundProgressBar

CWindowRoundProgressBar::CWindowRoundProgressBar() : fPosition (0), fStartAngle(-1), fFinishAngle(-1)
{
}

void CWindowRoundProgressBar::Visit( struct IUIVisitor *pVisitor )
{
/*	CTRect<float> rc;
	FillWindowRect( &rc );
	VirtualToScreen( rc, &rc );
	CClipStore s( pVisitor, rc );*/

	CWindow::Visit( pVisitor );
	
	roundVisitor.SetPlacement( GetWindowRect() );
	roundVisitor.Visit( pVisitor );
	roundVisitor2.SetPlacement( GetWindowRect() );
	roundVisitor2.Visit( pVisitor );
}

void CWindowRoundProgressBar::InitByDesc( const struct NDb::SUIDesc *pDesc )
{
	pInstance = checked_cast<const NDb::SWindowRoundProgressBar*>( pDesc )->Duplicate();
	pShared = checked_cast_ptr<const NDb::SWindowRoundProgressBarShared*>( pInstance->pShared );
	if ( pShared )
	{
		roundVisitor.SetTexture( pShared->pTexture );
		roundVisitor.SetColor( pShared->nColor );
		roundVisitor2.SetTexture( pShared->pTexture );
		roundVisitor2.SetColor( pShared->nColor );
	}

	CWindow::InitByDesc( pDesc );
}

#define MIN_DIFFERENCE 0.1f
void CWindowRoundProgressBar::SetAngles( float _fStartAngle, float _fFinishAngle )
{
	// CRAP - should fix CTextureRoundSegmentVisitor

	float fTotalDiff = 0.0f;			// Total change in values; if too small - ignore
	fTotalDiff += fabs( fStartAngle - _fStartAngle );
	fTotalDiff += fabs( fFinishAngle - _fFinishAngle );

	if ( fTotalDiff < MIN_DIFFERENCE )
	{
		return;
	}
	else
	{
		fStartAngle = _fStartAngle;
		fFinishAngle = _fFinishAngle;
	}

	float fStartAngle = _fStartAngle;
	if ( fStartAngle > fFinishAngle )
		fStartAngle -= FP_2PI;
	float fAngle = (fStartAngle + fFinishAngle) * 0.5f;
	roundVisitor.SetAngles( fStartAngle, fAngle );
	roundVisitor2.SetAngles( fAngle, fFinishAngle );
}

void CWindowRoundProgressBar::SetPosition( float fPos ) // considered full circle
{	
	fPosition = fPos;
	SetAngles( 0., fPosition * FP_2PI );
}

float CWindowRoundProgressBar::GetPosition() const
{
	return fPosition;
}

REGISTER_SAVELOAD_CLASS(0x171713C2, CWindowRoundProgressBar)


