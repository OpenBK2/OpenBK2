#pragma once

#include "MapEditorLib/PaletteList.h"

class CWnd;

// The field palette, behind a boundary that names no toolkit.
//
// Same shape as the other palettes: the tab control owns its tabs and deletes
// them through a CWnd*, so the factory hands one back rather than an interface.
// Its data type is CFieldState::SEditParameters and its command dispatch is
// CFieldCommands, both in FieldState.h, shared by the two implementations.
namespace NFieldView
{
	// Creates the palette in pPage, its page of the shortcut bar, registers it
	// in pPalettes, which owns it from then on, and returns it. Null if it could
	// not be created.
	IWidget* Create( CPaletteList *pPalettes, IWidget *pPage );
}
