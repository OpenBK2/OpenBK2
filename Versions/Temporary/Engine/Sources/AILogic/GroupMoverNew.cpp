#include "stdafx.h"
/*
#include "AIMap.h"
#include "GroupMover.h"
#include "CommonUnit.h"
#include "../Common_RTS_AI/AIMap.h"
#include "../Common_RTS_AI/StaticPath.h"
#include "../Common_RTS_AI/StaticPathInternal.h"
#include "../Common_RTS_AI/Terrain.h"
#include "../Stats_B2_M1/AIUnitCmd.h"
#include "../Stats_B2_M1/ActionCommand.h"
#include "../DebugTools/DebugInfoManager.h"

extern NTimer::STime curTime;

static float GRID_SIZE = 64.0f;
static float GROUP_DIST2 = sqr( 32.0f );

struct SPointInfo
{
	CVec2 vPoint;
	float fDist;
	SPointInfo() : vPoint( VNULL2 ), fDist( 0.0f ) {}
	SPointInfo( const CVec2 &_vPoint, const float _fDist ) : vPoint( _vPoint ), fDist( _fDist ) {}
};

void CGroupMover::SGroup::CalculateGroupInfo()
{

	if ( !IsValid() )
		return;

	if ( units.size() == 1 )
	{
		vCenter = units.front()->GetCenterPlain();
		vDirection = units.front()->GetFrontDirectionVector();
		fArea = GRID_SIZE * GRID_SIZE;
		return; 
	}

	// reset values
	vCenter = VNULL2;
	vDirection = VNULL2;
	fArea = 0.0f;

	// calculate center of group
	for ( vector<CPtr<CCommonUnit> >::const_iterator it = units.begin(); it != units.end(); ++it )
		vCenter += (*it)->GetCenterPlain();
	vCenter /= units.size();

	// search most distant point (we dont need unit)
	CVec2 vDirPerp;
	vector<SPointInfo> points( 4 );
	{
		vector<CPtr<CCommonUnit> >::const_iterator it = units.begin();

		points[0].vPoint = (*it)->GetCenterPlain();
		points[0].fDist = fabs2( points[0].vPoint - vCenter );
		for ( ++it; it != units.end(); ++it )
		{
			const CVec2 vThisPosition = (*it)->GetCenterPlain();
			const float fThisDist2 = fabs2( vThisPosition - vCenter );
			if ( fThisDist2 > points[0].fDist )
			{
				points[0].vPoint = vThisPosition;
				points[0].fDist = fThisDist2;
			}
		}
		vDirection = points[0].vPoint - vCenter;
		points[0].fDist = fabs( vDirection );
		vDirection /= points[0].fDist;
		vDirPerp.x = vDirection.y;
		vDirPerp.y = -vDirection.x;
	}

	// search remaining three units for building rectangle
	{
		vector<CPtr<CCommonUnit> >::const_iterator it = units.begin();
		const CVec2 vThisPosition = (*it)->GetCenterPlain();
		for ( int i = 1; i < 4; ++i )
			points[i].vPoint = vThisPosition;
		const CVec2 vThisDirection = (*it)->GetCenterPlain() - vCenter;
		points[1].fDist = vThisDirection * vDirection;	// search min
		points[2].fDist = vThisDirection * vDirPerp;	// search min
		points[3].fDist = vThisDirection * vDirPerp;	// search max
		for ( ++it; it != units.end(); ++it )
		{
			const CVec2 vThisPosition = (*it)->GetCenterPlain();
			const CVec2 vThisDirection = vThisPosition - vCenter;
			const float fThisProj = vThisDirection * vDirection;
			const float fThisPerpProj = vThisDirection * vDirPerp;
			if ( fThisProj < points[1].fDist )
				points[1].fDist = fThisProj, points[1].vPoint = vThisPosition;
			if ( fThisPerpProj < points[2].fDist )
				points[2].fDist = fThisProj, points[2].vPoint = vThisPosition;
			if ( fThisPerpProj > points[3].fDist )
				points[3].fDist = fThisProj, points[3].vPoint = vThisPosition;
		}
	}

	// calculate area
	fArea = ( points[0].fDist - points[1].fDist + GRID_SIZE ) * ( points[3].fDist - points[2].fDist + GRID_SIZE );
}

const bool CGroupMover::SGroup::operator>( const SGroup &group )
{
	NI_VERIFY( IsValid() && !group.IsValid(), "Cannot compare empty groups", return false );
	if ( fArea == group.fArea )
		return units.front()->GetUniqueID() > group.units.front()->GetUniqueID();
	return fArea > group.fArea;
}

const bool CGroupMover::SGroup::operator<( const SGroup &group )
{
	NI_VERIFY( IsValid() && !group.IsValid(), "Cannot compare empty groups", return false );
	if ( fArea == group.fArea )
		return units.front()->GetUniqueID() < group.units.front()->GetUniqueID();
	return fArea < group.fArea;
}

IStaticPath *CGroupMover::SSubGroupPathInfo::CreatePathForUnit( CCommonUnit *pUnit, const CVec2 &vWantedPosition )
{
	if ( pPath && fabs2( vStartPoint - pUnit->GetCenterPlain() ) < GROUP_DIST2 &&
		fabs2( pPath->GetFinishPoint() - vWantedPosition ) < GROUP_DIST2 && curTime - timeCalced < 500 )
	{
		pUnit->SetGroupShift( vWantedPosition - pPath->GetFinishPoint() );
		return pPath;
	}
	timeCalced = curTime;
	vStartPoint = pUnit->GetCenterPlain();
	pPath = pUnit->CreateBigStaticPath( pUnit->GetCenterPlain(), vWantedPosition, 0 );
	pUnit->SetGroupShift( VNULL2 );
	return pPath;
}

typedef hash_set<CPtr<CCommonUnit>, SPtrHash > TUnitsHashSet;

const int CGroupMover::BuildGroups( vector<SGroup> &groups )
{
	TUnitsHashSet unsortedUnits;
	for ( TUnits::const_iterator it = units.begin(); it != units.end(); ++it )
	{
		it->second.pUnit->SetSubGroup( -1 );
		unsortedUnits.insert( it->second.pUnit );
	}

	int nCurrentGroup = 0;
	TUnitsHashSet unitsToCheck;
	while( !unsortedUnits.empty() || !unitsToCheck.empty() )
	{
		if ( unitsToCheck.empty() )
		{
			++nCurrentGroup;
			TUnitsHashSet::iterator pos = unsortedUnits.begin();
			unitsToCheck.insert( *pos );
			pos->SetSubGroup( nCurrentGroup );
			unsortedUnits.remove( pos );
		}
		else
		{
			TUnitsHashSet::iterator pos = unitsToCheck.begin();
			const CVec2 vCenter = pos->GetCenterPlain();
			unitsToCheck.remove( pos );
			for( TUnitsHashSet::iterator it = unsortedUnits.begin(); it != unsortedUnits.end(); )
			{
				if ( fabs2( (*it)->GetCenterPlain() - vCenter ) < GROUP_DIST2 )
				{
					(*it)->SetSubGroup( nCurrentGroup );
					unitsToCheck.insert( *it );
					unsortedUnits.remove( it++ );
				} 
			}
		}
	}

	groups.clear();
	groups.resize( nCurrentGroup );
	for ( TUnits::const_iterator it = units.begin(); it != units.end(); ++it )
		groups[it->second.pUnit->GetSubGroup() - 1].AddUnit( it->second.pUnit );

	return nCurrentGroup;
}

const bool CGroupMover::CalcPositions()
{
	if ( !bNeedCalcPositions )
		return true;

	return true;
}

void CGroupMover::AddUnit( CCommonUnit *pUnit )
{
	NI_ASSERT( bNeedCalcPositions, "Cannot add unit. Positions already builded." );
	units[pUnit->GetUniqueID()] = pUnit;
}

void CGroupMover::DeleteUnit( const int nUnitID )
{
	TUnits::iterator posGroup = units.find( nUnitID );
	if ( posGroup == units.end() )
		return;

	units.erase( posGroup );
}

IStaticPath* CGroupMover::CreateStaticPath( CCommonUnit *pUnit )
{
	if ( !CalcPositions() )
		return 0;

	return subGroupsPaths[pUnit->GetSubGroup()].CreatePathForUnit( pUnit, units[pUnit->GetUniqueID()].vFinistPoint );
}

CGroupMover *CreateGroupMover( const SAIUnitCmd &command )
{
	if ( command.nCmdType == ACTION_COMMAND_MOVE_TO || command.nCmdType == ACTION_COMMAND_SWARM_TO || 
		command.nCmdType == ACTION_COMMAND_PLACEMINE || command.nCmdType == ACTION_COMMAND_CLEARMINE || 
		command.nCmdType == ACTION_COMMAND_DEPLOY_ARTILLERY || command.nCmdType == ACTION_COMMAND_UNLOAD ||
		command.nCmdType == ACTION_COMMAND_RESUPPLY_HR || command.nCmdType == ACTION_COMMAND_RESUPPLY ||
		command.nCmdType == ACTION_COMMAND_REPAIR )
		return new CGroupMover( command.vPos );
	return 0;
}

REGISTER_SAVELOAD_CLASS( 0x31227B40, CGroupMover );
*/