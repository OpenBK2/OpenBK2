#include "stdafx.h"
#include "..\mapeditorlib\commandhandlerdefines.h"
#include "wmdefines.h"
#include "pc_constants.h"
#include "..\mapeditorlib\resourcedefines.h"
#include "pc_maintreecontrol.h"
#include "pc_dblinkdialog.h"

#include "PC_MainTreeControl.h"

#include "..\MapEditorLib\Tools_HashSet.h"
#include "..\MapEditorLib\PCIEMnemonics.h"
//#include "..\MapEditorLib\Tools_SysCodes.h"
#include "..\Image\ImageColor.h"
#include "..\Misc\StrProc.h"
#include "..\MapEditorLib\Interface_MainFrame.h"
#include "..\MapEditorLib\ControlAlgorithms.h"

// Controls
#include "PC_IntInputEditor.h"
#include "PC_IntSliderEditor.h"
#include "PC_IntComboEditor.h"
#include "PC_IntColorEditor.h"
#include "PC_FloatInputEditor.h"
#include "PC_FloatSliderEditor.h"
#include "PC_FloatComboEditor.h"
#include "PC_BoolComboEditor.h"
#include "PC_BoolCheckBoxEditor.h"
#include "PC_BoolSwitcherEditor.h"
#include "PC_StringInputEditor.h"
#include "PC_StringRefEditor.h"
#include "PC_StringFileRefEditor.h"
#include "PC_StringDirRefEditor.h"
#include "PC_StringBigInputEditor.h"
#include "PC_TextFileEditor.h"
#include "PC_ExTextFileEditor.h"
#include "PC_StringComboRefEditor.h"
#include "PC_BinaryBitFieldEditor.h"
#include "PC_StringNewRefEditor.h"
#include "PC_GUIDEditor.h"
#include "PC_Vec3ColorEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CALLBACK PCMainTreeControlCompareFunc( LPARAM lParam0, LPARAM lParam1, LPARAM lParamSort )
{
	HTREEITEM hItem0 = reinterpret_cast<HTREEITEM>( lParam0 );
	HTREEITEM hItem1 = reinterpret_cast<HTREEITEM>( lParam1 );
	CPCMainTreeControl *pPCMainTreeControl = reinterpret_cast<CPCMainTreeControl*>( lParamSort );
	//
	EPCIEType nType0 = pPCMainTreeControl->GetTreeItemType( hItem0 );
	EPCIEType nType1 = pPCMainTreeControl->GetTreeItemType( hItem1 );
	CString strText0 = pPCMainTreeControl->GetItemText( hItem0 );
	CString strText1 = pPCMainTreeControl->GetItemText( hItem1 );
	//
	if ( nType0 != nType1 )
	{
		if ( nType0 == PCIE_UNKNOWN )
		{
			return -1;
		}
		if ( nType1 == PCIE_UNKNOWN ) 
		{
			return 1;
		}
		if ( ( nType0 != PCIE_LIST ) &&
				 ( nType0 != PCIE_STRUCT ) &&
				 ( nType1 != PCIE_LIST ) &&
				 ( nType1 != PCIE_STRUCT ) )
		{
			return pPCMainTreeControl->SortItemText( strText0, strText1 );
		}
		if ( ( nType0 != PCIE_LIST ) &&
				 ( nType0 != PCIE_STRUCT ) )
		{
			return -1;
		}
		if ( ( nType1 != PCIE_LIST ) &&
				 ( nType1 != PCIE_STRUCT ) )
		{
			return 1;
		}
		if (  nType0 == PCIE_STRUCT )
		{
			return -1;
		}
		return 1;
	}
	else
	{
		return pPCMainTreeControl->SortItemText( strText0, strText1 );
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CPCMainTreeControl::SortItemText( const CString &rstrText0, const CString &rstrText1 )
{
	if ( rstrText0[rstrText0.GetLength() - 1] == ARRAY_NODE_END_CHAR )
	{
		int nIndex0 = 0;
		int nIndex1 = 0;
		sscanf( rstrText0, "[%d]", &nIndex0 );
		sscanf( rstrText1, "[%d]", &nIndex1 );
		if ( nIndex0 < nIndex1 )
		{
			return -1;
		}
		if ( nIndex0 > nIndex1 )
		{
			return 1;
		}
		return 0;
	}
	else
	{
		if ( IsIgnoreCase() )
		{
			string szText0 = rstrText0;
			string szText1 = rstrText1;
			NStr::ToLower( &szText0 );
			NStr::ToLower( &szText1 );
			return strcmpi( szText0.c_str(), szText1.c_str() );
		}
		else
		{
			return strcmpi( rstrText0, rstrText1 );
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CPCMainTreeControl, CSortTreeControl)
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_HSCROLL()
	ON_WM_LBUTTONDOWN()
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
	ON_WM_KEYDOWN()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_PC_ITEM_CHANGE, OnMessagePCItemChange)
	ON_NOTIFY_REFLECT( TVN_ITEMEXPANDED, OnItemExpanded )
	ON_NOTIFY_REFLECT( TVN_SELCHANGED, OnSelchangedTree )
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CPCMainTreeControl::CPCMainTreeControl( bool _bNeedTranslateAccelerators, bool _bModal )
	: bNeedTranslateAccelerators( _bNeedTranslateAccelerators ),
		bModal( _bModal ),
		pwndStatusStringWindow( 0 ),
		pActiveItemEditor( 0 ),
		bCreateControls( false ),
		nCreateTreeTimer( 0 ),
		newElementExpandMode( NEEM_USER_DEFINED ),
		hCreateTreeParentItem( 0 ),
		bCreateTree( true ),
		bAsync( true ),
		bShowHidden( false )
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CPCMainTreeControl::~CPCMainTreeControl()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::PickTextColors( LvPaintContext* pPC )
{
	if ( pPC )
	{
		CSortTreeControl::PickTextColors( pPC );
		if ( !bCreateControls && GetViewManipulator() )
		{
			TvPaintContext *pTvPC = dynamic_cast<TvPaintContext*>( pPC );
			if ( ( pTvPC->lvi.state & ( LVIS_SELECTED | LVIS_DROPHILITED | LVIS_CUT | LVIS_FOCUSED ) ) == 0 )
			{
				if ( pPC->lvi.iSubItem == 1 )
				{
					if ( ( GetTreeItemType( pTvPC->tvi.hItem ) == PCIE_INT_COLOR ) ||
							 ( GetTreeItemType( pTvPC->tvi.hItem ) == PCIE_INT_COLOR_WITH_ALPHA ) ||
							 ( GetTreeItemType( pTvPC->tvi.hItem ) == PCIE_VEC3_COLOR ) )
					{
						COLORREF color = RGB( 0, 0, 0 );
						if ( !GetTreeItemColor( pTvPC->tvi.hItem, &color ) )
						{
							string szName;
							GetTreeItemName( pTvPC->tvi.hItem, &szName );
							//
							CVariant value;
							if ( GetValue( szName, &value ) )
							{
								if ( value.GetType() == CVariant::VT_MULTIVARIANT )
								{
									return;
								}
								color = GetBGRColorFromARGBColor( (int)value );
								SetTreeItemColor( pTvPC->tvi.hItem, color );
							}
						}
						pTvPC->rgbText = color;
						pTvPC->rgbTextBkgnd = color;
						pTvPC->rgbItemBkgnd = color;
						pTvPC->rgbIconBkgnd = color;
						return;
					}
				}
				if ( ForceRelativeParam_ReadOnly( pTvPC->tvi.hItem, false ) )
				{
					pTvPC->rgbText = ::GetSysColor( COLOR_GRAYTEXT );
				}
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::OnItemExpanded( NMHDR* pNMHDR, LRESULT* pResult )
{
	if ( !bCreateControls )
	{
		bCreateControls = true;
		NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
		//
		HTREEITEM hItem = pNMTreeView->itemNew.hItem;
		bool bExpanded = ( pNMTreeView->itemNew.state & TVIS_EXPANDED ) > 0;

		string szName;
		GetTreeItemName( hItem, &szName );

		SUserData::SObjectTypeData::CExpandedPropertySet &rExpandedPropertySet = Singleton<IUserDataContainer>()->Get()->objectTypeDataMap[GetObjectSet().szObjectTypeName].expandedPropertySet;
		if ( bExpanded )
		{
			InsertHashSetElement( &( rExpandedPropertySet ), szName );
		}
		else
		{
			SUserData::SObjectTypeData::CExpandedPropertySet::iterator posExpandedProperty = rExpandedPropertySet.find( szName );
			if ( posExpandedProperty != rExpandedPropertySet.end() )
			{
				rExpandedPropertySet.erase( posExpandedProperty );
			}
		}
		CPtr<CObjectBaseController> pObjectController = CreateController();
		if ( pObjectController->AddExpandOperation( szName, bExpanded, GetViewManipulator() ) )
		{
			//GetViewManipulator()->ClearCache();
			pObjectController->Redo( false, true, this );
		}
		bCreateControls = false;
	}
	( *pResult ) = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::OnSelchangedTree( NMHDR* pNMHDR, LRESULT* pResult ) 
{
	if ( !bCreateControls )
	{
		bCreateControls = true;
		NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
		//
		HTREEITEM hItem = pNMTreeView->itemNew.hItem;

		string szName;
		GetTreeItemName( hItem, &szName );

		Singleton<IUserDataContainer>()->Get()->objectTypeDataMap[GetObjectSet().szObjectTypeName].szCurrentProperty = szName;
		UpdateStatusStringWindow();
		UpdateMultilineStringEditor();
		bCreateControls = false;
	}
	( *pResult ) = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::OnDestroy() 
{
	Singleton<ICommandHandlerContainer>()->Remove( CHID_PROPERTY_CONTROL, this );
	Singleton<ICommandHandlerContainer>()->Remove( CHID_SELECTION, this );
	//
	KillCreateTreeTimer();
	ClosePCItemEditor( false );
	RemoveViewManipulator();
	CSortTreeControl::OnDestroy();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::OnTimer( UINT nIDEvent ) 
{
  if ( nIDEvent == GetCreateTreeTimerID() )
	{
		OnCreateTreeTimer();
	}
	CWnd::OnTimer( nIDEvent );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::SetCreateTreeTimer()
{
	//DebugTrace( "CPCMainTreeControl::SetCreateTreeTimer()" );
  KillCreateTreeTimer();
  nCreateTreeTimer = SetTimer( GetCreateTreeTimerID(), GetCreateTreeTimerInterval(), 0 );
  if ( nCreateTreeTimer == 0 )
  {
    NI_ASSERT( 0, "CCWInput::SetCreateTreeTimer() Can't create timer" );
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::KillCreateTreeTimer()
{
  if ( nCreateTreeTimer != 0 )
	{
		KillTimer( nCreateTreeTimer );
	}
  nCreateTreeTimer = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
LRESULT CPCMainTreeControl::OnMessagePCItemChange( WPARAM wParam, LPARAM lParam )
{
	//DebugTrace( "CPCMainTreeControl::OnPCItemChange: wParam: 0x%X(%u), lParam: 0x%X", wParam, wParam, lParam );
	//
	WORD wPCItemCode = LOWORD( wParam );
	WORD wPCTargetEditor = HIWORD( wParam );
	if ( ( wPCItemCode == IC_KILL_FOCUS ) || ( wPCItemCode == IC_VALUE_CHANGED ) )
	{
		if ( wPCTargetEditor == PC_TEMPORARY_EDITOR )
		{
			if ( ( !GetViewManipulator() ) || ( !pActiveItemEditor ) )
			{
				return 0;
			}
			//DebugTrace( "Handled!" );
			if ( UpdateValueFromPCItemEditor( pActiveItemEditor ) )
			{
				UpdateMultilineStringEditor();
			}
		}
		else if ( wPCTargetEditor == PC_MULTILINE_STRING_EDITOR )
		{
			if ( !pMultilineStringEditor )
			{
				return 0;
			}
			//DebugTrace( "Handled!" );
			UpdateValueFromPCItemEditor( pMultilineStringEditor );
		}
	}
	//
	if ( wPCItemCode == IC_KILL_FOCUS )
	{
		if ( wPCTargetEditor == PC_TEMPORARY_EDITOR )
		{
			pActiveItemEditor = 0;
		}
	}
	
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EPCIEType CPCMainTreeControl::GetTreeItemType( HTREEITEM hItem )
{
	EPCIEType itemType = PCIE_UNKNOWN;
	int nImageIndex = 0;
	int nSelectedImageIndex = 0;
	if ( GetItemImage( hItem, nImageIndex, nSelectedImageIndex ) )
	{
		itemType = static_cast<EPCIEType>( nImageIndex );
	}
	return itemType;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSortTreeControl
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HTREEITEM CPCMainTreeControl::GetTreeItem( const string &rszName )
{
	HTREEITEM hItem = CSortTreeControl::GetTreeItem( rszName );
	if ( hItem != 0 )
	{
		return hItem;
	}
	else
	{
		HTREEITEM hParentItem = TVI_ROOT;
		string szAdditionalName = rszName;
		int nDividerPos = string::npos;
		do
		{
			nDividerPos = szAdditionalName.find( LEVEL_SEPARATOR_CHAR );
			const string szName = szAdditionalName.substr( 0, nDividerPos );
			
			HTREEITEM hItem = GetChildItem( hParentItem );
			while ( hItem != 0 )
			{
				string szText = GetItemText( hItem );
				if ( szText == szName )
				{
					break;
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
		while ( nDividerPos != string::npos );
		SetNameCache( rszName, hParentItem );
		return hParentItem;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CPCMainTreeControl::GetTreeItemName( HTREEITEM hItem, string *pszName )
{
	string szItemText = GetItemText( hItem );
	if ( pszName->empty() )
	{
		if ( pszName )
		{
			*pszName = szItemText;
		}
	}
	else
	{
		if ( pszName )
		{
			*pszName = szItemText + LEVEL_SEPARATOR_CHAR + *pszName;
		}
	}
	HTREEITEM hParentItem = GetParentItem( hItem );
	if ( hParentItem != 0 )
	{
		GetTreeItemName( hParentItem, pszName );
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::EnableEdit( bool bEnable )
{
	CSortTreeControl::EnableEdit( bEnable );
	if ( bEnable )
	{
		SetBkColor( ::GetSysColor( COLOR_WINDOW ) );
		SetIconBkColor( ::GetSysColor( COLOR_WINDOW ) );
		SetSelIconBkColor( ::GetSysColor( COLOR_WINDOW ) );
	}
	else
	{
		SetBkColor( ::GetSysColor( COLOR_3DFACE ) );
		SetIconBkColor( ::GetSysColor( COLOR_3DFACE ) );
		SetSelIconBkColor( ::GetSysColor( COLOR_3DFACE ) );
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CPCMainTreeControl::GetTreeItemEditorPlace( HTREEITEM hTreeItem, CTRect<int> *pRect )
{
	CRect clientRect;
	GetClientRect( &clientRect );
	CRect itemRect;
	if ( !GetItemRect( hTreeItem, &itemRect, false ) )
	{
		return false;
	}
	CRect verScrollBarRect( 0, 0, 0, 0 );
	//
	{
		CScrollBar *pwndScrollBar = GetScrollBarCtrl( SB_VERT );
		if ( pwndScrollBar && ::IsWindow( pwndScrollBar->GetSafeHwnd() ) )
		{
			//const int nPos = GetScrollPos( SB_VERT );
			int nMinPos = 0;
			int nMaxPos = 0;
			GetScrollRange( SB_VERT, &nMinPos, &nMaxPos );
			if ( pwndScrollBar->IsWindowVisible() && ( nMinPos != nMaxPos ) )
			{
				pwndScrollBar->GetWindowRect( &verScrollBarRect );
			}
		}
	}
	//
	itemRect.left = itemRect.right;
	itemRect.right = itemRect.left + GetColumnWidth( 1 ) + 1;
	
	itemRect.right -= 1;
	itemRect.bottom += 1;
	//
	if ( ( clientRect.Width() - verScrollBarRect.Width() ) < itemRect.right )
	{
		itemRect.right = clientRect.Width() - verScrollBarRect.Width();
	}
	if ( itemRect.right < itemRect.left )
	{
		itemRect.right = itemRect.left;
	}
	//
	if ( pRect )
	{
		pRect->left = itemRect.left;
		pRect->top = itemRect.top;
		pRect->right = itemRect.right;
		pRect->bottom = itemRect.bottom;
	}
	//DebugTrace( "GetTreeItemEditorPlace: ( %d, %d, %d, %d ), [%dx%d]\n", pRect->left, pRect->top, pRect->right, pRect->bottom, pRect->Width(), pRect->Height() );
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::CreateTree( HTREEITEM hParentItem, bool _bCreateTree, bool _bAsync )
{
	KillCreateTreeTimer();
	//
	bCreateTree = _bCreateTree;
	bAsync = _bAsync;
	if ( bCreateTree )
	{
		bool bNeedIterate = true;
		szCreateTreeParentName.clear();
		newElementExpandMode = NEEM_USER_DEFINED;
		pCreateTreeManipulatorIterator = 0;
		//
		if ( hParentItem != TVI_ROOT )
		{
			const int nChildCount = GetChildCount( hParentItem, false, false );
			for ( int nChildIndex = 0; nChildIndex < nChildCount; ++nChildIndex )
			{
				DeleteTreeItem( GetChildItem( hParentItem ) );
			}
			GetTreeItemName( hParentItem, &szCreateTreeParentName );
			//		
			if ( GetViewManipulator() == 0 )
			{
				EnableHeaderCtrl( GetCount() != 0, false );
				return;
			}
			else
			{
				EnableHeaderCtrl( true, false );
			}
			//
			if ( const SPropertyDesc *pDesc = dynamic_cast<const SPropertyDesc*>( GetViewManipulator()->GetDesc( szCreateTreeParentName ) ) )
			{
				SetPCItemView( hParentItem, pDesc );
			}
			EPCIEType nType = GetTreeItemType( hParentItem );
			bNeedIterate = !typePCIEMnemonics.IsLeaf( nType );
		}
		else
		{
			DeleteAllTreeItems();
			if ( GetViewManipulator() == 0 )
			{
				EnableHeaderCtrl( false, false );
				return;
			}
			else
			{
				EnableHeaderCtrl( true, false );
			}
		}	
		//DebugTrace( "CPCMainTreeControl::CreateTree(): %s", bCreateTree ? "Erase elements" : "Update values" );
		if ( bNeedIterate )
		{
			if ( pCreateTreeManipulatorIterator = GetViewManipulator()->Iterate( bShowHidden, ECT_NO_CACHE ) ) //bShowHidden ? ECT_CACHE_LOCAL : ECT_NO_CACHE ) )
			{
				// необходимо найти первое вхождение интересующей нас ветки
				if ( !szCreateTreeParentName.empty() )
				{
					while ( !pCreateTreeManipulatorIterator->IsEnd() )
 					{
						string szName;
						pCreateTreeManipulatorIterator->GetName( &szName );
						if ( ( szName != szCreateTreeParentName ) && ( szName.compare( 0, szCreateTreeParentName.size(), szCreateTreeParentName ) == 0 ) )
						{
							break;
						}
						pCreateTreeManipulatorIterator->Next();
					}
				}
				OnCreateTreeTimer();
			}
		}
		else
		{
			EnableHeaderCtrl( GetCount() != 0, false );
		}
	}
	else
	{
		hCreateTreeParentItem = hParentItem;
		OnCreateTreeTimer();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::OnCreateTreeTimer()
{
	//DebugTrace( "CPCMainTreeControl::OnCreateTreeTimer()" );
	KillCreateTreeTimer();
	if ( bCreateTree )
	{
		if ( pCreateTreeManipulatorIterator )
		{
			int nCount = 0;
			while ( ( !pCreateTreeManipulatorIterator->IsEnd() ) && ( nCount < GetCreateTreeTimerCount() ) )
 			{
				string szName;
				pCreateTreeManipulatorIterator->GetName( &szName );
				if ( ( !szName.empty() ) &&
						( !szCreateTreeParentName.empty() ) &&
						( ( szName == szCreateTreeParentName ) ||
							( szName.find( szCreateTreeParentName ) != 0 ) ) )
				{
					// больше нет ни одного нужного нам элемента
					pCreateTreeManipulatorIterator = 0;
					break;	
				}
				if ( const SPropertyDesc *pDesc = dynamic_cast<const SPropertyDesc*>( GetViewManipulator()->GetDesc( szName ) ) )
				{
					EPCIEType nType = typePCIEMnemonics.Get( pDesc, szName );
					//DebugTrace( "CreateTree: %s (%s:%d)", szName.c_str(), pDesc->szPropControlType.c_str(), nType );
					AddTreeItem( szName, nType, pDesc );
					//HTREEITEM hItem = AddTreeItemInternal( TVI_ROOT, szName, nType, pDesc );
				}
				//
				pCreateTreeManipulatorIterator->Next();
				++nCount;
			}
		}
		if ( ( pCreateTreeManipulatorIterator == 0 ) || ( pCreateTreeManipulatorIterator->IsEnd() ) )
		{
			EnableHeaderCtrl( GetCount() != 0, false );
			//
			szCreateTreeParentName.clear();
			newElementExpandMode = NEEM_USER_DEFINED;
			pCreateTreeManipulatorIterator = 0;
			//
			bCreateControls = true;
			//SortTree( TVI_ROOT, PCMainTreeControlCompareFunc, reinterpret_cast<LPARAM>( this ) );
			SelectPCItem( Singleton<IUserDataContainer>()->Get()->objectTypeDataMap[GetObjectSet().szObjectTypeName].szCurrentProperty );
			UpdateStatusStringWindow();
			UpdateMultilineStringEditor();
			bCreateControls = false;
			UpdatePCItemEditorPosition( 0 );
			//
			RedrawWindow();
		}
		else
		{
			RedrawWindow();
			if ( bAsync )
			{
				SetCreateTreeTimer();
			}
			else
			{
				OnCreateTreeTimer();
			}
		}
	}
	else
	{
		int nCount = 0;
		while ( ( hCreateTreeParentItem != 0 ) && ( nCount < GetCreateTreeTimerCount() ) )
		{
			if ( ( hCreateTreeParentItem != 0 ) &&
					 ( hCreateTreeParentItem != TVI_ROOT ) )
			{
				/**
				string szItemName;
				GetTreeItemName( hCreateTreeParentItem, &szItemName );
				DebugTrace( "CPCMainTreeControl::OnCreateTreeTimer(): %s", szItemName.c_str() );
				/**/
				SetPCItemView( hCreateTreeParentItem, 0 );
			}
			if ( hCreateTreeParentItem != 0 )
			{
				//
				if ( HTREEITEM hItem = GetChildItem( hCreateTreeParentItem ) )
				{
					hCreateTreeParentItem = hItem;
				}
				else if ( HTREEITEM hItem = GetNextSiblingItem( hCreateTreeParentItem ) )
				{
					hCreateTreeParentItem = hItem;
				}
				else
				{
					HTREEITEM hParentItem = GetParentItem( hCreateTreeParentItem );
					while ( hParentItem != 0 )
					{
						if ( HTREEITEM hItem = GetNextSiblingItem( hParentItem ) )
						{
							hParentItem = hItem;
							break;
						}
						else
						{
							hParentItem = GetParentItem( hParentItem );
						}
					}
					hCreateTreeParentItem = hParentItem;
				}
			}
			++nCount;
		}
		//
		if ( hCreateTreeParentItem != 0 )
		{
			if ( bAsync )
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
			EnableHeaderCtrl( GetCount() != 0, false );
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HTREEITEM CPCMainTreeControl::AddTreeItem( const string &rszName, EPCIEType nType, const SPropertyDesc *pDesc )
{
	//DebugTrace( "AddTreeItem: name:<%s>", rszName.c_str() );
	//
	int nDividerPos = rszName.find( LEVEL_SEPARATOR_CHAR );
	string szParentName = rszName.substr( 0, nDividerPos );
	while ( nDividerPos != string::npos ) 
	{
		if ( HTREEITEM hItem = GetTreeItem( szParentName ) )
		{
			const string szAdditionalName = rszName.substr( nDividerPos + 1 );
			return AddTreeItemInternal( hItem, szAdditionalName, nType, pDesc );
		}
		nDividerPos = szParentName.find( LEVEL_SEPARATOR_CHAR );
		szParentName = szParentName.substr( 0, nDividerPos );
	}
	return AddTreeItemInternal( TVI_ROOT, rszName, nType, pDesc );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CPCMainTreeControl::ItemMustBeExpand( HTREEITEM hItem )
{
	switch ( newElementExpandMode )
	{
		case NEEM_USER_DEFINED:
		{
			string szName;
			GetTreeItemName( hItem, &szName );
			const SUserData::SObjectTypeData::CExpandedPropertySet &rExpandedPropertySet = Singleton<IUserDataContainer>()->Get()->objectTypeDataMap[GetObjectSet().szObjectTypeName].expandedPropertySet;
			//DebugTrace( "rExpandedPropertySet: %d: %s", bExpand, szHashName.c_str() );
			return ( rExpandedPropertySet.find( szName ) != rExpandedPropertySet.end() );
		}
		case NEEM_ALWAYS_EXPAND:
			return true;
		case MEEM_ALWAYS_COLLAPSE:
			return false;
		default:
			return false;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HTREEITEM CPCMainTreeControl::AddTreeItemInternal( HTREEITEM hRootItem, const string &rszAdditionalName, EPCIEType nType, const SPropertyDesc *pDesc )
{
	HTREEITEM hAddedItem = 0;
	if ( rszAdditionalName.empty() )
	{
		return hAddedItem;
	}
	const int nDividerPos = rszAdditionalName.find( LEVEL_SEPARATOR_CHAR );
	const string szShortName = rszAdditionalName.substr( 0, nDividerPos );
	if ( nDividerPos != string::npos )
	{
		const string szAdditionalName = rszAdditionalName.substr( nDividerPos + 1 );
		HTREEITEM hItem = GetChildItem( hRootItem );
		while ( hItem )
		{
			const string szItemText = GetItemText( hItem );
			if ( szItemText == szShortName )
			{
				break;
			}
			hItem = GetNextSiblingItem( hItem );
		}
		if ( !hItem )
		{
			hItem = InsertTreeItem( szShortName.c_str(),
															PCIE_FOLDER,
															PCIE_FOLDER + PCIE_COUNT,
															hRootItem,
															TVI_LAST );
			if ( hItem )
			{
				const bool bExpand = ItemMustBeExpand( hItem );
				bCreateControls = true;
				SetItemState( hItem, bExpand ? TVIS_EXPANDED : 0, TVIS_EXPANDED );
				bCreateControls = false;
				//
			}
		}
		if ( hItem )
		{
			if ( !szAdditionalName.empty() )
			{
				hAddedItem = AddTreeItemInternal( hItem, szAdditionalName, nType, pDesc );
			}
			else
			{
				hAddedItem = hItem;
			}
		}
	}
	else
	{
		hAddedItem = InsertTreeItem( szShortName.c_str(),
																	nType,
																	nType + PCIE_COUNT,
																	hRootItem, TVI_LAST );
		if ( hAddedItem )
		{
			if ( !typePCIEMnemonics.IsLeaf( nType ) || ( nType == PCIE_VEC3_COLOR ) )
			{
				const bool bExpand = ItemMustBeExpand( hAddedItem );
				bCreateControls = true;
				SetItemState( hAddedItem, bExpand ? TVIS_EXPANDED : 0, TVIS_EXPANDED );
				bCreateControls = false;
			}
			SetPCItemView( hAddedItem, pDesc );
		}
	}
	return hAddedItem;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::SetPCItemView( HTREEITEM hItem, const SPropertyDesc *pDesc )
{
	//DebugTrace( "CPCMainTreeControl::SetPCItemView()" );
	string szName;
	GetTreeItemName( hItem, &szName );
	const EPCIEType nType = GetTreeItemType( hItem );
	//
	if ( pDesc == 0 )
	{
		pDesc = dynamic_cast<const SPropertyDesc*>( GetViewManipulator()->GetDesc( szName ) );
	}
	//
	if ( pDesc )
	{
		if ( szName[szName.size() - 1] != ARRAY_NODE_END_CHAR )
		{
			SetItemText( hItem, 2, pDesc->szDesc.c_str() );
		}
	}
	//
	CVariant value;
	if ( typePCIEMnemonics.IsLeaf( nType ) )
	{
		if ( GetValue( szName.c_str(), &value ) )
		{
			string szValue;
			GetPCItemStringValue( &szValue, value, "", nType, pDesc, false );
			SetItemText( hItem, 1, szValue.c_str() );
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CPCMainTreeControl::ForceRelativeParam_ReadOnly( HTREEITEM hItem, bool bDefaultValue )
{
	bool bReadOnly = bDefaultValue;
	if ( !GetTreeItemReadOnly( hItem, &bReadOnly ) )
	{
		string szName;
		const SPropertyDesc *pDesc = 0;
		HTREEITEM hParentItem = hItem;
		while ( hParentItem != 0 )
		{
			szName.clear();
			GetTreeItemName( hParentItem, &szName );
			pDesc = dynamic_cast<const SPropertyDesc*>( GetViewManipulator()->GetDesc( szName ) );
			if ( pDesc )
			{
				if ( pDesc->bReadOnly != bDefaultValue )
				{
					bReadOnly = pDesc->bReadOnly;
					break;
				}
			}
			hParentItem = GetParentItem( hParentItem );
		}
		//
		SetTreeItemReadOnly( hItem, bReadOnly );
	}
	return bReadOnly;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
string CPCMainTreeControl::ForceRelativeParam_StringParam( HTREEITEM hItem, const string &rszDefaultValue )
{
	string szName;
	const SPropertyDesc *pDesc = 0;
	while ( hItem != 0 )
	{
		szName.clear();
		GetTreeItemName( hItem, &szName );
		pDesc = dynamic_cast<const SPropertyDesc*>( GetViewManipulator()->GetDesc( szName ) );
		if ( pDesc )
		{
			if ( pDesc->szStringParam != rszDefaultValue )
			{
				return pDesc->szStringParam;
			}
		}
		hItem = GetParentItem( hItem );
	}
	return rszDefaultValue;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CPCMainTreeControl::ForceRelativeParam_IntParam( HTREEITEM hItem, int nDefaultValue )
{
	string szName;
	const SPropertyDesc *pDesc = 0;
	while ( hItem != 0 )
	{
		szName.clear();
		GetTreeItemName( hItem, &szName );
		pDesc = dynamic_cast<const SPropertyDesc*>( GetViewManipulator()->GetDesc( szName ) );
		if ( pDesc )
		{
			if ( pDesc->nIntParam != nDefaultValue )
			{
				return pDesc->nIntParam;
			}
		}
		hItem = GetParentItem( hItem );
	}
	return nDefaultValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPCItemEditor* CPCMainTreeControl::CreatePCItemEditor( HTREEITEM hItem )
{
	const EPCIEType nType = GetTreeItemType( hItem );
	//
	string szName;
	const SPropertyDesc *pDesc = 0;
	if ( typePCIEMnemonics.IsLeaf( nType ) )
	{
		GetTreeItemName( hItem, &szName );
		pDesc = dynamic_cast<const SPropertyDesc*>( GetViewManipulator()->GetDesc( szName ) );
	}
	const bool bReadOnly = ForceRelativeParam_ReadOnly( hItem, false );
	if ( pDesc == 0 )
	{
		return 0;
	}

	IPCItemEditor *pwndEditor = 0;
	if ( IsEditEnabled() && !bReadOnly )
	{
		switch ( nType )
		{
			case PCIE_INT_INPUT:
				pwndEditor = new CPCIntInputEditor();
				break;
			case PCIE_INT_SLIDER:
				pwndEditor = new CPCIntSliderEditor();
				break;
			case PCIE_INT_COMBO:
				pwndEditor = new CPCIntComboEditor();
				break;
			case PCIE_INT_COLOR:
			case PCIE_INT_COLOR_WITH_ALPHA:
				pwndEditor = new CPCIntColorEditor();
				break;
			case PCIE_FLOAT_INPUT:
				pwndEditor = new CPCFloatInputEditor();
				break;
			case PCIE_FLOAT_SLIDER:
				pwndEditor = new CPCFloatSliderEditor();
				break;
			case PCIE_FLOAT_COMBO:
				pwndEditor = new CPCFloatComboEditor();
				break;
			case PCIE_BOOL_COMBO:
				pwndEditor = new CPCBoolComboEditor();
				break;
			case PCIE_BOOL_CHECKBOX:
				pwndEditor = new CPCBoolCheckBoxEditor();
				break;
			case PCIE_BOOL_SWITCHER:
				pwndEditor = new CPCBoolSwitcherEditor();
				break;
			case PCIE_STRING_REF:
			case PCIE_STRING_MULTI_REF:
				pwndEditor = new CPCStringRefEditor();
				break;
			case PCIE_STRING_INPUT:
				pwndEditor = new CPCStringInputEditor();
				break;
			case PCIE_STRING_BIG_INPUT:
				pwndEditor = new CPCStringBigInputEditor();
				break;
			case PCIE_STRING_COMBO:
				pwndEditor = new CPCStringComboEditor();
				break;
			case PCIE_STRING_COMBO_REF:
			case PCIE_STRING_COMBO_MULTI_REF:
				pwndEditor = new CPCStringComboRefEditor();
				break;
			case PCIE_STRING_FILE_REF:
				pwndEditor = new CPCStringFileRefEditor( GetObjectSet().szObjectTypeName );
				break;
			case PCIE_STRING_DIR_REF:
				pwndEditor = new CPCStringDirRefEditor( GetObjectSet().szObjectTypeName );
				break;
			case PCIE_BINARY_BIT_FIELD:
				pwndEditor = new CPCBinaryBitFieldEditor();
				break;
			case PCIE_STRING_NEW_REF:
			case PCIE_STRING_NEW_MULTI_REF:
				pwndEditor = new CPCStringNewRefEditor();
				break;
			case PCIE_GUID:
				pwndEditor = new CPCGUIDEditor();
				break;
			case PCIE_TEXT_FILE:
				pwndEditor = new CPCTextFileEditor();
				break;
			case PCIE_NEW_TEXT_FILE:
				pwndEditor = new CPCExTextFileEditor();
				break;
			case PCIE_VEC3_COLOR:
				pwndEditor = new CPCVec3ColorEditor();
				break;
			default:
				break;
		}
	}
	else
	{
		switch ( nType )
		{
			case PCIE_INT_INPUT:
			case PCIE_INT_SLIDER:
			case PCIE_INT_COMBO:
				pwndEditor = new CPCIntInputEditor();
				break;
			case PCIE_INT_COLOR:
			case PCIE_INT_COLOR_WITH_ALPHA:
				pwndEditor = new CPCIntColorEditor();
				break;
			case PCIE_FLOAT_INPUT:
			case PCIE_FLOAT_SLIDER:
			case PCIE_FLOAT_COMBO:
				pwndEditor = new CPCFloatInputEditor();
				break;
			case PCIE_STRING_REF:
			case PCIE_STRING_MULTI_REF:
			case PCIE_STRING_COMBO_REF:
			case PCIE_STRING_COMBO_MULTI_REF:
			case PCIE_STRING_NEW_REF:
			case PCIE_STRING_NEW_MULTI_REF:
				pwndEditor = new CPCStringRefEditor();
				break;
			case PCIE_STRING_INPUT:
			case PCIE_STRING_COMBO:
				pwndEditor = new CPCStringInputEditor();
				break;
			case PCIE_STRING_BIG_INPUT:
				pwndEditor = new CPCStringBigInputEditor();
				break;
			case PCIE_STRING_FILE_REF:
				pwndEditor = new CPCStringFileRefEditor( GetObjectSet().szObjectTypeName );
				break;
			case PCIE_STRING_DIR_REF:
				pwndEditor = new CPCStringDirRefEditor( GetObjectSet().szObjectTypeName );
				break;
			case PCIE_BINARY_BIT_FIELD:
				pwndEditor = new CPCBinaryBitFieldEditor();
				break;
			case PCIE_GUID:
				pwndEditor = new CPCGUIDEditor();
				break;
			case PCIE_TEXT_FILE:
				pwndEditor = new CPCTextFileEditor();
				break;
			case PCIE_NEW_TEXT_FILE:
				pwndEditor = new CPCExTextFileEditor();
				break;
			case PCIE_VEC3_COLOR:
				pwndEditor = new CPCVec3ColorEditor();
				break;
			default:
				break;
		}
	}
	if ( pwndEditor != 0 )
	{
		if ( pwndEditor->CreateEditor( szName, nType, pDesc, IDC_PC_ACTIVE_ITEM_EDITOR, GetObjectSet(), this ) )
		{
			pwndEditor->EnableEdit( IsEditEnabled() && !bReadOnly );
			CVariant value;
			if ( GetValue( szName, &value ) )
			{
				if ( value.GetType() != CVariant::VT_MULTIVARIANT )
				{
					pwndEditor->SetValue( value );
					return pwndEditor;
				}
			}
		}
	}
	pwndEditor = 0;
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CPCMainTreeControl::GetValue( const string &rszName, CVariant *pVariant )
{
	if ( pVariant )
	{
		if ( const SPropertyDesc* pDesc = dynamic_cast<const SPropertyDesc*>( GetViewManipulator()->GetDesc( rszName ) ) )
		{
			if ( typePCIEMnemonics.Get( pDesc, rszName ) == PCIE_VEC3_COLOR )
			{
				int nColor = 0xFFffFFff;
				if( CPCVec3ColorEditor::GetColorValue( &nColor, GetViewManipulator(), rszName ) )
				{
					( *pVariant ) = nColor;
					return true;
				}
			}
			else
			{
				return GetViewManipulator()->GetValue( rszName, pVariant );
			}
		}
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CPCMainTreeControl::AddChangeOperation( const string &rszName, const CVariant &rValue, CObjectBaseController *pObjectController )
{
	if ( const SPropertyDesc* pDesc = dynamic_cast<const SPropertyDesc*>( GetViewManipulator()->GetDesc( rszName ) ) )
	{
		if ( typePCIEMnemonics.Get( pDesc, rszName ) == PCIE_VEC3_COLOR )
		{
			return CPCVec3ColorEditor::AddChangeOperation( rszName, (int)rValue, pObjectController, GetViewManipulator() );
		}
		else
		{
			return pObjectController->AddChangeOperation( rszName, rValue, GetViewManipulator() );
		}
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CPCMainTreeControl::UpdateValueFromPCItemEditor( IPCItemEditor *pPCItemEditor )
{
	if ( GetViewManipulator() && pPCItemEditor && pPCItemEditor->IsEditEnabled() )
	{
		bool bValueChanged = false;
		string szName = pPCItemEditor->GetName();
		if ( !pPCItemEditor->IsDefaultValue() )
		{
			CVariant newValue;
			CVariant oldValue;
			pPCItemEditor->GetValue( &newValue );
			if ( GetValue( szName.c_str(), &oldValue ) )
			{
				bValueChanged = ( !( oldValue == newValue ) );
				if ( bValueChanged )
				{
					bool bResult = true;
					GetViewManipulator()->CheckValue( szName.c_str(), newValue, &bResult );
					//
					if ( bResult )
					{
						CPtr<CObjectBaseController> pObjectController = CreateController();
						if ( AddChangeOperation( szName, newValue, pObjectController ) )
						{
							pObjectController->Redo( false, true, 0 );
							Singleton<IControllerContainer>()->Add( pObjectController );
							return true;
						}
					}
				}
			}
		}
		if ( !bValueChanged )
		{
			// Если объект - ссылка, то имя его могло поменяться, но значение не поменялось
			HTREEITEM hItem = GetTreeItem( szName );
			const EPCIEType nType = GetTreeItemType( hItem );
			if ( typePCIEMnemonics.IsRef( nType ) )
			{
				CVariant oldValue;
				if ( GetValue( szName.c_str(), &oldValue ) )
				{
					const SPropertyDesc* pDesc = dynamic_cast<const SPropertyDesc*>( GetViewManipulator()->GetDesc( szName ) );
					string szValue;
					GetPCItemStringValue( &szValue, oldValue, "", nType, pDesc, false );
					SetItemText( hItem, 1, szValue.c_str() );
				}
			}
		}
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::UpdatePCItemEditorPosition( HTREEITEM hItem )
{
	if ( pActiveItemEditor )
	{
		CTRect<int> pcItemRect( 0, 0, 0, 0 );
		if ( hItem == 0 )
		{
			hItem = GetTreeItem( pActiveItemEditor->GetName() );
		}
		if ( GetTreeItemEditorPlace( hItem, &pcItemRect ) )
		{
			pActiveItemEditor->PlaceEditor( pcItemRect );
		}
		else
		{
			pActiveItemEditor->PlaceEditor( CTRect<int>( 0, 0, 0, 0 ) );
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::ClosePCItemEditor( bool bAcceptChanges )
{
	if ( bAcceptChanges )
	{
		if ( UpdateValueFromPCItemEditor( pActiveItemEditor ) )
		{
			UpdateMultilineStringEditor();
		}
	}
	pActiveItemEditor = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CPCMainTreeControl::InsertPCNode( HTREEITEM hArrayItem, HTREEITEM hItem, int nNewIndex )
{
	//DebugTrace( "InsertNode0: 0x%X 0x%X %d", hArrayItem, hItem, nNewIndex );
	string szArrayName;
	string szNewNodeName;
	string szNewText;
	//	
	GetTreeItemName( hArrayItem, &szArrayName );
	const SPropertyDesc *pDesc = dynamic_cast<const SPropertyDesc*>( GetViewManipulator()->GetDesc( szArrayName ) );
	if ( !pDesc )
	{
		return false;
	}
	//
	szNewText = StrFmt( "[%d]", nNewIndex );
	szNewNodeName = szArrayName + LEVEL_SEPARATOR_CHAR + szNewText;
	EPCIEType nType = typePCIEMnemonics.Get( pDesc, true );
	//
	//DebugTrace( "InsertNode1: %s %d 0x%X 0x%X", szNewText.c_str(), nType, hArrayItem, hItem );
	if ( HTREEITEM hAddedItem = InsertTreeItem( szNewText.c_str(),
																							nType,
																							nType + PCIE_COUNT,
																							hArrayItem, hItem ) )
	{
		if ( !typePCIEMnemonics.IsLeaf( nType ) || ( nType == PCIE_VEC3_COLOR ) )
		{
			const bool bExpand = ItemMustBeExpand( hAddedItem );
			SetItemState( hAddedItem, bExpand ? TVIS_EXPANDED : 0, TVIS_EXPANDED );
		}
		SetPCItemView( hAddedItem, pDesc );

		// Увеличиваем нумерацию элементов на 1
		HTREEITEM hSiblingItem = GetNextSiblingItem( hAddedItem );
		++nNewIndex;
		while ( hSiblingItem )
		{
			szNewText = StrFmt( "[%d]", nNewIndex );
			SetItemText( hSiblingItem, szNewText.c_str() );
			hSiblingItem = GetNextSiblingItem( hSiblingItem );
			++nNewIndex;
		}
	
		// Обновляем дерево ( могли добавится новые поля )
		CreateTree( hAddedItem, true, false );
		return true;
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CPCMainTreeControl::DeletePCNode( HTREEITEM hArrayItem, HTREEITEM hItem, int nDeleteIndex )
{
	//DebugTrace( "DeleteNode0: 0x%X 0x%X %d", hArrayItem, hItem, nDeleteIndex );
	string szArrayName;
	string szNewText;
	//
	GetTreeItemName( hArrayItem, &szArrayName );
	if ( hItem == 0 )
	{
		nDeleteIndex = NODE_REMOVEALL_INDEX;
	}
	//
	//DebugTrace( "DeleteNode1: %s %d", szArrayName.c_str(), nDeleteIndex );
	if ( hItem != 0 )
	{
		// Уменьшаем нумерацию элементов на 1
		HTREEITEM hSiblingItem = GetNextSiblingItem( hItem );
		while ( hSiblingItem )
		{
			szNewText = StrFmt( "[%d]", nDeleteIndex );
			SetItemText( hSiblingItem, szNewText.c_str() );
			hSiblingItem = GetNextSiblingItem( hSiblingItem );
			++nDeleteIndex;
		}
		DeleteTreeItem( hItem );
	}
	else
	{
		const int nChildCount = GetChildCount( hArrayItem, false, false );
		for ( int nChildIndex = 0; nChildIndex < nChildCount; ++nChildIndex )
		{
			HTREEITEM hChildItem = GetChildItem( hArrayItem );
			DeleteTreeItem( hChildItem );
		}
	}
	UpdatePCItemEditorPosition( 0 );
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::ExpandPCItem()
{
	if ( HTREEITEM hItem = GetSelectedItem() )
	{
		ExpandCompletely( hItem, true );
		UpdatePCItemEditorPosition( 0 );
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::ExpandAllPCItems()
{
	ExpandCompletely( TVI_ROOT, true );
	UpdatePCItemEditorPosition( 0 );
	newElementExpandMode = NEEM_ALWAYS_EXPAND;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::CollapsePCItem()
{
	if ( HTREEITEM hItem = GetSelectedItem() )
	{
		CollapseCompletely( hItem, true );
		UpdatePCItemEditorPosition( 0 );
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::CollapseAllPCItems()
{
	if ( HTREEITEM hItem = GetSelectedItem() )
	{
		hItem = GetRootItem( hItem );
		DeselectAllItems();
		Select( hItem, TVGN_CARET );
		EnsureVisible( hItem );
	}
	HTREEITEM hItem	= GetChildItem( TVI_ROOT );
	while ( hItem )
	{
		CollapseCompletely( hItem, true );
		hItem = GetNextSiblingItem( hItem );
	}
	UpdatePCItemEditorPosition( 0 );
	newElementExpandMode = MEEM_ALWAYS_COLLAPSE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::SetOptimalWidth()
{
	UpdatePCItemEditorPosition( 0 );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::Refresh()
{
	if ( nCreateTreeTimer != 0 )
	{
		return;
	}
	ClosePCItemEditor( true );
	//
	if ( HTREEITEM hItem = GetSelectedItem() )
	{
		CreateTree( hItem, true, true );
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::AddNode()
{
	if ( nCreateTreeTimer != 0 )
	{
		return;
	}
	// добавляем элемент к дереву и манипулятору
	if ( HTREEITEM hArrayItem = GetSelectedItem() )
	{
		string szName;
		GetTreeItemName( hArrayItem, &szName );
		//заполняем UNDO структуру
		CPtr<CObjectBaseController> pObjectController = CreateController();
		if ( pObjectController->AddInsertOperation( szName, NODE_ADD_INDEX, GetViewManipulator() ) )
		{
			pObjectController->Redo( false, true, 0 );
			Singleton<IControllerContainer>()->Add( pObjectController );
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::DeleteAllNodes()
{
	if ( nCreateTreeTimer != 0 )
	{
		return;
	}
	// удаляем элементы из дерева и манипулятора
	if ( HTREEITEM hArrayItem = GetSelectedItem() )
	{
		CString strMessage;
		strMessage.LoadString( IDS_PC_DELETE_ALL_MESSAGE );
		if ( MessageBox( strMessage, Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 ) == IDYES )
		{
			string szName;
			GetTreeItemName( hArrayItem, &szName );
			//заполняем UNDO структуру
			CPtr<CObjectBaseController> pObjectController = CreateController();
			if ( pObjectController->AddRemoveOperation( szName, NODE_REMOVEALL_INDEX, GetViewManipulator() ) )
			{
				pObjectController->Redo( false, true, 0 );
				Singleton<IControllerContainer>()->Add( pObjectController );
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::InsertNode()
{
	if ( nCreateTreeTimer != 0 )
	{
		return;
	}
	if ( HTREEITEM hItem = GetSelectedItem() )
	{
		HTREEITEM hArrayItem = GetParentItem( hItem );
		string szName;
		GetTreeItemName( hArrayItem, &szName );
		CString strText = GetItemText( hItem );
		int nNewIndex = 0;
		if ( sscanf( strText, "[%d]", &nNewIndex ) == 1 )
		{
			hItem = GetPrevSiblingItem( hItem );
			if ( hItem == 0 )
			{
				hItem = TVI_FIRST;
			}
			CPtr<CObjectBaseController> pObjectController = CreateController();
			if ( pObjectController->AddInsertOperation( szName, nNewIndex, GetViewManipulator() ) )
			{
				pObjectController->Redo( false, true, 0 );
				Singleton<IControllerContainer>()->Add( pObjectController );
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::DeleteNode()
{
	if ( nCreateTreeTimer != 0 )
	{
		return;
	}
	if ( HTREEITEM hItem = GetSelectedItem() )
	{
		HTREEITEM hArrayItem = GetParentItem( hItem );
		string szName;
		GetTreeItemName( hArrayItem, &szName );
		CString strText = GetItemText( hItem );
		int nNewIndex = 0;
		if ( sscanf( strText, "[%d]", &nNewIndex ) == 1 )
		{
			CString strMessage;
			strMessage.LoadString( IDS_PC_DELETE_MESSAGE );
			if ( MessageBox( strMessage, Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 ) == IDYES )
			{
				CPtr<CObjectBaseController> pObjectController = CreateController();
				if ( pObjectController->AddRemoveOperation( szName, nNewIndex, GetViewManipulator() ) )
				{
					pObjectController->Redo( false, true, 0 );
					Singleton<IControllerContainer>()->Add( pObjectController );
				}
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::ShowHidden()
{
	bShowHidden = !bShowHidden;
	CreateTree( TVI_ROOT, true, true );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CPCMainTreeControl::UpdateValue( const string &rszName )
{
	if ( !GetViewManipulator() )
	{
		return false;
	}
	if ( HTREEITEM hItem = GetTreeItem( rszName ) )
	{
		RemoveTreeItemColorFromCache( hItem );
		RemoveTreeItemReadOnlyFromCache( hItem ); 
		SetPCItemView( hItem, 0 );
		hItem = GetParentItem( hItem );
		if ( GetTreeItemType( hItem ) == PCIE_VEC3_COLOR )
		{
			RemoveTreeItemColorFromCache( hItem );
			RemoveTreeItemReadOnlyFromCache( hItem ); 
			SetPCItemView( hItem, 0 );
		}
		return true;
	}
	//
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CPCMainTreeControl::SelectPCItem( const string &rszName )
{
	if ( !GetViewManipulator() )
	{
		return false;
	}
	//
	if ( HTREEITEM hItem = GetTreeItem( rszName ) )
	{
		DeselectAllItems();
		Select( hItem, TVGN_CARET );
		EnsureVisible( hItem );
		return true;
	}
	//
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CPCMainTreeControl::GetSelectedPCItemName( string *pszName )
{
	if ( !GetViewManipulator() )
	{
		return false;
	}
	//
	if ( HTREEITEM hItem = GetSelectedItem() )
	{
		if ( pszName )
		{
			pszName->clear();
			GetTreeItemName( hItem, pszName );
			return true;
		}
	}
	//
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CPCMainTreeControl::GetSelectedPCItemDescription( string *pszName )
{
	if ( !GetViewManipulator() )
	{
		return false;
	}
	//
	if ( HTREEITEM hItem = GetSelectedItem() )
	{
		if ( pszName )
		{
			pszName->clear();
			if ( GetObjectSet().objectNameSet.size() == 1 )
			{
				( *pszName ) =  GetObjectSet().objectNameSet.begin()->first.ToString();
			}
			/**
			string szObjectID;
			string szObjectPropertyName;
			if ( GetObjectSet().objectNameSet.size() == 1 )
			{
				if ( szGUID.empty() )
				{
					szObjectID =  StrFmt( "%s:%d ", GetObjectSet().szObjectTypeName.c_str(), GetObjectSet().objectNameSet.begin()->first );
				}
				else
				{
					szObjectID =  StrFmt( "%s:%d {%s} ", GetObjectSet().szObjectTypeName.c_str(), GetObjectSet().objectNameSet.begin()->first, szGUID.c_str() );
				}
			}
			GetTreeItemName( hItem, &szObjectPropertyName );
			const string szDescription = GetItemText( hItem, 2 );
			if ( !szDescription.empty() )
			{
				( *pszName ) =  StrFmt( "%s%s (%s)", szObjectID.c_str(), szObjectPropertyName.c_str(), szDescription.c_str() );
			}
			else
			{
				( *pszName ) = StrFmt( "%s%s", szObjectID.c_str(), szObjectPropertyName.c_str() );
			}	
			/**/
			return true;
		}
	}
	//
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar ) 
{
	if ( pScrollBar == GetScrollBarCtrl( SB_HORZ ) )
	{
		CSortTreeControl::OnHScroll( nSBCode, nPos, pScrollBar );
		UpdatePCItemEditorPosition( 0 );
	}
	else
	{
		if ( pActiveItemEditor )
		{
			pActiveItemEditor->ProcessMessage( WM_PC_EDITOR_SLIDER_CHANGE, MAKEWPARAM( ESC_POSITION_CHANGED, 0 ), reinterpret_cast<LPARAM>( pScrollBar ) );
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar ) 
{
	if ( pScrollBar == GetScrollBarCtrl( SB_VERT ) )
	{
		CSortTreeControl::OnVScroll( nSBCode, nPos, pScrollBar );
		UpdatePCItemEditorPosition( 0 );
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CPCMainTreeControl::OnMouseWheel( UINT nFlags, short zDelta, CPoint pt ) 
{
	bool bresult = CSortTreeControl::OnMouseWheel( nFlags, zDelta, pt );
	UpdatePCItemEditorPosition( 0 );
	return bresult;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::OnSize( UINT nType, int cx, int cy ) 
{
	CSortTreeControl::OnSize( nType, cx, cy );
	UpdatePCItemEditorPosition( 0 );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::OnLButtonDown( UINT nFlags, CPoint point ) 
{
	CSortTreeControl::OnLButtonDown( nFlags, point );

	if ( !GetViewManipulator() )
	{
		return;
	}
	//
	if ( nCreateTreeTimer != 0 )
	{
		return;
	}
	if ( HTREEITEM hItem = GetSelectedItem() )
	{
		CTRect<int> pcItemRect( 0, 0, 0, 0 );
		GetTreeItemEditorPlace( hItem, &pcItemRect );
		if ( pcItemRect.IsInside( point.x, point.y ) )
		{
			if ( pActiveItemEditor = CreatePCItemEditor( hItem ) )
			{
				UpdatePCItemEditorPosition( hItem );
				//
				CDialog *pwndParentDialog = dynamic_cast<CDialog*>( GetParent() );
				if ( pwndParentDialog )
				{
					if ( !pActiveItemEditor->ActivateEditor( pwndParentDialog ) )
					{
						pActiveItemEditor = 0;	
					}
				}
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags ) 
{
	if ( nChar != VK_ESCAPE )
	{
		CSortTreeControl::OnKeyDown( nChar, nRepCnt, nFlags );
	}
	if ( !GetViewManipulator() )
	{
		return;
	}
	if ( HTREEITEM hItem = GetSelectedItem() )
	{
		if ( ( nChar == VK_RETURN ) || ( nChar == VK_SPACE ) )
		{
			if ( pActiveItemEditor = CreatePCItemEditor( hItem ) )
			{
				UpdatePCItemEditorPosition( hItem );
				//
				CDialog *pwndParentDialog = dynamic_cast<CDialog*>( GetParent() );
				if ( pwndParentDialog )
				{
					if ( !pActiveItemEditor->ActivateEditor( pwndParentDialog ) )
					{
						pActiveItemEditor = 0;	
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::OnContextMenu( CWnd *pwnd, CPoint point )
{
	CSortTreeControl::OnContextMenu( pwnd, point );

	CMenu mainPopupMenu;
	mainPopupMenu.LoadMenu( IDM_PC_CONTEXT_MENU );
	CMenu *pMenu = 0;
	bool bEnable = false;
	bool bChecked = false;
	if ( UpdateCommand( ID_PC_ADD_NODE, &bEnable, &bChecked ) && bEnable ) // array
	{
		if ( UpdateCommand( ID_PC_INSERT_NODE, &bEnable, &bChecked ) && bEnable ) // array_node
		{
			pMenu = mainPopupMenu.GetSubMenu( PCCMN_ARRAY_ARRAY );
		}
		else
		{
			pMenu = mainPopupMenu.GetSubMenu( PCCMN_ARRAY );
		}
	}
	else
	{
		if ( UpdateCommand( ID_PC_INSERT_NODE, &bEnable, &bChecked ) && bEnable ) // array_node
		{
			if ( ( UpdateCommand( ID_PC_EXPAND, &bEnable, &bChecked ) && bEnable ) || ( UpdateCommand( ID_PC_COLLAPSE, &bEnable, &bChecked ) && bEnable ) ) //multi_node
			{
				pMenu = mainPopupMenu.GetSubMenu( PCCMN_ARRAY_MULTI_NODE );
			}
			else
			{
				pMenu = mainPopupMenu.GetSubMenu( PCCMN_ARRAY_NODE );
			}
		}
		else
		{
			if ( ( UpdateCommand( ID_PC_EXPAND, &bEnable, &bChecked ) && bEnable ) || ( UpdateCommand( ID_PC_COLLAPSE, &bEnable, &bChecked ) && bEnable ) ) //multi_node
			{
				pMenu = mainPopupMenu.GetSubMenu( PCCMN_MULTI_NODE );
			}
			else
			{
				pMenu = mainPopupMenu.GetSubMenu( PCCMN_NODE );
			}
		}
	}
	if ( pMenu )
	{
		pMenu->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x, point.y, Singleton<IMainFrameContainer>()->GetSECWorkbook(), 0 );
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
	}
	mainPopupMenu.DestroyMenu();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//CWMMnemonicCodes mnemonicCodes;
LRESULT CPCMainTreeControl::WindowProc( UINT message, WPARAM wParam, LPARAM lParam ) 
{
	//DebugTrace( "Message: %s, wParam: 0x%X(%u), lParam: 0x%X\n", mnemonicCodes.Get( message ).c_str(), wParam, wParam, lParam );
	if ( bNeedTranslateAccelerators && ( message == WM_KEYDOWN ) && ( ::AfxGetMainWnd() != 0 ) )
	{
		if ( NCA::TranslateAccelerators( bModal, message, wParam, lParam ) )
		{
			return 0;
		}
	}
	return CSortTreeControl::WindowProc( message, wParam, lParam );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::OnSetFocus( CWnd* pOldWnd )
{
	CSortTreeControl::OnSetFocus( pOldWnd );
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_PROPERTY_CONTROL, this );
	Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, this );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::OnKillFocus( CWnd* pNewWnd )
{
	CSortTreeControl::OnKillFocus( pNewWnd );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::SetMultilineStringEditor( IPCItemEditor* _pMultilineStringEditor )
{
	pMultilineStringEditor = _pMultilineStringEditor;
	UpdateMultilineStringEditor();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::UpdateMultilineStringEditor()
{
	if ( pMultilineStringEditor != 0 )
	{
		if ( HTREEITEM hItem = GetSelectedItem() )
		{
			const EPCIEType nType = GetTreeItemType( hItem );
			
			string szName;
			const SPropertyDesc *pDesc = 0;
			if ( typePCIEMnemonics.IsLeaf( nType ) )
			{
				GetTreeItemName( hItem, &szName );
				pDesc = dynamic_cast<const SPropertyDesc*>( GetViewManipulator()->GetDesc( szName ) );
			}
			pMultilineStringEditor->CreateEditor( szName, nType, pDesc, 0, GetObjectSet(), this );
			//
			const bool bReadOnly = typePCIEMnemonics.IsPointer( nType ) ||
														 ( pDesc == 0 ) ||
														 ForceRelativeParam_ReadOnly( hItem, false );
			//pMultilineStringEditor->ProcessMessage( WM_ENABLE, bReadOnly ? 0 : 1, 0 );
			pMultilineStringEditor->EnableEdit( IsEditEnabled() && !bReadOnly );
			//
			CVariant value;
			if ( pDesc )
			{
				GetValue( szName, &value );
			}
			//
			pMultilineStringEditor->SetValue( value );
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::SetStatusStringWindow( CWnd* _pwndStatusStringWindow )
{
	pwndStatusStringWindow = _pwndStatusStringWindow;
	UpdateStatusStringWindow();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::UpdateStatusStringWindow()
{
	if ( ( pwndStatusStringWindow != 0 ) && ::IsWindow( pwndStatusStringWindow->m_hWnd ) )
	{
		string szItemName;
		GetSelectedPCItemDescription( &szItemName );
		pwndStatusStringWindow->SetWindowText( szItemName.c_str() );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::CopySelection()
{
	if ( SUserData *pUserData = Singleton<IUserDataContainer>()->Get() )
	{
		NCA::CHTREEITEMList hTreeItemList;
		int nCount = NCA::CreateSelection( &hTreeItemList, NCA::ST_COMPLETE_SELECT, 0, 0, this );
		NCA::FillWindowsClipboard( hTreeItemList, "", "", "", "", ".", "\t", this );
		//
		pUserData->pcSelection.Clear();
		for ( NCA::CHTREEITEMList::const_iterator itTreeItem = hTreeItemList.begin(); itTreeItem != hTreeItemList.end(); ++itTreeItem )
		{
			const EPCIEType nType = GetTreeItemType( *itTreeItem );
			if ( typePCIEMnemonics.IsLeaf( nType ) )
			{
				string szName;
				GetTreeItemName( *itTreeItem, &szName );
				//		
				SUserData::SPCSelectionData selectionData;
				if ( GetValue( szName.c_str(), &( selectionData.value ) ) )
				{
					pUserData->pcSelection.Insert( szName, selectionData );
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::PasteSelection()
{
	if ( SUserData *pUserData = Singleton<IUserDataContainer>()->Get() )
	{
		if ( !pUserData->pcSelection.IsEmpty() )
		{
			ClosePCItemEditor( false );
			if ( CPtr<CObjectBaseController> pObjectController = CreateController() )
			{
				const SUserData::CPCSelection::CControlSelectionDataMap &rSelectionDataMap = pUserData->pcSelection.Get();
				for ( SUserData::CPCSelection::CControlSelectionDataMap::const_iterator itSelectionData = rSelectionDataMap.begin(); itSelectionData != rSelectionDataMap.end(); ++itSelectionData )
				{
					if ( HTREEITEM hTreeItem = GetTreeItem( itSelectionData->first ) )
					{
						AddChangeOperation( itSelectionData->first, itSelectionData->second.data.value, pObjectController );
					}
				}
				//
				pObjectController->Redo( false, true, 0 );
				Singleton<IControllerContainer>()->Add( pObjectController );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::SelectAll()
{
	NCA::SelectAll( 0, 0, this );
	RedrawWindow();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CPCMainTreeControl::HandleCommand( UINT nCommandID, DWORD dwData )
{
	if ( !GetViewManipulator() )
	{
		return false;
	}
	switch( nCommandID )
	{
		case ID_SELECTION_CUT:
			break;
		case ID_SELECTION_COPY:
			CopySelection();
			break;
		case ID_SELECTION_PASTE:
			PasteSelection();
			break;
		case ID_SELECTION_NEW:
			if ( ( nCreateTreeTimer != 0 ) || ( !IsEditEnabled() ) )
			{
				break;
			}
			if ( HTREEITEM hItem = GetSelectedItem() )
			{
				if ( GetTreeItemType( hItem ) == PCIE_LIST )
				{
					AddNode();
				}
				else if ( HTREEITEM hParentItem = GetParentItem( hItem ) )
				{
					if ( GetTreeItemType( hParentItem ) == PCIE_LIST )
					{
						InsertNode();
					}
				}
				return true;
			}
			break;
		case ID_SELECTION_CLEAR:
			if ( ( nCreateTreeTimer != 0 ) || ( !IsEditEnabled() ) )
			{
				break;
			}
			if ( HTREEITEM hItem = GetSelectedItem() )
			{
				if ( ( GetTreeItemType( hItem ) == PCIE_LIST ) && ( GetChildItem( hItem ) != 0 ) )
				{
					DeleteAllNodes();
				}
				else if ( HTREEITEM hParentItem = GetParentItem( hItem ) )
				{
					if ( GetTreeItemType( hParentItem ) == PCIE_LIST  )
					{
						DeleteNode();
					}
				}
			}
			break;
		case ID_SELECTION_SELECT_ALL:
			if ( nCreateTreeTimer == 0 )
			{
				SelectAll();
			}
			break;
		case ID_SELECTION_RENAME:
		case ID_SELECTION_FIND:
		case ID_SELECTION_PROPERTIES:
			break;
		case ID_PC_EXPAND_ALL:
			ExpandAllPCItems();
			break;
		case ID_PC_EXPAND:
			ExpandPCItem();
			break;
		case ID_PC_COLLAPSE:
			CollapsePCItem();
			break;
		case ID_PC_COLLAPSE_ALL:
			CollapseAllPCItems();
			break;
		case ID_PC_OPTIMAL_WIDTH:
			SetOptimalWidth();
			break;
		case ID_PC_REFRESH:
			Refresh();
			break;
		case ID_PC_ADD_NODE:
			if ( IsEditEnabled() )
			{
				AddNode();
			}
			break;
		case ID_PC_DELETE_ALL_NODES:
			if ( IsEditEnabled() )
			{
				DeleteAllNodes();
			}
			break;
		case ID_PC_INSERT_NODE:
			if ( IsEditEnabled() )
			{
				InsertNode();
			}
			break;
		case ID_PC_DELETE_NODE:
			if ( IsEditEnabled() )
			{
				DeleteNode();
			}
			break;
		case ID_PC_SHOW_HIDDEN:
			ShowHidden();
			break;
		default:
			return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CPCMainTreeControl::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CPCMainTreeControl::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CPCMainTreeControl::UpdateCommand(), pbCheck == 0" );
	//
	if ( !GetViewManipulator() )
	{
		return false;
	}
	switch( nCommandID )
	{
		case ID_SELECTION_CUT:
			return false;
		case ID_SELECTION_COPY:
			if ( SUserData *pUserData = Singleton<IUserDataContainer>()->Get() )
			{
				( *pbEnable ) = ( GetSelectedCount() > 0 );
			}
			else
			{
				( *pbEnable ) = false;
			}
			( *pbCheck ) = false;
			return true;
		case ID_SELECTION_PASTE:
			if ( SUserData *pUserData = Singleton<IUserDataContainer>()->Get() )
			{
				( *pbEnable ) = ( !pUserData->pcSelection.IsEmpty() );
			}
			else
			{
				( *pbEnable ) = false;
			}
			( *pbCheck ) = false;
			return true;
		case ID_SELECTION_NEW:
			if ( ( nCreateTreeTimer != 0 ) || ( !IsEditEnabled() ) )
			{
				return false;
			}
			if ( HTREEITEM hItem = GetSelectedItem() )
			{
				( *pbEnable ) = ( GetTreeItemType( hItem ) == PCIE_LIST );
				if ( !( *pbEnable ) )
				{
					if ( HTREEITEM hParentItem = GetParentItem( hItem ) )
					{
						( *pbEnable ) = ( GetTreeItemType( hParentItem ) == PCIE_LIST );
					}
				}
				( *pbCheck ) = false;
				return true;
			}
			return false;
		case ID_SELECTION_CLEAR:
			if ( ( nCreateTreeTimer != 0 ) || ( !IsEditEnabled() ) )
			{
				return false;
			}
			if ( HTREEITEM hItem = GetSelectedItem() )
			{
				( *pbEnable ) = ( ( GetTreeItemType( hItem ) == PCIE_LIST ) && ( GetChildItem( hItem ) != 0 ) );
				if ( !( *pbEnable ) )
				{
					if ( HTREEITEM hParentItem = GetParentItem( hItem ) )
					{
						( *pbEnable ) = ( GetTreeItemType( hParentItem ) == PCIE_LIST );
					}
				}
				( *pbCheck ) = false;
				return true;
			}
			return true;
		case ID_SELECTION_SELECT_ALL:
			( *pbEnable ) = ( nCreateTreeTimer == 0 );
			( *pbCheck ) = false;
			return true;
		case ID_SELECTION_RENAME:
		case ID_SELECTION_FIND:
		case ID_SELECTION_PROPERTIES:
			return false;
		case ID_PC_EXPAND_ALL:
		case ID_PC_COLLAPSE_ALL:
		{
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		}
		case ID_PC_EXPAND:
		case ID_PC_COLLAPSE:
		{
			if ( HTREEITEM hItem = GetSelectedItem() )
			{
				if ( ItemHasChildren( hItem ) )
				{
					( *pbEnable ) = true;
					( *pbCheck ) = false;
					return true;
				}
			}
			return false;
		}
		case ID_PC_OPTIMAL_WIDTH:
			( *pbEnable ) = false;
			( *pbCheck ) = false;
			return true;
		case ID_PC_REFRESH:
			if ( nCreateTreeTimer != 0 )
			{
				return false;
			}
			( *pbEnable ) = ( GetSelectedItem() != 0 );
			( *pbCheck ) = false;
			return true;
		case ID_PC_ADD_NODE:
			if ( nCreateTreeTimer != 0 )
			{
				return false;
			}
			if ( HTREEITEM hItem = GetSelectedItem() )
			{
				const EPCIEType nType = GetTreeItemType( hItem );
				( *pbEnable ) = ( IsEditEnabled() && ( nType == PCIE_LIST ) );
				( *pbCheck ) = false;
				return true;
			}
			return false;
		case ID_PC_DELETE_ALL_NODES:
			if ( nCreateTreeTimer != 0 )
			{
				return false;
			}
			if ( HTREEITEM hItem = GetSelectedItem() )
			{
				const EPCIEType nType = GetTreeItemType( hItem );
				if ( nType == PCIE_LIST )
				{
					( *pbEnable ) = ( IsEditEnabled() && ( GetChildItem( hItem ) != 0 ) );
					( *pbCheck ) = false;
					return true;
				}
			}
			return false;
		case ID_PC_INSERT_NODE:
		case ID_PC_DELETE_NODE:
			if ( nCreateTreeTimer != 0 )
			{
				return false;
			}
			if ( HTREEITEM hItem = GetSelectedItem() )
			{
				if ( HTREEITEM hParentItem = GetParentItem( hItem ) )
				{
					const EPCIEType nType = GetTreeItemType( hParentItem );
					( *pbEnable ) = ( IsEditEnabled() && ( nType == PCIE_LIST ) );
					( *pbCheck ) = false;
					return true;
				}
			}
			return false;
		case ID_PC_SHOW_HIDDEN:
			( *pbEnable ) = true;
			( *pbCheck ) = bShowHidden;
			return true;
		default:
			return false;
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::SetViewManipulator( IManipulator* _pViewManipulator, const SObjectSet &rObjectSet, const string &rszTemporaryLabel )
{
	if ( pMultilineStringEditor != 0 )
	{
		pMultilineStringEditor->EnableEdit( IsEditEnabled() );
	}
	//
 	szGUID.clear();
	if ( ( _pViewManipulator != 0 ) && ( rObjectSet.objectNameSet.size() == 1 ) )
	{
		CVariant varUID;
		if ( CManipulatorManager::GetValue( &varUID, _pViewManipulator, Singleton<IUserDataContainer>()->Get()->constUserData.szGUIDName ) )
		{
			// дополнительная защита от не GUID полей и multivariant.
			if ( varUID.GetType() == CVariant::VT_POINTER )
			{
				GUID uid;
				memcpy( &uid, varUID.GetPtr(), sizeof( uid ) );
				NStr::GUID2String( &szGUID, uid ); 
			}
		}
	}
	//
	KillCreateTreeTimer();
	CDefaultView::SetViewManipulator( _pViewManipulator, rObjectSet, rszTemporaryLabel );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::Undo( IController* pController )
{
	bool bResult = true;
	if ( CObjectBaseController *pObjectController = dynamic_cast<CObjectBaseController*>( pController ) )
	{
		//pObjectController->Trace();
		//
		if ( !pObjectController->undoDataList.empty() )
		{
			GetViewManipulator()->ClearCache();
			IManipulator::CNameMap manipulatorNameMap;
			GetViewManipulator()->GetNameList( &manipulatorNameMap );
			//
			bool bUpdateTree = false;
			//
			for ( CObjectController::CUndoDataList::const_iterator posUndoData = pObjectController->undoDataList.end(); posUndoData != pObjectController->undoDataList.begin(); )
			{
				--posUndoData;
				IManipulator::CNameMap nameMap;
				pObjectController->GetNameListToUpdate( &nameMap, manipulatorNameMap, posUndoData->szName );
				//
				for ( IManipulator::CNameMap::const_iterator posName = nameMap.begin(); posName != nameMap.end(); ++posName )
				{
					switch ( posUndoData->eType )
					{
						///////////////////////////////////////////
						case CObjectBaseController::SUndoData::TYPE_INSERT:
							//DebugTrace( "Undo: TYPE_INSERT name: %s, new: %d, old: %d", posName->first.c_str(), (int)( posUndoData->newValue ), (int)( posUndoData->oldValue ) );
							if ( HTREEITEM hArrayItem = GetTreeItem( posName->first ) )
							{
								HTREEITEM hItemToDelete = GetChildItem( hArrayItem );
								for ( int nNodeIndex = 0; nNodeIndex < (int)( posUndoData->oldValue ); ++nNodeIndex )
								{
									if ( hItemToDelete == 0 )
									{
										break;
									}
									hItemToDelete = GetNextSiblingItem( hItemToDelete );
								}
								if ( hItemToDelete != 0 )
								{
									DeletePCNode( hArrayItem, hItemToDelete, (int)( posUndoData->oldValue ) );
								}
								ClearNameCache( posName->first );
							}
							break;
						///////////////////////////////////////////
						case CObjectBaseController::SUndoData::TYPE_REMOVE:
							//DebugTrace( "Undo: TYPE_REMOVE name: %s, new: %d, old: %d", posName->first.c_str(), (int)( posUndoData->newValue ), (int)( posUndoData->oldValue ) );
							if ( HTREEITEM hArrayItem = GetTreeItem( posName->first ) )
							{
								if ( (int)( posUndoData->newValue ) != NODE_REMOVEALL_INDEX )
								{
									HTREEITEM hNewItem = TVI_FIRST;
									if ( (int)( posUndoData->oldValue ) == NODE_ADD_INDEX )
									{
										hNewItem = TVI_LAST;
									}
									else if ( (int)( posUndoData->oldValue ) > 0 )
									{
										hNewItem = GetChildItem( hArrayItem );
										for ( int nNodeIndex = 0; nNodeIndex < ( (int)( posUndoData->newValue ) - 1 ); ++nNodeIndex )
										{
											if ( hNewItem == 0 )
											{
												break;
											}
											hNewItem = GetNextSiblingItem( hNewItem );
										}
									}
									if ( hNewItem == 0 )
									{
										hNewItem = TVI_LAST;
									}
									InsertPCNode( hArrayItem, hNewItem, (int)( posUndoData->newValue ) );
									ClearNameCache( posName->first );
								}
								else
								{
									ClearNameCache( posName->first );
									CreateTree( hArrayItem, true, false );
								}
							}
							break;
						///////////////////////////////////////////
						case CObjectBaseController::SUndoData::TYPE_CHANGE:
							//DebugTrace( "Undo: TYPE_CHANGE %s", posName->first.c_str() );
							UpdateValue( posName->first );
							break;
						///////////////////////////////////////////
						case CObjectBaseController::SUndoData::TYPE_EXPAND:
						{
							bCreateControls = true;
							const HTREEITEM hItem = GetTreeItem( posName->first );
							NI_ASSERT( hItem != 0, "CPCMainTreeControl::InternalUndo( TYPE_EXPAND ): hItem == 0" );
							Expand( hItem, (bool)( posUndoData->newValue ) ? TVE_COLLAPSE : TVE_EXPAND );
							bCreateControls = false;
							break;
						}
						///////////////////////////////////////////
						default:
							break;
					}
				}
			}
		}
		RedrawWindow();
   	// Посылаем сообщение родительскому диалогу
		{
			CDialog *pwndParentDialog = dynamic_cast<CDialog*>( GetParent() );
			pwndParentDialog->SendMessage( WM_PC_MANIPULATOR_CHANGE, 0, 0 );
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCMainTreeControl::Redo( IController* pController )
{
	if ( CObjectBaseController *pObjectController = dynamic_cast<CObjectBaseController*>( pController ) )
	{
		//pObjectController->Trace();
		//
		if ( !pObjectController->undoDataList.empty() )
		{
			GetViewManipulator()->ClearCache();
			IManipulator::CNameMap manipulatorNameMap;
			GetViewManipulator()->GetNameList( &manipulatorNameMap );
			//	
			for ( CObjectController::CUndoDataList::const_iterator posUndoData = pObjectController->undoDataList.begin(); posUndoData != pObjectController->undoDataList.end(); ++posUndoData )
			{
				IManipulator::CNameMap nameMap;
				pObjectController->GetNameListToUpdate( &nameMap, manipulatorNameMap, posUndoData->szName );
				//
				for ( IManipulator::CNameMap::const_iterator posName = nameMap.begin(); posName != nameMap.end(); ++posName )
				{
					switch ( posUndoData->eType )
					{
						///////////////////////////////////////////
						case CObjectBaseController::SUndoData::TYPE_INSERT:
							//DebugTrace( "Undo: TYPE_INSERT name: %s, new: %d, old: %d", posName->first.c_str(), (int)( posUndoData->newValue ), (int)( posUndoData->oldValue ) );
							if ( HTREEITEM hArrayItem = GetTreeItem( posName->first ) )
							{
								HTREEITEM hNewItem = TVI_FIRST;
								if ( (int)( posUndoData->newValue ) == NODE_ADD_INDEX )
								{
									hNewItem = TVI_LAST;
								}
								else if ( (int)( posUndoData->oldValue ) > 0 )
								{
									hNewItem = GetChildItem( hArrayItem );
									for ( int nNodeIndex = 0; nNodeIndex < ( (int)( posUndoData->oldValue ) - 1 ); ++nNodeIndex )
									{
										if ( hNewItem == 0 )
										{
											break;
										}
										hNewItem = GetNextSiblingItem( hNewItem );
									}
								}
								if ( hNewItem == 0 )
								{
									hNewItem = TVI_LAST;
								}
								InsertPCNode( hArrayItem, hNewItem, (int)( posUndoData->oldValue ) );
								ClearNameCache( posName->first );
							}
							break;
						///////////////////////////////////////////
						case CObjectBaseController::SUndoData::TYPE_REMOVE:
							//DebugTrace( "Undo: TYPE_REMOVE name: %s, new: %d, old: %d", posName->first.c_str(), (int)( posUndoData->newValue ), (int)( posUndoData->oldValue ) );
							if ( HTREEITEM hArrayItem = GetTreeItem( posName->first ) )
							{
								HTREEITEM hItemToDelete = 0;
								if ( (int)( posUndoData->newValue ) != NODE_REMOVEALL_INDEX )
								{
									hItemToDelete = GetChildItem( hArrayItem );
									for ( int nNodeIndex = 0; nNodeIndex < (int)( posUndoData->newValue ); ++nNodeIndex )
									{
										if ( hItemToDelete == 0 )
										{
											break;
										}
										hItemToDelete = GetNextSiblingItem( hItemToDelete );
									}
								}
								DeletePCNode( hArrayItem, hItemToDelete, (int)( posUndoData->newValue ) );
								ClearNameCache( posName->first );
							}
							break;
						///////////////////////////////////////////
						case CObjectBaseController::SUndoData::TYPE_CHANGE:
							//DebugTrace( "Redo: TYPE_CHANGE %s", posName->first.c_str() );
							UpdateValue( posName->first );
							break;
						///////////////////////////////////////////
						case CObjectBaseController::SUndoData::TYPE_EXPAND:
						{
							bCreateControls = true;
							const HTREEITEM hItem = GetTreeItem( posName->first );
							NI_ASSERT( hItem != 0, "CPCMainTreeControl::InternalRedo( TYPE_EXPAND ): hItem == 0" );
							Expand( hItem, (bool)( posUndoData->newValue ) ? TVE_EXPAND : TVE_COLLAPSE );
							bCreateControls = false;
							break;
						}
						///////////////////////////////////////////
						default:
							break;
					}
				}
			}
			RedrawWindow();
			// Посылаем сообщение родительскому диалогу
			{
				CDialog *pwndParentDialog = dynamic_cast<CDialog*>( GetParent() );
				pwndParentDialog->SendMessage( WM_PC_MANIPULATOR_CHANGE, 0, 0 );
			}
		}
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// basement storage  
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
