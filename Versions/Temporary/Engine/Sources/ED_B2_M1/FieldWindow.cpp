#include "stdafx.h"

#include "MapEditorLib/EditParameter.h"
#include "FieldWindow.h"

#include <cstdint>

CFieldWindow::CFieldWindow( CWnd* pParent )
	: CResizeDialog( CFieldWindow::IDD, pParent ), bCreateControls( false )
{
	SetControlStyle( IDC_TMITF_MOVE_SINGLE_RADIO, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_TMITF_MOVE_MULTIPLE_RADIO, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_TMITF_MOVE_ALL_RADIO, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_TMITF_DELIMITER_0, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_TMITF_FIELD_LABEL, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_TMITF_FIELD_COMBO, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_TMITF_RANDOMIZE_CHECK_BOX, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_TMITF_DELIMITER_1, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_TMITF_FILL_TERRAIN_CHECK_BOX, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_TMITF_FILL_OBJECTS_CHECK_BOX, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_TMITF_FILL_HEIGHTS_CHECK_BOX, ANCHORE_LEFT_TOP | RESIZE_HOR );
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_MAPINFO_TERRAIN_FIELD_WINDOW, this );
}


CFieldWindow::~CFieldWindow()
{
	Singleton<ICommandHandlerContainer>()->Remove( CHID_MAPINFO_TERRAIN_FIELD_WINDOW );
}


void CFieldWindow::DoDataExchange( CDataExchange* pDX )
{
	CResizeDialog::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_TMITF_FIELD_COMBO, wndFieldComboBox );
}


BEGIN_MESSAGE_MAP(CFieldWindow, CResizeDialog)
	ON_BN_CLICKED( IDC_TMITF_MOVE_SINGLE_RADIO, OnMoveRatio )
	ON_BN_CLICKED( IDC_TMITF_MOVE_MULTIPLE_RADIO, OnMoveRatio )
	ON_BN_CLICKED( IDC_TMITF_MOVE_ALL_RADIO, OnMoveRatio )
	ON_CBN_SELCHANGE(IDC_TMITF_FIELD_COMBO, OnSelchangeFieldComboBox)
	ON_BN_CLICKED( IDC_TMITF_RANDOMIZE_CHECK_BOX, OnRandomizeCheckBox )
	ON_BN_CLICKED( IDC_TMITF_FILL_TERRAIN_CHECK_BOX, OnFillTerrainCheckBox )
	ON_BN_CLICKED( IDC_TMITF_FILL_OBJECTS_CHECK_BOX, OnFillObjectsCheckBox )
	ON_BN_CLICKED( IDC_TMITF_FILL_HEIGHTS_CHECK_BOX, OnFillHeightsCheckBox )
END_MESSAGE_MAP()


BOOL CFieldWindow::OnInitDialog() 
{
	CResizeDialog::OnInitDialog();
	//
	bCreateControls = false;
	return true;
}


bool CFieldWindow::GetEditParameters( CFieldState::SEditParameters *pEditParameters )
{
	NI_ASSERT( pEditParameters != 0, "CFieldWindow::GetEditParameters(), pEditParameters == 0" );
	//
	if ( pEditParameters->nFlags & MITFEP_MOVE_TYPE )
	{
		pEditParameters->eMoveType = static_cast<CPolygonState::EMoveType>( GetCheckedRadioButton( IDC_TMITF_MOVE_SINGLE_RADIO, IDC_TMITF_MOVE_ALL_RADIO ) - IDC_TMITF_MOVE_SINGLE_RADIO );
	}
	if ( pEditParameters->nFlags & ( MITFEP_FIELD_COUNT | MITFEP_FIELD_INDEX ) )
	{
		GetComboBoxEditParameters( &( pEditParameters->fieldList ),
															 &( pEditParameters->nFieldIndex ),
															 wndFieldComboBox,
															 ( pEditParameters->nFlags & MITFEP_FIELD_COUNT ),
															 ( pEditParameters->nFlags & MITFEP_FIELD_INDEX ) );
	}
	if ( pEditParameters->nFlags & MITFEP_RANDOMIZE )
	{
		pEditParameters->bRandomize = ( IsDlgButtonChecked( IDC_TMITF_RANDOMIZE_CHECK_BOX ) > 0 );
	}
	if ( pEditParameters->nFlags & MITFEP_MIN_LENGTH )
	{
		if ( pEditParameters->fMinLength < 3.0f )
		{
			pEditParameters->fMinLength = 3.0f;
		}
		else if ( pEditParameters->fMinLength > ( 32.f * 16.0f ) )
		{
			pEditParameters->fMinLength = ( 32.f * 16.0f );
		}
	}
	if ( pEditParameters->nFlags & MITFEP_WIDTH )
	{
		if ( pEditParameters->fWidth < 0.0f )
		{
			pEditParameters->fWidth = 0.0f;
		}
		else if ( pEditParameters->fWidth > 0.5f )
		{
			pEditParameters->fWidth = 0.5f;
		}
	}
	if ( pEditParameters->nFlags & MITFEP_DISTURBANCE )
	{
		if ( pEditParameters->fDisturbance < 0.0f )
		{
			pEditParameters->fDisturbance = 0.0f;
		}
		else if ( pEditParameters->fDisturbance > 1.0f )
		{
			pEditParameters->fDisturbance = 1.0f;
		}
	}
	if ( pEditParameters->nFlags & MITFEP_FILL_TERRAIN )
	{
		pEditParameters->bFillTerrain = ( IsDlgButtonChecked( IDC_TMITF_FILL_TERRAIN_CHECK_BOX ) > 0 );
	}
	if ( pEditParameters->nFlags & MITFEP_FILL_OBJECTS )
	{
		pEditParameters->bFillObjects = ( IsDlgButtonChecked( IDC_TMITF_FILL_OBJECTS_CHECK_BOX ) > 0 );
	}
	if ( pEditParameters->nFlags & MITFEP_FILL_HEIGHTS )
	{
		pEditParameters->bFillHeights = ( IsDlgButtonChecked( IDC_TMITF_FILL_HEIGHTS_CHECK_BOX ) > 0 );
	}
	if ( pEditParameters->nFlags & MITFEP_UPDATE_MAP )
	{
		pEditParameters->bUpdateMap = true;
	}
	return true;
}


bool CFieldWindow::SetEditParameters( const CFieldState::SEditParameters &rEditParameters )
{
	bCreateControls = true;
	if ( rEditParameters.nFlags & MITFEP_MOVE_TYPE )
	{
		CheckRadioButton( IDC_TMITF_MOVE_SINGLE_RADIO, IDC_TMITF_MOVE_ALL_RADIO, IDC_TMITF_MOVE_SINGLE_RADIO + rEditParameters.eMoveType );
	}
	if ( rEditParameters.nFlags & ( MITFEP_FIELD_COUNT | MITFEP_FIELD_INDEX ) )
	{
		SetComboBoxEditParameters( rEditParameters.fieldList,
															 rEditParameters.nFieldIndex,
															 &wndFieldComboBox,
															 rEditParameters.nFlags & MITFEP_FIELD_COUNT,
															 rEditParameters.nFlags & MITFEP_FIELD_INDEX );
	}
	if ( rEditParameters.nFlags & MITFEP_RANDOMIZE )
	{
		CheckDlgButton( IDC_TMITF_RANDOMIZE_CHECK_BOX,  rEditParameters.bRandomize ? BST_CHECKED : BST_UNCHECKED );
	}
	if ( rEditParameters.nFlags & MITFEP_FILL_TERRAIN )
	{
		CheckDlgButton( IDC_TMITF_FILL_TERRAIN_CHECK_BOX, rEditParameters.bFillTerrain ? BST_CHECKED : BST_UNCHECKED );
	}
	if ( rEditParameters.nFlags & MITFEP_FILL_OBJECTS )
	{
		CheckDlgButton( IDC_TMITF_FILL_OBJECTS_CHECK_BOX, rEditParameters.bFillObjects ? BST_CHECKED : BST_UNCHECKED );
	}
	if ( rEditParameters.nFlags & MITFEP_FILL_HEIGHTS )
	{
		CheckDlgButton( IDC_TMITF_FILL_HEIGHTS_CHECK_BOX, rEditParameters.bFillHeights ? BST_CHECKED : BST_UNCHECKED );
	}
	bCreateControls = false;
	return true;
}


void CFieldWindow::OnMoveRatio()
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_FIELD_STATE, ID_GET_EDIT_PARAMETERS, MITFEP_MOVE_TYPE );
	}
}


void CFieldWindow::OnSelchangeFieldComboBox()
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_FIELD_STATE, ID_GET_EDIT_PARAMETERS, MITFEP_FIELD_INDEX );
	}
}


void CFieldWindow::OnRandomizeCheckBox()
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_FIELD_STATE, ID_GET_EDIT_PARAMETERS, MITFEP_RANDOMIZE );
	}
}


void CFieldWindow::OnFillTerrainCheckBox()
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_FIELD_STATE, ID_GET_EDIT_PARAMETERS, MITFEP_FILL_TERRAIN );
	}
}


void CFieldWindow::OnFillObjectsCheckBox()
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_FIELD_STATE, ID_GET_EDIT_PARAMETERS, MITFEP_FILL_OBJECTS );
	}
}


void CFieldWindow::OnFillHeightsCheckBox()
{
	if ( !bCreateControls )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_FIELD_STATE, ID_GET_EDIT_PARAMETERS, MITFEP_FILL_HEIGHTS );
	}
}


bool CFieldWindow::HandleCommand( unsigned nCommandID, uint32_t dwData )
{
	switch( nCommandID )
	{
		case ID_GET_EDIT_PARAMETERS:
		{
			CFieldState::SEditParameters *pEditParameters = reinterpret_cast<CFieldState::SEditParameters*>( dwData );
			if ( pEditParameters != 0 )
			{
				return GetEditParameters( pEditParameters );
			}
			return false;
		}
		case ID_SET_EDIT_PARAMETERS:
		{
			const CFieldState::SEditParameters *pEditParameters = reinterpret_cast<const CFieldState::SEditParameters*>( dwData );
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


bool CFieldWindow::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CFieldWindow::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CFieldWindow::UpdateCommand(), pbCheck == 0" );
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


