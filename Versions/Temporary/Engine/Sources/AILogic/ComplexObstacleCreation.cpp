#include "StdAfx.h"
#include ".\Complexobstaclecreation.h"
#include "FenceCreation.h"
#include "..\Stats_b2_M1\AIUpdates.h"
#include "../Common_RTS_AI/AIMap.h"
#include "UnitCreation.h"
#include "StaticObjects.h"

extern CUnitCreation theUnitCreation;
extern CStaticObjects theStatObjs;
namespace NLongObjectCreation
{
CCreations creations;

}


//*******************************************************************
//*												  CComplexObstacleCreation*
//*******************************************************************

void CComplexObstacleCreation::CreateObjects( SAIObjectsUnderConstructionUpdate * pUpdate )
{
	if ( !pFenceCreation ) 
		return;
	pFenceCreation->CreateObjects( pUpdate );

	const int nFormerInd = pUpdate->objects.size();
	pUpdate->objects.resize( nFormerInd + antitanks.size() );

	for ( int i = 0; i < antitanks.size(); ++i )
	{
		if ( !antitanks[i] )
			continue;
		list<SVector> tiles;
		antitanks[i]->GetCoveredTiles( &tiles );
		bool bCanBuildCurrent = true;
		for ( list<SVector>::const_iterator it = tiles.begin(); it != tiles.end(); ++it )
		{
			if ( !GetAIMap()->IsTileInside(*it ) || 0 != ( EAC_TERRAIN & GetTerrain()->GetTileLockInfo( *it ) ) )
				pUpdate->impossibleToBuildTiles.push_back( CVec2( it->x, it->y ) );
			else
			{
				pUpdate->buildTiles.push_back( CVec2( it->x, it->y ) );
				bCanBuildCurrent = false;
			}
		}
		antitanks[i]->GetNewUnitInfo( &pUpdate->objects[nFormerInd + i].info );	
		pUpdate->objects[nFormerInd + i].bCanBuild = bCanBuildCurrent;
	}
}

bool CComplexObstacleCreation::PreCreate( const CVec2 &vFrom, const CVec2 &vTo, const bool bCheckLock )
{
	if ( IsAIModificationAllowed() )
	{
  		int a = 0;
	}
	bCannot = false;
	nCurIndex = 0;
	pFenceCreation = new CFenceCreation( GetPlayer(), IsAIModificationAllowed() );
	pFenceCreation->PreCreate( vFrom, vTo, bCheckLock );
	antitanks.clear();
	CVec2 vDirPerp( vTo - vFrom );
	const WORD wDir( GetDirectionByVector(vDirPerp ) );
	Normalize( &vDirPerp );
	vDirPerp = CVec2( -vDirPerp.y, vDirPerp.x );

	// and create needed antitanks
	for ( int i = 0; i < pFenceCreation->GetMaxIndex(); i += SConsts::COMPLEX_OBSTACLE_ANTITANK_PERIOD )
	{
		const CVec2 vFenceCenter = pFenceCreation->GetCenter( i );
		const SObjectBaseRPGStats * pStats = theUnitCreation.GetRandomAntitankObject();

		CObj<CCommonStaticObject> pAntitank;

		const CVec2 vDesiredPoint( vFenceCenter + 3 * SConsts::TILE_SIZE * vDirPerp );

		if ( !IsAIModificationAllowed() || CGivenPassabilityStObject::CheckStaticObject( pStats, vDesiredPoint, wDir, 0 ) )
		{
			pAntitank = new CSimpleStaticObject( pStats, CVec3(vDesiredPoint,0), wDir, pStats->fMaxHP, 0, ESOT_COMMON, GetPlayer(), true );
			if ( IsAIModificationAllowed() )
				pAntitank->Mem2UniqueIdObjs();
			pAntitank->Init();
		}
		antitanks.push_back( pAntitank.GetPtr() ); // yes, antitank can be NULL
	}
	while ( !CanBuildNext() && ++nCurIndex < GetMaxIndex() )
	{
	}
	return nCurIndex < GetMaxIndex();
}

const int CComplexObstacleCreation::GetMaxIndex() const
{
	return pFenceCreation->GetMaxIndex() + antitanks.size();
}

const int CComplexObstacleCreation::GetCurIndex() const
{
	return nCurIndex;
}

const CVec2 CComplexObstacleCreation::GetNextPoint( const int nPlace, const int nMaxPlace ) const
{
	if ( IsAntitank( nCurIndex ) )
	{
		NI_ASSERT( antitanks[GetAntitankIndex(nCurIndex)] != 0, "zero antitank" );
		const CVec3 vCenter( antitanks[GetAntitankIndex(nCurIndex)]->GetCenter() );
		return CVec2( vCenter.x, vCenter.y ) + SConsts::TILE_SIZE * GetVectorByDirection( nPlace * 65535 / nMaxPlace );
	}
	else
	{
		return pFenceCreation->GetNextPoint( nPlace, nMaxPlace );
	}
}

void CComplexObstacleCreation::BuildNext()
{
	bTmpNonCheatPath = false;
	CLongObjectCreation::BuildNext();
	if ( IsAntitank( nCurIndex ) )
	{
		CCommonStaticObject *pAntitank = antitanks[GetAntitankIndex(nCurIndex)];
		theStatObjs.AddStaticObject( pAntitank, false );
	}
	else
	{
		pFenceCreation->BuildNext();
	}

	AdvanceToNextIndex();
}

void CComplexObstacleCreation::AdvanceToNextIndex()
{
	++nCurIndex;
	while ( IsAntitank( nCurIndex ) && !antitanks[GetAntitankIndex(nCurIndex)] )
	{
		bTmpNonCheatPath = true;
		++nCurIndex;
	}
	tilesUnder.clear();
	if ( nCurIndex < GetMaxIndex() && IsAntitank( nCurIndex ) )
	{
		CCommonStaticObject *pAntitank = antitanks[GetAntitankIndex(nCurIndex)];
		pAntitank->GetCoveredTiles( &tilesUnder );
	}
}

void CComplexObstacleCreation::GetUnitsPreventing( list< CPtr<CAIUnit> > * units )
{
	if ( IsAntitank( nCurIndex ) )
	{
		SRect r1;
		antitanks[GetAntitankIndex(nCurIndex)]->GetBoundRect( &r1 );
		CLongObjectCreation::GetUnitsPreventing( r1, units );
	}
	else
	{
		pFenceCreation->GetUnitsPreventing( units );
	}
}

bool CComplexObstacleCreation::IsAnyUnitPrevent() const
{
	if ( IsAntitank( nCurIndex ) )
	{
		SRect r1;
		antitanks[GetAntitankIndex(nCurIndex)]->GetBoundRect( &r1 );
		return CLongObjectCreation::IsAnyUnitPrevent( r1 );
	}
	else
		return pFenceCreation->IsAnyUnitPrevent();
}

bool CComplexObstacleCreation::CanBuildNext() const
{
	if ( IsAntitank( nCurIndex ) )
	{
		if ( antitanks[GetAntitankIndex(nCurIndex)] )
		{
			list<SVector> tiles;
			antitanks[GetAntitankIndex(nCurIndex)]->GetCoveredTiles( &tiles );
			for ( list<SVector>::const_iterator it = tiles.begin(); it != tiles.end(); ++it )
			{
				if ( !GetAIMap()->IsTileInside(*it ) || 0 != ( EAC_TERRAIN & GetTerrain()->GetTileLockInfo( *it ) ) )
					return false;
			}
			return true;
		}
		return false;
	}
	else
		return pFenceCreation->CanBuildNext();
}

void CComplexObstacleCreation::LockCannotBuild()
{
	//bCannot = true;
	AdvanceToNextIndex();
}

void CComplexObstacleCreation::LockNext()
{
}

CLine2 CComplexObstacleCreation::GetCurLine()
{
	return pFenceCreation->GetCurLine();
}

float CComplexObstacleCreation::GetPrice()
{
	if ( IsAntitank( nCurIndex ) )
		return SConsts::ANTITANK_RU_PRICE;
	else
		return SConsts::FENCE_SEGMENT_RU_PRICE;
}

bool CComplexObstacleCreation::IsCheatPath() const 
{
	return !bTmpNonCheatPath && pFenceCreation->IsCheatPath(); 
}

bool CComplexObstacleCreation::CannotFinish() const
{
	return false;
}

void CComplexObstacleCreation::AddWork( const float fAdd )
{
	if ( IsAntitank( nCurIndex ) )
		CLongObjectCreation::AddWork( fAdd );
	else
		pFenceCreation->AddWork( fAdd );
}

float CComplexObstacleCreation::GetWorkDone() const
{
	if ( IsAntitank( nCurIndex ) )
		return CLongObjectCreation::GetWorkDone();
	else
		return pFenceCreation->GetWorkDone();
}
REGISTER_SAVELOAD_CLASS( 0x11147C00, CComplexObstacleCreation );
