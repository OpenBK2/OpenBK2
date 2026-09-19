#pragma once

#include "MapEditorLib/PaletteList.h"

class CWnd;

// The squad formations palette, behind a boundary that names no toolkit.
//
// Same shape as NCameraPositionView, and for the same reason: the tab control
// owns its tabs and deletes them through a CWnd*, so the factory hands one back
// rather than an interface. Its data type and command dispatch are in
// DialogData.h and PaletteCommands.h, shared by both implementations.
namespace NFormationView
{
	// Creates the palette in pPage, its page of the shortcut bar, registers it
	// in pPalettes, which owns it from then on, and returns it. Null if it could
	// not be created.
	IWidget* Create( CPaletteList *pPalettes, IWidget *pPage );
}
