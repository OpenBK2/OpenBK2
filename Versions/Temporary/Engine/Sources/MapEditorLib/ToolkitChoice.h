#pragma once

#include <cstdlib>

// Which frame draws this session's editor.
//
// The views -- panes, palettes, dialogs, the viewport's window -- are wx's
// alone. The main frame is wx's too, and the MFC one, CMainFrame, stays beside
// it for comparison until it is taken out: a session asks for it by setting
//
//   OBK2_WX_FRAME=0     the MFC main frame, CMainFrame, instead of the wx one
//
// in the environment. Any other value, an empty one, or none means wx. Read
// once, at startup. See docs/EditorToolkits.md.
namespace NToolkit
{
	// The switch is set, to 0.
	inline bool IsSwitchedOff( const char *pszName )
	{
		const char *const pszValue = std::getenv( pszName );
		return ( pszValue != 0 ) && ( pszValue[0] == '0' );
	}

	inline bool UseWxFrame()
	{
		return !IsSwitchedOff( "OBK2_WX_FRAME" );
	}
}
