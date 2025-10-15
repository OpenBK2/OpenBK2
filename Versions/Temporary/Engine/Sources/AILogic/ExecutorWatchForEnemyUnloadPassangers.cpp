#include "stdafx.h"
#include "./executorwatchforenemyunloadpassangers.h"
#include "GroupLogic.h"
#include "Technics.h"
#include "UnitsIterators2.h"
#include "Soldier.h"
#include "Formation.h"
#include "Artillery.h"
#include "UnitStates.h"

#include "AILogic_export.h"
#include <map>

extern CGroupLogic theGroupLogic;

REGISTER_SAVELOAD_CLASS( AILOGIC, 0x111AE380, CExecutorWatchForEnemyUnloadPassangers )

int CExecutorWatchForEnemyUnloadPassangers::Segment()
{
	if ( IsExecutorValidInternal() )
	{
		if ( !pUnit->GetNPassengers() )
		{
			RecordRandomCall();
			return GetNextTime() + NRandom::Random( 20 );
		}
		else // scan for enemies
		{
			const int nParty = pUnit->GetParty();
			for ( CUnitsIter<1,3> iter( nParty, EDI_ENEMY, pUnit->GetCenterPlain(), pUnit->GetSightRadius() ); !iter.IsFinished(); iter.Iterate() )
			{
				CAIUnit * pEnemy = *iter;
				if ( pEnemy->IsRefValid() && pEnemy->IsAlive() && pEnemy->IsVisible( nParty ) && 
						 !pEnemy->GetStats()->IsAviation() && pEnemy->IsFree() &&
							pEnemy->GetState()->GetName() != EUSN_PARTROOP )
				{
					// unload passengers

					std::map<int, bool> formations;	// use regular std::map for beter determinism
					for ( int i = 0; i < pUnit->GetNPassengers(); ++i )
					{
						CSoldier *pPass = pUnit->GetPassenger( i );
						if ( !pPass )
							continue;
						formations[pPass->GetFormation()->GetUniqueId()] = true;
					}
					int nTowedGunCrewID = -1;
					if ( pUnit->GetTowedArtillery() && pUnit->GetTowedArtillery()->GetCrew() )
						nTowedGunCrewID = pUnit->GetTowedArtillery()->GetCrew()->GetUniqueId();
					for ( std::map<int, bool>::iterator it = formations.begin(); it != formations.end(); ++it )
					{
						if ( it->first != nTowedGunCrewID )
						{
							SAIUnitCmd cmd( ACTION_COMMAND_UNLOAD, it->first );
							cmd.vPos = pUnit->GetCenterPlain();
							cmd.fNumber = float( int( ALP_POSITION_VALID ) );
							theGroupLogic.InsertUnitCommand( cmd, pUnit );
						}
					}
					RecordRandomCall();
					return GetNextTime() + NRandom::Random( 20 );
				}
			}
		}
	}
	else
		return -1;

	RecordRandomCall();
	return GetNextTime() + NRandom::Random( 50 );
}

bool CExecutorWatchForEnemyUnloadPassangers::IsExecutorValidInternal() const
{
	return IsValidObj( pUnit );
}

