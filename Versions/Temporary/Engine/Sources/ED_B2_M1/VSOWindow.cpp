#include "stdafx.h"
#include <fmt/format.h>

#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/ResourceDefines.h"
#include "Misc/2Darray.h"
#include "Stats_B2_M1/IconsSet.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/MultiManipulator.h"
#include "MapEditorLib/Interface_ObjectCollector.h"
#include "MapEditorLib/Tools_HashSet.h"
#include "libdb/ResourceManager.h"
#include "ED_B2_M1Dll.h"
#include "VSOWindow.h"

#include <cstdint>

#include <zconf.h>

const char CVSOWindow::FILTER_TYPE[] = "VSO";
const char CVSOWindow::EXTRACTOR_TYPE[] = "VSO";

CVSOWindow::CVSOWindow( CWnd* pParent )
	: CResizeDialog( CVSOWindow::IDD, pParent ), bCreateControls( true ), bEnableHeight( false ), nStyle( LVS_ICON )
{
	selectedObjectListElement.szObjectTypeName.clear();
	selectedObjectListElement.objectDBID.Clear();
	//
	SetControlStyle( IDC_TMIVSO_SINGLE_RADIO, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_TMIVSO_MULTI_RADIO, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_TMIVSO_ALL_RATIO, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_TMIVSO_PREDEFINED_STATS_RADIO, ANCHORE_HOR_CENTER | ANCHORE_TOP );
	SetControlStyle( IDC_TMIVSO_CUSTOM_STATS_RADIO, ANCHORE_HOR_CENTER | ANCHORE_TOP );
	SetControlStyle( IDC_TMIVSO_DELIMITER_0, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_TMIVSO_WIDTH_LABEL_LEFT, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_TMIVSO_WIDTH, ANCHORE_LEFT_TOP | RESIZE_HOR, 0.5f, 1.0f, 0.5f, 0.5f );
	SetControlStyle( IDC_TMIVSO_WIDTH_LABEL_RIGHT, ANCHORE_HOR_CENTER | ANCHORE_TOP );
	SetControlStyle( IDC_TMIVSO_OPACITY_LABEL_LEFT, ANCHORE_HOR_CENTER | ANCHORE_TOP );
	SetControlStyle( IDC_TMIVSO_OPACITY, ANCHORE_RIGHT_TOP | RESIZE_HOR, 0.5f, 1.0f, 0.5f, 0.5f );
	SetControlStyle( IDC_TMIVSO_OPACITY_LABEL_RIGHT, ANCHORE_RIGHT_TOP );
	SetControlStyle( IDC_TMIVSO_DELIMITER_1, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_TMIVSO_FILTER_LABEL, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_TMIVSO_FILTER_COMBO, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_TMIVSO_OBJECT_LIST, ANCHORE_LEFT_TOP | RESIZE_HOR_VER );
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_MAPINFO_VSO_WINDOW, this );
	Singleton<ICommandHandlerContainer>()->Register( CHID_MAPINFO_VSO_WINDOW, ID_MIVSOOLCM_LIST, ID_MIVSOOLCM_PROPERTIES );
}


CVSOWindow::~CVSOWindow()
{
	Singleton<ICommandHandlerContainer>()->UnRegister( CHID_MAPINFO_VSO_WINDOW );
	Singleton<ICommandHandlerContainer>()->Remove( CHID_MAPINFO_VSO_WINDOW );
}


void CVSOWindow::DoDataExchange( CDataExchange* pDX )
{
	CResizeDialog::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_TMIVSO_FILTER_COMBO, wndFilterComboBox );
	DDX_Control( pDX, IDC_TMIVSO_OBJECT_LIST, wndObjectList );
}


BEGIN_MESSAGE_MAP(CVSOWindow, CResizeDialog)
	ON_WM_SIZE()
	ON_CBN_SELCHANGE(IDC_TMIVSO_FILTER_COMBO, OnSelchangeFilterComboBox)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_TMIVSO_OBJECT_LIST, OnItemchangedObjectList)
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()


BOOL CVSOWindow::OnInitDialog() 
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


int CVSOWindow::GetSelectedFilterIndex()
{
	const int nStringNumber = wndFilterComboBox.GetCurSel();
	if ( nStringNumber >= 0 )
	{
		CString strText;
		wndFilterComboBox.GetLBText( nStringNumber, strText );
		resizeDialogOptions.szParameters[0] = strText;
		return wndFilterComboBox.GetItemData( nStringNumber );
	}
	return INVALID_NODE_ID;
}


bool CVSOWindow::GetEditParameters( CVSOMultiState::SEditParameters *pEditParameters )
{
	NI_ASSERT( pEditParameters != 0, "CVSOWindow::GetEditParameters(), pEditParameters == 0" );

	if ( pEditParameters->nFlags & MIVSOSEP_POINT_NUMBER )
	{
		pEditParameters->ePointNumber = static_cast<CVSOMultiState::SEditParameters::EPointNumber>( GetCheckedRadioButton( IDC_TMIVSO_SINGLE_RADIO, IDC_TMIVSO_ALL_RATIO ) - IDC_TMIVSO_SINGLE_RADIO );
	}
	//
	if ( pEditParameters->nFlags & MIVSOSEP_STATS_TYPE )
	{
		pEditParameters->eStatsType = static_cast<CVSOMultiState::SEditParameters::EStatsType>( GetCheckedRadioButton( IDC_TMIVSO_PREDEFINED_STATS_RADIO, IDC_TMIVSO_CUSTOM_STATS_RADIO ) - IDC_TMIVSO_PREDEFINED_STATS_RADIO );
	}
	//
	if ( ( pEditParameters->nFlags & MIVSOSEP_WIDTH ) > 0 )
	{
		CString strText;
		GetDlgItemText( IDC_TMIVSO_WIDTH, strText );
		if ( ( sscanf( strText, "%g", &( pEditParameters->fWidth ) ) < 1 ) ||
				 ( pEditParameters->fWidth < 0.0f ) || 
				 ( pEditParameters->fWidth > ( AI_TILE_SIZE * 2.0f * 16.0f ) ) )
		{
			bCreateControls = true;
			if ( pEditParameters->fWidth < 0.0f )
			{
				pEditParameters->fWidth = CVSOManager::DEFAULT_WIDTH;
			}
			else if ( pEditParameters->fWidth > ( AI_TILE_SIZE * 2.0f * 16.0f ) )
			{
				pEditParameters->fWidth = CVSOManager::DEFAULT_WIDTH;
			}
			SetDlgItemText( IDC_TMIVSO_WIDTH, fmt::format( "{:g}", pEditParameters->fWidth ) );
			bCreateControls = false;
		}
	}
	//
	if ( ( pEditParameters->nFlags & MIVSOSEP_OPACITY ) > 0 )
	{
		if ( !bEnableHeight )
		{
			CString strText;
			GetDlgItemText( IDC_TMIVSO_OPACITY, strText );
			if ( ( sscanf( strText, "%g", &( pEditParameters->fOpacity ) ) < 1 ) ||
					( pEditParameters->fOpacity < 0.0f ) || 
					( pEditParameters->fOpacity > 100.0f ) )
			{
				bCreateControls = true;
				if ( pEditParameters->fOpacity < 0.0f )
				{
					pEditParameters->fOpacity = 0.0f;
					SetDlgItemText( IDC_TMIVSO_OPACITY, fmt::format( "{:g}", pEditParameters->fOpacity ) );
				}
				else if ( pEditParameters->fOpacity > 100.0f )
				{
					pEditParameters->fOpacity = 100.0f;
					SetDlgItemText( IDC_TMIVSO_OPACITY, fmt::format( "{:g}", pEditParameters->fOpacity ) );
					pEditParameters->fOpacity = 1.0f;
				}
				else
				{
					SetDlgItemText( IDC_TMIVSO_OPACITY, fmt::format( "{:g}", pEditParameters->fOpacity ) );
				}
				bCreateControls = false;
			}
			else
			{
				pEditParameters->fOpacity = pEditParameters->fOpacity / 100.f;
			}
		}
		else
		{
			pEditParameters->fOpacity = CVSOManager::DEFAULT_OPACITY;
		}
	}
	//
	if ( ( pEditParameters->nFlags & MIVSOSEP_HEIGHT ) > 0 )
	{
		if ( bEnableHeight )
		{
			CString strText;
			GetDlgItemText( IDC_TMIVSO_OPACITY, strText );
			if ( ( sscanf( strText, "%g", &( pEditParameters->fHeight ) ) < 1 ) ||
					( pEditParameters->fHeight < 0.0f ) || 
					( pEditParameters->fHeight > ( AI_TILE_SIZE * 2.0f * 16.0f ) ) )
			{
				bCreateControls = true;
				if ( pEditParameters->fHeight < 0.0f )
				{
					pEditParameters->fHeight = 0.0f;
				}
				else if ( pEditParameters->fHeight > ( AI_TILE_SIZE * 2.0f * 16.0f ) )
				{
					pEditParameters->fHeight = AI_TILE_SIZE * 2.0f * 16.0f;
				}
				SetDlgItemText( IDC_TMIVSO_OPACITY, fmt::format( "{:g}", pEditParameters->fHeight ) );
				bCreateControls = false;
			}
		}
		else
		{
			pEditParameters->fHeight = CVSOManager::DEFAULT_OPACITY;
		}
	}
	//
	if ( ( pEditParameters->nFlags & MIVSOSEP_THUMBNAILS ) > 0 )
	{
		pEditParameters->bThumbnails = ( nStyle == LVS_ICON );
	}
	return true;
}


bool CVSOWindow::SetEditParameters( const CVSOMultiState::SEditParameters &rEditParameters )
{
	if ( rEditParameters.nFlags & MIVSOSEP_POINT_NUMBER )
	{
		CheckRadioButton( IDC_TMIVSO_SINGLE_RADIO, IDC_TMIVSO_ALL_RATIO, IDC_TMIVSO_SINGLE_RADIO + rEditParameters.ePointNumber );
	}
	//
	if ( rEditParameters.nFlags & MIVSOSEP_STATS_TYPE )
	{
		CheckRadioButton( IDC_TMIVSO_PREDEFINED_STATS_RADIO, IDC_TMIVSO_CUSTOM_STATS_RADIO, IDC_TMIVSO_PREDEFINED_STATS_RADIO + rEditParameters.eStatsType );
	}
	//
	if ( ( rEditParameters.nFlags & MIVSOSEP_WIDTH ) > 0 )
	{
		SetDlgItemText( IDC_TMIVSO_WIDTH, fmt::format( "{:.2f}", rEditParameters.fWidth ) );
	}
	//
	if ( ( rEditParameters.nFlags & MIVSOSEP_OPACITY ) > 0 )
	{
		if ( !bEnableHeight )
		{
			SetDlgItemText( IDC_TMIVSO_OPACITY, fmt::format( "{:.2f}", rEditParameters.fOpacity * 100.0f ) );
		}
	}
	//
	if ( ( rEditParameters.nFlags & MIVSOSEP_HEIGHT ) > 0 )
	{
		if ( bEnableHeight )
		{
			SetDlgItemText( IDC_TMIVSO_OPACITY, fmt::format( "{:.2f}", rEditParameters.fHeight ) );
		}
	}
	if ( ( rEditParameters.nFlags & MIVSOSEP_THUMBNAILS ) > 0 )
	{
		SetObjectsListStyle( rEditParameters.bThumbnails ? LVS_ICON : LVS_LIST );
	}
	return true;
}


void CVSOWindow::UpdateObjectsListStyle()
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


void CVSOWindow::SetObjectsListStyle( int _nStyle )
{
	nStyle = _nStyle;
	UpdateObjectsListStyle();
}


void CVSOWindow::FillFilterComboBox()
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
			else if ( filterList.size() > 6 )
			{
				wndFilterComboBox.SetCurSel( 5 );
			}
		}
	}

	bCreateControls = false;
}


void CVSOWindow::FillObjectList()
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


void CVSOWindow::OnSetFocus( CWnd* pOldWnd )
{
	Singleton<ICommandHandlerContainer>()->Set( CHID_OBJECT_STORAGE, this );
}


void CVSOWindow::UpdateSelection()
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
							//DebugTrace( "CVSOWindow::UpdateSelection: [%d] = %s:%d", nItemIndex, selectedObjectListElement.szObjectTypeName.c_str(), selectedObjectListElement.nObjectID );
							//
							ICommandHandlerContainer* pCommandHandlerContainer = Singleton<ICommandHandlerContainer>();
							pCommandHandlerContainer->Set( CHID_OBJECT_STORAGE, this );
							pCommandHandlerContainer->HandleCommand( CHID_MAPINFO_VSO_MULTI_STATE, ID_MIVSO_SWITCH_MULTI_STATE, reinterpret_cast<uint32_t>( &( selectedObjectListElement.szObjectTypeName ) ) );
							pCommandHandlerContainer->HandleCommand( CHID_MAPINFO_VSO_STATE, ID_MIVSO_SWITCH_ADD_STATE, 0 );	
						}
					}
				}
			}
		}
	}
}


void CVSOWindow::ClearSelection()
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


void CVSOWindow::EnableHeight( bool bEnableHeight )
{
}


bool CVSOWindow::HandleCommand( unsigned nCommandID, uint32_t dwData )
{
	switch( nCommandID )
	{
		case ID_GET_EDIT_PARAMETERS:
		{
			CVSOMultiState::SEditParameters *pEditParameters = reinterpret_cast<CVSOMultiState::SEditParameters*>( dwData );
			if ( pEditParameters != 0 )
			{
				return GetEditParameters( pEditParameters );
			}
			return false;
		}
		case ID_SET_EDIT_PARAMETERS:
		{
			const CVSOMultiState::SEditParameters *pEditParameters = reinterpret_cast<const CVSOMultiState::SEditParameters*>( dwData );
			if ( pEditParameters != 0 )
			{
				return SetEditParameters( *pEditParameters );
			}
			return false;
		}
		case ID_MIVSO_CLEAR_SELECTION:
		{
			ClearSelection();
			return true;
		}
		case ID_MIVSO_ENABLE_HEIGHT:
			EnableHeight( dwData > 0 );
			return true;
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
		case ID_MIVSOOLCM_LIST:
			SetObjectsListStyle( LVS_LIST );
			return true;
		case ID_MIVSOOLCM_THUMBNAILS:
			SetObjectsListStyle( LVS_ICON );
			return true;
		case ID_MIVSOOLCM_PROPERTIES:
		{
			CPtr<IManipulator> pObjectManipulator = 0;
			SObjectSet objectSet;
			objectSet.szObjectTypeName = selectedObjectListElement.szObjectTypeName;
			InsertHashSetElement( &( objectSet.objectNameSet ), selectedObjectListElement.objectDBID );
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
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, ID_PC_DIALOG_GET_VIEW, reinterpret_cast<uint32_t>( &pView ) );
			if ( pView != 0 )
			{
				pView->SetViewManipulator( pObjectManipulator, objectSet, std::string() );
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


bool CVSOWindow::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CVSOWindow::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CVSOWindow::UpdateCommand(), pbCheck == 0" );
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
		case ID_MIVSO_CLEAR_SELECTION:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		case ID_MIVSO_ENABLE_HEIGHT:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		case ID_OS_GET_OBJECTSET:
			( *pbEnable ) = ( ( !selectedObjectListElement.szObjectTypeName.empty() ) && ( !selectedObjectListElement.objectDBID.IsEmpty() ) );
			( *pbCheck ) = false;
			return true;
		case ID_MIVSOOLCM_LIST:
			( *pbEnable ) = true;
			( *pbCheck ) = ( nStyle == LVS_LIST );
			return true;
		case ID_MIVSOOLCM_THUMBNAILS:
			( *pbEnable ) = true;
			( *pbCheck ) = ( nStyle == LVS_ICON );
			return true;
		case ID_MIVSOOLCM_PROPERTIES:
			( *pbEnable ) = ( ( !selectedObjectListElement.szObjectTypeName.empty() ) && ( !selectedObjectListElement.objectDBID.IsEmpty() ) );
			( *pbCheck ) = false;
			return true;
		default:
			return false;
	}
	return false;
}


void CVSOWindow::OnSize( unsigned nType, int cx, int cy ) 
{
	CResizeDialog::OnSize( nType, cx, cy );
	//
	UpdateObjectsListStyle();
}


void CVSOWindow::OnSelchangeFilterComboBox()
{
	if ( !bCreateControls )
	{
		FillObjectList();
		SaveResizeDialogOptions();
	}
}


void CVSOWindow::OnItemchangedObjectList( NMHDR* pNMHDR, LRESULT* pResult )
{
	UpdateSelection();
}


void CVSOWindow::OnContextMenu( CWnd *pwnd, CPoint point )
{
	CResizeDialog::OnContextMenu( pwnd, point );

	if ( pwnd->m_hWnd == wndObjectList.m_hWnd )
	{
		CMenu mainPopupMenu;
		AfxSetResourceHandle( theEDB2M1Instance );
		mainPopupMenu.LoadMenu( IDM_MAPINFO_CONTEXT_MENU );
		AfxSetResourceHandle( AfxGetInstanceHandle() );
		CMenu *pMenu = mainPopupMenu.GetSubMenu( MICM_VSO_OBJECT_LIST );
		if ( pMenu )
		{
			pMenu->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x, point.y, Singleton<IMainFrameContainer>()->GetSECWorkbook(), 0 );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
		}
		mainPopupMenu.DestroyMenu();
	}
}

// basement storage  


