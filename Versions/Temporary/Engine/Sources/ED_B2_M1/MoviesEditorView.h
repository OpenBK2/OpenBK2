#pragma once

#include "MapEditorLib/Interface_Widget.h"

// The script movies editor, behind a boundary that names no toolkit.
//
// A docking pane along the bottom of the frame, holding the movie timeline and
// the transport: play, the key jumps, the movie and speed lists, the length
// settings button. CMapInfoEditor::CreateControls makes the pane and puts this
// inside it.
//
// The boundary is small, and that is not because the window is small -- it is
// the largest thing migrated so far. It is because everything the editor says
// to this window it already says through ICommandHandler: CHID_MOVIES_EDITOR_WINDOW
// carries ID_WINDOW_SET_DIALOG_DATA and ID_WINDOW_GET_DIALOG_DATA with an
// SScriptMovieEditorData, the timer commands, and the four key commands from
// the context menu. That channel names no toolkit and did not have to change.
// What is left is the part a pane needs of its contents: make it, show it,
// hand it over, take it down.
namespace NMoviesEditorView
{
	class IView
	{
	public:
		virtual ~IView() {}

		// Builds the window inside pPane, and registers it as the
		// CHID_MOVIES_EDITOR_WINDOW command handler, which is how everything
		// else reaches it.
		virtual bool Create( IWidget *pPane ) = 0;
		// Unregisters and destroys the window. The pane is not touched.
		virtual void Destroy() = 0;
		virtual void Show( bool bShow ) = 0;

		// What the pane is given as its contents. Null before Create.
		virtual IWidget* GetWidget() = 0;
	};


	// The pane's view. The caller owns the result and destroys it with delete.
	IView* Create();
}
