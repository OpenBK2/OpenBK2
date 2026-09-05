#include "stdafx.h"

#include "ShellFont.h"

#include <map>


namespace
{

//! The height of one line in a font, which is the number dialog layout is made
//! of: a dialog's units come from its font's cell, so an eight unit label is
//! thirteen pixels tall precisely because 8 point MS Sans Serif is.
int GetCellHeight( HDC hDC, HFONT hFont )
{
	const HGDIOBJ hOldFont = ::SelectObject( hDC, hFont );
	TEXTMETRIC textMetric;
	::ZeroMemory( &textMetric, sizeof( textMetric ) );
	::GetTextMetrics( hDC, &textMetric );
	::SelectObject( hDC, hOldFont );
	return textMetric.tmHeight;
}

//! The shell font, shrunk until a line of it fits the cell the template used.
//!
//! This is what makes a font swap safe without moving anything. The templates
//! lay their controls out in dialog units derived from their own font, so an
//! eight unit label is thirteen pixels tall here. The shell's font is Segoe UI
//! at nine point, whose line is fifteen, and dropping that in as it comes costs
//! every one line label its descenders. One point smaller its line is thirteen
//! again, with the same eleven pixel ascent and two pixel descent the template
//! font has, so the text lands exactly where the template put it.
//!
//! Measured rather than assumed. Those numbers are this machine's, at this DPI,
//! for these two template fonts; the loop is what makes the rule hold anywhere
//! else. If no size fits, nothing is returned and the dialog keeps the font it
//! had, which is ugly but is never clipped.
HFONT MakeShellFont( int nCellHeight )
{
	NONCLIENTMETRICS nonClientMetrics;
	::ZeroMemory( &nonClientMetrics, sizeof( nonClientMetrics ) );
	nonClientMetrics.cbSize = sizeof( nonClientMetrics );
	if ( !::SystemParametersInfo( SPI_GETNONCLIENTMETRICS, sizeof( nonClientMetrics ), &nonClientMetrics, 0 ) )
	{
		return 0;
	}
	LOGFONT logFont = nonClientMetrics.lfMessageFont;
	const HDC hDC = ::GetDC( 0 );
	HFONT hFont = 0;
	// lfHeight is a character height and is negative, so counting it up is what
	// makes the font smaller. Four steps is more than the one this needs, and
	// stops a face with unexpected metrics from looping.
	for ( int nStep = 0; nStep < 4; ++nStep )
	{
		hFont = ::CreateFontIndirect( &logFont );
		if ( hFont == 0 )
		{
			break;
		}
		if ( ( nCellHeight <= 0 ) || ( GetCellHeight( hDC, hFont ) <= nCellHeight ) )
		{
			break;
		}
		::DeleteObject( hFont );
		hFont = 0;
		logFont.lfHeight += ( logFont.lfHeight < 0 ) ? 1 : -1;
	}
	::ReleaseDC( 0, hDC );
	return hFont;
}

//! One font per cell height asked for.
//!
//! There are only ever one or two of those in this editor, and a font handed to
//! WM_SETFONT has to outlive every window drawing with it, so these are made
//! once and kept for the life of the process rather than owned by a dialog. A
//! size that could not be made is remembered as well, so it is not retried on
//! every dialog.
std::map<int, HFONT> shellFonts;

HFONT GetShellFont( int nCellHeight )
{
	const std::map<int, HFONT>::const_iterator itFont = shellFonts.find( nCellHeight );
	if ( itFont != shellFonts.end() )
	{
		return itFont->second;
	}
	const HFONT hFont = MakeShellFont( nCellHeight );
	shellFonts[nCellHeight] = hFont;
	return hFont;
}

//! Everything under a window that is still on the font that window handed out.
//!
//! A control carrying a font of its own was given it deliberately -- the script
//! and text editors set a fixed pitch face on their Scintilla views -- and
//! keeps it. A control carrying no font at all draws in the stock system face,
//! which is the blocky one and the worst looking of the three, so those are
//! taken too.
void ApplyToChildren( CWnd *pWnd, HFONT hFont, HFONT hTemplateFont )
{
	for ( CWnd *pChild = pWnd->GetWindow( GW_CHILD ); pChild != 0;
				pChild = pChild->GetWindow( GW_HWNDNEXT ) )
	{
		const HFONT hCurrentFont = reinterpret_cast<HFONT>( pChild->SendMessage( WM_GETFONT ) );
		if ( ( hCurrentFont != hTemplateFont ) && ( hCurrentFont != 0 ) )
		{
			continue;
		}
		pChild->SendMessage( WM_SETFONT, reinterpret_cast<WPARAM>( hFont ), MAKELPARAM( TRUE, 0 ) );
		ApplyToChildren( pChild, hFont, hTemplateFont );
	}
}

}


namespace NEditorFont
{

void ApplyShellFont( CWnd *pWnd )
{
	if ( ( pWnd == 0 ) || ( pWnd->GetSafeHwnd() == 0 ) )
	{
		return;
	}
	// The font the template gave the dialog. It is both the size to stay within
	// and the mark of a control that has not chosen a font of its own.
	const HFONT hTemplateFont = reinterpret_cast<HFONT>( pWnd->SendMessage( WM_GETFONT ) );
	int nCellHeight = 0;
	if ( hTemplateFont != 0 )
	{
		const HDC hDC = ::GetDC( pWnd->GetSafeHwnd() );
		nCellHeight = GetCellHeight( hDC, hTemplateFont );
		::ReleaseDC( pWnd->GetSafeHwnd(), hDC );
	}
	const HFONT hShellFont = GetShellFont( nCellHeight );
	if ( ( hShellFont == 0 ) || ( hShellFont == hTemplateFont ) )
	{
		return;
	}
	pWnd->SendMessage( WM_SETFONT, reinterpret_cast<WPARAM>( hShellFont ), MAKELPARAM( TRUE, 0 ) );
	ApplyToChildren( pWnd, hShellFont, hTemplateFont );
}

}
