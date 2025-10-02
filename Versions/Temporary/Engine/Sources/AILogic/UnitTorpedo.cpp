#include "stdafx.h"

#include "UnitTorpedo.h"
#include "../Common_RTS_AI/StaticMapHeights.h"
#include "Shell.h"
#include "NewUpdater.h"
#include "UnitsIterators2.h"

extern CShellsStore theShellsStore;
extern CEventUpdater updater;
extern NTimer::STime curTime;

const int nTorpedoSearchRadius = 10 * AI_TILE_SIZE;

// CTorpedoStatesFactory

CPtr<CTorpedoStatesFactory> CTorpedoStatesFactory::pFactory = 0;

IStatesFactory* CTorpedoStatesFactory::Instance()
{
	if ( pFactory == 0 )
		pFactory = new CTorpedoStatesFactory();

	return pFactory;
}

bool CTorpedoStatesFactory::CanCommandBeExecuted( class CAICommand *pCommand )
{
	return false;
}

IUnitState* CTorpedoStatesFactory::ProduceState( class CQueueUnit *pObj, CAICommand *pCommand )
{
	return 0;
}

IUnitState* CTorpedoStatesFactory::ProduceRestState( class CQueueUnit *pUnit )
{
	NI_ASSERT( dynamic_cast<CUnitTorpedo*>( pUnit ) != 0, "Wrong unit type" );
	CUnitTorpedo *pTorpedo = checked_cast<CUnitTorpedo*>( pUnit );

	CVec2 vDir = pTorpedo->GetCenterPlain() + 1000 * GetVectorByDirection( pTorpedo->GetDirection() );
	CTorpedoAttackState *pState = new CTorpedoAttackState( pTorpedo, vDir );

	return pState;
}

// CTorpedoAttackState

CTorpedoAttackState::CTorpedoAttackState( CUnitTorpedo *pUnit, const CVec2 &_vPoint )
: pTorpedo( pUnit )
{
	// Calculate movement parameters

	// Create path
	pPath = new CTorpedoPath;
	pPath->Init( pUnit, pUnit->GetCenterPlain(), _vPoint, pTorpedo->GetSpeed() );

	pUnit->SendAlongSmoothPath( pPath );
}

void CTorpedoAttackState::Segment()
{
}

// CUnitTorpedo

void CUnitTorpedo::Init( const CVec2 &center, const int z, CAIUnit *_pOwner, const SWeaponRPGStats *_pShooterStats, const SMechUnitRPGStats *_pTorpedoStats, const float fHP, const WORD dir, const BYTE player, ICollisionsCollector *pCollisionsCollector )
{
	pShooterStats = _pShooterStats;
	pTorpedoStats = _pTorpedoStats;
	pOwner = _pOwner;

	// Speed might be too fast
	fSpeed = pShooterStats->shells[0].fSpeed;

	timeLaunched = curTime;

	CAIUnit::Init( center, z, fHP, dir, player, pCollisionsCollector );
}

void CUnitTorpedo::Segment()
{
	// Calculate the tip of the torpedo
	vContactPoint = GetVectorByDirection( GetDirection() ) * GetAABBHalfSize().y + GetCenterPlain();

	// Check for being outside AI map
	if ( !GetAIMap()->IsPointInside( GetCenterPlain() ) )
	{
		Die( false, 1.0f );
		return;
	}

	// Do not explode in the 1st second
	if ( curTime - timeLaunched < 1000 )
		return;

	// Check for impending collisions
	for ( CUnitsIter<0,3> iter( 0, ANY_PARTY, GetCenterPlain(), nTorpedoSearchRadius );
		!iter.IsFinished(); iter.Iterate() )
	{
		CPtr<CAIUnit> curUnit = *iter;
		if ( curUnit == this )
			continue;

		if ( curUnit == pOwner )			// torpedo is under the owner, continue
			return;

		if ( curUnit->GetStats() == GetStats() )		// another torpedo
			continue;

		if ( curUnit->IsAlive() && !curUnit->GetStats()->IsAviation() )
		{
			if ( curUnit->GetUnitRect().IsPointInside( vContactPoint ) )
			{
				Die( true, 1.0f );
				return;
			}
		}
	}

	// Check for passability
	if ( GetTerrain()->IsLocked( GetCenterTile(), (EAIClasses)pTorpedoStats->nAIPassabilityClass ) )
	{
		Die( true, 1.0f );
		return;
	}
}

void CUnitTorpedo::Die( const bool fromExplosion, const float fDamage )
{
	if ( fromExplosion )	// hit an object 
	{
		CAIUnit::Die( false, fDamage );
		CVec3 vExplPoint( vContactPoint, GetHeights()->GetZ( vContactPoint ) );
		theShellsStore.AddShell( CMomentShell( new CBurstExpl( this, pShooterStats, vExplPoint, vExplPoint, 0, true, 1, true ) ) );
		updater.AddUpdate( 0, ACTION_NOTIFY_DISSAPEAR_OBJ, this, 1 );
	}
	else			// reached edge of map
	{
		CAIUnit::Die( false, fDamage );
		updater.AddUpdate( 0, ACTION_NOTIFY_DISSAPEAR_OBJ, this, 0 );
	}
}

const bool CUnitTorpedo::IterateUnits( const CVec2 &vCenter, const float fRadius,	const bool bOnlyMech, const SIterateUnitsCallback &callback ) const
{
	return true;
}

// CTorpedoPath

bool CTorpedoPath::Init( CBasePathUnit *_pUnit, const class CVec2 &_vCurPoint, const CVec2 &_vEndPoint, const float fSpeed )
{
	pUnit = _pUnit;
	vCurPoint = _vCurPoint;

	vSpeed = _vEndPoint - _vCurPoint;
	Normalize( &vSpeed );
	vSpeed *= fSpeed;

	return true;
}

const CVec2 CTorpedoPath::PeekPathPoint( const int nToShift ) const
{
	return vCurPoint + vSpeed * nToShift;
}

void CTorpedoPath::Segment( const NTimer::STime timeDiff )
{
	vCurPoint += vSpeed * timeDiff;

	pUnit->SetCenter( CVec3( vCurPoint.x, vCurPoint.y, GetHeights()->GetZ( vCurPoint ) - 1 ) );
}

int CTorpedoPath::operator&( IBinSaver &saver )
{
	saver.Add( 1, &vCurPoint );
	saver.Add( 2, &vEndPoint );
	saver.Add( 3, &vSpeed );
	SerializeBasePathUnit( saver, 4, &pUnit );

	return 0;
}

REGISTER_SAVELOAD_CLASS( 0x191644C0, CUnitTorpedo );
REGISTER_SAVELOAD_CLASS( 0x191644C1, CTorpedoStatesFactory );
REGISTER_SAVELOAD_CLASS( 0x19164B00, CTorpedoAttackState );
REGISTER_SAVELOAD_CLASS( 0x1917A2C0, CTorpedoPath );

