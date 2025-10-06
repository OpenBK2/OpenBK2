#include "stdafx.h"

#include "BasePathUnit.h"
#include "CommonPathFinder.h"
#include "Collision.h"
#include "PathFinder.h"
#include "StandartPath2.h"
#include "StandartSmoothMechPath.h"
#include "StaticPathInternal.h"

#include "Common_RTS_AI/StaticMapHeights.h"
#include "System/RandomGen.h"
#include <float.h>


#include "PointChecking.h"

BASIC_REGISTER_CLASS ( IPointChecking );

const WORD TURN_TOLERANCE = 0;
NTimer::STime lastTimeDiff = 50;

struct SCheckTurnIterateUnitsCallback : public SIterateUnitsCallback
{
	std::vector<SRect> mediateRects;
	//
	SCheckTurnIterateUnitsCallback( const std::vector<SRect> &_mediateRects ): mediateRects( _mediateRects ) {}
	//
	bool Iterate( CBasePathUnit *unit ) const
	{
		const bool bCanPush = unit->ShouldScatter() /* && unit's collision's priority is greater or less than owner collision's priority */;
		if ( !bCanPush )
		{
			const SUnitProfile profile( unit->GetUnitProfile() );
			for ( int i = 0; i < mediateRects.size(); ++i )
			{
				if ( profile.IsIntersected( mediateRects[i] ) )
					return false;
			}
		}
		return true;
	}
};

CBasePathUnit::CBasePathUnit()
{
	bPlacementUpdated = false;
	bStoppedSent = false;
	eMovementPlane = PLANE_TERRAIN;
	vCenter = VNULL3;
	wDirection = 0;
	wFrontDirection = wDirection;
	wLastDirection = wDirection;
	fDesiredSpeed = -1.0f;
	bGoForward = true;
	pLastPushUnit = 0;
	pLastPusherUnit = 0;
	vTile = SVector( 0, 0 );
	vLastKnownGoodTile = vTile;

	bIdle = true;

	fSpeed = 0.0f;

	bLocking = bOnLockedTiles = bFixUnlocking = false;

	bTurnCalled = bTurning = bTurningToDirContinuesly = false;

	wDirToContinueslyTurn = 0;

	pSmoothPath = 0;

	pDefaultPath = pSmoothPath;
	pPathMemento = 0;
	stayTime = 0;
	collStayTime = 0;
	nextSecondPathSegmTime = 0;
	checkOnLockedTime = 0;

	pCurrentCollision = 0;
	nCollisionsCount = 0;
	bNoCollision = false;
	pInterruptedCollision = 0;

	pInterruptedPath = 0;

	bMaxSlowed = bMinSlowed = bNotified = false;
	wOldDirection = 0;
	//DebugTrace( "wOldDirection = %d(0x%04x)", wOldDirection, wOldDirection );
}

ISmoothPath* CBasePathUnit::CreateSmoothPath()
{
	ISmoothPath *result = IsInfantry() ? MakeObject<ISmoothPath>( STANDART_SMOOTH_SOLDIER_PATH ) : MakeObject<ISmoothPath>( STANDART_SMOOTH_MECH_PATH );
	CPtr<IPath> pPath = CreatePathByDirection( GetCenterPlain(), CVec2( 1, 1 ), GetCenterPlain(), pAIMap );
	result->Init( this, pPath, !IsInfantry(), true, pAIMap );
	return result;
}

void CBasePathUnit::Init( const CVec3 &_vCenter, const WORD _wDirection, CAIMap *_pAIMap, ICollisionsCollector *_pCollisionsCollector, CCommonPathFinder *_pPathFinder )
{
	pPathFinder = _pPathFinder;
	pAIMap = _pAIMap;
	pCollisionsCollector = _pCollisionsCollector;

	bTurningToDirContinuesly = false;
	wDirToContinueslyTurn = _wDirection;

	nextSecondPathSegmTime = 0;
	checkOnLockedTime = 0;

	vCenter = _vCenter;
	vOldPlacement = vCenter;
	wDirection = _wDirection;
	wFrontDirection = wDirection;
	wLastDirection = wDirection;
	wOldDirection = wFrontDirection;
	fDesiredSpeed = -1.0f;
	bGoForward = true;
	pLastPushUnit = 0;
	pLastPusherUnit = 0;
	vTile = pAIMap->GetTile( CVec2( vCenter.x, vCenter.y ) );
	vLastKnownGoodTile = GetCenterTile();

	eMovementPlane = ( pAIMap->GetTerrain()->GetTerrainType( GetCenterTile() ) == ETT_WATER_TERRAIN ) ? PLANE_WATER : PLANE_TERRAIN;

	bIdle = true;

	fSpeed = 0.0f;

	bLocking = bOnLockedTiles = bFixUnlocking = false;

	bTurnCalled = bTurning = false;

	pSmoothPath = CreateSmoothPath();

	pDefaultPath = pSmoothPath;
	stayTime = 0;
	collStayTime = 0;

	nCollisionsCount = 0;
	pCurrentCollision = 0;
	pInterruptedCollision = 0;
	pPathMemento = 0;
	bMaxSlowed = bMinSlowed = bNotified = false;
	pInterruptedPath = 0;
	bPlacementUpdated = false;
	bStoppedSent = false;

	bNoCollision = true;
	{
		CPtr<ICollision> pCollision = CreateCollision( this, 0, -1, NCollision::ECN_FREE );
	}
	bNoCollision = false;
}

const SRect CBasePathUnit::GetUnitModifiedRect( const float fCompress ) const
{
	SRect result;
	const SUnitProfile profile( GetUnitProfile() );
	if ( profile.bRect )
		result = profile.rect;
	else
		result.InitRect( profile.circle.center, GetVectorByDirection( GetFrontDirection() ), profile.circle.r, profile.circle.r, 2.0f * profile.circle.r );

	result.Compress( fCompress );
	return result;
}

void CBasePathUnit::SetDirection( const WORD _wDirection, const bool bNeedUpdate )
{
//	const WORD wNewDirection = IsGoForward() ? _wDirection : _wDirection + 32768;
	if ( wDirection != _wDirection /*wNewDirection != GetDirection() */)
	{
		if ( !bPlacementUpdated && bNeedUpdate )
			bPlacementUpdated = true;

		bTurning = true;
		const bool bNeedLockTiles		= IsLockingTiles();
		if ( bNeedLockTiles )
			UnlockTiles();

		wDirection = _wDirection;
		wFrontDirection = IsGoForward() ? wDirection : wDirection + 32768;

		if ( bNeedLockTiles )
			LockTiles();
	}
}

void CBasePathUnit::SetCenter( const CVec3 &_vCenter, const bool bNeedUpdate )
{
	//DEBUG{ переодически попадаются "подземные юниты" (с z координатой равной 0). выставлять z координату - 
	//головная боль тех, кто двигает юнит. полная трехмерность ;-)
	//if ( _vCenter.z <= 0.1f )
	//	DebugTrace( "WARNING: undeground position !!!" );
	//DEBUG}

	//if ( GetUniqueID() == 22 )
	//	DebugTrace( "%2.3f x %2.3f x %2.3f", _vCenter.x, _vCenter.y, _vCenter.z );

	const bool bNeedLockTiles = IsLockingTiles();
	if ( !bPlacementUpdated && bNeedUpdate )
		bPlacementUpdated = true;

	const CVec3 vOldCenter = GetCenter();
	vCenter = _vCenter;
	const SVector vNewTile = pAIMap->GetTile( CVec2( vCenter.x, vCenter.y ) );
	if ( vTile != vNewTile )
	{
		UpdateTile();

		if ( bNeedLockTiles )  
			UnlockTiles();

		const EMovementPlane newPlane = ( pAIMap->GetTerrain()->GetTerrainType( vNewTile ) == ETT_WATER_TERRAIN ) ? PLANE_WATER : PLANE_TERRAIN;
		if ( abs( vTile.x - vNewTile.x ) < 2 && abs( vTile.y - vNewTile.y ) < 2 )
		{
			if ( newPlane != eMovementPlane )
			{
				if ( eMovementPlane != PLANE_WATER || !pAIMap->GetTerrain()->IsBridge( vNewTile ) )
					eMovementPlane = newPlane;
			}
		}
		else
			eMovementPlane = newPlane;
		vTile = vNewTile;

		STerrainModeSetter modeSetter( ELM_STATIC, pAIMap->GetTerrain() );
		if ( pAIMap->GetTerrain()->CanUnitGo( GetBoundTileRadius(), vNewTile, GetAIPassabilityClass() ) != FREE_NONE )
			vLastKnownGoodTile = vNewTile;

		if ( bNeedLockTiles )
			LockTiles();
	}

	if ( !bNeedUpdate )
	{
		UpdatePlacement( vOldCenter, wOldDirection, false );
		vOldPlacement = GetCenter();
	}
	//bPlacementUpdated |= bNeedUpdate;
}

void CBasePathUnit::SetGoForward( const bool _bGoForward )
{
	if ( _bGoForward != bGoForward )
	{
		bGoForward = _bGoForward;
		wDirection += 32768;
	}
}

const bool CBasePathUnit::MakeTurnToDirection( const WORD _wDirection )
{
	const WORD wNewFrontDir = IsGoForward() ? _wDirection : _wDirection + 32768;
	const WORD wClockWise = wNewFrontDir - GetFrontDirection();
	const WORD wAntiClockWise = GetFrontDirection() - wNewFrontDir;

	const WORD wMinTurn = (std::min)( wClockWise, wAntiClockWise );
	if ( wMinTurn < TURN_TOLERANCE )
	{
		SetDirection( _wDirection );
		bTurning = false;
		return true;
	}
	else
	{
		const NTimer::STime turnTime = wMinTurn / GetTurnSpeed();
		if ( turnTime < lastTimeDiff )
		{
			SetDirection( _wDirection );
			bTurning = false;
			return true;
		}
		else
		{
			const int nTurnMultiply = ( wClockWise < wAntiClockWise ) ? int( lastTimeDiff ) : -int( lastTimeDiff );
			NI_ASSERT( (_MCW_RC  & _control87( 0, 0 )) == 0, "something changed processor control word" );
			SetDirection( GetDirection() + Float2Int( nTurnMultiply * GetTurnSpeed() ) );
			bTurning = true;
			return false;
		}
	}
}

const bool CBasePathUnit::TurnToDirection( const WORD _wDirection, const bool bCanBackward, const bool bCanForward )
{
	bTurnCalled = true;
	if ( GetTurnSpeed() == 0 || CanTurnInstant() )
	{
		SetDirection( _wDirection );
		bTurning = false;
		return true;
	}
	else
	{
		// начало поворота или поворот к другому направлению
		if ( !bTurning || _wDirection != wLastDirection )
		{
			if ( bCanBackward && bCanForward )
			{
				if ( GetSmoothPath()->CanGoBackward() || bOnLockedTiles )
					SetGoForward( DirsDifference( GetFrontDirection(), _wDirection ) <= DirsDifference( GetFrontDirection() + 32768, _wDirection ) );
				else
					SetGoForward( true );
			}
			else
				SetGoForward( bCanForward || !CanGoBackward() );

			wLastDirection = _wDirection; 
		}

		return MakeTurnToDirection( _wDirection );
	}
}

const bool CBasePathUnit::TurnToTarget( const CVec2 &vTarget )
{
	const CVec3 vPos( GetCenter() );
	return TurnToDirection( GetDirectionByVector( CVec2( vTarget.x - vPos.x, vTarget.y - vPos.y ) ), false, true );
}

bool CBasePathUnit::CanMake180DegreesTurn( SRect rect ) const
{
	if ( IsRound() )
		return true;

	CTemporaryUnitProfileUnlocker unlocker( GetUniqueID(), GetUnitProfile(), IsLockingTiles(), false, pAIMap->GetTerrain() );

	const CVec2 vRotateAngle( 1.0f/FP_SQRT_2, 1.0f/FP_SQRT_2 );
	for ( int i = 0; i < 4; ++i )
	{
		rect.InitRect( rect.center, rect.dir ^ vRotateAngle, rect.lengthAhead, rect.lengthBack, rect.width );

		if ( IsOnLockedTiles( rect ) )
			return false;
	}

	return true;
}

const bool CBasePathUnit::CanTurnTo( const WORD wNewDir, const bool bCanRebuildPath )
{
	if ( IsRound() )
		return true;

	const SUnitProfile profile( GetUnitProfile() );
	if ( profile.IsCircle() )
		return true;

	const SRect rect( profile.rect );
	const CVec2 vUnitCenter( GetCenterPlain() );
	const WORD wUnitDir( GetFrontDirection() );
	CVec2 vBestPoint( -1.0f, -1.0f );

	bool bResult = true;
	{
		//		CTemporaryUnitRectUnlocker unlocker( GetUniqueID(), GetUnitRect(), IsLockingTiles(), false );

		if ( !CheckTurn( 1.0f, GetVectorByDirection( wNewDir ), true, false ) )
		{
			float fBestTime = 1000000;

			const float fMinAngle = 22.5f;
			const CVec2 vMinAngle( NMath::Cos( fMinAngle / 180.0f * FP_PI ), NMath::Sin( fMinAngle / 180.0f * FP_PI ) );
			CVec2 vCurDir( GetFrontDirectionVector() );

			for ( int i = 0; i < 360 / fMinAngle; ++i )
			{
				const WORD wCurDir( GetDirectionByVector( vCurDir ) );
				const WORD wRightDirDiff = DirsDifference( wUnitDir, wCurDir );
				const WORD wBackDirDiff = DirsDifference( wUnitDir + 32768, wCurDir );
				const WORD wDirsDiff = (std::min)( wRightDirDiff, wBackDirDiff );
				if ( ( CanGoBackward() || wDirsDiff == wRightDirDiff ) && 
					CheckTurn( 1.0f, vCurDir, false, true ) )
				{
					CVec2 vPoint( vUnitCenter + vCurDir * pAIMap->GetTileSize() );
					SRect alongPathRect( rect );
					alongPathRect.InitRect( vPoint, vCurDir, rect.lengthAhead, rect.lengthBack, rect.width );

					bool bOnLockTiles = false;
					bool bCanTurn = false;
					do
					{
						bOnLockTiles = IsOnLockedTiles( alongPathRect );
						bCanTurn = CanMake180DegreesTurn( alongPathRect );

						if ( !bOnLockTiles && !bCanTurn )
						{
							vPoint += vCurDir * pAIMap->GetTileSize();
							alongPathRect.InitRect( vPoint, vCurDir, rect.lengthAhead, rect.lengthBack, rect.width );
						}
					}
					while ( !bOnLockTiles && !bCanTurn );

					if ( !bOnLockTiles && bCanTurn )
					{
						const float fTime = 
							fabs( vPoint - vUnitCenter ) * GetMaxPossibleSpeed() + 
							wDirsDiff * GetTurnSpeed();

						if ( fTime < fBestTime )
						{
							fBestTime = fTime;
							vBestPoint = vPoint;
						}
					}
				}

				vCurDir = vCurDir ^ vMinAngle;
			}

			bResult = false;
		}
		else
			bResult = true;
	}

	if ( !bResult && bCanRebuildPath && vBestPoint.x != -1.0f )
	{
		if ( pInterruptedCollision == 0 )
		{
			pInterruptedCollision = pCurrentCollision;
			pPathMemento = pSmoothPath->CreateMemento();
		}

		bNoCollision = true;
		const CVec2 vDir( vBestPoint - vUnitCenter );
		SendAlongPath( CreatePathByDirection( vUnitCenter, vDir, vBestPoint, pAIMap ) );
		bNoCollision = false;
	}

	return bResult;
}

const bool CBasePathUnit::CanTurnToFrontDir( const WORD _wDirection ) const
{
	return CheckTurn( 1.0f, GetVectorByDirection( _wDirection ), true, false );
}

const bool CBasePathUnit::CheckTurn( const float fRectCoeff, const CVec2 &vDir, const bool bWithUnits, const bool bCanGoBackward ) const
{
	if ( IsRound() )
		return true;
	if ( GetFrontDirection() != GetDirectionByVector( vDir ) && !CanRotate() )
		return false;

	const SRect rect( GetUnitModifiedRect( fRectCoeff ) );

	CTemporaryUnitProfileUnlocker unlocker( GetUniqueID(), GetUnitProfile(), IsLockingTiles(), GetMovementPlane() == PLANE_WATER, pAIMap->GetTerrain() );

	const WORD wCurDir = GetDirectionByVector( rect.dir ); // GetFrontDirection();
	WORD wFinalDir = GetDirectionByVector( vDir );
	const WORD wFinalDirBack = wFinalDir + 32768;

	// если выгоднее ехать задом
	if ( bCanGoBackward && DirsDifference( GetFrontDirection(), wFinalDirBack ) < DirsDifference( GetFrontDirection(), wFinalDir ) )
		wFinalDir = wFinalDirBack;

	const WORD wClockWise = wFinalDir - GetFrontDirection();
	const WORD wAntiClockWise = GetFrontDirection() - wFinalDir;
	const WORD wBestDir = (std::min)( wClockWise, wAntiClockWise );
	const int nDirSign = ( wClockWise < wAntiClockWise ) ? ( 1 ) : ( -1 );

	const int nParts = 4;
	const int nAdd = nDirSign * int( wBestDir / nParts );
	std::vector<SRect> mediateRects( nParts );
	for ( int i = 1; i <= nParts; ++i )
	{
		const WORD wMediateDir = wCurDir + i * nAdd;  // dirSign * int(bestDir / 2);
		mediateRects[i-1].InitRect( rect.center, GetVectorByDirection( wMediateDir ), rect.lengthAhead, rect.lengthBack, rect.width );
	}

	for ( int i = 0; i < nParts; ++i )
	{
		if ( pAIMap->IsRectOnLockedTiles( mediateRects[i], GetAIPassabilityClass() ) )
			return false;
	}

	if ( bWithUnits )
		return IterateUnits( rect.center, rect.lengthAhead, false, SCheckTurnIterateUnitsCallback( mediateRects ) );
	return true;
}

void CBasePathUnit::SetSpeed( const EAdjustSpeedParam eAdjust, const float fValue )
{
	switch( eAdjust ) 
	{
	case ADJUST_SLOW:
		fSpeed /= fValue;
		break;
	case ADJUST_FAST:
		fSpeed *= fValue;
		break;
	case ADJUST_SET:
		fSpeed = fValue;
		break;
	}
}

void CBasePathUnit::LockTiles()
{
	if ( CanLockTiles() && !IsInfantry() && !bFixUnlocking && !IsStaticUnit() )
	{
		pAIMap->GetTerrain()->AddUnitTiles( GetUniqueID(), GetUnitProfile(), GetMovementPlane() == PLANE_WATER );
		bLocking = true;
	}
}

void CBasePathUnit::ForceLockTiles()
{
	if ( CanLockTiles() && !IsStaticUnit() )
	{
		pAIMap->GetTerrain()->AddUnitTiles( GetUniqueID(), GetUnitProfile(), GetMovementPlane() == PLANE_WATER );
		bLocking = true;
	}
}

void CBasePathUnit::UnlockTiles()
{
	if ( bLocking && !IsStaticUnit() )
	{
		pAIMap->GetTerrain()->RemoveUnitTiles( GetUniqueID() );
		bLocking = false;
	}
}

void CBasePathUnit::StaticLockTiles() const
{
	if ( IsStaticUnit() )
	{
		SUnitProfile profile( GetUnitProfile() );
		std::list<SObjTileInfo> tiles;
		if ( profile.bRect )
		{
			const SRect &rect( profile.rect );
			pAIMap->GetTilesCoveredByQuadrangle( rect.v1, rect.v2, rect.v3, rect.v4, &tiles );
		}
		else
		{
			const CCircle &circle( profile.circle );
			pAIMap->GetTilesCoveredByCircle( circle.center, circle.r, &tiles );
		}
		if ( GetMovementPlane() == PLANE_WATER )
			for ( std::list<SObjTileInfo>::iterator it = tiles.begin(); it != tiles.end(); ++it )
				it->lockInfo = EAC_WATER;
		else
			for ( std::list<SObjTileInfo>::iterator it = tiles.begin(); it != tiles.end(); ++it )
				it->lockInfo = EAC_TERRAIN;
		pAIMap->GetTerrain()->AddStaticObjectTiles( tiles );
	}
}

const bool CBasePathUnit::IsOnLockedTiles( const SUnitProfile &profile ) const
{
	CTemporaryUnitProfileUnlocker unlocker( GetUniqueID(), GetUnitProfile(), IsLockingTiles(), GetMovementPlane() == PLANE_WATER, pAIMap->GetTerrain() );

	if ( profile.bRect )
		return pAIMap->IsRectOnLockedTiles( profile.rect, GetAIPassabilityClass() );
	else
		return pAIMap->IsCircleOnLockedTiles( profile.circle, GetAIPassabilityClass() );
}

void CBasePathUnit::Stop()
{
	pLastPushUnit = 0;

	GetSmoothPath()->Stop();

	if ( pCurrentCollision->GetName() != NCollision::ECN_FREE )
	{
		CPtr<ICollision> pCollision = CreateCollision( this, 0, -1, NCollision::ECN_FREE );
	}

	pPathMemento = 0;
	pInterruptedCollision = 0;

	fSpeed = 0;
	StopTurning();
	if ( IsInfantry() )
		UnlockTiles();

	CalculateIdle();
}

void CBasePathUnit::AdjustSpeed()
{
	if ( !bNotified )
	{
		const float fDiffSpeed = lastTimeDiff * GetMaxPossibleSpeed() / 2000.0f; // old version works with 50ms AI_SEGMENT
		const float minSpeed = fSpeed - fDiffSpeed;//GetMaxPossibleSpeed() / 40.0f;
		const float maxSpeed = fSpeed + fDiffSpeed;//GetMaxPossibleSpeed() / 40.0f;
		float maxSpeedHere = GetMaxSpeedHere();		

		if ( maxSpeed < maxSpeedHere )
			fSpeed = maxSpeed;
		else if ( minSpeed > maxSpeedHere )
			fSpeed = minSpeed;
		else
			fSpeed = maxSpeedHere;
	}
	else if ( IsInfantry() )
		fSpeed = GetMaxSpeedHere();
}

void CBasePathUnit::FirstSegment( const NTimer::STime timeDiff )
{
	lastTimeDiff = timeDiff;
	if ( CanMoveCritical() && !IsIdle() )
	{
		pCurrentCollision->Segment( timeDiff );

 		if ( !IsAviation() )
		{
			if ( GetCollStayTime() != 0 )
			{
				const float fProb = ( 3000.0f + NRandom::Random( 0.0f, 800.0f ) ) / float( GetCollStayTime() );
				if ( NRandom::Random( 0.0f, 1.0f ) > fProb )
				{
					CPtr<ICollision> pCollision = CreateCollision( this, 0, -1, NCollision::ECN_STOP );
				}
			}
			ResetCollStayTime();
			ResetCollisionsCount();
		}

		if ( pCurrentCollision->IsSolved() && pInterruptedCollision != 0 )
		{
			UnlockTiles();

			pCurrentCollision = pInterruptedCollision;
			pSmoothPath = pInterruptedPath;
			GetSmoothPath()->Init( pPathMemento, this, pAIMap );

			pLastPushUnit = 0;
			pPathMemento = 0;
			pInterruptedCollision = 0;
		}

		if ( !pCurrentCollision->IsSolved() )
			pCurrentCollision->FindCandidates( pCollisionsCollector );

		if  ( IsMoving() && pCurrentCollision->IsSolved() )
			Stop();
	}
}

void CBasePathUnit::CallUpdatePlacement()
{
	if ( bPlacementUpdated )
	{
		bPlacementUpdated = false;
		UpdatePlacement( vOldPlacement, wOldDirection, true );
		vOldPlacement = GetCenter();
		wOldDirection = GetFrontDirection();
	}
}

void CBasePathUnit::SecondSegment( const NTimer::STime timeDiff )
{
	if ( bTurning && !bTurnCalled )
		bTurning = false;

	bTurnCalled = false;

	if ( CanMoveCritical() && ( !IsIdle() || stayTime == 0 ) )
	{
		bTurningToDirContinuesly = false;
		bool bPathIsFinished = pSmoothPath->IsFinished();
		const WORD oldDir = GetFrontDirection();

		const CVec2 vOldCenter( GetCenterPlain() );
		const SVector vOldTile( GetCenterTile() );

		if ( bNotified || !bMaxSlowed || fSpeed != 0 )
		{
			AdjustSpeed();
			pSmoothPath->Segment( timeDiff );
			fSpeed = GetSmoothPath()->GetSpeed();
			if ( fSpeed < 0.0f )
				fSpeed = fabs( GetCenterPlain() - vOldCenter ) / timeDiff;
		}
		bNotified = bMaxSlowed = bMinSlowed = false;

		// сдвинулись
		if ( GetCenterPlain() != vOldCenter || GetFrontDirection() != oldDir )
			stayTime = 0;
		else
			stayTime += timeDiff;

		if ( GetCenterTile() != vOldTile && fabs( vCenter.z - pAIMap->GetHeights()->GetZ( GetCenter() ) ) < 0.001f && !IsInfantry() )
			CheckForDestroyedObjects( GetCenterPlain() );

		if ( checkOnLockedTime < timeDiff && !IsInfantry() && !IsAviation() )
		{
			bOnLockedTiles = IsOnLockedTiles( GetUnitProfile() );
			checkOnLockedTime = 2 * timeDiff;
		}
		else
			checkOnLockedTime -= timeDiff;

		if ( !bPathIsFinished && pSmoothPath->IsFinished() )
			NullSegmTime();

		SetNextSecondPathSegmTime( 0 );
	}
	else if ( !bTurning )
	{
		if ( bTurningToDirContinuesly )
		{
			if ( TurnToDirection( wDirToContinueslyTurn, false, true ) )
				bTurningToDirContinuesly = false;
		}

		fSpeed = 0.0f;
		LockTiles();

		SetNextSecondPathSegmTime( NRandom::Random( 250, 550 ) );
	}

	CalculateIdle();

	if ( !IsIdle() || IsTurning() )
		SetNextSecondPathSegmTime( 0 );

	if ( bPlacementUpdated )
		bStoppedSent = false;
	else if ( !bStoppedSent )
	{
		OnStopped();
		bStoppedSent = true;
	}
	CallUpdatePlacement();
}

const float CBasePathUnit::GetMaxSpeedHere( const CVec2 &vPosition, const bool bAdjust ) const
{
	const float fMapPass = pAIMap->GetTerrain()->GetPass( vPosition );
	float fSpeed = GetMaxPossibleSpeed() * ( fMapPass + ( 1 - fMapPass ) * GetPassability() );
	if ( bAdjust && GetDesiredSpeed() > 0.0f  )
		fSpeed = (std::min)( fSpeed, GetDesiredSpeed() );

	return fSpeed;
}

IStaticPath *CBasePathUnit::CreateBigStaticPath( const CVec2 &vStartPoint, const CVec2 &vFinishPoint, IPointChecking *pChecking )
{
	CPtr<IStaticPath> pPath = 0;
	const bool bNeedRelock = IsLockingTiles();
	if ( bNeedRelock )
		UnlockTiles();
	{
		STerrainModeSetter modeSetter( ELM_STATIC, pAIMap->GetTerrain() );
		pPathFinder->SetCheckingPathParameters( GetBoundTileRadius(), GetAIPassabilityClass(), vStartPoint, vFinishPoint, GetLastKnownGoodTile(), pChecking, pAIMap );	
		pPath = pPathFinder->CreatePath( false );
		if ( !IsInfantry() && !pChecking && IsValid( pPath ) && pPath->GetLength() > 2 )
		{
			const SVector vFinishTile = pAIMap->GetTile( vFinishPoint );
			bool bPathToFinishPoint = mDistance( pPath->GetFinishTile(), vFinishTile ) < 2;
			bool bEmptyPathFound = false;
			int nStepCount = 4;
			while ( !bPathToFinishPoint && !bEmptyPathFound && --nStepCount >= 0 )
			{
				pPathFinder->SetPathParameters( GetBoundTileRadius(), GetAIPassabilityClass(), pAIMap->GetPointByTile( pPath->GetFinishTile() ), vFinishPoint, GetLastKnownGoodTile(), pAIMap );	
				CPtr<IStaticPath> pTempPath = pPathFinder->CreatePath( false );
				if ( IsValid( pTempPath ) && pTempPath->GetLength() > 2 )
				{
					pPath->MergePath( pTempPath, 0 );
					bPathToFinishPoint = mDistance( pPath->GetFinishTile(), vFinishTile ) < 2;
					bEmptyPathFound = false;
				}
				else 
					bEmptyPathFound = true;
			}
		}
	}
	if ( bNeedRelock )
		LockTiles();

	return pPath.Extract();
}

const bool CBasePathUnit::SendAlongPath( IStaticPath *pStaticPath, const CVec2 &vShift, const bool bSmoothTurn )
{
	bool bResult = true;
	// чтобы удалилось
	CPtr<IStaticPath> pTempPath = pStaticPath;
	NI_VERIFY( pStaticPath, "NULL Static Path passed", bResult = false );
	if ( CanMoveCritical() && bResult )
	{
		pPathMemento = 0;
		pInterruptedCollision = 0;
		stayTime = 0;
		pSmoothPath = pDefaultPath;
		UnlockTiles();

		const CVec2 vShiftPoint = pStaticPath->GetFinishPoint() + vShift;
		const CVec2 vPoint2To = pAIMap->IsPointInside( vShiftPoint ) ? vShiftPoint : pStaticPath->GetFinishPoint();

		//DebugTrace( "Unit %d goes to point %2.3f x %2.3f + %2.3f x %2.3f = %2.3f x %2.3f", GetUniqueID(), pStaticPath->GetFinishPoint().x, pStaticPath->GetFinishPoint().y, vShift.x, vShift.y, vShiftPoint.x, vShiftPoint.y );

		IPath *pPath = new CStandartPath2( this, pStaticPath, GetCenterPlain(), vPoint2To, pAIMap );
		bResult = pSmoothPath->Init( this, pPath, !IsInfantry() && bSmoothTurn, true, pAIMap );
		if ( bResult )
		{
			bNoCollision = true;
			{
				CPtr<ICollision> pCollision = CreateCollision( this, 0, -1, NCollision::ECN_FREE );
			}
			bNoCollision = false;
		}
	}
	else
		bResult = false;

	if ( bResult )
		CalculateIdle();
	/*
	else
		OnIdle();
	*/
	return bResult;
}

const bool CBasePathUnit::SendAlongPath( IPath *pPath )
{
	bool bResult = false;

	// чтобы удалилось
	CPtr<IPath> pUnitPath = pPath;
	if ( CanMoveCritical() )
	{
//		pPathMemento = 0;
//		pInterruptedCollision = 0;
		stayTime = 0;
		UnlockTiles();
		pSmoothPath = pDefaultPath;

		bResult = pSmoothPath->Init( this, pPath, !IsInfantry(), true, pAIMap );

		bNoCollision = true;
		{
			CPtr<ICollision> pCollision = CreateCollision( this, 0, -1, NCollision::ECN_FREE );
		}
		bNoCollision = false;
	}

	if ( bResult )
		CalculateIdle();
	else
		OnIdle();

	return bResult;
}

void CBasePathUnit::SendAlongSmoothPath( ISmoothPath *pPath )
{
	if ( CanMoveCritical() )
	{
		pPathMemento = 0;
		pInterruptedCollision = 0;
		stayTime = 0;
		UnlockTiles();

		pSmoothPath = pPath;

		bNoCollision = true;
		{
			CPtr<ICollision> pCollision = CreateCollision( this, 0, -1, NCollision::ECN_FREE );
		}
		bNoCollision = false;

		CalculateIdle();
	}
}

void CBasePathUnit::CalculateIdle()
{
	const bool bOldIdle = bIdle;
	bIdle = ( pInterruptedCollision == 0 ) && GetCollision()->IsSolved();
	if ( !bOldIdle && bIdle )
		OnIdle();
}

void CBasePathUnit::TurnToDirectionContinuesly( const WORD _wDirection )
{
	bTurningToDirContinuesly = true;
	wDirToContinueslyTurn = _wDirection;
}

void CBasePathUnit::SetCollision( ICollision *pCollision, IPath *pPath )
{
	if ( pCollision->GetName() == NCollision::ECN_GIVE_PLACE )
		pLastPusherUnit = pCollision->GetPushUnit();
	else
		pLastPusherUnit = 0;

	if ( pInterruptedCollision == 0 && !bNoCollision )
	{
		pInterruptedCollision = pCurrentCollision;
		pInterruptedPath = pSmoothPath;
		pPathMemento = GetSmoothPath()->CreateMemento();
	}
	
	if ( !bNoCollision )
	{
		pSmoothPath = pDefaultPath;
		if ( pPath == 0 )
			GetSmoothPath()->Stop();
		else
			GetSmoothPath()->Init( this, pPath, true, true, pAIMap );
	}

	pCurrentCollision = pCollision;

	CalculateIdle();
}

void CBasePathUnit::NotifyAboutClosestThreat( const CBasePathUnit *pUnit, const float fDistance )
{
	if ( pUnit->CanMoveCritical() )
	{
		const float maxSpeedHere = pUnit->GetMaxSpeedHere();
		const float maxPossibleSpeed = pUnit->GetMaxPossibleSpeed();
		if ( fDistance >= 3.0f * pAIMap->GetTileSize() )
		{
			if ( fSpeed >= 2 * maxSpeedHere / 3 && !bMaxSlowed && !bMinSlowed )
			{
				fSpeed = fSpeed - maxPossibleSpeed / 40;
				bMinSlowed = true;
			}
			else
			{
				if ( !bMinSlowed && !bMaxSlowed )
					fSpeed = fSpeed + maxPossibleSpeed / 40;
			}
		}
		else if ( fDistance >= pAIMap->GetTileSize() )
		{
			if ( fSpeed >= maxSpeedHere / 2 && !bMaxSlowed )
			{
				if ( bMinSlowed )
					fSpeed = fSpeed - maxPossibleSpeed / 40;
				else
					fSpeed = fSpeed - maxPossibleSpeed / 20;

				bMaxSlowed = true;
			}
			else
			{
				if ( !bMinSlowed && !bMaxSlowed )
					fSpeed = fSpeed + maxPossibleSpeed / 20;
			}
		}
		else
		{
			fSpeed = 0;
			bMaxSlowed = true;
		}
	}

	bNotified = true;
}

const EMovementPlane CBasePathUnit::GetProbablePlane( const SVector &vPos ) const
{
	EMovementPlane result = PLANE_TERRAIN;

	if ( pAIMap->GetTerrain()->IsBridge( vPos ) )
	{
		const EAIClasses aiClass = GetAIPassabilityClass();
		if ( aiClass & EAC_TERRAIN && aiClass & EAC_WATER )
			result = GetMovementPlane();
		else if ( aiClass & EAC_TERRAIN )
			result = PLANE_TERRAIN;
		else
			result = PLANE_WATER;
	}
	else
		result = ( pAIMap->GetTerrain()->GetTerrainType( vPos ) == ETT_WATER_TERRAIN ) ? PLANE_WATER : PLANE_TERRAIN;

	//Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_DEBUG_WINDOW, StrFmt( "%d x %d = %d ", vPos.x, vPos.y, result ), 0xFFFFFFFF, true );

	return result;
}

const bool CBasePathUnit::IsValidCenter( const CVec3 &_vCenter )
{
	const CVec3 vOldCenter( vCenter );
	const SVector vOldTile( vTile );

	vCenter = _vCenter;
	vTile = pAIMap->GetTile( CVec2( _vCenter.x, _vCenter.y ) );

	const SUnitProfile unitProfile = GetUnitProfile();

	vCenter = vOldCenter;
	vTile = vOldTile;

	if ( unitProfile.bRect )
	{
		return 
			pAIMap->IsPointInside( unitProfile.rect.v1 ) && pAIMap->IsPointInside( unitProfile.rect.v2 ) &&
			pAIMap->IsPointInside( unitProfile.rect.v3 ) && pAIMap->IsPointInside( unitProfile.rect.v4 );
	}
	else
	{
		return
			pAIMap->IsPointInside( unitProfile.circle.center+CVec2(  unitProfile.circle.r, 0 ) ) &&
			pAIMap->IsPointInside( unitProfile.circle.center+CVec2( -unitProfile.circle.r, 0 ) ) &&
			pAIMap->IsPointInside( unitProfile.circle.center+CVec2( 0,  unitProfile.circle.r ) ) &&
			pAIMap->IsPointInside( unitProfile.circle.center+CVec2( 0, -unitProfile.circle.r ) );
	}
}

const bool CBasePathUnit::IsValidDirection( const WORD _wDirection )
{
	// GetUnitProfile использует GetFrontDirectionVector для вычисления профайла ПРЯМОУГОЛЬНЫХ юнитов.
	// это значение не меняется, то есть для ПРЯМОУГОЛЬНЫХ юнитов проверяется валидность текущей позиции, а не запрашиваемой
	const WORD wTempDirection( wDirection );

	wDirection = _wDirection;

	const SUnitProfile unitProfile = GetUnitProfile();

	wDirection = wTempDirection;

	if ( unitProfile.bRect )
	{
		return 
			pAIMap->IsPointInside( unitProfile.rect.v1 ) && pAIMap->IsPointInside( unitProfile.rect.v2 ) &&
			pAIMap->IsPointInside( unitProfile.rect.v3 ) && pAIMap->IsPointInside( unitProfile.rect.v4 );
	}
	else
	{
		return
			pAIMap->IsPointInside( unitProfile.circle.center+CVec2(  unitProfile.circle.r, 0 ) ) &&
			pAIMap->IsPointInside( unitProfile.circle.center+CVec2( -unitProfile.circle.r, 0 ) ) &&
			pAIMap->IsPointInside( unitProfile.circle.center+CVec2( 0,  unitProfile.circle.r ) ) &&
			pAIMap->IsPointInside( unitProfile.circle.center+CVec2( 0, -unitProfile.circle.r ) );
	}
}

void CBasePathUnit::RestoreLock()
{
	if ( IsLockingTiles() )
	{
		//eMovementPlane = ( pAIMap->GetTerrain()->GetTerrainType( GetCenterTile() ) == ETT_WATER_TERRAIN ) ? PLANE_WATER : PLANE_TERRAIN;
		pAIMap->GetTerrain()->AddUnitTiles( GetUniqueID(), GetUnitProfile(), GetMovementPlane() == PLANE_WATER );
	}
}

void CBasePathUnit::RestoreSmoothPath()
{
	//DebugTrace( "Restore smooth path for unit %d", GetUniqueID() );
	pSmoothPath = pDefaultPath;
	pPathMemento = 0;
	pInterruptedCollision = 0;
	bNoCollision = true;
	{
		CPtr<ICollision> pCollision = CreateCollision( this, 0, -1, NCollision::ECN_FREE );
	}
	bNoCollision = false;
	Stop();
}

const CVec2 CBasePathUnit::GetMoveDirection() const
{
	CVec2 vResult = pSmoothPath->PeekPathPoint( 0 ) - GetCenterPlain();
	if ( !Normalize( &vResult ) )
		vResult = GetDirectionVector();
	return vResult;
}

void CBasePathUnit::OnSerialize( IBinSaver &f )
{
	SerializeBasePathUnit( f, 2, &pLastPushUnit );
	SerializeBasePathUnit( f, 42, &pLastPusherUnit );

	// CRAP{ for compatibility with legacy saves
	if ( pPathFinder == 0 )
		pPathFinder = Singleton<CCommonPathFinder>();
}

void SerializeBasePathUnit( IBinSaver &saver, const int nChunkID, CBasePathUnit **pUnit )
{
	if ( saver.IsReading() )
	{
		CPtr<CObjectBase> pObject = 0;
		saver.Add( nChunkID, &pObject );
		*pUnit = dynamic_cast_ptr< CBasePathUnit * >( pObject );
	}
	else
	{
		CPtr<CObjectBase> pObject = dynamic_cast< CObjectBase * >( *pUnit );
		saver.Add( nChunkID, &pObject );
	}
}


