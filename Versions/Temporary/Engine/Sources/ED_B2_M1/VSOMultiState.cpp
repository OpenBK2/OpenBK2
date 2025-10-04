#include "stdafx.h"

#include "mapeditorlib/commoneditormethods.h"
#include "Misc/2Darray.h"
#include "stats_b2_m1/IconsSet.h"
#include "ResourceDefines.h"
#include "EditorMethods.h"
#include "MapInfoEditor.h"

#include "VSOMultiState.h"

#include <zconf.h>

void CVSOMultiState::UpdateEditParameters( UINT nFlags )
{
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		pEditParameters->nFlags = nFlags;
	}
}


bool CVSOMultiState::PickOtherVSO( UINT nFlags, const CTPoint<int> &rMousePoint, const CVec3 &rvPos )
{
	const int nActiveStateIndex = GetActiveInputStateIndex();
	const int nStateCount = GetCount();
	
	for ( int nStateIndex = 0; nStateIndex < nStateCount; ++nStateIndex )
	{
		if ( nStateIndex != nActiveStateIndex )
		{
			CVSOState::CVSOIDList pickVSOIDList;
			CVSOState *pVSOState = dynamic_cast<CVSOState*>( GetInputState( nStateIndex ) );
			pVSOState->PickVSO( rvPos, &pickVSOIDList );
			if ( !pickVSOIDList.empty() )
			{
				SetActiveInputState( nStateIndex, true, false );
				pVSOState->EmulateSelectLButtonDown( nFlags, rMousePoint, rvPos );
				return true;
			}
		}
	}
	return false;
}


void CVSOMultiState::SwitchState( const string &rszObjectTypeName )
{
	int nNewState = GetActiveInputStateIndex();
	//
	if ( rszObjectTypeName == "RoadDesc" )
	{
		nNewState = IS_ROAD;
	}
	else if ( rszObjectTypeName == "RiverDesc" )
	{
		nNewState = IS_RIVER;
	}
	else if ( rszObjectTypeName == "CragDesc" )
	{
		nNewState = IS_CRAG;
	}
	else if ( rszObjectTypeName == "LakeDesc" )
	{
		nNewState = IS_LAKE;
	}
	else if ( rszObjectTypeName == "CoastDesc" )
	{
		nNewState = IS_COAST;
	}
	//
	if ( GetActiveInputStateIndex() != nNewState )
	{
		SetActiveInputState( nNewState, true, false );
	}
}


void CVSOMultiState::Enter()
{
	CMultiInputState::Enter();
	//
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		pEditParameters->nFlags = MIVSOSEP_ALL;
		SetEditParameters( *pEditParameters, CHID_MAPINFO_VSO_WINDOW );
	}
}


void CVSOMultiState::Leave()
{
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		pEditParameters->nFlags = MIVSOSEP_ALL;
		::GetEditParameters( pEditParameters, CHID_MAPINFO_VSO_WINDOW );
	}
	//
	CMultiInputState::Leave();
}


bool CVSOMultiState::HandleCommand( UINT nCommandID, DWORD dwData )
{
	switch( nCommandID )
	{
		case ID_GET_EDIT_PARAMETERS:
		{
			if ( SEditParameters *pEditParameters = GetEditParameters() )
			{
				pEditParameters->nFlags = dwData;
				::GetEditParameters( pEditParameters, CHID_MAPINFO_VSO_WINDOW );
			}
			return true;
		}
		case ID_SET_EDIT_PARAMETERS:
		{
			if ( SEditParameters *pEditParameters = GetEditParameters() )
			{
				pEditParameters->nFlags = dwData;
				SetEditParameters( *pEditParameters, CHID_MAPINFO_VSO_WINDOW );
			}
			return true;
		}
		case ID_UPDATE_EDIT_PARAMETERS:
		{
			if ( SEditParameters *pEditParameters = GetEditParameters() )
			{
				UpdateEditParameters( dwData );
				SetEditParameters( *pEditParameters, CHID_MAPINFO_VSO_WINDOW );
			}
			return true;
		}
		case ID_MIVSO_SWITCH_MULTI_STATE:
		{
			string *pszObjectTypeName = reinterpret_cast<string*>( dwData );
			if ( pszObjectTypeName != 0 )
			{
				SwitchState( *pszObjectTypeName );
			}
			return true;
		}
		default:
			return false;
	}
	return false;
}


bool CVSOMultiState::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CVSOMultiState::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CVSOMultiState::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID )
	{
		case ID_GET_EDIT_PARAMETERS:
		case ID_SET_EDIT_PARAMETERS:
		case ID_UPDATE_EDIT_PARAMETERS:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		case ID_MIVSO_SWITCH_MULTI_STATE:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		default:
			return false;
	}
	return false;
}


CVSOMultiState::SEditParameters* CVSOMultiState::GetEditParameters()
{ 
	return ( ( pMapInfoEditor != 0 ) ? &( pMapInfoEditor->editorSettings.epVSOMultiState ) : 0 );
}


int CVSOMultiState::SEditParameters::operator&( IXmlSaver &xs )
{
	xs.Add( "PointNumber", &ePointNumber );
	xs.Add( "StatsType", &eStatsType );
	xs.Add( "Width", &fWidth );
	xs.Add( "Opacity", &fOpacity );
	xs.Add( "Height", &fHeight );
	xs.Add( "Thumbnails", &bThumbnails );
	//
	//do not serialise this fields:
	// UINT nFlags;
	return 0;
}

// basement storage  


