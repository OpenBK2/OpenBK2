#pragma once

#include <memory>

class CWnd;

// The model editor's palette -- light, background, FOV, terrain, animation
// preview and AI geometry -- behind a boundary that names no toolkit.
//
// **A different shape from the map info palettes.** Those are tabs, and a tab
// control owns its tabs and deletes them through a CWnd*, so their factories
// hand back a borrowed pointer. This one is the contents of a docking pane, and
// CModelEditor used to hold it by value as a CModelWindow. So the factory hands
// back ownership: CModelEditor keeps it, destroys its window in DestroyControls
// as it always has, and deletes it after.
//
// Its data type is CModelState::SEditParameters and its command dispatch is
// CModelCommands, both in ModelState.h.
namespace NModelView
{
	// Creates the palette as a child of pParent, which is the model editor's
	// docking pane, ready to be handed to SetControlBarWindowContents and shown.
	// Null if it could not be created.
	std::unique_ptr<CWnd> Create( CWnd *pParent );

	// Named so the factory can reach them; not for anything else to call.
	std::unique_ptr<CWnd> CreateMfc( CWnd *pParent );
#ifdef OBK2_WITH_WX
	std::unique_ptr<CWnd> CreateWx( CWnd *pParent );
#endif
}
