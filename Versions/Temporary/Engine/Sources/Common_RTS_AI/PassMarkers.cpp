#include "stdafx.h"

#include "System/Commands.h"
#include "Misc/StrProc.h"

#include "PassMarkers.h"

#include <cstdint>

namespace NPassMarkers
{
	static bool s_bShowWater = false;
}


CPassMarkersDraw::CPassMarkersDraw()
{
	markers_colors[0] = NDebugInfo::RED;
	markers_colors[1] = NDebugInfo::GREEN;
	markers_colors[2] = NDebugInfo::BLUE;
	nWaterMarker = NDebugInfo::OBJECT_ID_GENERATE;
	Reset();
}


void CPassMarkersDraw::Reset()
{
	for ( int i = 0; i < MARKERS_COLORS_COUNT; ++i )
	{
		markers_id[i] = NDebugInfo::OBJECT_ID_GENERATE;
		markers_radiuses[i] = 0;
		markers_aiClasses[i] = EAC_NONE;
		markers_freeClass[i] = FREE_NONE;
	}
	SetPassMarkers( NDebugInfo::GREEN, EAC_WHELL, FREE_NONE, 1 );
	bDrawMarkers = false;
}


void CPassMarkersDraw::Init( CAIMap *_pAIMap )
{
	pAIMap = _pAIMap;
	pTerrain = pAIMap->GetTerrain();
}


void CPassMarkersDraw::Clear()
{
	for ( int i = 0; i < MARKERS_COLORS_COUNT; ++i )
	{
		DebugInfoManager()->DeleteObject( markers_id[i] );
		markers_id[i] = NDebugInfo::OBJECT_ID_GENERATE;
	}
	DebugInfoManager()->DeleteObject( nWaterMarker );
}


void CPassMarkersDraw::ToggleDrawPassMarkers()
{
	if ( bDrawMarkers )
	{
		Clear();
	}
	else
	{
		lastPassabilityUpdate = 0;
		DrawPassabilities();
	}

	bDrawMarkers = !bDrawMarkers;
	DebugInfoManager()->ShowAxes( bDrawMarkers );
}


void CPassMarkersDraw::SetPassMarkers( const NDebugInfo::EColor color, const EAIClasses aiClass, const EFreeTileInfo freeClass, const int nBoundTileRadius )
{
	for ( int i = 0; i < MARKERS_COLORS_COUNT; ++i )
	{
		if ( markers_colors[i] == color )
		{
			markers_aiClasses[i] = aiClass;
			markers_freeClass[i] = freeClass;
			markers_radiuses[i] = nBoundTileRadius;

			break;
		}
	}
}


void CPassMarkersDraw::DrawPassabilities()
{
	STerrainModeSetter modeSetter( ELM_ALL, pTerrain );
	std::vector<SVector> tiles[MARKERS_COLORS_COUNT];

	for ( int x = 0; x < pAIMap->GetSizeX(); ++x )
	{
		for ( int y = 0; y < pAIMap->GetSizeY(); ++y )
		{
			const SVector tile( x, y );

			for ( int i = 0; i < MARKERS_COLORS_COUNT; ++i )
				if ( markers_aiClasses[i] != EAC_NONE )
				{
					if ( pTerrain->CanUnitGo( markers_radiuses[i], tile, markers_aiClasses[i] ) == markers_freeClass[i] )
						tiles[i].push_back( tile );
				}
		}
	}

	for ( int i = 0; i < MARKERS_COLORS_COUNT; ++i )
		if ( markers_aiClasses[i] != EAC_NONE )
			markers_id[i] = DebugInfoManager()->CreateMarker( markers_id[i], tiles[i], markers_colors[i] );

	if ( NPassMarkers::s_bShowWater )
	{
		std::vector<SVector> water;
		for ( int x = 0; x < pAIMap->GetSizeX(); ++x )
		{
			for ( int y = 0; y < pAIMap->GetSizeY(); ++y )
			{
				const SVector tile( x, y );
				if ( pTerrain->GetTerrainType( x, y ) == ETT_WATER_TERRAIN )  
					water.push_back( tile );
			}
		}
		nWaterMarker = DebugInfoManager()->CreateMarker( nWaterMarker, water, NDebugInfo::WHITE );
	}

	//DEBUG{ draw soil types here
	//vector<SVector> redTiles;
	//vector<SVector> greenTiles;
	//vector<SVector> blueTiles;
	//for ( int x = 0; x < pAIMap->GetSizeX(); ++x )
	//{
	//	for ( int y = 0; y < pAIMap->GetSizeY(); ++y )
	//	{
	//		const SVector tile( x, y );
	//		const uint8_t soil = pTerrain->GetSoilType( tile );

	//		if ( (soil & SVectorStripeObject::ESP_DUST) && (soil & SVectorStripeObject::ESP_TRACE) )
	//			redTiles.push_back( tile );
	//		else if ( soil & SVectorStripeObject::ESP_DUST )  
	//			blueTiles.push_back( tile );
	//		else if ( soil & SVectorStripeObject::ESP_TRACE )
	//			greenTiles.push_back( tile );
	//	}
	//}
	//nRedMarker = DebugInfoManager()->CreateMarker( nRedMarker, redTiles, NAIVisInfo::RED );
	//nGreenMarker = DebugInfoManager()->CreateMarker( nGreenMarker, greenTiles, NAIVisInfo::GREEN );
	//nBlueMarker = DebugInfoManager()->CreateMarker( nBlueMarker, blueTiles, NAIVisInfo::BLUE );
	//DEBUG}
}

void CPassMarkersDraw::ToggleDrawPassMarkers1()
{
	bDrawMarkers = !bDrawMarkers;
	//
	if ( bDrawMarkers )
		DrawPassabilities();
}

void CPassMarkersDraw::UpdatePassMarkers()
{
	if ( !bDrawMarkers )
		return;

	// process update
}

void CPassMarkersDraw::DrawPassabilities1()
{
	//if ( bDrawMarkers )
	//{
	//	if ( lastPassabilityUpdate == 0 )
	//		//lastPassabilityUpdate = curTime;

	//	if ( lastPassabilityUpdate + 1000 < curTime )
	//	{
	//		DrawPassabilities();
	//		//lastPassabilityUpdate = curTime;
	//	}
	//}
}

START_REGISTER( PassMarkers )
REGISTER_VAR_EX( "show_water", NGlobal::VarBoolHandler, &NPassMarkers::s_bShowWater, false, STORAGE_NONE );
FINISH_REGISTER

