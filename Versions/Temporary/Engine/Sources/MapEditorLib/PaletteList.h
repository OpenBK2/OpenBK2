#pragma once

#include "Interface_Widget.h"

#include <memory>
#include <vector>

// What a shortcut bar reports for "no bar" and "no tab", in the MAKELONG( tab,
// bar ) it sends. Were defined with the MFC bar (DefaultShortcutBar.h) and tab
// window (DefaultTabWindow.h).
#define INVALID_SHORTCUT_INDEX ( 0xFFFF )
#define INVALID_TAB_INDEX ( 0xFFFF )

// One bar's palettes: the list a palette's Create registers in, and the owner
// that deletes them, in the order they were added, when the bar goes.
//
// This is what was left of CDefault3DTabWindow, the Stingray tab window the
// palettes were made in, once the wx shortcut bar kept one only as a list that
// never became a window.
class CPaletteList
{
	std::vector<std::unique_ptr<IWidget>> palettes;

public:
	// Takes pPalette, which may be null, and hands it back.
	template<class TPalette>
	TPalette* Add( TPalette *pPalette )
	{
		if ( pPalette != nullptr )
		{
			palettes.emplace_back( pPalette );
		}
		return pPalette;
	}

	void Clear()
	{
		palettes.clear();
	}
};
