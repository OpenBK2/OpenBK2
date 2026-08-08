#include "stdafx.h"

#include "AIMap.h"
#include "GroupMover.h"
#include "CommonUnit.h"

#include "Common_RTS_AI/AIMap.h"
#include "Common_RTS_AI/StaticPath.h"
#include "Common_RTS_AI/StaticPathInternal.h"
#include "Common_RTS_AI/Terrain.h"

#include "DebugTools/DebugInfoManager.h"

#include "Stats_B2_M1/AIUnitCmd.h"
#include "Stats_B2_M1/ActionCommand.h"

#include "System/RandomGen.h"

#include <map>

extern NTimer::STime curTime;

static const float DIST_THRESHOLD = 256.0f;
static const float FORCE_COEFF    = 0.25f;
static const int BALANCE_STEP_COUNT = 4;
static const NTimer::STime PATH_LIFE_TIME = 500;

struct SForce
{
	int nUnitID;
	CVec2 vForce;
	CVec2 vPosition;

	SForce() : nUnitID( -1 ), vForce( VNULL2 ), vPosition( VNULL2 ) {}
	SForce( const int _nUnitID, const CVec2 &_vPosition ) : nUnitID( _nUnitID ), vForce( VNULL2 ), vPosition( _vPosition ) {}
};

struct SUnitHash
{
	int operator()( CCommonUnit *pUnit ) const { return pUnit->GetUniqueID(); }
};

static const bool CanBeOneGroup( const CCommonUnit *pUnit1, const CCommonUnit *pUnit2 )
{
	return fabs2( pUnit1->GetCenterPlain() - pUnit2->GetCenterPlain() ) < sqr( SConsts::GROUP_DISTANCE );
}

IStaticPath *CreatePathToPoint( CCommonUnit *pUnit, const CVec2 &vPosition )
{
	pUnit->SetGroupShift( VNULL2 );
	return pUnit->CreateBigStaticPath( pUnit->GetCenterPlain(), vPosition, 0 );
}

void CGroupMover::SSubGroup::Add( CCommonUnit *pUnit )
{
	units[pUnit->GetUniqueID()] = pUnit;
}

void CGroupMover::SSubGroup::Delete( const int nUnitID )
{
	TSubGroupUnits::iterator pos = units.find( nUnitID );
	if ( pos != units.end() )
		units.erase( pos );
}

void CGroupMover::SSubGroup::BalanceGroup( const CVec2 &vPosition )
{
	// special cases for empty and one-unit groups
	if ( IsEmpty() )
		return;
	else if ( units.size() == 1 )
	{
		units.begin()->second.vPosition = vPosition;
		return;
	}


	// remove invalid units from hash
	for ( TSubGroupUnits::iterator it = units.begin(); it != units.end(); )
	{
		if ( IsValid( it->second.pUnit ) )
			++it;
		else
			units.erase( it++ );
	}

	// calculate group center
	CVec2 vGroupCenter( VNULL2 );
	for ( TSubGroupUnits::const_iterator it = units.begin(); it != units.end(); ++it )
		vGroupCenter += it->second.pUnit->GetCenterPlain();
	vGroupCenter /= units.size();

	for ( TSubGroupUnits::iterator it = units.begin(); it != units.end(); ++it )
		it->second.vPosition = vPosition + it->second.pUnit->GetCenterPlain() - vGroupCenter;

	// balance group
	std::vector<SForce> forces;
	forces.reserve( units.size() );
	for ( TSubGroupUnits::iterator it = units.begin(); it != units.end(); ++it )
		forces.push_back( SForce( it->second.pUnit->GetUniqueID(), it->second.vPosition ) );

	// sort forces by uniqueID for determinism
	std::sort(forces.begin(), forces.end(), [](const SForce& f1, const SForce& f2) {
		return f1.nUnitID < f2.nUnitID;
	});
  
	bool bNullForces = false;
	for ( int i = 0; i < BALANCE_STEP_COUNT && !bNullForces; ++i )
	{
		bNullForces = true;

		for ( std::vector<SForce>::iterator it = forces.begin(); it != forces.end(); ++it )
		{
			std::vector<SForce>::iterator it2 = it;
			for ( ++it2; it2 != forces.end(); ++it2 )
			{
				CVec2 vDirection = it->vPosition - it2->vPosition;
				const float fDistance = fabs( vDirection );
				if ( fDistance < DIST_THRESHOLD )
				{
					if ( fDistance >= FP_EPSILON )
						vDirection /= fDistance;
					else
						{ vDirection = GetVectorByDirection( NRandom::Random( 0, 65535 ) ); RecordRandomCall(); }

					const CVec2 vForce = FORCE_COEFF * ( DIST_THRESHOLD - fDistance ) * vDirection;
					it->vForce += vForce;
					it2->vForce -= vForce;
					bNullForces = false;
				}
			}
		}

		for ( std::vector<SForce>::iterator it = forces.begin(); it != forces.end(); ++it )
		{
			it->vPosition += it->vForce;
			it->vForce = VNULL2;
		}
	}

	int nCount = 0;
	std::map<int, SSubGroupUnitInfo*> sortedUnits;

	for ( TSubGroupUnits::iterator it = units.begin(); it != units.end(); ++it )
	{
		sortedUnits[it->first] = &it->second;
	}

	for (auto it = sortedUnits.begin(); it != sortedUnits.end(); it++, nCount++)
	{
		it->second->vPosition = GetAIMap()->GetTerrain()->GetValidPoint( it->second->pUnit->GetBoundTileRadius(), vPosition, forces[nCount].vPosition, it->second->pUnit->GetAIPassabilityClass(), false, GetAIMap() );
		it->second->vPosition = forces[nCount].vPosition;
	}
}

IStaticPath* CGroupMover::SSubGroup::CreateStaticPath( CCommonUnit *pUnit, const CVec2 &vDefaultPoint )
{
	TSubGroupUnits::const_iterator pos = units.find( pUnit->GetUniqueID() );
	if ( pos == units.end() )
		return CreatePathToPoint( pUnit, vDefaultPoint );
	else
		return paths[pUnit->GetAIPassabilityClass()].CreateStaticPath( pUnit, pos->second.vPosition );
}

IStaticPath* CGroupMover::SSubGroup::SSubGroupPathInfo::CreateStaticPath( CCommonUnit *pUnit, const CVec2 &vPoint )
{
	if ( IsValid( pStaticPath ) && pUnit->GetBoundTileRadius() <= nBoundTileRadius && curTime - timeCalced < PATH_LIFE_TIME &&
		fabs2( vStartPoint - pUnit->GetCenterPlain() ) < sqr( SConsts::GROUP_DISTANCE ) && fabs2( vFinishPoint - vPoint ) < sqr( SConsts::GROUP_DISTANCE ) )
	{
		const CVec2 vValidPoint = GetAIMap()->GetTerrain()->GetValidPoint( pUnit->GetBoundTileRadius(), vFinishPoint, vPoint, pUnit->GetAIPassabilityClass(), false, GetAIMap() );
		pUnit->SetGroupShift( vValidPoint - vFinishPoint );
	}
	else
	{
		pStaticPath = CreatePathToPoint( pUnit, vPoint );
		timeCalced = curTime;
		nBoundTileRadius = pUnit->GetBoundTileRadius();
		vStartPoint = pUnit->GetCenterPlain();
		vFinishPoint = IsValid( pStaticPath ) ? pStaticPath->GetFinishPoint() : vStartPoint;
	}
	return pStaticPath;
}

void CGroupMover::AddUnit( CCommonUnit *pUnit )
{
	NI_ASSERT( bNeedCalcPositions, "Cannot add unit. Positions already builded." );
	group[pUnit->GetUniqueID()] = pUnit;
}

void CGroupMover::DeleteUnit( const int nUnitID )
{
	TGroup::iterator posGroup = group.find( nUnitID );
	if ( posGroup == group.end() )
		return;

	const int nUnitSubGroup = posGroup->second->GetSubGroup();
	if ( nUnitSubGroup >= 0 && nUnitSubGroup < subGroups.size() )
		subGroups[nUnitSubGroup].Delete( nUnitID );

	group.erase( posGroup );
}

const bool CGroupMover::CalcPositions()
{
	if ( !bNeedCalcPositions )
		return true;

	// reset subgroups for units
	typedef std::map<int, CPtr<CCommonUnit> > TUnitsHashSet;
	TUnitsHashSet sortedUnits;
	for ( TGroup::const_iterator it = group.begin(); it != group.end(); ++it )
	{
		if ( IsValid( it->second ) )
		{
			it->second->SetSubGroup( -1 );
			sortedUnits[it->second->GetUniqueID()] = it->second;
		}
	}

	// split all units to subgroups
	int nCurrentGroup = 0;
	TUnitsHashSet unitsToCheck;
	while( !sortedUnits.empty() || !unitsToCheck.empty() )
	{
		if ( unitsToCheck.empty() )
		{
			++nCurrentGroup;
			TUnitsHashSet::iterator pos = sortedUnits.begin();
			pos->second->SetSubGroup( nCurrentGroup - 1 );
			unitsToCheck[pos->second->GetUniqueID()] = pos->second;
			sortedUnits.erase( pos );
		}
		else
		{
			TUnitsHashSet::iterator pos = unitsToCheck.begin();
			for( TUnitsHashSet::iterator it = sortedUnits.begin(); it != sortedUnits.end(); )
			{
				if ( CanBeOneGroup( pos->second, it->second ) )
				{
					it->second->SetSubGroup( nCurrentGroup - 1 );
					unitsToCheck[it->second->GetUniqueID()] = it->second;
					sortedUnits.erase( it++ );
				} 
				else
					++it;
			}
			unitsToCheck.erase( pos );
		}
	}
	
	// set units to appropriate subgroups and find largest subgroup
	subGroups.resize( nCurrentGroup );
	int nMaxSubGroupIndex = -1;
	int nMaxSubGroupSize = -1;
	std::vector<int> subGroupSizes( nCurrentGroup, 0 );
	for ( TGroup::const_iterator it = group.begin(); it != group.end(); ++it )
	{
		subGroups[it->second->GetSubGroup()].Add( it->second );
		++subGroupSizes[it->second->GetSubGroup()];
		if ( subGroupSizes[it->second->GetSubGroup()] > nMaxSubGroupSize )
		{
			nMaxSubGroupSize = subGroupSizes[it->second->GetSubGroup()];
			nMaxSubGroupIndex = it->second->GetSubGroup();
		}
	}

	// balance every group to center
	for ( int i = 0; i < subGroups.size(); ++i )
		subGroups[i].BalanceGroup( vPosition );

	bNeedCalcPositions = false;
	return true;
}

IStaticPath* CGroupMover::CreateStaticPath( CCommonUnit *pUnit )
{
	if ( !CalcPositions() )
		return 0;
	if ( !IsValid( pUnit ) )
		return 0;

	const int nUnitSubGroup = pUnit->GetSubGroup();
	if ( nUnitSubGroup >= 0 && nUnitSubGroup < subGroups.size() )
		return subGroups[nUnitSubGroup].CreateStaticPath( pUnit, vPosition );
	else
		return CreatePathToPoint( pUnit, vPosition );
}

CVec2 CGroupMover::GetMoveTarget( CCommonUnit *pUnit )
{
	if ( !CalcPositions() || !IsValid( pUnit ) )
		return vPosition;

	const int nUnitSubGroup = pUnit->GetSubGroup();
	if ( nUnitSubGroup >= 0 && nUnitSubGroup < subGroups.size() )
	{
		SSubGroup::TSubGroupUnits &units = subGroups[nUnitSubGroup].units;
		SSubGroup::TSubGroupUnits::iterator pos = units.find( pUnit->GetUniqueID() );
		if ( pos != units.end() )
		{
			// Aviation selection keeps its relative layout in GroupShift even when the
			// generic mover splits distant helicopters into one-unit subgroups. Capture
			// that shift in the persistent slot before CAICommand clears it.
			if ( fabs2( pUnit->GetGroupShift() ) > FP_EPSILON )
				pos->second.vPosition = vPosition + pUnit->GetGroupShift();
			return pos->second.vPosition;
		}
	}

	return vPosition;
}

CGroupMover *CreateGroupMover( const SAIUnitCmd &command )
{
	if ( command.nCmdType == ACTION_COMMAND_MOVE_TO || command.nCmdType == ACTION_COMMAND_REVERSE_TO ||
		command.nCmdType == ACTION_COMMAND_SWARM_TO ||
		command.nCmdType == ACTION_COMMAND_PLACEMINE || command.nCmdType == ACTION_COMMAND_CLEARMINE || 
		command.nCmdType == ACTION_COMMAND_DEPLOY_ARTILLERY || command.nCmdType == ACTION_COMMAND_UNLOAD ||
		command.nCmdType == ACTION_COMMAND_RESUPPLY_HR || command.nCmdType == ACTION_COMMAND_RESUPPLY ||
		command.nCmdType == ACTION_COMMAND_REPAIR )
		return new CGroupMover( command.vPos );
	return 0;
}

REGISTER_SAVELOAD_CLASS( 0x31227B40, CGroupMover );
