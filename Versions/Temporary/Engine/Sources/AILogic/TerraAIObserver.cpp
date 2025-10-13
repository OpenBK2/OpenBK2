#include "stdafx.h"

#include "StaticObjects.h"
#include "UnitsIterators.h"
#include "NewUpdater.h"
#include "AIUnit.h"
#include "GlobalWarFog.h"
#include "RailRoads.h"

#include "TerraAIObserver.h"

#include <cstdint>

extern CEventUpdater updater;
extern CStaticObjects theStatObjs;
extern CUnits units;
extern CGlobalWarFog theWarFog;
extern SRailRoadSystem theRailRoadSystem;
CRiverSounds CTerraAIObserverInGame::riverSounds;

CTerraAIObserverInGame::CTerraAIObserverInGame( const int nSizeX, const int nSizeY  )
{
	CTerraAIObserver::CTerraAIObserver();

	pAIMap = GetAIMap();
	pTerrain = pAIMap->GetTerrain();
	pHeights = pAIMap->GetHeights();

	InitSizes( nSizeX, nSizeY );

	pMarkers->Init( pAIMap );
}

void CTerraAIObserverInGame::InitSizes( const int nSizeX, const int nSizeY )
{
}

void CTerraAIObserverInGame::AddVSO( const NDb::SVSOInstance *pInstance )
{
	CTerraAIObserver::AddVSO( pInstance );

	// Collect railroads
	if ( pInstance->pDescriptor->aIProperty.nSoilType & SVectorStripeObjectDesc::ESP_RAIL )
	{
		theRailRoadSystem.AddRailRoad( pInstance );
	}
}

void CTerraAIObserverInGame::UpdateHeights( const int nX1, const int nY1, const int nX2, const int nY2,
																						const CArray2D<float> &heights )
{
	CTerraAIObserver::UpdateHeights( nX1, nY1, nX2, nY2, heights );

	pHeights->FinalizeUpdateHeights();
	theWarFog.SynchronizeHeights( SVector( nX1, nY1 ), SVector( nX2, nY2 ) );
	//{ cheat
	theStatObjs.UpdateAllObjectsPos();
	for ( CGlobalIter iter( 0, ANY_PARTY ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pUnit = *iter;
		//if ( IsValidObj( pUnit ) )
			updater.AddUpdate( 0, ACTION_NOTIFY_PLACEMENT, pUnit, -1 );
	}
	theStatObjs.PostAllObjectsInit();
	//}
}

void CTerraAIObserverInGame::UpdateTypes( const int nX1, const int nY1, const int nX2, const int nY2,
																					const CArray2D<uint8_t> &types )
{
	CTerraAIObserver::UpdateTypes( nX1, nY1, nX2, nY2, types );

	for ( CGlobalIter iter( 0, ANY_PARTY ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pUnit = *iter;
		pUnit->RestoreLock();
	}
}

void CTerraAIObserverInGame::FinalizeUpdates()
{
	CTerraAIObserver::FinalizeUpdates();

	theStatObjs.UpdateAllObjectsPos();
	theWarFog.FinishInitialization();
}

void CTerraAIObserverInGame::InitInGame()
{
	for ( CRiverSounds::iterator it = riverSounds.begin(); it != riverSounds.end(); ++it )
		updater.AddUpdate( EFB_RIVER_POINT, MAKELONG( it->x, it->y ), 0 );
}

void CTerraAIObserverInGame::AddRiver( const NDb::SVSOInstance *pInstance )
{
	CTerraAIObserver::AddRiver( pInstance );

	const float fStep = 1000;
	float fDistSoFar = 0;
	for ( int i = 0; i < pInstance->points.size() - 1; ++i )
	{
		CVec3 vDist( pInstance->points[i+1].vPos - pInstance->points[i].vPos );
		const float fSegmentLength = fabs( vDist.x, vDist.y );
		vDist.z = 0;
		Normalize( &vDist );

		float fDist = fDistSoFar;
		for ( ; fDist < fSegmentLength; fDist += fStep )
		{
			const CVec3 vPos( pInstance->points[i].vPos + vDist * fDist );
			riverSounds.push_back( CVec2( vPos.x, vPos.y ) );
		}		
		fDistSoFar = fDist - fSegmentLength;
	}
}

int CTerraAIObserverInGame::operator&( IBinSaver &saver )
{
	saver.Add( 2,(CTerraAIObserver*)this );
	saver.Add( 3, &riverSounds );

	if ( saver.IsReading() )
	{
		InitSizes( pAIMap->GetSizeX(), pAIMap->GetSizeY() );
		pMarkers->Init( pAIMap );
	}
	return 0;
}

REGISTER_SAVELOAD_CLASS( 0x1B214300, CTerraAIObserverInGame )

