#include "stdafx.h"
#include "BalanceTest.h"

#include "UnitsIterators.h"
#include "AIUnit.h"
#include "GroupLogic.h"
#include "Cheats.h"
#include "Main/GameTimer.h"
#include "System/Text.h"
#include "System/Time.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "Misc/StrProc.h"
#include "B2AI.h"

#include <cstdint>

#include <fmt/format.h>

extern NTimer::STime curTime;
extern CGroupLogic theGroupLogic;
extern SCheats theCheats;

namespace NReinforcement
{
	void PlaceReinforcement( EReinforcementType eType, const int nPlayer, const std::vector<NDb::SReinforcementEntry> &entries,
		const std::vector<NDb::SDeployTemplate::SDeployTemplateEntry> &pos, const CVec2 &vPosition, uint16_t wDirection,
		std::list< std::pair<int, CObjectBase*> > *pObjects, const int nForceID, const int nScriptID, const bool bDisableUpdates );
}

void CBalanceTest::AllignSizes( std::string *szTitle, std::string *szSide0, std::string *szSide1, int nAdd )
{
	const int nMaxSize = (std::max)( szTitle->size(), (std::max)( szSide0->size(), szSide1->size() ) ) + nAdd;
	szTitle->resize( nMaxSize, ' ' );
	szSide0->resize( nMaxSize, ' ' );
	szSide1->resize( nMaxSize, ' ' );
}

void CBalanceTest::PrintBalanceTestData()
{
	std::string szResultFile = "balance_test.txt";
	FILE *pFile = fopen( szResultFile.c_str(), "w+" );
	if ( pFile == 0 )
	{
		csSystem << "Can't open " << szResultFile << endl;
		return;
	}

	std::string szTitle;
	std::string szSide[2];

	szTitle += "| Name";
	szSide[0] += "| ";
	szSide[1] += "| ";

	for ( int nPlayer = 0; nPlayer < 2; ++nPlayer )
	{
		bool bIsMech = NGlobal::GetVar( fmt::format( "balance_params_is_mech_{}", nPlayer ), true );
		const std::string szDBID = NStr::ToMBCS( NGlobal::GetVar( fmt::format( "balance_params_unit_id_{}", nPlayer ), "" ).GetString() );
		const std::wstring wszName = bIsMech ?
			NText::GetText( NDb::Get<SMechUnitRPGStats>(szDBID)->szLocalizedNameFileRef ) :
			NText::GetText( NDb::Get<SSquadRPGStats>(szDBID)->szLocalizedNameFileRef );
		if ( !wszName.empty() )
			szSide[nPlayer] += NStr::ToMBCS( wszName );
		else
			szSide[nPlayer] += "error, no name";
	}
	AllignSizes( &szTitle, &szSide[0], &szSide[1], 1 );

	szTitle += "| ID ";
	szSide[0]  += fmt::format( "| {}", int(NGlobal::GetVar( "balance_params_unit_id_0", 0 )) );
	szSide[1] += fmt::format( "| {}", int(NGlobal::GetVar( "balance_params_unit_id_1", 0 )) );
	AllignSizes( &szTitle, &szSide[0], &szSide[1], 1 );

	szTitle += "| Qty";
	szSide[0]  += fmt::format( "| {}", int(NGlobal::GetVar( "balance_params_quantity_0", 0 )) );
	szSide[1] += fmt::format( "| {}", int(NGlobal::GetVar( "balance_params_quantity_1", 0 )) );
	AllignSizes( &szTitle, &szSide[0], &szSide[1], 1 );

	szTitle += "| Swarm";
	szSide[0] += NGlobal::GetVar( "balance_params_is_moving_0", 0 ) ? "| Yes" : "";
	szSide[1] += NGlobal::GetVar( "balance_params_is_moving_1", 0 ) ? "| Yes" : "";
	AllignSizes( &szTitle, &szSide[0], &szSide[1], 1 );

	szTitle += "| Shoot";
	szSide[0] += shoot[0] ? "| Yes" : "| No";
	szSide[1] += shoot[1] ? "| Yes" : "| No";
	AllignSizes( &szTitle, &szSide[0], &szSide[1], 1 );

	std::vector<int> nVictories( 2, 0 );
	std::vector<int> nDamagedTotal( 2, 0	);
	std::vector<int> nFullHealthTotal( 2, 0 );
	std::vector<int> nDeadTotal( 2, 0 );


	for ( int nIteration = 0; nIteration < NGlobal::GetVar( "balance_test_n_iteration", 0 ); ++nIteration )
	{
		szTitle += fmt::format( "| #{}", nIteration );

		std::vector<bool> bCanBeVictory( 2, false );

		for ( int nPlayer = 0; nPlayer < 2; ++nPlayer )
		{
			const int nDamaged = NGlobal::GetVar( fmt::format( "balance_test_iter_{}_player_{}_damaged", nIteration, nPlayer ) );
			const int nFullHealth = NGlobal::GetVar( fmt::format( "balance_test_iter_{}_player_{}_full_health", nIteration, nPlayer ) );
			const int nTotal = NGlobal::GetVar( fmt::format( "balance_test_iter_{}_player_{}_total", nIteration, nPlayer ) );
			nDamagedTotal[nPlayer] += nDamaged;
			nFullHealthTotal[nPlayer] += nFullHealth;
			nDeadTotal[nPlayer] += nTotal - nDamaged - nFullHealth;

			const bool bSomeAreAlive = nDamaged + nFullHealth != 0;
			bCanBeVictory[nPlayer] = bSomeAreAlive;
			szSide[nPlayer] += bSomeAreAlive ?  fmt::format( "| {} - {} - {}", nTotal - nDamaged - nFullHealth, nDamaged, nFullHealth ) :
			"| All dead";
		}
		if ( bCanBeVictory[0] && !bCanBeVictory[1] )
			++nVictories[0];
		if ( !bCanBeVictory[0] && bCanBeVictory[1] )
			++nVictories[1];
		AllignSizes( &szTitle, &szSide[0], &szSide[1], 1 );
	}
	szTitle += "| Score";
	szSide[0] += "|";
	szSide[1] += "|";

	szSide[0] += fmt::format( " {} ", nVictories[0] );
	szSide[1] += fmt::format( " {} ", nVictories[1] );
	AllignSizes( &szTitle, &szSide[0], &szSide[1], 1 );

	szTitle += "| Percentage";
	szSide[0] += "|";
	szSide[1] += "|";

	for ( int nPlayer = 0; nPlayer < 2; ++nPlayer )
	{
		int nTotal = nDeadTotal[nPlayer] + nDamagedTotal[nPlayer] + nFullHealthTotal[nPlayer];
		nDeadTotal[nPlayer] = 100 * nDeadTotal[nPlayer] / nTotal;
		nDamagedTotal[nPlayer] = 100 * nDamagedTotal[nPlayer] / nTotal;
		nFullHealthTotal[nPlayer] = 100 * nFullHealthTotal[nPlayer] / nTotal;
	}
	szSide[0] += fmt::format( " {} - {} - {}", nDeadTotal[0], nDamagedTotal[0], nFullHealthTotal[0] );
	szSide[1] += fmt::format( " {} - {} - {}", nDeadTotal[1], nDamagedTotal[1], nFullHealthTotal[1] );
	AllignSizes( &szTitle, &szSide[0], &szSide[1], 1 );

	szTitle += "|";
	szSide[0] += "|";
	szSide[1] += "|";

	std::string szSeparator;
	szSeparator.resize( szTitle.size(), '-' );
	fprintf( pFile, "\n" );
	fprintf( pFile, szSeparator.c_str() );
	fprintf( pFile, "\n" );
	fprintf( pFile, szTitle.c_str() );
	fprintf( pFile, "\n" );
	fprintf( pFile, szSeparator.c_str() );
	fprintf( pFile, "\n" );
	fprintf( pFile, szSide[0].c_str() );
	fprintf( pFile, "\n" );
	fprintf( pFile, szSeparator.c_str() );
	fprintf( pFile, "\n" );
	fprintf( pFile, szSide[1].c_str() );
	fprintf( pFile, "\n" );
	fprintf( pFile, szSeparator.c_str() );
	fprintf( pFile, "\n" );

	fclose( pFile );
}

void CBalanceTest::UnitDead( CAIUnit *pUnit )
{
	const int nPlayer = pUnit->GetPlayer();
	if ( !shoot[nPlayer] )
	{
		for ( int i = 0; i < pUnit->GetNCommonGuns(); ++i )
		{
			const SBaseGunRPGStats &rStats = pUnit->GetCommonGunStats( i );
			if ( rStats.nAmmo != pUnit->GetNAmmo( i ) )
			{
				shoot[nPlayer] = true;
				break;
			}
		}
	}
}

void CBalanceTest::CollectBalanceTestData( int nIteration )
{
	for ( int nPlayer = 0; nPlayer < 2; ++nPlayer )
	{
		bool bIsMech = NGlobal::GetVar( fmt::format( "balance_params_is_mech_{}", nPlayer ), true );
		int nFullHealth = 0;
		int nDamaged = 0;

		for ( CGlobalIter iter( 0, ANY_PARTY ); !iter.IsFinished(); iter.Iterate() )
		{
			if ( IsValidObj( (*iter) ) )
			{
				CAIUnit *pUnit = (*iter);
				if ( pUnit->GetPlayer() == nPlayer && ( bIsMech ? !pUnit->IsInfantry() : true ))
				{
					if ( !shoot[nPlayer] )
					{
						for ( int i = 0; i < pUnit->GetNCommonGuns(); ++i )
						{
							const SBaseGunRPGStats &rStats = pUnit->GetCommonGunStats( i );
							if ( rStats.nAmmo != pUnit->GetNAmmo( i ) )
							{
								shoot[nPlayer] = true;
								break;
							}
						}
					}
					if ( pUnit->GetHitPoints() != pUnit->GetStats()->fMaxHP )
						++nDamaged;
					else
						++nFullHealth;
				}
			}
		}
		NGlobal::SetVar( fmt::format( "balance_test_iter_{}_player_{}_damaged", nIteration, nPlayer ), nDamaged );
		NGlobal::SetVar( fmt::format( "balance_test_iter_{}_player_{}_full_health", nIteration, nPlayer ), nFullHealth );
	}
}

void CBalanceTest::InitBalanceTest( const NDb::SMapInfo *pMapInfo )
{
	bTest = NGlobal::GetVar( "balance_test", 0 );
	if ( !bTest ) 
		return;
	NGlobal::SetVar( "temp.nogeneral_sript", true );

	if ( NGlobal::GetVar( "balance_test_no_visual", 1 ) )
		Singleton<IGameTimer>()->SetSpeed( 10000 );

	shoot.clear();
	shoot.resize( 3, false );

	pBalanceMapInfo = pMapInfo;
	timeBalanceStart = curTime;
	int nLinkIndex = 1;

	theCheats.SetTurnOffWarFog( NGlobal::GetVar( "balance_params_warfog", 0 ) );
	std::vector<CVec2> vEnemyPoint( 2 );

	NI_ASSERT( pMapInfo->players.size() >= 3, "MAP error: need 2 players and neutral" );
	for ( int nPlayer = 0; nPlayer < 2; ++nPlayer )
	{
		int nPoint = NGlobal::GetVar( fmt::format( "balance_params_point_{}", nPlayer ), 0 );
		NI_ASSERT( !pMapInfo->players[nPlayer].reinforcementPoints.empty(), fmt::format( "MAP ERROR: no reinforcement point for player {}", nPlayer ) );
		vEnemyPoint[1-nPlayer] = pMapInfo->players[nPlayer].reinforcementPoints[nPoint].vPosition;
	}

	for ( int nPlayer = 0; nPlayer < 2; ++nPlayer )
	{
		bool bIsMech = NGlobal::GetVar( fmt::format( "balance_params_is_mech_{}", nPlayer ), true );
		int nPoint = NGlobal::GetVar( fmt::format( "balance_params_point_{}", nPlayer ), 0 );
		const std::string szUnit = NStr::ToMBCS( NGlobal::GetVar( fmt::format( "balance_params_unit_id_{}", nPlayer ), "" ).GetString() );
		int nQuantity = NGlobal::GetVar( fmt::format( "balance_params_quantity_{}", nPlayer ), 0 );
		bool bSwarm = NGlobal::GetVar( fmt::format( "balance_params_is_moving_{}", nPlayer ), 0 );

		std::vector<NDb::SReinforcementEntry> entries;
		entries.resize( nQuantity );

		const NDb::SSquadRPGStats *pSquad = bIsMech ? 0 : NDb::Get<NDb::SSquadRPGStats>( szUnit );
		const NDb::SMechUnitRPGStats *pMech = bIsMech ? NDb::Get<NDb::SMechUnitRPGStats>( szUnit ) : 0 ;

		for ( int i = 0; i < nQuantity; ++i )
		{
			entries[i].nLinkIndex = ++nLinkIndex;
			entries[i].nLinkWith = -1;
			entries[i].pMechUnit = pMech;
			entries[i].pSquad = pSquad;
		}

		std::list<std::pair<int, CObjectBase*> > objects;

		NReinforcement::PlaceReinforcement( _RT_NONE, nPlayer, entries,
			pMapInfo->players[nPlayer].reinforcementPoints[nPoint].pTemplate->entries,
			pMapInfo->players[nPlayer].reinforcementPoints[nPoint].vPosition,
			pMapInfo->players[nPlayer].reinforcementPoints[nPoint].nDirection,
			&objects, -1, -1, false );

		std::vector<int> ids;
		for ( std::list< std::pair<int, CObjectBase*> >::iterator it = objects.begin(); it != objects.end(); ++it )
		{
			CCommonUnit * pUnit = checked_cast<CCommonUnit*>( it->second );
			if ( pUnit->IsEmptyCmdQueue() )
				ids.push_back( checked_cast<CLinkObject*>( it->second )->GetUniqueId() );
		}
		const int nGroup = theGroupLogic.GenerateGroupNumber();
		theGroupLogic.RegisterGroup( ids, nGroup );

		SAIUnitCmd cmd;
		cmd.nCmdType = ACTION_COMMAND_SWARM_TO;
		cmd.vPos = vEnemyPoint[nPlayer];
		theGroupLogic.GroupCommand( cmd, nGroup, false );
		theGroupLogic.UnregisterGroup( nGroup );
		Singleton<IAILogic>()->SetNeedNewGroupNumber();

		int nTotal = 0;
		int nIteration = NGlobal::GetVar( "balance_test_n_iteration", 0 );
		for ( CGlobalIter iter( 0, ANY_PARTY ); !iter.IsFinished(); iter.Iterate() )
		{
			if ( IsValidObj( (*iter) ) )
			{
				CAIUnit *pUnit = (*iter);
				if ( pUnit->GetPlayer() == nPlayer && (bIsMech ? !pUnit->IsInfantry() : true ) )
					++nTotal;
			}
		}
		NGlobal::SetVar( fmt::format( "balance_test_iter_{}_player_{}_total", nIteration, nPlayer ), nTotal );
	}
}

void CBalanceTest::SegmentBalanceTest()
{
	if ( !bTest ) 
		return;

	const int nTestTime = NGlobal::GetVar( "balance_params_iteration_time", 60 );

	bool bFinishIteration = false;
	// check if both sides are alive
	std::vector<bool> bAlive( 3, false );
	for ( CGlobalIter iter( 0, ANY_PARTY ); !iter.IsFinished(); iter.Iterate() )
	{
		if ( IsValidObj( (*iter) ) )
			bAlive[(*iter)->GetParty()] = true;
	}
	bFinishIteration = !bAlive[0] || !bAlive[1];

	// check time
	bFinishIteration |= nTestTime * 1000 <= curTime - timeBalanceStart;

	if ( bFinishIteration )
	{
		// collect data
		int nIteration = NGlobal::GetVar( "balance_test_n_iteration", 0 );
		CollectBalanceTestData( nIteration );

		++nIteration;
		NGlobal::SetVar( "balance_test_n_iteration", nIteration );

		if ( nIteration < NGlobal::GetVar( "balance_params_iteration_number", 1 ) )
		{
			for ( CGlobalIter iter( 0, ANY_PARTY ); !iter.IsFinished(); iter.Iterate() )
			{
				if ( IsValidObj( (*iter) ) )
					(*iter)->Disappear();
			}
			// launch map again
			InitBalanceTest( pBalanceMapInfo );
		}
		else
		{
			// print data and exit game
			PrintBalanceTestData();
			WriteToPipe( PIPE_CONSOLE_CMDS, "exit" );
			bTest = false;
		}
	}
}

const NDb::SUnitStatsModifier * CBalanceTest::GetModifier( int nPlayer ) const
{
	if ( !bTest )
		return 0;

	const std::string szDBID = NStr::ToMBCS( NGlobal::GetVar( nPlayer == 0 ? "balance_params_stats_modifier_0" : "balance_params_stats_modifier_1", "" ).GetString() );
	if ( szDBID.empty() )
		return NDb::Get<NDb::SUnitStatsModifier>( szDBID );
	return 0;
}

