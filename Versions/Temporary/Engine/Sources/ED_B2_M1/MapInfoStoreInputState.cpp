#include "StdAfx.h"

#include "Tools_SceneGeometry.h"
#include "MapInfoStoreInputState.h"
#include "../Stats_B2_M1/Vis2AI.h"
#include "../MapEditorLib/Interface_Mainframe.h"

void CMapInfoStoreInputState::OnInputStateEvent( const SInputStateEventInfo &rInputStateEventInfo )
{
	NI_ASSERT( ( rInputStateEventInfo.nEventType >= 0 ) && ( rInputStateEventInfo.nEventType < ISE_COUNT ),
							StrFmt( "CMapInfoStoreInputState::OnInputStateEvent(): Invalid rInputStateEventInfo.nEventType: %d [0, %d)\n",
											rInputStateEventInfo.nEventType,
											ISE_COUNT ) );
	//
	eventInfoList[rInputStateEventInfo.nEventType] = rInputStateEventInfo;
	if ( rInputStateEventInfo.nType == IST_MOUSE )
	{
		CVec3 vTerrainPos = VNULL3;
		CTPoint<int> visTilePos = CTPoint<int>( 0, 0 );
		CTPoint<int> aiTilePos = CTPoint<int>( 0, 0 );
		CTPoint<int> gridPos = CTPoint<int>( 0, 0 );
		//
		Get2DPosOnMapHeights( &vTerrainPos, CVec2( rInputStateEventInfo.point.x, rInputStateEventInfo.point.y ) );
		visTilePos.x = (int)( vTerrainPos.x / ( AI_TILE_SIZE * 2.0f ) );
		visTilePos.y = (int)( vTerrainPos.y / ( AI_TILE_SIZE * 2.0f ) );
		aiTilePos.x = (int)( vTerrainPos.x / ( AI_TILE_SIZE * 1.0f ) );
		aiTilePos.y = (int)( vTerrainPos.y / ( AI_TILE_SIZE * 1.0f ) );
		gridPos.x = (int)( vTerrainPos.x / ( AI_TILE_SIZE * 2.0f ) + 0.5f );
		gridPos.y = (int)( vTerrainPos.y / ( AI_TILE_SIZE * 2.0f ) + 0.5f );
		//
		if ( bFixInvalidPos )
		{
			if ( visSize.x > 0 )
			{
				visTilePos.x = Clamp( visTilePos.x, 0, visSize.x - 1 );
				vTerrainPos.x = Clamp( vTerrainPos.x, 0.0f, visSize.x * AI_TILES_IN_VIS_TILE * AI_TILE_SIZE * 1.0f );
				aiTilePos.x = Clamp( aiTilePos.x, 0, ( visSize.x * AI_TILES_IN_VIS_TILE ) - 1 );
				gridPos.x = Clamp( gridPos.x, 0, visSize.x );
			}
			if ( visSize.y > 0 )
			{
				visTilePos.y = Clamp( visTilePos.y, 0, visSize.y - 1 );
				vTerrainPos.y = Clamp( vTerrainPos.y, 0.0f, visSize.y * AI_TILES_IN_VIS_TILE * AI_TILE_SIZE * 1.0f );
				aiTilePos.y = Clamp( aiTilePos.y, 0, ( visSize.y * AI_TILES_IN_VIS_TILE ) - 1 );
				gridPos.y = Clamp( gridPos.y, 0, visSize.y );
			}
		}
		//
		eventInfoList[rInputStateEventInfo.nEventType].vTerrainPos = vTerrainPos;
		eventInfoList[rInputStateEventInfo.nEventType].visTilePos = visTilePos;
		eventInfoList[rInputStateEventInfo.nEventType].aiTilePos = aiTilePos;
		eventInfoList[rInputStateEventInfo.nEventType].gridPos = gridPos;
		//
		lastEventInfo = eventInfoList[rInputStateEventInfo.nEventType];
		lastEventInfo.isValid = true;
	}
	eventInfoList[rInputStateEventInfo.nEventType].isValid = true;
	Singleton<IMainFrameContainer>()->Get()->SetStatusBarText( 2, StrFmt( "(%g, %g), [%d, %d]", lastEventInfo.vTerrainPos.x, lastEventInfo.vTerrainPos.y, lastEventInfo.visTilePos.x, lastEventInfo.visTilePos.y ) );
}

// basement storage  


