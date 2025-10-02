#include "stdafx.h"

#include "../mapeditorlib/commandhandlerdefines.h"
#include "../mapeditorlib/resourcedefines.h"
#include "../misc/2darray.h"
#include "../stats_b2_m1/iconsset.h"
#include "../sceneb2/scene.h"
#include "../mapeditorlib/multimanipulator.h"

#include "HeightWindowV3.h"

#include "../libdb/ResourceManager.h"
#include "../MapEditorLib/Tools_HashSet.h"
#include "../MapEditorLib/EditParameter.h"
#include "../MapEditorLib/Interface_MainFrame.h"
#include "../MapEditorLib/Interface_ObjectCollector.h"

#include <zconf.h>

const char CHeightWindowV3::FILTER_TYPE[] = "TILE";
const char CHeightWindowV3::EXTRACTOR_TYPE[] = "TILE";
const char CHeightWindowV3::TILE_TYPE_NAME[] = "TGTerraType";


#define TMITH_TILE					0
#define TMITH_UP						1
#define TMITH_DOWN					2
#define TMITH_ROUND					3
#define TMITH_PLATO					4
#define TMITH_CIRCLE				5
#define TMITH_SQUARE				6
#define TMITH_BRUSH_SIZE_C0	7
#define TMITH_BRUSH_SIZE_C1	8
#define TMITH_BRUSH_SIZE_C2	9
#define TMITH_BRUSH_SIZE_C3	10
#define TMITH_BRUSH_SIZE_C4	11
#define TMITH_BRUSH_SIZE_R0	12
#define TMITH_BRUSH_SIZE_R1	13
#define TMITH_BRUSH_SIZE_R2	14
#define TMITH_BRUSH_SIZE_R3	15
#define TMITH_BRUSH_SIZE_R4	16
#define TMITH_COUNT					17


CHeightWindowV3::CHeightWindowV3( CWnd* pParent )
	: CResizeDialog( CHeightWindowV3::IDD, pParent ), bCreateControls( true ), nLastIndex( -1 )
{
	SetControlStyle( IDC_TMITHV3_BRUSH_LABEL, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_TMITHV3_BRUSH_UP, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_TMITHV3_BRUSH_UP, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_TMITHV3_BRUSH_DOWN, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_TMITHV3_BRUSH_ROUND, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_TMITHV3_BRUSH_PLATO, ANCHORE_LEFT_TOP );
	//
	SetControlStyle( IDC_TMITHV3_BRUSH_SIZE_LABEL, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_TMITHV3_BRUSH_SIZE_0, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_TMITHV3_BRUSH_SIZE_1, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_TMITHV3_BRUSH_SIZE_2, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_TMITHV3_BRUSH_SIZE_3, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_TMITHV3_BRUSH_SIZE_4, ANCHORE_LEFT_TOP );
	//
	SetControlStyle( IDC_TMITHV3_BRUSH_TYPE_LABEL, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_TMITHV3_BRUSH_TYPE_CIRCLE, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_TMITHV3_BRUSH_TYPE_SQUARE, ANCHORE_LEFT_TOP );
	//
	SetControlStyle( IDC_TMITHV3_UPDATE_HEIGHTS, ANCHORE_LEFT_TOP );
	//
	SetControlStyle( IDC_TMITHV3_TILE_LABEL, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_TMITHV3_TILE_LIST, ANCHORE_LEFT_TOP | RESIZE_HOR_VER );
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, this );
	Singleton<ICommandHandlerContainer>()->Register( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_LIST, ID_MITHV3_PROPERTIES );
}


CHeightWindowV3::~CHeightWindowV3()
{
	Singleton<ICommandHandlerContainer>()->UnRegister( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3 );
	Singleton<ICommandHandlerContainer>()->Remove( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3 );
}


void CHeightWindowV3::DoDataExchange( CDataExchange* pDX )
{
	CResizeDialog::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_TMITHV3_BRUSH_TILE, wndBrushTileButton );
	DDX_Control( pDX, IDC_TMITHV3_BRUSH_UP, wndBrushUpButton );
	DDX_Control( pDX, IDC_TMITHV3_BRUSH_DOWN, wndBrushDownButton );
	DDX_Control( pDX, IDC_TMITHV3_BRUSH_ROUND, wndBrushRoundButton );
	DDX_Control( pDX, IDC_TMITHV3_BRUSH_PLATO, wndBrushPlatoButton );
	DDX_Control( pDX, IDC_TMITHV3_BRUSH_SIZE_0, wndBrushSize0Button );
	DDX_Control( pDX, IDC_TMITHV3_BRUSH_SIZE_1, wndBrushSize1Button );
	DDX_Control( pDX, IDC_TMITHV3_BRUSH_SIZE_2, wndBrushSize2Button );
	DDX_Control( pDX, IDC_TMITHV3_BRUSH_SIZE_3, wndBrushSize3Button );
	DDX_Control( pDX, IDC_TMITHV3_BRUSH_SIZE_4, wndBrushSize4Button );
	DDX_Control( pDX, IDC_TMITHV3_BRUSH_TYPE_CIRCLE, wndBrushTypeCircleButton );
	DDX_Control( pDX, IDC_TMITHV3_BRUSH_TYPE_SQUARE, wndBrushTypeSquareButton );
	DDX_Control( pDX, IDC_TMITHV3_TILE_LIST, wndTileList );
}

BEGIN_MESSAGE_MAP(CHeightWindowV3, CResizeDialog)
	ON_WM_TIMER()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_BN_CLICKED(IDC_TMITHV3_BRUSH_TILE, OnBrushRadio)
	ON_BN_CLICKED(IDC_TMITHV3_BRUSH_UP, OnBrushRadio)
	ON_BN_CLICKED(IDC_TMITHV3_BRUSH_DOWN, OnBrushRadio)
	ON_BN_CLICKED(IDC_TMITHV3_BRUSH_ROUND, OnBrushRadio)
	ON_BN_CLICKED(IDC_TMITHV3_BRUSH_PLATO, OnBrushRadio)
	ON_BN_CLICKED(IDC_TMITHV3_BRUSH_SIZE_0, OnBrushSizeRadio)
	ON_BN_CLICKED(IDC_TMITHV3_BRUSH_SIZE_1, OnBrushSizeRadio)
	ON_BN_CLICKED(IDC_TMITHV3_BRUSH_SIZE_2, OnBrushSizeRadio)
	ON_BN_CLICKED(IDC_TMITHV3_BRUSH_SIZE_3, OnBrushSizeRadio)
	ON_BN_CLICKED(IDC_TMITHV3_BRUSH_SIZE_4, OnBrushSizeRadio)
	ON_BN_CLICKED(IDC_TMITHV3_BRUSH_TYPE_CIRCLE, OnBrushTypeRadio)
	ON_BN_CLICKED(IDC_TMITHV3_BRUSH_TYPE_SQUARE, OnBrushTypeRadio)
	ON_BN_CLICKED(IDC_TMITHV3_UPDATE_HEIGHTS, OnUpdateHeights)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_TMITHV3_TILE_LIST, OnItemchangedTileList)
END_MESSAGE_MAP()


BOOL CHeightWindowV3::OnInitDialog() 
{
	CResizeDialog::OnInitDialog();
	//
	imageList.Create( 32, 32, ILC_MASK, TMITH_COUNT, 1 );
	CBitmap bmp;
	bmp.LoadBitmap( IDB_TMITH_BITMAP );
	imageList.Add( &bmp, RGB( 255, 0, 255 ) );
	bmp.DeleteObject();
	//
	wndBrushTileButton.SetIcon( imageList.ExtractIcon( TMITH_TILE ) );
	wndBrushUpButton.SetIcon( imageList.ExtractIcon( TMITH_UP ) );
	wndBrushDownButton.SetIcon( imageList.ExtractIcon( TMITH_DOWN ) );
	wndBrushRoundButton.SetIcon( imageList.ExtractIcon( TMITH_ROUND ) );
	wndBrushPlatoButton.SetIcon( imageList.ExtractIcon( TMITH_PLATO ) );
	//
	UpdateSizeButtons( CHeightStateV3::SEditParameters::BT_CIRCLE );
	//
	wndBrushTypeCircleButton.SetIcon( imageList.ExtractIcon( TMITH_CIRCLE ) );
	wndBrushTypeSquareButton.SetIcon( imageList.ExtractIcon( TMITH_SQUARE ) );
	//	
	SetTileListStyle( LVS_ICON );
	wndTileList.SetImageList( Singleton<IObjectCollector>()->GetImageList( LVSIL_NORMAL ), LVSIL_NORMAL );
	wndTileList.SetImageList( Singleton<IObjectCollector>()->GetImageList( LVSIL_SMALL ), LVSIL_SMALL );

	EnableToolTips( true );
	bCreateControls = false;
	return true;
}


void CHeightWindowV3::UpdateSizeButtons( CHeightStateV3::SEditParameters::EBrushType eBrushType )
{
	if ( eBrushType == CHeightStateV3::SEditParameters::BT_CIRCLE )
	{
		wndBrushSize0Button.SetIcon( imageList.ExtractIcon( TMITH_BRUSH_SIZE_C0 ) );
		wndBrushSize1Button.SetIcon( imageList.ExtractIcon( TMITH_BRUSH_SIZE_C1 ) );
		wndBrushSize2Button.SetIcon( imageList.ExtractIcon( TMITH_BRUSH_SIZE_C2 ) );
		wndBrushSize3Button.SetIcon( imageList.ExtractIcon( TMITH_BRUSH_SIZE_C3 ) );
		wndBrushSize4Button.SetIcon( imageList.ExtractIcon( TMITH_BRUSH_SIZE_C4 ) );
	}
	else
	{
		wndBrushSize0Button.SetIcon( imageList.ExtractIcon( TMITH_BRUSH_SIZE_R0 ) );
		wndBrushSize1Button.SetIcon( imageList.ExtractIcon( TMITH_BRUSH_SIZE_R1 ) );
		wndBrushSize2Button.SetIcon( imageList.ExtractIcon( TMITH_BRUSH_SIZE_R2 ) );
		wndBrushSize3Button.SetIcon( imageList.ExtractIcon( TMITH_BRUSH_SIZE_R3 ) );
		wndBrushSize4Button.SetIcon( imageList.ExtractIcon( TMITH_BRUSH_SIZE_R4 ) );
	}
}


bool CHeightWindowV3::GetEditParameters( CHeightStateV3::SEditParameters *pEditParameters )
{
	NI_ASSERT( pEditParameters != 0, "CHeightWindowV3::GetEditParameters(), pEditParameters == 0" );
	//
	if ( pEditParameters->nFlags & MITHV3EP_BRUSH )
	{
		pEditParameters->eBrush = static_cast<CHeightStateV3::SEditParameters::EBrush>( GetCheckedRadioButton( IDC_TMITHV3_BRUSH_TILE, IDC_TMITHV3_BRUSH_PLATO ) - IDC_TMITHV3_BRUSH_TILE );
	}
	if ( pEditParameters->nFlags & MITHV3EP_BRUSH_SIZE )
	{
		pEditParameters->eBrushSize = static_cast<CHeightStateV3::SEditParameters::EBrushSize>( GetCheckedRadioButton( IDC_TMITHV3_BRUSH_SIZE_0, IDC_TMITHV3_BRUSH_SIZE_4 ) - IDC_TMITHV3_BRUSH_SIZE_0 );
	}
	if ( pEditParameters->nFlags & MITHV3EP_BRUSH_TYPE )
	{
		pEditParameters->eBrushType = static_cast<CHeightStateV3::SEditParameters::EBrushType>( GetCheckedRadioButton( IDC_TMITHV3_BRUSH_TYPE_CIRCLE, IDC_TMITHV3_BRUSH_TYPE_SQUARE ) - IDC_TMITHV3_BRUSH_TYPE_CIRCLE );
	}
	if ( pEditParameters->nFlags & ( MITHV3EP_TILE_COUNT | MITHV3EP_TILE_INDEX ) )
	{
		GetListEditParameters( &( pEditParameters->tileList ),
													 &( pEditParameters->nTileIndex ),
													 wndTileList,
													 ( pEditParameters->nFlags & MITHV3EP_TILE_COUNT ),
													 ( pEditParameters->nFlags & MITHV3EP_TILE_INDEX ) );
		nLastIndex = pEditParameters->nTileIndex;
	}
	//
	if ( ( pEditParameters->nFlags & MITHV3EP_THUMBNAILS ) > 0 )
	{
		pEditParameters->bThumbnails = ( nStyle == LVS_ICON );
	}
	if ( pEditParameters->nFlags & MITHV3EP_UPDATE_HEIGHT )
	{
		pEditParameters->bUpdateHeight = IsDlgButtonChecked( IDC_TMITHV3_UPDATE_HEIGHTS );
	}
	return true;
}


bool CHeightWindowV3::SetEditParameters( const CHeightStateV3::SEditParameters &rEditParameters )
{
	bCreateControls = true;
	if ( rEditParameters.nFlags & MITHV3EP_BRUSH )
	{
		CheckRadioButton( IDC_TMITHV3_BRUSH_TILE, IDC_TMITHV3_BRUSH_PLATO, rEditParameters.eBrush + IDC_TMITHV3_BRUSH_TILE );
		/**
		nLastIndex = rEditParameters.nTileIndex;
		SetListEditParameters( vector<string>(),
													 ( rEditParameters.eBrush == CHeightStateV3::SEditParameters::B_TILE ) ? rEditParameters.nTileIndex : -1,
													 &wndTileList,
													 TILE_TYPE_NAME,
													 &tileList,
													 false,
													 true );
		/**/
	}
	if ( rEditParameters.nFlags & MITHV3EP_BRUSH_SIZE )
	{
		CheckRadioButton( IDC_TMITHV3_BRUSH_SIZE_0, IDC_TMITHV3_BRUSH_SIZE_4, rEditParameters.eBrushSize + IDC_TMITHV3_BRUSH_SIZE_0 );
	}
	if ( rEditParameters.nFlags & MITHV3EP_BRUSH_TYPE )
	{
		CheckRadioButton( IDC_TMITHV3_BRUSH_TYPE_CIRCLE, IDC_TMITHV3_BRUSH_TYPE_SQUARE, rEditParameters.eBrushType + IDC_TMITHV3_BRUSH_TYPE_CIRCLE );
		UpdateSizeButtons( rEditParameters.eBrushType );
	}
	if ( rEditParameters.nFlags & ( MITHV3EP_TILE_COUNT | MITHV3EP_TILE_INDEX ) )
	{
		nLastIndex = rEditParameters.nTileIndex;
		SetListEditParameters( rEditParameters.tileList,
													 rEditParameters.nTileIndex,
													 &wndTileList,
													 TILE_TYPE_NAME,
													 &tileList,
													 rEditParameters.nFlags & MITHV3EP_TILE_COUNT,
													 rEditParameters.nFlags & MITHV3EP_TILE_INDEX );
	}
	if ( ( rEditParameters.nFlags & MITHV3EP_THUMBNAILS ) > 0 )
	{
		SetTileListStyle( rEditParameters.bThumbnails ? LVS_ICON : LVS_LIST );
	}
	if ( rEditParameters.nFlags & MITHV3EP_UPDATE_HEIGHT )
	{
		CheckDlgButton( IDC_TMITHV3_UPDATE_HEIGHTS, rEditParameters.bUpdateHeight );
	}
	bCreateControls = false;
	return true;
}


void CHeightWindowV3::UpdateTileListStyle()
{
	if ( ::IsWindow( wndTileList.m_hWnd ) )
	{
		wndTileList.ModifyStyle( LVS_TYPEMASK, nStyle );
		if ( nStyle == LVS_ICON )
		{
			wndTileList.SetIconSpacing( NORMAL_IMAGE_SIZE_X + NORMAL_IMAGE_SPACE_X, NORMAL_IMAGE_SIZE_Y + NORMAL_IMAGE_SPACE_Y );
		}
		wndTileList.Arrange( LVA_DEFAULT );
	}
}


void CHeightWindowV3::SetTileListStyle( int _nStyle )
{
	nStyle = _nStyle;
	UpdateTileListStyle();
}


void CHeightWindowV3::OnSize( UINT nType, int cx, int cy )
{
	CResizeDialog::OnSize( nType, cx, cy );
	//
	UpdateTileListStyle();
}


void CHeightWindowV3::OnContextMenu( CWnd *pwnd, CPoint point )
{
	CResizeDialog::OnContextMenu( pwnd, point );

	if ( pwnd->m_hWnd == wndTileList.m_hWnd )
	{
		CMenu mainPopupMenu;
		AfxSetResourceHandle( theEDB2M1Instance );
		mainPopupMenu.LoadMenu( IDM_MAPINFO_CONTEXT_MENU );
		AfxSetResourceHandle( AfxGetInstanceHandle() );
		CMenu *pMenu = mainPopupMenu.GetSubMenu( MICM_TERRAIN_HEIGHT_STATE_V3_TILE_LIST );
		if ( pMenu )
		{
			pMenu->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x, point.y, Singleton<IMainFrameContainer>()->GetSECWorkbook(), 0 );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
		}
		mainPopupMenu.DestroyMenu();
	}
}


void CHeightWindowV3::OnBrushRadio()
{
	if ( !bCreateControls )
	{
		CHeightStateV3::SEditParameters::EBrush eBrush = static_cast<CHeightStateV3::SEditParameters::EBrush>( GetCheckedRadioButton( IDC_TMITHV3_BRUSH_TILE, IDC_TMITHV3_BRUSH_PLATO ) - IDC_TMITHV3_BRUSH_TILE );
		bCreateControls = true;
		SetListEditParameters( vector<string>(),
													 ( eBrush == CHeightStateV3::SEditParameters::B_TILE ) ? nLastIndex : -1,
													 &wndTileList,
													 TILE_TYPE_NAME,
													 &tileList,
													 false,
													 true );
		bCreateControls = false;
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_STATE_V3, ID_GET_EDIT_PARAMETERS, MITHV3EP_BRUSH );
	}
}


void CHeightWindowV3::OnBrushSizeRadio()
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_STATE_V3, ID_GET_EDIT_PARAMETERS, MITHV3EP_BRUSH_SIZE );
	}
}


void CHeightWindowV3::OnBrushTypeRadio()
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_STATE_V3, ID_GET_EDIT_PARAMETERS, MITHV3EP_BRUSH_TYPE );
		CHeightStateV3::SEditParameters::EBrushType eBrushType = static_cast<CHeightStateV3::SEditParameters::EBrushType>( GetCheckedRadioButton( IDC_TMITHV3_BRUSH_TYPE_CIRCLE, IDC_TMITHV3_BRUSH_TYPE_SQUARE ) - IDC_TMITHV3_BRUSH_TYPE_CIRCLE );
		UpdateSizeButtons( eBrushType );
	}
}


void CHeightWindowV3::OnUpdateHeights()
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_STATE_V3, ID_GET_EDIT_PARAMETERS, MITHV3EP_UPDATE_HEIGHT );
	}
}


void CHeightWindowV3::OnItemchangedTileList( NMHDR* pNMHDR, LRESULT* pResult )
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_STATE_V3, ID_GET_EDIT_PARAMETERS, MITHV3EP_TILE_INDEX );
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CHeightWindowV3::OnTimer( UINT nIDEvent ) 
{
  if ( nIDEvent == GetHeightID() )
	{
		OnHeightTimer();
	}
	CResizeDialog::OnTimer( nIDEvent );
}


void CHeightWindowV3::SetHeightTimer()
{
  KillHeightTimer();
  nHeightTimer = SetTimer( GetHeightID(), GetHeightTimerInterval(), 0 );
}


void CHeightWindowV3::KillHeightTimer()
{
  if ( nHeightTimer != 0 )
	{
		KillTimer( nHeightTimer );
	}
  nHeightTimer = 0;
}


void CHeightWindowV3::OnHeightTimer()
{
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_STATE_V3, ID_MITHV3_ON_TIMER, dwHeightData );
}


bool CHeightWindowV3::HandleCommand( UINT nCommandID, DWORD dwData )
{
	switch( nCommandID )
	{
		case ID_GET_EDIT_PARAMETERS:
		{
			CHeightStateV3::SEditParameters *pEditParameters = reinterpret_cast<CHeightStateV3::SEditParameters*>( dwData );
			if ( pEditParameters != 0 )
			{
				return GetEditParameters( pEditParameters );
			}
			return false;
		}
		case ID_SET_EDIT_PARAMETERS:
		{
			const CHeightStateV3::SEditParameters *pEditParameters = reinterpret_cast<const CHeightStateV3::SEditParameters*>( dwData );
			if ( pEditParameters != 0 )
			{
				return SetEditParameters( *pEditParameters );
			}
			return false;
		}
		case ID_MITHV3_SET_TIMER:
		{
			SetHeightTimer();
			dwHeightData = dwData;
			return true;
		}
		case ID_MITHV3_KILL_TIMER:
		{
			KillHeightTimer();
			return true;
		}
		case ID_MITHV3_LIST:
			SetTileListStyle( LVS_LIST );
			return true;
		case ID_MITHV3_THUMBNAILS:
			SetTileListStyle( LVS_ICON );
			return true;
		case ID_MITHV3_PROPERTIES:
		{
			int nListIndex = GetSelectedListIndex( wndTileList );
			if ( ( nListIndex >= 0 ) && ( nListIndex <  tileList.size() ) )
			{
				IResourceManager *pResourceManager = Singleton<IResourceManager>();
				CPtr<IManipulator> pObjectManipulator = 0;
				SObjectSet objectSet;
				objectSet.szObjectTypeName = TILE_TYPE_NAME;
				InsertHashSetElement( &( objectSet.objectNameSet ), tileList[nListIndex] );
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
		}
		default:
			return false;
	}
	return false;
}


bool CHeightWindowV3::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CHeightWindowV3::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CHeightWindowV3::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID )
	{
		case ID_GET_EDIT_PARAMETERS:
		case ID_SET_EDIT_PARAMETERS:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		case ID_MITHV3_SET_TIMER:
		case ID_MITHV3_KILL_TIMER:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		case ID_MITHV3_LIST:
			( *pbEnable ) = true;
			( *pbCheck ) = ( nStyle == LVS_LIST );
			return true;
		case ID_MITHV3_THUMBNAILS:
			( *pbEnable ) = true;
			( *pbCheck ) = ( nStyle == LVS_ICON );
			return true;
		case ID_MITHV3_PROPERTIES:
			( *pbEnable ) = ( GetSelectedListIndex( wndTileList ) != INVALID_NODE_ID );
			( *pbCheck ) = false;
			return true;
		default:
			return false;
	}
	return false;
}

// basement storage  


