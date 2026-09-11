#pragma once

#include "PaletteCommands.h"
#include "ScriptCameraEditorData.h"
#include "MapEditorLib/DefaultTabWindow.h"

class CWnd;

// The palette's half of the exchange with CScriptCameraState, whichever toolkit
// draws it. The data type was already in a header of its own,
// ScriptCameraEditorData.h, so only the dispatch is declared here.
//
// A partial dispatch, unlike the script area palette's: this one answers eight
// commands of its own -- show the manual controls, and a get and a set for each
// of yaw, pitch and FOV -- so it overrides HandleCommand and UpdateCommand and
// falls through to here for the dialog-data pair.
typedef CPaletteCommands<SScriptCameraWindowData> CScriptCameraCommands;


// The script camera palette -- the list of saved camera placements a mission
// script can cut to -- behind a boundary that names no toolkit.
namespace NScriptCameraView
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
