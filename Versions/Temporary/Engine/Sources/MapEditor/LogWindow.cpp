#include "stdafx.h"
#include "MapEditorLib/MfcWidget.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "Scintilla/Scintilla.h"
#include "ResourceDefines.h"
#include "LogWindow.h"

#include "MapEditorLib/Interface_MainFrame.h"

#include <cstdint>

BEGIN_MESSAGE_MAP(CLogWindow, CScintillaEditorWindow)
	ON_WM_SETFOCUS()
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()


void CLogWindow::OnSetFocus( CWnd* pOldWnd )
{
	CScintillaEditorWindow::OnSetFocus( pOldWnd );
	//
	// The pane is the selection handler now, not this control: see LogWindow.h.
	if ( pSelectionHandler != 0 )
	{
		Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, pSelectionHandler );
	}
}


void CLogWindow::OnContextMenu( CWnd *pwnd, CPoint point )
{
	SetFocus();
	CMenu mainPopupMenu;
	mainPopupMenu.LoadMenu( IDM_LOG_CONTEXT_MENU );
	CMenu *pMenu = mainPopupMenu.GetSubMenu( 0 );
	if ( pMenu )
	{
		pMenu->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x, point.y, MainFrameWnd(), 0 );
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
	}
	mainPopupMenu.DestroyMenu();
}

// basement storage
