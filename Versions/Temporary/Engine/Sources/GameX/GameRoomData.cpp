#include "stdafx.h"

#include "3Dmotor/RectLayout.h"
#include "DBConsts.h"
#include "DBGameRoot.h"
#include "DBMPConsts.h"
#include "GameRoomData.h"
#include "Input/GameMessage.h"
#include "InterfaceMPGameRoom.h"
#include "InterfaceState.h"
#include "Misc/2Darray.h"
#include "Misc/StrProc.h"
#include "UI/BackgroundVectorImage.h"
#include "UI/CommandParam.h"
#include "UI/DBUserInterface.h"
#include "UI/UI.h"
#include "UI/UIFactory.h"
#include "UI/UIVisitor.h"
#include "UI/Window.h"
#include "UISpecificB2/DBUISpecificB2.h"

#include <cstdint>

#include <zconf.h>

static std::vector<uint32_t> colors;
static std::vector<std::wstring> sides;

static uint32_t ConvertColor( const CVec3 &vColor )
{
	const int r = vColor.r * 255.0f;
	const int g = vColor.g * 255.0f;
	const int b = vColor.b * 255.0f;

	return 0xFF000000 + ( ( r & 0xFF ) << 16 ) + ( ( g & 0xFF ) << 8 ) + ( b & 0xFF );
}

void CColorBackground::Visit( struct IUIVisitor *pVisitor )
{
	if ( pos.IsEmpty() )
	{
		return;
	}

	CTPoint<int> size;

	CRectLayout::SRect rc;
	rc.fX = pos.x1;
	rc.fY = pos.y1;
	rc.sColor = nColor;

	rc.fSizeX = pos.Width();
	rc.fSizeY = pos.Height();

	CRectLayout rects;
	rects.AddRect( rc.fX, rc.fY, rc.fSizeX, rc.fSizeY, rc.sTex.rcTexRect, rc.sColor );
	VirtualToScreen( &rects );

	pVisitor->VisitUIRect( 0, 3, rects );
}

void CColorViewer::MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const
{
	CDynamicCast<IListControlItem> pList = pWindow;
	NI_VERIFY( pList, "Wrong window", return );

	CDynamicCast<CColorData> pInfo = pData;
	NI_VERIFY( pInfo, "Wrong data", return );

	CWindow *pWnd = GetChildChecked<CWindow>( pList, "ComboColorWnd", true );
	CPtr<CColorBackground> pBackground = new CColorBackground();
	pBackground->nColor = pInfo->nColor;

	if ( pWnd )
	{
		pWnd->SetBackground( pBackground );
	}
}

void CTextureViewer::MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const
{
	CDynamicCast<IListControlItem> pList = pWindow;
	NI_VERIFY( pList, "Wrong window", return );

	CDynamicCast<CTextureData> pInfo = pData;
	NI_VERIFY( pInfo, "Wrong data", return );

	CWindow *pWnd = GetChildChecked<CWindow>( pList, "ComboTextureWnd", true );

	if ( pWnd )
	{
		// The combo box redraws its selected line through this viewer for every
		// item it shows, so the same window can go from a flag to a plain texture
		// (the random country icon) and back: the vector background is installed
		// over the window's own one, and that one is put back when it is not needed.
		CDynamicCast<CBackgroundVectorImage> pVector( pWnd->GetBackground() );
		if ( !pInfo->szVectorImage.empty() )
		{
			if ( pVector )
			{
				pVector->SetFileName( pInfo->szVectorImage );
			}
			else
			{
				pWnd->SetBackground( new CBackgroundVectorImage( pInfo->szVectorImage, pWnd->GetBackground() ) );
			}
		}
		else
		{
			if ( pVector )
			{
				pWnd->SetBackground( pVector->GetReplaced() );
			}
			pWnd->SetTexture( pInfo->pTexture );
		}
		pWnd->SetTooltip( pInfo->wszTooltip );
	}
}

CTextData::CTextData( int i )
{
	wszText = NStr::ToUnicode( std::to_string( i ) );
}

void CTextDataViewer::MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const
{
	CDynamicCast<IListControlItem> pItem = pWindow;
	NI_VERIFY( pItem, "Wrong window", return );
	CDynamicCast<CTextData> pInfo = pData;

	std::wstring wszText = L"";
	if ( pInfo )
	{
		wszText = pInfo->wszText;
	}

	ITextView *pTxt = GetChildChecked<ITextView>( pItem, "ItemComboText", true );
	pTxt->SetText( pTxt->GetDBText() + wszText );
}

// Some strange utilities

void NMPSetData::SetText( ITextView *pText, const std::wstring &szText )
{
	if ( !pText )
	{
		return;
	}
	pText->SetText( pText->GetDBText() + szText );
}

void NMPSetData::SetNum( ITextView *pWindow, int nText )
{
	std::wstring newstring = NStr::ToUnicode( std::to_string( nText ) );
	SetText( pWindow, newstring );
}

void NMPSetData::SetChildText( IListControlItem *pItem, const std::string &szName, const std::wstring &szText )
{
	ITextView *pTxt = GetChildChecked<ITextView>( pItem, szName, true );
	SetText( pTxt, szText );
}
