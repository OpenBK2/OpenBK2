#include "StdAfx.h"
#include "wmdefines.h"
#include "tree_gdbbrowserbase_constants.h"
#include "ResourceDefines.h"
//#include "afxdlgs.h"

#include "Tree_GDBLinkBrowser.h"
#include "..\MapEditorLib\Interface_UserData.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


const char CTreeGDBLinkBrowser::TABGDBB_TREE_HEAD_WIDTH_UPDATE_TYPE[] = "__CTGDBLB__";


void CTreeGDBLinkBrowser::Load()
{
	if ( CWnd *pwndParent = GetParent() )
	{
		pwndParent->SendMessage( WM_TREE_GDB_BROWSER, MAKEWPARAM( TREE_GDB_BROWSER_LOAD, 0 ), reinterpret_cast<LPARAM>( this ) );
	}
}


bool CTreeGDBLinkBrowser::CanLoad()
{
	if ( GetSelectedCount() == 1 )
	{
		if ( HTREEITEM hFocusedItem = GetSelectedItem() )
		{
			return ( GetTreeItemType( hFocusedItem ) == GDBO_OBJECT );
		}
	}
	return false;
}


bool CTreeGDBLinkBrowser::GetLoadContextMenuLabel( string *pszLabel )
{
	if ( pszLabel )
	{
		CString strLabel;
		strLabel.LoadString( IDS_TREE_GDB_LINK_BROWSE_SELECT );
		( *pszLabel ) = strLabel;
	}
	return true;
}

bool CTreeGDBLinkBrowser::GetSaveHeaderWidthLabel( string *pszLabel )
{
	if ( pszLabel )
	{
		( *pszLabel ) = TABGDBB_TREE_HEAD_WIDTH_UPDATE_TYPE;
	}
	return true;
}


void CTreeGDBLinkBrowser::LoadHeaderWidth()
{
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	pUserData->tableLinkHeaderWidthList.resize( TABGDBB_TREE_COLUMN_COUNT, 0 );
	//
	for ( int index = 0; index < TABGDBB_TREE_COLUMN_COUNT; ++index )
	{
		SetColumnWidth( index, ( pUserData->tableLinkHeaderWidthList[index] > 0 ) ? pUserData->tableLinkHeaderWidthList[index] : TABGDBB_TREE_COLUMN_WIDTH[index] );
	}
	RecalcScrollBars();
	RedrawWindow();
}


void CTreeGDBLinkBrowser::SaveHeaderWidth()
{
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	pUserData->tableLinkHeaderWidthList.resize( TABGDBB_TREE_COLUMN_COUNT, 0 );
	//
	for ( int index = 0; index < TABGDBB_TREE_COLUMN_COUNT; ++index )
	{
		pUserData->tableLinkHeaderWidthList[index] = GetColumnWidth( index );
	}
}

// basement storage


