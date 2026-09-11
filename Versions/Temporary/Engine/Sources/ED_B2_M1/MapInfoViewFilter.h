#pragma once

#include "MapEditorLib/Interface_Widget.h"
#include "MapInfoEditorSettings.h"

// View -> Filter in the map editor: which object types the viewport shows, the
// grid and its size, and the render toggles, behind a boundary that names no
// toolkit. CMapInfoEditor::ConfigureViewFilter opens it from ID_VIEW_FILTER.
//
// It is a live dialog, and that is the contract both sides keep: every change
// is written into *pFilter as it is made and announced with Apply, so the map
// redraws under the dialog while it is open. Cancel -- the button, Escape or
// the close box -- puts *pFilter back as it was on entry and announces that
// too. So on false the filter is untouched, and on true it holds what the user
// left it at; either way the scene already agrees with it.
namespace NMapInfoViewFilter
{
	// Runs the dialog modally over pParent. True on OK.
	bool Run( IWidget *pParent, CMapInfoEditorSettings::SViewFilterData *pFilter );

	// What every change calls: the map editor's ID_VIEW_APPLY_MI_FILTER, which
	// applies the filter to the scene and redraws it. Here so both
	// implementations send the same command the same way.
	void Apply();

	// Named so the dispatcher can reach them; not for anything else to call.
	bool RunMfc( IWidget *pParent, CMapInfoEditorSettings::SViewFilterData *pFilter );
#ifdef OBK2_WITH_WX
	bool RunWx( IWidget *pParent, CMapInfoEditorSettings::SViewFilterData *pFilter );
#endif
}
