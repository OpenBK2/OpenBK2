#include "stdafx.h"
#include <fmt/format.h>
#include <fmt/printf.h>
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "WMDefines.h"
#include "Tree_GDBBrowserBase_Constants.h"

#include "Tree_GDBBrowserBase.h"
#include "MapEditorLib/Interface_Exporter.h"
#include "MapEditorLib/Interface_Editor.h"
#include "MapEditorLib/Interface_Logger.h"
#include "MapEditorLib/Tools_HashSet.h"
#include "MapEditorLib/StringManager.h"
//#include "../MapEditorLib/Tools_SysCodes.h"
#include "libdb/ResourceManager.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/MultiManipulator.h"
#include "MapEditorLib/ControlAlgorithms.h"
#include "SearchObjectDialog.h"
#include "NewObjectDialog.h"
#include "RefListDialog.h"
#include "RefListWaitDialog.h"
#include "Misc/StrProc.h"
#include "System/FilePath.h"
#include "libdb/Db.h"

#include <cstdint>

/**
const	unsigned  CTreeGDBBrowserBase::TABGDBB_TREE_COLUMN_NAME  [TABGDBB_TREE_COLUMN_COUNT] = { IDS_TABGDBB_PROPERTY_THN_0, IDS_TABGDBB_PROPERTY_THN_1 };
const int   CTreeGDBBrowserBase::TABGDBB_TREE_COLUMN_FORMAT[TABGDBB_TREE_COLUMN_COUNT] = { LVCFMT_LEFT, LVCFMT_LEFT };
const int		CTreeGDBBrowserBase::TABGDBB_TREE_COLUMN_WIDTH [TABGDBB_TREE_COLUMN_COUNT] = { 100, 50 };
/**/
const	unsigned  CTreeGDBBrowserBase::TABGDBB_TREE_COLUMN_NAME  [TABGDBB_TREE_COLUMN_COUNT] = { IDS_TABGDBB_PROPERTY_THN_0 };
const int   CTreeGDBBrowserBase::TABGDBB_TREE_COLUMN_FORMAT[TABGDBB_TREE_COLUMN_COUNT] = { LVCFMT_LEFT };
const int		CTreeGDBBrowserBase::TABGDBB_TREE_COLUMN_WIDTH [TABGDBB_TREE_COLUMN_COUNT] = { 150 };


int CALLBACK TreeGDBBrowserBaseCompareFunc( LPARAM lParam0, LPARAM lParam1, LPARAM lParamSort )
{
	HTREEITEM hItem0 = reinterpret_cast<HTREEITEM>( lParam0 );
	HTREEITEM hItem1 = reinterpret_cast<HTREEITEM>( lParam1 );
	CTreeGDBBrowserBase *pTreeGDBBrowserBase = reinterpret_cast<CTreeGDBBrowserBase*>( lParamSort );
	//
	CTreeGDBBrowserBase::EGDBOType nType0 = pTreeGDBBrowserBase->GetTreeItemType( hItem0 );
	CTreeGDBBrowserBase::EGDBOType nType1 = pTreeGDBBrowserBase->GetTreeItemType( hItem1 );
	CString strText0 = pTreeGDBBrowserBase->GetItemText( hItem0 );
	CString strText1 = pTreeGDBBrowserBase->GetItemText( hItem1 );
	//
	return pTreeGDBBrowserBase->SortItemText( strText0, nType0, strText1, nType1 );
}


int CTreeGDBBrowserBase::SortItemText( const CString &rstrText0, EGDBOType nType0, const CString &rstrText1, EGDBOType nType1 )
{
	if ( nType0 != nType1 )
	{
		if ( nType0 == CTreeGDBBrowserBase::GDBO_UNKNOWN )
		{
			return -1;
		}
		if ( nType1 == CTreeGDBBrowserBase::GDBO_UNKNOWN ) 
		{
			return 1;
		}
		if ( ( nType0 != CTreeGDBBrowserBase::GDBO_FOLDER ) &&
				 ( nType1 != CTreeGDBBrowserBase::GDBO_FOLDER ) )
		{
			if ( IsIgnoreCase() )
			{
				std::string szText0 = rstrText0;
				std::string szText1 = rstrText1;
				NStr::ToLower( &szText0 );
				NStr::ToLower( &szText1 );
				return strcmpi( szText0.c_str(), szText1.c_str() );
			}
			else
			{
				return strcmpi( rstrText0, rstrText1 );
			}
		}
		if ( nType0 != CTreeGDBBrowserBase::GDBO_FOLDER )
		{
			return 1;
		}
		return -1;
	}
	else
	{
		return strcmpi( rstrText0, rstrText1 );
	}
}


CTreeGDBBrowserBase::CGDBOMnemonics::CGDBOMnemonics() : CMnemonicsCollector<int>( CTreeGDBBrowserBase::GDBO_UNKNOWN, "" )
{
	Insert( GDBO_OBJECT, "object" );
	Insert( GDBO_FOLDER, "folder" );
}


CTreeGDBBrowserBase::EGDBOType CTreeGDBBrowserBase::CGDBOMnemonics::Get( const std::string &rszGDBOMnemonic )
{
	return static_cast<EGDBOType>( GetValue( rszGDBOMnemonic ) );
}


BEGIN_MESSAGE_MAP(CTreeGDBBrowserBase, CSortTreeControl)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(TVN_BEGINLABELEDIT, OnBeginLabelEdit)
	ON_NOTIFY_REFLECT(TVN_ENDLABELEDIT, OnEndLabelEdit)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelChanged)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, OnItemExpanded)
	ON_NOTIFY_REFLECT(LVN_BEGINLABELEDIT, OnBeginLabelEditList)
	ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, OnEndLabelEditList)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()


CTreeGDBBrowserBase::CTreeGDBBrowserBase( bool _bNeedTranslateAccelerators, bool _bModal, int _nGDBBrowserID )
	: bNeedTranslateAccelerators( _bNeedTranslateAccelerators ),
		bModal ( _bModal ),
		nPCDialogCommandHandlerID( INVALID_COMMAND_HANDLER_ID ),
		nGDBBrowserID( _nGDBBrowserID ),
		bCreateControls( false ),
		bStrongSelection( false ),
		hLabelEditItem( 0 ),
		nLabelEditSortTimer( 0 ),
		nCreateTreeTimer( 0 ),
		bCreateTreeSelectionChanged( false ),
		hLabelEditSortTimerItem( 0 )
{
}


CTreeGDBBrowserBase::~CTreeGDBBrowserBase()
{
}


LRESULT CTreeGDBBrowserBase::WindowProc( unsigned message, WPARAM wParam, LPARAM lParam )
{
	if ( IsNotEditLabel() )
	{
		if ( bNeedTranslateAccelerators && ( message == WM_KEYDOWN ) && ( ::AfxGetMainWnd() != 0 ) )
		{
			if ( NCA::TranslateAccelerators( bModal, message, wParam, lParam ) )
			{
				return 0;
			}
		}
	}
	return CSortTreeControl::WindowProc( message, wParam, lParam );
}


int CTreeGDBBrowserBase::OnCreate( LPCREATESTRUCT pCreateStruct )
{
	if ( CSortTreeControl::OnCreate( pCreateStruct ) == -1 )
	{
		return -1;
	}
	SObjectSet objectSet;
	GetSaveHeaderWidthLabel( &( objectSet.szObjectTypeName ) );
	InsertHashSetElement( &( objectSet.objectNameSet ), VIEW_COLLECTION_ID );
	Singleton<IViewContainer>()->Add( this, objectSet );
	//
	dragAndDropState.SetTargetWindow( this );
	dragAndDropState.Enter();
	szItemTextFromBeginLabelEdit.clear();
	CreateTree();
	//
	return 0;
}


void CTreeGDBBrowserBase::OnDestroy() 
{
	Singleton<ICommandHandlerContainer>()->Remove( CHID_OBJECT, this );
	Singleton<ICommandHandlerContainer>()->Remove( CHID_SELECTION, this );
	//
	SObjectSet objectSet;
	GetSaveHeaderWidthLabel( &( objectSet.szObjectTypeName ) );
	InsertHashSetElement( &( objectSet.objectNameSet ), VIEW_COLLECTION_ID );
	Singleton<IViewContainer>()->Remove( this, objectSet );
	//
	KillLabelEditSortTimer();
	KillCreateTreeTimer();
	RemoveViewManipulator();
	CSortTreeControl::OnDestroy();
}


void CTreeGDBBrowserBase::OnTimer( UINT_PTR nIDEvent ) 
{
  if ( nIDEvent == GetLabelEditSortTimerID() )
	{
		OnLabelEditSortTimer();
	}
	else if ( nIDEvent == GetCreateTreeTimerID() )
	{
		OnCreateTreeTimer();
	}
	CSortTreeControl::OnTimer( nIDEvent );
}


void CTreeGDBBrowserBase::SetLabelEditSortTimer()
{
  KillLabelEditSortTimer();
  nLabelEditSortTimer = SetTimer( GetLabelEditSortTimerID(), GetLabelEditSortTimerInterval(), 0 );
  if ( nLabelEditSortTimer == 0 )
  {
    NI_ASSERT( 0, "CTreeGDBBrowserBase::SetLabelEditSortTimer() Can't create timer." );
  }
}


void CTreeGDBBrowserBase::KillLabelEditSortTimer()
{
  if ( nLabelEditSortTimer != 0 )
	{
		KillTimer( nLabelEditSortTimer );
	}
  nLabelEditSortTimer = 0;
}


void CTreeGDBBrowserBase::OnLabelEditSortTimer()
{
	KillLabelEditSortTimer();
	bCreateControls = true;
	SortTree( hLabelEditSortTimerItem, TreeGDBBrowserBaseCompareFunc, reinterpret_cast<LPARAM>( this ) );
	RedrawWindow();
	bCreateControls = false;
	hLabelEditSortTimerItem = 0;
}


void CTreeGDBBrowserBase::SetCreateTreeTimer()
{
  KillCreateTreeTimer();
  nCreateTreeTimer = SetTimer( GetCreateTreeTimerID(), GetCreateTreeTimerInterval(), 0 );
  if ( nCreateTreeTimer == 0 )
  {
    NI_ASSERT( 0, "CTreeGDBBrowserBase::SetCreateTreeTimer() Can't create timer." );
  }
	//DebugTrace( "CTreeGDBBrowserBase::SetCreateTreeTimer(%d)", nCreateTreeTimer );
}


void CTreeGDBBrowserBase::KillCreateTreeTimer()
{
  if ( nCreateTreeTimer != 0 )
	{
		KillTimer( nCreateTreeTimer );
	}
  nCreateTreeTimer = 0;
}


void CTreeGDBBrowserBase::OnMouseMove( unsigned nFlags, CPoint point )
{
	CSortTreeControl::OnMouseMove( nFlags, point );
	//
	dragAndDropState.OnMouseMove( nFlags, CTPoint<int>( point.x, point.y ) );
}


void CTreeGDBBrowserBase::OnLButtonDown( unsigned nFlags, CPoint point )
{
	CSortTreeControl::OnLButtonDown( nFlags, point );
	//	
	dragAndDropState.OnLButtonDown( nFlags, CTPoint<int>( point.x, point.y ) );
}


void CTreeGDBBrowserBase::OnLButtonUp( unsigned nFlags, CPoint point )
{
	CSortTreeControl::OnLButtonUp( nFlags, point );
	//
	dragAndDropState.OnLButtonUp( nFlags, CTPoint<int>( point.x, point.y ) );
}


void CTreeGDBBrowserBase::OnLButtonDblClk( unsigned nFlags, CPoint point ) 
{
	CSortTreeControl::OnLButtonDblClk( nFlags, point );
	//
	dragAndDropState.OnLButtonDblClk( nFlags, CTPoint<int>( point.x, point.y ) );
}


void CTreeGDBBrowserBase::OnRButtonDown( unsigned nFlags, CPoint point )
{
	CSortTreeControl::OnRButtonDown( nFlags, point );
	//
	dragAndDropState.OnRButtonDown( nFlags, CTPoint<int>( point.x, point.y ) );
}


void CTreeGDBBrowserBase::OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	CSortTreeControl::OnKeyDown( nChar, nRepCnt, nFlags );
	//
	dragAndDropState.OnKeyDown( nChar, nRepCnt, nFlags );
}


void CTreeGDBBrowserBase::OnContextMenu( CWnd *pwnd, CPoint point )
{
	CSortTreeControl::OnContextMenu( pwnd, point );
	//
	ScreenToClient( &point );
	dragAndDropState.OnContextMenu( CTPoint<int>( point.x, point.y ) );
}


void CTreeGDBBrowserBase::OnKeyUp( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	CSortTreeControl::OnKeyUp( nChar, nRepCnt, nFlags );
	//
	dragAndDropState.OnKeyUp( nChar, nRepCnt, nFlags );
}


void CTreeGDBBrowserBase::ShowContextMenu( const CTPoint<int> &rPoint )
{
	CMenu mainPopupMenu;
	mainPopupMenu.LoadMenu( IDM_MAIN_CONTEXT_MENU );
	CMenu *pMenu = mainPopupMenu.GetSubMenu( MCMN_TREE_GDB_BROWSER );
	if ( pMenu )
	{
		std::string szLabel;
		GetLoadContextMenuLabel( &szLabel );
		MENUITEMINFO menuItemInfo;
		menuItemInfo.cbSize = sizeof( MENUITEMINFO );
		menuItemInfo.fMask = MIIM_STRING;
		menuItemInfo.dwTypeData = const_cast<LPSTR>( szLabel.c_str() );
		pMenu->SetMenuItemInfo( ID_OBJECT_LOAD, &menuItemInfo, false );  
		CPoint point( rPoint.x, rPoint.y );
		ClientToScreen( &point ); 
		pMenu->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x, point.y, Singleton<IMainFrameContainer>()->GetSECWorkbook(), 0 );
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
	}
	mainPopupMenu.DestroyMenu();
}


void CTreeGDBBrowserBase::OnBeginLabelEdit( NMHDR *pNotifyStruct, LRESULT *pResult )
{
	if ( IsEditEnabled() && szItemTextFromBeginLabelEdit.empty() )
	{
		LPNMTVDISPINFO pTVDi = reinterpret_cast<LPNMTVDISPINFO>( pNotifyStruct );
		hLabelEditItem = pTVDi->item.hItem;
		szItemTextFromBeginLabelEdit = GetItemText( pTVDi->item.hItem );
		( *pResult ) = 0;
	}
	else
	{
		( *pResult ) = 1;
	}
}


void CTreeGDBBrowserBase::OnEndLabelEdit( NMHDR *pNotifyStruct, LRESULT *pResult )
{
	//bool bRenamed = false;
	LPNMTVDISPINFO pTVDi = reinterpret_cast<LPNMTVDISPINFO>( pNotifyStruct );
	if ( pTVDi->item.pszText )
	{
		const std::string szItemTextFromEndLabelEdit = std::string( pTVDi->item.pszText );
		const EGDBOType nType = GetTreeItemType( pTVDi->item.hItem );
		if ( ( !szItemTextFromEndLabelEdit.empty() ) &&
				 ( szItemTextFromBeginLabelEdit != szItemTextFromEndLabelEdit ) &&
				 ( FindName( GetParentItem( pTVDi->item.hItem ), szItemTextFromEndLabelEdit, nType, false, pTVDi->item.hItem ) == 0 ) )
		{
			std::string szParentName;	
			GetTreeItemName( GetParentItem( pTVDi->item.hItem ), &szParentName );
			//
			CPtr<CFolderController> pFolderController = CreateController();
			//
			std::string szDestination = szParentName + szItemTextFromEndLabelEdit;
			std::string szSource = szParentName + szItemTextFromBeginLabelEdit;
			if ( nType == GDBO_FOLDER )
			{
				szDestination += PATH_SEPARATOR_CHAR;
				szSource += PATH_SEPARATOR_CHAR;
			}
			pFolderController->AddRenameOperation( szDestination, szSource, false );
			// Надо передавать 0!
			( *pResult ) = pFolderController->Redo( true, true, 0 );
			if ( ( *pResult ) )
			{
				GetViewManipulator()->ClearCache();
				//
				EnsureVisible( pTVDi->item.hItem );
				UpdateSelectionManipulator( true );
				hLabelEditSortTimerItem = GetParentItem( pTVDi->item.hItem );
				SetLabelEditSortTimer();
			}
		}
		else
		{
			( *pResult ) = 0;
		}
	}
	else
	{
		( *pResult ) = 0;
	}
	szItemTextFromBeginLabelEdit.clear();
	hLabelEditItem = 0;
	SetFocus();
}


void CTreeGDBBrowserBase::OnBeginLabelEditList( NMHDR *pNotifyStruct, LRESULT *pResult )
{
	( *pResult ) = 1;
}


void CTreeGDBBrowserBase::OnEndLabelEditList( NMHDR *pNotifyStruct, LRESULT *pResult )
{
	( *pResult ) = 0;
	SetFocus();
}


void CTreeGDBBrowserBase::OnSetFocus( CWnd* pOldWnd )
{
	CSortTreeControl::OnSetFocus( pOldWnd );
	
	Singleton<ICommandHandlerContainer>()->Set( CHID_OBJECT, this );
	Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, this );
	if ( nGDBBrowserID != -1 )
	{
		Singleton<IMainFrameContainer>()->Get()->SaveObjectStorage( nGDBBrowserID );
	}
}


void CTreeGDBBrowserBase::OnKillFocus( CWnd* pNewWnd )
{
	CSortTreeControl::OnKillFocus( pNewWnd );
}


void CTreeGDBBrowserBase::SaveHeaderWidthInternal()
{
	SaveHeaderWidth();
	//
	SObjectSet objectSet;
	GetSaveHeaderWidthLabel( &( objectSet.szObjectTypeName ) );
	InsertHashSetElement( &( objectSet.objectNameSet ), VIEW_COLLECTION_ID );
	//
	CFolderController folderController;
	folderController.SetObjectSet( objectSet );
	folderController.Redo( true, true, this );
}


bool CTreeGDBBrowserBase::GetCurrentObjectSet( SObjectSet *pObjectSet )
{
	NI_ASSERT( pObjectSet != 0, "CTreeGDBBrowserBase::GetActiveObjectSet: pObjectSet == 0" );
	//
	if ( GetViewManipulator() )
	{
		pObjectSet->szObjectTypeName = GetObjectSet().szObjectTypeName;
		pObjectSet->objectNameSet.clear();
		//if ( GetSelectedCount() == 1 )
		{
			HTREEITEM hSelectedItem = GetFirstSelectedItem();
			while ( hSelectedItem )
			{
				std::string szName;
				GetTreeItemName( hSelectedItem, &szName ); 
				if ( ( !szName.empty() ) && 
						 ( szName[szName.size() - 1] != PATH_SEPARATOR_CHAR ) )
				{
					InsertHashSetElement( &( pObjectSet->objectNameSet ), CDBID( szName ) );
				}
				hSelectedItem = GetNextSelectedItem( hSelectedItem );
			}
		}
		return true;
	}
	return false;
}


bool CTreeGDBBrowserBase::GetCurrentSelectionSet( SSelectionSet *pSelectionSet )
{
	NI_ASSERT( pSelectionSet != 0, "CTreeGDBBrowserBase::GetActiveObjectSet: pSelectionSet == 0" );
	//
	if ( GetViewManipulator() )
	{
		pSelectionSet->szObjectTypeName = GetObjectSet().szObjectTypeName;
		pSelectionSet->objectNameList.clear();
		if ( GetSelectedCount() == 1 )
		{
			std::string szName;
			HTREEITEM hSelectedItem = GetFirstSelectedItem();
			while ( hSelectedItem )
			{
				GetTreeItemName( hSelectedItem, &szName ); 
				if ( !szName.empty() )
				{
					pSelectionSet->objectNameList.push_back( szName );
				}
				hSelectedItem = GetNextSelectedItem( hSelectedItem );
			}
		}
		return true;
	}
	return false;
}


void CTreeGDBBrowserBase::UpdateSelectionManipulator( bool bUpdate )
{
	//DebugTrace( "CTreeGDBBrowserBase::UpdateSelectionManipulator()" );
	// Может быть ноль ( если нет выделенных объектов )
	std::string szCurrentObject;
	GetCurrentTreeItemName( &szCurrentObject );
	if ( GetStrongSelection() && ( szCurrentObject != Singleton<IUserDataContainer>()->Get()->objectTypeDataMap[GetObjectSet().szObjectTypeName].szCurrentObject ) )
	{
		SetCurrentTreeItemName( Singleton<IUserDataContainer>()->Get()->objectTypeDataMap[GetObjectSet().szObjectTypeName].szCurrentObject, true );
	}
	else
	{
		SObjectSet objectSet;
		GetCurrentObjectSet( &objectSet );
		CPtr<IManipulator> pObjectManipulator = CManipulatorManager::CreateObectSetManipulator( objectSet );
		if ( nPCDialogCommandHandlerID != INVALID_COMMAND_HANDLER_ID )
		{
			IView *pView = 0;
			Singleton<ICommandHandlerContainer>()->HandleCommand( nPCDialogCommandHandlerID, ID_PC_DIALOG_GET_VIEW, reinterpret_cast<uint32_t>( &pView ) );
			if ( pView != 0 )
			{
				//DebugTrace( "CTreeGDBBrowserBase::UpdateSelectionManipulator() SetViewManipulator( %d )", bUpdate );
				pView->SetViewManipulator( pObjectManipulator, objectSet, std::string() );
				if ( bUpdate )
				{
					Singleton<ICommandHandlerContainer>()->HandleCommand( nPCDialogCommandHandlerID, ID_PC_DIALOG_CREATE_TREE, 0 );
				}
			}
		}
		if ( CWnd *pwndParent = GetParent() )
		{
			pwndParent->SendMessage( WM_TREE_GDB_BROWSER, MAKEWPARAM( TREE_GDB_BROWSER_CHANGE_SELECTION, 0 ), reinterpret_cast<LPARAM>( this ) );
		}
	}
}


void CTreeGDBBrowserBase::OnItemExpanded( NMHDR* pNMHDR, LRESULT* pResult ) 
{
	if ( !GetObjectSet().szObjectTypeName.empty() )
	{
		if ( !bCreateControls )
		{
			bCreateControls = true;
			NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
			//
			HTREEITEM hItem = pNMTreeView->itemNew.hItem;
			bool bExpanded = ( pNMTreeView->itemNew.state & TVIS_EXPANDED ) > 0;

			std::string szCurrentObject;
			GetTreeItemName( hItem, &szCurrentObject );

			SUserData::SObjectTypeData::CExpandedObjectSet &rExpandedObjectSet = Singleton<IUserDataContainer>()->Get()->objectTypeDataMap[GetObjectSet().szObjectTypeName].expandedObjectSet;
			if ( bExpanded )
			{
				InsertHashSetElement( &( rExpandedObjectSet ), szCurrentObject );
			}
			else
			{
				SUserData::SObjectTypeData::CExpandedObjectSet::iterator posExpandedObject = rExpandedObjectSet.find( szCurrentObject );
				if ( posExpandedObject != rExpandedObjectSet.end() )
				{
					rExpandedObjectSet.erase( posExpandedObject );
				}
			}
			//
			CPtr<CFolderController> pFolderController = CreateController();
			pFolderController->AddExpandOperation( szCurrentObject, bExpanded );
			pFolderController->Redo( true, true, this );
			//
			bCreateControls = false;
		}
	}
	( *pResult ) = 0;
}


void CTreeGDBBrowserBase::OnSelChanged( NMHDR *pNotifyStruct, LRESULT *pResult )
{
	if ( !bCreateControls )
	{
		bCreateTreeSelectionChanged = true;
		//DebugTrace( "CTreeGDBBrowserBase::OnSelChanged()" );
		//LPNMTREEVIEW pNMTv = reinterpret_cast<LPNMTREEVIEW>( pNotifyStruct ); 
		//
		if ( !GetObjectSet().szObjectTypeName.empty() )
		{
			if ( HTREEITEM hFocusedItem = GetSelectedItem() )
			{
				std::string szCurrentObject;
				GetTreeItemName( hFocusedItem, &szCurrentObject );
				Singleton<IUserDataContainer>()->Get()->objectTypeDataMap[GetObjectSet().szObjectTypeName].szCurrentObject = szCurrentObject;
			}
			else
			{
				Singleton<IUserDataContainer>()->Get()->objectTypeDataMap[GetObjectSet().szObjectTypeName].szCurrentObject.clear();
			}
		}
		//
		UpdateSelectionManipulator( true );
	}
	if ( pResult )
	{
		( *pResult ) = 0;
	}
}


void CTreeGDBBrowserBase::SetPCDialogCommandHandlerID( unsigned _nPCDialogCommandHandlerID, bool bUpdate )
{
	if ( nPCDialogCommandHandlerID != INVALID_COMMAND_HANDLER_ID )
	{
		IView *pView = 0;
		Singleton<ICommandHandlerContainer>()->HandleCommand( nPCDialogCommandHandlerID, ID_PC_DIALOG_GET_VIEW, reinterpret_cast<uint32_t>( &pView ) );
		if ( pView != 0 )
		{
			pView->RemoveViewManipulator();
		}
	}
	nPCDialogCommandHandlerID = _nPCDialogCommandHandlerID;
	if ( nPCDialogCommandHandlerID != INVALID_COMMAND_HANDLER_ID )
	{
		UpdateSelectionManipulator( bUpdate );
	}
}


CTreeGDBBrowserBase::EGDBOType CTreeGDBBrowserBase::GetTreeItemType( HTREEITEM hItem )
{
	EGDBOType itemType = GDBO_UNKNOWN;
	int nImageIndex = 0;
	int nSelectedImageIndex = 0;
	if ( GetItemImage( hItem, nImageIndex, nSelectedImageIndex ) )
	{
		itemType = static_cast<EGDBOType>( nImageIndex );
	}
	return itemType;
}


// CSortTreeControl

HTREEITEM CTreeGDBBrowserBase::GetTreeItem( const std::string &rszName )
{
	if ( rszName.empty() )
	{
		return 0;
	}
	//
	HTREEITEM hItem = CSortTreeControl::GetTreeItem( rszName );
	if ( hItem != 0 )
	{
		return hItem;
	}
	else
	{
		HTREEITEM hParentItem = TVI_ROOT;
		
		std::string szAdditionalName = rszName;
		if ( szAdditionalName[szAdditionalName.size() - 1] == PATH_SEPARATOR_CHAR )
		{
			szAdditionalName = szAdditionalName.substr( 0, szAdditionalName.size() - 1 );
		}

		int nDividerPos = std::string::npos;
		do
		{
			nDividerPos = szAdditionalName.find( PATH_SEPARATOR_CHAR );
			const std::string szName = szAdditionalName.substr( 0, nDividerPos );
			
			HTREEITEM hItem = GetChildItem( hParentItem );
			while ( hItem != 0 )
			{
				std::string szText = GetItemText( hItem );
				if ( IsIgnoreCase() )
				{
					std::string szTextIgnoreCase = szText;
					std::string szNameIgnoreCase = szName;
					NStr::ToLower( &szTextIgnoreCase );
					NStr::ToLower( &szNameIgnoreCase );
					if ( szTextIgnoreCase == szNameIgnoreCase )
					{
						break;
					}
				}
				else
				{
					if ( szText == szName )
					{
						break;
					}
				}
				hItem = GetNextSiblingItem( hItem );
			}
			hParentItem = hItem;
			if ( hParentItem == 0 )
			{
				break;
			}
			szAdditionalName = szAdditionalName.substr( nDividerPos + 1 );
		}
		while ( nDividerPos != std::string::npos );
		return hParentItem;
	}
}


bool CTreeGDBBrowserBase::GetTreeItemName( HTREEITEM hItem, std::string *pszName )
{
	if ( ( hItem == 0 ) || ( hItem == TVI_ROOT ) || ( hItem == TVI_FIRST ) || ( hItem == TVI_LAST ) )
	{
		return true;
	}
	std::string szItemText = GetItemText( hItem );
	EGDBOType nType = GetTreeItemType( hItem );
	if ( nType == GDBO_OBJECT )
	{
		if ( pszName )
		{
			*pszName = szItemText + *pszName;
		}
	}
	else if ( nType == GDBO_FOLDER )
	{
		if ( pszName )
		{
			*pszName = szItemText + PATH_SEPARATOR_CHAR + *pszName;
		}
	}
	HTREEITEM hParentItem = GetParentItem( hItem );
	return GetTreeItemName( hParentItem, pszName );
}


bool CTreeGDBBrowserBase::GetCurrentTreeItemName( std::string *pszName )
{
	if ( GetSelectedCount() == 1 )
	{
		if ( HTREEITEM hFocusedItem = GetSelectedItem() )
		{
			GetTreeItemName( hFocusedItem, pszName ); 
			return true;
		}
	}
	return false;
}


bool CTreeGDBBrowserBase::SetCurrentTreeItemName( const std::string &rszName, bool bUpdateSelection )
{
	if ( bUpdateSelection )
	{
		if ( HTREEITEM hItem = GetTreeItem( rszName ) )
		{
			DeselectAllItems();
			Select( hItem, TVGN_CARET );
			EnsureVisible( hItem );
			return true;
		}
	}
	else
	{
		if ( !GetObjectSet().szObjectTypeName.empty() )
		{
			Singleton<IUserDataContainer>()->Get()->objectTypeDataMap[GetObjectSet().szObjectTypeName].szCurrentObject = rszName;
		}
		return true;
	}
	return false;
}


void CTreeGDBBrowserBase::CreateTree()
{
	if ( !IsWindow( m_hWnd ) )
	{
		return;
	}
	//
	KillCreateTreeTimer();
	//
	bCreateControls = true;
	DeleteAllTreeItems();
	bCreateTreeSelectionChanged = false;
	szIgnoreSelectionName.clear();
	bCreateControls = false;
	//
	if ( GetViewManipulator() )
	{
		if ( !GetObjectSet().szObjectTypeName.empty() )
		{	
			bool bNeedStartTimer = false;
			const std::string szCurrentObject = Singleton<IUserDataContainer>()->Get()->objectTypeDataMap[GetObjectSet().szObjectTypeName].szCurrentObject;
			if ( ( !szCurrentObject.empty() ) && 
					 ( szCurrentObject[szCurrentObject.size() - 1] != PATH_SEPARATOR_CHAR ) )
			{
				szIgnoreSelectionName = szCurrentObject;
				CDBID objectDBID = CDBID( szIgnoreSelectionName );
				if ( NDb::DoesObjectExist( objectDBID ) && ( NDb::GetClassTypeName( objectDBID ) == GetObjectSet().szObjectTypeName ) )
				{
					const SIteratorDesc *pDesc = dynamic_cast<const SIteratorDesc*>( GetViewManipulator()->GetDesc( szIgnoreSelectionName ) );
					if ( HTREEITEM hItem = AddTreeItem( TVI_ROOT, szIgnoreSelectionName, GDBO_OBJECT, pDesc ) )
					{
						DeselectAllItems();
						Select( hItem, TVGN_CARET );
						EnsureVisible( hItem );
						RedrawWindow();
					}
				}
				bNeedStartTimer = true;
			}
			if ( pCreateTreeManipulatorIterator = GetViewManipulator()->Iterate( true, ECT_CACHE_LOCAL ) )
			{
				if ( bNeedStartTimer )
				{
					SetCreateTreeTimer();
				}
				else
				{
					OnCreateTreeTimer();
				}
			}
			else
			{
				UpdateSelectionManipulator( true );
			}
		}
	}
}


void CTreeGDBBrowserBase::OnCreateTreeTimer()
{
	KillCreateTreeTimer();
	//
	if ( pCreateTreeManipulatorIterator )
	{
		int nCount = 0;
		if ( !GetObjectSet().szObjectTypeName.empty() )
		{
			while ( ( !pCreateTreeManipulatorIterator->IsEnd() ) && ( nCount < GetCreateTreeTimerCount() ) )
			{
				std::string szName;
				std::string szType;
				pCreateTreeManipulatorIterator->GetName( &szName );
				pCreateTreeManipulatorIterator->GetType( &szType );
				//DebugTrace("Tree: %s:%s", szType.c_str(), szName.c_str() );
				bool bNeedAddTreeItem = szIgnoreSelectionName.empty();
				if ( !bNeedAddTreeItem )
				{
					if ( IsIgnoreCase() )
					{
						std::string szNameIgnoreCase = szName;
						std::string szIgnoreSelectionNameIgnoreCase = szIgnoreSelectionName;
						NStr::ToLower( &szNameIgnoreCase );
						NStr::ToLower( &szIgnoreSelectionNameIgnoreCase );
						bNeedAddTreeItem = ( szNameIgnoreCase != szIgnoreSelectionNameIgnoreCase );
					}
					else
					{
						bNeedAddTreeItem = ( szName != szIgnoreSelectionName );
					}
				}
				if ( bNeedAddTreeItem )
				{
					EGDBOType nType = typeMnemonics.Get( szType );
					if ( ( nType == GDBO_OBJECT ) || ( nType == GDBO_FOLDER ) )
					//if ( nType == GDBO_OBJECT )
					{
						const SIteratorDesc *pDesc = dynamic_cast<const SIteratorDesc*>( GetViewManipulator()->GetDesc( szName ) );
						AddTreeItem( TVI_ROOT, szName, nType, pDesc );
					}
				}
				pCreateTreeManipulatorIterator->Next();
				++nCount;
			}
		}
	}
	if ( ( pCreateTreeManipulatorIterator == 0 ) || ( pCreateTreeManipulatorIterator->IsEnd() ) )
	{
		pCreateTreeManipulatorIterator = 0;
		bCreateControls = true;
		if ( HTREEITEM hItem = GetFirstSelectedItem() )
		{
			EnsureVisible( hItem );
		}
		bCreateControls = false;
		RedrawWindow();

		if ( !bCreateTreeSelectionChanged && szIgnoreSelectionName.empty() )
		{
			bCreateControls = true;
			if ( !GetObjectSet().szObjectTypeName.empty() )
			{
				UpdateSelectionManipulator( true );
			}
			bCreateControls = false;
		}
		//
		RedrawWindow();
	}
	else
	{
		bCreateControls = true;
		if ( HTREEITEM hItem = GetFirstSelectedItem() )
		{
			EnsureVisible( hItem );
		}
		bCreateControls = false;
		RedrawWindow();
		SetCreateTreeTimer();
	}
}


HTREEITEM CTreeGDBBrowserBase::AddTreeItem( HTREEITEM hRootItem, const std::string &rszAdditionalName, EGDBOType nType, const SIteratorDesc *pDesc )
{
	HTREEITEM hAddedItem = 0;
	if ( rszAdditionalName.empty() )
	{
		return hAddedItem;
	}
	const int nDividerPos = rszAdditionalName.find( PATH_SEPARATOR_CHAR );
	const std::string szShortName = rszAdditionalName.substr( 0, nDividerPos );
	if ( nDividerPos != std::string::npos )
	{
		const std::string szAdditionalName = rszAdditionalName.substr( nDividerPos + 1 );
		HTREEITEM hItem = GetChildItem( hRootItem );
		while ( hItem != 0 )
		{
			const std::string szItemText = GetItemText( hItem );
			if ( IsIgnoreCase() )
			{
				std::string szItemTextIgnoreCase = szItemText;
				std::string szShortNameIgnoreCase = szShortName;
				NStr::ToLower( &szItemTextIgnoreCase );
				NStr::ToLower( &szShortNameIgnoreCase );
				if ( szItemTextIgnoreCase == szShortNameIgnoreCase )
				{
					break;
				}
			}
			else
			{
				if ( szItemText == szShortName )
				{
					break;
				}
			}
			hItem = GetNextSiblingItem( hItem );
		}
		if ( hItem == 0 )
		{
			HTREEITEM hInsertAfter = FindPlaceToInsert( hRootItem, szShortName, GDBO_FOLDER );
			hItem = InsertTreeItem( szShortName.c_str(),
															GDBO_FOLDER,
															GDBO_FOLDER + GDBO_COUNT,
															hRootItem,
															hInsertAfter );
			if ( hItem && !GetObjectSet().szObjectTypeName.empty() )
			{
				SetTreeItemView( hItem, 0 );
				std::string szName;
				GetTreeItemName( hItem, &szName ); 
				const SUserData::SObjectTypeData::CExpandedObjectSet &rExpandedObjectSet = Singleton<IUserDataContainer>()->Get()->objectTypeDataMap[GetObjectSet().szObjectTypeName].expandedObjectSet;
				const bool bExpand = ( rExpandedObjectSet.find( szName ) != rExpandedObjectSet.end() );
				//DebugTrace( "rExpandedObjectSet: %d: %s", bExpand, szHashName.c_str() );
				bCreateControls = true;
				SetItemState( hItem, bExpand ? TVIS_EXPANDED : 0, TVIS_EXPANDED );
				bCreateControls = false;
			}
		}
		if ( hItem )
		{
			if ( !szAdditionalName.empty() )
			{
				hAddedItem = AddTreeItem( hItem, szAdditionalName, nType, pDesc );
			}
			else
			{
				hAddedItem = hItem;
			}
		}
	}
	else
	{
		HTREEITEM hInsertAfter = FindPlaceToInsert( hRootItem, szShortName, nType );
		hAddedItem = InsertTreeItem( szShortName.c_str(),
																 nType,
																 nType + GDBO_COUNT,
																 hRootItem,
																 hInsertAfter );
		SetTreeItemView( hAddedItem, 0 );
	}
	return hAddedItem;
}


void CTreeGDBBrowserBase::SetTreeItemView( HTREEITEM hItem, const std::string *pszName )
{
	if ( hItem != 0 )
	{
		EGDBOType nType = GetTreeItemType( hItem );
		if ( nType != GDBO_OBJECT )
		{
			SetItemText( hItem, 1, "" );
		}
		else
		{
			std::string szName;
			if ( pszName == 0 )
			{
				GetTreeItemName( hItem, &szName );
			}
			else
			{
				szName = ( *pszName );
			}
			SetItemText( hItem, 1, "" );
		}
	}
}


void CTreeGDBBrowserBase::GetUniqueName( HTREEITEM hParentItem, const std::string &rszName, EGDBOType nType, std::string *pszName )
{
	if ( pszName )
	{
		( *pszName ) = rszName;
		bool bExists = ( FindName( hParentItem, ( *pszName ), nType, true, 0 ) != 0 );
		uint32_t nNumber = 2;
		//
		std::string szBaseName = rszName;
		const bool bExtendExtention = CStringManager::CutFileExtention( &szBaseName, ".xdb" );
		//
		while ( bExists )
		{
			( *pszName ) = szBaseName + fmt::format( " ({})", nNumber );
      if ( bExtendExtention ) 
			{
				CStringManager::ExtendFileExtention( pszName, ".xdb" );
			}
			bExists = ( FindName( hParentItem, ( *pszName ), nType, true, 0 ) != 0 );
			++nNumber;
		}
	}
}


HTREEITEM CTreeGDBBrowserBase::FindName( HTREEITEM hParentItem, const std::string &rszName, EGDBOType nType, bool bCheckType, HTREEITEM hItemToSkip )
{
	HTREEITEM hItem = GetChildItem( hParentItem );
	while( hItem != 0 )
	{
		if ( hItem != hItemToSkip )
		{
			std::string szText = GetItemText( hItem );
			EGDBOType nLocalType = GetTreeItemType( hItem );
			if ( IsIgnoreCase() )
			{
				std::string szNameIgnoreCase = rszName;
				std::string szTextIgnoreCase = szText;
				NStr::ToLower( &szNameIgnoreCase );
				NStr::ToLower( &szTextIgnoreCase );
				if ( ( szNameIgnoreCase == szTextIgnoreCase ) && ( bCheckType ? ( nLocalType == nType ) : true ) )
				{
					return hItem;
				}
			}
			else
			{
				if ( ( rszName == szText ) && ( bCheckType ? ( nLocalType == nType ) : true ) )
				{
					return hItem;
				}
			}
		}
		hItem = GetNextSiblingItem( hItem );
	}
	return 0;
}


HTREEITEM CTreeGDBBrowserBase::FindPlaceToInsert( HTREEITEM hParentItem, const std::string &rszName, EGDBOType nType )
{
	CString strName = rszName.c_str();
	HTREEITEM hItemToReturn = TVI_FIRST;
	HTREEITEM hItem = GetChildItem( hParentItem );
	while ( hItem != 0 )
	{
		CString strLocalName = GetItemText( hItem );
		EGDBOType nLocalType = GetTreeItemType( hItem );
		if ( SortItemText( strLocalName, nLocalType, strName, nType ) >= 0 )
		{
			return hItemToReturn;
		}
		hItemToReturn = hItem;
		hItem = GetNextSiblingItem( hItem );
	}
	return TVI_LAST;
}


bool CTreeGDBBrowserBase::IsNotEditLabel()
{ 
	return szItemTextFromBeginLabelEdit.empty();
}


void CTreeGDBBrowserBase::PickTextColors( LvPaintContext* pPC )
{
	if ( pPC )
	{
		CSortTreeControl::PickTextColors( pPC );
		if ( ( !bCreateControls ) && GetViewManipulator() && ( pPC->lvi.iSubItem == 0 ) )
		{
			TvPaintContext *pTvPC = dynamic_cast<TvPaintContext*>( pPC );
			if ( pTvPC->tvi.hItem != hLabelEditItem )
			{
				if ( ( ::GetFocus() != m_hWnd ) || ( ( pTvPC->lvi.state & ( LVIS_SELECTED | LVIS_DROPHILITED | LVIS_CUT | LVIS_FOCUSED ) ) == 0 ) )
				{
					//if ( GetTreeItemType( pTvPC->tvi.hItem ) != GDBO_OBJECT )
					{
						COLORREF color = RGB( 0, 0, 0 );
						if ( !GetTreeItemColor( pTvPC->tvi.hItem, &color ) )
						{
							std::string szName;	
							GetTreeItemName( pTvPC->tvi.hItem, &szName );
							int nColor = 0;
							if ( CManipulatorManager::GetValue( &nColor, GetViewManipulator(), szName ) )
							{
								color = nColor;
								SetTreeItemColor( pTvPC->tvi.hItem, color );
							}
							//
						}
						pTvPC->rgbText = color;
					}
				}
			}
		}
	}
}


bool CTreeGDBBrowserBase::ExecuteTreeOperation( const STreeOperation &rTreeOperation )
{
	//bCreateControls = true;
	switch( rTreeOperation.nType )
	{
		case STreeOperation::TYPE_REMOVE:
		{
			HTREEITEM hChildItem = GetChildItem( rTreeOperation.hDestination );
			while ( hChildItem )
			{
				STreeOperation deleteTreeOperation;
				deleteTreeOperation.nType = STreeOperation::TYPE_REMOVE;
				deleteTreeOperation.hDestination = hChildItem;
				deleteTreeOperation.hSource = 0;
				//
				hChildItem = GetNextSiblingItem( hChildItem );
				//
				if ( !ExecuteTreeOperation( deleteTreeOperation ) )
				{
					return false;
				}
			}
			// все правильно, количество элементов может поменяться
			if ( GetChildItem( rTreeOperation.hDestination ) == 0 )
			{
				EGDBOType nType = GetTreeItemType( rTreeOperation.hDestination );
				std::string szObjectName;
				GetTreeItemName( rTreeOperation.hDestination, &szObjectName );
				if ( nType == GDBO_FOLDER )
				{
					CPtr<CFolderController> pFolderController = CreateController();
					pFolderController->AddRemoveOperation( szObjectName );
					pFolderController->Redo( true, true, 0 );
				}
				else
				{
					Singleton<IFolderCallback>()->ClearUndoData();
					if ( !Singleton<IFolderCallback>()->IsObjectLocked( GetObjectSet().szObjectTypeName, szObjectName ) )
					{
						if ( !Singleton<IBuilderContainer>()->RemoveObject( GetObjectSet().szObjectTypeName, szObjectName ) )
						{
							Singleton<IFolderCallback>()->UndoChanges();
						}
					}
					else
					{
						NLog::Log( LT_IMPORTANT, "Can't remove object. Object is locked. %s:%s\n", GetObjectSet().szObjectTypeName.c_str(), szObjectName.c_str() ); 
					}
					Singleton<IFolderCallback>()->ClearUndoData();
				}
			}
			break;
		}
		case STreeOperation::TYPE_CHECK:
		{
			HTREEITEM hChildItem = GetChildItem( rTreeOperation.hDestination );
			while ( hChildItem )
			{
				STreeOperation exportTreeOperation;
				exportTreeOperation.nType = rTreeOperation.nType;
				exportTreeOperation.bExportReferences = rTreeOperation.bExportReferences;
				exportTreeOperation.hDestination = hChildItem;
				exportTreeOperation.hSource = 0;
				//
				hChildItem = GetNextSiblingItem( hChildItem );
				//
				if ( !ExecuteTreeOperation( exportTreeOperation ) )
				{
					return false;
				}
			}
			if ( GetTreeItemType( rTreeOperation.hDestination ) == GDBO_OBJECT )
			{
				std::string szObjectName;
				GetTreeItemName( rTreeOperation.hDestination, &szObjectName );
				//
				if ( CPtr<IManipulator> pObjectManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( GetObjectSet().szObjectTypeName, szObjectName ) )
				{
					Singleton<IExporterContainer>()->CheckObject( pObjectManipulator,
																												GetObjectSet().szObjectTypeName,
																												szObjectName,
																												rTreeOperation.bExportReferences );
				}
			}
			break;
		}
		case STreeOperation::TYPE_EXPORT:
		case STreeOperation::TYPE_EXPORT_FORCE:
		{
			HTREEITEM hChildItem = GetChildItem( rTreeOperation.hDestination );
			while ( hChildItem )
			{
				STreeOperation exportTreeOperation;
				exportTreeOperation.nType = rTreeOperation.nType;
				exportTreeOperation.bExportReferences = rTreeOperation.bExportReferences;
				exportTreeOperation.hDestination = hChildItem;
				exportTreeOperation.hSource = 0;
				//
				hChildItem = GetNextSiblingItem( hChildItem );
				//
				if ( !ExecuteTreeOperation( exportTreeOperation ) )
				{
					return false;
				}
			}
			if ( GetTreeItemType( rTreeOperation.hDestination ) == GDBO_OBJECT )
			{
				std::string szObjectName;
				GetTreeItemName( rTreeOperation.hDestination, &szObjectName );
				//
				if ( CPtr<IManipulator> pObjectManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( GetObjectSet().szObjectTypeName, szObjectName ) )
				{
					Singleton<IExporterContainer>()->ExportObject( pObjectManipulator,
																												 GetObjectSet().szObjectTypeName,
																												 szObjectName,
																												 ( rTreeOperation.nType == STreeOperation::TYPE_EXPORT_FORCE ),
																												 rTreeOperation.bExportReferences );
					return true;
				}
			}
			break;
		}
		case STreeOperation::TYPE_COLOR:
		{
			{
				std::string szObjectName;
				GetTreeItemName( rTreeOperation.hDestination, &szObjectName );
				//
				CPtr<CFolderController> pFolderController = CreateController();
				pFolderController->AddColorOperation( szObjectName, rTreeOperation.color );
				pFolderController->Redo( true, true, 0 );
			}
			break;
		}
		case STreeOperation::TYPE_COPY:
		{
			std::string szDestination;
			std::string szSource;
			GetTreeItemName( rTreeOperation.hDestination, &szDestination );
			GetTreeItemName( rTreeOperation.hSource, &szSource );
			//
			std::string szText = GetItemText( rTreeOperation.hSource );
			EGDBOType nType = GetTreeItemType( rTreeOperation.hSource );
			HTREEITEM hNewItem = FindName( rTreeOperation.hDestination, szText, nType, true, 0 );
			bool bNeedCopy = true;
			if ( hNewItem == 0 )
			{
				szDestination += szText;
				if ( nType == GDBO_FOLDER )
				{
					szDestination += PATH_SEPARATOR_CHAR;
				}
			}
			else
			{
				if ( nType == GDBO_OBJECT )
				{
					std::string szNewText;
					GetUniqueName( rTreeOperation.hDestination, szText, nType, &szNewText );
					szDestination += szNewText;
				}
				else
				{
					// проверяем на тождественность ссылок
					// копирование папки происходит только при копировании в того же родителя
					std::string szDestinationName;
					GetTreeItemName( hNewItem, &szDestinationName );
					if ( szDestinationName == szSource )
					{
						std::string szNewText;
						GetUniqueName( rTreeOperation.hDestination, szText, nType, &szNewText );
						szDestination += szNewText + PATH_SEPARATOR_CHAR;
					}
					else
					{
						bNeedCopy = false;
					}
				}
			}
			// Непосредственно - копирование
			if ( bNeedCopy )
			{
				if ( nType == GDBO_FOLDER )
				{
					CPtr<CFolderController> pFolderController = CreateController();
					pFolderController->AddCopyOperation( szDestination, szSource );
					pFolderController->Redo( true, true, 0 );
				}
				else
				{
					std::string szObjectName;
					GetTreeItemName( rTreeOperation.hDestination, &szObjectName );
					//
					Singleton<IFolderCallback>()->ClearUndoData();
					if ( !Singleton<IBuilderContainer>()->CopyObject( GetObjectSet().szObjectTypeName, szDestination, szSource ) )
					{
						Singleton<IFolderCallback>()->UndoChanges();
					}
					Singleton<IFolderCallback>()->ClearUndoData();
				}
				hNewItem = GetTreeItem( szDestination );
			}
			//Запускаем копирование детей
			if ( hNewItem && ( nType == GDBO_FOLDER ) )
			{
				HTREEITEM hChildItem = GetChildItem( rTreeOperation.hSource );
				while ( hChildItem )
				{
					STreeOperation copyTreeOperation;
					copyTreeOperation.nType = STreeOperation::TYPE_COPY;
					copyTreeOperation.hDestination = hNewItem;
					copyTreeOperation.hSource = hChildItem;
					//
					hChildItem = GetNextSiblingItem( hChildItem );
					//
					if ( !ExecuteTreeOperation( copyTreeOperation ) )
					{
						return false;
					}
				}
			}
			break;
		}
		case STreeOperation::TYPE_RENAME:
		{
			std::string szDestination;
			std::string szSource;
			GetTreeItemName( rTreeOperation.hDestination, &szDestination );
			GetTreeItemName( rTreeOperation.hSource, &szSource );
			//
			std::string szText = GetItemText( rTreeOperation.hSource );
			EGDBOType nType = GetTreeItemType( rTreeOperation.hSource );
			HTREEITEM hNewItem = FindName( rTreeOperation.hDestination, szText, nType, true, 0 );
			bool bNeedRename = true;
			bool bNeedRemove = false;
			if ( hNewItem == 0 )
			{
				if ( nType == GDBO_OBJECT )
				{
					szDestination += szText;
				}
				else
				{
					szDestination += szText + PATH_SEPARATOR_CHAR;
					//
					CPtr<CFolderController> pFolderController = CreateController();
					pFolderController->AddInsertOperation( szDestination );
					pFolderController->Redo( true, true, 0 );
					hNewItem = GetTreeItem( szDestination );
					//
					bNeedRename = false;
					bNeedRemove = true;
				}
			}
			else
			{
				// проверяем на тождественность ссылок
				std::string szDestinationName;
				GetTreeItemName( hNewItem, &szDestinationName );
				if ( szDestinationName != szSource )
				{
					if ( nType == GDBO_OBJECT )
					{
						std::string szNewText;
						GetUniqueName( rTreeOperation.hDestination, szText, nType, &szNewText );
						szDestination += szNewText;
					}
					else
					{
						bNeedRename = false;
						bNeedRemove = true;
					}
				}
				else
				{
					bNeedRename = false;
					bNeedRemove = false;
				}
			}
			// Непосредственно - перенос
			if ( ( bNeedRename ) && ( nType != GDBO_FOLDER ) )
			{
				Singleton<IFolderCallback>()->ClearUndoData();
				if ( !Singleton<IBuilderContainer>()->RenameObject( GetObjectSet().szObjectTypeName, szDestination, szSource ) )
				{
					Singleton<IFolderCallback>()->UndoChanges();
				}
				Singleton<IFolderCallback>()->ClearUndoData();
				hNewItem = GetTreeItem( szDestination );
			}
			//Запускаем перенос детей
			if ( hNewItem && ( nType == GDBO_FOLDER ) )
			{
				HTREEITEM hChildItem = GetChildItem( rTreeOperation.hSource );
				while ( hChildItem )
				{
					STreeOperation renameTreeOperation;
					renameTreeOperation.nType = STreeOperation::TYPE_RENAME;
					renameTreeOperation.hDestination = hNewItem;
					renameTreeOperation.hSource = hChildItem;
					//
					hChildItem = GetNextSiblingItem( hChildItem );
					//
					if ( !ExecuteTreeOperation( renameTreeOperation ) )
					{
						return false;
					}
				}
				if ( bNeedRemove )
				{
					CPtr<CFolderController> pFolderController = CreateController();
					pFolderController->AddRemoveOperation( szSource );
					pFolderController->Redo( true, true, 0 );
				}
			}
			break;
		}
		default:
			break;
	}
	return true;
}


bool CTreeGDBBrowserBase::ExecuteTreeOperations( const CTreeOperationList &rTreeOperationList )
{
	//Singleton<IResourceManager>()->ResetCache();
	for ( CTreeOperationList::const_iterator itTreeOperation = rTreeOperationList.begin(); itTreeOperation != rTreeOperationList.end(); ++itTreeOperation )
	{
		if ( !ExecuteTreeOperation( *itTreeOperation ) )
		{
			return false;
		}
	}
	return true;
}


void CTreeGDBBrowserBase::NewFolder( HTREEITEM hParentItem )
{
	if ( nCreateTreeTimer != 0 )
	{
		return;
	}
	if ( !GetViewManipulator() )
	{
		return;
	}
	CString strNewName;
	strNewName.LoadString( IDS_TREE_GDB_BROWSE_NEW_FOLDER );
	const std::string szNewName = strNewName;
	std::string szNewFolderName;
	GetUniqueName( hParentItem, szNewName, GDBO_FOLDER, &szNewFolderName );
	szNewFolderName += PATH_SEPARATOR_CHAR;
	std::string szUniqueObjectName = szNewFolderName;
	// Расштряем имя до полного
	GetTreeItemName( hParentItem, &szUniqueObjectName );
	
	CPtr<CFolderController> pFolderController = CreateController();
	pFolderController->AddInsertOperation( szUniqueObjectName );
	if ( pFolderController->Redo( true, true, this ) )
	{
		GetViewManipulator()->ClearCache();
		//
		if ( HTREEITEM hItem = AddTreeItem( hParentItem, szNewFolderName, GDBO_FOLDER, 0 ) )
		{
			bCreateControls = true;
			EnsureVisible( hItem );
			RedrawWindow();
			bCreateControls = false;
			EditLabel( hItem );
		}
	}
}


void CTreeGDBBrowserBase::New( HTREEITEM hParentItem )
{
	if ( !GetViewManipulator() )
	{
		return;
	}
	if ( !GetViewManipulator() )
	{
		return;
	}
	CString strNewName;
	strNewName.LoadString( IDS_TREE_GDB_BROWSE_NEW_RESOURCE );
	const std::string szNewName = strNewName;
	std::string szObjectTypeName = GetObjectSet().szObjectTypeName;
	std::string szUniqueObjectName;
	GetUniqueName( hParentItem, szNewName, GDBO_OBJECT, &szUniqueObjectName );
	CStringManager::ExtendFileExtention( &szUniqueObjectName, ".xdb" );
	GetTreeItemName( hParentItem, &szUniqueObjectName );
	{
		bool bCanChangeObjectName = true;
		bool bNeedEdit = true;
		bool bNeedExport = false;
		// Проверяем возможность создания объекта
		{
			Singleton<IFolderCallback>()->ClearUndoData();
			if ( Singleton<IBuilderContainer>()->InsertObject( &szObjectTypeName,
																												 &szUniqueObjectName,
																												 false,
																												 &bCanChangeObjectName,
																												 &bNeedExport,
																												 &bNeedEdit ) )
			{
				if ( HTREEITEM hNewItem = GetTreeItem( szUniqueObjectName ) )
				{
					EnsureVisible( hNewItem );
					//
					if ( GetCount() == 1 )
					{
						bCreateControls = false;
						OnSelChanged( 0, 0 );
						bCreateControls = true;
					}
					RedrawWindow();
					bCreateControls = false;
					// Экспортируем вновь созданный объект
					if ( bNeedExport )
					{
						if ( CPtr<IManipulator> pObjectManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( szObjectTypeName, szObjectTypeName ) )
						{
							Singleton<IExporterContainer>()->StartExport( szObjectTypeName, FORCE_EXPORT, START_EXPORT_TOOLS, EXPORT_REFERENCES );
							Singleton<IExporterContainer>()->ExportObject( pObjectManipulator, szObjectTypeName, szUniqueObjectName, FORCE_EXPORT, EXPORT_REFERENCES );
							Singleton<IExporterContainer>()->FinishExport( szObjectTypeName, FORCE_EXPORT, FINISH_EXPORT_TOOLS, EXPORT_REFERENCES );
						}
					}
					//
					if ( CanAutoLoadAfterBuildingObject () && bNeedEdit )
					{
						DeselectAllItems();
						EnsureVisible( hNewItem );
						Select( hNewItem, TVGN_CARET );
						if ( CanLoad() )
						{
							Load();
						}
					}
					//
					RedrawWindow();
					/**
					// Если объект можно переименовывать - его необходимо позволить переименовать
					if ( bCanChangeObjectName )
					{
						EditLabel( hNewItem );
					}
					/**/
				}
				Singleton<IFolderCallback>()->ClearUndoData();
			}
			else
			{
				Singleton<IFolderCallback>()->UndoChanges();
			}
		}
	}
}


void CTreeGDBBrowserBase::NewFolder()
{
	if ( !GetViewManipulator() )
	{
		return;
	}
	if ( !GetViewManipulator() )
	{
		return;
	}
	HTREEITEM hParentItem = GetSelectedItem();
	while ( hParentItem )
	{
		const EGDBOType nType = GetTreeItemType( hParentItem );
		if ( nType == GDBO_FOLDER )
		{
			break;		
		}
		hParentItem = GetParentItem( hParentItem );
	}
	if ( hParentItem == 0 )
	{
		hParentItem = TVI_ROOT;
	}
	NewFolder( hParentItem );
}


void CTreeGDBBrowserBase::New()
{
	if ( !GetViewManipulator() )
	{
		return;
	}
	if ( !GetViewManipulator() )
	{
		return;
	}
	HTREEITEM hParentItem = GetSelectedItem();
	while ( hParentItem )
	{
		const EGDBOType nType = GetTreeItemType( hParentItem );
		if ( nType == GDBO_FOLDER )
		{
			break;		
		}
		hParentItem = GetParentItem( hParentItem );
	}
	if ( hParentItem == 0 )
	{
		hParentItem = TVI_ROOT;
	}
	New( hParentItem );
}


void CTreeGDBBrowserBase::NewFolderAtRoot()
{
	NewFolder( TVI_ROOT );
}


void CTreeGDBBrowserBase::NewAtRoot()
{
	New( TVI_ROOT );
}


bool CTreeGDBBrowserBase::CanNew()
{
	if ( nCreateTreeTimer != 0 )
	{
		return false;
	}
	return Singleton<IBuilderContainer>()->CanDefaultBuildObject( GetObjectSet().szObjectTypeName );
}


void CTreeGDBBrowserBase::Cut()
{
	if ( nCreateTreeTimer != 0 )
	{
		return;
	}
	if ( !GetViewManipulator() )
	{
		return;
	}
	FillClipboard( true );
	SetFocus();
}


void CTreeGDBBrowserBase::Copy()
{
	if ( !GetViewManipulator() )
	{
		return;
	}
	FillClipboard( false );
	SetFocus();
}


void CTreeGDBBrowserBase::Paste()
{
	if ( nCreateTreeTimer != 0 )
	{
		return;
	}
	if ( !GetViewManipulator() )
	{
		return;
	}

	HTREEITEM hTargetItem = GetSelectedItem();
	while ( ( hTargetItem != TVI_ROOT ) &&
					( hTargetItem != 0 ) &&
					( GetTreeItemType( hTargetItem ) == CTreeGDBBrowserBase::GDBO_OBJECT ) )
	{
		hTargetItem = GetParentItem( hTargetItem );
	}
	if ( !IsClipboardItem( hTargetItem ) )
	{
		CTreeGDBBrowserBase::CTreeOperationList pasteTreeOperations;
		const CTreeItemMap& rClipboardTreeItemMap = GetClipboard();
		for ( CTreeItemMap::const_iterator itClipboardTreeItem = rClipboardTreeItemMap.begin(); itClipboardTreeItem != rClipboardTreeItemMap.end(); ++itClipboardTreeItem )
		{
			HTREEITEM hClipboardItem = itClipboardTreeItem->second;
			CTreeGDBBrowserBase::CTreeOperationList::iterator posNewPasteTreeOperation = pasteTreeOperations.insert( pasteTreeOperations.end(), CTreeGDBBrowserBase::STreeOperation() );
			//
			if ( IsClipboardCut() )
			{
				posNewPasteTreeOperation->nType = CTreeGDBBrowserBase::STreeOperation::TYPE_RENAME;
			}
			else
			{
				posNewPasteTreeOperation->nType = CTreeGDBBrowserBase::STreeOperation::TYPE_COPY;
			}
			posNewPasteTreeOperation->hDestination = hTargetItem;
			posNewPasteTreeOperation->hSource = hClipboardItem;
		}
		ExecuteTreeOperations( pasteTreeOperations );
		RedrawWindow();
		//hLabelEditSortTimerItem = TVI_ROOT;
		//SetLabelEditSortTimer();
		UpdateSelectionManipulator( true );
	}
	SetFocus();
}


void CTreeGDBBrowserBase::Delete()
{
	if ( nCreateTreeTimer != 0 )
	{
		return;
	}
	if ( !GetViewManipulator() )
	{
		return;
	}
	CString strMessage;
	strMessage.LoadString( IDS_TREE_GDB_BROWSE_DELETE_OBJECTS_MESSAGE );
	if ( MessageBox( strMessage, Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 ) == IDYES )
	{
		CTreeGDBBrowserBase::CTreeOperationList deleteTreeOperations;
		HTREEITEM hSelectedItem = GetFirstSelectedItem();
		while ( hSelectedItem != 0 )
		{
			if ( IsTopSelection( hSelectedItem, 0 ) )
			{
				CTreeGDBBrowserBase::CTreeOperationList::iterator posNewDeleteTreeOperation = deleteTreeOperations.insert( deleteTreeOperations.end(), CTreeGDBBrowserBase::STreeOperation() );
				//
				posNewDeleteTreeOperation->nType = CTreeGDBBrowserBase::STreeOperation::TYPE_REMOVE;
				posNewDeleteTreeOperation->hDestination = hSelectedItem;
				posNewDeleteTreeOperation->hSource = 0;
			}
			hSelectedItem = GetNextSelectedItem( hSelectedItem );
		}
		ExecuteTreeOperations( deleteTreeOperations );
		UpdateSelectionManipulator( true );
		//hLabelEditSortTimerItem = TVI_ROOT;
		//SetLabelEditSortTimer();
	}
	SetFocus();
}


void CTreeGDBBrowserBase::Rename()
{
	if ( nCreateTreeTimer != 0 )
	{
		return;
	}
	if ( !GetViewManipulator() )
	{
		return;
	}
	HTREEITEM hFocusedItem = GetSelectedItem();
	EnsureVisible( hFocusedItem );
	/*CEdit* pwndEdit =*/
	EditLabel( hFocusedItem );
}


void CTreeGDBBrowserBase::Color()
{
	if ( nCreateTreeTimer != 0 )
	{
		return;
	}
	if ( !GetViewManipulator() )
	{
		return;
	}
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	
	COLORREF startColor = RGB( 0, 0, 0 );
	if ( GetSelectedCount() == 1 )
	{
		HTREEITEM hFocusedItem = GetSelectedItem();
		//
		std::string szName;
		GetTreeItemName( hFocusedItem, &szName );
		int nColor = 0;
		CManipulatorManager::GetValue( &nColor, GetViewManipulator(), szName );
		startColor = (int)nColor;
	}
	CColorDialog colorDialog( startColor, CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT, this );
	pUserData->colorList.resize( 16, 0xFFffFFff );
	colorDialog.m_cc.lpCustColors = &( pUserData->colorList[0] );
	if ( colorDialog.DoModal() == IDOK )
	{
		CTreeGDBBrowserBase::CTreeOperationList colorTreeOperations;
		HTREEITEM hSelectedItem = GetFirstSelectedItem();
		while ( hSelectedItem != 0 )
		{
			{
				CTreeGDBBrowserBase::CTreeOperationList::iterator posNewColorTreeOperation = colorTreeOperations.insert( colorTreeOperations.end(), CTreeGDBBrowserBase::STreeOperation() );
				//
				posNewColorTreeOperation->nType = CTreeGDBBrowserBase::STreeOperation::TYPE_COLOR;
				posNewColorTreeOperation->hDestination = hSelectedItem;
				posNewColorTreeOperation->hSource = 0;
				posNewColorTreeOperation->color = colorDialog.GetColor();
			}
			hSelectedItem = GetNextSelectedItem( hSelectedItem );
		}
		ExecuteTreeOperations( colorTreeOperations );
	}
	RedrawWindow();
}


HTREEITEM CTreeGDBBrowserBase::FindFirstItem( const std::string &rszSearch, HTREEITEM hStartItem )
{
	int nSearchSize = rszSearch.size();
	if ( nSearchSize != 0 )
	{
		std::string szSearch = rszSearch;
		NStr::ToLower( &szSearch );
		NStr::ReplaceAllChars( &szSearch, '/', '\\' );
		//
		HTREEITEM hItem = 0;
		HTREEITEM hParentItem = 0;
		if ( hStartItem == 0 )
		{
			hParentItem = TVI_ROOT;
			hItem = GetChildItem( hParentItem );
		}
		else
		{
			hItem = hStartItem;
			hParentItem = GetParentItem( hItem );
		}
		while ( true )
		{
			if ( hItem != 0 )
			{
				std::string szName;
				if ( GetTreeItemName( hItem, &szName ) )
				{
					if ( szName.size() >= nSearchSize	 )
					{
						NStr::ToLower( &szName );
						if ( szName.find( szSearch ) != std::string::npos )
						{
							if ( ( hStartItem == 0 ) || ( hItem != hStartItem  ) )
							{
								return hItem;
							}
						}
					}
				}
				if ( GetChildItem( hItem ) != 0 )
				{
					hParentItem = hItem;
					hItem = GetChildItem( hParentItem );
				}
				else
				{
					hItem = GetNextItem( hItem, TVGN_NEXT );
				}
			}
			while ( hItem == 0 )
			{
				if ( ( hParentItem != TVI_ROOT ) && ( hParentItem != 0 ) )
				{
					hItem = GetNextItem( hParentItem, TVGN_NEXT );
					hParentItem = GetParentItem( hParentItem );
				}
				else
				{
					break;
				}
			}
			if ( ( ( hParentItem == TVI_ROOT ) || ( hParentItem == 0 ) ) && ( hItem == 0 ) )
			{
				break;
			}
		}
	}
	return 0;
}


void CTreeGDBBrowserBase::Find()
{
	std::string szSearch = Singleton<IUserDataContainer>()->Get()->szLastSearchedText;
	CSearchObjectDialog searchObjectDialog;
	searchObjectDialog.SetText( szSearch );
	if ( searchObjectDialog.DoModal() == IDOK )
	{
		szSearch = searchObjectDialog.GetText();
		Singleton<IUserDataContainer>()->Get()->szLastSearchedText = szSearch;
		//
		HTREEITEM hFocusedItem = GetSelectedItem();
		if ( hFocusedItem == 0 )
		{
			hFocusedItem = TVI_ROOT;
		}
		HTREEITEM hItemToDisplay = FindFirstItem( szSearch, hFocusedItem );
		if ( hItemToDisplay == 0 )
		{
			hItemToDisplay = FindFirstItem( szSearch, 0 );
		}
		if ( hItemToDisplay != 0 )
		{
			if ( GetSelectedCount() == 1 )
			{
				if ( hFocusedItem != hItemToDisplay )
				{
					DeselectAllItems();
					Select( hItemToDisplay, TVGN_CARET );
					EnsureVisible( hItemToDisplay );
				}
			}
			SetFocus();
		}
		else
		{
			CString strProgramTitle;
			CString strMessagePattern;
			strProgramTitle.LoadString( AFX_IDS_APP_TITLE );
			strMessagePattern.LoadString( IDS_TREE_GDB_BROWSE_NO_OBJECT_FOUND_MESSAGE );
			const CString strMessage = fmt::sprintf( strMessagePattern.GetString(), GetObjectSet().szObjectTypeName.c_str() ).c_str();
			MessageBox( strMessage, strProgramTitle, MB_ICONINFORMATION | MB_OK | MB_DEFBUTTON1 );
		}
	}
	SetFocus();
}



void CTreeGDBBrowserBase::GotoID()
{
}


void CTreeGDBBrowserBase::LookupReferences()
{
	CWaitCursor	wait;
	const std::string &szObjectTypeName = GetObjectSet().szObjectTypeName;
	std::string szObjectName;
	GetTreeItemName( GetSelectedItem(), &szObjectName );
	//
	std::list<std::string> referenceObjectsList;
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	NI_VERIFY( pResourceManager, "Cannot find resource manager", return )
	CRefListWaitDialog xdbAskDlg( this );
	xdbAskDlg.SetData( &referenceObjectsList, szObjectTypeName, szObjectName, pResourceManager );
	xdbAskDlg.DoModal();
	if ( xdbAskDlg.IsComplete() )
	{
		CRefListDialog refListDialog( this );
		refListDialog.SetData( szObjectTypeName, szObjectName, &referenceObjectsList );
		refListDialog.DoModal();
	}
	SetFocus();
}


void CTreeGDBBrowserBase::Check( bool bCheckReferences )
{
	if ( !GetViewManipulator() )
	{
		return;
	}

	CWaitCursor	wait;
	
	Singleton<IFolderCallback>()->ClearUndoData();
	Singleton<IExporterContainer>()->StartCheck( GetObjectSet().szObjectTypeName, START_EXPORT_TOOLS, bCheckReferences );
	{
		CTreeGDBBrowserBase::CTreeOperationList exportTreeOperations;
		HTREEITEM hSelectedItem = GetFirstSelectedItem();
		while ( hSelectedItem != 0 )
		{
			if ( IsTopSelection( hSelectedItem, 0 ) )
			{
				CTreeGDBBrowserBase::CTreeOperationList::iterator posNewExportTreeOperation = exportTreeOperations.insert( exportTreeOperations.end(), CTreeGDBBrowserBase::STreeOperation() );
				//
				posNewExportTreeOperation->nType = CTreeGDBBrowserBase::STreeOperation::TYPE_CHECK;
				posNewExportTreeOperation->bExportReferences = bCheckReferences;
				posNewExportTreeOperation->hDestination = hSelectedItem;
				posNewExportTreeOperation->hSource = 0;
			}
			hSelectedItem = GetNextSelectedItem( hSelectedItem );
		}
		ExecuteTreeOperations( exportTreeOperations );
	}
	Singleton<IExporterContainer>()->FinishCheck( GetObjectSet().szObjectTypeName, FINISH_EXPORT_TOOLS, bCheckReferences );
	Singleton<IFolderCallback>()->ClearUndoData();
	//
	UpdateSelectionManipulator( true );
	SetFocus();
}


void CTreeGDBBrowserBase::Export( bool bForce, bool bExportReferences )
{
	if ( !GetViewManipulator() )
	{
		return;
	}

	CWaitCursor	wait;

	Singleton<IFolderCallback>()->ClearUndoData();
	//
	Singleton<IExporterContainer>()->StartExport( GetObjectSet().szObjectTypeName, bForce, START_EXPORT_TOOLS, bExportReferences );
	{
		CTreeGDBBrowserBase::CTreeOperationList exportTreeOperations;
		HTREEITEM hSelectedItem = GetFirstSelectedItem();
		while ( hSelectedItem != 0 )
		{
			if ( IsTopSelection( hSelectedItem, 0 ) )
			{
				CTreeGDBBrowserBase::CTreeOperationList::iterator posNewExportTreeOperation = exportTreeOperations.insert( exportTreeOperations.end(), CTreeGDBBrowserBase::STreeOperation() );
				//
				posNewExportTreeOperation->nType = ( bForce ? CTreeGDBBrowserBase::STreeOperation::TYPE_EXPORT_FORCE : CTreeGDBBrowserBase::STreeOperation::TYPE_EXPORT );
				posNewExportTreeOperation->bExportReferences = bExportReferences;
				posNewExportTreeOperation->hDestination = hSelectedItem;
				posNewExportTreeOperation->hSource = 0;
			}
			hSelectedItem = GetNextSelectedItem( hSelectedItem );
		}
		ExecuteTreeOperations( exportTreeOperations );
	}
	Singleton<IExporterContainer>()->FinishExport( GetObjectSet().szObjectTypeName, bForce, FINISH_EXPORT_TOOLS, bExportReferences );
	Singleton<IEditorContainer>()->ReloadActiveEditor( true );
	Singleton<IFolderCallback>()->ClearUndoData();
	//
	UpdateSelectionManipulator( true );
	SetFocus();
}


bool CTreeGDBBrowserBase::CanExport( bool bForce )
{
	if ( Singleton<IExporterContainer>()->CanExportObject( DEFAULT_EXPORTER_LABEL_TXT ) )
	{
		return ( GetSelectedItem() != 0 );
	}
	return false;
}


bool CTreeGDBBrowserBase::HandleCommand( unsigned nCommandID, uint32_t dwData )
{
	switch( nCommandID )
	{
		case ID_OBJECT_LOAD:
			Load();
			break;
		case ID_OBJECT_LOCATE:
			return false;
		case ID_OBJECT_NEW_FOLDER:
			if ( IsEditEnabled() )
			{
				NewFolder();
			}
			break;
		case ID_OBJECT_NEW:
			if ( IsEditEnabled() )
			{
				New();
			}
			break;
		case ID_OBJECT_NEW_FOLDER_AT_ROOT:
			if ( IsEditEnabled() )
			{
				NewFolderAtRoot();
			}
			break;
		case ID_OBJECT_NEW_AT_ROOT:
			if ( IsEditEnabled() )
			{
				NewAtRoot();
			}
			break;
		case ID_SELECTION_CUT:
			if ( IsEditEnabled() )
			{
				if ( nCreateTreeTimer == 0 )
				{
					if ( IsNotEditLabel() )
					{
						Cut();
					}
					else if ( CEdit *pEdit = GetEditControl() )
					{
						pEdit->Cut();
					}
				}
			}
			break;
		case ID_SELECTION_COPY:
			if ( IsNotEditLabel() )
			{
				Copy();
			}
			else if ( CEdit *pEdit = GetEditControl() )
			{
				pEdit->Copy();
			}
			break;
		case ID_SELECTION_PASTE:
			if ( IsEditEnabled() )
			{
				if ( nCreateTreeTimer == 0 )
				{
					if ( IsNotEditLabel() )
					{
						Paste();
					}
					else if ( CEdit *pEdit = GetEditControl() )
					{
						pEdit->Paste();
					}
				}
			}
			break;
		case ID_SELECTION_NEW:
			if ( IsEditEnabled() )
			{
				New();
			}
			break;
		case ID_SELECTION_CLEAR:
			if ( IsEditEnabled() )
			{
				if ( nCreateTreeTimer == 0 )
				{
					if ( IsNotEditLabel() )
					{
						Delete();
					}
					else if ( CEdit *pEdit = GetEditControl() )
					{
						int nStartChar = 0;
						int nEndChar = 0;
						pEdit->GetSel( nStartChar, nEndChar );
						if ( nStartChar != nEndChar )
						{
							pEdit->Clear();
						}
						else
						{
							pEdit->SendMessage( WM_KEYDOWN, VK_DELETE, 0 ); 
						}
					}
				}
			}
			break;
		case ID_SELECTION_RENAME:
			if ( IsEditEnabled() )
			{
				if ( nCreateTreeTimer == 0 )
				{
					Rename();
				}
			}
			break;
		case ID_SELECTION_SELECT_ALL:
			if ( IsNotEditLabel() )
			{
			}
			else if ( CEdit *pEdit = GetEditControl() )
			{
				pEdit->SetSel( 0, -1, false );
			}
			break;
		case ID_OBJECT_CHECK:
		case ID_OBJECT_EXPORT:
		case ID_OBJECT_EXPORT_NO_REF:
		case ID_OBJECT_EXPORT_FORCE:
		case ID_OBJECT_EXPORT_NO_REF_FORCE:
		{
			const bool bForceExport = ( ( nCommandID == ID_OBJECT_EXPORT_FORCE ) ||
																	( nCommandID == ID_OBJECT_EXPORT_NO_REF_FORCE ) );
			const bool bExportReferences = ( ( ( nCommandID == ID_OBJECT_CHECK ) ||
																				 ( nCommandID == ID_OBJECT_EXPORT ) ||
																				 ( nCommandID == ID_OBJECT_EXPORT_FORCE ) ) &&
																			 ( ( GetAsyncKeyState( VK_SHIFT ) & 0x8000 ) == 0 ) );
			CString strMessage;
			switch ( nCommandID )
			{
				case ID_OBJECT_CHECK:
					strMessage.LoadString( IDS_TREE_GDB_BROWSE_CHECK_WARNING );
					break;
				default:
				case ID_OBJECT_EXPORT:
					strMessage.LoadString( IDS_TREE_GDB_BROWSE_EXPORT_WARNING );
					break;
				case ID_OBJECT_EXPORT_NO_REF:
					strMessage.LoadString( IDS_TREE_GDB_BROWSE_EXPORT_NO_REF_WARNING );
					break;
				case ID_OBJECT_EXPORT_FORCE:
					strMessage.LoadString( IDS_TREE_GDB_BROWSE_EXPORT_FORCE_WARNING );
					break;
				case ID_OBJECT_EXPORT_NO_REF_FORCE:
					strMessage.LoadString( IDS_TREE_GDB_BROWSE_EXPORT_NO_REF_FORCE_WARNING );
					break;
			}
			if ( strMessage.IsEmpty() || ( MessageBox( strMessage, Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 ) == IDYES ) )
			{
				if ( nCommandID == ID_OBJECT_CHECK )
				{
					Check( bExportReferences );
				}
				else
				{
					Export( bForceExport, bExportReferences );
				}
			}
			break;
		}
		case ID_OBJECT_COLOR:
			if ( IsEditEnabled() )
			{
				Color();
			}
			break;
		case ID_SELECTION_FIND:
			Find();
			break;
		case ID_OBJECT_REF_LOOKUP:
			LookupReferences();
			break;
		case ID_SELECTION_PROPERTIES:
			return false;
		default:
			return false;
	}
	return true;
}


bool CTreeGDBBrowserBase::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CTreeGDBBrowserBase::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CTreeGDBBrowserBase::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID )
	{
		case ID_OBJECT_LOAD:
			( *pbEnable ) = ( CanLoad() && IsNotEditLabel() );
			( *pbCheck ) = false;
			return true;
		case ID_OBJECT_LOCATE:
			return false;
		case ID_OBJECT_NEW_FOLDER:
			( *pbEnable ) = ( IsEditEnabled() && CanNew() && IsNotEditLabel() );
			( *pbCheck ) = false;
			return true;
		case ID_OBJECT_NEW:
			( *pbEnable ) = ( IsEditEnabled() && CanNew() && IsNotEditLabel() );
			( *pbCheck ) = false;
			return true;
		case ID_OBJECT_NEW_FOLDER_AT_ROOT:
			( *pbEnable ) = ( IsEditEnabled() && CanNew() && IsNotEditLabel() );
			( *pbCheck ) = false;
			return true;
		case ID_OBJECT_NEW_AT_ROOT:
			( *pbEnable ) = ( IsEditEnabled() && CanNew() && IsNotEditLabel() );
			( *pbCheck ) = false;
			return true;
		case ID_SELECTION_CUT:
			( *pbEnable ) = false;
			( *pbCheck ) = false;
			if ( IsEditEnabled() )
			{
				if ( nCreateTreeTimer == 0 )
				{
					if ( IsNotEditLabel() )
					{
						( *pbEnable ) = ( GetSelectedCount() > 0 );
					}
					else if ( CEdit *pEdit = GetEditControl() )
					{
						int nStartChar = 0;
						int nEndChar = 0;
						pEdit->GetSel( nStartChar, nEndChar );
						( *pbEnable ) = ( nStartChar != nEndChar );
					}
				}
			}
			return true;
		case ID_SELECTION_COPY:
			( *pbEnable ) = false;
			( *pbCheck ) = false;
			if ( IsNotEditLabel() )
			{
				( *pbEnable ) = ( GetSelectedCount() > 0 );
			}
			else if ( CEdit *pEdit = GetEditControl() )
			{
				int nStartChar = 0;
				int nEndChar = 0;
				pEdit->GetSel( nStartChar, nEndChar );
				( *pbEnable ) = ( nStartChar != nEndChar );
			}
			return true;
		case ID_SELECTION_PASTE:
			( *pbEnable ) = false;
			( *pbCheck ) = false;
			if ( IsEditEnabled() )
			{
				if ( nCreateTreeTimer == 0 )
				{
					if ( IsNotEditLabel() )
					{
						if ( CanNew() && ( GetSelectedCount() == 1 ) && !IsClipboardEmpty() )
						{
							HTREEITEM hTargetItem = GetSelectedItem();
							while ( ( hTargetItem != TVI_ROOT ) &&
											( hTargetItem != 0 ) &&
											( GetTreeItemType( hTargetItem ) == CTreeGDBBrowserBase::GDBO_OBJECT ) )
							{
								hTargetItem = GetParentItem( hTargetItem );
							}
							( *pbEnable ) = !IsClipboardItem( hTargetItem );
						}
					}
					else if ( CEdit *pEdit = GetEditControl() )
					{
						( *pbEnable ) = ::IsClipboardFormatAvailable( CF_TEXT );
					}
				}
			}
			return true;
		case ID_SELECTION_NEW:
			( *pbEnable ) = ( IsEditEnabled() && CanNew() && IsNotEditLabel() );
			( *pbCheck ) = false;
		case ID_SELECTION_CLEAR:
			( *pbEnable ) = false;
			( *pbCheck ) = false;
			if ( IsEditEnabled() )
			{
				if ( nCreateTreeTimer == 0 )
				{
					if ( IsNotEditLabel() )
					{
						( *pbEnable ) = ( GetSelectedCount() > 0 );
					}
					else if ( CEdit *pEdit = GetEditControl() )
					{
						( *pbEnable ) = true;
					}
				}
			}
			return true;
		case ID_SELECTION_RENAME:
			( *pbEnable ) = ( IsEditEnabled() && CanNew() && ( GetSelectedCount() == 1 ) && IsNotEditLabel() );
			( *pbCheck ) = false;
			return true;
		case ID_SELECTION_SELECT_ALL:
			( *pbEnable ) = false;
			( *pbCheck ) = false;
			if ( !IsNotEditLabel() )
			{
				if ( CEdit *pEdit = GetEditControl() )
				{
					( *pbEnable ) = true;
				}
			}
			return false;		
		case ID_OBJECT_CHECK:
			( *pbEnable ) = ( CanExport( true ) && IsNotEditLabel() );
			( *pbCheck ) = false;
			return true;
		case ID_OBJECT_EXPORT:
		case ID_OBJECT_EXPORT_NO_REF:
			( *pbEnable ) = ( CanExport( false ) && IsNotEditLabel() );
			( *pbCheck ) = false;
			return true;
		case ID_OBJECT_EXPORT_FORCE:
		case ID_OBJECT_EXPORT_NO_REF_FORCE:
			( *pbEnable ) = ( CanExport( true ) && IsNotEditLabel() );
			( *pbCheck ) = false;
			return true;
		case ID_OBJECT_COLOR:
			( *pbEnable ) = ( IsEditEnabled() && ( GetSelectedCount() > 0 ) && IsNotEditLabel() );
			( *pbCheck ) = false;
			return true;
		case ID_SELECTION_FIND:
			( *pbEnable ) = ( IsNotEditLabel() );
			( *pbCheck ) = false;
			return true;
		case ID_OBJECT_REF_LOOKUP:
			( *pbEnable ) = ( ( GetSelectedCount() == 1 ) && IsNotEditLabel() && ( GetTreeItemType( GetSelectedItem() ) == GDBO_OBJECT ) );
			( *pbCheck ) = false;
			return true;
		case ID_SELECTION_PROPERTIES:
			return false;
		default:
			return false;
	}
	return false;
}


void CTreeGDBBrowserBase::InternalUndo( IController* pController ) 
{
	bool bResult = true;
	if ( CFolderController *pFolderController = dynamic_cast<CFolderController*>( pController ) )
	{
		std::string szTypeLabel;
		GetSaveHeaderWidthLabel( &szTypeLabel );
		if ( pFolderController->GetObjectSet().szObjectTypeName == szTypeLabel )
		{
		}
		else
		{
			for ( CFolderController::CUndoDataList::const_iterator itUndoData = pFolderController->undoDataList.begin(); itUndoData != pFolderController->undoDataList.end(); ++itUndoData )
			{
				switch ( itUndoData->eType )
				{
					///////////////////////////////////////////
					case CFolderController::SUndoData::TYPE_INSERT:
						break;
					///////////////////////////////////////////
					case CFolderController::SUndoData::TYPE_REMOVE:
						break;
					///////////////////////////////////////////
					case CFolderController::SUndoData::TYPE_COPY:
						break;
					///////////////////////////////////////////
					case CFolderController::SUndoData::TYPE_RENAME:
						break;
					///////////////////////////////////////////
					case CFolderController::SUndoData::TYPE_COLOR:
						break;
					///////////////////////////////////////////
					case CFolderController::SUndoData::TYPE_EXPAND:
						break;
					///////////////////////////////////////////
					default:
						bResult = false;
				}
			}
		}
	}
}


void CTreeGDBBrowserBase::InternalRedo( IController* pController )
{
	bool bResult = true;
	if ( CFolderController *pFolderController = dynamic_cast<CFolderController*>( pController ) )
	{
		std::string szTypeLabel;
		GetSaveHeaderWidthLabel( &szTypeLabel );
		if ( pFolderController->GetObjectSet().szObjectTypeName == szTypeLabel )
		{
			LoadHeaderWidth();
		}
		else
		{
			for ( CFolderController::CUndoDataList::const_iterator itUndoData = pFolderController->undoDataList.begin(); itUndoData != pFolderController->undoDataList.end(); ++itUndoData )
			{
				switch ( itUndoData->eType )
				{
					///////////////////////////////////////////
					case CFolderController::SUndoData::TYPE_INSERT:
					{
						if ( !itUndoData->szDestination.empty() )
						{
							GetViewManipulator()->ClearCache();
							const EGDBOType type = ( itUndoData->szDestination[itUndoData->szDestination.size() - 1] != PATH_SEPARATOR_CHAR ) ? GDBO_OBJECT : GDBO_FOLDER;
							const SIteratorDesc *pDesc = dynamic_cast<const SIteratorDesc*>( GetViewManipulator()->GetDesc( szIgnoreSelectionName ) );
							const HTREEITEM hItem = AddTreeItem( TVI_ROOT, itUndoData->szDestination, type, pDesc );
							bResult = ( hItem != 0 );
						}
						else
						{
							bResult = false;
						}
						break;
					}
					///////////////////////////////////////////
					case CFolderController::SUndoData::TYPE_REMOVE:
						if ( HTREEITEM hDestination = GetTreeItem( itUndoData->szDestination ) )
						{
							if ( GetChildItem( hDestination ) == 0 )
							{
								GetViewManipulator()->ClearCache();
								bResult = DeleteTreeItem( hDestination );
							}
						}
						break;
					///////////////////////////////////////////
					case CFolderController::SUndoData::TYPE_COPY:
					{
						if ( !itUndoData->szDestination.empty() )
						{
							GetViewManipulator()->ClearCache();
							const EGDBOType type = ( itUndoData->szDestination[itUndoData->szDestination.size() - 1] != PATH_SEPARATOR_CHAR ) ? GDBO_OBJECT : GDBO_FOLDER;
							const SIteratorDesc *pDesc = dynamic_cast<const SIteratorDesc*>( GetViewManipulator()->GetDesc( szIgnoreSelectionName ) );
							const HTREEITEM hItem = AddTreeItem( TVI_ROOT, itUndoData->szDestination, type, pDesc );
							bResult = ( hItem != 0 );
						}
						else
						{
							bResult = false;
						}
						break;
					}
					///////////////////////////////////////////
					case CFolderController::SUndoData::TYPE_RENAME:
					{
						if ( ( !itUndoData->szDestination.empty() ) &&
								 ( !itUndoData->szSource.empty() ) )
						{
							GetViewManipulator()->ClearCache();
							if ( (bool) itUndoData->newValue )
							{
								const EGDBOType type = ( itUndoData->szDestination[itUndoData->szDestination.size() - 1] != PATH_SEPARATOR_CHAR ) ? GDBO_OBJECT : GDBO_FOLDER;
								const SIteratorDesc *pDesc = dynamic_cast<const SIteratorDesc*>( GetViewManipulator()->GetDesc( szIgnoreSelectionName ) );
								const HTREEITEM hItem = AddTreeItem( TVI_ROOT, itUndoData->szDestination, type, pDesc );
								bResult = ( hItem != 0 );
								if ( HTREEITEM hSource = GetTreeItem( itUndoData->szSource ) )
								{
									if ( GetChildItem( hSource ) == 0 )
									{
										if ( !DeleteTreeItem( hSource ) )
										{
											bResult = false;
										}
									}
								}
							}
							else
							{
								HTREEITEM hSource = GetTreeItem( itUndoData->szSource );
								if ( hSource != 0 )
								{
									HTREEITEM hParent = GetParentItem( hSource );
									std::string szParentName;
									GetTreeItemName( hParent, &szParentName );
									const int nPos = szParentName.size();
									std::string szDestination = itUndoData->szDestination;
									if ( nPos > 0 )
									{
										szDestination = szDestination.substr( nPos );
									}
									if ( szDestination[szDestination.size() - 1] == PATH_SEPARATOR_CHAR )
									{
										szDestination = szDestination.substr( 0, szDestination.size() - 1 );
									}
									SetItemText( hSource, 0, szDestination.c_str() );
								}
								CSortTreeControl::RenameTreeItem( itUndoData->szDestination, itUndoData->szSource );
							}
						}
						else
						{
							bResult = false;
						}
						break;
					}
					///////////////////////////////////////////
					case CFolderController::SUndoData::TYPE_COLOR:
						if ( HTREEITEM hDestination = GetTreeItem( itUndoData->szDestination ) )
						{
							GetViewManipulator()->ClearCache();
							SetTreeItemColor( hDestination, (int)( itUndoData->newValue ) );
						}
						break;
					///////////////////////////////////////////
					case CFolderController::SUndoData::TYPE_EXPAND:
						bCreateControls = true;
						if ( const HTREEITEM hItem = GetTreeItem( itUndoData->szDestination ) )
						{
							Expand( hItem, (bool)( itUndoData->newValue ) ? TVE_EXPAND : TVE_COLLAPSE );
							RecalcScrollBars();
						}
						bCreateControls = false;
						break;
					///////////////////////////////////////////
					default:
						bResult = false;
				}
			}
		}
		//RedrawWindow();
	}
}


void CTreeGDBBrowserBase::SetViewManipulator( IManipulator* _pViewManipulator, const SObjectSet &rObjectSet, const std::string &rszTemporaryLabel )
{
	KillCreateTreeTimer();
	CDefaultView::SetViewManipulator( _pViewManipulator, rObjectSet, rszTemporaryLabel );
}


void CTreeGDBBrowserBase::Undo( IController* pController ) 
{
	if ( ( GetViewManipulator() != 0 ) && ( nCreateTreeTimer == 0 ) )
	{
		InternalUndo( pController );
	}
}


void CTreeGDBBrowserBase::Redo( IController* pController )
{
	if ( ( GetViewManipulator() != 0 ) && ( nCreateTreeTimer == 0 ) )
	{
		InternalRedo( pController );
	}
}

// basement storage


