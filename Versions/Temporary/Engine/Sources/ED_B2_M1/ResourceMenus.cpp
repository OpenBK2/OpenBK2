// Generated from ED_B2_M1.rc by scripts/port/rc2menus.py.
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
	const NResources::SMenuItem MENU_IDM_MAIN_0_14[] =
	{
		{ "Recent Map 0", ID_MAIN_RECENT_0, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAIN_0_15[] =
	{
		{ "Recent Resorce 0", ID_MAIN_RECENT_RESOURCE_0, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAIN_0[] =
	{
		{ "&New\t Ctrl+N", ID_MAIN_NEW, nullptr, 0 },
		{ "&Open\tCtrl+O", ID_MAIN_OPEN, nullptr, 0 },
		{ "O&pen Resource\tCtrl+Shift+O", ID_MAIN_OPEN_RESOURCE, nullptr, 0 },
		{ "&Close", ID_MAIN_CLOSE, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Select &Tables\tCtrl+T", ID_MAIN_SELECT, nullptr, 0 },
		{ "&Refresh Tables", ID_MAIN_RELOAD, nullptr, 0 },
		{ "Save A&ll\t Ctrl+S", ID_MAIN_SAVE, nullptr, 0 },
		{ "Register &XDB...", ID_MAIN_REGISTER_XDB, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "New MOD", ID_MAIN_NEW_MOD, nullptr, 0 },
		{ "Open MOD", ID_MAIN_OPEN_MOD, nullptr, 0 },
		{ "Close MOD", ID_MAIN_CLOSE_MOD, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Recent &Maps", 0, MENU_IDM_MAIN_0_14, sizeof( MENU_IDM_MAIN_0_14 ) / sizeof( MENU_IDM_MAIN_0_14[0] ) },
		{ "Recent Re&sources", 0, MENU_IDM_MAIN_0_15, sizeof( MENU_IDM_MAIN_0_15 ) / sizeof( MENU_IDM_MAIN_0_15[0] ) },
		{ nullptr, 0, nullptr, 0 },
		{ "E&xit\tAlt+F4", ID_APP_EXIT, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAIN_1[] =
	{
		{ "&Undo\tCtrl+Z", ID_CC_UNDO, nullptr, 0 },
		{ "&Redo\tCtrl+Y", ID_CC_REDO, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Cu&t\tCtrl+X", ID_SELECTION_CUT, nullptr, 0 },
		{ "&Copy\tCtrl+C", ID_SELECTION_COPY, nullptr, 0 },
		{ "&Paste\tCtrl+V", ID_SELECTION_PASTE, nullptr, 0 },
		{ "&Delete\tDel", ID_SELECTION_CLEAR, nullptr, 0 },
		{ "Rena&me", ID_SELECTION_RENAME, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Select &All\tCtrl+A", ID_SELECTION_SELECT_ALL, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Find...\tCtrl+F", ID_SELECTION_FIND, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAIN_2_11[] =
	{
		{ "C&heck", ID_OBJECT_CHECK, nullptr, 0 },
		{ "&Export", ID_OBJECT_EXPORT, nullptr, 0 },
		{ "Force E&xport", ID_OBJECT_EXPORT_FORCE, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAIN_2[] =
	{
		{ "Edit &Selection", ID_OBJECT_LOAD, nullptr, 0 },
		{ "List &References...", ID_OBJECT_REF_LOOKUP, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "New &Folder", ID_OBJECT_NEW_FOLDER, nullptr, 0 },
		{ "&New Object\tIns", ID_OBJECT_NEW, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "New Root F&older", ID_OBJECT_NEW_FOLDER_AT_ROOT, nullptr, 0 },
		{ "Ne&w Root Object", ID_OBJECT_NEW_AT_ROOT, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Export", ID_OBJECT_EXPORT_NO_REF, nullptr, 0 },
		{ "Force E&xport", ID_OBJECT_EXPORT_NO_REF_FORCE, nullptr, 0 },
		{ "Hierarchical Export", 0, MENU_IDM_MAIN_2_11, sizeof( MENU_IDM_MAIN_2_11 ) / sizeof( MENU_IDM_MAIN_2_11[0] ) },
		{ nullptr, 0, nullptr, 0 },
		{ "&Properties", ID_MAIN_OBJECT_LOCATE, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAIN_3_7[] =
	{
		{ "Game Database Window 0", ID_VIEW_DW_GDB_BROWSER_FIRST, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&New Window", ID_VIEW_DW_GDB_BROWSER_NEW, nullptr, 0 },
		{ "&Remove Window", ID_VIEW_DW_GDB_BROWSER_REMOVE, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAIN_3[] =
	{
		{ "&Main Toolbar", ID_VIEW_TOOLBAR_MAIN, nullptr, 0 },
		{ "&Undo / Redo Toolbar", ID_VIEW_TOOLBAR_CONTROLLER_CONTAINER, nullptr, 0 },
		{ "&Edit Toolbar", ID_VIEW_TOOLBAR_SELECTION, nullptr, 0 },
		{ "&Object Toolbar", ID_VIEW_TOOLBAR_OBJECT, nullptr, 0 },
		{ "&Property Control Toolbar", ID_VIEW_TOOLBAR_PROPERTY_CONTROL, nullptr, 0 },
		{ "&View  Toolbar", ID_VIEW_TOOLBAR_VIEW, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Game Database", 0, MENU_IDM_MAIN_3_7, sizeof( MENU_IDM_MAIN_3_7 ) / sizeof( MENU_IDM_MAIN_3_7[0] ) },
		{ "&Selection Property Window\tCtr+Shift+Enter", ID_VIEW_DW_PROPERTY_BROWSER, nullptr, 0 },
		{ "&Log Window\tCtrl+Shift+L", ID_VIEW_DW_LOG, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Reset GUI", ID_VIEW_RESET_GUI, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAIN_4[] =
	{
		{ "Create Script Path Points", ID_TOOLS_CREATE_SCRIPT_PATH_POINTS, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "De&bug Check Map", ID_TOOLS_DEBUG_CHECK_MAP, nullptr, 0 },
		{ "Regenerate &Terrain", ID_TOOLS_REGEN_GEOMETRY, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Create &Infantry Animations", ID_TOOLS_CREATE_INF_ANIMS, nullptr, 0 },
		{ "Create &Ack Sets", ID_TOOLS_CREATE_ACK_SETS, nullptr, 0 },
		{ "Create &VisObj", ID_TOOLS_CREATE_VIS_OBJ, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Customize...", ID_TOOLS_CUSTOMIZE, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAIN_5[] =
	{
		{ "&Contents\tF1", ID_HELP_CONTENTS, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&About...", ID_HELP_ABOUT, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAIN[] =
	{
		{ "&File", 0, MENU_IDM_MAIN_0, sizeof( MENU_IDM_MAIN_0 ) / sizeof( MENU_IDM_MAIN_0[0] ) },
		{ "&Edit", 0, MENU_IDM_MAIN_1, sizeof( MENU_IDM_MAIN_1 ) / sizeof( MENU_IDM_MAIN_1[0] ) },
		{ "&Object", 0, MENU_IDM_MAIN_2, sizeof( MENU_IDM_MAIN_2 ) / sizeof( MENU_IDM_MAIN_2[0] ) },
		{ "&View", 0, MENU_IDM_MAIN_3, sizeof( MENU_IDM_MAIN_3 ) / sizeof( MENU_IDM_MAIN_3[0] ) },
		{ "&Tools", 0, MENU_IDM_MAIN_4, sizeof( MENU_IDM_MAIN_4 ) / sizeof( MENU_IDM_MAIN_4[0] ) },
		{ "&Help", 0, MENU_IDM_MAIN_5, sizeof( MENU_IDM_MAIN_5 ) / sizeof( MENU_IDM_MAIN_5[0] ) },
	};

	const NResources::SMenuItem MENU_IDM_MAPINFO_0_14[] =
	{
		{ "Recent Map 0", ID_MAIN_RECENT_0, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAPINFO_0_15[] =
	{
		{ "Recent Resorce 0", ID_MAIN_RECENT_RESOURCE_0, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAPINFO_0[] =
	{
		{ "&New\t Ctrl+N", ID_MAIN_NEW, nullptr, 0 },
		{ "&Open\tCtrl+O", ID_MAIN_OPEN, nullptr, 0 },
		{ "O&pen Resource\tCtrl+Shift+O", ID_MAIN_OPEN_RESOURCE, nullptr, 0 },
		{ "&Close", ID_MAIN_CLOSE, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Select &Tables\tCtrl+T", ID_MAIN_SELECT, nullptr, 0 },
		{ "&Refresh Tables", ID_MAIN_RELOAD, nullptr, 0 },
		{ "Save A&ll\t Ctrl+S", ID_MAIN_SAVE, nullptr, 0 },
		{ "Register &XDB...", ID_MAIN_REGISTER_XDB, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "New MOD", 1073, nullptr, 0 },
		{ "Open MOD", 1074, nullptr, 0 },
		{ "Close MOD", 1075, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Recent &Maps", 0, MENU_IDM_MAPINFO_0_14, sizeof( MENU_IDM_MAPINFO_0_14 ) / sizeof( MENU_IDM_MAPINFO_0_14[0] ) },
		{ "Recent Re&sources", 0, MENU_IDM_MAPINFO_0_15, sizeof( MENU_IDM_MAPINFO_0_15 ) / sizeof( MENU_IDM_MAPINFO_0_15[0] ) },
		{ nullptr, 0, nullptr, 0 },
		{ "E&xit\tAlt+F4", ID_APP_EXIT, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAPINFO_1[] =
	{
		{ "&Undo\tCtrl+Z", ID_CC_UNDO, nullptr, 0 },
		{ "&Redo\tCtrl+Y", ID_CC_REDO, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Cu&t\tCtrl+X", ID_SELECTION_CUT, nullptr, 0 },
		{ "&Copy\tCtrl+C", ID_SELECTION_COPY, nullptr, 0 },
		{ "&Paste\tCtrl+V", ID_SELECTION_PASTE, nullptr, 0 },
		{ "&Delete\tDel", ID_SELECTION_CLEAR, nullptr, 0 },
		{ "Rena&me", ID_SELECTION_RENAME, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Select &All\tCtrl+A", ID_SELECTION_SELECT_ALL, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Find...\tCtrl+F", ID_SELECTION_FIND, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAPINFO_2_11[] =
	{
		{ "C&heck", 65535, nullptr, 0 },
		{ "&Export", ID_OBJECT_EXPORT, nullptr, 0 },
		{ "Force E&xport", ID_OBJECT_EXPORT_FORCE, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAPINFO_2[] =
	{
		{ "Edit &Selection", ID_OBJECT_LOAD, nullptr, 0 },
		{ "List &References...", ID_OBJECT_REF_LOOKUP, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "New &Folder", ID_OBJECT_NEW_FOLDER, nullptr, 0 },
		{ "&New Object\tIns", ID_OBJECT_NEW, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "New Root F&older", ID_OBJECT_NEW_FOLDER_AT_ROOT, nullptr, 0 },
		{ "Ne&w Root Object", ID_OBJECT_NEW_AT_ROOT, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Export", ID_OBJECT_EXPORT_NO_REF, nullptr, 0 },
		{ "Force E&xport", ID_OBJECT_EXPORT_NO_REF_FORCE, nullptr, 0 },
		{ "Hierarchical Export", 0, MENU_IDM_MAPINFO_2_11, sizeof( MENU_IDM_MAPINFO_2_11 ) / sizeof( MENU_IDM_MAPINFO_2_11[0] ) },
		{ nullptr, 0, nullptr, 0 },
		{ "&Properties", ID_MAIN_OBJECT_LOCATE, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAPINFO_3_7[] =
	{
		{ "Game Database Window 0", ID_VIEW_DW_GDB_BROWSER_FIRST, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&New Window", ID_VIEW_DW_GDB_BROWSER_NEW, nullptr, 0 },
		{ "&Remove Window", ID_VIEW_DW_GDB_BROWSER_REMOVE, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAPINFO_3[] =
	{
		{ "&Main Toolbar", ID_VIEW_TOOLBAR_MAIN, nullptr, 0 },
		{ "&Undo / Redo Toolbar", ID_VIEW_TOOLBAR_CONTROLLER_CONTAINER, nullptr, 0 },
		{ "&Edit Toolbar", ID_VIEW_TOOLBAR_SELECTION, nullptr, 0 },
		{ "&Object Toolbar", ID_VIEW_TOOLBAR_OBJECT, nullptr, 0 },
		{ "&Property Control Toolbar", ID_VIEW_TOOLBAR_PROPERTY_CONTROL, nullptr, 0 },
		{ "&View  Toolbar", ID_VIEW_TOOLBAR_VIEW, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Game Database", 0, MENU_IDM_MAPINFO_3_7, sizeof( MENU_IDM_MAPINFO_3_7 ) / sizeof( MENU_IDM_MAPINFO_3_7[0] ) },
		{ "&Selection Property Window\tCtr+Shift+Enter", ID_VIEW_DW_PROPERTY_BROWSER, nullptr, 0 },
		{ "&Log Window\tCtrl+Shift+L", ID_VIEW_DW_LOG, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Additional View Toolbar", ID_MI_VIEW_VIEW_TOOLBAR, nullptr, 0 },
		{ "&Tools Toolbar", ID_MI_VIEW_TOOLS_TOOLBAR, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "M&inimap Window", ID_MI_VIEW_MINIMAP, nullptr, 0 },
		{ "Tools &Window", ID_MI_VIEW_TOOL, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Reset GUI", ID_VIEW_RESET_GUI, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAPINFO_4[] =
	{
		{ "Start Mission in &Game", ID_TOOLS_RUN_GAME, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Reset Ca&mera", ID_TOOLS_RESET_CAMERA, nullptr, 0 },
		{ "&Update Roads, Cliffs, Rivers", ID_TOOLS_UPDATE_VSO, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Fit To AI &Grid", ID_TOOLS_FIT_TO_GRID, nullptr, 0 },
		{ "Rotate Multiple To 90 &Degree", ID_TOOLS_ROTATE_90, nullptr, 0 },
		{ "Draw S&hoot Areas", ID_TOOLS_DRAW_SHOOT_AREAS, nullptr, 0 },
		{ "Dra&w AI Map", ID_TOOLS_DRAW_AI_MAP, nullptr, 0 },
		{ "Draw Passabilit&y", ID_TOOLS_DRAW_PASSABILITY, nullptr, 0 },
		{ "&Show Grid", ID_TOOLS_SHOW_GRID, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "View &Filter", ID_VIEW_FILTER, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAPINFO_5[] =
	{
		{ "Create Script Path Points", ID_TOOLS_CREATE_SCRIPT_PATH_POINTS, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "De&bug Check Map", ID_TOOLS_DEBUG_CHECK_MAP, nullptr, 0 },
		{ "Regenerate &Terrain", ID_TOOLS_REGEN_GEOMETRY, nullptr, 0 },
		{ "&Update Terrain Objects Normals", ID_TOOLS_REGEN_VSO_NORMALS, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Create &Infantry Animations", ID_TOOLS_CREATE_INF_ANIMS, nullptr, 0 },
		{ "Create &Ack Sets", ID_TOOLS_CREATE_ACK_SETS, nullptr, 0 },
		{ "Create &VisObj", ID_TOOLS_CREATE_VIS_OBJ, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Customize...", ID_TOOLS_CUSTOMIZE, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAPINFO_6[] =
	{
		{ "&Contents\tF1", ID_HELP_CONTENTS, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&About...", ID_HELP_ABOUT, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAPINFO[] =
	{
		{ "&File", 0, MENU_IDM_MAPINFO_0, sizeof( MENU_IDM_MAPINFO_0 ) / sizeof( MENU_IDM_MAPINFO_0[0] ) },
		{ "&Edit", 0, MENU_IDM_MAPINFO_1, sizeof( MENU_IDM_MAPINFO_1 ) / sizeof( MENU_IDM_MAPINFO_1[0] ) },
		{ "&Object", 0, MENU_IDM_MAPINFO_2, sizeof( MENU_IDM_MAPINFO_2 ) / sizeof( MENU_IDM_MAPINFO_2[0] ) },
		{ "&View", 0, MENU_IDM_MAPINFO_3, sizeof( MENU_IDM_MAPINFO_3 ) / sizeof( MENU_IDM_MAPINFO_3[0] ) },
		{ "&Map", 0, MENU_IDM_MAPINFO_4, sizeof( MENU_IDM_MAPINFO_4 ) / sizeof( MENU_IDM_MAPINFO_4[0] ) },
		{ "&Tools", 0, MENU_IDM_MAPINFO_5, sizeof( MENU_IDM_MAPINFO_5 ) / sizeof( MENU_IDM_MAPINFO_5[0] ) },
		{ "&Help", 0, MENU_IDM_MAPINFO_6, sizeof( MENU_IDM_MAPINFO_6 ) / sizeof( MENU_IDM_MAPINFO_6[0] ) },
	};

	const NResources::SMenuItem MENU_IDM_MODEL_0_14[] =
	{
		{ "Recent Map 0", ID_MAIN_RECENT_0, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MODEL_0_15[] =
	{
		{ "Recent Resorce 0", ID_MAIN_RECENT_RESOURCE_0, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MODEL_0[] =
	{
		{ "&New\t Ctrl+N", ID_MAIN_NEW, nullptr, 0 },
		{ "&Open\tCtrl+O", ID_MAIN_OPEN, nullptr, 0 },
		{ "O&pen Resource\tCtrl+Shift+O", ID_MAIN_OPEN_RESOURCE, nullptr, 0 },
		{ "&Close", ID_MAIN_CLOSE, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Select &Tables\tCtrl+T", ID_MAIN_SELECT, nullptr, 0 },
		{ "&Refresh Tables", ID_MAIN_RELOAD, nullptr, 0 },
		{ "Save A&ll\t Ctrl+S", ID_MAIN_SAVE, nullptr, 0 },
		{ "Register &XDB...", ID_MAIN_REGISTER_XDB, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "New MOD", 1073, nullptr, 0 },
		{ "Open MOD", 1074, nullptr, 0 },
		{ "Close MOD", 1075, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Recent &Maps", 0, MENU_IDM_MODEL_0_14, sizeof( MENU_IDM_MODEL_0_14 ) / sizeof( MENU_IDM_MODEL_0_14[0] ) },
		{ "Recent Re&sources", 0, MENU_IDM_MODEL_0_15, sizeof( MENU_IDM_MODEL_0_15 ) / sizeof( MENU_IDM_MODEL_0_15[0] ) },
		{ nullptr, 0, nullptr, 0 },
		{ "E&xit\tAlt+F4", ID_APP_EXIT, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MODEL_1[] =
	{
		{ "&Undo\tCtrl+Z", ID_CC_UNDO, nullptr, 0 },
		{ "&Redo\tCtrl+Y", ID_CC_REDO, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Cu&t\tCtrl+X", ID_SELECTION_CUT, nullptr, 0 },
		{ "&Copy\tCtrl+C", ID_SELECTION_COPY, nullptr, 0 },
		{ "&Paste\tCtrl+V", ID_SELECTION_PASTE, nullptr, 0 },
		{ "&Delete\tDel", ID_SELECTION_CLEAR, nullptr, 0 },
		{ "Rena&me", ID_SELECTION_RENAME, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Select &All\tCtrl+A", ID_SELECTION_SELECT_ALL, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Find...\tCtrl+F", ID_SELECTION_FIND, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MODEL_2_11[] =
	{
		{ "C&heck", 65535, nullptr, 0 },
		{ "&Export", ID_OBJECT_EXPORT, nullptr, 0 },
		{ "Force E&xport", ID_OBJECT_EXPORT_FORCE, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MODEL_2[] =
	{
		{ "Edit &Selection", ID_OBJECT_LOAD, nullptr, 0 },
		{ "List &References...", ID_OBJECT_REF_LOOKUP, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "New &Folder", ID_OBJECT_NEW_FOLDER, nullptr, 0 },
		{ "&New Object\tIns", ID_OBJECT_NEW, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "New Root F&older", ID_OBJECT_NEW_FOLDER_AT_ROOT, nullptr, 0 },
		{ "Ne&w Root Object", ID_OBJECT_NEW_AT_ROOT, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Export", ID_OBJECT_EXPORT_NO_REF, nullptr, 0 },
		{ "Force E&xport", ID_OBJECT_EXPORT_NO_REF_FORCE, nullptr, 0 },
		{ "Hierarchical Export", 0, MENU_IDM_MODEL_2_11, sizeof( MENU_IDM_MODEL_2_11 ) / sizeof( MENU_IDM_MODEL_2_11[0] ) },
		{ nullptr, 0, nullptr, 0 },
		{ "&Properties", ID_MAIN_OBJECT_LOCATE, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MODEL_3_7[] =
	{
		{ "Game Database Window &0", ID_VIEW_DW_GDB_BROWSER_FIRST, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&New Window", ID_VIEW_DW_GDB_BROWSER_NEW, nullptr, 0 },
		{ "&Remove Window", ID_VIEW_DW_GDB_BROWSER_REMOVE, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MODEL_3[] =
	{
		{ "&Main Toolbar", ID_VIEW_TOOLBAR_MAIN, nullptr, 0 },
		{ "&Undo / Redo Toolbar", ID_VIEW_TOOLBAR_CONTROLLER_CONTAINER, nullptr, 0 },
		{ "&Edit Toolbar", ID_VIEW_TOOLBAR_SELECTION, nullptr, 0 },
		{ "&Object Toolbar", ID_VIEW_TOOLBAR_OBJECT, nullptr, 0 },
		{ "&Property Control Toolbar", ID_VIEW_TOOLBAR_PROPERTY_CONTROL, nullptr, 0 },
		{ "&View  Toolbar", ID_VIEW_TOOLBAR_VIEW, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Game Database", 0, MENU_IDM_MODEL_3_7, sizeof( MENU_IDM_MODEL_3_7 ) / sizeof( MENU_IDM_MODEL_3_7[0] ) },
		{ "&Selection Property Window\tCtr+Shift+Enter", ID_VIEW_DW_PROPERTY_BROWSER, nullptr, 0 },
		{ "&Log Window\tCtrl+Shift+L", ID_VIEW_DW_LOG, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Mo&del Toolbar", ID_MODEL_VIEW_TOOLBAR, nullptr, 0 },
		{ "Model Tool &Window", ID_MODEL_VIEW_TOOL, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Reset GUI", ID_VIEW_RESET_GUI, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MODEL_4[] =
	{
		{ "Reload &Editor", ID_MODEL_RELOAD_EDITOR, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Show &Terrain", ID_MODEL_DRAW_TERRAIN, nullptr, 0 },
		{ "Show &Animations", ID_MODEL_DRAW_ANIMATIONS, nullptr, 0 },
		{ "Show AI &Geometry", 3685, nullptr, 0 },
		{ "Set &Light...", ID_MODEL_SET_LIGHT, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Center Camera", ID_MODEL_CENTER_CAMERA, nullptr, 0 },
		{ "&Save Camera", ID_MODEL_SAVE_CAMERA, nullptr, 0 },
		{ "&Reset Camera", ID_MODEL_RESET_CAMERA, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Animation Speed &Down", ID_MODEL_SPEED_DOWN, nullptr, 0 },
		{ "Animation Speed &Up", ID_MODEL_SPEED_UP, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MODEL_5[] =
	{
		{ "Create Script Path Points", ID_TOOLS_CREATE_SCRIPT_PATH_POINTS, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "De&bug Check Map", ID_TOOLS_DEBUG_CHECK_MAP, nullptr, 0 },
		{ "Regenerate &Geometry", ID_TOOLS_REGEN_GEOMETRY, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Create &Infantry Animations", ID_TOOLS_CREATE_INF_ANIMS, nullptr, 0 },
		{ "Create &Ack Sets", ID_TOOLS_CREATE_ACK_SETS, nullptr, 0 },
		{ "Create &VisObj", ID_TOOLS_CREATE_VIS_OBJ, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Customize...", ID_TOOLS_CUSTOMIZE, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MODEL_6[] =
	{
		{ "&Contents\tF1", ID_HELP_CONTENTS, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&About...", ID_HELP_ABOUT, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MODEL[] =
	{
		{ "&File", 0, MENU_IDM_MODEL_0, sizeof( MENU_IDM_MODEL_0 ) / sizeof( MENU_IDM_MODEL_0[0] ) },
		{ "&Edit", 0, MENU_IDM_MODEL_1, sizeof( MENU_IDM_MODEL_1 ) / sizeof( MENU_IDM_MODEL_1[0] ) },
		{ "&Object", 0, MENU_IDM_MODEL_2, sizeof( MENU_IDM_MODEL_2 ) / sizeof( MENU_IDM_MODEL_2[0] ) },
		{ "&View", 0, MENU_IDM_MODEL_3, sizeof( MENU_IDM_MODEL_3 ) / sizeof( MENU_IDM_MODEL_3[0] ) },
		{ "Model", 0, MENU_IDM_MODEL_4, sizeof( MENU_IDM_MODEL_4 ) / sizeof( MENU_IDM_MODEL_4[0] ) },
		{ "&Tools", 0, MENU_IDM_MODEL_5, sizeof( MENU_IDM_MODEL_5 ) / sizeof( MENU_IDM_MODEL_5[0] ) },
		{ "&Help", 0, MENU_IDM_MODEL_6, sizeof( MENU_IDM_MODEL_6 ) / sizeof( MENU_IDM_MODEL_6[0] ) },
	};

	const NResources::SMenuItem MENU_IDM_MAPINFO_CONTEXT_MENU_0[] =
	{
		{ "&List", ID_MIMOOLCM_LIST, nullptr, 0 },
		{ "&Thumbnails", ID_MIMOOLCM_THUMBNAILS, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "P&roperties", ID_MIMOOLCM_PROPERTIES, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAPINFO_CONTEXT_MENU_1[] =
	{
		{ "&List", ID_MIVSOOLCM_LIST, nullptr, 0 },
		{ "&Thumbnails", ID_MIVSOOLCM_THUMBNAILS, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "P&roperties", ID_MIVSOOLCM_PROPERTIES, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAPINFO_CONTEXT_MENU_2[] =
	{
		{ "&Generate Minimap Image ...", ID_MIMCO_GENERATE_MINIMAP_IMAGE, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAPINFO_CONTEXT_MENU_3[] =
	{
		{ "&List", ID_MITHV3_LIST, nullptr, 0 },
		{ "&Thumbnails", ID_MITHV3_THUMBNAILS, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "P&roperties", ID_MITHV3_PROPERTIES, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAPINFO_CONTEXT_MENU_4[] =
	{
		{ "Insert Key", ID_MIMOVED_INSERT_KEY, nullptr, 0 },
		{ "Save Key", ID_MIMOVED_SAVE_KEY, nullptr, 0 },
		{ "Key Settings", ID_MIMOVED_KEY_SETTINGS, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Delete keys", ID_MIMOVED_DELETE_KEYS, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MAPINFO_CONTEXT_MENU[] =
	{
		{ "MI_MAPOBJECT_OBJECT_LIST", 0, MENU_IDM_MAPINFO_CONTEXT_MENU_0, sizeof( MENU_IDM_MAPINFO_CONTEXT_MENU_0 ) / sizeof( MENU_IDM_MAPINFO_CONTEXT_MENU_0[0] ) },
		{ "MI_VSO_OBJECT_LIST", 0, MENU_IDM_MAPINFO_CONTEXT_MENU_1, sizeof( MENU_IDM_MAPINFO_CONTEXT_MENU_1 ) / sizeof( MENU_IDM_MAPINFO_CONTEXT_MENU_1[0] ) },
		{ "MI_MINIMAP", 0, MENU_IDM_MAPINFO_CONTEXT_MENU_2, sizeof( MENU_IDM_MAPINFO_CONTEXT_MENU_2 ) / sizeof( MENU_IDM_MAPINFO_CONTEXT_MENU_2[0] ) },
		{ "MI_TERRAIN_HEIGHT_V3_TILE_LIST", 0, MENU_IDM_MAPINFO_CONTEXT_MENU_3, sizeof( MENU_IDM_MAPINFO_CONTEXT_MENU_3 ) / sizeof( MENU_IDM_MAPINFO_CONTEXT_MENU_3[0] ) },
		{ "MI_MOVIES_EDITOR", 0, MENU_IDM_MAPINFO_CONTEXT_MENU_4, sizeof( MENU_IDM_MAPINFO_CONTEXT_MENU_4 ) / sizeof( MENU_IDM_MAPINFO_CONTEXT_MENU_4[0] ) },
	};

	const NResources::SMenuItem MENU_IDM_MODEL_CONTEXT_MENU_0[] =
	{
		{ "Reload &Editor", ID_MODEL_RELOAD_EDITOR, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Show &Terrain", ID_MODEL_DRAW_TERRAIN, nullptr, 0 },
		{ "Show &Animations", ID_MODEL_DRAW_ANIMATIONS, nullptr, 0 },
		{ "Show AI &Geometry", ID_MODEL_DRAW_AI_GEOMETRY, nullptr, 0 },
		{ "Set &Light...", ID_MODEL_SET_LIGHT, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Center &Camera", ID_MODEL_CENTER_CAMERA, nullptr, 0 },
		{ "&Save Camera", ID_MODEL_SAVE_CAMERA, nullptr, 0 },
		{ "&Reset Camera", ID_MODEL_RESET_CAMERA, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "Animation Speed &Down", ID_MODEL_SPEED_DOWN, nullptr, 0 },
		{ "Animation Speed &Up", ID_MODEL_SPEED_UP, nullptr, 0 },
		{ nullptr, 0, nullptr, 0 },
		{ "&Properties", 1076, nullptr, 0 },
	};

	const NResources::SMenuItem MENU_IDM_MODEL_CONTEXT_MENU[] =
	{
		{ "MODEL_STATE", 0, MENU_IDM_MODEL_CONTEXT_MENU_0, sizeof( MENU_IDM_MODEL_CONTEXT_MENU_0 ) / sizeof( MENU_IDM_MODEL_CONTEXT_MENU_0[0] ) },
	};

	const NResources::SMenuEntry MENUS[] =
	{
		{ IDM_MAIN, MENU_IDM_MAIN, sizeof( MENU_IDM_MAIN ) / sizeof( MENU_IDM_MAIN[0] ) },
		{ IDM_MAPINFO, MENU_IDM_MAPINFO, sizeof( MENU_IDM_MAPINFO ) / sizeof( MENU_IDM_MAPINFO[0] ) },
		{ IDM_MODEL, MENU_IDM_MODEL, sizeof( MENU_IDM_MODEL ) / sizeof( MENU_IDM_MODEL[0] ) },
		{ IDM_MAPINFO_CONTEXT_MENU, MENU_IDM_MAPINFO_CONTEXT_MENU, sizeof( MENU_IDM_MAPINFO_CONTEXT_MENU ) / sizeof( MENU_IDM_MAPINFO_CONTEXT_MENU[0] ) },
		{ IDM_MODEL_CONTEXT_MENU, MENU_IDM_MODEL_CONTEXT_MENU, sizeof( MENU_IDM_MODEL_CONTEXT_MENU ) / sizeof( MENU_IDM_MODEL_CONTEXT_MENU[0] ) },
	};

	const NResources::CMenuTable MENU_TABLE( MENUS, sizeof( MENUS ) / sizeof( MENUS[0] ) );

	const unsigned TOOLBAR_IDT_MAPINFO_TOOLS[] =
	{
		ID_TOOLS_RESET_CAMERA, ID_TOOLS_UPDATE_VSO, 0, ID_TOOLS_FIT_TO_GRID,
		ID_TOOLS_ROTATE_90, ID_TOOLS_DRAW_SHOOT_AREAS, ID_TOOLS_DRAW_AI_MAP, ID_TOOLS_DRAW_PASSABILITY,
		ID_TOOLS_SHOW_GRID, 0, ID_VIEW_FILTER,
	};

	const unsigned TOOLBAR_IDT_MAPINFO_VIEW[] =
	{
		ID_MI_VIEW_MINIMAP, ID_MI_VIEW_TOOL, ID_MI_VIEW_MOVIE_EDITOR,
	};

	const unsigned TOOLBAR_IDT_MODEL[] =
	{
		ID_MODEL_RELOAD_EDITOR, ID_MODEL_DRAW_TERRAIN, ID_MODEL_DRAW_ANIMATIONS, ID_MODEL_DRAW_AI_GEOMETRY,
		ID_MODEL_SET_LIGHT, ID_MODEL_CENTER_CAMERA, ID_MODEL_SAVE_CAMERA, ID_MODEL_RESET_CAMERA,
		ID_MODEL_SPEED_DOWN, ID_MODEL_SPEED_UP,
	};

	const NResources::SToolBarEntry TOOLBARS[] =
	{
		{ IDT_MAPINFO_TOOLS, 16, 16, TOOLBAR_IDT_MAPINFO_TOOLS, sizeof( TOOLBAR_IDT_MAPINFO_TOOLS ) / sizeof( TOOLBAR_IDT_MAPINFO_TOOLS[0] ) },
		{ IDT_MAPINFO_VIEW, 16, 16, TOOLBAR_IDT_MAPINFO_VIEW, sizeof( TOOLBAR_IDT_MAPINFO_VIEW ) / sizeof( TOOLBAR_IDT_MAPINFO_VIEW[0] ) },
		{ IDT_MODEL, 16, 16, TOOLBAR_IDT_MODEL, sizeof( TOOLBAR_IDT_MODEL ) / sizeof( TOOLBAR_IDT_MODEL[0] ) },
	};

	const NResources::CToolBarTable TOOLBAR_TABLE( TOOLBARS, sizeof( TOOLBARS ) / sizeof( TOOLBARS[0] ) );

}
