#pragma once

#include "CameraPositionData.h"
#include "MapEditorLib/PaletteList.h"

// The start-camera-positions palette, behind a boundary that names no toolkit.
//
// A palette is not a modal dialog, and the boundary is a different shape
// because of it. A dialog is a call that blocks until the user answers, so it
// is a function. A palette is a widget with a lifetime, owned by the list of
// its shortcut bar (CPaletteList), which deletes it when the bar goes.
//
// The palette's behaviour is not in the widget. It is in
// CCameraPositionCommands (CameraPositionData.h), so that the command plumbing
// and the shape of the data exist once.
namespace NCameraPositionView
{
	// Creates the palette in pPage, its page of the shortcut bar, registers it
	// in pPalettes, which owns it from then on, and returns it. Null if it could
	// not be created.
	IWidget* Create( CPaletteList *pPalettes, IWidget *pPage );
}
