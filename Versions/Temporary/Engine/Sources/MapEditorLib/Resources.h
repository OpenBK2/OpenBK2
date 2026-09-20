#pragma once

#include "MapEditorLib_export.h"

#include <cstddef>
#include <string>

// The editor's resources: strings, menus, accelerators, toolbars, bitmaps,
// icons and cursors.
//
// These were Win32 resources in a section of the PE file, found first through
// MFC's resource handle chain and then, once MFC was gone, through
// FindResource over the executable and the modules that registered themselves.
// ELF has no resource section, so they are generated C++ tables now, one set
// per module in that module's ResourceStrings.cpp, ResourceBinary.cpp and
// ResourceMenus.cpp, each registering itself as it is constructed.
//
// What stays in the .rc, on Windows only, is what the shell reads out of the
// PE and nothing else can provide: VERSIONINFO, the application icon and the
// manifest. The icons the editor draws itself are in the tables as well.
//
// Strings are UTF-8, like every narrow string in the tree.
namespace NResources
{
	// One entry of a module's generated string table.
	//
	// The strings were a Win32 STRINGTABLE, found with LoadStringW, until the
	// PE resource section stopped being available on every platform the editor
	// builds for. They are generated C++ now, one table per module, in that
	// module's ResourceStrings.cpp.
	struct SStringEntry
	{
		unsigned nID;
		// UTF-8, like every narrow string in the tree. The .rc files these came
		// from were pure ASCII, so nothing had to be re-encoded.
		const char *pszText;
	};

	// A module's table, made as a namespace-scope object in the generated file
	// so it registers itself before main runs, as the resource section was
	// mapped before it. Tables are searched in registration order; the ids of
	// the editor's two tables do not overlap, so that order does not decide
	// anything.
	class CStringTable
	{
	public:
		MAPEDITORLIB_EXPORT CStringTable( const SStringEntry *pEntries, size_t nCount );
	};

	// One entry of a module's generated binary table: a whole .bmp file, as it
	// sat in the resource section, in initialised read-only data.
	struct SBinaryEntry
	{
		unsigned nID;
		const unsigned char *pData;
		size_t nSize;
	};

	// A module's binary table, registered like CStringTable above.
	class CBinaryTable
	{
	public:
		MAPEDITORLIB_EXPORT CBinaryTable( const SBinaryEntry *pEntries, size_t nCount );
	};

	// The bytes of the binary resource nID, or false and nothing written when
	// no module has it. The bytes outlive the call: they are in the image.
	MAPEDITORLIB_EXPORT bool GetBinaryResource( unsigned nID, const unsigned char **ppData, size_t *pnSize );

	// A menu item: a command, a separator, or a popup with children.
	//
	// The MENUITEM flags the .rc carried are not here. The editor never read
	// them: every item's enabled and checked state comes from its UPDATE_UI
	// handler when the menu opens.
	struct SMenuItem
	{
		// UTF-8, null for a separator.
		const char *pszText;
		// 0 for a popup or a separator.
		unsigned nCommandID;
		const SMenuItem *pSubItems;
		size_t nSubCount;
	};

	struct SMenuEntry
	{
		unsigned nID;
		const SMenuItem *pItems;
		size_t nCount;
	};

	class CMenuTable
	{
	public:
		MAPEDITORLIB_EXPORT CMenuTable( const SMenuEntry *pEntries, size_t nCount );
	};

	MAPEDITORLIB_EXPORT bool GetMenu( unsigned nID, const SMenuItem **ppItems, size_t *pnCount );

	// What a VIRTKEY accelerator is held down with. Accelerators that were not
	// VIRTKEY are not carried over: the editor skipped those already.
	enum EAcceleratorModifier
	{
		ACCEL_MOD_SHIFT = 1,
		ACCEL_MOD_CONTROL = 2,
		ACCEL_MOD_ALT = 4,
	};

	struct SAcceleratorEntry
	{
		// A VK_ code, from windows.h or port/vkcodes.h.
		unsigned nKey;
		unsigned nModifiers;
		unsigned nCommandID;
	};

	struct SAcceleratorTableEntry
	{
		unsigned nID;
		const SAcceleratorEntry *pEntries;
		size_t nCount;
	};

	class CAcceleratorTable
	{
	public:
		MAPEDITORLIB_EXPORT CAcceleratorTable( const SAcceleratorTableEntry *pEntries, size_t nCount );
	};

	MAPEDITORLIB_EXPORT bool GetAccelerators( unsigned nID, const SAcceleratorEntry **ppEntries, size_t *pnCount );

	// A toolbar's button size and the command each button sends, 0 for a
	// separator. This was MFC's CToolBarData, read out of a RT_TOOLBAR block.
	struct SToolBarEntry
	{
		unsigned nID;
		int nWidth;
		int nHeight;
		const unsigned *pCommands;
		size_t nCount;
	};

	class CToolBarTable
	{
	public:
		MAPEDITORLIB_EXPORT CToolBarTable( const SToolBarEntry *pEntries, size_t nCount );
	};

	MAPEDITORLIB_EXPORT const SToolBarEntry* GetToolBar( unsigned nID );

	// The string nID, as UTF-8. Empty when there is none, which the string
	// tables never use for a real string.
	MAPEDITORLIB_EXPORT std::string GetString( unsigned nID );

	// The same, answering whether the string exists.
	MAPEDITORLIB_EXPORT bool GetString( unsigned nID, std::string *pszText );
}
