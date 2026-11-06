#include "stdafx.h"

#include "Diplomacy.h"
#include "Formation.h"
#include "GeneralAirForce.h"
#include "GeneralInternal.h"
#include "PlayerReinforcement.h"
#include "Reinforcement.h"
#include "Soldier.h"

#include <cmath>

#include <fmt/format.h>

//
// Contains CGeneral functionality related to calling in reinforcements
//

#define AIR_REINF_TURN_LIMIT 2

extern NTimer::STime curTime;
extern CDiplomacy theDipl;
extern CPlayerReinforcementArray theReinfArray;

void CGeneral::InitRearManager()
{
	enemyByRType.resize( NDb::_RT_NONE, 0.0f );
}

static const bool IsAviationReinfType( const int /*NDb::EReinforcementType*/ eRType )
{
	return ( eRType == NDb::RT_BOMBERS || eRType == NDb::RT_GROUND_ATTACK_PLANES || eRType == NDb::RT_RECON ||
	         eRType == NDb::RT_FIGHTERS || eRType == NDb::RT_EXTRA_AIR_1 || eRType == NDb::RT_EXTRA_AIR_2 ||
	         eRType == NDb::RT_EXTRA_AIR_3 || eRType == NDb::RT_EXTRA_AIR_4 || eRType == NDb::RT_EXTRA_AIR_5 ||
	         eRType == NDb::RT_EXTRA_MAXLVL_AIR_1 || eRType == NDb::RT_EXTRA_MAXLVL_AIR_2 ||
	         eRType == NDb::RT_EXTRA_MAXLVL_AIR_3 || eRType == NDb::RT_EXTRA_MAXLVL_AIR_4 ||
	         eRType == NDb::RT_EXTRA_MAXLVL_AIR_5 || eRType == NDb::RT_EXTRA_MAXLVL_MIXED_1 ||
	         eRType == NDb::RT_EXTRA_MAXLVL_MIXED_2 || eRType == NDb::RT_EXTRA_MAXLVL_MIXED_3 ||
	         eRType == NDb::RT_EXTRA_MAXLVL_MIXED_4 || eRType == NDb::RT_EXTRA_MAXLVL_MIXED_5 );
}

void CGeneral::CheckAvailableReinforcement()
{
	// Reinforcements belong to map players, not their shared diplomacy side.
	const int nPlayer = GetPlayer();
	if ( nPlayer < 0 || nPlayer >= theReinfArray.size() )
	{
		return;
	}
	int nReinfTypeNeeded, nReinfTypeToCall = -2;
	float fEffect, fMaxEffect = enemyByRType[0];
	bool bReinfFound = false;

	// there is no ground reinforcements
	if ( !theReinfArray[nPlayer].HasGroundReinforcements() )
	{
		pAirForce->PassTurn();
		return;
	}

	if ( !theReinfArray[nPlayer].CanCallNow() )
	{
		return;
	}

	if ( !pAirForce->TurnReturned() )
	{
		return;
	}

	// Pass some of the calls to airforce general
	++nAirReinfTurnCounter;
	nAirReinfTurnCounter %= AIR_REINF_TURN_LIMIT;
	if ( !nAirReinfTurnCounter )
	{
		pAirForce->PassTurn();
		//CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW, StrFmt( "Passing turn to airforce" ) );
		return;
	}

	// Select the most needed category (max value)
	nReinfTypeNeeded = 0;
	for ( int i = 0; i < NDb::_RT_NONE; ++i )
	{
		if ( enemyByRType[i] > fMaxEffect && !IsAviationReinfType( i ) )
		{
			fMaxEffect = enemyByRType[i];
			nReinfTypeNeeded = i;
		}
	}

	// if time has come, select one and call
	fMaxEffect = -2.0f; //Since effectiveness is [-1 .. 1]
	int nAvailReinfs[NDb::_RT_NONE];
	float fReinfEff[NDb::_RT_NONE];
	float fSum = 0.0f;
	int nARQuantity = 0;
	for ( int i = 0; i < NDb::_RT_NONE; ++i )
	{
		if ( !theReinfArray[nPlayer].HasReinforcement( NDb::EReinforcementType( i ) ) || IsAviationReinfType( i ) )
		{
			continue;
		}

		// calculate potential effect
		fEffect = 0;
		for ( int j = 0; j < NDb::_RT_NONE; ++j )
		{
			// Air categories can outweigh the selected ground category. Use distance
			// between needs so their weight stays finite and decreases with that distance.
			const float fNeedDistance = 1.0f + std::fabs( enemyByRType[nReinfTypeNeeded] - enemyByRType[j] );
			fEffect +=
					NReinforcement::GetReinforcementExpediency( NDb::EReinforcementType( i ), NDb::EReinforcementType( j ) ) /
					fNeedDistance / fNeedDistance * ( ( j == nReinfTypeNeeded ) ? 2 : 1 );
		}
		if ( fEffect > fMaxEffect )
		{
			nReinfTypeToCall = i;
			fMaxEffect = fEffect;
			bReinfFound = true;
		}
		fSum += fEffect;
		nAvailReinfs[nARQuantity] = i;
		fReinfEff[nARQuantity] = fSum;
		++nARQuantity;
	}
	if ( bReinfFound ) // if there was a reinforcement available, call it
	{
		//theReinfArray[nPlayer].CallReinforcement( NDb::EReinforcementType( nReinfTypeToCall ),	theReinfArray[nPlayer].GetRandomPoint(), -1 );
		//CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW, StrFmt( "Calling reinforcement %d", nReinfTypeToCall ) );
		float fRandValue = NRandom::Random( 0.0f, fSum );
		RecordRandomCall();
		const int i = NReinforcement::SelectByCumulativeWeight( fReinfEff, nARQuantity, fRandValue );
		// check if this reinforcement will increase mobile tanks nuber over the maximum allowed number
		// don't call if it will
		int nWorkers = theReinfArray[nPlayer].GetNEntries( NDb::EReinforcementType( nAvailReinfs[i] ) );
		for ( Tasks::iterator it = tasks.begin(); it != tasks.end(); ++it )
		{
			nWorkers += ( *it )->GetWorkerCount();
		}
		nWorkers += tanksFree.size();

		if ( nWorkers <= nMaxAllowedMobileTanks )
		{
			theReinfArray[nPlayer].CallReinforcement(
					NDb::EReinforcementType( nAvailReinfs[i] ), theReinfArray[nPlayer].GetRandomPoint(), -1 );
		}
	}
}

void CGeneral::BalanceUpdate( EBalanceAction eAction, CCommonUnit *_pUnit )
{
	const bool bRemoving = eAction == BA_REMOVE_OWN || eAction == BA_REMOVE_ENEMY;
	// Death notifications still have valid stats and ownership after IsAlive becomes false.
	if ( !_pUnit || !_pUnit->IsRefValid() || ( !bRemoving && !_pUnit->IsAlive() ) )
	{
		return;
	}

	if ( _pUnit->IsFormation() ) // Filter out formations, and count soldiers in there
	{
		CFormation *pForm = checked_cast<CFormation *>( _pUnit );

		for ( int i = 0; i < pForm->Size(); ++i )
		{
			BalanceUpdate( eAction, ( *pForm )[i] ); //Self-call!
		}

		return;
	}

	CAIUnit *pUnit = dynamic_cast<CAIUnit *>( _pUnit );
	NI_VERIFY( pUnit, "Invalid unit (not AIUnit)", return );
	const int nUnitPlayer = pUnit->GetPlayer();
	if ( nUnitPlayer < 0 || nUnitPlayer >= theReinfArray.size() )
	{
		return;
	}
	if ( theDipl.IsNetGame() )
	{
		// Visibility changes and formation callbacks may report a soldier more than once.
		// Each registered unit contributes to the COOP balance exactly once until removal.
		det_set<int> &registered =
				( eAction == BA_ADD_OWN || eAction == BA_REMOVE_OWN ) ? balanceOwnUnits : balanceEnemyUnits;
		const bool bAdding = eAction == BA_ADD_OWN || eAction == BA_ADD_ENEMY;
		if ( bAdding ? !registered.insert( pUnit->GetUniqueId() ).second : !registered.erase( pUnit->GetUniqueId() ) )
		{
			return;
		}
	}
	const NDb::SUnitBaseRPGStats *pUnitStats = pUnit->GetStats();
	NDb::EDBUnitRPGType eUType = pUnitStats->eDBtype;
	NDb::EReinforcementType eRType = pUnit->GetReinforcementType();
	std::vector<float> effects( NDb::_RT_NONE, 0.0f );
	float fTypeMult = pUnitStats->fPrice; //Type multiplier, to compensate for infantry numbers and valuable units

	if ( pUnit->GetStats()->IsInfantry() )
	{
		if ( eUType != DB_RPG_TYPE_SNIPER )
		{
			fTypeMult *= 0.1f; // Soldier counts as 1/10 (all but snipers);
		}
	}

	int nAvail;
	int nAffected = 0;

	//Check validity of values, and reset to least harmful
	NI_VERIFY( eUType >= 0 && eUType < NDb::DB_RPG_TYPE_COUNT,
	           fmt::format( "Invalid unit type {}", int( eUType ) ),
	           eUType = NDb::DB_RPG_TYPE_SOLDIER );

	if ( eRType < 0 || eRType >= _RT_NONE )
	{
		eRType = NReinforcement::GetReinforcementType( eUType );
	}

	//Find out what reinfs the unit can be in
	for ( int i = 0; i < NDb::_RT_NONE; ++i )
	{
		effects[i] = 0.0;

		// Get availability for enemy
		nAvail = theReinfArray[nUnitPlayer].GetUnitAvailability( eUType, NDb::EReinforcementType( i ) );

		if ( nAvail )
		{
			effects[i] += 1.0f / nAvail;
			++nAffected;
		}
		else if ( i == eRType )
		{
			effects[eRType] += 1;
		}
	}

	//Depending
	for ( int i = 0; i < NDb::_RT_NONE; ++i )
	{
		switch ( eAction )
		{
			case BA_ADD_ENEMY:
				enemyByRType[i] += effects[i] * fTypeMult;
				break;
			case BA_REMOVE_ENEMY:
				enemyByRType[i] -= effects[i] * fTypeMult;
				break;
			case BA_ADD_OWN:
				for ( int j = 0; j < NDb::_RT_NONE; ++j )
				{
					enemyByRType[i] -=
							effects[j] * fTypeMult *
							NReinforcement::GetReinforcementExpediency( NDb::EReinforcementType( j ), NDb::EReinforcementType( i ) ) /
							2;
				}
				break;
			case BA_REMOVE_OWN:
				for ( int j = 0; j < NDb::_RT_NONE; ++j )
				{
					enemyByRType[i] +=
							effects[j] * fTypeMult *
							NReinforcement::GetReinforcementExpediency( NDb::EReinforcementType( j ), NDb::EReinforcementType( i ) ) /
							2;
				}
				break;
		}
	}
}
