#include "stdafx.h"
#include "windowtooltip.h"
#include "System/Text.h"

REGISTER_SAVELOAD_CLASS( UI, 0x11075B8D, CWindowTooltip)

void CWindowTooltip::InitByDesc( const struct NDb::SUIDesc* pDesc )
{
	pInstance = checked_cast<const NDb::SWindowTooltip*>( pDesc )->Duplicate();
	pShared = checked_cast_ptr<const NDb::SWindowTooltipShared*>( pInstance->pShared );
	CWindow::InitByDesc( pDesc );
}

void CWindowTooltip::InitTooltip( const CVec2 &vPos, const CTRect<float> &wndRect, const std::wstring &_szText, IScreen *_pScreen,
																	const int nTooltipWidth, const float fHorisontalToVerticalRatio, int nIDForMLHandler )
{
	wszText = _szText;
	
	std::wstring szText;
	if ( const NDb::SForegroundTextString *pForeground = pInstance->pTextString )
	{
		if ( pForeground->pShared && CHECK_TEXT_NOT_EMPTY_PRE(pForeground->pShared->,FormatString) )
			szText = GET_TEXT_PRE(pForeground->pShared->,FormatString); // format
	}
	szText += _szText;
	
	CWindow * pChild = dynamic_cast<CWindow*>(GetChild( "TooltipText", false ));
	ITextView *pText = dynamic_cast<ITextView*>( pChild );
	NI_ASSERT( pText != 0, StrFmt( "tooltip window must have TextView child" ) );
	
	CWindow *pScreen = dynamic_cast<CWindow*>( _pScreen );
	if ( pText && pScreen )
	{
		bInitializedByText = true;

		pText->SetIDForMLHandler( nIDForMLHandler );
		pChild->SetPlacement( 0, 0, nTooltipWidth, 0, EWPF_SIZE_X );
		pText->SetText( pText->GetDBText() + szText );

		CTPoint<int> size = pText->GetSize();
		pText->SetWidth( size.x );

		int nSizeX, nSizeY;
		pChild->GetPlacement( 0, 0, &nSizeX, &nSizeY );
		pScreen->AddChild( this, true );
		
		const int nW = nSizeX + pShared->vLowerBorder.x + pShared->vHigherBorder.x;
		const int nH = nSizeY + pShared->vLowerBorder.y + pShared->vHigherBorder.y;

		int nScreenW, nScreenH;
		pScreen->GetPlacement( 0, 0, &nScreenW, &nScreenH );

		// position inside screen
		// adjust X
		nMaxXPos = (std::max)( 0, nScreenW - nW - 3 );

		// adjust Y
		// if it is possible - tooltip window is above window that tooltip is about
		nDesiredYPos = wndRect.top - nH - 3;

		SetPlacement( vPos.x, vPos.y, nW, nH, EWPF_ALL );
		NGlobal::SetVar( "TOOLTIP_X", (int)vPos.x );
		NGlobal::SetVar( "TOOLTIP_Y", (int)vPos.y );

		NGlobal::SetVar( "TOOLTIP_ENABLED", true );
	}
	else
		NGlobal::RemoveVar( "TOOLTIP_ENABLED" );
}

void CWindowTooltip::AdjustPosByMousePos( const CVec2 &vMousePos )
{
	int nX, nY, nSizeX, nSizeY;
	GetPlacement( &nX, &nY, &nSizeX, &nSizeY );
	
	if ( bInitializedByText )
	{
		nX = Clamp( int(vMousePos.x), 0, nMaxXPos );
		nY = nDesiredYPos;
		if ( nY < 0 )
			nY = vMousePos.y + 32;
	}
	else
	{
		nX = vMousePos.x;
		nY = vMousePos.y;
	}

	SetPlacement( nX, nY, nSizeX, nSizeY, EWPF_ALL );
	NGlobal::SetVar( "TOOLTIP_X", nX );
	NGlobal::SetVar( "TOOLTIP_Y", nY );
}


