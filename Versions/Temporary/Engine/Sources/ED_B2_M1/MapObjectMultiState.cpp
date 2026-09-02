#include "stdafx.h"
#include "Misc/2Darray.h"
#include "Stats_B2_M1/IconsSet.h"
#include "SceneB2/Scene.h"
#include "MapEditorLib/CommonEditorMethods.h"
#include "ResourceDefines.h"

#include "MapInfoEditor.h"
#include "MapObjectMultiState.h"
#include "EditorMethods.h"

#include <cstdint>

#include <zconf.h>

void CMapObjectMultiState::UpdateEditParameters( unsigned nFlags )
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
				pEditParameters->playerList.push_back( std::to_string(  nPlayerIndex ) );
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


void CMapObjectMultiState::SwitchState( const std::string &rszObjectTypeName )
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


bool CMapObjectMultiState::HandleCommand( unsigned nCommandID, uint32_t dwData )
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
			std::string *pszObjectTypeName = reinterpret_cast<std::string*>( dwData );
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


bool CMapObjectMultiState::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
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
	//unsigned nFlags;
	//CPlayerList playerList;
	return 0;
}

// basement storage  


