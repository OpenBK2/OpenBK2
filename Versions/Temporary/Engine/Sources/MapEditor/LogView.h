#pragma once

#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/Interface_Widget.h"

#include <string>

// The contents of the Log Window, as an interface rather than a control.
//
// CDWLog is the docking pane -- SECControlBar, MFC, part of the frame -- and it
// used to hold a CLogWindow by value and drive it in Scintilla's own terms:
// SCI_APPENDTEXT, SCI_STARTSTYLING, SCI_GETLENGTH and so on. That is precisely
// why the pane could not have any other kind of contents. Everything the pane
// actually needs is below, and none of it names a toolkit or a text control.
//
// This is the first panel-at-a-time slice of the wx migration, and the shape is
// meant to be the pattern for the rest: the pane stays MFC, the contents become
// swappable, and the two implementations can be run against each other in the
// same build.
struct ILogView
{
	virtual ~ILogView() {}

	// Build the contents inside the pane, which is passed as a widget because
	// that is all either implementation needs of it.
	//
	// pSelectionHandler is registered as the CHID_SELECTION command handler
	// whenever the contents take focus. It is passed in rather than implemented
	// here so that the copy/clear/select-all logic lives once, in the pane,
	// rather than once per view.
	virtual bool Create( IWidget *pParentPane, ICommandHandler *pSelectionHandler ) = 0;
	virtual bool IsCreated() const = 0;

	// Position within the pane, in the pane's client coordinates. The pane owns
	// its own geometry; this only moves what is inside it.
	virtual void SetBounds( const CTRect<int> &rBounds ) = 0;
	virtual void Show( bool bShow ) = 0;

	// Content. Append is styled by log type; what that means is the view's
	// business.
	virtual void Append( ELogOutputType eLogOutputType, const std::string &rszText ) = 0;
	virtual void Clear() = 0;
	// Paint what is pending. The log is written from code that is often mid
	// operation and not about to return to an idle loop.
	virtual void Redraw() = 0;

	// The three commands the pane and the context menu route here.
	virtual void Copy() = 0;
	virtual void SelectAll() = 0;
	virtual bool HasSelection() const = 0;
	virtual bool IsEmpty() const = 0;
};


namespace NLogView
{
	// Which implementation the pane gets. The wx one exists only in a build with
	// BUILD_WX_EDITOR and is selected by OBK2_WX_LOG in the environment, so the
	// two can be compared by restarting rather than rebuilding -- the same habit
	// the probe scripts are built around. Falls back to the Scintilla one if wx
	// is not compiled in or the variable is not set.
	ILogView* Create();

	// Named so the factory can reach them; not for anything else to call.
	ILogView* CreateScintillaLogView();
#ifdef OBK2_WITH_WX
	ILogView* CreateWxLogView();
#endif
}
