#include "StdAfx.h"
#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "..\stats_b2_m1\iconsset.h"
#include "..\sceneb2\scene.h"
#include "..\mapeditorlib\commoneditormethods.h"
#include "ResourceDefines.h"

#include "MapInfoEditor.h"
#include "MapObjectMultiState.h"
#include "EditorMethods.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void CMapObjectMultiState::UpdateEditParameters( UINT nFlags )
{
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		pEditParameters->nFlags = nFlags;
		if ( pEditParameters->nFlags & MIMOSEP_PLAYER_COUNT )
		{
			int nPlayerCount = 0;
			CManipulatorManager::GetValue( &nPlayerCount, pMapInfoEditor->GetViewManipulator(), "Players" );
			pEditParameters->playerList.clear();
			for ( int nPlayerIndex = 0; nPlayerIndex < nPlayerCount; ++nPlayerIndex )
			{
				pEditParameters->playerList.push_back( StrFmt( "%d", nPlayerIndex ) );
			}
		}
		if ( pEditParameters->nFlags & MIMOSEP_DIRECTION )
		{
			if ( !pMapInfoEditor->objectInfoCollector.IsSelectionEmpty() )
			{
				pEditParameters->fDirection = pMapInfoEditor->objectInfoCollector.GetSelectionDirection()	* 180.0f / FP_PI;
				while ( pEditParameters->fDirection > 360.0f )
				{
					pEditParameters->fDirection -= 360.0f;
				}
				while ( pEditParameters->fDirection < 0.0f )
				{
					pEditParameters->fDirection += 360.0f;
				}
			}
		}
	}
}


void CMapObjectMultiState::SwitchState( const string &rszObjectTypeName )
{
	int nNewState = GetActiveInputStateIndex();
	//
	if ( rszObjectTypeName == "MineRPGStats" )
	{
		nNewState = IS_SIMPLE_OBJECT;
	}
	else if ( rszObjectTypeName == "BuildingRPGStats" )
	{
		nNewState = IS_SIMPLE_OBJECT;
	}
	else if ( rszObjectTypeName == "MechUnitRPGStats" )
	{
		nNewState = IS_SIMPLE_OBJECT;
	}
	else if ( rszObjectTypeName == "ObjectRPGStats" )
	{
		nNewState = IS_SIMPLE_OBJECT;
	}
	else if ( rszObjectTypeName == "TerraObjSetRPGStats" )
	{
		nNewState = IS_SIMPLE_OBJECT;
	}
	else if ( rszObjectTypeName == "SquadRPGStats" )
	{
		nNewState = IS_SIMPLE_OBJECT;
	}
	else if ( rszObjectTypeName == "FenceRPGStats" )
	{
		nNewState = IS_FENCE;
	}
	else if ( rszObjectTypeName == "EntrenchmentRPGStats" )
	{
		nNewState = IS_ENTRENCHMENT;
	}
	else if ( rszObjectTypeName == "BridgeRPGStats" )
	{
		nNewState = IS_BRIDGE;
	}
	else if ( rszObjectTypeName == "TerrainSpotDesc" )
	{
		nNewState = IS_SPOT;
	}
	//
	if ( GetActiveInputStateIndex() != nNewState )
	{
		SetActiveInputState( nNewState, true, false );
	}
}


void CMapObjectMultiState::Enter()
{
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		pEditParameters->nFlags = MIMOSEP_ALL;
		SetEditParameters( *pEditParameters, CHID_MAPINFO_MAPOBJECT_WINDOW );
	}
	//
	CMultiInputState::Enter();
}


void CMapObjectMultiState::Leave()
{
	CMultiInputState::Leave();
	//
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		pEditParameters->nFlags = MIMOSEP_ALL;
		::GetEditParameters( pEditParameters, CHID_MAPINFO_MAPOBJECT_WINDOW );
	}
}


bool CMapObjectMultiState::HandleCommand( UINT nCommandID, DWORD dwData )
{
	switch( nCommandID )
	{
		case ID_GET_EDIT_PARAMETERS:
		{
			if ( SEditParameters *pEditParameters = GetEditParameters() )
			{
				pEditParameters->nFlags = dwData;
				::GetEditParameters( pEditParameters, CHID_MAPINFO_MAPOBJECT_WINDOW );
			}
			return true;
		}
		case ID_SET_EDIT_PARAMETERS:
		{
			if ( SEditParameters *pEditParameters = GetEditParameters() )
			{
				pEditParameters->nFlags = dwData;
				SetEditParameters( *pEditParameters, CHID_MAPINFO_MAPOBJECT_WINDOW );
			}
			return true;
		}
		case ID_UPDATE_EDIT_PARAMETERS:
		{
			if ( SEditParameters *pEditParameters = GetEditParameters() )
			{
				UpdateEditParameters( dwData );
				SetEditParameters( *pEditParameters, CHID_MAPINFO_MAPOBJECT_WINDOW );
			}
			return true;
		}
		case ID_MIMO_SWITCH_MULTI_STATE:
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


bool CMapObjectMultiState::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CMapObjectMultiState::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CMapObjectMultiState::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID )
	{
		case ID_GET_EDIT_PARAMETERS:
		case ID_SET_EDIT_PARAMETERS:
		case ID_UPDATE_EDIT_PARAMETERS:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		case ID_MIMO_SWITCH_MULTI_STATE:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		default:
			return false;
	}
	return false;
}


CMapObjectMultiState::SEditParameters* CMapObjectMultiState::GetEditParameters()
{ 
	return ( ( pMapInfoEditor != 0 ) ? &( pMapInfoEditor->editorSettings.epMapObjectMultiState ) : 0 );
}


int CMapObjectMultiState::SEditParameters::operator&( IXmlSaver &xs )
{
	xs.Add( "DirectionType", &eDirectionType );
	xs.Add( "PlayerIndex", &nPlayerIndex );
	xs.Add( "Direction", &fDirection );
	xs.Add( "Thumbnails", &bThumbnails );
	//
	//do not serialise this fields:
	//UINT nFlags;
	//CPlayerList playerList;
	return 0;
}

// basement storage  

