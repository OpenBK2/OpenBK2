#include "stdafx.h"

#include "AIMap.h"
#include "BasePathUnit.h"
#include "CollisionInternal2.h"
#include "StandartDirPath.h"
#include "Terrain.h"

#include "DebugTools/DebugInfoManager.h"
#include "Misc/Geom.h"
#include "System/RandomGen.h"

#include <algorithm>

static const float SPEED_TIME = 2;
static const float MIN_SPEED_LENGTH = 1.5f;
static const float FULL_SPEED_TIME = 5;
static const float MIN_FULL_SPEED_LENGTH = 3.5f;
static const float PASS_ONE_ANOTHER_BOUND = 1.7f;
static const float PASS_ONE_ANOTHER_COEFF = 0.25f;

// find intersection of two segments, return -1 x -1 if segments not intersected
static const float FAR_INTERSECTION_DIST = 400.0f;

static std::string szTempString;

const std::string &GetCollideTypeName( const NCollision::ECollideType eType )
{
	if ( eType & NCollision::ECT_FIRST )
		szTempString = "ECT_FIRST";
	else if( eType & NCollision::ECT_SECOND )
		szTempString = "ECT_SECOND";
	else
		szTempString = "ECT";
  if ( eType & NCollision::ECT_WAIT )
		szTempString += "_WAIT";
	else if ( eType & NCollision::ECT_GIVEPLACE )
		szTempString += "_GIVEPLACE";
	else if ( eType & NCollision::ECT_HARD )
		szTempString += "_HARD";
	else
		szTempString += "_NONE";

  return szTempString;
}

enum EIntersectionType
{
	EIT_PARALLEL,
	EIT_OUTSIDE_BOTH,
	EIT_OUTSIDE_FIRST,
	EIT_OUTSIDE_SECOND,
	EIT_INTERSECTED,
	EIT_INTERSECTED_FAR,
	EIT_INTERSECTED_PARALLEL, // returns center of segment vPoint1 - vPoint2
};

const EIntersectionType FindIntersection( CVec2 *pvResult, const CVec2 &vPoint1, const CVec2 &vDir1, const CVec2 &vPoint2, const CVec2 &vDir2 )
{
	const float dx = vPoint1.x - vPoint2.x;
	const float dy = vPoint1.y - vPoint2.y;
	const float d = vDir1.x*vDir2.y - vDir2.x*vDir1.y;
	if ( fabs( d ) < FP_EPSILON )
	{
		if ( fabs( dx/vDir2.x - dy/vDir2.y ) < FP_EPSILON )
		{
			if ( pvResult )
				*pvResult = ( vPoint1 + vPoint2 )/2.0f;
			return EIT_INTERSECTED_PARALLEL;
		}
		if ( pvResult )
			*pvResult = VNULL2;
		return EIT_PARALLEL;
	}
	const float a = ( -dx*vDir2.y + dy*vDir2.x )/d;
	const float b = ( fabs( vDir2.x ) < FP_EPSILON ) ? ( dy + a*vDir1.y )/vDir2.y : ( dx + a*vDir1.x )/vDir2.x;
	if ( pvResult )
		*pvResult = vPoint1 + a*vDir1;
	if ( a < 0 && b < 0 )
		return EIT_OUTSIDE_BOTH;
	else if ( a < 0 )
		return EIT_OUTSIDE_FIRST;
	else if ( b < 0 )
		return EIT_OUTSIDE_SECOND;
	else if ( a > FAR_INTERSECTION_DIST || b > FAR_INTERSECTION_DIST )
		return EIT_INTERSECTED_FAR;
	else
		return EIT_INTERSECTED;
}

// vDir1 must be normalized, vDir2 can be unnormalized !!!
inline float GetAngleCos( const CVec2 &vDir1, const CVec2 &vDir2 )
{
	CVec2 vDir2Norm( vDir2 );
	if ( !Normalize( &vDir2Norm ) )
		return 0.0f;
	return vDir1*vDir2Norm;
}

inline const CVec2 GetAABBHalfSize( const CBasePathUnit *pUnit )
{
	const SUnitProfile profile( pUnit->GetUnitProfile() );
	const float fWidthCoeff = profile.IsCircle() ? 0.5 : 1.0f;
	return CVec2( fWidthCoeff * profile.GetHalfWidth(), profile.GetHalfLength() );
}

// fAhead - multiplied
inline const SRect GetUnitRect( const CBasePathUnit *pUnit, const float fAhead )
{
	const CVec2 vAABBHalfSize( GetAABBHalfSize( pUnit ) );
	SRect unitRect;
	unitRect.InitRect( pUnit->GetCenterPlain(), pUnit->GetDirectionVector(), vAABBHalfSize.y * fAhead, vAABBHalfSize.y, vAABBHalfSize.x * 0.8f );
	return unitRect;
}

// fAhead - added
inline const SRect GetSpeedRect( const CBasePathUnit *pUnit, const float fAhead )
{	
	const CVec2 vAABBHalfSize( GetAABBHalfSize( pUnit ) );
	SRect unitRect;
	unitRect.InitRect( pUnit->GetCenterPlain(), pUnit->GetMoveDirection(), vAABBHalfSize.y + fAhead, 0.0f, vAABBHalfSize.x * 0.8f );
	return unitRect;
} 

struct SCheckRect : public SIterateUnitsCallback
{
	SRect unitRect;
	int nSkipID;
	SCheckRect() : nSkipID( -1 ) { unitRect.InitRect( VNULL2, VNULL2, VNULL2, VNULL2 ); }
	SCheckRect( const SRect &_unitRect, const int _nSkipID ) : nSkipID( _nSkipID ),  unitRect( _unitRect ) {}
	bool Iterate( CBasePathUnit *pCand ) const
	{
		if ( pCand->GetUniqueID() == nSkipID ) 
			return true;
		const SRect candRect( GetUnitRect( pCand, 1.0f ) );
		return !unitRect.IsIntersected( candRect );
	}
};

const bool CheckRect( CBasePathUnit *pUnit, CBasePathUnit *pPusher, const CVec2 &vFinishPoint )
{
	const CVec2 vAABBHalfSize( GetAABBHalfSize( pUnit ) );
	const float fDistance = fabs( vFinishPoint - pUnit->GetCenterPlain() );
	if ( fDistance < FP_EPSILON )
		return true;
	const CVec2 vDirection = ( vFinishPoint - pUnit->GetCenterPlain() )/fDistance;
	SRect unitRect;
	unitRect.InitRect( pUnit->GetCenterPlain(), vDirection, vAABBHalfSize.y + fDistance, vAABBHalfSize.y, vAABBHalfSize.x );
	return pUnit->IterateUnits( pUnit->GetCenterPlain(), fDistance, true, SCheckRect( unitRect, pPusher->GetUniqueID() ) );
}

const NCollision::ECollideType GetCollideType( const CBasePathUnit *pUnit, const SRect &unitRect, const CBasePathUnit *pCand, const SRect &candRect, float *pfDistance )
{
	const float fUnitSpeed = ( pCand->IsInfantry() ) ? pUnit->GetMaxPossibleSpeed() : pUnit->GetSpeed();
	const float fMaxSpeed = (std::max)( fUnitSpeed * unitRect.lengthAhead, pCand->GetSpeed() * candRect.lengthAhead );
	const float fFullSpeedAhead = (std::max)( fMaxSpeed * FULL_SPEED_TIME, MIN_FULL_SPEED_LENGTH );

	const SRect unitFullSpeedRect( GetSpeedRect( pUnit, fFullSpeedAhead ) );
	const SRect candFullSpeedRect( GetSpeedRect( pCand, fFullSpeedAhead ) );

	const SRect unitSpeedRect( GetUnitRect( pUnit, (std::max)( SPEED_TIME * pUnit->GetSpeed(), MIN_SPEED_LENGTH ) ) );
	const SRect candSpeedRect( GetUnitRect( pCand, (std::max)( SPEED_TIME * pCand->GetSpeed(), MIN_SPEED_LENGTH ) ) );

	//DebugInfoManager()->DrawRect( pUnit->GetUniqueID()*100, unitRect, 204.0f, NDebugInfo::WHITE );
	//DebugInfoManager()->DrawRect( pCand->GetUniqueID()*100, candRect, 204.0f, NDebugInfo::WHITE );
	//DebugInfoManager()->DrawRect( pUnit->GetUniqueID()*10000, unitSpeedRect, 200.0f, NDebugInfo::GREEN );
	//DebugInfoManager()->DrawRect( pCand->GetUniqueID()*10000, candSpeedRect, 200.0f, NDebugInfo::GREEN );
	//DebugInfoManager()->DrawRect( pUnit->GetUniqueID()*1000000, unitFullSpeedRect, 202.0f, NDebugInfo::RED );
	//DebugInfoManager()->DrawRect( pCand->GetUniqueID()*1000000, candFullSpeedRect, 202.0f, NDebugInfo::RED );

	if ( !unitFullSpeedRect.IsIntersected( candFullSpeedRect ) && !unitSpeedRect.IsIntersected( candSpeedRect ) )
		return NCollision::ECT_NONE;

	const bool bUnitSpeedRectCand = unitSpeedRect.IsIntersected( candRect );
	const bool bCandSpeedRectUnit = candSpeedRect.IsIntersected( unitRect );

	if ( bUnitSpeedRectCand && bCandSpeedRectUnit )
	{
		*pfDistance = fabs( unitRect, candRect );
		if ( pCand->IsInfantry() )
			return NCollision::ECT_SECOND_HARD;
		const CVec2 vDiff21( pCand->GetCenterPlain() - pUnit->GetCenterPlain() );
		if ( pUnit->GetMoveDirection()*vDiff21 > -pCand->GetMoveDirection()*vDiff21 )
			return NCollision::ECT_FIRST_HARD;
		else
			return NCollision::ECT_SECOND_HARD;
	}
	else if ( bUnitSpeedRectCand && !bCandSpeedRectUnit )
	{
		*pfDistance = fabs( unitSpeedRect, candRect );
		return ( pCand->IsInfantry() ) ? NCollision::ECT_SECOND_WAIT : NCollision::ECT_FIRST_WAIT;
	}
	else if ( bCandSpeedRectUnit && !bUnitSpeedRectCand )
	{
		*pfDistance = fabs( candSpeedRect, unitRect );
		return NCollision::ECT_SECOND_WAIT;
	}
	else
	{
		const bool bUnitFullSpeedRectCand = unitFullSpeedRect.IsIntersected( candRect );
		const bool bCandFullSpeedRectUnit = candFullSpeedRect.IsIntersected( unitRect );

		if ( bUnitFullSpeedRectCand && !bCandFullSpeedRectUnit )
		{
			*pfDistance = fabs( unitFullSpeedRect, candRect );
			return ( pCand->IsInfantry() ) ? NCollision::ECT_SECOND_WAIT : NCollision::ECT_FIRST_WAIT;
		}
		else if ( bCandFullSpeedRectUnit && !bUnitFullSpeedRectCand )
		{
			*pfDistance = fabs( candFullSpeedRect, unitRect );
			return NCollision::ECT_SECOND_WAIT;
		}
		else if ( bUnitFullSpeedRectCand && bCandFullSpeedRectUnit )
		{
			*pfDistance = (std::min)( fabs( unitFullSpeedRect, candRect ), fabs( candFullSpeedRect, unitRect ) );
			if ( Cross( unitRect.dir,pCand->GetCenterPlain() - pUnit->GetCenterPlain() ) > 0.0f && !pCand->IsInfantry() )
				return NCollision::ECT_FIRST_GIVEPLACE;
			else
				return NCollision::ECT_SECOND_GIVEPLACE;
		}
		return  NCollision::ECT_NONE;
	}
}

struct SFindCandidates : public SIterateUnitsCallback
{
	CBasePathUnit *pUnit;
	SRect unitRect;
	ICollisionsCollector *pCollisionsCollector;

	SFindCandidates() : pUnit( 0 ), pCollisionsCollector( 0 ) {}
	SFindCandidates( CBasePathUnit *_pUnit, ICollisionsCollector *_pCollisionsCollector )
		: pUnit( _pUnit ), pCollisionsCollector( _pCollisionsCollector )
	{
		unitRect = GetUnitRect( pUnit, 1.0f );
	}

	bool Iterate( CBasePathUnit *pCand ) const
	{
		if ( !pCand->IsLockingTiles() && pCand->GetCollidingType( pUnit ) != ECT_NONE && ( pCand->IsInfantry() || pCand->GetUniqueID() > pUnit->GetUniqueID() ) )
		{
			const SRect candRect( GetUnitRect( pCand, 1.0f ) );

			float fDistance = 0.0f;
			const NCollision::ECollideType eCollideType = GetCollideType( pUnit, unitRect, pCand, candRect, &fDistance );
			if ( eCollideType == NCollision::ECT_NONE )
				return true;

			// код где давим, расчитано на то, что давить будем только пехоту !!!
			if ( eCollideType == NCollision::ECT_SECOND_HARD && fDistance == 0.0f && pCand->CanUnitTrampled( pUnit ) )
			{
				pCand->UnitTrampled( pUnit );
				return true;
			}

			//DebugTrace( "%d collide with %d at distance %2.3f by %s", pUnit->GetUniqueID(), pCand->GetUniqueID(), fDistance, GetCollideTypeName( eCollideType ).c_str() );
			if ( eCollideType & NCollision::ECT_FIRST )
				pCollisionsCollector->AddCollision( pUnit, pCand, fDistance, eCollideType );
			else
				pCollisionsCollector->AddCollision( pCand, pUnit, fDistance, eCollideType );
		}
		return true;
	}
};

// CCollisionBase                                                       

void CCollisionBase::Init( CBasePathUnit *_pUnit, CBasePathUnit *_pPushUnit, const int _nPriority )
{
	pUnit = _pUnit;
	pPushUnit = _pPushUnit;
	nPriority = _nPriority;
	GetUnit()->SetCollision( this, GetPath() );
}

const bool CCollisionBase::CanFindCandidates() const
{
	return !pUnit->IsInfantry() && pUnit->GetCollidingType( 0 ) != ECT_NONE && !pUnit->IsLockingTiles();
}

void CCollisionBase::FindCandidates( ICollisionsCollector *pCollisionsCollector )
{
	if ( CanFindCandidates() )
	{
		const SRect rect( GetUnitRect( pUnit, 1.0f ) );
		pUnit->IterateUnits( pUnit->GetCenterPlain(), 3.5f * rect.lengthAhead, false, SFindCandidates( pUnit, pCollisionsCollector ) );
	}
}

void CCollisionBase::OnSerialize( IBinSaver &f )
{
	SerializeBasePathUnit( f, 2, &pUnit );
	SerializeBasePathUnit( f, 3, &pPushUnit );
}

// CFreeCollision

const bool CFreeCollision::IsSolved() const
{
	return GetUnit()->IsPathFinished() && !GetUnit()->IsTurning();
}

// CGivingPlace

void CGivingPlaceCollision::Init( CBasePathUnit *pUnit, CBasePathUnit *pPushUnit, const int nPriority, const CVec2 &vFinishPoint, const int nTileSize )
{
	CPtr<CStandartDirPath> pPath = new CStandartDirPath( pUnit->GetCenterPlain(), vFinishPoint, nTileSize );
	SetPath( pPath );
	CCollisionBase::Init( pUnit, pPushUnit, nPriority );
	//DebugInfoManager()->DrawLine( pUnit->GetUniqueID(), pUnit->GetCenterPlain(), vFinishPoint, true, 100.0f, CVec4( 255, 255, 255, 255 ) );
}

const bool CGivingPlaceCollision::IsSolved() const
{
	return GetUnit()->IsPathFinished() || GetPushUnit()->IsLockingTiles();
}

// CWaitingCollision

void CWaitingCollision::Init( CBasePathUnit *pUnit, CBasePathUnit *pPushUnit, const int nPriority, const float fDistance )
{
	bIsSolved = false;
	CCollisionBase::Init( pUnit, pPushUnit, nPriority );
	pUnit->NotifyAboutClosestThreat( pPushUnit, fDistance );
	pUnit->UpdateCollisionStayTime( pPushUnit->GetStayTime() );
	timeToWait = NRandom::Random( 2000, 5000 );
}

void CWaitingCollision::Segment( const NTimer::STime timeDiff )
{
	if ( timeDiff > timeToWait )
		timeToWait = 0;
	else
		timeToWait -= timeDiff;
	const SRect unitSpeedRect( GetUnitRect( GetUnit(), (std::max)( SPEED_TIME * GetUnit()->GetSpeed(), MIN_SPEED_LENGTH ) ) );
	const SRect pusherRect(GetUnitRect( GetPushUnit(), 1.0f ) );
	bIsSolved = !unitSpeedRect.IsIntersected( pusherRect ) || timeToWait == 0;
}

// CStopCollision

void CStopCollision::Init( CBasePathUnit *pUnit, CBasePathUnit *pPushUnit, const int nPriority )
{
	CCollisionBase::Init( pUnit, pPushUnit, nPriority );
	timeLeft = NRandom::Random( 1000, 3000 );
	pUnit->ForceLockTiles();
}

void CStopCollision::Segment( const NTimer::STime timeDiff )
{
	timeLeft = timeDiff > timeLeft ? 0 : timeLeft - timeDiff;
}

// SSortCollisions                                                  

struct SSortCollisions
{
	CPtr<CCollisionsCollector> pCollector;
	SSortCollisions() : pCollector( 0 ) {}
	SSortCollisions( CCollisionsCollector *_pCollector ) : pCollector ( _pCollector ) {}
	const bool operator()( const int nIndex1, const int nIndex2 )
	{
		CCollisionsCollector::TCollisions::const_iterator pos1 = pCollector->collisions.find( nIndex1 );
		CCollisionsCollector::TCollisions::const_iterator pos2 = pCollector->collisions.find( nIndex2 );
		if ( pos1 == pos2 )
			return nIndex1 >= nIndex2;	// CRAP: this is bad as std::sort wants only > instead of >=, but indexes are unique so it's not critical
		else if ( pos1 != pCollector->collisions.end() && pos2 != pCollector->collisions.end() )
		{
			if ( pos1->second.pushers.size() == pos2->second.pushers.size() )
				return nIndex1 >= nIndex2;
			else
				return pos1->second.pushers.size() < pos2->second.pushers.size();
		}
		else
			return pos1 == pCollector->collisions.end();
	}
};

const bool CCollisionsCollector::SPushersSort::operator()( const SPusherInfo &pusher1, const SPusherInfo &pusher2 ) const
{
	if ( ( pusher1.eCollideType & NCollision::ECT_TYPE_MASK ) == ( pusher2.eCollideType & NCollision::ECT_TYPE_MASK ) )
	{
		if ( pusher1.fDistance == pusher2.fDistance )
			return pusher1.fDistance < pusher2.fDistance;
		return pusher1.pUnit->GetUniqueID() > pusher2.pUnit->GetUniqueID();
	}
	return (int)pusher1.eCollideType < (int)pusher2.eCollideType;
};

// CCollsionsCollector                                                  

void CCollisionsCollector::AddCollision( CBasePathUnit *pUnit, CBasePathUnit *pPusher, const float fDistance, const NCollision::ECollideType eCollideType )
{
	TCollisions::iterator pos = collisions.find( pUnit->GetUniqueID() );
	if ( pos == collisions.end() )
		collisions[pUnit->GetUniqueID()] = SCollision( pUnit, pPusher, fDistance, (NCollision::ECollideType)( eCollideType & NCollision::ECT_TYPE_MASK ) );
	else
		pos->second.AddPusher( pPusher, fDistance, (NCollision::ECollideType)( eCollideType & NCollision::ECT_TYPE_MASK ) );
}

const bool CCollisionsCollector::PassOneAnother( CBasePathUnit *pUnit1, CBasePathUnit *pUnit2, const int nTileSize ) const
{
	if ( pUnit1->GetCollision()->GetName() == NCollision::ECN_GIVE_PLACE || pUnit2->GetCollision()->GetName() == NCollision::ECN_GIVE_PLACE )
		return true;

	if ( pUnit1->IsInfantry() || pUnit2->IsInfantry() ) 
		return false;

	if ( pUnit1->GetMoveDirection() * pUnit2->GetMoveDirection() > -0.8f )
		return false;

	CVec2 vCenter;
	const EIntersectionType eIntersection = FindIntersection( &vCenter, pUnit1->GetCenterPlain(), pUnit1->GetMoveDirection(), pUnit2->GetCenterPlain(), pUnit2->GetMoveDirection() );
	if ( eIntersection != EIT_INTERSECTED /* && eIntersection != EIT_PARALLEL */ )
		vCenter = ( pUnit1->GetCenterPlain() + pUnit2->GetCenterPlain() )/2.0f;

	CVec2 vDir( pUnit1->GetMoveDirection() + pUnit2->GetMoveDirection() );
	if ( !Normalize( &vDir ) )
	{
		vDir = pUnit1->GetCenterPlain() - pUnit2->GetCenterPlain();
		if ( !Normalize( &vDir ) )
			vDir = pUnit1->GetMoveDirection();
		const float f = vDir.x;
		vDir.x = vDir.y;
		vDir.y = -f;
	}
	float fDist = 0.5f * PASS_ONE_ANOTHER_BOUND * ( pUnit1->GetAABBHalfSize().x + pUnit2->GetAABBHalfSize().x );
	const CVec2 vPoint1 = ( vCenter + fDist * vDir ) * ( 1 + PASS_ONE_ANOTHER_COEFF ) - pUnit1->GetCenterPlain() * PASS_ONE_ANOTHER_COEFF;
	const CVec2 vPoint2 = ( vCenter - fDist * vDir ) * ( 1 + PASS_ONE_ANOTHER_COEFF ) - pUnit2->GetCenterPlain() * PASS_ONE_ANOTHER_COEFF;

	const float fDiffAngle1 = GetAngleCos( pUnit1->GetMoveDirection(), vPoint1 - pUnit1->GetCenterPlain() ) - GetAngleCos( pUnit1->GetMoveDirection(), vPoint2 - pUnit1->GetCenterPlain() );
	const float fDiffAngle2 = GetAngleCos( pUnit2->GetMoveDirection(), vPoint1 - pUnit2->GetCenterPlain() ) - GetAngleCos( pUnit2->GetMoveDirection(), vPoint2 - pUnit2->GetCenterPlain() );
	if ( fabs( fDiffAngle1 ) > fabs( fDiffAngle2 ) && fDiffAngle1 > 0 || fabs( fDiffAngle1 ) < fabs( fDiffAngle2 ) && fDiffAngle2 < 0 )
	{
		CPtr<CGivingPlaceCollision> pCollision1 = new CGivingPlaceCollision();
		pCollision1->Init( pUnit1, pUnit2, -1, vPoint1, nTileSize );
		CPtr<CGivingPlaceCollision> pCollision2 = new CGivingPlaceCollision();
		pCollision2->Init( pUnit2, pUnit1, -1, vPoint2, nTileSize );
	}
	else
	{
		CPtr<CGivingPlaceCollision> pCollision1 = new CGivingPlaceCollision();
		pCollision1->Init( pUnit1, pUnit2, -1, vPoint2, nTileSize );
		CPtr<CGivingPlaceCollision> pCollision2 = new CGivingPlaceCollision();
		pCollision2->Init( pUnit2, pUnit1, -1, vPoint1, nTileSize );
	}

	return true;
}

const bool CCollisionsCollector::NotifyAboutClosestThreat( CBasePathUnit *pUnit, CBasePathUnit *pPusher, const float fDistance ) const
{
	if ( pPusher->GetCollision()->GetPushUnit() != pUnit && !pUnit->IsInfantry() )
	{
		pUnit->NotifyAboutClosestThreat( pPusher, fDistance );
		pUnit->UpdateCollisionStayTime( pPusher->GetStayTime() );
		return true;
	}
	return false;
}

const bool CCollisionsCollector::FindWayForInfantry( CBasePathUnit *pInfantry, CBasePathUnit *pPusher, CAIMap *pAIMap ) const
{
	if ( !pInfantry->IsInfantry() )
		return false;

	if ( pInfantry->GetLastPushUnit() == pPusher ) 
		return true;

	const CVec2 vPusherMoveDir( pPusher->GetMoveDirection() );
	const CVec2 vPusherPerpDir( vPusherMoveDir.y, -vPusherMoveDir.x );
	const CVec2 vAABBHalfSize( GetAABBHalfSize( pPusher ) );
	const CVec2 vInfantryCenter( pInfantry->GetCenterPlain() );
	const int nBoundTileRadius( pInfantry->GetBoundTileRadius() );
	const EAIClasses aiClass( pInfantry->GetAIPassabilityClass() );
	const CVec2 vPoint1 = pAIMap->GetTerrain()->GetValidPoint( nBoundTileRadius, vInfantryCenter, vInfantryCenter + 1.25f * vPusherPerpDir * vAABBHalfSize.x, aiClass, false, pAIMap );
	const CVec2 vPoint2 = pAIMap->GetTerrain()->GetValidPoint( nBoundTileRadius, vInfantryCenter, vInfantryCenter - 1.25f * vPusherPerpDir * vAABBHalfSize.x, aiClass, false, pAIMap );
	const CLine2 vCenterLine( pPusher->GetCenterPlain(), pPusher->GetCenterPlain() + vPusherMoveDir );
	const float fDist =  vCenterLine.a * vInfantryCenter.x + vCenterLine.b * vInfantryCenter.y + vCenterLine.c;
	const float fDist21 = fabs2( vPoint1 - vInfantryCenter );
	const float fDist22 = fabs2( vPoint2 - vInfantryCenter );
	const float fDiff2 = fDist21 - fDist22;
	if ( fDiff2 > sqr( 64.0f ) || fDiff2 >= -sqr( 16.0f ) && fDist > 0.0f )
	{
		CPtr<CGivingPlaceCollision> pCollision = new CGivingPlaceCollision();
		pCollision->Init( pInfantry, pPusher, -1, vPoint1, pAIMap->GetTileSize() );
	}
	else
	{
		CPtr<CGivingPlaceCollision> pCollision = new CGivingPlaceCollision();
		pCollision->Init( pInfantry, pPusher, -1, vPoint2, pAIMap->GetTileSize() );
	}
	return true;
}

void CCollisionsCollector::HandOutCollisions( CAIMap *pAIMap )
{
	std::vector<int> sorted;
	for ( TCollisions::const_iterator it = collisions.begin(); it != collisions.end(); ++it ) 
		sorted.push_back( it->first );
	std::sort( sorted.begin(), sorted.end(), SSortCollisions( this ) );
	for ( std::vector<int>::const_iterator it = sorted.begin(); it != sorted.end(); ++it )
	{
		TCollisions::iterator pos = collisions.find( *it );
		if ( pos != collisions.end() )
		{
			CBasePathUnit *pUnit = pos->second.pUnit->GetUnit();
			std::vector<SPusherInfo> &pushers =  pos->second.pushers;
			std::sort( pushers.begin(), pushers.end(), SPushersSort() );
			if ( pUnit->IsInfantry() )
				FindWayForInfantry( pUnit, pushers[0].pUnit->GetUnit(), pAIMap );
			else
			{
				for ( std::vector<SPusherInfo>::const_iterator itPusher = pushers.begin(); itPusher != pushers.end(); ++ itPusher )
				{
					CBasePathUnit *pPusher = itPusher->pUnit->GetUnit();
					const NCollision::ECollideType eCollideType = itPusher->eCollideType;
					const float fDistance = itPusher->fDistance;
					switch ( eCollideType )
					{
					case NCollision::ECT_WAIT:
						NotifyAboutClosestThreat( pUnit, pPusher, fDistance );
						break;
					case NCollision::ECT_GIVEPLACE:
						if ( !PassOneAnother( pUnit, pPusher, pAIMap->GetTileSize() ) )
							NotifyAboutClosestThreat( pUnit, pPusher, fDistance );
						break;
					case NCollision::ECT_HARD:
						if ( pPusher->GetCollision()->GetPushUnit() != pUnit ) 
						{
							CPtr<CWaitingCollision> pCollision = new CWaitingCollision();
							pCollision->Init( pUnit, pPusher, -1, fDistance );
						}
						break;
					}
				}
			}
			collisions.erase( pos );
		}
	}
	NI_ASSERT( collisions.empty(), "WARNING: Collisions not empty !!!" );
	collisions.clear();
}

// Creators                                                             

ICollisionsCollector *CreateCollisionsCollector()
{
	return new CCollisionsCollector();
}

ICollision *CreateCollision( CBasePathUnit *pUnit, CBasePathUnit *pPushUnit, const int nPriority, const NCollision::ECollisionName eName )
{
	CPtr<ICollision> pCollision = MakeObject<ICollision>( (int)eName );
	if ( IsValid( pCollision ) )
	{
		pCollision->Init( pUnit, pPushUnit, nPriority );
		return pCollision.Extract();
	}
	return 0;
}

BASIC_REGISTER_CLASS( ICollisionsCollector );
REGISTER_SAVELOAD_CLASS( 0x31223C00, CBasePathUnitHolder );
REGISTER_SAVELOAD_CLASS( 0x31223C02, CCollisionsCollector );

// save/load for collisions
using namespace NCollision;
REGISTER_SAVELOAD_CLASS( ECN_FREE, CFreeCollision );
REGISTER_SAVELOAD_CLASS( ECN_WAIT, CWaitingCollision );
REGISTER_SAVELOAD_CLASS( ECN_GIVE_PLACE, CGivingPlaceCollision );
REGISTER_SAVELOAD_CLASS( ECN_STOP, CStopCollision );

