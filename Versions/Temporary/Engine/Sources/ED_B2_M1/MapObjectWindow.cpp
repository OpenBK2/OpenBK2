#include "stdafx.h"

#include "../mapeditorlib/commandhandlerdefines.h"
#include "../mapeditorlib/resourcedefines.h"
#include "../misc/2darray.h"
#include "../stats_b2_m1/iconsset.h"
#include "../sceneb2/scene.h"
#include "../libdb/resourcemanager.h"
#include "../mapeditorlib/multimanipulator.h"
#include "../MapEditorLib/Interface_MainFrame.h"
#include "../MapEditorLib/EditParameter.h"
#include "../MapEditorLib/Interface_ObjectCollector.h"
#include "ED_B2_M1Dll.h"
#include "MapObjectWindow.h"

#include <zconf.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


const char CMapObjectWindow::FILTER_TYPE[] = "MAPOBJECT";
const char CMapObjectWindow::MAPOBJECT_EXTRACTOR_TYPE[] = "MAPOBJECT";
const char CMapObjectWindow::SPOT_EXTRACTOR_TYPE[] = "SPOT";


CMapObjectWindow::CMapObjectWindow( bool _bFull, CWnd* pParent )
	: CResizeDialog( _bFull ? CMapObjectWindow::IDD_FULL : CMapObjectWindow::IDD_NO_BUTTONS, pParent ), bCreateControls( true ), bFull( _bFull ), nStyle( LVS_ICON )
{
	selectedObjectListElement.szObjectTypeName.clear();
	selectedObjectListElement.objectDBID.Clear();
	//
	if ( bFull )
	{
		SetControlStyle( IDC_TMIMO_PLAYER_LABEL, ANCHORE_LEFT_TOP );
		SetControlStyle( IDC_TMIMO_PLAYER_COMBO, ANCHORE_LEFT_TOP | RESIZE_HOR );
		SetControlStyle( IDC_TMIMO_DIRECTION_RANDOM_COMBO, ANCHORE_LEFT_TOP );
		SetControlStyle( IDC_TMIMO_DIRECTION_CUSTOM_COMBO, ANCHORE_LEFT_TOP );
		SetControlStyle( IDC_TMIMO_DIRECTION_CUSTOM_EDIT, ANCHORE_LEFT_TOP | RESIZE_HOR );
		SetControlStyle( IDC_TMIMO_DIRECTION_LABEL, ANCHORE_LEFT_TOP );
		SetControlStyle( IDC_TMIMO_DIRECTION_CUSTOM_LABEL, ANCHORE_RIGHT_TOP );
		SetControlStyle( IDC_TMIMO_DELIMITER_0, ANCHORE_LEFT_TOP | RESIZE_HOR );
		SetControlStyle( IDC_TMIMO_FILTER_LABEL, ANCHORE_LEFT_TOP );
		SetControlStyle( IDC_TMIMO_FILTER_SHORTCUT_0, ANCHORE_LEFT_TOP | RESIZE_HOR, 0.5f, 0.5f, 1.0f / 3.0f, 1.0f );
		SetControlStyle( IDC_TMIMO_FILTER_SHORTCUT_1, ANCHORE_HOR_CENTER | ANCHORE_TOP | RESIZE_HOR, 0.5f, 0.5f, 1.0f / 3.0f, 1.0f );
		SetControlStyle( IDC_TMIMO_FILTER_SHORTCUT_2, ANCHORE_RIGHT_TOP | RESIZE_HOR, 0.5f, 0.5f, 1.0f / 3.0f, 1.0f );
		SetControlStyle( IDC_TMIMO_FILTER_SHORTCUT_3, ANCHORE_LEFT_TOP | RESIZE_HOR, 0.5f, 0.5f, 1.0f / 3.0f, 1.0f );
		SetControlStyle( IDC_TMIMO_FILTER_SHORTCUT_4, ANCHORE_HOR_CENTER | ANCHORE_TOP | RESIZE_HOR, 0.5f, 0.5f, 1.0f / 3.0f, 1.0f );
		SetControlStyle( IDC_TMIMO_FILTER_SHORTCUT_5, ANCHORE_RIGHT_TOP | RESIZE_HOR, 0.5f, 0.5f, 1.0f / 3.0f, 1.0f );
		SetControlStyle( IDC_TMIMO_FILTER_SHORTCUT_6, ANCHORE_LEFT_TOP | RESIZE_HOR, 0.5f, 0.5f, 1.0f / 3.0f, 1.0f );
		SetControlStyle( IDC_TMIMO_FILTER_SHORTCUT_7, ANCHORE_HOR_CENTER | ANCHORE_TOP | RESIZE_HOR, 0.5f, 0.5f, 1.0f / 3.0f, 1.0f );
		SetControlStyle( IDC_TMIMO_FILTER_SHORTCUT_8, ANCHORE_RIGHT_TOP | RESIZE_HOR, 0.5f, 0.5f, 1.0f / 3.0f, 1.0f );
		SetControlStyle( IDC_TMIMO_FILTER_SHORTCUT_9, ANCHORE_LEFT_TOP );
		SetControlStyle( IDC_TMIMO_FILTER_COMBO, ANCHORE_LEFT_TOP | RESIZE_HOR );
		SetControlStyle( IDC_TMIMO_OBJECT_LIST, ANCHORE_LEFT_TOP | RESIZE_HOR_VER );
	}
	else
	{
		SetControlStyle( IDC_TMIMO_PLAYER_LABEL, ANCHORE_LEFT_TOP );
		SetControlStyle( IDC_TMIMO_PLAYER_COMBO, ANCHORE_LEFT_TOP | RESIZE_HOR );
		SetControlStyle( IDC_TMIMO_DIRECTION_RANDOM_COMBO, ANCHORE_LEFT_TOP );
		SetControlStyle( IDC_TMIMO_DIRECTION_CUSTOM_COMBO, ANCHORE_LEFT_TOP );
		SetControlStyle( IDC_TMIMO_DIRECTION_CUSTOM_EDIT, ANCHORE_LEFT_TOP | RESIZE_HOR );
		SetControlStyle( IDC_TMIMO_DIRECTION_LABEL, ANCHORE_LEFT_TOP );
		SetControlStyle( IDC_TMIMO_DIRECTION_CUSTOM_LABEL, ANCHORE_RIGHT_TOP );
		SetControlStyle( IDC_TMIMO_DELIMITER_0, ANCHORE_LEFT_TOP | RESIZE_HOR );
		SetControlStyle( IDC_TMIMO_FILTER_LABEL, ANCHORE_LEFT_TOP );
		SetControlStyle( IDC_TMIMO_FILTER_COMBO, ANCHORE_LEFT_TOP | RESIZE_HOR );
		SetControlStyle( IDC_TMIMO_OBJECT_LIST, ANCHORE_LEFT_TOP | RESIZE_HOR_VER );
	}
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_MAPINFO_MAPOBJECT_WINDOW, this );
	Singleton<ICommandHandlerContainer>()->Register( CHID_MAPINFO_MAPOBJECT_WINDOW, ID_MIMOOLCM_LIST, ID_MIMOOLCM_PROPERTIES );
}


CMapObjectWindow::~CMapObjectWindow()
{
	Singleton<ICommandHandlerContainer>()->UnRegister( CHID_MAPINFO_MAPOBJECT_WINDOW );
	Singleton<ICommandHandlerContainer>()->Remove( CHID_MAPINFO_MAPOBJECT_WINDOW );
}


/**
CWMMnemonicCodes mnemonicCodes;
LRESULT CMapObjectWindow::WindowProc( UINT message, WPARAM wParam, LPARAM lParam ) 
{
	DebugTrace( "Message: %s, wParam: 0x%X(%u), lParam: 0x%X\n", mnemonicCodes.Get( message ).c_str(), wParam, wParam, lParam );
	return CResizeDialog::WindowProc( message, wParam, lParam );
}
/**/


void CMapObjectWindow::DoDataExchange( CDataExchange* pDX )
{
	CResizeDialog::DoDataExchange( pDX );

	DDX_Control( pDX, IDC_TMIMO_PLAYER_COMBO, wndPalyerComboBox );
	DDX_Control( pDX, IDC_TMIMO_DIRECTION_CUSTOM_EDIT, wndDirectionEdit );
	DDX_Control( pDX, IDC_TMIMO_FILTER_COMBO, wndFilterComboBox );
	DDX_Control( pDX, IDC_TMIMO_OBJECT_LIST, wndObjectList );
}


BEGIN_MESSAGE_MAP(CMapObjectWindow, CResizeDialog)
	ON_WM_SIZE()
	ON_CBN_SELCHANGE(IDC_TMIMO_FILTER_COMBO, OnSelchangeFilterComboBox)
	ON_BN_CLICKED(IDC_TMIMO_DIRECTION_RANDOM_COMBO, OnDirectionRadio)
	ON_BN_CLICKED(IDC_TMIMO_DIRECTION_CUSTOM_COMBO, OnDirectionRadio)
	ON_EN_CHANGE(IDC_TMIMO_DIRECTION_CUSTOM_EDIT, OnChangeDirection)
	ON_BN_CLICKED(IDC_TMIMO_FILTER_SHORTCUT_0, OnFilterRadio)
	ON_BN_CLICKED(IDC_TMIMO_FILTER_SHORTCUT_1, OnFilterRadio)
	ON_BN_CLICKED(IDC_TMIMO_FILTER_SHORTCUT_2, OnFilterRadio)
	ON_BN_CLICKED(IDC_TMIMO_FILTER_SHORTCUT_3, OnFilterRadio)
	ON_BN_CLICKED(IDC_TMIMO_FILTER_SHORTCUT_4, OnFilterRadio)
	ON_BN_CLICKED(IDC_TMIMO_FILTER_SHORTCUT_5, OnFilterRadio)
	ON_BN_CLICKED(IDC_TMIMO_FILTER_SHORTCUT_6, OnFilterRadio)
	ON_BN_CLICKED(IDC_TMIMO_FILTER_SHORTCUT_7, OnFilterRadio)
	ON_BN_CLICKED(IDC_TMIMO_FILTER_SHORTCUT_8, OnFilterRadio)
	ON_BN_CLICKED(IDC_TMIMO_FILTER_SHORTCUT_9, OnFilterRadio)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_TMIMO_OBJECT_LIST, OnItemchangedObjectList)
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()


BOOL CMapObjectWindow::OnInitDialog() 
{
	CResizeDialog::OnInitDialog();
	//	
	resizeDialogOptions.szParameters.resize( 1 );
	//
	SetObjectsListStyle( LVS_ICON );
	wndObjectList.SetImageList( Singleton<IObjectCollector>()->GetImageList( LVSIL_NORMAL ), LVSIL_NORMAL );
	wndObjectList.SetImageList( Singleton<IObjectCollector>()->GetImageList( LVSIL_SMALL ), LVSIL_SMALL );
	//	
	FillFilterComboBox();
	//
	FillObjectList();
	//
	bCreateControls = false;
	return true;
}


int CMapObjectWindow::GetSelectedFilterIndex()
{
	if ( bFull )
	{
		int nRadioButtonID = GetCheckedRadioButton( IDC_TMIMO_FILTER_SHORTCUT_0, IDC_TMIMO_FILTER_SHORTCUT_9 );
		if ( nRadioButtonID != 0 )
		{
			if ( nRadioButtonID == IDC_TMIMO_FILTER_SHORTCUT_9 )
			{
				const int nStringNumber = wndFilterComboBox.GetCurSel();
				if ( nStringNumber >= 0 )
				{
					CString strText;
					wndFilterComboBox.GetLBText( nStringNumber, strText );
					resizeDialogOptions.szParameters[0] = strText;
					return wndFilterComboBox.GetItemData( nStringNumber );
				}
			}
			else
			{
			}
		}
	}
	else
	{
		const int nStringNumber = wndFilterComboBox.GetCurSel();
		if ( nStringNumber >= 0 )
		{
			CString strText;
			wndFilterComboBox.GetLBText( nStringNumber, strText );
			resizeDialogOptions.szParameters[0] = strText;
			return wndFilterComboBox.GetItemData( nStringNumber );
		}
	}
	return INVALID_NODE_ID;
}


bool CMapObjectWindow::GetEditParameters( CMapObjectMultiState::SEditParameters *pEditParameters )
{
	NI_ASSERT( pEditParameters != 0, "CMapObjectWindow::GetEditParameters(), pEditParameters == 0" );

	if ( pEditParameters->nFlags & ( MIMOSEP_PLAYER_COUNT | MIMOSEP_PLAYER_INDEX ) )
	{
		GetComboBoxEditParameters( &( pEditParameters->playerList ),
															 &( pEditParameters->nPlayerIndex ),
															 wndPalyerComboBox,
															 ( pEditParameters->nFlags & MIMOSEP_PLAYER_COUNT ),
															 ( pEditParameters->nFlags & MIMOSEP_PLAYER_INDEX ) );
	}
	if ( pEditParameters->nFlags & MIMOSEP_DIRECTION_TYPE )
	{
		pEditParameters->eDirectionType = static_cast<CMapObjectMultiState::SEditParameters::EDirectionType>( GetCheckedRadioButton( IDC_TMIMO_DIRECTION_RANDOM_COMBO, IDC_TMIMO_DIRECTION_CUSTOM_COMBO ) - IDC_TMIMO_DIRECTION_RANDOM_COMBO );
	}
	if ( ( pEditParameters->nFlags & MIMOSEP_DIRECTION ) > 0 )
	{
		CString strText;
		GetDlgItemText( IDC_TMIMO_DIRECTION_CUSTOM_EDIT, strText );
		if ( ( sscanf( strText, "%g", &( pEditParameters->fDirection ) ) < 1 ) ||
				 ( pEditParameters->fDirection < 0.0f ) || 
				 ( pEditParameters->fDirection > 360.0f ) )
		{
			bCreateControls = true;
			if ( pEditParameters->fDirection < 0.0f )
			{
				pEditParameters->fDirection = 0.0f;
			}
			else if ( pEditParameters->fDirection > 360.0f )
			{
				pEditParameters->fDirection = 360.0f;
			}
			SetDlgItemText( IDC_TMIMO_DIRECTION_CUSTOM_EDIT, StrFmt( "%g", pEditParameters->fDirection ) );
			bCreateControls = false;
		}
	}
	if ( ( pEditParameters->nFlags & MIMOSEP_THUMBNAILS ) > 0 )
	{
		pEditParameters->bThumbnails = ( nStyle == LVS_ICON );
	}
	return true;
}


bool CMapObjectWindow::SetEditParameters( const CMapObjectMultiState::SEditParameters &rEditParameters )
{
	if ( rEditParameters.nFlags & ( MIMOSEP_PLAYER_COUNT | MIMOSEP_PLAYER_INDEX ) )
	{
		SetComboBoxEditParameters( rEditParameters.playerList,
															 rEditParameters.nPlayerIndex,
															 &wndPalyerComboBox,
															 rEditParameters.nFlags & MIMOSEP_PLAYER_COUNT,
															 rEditParameters.nFlags & MIMOSEP_PLAYER_INDEX );
	}
	if ( rEditParameters.nFlags & MIMOSEP_DIRECTION_TYPE )
	{
		CheckRadioButton( IDC_TMIMO_DIRECTION_RANDOM_COMBO, IDC_TMIMO_DIRECTION_CUSTOM_COMBO, IDC_TMIMO_DIRECTION_RANDOM_COMBO + rEditParameters.eDirectionType );
	}
	if ( ( rEditParameters.nFlags & MIMOSEP_DIRECTION ) > 0 )
	{
		SetDlgItemText( IDC_TMIMO_DIRECTION_CUSTOM_EDIT, StrFmt( "%g", rEditParameters.fDirection ) );
	}
	if ( ( rEditParameters.nFlags & MIMOSEP_THUMBNAILS ) > 0 )
	{
		SetObjectsListStyle( rEditParameters.bThumbnails ? LVS_ICON : LVS_LIST );
	}
	return true;
}


void CMapObjectWindow::UpdateObjectsListStyle()
{
	if ( ::IsWindow( wndObjectList.m_hWnd ) )
	{
		wndObjectList.ModifyStyle( LVS_TYPEMASK, nStyle );
		if ( nStyle == LVS_ICON )
		{
			wndObjectList.SetIconSpacing( NORMAL_IMAGE_SIZE_X + NORMAL_IMAGE_SPACE_X, NORMAL_IMAGE_SIZE_Y + NORMAL_IMAGE_SPACE_Y );
		}
		wndObjectList.Arrange( LVA_DEFAULT );
	}
}


void CMapObjectWindow::SetObjectsListStyle( int _nStyle )
{
	nStyle = _nStyle;
	UpdateObjectsListStyle();
}


void CMapObjectWindow::FillFilterComboBox()
{
	bCreateControls = true;
	wndFilterComboBox.ResetContent();
	//
	IObjectFilterCollector::CFilterList filterList;
	Singleton<IObjectFilterCollector>()->GetFilterList( &filterList, FILTER_TYPE );
	{
		for ( int nFilterIndex = 0; nFilterIndex < filterList.size(); ++nFilterIndex ) 
		{
			const int nStringNumber = wndFilterComboBox.AddString( filterList[nFilterIndex] );
			wndFilterComboBox.SetItemData( nStringNumber, nFilterIndex );
		}
		if ( filterList.size() > 0 )
		{
			if ( !resizeDialogOptions.szParameters[0].empty() )
			{
				wndFilterComboBox.SelectString( -1, resizeDialogOptions.szParameters[0].c_str() );
			}
			else if ( filterList.size() > 1 )
			{
				wndFilterComboBox.SetCurSel( 1 );
			}
		}
	}
	bCreateControls = false;
}


void CMapObjectWindow::FillObjectList()
{
	CWaitCursor waitCursor;
	int nFilterIndex = GetSelectedFilterIndex();
	if ( nFilterIndex != INVALID_NODE_ID )
	{
		if ( !Singleton<IObjectFilterCollector>()->IsSeparator( FILTER_TYPE, nFilterIndex ) )
		{
			if ( const IObjectFilter *pObjectFilter = Singleton<IObjectFilterCollector>()->Get( FILTER_TYPE, nFilterIndex ) )
			{
				bCreateControls = true;
				//
				wndObjectList.DeleteAllItems();
				objectListElementMap.clear();
				IObjectCollector::CObjectCollection objectCollection;
				Singleton<IObjectCollector>()->ApplyFilter( &objectCollection, pObjectFilter );
				int nObjectsCount = 0;
				int nObjectIndex = 0;
				for ( IObjectCollector::CObjectCollection::const_iterator itObjectCollection = objectCollection.begin(); itObjectCollection != objectCollection.end(); ++itObjectCollection )
				{
					for ( IObjectCollector::CObjectNameCollection::const_iterator itObjectNameCollection = itObjectCollection->second.begin(); itObjectNameCollection != itObjectCollection->second.end(); ++itObjectNameCollection )
					{
						SObjectListElement objectListElement;
						objectListElement.szObjectTypeName = itObjectCollection->first;
						objectListElement.objectDBID = itObjectNameCollection->first;
						//
						nObjectIndex = wndObjectList.InsertItem( nObjectsCount, itObjectNameCollection->second.strLabel, itObjectNameCollection->second.nIconIndex );
						wndObjectList.SetItemData( nObjectIndex, nObjectsCount );
						objectListElementMap[nObjectsCount] = objectListElement;
						//DebugTrace( "CVSOWindow::FillObjectList: [%d] = %s:%d", nObjectsCount, objectListElement.szObjectTypeName.c_str(), objectListElement.nObjectID );
						++nObjectsCount;
					}
				}
				bCreateControls = false;
			}
		}
	}
}


void CMapObjectWindow::OnSetFocus( CWnd* pOldWnd )
{
	Singleton<ICommandHandlerContainer>()->Set( CHID_OBJECT_STORAGE, this );
}


void CMapObjectWindow::UpdateSelection()
{
	if ( !bCreateControls )
	{
		if ( wndObjectList.GetSelectedCount() > 0 )
		{
			int nItemIndex = wndObjectList.GetNextItem( -1, LVNI_SELECTED );
			if ( nItemIndex != INVALID_NODE_ID )
			{
				nItemIndex = wndObjectList.GetItemData( nItemIndex );
				if ( nItemIndex != INVALID_NODE_ID )
				{
					CObjectListElementMap::const_iterator posObjectListElement = objectListElementMap.find( nItemIndex );
					if ( posObjectListElement != objectListElementMap.end() )
					{
						if ( ( posObjectListElement->second.szObjectTypeName != selectedObjectListElement.szObjectTypeName ) || 
								 ( posObjectListElement->second.objectDBID != selectedObjectListElement.objectDBID ) )
						{
							selectedObjectListElement.szObjectTypeName = posObjectListElement->second.szObjectTypeName;
							selectedObjectListElement.objectDBID = posObjectListElement->second.objectDBID;
							//DebugTrace( "CMapObjectWindow::UpdateSelection: %s:%d", selectedObjectListElement.szObjectTypeName.c_str(), selectedObjectListElement.nObjectID );
							//
							ICommandHandlerContainer* pCommandHandlerContainer = Singleton<ICommandHandlerContainer>();
							pCommandHandlerContainer->Set( CHID_OBJECT_STORAGE, this );
							pCommandHandlerContainer->HandleCommand( CHID_MAPINFO_MAPOBJECT_MULTI_STATE, ID_MIMO_SWITCH_MULTI_STATE, reinterpret_cast<DWORD>( &( selectedObjectListElement.szObjectTypeName ) ) );	
							pCommandHandlerContainer->HandleCommand( CHID_MAPINFO_MAPOBJECT_STATE, ID_MIMO_SWITCH_ADD_STATE, 0 );	
						}
					}
				}
			}
		}
	}
}


void CMapObjectWindow::ClearSelection()
{
	bCreateControls = true;
	int nItemIndex = wndObjectList.GetNextItem( -1, LVNI_SELECTED );
	if ( nItemIndex != INVALID_NODE_ID )
	{
		wndObjectList.SetItemState( nItemIndex, 0, LVIS_SELECTED | LVIS_FOCUSED );
	}
	selectedObjectListElement.szObjectTypeName.clear();
	selectedObjectListElement.objectDBID.Clear();
	Singleton<IMainFrameContainer>()->Get()->RestoreObjectStorage();
	bCreateControls = false;
}


bool CMapObjectWindow::HandleCommand( UINT nCommandID, DWORD dwData )
{
	switch( nCommandID )
	{
		case ID_GET_EDIT_PARAMETERS:
		{
			CMapObjectMultiState::SEditParameters *pEditParameters = reinterpret_cast<CMapObjectMultiState::SEditParameters*>( dwData );
			if ( pEditParameters != 0 )
			{
				return GetEditParameters( pEditParameters );
			}
			return false;
		}
		case ID_SET_EDIT_PARAMETERS:
		{
			const CMapObjectMultiState::SEditParameters *pEditParameters = reinterpret_cast<const CMapObjectMultiState::SEditParameters*>( dwData );
			if ( pEditParameters != 0 )
			{
				return SetEditParameters( *pEditParameters );
			}
			return false;
		}
		case ID_MIMO_CLEAR_SELECTION:
		{
			ClearSelection();
			return true;
		}
		case ID_OS_GET_OBJECTSET:
		{
			if ( SObjectSet *pObjectSet = reinterpret_cast<SObjectSet*>( dwData ) )
			{
				pObjectSet->szObjectTypeName = selectedObjectListElement.szObjectTypeName;
				pObjectSet->objectNameSet.clear();
				pObjectSet->objectNameSet[selectedObjectListElement.objectDBID] = 0;
			}
			return true;
		}
		case ID_MIMOOLCM_LIST:
			SetObjectsListStyle( LVS_LIST );
			return true;
		case ID_MIMOOLCM_THUMBNAILS:
			SetObjectsListStyle( LVS_ICON );
			return true;
		case ID_MIMOOLCM_PROPERTIES:
		{
			CPtr<IManipulator> pObjectManipulator = 0;
			SObjectSet objectSet;
			objectSet.szObjectTypeName = selectedObjectListElement.szObjectTypeName;
			objectSet.objectNameSet[selectedObjectListElement.objectDBID] = 0;
			IResourceManager *pResourceManager = Singleton<IResourceManager>();
			{
				CMultiManipulator *pMultiManipulator = new CMultiManipulator();
				for ( CObjectNameSet::const_iterator itObjectName = objectSet.objectNameSet.begin(); itObjectName != objectSet.objectNameSet.end(); ++itObjectName )
				{
					pMultiManipulator->InsertManipulator( itObjectName->first, pResourceManager->CreateObjectManipulator( objectSet.szObjectTypeName, itObjectName->first ), false, false );
				}
				pObjectManipulator = pMultiManipulator;
			}
			IView *pView = 0;
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, ID_PC_DIALOG_GET_VIEW, reinterpret_cast<DWORD>( &pView ) );
			if ( pView != 0 )
			{
				pView->SetViewManipulator( pObjectManipulator, objectSet, string() );
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_VIEW, ID_VIEW_SHOW_PROPERTY_BROWSER, 1 );
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, ID_PC_DIALOG_CREATE_TREE, 0 );
			}
			return true;
		}
		default:
			return false;
	}
	return false;
}


bool CMapObjectWindow::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CMapObjectWindow::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CMapObjectWindow::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID )
	{
		case ID_GET_EDIT_PARAMETERS:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		case ID_SET_EDIT_PARAMETERS:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		case ID_MIMO_CLEAR_SELECTION:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		case ID_OS_GET_OBJECTSET:
			( *pbEnable ) = ( ( !selectedObjectListElement.szObjectTypeName.empty() ) && ( !selectedObjectListElement.objectDBID.IsEmpty() ) );
			( *pbCheck ) = false;
			return true;
		case ID_MIMOOLCM_LIST:
			( *pbEnable ) = true;
			( *pbCheck ) = ( nStyle == LVS_LIST );
			return true;
		case ID_MIMOOLCM_THUMBNAILS:
			( *pbEnable ) = true;
			( *pbCheck ) = ( nStyle == LVS_ICON );
			return true;
		case ID_MIMOOLCM_PROPERTIES:
			( *pbEnable ) = ( ( !selectedObjectListElement.szObjectTypeName.empty() ) && ( !selectedObjectListElement.objectDBID.IsEmpty() ) );
			( *pbCheck ) = false;
			return true;
		default:
			return false;
	}
	return false;
}


void CMapObjectWindow::OnSize( UINT nType, int cx, int cy ) 
{
	CResizeDialog::OnSize( nType, cx, cy );
	//
	UpdateObjectsListStyle();
}


void CMapObjectWindow::OnSelchangeFilterComboBox()
{
	if ( !bCreateControls )
	{
		FillObjectList();
		SaveResizeDialogOptions();
	}
}


void CMapObjectWindow::OnDirectionRadio()
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_MULTI_STATE, ID_GET_EDIT_PARAMETERS, MIMOSEP_DIRECTION_TYPE );
	}
}


void CMapObjectWindow::OnChangeDirection()
{
if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_MULTI_STATE, ID_GET_EDIT_PARAMETERS, MIMOSEP_DIRECTION );
	}
}


void CMapObjectWindow::OnFilterRadio()
{
	FillObjectList();
}

void CMapObjectWindow::OnItemchangedObjectList( NMHDR* pNMHDR, LRESULT* pResult )
{
	UpdateSelection();
}


void CMapObjectWindow::OnContextMenu( CWnd *pwnd, CPoint point )
{
	CResizeDialog::OnContextMenu( pwnd, point );

	if ( pwnd->m_hWnd == wndObjectList.m_hWnd )
	{
		CMenu mainPopupMenu;
		AfxSetResourceHandle( theEDB2M1Instance );
		mainPopupMenu.LoadMenu( IDM_MAPINFO_CONTEXT_MENU );
		AfxSetResourceHandle( AfxGetInstanceHandle() );
		CMenu *pMenu = mainPopupMenu.GetSubMenu( MICM_MAPOBJECT_OBJECT_LIST );
		if ( pMenu )
		{
			pMenu->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x, point.y, Singleton<IMainFrameContainer>()->GetSECWorkbook(), 0 );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
		}
		mainPopupMenu.DestroyMenu();
	}
}


// basement storage  


