// The SVG icon each of the main frame's toolbar commands wears, by name in
// MapEditorLib/res/icons (see the README there). Hand-written, unlike the other
// Resource*.cpp: it is the one place a button's picture is chosen, and
// changing one is changing a line here.
//
// A command missing here keeps its glyph from the old bitmap strip. Run Game
// is missing on purpose: it wears the game's crest (IDI_GAME_LAUNCH).

#include "stdafx.h"

#include "MapEditorLib/Resources.h"
#include "ResourceDefines.h"

namespace
{
const NResources::SIconEntry ICONS[] = {
	// Main
	{ ID_MAIN_NEW, "file-plus" },
	{ ID_MAIN_OPEN, "folder-open" },
	{ ID_MAIN_OPEN_RESOURCE, "folder-search" },
	{ ID_MAIN_SAVE, "save" },
	{ ID_MAIN_SELECT, "square-mouse-pointer" },
	// Undo and redo; their dropdown arrows are drawn by wx.
	{ ID_CC_UNDO, "undo-2" },
	{ ID_CC_REDO, "redo-2" },
	// Selection
	{ ID_SELECTION_CUT, "scissors" },
	{ ID_SELECTION_COPY, "copy" },
	{ ID_SELECTION_PASTE, "clipboard-paste" },
	{ ID_SELECTION_CLEAR, "x" },
	{ ID_SELECTION_RENAME, "text-cursor-input" },
	{ ID_SELECTION_FIND, "search" },
	{ ID_SELECTION_PROPERTIES, "settings-2" },
	// Object
	{ ID_OBJECT_LOAD, "wand" },
	{ ID_MAIN_OBJECT_LOCATE, "crosshair" },
	{ ID_OBJECT_REF_LOOKUP, "git-fork" },
	{ ID_OBJECT_NEW_FOLDER, "folder-plus" },
	{ ID_OBJECT_NEW, "file-plus" },
	{ ID_OBJECT_CHECK, "circle-check" },
	{ ID_OBJECT_EXPORT, "file-output" },
	{ ID_OBJECT_EXPORT_FORCE, "file-output-alert" },
	{ ID_OBJECT_EXPORT_NO_REF, "file-output-unlink" },
	{ ID_OBJECT_EXPORT_NO_REF_FORCE, "file-output-unlink-alert" },
	{ ID_OBJECT_COLOR, "palette" },
	// Property control
	{ ID_PC_EXPAND, "plus" },
	{ ID_PC_COLLAPSE, "minus" },
	{ ID_PC_EXPAND_ALL, "chevrons-up-down" },
	{ ID_PC_COLLAPSE_ALL, "chevrons-down-up" },
	{ ID_PC_OPTIMAL_WIDTH, "move-horizontal" },
	{ ID_PC_REFRESH, "refresh-cw" },
	{ ID_PC_ADD_NODE, "list-plus" },
	{ ID_PC_DELETE_ALL_NODES, "list-x" },
	{ ID_PC_INSERT_NODE, "between-horizontal-start" },
	{ ID_PC_DELETE_NODE, "list-minus" },
	{ ID_PC_SHOW_HIDDEN, "eye" },
	// View
	{ ID_VIEW_DW_GDB_BROWSER_FIRST, "folder-tree" },
	{ ID_VIEW_DW_PROPERTY_BROWSER, "list-tree" },
	{ ID_VIEW_DW_LOG, "scroll-text" },
};

const NResources::CIconTable TABLE( ICONS, sizeof( ICONS ) / sizeof( ICONS[0] ) );
} // namespace
