#include "stdafx.h"

#include "InGroupPath.h"
#include "StandartPath2.h"
#include "PathFinder.h"
#include "Common_RTS_AI/StaticMapHeights.h"
#include "System/RandomGen.h"
#include "StaticPathInternal.h"

REGISTER_SAVELOAD_CLASS( COMMON_RTS_AI, 0x310CBCC2, CInGroupPathMemento );
REGISTER_SAVELOAD_CLASS( COMMON_RTS_AI, 0x310CBCC3, CInGroupPath )

const NTimer::STime PERIOD_OF_PATH_TO_FORMATION_SEARCH = 100;

const int MAX_DIST_TO_GO = 8; // пытаться закончить собственный путь если он завел от нормальной точки дальше чем на MAX_DIST_TO_GO тайлов

bool CInGroupPathBasis::Init( IMemento *_pMemento, CBasePathUnit *_pUnit, CAIMap *_pAIMap )
{
	pAIMap = _pAIMap;
	CInGroupPathMemento* pMemento = dynamic_cast<CInGroupPathMemento*>(_pMemento);
	NI_ASSERT( pMemento != 0, "Wrong memento passed" );
	if ( pMemento == 0 )
	{
		pUnit = 0;
		pSmoothGroupPath = 0;
		pFormation = 0;
		pUnitOwnPath = 0;
		bFinished = true;
		bGoByOwnPath = false;
		return false;
	}
	return Init( pUnit, pMemento->GetSmoothGroupPath(), false, true, pAIMap );
}

bool CInGroupPathBasis::Init( CBasePathUnit *_pUnit, CGroupSmoothPath *_pGroupSmoothPath, const bool bSmoothTurn, const bool bCheckTurn, CAIMap *_pAIMap )
{
	pAIMap = _pAIMap;
	pUnit = _pUnit;
	pUnitOwnPath = pUnit->GetDefaultPath();
	pSmoothGroupPath = _pGroupSmoothPath;
	pFormation = pSmoothGroupPath->GetUnit();
	timeToSearchPathToBack = 0;

	bFinished = bGoByOwnPath = false;

	return true;
}

void CInGroupPathBasis::CantFindPathToFormation()
{
	timeToSearchPathToBack = PERIOD_OF_PATH_TO_FORMATION_SEARCH + NRandom::Random( 0, PERIOD_OF_PATH_TO_FORMATION_SEARCH ); RecordRandomCall();
}

void CInGroupPathBasis::CutDriveToFormationPath( IStaticPath *pPath )
{
	if ( pPath->GetLength() >= 4 )
		pPath->MoveFinishTileTo( pPath->GetLength() / 2 );
}

bool CInGroupPathBasis::CanGoToFormationPos( const CVec2 &newCenter, const CVec2 &vDesPos, const CVec2 &vFormPos )
{
	// помещается в нужную точку
	if ( pAIMap->GetTerrain()->CanUnitGoToPoint( pUnit->GetBoundTileRadius(), vDesPos, pUnit->GetAIPassabilityClass(), pAIMap ) != FREE_NONE )
	{
		CPtr<IStaticPath> pPath = CreateStaticPathToPoint( newCenter, vDesPos, VNULL2, pUnit, false, pAIMap );
		// можно дойти
		if ( pPath != 0 )
		{

			CPtr<IStaticPath> pCheckPath = CreateStaticPathToPoint( pPath->GetFinishPoint(), vFormPos, VNULL2, pUnit, false, pAIMap );

			// от нужной точки можно дойти до центра формации и путь не слишком длинный
			if ( true /* pCheckPath != 0 && ( pCheckPath->GetLength() * pAIMap->GetTileSize() <= 2.0f * fabs( pSmoothGroupPath->GetUnitShift( pUnit ) ) ) */ )
			{
				//CutDriveToFormationPath( pPath );

				CPtr<CStandartPath2> pStandartPath = new CStandartPath2( pUnit, pPath, newCenter, pPath->GetFinishPoint(), pAIMap );

				if ( pUnitOwnPath->Init( pUnit, pStandartPath, false, true, pAIMap ) )
				{
					bGoByOwnPath = true;
					//DebugTrace( "Unit %d goes by own path (1)", pUnit->GetUniqueID() );
					/*
					if ( pUnit->GetUniqueID() == 70 )
					{
						DebugTrace( "unit: %2.3f x %2.3f; from: %2.3f x %2.3f; to: %2.3f x %2.3f", pUnit->GetCenterPlain().x, pUnit->GetCenterPlain().y, newCenter.x, newCenter.y, vDesPos.x, vDesPos.y );
						pPath->MarkStaticPath( pUnit->GetUniqueID(), NDebugInfo::RED );
					}
					*/
				}
				else
				{
					pPath = CreateStaticPathToPoint( newCenter, vDesPos, VNULL2, pUnit, false, pAIMap );
					if ( !pPath )
						return false;
					pStandartPath = new CStandartPath2( pUnit, pPath, newCenter, pPath->GetFinishPoint(), pAIMap );
					if ( pUnitOwnPath->Init( pUnit, pStandartPath, false, true, pAIMap ) )
					{
						bGoByOwnPath = true;
						//DebugTrace( "Unit %d goes by own path (2)", pUnit->GetUniqueID() );
					}
					else
						return false;
				}
				return true;
			}
		}
	}

	return false;
}

void CInGroupPathBasis::ValidateCurPath( const CVec2 &newCenter )
{
	if ( !pUnit->CanMoveBetweenTiles( pAIMap->GetTile( pUnit->GetCenterPlain() ), pAIMap->GetTile( newCenter ) ) ||
		pAIMap->GetTerrain()->CanUnitGoToPoint( pUnit->GetBoundTileRadius(), newCenter, pUnit->GetAIPassabilityClass(), pAIMap ) == FREE_NONE )
	{
		const bool bDrive = DriveToFormation( pUnit->GetCenterPlain(), false );
		// идти никуда не может, а формация остановилась
		if ( !bDrive && !pFormation->IsMoving() )
		{
			bFinished = true;
			pUnit->SetDirectionVec( pSmoothGroupPath->GetUnitDirection( pUnit ) );
			Stop();
		}

		return;
	}

	const SVector vNewCenterTile = pAIMap->GetTile( newCenter );
	CVec2 vCenterAhead = newCenter + pUnit->GetDirectionVector() * pAIMap->GetTileSize() / 2;
	if ( pAIMap->GetTile( vCenterAhead ) == vNewCenterTile )
		vCenterAhead += pUnit->GetDirectionVector() * pAIMap->GetTileSize() / 2;

	if ( !pUnit->CanMoveBetweenTiles( vNewCenterTile, pAIMap->GetTile( vCenterAhead ) ) ||
		pAIMap->GetTerrain()->CanUnitGoToPoint( pUnit->GetBoundTileRadius(), vCenterAhead, pUnit->GetAIPassabilityClass(), pAIMap ) == FREE_NONE )
	{
		const bool bDrive = DriveToFormation( newCenter, false );
		// идти никуда не может, а формация остановилась
		if ( !bDrive && !pFormation->IsMoving() )
		{
			bFinished = true;
			pUnit->SetDirectionVec( pSmoothGroupPath->GetUnitDirection( pUnit ) );
			Stop();
		}

		return;
	}
}

bool CInGroupPathBasis::DriveToFormation( const CVec2 &newCenter, const bool bAnyPoint )
{
	if ( bAnyPoint )
	{
		const CVec2 &vDesPos = pSmoothGroupPath->GetValidUnitCenter( pUnit );
		if ( CanGoToFormationPos( newCenter, vDesPos, pFormation->GetCenterPlain() ) )
		{
			//DebugTrace( "CanGoToFormationPos returns true (1)" );
			return true;
		}
		//DebugTrace( "CanGoToFormationPos returns false (2)" );
	}

	if ( pFormation->IsMoving() )
	{
		const CVec2 &vFarFormationPos = pSmoothGroupPath->GetFarCenter();
		const CVec2 &vUnitPos = pAIMap->GetTerrain()->GetValidPoint( pUnit->GetBoundTileRadius(), vFarFormationPos, vFarFormationPos + pSmoothGroupPath->GetUnitShift( pUnit ), pUnit->GetAIPassabilityClass(), false, pAIMap );
		if ( !CanGoToFormationPos( newCenter, vUnitPos, vFarFormationPos ) )
		{
			//DebugTrace( "CanGoToFormationPos returns false (3)" );
			CPtr<IStaticPath> pPath = CreateStaticPathToPoint( newCenter, vFarFormationPos, VNULL2, pUnit, false, pAIMap );
			if ( pPath != 0 )
			{
				CutDriveToFormationPath( pPath );
				CPtr<CStandartPath2> pStandartPath = new CStandartPath2( pUnit, pPath, newCenter, pPath->GetFinishPoint(), pAIMap );
	
				if ( pUnitOwnPath->Init( pUnit, pStandartPath, false, true, pAIMap ) )
				{
					bGoByOwnPath = true;
					//DebugTrace( "Unit %d goes by own path (3)", pUnit->GetUniqueID() );
				}
				else
					return false;

				return true;
			}
			else 
				return false;
		}
		//DebugTrace( "CanGoToFormationPos returns true (4)" );
		return true;
	}
	else
		return false;
}

const CVec2 CInGroupPathBasis::MoveUnit( const NTimer::STime timeDiff, const float fSpeed )
{
	CVec2 vCenter = pUnit->GetCenterPlain();

	// не смогли найти пути до формации (юнит где-то заблокался)
	if ( timeToSearchPathToBack > 0 )
	{
		if ( timeToSearchPathToBack < timeDiff )
			timeToSearchPathToBack -= timeDiff;
		else
			timeToSearchPathToBack = 0;
		return vCenter;
	}

	if ( !pFormation->IsMoving() )
	{
		Stop();
		if ( pFormation->IsPathFinished() )
			bFinished = true;
	}

	if ( bFinished )
	{
		return vCenter;
	}

	CVec2 result = pSmoothGroupPath->GetValidUnitCenter( pUnit );
	const float lineShift = pSmoothGroupPath->GetUnitPathShift( pUnit );

	// стоим впереди
	if ( lineShift > 0 )
		result += pFormation->GetDirectionVector() * lineShift;

	const float fDist = fabs( result - vCenter );
	const float fDiff = fDist - fSpeed * timeDiff;

	// точка по направлению к нужному положению
	if ( fDiff > 0 )
	{
		const CVec2 vDelta( result - vCenter );
		const float fDelta( fSpeed * timeDiff / fDist );
		const CVec2 vDeltaResult( vDelta * fDelta );
		result = vCenter + vDeltaResult;
	}

	if ( !pAIMap->IsPointInside( result ) )
	{
		DriveToFormation( result, false );
		return vCenter;
	}

	if ( fDist == 0.0f && pSmoothGroupPath->IsFinished() )
		bFinished = true;

	pSmoothGroupPath->NotifyPathShift( -lineShift );

	if ( pAIMap->GetTile( result ) != pAIMap->GetTile( vCenter ) )
	{
		// далеко от нужного положения
		const bool bCanStep = pUnit->CanMoveBetweenTiles( pAIMap->GetTile( vCenter ), pAIMap->GetTile( result ) );
		const bool bCanGoToPoint = 
			bCanStep && ( pAIMap->GetTerrain()->CanUnitGoToPoint( pUnit->GetBoundTileRadius(), result, pUnit->GetAIPassabilityClass(), pAIMap ) != FREE_NONE );
		if ( lineShift <= 0 && fDiff > 6 * pAIMap->GetTileSize() && bCanGoToPoint )
		{
			if ( DriveToFormation( result, true ) )
				return result;
			else
			{
				CantFindPathToFormation();
				return vCenter;
			}
		}
		else
		{
			ValidateCurPath( result );
			if ( !bCanGoToPoint )
			{
				CantFindPathToFormation();
				return vCenter;
			}
		}

		// всё ещё путь с формацией
		if ( !bGoByOwnPath )
		{
			const CVec2 vUnitPosition( pSmoothGroupPath->GetValidUnitCenter( pUnit ) );
			CPtr<IStaticPath> pCheckPath = 
				CreateStaticPathToPoint( vUnitPosition, VNULL2, pUnit, true, pAIMap );
			const float fR = fabs( vUnitPosition - vCenter );
			if ( pCheckPath && 
				( fR < pAIMap->GetTileSize() && pCheckPath->GetLength() > 3 ||
				fR >= pAIMap->GetTileSize() && pCheckPath->GetLength() * pAIMap->GetTileSize() > 3.0f * fR ) )
			{
				DriveToFormation( result, false );
			}
		}
	}

	// прошли слишком мало
	if ( bGoByOwnPath && fabs2( vCenter - result ) < sqr( pFormation->GetMaxSpeedHere() * timeDiff / 1.5f ) )
		return vCenter;
	else if ( result != vCenter && !bFinished )
	{
		if ( fabs2( result - vCenter ) > sqr( pFormation->GetMaxSpeedHere() * timeDiff / 1.5f ) )
			pUnit->SetDirectionVec( result - vCenter );
		else
      pUnit->SetDirectionVec( pSmoothGroupPath->GetFarCenter() + pSmoothGroupPath->GetUnitShift( pUnit ) - result );
	}

	return result;
}

void CInGroupPathBasis::Segment( const NTimer::STime timeDiff )
{
	if ( bGoByOwnPath )
	{
		if ( !pUnitOwnPath->IsFinished() )
			pUnitOwnPath->Segment( timeDiff );
		if ( pUnitOwnPath->IsFinished() )
		{
			bGoByOwnPath = false;
			//DebugTrace( "Unit %d goes by formation's path (2)", pUnit->GetUniqueID() );
		}
	}

	if ( !bGoByOwnPath )
	{
		if ( IsFinished() )
			pUnit->RestoreSmoothPath();
		else
		{
			const CVec3 vCenter = pUnit->GetCenter();
			const float fSpeed = pUnit->GetSpeed();
			const CVec2 vNewCenter( MoveUnit( timeDiff, fSpeed ) );
			pUnit->SetCenter( CVec3( vNewCenter, pAIMap->GetHeights()->GetZ( vNewCenter ) ) );

			if ( bFinished )
				pUnit->RestoreSmoothPath();
		}
	}
}

void CInGroupPathBasis::Stop()
{
//	pUnit->RestoreSmoothPath();
//	pUnit->Stop();
}

void CInGroupPathBasis::OnSerialize( IBinSaver &f )
{
	SerializeBasePathUnit( f, 2, &pUnit );
	SerializeBasePathUnit( f, 3, &pFormation );
}
