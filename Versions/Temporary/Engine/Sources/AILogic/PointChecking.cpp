#include "stdafx.h"

#include "PointChecking.h"
#include "GlobalWarFog.h"
#include "AIGeometry.h"
#include "StaticObject.h"

#include <cstdint>

extern CGlobalWarFog theWarFog;

bool CheckObstacles( const bool bIgnoreObstacles, const SVector &curTile, const SVector &targetTile )
{
	if ( !bIgnoreObstacles )
	{
		const SVector vVisCurTile( curTile.x / AI_TILES_IN_VIS_TILE, curTile.y / AI_TILES_IN_VIS_TILE );
		const SVector vVisTargetTile( targetTile.x / AI_TILES_IN_VIS_TILE, targetTile.y / AI_TILES_IN_VIS_TILE );
		if ( !theWarFog.IsTraceable( vVisCurTile, vVisTargetTile ) )
			return false;
	}
	return true;
}

//*******************************************************************
//*											CAttackPointChecking												*
//*******************************************************************

bool CAttackPointChecking::IsGoodTile( const SVector &curTile ) const
{
	if ( !CheckObstacles( bIgnoreObstacles, curTile, targetTile ) )
		return false;

	const float fDist = SquareOfDistance( curTile, targetTile );

	return 
		fDist <= sqr( long( fRangeMax / SConsts::TILE_SIZE ) ) &&
		fDist >= sqr( long( fRangeMin / SConsts::TILE_SIZE ) );
}

//*******************************************************************
//*												CGoToDistance															*
//*******************************************************************

bool CGoToDistance::IsGoodTile( const SVector &curTile ) const
{
	return SquareOfDistance( curTile, targetTile ) <= tileDistance2;
}

//*******************************************************************
//*											CAttackSideChecking													*
//*******************************************************************

bool CAttackSideChecking::IsGoodTile( const SVector &curTile ) const
{
	if ( !CheckObstacles( bIgnoreObstacles, curTile, targetTile ) )
		return false;

	const uint16_t wCurDir = GetDirectionByVector( (curTile - targetTile).ToCVec2() );
	const float fDist = SquareOfDistance( curTile, targetTile );

	const bool bRet =	
		DirsDifference( wCurDir, wAttackDir ) <= wHalfAngle &&
		fDist <= sqr( long( fRangeMax / SConsts::TILE_SIZE ) ) &&
		fDist >= sqr( long( fRangeMin / SConsts::TILE_SIZE ) );
	
	return bRet;
}

//*******************************************************************
//*											CAttackStObjectChecking											*
//*******************************************************************

CAttackStObjectChecking::CAttackStObjectChecking( const float _fRangeMin, const float _fRangeMax, CStaticObject *_pObj, const bool _bIgnoreObstacles  )
: fRangeMin( _fRangeMin ), fRangeMax( _fRangeMax ), pObj( _pObj ), bIgnoreObstacles( _bIgnoreObstacles )
{
}

bool CAttackStObjectChecking::IsGoodTile( const SVector &curTile ) const
{
	if ( !pObj->IsAlive() )
		return true;

	const SVector targetTile = AICellsTiles::GetTile( pObj->GetAttackCenter( AICellsTiles::GetPointByTile( curTile ) ) );
	if ( !CheckObstacles( bIgnoreObstacles, curTile, targetTile ) )
		return false;

	const float fDist = SquareOfDistance( curTile, targetTile );

	return 
		fDist <= sqr( long( fRangeMax / SConsts::TILE_SIZE ) ) &&
		fDist >= sqr( long( fRangeMin / SConsts::TILE_SIZE ) );
}


