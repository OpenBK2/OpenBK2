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
	// The log's three colours, as components rather than a packed literal.
	//
	// They lived as 0x000000, 0x227722 and 0x3333ff passed straight to
	// SCI_STYLESETFORE, and the last of those is the reason this struct exists:
	// Scintilla packs colours 0x00BBGGRR, like a Win32 COLORREF and the opposite
	// way round from an HTML #RRGGBB, so 0x3333ff is **red**, not blue. Scintilla
	// says so itself in Platform.h -- GetRed() is `co & 0xff`. Anyone reading
	// those literals as HTML got two of the three wrong.
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

