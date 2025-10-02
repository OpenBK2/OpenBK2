#include "StdAfx.h"
#include ".\executorwatchforenemyunloadpassangers.h"
#include "GroupLogic.h"
#include "Technics.h"
#include "UnitsIterators2.h"
#include "Soldier.h"
#include "Formation.h"
#include "Artillery.h"
#include "UnitStates.h"
extern CGroupLogic theGroupLogic;

REGISTER_SAVELOAD_CLASS( 0x111AE380, CExecutorWatchForEnemyUnloadPassangers )

int CExecutorWatchForEnemyUnloadPassangers::Segment()
{
	if ( IsExecutorValidInternal() )
	{
		if ( !pUnit->GetNPassengers() )
		{
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

					hash_map<int, bool> formations;
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
					for ( hash_map<int, bool>::iterator it = formations.begin(); it != formations.end(); ++it )
					{
						if ( it->first != nTowedGunCrewID )
						{
							SAIUnitCmd cmd( ACTION_COMMAND_UNLOAD, it->first );
							cmd.vPos = pUnit->GetCenterPlain();
							cmd.fNumber = float( int( ALP_POSITION_VALID ) );
							theGroupLogic.InsertUnitCommand( cmd, pUnit );
						}
					}
					return GetNextTime() + NRandom::Random( 20 );
				}
			}
		}
	}
	else
		return -1;

	return GetNextTime() + NRandom::Random( 50 );
}

bool CExecutorWatchForEnemyUnloadPassangers::IsExecutorValidInternal() const
{
	return IsValidObj( pUnit );
}

