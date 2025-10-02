#include "stdafx.h"
#include "DefaultTabWindow.h"
#include "DefaultShortcutBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP( CDefault3DTabWindow, SEC3DTabWnd )
END_MESSAGE_MAP()


CDefault3DTabWindow::~CDefault3DTabWindow()
{
	for ( int nTabIndex = 0; nTabIndex < tabList.size(); ++nTabIndex )
	{
		if ( tabList[nTabIndex] )
		{
			delete ( tabList[nTabIndex] );
			tabList[nTabIndex] = 0;
		}
	}
}


void CDefault3DTabWindow::RemoveAllTabs()
{
	const int nTabCount = GetTabCount();
	for ( int nTabIndex = 0; nTabIndex < nTabCount; ++nTabIndex )
	{
		 RemoveTab( 0 );
	}
	for ( int nTabIndex = 0; nTabIndex < tabList.size(); ++nTabIndex )
	{
		if ( tabList[nTabIndex] )
		{
			tabList[nTabIndex]->DestroyWindow();
			delete ( tabList[nTabIndex] );
			tabList[nTabIndex] = 0;
		}
	}
}


CWnd* CDefault3DTabWindow::GetTabWindow( int nTabIndex )
{
	NI_ASSERT( ( nTabIndex >=0 ) &&
						 ( nTabIndex < tabList.size() ),
						 StrFmt( "CDefault3DTabWindow::GetTabWindow(): invalid nTabIndex: %d [0...%d)", nTabIndex, tabList.size() ) );
	return tabList[nTabIndex]; 
}

// Необходимо делать так, иначе родительскому окну не идут сообщения

LRESULT CDefault3DTabWindow::WindowProc( UINT message, WPARAM wParam, LPARAM lParam ) 
{
	if ( message == TCM_TABSEL )
	{
		OnNotifyChangeTab( wParam, lParam );
	}
	return SEC3DTabWnd::WindowProc( message, wParam, lParam );
}


void CDefault3DTabWindow::OnNotifyChangeTab( WPARAM wParam, LPARAM lParam )
{
	if ( ( nCommandHandlerID != INVALID_COMMAND_HANDLER_ID ) &&
			 ( nCommandID != INVALID_COMMAND_ID ) )
	{
		int nShortcutIndex = INVALID_SHORTCUT_INDEX;
		int nTabIndex = wParam;
		if ( ( nTabIndex < 0 ) || ( nTabIndex > INVALID_TAB_INDEX ) )
		{
			nTabIndex = INVALID_TAB_INDEX;
		}
		DWORD dwParam = MAKELONG( nTabIndex, nShortcutIndex );
		//DebugTrace( "CDefault3DTabWindow::OnNotifyChangeTab(), dwParam: %d(0x%X)", dwParam, dwParam );
		bool bEnable = false;
		bool bChecked = false;
		if ( Singleton<ICommandHandlerContainer>()->UpdateCommand( nCommandHandlerID, nCommandID, &bEnable, &bChecked ) && bEnable )
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( nCommandHandlerID, nCommandID, dwParam );
		}
	}
}

// basement storage  


