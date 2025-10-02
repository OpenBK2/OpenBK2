#include "StdAfx.h"
#include ".\fencecreation.h"
#include "StaticObjects.h"
#include "Fence.h"
#include "..\Stats_B2_M1\AIUpdates.h"
#include "UnitCreation.h"
#include "UnitsIterators2.h"
#include "AIUnit.h"
#include "UnitStates.h"


REGISTER_SAVELOAD_CLASS( 0x1508D4A9, CFenceCreation );
extern CStaticObjects theStatObjs;
extern CUnitCreation theUnitCreation;

//*******************************************************************
//*												  CFenceCreation												*
//*******************************************************************

bool CFenceCreation::IsCheatPath() const
{
	return !bTmpNonCheatPath;
}

float CFenceCreation::GetPrice()
{
	return SConsts::FENCE_SEGMENT_RU_PRICE;
}

CFenceCreation::CFenceCreation( const int nPlayer, const bool bAllowAIModification ) 
: CLongObjectCreation ( nPlayer, bAllowAIModification ), bCannot( false ), 
	bSayAck( false ), bTmpNonCheatPath( false ), line( 0, 0, 0 ), nCurIndex( -1 ), wAngle( 0 )
{
}

void CFenceCreation::LockNext()
{
}

void CFenceCreation::BuildNext()
{
	bTmpNonCheatPath = false;
	if ( CanBuildNext() )
	{
		CLongObjectCreation::BuildNext();
		theStatObjs.AddStaticObject( fenceSegements[nCurIndex], false );
		fenceSegements[nCurIndex]->Init();
		fenceSegements[nCurIndex]->LockTiles();
	}
	++nCurIndex;
	while ( nCurIndex < GetMaxIndex() && fenceSegements[nCurIndex] == 0 )
	{
		++nCurIndex;
		bTmpNonCheatPath = true;
	}
}

bool CFenceCreation::CanPlaceOnTerrain( CGivenPassabilityStObject *pObj ) const
{
	list<SVector> tiles;
	pObj->GetCoveredTiles( &tiles );
	for ( list<SVector>::const_iterator it = tiles.begin(); it != tiles.end(); ++it )
	{
		if ( !GetAIMap()->IsTileInside( *it ) || ( EAC_TERRAIN & GetTerrain()->GetTileLockInfo( *it ) ) )
			return false;
	}
	return true;
}

void CFenceCreation::CreateObjects( SAIObjectsUnderConstructionUpdate * pUpdate )
{
	if ( vPoints.empty() )
		return;

	// add all objects, check if tiles are locked and mark objects and tiles.
	pUpdate->objects.resize( fenceSegements.size() );

	pUpdate->bCanBuild = false;
	for ( int i = 0; i < fenceSegements.size(); ++i )
	{
		// check part
		list<SVector> tiles;
		fenceSegements[i]->GetCoveredTiles( &tiles );
		bool bCanBuildCurrent = false;
		for ( list<SVector>::const_iterator it = tiles.begin(); it != tiles.end(); ++it )
		{
			if ( !GetAIMap()->IsTileInside( *it ) || ( EAC_TERRAIN & GetTerrain()->GetTileLockInfo( *it ) ) )
				pUpdate->impossibleToBuildTiles.push_back( CVec2(it->x, it->y) );
			else
			{
				pUpdate->buildTiles.push_back( CVec2(it->x, it->y) );
				bCanBuildCurrent = false;
			}
		}
		fenceSegements[i]->GetNewUnitInfo( &pUpdate->objects[i].info );
		pUpdate->objects[i].bCanBuild = bCanBuildCurrent;
		pUpdate->bCanBuild |= pUpdate->objects[i].bCanBuild;
	}
	if ( !pUpdate->objects[0].bCanBuild )
		pUpdate->bCanBuild = false;
}

bool CFenceCreation::PreCreate( const CVec2 &vFrom, const CVec2 &vTo, const bool bCheckLock )
{
	bCannot = false;
	bSayAck = false;
	nCurIndex = 0;
	line = CLine2( vFrom, vTo );
	//NI_ASSERT( vPoints.empty() && parts.empty(), "repeative calls are not allowed" );
	vPoints.clear();
	fenceSegements.clear();

	const SFenceRPGStats *pRPG = theUnitCreation.GetWireFence();
	SplitLineToSegrments( &vPoints, vFrom, vTo, SConsts::TILE_SIZE *2 );
	const WORD wDir = GetDirectionByVector( vTo - vFrom );
	wAngle = GetLineAngle( vFrom, vTo );
	if ( vPoints.size() <= 1 ) return false;

	bool switcher = false;
	for ( int i = 0; i < vPoints.size() - 1; ++i )
	{
		CObj<CFence> pObj = new CFence( pRPG, CVec3(vPoints[i],0.0f), pRPG->fMaxHP, wDir, GetPlayer(), 0 );
		//
		if ( !bCheckLock || CanPlaceOnTerrain( pObj ) )
		{
			fenceSegements.push_back( pObj );
			if ( IsAIModificationAllowed() )
				pObj->Mem2UniqueIdObjs();
		}
		else
			fenceSegements.push_back( 0 );
		//
	}
	while ( nCurIndex < fenceSegements.size() && fenceSegements[nCurIndex] == 0 )
	{
		++nCurIndex;
	} 

	if ( fenceSegements.empty() ) 
		return false;
	return true;
}

bool CFenceCreation::IsCegmentToBeBuilt( class CFence *pObj ) const
{
	// проверить, нет ли какого-нить забора, на тех тайлах, которые лочит pObj

	// найти сегмент, который лочит тайлы
	SRect r1;
	pObj->GetBoundRect( &r1 );
	const float fRadius = r1.lengthAhead + r1.lengthBack + r1.width + SConsts::TILE_SIZE * 5;

	for ( CUnitsIter<0,2> iter( 0, ANY_PARTY, CVec2(pObj->GetCenter().x,pObj->GetCenter().y), fRadius ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pUnit = *iter;
		if ( !pUnit->GetStats()->IsInfantry() )
		{
			SRect rect = pUnit->GetUnitRect();
			rect.InitRect( rect.center, rect.dir, rect.lengthAhead + SConsts::TILE_SIZE, rect.lengthBack + SConsts::TILE_SIZE, rect.width + SConsts::TILE_SIZE );

			if ( r1.IsIntersected( rect ) )
			{
				if ( pUnit->GetPlayer() == GetPlayer() && EUSN_REST == pUnit->GetState()->GetName() && pUnit->CanMove() )
					(*iter)->UnlockTiles();
			}
		}
	}
	list<SVector> tiles;
	pObj->GetCoveredTiles( &tiles );
	for ( list<SVector>::iterator it = tiles.begin(); it != tiles.end(); ++it )
	{
		const SStaticObjectRPGStats * pStats = checked_cast<const SStaticObjectRPGStats *>(pObj->GetStats());
		if ( GetTerrain()->IsLocked( *it, (EAIClasses)pStats->nAIPassabilityClass ) )
			return false;
	}
	return true;
}

void CFenceCreation::GetUnitsPreventing( list< CPtr<CAIUnit> > *units )
{
	SRect r1;
	fenceSegements[nCurIndex]->GetBoundRect( &r1 );
	CLongObjectCreation::GetUnitsPreventing( r1, units );
}

bool CFenceCreation::IsAnyUnitPrevent() const
{
	SRect r1;
	fenceSegements[nCurIndex]->GetBoundRect( &r1 );
	return CLongObjectCreation::IsAnyUnitPrevent( r1 );
}

const int CFenceCreation::GetMaxIndex() const
{
	return fenceSegements.size();
}

const int CFenceCreation::GetCurIndex() const
{
	return nCurIndex;
}

bool CFenceCreation::CanBuildNext() const
{
	if ( bCannot || nCurIndex >= fenceSegements.size() || fenceSegements[nCurIndex] == 0 ) 
		return false;

	SRect r1;
	fenceSegements[nCurIndex]->GetBoundRect( &r1 );
	return CanBuildOnRect( r1, tilesUnder );
}

void CFenceCreation::CalcTilesUnder()
{
	fenceSegements[nCurIndex]->GetCoveredTiles( &tilesUnder );
}

const CVec2 CFenceCreation::GetNextPoint( const int nPlace, const int nMaxPlace ) const
{
	NI_ASSERT( nMaxPlace, "builders number = 0" );

	const CVec2 vDir			= GetVectorByDirection( wAngle );
	const CVec2 vDirPerp	( vDir.y, -vDir.x );

	SRect rect;
	fenceSegements[nCurIndex]->GetBoundRect( &rect );

	return vPoints[nCurIndex]  + vDirPerp * SConsts::TILE_SIZE * 2.0f * nPlace / nMaxPlace;
}
