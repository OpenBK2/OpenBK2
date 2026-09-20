// Generated from MapEditor.rc by scripts/port/rc2menus.py.
//
// The editor's menus, accelerators and toolbar layouts, which were Win32
// resources until the PE resource section stopped being available on every
// platform the editor builds for. This file is the source now.
//
// The MENUITEM flags in the .rc are not here: the editor never read them,
// and every item's enabled and checked state comes from its UPDATE_UI
// handler when the menu opens.

#include "stdafx.h"

#include "ResourceDefines.h"
#include "MapEditorLib/Resources.h"

namespace
{
	const NResources::SMenuItem MENU_IDM_MAIN_CONTEXT_MENU_0[] =
	{
		{ "Select &Tables", ID_MAIN_SELECT, nullptr, 0 },
		{ "&Refresh Tables", ID_MAIN_RELOAD, nullptr, 0 },
		{ "Save A&ll Tables", ID_MAIN_SAVE, nullptr, 0 },
		{ "Register &XDB...", ID_MAIN_REGISTER_XDB, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAIN_CONTEXT_MENU_1_19[] =
	{
		{ "C&heck", ID_OBJECT_CHECK, nullptr, 0 },
		{ "&Export", ID_OBJECT_EXPORT, nullptr, 0 },
		{ "Force E&xport", ID_OBJECT_EXPORT_FORCE, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAIN_CONTEXT_MENU_1[] =
	{
		{ "GET_LABEL", ID_OBJECT_LOAD, nullptr, 0 },
		{ "List &References...", ID_OBJECT_REF_LOOKUP, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "New &Folder", ID_OBJECT_NEW_FOLDER, nullptr, 0 },
		{ "&New Object", ID_OBJECT_NEW, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "New Root F&older", ID_OBJECT_NEW_FOLDER_AT_ROOT, nullptr, 0 },
		{ "Ne&w Root Object", ID_OBJECT_NEW_AT_ROOT, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Cu&t", ID_SELECTION_CUT, nullptr, 0 },
		{ "&Copy", ID_SELECTION_COPY, nullptr, 0 },
		{ "&Paste", ID_SELECTION_PASTE, nullptr, 0 },
		{ "&Delete", ID_SELECTION_CLEAR, nullptr, 0 },
		{ "Rena&me", ID_SELECTION_RENAME, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Select &All", ID_SELECTION_SELECT_ALL, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Export", ID_OBJECT_EXPORT_NO_REF, nullptr, 0 },
		{ "Force E&xport", ID_OBJECT_EXPORT_NO_REF_FORCE, nullptr, 0 },
		{ "Hierarchical Export", 0, MENU_IDM_MAIN_CONTEXT_MENU_1_19, sizeof( MENU_IDM_MAIN_CONTEXT_MENU_1_19 ) / sizeof( MENU_IDM_MAIN_CONTEXT_MENU_1_19[0] ) },
		{ nullptr, 0, nullptr, 0 },
		{ "&Find...", ID_SELECTION_FIND, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAIN_CONTEXT_MENU[] =
	{
		{ "DW_GDB_BROWSER", 0, MENU_IDM_MAIN_CONTEXT_MENU_0, sizeof( MENU_IDM_MAIN_CONTEXT_MENU_0 ) / sizeof( MENU_IDM_MAIN_CONTEXT_MENU_0[0] ) },
		{ "TREE_GDB_BROWSER", 0, MENU_IDM_MAIN_CONTEXT_MENU_1, sizeof( MENU_IDM_MAIN_CONTEXT_MENU_1 ) / sizeof( MENU_IDM_MAIN_CONTEXT_MENU_1[0] ) },
	};

	const NResources::SMenuItem MENU_IDM_PC_CONTEXT_MENU_0[] =
	{
		{ "Re&fesh", ID_PC_REFRESH, nullptr, 0 },
		{ "&Copy", ID_SELECTION_COPY, nullptr, 0 },
		{ "&Paste", ID_SELECTION_PASTE, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Undo", ID_CC_UNDO, nullptr, 0 },
		{ "&Redo", ID_CC_REDO, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Show Hidden", ID_PC_SHOW_HIDDEN, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_PC_CONTEXT_MENU_1[] =
	{
		{ "Re&fesh", ID_PC_REFRESH, nullptr, 0 },
		{ "&Copy", ID_SELECTION_COPY, nullptr, 0 },
		{ "&Paste", ID_SELECTION_PASTE, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Expand", ID_PC_EXPAND, nullptr, 0 },
		{ "&Collapse", ID_PC_COLLAPSE, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Undo", ID_CC_UNDO, nullptr, 0 },
		{ "&Redo", ID_CC_REDO, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Show Hidden", ID_PC_SHOW_HIDDEN, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_PC_CONTEXT_MENU_2[] =
	{
		{ "Re&fesh", ID_PC_REFRESH, nullptr, 0 },
		{ "&Copy", ID_SELECTION_COPY, nullptr, 0 },
		{ "&Paste", ID_SELECTION_PASTE, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Expand", ID_PC_EXPAND, nullptr, 0 },
		{ "&Collapse", ID_PC_COLLAPSE, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Add", ID_PC_ADD_NODE, nullptr, 0 },
		{ "De&lete All", ID_PC_DELETE_ALL_NODES, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Undo", ID_CC_UNDO, nullptr, 0 },
		{ "&Redo", ID_CC_REDO, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Show Hidden", ID_PC_SHOW_HIDDEN, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_PC_CONTEXT_MENU_3[] =
	{
		{ "Re&fesh", ID_PC_REFRESH, nullptr, 0 },
		{ "&Copy", ID_SELECTION_COPY, nullptr, 0 },
		{ "&Paste", ID_SELECTION_PASTE, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Insert", ID_PC_INSERT_NODE, nullptr, 0 },
		{ "&Delete", ID_PC_DELETE_NODE, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Undo", ID_CC_UNDO, nullptr, 0 },
		{ "&Redo", ID_CC_REDO, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Show Hidden", ID_PC_SHOW_HIDDEN, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_PC_CONTEXT_MENU_4[] =
	{
		{ "Re&fesh", ID_PC_REFRESH, nullptr, 0 },
		{ "&Copy", ID_SELECTION_COPY, nullptr, 0 },
		{ "&Paste", ID_SELECTION_PASTE, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Expand", ID_PC_EXPAND, nullptr, 0 },
		{ "&Collapse", ID_PC_COLLAPSE, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Insert", ID_PC_INSERT_NODE, nullptr, 0 },
		{ "&Delete", ID_PC_DELETE_NODE, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Undo", ID_CC_UNDO, nullptr, 0 },
		{ "&Redo", ID_CC_REDO, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Show Hidden", ID_PC_SHOW_HIDDEN, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_PC_CONTEXT_MENU_5[] =
	{
		{ "Re&fesh", ID_PC_REFRESH, nullptr, 0 },
		{ "&Copy", ID_SELECTION_COPY, nullptr, 0 },
		{ "&Paste", ID_SELECTION_PASTE, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Expand", ID_PC_EXPAND, nullptr, 0 },
		{ "&Collapse", ID_PC_COLLAPSE, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Add", ID_PC_ADD_NODE, nullptr, 0 },
		{ "De&lete All", ID_PC_DELETE_ALL_NODES, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Insert", ID_PC_INSERT_NODE, nullptr, 0 },
		{ "&Delete", ID_PC_DELETE_NODE, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Undo", ID_CC_UNDO, nullptr, 0 },
		{ "&Redo", ID_CC_REDO, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Show Hidden", ID_PC_SHOW_HIDDEN, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_PC_CONTEXT_MENU[] =
	{
		{ "NODE", 0, MENU_IDM_PC_CONTEXT_MENU_0, sizeof( MENU_IDM_PC_CONTEXT_MENU_0 ) / sizeof( MENU_IDM_PC_CONTEXT_MENU_0[0] ) },
		{ "MULTI_NODE", 0, MENU_IDM_PC_CONTEXT_MENU_1, sizeof( MENU_IDM_PC_CONTEXT_MENU_1 ) / sizeof( MENU_IDM_PC_CONTEXT_MENU_1[0] ) },
		{ "ARRAY", 0, MENU_IDM_PC_CONTEXT_MENU_2, sizeof( MENU_IDM_PC_CONTEXT_MENU_2 ) / sizeof( MENU_IDM_PC_CONTEXT_MENU_2[0] ) },
		{ "ARRAY_NODE", 0, MENU_IDM_PC_CONTEXT_MENU_3, sizeof( MENU_IDM_PC_CONTEXT_MENU_3 ) / sizeof( MENU_IDM_PC_CONTEXT_MENU_3[0] ) },
		{ "ARRAY_MULTI_NODE", 0, MENU_IDM_PC_CONTEXT_MENU_4, sizeof( MENU_IDM_PC_CONTEXT_MENU_4 ) / sizeof( MENU_IDM_PC_CONTEXT_MENU_4[0] ) },
		{ "ARRAY_ARRAY", 0, MENU_IDM_PC_CONTEXT_MENU_5, sizeof( MENU_IDM_PC_CONTEXT_MENU_5 ) / sizeof( MENU_IDM_PC_CONTEXT_MENU_5[0] ) },
	};

	const NResources::SMenuItem MENU_IDM_TEXT_EDITOR_0[] =
	{
		{ "&Save\tCtrl+S", ID_TE_SAVE, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "E&xit\tAlt+F4", ID_TE_CLOSE, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_TEXT_EDITOR[] =
	{
		{ "Text", 0, MENU_IDM_TEXT_EDITOR_0, sizeof( MENU_IDM_TEXT_EDITOR_0 ) / sizeof( MENU_IDM_TEXT_EDITOR_0[0] ) },
	};

	const NResources::SMenuItem MENU_IDM_LOG_CONTEXT_MENU_0[] =
	{
		{ "&Copy\tCtrl+C", 1081, nullptr, 0 },
		{ "C&lear All\tDel", 1085, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Select &All\tCtrl+A", 1086, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Messages", ID_LOG_SHOW_MESSAGES, nullptr, 0 },
		{ "Warnings", ID_LOG_SHOW_WARNINGS, nullptr, 0 },
		{ "Errors", ID_LOG_SHOW_ERRORS, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_LOG_CONTEXT_MENU[] =
	{
		{ "CONTEXT_MENU", 0, MENU_IDM_LOG_CONTEXT_MENU_0, sizeof( MENU_IDM_LOG_CONTEXT_MENU_0 ) / sizeof( MENU_IDM_LOG_CONTEXT_MENU_0[0] ) },
	};

	const NResources::SMenuEntry MENUS[] =
	{
		{ IDM_MAIN_CONTEXT_MENU, MENU_IDM_MAIN_CONTEXT_MENU, sizeof( MENU_IDM_MAIN_CONTEXT_MENU ) / sizeof( MENU_IDM_MAIN_CONTEXT_MENU[0] ) },
		{ IDM_PC_CONTEXT_MENU, MENU_IDM_PC_CONTEXT_MENU, sizeof( MENU_IDM_PC_CONTEXT_MENU ) / sizeof( MENU_IDM_PC_CONTEXT_MENU[0] ) },
		{ IDM_TEXT_EDITOR, MENU_IDM_TEXT_EDITOR, sizeof( MENU_IDM_TEXT_EDITOR ) / sizeof( MENU_IDM_TEXT_EDITOR[0] ) },
		{ IDM_LOG_CONTEXT_MENU, MENU_IDM_LOG_CONTEXT_MENU, sizeof( MENU_IDM_LOG_CONTEXT_MENU ) / sizeof( MENU_IDM_LOG_CONTEXT_MENU[0] ) },
	};

	const NResources::CMenuTable MENU_TABLE( MENUS, sizeof( MENUS ) / sizeof( MENUS[0] ) );

	const NResources::SAcceleratorEntry ACCEL_IDA_MODAL[] =
	{
		{ 'Y', NResources::ACCEL_MOD_CONTROL, ID_CC_REDO },
		{ VK_BACK, NResources::ACCEL_MOD_ALT, ID_CC_UNDO },
		{ 'Z', NResources::ACCEL_MOD_CONTROL, ID_CC_UNDO },
		{ 'S', NResources::ACCEL_MOD_CONTROL, ID_MAIN_SAVE },
		{ 'T', NResources::ACCEL_MOD_CONTROL, ID_MAIN_SELECT },
		{ VK_OEM_MINUS, NResources::ACCEL_MOD_CONTROL, ID_PC_COLLAPSE_ALL },
		{ VK_SUBTRACT, NResources::ACCEL_MOD_CONTROL, ID_PC_COLLAPSE_ALL },
		{ VK_ADD, NResources::ACCEL_MOD_CONTROL, ID_PC_EXPAND_ALL },
		{ VK_OEM_PLUS, NResources::ACCEL_MOD_CONTROL, ID_PC_EXPAND_ALL },
		{ VK_DELETE, 0, ID_SELECTION_CLEAR },
		{ VK_DELETE, 0, ID_SELECTION_CLEAR },
		{ 'C', NResources::ACCEL_MOD_CONTROL, ID_SELECTION_COPY },
		{ VK_INSERT, NResources::ACCEL_MOD_CONTROL, ID_SELECTION_COPY },
		{ 'X', NResources::ACCEL_MOD_CONTROL, ID_SELECTION_CUT },
		{ 'F', NResources::ACCEL_MOD_CONTROL, ID_SELECTION_FIND },
		{ 'V', NResources::ACCEL_MOD_CONTROL, ID_SELECTION_PASTE },
		{ VK_INSERT, NResources::ACCEL_MOD_SHIFT, ID_SELECTION_PASTE },
		{ 'A', NResources::ACCEL_MOD_CONTROL, ID_SELECTION_SELECT_ALL },
	};

	const NResources::SAcceleratorEntry ACCEL_IDA_MAIN[] =
	{
		{ 'Y', NResources::ACCEL_MOD_CONTROL, ID_CC_REDO },
		{ VK_BACK, NResources::ACCEL_MOD_ALT, ID_CC_UNDO },
		{ 'Z', NResources::ACCEL_MOD_CONTROL, ID_CC_UNDO },
		{ 'N', NResources::ACCEL_MOD_CONTROL, ID_MAIN_NEW },
		{ 'N', NResources::ACCEL_MOD_SHIFT | NResources::ACCEL_MOD_CONTROL, ID_MAIN_NEW_RESOURCE },
		{ 'O', NResources::ACCEL_MOD_CONTROL, ID_MAIN_OPEN },
		{ 'O', NResources::ACCEL_MOD_SHIFT | NResources::ACCEL_MOD_CONTROL, ID_MAIN_OPEN_RESOURCE },
		{ 'S', NResources::ACCEL_MOD_CONTROL, ID_MAIN_SAVE },
		{ 'T', NResources::ACCEL_MOD_CONTROL, ID_MAIN_SELECT },
		{ VK_OEM_MINUS, NResources::ACCEL_MOD_CONTROL, ID_PC_COLLAPSE_ALL },
		{ VK_SUBTRACT, NResources::ACCEL_MOD_CONTROL, ID_PC_COLLAPSE_ALL },
		{ VK_ADD, NResources::ACCEL_MOD_CONTROL, ID_PC_EXPAND_ALL },
		{ VK_OEM_PLUS, NResources::ACCEL_MOD_CONTROL, ID_PC_EXPAND_ALL },
		{ VK_DELETE, 0, ID_SELECTION_CLEAR },
		{ 'C', NResources::ACCEL_MOD_CONTROL, ID_SELECTION_COPY },
		{ VK_INSERT, NResources::ACCEL_MOD_CONTROL, ID_SELECTION_COPY },
		{ 'X', NResources::ACCEL_MOD_CONTROL, ID_SELECTION_CUT },
		{ 'F', NResources::ACCEL_MOD_CONTROL, ID_SELECTION_FIND },
		{ 'V', NResources::ACCEL_MOD_CONTROL, ID_SELECTION_PASTE },
		{ VK_INSERT, NResources::ACCEL_MOD_SHIFT, ID_SELECTION_PASTE },
		{ 'A', NResources::ACCEL_MOD_CONTROL, ID_SELECTION_SELECT_ALL },
		{ 'B', NResources::ACCEL_MOD_SHIFT | NResources::ACCEL_MOD_CONTROL, ID_VIEW_DW_GDB_BROWSER_FIRST },
		{ 'L', NResources::ACCEL_MOD_SHIFT | NResources::ACCEL_MOD_CONTROL, ID_VIEW_DW_LOG },
		{ VK_RETURN, NResources::ACCEL_MOD_SHIFT | NResources::ACCEL_MOD_CONTROL, ID_VIEW_DW_PROPERTY_BROWSER },
	};

	const NResources::SAcceleratorTableEntry ACCELERATORS[] =
	{
		{ IDA_MODAL, ACCEL_IDA_MODAL, sizeof( ACCEL_IDA_MODAL ) / sizeof( ACCEL_IDA_MODAL[0] ) },
		{ IDA_MAIN, ACCEL_IDA_MAIN, sizeof( ACCEL_IDA_MAIN ) / sizeof( ACCEL_IDA_MAIN[0] ) },
	};

	const NResources::CAcceleratorTable ACCEL_TABLE( ACCELERATORS, sizeof( ACCELERATORS ) / sizeof( ACCELERATORS[0] ) );

	const unsigned TOOLBAR_IDT_MAIN[] =
	{
		ID_MAIN_NEW, ID_MAIN_OPEN, ID_MAIN_OPEN_RESOURCE, ID_MAIN_SAVE,
		ID_MAIN_SELECT, 0, ID_TOOLS_RUN_GAME,
	};

	const unsigned TOOLBAR_IDT_PROPERTY_CONTROL[] =
	{
		ID_PC_EXPAND, ID_PC_COLLAPSE, ID_PC_EXPAND_ALL, ID_PC_COLLAPSE_ALL,
		ID_PC_OPTIMAL_WIDTH, ID_PC_REFRESH, ID_PC_ADD_NODE, ID_PC_DELETE_ALL_NODES,
		ID_PC_INSERT_NODE, ID_PC_DELETE_NODE, ID_PC_SHOW_HIDDEN,
	};

	const unsigned TOOLBAR_IDT_VIEW[] =
	{
		ID_VIEW_DW_GDB_BROWSER_FIRST, ID_VIEW_DW_PROPERTY_BROWSER, ID_VIEW_DW_LOG,
	};

	const unsigned TOOLBAR_IDT_CONTROLLER_CONTAINER[] =
	{
		ID_CC_UNDO, ID_CC_REDO, ID_CC_UNDO_ARROW, ID_CC_REDO_ARROW,
	};

	const unsigned TOOLBAR_IDT_SELECTION[] =
	{
		ID_SELECTION_CUT, ID_SELECTION_COPY, ID_SELECTION_PASTE, ID_SELECTION_CLEAR,
		ID_SELECTION_RENAME, ID_SELECTION_FIND, 0, ID_SELECTION_PROPERTIES,
	};

	const unsigned TOOLBAR_IDT_OBJECT[] =
	{
		ID_OBJECT_LOAD, ID_MAIN_OBJECT_LOCATE, ID_OBJECT_REF_LOOKUP, ID_OBJECT_NEW_FOLDER,
		ID_OBJECT_NEW, ID_OBJECT_CHECK, ID_OBJECT_EXPORT, ID_OBJECT_EXPORT_FORCE,
		ID_OBJECT_EXPORT_NO_REF, ID_OBJECT_EXPORT_NO_REF_FORCE, ID_OBJECT_COLOR,
	};

	const NResources::SToolBarEntry TOOLBARS[] =
	{
		{ IDT_MAIN, 16, 16, TOOLBAR_IDT_MAIN, sizeof( TOOLBAR_IDT_MAIN ) / sizeof( TOOLBAR_IDT_MAIN[0] ) },
		{ IDT_PROPERTY_CONTROL, 16, 16, TOOLBAR_IDT_PROPERTY_CONTROL, sizeof( TOOLBAR_IDT_PROPERTY_CONTROL ) / sizeof( TOOLBAR_IDT_PROPERTY_CONTROL[0] ) },
		{ IDT_VIEW, 16, 16, TOOLBAR_IDT_VIEW, sizeof( TOOLBAR_IDT_VIEW ) / sizeof( TOOLBAR_IDT_VIEW[0] ) },
		{ IDT_CONTROLLER_CONTAINER, 16, 16, TOOLBAR_IDT_CONTROLLER_CONTAINER, sizeof( TOOLBAR_IDT_CONTROLLER_CONTAINER ) / sizeof( TOOLBAR_IDT_CONTROLLER_CONTAINER[0] ) },
		{ IDT_SELECTION, 16, 16, TOOLBAR_IDT_SELECTION, sizeof( TOOLBAR_IDT_SELECTION ) / sizeof( TOOLBAR_IDT_SELECTION[0] ) },
		{ IDT_OBJECT, 16, 16, TOOLBAR_IDT_OBJECT, sizeof( TOOLBAR_IDT_OBJECT ) / sizeof( TOOLBAR_IDT_OBJECT[0] ) },
	};

	const NResources::CToolBarTable TOOLBAR_TABLE( TOOLBARS, sizeof( TOOLBARS ) / sizeof( TOOLBARS[0] ) );

}
