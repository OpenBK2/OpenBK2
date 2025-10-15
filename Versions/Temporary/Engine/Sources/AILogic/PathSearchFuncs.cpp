#include "stdafx.h"

#include "PathFinder.h"
#include "Soldier.h"
#include "PointChecking.h"

#include <cstdint>

BASIC_REGISTER_CLASS( IStaticPathFinder );

IStaticPath* CreateStaticPathForAttack( CBasePathUnit *pUnit, CAIUnit *pTarget, const float fRangeMin, const float fRangeMax, const float fRandomCant, bool bIgnoreObstacles )
{
	// видно в любой стороны
	if ( !( pTarget->GetStats()->IsInfantry() && 
				  checked_cast<CSoldier*>(pTarget)->IsInBuilding() && ( checked_cast<CSoldier*>(pTarget)->GetMinAngle() !=0 || checked_cast<CSoldier*>(pTarget)->GetMaxAngle() != 65535 ) ) )
	{
		CPtr<IPointChecking> pPointChecking = new CAttackPointChecking( fRangeMin, (std::max)( 0.0f, fRangeMax - 4.0f * SConsts::TILE_SIZE ), pTarget->GetCenterTile(), bIgnoreObstacles );

		CVec2 vRandomCant( VNULL2 );
		if ( fRandomCant != 0.0f )
		{
			const float fRandomDist = NRandom::Random( 0.0f, fRandomCant ); RecordRandomCall();
			CVec2 vDirFromEnemy = pUnit->GetCenterPlain() - pTarget->GetCenterPlain();
			Normalize( &vDirFromEnemy );
			const CVec2 vPerpDir( -vDirFromEnemy.y, vDirFromEnemy.x );

			vRandomCant = vPerpDir * fRandomDist;
			
			RecordRandomCall();
			if ( NRandom::Random( 0.0f, 1.0f ) < 0.5f )
				vRandomCant = -vRandomCant;
		}
		
		const CVec2 vFinishPoint = pTarget->GetCenterPlain() + vRandomCant;
		IStaticPath *pPath = pUnit->CreateBigStaticPath( pUnit->GetCenterPlain(), vFinishPoint, pPointChecking );

		if ( pPath != 0 )
		{
			if ( !pUnit->CanGoToPoint( pPath->GetFinishPoint() ) )
			{
				CPtr<IStaticPath> pGarbage = pPath;
				return 0;
			}
			// путь не найден
			else if ( fabs2( pPath->GetFinishPoint() - pTarget->GetCenterPlain() ) > sqr( fRangeMax ) )
			{
				CPtr<IStaticPath> pGarbage = pPath;
				return 0;
			}
			else 
			{
				if ( fRandomCant != 0.0f )
				{
					Normalize( &vRandomCant );
					pPath->MoveFinishPointBy( vRandomCant * ( SConsts::TILE_SIZE / 2 - 1 ) );
				}

				return pPath;
			}
		}
		else
			return 0;
	}
	else
	{
		CSoldier *pSoldier = checked_cast<CSoldier*>( pTarget );
		CVec2 vAttackDir( GetVectorByDirection( ( pSoldier->GetMaxAngle() + pSoldier->GetMinAngle() ) / 2 ) );
		const CVec2 vDirFromEnemy( pSoldier->GetCenterPlain() - pUnit->GetCenterPlain() );
		const float fProj = vAttackDir * vDirFromEnemy;
		const float fMinDist = (std::min)( fProj, fRangeMax * 3.0f / 4.0f );

		return 
			CreateStaticPathForSideAttack( 
				pUnit, pTarget, vAttackDir, 
				fRangeMin, fRangeMax, fMinDist, ( pSoldier->GetMaxAngle() - pSoldier->GetMinAngle() ) / 2, bIgnoreObstacles );
	}
}

IStaticPath* CreateStaticPathForSideAttack( CBasePathUnit *pUnit, CAIUnit *pTarget, const CVec2 &attackDir, const float fRangeMin, const float fRangeMax, const float fDistToPoint, const uint16_t wHalfAngle, const bool bIgnoreObstacles )
{
	const uint16_t wAttackDir = GetDirectionByVector( attackDir );
	CVec2 vNormAttackDir( attackDir );
	Normalize( &vNormAttackDir );
	
	CPtr<IPointChecking> pPointChecking = new CAttackSideChecking( fRangeMin, fRangeMax, pTarget->GetCenterTile(), wAttackDir, wHalfAngle, bIgnoreObstacles );
	IStaticPath *pPath = pUnit->CreateBigStaticPath( pUnit->GetCenterPlain(), pTarget->GetCenterPlain() + vNormAttackDir * fDistToPoint, pPointChecking );

	if ( pPath != 0 )
	{
		CVec2 finishDir = pPath->GetFinishPoint() - pTarget->GetCenterPlain();
		const uint16_t wFinishDir = GetDirectionByVector( finishDir );
		// нельзя дойти до точки, откуда можно будет атаковать
		if ( DirsDifference( wFinishDir, wAttackDir ) > wHalfAngle || fabs2( pPath->GetFinishPoint() - pTarget->GetCenterPlain() ) > sqr( fRangeMax ) )
		{
			CPtr<IStaticPath> pGarbage = pPath;			
			return 0;
		}
	}
	
	if ( pPath && !pUnit->CanGoToPoint( pPath->GetFinishPoint() ) )
	{
		CPtr<IStaticPath> pGarbage = pPath;		
		return 0;
	}
	else
		return pPath;
}

IStaticPath* CreatePathWithChecking( CBasePathUnit *pUnit, const SVector &vTargetTile, IPointChecking *pPointChecking )
{
	// чтобы удалилось
	CPtr<IPointChecking> pGarbage = pPointChecking;

	IStaticPath *pPath = pUnit->CreateBigStaticPath( pUnit->GetCenterPlain(), AICellsTiles::GetPointByTile( vTargetTile ), pPointChecking );
	if ( pPath != 0 && ( !pUnit->CanGoToPoint( pPath->GetFinishPoint() ) || !pPointChecking->IsGoodTile( pPath->GetFinishTile() ) ) )
	{
		CPtr<IStaticPath> pGarbage = pPath;		
		return 0;
	}

	return pPath;
}

IStaticPath* CreateStaticPathForStObjAttack( CBasePathUnit *pUnit, CStaticObject *pObj, const float fRangeMin, const float fRangeMax, const bool bIgnoreObstacles )
{
	const CVec2 vUnitCenter( pUnit->GetCenterPlain() );	
	
	CPtr<IPointChecking> pPointChecking = new CAttackStObjectChecking( fRangeMin, fRangeMax, pObj, bIgnoreObstacles );
	IStaticPath *pPath = pUnit->CreateBigStaticPath( vUnitCenter, pObj->GetAttackCenter( vUnitCenter ), pPointChecking );

	if ( pPath != 0 && !pUnit->CanGoToPoint( pPath->GetFinishPoint() ) )
	{
		CPtr<IStaticPath> pGarbage = pPath;
		return 0;
	}

	return pPath;
}

bool CanUnitApproachToUnitByPath( const CAIUnit *Moving, const IStaticPath *Path, const CAIUnit *Standing )
{
	if( !Path ) 
		return false;

	SRect rMoving = Moving->GetUnitRect();
	CVec2 vShift = Path->GetFinishPoint() - Moving->GetCenterPlain();
	SRect rMovingFinal;
	rMovingFinal.InitRect( rMoving.v1() + vShift, rMoving.v2() + vShift, rMoving.v3() + vShift, rMoving.v4() + vShift);
	
	SRect rStanding = Standing->GetUnitRect();
	rMovingFinal.Compress( 1.2f );
	rStanding.Compress( 1.2f );
	
	return 
		rMovingFinal.IsIntersected( rStanding );
}

bool CanUnitApproachToPointByPath( const CAIUnit *Moving, const IStaticPath *Path, const CVec2 &point )
{
	if( !Path ) 
		return false;

	SRect rMoving = Moving->GetUnitRect();
	CVec2 vShift = Path->GetFinishPoint() - Moving->GetCenterPlain();
	SRect rMovingFinal;
	rMovingFinal.InitRect( rMoving.v1() + vShift, rMoving.v2() + vShift, rMoving.v3() + vShift, rMoving.v4() + vShift);

	rMovingFinal.center = Path->GetFinishPoint();

	rMovingFinal.Compress( 1.2f );
	
	return 
		rMovingFinal.IsPointInside( point );
}

bool CanUnitApproachToObjectByPath( const CAIUnit *Moving, const IStaticPath *Path, const CStaticObject *standing )
{
	if( !Path ) 
		return false;

	SRect rMoving = Moving->GetUnitRect();
	CVec2 vShift = Path->GetFinishPoint() - Moving->GetCenterPlain();
	SRect rMovingFinal;
	rMovingFinal.InitRect( rMoving.v1() + vShift, rMoving.v2() + vShift, rMoving.v3() + vShift, rMoving.v4() + vShift);

	SRect rStanding;
	standing->GetBoundRect( &rStanding );

	rMovingFinal.Compress( 1.2f );
	rStanding.Compress( 1.2f );
	
	return 
		rMovingFinal.IsIntersected( rStanding );
}

bool IsUnitNearObject( const CAIUnit *pUnit, const CStaticObject *pObj )
{
	SRect r1 = pUnit->GetUnitRect();
	SRect r2;
	pObj->GetBoundRect( &r2 );

	r1.Compress( 1.2f );
	r2.Compress( 1.2f );
	
	return r1.IsIntersected( r2 );
}

bool IsUnitNearUnit( const CAIUnit *pUnit1, const CAIUnit *pUnit2 )
{
	SRect r1 = pUnit1->GetUnitRect();
	SRect r2 = pUnit2->GetUnitRect();
	
	r1.Compress( 1.2f );
	r2.Compress( 1.2f );
	
	return r1.IsIntersected( r2 );
}

bool IsUnitNearPoint( const CAIUnit *pUnit, const CVec2 &point, const int add )
{
	SRect r1 = pUnit->GetUnitRect();
	r1.InitRect( r1.center, r1.dir, r1.width + add, r1.lengthAhead + add);

	return r1.IsPointInside( point );
}

bool IsPointNearPoint( const CVec2 &point1, const CVec2 &point2 )
{
	return fabs2(point1-point2) < sqr(static_cast<int>(SConsts::TILE_SIZE) );
}


