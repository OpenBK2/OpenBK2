#include "stdafx.h"

#include "Diplomacy.h"
#include "GeneralConsts.h"
#include "GeneralIntendant.h"
#include "GeneralInternal.h"
#include "ScenarioTracker.h"
#include "SerializeOwner.h"

int CSupremeBeing::operator&( IBinSaver &saver )
{
	if ( !saver.IsChecksum() || ( GetScenarioTracker() && GetScenarioTracker()->IsCooperative() ) )
	{
		saver.Add( 1, &generals );
		saver.Add( 2, &ironmans );
		saver.Add( 3, &delayedTasks );
	}
	if ( saver.IsReading() )
	{
		// Map player order also defines RNG consumption order after a saved game is restored.
		std::vector<int> players;
		for ( const auto &entry : generals )
		{
			players.push_back( entry.first );
		}
		std::sort( players.begin(), players.end() );
		Generals ordered;
		for ( int nPlayer : players )
		{
			ordered[nPlayer] = generals[nPlayer];
		}
		generals.swap( ordered );
		SGeneralConsts::Init( theDipl.IsNetGame() );
	}
	return 0;
}

int CCommander::operator&( IBinSaver &saver )
{

	saver.Add( 1, &tasks );
	saver.Add( 2, &fMeanSeverity );
	return 0;
}

void CGeneral::OnSerialize( IBinSaver &saver )
{
	// Save the next enemy by stable unit ID, never the address of a container iterator.
	int nNextEnemy = -1;
	if ( !saver.IsReading() && curProcessed != enemys.end() )
	{
		nNextEnemy = curProcessed->first;
	}
	if ( !saver.IsReading() || saver.HasChunk( 25 ) )
	{
		saver.Add( 25, &nNextEnemy );
	}
	if ( saver.IsReading() )
	{
		curProcessed = enemys.find( nNextEnemy );
		erased.clear();

		if ( enemyByRType.size() == 0 )
		{
			enemyByRType.resize( NDb::_RT_NONE );
		}
		SGeneralConsts::Init( theDipl.IsNetGame() );
	}
}

void CGeneralTaskToResupplyCell::OnSerialize( IBinSaver &saver )
{
	SerializeOwner( 2, &pCells, &saver );
}
