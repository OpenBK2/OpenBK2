#include "stdafx.h"

#include "DefaultTabWindow.h"
#include "DefaultShortcutBar.h"

#include <cstdint>

BEGIN_MESSAGE_MAP( CDefaultShortcutBar, SECShortcutBar )
	ON_MESSAGE( TCM_TABSEL, OnNotifyChangeTab )	
END_MESSAGE_MAP()


CDefaultShortcutBar::~CDefaultShortcutBar()
{
	for ( int nShortcutIndex = 0; nShortcutIndex < shortcutList.size(); ++nShortcutIndex )
	{
		if ( shortcutList[nShortcutIndex] )
		{
			delete ( shortcutList[nShortcutIndex] );
			shortcutList[nShortcutIndex] = 0;
		}
	}
}


void CDefaultShortcutBar::RemoveAllShortcuts()
{
	const int nShortcutCount = GetBarCount();
	for ( int nShortcutIndex = 0; nShortcutIndex < nShortcutCount; ++nShortcutIndex )
	{
		 RemoveBar( 0 );
	}
	for ( int nShortcutIndex = 0; nShortcutIndex < shortcutList.size(); ++nShortcutIndex )
	{
		if ( shortcutList[nShortcutIndex] )
		{
			shortcutList[nShortcutIndex]->DestroyWindow();
			delete ( shortcutList[nShortcutIndex] );
			shortcutList[nShortcutIndex] = 0;
		}
	}
}


BOOL CDefaultShortcutBar::OnChangeBar( int nShortcutIndex )
{
	if ( ( nCommandHandlerID != INVALID_COMMAND_HANDLER_ID ) &&
			 ( nCommandID != INVALID_COMMAND_ID ) )
	{
		int nTabIndex = INVALID_TAB_INDEX;
		if ( ( nShortcutIndex < 0 ) || ( nShortcutIndex > INVALID_SHORTCUT_INDEX ) )
		{
			nShortcutIndex = INVALID_SHORTCUT_INDEX;
		}
		uint32_t dwParam = MAKELONG( nTabIndex, nShortcutIndex );
		DebugTrace( "CDefaultShortcutBar::OnChangeBar(), dwParam: %d(0x%X)", dwParam, dwParam );
		bool bEnable = false;
		bool bChecked = false;
		if ( Singleton<ICommandHandlerContainer>()->UpdateCommand( nCommandHandlerID, nCommandID, &bEnable, &bChecked ) && bEnable )
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( nCommandHandlerID, nCommandID, dwParam );
		}
	}
	return true;
}


LRESULT CDefaultShortcutBar::OnNotifyChangeTab( WPARAM wParam, LPARAM lParam )
{
	if ( ( nCommandHandlerID != INVALID_COMMAND_HANDLER_ID ) &&
			 ( nCommandID != INVALID_COMMAND_ID ) )
	{
		int nShortcutIndex = GetActiveIndex();
		int nTabIndex = wParam;
		if ( ( nShortcutIndex < 0 ) || ( nShortcutIndex > INVALID_SHORTCUT_INDEX ) )
		{
			nShortcutIndex = INVALID_SHORTCUT_INDEX;
		}
		if ( ( nTabIndex < 0 ) || ( nTabIndex > INVALID_TAB_INDEX ) )
		{
			nTabIndex = INVALID_TAB_INDEX;
		}
		uint32_t dwParam = MAKELONG( nTabIndex, nShortcutIndex );
		//DebugTrace( "CDefaultShortcutBar::OnNotifyChangeTab(), dwParam: %d(0x%X)", dwParam, dwParam );
		bool bEnable = false;
		bool bChecked = false;
		if ( Singleton<ICommandHandlerContainer>()->UpdateCommand( nCommandHandlerID, nCommandID, &bEnable, &bChecked ) && bEnable )
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( nCommandHandlerID, nCommandID, dwParam );
		}
	}
	return 0;
}


CWnd* CDefaultShortcutBar::GetShortcutWindow( int nShortcutIndex )
{
	NI_ASSERT( ( nShortcutIndex >=0 ) &&
						 ( nShortcutIndex < shortcutList.size() ),
						 StrFmt( "CDefaultShortcutBar::GetShortcutWindow(): invalid nShortcutIndex: %d [0...%d)", nShortcutIndex, shortcutList.size() ) );
	return shortcutList[nShortcutIndex]; 
}

// basement storage  


