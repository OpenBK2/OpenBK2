#include "stdafx.h"
#include "./windowstatssystem.h"
#include "UIVisitor.h"

#include "UIML.h"



int CWindowStatsSystem::SColorString::operator&( IBinSaver &saver )
{
	saver.Add( 1, &szString );
	saver.Add( 2, &dwColor );
	return 0;
}

CWindowStatsSystem::SColorString::SColorString( const wchar_t *pszStr, DWORD col, const int nWidth ) 
{
	Init( pszStr, col, nWidth );
}

void CWindowStatsSystem::SColorString::SetText( const wchar_t *pszStr, DWORD col, const int nWidth )
{
	std::wstring szText = NStr::ToUnicode( StrFmt( "<color=%.8X>", col ) ) + pszStr;
	pGfxText->SetText( szText, 0 );
	pGfxText->Generate( VirtualToScreenX( nWidth ) );
}

void CWindowStatsSystem::SColorString::Init( const wchar_t *pszStr, DWORD col, const int nWidth )
{
	pGfxText = CreateML();
	CUIFactory::RegisterMLHandlers( pGfxText );
	SetText( pszStr, col, nWidth );
}

void CWindowStatsSystem::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	const NDb::SWindowStatsSystem *pDesc( checked_cast<const NDb::SWindowStatsSystem*>( _pDesc ) );
	pInstance = pDesc->Duplicate();
	CWindow::InitByDesc( _pDesc );

	pShared = checked_cast_ptr<const NDb::SWindowStatsSystemShared *>( pDesc->pShared );
}

void CWindowStatsSystem::UpdateEntry( const std::wstring &szEntry, const std::wstring &szValue, const DWORD dwColor )
{
	CEntries::iterator entryPos = entries.find( szEntry );
	const std::wstring szTmp = szEntry + L": " + szValue;
	CTRect<float> wndRect;
	FillWindowRect( &wndRect );

	if ( entryPos == entries.end() )
		entries[szEntry].Init( szTmp.c_str(), dwColor, wndRect.Width() );
	else
		entryPos->second.SetText( szTmp.c_str(), dwColor, wndRect.Width() );
}

void CWindowStatsSystem::Visit( struct IUIVisitor *pVisitor )
{
	CWindow::Visit( pVisitor );

	if ( IsVisible() )
	{
		CTRect<float> wndRect;
		FillWindowRect( &wndRect );

		int nCurrentY = 0;
		for ( CEntries::const_iterator it = entries.begin(); it != entries.end(); ++it )
		{
			CTPoint<float> vPosition( 0, nCurrentY );
			VirtualToScreen( vPosition, &vPosition );
			CTRect<float> tmp;
			VirtualToScreen( wndRect, &tmp );
			pVisitor->VisitUIText( it->second.pGfxText, tmp.GetLeftTop() + vPosition, tmp );
			nCurrentY += 20;
		}
	}
}

REGISTER_SAVELOAD_CLASS( 0x110AC482, CWindowStatsSystem );

