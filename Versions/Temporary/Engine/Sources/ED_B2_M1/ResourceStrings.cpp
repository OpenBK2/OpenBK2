// Generated from ED_B2_M1.rc by scripts/port/rc2strings.py.
//
// The editor's string table, which was a Win32 STRINGTABLE until the PE
// resource section stopped being available on every platform the editor
// builds for. This file is the source now: the .rc block it came from is
// gone, and a new string is added here.
//
// The ids are the macros, not their numbers, so ResourceDefines.h stays the one
// place that says what a number means.

#include "stdafx.h"

#include "ResourceDefines.h"
#include "MapEditorLib/Resources.h"

namespace
{
	const NResources::SStringEntry ENTRIES[] =
	{
		{ ID_TOOLS_DRAW_SHOOT_AREAS,          "Toggle ( show or hide ) the Shoot Areas\nDraw Shoot Areas" },
		{ ID_TOOLS_DRAW_AI_MAP,               "Toggle ( show or hide ) the AI map\nDraw AI Map" },
		{ ID_TOOLS_DRAW_PASSABILITY,          "Toggle ( show or hide ) the Passability\nDraw Passability" },
		{ ID_TOOLS_SHOW_GRID,                 "Show grid\nShow Grid" },
		{ ID_VIEW_FILTER,                     "Update view filter\nView Filter" },
		{ ID_TOOLS_REGEN_VSO_NORMALS,         "Regenerate normals of terrain objects\nRegenerate normals of terrain objects" },
		{ ID_TOOLS_DEBUG_CHECK_MAP,           "Debug check map\nDebug Check Map" },
		{ ID_TOOLS_CREATE_SCRIPT_PATH_POINTS, "Copy a Lua path: left click adds a point, right click removes the last point, Escape finishes\nCreate Script Path Points" },
		{ ID_TOOLS_REGEN_GEOMETRY,            "Regenerate terrain geometry to fix any bad terrain junctions\nRegenerate Terrain" },
		{ ID_TOOLS_CREATE_INF_ANIMS,          "Create infanrty animations\nCreate Infanrty Animations" },
		{ ID_TOOLS_CREATE_ACK_SETS,           "Create ack sets\nCreate Ack Sets" },
		{ ID_TOOLS_CREATE_VIS_OBJ,            "Create VisObj\nCreate VisObj" },
		{ ID_TOOLS_RESET_CAMERA,              "Reset Camera\nReset Camera" },
		{ ID_TOOLS_UPDATE_VSO,                "Update roads, cliffs and rivers\nUpdate Roads, Cliffs, Rivers" },
		{ ID_TOOLS_FIT_TO_GRID,               "Fit to AI grid\nFit To AI Grid" },
		{ ID_TOOLS_ROTATE_90,                 "Rotate Multiple To 90 &Degree\nRotate Multiple To 90 &Degree" },
		{ ID_MI_VIEW_MINIMAP,                 "Toggle ( show or hide ) the Minimap window\nMinimap Window" },
		{ ID_MI_VIEW_TOOL,                    "Toggle ( show or hide ) the Tools window\nTools Window" },
		{ ID_MI_VIEW_VIEW_TOOLBAR,            "Toggle ( show or hide ) the Additional View toolbar\nAdditional View Toolbar" },
		{ ID_MI_VIEW_TOOLS_TOOLBAR,           "Toggle ( show or hide ) the Tools toolbar\nTools Toolbar" },
		{ ID_MODEL_VIEW_TOOLBAR,              "Toggle ( show or hide ) the Model toolbar\nModel Toolbar" },
		{ ID_MODEL_VIEW_TOOL,                 "Toggle ( show or hide ) the Model tool window\nModel Tool Window" },
		{ ID_MODEL_RELOAD_EDITOR,             "Reload all cached resoures\nReload Model Editor" },
		{ ID_MODEL_DRAW_TERRAIN,              "Toggle ( show or hide ) the Model terrain\nShow Model Terrain" },
		{ ID_MODEL_DRAW_ANIMATIONS,           "Toggle ( show or hide ) the Model animations\nShow Model Animations" },
		{ ID_MODEL_DRAW_AI_GEOMETRY,          "Toggle ( show or hide ) the AI Geometry\nShow AI Geometry" },
		{ ID_MODEL_SET_LIGHT,                 "Choose appropriate light for model scene\nSet Model Light" },
		{ ID_MODEL_CENTER_CAMERA,             "Center camera on model\nCenter Camera" },
		{ ID_MODEL_SAVE_CAMERA,               "Save current camera position\nSave Camera" },
		{ ID_MODEL_RESET_CAMERA,              "Restore saved camera position or restore default\nReset Camera" },
		{ ID_MODEL_SPEED_DOWN,                "Low the animation speed\nAnimation Speed Down" },
		{ ID_MODEL_SPEED_UP,                  "Up the animation speed\nAnimation Speed Up" },
		{ IDS_OBJECT_ISS_MAP_OBJECT_LABEL,    "Objects" },
		{ IDS_OBJECT_ISS_VSO_LABEL,           "Terrain Objects" },
		{ IDS_GP_ISS_REINF_POINTS_LABEL,      "Reinforcement Points" },
		{ IDS_GP_ISS_START_CAMERA_LABEL,      "Start Cameras" },
		{ IDS_GP_ISS_AIGENERAL_LABEL,         "AI General" },
		{ IDS_GP_ISS_UNIT_START_CMD_LABEL,    "Unit Start Commands" },
		{ IDS_SCRIPT_ISS_SCRIPT_AREAS_LABEL,  "Script Areas" },
		{ IDS_SCRIPT_ISS_SCRIPT_MOVIES_LABEL, "Script Movies" },
		{ IDS_SMOKE_POINTS,                   "Smoke Points" },
		{ IDS_ENTRANCE_POINTS,                "Entrance Points" },
		{ IDS_FIRE_POINTS,                    "Fire Points" },
		{ IDS_BUILDING_POINTS,                "Building Points" },
		{ IDS_SURFACE_POINTS,                 "Surface Points" },
		{ IDS_DAMAGE_LEVELS,                  "Damage Levels" },
		{ IDS_MIMO_DELETE_OBJECTS_MESSAGE,    "Are you sure you want to delete the selected objects?" },
		{ IDS_MIMO_DELETE_OBJECT_MESSAGE,     "Are you sure you want to delete the selected object?" },
		{ IDS_IS_TERRAIN_LABEL,               "Terrain" },
		{ IDS_IS_OBJECT_LABEL,                "Objects" },
		{ IDS_IS_GAMEPLAY_LABEL,              "Gameplay" },
		{ IDS_IS_SCRIPT_LABEL,                "Script" },
		{ IDS_TERRAIN_ISS_TILE_LABEL,         "Tiles" },
		{ IDS_TERRAIN_ISS_HEIGHT_LABEL,       "Heights" },
		{ IDS_TERRAIN_ISS_HEIGHT_V2_LABEL,    "Heights V2" },
		{ IDS_TERRAIN_ISS_HEIGHT_V3_LABEL,    "Terrain" },
		{ IDS_TERRAIN_ISS_FIELD_LABEL,        "Fields" },
		{ IDS_TOOLBAR_MAPINFO_TOOLS,          "MapInfo Tools" },
		{ IDS_TOOLBAR_MAPINFO_VIEW,           "MapInfo View" },
		{ IDS_TOOLBAR_MODEL,                  "Model" },
		{ IDS_MODEL_SAVE_CAMERA_MESSAGE,      "Do you really want save this camera position?" },
		{ IDC_TMITHV3_BRUSH_TILE,             "Fill Terrain" },
		{ IDC_TMITHV3_BRUSH_UP,               "Up Terrain Height" },
		{ IDC_TMITHV3_BRUSH_DOWN,             "Low Terrain Height" },
		{ IDC_TMITHV3_BRUSH_ROUND,            "Round Terrain Height" },
		{ IDC_TMITHV3_BRUSH_PLATO,            "Make Plato" },
		{ IDC_TMITHV3_BRUSH_SIZE_0,           "Very Small Brush" },
		{ IDC_TMITHV3_BRUSH_SIZE_1,           "Small Brush" },
		{ IDC_TMITHV3_BRUSH_SIZE_2,           "Medium Brush" },
		{ IDC_TMITHV3_BRUSH_SIZE_3,           "Large Brush" },
		{ IDC_TMITHV3_BRUSH_SIZE_4,           "Very Large Brush" },
		{ IDC_TMITHV3_BRUSH_TYPE_CIRCLE,      "Circle Brush" },
		{ IDC_TMITHV3_BRUSH_TYPE_SQUARE,      "Square Brush" },
		{ IDC_TMITHV3_UPDATE_HEIGHTS,         "Fix Cliffs" },
		{ IDS_MOV_EDITOR_ISS_EDITOR_LABEL,    "Movie Editor" },
		{ IDS_STATUS_STRING_OBJECT,           "ScriptID: %d, Stats: %s" },
		{ IDS_STATUS_STRING_OBJECTS,          "%d objects" },
		{ IDS_PM_PLACE_FIELD,                 "Apply field: %s..." },
		{ IDC_DMOVED_JUMP_FIRST_KEY_BUTTON,   "Jump to the start of the movie" },
		{ IDC_DMOVED_JUMP_LAST_KEY_BUTTON,    "Jump to the end of the movie" },
		{ IDC_DMOVED_STEP_PREV_KEY_BUTTON,    "Step to the previous key" },
		{ IDC_DMOVED_STEP_NEXT_KEY_BUTTON,    "Step to the next key" },
		{ IDC_DMOVED_STOP_MOVIE_BUTTON,       "Stop movie" },
		{ IDC_DMOVED_PLAY_PAUSE_MOVIE_BUTTON, "Play or pause movie" },
	};

	// Registered before main runs, like the resource section was mapped
	// before it. Order between modules is load order, which is the order
	// the executable's resources used to be searched in.
	const NResources::CStringTable TABLE( ENTRIES, sizeof( ENTRIES ) / sizeof( ENTRIES[0] ) );
}
