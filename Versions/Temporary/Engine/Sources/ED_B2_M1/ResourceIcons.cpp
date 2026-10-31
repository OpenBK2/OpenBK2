// The SVG icon each of this module's toolbar commands wears, by name in
// MapEditorLib/res/icons (see the README there); the map info and model
// editors' toolbars. Hand-written, as MapEditor/ResourceIcons.cpp.
//
// The four map overlays with no stock picture -- shoot areas, AI map,
// passability and the grid -- wear stand-ins until icons are drawn for them.

#include "stdafx.h"

#include "MapEditorLib/Resources.h"
#include "ResourceDefines.h"

namespace
{
const NResources::SIconEntry ICONS[] = {
	// Map info tools
	{ ID_TOOLS_RESET_CAMERA, "video" },
	{ ID_TOOLS_UPDATE_VSO, "spline" },
	{ ID_TOOLS_FIT_TO_GRID, "grid-2x2-check" },
	{ ID_TOOLS_ROTATE_90, "rotate-cw" },
	{ ID_TOOLS_DRAW_SHOOT_AREAS, "radius" },
	{ ID_TOOLS_DRAW_AI_MAP, "brain" },
	{ ID_TOOLS_DRAW_PASSABILITY, "footprints" },
	{ ID_TOOLS_SHOW_GRID, "grid-3x3" },
	{ ID_VIEW_FILTER, "funnel" },
	// Map info view
	{ ID_MI_VIEW_MINIMAP, "map" },
	{ ID_MI_VIEW_TOOL, "toolbox" },
	{ ID_MI_VIEW_MOVIE_EDITOR, "clapperboard" },
	// Model
	{ ID_MODEL_RELOAD_EDITOR, "refresh-ccw" },
	{ ID_MODEL_DRAW_TERRAIN, "land-plot" },
	{ ID_MODEL_DRAW_ANIMATIONS, "film" },
	{ ID_MODEL_DRAW_AI_GEOMETRY, "box" },
	{ ID_MODEL_SET_LIGHT, "sun" },
	{ ID_MODEL_CENTER_CAMERA, "focus" },
	{ ID_MODEL_SAVE_CAMERA, "camera-save" },
	{ ID_MODEL_RESET_CAMERA, "switch-camera" },
	{ ID_MODEL_SPEED_DOWN, "rewind" },
	{ ID_MODEL_SPEED_UP, "fast-forward" },
};

const NResources::CIconTable TABLE( ICONS, sizeof( ICONS ) / sizeof( ICONS[0] ) );
} // namespace
