#pragma once

#include "MapEditorLib/DefaultTabWindow.h"

class CWnd;

// The AI general points palette, behind a boundary that names no toolkit.
//
// Same shape as the other palettes: the tab control owns its tabs and deletes
// them through a CWnd*, so the factory hands one back rather than an interface.
// Its data type and command dispatch are in AIGeneralData.h, shared by both
// implementations.
namespace NAIGeneralView
{
	// Creates the palette inside pTabWindow, registers it in the tab list, and
	// returns it ready to be handed to AddTab with a label. Null if it could not
	// be created.
	CWnd* Create( CDefault3DTabWindow *pTabWindow );

	// Named so the factory can reach them; not for anything else to call.
	CWnd* CreateMfc( CDefault3DTabWindow *pTabWindow );
#ifdef OBK2_WITH_WX
	CWnd* CreateWx( CDefault3DTabWindow *pTabWindow );
#endif
}
