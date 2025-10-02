#include "stdafx.h"
#include "..\mapeditorlib\commandhandlerdefines.h"
#include "..\mapeditorlib\resourcedefines.h"
#include "CommandHandlerDefines.h"
#include "ModelWindow.h"
#include "..\MapEditorLib\EditParameter.h"
#include "..\MapEditorLib\Interface_UserData.h"
#include "..\Image\ImageColor.h"
#include ".\modelwindow.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CModelWindow::CModelWindow( CWnd* pParent )
	: CResizeDialog( CModelWindow::IDD, pParent ),
		bCreateControls( true ),
		nFOVTimer( 0 ),
		nSceneColorTimer( 0 ),
		nTerrainColorTimer( 0 ),
		nTerrainColorOpacityTimer( 0 )
{
	SetControlStyle( IDC_MODEL_LIGHT_LABEL, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_MODEL_LIGHT_COMBO, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_MODEL_LIGHT_BUTTON, ANCHORE_RIGHT_TOP );
	SetControlStyle( IDC_MODEL_SCENE_COLOR_LABEL, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_MODEL_SCENE_COLOR_EDIT, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_MODEL_SCENE_COLOR_BUTTON, ANCHORE_RIGHT_TOP );
	SetControlStyle( IDC_MODEL_FOV_LABEL_LEFT, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_MODEL_FOV_EDIT, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_MODEL_FOV_LABEL_RIGHT, ANCHORE_RIGHT_TOP );
	//
	SetControlStyle( IDC_MODEL_DELIMITER_0, ANCHORE_LEFT_TOP | RESIZE_HOR );
	//
	SetControlStyle( IDC_MODEL_TERRAIN_CHECK, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_MODEL_TERRAIN_SIZE_LABEL, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_MODEL_TERRAIN_SIZE_COMBO, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_MODEL_TERRAIN_COLOR_LABEL, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_MODEL_TERRAIN_COLOR_EDIT, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_MODEL_TERRAIN_COLOR_BUTTON, ANCHORE_RIGHT_TOP );
	SetControlStyle( IDC_MODEL_TERRAIN_COLOR_OPACITY_LABEL_LEFT, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_MODEL_TERRAIN_COLOR_OPACITY_EDIT, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_MODEL_TERRAIN_COLOR_OPACITY_LABEL_RIGHT, ANCHORE_RIGHT_TOP );
	SetControlStyle( IDC_MODEL_TERRAIN_DOUBLESIDED_CHECK, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_MODEL_TERRAIN_GRID_CHECK, ANCHORE_LEFT_TOP | RESIZE_HOR );
	//
	SetControlStyle( IDC_MODEL_DELIMITER_1, ANCHORE_LEFT_TOP | RESIZE_HOR );
	//
	SetControlStyle( IDC_MODEL_ANIM_CHECK, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_MODEL_ANIM_COUNT_LABEL, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_MODEL_ANIM_COUNT_COMBO, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_MODEL_ANIM_SPEED_LABEL, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_MODEL_ANIM_SPEED_COMBO, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_MODEL_ANIM_SPEED_BUTTON_DOWN, ANCHORE_RIGHT_TOP );
	SetControlStyle( IDC_MODEL_ANIM_SPEED_BUTTON_UP, ANCHORE_RIGHT_TOP );
	SetControlStyle( IDC_MODEL_ANIM_CIRCLE_RADIO, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_MODEL_ANIM_CIRCLE_RADIUS_LABEL, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_MODEL_ANIM_CIRCLE_RADIUS_COMBO, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_MODEL_ANIM_LINE_RADIO, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_MODEL_ANIM_LINE_DISTANCE_LABEL, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_MODEL_ANIM_LINE_DISTANCE_COMBO, ANCHORE_LEFT_TOP | RESIZE_HOR );
	//
	SetControlStyle( IDC_MODEL_DELIMITER_2, ANCHORE_LEFT_TOP | RESIZE_HOR );
	//
	SetControlStyle( IDC_MODEL_AI_GEOMETRY, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_MODEL_AI_GEOMETRY_TRANSPARENT_RADIO, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_MODEL_AI_GEOMETRY_SOLID_RADIO, ANCHORE_LEFT_TOP | RESIZE_HOR );
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_MODEL_WINDOW, this );
}


CModelWindow::~CModelWindow()
{
	Singleton<ICommandHandlerContainer>()->Remove( CHID_MODEL_WINDOW );
}


void CModelWindow::DoDataExchange( CDataExchange* pDX )
{
	CResizeDialog::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_MODEL_LIGHT_COMBO, wndLightComboBox );
	DDX_Control( pDX, IDC_MODEL_TERRAIN_SIZE_COMBO, wndTerrainSizeComboBox );
	DDX_Control( pDX, IDC_MODEL_ANIM_COUNT_COMBO, wndAnimCountComboBox );
	DDX_Control( pDX, IDC_MODEL_ANIM_SPEED_COMBO, wndAnimSpeedComboBox );
	DDX_Control( pDX, IDC_MODEL_ANIM_CIRCLE_RADIUS_COMBO, wndAnimRadiusComboBox );
	DDX_Control( pDX, IDC_MODEL_ANIM_LINE_DISTANCE_COMBO, wndAnimDistanceComboBox );
}


BEGIN_MESSAGE_MAP(CModelWindow, CResizeDialog)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_MODEL_LIGHT_BUTTON, OnClickedLightButton)
	ON_BN_CLICKED(IDC_MODEL_ANIM_SPEED_BUTTON_DOWN, OnClickedAnimSpeedDown)
	ON_BN_CLICKED(IDC_MODEL_ANIM_SPEED_BUTTON_UP, OnClickedAnimSpeedUp)
	ON_CBN_SELCHANGE(IDC_MODEL_LIGHT_COMBO, OnSelChangeLightCombo)
	ON_CBN_SELCHANGE(IDC_MODEL_TERRAIN_SIZE_COMBO, OnSelChangeTerrainSizeCombo)
	ON_CBN_SELCHANGE(IDC_MODEL_ANIM_COUNT_COMBO, OnSelChangeAnimCountCombo)
	ON_CBN_SELCHANGE(IDC_MODEL_ANIM_SPEED_COMBO, OnSelChangeAnimSpeedCombo)
	ON_CBN_SELCHANGE(IDC_MODEL_ANIM_CIRCLE_RADIUS_COMBO, OnSelChangeAnimCircleRadiusCombo)
	ON_CBN_SELCHANGE(IDC_MODEL_ANIM_LINE_DISTANCE_COMBO, OnSelChangeAnimLineDistanceCombo)
	ON_BN_CLICKED(IDC_MODEL_TERRAIN_CHECK, OnClickedTerrain)
	ON_BN_CLICKED(IDC_MODEL_TERRAIN_DOUBLESIDED_CHECK, OnClickedTerrainDoubleSided)
	ON_BN_CLICKED(IDC_MODEL_TERRAIN_GRID_CHECK, OnClickedTerrainGrid)
	ON_BN_CLICKED(IDC_MODEL_ANIM_CHECK, OnClickedAnim)
	ON_BN_CLICKED(IDC_MODEL_ANIM_CIRCLE_RADIO, OnClickedAnimType)
	ON_BN_CLICKED(IDC_MODEL_ANIM_LINE_RADIO, OnClickedAnimType)
	ON_BN_CLICKED(IDC_MODEL_AI_GEOMETRY, OnClickedAIGeometry)
	ON_BN_CLICKED(IDC_MODEL_AI_GEOMETRY_TRANSPARENT_RADIO, OnClickedAIGeometryType)
	ON_BN_CLICKED(IDC_MODEL_AI_GEOMETRY_SOLID_RADIO, OnClickedAIGeometryType)
	ON_EN_CHANGE(IDC_MODEL_SCENE_COLOR_EDIT, OnChangeSceneColor)
	ON_EN_CHANGE(IDC_MODEL_FOV_EDIT, OnChangeFOV)
	ON_EN_CHANGE(IDC_MODEL_TERRAIN_COLOR_EDIT, OnChangeTerrainColor)
	ON_EN_CHANGE(IDC_MODEL_TERRAIN_COLOR_OPACITY_EDIT, OnChangeTerrainColorOpacity)
	ON_BN_CLICKED(IDC_MODEL_SCENE_COLOR_BUTTON, OnClickedSceneColorButton)
	ON_BN_CLICKED(IDC_MODEL_TERRAIN_COLOR_BUTTON, OnClickedTerrainColorButton)
END_MESSAGE_MAP()


BOOL CModelWindow::OnInitDialog() 
{
	CResizeDialog::OnInitDialog();
	//
	bCreateControls = false;
	return true;
}

bool CModelWindow::GetEditParameters( CModelState::SEditParameters *pEditParameters )
{
	NI_ASSERT( pEditParameters != 0, "CModelWindow::GetEditParameters(), pEditParameters == 0" );
	//
	if ( pEditParameters->nFlags & ( MODEL_EP_LIGHT_COUNT | MODEL_EP_LIGHT_INDEX ) )
	{
		GetComboBoxEditParameters( &( pEditParameters->lightList ),
															 &( pEditParameters->nLightIndex ),
															 wndLightComboBox,
															 ( pEditParameters->nFlags & MODEL_EP_LIGHT_COUNT ),
															 ( pEditParameters->nFlags & MODEL_EP_LIGHT_INDEX ) );
	}
	if ( pEditParameters->nFlags & ( MODEL_EP_TERRAIN_SIZE_COUNT | MODEL_EP_TERRAIN_SIZE_INDEX ) )
	{
		GetComboBoxEditParameters( &( pEditParameters->terrainSizeList ),
															 &( pEditParameters->nTerrainSizeIndex ),
															 wndTerrainSizeComboBox,
															 ( pEditParameters->nFlags & MODEL_EP_TERRAIN_SIZE_COUNT ),
															 ( pEditParameters->nFlags & MODEL_EP_TERRAIN_SIZE_INDEX ) );
	}
	if ( pEditParameters->nFlags & ( MODEL_EP_ANIM_COUNT_COUNT | MODEL_EP_ANIM_COUNT_INDEX ) )
	{
		GetComboBoxEditParameters( &( pEditParameters->animCountList ),
															 &( pEditParameters->nAnimCountIndex ),
															 wndAnimCountComboBox,
															 ( pEditParameters->nFlags & MODEL_EP_ANIM_COUNT_COUNT ),
															 ( pEditParameters->nFlags & MODEL_EP_ANIM_COUNT_INDEX ) );
	}
	if ( pEditParameters->nFlags & ( MODEL_EP_ANIM_SPEED_COUNT | MODEL_EP_ANIM_SPEED_INDEX ) )
	{
		GetComboBoxEditParameters( &( pEditParameters->animSpeedList ),
															 &( pEditParameters->nAnimSpeedIndex ),
															 wndAnimSpeedComboBox,
															 ( pEditParameters->nFlags & MODEL_EP_ANIM_SPEED_COUNT ),
															 ( pEditParameters->nFlags & MODEL_EP_ANIM_SPEED_INDEX ) );
	}
	if ( pEditParameters->nFlags & ( MODEL_EP_ANIM_RADIUS_COUNT | MODEL_EP_ANIM_RADIUS_INDEX ) )
	{
		GetComboBoxEditParameters( &( pEditParameters->animRadiusList ),
															 &( pEditParameters->nAnimRadiusIndex ),
															 wndAnimRadiusComboBox,
															 ( pEditParameters->nFlags & MODEL_EP_ANIM_RADIUS_COUNT ),
															 ( pEditParameters->nFlags & MODEL_EP_ANIM_RADIUS_INDEX ) );
	}
	if ( pEditParameters->nFlags & ( MODEL_EP_ANIM_DISTANCE_COUNT | MODEL_EP_ANIM_DISTANCE_INDEX ) )
	{
		GetComboBoxEditParameters( &( pEditParameters->animDistanceList ),
															 &( pEditParameters->nAnimDistanceIndex ),
															 wndAnimDistanceComboBox,
															 ( pEditParameters->nFlags & MODEL_EP_ANIM_DISTANCE_COUNT ),
															 ( pEditParameters->nFlags & MODEL_EP_ANIM_DISTANCE_INDEX ) );
	}
	//
	if ( pEditParameters->nFlags & MODEL_EP_TERRAIN )
	{
		pEditParameters->bTerrain = ( IsDlgButtonChecked( IDC_MODEL_TERRAIN_CHECK ) == BST_CHECKED );
	}
	if ( pEditParameters->nFlags & MODEL_EP_TERRAIN_DOUBLESIDED )
	{
		pEditParameters->bTerrainDoubleSided = ( IsDlgButtonChecked( IDC_MODEL_TERRAIN_DOUBLESIDED_CHECK ) == BST_CHECKED );
	}
	if ( pEditParameters->nFlags & MODEL_EP_TERRAIN_GRID )
	{
		pEditParameters->bTerrainGrid = ( IsDlgButtonChecked( IDC_MODEL_TERRAIN_GRID_CHECK ) == BST_CHECKED );
	}
	if ( pEditParameters->nFlags & MODEL_EP_ANIM )
	{
		pEditParameters->bAnim = ( IsDlgButtonChecked( IDC_MODEL_ANIM_CHECK ) == BST_CHECKED );
	}
	if ( pEditParameters->nFlags & MODEL_EP_ANIM_TYPE )
	{
		pEditParameters->eAnimType = static_cast<CModelState::SEditParameters::EAnimType>( GetCheckedRadioButton( IDC_MODEL_ANIM_CIRCLE_RADIO, IDC_MODEL_ANIM_LINE_RADIO ) - IDC_MODEL_ANIM_CIRCLE_RADIO );
	}
	if ( pEditParameters->nFlags & MODEL_EP_AI_GEOMETRY )
	{
		pEditParameters->bAIGeometry = ( IsDlgButtonChecked( IDC_MODEL_AI_GEOMETRY ) == BST_CHECKED );
	}
	if ( pEditParameters->nFlags & MODEL_EP_AI_GEOMETRY_TYPE )
	{
		pEditParameters->eAIGeometryType = static_cast<CModelState::SEditParameters::EAIGeometryType>( GetCheckedRadioButton( IDC_MODEL_AI_GEOMETRY_TRANSPARENT_RADIO, IDC_MODEL_AI_GEOMETRY_SOLID_RADIO ) - IDC_MODEL_AI_GEOMETRY_TRANSPARENT_RADIO );
	}
	//
	if ( pEditParameters->nFlags & MODEL_EP_FOV )
	{
		CString strText;
		GetDlgItemText( IDC_MODEL_FOV_EDIT, strText );
		int nFOV = pEditParameters->nFOV;
		if ( ( sscanf( strText, "%d", &nFOV ) == 1 ) &&
				 ( nFOV > 0 ) && 
				 ( nFOV <= 179 ) )
		{
			pEditParameters->nFOV = nFOV;
		}
	}
	if ( pEditParameters->nFlags & MODEL_EP_TERRAIN_COLOR_OPACITY )
	{
		CString strText;
		GetDlgItemText( IDC_MODEL_TERRAIN_COLOR_OPACITY_EDIT, strText );
		int nTerrainColorOpacity = pEditParameters->nTerrainColorOpacity;
		if ( ( sscanf( strText, "%d", &nTerrainColorOpacity ) == 1 ) &&
				 ( nTerrainColorOpacity >= 0 ) && 
				 ( nTerrainColorOpacity < 256 ) )
		{
			pEditParameters->nTerrainColorOpacity = nTerrainColorOpacity;
		}
	}
	if ( pEditParameters->nFlags & MODEL_EP_COLOR )
	{
		CString strText;
		GetDlgItemText( IDC_MODEL_SCENE_COLOR_EDIT, strText );
		int r = 0;
		int g = 0;
		int b = 0;
		if ( ( sscanf( strText, "%d,%d,%d", &r, &g, &b ) == 3 ) &&
					( ( r >= 0 ) && ( r < 256 ) ) &&
					( ( g >= 0 ) && ( g < 256 ) ) &&
					( ( b >= 0 ) && ( b < 256 ) ) )
		{
			pEditParameters->vColor.r = r;
			pEditParameters->vColor.g = g;
			pEditParameters->vColor.b = b;
		}
	}
	if ( pEditParameters->nFlags & MODEL_EP_TERRAIN_COLOR )
	{
		CString strText;
		GetDlgItemText( IDC_MODEL_TERRAIN_COLOR_EDIT, strText );
		int r = 0;
		int g = 0;
		int b = 0;
		if ( ( sscanf( strText, "%d,%d,%d", &r, &g, &b ) == 3 ) &&
					( ( r >= 0 ) && ( r < 256 ) ) &&
					( ( g >= 0 ) && ( g < 256 ) ) &&
					( ( b >= 0 ) && ( b < 256 ) ) )
		{
			pEditParameters->vTerrainColor.r = r;
			pEditParameters->vTerrainColor.g = g;
			pEditParameters->vTerrainColor.b = b;
		}
	}
	return true;
}


bool CModelWindow::SetEditParameters( const CModelState::SEditParameters &rEditParameters )
{
	bCreateControls = true;
	if ( rEditParameters.nFlags & ( MODEL_EP_LIGHT_COUNT | MODEL_EP_LIGHT_INDEX ) )
	{
		SetComboBoxEditParameters( rEditParameters.lightList,
															 rEditParameters.nLightIndex,
															 &wndLightComboBox,
															 ( rEditParameters.nFlags & MODEL_EP_LIGHT_COUNT ),
															 ( rEditParameters.nFlags & MODEL_EP_LIGHT_INDEX ) );
	}
	if ( rEditParameters.nFlags & ( MODEL_EP_TERRAIN_SIZE_COUNT | MODEL_EP_TERRAIN_SIZE_INDEX ) )
	{
		SetComboBoxEditParameters( rEditParameters.terrainSizeList,
															 rEditParameters.nTerrainSizeIndex,
															 &wndTerrainSizeComboBox,
															 ( rEditParameters.nFlags & MODEL_EP_TERRAIN_SIZE_COUNT ),
															 ( rEditParameters.nFlags & MODEL_EP_TERRAIN_SIZE_INDEX ) );
	}
	if ( rEditParameters.nFlags & ( MODEL_EP_ANIM_COUNT_COUNT | MODEL_EP_ANIM_COUNT_INDEX ) )
	{
		SetComboBoxEditParameters( rEditParameters.animCountList,
															 rEditParameters.nAnimCountIndex,
															 &wndAnimCountComboBox,
															 ( rEditParameters.nFlags & MODEL_EP_ANIM_COUNT_COUNT ),
															 ( rEditParameters.nFlags & MODEL_EP_ANIM_COUNT_INDEX ) );
	}
	if ( rEditParameters.nFlags & ( MODEL_EP_ANIM_SPEED_COUNT | MODEL_EP_ANIM_SPEED_INDEX ) )
	{
		SetComboBoxEditParameters( rEditParameters.animSpeedList,
															 rEditParameters.nAnimSpeedIndex,
															 &wndAnimSpeedComboBox,
															 ( rEditParameters.nFlags & MODEL_EP_ANIM_SPEED_COUNT ),
															 ( rEditParameters.nFlags & MODEL_EP_ANIM_SPEED_INDEX ) );
	}
	if ( rEditParameters.nFlags & ( MODEL_EP_ANIM_RADIUS_COUNT | MODEL_EP_ANIM_RADIUS_INDEX ) )
	{
		SetComboBoxEditParameters( rEditParameters.animRadiusList,
															 rEditParameters.nAnimRadiusIndex,
															 &wndAnimRadiusComboBox,
															 ( rEditParameters.nFlags & MODEL_EP_ANIM_RADIUS_COUNT ),
															 ( rEditParameters.nFlags & MODEL_EP_ANIM_RADIUS_INDEX ) );
	}
	if ( rEditParameters.nFlags & ( MODEL_EP_ANIM_DISTANCE_COUNT | MODEL_EP_ANIM_DISTANCE_INDEX ) )
	{
		SetComboBoxEditParameters( rEditParameters.animDistanceList,
															 rEditParameters.nAnimDistanceIndex,
															 &wndAnimDistanceComboBox,
															 ( rEditParameters.nFlags & MODEL_EP_ANIM_DISTANCE_COUNT ),
															 ( rEditParameters.nFlags & MODEL_EP_ANIM_DISTANCE_INDEX ) );
	}
	//
	if ( rEditParameters.nFlags & MODEL_EP_TERRAIN )
	{
		CheckDlgButton( IDC_MODEL_TERRAIN_CHECK, rEditParameters.bTerrain ? BST_CHECKED : BST_UNCHECKED );
	}
	if ( rEditParameters.nFlags & MODEL_EP_TERRAIN_DOUBLESIDED )
	{
		CheckDlgButton( IDC_MODEL_TERRAIN_DOUBLESIDED_CHECK, rEditParameters.bTerrainDoubleSided ? BST_CHECKED : BST_UNCHECKED );
	}
	if ( rEditParameters.nFlags & MODEL_EP_TERRAIN_GRID )
	{
		CheckDlgButton( IDC_MODEL_TERRAIN_GRID_CHECK, rEditParameters.bTerrainGrid ? BST_CHECKED : BST_UNCHECKED );
	}
	if ( rEditParameters.nFlags & MODEL_EP_ANIM )
	{
		CheckDlgButton( IDC_MODEL_ANIM_CHECK, rEditParameters.bAnim ? BST_CHECKED : BST_UNCHECKED );
	}
	if ( rEditParameters.nFlags & MODEL_EP_ANIM_TYPE )
	{
		CheckRadioButton( IDC_MODEL_ANIM_CIRCLE_RADIO, IDC_MODEL_ANIM_LINE_RADIO, rEditParameters.eAnimType + IDC_MODEL_ANIM_CIRCLE_RADIO );
	}
	if ( rEditParameters.nFlags & MODEL_EP_AI_GEOMETRY )
	{
		CheckDlgButton( IDC_MODEL_AI_GEOMETRY, rEditParameters.bAIGeometry ? BST_CHECKED : BST_UNCHECKED );
	}
	if ( rEditParameters.nFlags & MODEL_EP_AI_GEOMETRY_TYPE )
	{
		CheckRadioButton( IDC_MODEL_AI_GEOMETRY_TRANSPARENT_RADIO, IDC_MODEL_AI_GEOMETRY_SOLID_RADIO, rEditParameters.eAIGeometryType + IDC_MODEL_AI_GEOMETRY_TRANSPARENT_RADIO );
	}
	//
	if ( rEditParameters.nFlags & MODEL_EP_FOV )
	{
		SetDlgItemText( IDC_MODEL_FOV_EDIT, StrFmt( "%d", rEditParameters.nFOV ) );
	}
	if ( rEditParameters.nFlags & MODEL_EP_TERRAIN_COLOR_OPACITY )
	{
		SetDlgItemText( IDC_MODEL_TERRAIN_COLOR_OPACITY_EDIT, StrFmt( "%d", rEditParameters.nTerrainColorOpacity ) );
	}
	if ( rEditParameters.nFlags & MODEL_EP_COLOR )
	{
		SetDlgItemText( IDC_MODEL_SCENE_COLOR_EDIT, StrFmt( "%d, %d, %d", (int)( rEditParameters.vColor.r ),(int)( rEditParameters.vColor.g ), (int)( rEditParameters.vColor.b ) ) );
	}
	if ( rEditParameters.nFlags & MODEL_EP_TERRAIN_COLOR )
	{
		SetDlgItemText( IDC_MODEL_TERRAIN_COLOR_EDIT, StrFmt( "%d, %d, %d", (int)( rEditParameters.vTerrainColor.r ), (int)( rEditParameters.vTerrainColor.g ), (int)( rEditParameters.vTerrainColor.b ) ) );
	}
	bCreateControls = false;
	UpdateControls( rEditParameters );
	return true;
}


void CModelWindow::UpdateControls( const CModelState::SEditParameters &rEditParameters )
{
	if ( CWnd *pWnd = GetDlgItem( IDC_MODEL_ANIM_SPEED_BUTTON_DOWN ) )
	{
		pWnd->EnableWindow( rEditParameters.nAnimSpeedIndex > 0 );
	}
	if ( CWnd *pWnd = GetDlgItem( IDC_MODEL_ANIM_SPEED_BUTTON_UP ) )
	{
		pWnd->EnableWindow( rEditParameters.nAnimSpeedIndex < ( rEditParameters.animSpeedList.size() - 1 ) );
	}
}


void CModelWindow::OnClickedLightButton()
{
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_MODEL_SET_LIGHT, 0 );
}


void CModelWindow::OnClickedAnimSpeedDown()
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_MODEL_SPEED_DOWN, 0 );
	}
}


void CModelWindow::OnClickedAnimSpeedUp()
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_MODEL_SPEED_UP, 0 );
	}
}


void CModelWindow::OnClickedTerrain()
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_GET_EDIT_PARAMETERS, MODEL_EP_TERRAIN );
	}
}


void CModelWindow::OnClickedTerrainDoubleSided()
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_GET_EDIT_PARAMETERS, MODEL_EP_TERRAIN_DOUBLESIDED );
	}
}


void CModelWindow::OnClickedTerrainGrid()
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_GET_EDIT_PARAMETERS, MODEL_EP_TERRAIN_GRID );
	}
}


void CModelWindow::OnClickedAnim()
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_GET_EDIT_PARAMETERS, MODEL_EP_ANIM );
	}
}


void CModelWindow::OnClickedAnimType()
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_GET_EDIT_PARAMETERS, MODEL_EP_ANIM_TYPE );
	}
}


void CModelWindow::OnClickedAIGeometry()
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_GET_EDIT_PARAMETERS, MODEL_EP_AI_GEOMETRY );
	}
}


void CModelWindow::OnClickedAIGeometryType()
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_GET_EDIT_PARAMETERS, MODEL_EP_AI_GEOMETRY_TYPE );
	}
}


void CModelWindow::OnSelChangeLightCombo()
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_GET_EDIT_PARAMETERS, MODEL_EP_LIGHT_INDEX );
	}
}


void CModelWindow::OnSelChangeTerrainSizeCombo()
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_GET_EDIT_PARAMETERS, MODEL_EP_TERRAIN_SIZE_INDEX );
	}
}


void CModelWindow::OnSelChangeAnimCountCombo()
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_GET_EDIT_PARAMETERS, MODEL_EP_ANIM_COUNT_INDEX );
	}
}


void CModelWindow::OnSelChangeAnimSpeedCombo()
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_GET_EDIT_PARAMETERS, MODEL_EP_ANIM_SPEED_INDEX );
	}
}


void CModelWindow::OnSelChangeAnimCircleRadiusCombo()
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_GET_EDIT_PARAMETERS, MODEL_EP_ANIM_RADIUS_INDEX );
	}
}


void CModelWindow::OnSelChangeAnimLineDistanceCombo()
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_GET_EDIT_PARAMETERS, MODEL_EP_ANIM_DISTANCE_INDEX );
	}
}


void CModelWindow::OnChangeSceneColor()
{
	if ( !bCreateControls )
	{
		SetSceneColorTimer();
	}
}


void CModelWindow::OnChangeFOV()
{
	if ( !bCreateControls )
	{
		SetFOVTimer();
	}
}


void CModelWindow::OnChangeTerrainColor()
{
	if ( !bCreateControls )
	{
		SetTerrainColorTimer();
	}
}


void CModelWindow::OnChangeTerrainColorOpacity()
{
	if ( !bCreateControls )
	{
		SetTerrainColorOpacityTimer();
	}
}


void CModelWindow::OnClickedSceneColorButton()
{
	if ( SUserData *pUserData = Singleton<IUserDataContainer>()->Get() )
	{
		CString strText;
		GetDlgItemText( IDC_MODEL_SCENE_COLOR_EDIT, strText );
		int r = 128;
		int g = 128;
		int b = 128;
		sscanf( strText, "%d,%d,%d", &r, &g, &b );
		//
		COLORREF startColor = MakeBGRColor( r, g, b );
		CColorDialog colorDialog( startColor, CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT, AfxGetMainWnd() );
		pUserData->colorList.resize( 16, 0xFFffFFff );
		colorDialog.m_cc.lpCustColors = &( pUserData->colorList[0] );
		if ( ( colorDialog.DoModal() == IDOK ) && ( ( GetStyle() & ES_READONLY ) == 0 ) )
		{
			const int nColor = colorDialog.GetColor();
			r = nColor & 0xFF;
			g = ( nColor >> 8 ) & 0xFF;
			b = ( nColor >> 16 ) & 0xFF;
			bCreateControls = true;
			SetDlgItemText( IDC_MODEL_SCENE_COLOR_EDIT, StrFmt( "%d, %d, %d", r, g, b ) );
			bCreateControls = false;
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_GET_EDIT_PARAMETERS, MODEL_EP_COLOR );
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
	}
}


void CModelWindow::OnClickedTerrainColorButton()
{
	if ( SUserData *pUserData = Singleton<IUserDataContainer>()->Get() )
	{
		CString strText;
		GetDlgItemText( IDC_MODEL_TERRAIN_COLOR_EDIT, strText );
		int r = 128;
		int g = 128;
		int b = 128;
		sscanf( strText, "%d,%d,%d", &r, &g, &b );
		//
		COLORREF startColor = MakeBGRColor( r, g, b );
		CColorDialog colorDialog( startColor, CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT, AfxGetMainWnd() );
		pUserData->colorList.resize( 16, 0xFFffFFff );
		colorDialog.m_cc.lpCustColors = &( pUserData->colorList[0] );
		if ( ( colorDialog.DoModal() == IDOK ) && ( ( GetStyle() & ES_READONLY ) == 0 ) )
		{
			const int nColor = colorDialog.GetColor();
			r = nColor & 0xFF;
			g = ( nColor >> 8 ) & 0xFF;
			b = ( nColor >> 16 ) & 0xFF;
			bCreateControls = true;
			SetDlgItemText( IDC_MODEL_TERRAIN_COLOR_EDIT, StrFmt( "%d, %d, %d", r, g, b ) );
			bCreateControls = false;
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_GET_EDIT_PARAMETERS, MODEL_EP_TERRAIN_COLOR );
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
	}
}


void CModelWindow::OnTimer( UINT nIDEvent ) 
{
  if ( nIDEvent == GetSceneColorID() )
	{
		OnSceneColorTimer();
	}
  else if ( nIDEvent == GetTerrainColorID() )
	{
		OnTerrainColorTimer();
	}
  else if ( nIDEvent == GetTerrainColorOpacityID() )
	{
		OnTerrainColorOpacityTimer();
	}
  else if ( nIDEvent == GetFOVID() )
	{
		OnFOVTimer();
	}
	CResizeDialog::OnTimer( nIDEvent );
}


void CModelWindow::SetSceneColorTimer()
{
  KillSceneColorTimer();
  nSceneColorTimer = SetTimer( GetSceneColorID(), GetSceneColorTimerInterval(), 0 );
}


void CModelWindow::KillSceneColorTimer()
{
  if ( nSceneColorTimer != 0 )
	{
		KillTimer( nSceneColorTimer );
	}
  nSceneColorTimer = 0;
}


void CModelWindow::OnSceneColorTimer()
{
	KillSceneColorTimer();
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_GET_EDIT_PARAMETERS, MODEL_EP_COLOR );
}


void CModelWindow::SetTerrainColorTimer()
{
  KillTerrainColorTimer();
  nTerrainColorTimer = SetTimer( GetTerrainColorID(), GetTerrainColorTimerInterval(), 0 );
}


void CModelWindow::KillTerrainColorTimer()
{
  if ( nTerrainColorTimer != 0 )
	{
		KillTimer( nTerrainColorTimer );
	}
  nTerrainColorTimer = 0;
}


void CModelWindow::OnTerrainColorTimer()
{
	KillTerrainColorTimer();
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_GET_EDIT_PARAMETERS, MODEL_EP_TERRAIN_COLOR );
}


void CModelWindow::SetTerrainColorOpacityTimer()
{
  KillTerrainColorOpacityTimer();
  nTerrainColorOpacityTimer = SetTimer( GetTerrainColorOpacityID(), GetTerrainColorOpacityTimerInterval(), 0 );
}


void CModelWindow::KillTerrainColorOpacityTimer()
{
  if ( nTerrainColorOpacityTimer != 0 )
	{
		KillTimer( nTerrainColorOpacityTimer );
	}
  nTerrainColorOpacityTimer = 0;
}


void CModelWindow::OnTerrainColorOpacityTimer()
{
	KillTerrainColorOpacityTimer();
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_GET_EDIT_PARAMETERS, MODEL_EP_TERRAIN_COLOR_OPACITY );
}


void CModelWindow::SetFOVTimer()
{
  KillFOVTimer();
  nFOVTimer = SetTimer( GetFOVID(), GetFOVTimerInterval(), 0 );
}


void CModelWindow::KillFOVTimer()
{
  if ( nFOVTimer != 0 )
	{
		KillTimer( nFOVTimer );
	}
  nFOVTimer = 0;
}


void CModelWindow::OnFOVTimer()
{
	KillFOVTimer();
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_GET_EDIT_PARAMETERS, MODEL_EP_FOV );
}


bool CModelWindow::HandleCommand( UINT nCommandID, DWORD dwData )
{
	switch( nCommandID )
	{
		case ID_GET_EDIT_PARAMETERS:
		{
			CModelState::SEditParameters *pEditParameters = reinterpret_cast<CModelState::SEditParameters*>( dwData );
			if ( pEditParameters != 0 )
			{
				return GetEditParameters( pEditParameters );
			}
			return false;
		}
		case ID_SET_EDIT_PARAMETERS:
		{
			const CModelState::SEditParameters *pEditParameters = reinterpret_cast<const CModelState::SEditParameters*>( dwData );
			if ( pEditParameters != 0 )
			{
				return SetEditParameters( *pEditParameters );
			}
			return false;
		}
		default:
			return false;
	}
	return false;
}


bool CModelWindow::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CModelState::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CModelState::UpdateCommand(), pbCheck == 0" );
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
		default:
			return false;
	}
	return false;
}

// basement storage  


