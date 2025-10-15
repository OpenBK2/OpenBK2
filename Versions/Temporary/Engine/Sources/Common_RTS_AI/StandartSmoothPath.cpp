#include "stdafx.h"

#include "StandartSmoothPath.h"
#include "BasePathUnit.h"

#include "Common_RTS_AI/StaticMapHeights.h"


REGISTER_SAVELOAD_CLASS( COMMON_RTS_AI, STANDART_SMOOTH_SOLDIER_PATH, CStandartSmoothPath );
REGISTER_SAVELOAD_CLASS( COMMON_RTS_AI, 0x311133C1, CStandartSmoothPathMemento );

bool CStandartSmoothPathBasis::Init( IMemento *_pMemento, CBasePathUnit *_pUnit, CAIMap *_pAIMap )
{
	pAIMap = _pAIMap;
	CStandartSmoothPathMemento* pMemento = dynamic_cast<CStandartSmoothPathMemento*>(_pMemento);
	NI_ASSERT( pMemento != 0, "Wrong memento passed" );
	if ( pMemento == 0 )
	{
		pPath = 0;
		bFinished = true;
		return false;
	}
	pPath = pMemento->GetPath();

	if ( pPath == 0 )
	{
		pPath = 0;
		bFinished = true;
		return false;
	}
	else
	{
		pPath->RecoverPath( pUnit->GetCenterPlain(), pUnit->GetProbablePlane( pAIMap->GetTile( pUnit->GetCenterPlain() ) ) == PLANE_WATER, pUnit->GetLastKnownGoodTile() );
		return Init( pUnit, pPath, false, true, pAIMap );
	}
}

bool CStandartSmoothPathBasis::Init( CBasePathUnit *_pUnit, IPath *_pPath, bool _bSmoothTurn, bool bCheckTurn, CAIMap *_pAIMap )
{
	pAIMap = _pAIMap;
	NI_ASSERT( _pPath != 0, "Smooth path is trying to be initialized by NULL static path" );
	if ( _pPath == 0 )
	{
		bFinished = true;
		return false;
	}
	pUnit = _pUnit;
	pPath = _pPath;

	if ( pUnit->GetCenterTile() == pAIMap->GetTile( pPath->GetFinishPoint() ) )
	{
		pPath = 0;
		bFinished = true;
		return false;
	}
	else
	{
		predPoint = p0 = p1 = p2 = p3 = pPath->GetStartPoint();
		InitSpline();
		bFinished = false;
		if ( pPath->IsFinished() && spline.GetPoint() == pPath->GetFinishPoint() )
		{
			pPath = 0;
			bFinished = true;
			return false;
		}
		nPoints = 0;

		IterateSpline();
		bStopped = false;
	}

	return true;
}

const CVec2 CStandartSmoothPathBasis::PeekPathPoint( const int nShift ) const
{
	if ( pPath == 0 )
		return VNULL2;
	else
		return pPath->PeekPoint( nShift );
}

const CVec2 CStandartSmoothPathBasis::MoveUnit( const NTimer::STime timeDiff, const float _fSpeed )
{
	if ( IsFinished() || IsSplinePointsEqual() || bStopped )
	{
		if ( !bStopped && !bFinished )
			FinishPath();
		return GetUnit()->GetCenterPlain();
	}

	if ( GetSplineDX() != VNULL2 )
		GetUnit()->SetDirectionVec( GetSplineDX() );

	// Clamp consumed movement speed to the unit's current cap (terrain + desired speed + reverse modifier).
	const float fSpeed = (std::min)( _fSpeed, GetUnit()->GetMaxSpeedHere() );
	const float fRemain = fSpeed * timeDiff;
	const CVec2 vCenter = GetUnit()->GetCenterPlain();
	while ( !IsFinished() && fabs( GetSplinePoint() - vCenter ) < fRemain ) 
	{
		while ( fabs( GetSplinePoint() - vCenter ) < fRemain && GetNIter() < CBSpline::N_OF_ITERATONS )
			IterateSpline();

		if ( pPath->IsFinished() && mDistance( GetSplinePoint(), pPath->GetFinishPoint() ) < 2 )
		{
			FinishPath();
			return GetSplinePoint();
		}
		else		
		{
			if ( fabs( spline.GetPoint() - vCenter ) < fRemain && GetNIter() >= CBSpline::N_OF_ITERATONS )
				InitSpline();
		}
	}

	if ( bFinished )
	{
		if ( pPath->IsFinished() && mDistance( GetSplinePoint(), pPath->GetFinishPoint() ) < 2 )
		{
			FinishPath();
			return GetSplinePoint(); 
		}
	}

	if ( GetSplineDX() != VNULL2 )
		GetUnit()->SetDirectionVec( GetSplineDX() );

	const CVec2 vResult = fabs( GetSplinePoint() - vCenter ) < fRemain ? GetSplinePoint() : vCenter + Norm( GetSplinePoint() - vCenter ) * fRemain;

	return vResult;
}

void CStandartSmoothPathBasis::Segment( const NTimer::STime timeDiff )
{
	if ( !IsFinished() )
	{
		const CVec2 vNewCenter = MoveUnit( timeDiff, GetUnit()->GetSpeed() );
		if ( !IsFinished() )  
		{
			const SVector vOldTile = pAIMap->GetTile( GetUnit()->GetCenterPlain() );
			const SVector vNewTile = pAIMap->GetTile( vNewCenter );
			if ( !GetUnit()->CanMoveBetweenTiles( vOldTile, vNewTile ) )
			{
				// Amphibian shore-height gates can reject a path step even when passability is clear.
				FinishPath();
				return;
			}
			ValidateCurrentPath( GetUnit()->GetCenterPlain(), vNewCenter );
		}
		const float fZ = ( GetUnit()->GetMovementPlane() == PLANE_TERRAIN ) ? pAIMap->GetHeights()->GetZ( vNewCenter ) : 0.0f;
		GetUnit()->SetCenter( CVec3( vNewCenter, fZ ) );
	}
}

const bool CStandartSmoothPathBasis::ValidateCurrentPath( const CVec2 &vCenter, const CVec2 &vNewPoint )
{
	if ( pAIMap->GetTile( vCenter ) != pAIMap->GetTile( vNewPoint ) )
	{
		CBSpline::SForwardIter iter;
		StartForwardIterating( &iter );

		CVec2 iterPoint( iter.x );
		IterateForwardSpline( &iter ); 

		while ( iter.t != -1 && fabs2( iterPoint - iter.x ) <= sqr( pAIMap->GetTileSize() ) / 4 )
			IterateForwardSpline( &iter );

		int i = 1;
		bool bBad = false;
		while ( iter.t != -1 )
		{
			if ( fabs2( iterPoint - iter.x ) >= sqr( pAIMap->GetTileSize() ) / 2 )
			{
				const bool bBadStep = !GetUnit()->CanMoveBetweenTiles( pAIMap->GetTile( iterPoint ), pAIMap->GetTile( iter.x ) );
				if ( bBadStep || pAIMap->GetTerrain()->CanUnitGoToPoint( GetUnit()->GetBoundTileRadius(), iter.x, GetUnit()->GetAIPassabilityClass(), pAIMap ) == FREE_NONE )
				{
					if ( !bBad )
						bBad = true;
					else
					{
						// до конца пути осталось немного, лучше остановиться
						const float fDist2 = fabs2( vCenter - pPath->GetFinishPoint() );
						if ( fDist2 <= sqr( 3 * GetUnit()->GetUnitProfile().GetHalfLength() ) )
							FinishPath();
						else
						{
							pPath->RecalcPath( vNewPoint, GetUnit()->GetProbablePlane( pAIMap->GetTile( vNewPoint ) ) == PLANE_WATER, GetUnit()->GetLastKnownGoodTile() );
							if ( pPath->IsFinished() )
								FinishPath();
							else if ( !Init( GetUnit(), pPath, false, false, pAIMap ) )
								FinishPath();
						}

						return false;
					}
				}
				else
					bBad = false;

				iterPoint = iter.x;
			}

			++i;
			IterateForwardSpline( &iter ); 
		}
	}

	return true;
}

const bool CStandartSmoothPathBasis::IsPathBroken( const int nStartPoint ) const
{
	const SVector p0 = nStartPoint < 0 ? pAIMap->GetTile( predPoint ) : pAIMap->GetTile( pPath->PeekPoint( nStartPoint ) );
	const SVector p1 = pAIMap->GetTile( pPath->PeekPoint( nStartPoint + 1 ) );
	const SVector p2 = pAIMap->GetTile( pPath->PeekPoint( nStartPoint + 2 ) );

	const SVector v1 = p2 - p1;
	const SVector v2 = p1 - p0;

	if ( v1 == v2 )
		return false;
	else
		return pAIMap->GetTerrain()->CanUnitGo( pUnit->GetBoundTileRadius(), p0 + v1, pUnit->GetAIPassabilityClass() ) == FREE_NONE;
}

int CStandartSmoothPathBasis::InitSpline()
{
	p0 = p1; p1 = p2; p2 = p3;

	int inc = 0;
/*{LOG
в условии был ||, что приводило с плохому проезжанию зубцов, плюс изабивлся от ужасного вызова IsPathBroken
LOG}*/
	if ( IsPathBroken( -1 ) && IsPathBroken( 0 ) )
	{
		predPoint = p3;
		p3 = pPath->PeekPoint( 1 );
		pPath->Shift( 1 );
		++nPoints;
	}
	else
	{
		while ( !pPath->IsFinished() && inc < SPLINE_STEP && !IsPathBroken( 0 ) )
		{
			++inc;
			++nPoints;
			predPoint = pPath->PeekPoint( 1 );
			pPath->Shift( 1 );
		}

		//DebugTrace( ">>>> stop getting points, pPath->IsFinished() = %s, inc = %d, IsPathBroken( 0 ) = %s", pPath->IsFinished() ? "true" : "false", inc, IsPathBroken( 0 ) ? "true" : "false" );
		p3 = pPath->PeekPoint( 1 );
		pPath->Shift( 1 );
	}

	nIter = 0;	
	spline.Init( p0, p1, p2, p3 );

	//DebugInfoManager()->CreateCircle( NDebugInfo::OBJECT_ID_FORGET, CCircle( p3, 10 ), NDebugInfo::RED );

	return inc;
}

void CStandartSmoothPathBasis::Stop()
{
	bStopped = true;
	bFinished = true;
}

void CStandartSmoothPathBasis::FinishPath()
{
	CStandartSmoothPathBasis::Stop();
	pPath = 0;
}

void CStandartSmoothPathBasis::OnSerialize( IBinSaver &f )
{
	SerializeBasePathUnit( f, 2, &pUnit );
}


