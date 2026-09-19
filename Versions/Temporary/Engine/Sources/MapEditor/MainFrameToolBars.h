#pragma once

#include <cstdint>

// The main frame's own toolbars: which bitmap resource each draws from, the
// id it is known by, its title, whether it starts shown, and its buttons.
//
// These were CMainFrame's static tables (MainFrame_Consts.cpp). They moved here
// when CMainFrame was taken out, because the wx frame builds the same six
// toolbars from them, and the ids and titles are what the saved layouts and
// the View > Toolbars commands know the toolbars by.
namespace NMainFrameToolBars
{
	const int TOOLBARS_COUNT = 6;

	// The toolbar bitmap resource each toolbar's buttons are cut from.
	extern const unsigned TOOLBAR_ID[TOOLBARS_COUNT];
	// The id each toolbar is known by: AFX_IDW_TOOLBAR + n, where n = 0, 4, 5,
	// ... (1, 2 and 3 were MFC's own control bars). View > Toolbars counts from
	// ID_VIEW_TOOLBAR_MAIN in this order.
	extern const unsigned TOOLBAR_CONTROL_ID[TOOLBARS_COUNT];
	// The string resource of each toolbar's title.
	extern const unsigned TOOLBAR_NAME_ID[TOOLBARS_COUNT];
	// The CBRS_ALIGN_ flags each toolbar was docked with.
	extern const uint32_t TOOLBAR_STYLE[TOOLBARS_COUNT];
	// Whether each toolbar starts shown.
	extern const bool TOOLBAR_SHOW[TOOLBARS_COUNT];
	// Each toolbar's buttons, as command ids with ID_SEPARATOR between groups,
	// and how many there are.
	extern const uint32_t TOOLBAR_ELEMENTS_COUNT[TOOLBARS_COUNT];
	extern const unsigned* const TOOLBAR_ELEMENTS_ID[TOOLBARS_COUNT];
}
