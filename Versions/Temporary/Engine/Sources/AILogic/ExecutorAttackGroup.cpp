#include "StdAfx.h"
#include ".\executorattackgroup.h"

#include "GroupLogic.h"
#include "CommonUnit.h"
#include "ExecutorAttackGroupEvent.h"
#include "UnitStates.h"

extern CGroupLogic theGroupLogic;


int CExecutorAttackGroup::Segment()
{
	switch( eState )
	{
	case EAGS_BEGIN:
		
		return 20;
	case EAGS_APPROACH_RADIUS:
		{
			bool bAllAtPlace = true;
			for ( CAttackGroupUnits::iterator it = unitsGoingToRadius.begin(); it != unitsGoingToRadius.end(); ++it )
			{
				CCommonUnit * pUnit = *it;
				if ( IsValidObj( pUnit ) && pUnit->GetState()->GetName() == EUSN_MOVE_TO_RESUPPLY_CELL )
					bAllAtPlace = false;
			}
			if ( bAllAtPlace )
			{
				for ( CAttackGroupUnits::iterator it = unitsGoingToRadius.begin(); it != unitsGoingToRadius.end(); ++it )
				{
					CCommonUnit * pUnit = *it;
					if ( IsValidObj( pUnit ) )
						theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_SWARM_TO, vPoint), pUnit, false );
				}
				eState = EAGS_SWARM;
			}
		}

		return 20;
	case EAGS_SWARM:

		return 20;
	case EAGS_END:			 

		return -1;
	}
	return -1;
}

void CExecutorAttackGroup::RegisterOnEvents( IExecutorContainer *pContainer )
{
	vector<EExecutorEventID> events;

	events.push_back( EID_ATTACKGROUP_ADD_UNIT );
	events.push_back( EID_ATTACKGROUP_ATTACK );
	events.push_back( EID_ATTACKGROUP_DELETE );
	SExecutorEventParam param;
	param.nExecutorID = GetID();
	for ( int i = 0; i < events.size(); ++i )
	{
		param.eEventID = events[i];
		pContainer->RegisterOnEvent( this, param );
	}

}

bool CExecutorAttackGroup::NotifyEvent( const CExecutorEvent &event )
{
	switch( event.GetParam().eEventID )
	{
	case EID_ATTACKGROUP_ADD_UNIT:
		{
			int nUnitID = static_cast<const CExecutorAttackGroupAddUnitEvent*>( &event )->GetUnitID();
			CDynamicCast<CCommonUnit> pUnit( CLinkObject::GetObjectByUniqueIdSafe( nUnitID ) );
			if ( pUnit )
				units.push_back( pUnit.GetPtr() );
		}

		return true;
	case EID_ATTACKGROUP_ATTACK:
		{
			vPoint = static_cast<const CExecutorEventAttackGroupAttackPoint*>( &event )->GetPoint();
			fRadius = static_cast<const CExecutorEventAttackGroupAttackPoint*>( &event )->GetRange();
			unitsGoingToRadius.splice( unitsGoingToRadius.begin(), units );
			//ToDo: launch all units to radius
			for ( CAttackGroupUnits::iterator it = unitsGoingToRadius.begin(); it != unitsGoingToRadius.end(); ++it )
				theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_TO_NOT_PRESIZE, vPoint, fRadius), *it, false );
			eState = EAGS_APPROACH_RADIUS;
		}

		return true;
	case EID_ATTACKGROUP_DELETE:
		for ( CAttackGroupUnits::iterator it = units.begin(); it != units.end(); ++it )
			theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_STOP), *it, false );
		for ( CAttackGroupUnits::iterator it = unitsGoingToRadius.begin(); it != unitsGoingToRadius.end(); ++it )
			theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_STOP), *it, false );

		eState = EAGS_END;

		return true;
	}
	return false;
}

bool CExecutorAttackGroup::IsExecutorValid() const
{
	return true;
}

