#pragma once

#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/ScintillaEditor.h"

#include <cstdint>

// The Scintilla text control the Log Window used to be, and still is behind
// ILogView.
//
// It no longer implements ICommandHandler. Copy, clear and select-all used to
// be handled here, which meant that giving the pane a different kind of
// contents would have meant writing them a second time. They are on CDWLog now,
// expressed against ILogView, and this reports focus to whoever the pane
// nominated instead of registering itself.
class CLogWindow : public CScintillaEditorWindow
{
	ICommandHandler *pSelectionHandler = nullptr;

protected:
	afx_msg void OnSetFocus( CWnd* pOldWnd );
	afx_msg void OnContextMenu( CWnd *pwnd, CPoint point );

public:
	void SetSelectionHandler( ICommandHandler *_pSelectionHandler )
	{
		pSelectionHandler = _pSelectionHandler;
	}

	DECLARE_MESSAGE_MAP()
};
