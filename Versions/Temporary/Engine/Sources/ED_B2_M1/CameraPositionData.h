#pragma once

#include "PaletteCommands.h"

#include <cstdint>

// What the start-camera-positions palette holds, and how the editor asks for
// it. Both are toolkit-neutral and both used to live in CameraPositionWindow.h
// beside the MFC window; they are here so the wx palette can share them rather
// than restate them.

struct SCameraPositionWindowData
{
	int nPlayerIndex;
	int nPlayerCount;
	bool bAllParams;
	//
	SCameraPositionWindowData() :
		nPlayerIndex( -1 ),
		nPlayerCount( 0 ),
		bAllParams( false )
	{
	}
	//
	void Clear()
	{
		nPlayerIndex = -1;
		nPlayerCount = 0;
		bAllParams = false;
	}
};


// CMapInfoState drives this palette with the two window commands every palette
// takes, so the dispatch is CPaletteCommands' and only the reading and writing
// of controls is written per implementation.
typedef CPaletteCommands<SCameraPositionWindowData> CCameraPositionCommands;
