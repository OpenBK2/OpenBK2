#pragma once

#include "MapEditorLib/PaletteList.h"

// The unit start commands palette, behind a boundary that names no toolkit.
//
// Same shape as the other palettes: the factory hands back the palette's
// widget, which pPalettes owns. Its data type and command dispatch are in
// UnitStartCmdData.h, shared by both implementations.
namespace NUnitStartCmdView
{
	// Creates the palette in pPage, its page of the shortcut bar, registers it
	// in pPalettes, which owns it from then on, and returns it. Null if it could
	// not be created.
	IWidget* Create( CPaletteList *pPalettes, IWidget *pPage );
}
