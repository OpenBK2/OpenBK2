#include "stdafx.h"
#include "ForegroundTextString.h"

#include "UIVisitor.h"
#include "UIML.h"
#include "Tools.h"
#include "System/Text.h"

REGISTER_SAVELOAD_CLASS(0x11075B43,CForegroundTextString)
REGISTER_SAVELOAD_CLASS(0x1715A340,CPlacedText)
extern CVec2 vScreenRect;

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
	return ScreenToVirtualX( pGfxText->GetSize().x );
}

const wstring& CForegroundTextString::GetDBInstanceText() const
{
	static wstring szEmpty;
	if ( CHECK_TEXT_NOT_EMPTY_PRE(pInstance->,TextString) )
		return GET_TEXT_PRE(pInstance->,TextString);
	else if ( pInstance->pShared && CHECK_TEXT_NOT_EMPTY_PRE(pInstance->pShared->,TextString) )
		return GET_TEXT_PRE(pInstance->pShared->,TextString);
	else 
		return szEmpty;
}

const wstring& CForegroundTextString::GetDBFormatText() const
{
	static wstring szEmpty;
	if ( pInstance->pShared && CHECK_TEXT_NOT_EMPTY_PRE(pInstance->pShared->,FormatString) )
		return GET_TEXT_PRE(pInstance->pShared->,FormatString);
	else 
		return szEmpty;
}

const wstring & CForegroundTextString::GetText() const
{
	return wszCustomText;
}

void CForegroundTextString::SetText( const wstring &_szText ) 
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
		CTPoint<int> size ;
		ScreenToVirtual( pGfxText->GetSize(), &size );
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
	static CTPoint<int> ptEmpty( 0, 0 );
	if ( pGfxText )
	{
		CTPoint<int> size;
		ScreenToVirtual( pGfxText->GetSize(), &size );
		CTPoint<int> size0;
		ScreenToVirtual( ptEmpty, &size0 );
		CTRect<float> place( rcParent.x1, rcParent.y1, rcParent.x1 + size.x - size0.x, rcParent.y1 + size.y - size0.y );
		NUITools::ApplyPlacement( placement, rcParent, &place );

		CTRect<float> tmp;
		VirtualToScreen( place, &tmp );
		pVisitor->VisitUIText( pGfxText, tmp.GetLeftTop(), tmp );
	}
}

void CPlacedText::SetText( const wstring &_wszText )
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
	static CTPoint<int> ptEmpty( 0, 0 );
	
	if ( !pGfxText )
		return ptEmpty;
	
	CTPoint<int> point;
	ScreenToVirtual( pGfxText->GetSize(), &point );
	CTPoint<int> point0;
	ScreenToVirtual( ptEmpty, &point0 );
	return point - point0;
}

int CPlacedText::GetOptimalWidth() const
{
	if ( !pGfxText )
		return 0;
	return ScreenToVirtualX( pGfxText->GetSize().x ) - ScreenToVirtualX( 0 );
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


