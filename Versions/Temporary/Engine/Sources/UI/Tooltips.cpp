#include "stdafx.h"
#include "Tooltips.h"
#include "WindowTooltip.h"
#include "UIScreen.h"
#include "DBUIConsts.h"

CTooltips::CTooltips()
: vLastMousePos( -1, -1 ), timeMouseFreese( 0 ), nContext( -1 )
{
}

void CTooltips::AdjustTooltipPos( const CVec2 &vMousePos )
{
	CDynamicCast<CWindowTooltip> pWindowToolTip = pTooltip;
	if ( pWindowToolTip )
		pWindowToolTip->AdjustPosByMousePos( vMousePos );
	else
		pTooltip->SetPlacement( vMousePos.x, vMousePos.y, 0, 0, EWPF_POS_X | EWPF_POS_Y	);
}

void CTooltips::Segment( const int timeDiff, class CWindowScreen *pScreen )
{
	if ( nContext == -1 )
		return;

	if ( pTooltip )  // tool tip is on screen
	{
		CVec2 vMousePos;
		ScreenToVirtual( vLastMousePos, &vMousePos );
	
		// check if another window now have tool tip (a child of current pTooltipedWindow)
		CWindow *pNewTooltiped = dynamic_cast<CWindow*>( pScreen->Pick( vLastMousePos, true ) );
		while ( pNewTooltiped && !pNewTooltiped->DemandTooltip() )
			pNewTooltiped = pNewTooltiped->GetParent();
		
		const bool bShouldHideTooltip = 
			pNewTooltiped != pTooltipedWindow || !pTooltipedWindow->IsInside( vMousePos ) 
			|| !pScreen->IsEnabled()
			|| !pNewTooltiped->DemandTooltip();

		if ( bShouldHideTooltip )
			HideTooltip();
		else if ( pNewTooltiped->DemandTooltip() != pTooltip )
		{
			pTooltip->ShowWindow( false );

			pTooltipedWindow = pNewTooltiped;
			pTooltip = pNewTooltiped->DemandTooltip();
			AdjustTooltipPos( vMousePos );
			pTooltip->ShowWindow( true );
		}
	}
	else if ( pScreen->IsEnabled() )
	{
		timeMouseFreese += timeDiff;
		if ( timeMouseFreese > CUIFactory::GetConsts()->contexts[nContext]->nAppearDelay )
		{
			pTooltipedWindow = dynamic_cast<CWindow*>( pScreen->Pick( vLastMousePos, true ) );

			// check if tooltip exists and if not, check parents untill some of them have tooltip
			while ( pTooltipedWindow && !pTooltipedWindow->DemandTooltip() )
				pTooltipedWindow = pTooltipedWindow->GetParent();

			if ( pTooltipedWindow && pTooltipedWindow->DemandTooltip() )
			{
				pTooltip = pTooltipedWindow->DemandTooltip();

				CVec2 vMousePos;
				ScreenToVirtual( vLastMousePos, &vMousePos );
				AdjustTooltipPos( vMousePos );

				pTooltip->ShowWindow( true );
			}

			timeMouseFreese = 0;
		}
	}
}

IWindow *CTooltips::CreateTooltipWindow( const std::wstring &wszTooltipText, IWindow *pTooltipOwner, IScreen *pScreen )
{
	if ( nContext == -1 )
		return new CWindowTooltip( wszTooltipText );
	
	CDynamicCast<CWindowTooltip> pTooltip = CUIFactory::MakeWindow( CUIFactory::GetConsts()->contexts[nContext]->pWindow );
	if ( !pTooltip )
		return 0;

	pTooltip->Init();

	CTRect<float> rect;
	pTooltipOwner->FillWindowRect( &rect );

	CVec2 vCenter( rect.GetCenter().x, rect.GetCenter().y );
	pTooltip->InitTooltip( vCenter, rect, wszTooltipText, pScreen,
		CUIFactory::GetConsts()->contexts[nContext]->nSingleLineWidth,
		CUIFactory::GetConsts()->contexts[nContext]->fHorisontalToVerticalRatio,
		pTooltipOwner->GetTooltipIDForMLHandler() );
	pTooltip->ShowWindow( false );

	return pTooltip;
}

void CTooltips::OnMouseMove( const CVec2 &vPos, const int nButton, class CWindowScreen *pScreen )
{
	if ( nContext == -1 )
		return;
	
	if ( fabs( vLastMousePos - vPos ) > CUIFactory::GetConsts()->contexts[nContext]->nMouseMaxOffsetToAppear )	
		timeMouseFreese = 0;			

	vLastMousePos = vPos;	
}

void CTooltips::HideTooltip()
{
	if ( pTooltip )
	{
		pTooltip->ShowWindow( false );
		pTooltipedWindow = 0;
		pTooltip = 0;
		timeMouseFreese = 0;
	}
}

void CTooltips::SetTooltipContext( const int _nContext, class CWindowScreen *pScreen )
{
	if ( !CUIFactory::GetConsts() ) 
		return;
	nContext = _nContext;
	if ( CUIFactory::GetConsts()->contexts[nContext] == 0 ) 
		return;

	if ( CUIFactory::GetConsts()->contexts[nContext]->pWindow == 0 )
		return;
	
	NI_ASSERT( CUIFactory::GetConsts()->contexts.size() > nContext, "no such context" );
}


