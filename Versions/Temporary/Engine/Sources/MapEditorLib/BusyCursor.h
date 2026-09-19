#pragma once

#include "MapEditorLib_export.h"

// The busy cursor, for as long as one of these lives: what MFC's CWaitCursor
// was, for a long operation run straight from a command.
//
// Toolkit-neutral, because MapEditorLib does not link wx and its users -- the
// editor states, the builders -- include no toolkit either. The front end says
// how to show and hide the cursor, once, at startup (the wx frame hands it
// wxBeginBusyCursor and wxEndBusyCursor). Until then this does nothing. Nests.
//
// CWaitCursor did not work under the wx frame anyway: MFC keeps its wait
// cursor up by answering WM_SETCURSOR in its own windows, and there are none.
class MAPEDITORLIB_EXPORT CBusyCursor
{
	// Whether this one showed the cursor, and so has it to put back.
	bool bShown = false;

public:
	typedef void ( *TCursorHandler )();

	CBusyCursor();
	~CBusyCursor();

	CBusyCursor( const CBusyCursor& ) = delete;
	CBusyCursor& operator=( const CBusyCursor& ) = delete;

	// The front end's two halves: show the busy cursor, and put back what was
	// there before it. Null for either turns this off.
	static void SetHandlers( TCursorHandler pfnBegin, TCursorHandler pfnEnd );
};
