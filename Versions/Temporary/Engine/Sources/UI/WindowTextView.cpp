// WindowTextView.cpp: implementation of the CWindowTextView class.
//

#include "stdafx.h"
#include "WindowTextView.h"
#include "UIVisitor.h"
#include "System/Text.h"

#include "UIML.h"
#include "3Dmotor/GLocale.h"

#include <cmath>
#include <cstdint>

REGISTER_SAVELOAD_CLASS(UI, 0x11075B8C, CWindowTextView)
extern CVec2 vScreenRect;
bool g_bNoInitText = false;


// CWindowTextView


void CWindowTextView::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	const NDb::SWindowTextView *pDesc( checked_cast<const NDb::SWindowTextView*>( _pDesc ) );
	pInstance = pDesc->Duplicate();

	CWindow::InitByDesc( _pDesc );
	pShared = checked_cast_ptr<const NDb::SWindowTextViewShared*>( pDesc->pShared ) ;
	wszCustomText = GetDBText();
	InitText();
}

void CWindowTextView::SetWidth( const int nWidth )
{
	float fOldWidth = GetWindowRect().Width();
	SetPlacement( 0, 0, nWidth, 0, EWPF_SIZE_X );
	if ( fOldWidth == GetWindowRect().Width() )
		return;
	InitText();
}

const CTPoint<int> CWindowTextView::GetSize() const
{
	CTPoint<int> ptSize(0,0);
	//CRAP{ CHECK WHY EMPTY TOOLTIPS BROKE THIS
	if ( pGfxText )
		ScreenToVirtual( pGfxText->GetSize(), &ptSize );
	//CRAP}
	return ptSize;
}

void CWindowTextView::SetPlacement( const float x, const float y, const float sizeX, const float sizeY, const uint32_t flags )
{
	float fOldWidth = GetWindowRect().Width();
	CWindow::SetPlacement( x, y, sizeX, sizeY, flags );
	if ( fOldWidth == GetWindowRect().Width() )
		return;
	if ( !g_bNoInitText )
		InitText();
}

void CWindowTextView::Reposition( const CTRect<float> &parentRect )
{
	float fOldWidth = GetWindowRect().Width();
	CWindow::Reposition( parentRect );
	if ( fOldWidth == GetWindowRect().Width() && ::vScreenRect == this->vScreenRect )
		return;
	this->vScreenRect = ::vScreenRect;
	InitText();
}

bool CWindowTextView::InitText()
{
	fFontScale = NGScene::GetRuntimeFontScale();
	if ( wszCustomText.empty() || GetWindowRect().Width() <= 0 )
	{
		pGfxText = 0;
		if ( pInstance->bResizeOnTextSet && GetWindowRect().Height() > 0 )
		{
			SetPlacement( 0, 0, 0, 0, EWPF_SIZE_Y );
			return true;
		}
		return false;
	}
	
	pGfxText = CreateML();
	if ( nIDForMLHandler >= 0 )
		pGfxText->SetIDForHandler( nIDForMLHandler );
	CUIFactory::RegisterMLHandlers( pGfxText );
	pGfxText->SetText( wszCustomText, 0 );

	// GetWindowRect returns by value, so the pointer overload was being handed the
	// address of a temporary, which only MSVC accepts. The two argument overload
	// takes the source by const reference and says the same thing.
	CTRect<float> rc;
	VirtualToScreen( GetWindowRect(), &rc );
	pGfxText->Generate( rc.Width() + 0.5f );

	if ( !pInstance->bResizeOnTextSet )
		return false;

	int nHeight = ScreenToVirtualY( pGfxText->GetSize().y ) - ScreenToVirtualY( 0 );
	const bool bRet = (nHeight != GetWindowRect().Height());
	if ( bRet )
	{
		g_bNoInitText = true;
		SetPlacement( 0, 0, 0, nHeight, EWPF_SIZE_Y );
		g_bNoInitText = false;
	}
	return bRet;
}

int CWindowTextView::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CWindow*>( this ) );
	saver.Add( 2, &pInstance );
	saver.Add( 3, &pShared );
	saver.Add( 4, &wszCustomText );
	saver.Add( 6, &pGfxText );								// text to display
	saver.Add( 7, &vScreenRect );
	saver.Add( 8, &nIDForMLHandler );
	return 0;
}


void CWindowTextView::Visit( struct IUIVisitor *pVisitor )
{
	// Resizable labels must update their control bounds along with the glyphs.
	if ( fFontScale != NGScene::GetRuntimeFontScale() )
		InitText();
	CWindow::Visit( pVisitor );
	
	if ( pGfxText ) 
	{
		CTRect<float> textRC;
		FillWindowRect( &textRC );
		VirtualToScreen( textRC, &textRC );
		CTPoint<float> textPos = textRC.GetLeftTop();
		const int nTextHeight = pGfxText->GetSize().y;
		if ( !pInstance->bResizeOnTextSet && nTextHeight > textRC.Height() && pGfxText->IsSingleLine() )
		{
			// Fixed HUD fields can be shorter than the font's accent/descender
			// cell even when the visible digits fit. Center that cell instead of
			// clipping all its excess from the bottom; retain the window's clip
			// and snap the draw origin to pixels so runtime glyphs stay sharp.
			textPos.y = std::floor( textRC.y1 + ( textRC.Height() - nTextHeight ) * 0.5f + 0.5f );
		}
		pVisitor->VisitUIText( pGfxText, textPos, textRC );
	}
}

const std::wstring& CWindowTextView::GetDBFormatText() const
{
	if ( const NDb::STextFormat *pTextFormat = pInstance->pTextFormat )
	{
		if ( CHECK_TEXT_NOT_EMPTY_PRE(pTextFormat->,FormatString) )
			return GET_TEXT_PRE(pTextFormat->,FormatString);
	}
	
	return CWindow::GetDBFormatText();
}

const std::wstring& CWindowTextView::GetDBInstanceText() const
{
	if ( CHECK_TEXT_NOT_EMPTY_PRE(pInstance->,Text) )
		return GET_TEXT_PRE(pInstance->,Text);

	return CWindow::GetDBInstanceText();
}

std::wstring CWindowTextView::GetDBText() const
{
	return CWindow::GetDBText();
}

const std::wstring& CWindowTextView::GetText() const
{
	return wszCustomText;
}

bool CWindowTextView::SetText( const std::wstring &_szText )
{
	if ( wszCustomText == _szText )
		return true;
		
	wszCustomText = _szText;
	return InitText();
}

void CWindowTextView::SetIDForMLHandler( int nID )
{
	bool bChanged = (nIDForMLHandler != nID);
	nIDForMLHandler = nID;

	if ( pGfxText )
		InitText();
}


