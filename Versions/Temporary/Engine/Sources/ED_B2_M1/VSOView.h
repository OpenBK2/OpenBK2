#pragma once

#include "MapEditorLib/PaletteList.h"

// The VSO palette -- roads, rivers, crags, lakes and coasts -- behind a
// boundary that names no toolkit.
//
// Same shape as NMapObjectView, which it is very nearly a copy of: a filter
// combo over IObjectFilterCollector, an object list the collector fills, the
// same thumbnails and properties context menu, and the same job of being the
// editor's object storage while it has a selection. The two were not merged.
// They differ in their command ids, their CHID ids, their state type, their
// edit parameters, which controls sit above the filter and which filter is
// picked by default, and a base class parameterised on all six would be harder
// to read than the two files are -- which is, for what it is worth, the same
// conclusion Nival came to.
namespace NVSOView
{
	// The object filter type this palette lists under. Shared rather than
	// duplicated: both palettes pass it to the filter collector on every refill
	// and a typo in one of them would show an empty list with no other symptom.
	inline constexpr char FILTER_TYPE[] = "VSO";

	// Creates the palette in pPage, its page of the shortcut bar, registers it
	// in pPalettes, which owns it from then on, and returns it. Null if it could
	// not be created.
	IWidget* Create( CPaletteList *pPalettes, IWidget *pPage );
}
