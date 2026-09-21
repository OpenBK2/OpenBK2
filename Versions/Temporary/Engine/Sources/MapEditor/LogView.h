#pragma once

#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/Interface_Widget.h"

#include <string>

// The contents of the Log Window, as an interface rather than a control.
//
// In the MFC editor the pane was CDWLog, which held a CLogWindow by value and
// drove it in Scintilla's own terms: SCI_APPENDTEXT, SCI_STARTSTYLING,
// SCI_GETLENGTH and so on. That is precisely why the pane could not have any
// other kind of contents. Everything the pane actually needs is below, and
// none of it names a toolkit or a text control. The one implementation now is
// wx's (LogViewWx.cpp); CDWLog and CLogWindow are gone.
struct ILogView
{
	virtual ~ILogView() {}

	// Made by NLogView::CreateWxLogViewIn, below, with the pane's selection
	// handler, which is registered as CHID_SELECTION whenever the contents take
	// focus: the copy/clear/select-all logic lives once, in the pane. Create,
	// SetBounds and Show were for the MFC pane and went with it.

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
	// Native text colour for normal output, with green/red severity colours
	// adjusted for the current theme. Components are RGB, not Win32 COLORREF.
	struct SLogColour
	{
		unsigned char nRed;
		unsigned char nGreen;
		unsigned char nBlue;
	};

	SLogColour GetColour( ELogOutputType eLogOutputType );
}



class wxWindow;

namespace NLogView
{
	// The wx contents made straight inside a wx window, for a wx frame's pane
	// rather than inside an MFC one, and already created with
	// pSelectionHandler. *ppWindow is what to put in the pane's layout, which
	// then does what SetBounds does for an MFC pane. Owned by the caller; null
	// on failure.
	ILogView* CreateWxLogViewIn( wxWindow *pParent, ICommandHandler *pSelectionHandler, wxWindow **ppWindow );
}

