#pragma once

#include "MapEditorLib/Interface_Widget.h"

// The model editor's palette -- light, background, FOV, terrain, animation
// preview and AI geometry -- behind a boundary that names no toolkit.
//
// **A different shape from the map info palettes.** Those are tabs, and a tab
// control owns its tabs and deletes them through a CWnd*, so their factories
// hand back a borrowed pointer. This one is the contents of a docking pane, as
// the minimap is, and has the minimap's shape: CModelEditor makes the view,
// has it made in its pane, hands the pane its widget, destroys it in
// DestroyControls as it always has, and deletes it after.
//
// Its data type is CModelState::SEditParameters and its command dispatch is
// CModelCommands, both in ModelState.h.
namespace NModelView
{
	class IView
	{
	public:
		virtual ~IView() {}

		// Makes the palette in pPane, the model editor's docking pane. False if
		// it could not be made.
		virtual bool Create( IWidget *pPane ) = 0;
		// Takes the palette's window down. Safe when it was never made.
		virtual void Destroy() = 0;
		virtual void Show( bool bShow ) = 0;
		// What the pane is given as its contents. Null before Create.
		virtual IWidget* GetWidget() = 0;
	};

	// The view for this session. Owned by the caller.
	IView* Create();
}
