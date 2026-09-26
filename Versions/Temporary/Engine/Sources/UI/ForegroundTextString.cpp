#include "stdafx.h"
#include "ForegroundTextString.h"

#include "UIVisitor.h"
#include "UIML.h"
#include "Tools.h"
#include "System/Text.h"

REGISTER_SAVELOAD_CLASS(UI, 0x11075B43, CForegroundTextString)
REGISTER_SAVELOAD_CLASS(UI, 0x1715A340, CPlacedText)
extern CVec2 vScreenRect;

// The size of generated text in virtual units, kept fractional.
//
// This used to go through the CTPoint<int> overload of ScreenToVirtual, which
// truncates to whole virtual units. At 1024x768 a virtual unit is one pixel and
// nothing is lost, but at 2560x1600 it is 2.5 pixels across and 2.08 down, and
// the rect built from the result is also the clip window VisitUIText draws
// through. So up to 2.5 px came off the right of the last letter and 2 px off
// the bottom of the descenders, on every resolution but the original one.
static CVec2 GetVirtualTextSize( IML *pGfxText )
{
	const CTPoint<int> &size = pGfxText->GetSize();
	return ScreenToVirtual( CVec2( size.x, size.y ) ) - ScreenToVirtual( VNULL2 );
}

// The same, rounded up to whole virtual units for callers that size a window
// from it, so the window is never narrower than the text it has to hold
static CTPoint<int> GetVirtualTextSizeCeil( IML *pGfxText )
{
	const CVec2 vSize = GetVirtualTextSize( pGfxText );
	return CTPoint<int>( static_cast<int>( ceilf( vSize.x ) ), static_cast<int>( ceilf( vSize.y ) ) );
}

void CForegroundTextString::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	const NDb::SForegroundTextString *pDesc ( checked_cast<const NDb::SForegroundTextString*>( _pDesc ) );
	//NI_ASSERT( pDesc->pShared, StrFmt( "no shared window found for ForegroundTextString (%s)", pDesc->GetDBID().ToString().c_str() ) );
	pInstance = pDesc->Duplicate();
	wszCustomText = GetDBFormatText() + GetDBInstanceText();
	Init();
}

void CForegroundTextString::InitText()
{
	pGfxText = CreateML();
	CUIFactory::RegisterMLHandlers( pGfxText );
	pGfxText->SetText( GetText(), 0 );
	pGfxText->Generate( VirtualToScreenX( rcParent.GetSizeX() ) );
}

int CForegroundTextString::GetOptimalWidth() const
{
	return GetVirtualTextSizeCeil( pGfxText ).x;
}

const std::wstring& CForegroundTextString::GetDBInstanceText() const
{
	static std::wstring szEmpty;
	if ( CHECK_TEXT_NOT_EMPTY_PRE(pInstance->,TextString) )
		return GET_TEXT_PRE(pInstance->,TextString);
	else if ( pInstance->pShared && CHECK_TEXT_NOT_EMPTY_PRE(pInstance->pShared->,TextString) )
		return GET_TEXT_PRE(pInstance->pShared->,TextString);
	else 
		return szEmpty;
}

const std::wstring& CForegroundTextString::GetDBFormatText() const
{
	static std::wstring szEmpty;
	if ( pInstance->pShared && CHECK_TEXT_NOT_EMPTY_PRE(pInstance->pShared->,FormatString) )
		return GET_TEXT_PRE(pInstance->pShared->,FormatString);
	else 
		return szEmpty;
}

const std::wstring & CForegroundTextString::GetText() const
{
	return wszCustomText;
}

void CForegroundTextString::SetText( const std::wstring &_szText )
{ 
	wszCustomText = _szText;
	InitText();
}

const NDb::SWindowPlacement* CForegroundTextString::GetPlacement() const
{
	if ( pInstance->pShared )
		return &pInstance->pShared->position;
	return 0;
}

void CForegroundTextString::Visit( struct IUIVisitor *pVisitor )
{
	if ( pGfxText ) 
	{
		const CVec2 size = GetVirtualTextSize( pGfxText );
		CTRect<float> place( rcParent.x1, rcParent.y1, rcParent.x1 + size.x, rcParent.y1 + size.y );
		if ( pInstance->pShared )
			NUITools::ApplyPlacement( pInstance->pShared->position, rcParent, &place );

		CTRect<float> tmp;
		VirtualToScreen( place, &tmp );
		pVisitor->VisitUIText( pGfxText, tmp.GetLeftTop(), tmp );
	}
}

void CForegroundTextString::Init()
{
	InitText();
}

void CForegroundTextString::SetPos( const CVec2 &vPos, const CVec2 &vSize )
{
	rcParent.Set( vPos.x, vPos.y, vPos.x + vSize.x, vPos.y + vSize.y );
	InitText();
}

void CForegroundTextString::SetFadeValue( float fValue )
{
}

void CForegroundTextString::SetInternalFadeValue( float fValue )
{
}

// CPlacedText

CPlacedText::CPlacedText()
: vScreenRect( VNULL2 )
{
	rcParent.SetEmpty();
}

void CPlacedText::Init()
{
}

void CPlacedText::Visit( struct IUIVisitor *pVisitor )
{
	if ( pGfxText )
	{
		const CVec2 size = GetVirtualTextSize( pGfxText );
		CTRect<float> place( rcParent.x1, rcParent.y1, rcParent.x1 + size.x, rcParent.y1 + size.y );
		NUITools::ApplyPlacement( placement, rcParent, &place );

		CTRect<float> tmp;
		VirtualToScreen( place, &tmp );
		pVisitor->VisitUIText( pGfxText, tmp.GetLeftTop(), tmp );
	}
}

void CPlacedText::SetText( const std::wstring &_wszText )
{
	if ( wszText == _wszText )
		return;

	wszText = _wszText;
	InitGfxText();
}

void CPlacedText::SetPlacement( const struct NDb::SWindowPlacement &_placement )
{
	placement = _placement;
}

void CPlacedText::InitGfxText()
{
	if ( rcParent.GetSizeX() <= 0 || GetText().empty() )
	{
		pGfxText = 0;
		return;
	}
	pGfxText = CreateML();
	CUIFactory::RegisterMLHandlers( pGfxText );
	pGfxText->SetText( GetText(), 0 );
	pGfxText->SetFade( fFadeValue );
	pGfxText->Generate( VirtualToScreenX( rcParent.GetSizeX() ) - VirtualToScreenX( 0 ) );
}

CTPoint<int> CPlacedText::GetSize() const
{
	if ( !pGfxText )
		return CTPoint<int>( 0, 0 );
	return GetVirtualTextSizeCeil( pGfxText );
}

int CPlacedText::GetOptimalWidth() const
{
	if ( !pGfxText )
		return 0;
	return GetVirtualTextSizeCeil( pGfxText ).x;
}

void CPlacedText::Reposition( const CTRect<float> &parentRect )
{
	float fOldWidth = rcParent.Width();
	rcParent = parentRect;
	if ( fOldWidth == rcParent.Width() && ::vScreenRect == this->vScreenRect )
		return;
	this->vScreenRect = ::vScreenRect;
	InitGfxText();
}

void CPlacedText::SetInternalFadeValue( float fValue )
{
	if ( pGfxText && fValue != fFadeValue )
	{
		fFadeValue = fValue;
		InitGfxText();
	}
}


