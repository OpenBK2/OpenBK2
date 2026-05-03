#include "stdafx.h"
#include "Misc/bresenham.h"
#include "StandartSmoothMechPath.h"
#include "Terrain.h"
#include "System/RandomGen.h"
#include "BasePathUnit.h"


const float MIN_LENGTH_FOR_LARGE_TURN = 1024.0f;  //! минимальное расстояние с которого начинает строиться большой разворот
const float MIN_LENGTH_FOR_SMALL_TURN = 256.0f;   //! минимальное расстояние с которого начинает строиться серий маленьких разворотов
const float THRESHOLD_FOR_LARGE_TURN = 0.95f;			//! минимальное значение косинуса угла между обратным направлением юнита и сплайном, при котором возможет большой разворот
const WORD  DIR_DIFF_TO_SMOOTH_TURNING = 2000;		//! при какой разнице в угле нужно гладко поворачиваться 

class CPushTileFunctional
{
	std::list<SVector> *pTiles;
public:
	CPushTileFunctional( std::list<SVector> *_pTiles ) : pTiles( _pTiles ) {}

	bool operator()( const SVector tile )
	{
		pTiles->push_back( tile );
		return true;
	}
};

class CCheckTileFunctional
{
	CBasePathUnit *pUnit;
	CTerrain *pTerrain;
public:
	CCheckTileFunctional( CBasePathUnit *_pUnit, CTerrain *_pTerrain ) : pUnit( _pUnit ), pTerrain( _pTerrain ) {}

	bool operator()( const SVector tile )
	{
		return ( pTerrain->CanUnitGo( pUnit->GetBoundTileRadius(), tile, pUnit->GetAIPassabilityClass() ) != FREE_NONE );
	}
};

class CCheckTileFunctionalWithMark
{
	CBasePathUnit *pUnit;
	std::vector<SVector> *pTiles;
	CTerrain *pTerrain;
public:
	CCheckTileFunctionalWithMark( CBasePathUnit *_pUnit, std::vector<SVector> *_pTiles, CTerrain *_pTerrain )
		: pUnit( _pUnit ), pTiles( _pTiles ), pTerrain( _pTerrain ) {}

	bool operator()( const SVector tile )
	{
		pTiles->push_back( tile );
		return ( pTerrain->CanUnitGo( pUnit->GetBoundTileRadius(), tile, pUnit->GetAIPassabilityClass() ) != FREE_NONE );
	}
};

WORD CStandartSmoothMechPath::CheckArc( const CVec2 &vUnit, const WORD wStartAngle, const WORD wDiffAngle, const float fRadius, const bool bClockWise, const bool bForward )
{
	//DEBUG{
	/*
	vector<SVector> tiles;
	CCheckTileFunctionalWithMark checkFunc2( GetUnit(), &tiles, GetAIMap()->GetTerrain() );
	WORD wResult = CheckArcTiles( checkFunc2, vUnit, wStartAngle, wDiffAngle, fRadius, bClockWise, bForward, GetAIMap() );
	DebugInfoManager()->CreateMarker( OBJECT_ID_FORGET, tiles, NDebugInfo::RED );
	return wResult;
	*/
	//DEBUG}
	CCheckTileFunctional checkFunc( GetUnit(), GetAIMap()->GetTerrain() );
	return CheckArcTiles( checkFunc, vUnit, wStartAngle, wDiffAngle, fRadius, bClockWise, bForward, GetAIMap() );
}

// построить большой разворот
bool CStandartSmoothMechPath::BuildLargeTurn( const WORD wStartDir, const WORD wEndDir, const CVec2 &vFinishPoint )
{
	if ( GetUnit()->GetMovementPlane() != PLANE_WATER || GetUnit()->GetTurnRadius() == 0.0f )
	{
		//DebugTrace( "BuildLargeTurn returns false (1)" );
		return false;
	}
	/*
	if ( GetUnit()->CanGoBackward() )
	{
		if ( fabs( GetUnit()->GetCenterPlain() - vFinishPoint ) < MIN_LENGTH_FOR_LARGE_TURN )
		{
			//DebugTrace( "BuildLargeTurn returns false (2)" );
			return false;
		}
		const CVec2 vPathDir = Norm( GetUnit()->GetCenterPlain() - vFinishPoint );
		if ( GetVectorByDirection( wStartDir ) * vPathDir < THRESHOLD_FOR_LARGE_TURN )
		{
			//DebugTrace( "BuildLargeTurn returns false (3)" );
			return false;
		}
	}
	*/
	circles.clear();
	const	WORD clockWise = wStartDir - wEndDir;
	const	WORD antiClockWise = wEndDir - wStartDir;
	bool bClockWise = ( clockWise < antiClockWise );
	WORD dirDiff = (bClockWise) ? clockWise : antiClockWise;

	if ( CheckArc( GetUnit()->GetCenterPlain(), wStartDir, dirDiff, GetUnit()->GetTurnRadius(), bClockWise, true ) < dirDiff )
	{
		bClockWise = !bClockWise;
		dirDiff = (bClockWise) ? clockWise : antiClockWise;
		if ( CheckArc( GetUnit()->GetCenterPlain(), wStartDir, dirDiff, GetUnit()->GetTurnRadius(), bClockWise, true ) < dirDiff )
		{
			//DebugTrace( "BuildLargeTurn returns false (4)" );
			return false;
		}
	}

	if ( dirDiff > DIR_DIFF_TO_SMOOTH_TURNING ) 
	{
		circles.push_back( CCirclePath( wStartDir, dirDiff, GetUnit()->GetCenterPlain(), GetUnit()->GetTurnRadius(), bClockWise, true ) );
		GetPath()->RecoverPath( circles.back().GetLastX(), GetUnit()->GetProbablePlane( GetAIMap()->GetTile( circles.back().GetLastX() ) ) == PLANE_WATER , GetAIMap()->GetTile( circles.back().GetLastX() ) );
		SetSplinePoints( GetPath()->GetStartPoint() );
		InitSpline();
		BuildSmoothTurn( circles.back().GetLastDX(), false );
	}

	//DebugTrace( "BuildLargeTurn returns true (circles.empty() = %s)", circles.empty() ? "true" : "false" );
	return true;
}

bool CStandartSmoothMechPath::BuildSmallTurns( const WORD wStartDir, const WORD wEndDir, const CVec2 &vFinishPoint, const bool bPrefereForward )
{
	return false;
	/*
	if ( !GetUnit()->CanGoBackward() || !GetPath()->CanBuildComplexTurn() || GetUnit()->GetTurnRadius() <= 0.0f || !NGlobal::GetVar( "use_round_turns", 0 ) )
		return false;
	const float fPathLen = fabs( GetUnit()->GetCenterPlain() - vFinishPoint );
	if ( fPathLen < MIN_LENGTH_FOR_SMALL_TURN )
		return false;
	const CVec2 vPathStartPoint = GetPath()->GetStartPoint();
	const	WORD clockWise = wStartDir - wEndDir;
	const	WORD antiClockWise = wEndDir - wStartDir;
	bool bLocalClockWise = ( clockWise < antiClockWise );
	bool bLocalForward = bPrefereForward;
	for ( int i = 0; i < 4; i++ )
	{
		circles.clear();
		WORD currentDir = wStartDir;
		bool bClockWise = bLocalClockWise;
		bool bForward = bLocalForward;
		CVec2 vPosition = GetUnit()->GetCenterPlain();
		WORD wRemain = ( bClockWise ) ? ( currentDir - wEndDir ) : ( wEndDir - currentDir );
		const WORD wBestTurn = wRemain/2;
		bool bFirstStep = true;
		while ( wRemain > DIR_DIFF_TO_SMOOTH_TURNING/2 )
		{
			const WORD MAX_TURN = 16384;
			const WORD wPrefer = ( bFirstStep ) ? wBestTurn : Min( wRemain, MAX_TURN );
			bFirstStep = false;
			WORD wClear = CheckArc( vPosition, currentDir, wPrefer, GetUnit()->GetTurnRadius(), bClockWise, bForward );
			if ( wClear < wPrefer && wClear < 1024 )
				break;
			circles.push_back( CCirclePath( currentDir, wClear, vPosition, GetUnit()->GetTurnRadius(), bClockWise, bForward ) );
			wRemain -= wClear;
			if ( bClockWise )
				currentDir -= wClear;
			else
				currentDir += wClear;

			vPosition = circles.back().GetLastX();

			GetPath()->RecoverPath( vPosition, GetUnit()->GetProbablePlane( GetAIMap()->GetTile( vPosition ) ) == PLANE_WATER, GetAIMap()->GetTile( vPosition ) );
			SetSplinePoints( GetPath()->GetStartPoint() );
			const CVec2 vUnitLastDir = ( bForward ) ? circles.back().GetLastDX() : -circles.back().GetLastDX();

			const bool bSmoothPoint = BuildSmoothTurn( vUnitLastDir, true );

			if ( bSmoothPoint )
			{
				if ( !bSmoothPoint )
					BuildSmoothTurn( vUnitLastDir, false );
				IterateSpline();
				return true;
			}
			bForward = !bForward;
		}
		if ( i == 1 )
			bLocalClockWise = !bLocalClockWise;
		else
			bLocalForward = !bLocalForward;
	}
	circles.clear();

	GetPath()->RecoverPath( vPathStartPoint, GetUnit()->GetProbablePlane( GetAIMap()->GetTile( vPathStartPoint ) ) == PLANE_WATER, GetAIMap()->GetTile( vPathStartPoint ) );
	SetSplinePoints( GetPath()->GetStartPoint() );
	InitSpline();
	IterateSpline();

	return false;
	*/
}

bool CStandartSmoothMechPath::BuildSmoothTurn( const CVec2 &vUnitMoveDir, const bool bCheckThreshold )
{
	CVec2 vDir = GetSplineDX();
	Normalize( &vDir );

	const int nTileRadius = GetUnit()->GetBoundTileRadius();
	const EAIClasses cAIClass = GetUnit()->GetAIPassabilityClass();

	if ( !bCheckThreshold || vUnitMoveDir * vDir > GetUnit()->GetSmoothTurnThreshold() )
	{
		const float dist = fabs( GetSplineLastPoint() - GetSplineFirstPoint() );
		const float fProj = vDir * vUnitMoveDir;
		const float coeff = ( dist / 2.0f ) / fProj;

		const CVec2 point = GetSplineFirstPoint() + vUnitMoveDir * coeff / 1.5f;

		const SVector tile = GetAIMap()->GetTile( point );
		CBres bres;
		bres.InitPoint( GetAIMap()->GetTile( GetSplineFirstPoint() ), tile );

		while ( bres.GetDirection() != tile )
		{
			if ( GetAIMap()->GetTerrain()->CanUnitGo( nTileRadius, bres.GetDirection(), cAIClass ) == FREE_NONE )
				break;

			bres.MakePointStep();
		}

		if ( bres.GetDirection() == tile && ( GetAIMap()->GetTerrain()->CanUnitGo( nTileRadius, bres.GetDirection(), cAIClass ) != FREE_NONE ) )
		{
			std::list<SVector> insertedTiles;

			bres.Init( tile, GetAIMap()->GetTile( GetSplineLastPoint() ) );
			while ( bres.GetDirection() != tile )
			{
				if ( GetAIMap()->GetTerrain()->CanUnitGo( nTileRadius, bres.GetDirection(), cAIClass ) == FREE_NONE )
					break;

				insertedTiles.push_back( bres.GetDirection() );
				bres.MakePointStep();
			}

			if ( bres.GetDirection() == tile && ( GetAIMap()->GetTerrain()->CanUnitGo( nTileRadius, bres.GetDirection(), cAIClass ) != FREE_NONE ) )
			{
				GetPath()->InsertTiles( insertedTiles );

				SetSplineLastPoint( point );
				RecalcSpline();
				return true;
			}
		}
		return false;
	}
	else
		return false;
}

void CStandartSmoothMechPath::AddSmoothTurn()
{
	// Для небольшого поворота, просто сглаживание
	if ( !BuildSmoothTurn( GetUnit()->GetDirectionVector(), true ) )
	{
		// Поворот достаточно большой, необходимо хитро разворачиваться, сначала по большому полукругу
		if ( !BuildLargeTurn( GetUnit()->GetFrontDirection(), GetDirectionByVector( GetSplineDX() ), GetPath()->GetFinishPoint() ) )
		{
			// Ну и напоследок через серею маленьких разворотов
			if ( !BuildSmallTurns( GetUnit()->GetFrontDirection(), GetDirectionByVector( GetSplineDX() ), GetPath()->GetFinishPoint(), true ) )
			{
			}
		}
	}
}

bool CStandartSmoothMechPath::CheckTurn( const WORD wNewDir )
{
	bool bCanBackward = GetPath()->CanGoBackward( GetUnit() );
	const WORD wUnitDir = GetUnit()->GetDirection();
	const WORD wRightDirsDiff = DirsDifference( wUnitDir, wNewDir );
	const WORD wBackDirsDiff = DirsDifference( wUnitDir + 32768, wNewDir );
	// поворот небольшой
	if ( wRightDirsDiff < DIR_DIFF_TO_SMOOTH_TURNING || 
		bCanBackward && wBackDirsDiff < DIR_DIFF_TO_SMOOTH_TURNING )
		return true;
	else
	{
		const CVec2 vNewDir = GetVectorByDirection( wNewDir );
		int nResult = -1;

		CTemporaryUnitProfileUnlocker unlocker( GetUnit()->GetUniqueID(), GetUnit()->GetUnitProfile(), GetUnit()->IsLockingTiles(), false, GetAIMap()->GetTerrain() );

		bool bCanRotateForward = GetUnit()->CheckTurn( 1.0f, vNewDir, false, false );
		bool bCanRotateBackward = GetUnit()->CheckTurn( 1.0f, -vNewDir, false, false );

		if ( bCanRotateForward && bCanRotateBackward )
		{
			if ( GetUnit()->IsDangerousDirExist() )
			{
				const WORD wDangerousDir = GetUnit()->GetDangerousDir();

				bCanRotateForward =
					DirsDifference( wNewDir, wDangerousDir ) < DirsDifference( wNewDir + 32768, wDangerousDir );
				bCanRotateBackward = !bCanRotateForward;
			}
		}

		if ( nResult == -1 && bCanRotateForward && bCanRotateBackward )
			nResult = 1;
		if ( nResult == -1 && !bCanBackward && bCanRotateForward )
			nResult = 1;
		if ( nResult == -1 && bCanBackward && wBackDirsDiff < wRightDirsDiff && bCanRotateBackward )
			nResult = 1;

		if ( nResult == -1 && bCanRotateForward )
		{
			bCanGoBackward = false;
			nResult = 1;
		}

		if ( nResult == -1 && bCanRotateBackward /* && GetUnit()->CanGoBackward() */ )
		{
			bCanGoForward = false;
			nResult = 1;
		}

		if ( nResult == -1 && GetUnit()->CanTurnTo( wNewDir, false ) )
			nResult = 1;

		return ( nResult == 1 );
	}
}

bool CStandartSmoothMechPath::RideAway()
{
	return false;
/*
	CVec2 vDir = GetSplineDX();
	Normalize( &vDir );

	const int nTileRadius = GetUnit()->GetBoundTileRadius();
	const EAIClasses cAIClass = GetUnit()->GetAIPassabilityClass();

	if ( !bCheckThreshold || vUnitMoveDir * vDir > GetUnit()->GetSmoothTurnThreshold() )
	{
		const float dist = fabs( GetSplineLastPoint() - GetSplineFirstPoint() );
		const float fProj = vDir * vUnitMoveDir;
		const float coeff = ( dist / 2.0f ) / fProj;

		const CVec2 point = GetSplineFirstPoint() + vUnitMoveDir * coeff / 1.5f;

		const SVector tile = GetAIMap()->GetTile( point );
		CBres bres;
		bres.InitPoint( GetAIMap()->GetTile( GetSplineFirstPoint() ), tile );

		while ( bres.GetDirection() != tile )
		{
			if ( GetAIMap()->GetTerrain()->CanUnitGo( nTileRadius, bres.GetDirection(), cAIClass ) == FREE_NONE )
				break;

			bres.MakePointStep();
		}

		if ( bres.GetDirection() == tile && ( GetAIMap()->GetTerrain()->CanUnitGo( nTileRadius, bres.GetDirection(), cAIClass ) != FREE_NONE ) )
		{
			list<SVector> insertedTiles;

			bres.Init( tile, GetAIMap()->GetTile( GetSplineLastPoint() ) );
			while ( bres.GetDirection() != tile )
			{
				if ( GetAIMap()->GetTerrain()->CanUnitGo( nTileRadius, bres.GetDirection(), cAIClass ) == FREE_NONE )
					break;

				insertedTiles.push_back( bres.GetDirection() );
				bres.MakePointStep();
			}

			if ( bres.GetDirection() == tile && ( GetAIMap()->GetTerrain()->CanUnitGo( nTileRadius, bres.GetDirection(), cAIClass ) != FREE_NONE ) )
			{
				GetPath()->InsertTiles( insertedTiles );

				SetSplineLastPoint( point );
				RecalcSpline();
				return true;
			}
		}
		return false;
	}
	else
		return false;
*/
}

bool CStandartSmoothMechPath::Init( CBasePathUnit *_pUnit, IPath *_pPath, bool _bSmoothTurn, bool _bCheckTurn, CAIMap *pAIMap )
{
	lastCheckToRightTurn = 0;
	bSmoothTurn = _bSmoothTurn;
	if ( GetUnit() != 0 && !circles.empty() )
	{
		if ( !circles.front().IsForwardDir() )
		{
			GetUnit()->SetGoForward( true );
			GetUnit()->SetDirectionVec( -circles.front().GetDX() );
		}
		circles.clear();
	}

	const bool bResult = CStandartSmoothPathBasis::Init( _pUnit, _pPath, _bSmoothTurn, _bCheckTurn, pAIMap );
	if ( bResult )
	{
		bCanGoForward = true;
		bCanGoBackward = ( GetUnit() != 0 && GetUnit()->CanGoBackward() );

		if ( _bCheckTurn && GetPath()->ShouldCheckTurn() )
			CheckTurn( GetDirectionByVector( GetSplineDX() ) );

		if ( GetPath() && _bSmoothTurn )
			AddSmoothTurn();
	}

	return bResult;
}

const bool CStandartSmoothMechPath::ValidateCurrentPath( const CVec2 &vCenter, const CVec2 &vNewPoint )
{
	if ( !IsFinished() && circles.empty() && ( fabs2( vCenter - vLastValidatedPoint ) >= sqr( GetAIMap()->GetTileSize() / 2.0f ) /* || GetUnit()->IsTurning()*/ ) )
	{
		vLastValidatedPoint = vCenter;

		SUnitProfile unitProfile( GetUnit()->GetUnitProfile() );
		SRect unitRect;
		if ( unitProfile.bRect )
			unitRect.InitRect( vCenter, GetUnit()->GetDirectionVector(), unitProfile.rect.lengthAhead + GetAIMap()->GetTileSize() * 0.85f,
			0, unitProfile.rect.width * 0.9f );
		else
			unitRect.InitRect( vCenter, GetUnit()->GetDirectionVector(), unitProfile.circle.r + GetAIMap()->GetTileSize() * 0.85f,
			0, unitProfile.circle.r * 0.6f );

		const EAIClasses aiClass = GetUnit()->GetAIPassabilityClass();
		if ( GetAIMap()->IsRectOnLockedTiles( unitRect, aiClass ) )

		{
			const SVector unitTileCenter = GetAIMap()->GetTile( vCenter );
			const int nBoundTileRadius = GetUnit()->GetBoundTileRadius();

			std::list<SObjTileInfo> tiles;
			GetAIMap()->GetTilesCoveredByRect( unitRect, &tiles );
			std::list<SObjTileInfo>::iterator iter = tiles.begin();
			while ( iter != tiles.end() )
			{
				const SVector &tile = iter->tile;
				if ( abs( tile.x - unitTileCenter.x ) <= nBoundTileRadius || 
					   abs( tile.y - unitTileCenter.y ) <= nBoundTileRadius )
					iter = tiles.erase( iter );
				else
				{
					const CVec2 vTileCenter = GetAIMap()->GetPointByTile( tile );
					CVec2 vDiff = vTileCenter - vCenter;
					Normalize( &vDiff );
					if ( fabs( vDiff * GetUnit()->GetDirectionVector() ) > 1/FP_SQRT_2 )
						iter = tiles.erase( iter );
					else
					{
						iter->lockInfo = EAC_ANY;
						++iter;
					}
				}
			}
			
			const float fDist2 = fabs2( vCenter - GetPath()->GetFinishPoint() );
			if ( fDist2 <= sqr( 3 * GetUnit()->GetUnitProfile().GetHalfLength() ) )
				FinishPath();
			else
			{
				const int nLockID = GetAIMap()->GetTerrain()->TemporaryLockTiles( tiles );
				GetPath()->RecalcPath( vNewPoint, GetUnit()->GetProbablePlane( GetAIMap()->GetTile( vNewPoint ) ) == PLANE_WATER, GetUnit()->GetLastKnownGoodTile() );
				if ( GetPath()->IsFinished() || !Init( GetUnit(), GetPath(), false, false, GetAIMap() ) )
				{
					if ( GetUnit()->GetCollision()->GetName() == NCollision::ECN_FREE && ( vLastStopPosition.x == -1 || mDistance( vLastStopPosition, GetUnit()->GetCenterTile() ) > 4 ) )
						CPtr<ICollision> pCollison = CreateCollision( GetUnit(), 0, -1, NCollision::ECN_STOP );
					else
						FinishPath();
				}
				GetAIMap()->GetTerrain()->RemoveTemporaryLock( nLockID );
			}

			//DEBUG{
			/*
			vector<SVector> tiles2;
			for ( list<SObjTileInfo>::const_iterator it = tiles.begin(); it != tiles.end(); ++it )
				tiles2.push_back( it->tile );
			DebugInfoManager()->CreateMarker( GetUnit()->GetUniqueID(), tiles2, NDebugInfo::RED );
			*/
			//DEBUG}
		}
	}
	return true;
}

bool CStandartSmoothMechPath::UpdateDirection()
{
	if ( GetSplineDX() != VNULL2 )
	{
		// слишком велика разница между старым и новым направлениями, нужно повернуться
		const WORD wDirsDiff = DirsDifference( GetUnit()->GetDirection(), GetDirectionByVector( GetSplineDX() ) );
		if ( wDirsDiff != 0 && ( GetUnit()->IsTurning() || wDirsDiff	> DIR_DIFF_TO_SMOOTH_TURNING ) )
		{
			if ( !GetUnit()->TurnToDirection( GetDirectionByVector( GetSplineDX() ) , true, true ) )
				return true;
		}
		else
			GetUnit()->SetDirection( GetDirectionByVector( GetSplineDX() ) );
	}
	return false;
}

const CVec2 CStandartSmoothMechPath::MoveUnit( const NTimer::STime timeDiff, const float fSpeed )
{
	// Clamp consumed movement speed to the unit's current cap (terrain + desired speed + reverse modifier).
	const float fMoveSpeed = (std::min)( fSpeed, GetUnit()->GetMaxSpeedHere() );

	if ( IsFinished() || ( circles.empty() && IsSplinePointsEqual() ) )
	{
		if ( !IsFinished() )
			FinishPath();
		return GetUnit()->GetCenterPlain();
	}

	// пропустить сегмент, если хотим останавливаться
	if ( bSkipNextSegment )
	{
		bSkipNextSegment = false;
		return GetUnit()->GetCenterPlain();
	}

	// если есть окружности, по которым ехать, используем их !!!
	if ( !circles.empty() ) 
	{
		const float fLength = circles.front().Iterate( fMoveSpeed * timeDiff );
		const CVec2 vResult = circles.front().GetX();
		if ( fLength > 0 )
		{
			GetUnit()->SetGoForward( true );
			const bool bLastForward = circles.front().IsForwardDir();
			const CVec2 vLastDX = circles.front().GetLastDX();
			circles.pop_front();
			GetUnit()->SetDirection( GetDirectionByVector( ( bLastForward ) ? vLastDX : -vLastDX ) );
			if ( !circles.empty() || !bLastForward )
			{
				bSkipNextSegment = true;
				return vResult;
			}
		}
		else
		{
			GetUnit()->SetGoForward( circles.front().IsForwardDir() );
			GetUnit()->SetDirection( GetDirectionByVector( circles.front().GetDX() ) );
			return vResult;
		}
	}

	const CVec2 vCenter = GetUnit()->GetCenterPlain();
	// если едем задом, проверить - нельзя ли развернуться, чтобы поехать передом
	if ( !GetUnit()->IsGoForward() )
	{
		if ( lastCheckToRightTurn >= (NTimer::STime)( NRandom::Random( 200, 500 ) ) )
		{
			lastCheckToRightTurn = 0;

			const WORD wFrontDir = GetUnit()->GetFrontDirection();
			bool bCheck = true;
			if ( GetUnit()->IsDangerousDirExist() )
			{
				const WORD wDangerousDir = GetUnit()->GetDangerousDir();
				bCheck = DirsDifference( wFrontDir, wDangerousDir ) > 
					DirsDifference( wFrontDir + 32768, wDangerousDir );
			}

			if ( bCheck )
			{
				const CVec2 vFrontDir = GetVectorByDirection( wFrontDir );
				if ( GetUnit()->CheckTurn( 1.0f, -vFrontDir, false, false ) )
				{
					bCanGoForward = true;
//					GetUnit()->ForceGoForward();
					if ( !GetPath()->CanGoBackward( GetUnit() ) && GetUnit()->CanTurnRound() )
						GetUnit()->TurnToDirection( wFrontDir, false, true );
				}
			}
		}
		else
			lastCheckToRightTurn += timeDiff;
	}

	const float fRemain = fMoveSpeed * timeDiff;
	while ( !IsFinished() && fabs( GetSplinePoint() - vCenter ) < fRemain )
	{
		while ( fabs( GetSplinePoint() - vCenter ) < fRemain && GetNIter() < CBSpline::N_OF_ITERATONS )
			IterateSpline();

		if ( GetPath()->IsFinished() && mDistance( GetSplinePoint(), GetPath()->GetFinishPoint() ) < 2 )
		{
			UpdateDirection();
			FinishPath();
			return GetSplinePoint();
		}
		else		
		{
			if ( fabs( GetSplinePoint() - vCenter ) < fRemain && GetNIter() >= CBSpline::N_OF_ITERATONS )
				InitSpline();
		}
	}

	if ( IsFinished() )
	{
		if ( GetPath()->IsFinished() && mDistance( GetSplinePoint(), GetPath()->GetFinishPoint() ) < 2 )
		{
			UpdateDirection();
			FinishPath();
			return GetSplinePoint();
		}
	}

	if ( UpdateDirection() )
		return vCenter;

	CVec2 vResult( vCenter );
	if ( fabs( GetSplinePoint() - vCenter ) > fRemain )
		// прошли чуть дальше, нужно точно отсчитать fRemain
		vResult = vCenter + Norm( GetSplinePoint() - vCenter ) * fRemain;
	else
		// так и не прошли по сплайну, сколько надо
		vResult = GetSplinePoint();

	// для юнитов с ненулевой скоростью поворота
	// до конца пути осталось совсем немного
	float fDist = fabs2( vResult - GetPath()->GetFinishPoint() );
	{
		// нет гомосекам!!!
		SUnitProfile unitProfile( GetUnit()->GetUnitProfile() );
		if ( fDist <= sqr( 2.5f * unitProfile.GetHalfLength() ) )
		{
			SRect rect;
			rect.InitRect( vResult, GetUnit()->GetDirectionVector(), unitProfile.GetLengthAhead() + GetAIMap()->GetTileSize() * 0.5f, 0.0f, unitProfile.GetHalfWidth() * 0.45f );

			if ( GetUnit()->IsOnLockedTiles( rect ) )
			{
				FinishPath();
				return vCenter;
			}
		}
	}
	return vResult;
}

const bool CStandartSmoothMechPath::CanGoBackward() const
{
	if ( GetPath() != 0 )
	{
		if ( bCanGoBackward && !bCanGoForward )
			return true;
		else
			return GetPath()->CanGoBackward( GetUnit() );
	}
	else
		return false;
}

void CStandartSmoothMechPath::Stop()
{
	GetUnit()->StopTurning();
	CStandartSmoothPathBasis::FinishPath();
}

REGISTER_SAVELOAD_CLASS( STANDART_SMOOTH_MECH_PATH, CStandartSmoothMechPath );

