#pragma once

#include "MapEditorLib/PaletteList.h"

class CWnd;

// The map object palette, behind a boundary that names no toolkit.
//
// Same shape as NFieldView and NHeightViewV3: the tab control owns its tabs and
// deletes them through a CWnd*, so the factory hands one back rather than an
// interface. Its data type is CMapObjectMultiState::SEditParameters and its
// command dispatch is CMapObjectCommands, both in MapObjectMultiState.h.
//
// **There is no bFull here, and the FULL template is not drawn.**
// CMapObjectWindow takes a bFull that picks between IDD_TAB_MI_MAPOBJECT_FULL
// and IDD_TAB_MI_MAPOBJECT_NO_BUTTONS, and the one construction of it in the
// whole tree passes false. The FULL template's ten filter shortcut buttons are
// wired to a handler whose helper, GetSelectedFilterIndex, answers
// INVALID_NODE_ID for nine of them -- their branch of the `if` is an empty
// `else` -- so nine of the ten do nothing and the tenth means "use the combo
// box", which is what the no-buttons palette does anyway. It is an unfinished
// feature, not a mode anyone can reach. The MFC class keeps its bFull; this
// factory offers what the editor actually creates.
namespace NMapObjectView
{
	// The object filter type this palette lists under. Shared rather than
	// duplicated: both palettes pass it to the filter collector on every refill
	// and a typo in one of them would show an empty list with no other symptom.
	inline constexpr char FILTER_TYPE[] = "MAPOBJECT";

	// Creates the palette in pPage, its page of the shortcut bar, registers it
	// in pPalettes, which owns it from then on, and returns it. Null if it could
	// not be created.
	IWidget* Create( CPaletteList *pPalettes, IWidget *pPage );
}
