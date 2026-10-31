#pragma once
#include "Resources.h"

// The editor's SVG icons (MapEditorLib/res/icons) as wx bitmap bundles.
//
// The icons are drawn in currentColor, which nanosvg -- what wx renders SVG
// with -- does not know. So the colour is put into the text before wx parses
// it: the theme's text colour by default, which is also what makes them right
// on a dark theme, where the old black-outlined bitmaps all but vanished.
//
// A bundle renders on demand at whatever size the control asks for, so one
// icon serves 16 pixels, a high-DPI 32, and anything else.
//
// Header-only, like the other Wx*.h here; see WxWidget.h for why these stay
// out of MapEditorLib's own translation units.

#include <string>

#include <wx/bitmap.h>
#include <wx/bmpbndl.h>
#include <wx/colour.h>
#include <wx/settings.h>

namespace NWxSvgIcons
{
// The icon pszName at nSize DIPs, drawn in rColour; an empty bundle when no
// icon has that name, which every wx control taking a bundle accepts.
inline wxBitmapBundle Load( const char *pszName, const wxColour &rColour, int nSize = 16 )
{
	const char *const pszSvg = NResources::GetSvg( pszName );
	if ( pszSvg == nullptr )
	{
		return wxBitmapBundle();
	}
	std::string szSvg( pszSvg );
	const std::string szColour( rColour.GetAsString( wxC2S_HTML_SYNTAX ).utf8_str() );
	static const std::string CURRENT_COLOR = "currentColor";
	for ( size_t nPos = szSvg.find( CURRENT_COLOR ); nPos != std::string::npos;
	      nPos = szSvg.find( CURRENT_COLOR, nPos + szColour.size() ) )
	{
		szSvg.replace( nPos, CURRENT_COLOR.size(), szColour );
	}
	return wxBitmapBundle::FromSVG( szSvg.c_str(), wxSize( nSize, nSize ) );
}

// The same in the theme's text colour, the usual case.
inline wxBitmapBundle Load( const char *pszName, int nSize = 16 )
{
	return Load( pszName, wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ), nSize );
}

// One rendering of the icon, for the controls that still take a wxBitmap or a
// wxImageList; invalid when no icon has that name.
inline wxBitmap Render( const char *pszName, int nSize = 16 )
{
	const wxBitmapBundle bundle = Load( pszName, nSize );
	return bundle.IsOk() ? bundle.GetBitmap( wxSize( nSize, nSize ) ) : wxBitmap();
}
} // namespace NWxSvgIcons
