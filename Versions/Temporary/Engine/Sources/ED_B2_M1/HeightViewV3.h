#pragma once

#include "MapEditorLib/DefaultTabWindow.h"

#include <string>

class CWnd;

// The frames of IDB_TMITH_BITMAP, which is a 544x32 strip of seventeen 32x32
// icons keyed on magenta. Both palettes wear these, one through a CImageList
// and the other by cutting the strip up itself, so the layout is stated once.
//
// At namespace scope with the prefix it has always had, rather than inside
// NHeightViewV3, so that the palette that has used these names since 2005 does
// not have to spell them differently.
enum EHeightIconV3
{
	TMITH_TILE					= 0,
	TMITH_UP						= 1,
	TMITH_DOWN					= 2,
	TMITH_ROUND					= 3,
	TMITH_PLATO					= 4,
	TMITH_CIRCLE				= 5,
	TMITH_SQUARE				= 6,
	TMITH_BRUSH_SIZE_C0	= 7,
	TMITH_BRUSH_SIZE_C1	= 8,
	TMITH_BRUSH_SIZE_C2	= 9,
	TMITH_BRUSH_SIZE_C3	= 10,
	TMITH_BRUSH_SIZE_C4	= 11,
	TMITH_BRUSH_SIZE_R0	= 12,
	TMITH_BRUSH_SIZE_R1	= 13,
	TMITH_BRUSH_SIZE_R2	= 14,
	TMITH_BRUSH_SIZE_R3	= 15,
	TMITH_BRUSH_SIZE_R4	= 16,
	TMITH_COUNT					= 17,
};

// The terrain height palette, behind a boundary that names no toolkit.
//
// Same shape as NFieldView: the tab control owns its tabs and deletes them
// through a CWnd*, so the factory hands one back rather than an interface. Its
// data type is CHeightStateV3::SEditParameters and its command dispatch is
// CHeightCommandsV3, both in HeightStateV3.h, shared by the two
// implementations.
namespace NHeightViewV3
{
	// The object collector's type name for the terrain tiles the list shows.
	// Shared rather than duplicated: both palettes pass it to the collector on
	// every refill, and a typo in one of them would show an empty list with no
	// other symptom.
	inline constexpr char TILE_TYPE_NAME[] = "TGTerraType";

	// One frame of the icon strip, square.
	inline constexpr int ICON_PIXELS = 32;

	// Puts a tile into the property browser -- what the tile list's Properties
	// context menu item does.
	//
	// Nothing in it is a toolkit's business: it builds a manipulator over the
	// named object and hands it to the property control dialog. Both palettes
	// call it with the name of the selected tile, which is the one thing they
	// have to work out for themselves.
	//
	// Defined in HeightViewV3Mfc.cpp, which is this palette's always-compiled
	// translation unit -- the wx half is behind OBK2_WITH_WX and cannot hold
	// anything the MFC half needs.
	void ShowTileProperties( const std::string &rszTileName );

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
