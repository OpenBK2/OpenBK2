#pragma once

#include "MapEditorLib_export.h"

#include <string>

// The editor's Win32 resources -- strings, menus, accelerators, icons, cursors,
// bitmaps, toolbars -- found without MFC.
//
// MFC found them through its resource handle, which callers swapped with
// AfxSetResourceHandle to point at the DLL whose .rc had the id, and
// AfxFindResourceHandle, which searched that handle and then MFC's extension
// DLL chain. None of that is needed: B2_MapEditor.exe's b2_res.rc includes
// MapEditor.rc and ED_B2_M1.rc, so every resource the editor asks for is in
// the executable, and a module whose resources are not can register itself.
//
// Strings come back as UTF-8, read with LoadStringW, like every narrow string
// in the tree.
namespace NResources
{
	// The module whose resources hold pszName of pszType, as the Win32
	// FindResource calls take them (MAKEINTRESOURCE ids work): the executable
	// first, then the registered modules in the order they registered. Null
	// when none has it.
	MAPEDITORLIB_EXPORT HINSTANCE FindModule( LPCSTR pszName, LPCSTR pszType );

	// A module to search after the executable.
	MAPEDITORLIB_EXPORT void RegisterModule( HINSTANCE hModule );

	// The string resource nID, as UTF-8. Empty when there is none, which the
	// string tables never use for a real string.
	MAPEDITORLIB_EXPORT std::string GetString( unsigned nID );

	// The same, answering whether the string exists.
	MAPEDITORLIB_EXPORT bool GetString( unsigned nID, std::string *pszText );
}
