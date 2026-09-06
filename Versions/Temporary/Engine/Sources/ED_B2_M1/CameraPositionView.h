#pragma once

#include "CameraPositionData.h"
#include "MapEditorLib/DefaultTabWindow.h"

class CWnd;

// The start-camera-positions palette, behind a boundary that names no toolkit.
//
// A palette is not a modal dialog, and the boundary is a different shape
// because of it. A dialog is a call that blocks until the user answers, so it
// is a function. A palette is a child window with a lifetime, owned by the tab
// control it sits in -- CDefault3DTabWindow deletes every tab it holds, through
// a CWnd*, in its destructor and in RemoveAllTabs. So whatever is created here
// has to *be* a CWnd, and the factory hands one back rather than an interface.
//
// The palette's behaviour is not in the CWnd. It is in CCameraPositionCommands
// below, which both implementations derive from, so that the command plumbing
// and the shape of the data exist once.
namespace NCameraPositionView
{
	// Creates the palette inside pTabWindow, registers it in the tab list, and
	// returns it ready to be handed to AddTab with a label. Null if it could not
	// be created.
	//
	// Which implementation is built is decided here and nowhere else.
	CWnd* Create( CDefault3DTabWindow *pTabWindow );

	// Named so the factory can reach them; not for anything else to call.
	CWnd* CreateMfc( CDefault3DTabWindow *pTabWindow );
#ifdef OBK2_WITH_WX
	CWnd* CreateWx( CDefault3DTabWindow *pTabWindow );
#endif
}
