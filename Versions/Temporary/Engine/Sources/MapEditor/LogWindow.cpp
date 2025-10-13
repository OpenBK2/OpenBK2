#include "stdafx.h"
#include "mapeditorlib/resourcedefines.h"
#include "mapeditorlib/commandhandlerdefines.h"
#include "scintilla/scintilla.h"
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
	Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, this );
}


void CLogWindow::OnContextMenu( CWnd *pwnd, CPoint point )
{
	SetFocus();
	CMenu mainPopupMenu;
	mainPopupMenu.LoadMenu( IDM_LOG_CONTEXT_MENU );
	CMenu *pMenu = mainPopupMenu.GetSubMenu( 0 );
	if ( pMenu )
	{
		pMenu->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x, point.y, Singleton<IMainFrameContainer>()->GetSECWorkbook(), 0 );
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
	}
	mainPopupMenu.DestroyMenu();
}


bool CLogWindow::HandleCommand( UINT nCommandID, uint32_t dwData )
{
	switch( nCommandID )
	{
		case ID_SELECTION_COPY:
			Command( SCI_COPY );
			return true;
		case ID_SELECTION_CLEAR:
			Singleton<ICommandHandlerContainer>()->HandleCommand( ID_LOG_CLEAR_ALL, 0 );
			return true;
		case ID_SELECTION_SELECT_ALL:
			Command( SCI_SELECTALL );
			return true;
		default:
			return false;
	}
}


bool CLogWindow::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CLogWindow::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CLogWindow::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID )
	{
		case ID_SELECTION_COPY:
			( *pbEnable ) = ( Command( SCI_GETSELECTIONSTART ) != Command( SCI_GETSELECTIONEND ) );
			( *pbCheck ) = false;
			return true;
		case ID_SELECTION_CLEAR:
			( *pbEnable ) = ( Command( SCI_GETLENGTH ) != 0 );
			( *pbCheck ) = false;
			return true;
		case ID_SELECTION_SELECT_ALL:
			( *pbEnable ) = ( Command( SCI_GETLENGTH ) != 0 );
			( *pbCheck ) = false;
			return true;
		default:
			return false;
	}
}

// basement storage  


