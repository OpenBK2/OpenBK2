#pragma once

#include <cstdlib>

// Which toolkit draws this session's editor.
//
// wx, wherever it is built in (OBK2_WITH_WX). MFC stays beside it, for
// comparison, until it is taken out, and a session asks for it by setting a
// switch to 0 in the environment:
//
//   OBK2_WX_FRAME=0     the MFC main frame, CMainFrame, instead of the wx one
//   OBK2_WX_DIALOGS=0   the MFC views -- panes, palettes, dialogs, the
//                       viewport's window -- instead of the wx ones
//
// The two are independent, so all four combinations run. Any other value, an
// empty one, or none means wx. Read each time it is asked, which is once per
// view made; nothing changes it while the editor runs. In a build without wx
// nothing asks, and everything is MFC. See docs/EditorToolkits.md.
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

	inline bool UseWxViews()
	{
		return !IsSwitchedOff( "OBK2_WX_DIALOGS" );
	}
}
