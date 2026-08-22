#pragma once

#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/ScintillaEditor.h"

#include <cstdint>

class CLogWindow : public CScintillaEditorWindow, public ICommandHandler
{
protected:
	afx_msg void OnSetFocus( CWnd* pOldWnd );
	afx_msg void OnContextMenu( CWnd *pwnd, CPoint point );
public:

	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uint32_t dwData );
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );

	DECLARE_MESSAGE_MAP()
};


