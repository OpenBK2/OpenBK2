#include "stdafx.h"

#include "GeneralHelper.h"
#include "GeneralInternal.h"
#include "UnitsIterators2.h"
#include "Formation.h"
#include "GeneralTasks.h"
#include "UnitStates.h"
#include "Commands.h"
#include "GeneralAirForce.h"
#include "GeneralArtillery.h"
#include "AIUnitInfoForGeneral.h"
#include "Technics.h"
#include "GeneralIntendant.h"
#include "GeneralConsts.h"
#include "TimeCounter.h"
#include "UnitCreation.h"
#include "AILogicInternal.h"

//#include "..\Scene\Statistics.h"
#include "PlayerREinforcement.h"


extern CPlayerReinforcementArray theReinfArray;
extern NAI::CTimeCounter timeCounter;
extern NTimer::STime curTime;
extern CDiplomacy theDipl;
extern CGroupLogic theGroupLogic;
extern CUnitCreation theUnitCreation;

//BASIC_REGISTER_CLASS( CGeneral );
REGISTER_SAVELOAD_CLASS( 0x1508D4B3, CGeneral );

//*******************************************************************
//*														CGeneral															*
//*******************************************************************

void CGeneral::EnumWorkers( const enum EForceType eType, IWorkerEnumerator *pEnumerator )
{
	//search trough reserve to give it to the task
	switch( eType )
	{
	case FT_INFANTRY_IN_TRENCHES:
		EnumWorkersInternal( eType, pEnumerator, &infantryInTrenches );

		break;
	case FT_FREE_INFANTRY:
		EnumWorkersInternal( eType, pEnumerator, &infantryFree );

		break;
	case FT_SWARMING_TANKS:
		EnumWorkersInternal( eType, pEnumerator, &tanksFree );

		break;
	case FT_MOBILE_TANKS:
		EnumWorkersInternal( eType, pEnumerator, &tanksFree );

		break;
	case FT_STATIONARY_MECH_UNITS:
		EnumWorkersInternal( eType, pEnumerator, &stationaryTanks );

		break;
	case FT_TRUCK_REPAIR_BUILDING:
		EnumWorkersInternal( eType, pEnumerator, &transportsFree );
		break;

	default:
		NI_ASSERT( false, StrFmt( "wrong type asked from commander %d", eType ) );
		break;
	}
}

void CGeneral::CancelRequest( int nRequestID, enum EForceType eType ) 
{  
	switch( eType )
	{
	case FT_AIR_SCOUT:
	case FT_AIR_GUNPLANE:
		pAirForce->CancelRequest( nRequestID, eType );
		break;
	case FT_RECAPTURE_STORAGE:
		{
			//find desired task
			RequestedTasks::iterator it = requestedTasks.find( nRequestID );
			if ( it != requestedTasks.end() )
			{
				IGeneralTask * pTask = requestedTasks[nRequestID];
				pTask->CancelTask( this );
				requestIDs.Return( nRequestID );
			}
		}
		break;
	}
}

int /*request ID*/CGeneral::RequestForSupport( const CVec2 &vSupportCenter, enum EForceType eType ) 
{ 
	switch( eType )
	{
	case FT_AIR_SCOUT:
	case FT_AIR_GUNPLANE:
		return pAirForce->RequestForSupport( vSupportCenter, eType );
		break;
	case FT_RECAPTURE_STORAGE:
		{
			const int nID = requestIDs.Get();
			CGeneralTaskRecaptureStorage * pTask = new CGeneralTaskRecaptureStorage( vSupportCenter );
			requestedTasks[nID] = pTask;
			tasks.push_back( pTask );

			return nID;
		}
		break;
	}
	return 0;
}

void CGeneral::Give( CCommonUnit *pWorker, bool bFromReinforcement )
{
	if ( !pWorker || !pWorker->IsRefValid() || !pWorker->IsAlive() ) 
		return;

	const int nScriptID = dynamic_cast<CAILogic*>(Singleton<IAILogic>())->GetScriptID( pWorker );
	if ( IsMobileReinforcement( nScriptID ) || ( bFromReinforcement && nScriptID == -1 )  )
	{
		if ( pWorker->IsFormation() )
		{
			CFormation * pFormation = checked_cast<CFormation*>( pWorker );
			if ( pFormation->GetState()->GetName() == EUSN_GUN_CAPTURE )
			{
				// don't consider gunners as infantry. they are pictures.
			}
			else if ( pFormation->IsInEntrenchment() || 
						( pFormation->GetNextCommand() && 
							( ACTION_COMMAND_IDLE_TRENCH == pFormation->GetNextCommand()->ToUnitCmd().nCmdType ||
								ACTION_COMMAND_IDLE_BUILDING == pFormation->GetNextCommand()->ToUnitCmd().nCmdType ||
								ACTION_COMMAND_ENTER == pFormation->GetNextCommand()->ToUnitCmd().nCmdType
							)
						)
					)
			{
				infantryInTrenches.push_back( checked_cast<CFormation*>(pWorker) );
			}
			else
				infantryFree.push_back( checked_cast<CFormation*>(pWorker) );
		}
		else
		{
			NI_ASSERT( dynamic_cast<CAIUnit*>(pWorker) != 0, "Wrong unit passed" );
			CAIUnit *pUnit = checked_cast<CAIUnit*>(pWorker);
			const SUnitBaseRPGStats *pStats = pUnit->GetStats();

			if ( pStats->IsAviation() )
			{
				pAirForce->Give( pUnit );
			}
			// дальнобойные орудия
			else if ( pUnit->GetFirstArtilleryGun() != 0 )
			{
				if ( pUnit->GetStats()->IsArtillery() || pUnit->GetStats()->IsSPG() || pUnit->GetStats()->etype == RPG_TYPE_TRAIN_SUPER )
					pGeneralArtillery->TakeArtillery( pUnit );
			}
			else
			{
				if ( pStats->IsTransport() )
				{
					const int nScriptID = dynamic_cast<CAILogic*>(Singleton<IAILogic>())->GetScriptID( pWorker );
					if ( IsMobileReinforcement( nScriptID ) || ( bFromReinforcement && nScriptID == -1 )  )
					{
						if ( pStats->etype == RPG_TYPE_TRN_CIVILIAN_AUTO ) 
						{
							// такая байда нафиг не нужна генералу
						}
						else if ( pStats->etype == RPG_TYPE_TRN_MILITARY_AUTO )
						{
							pIntendant->AddReiforcePosition( pUnit->GetCenterPlain(), pUnit->GetDirection() );
							pIntendant->Give( pUnit );
						}
						else
						{
							CAITransportUnit * pTransport = checked_cast<CAITransportUnit*>( pUnit );
							if ( !pTransport->IsMustTow() )
							{
								pIntendant->AddReiforcePosition( pUnit->GetCenterPlain(), pUnit->GetDirection() );
								pIntendant->Give( pWorker );
							}
							else
							{
								pGeneralArtillery->TakeTruck( pUnit );
							}
						}
					}
				}
				else 
				{
					if ( bFromReinforcement || IsMobileReinforcement( dynamic_cast<CAILogic*>(Singleton<IAILogic>())->GetScriptID( pWorker ) ) )
						tanksFree.push_back( pWorker );
					else 
						stationaryTanks.push_back( pWorker );
				}
			}
		}
	}
}

void CGeneral::EraseLastSeen()
{
	int nStep = 10; // optimisation parameter;

	// if finished, start again
	if ( curProcessed == enemys.end() ) 
		curProcessed = enemys.begin();

	// убрать всех врагов, которых видели давно.
	for ( ; curProcessed != enemys.end() && nStep > 0; ++curProcessed )
	{
		--nStep;
		const CUnitTimeSeen &unitTimeSeen = curProcessed->second;
		if ( unitTimeSeen.second != -1 && curTime - unitTimeSeen.second > SGeneralConsts::TIME_DONT_SEE_ENEMY_BEFORE_FORGET )
			erased.push_back( curProcessed->first );
	}
	while ( !erased.empty() )
	{
		BalanceUpdate( BA_REMOVE_ENEMY, CAIUnit::GetUnitByUniqueID( *erased.begin() ) );
		enemys.erase( *erased.begin() );
		erased.pop_front();
	}
}

void CGeneral::Segment()
{
	if ( curTime > timeNextUpdate )
	{
		SGeneralHelper::RemoveDead( &infantryInTrenches );
		SGeneralHelper::RemoveDead( &infantryFree );
		SGeneralHelper::RemoveDead( &tanksFree );
		SGeneralHelper::RemoveNotCurrentPlayer( &stationaryTanks, nParty );
		SGeneralHelper::RemoveDead( &transportsFree );

		EraseLastSeen();

		CCommander::Segment();
		
		pAirForce->Segment();

		// бомбардировака областей
		BombardmentSegment();
		pGeneralArtillery->Segment();
		if ( pIntendant )
			pIntendant->Segment();
		if ( NGlobal::GetVar( "temp.general_reinforcement", true ) )
			CheckAvailableReinforcement();

		timeNextUpdate = curTime + SGeneralConsts::GENERAL_UPDATE_PERIOD + NRandom::Random( 1000 );
		
		//Singleton<IStatistics>()->UpdateEntry( "General: visible enemies", "" );
		//Singleton<IStatistics>()->UpdateEntry( "General: antiartillery circles", "" );
	}
}

void CGeneral::GiveCommandToBombardment()
{
	if ( 2 == cBombardmentType ) return;
	
	const float fComparativeWeight = 
		cBombardmentType == 1 ? SGeneralConsts::MIN_WEIGHT_TO_SEND_BOMBERS : SGeneralConsts::MIN_WEIGHT_TO_ARTILLERY_FIRE;
	// количество областей, среди которых кидать рандом
	int cnt = 0;
	CResistancesContainer::iterator iter = resContainer.begin();
	while ( cnt < 10 && !iter.IsFinished() && (*iter).GetWeight() >= fComparativeWeight )
	{
		const CVec2 vCellCenter = SResistance::GetResistanceCellCenter( (*iter).GetCellNumber() );
		if ( cBombardmentType == 1 || pGeneralArtillery->CanBombardRegion( vCellCenter ) )
			++cnt;

		iter.Iterate();
	}

	// можно бомбить
	if ( cnt > 0 )
	{
		const int nRegion = NRandom::Random( 1, cnt );
		
		cnt = 0;
		iter = resContainer.begin();
		while ( cnt < nRegion )
		{
			const CVec2 vCellCenter = SResistance::GetResistanceCellCenter( (*iter).GetCellNumber() );
			if ( cBombardmentType == 1 || pGeneralArtillery->CanBombardRegion( vCellCenter ) )
				++cnt;
			if ( cnt < nRegion )
				iter.Iterate();
		}

		const CVec2 vCellCenter = SResistance::GetResistanceCellCenter( (*iter).GetCellNumber() );
		const float fRadius = SGeneralConsts::RESISTANCE_CELL_SIZE * SConsts::TILE_SIZE + NRandom::Random( 0, 2 ) * SConsts::TILE_SIZE;

		CVec2 vCenter( VNULL2 );
		int nUnits = 0;
		bool bIsAntiArtilleryFight = false;
		for ( CUnitsIter<0,3> unitsIter( nParty, EDI_ENEMY, vCellCenter, fRadius ); !unitsIter.IsFinished(); unitsIter.Iterate() )
		{
			CAIUnit *pUnit = *unitsIter;
			CAIUnitInfoForGeneral* pInfo = pUnit->GetUnitInfoForGeneral();

			// если послать бомберов или если это враг и его видели не так давно
			if ( cBombardmentType == 1 || curTime - pInfo->GetLastTimeOfVisibility() < pUnit->GetTimeToForget() )
			{
				if ( pInfo->IsLastVisibleAntiArt() )
					bIsAntiArtilleryFight = true;
				
				vCenter += pUnit->GetCenterPlain();
				++nUnits;
			}
		}

		if ( nUnits > 0 )
		{
			vCenter /= float( nUnits );

			if ( cBombardmentType == 1 )
			{
				if ( (*iter).IsWeightExceed() )
					pAirForce->RequestForSupport( vCenter, FT_AIR_GUNPLANE, (*iter).GetCellNumber() );
				pAirForce->RequestForSupport( vCenter, FT_AIR_BOMBER, (*iter).GetCellNumber() );
			}
			else	
			{
				float fMaxDistance = 0;
				for ( CUnitsIter<0,3> unitsIter( nParty, EDI_ENEMY, vCellCenter, fRadius ); !unitsIter.IsFinished(); unitsIter.Iterate() )
				{
					CAIUnit *pUnit = *unitsIter;
					CAIUnitInfoForGeneral* pInfo = pUnit->GetUnitInfoForGeneral();

					// если это враг и его видели не так давно		
					if ( curTime - pInfo->GetLastTimeOfVisibility() < pUnit->GetTimeToForget() )
					{
						const float fDist = fabs( pUnit->GetCenterPlain() - vCenter );
						if ( fDist > fMaxDistance )
							fMaxDistance = fDist;

						// "забыть" о невидимых юнитах в этом радиусе
						if ( !pUnit->IsVisible( nParty ) )
							pUnit->SetLastVisibleTime( 0 );
					}
				}

				pGeneralArtillery->RequestForSupport( vCenter, fMaxDistance, bIsAntiArtilleryFight, (*iter).GetCellNumber() );
			}
		}
	}
}

void CGeneral::GiveResistances( IEnemyEnumerator *pEnumerator )
{
	if ( bSendReserves )
	{
		CResistancesContainer::iterator iter = resContainer.begin();
		while ( !iter.IsFinished() && (*iter).GetWeight() > SGeneralConsts::MIN_WEIGHT_TO_SEND_SWARM && pEnumerator->EnumResistances( *iter ) )
			iter.Iterate();
	}
}

const float Func( const float fX, const float fBound )
{
	//	return ( 1 - exp( -fX * 0.3 ) ) * fBound;

	// x <= f  -->  exp( k * ( x - f ) )
	// x > f   -->  2 - exp( -k * ( x - f ) )

	// точка смены - пятерная минимальная
	const float f = 5.0f;
	// угол касательной в точке f
	const float k = 2.0f;

	float func;
	if ( fX <= f )
		func = exp( k * ( fX - f ) );
	else
		func = 2 - exp( -k * ( fX - f ) );

	// сдвиг функции в ноль в нуле и в fBound в бесконечности
	func = ( func - exp( -k*f ) ) / ( 2 - exp( -k*f ) ) * fBound;

	return func;
}

void CGeneral::Bombardment()
{
	const float fComparativeWeight = 
		cBombardmentType == 1 ? SGeneralConsts::MIN_WEIGHT_TO_SEND_BOMBERS : SGeneralConsts::MIN_WEIGHT_TO_ARTILLERY_FIRE;
	// количество областей, среди которых кидать рандом
	int cnt = 0;
	CResistancesContainer::iterator iter = resContainer.begin();
	while ( cnt < 10 && !iter.IsFinished() && (*iter).GetWeight() >= fComparativeWeight )
	{
		const CVec2 vCellCenter = SResistance::GetResistanceCellCenter( (*iter).GetCellNumber() );
		if ( cBombardmentType == 1 || pGeneralArtillery->CanBombardRegion( vCellCenter ) )
			++cnt;

		iter.Iterate();
	}

	// можно бомбить
	if ( cnt > 0 )
	{
		const int nRegion = NRandom::Random( 1, cnt );

		cnt = 0;
		iter = resContainer.begin();
		while ( cnt < nRegion )
		{
			const CVec2 vCellCenter = SResistance::GetResistanceCellCenter( (*iter).GetCellNumber() );
			if ( cBombardmentType == 1 || pGeneralArtillery->CanBombardRegion( vCellCenter ) )
				++cnt;
			if ( cnt < nRegion )
				iter.Iterate();
		}

		const CVec2 vCellCenter = SResistance::GetResistanceCellCenter( (*iter).GetCellNumber() );
		const float fRadius = SGeneralConsts::RESISTANCE_CELL_SIZE * SConsts::TILE_SIZE + NRandom::Random( 0, 2 ) * SConsts::TILE_SIZE;

		CVec2 vCenter( VNULL2 );
		int nUnits = 0;
		bool bIsAntiArtilleryFight = false;
		for ( CUnitsIter<0,3> unitsIter( nParty, EDI_ENEMY, vCellCenter, fRadius ); !unitsIter.IsFinished(); unitsIter.Iterate() )
		{
			CAIUnit *pUnit = *unitsIter;
			CAIUnitInfoForGeneral* pInfo = pUnit->GetUnitInfoForGeneral();

			// если послать бомберов или если это враг и его видели не так давно
			if ( cBombardmentType == 1 || curTime - pInfo->GetLastTimeOfVisibility() < pUnit->GetTimeToForget() )
			{
				if ( pInfo->IsLastVisibleAntiArt() )
					bIsAntiArtilleryFight = true;

				vCenter += pUnit->GetCenterPlain();
				++nUnits;
			}
		}

		if ( nUnits > 0 )
		{
			vCenter /= float( nUnits );

			if ( cBombardmentType == 1 )
			{
				vector<EForceType> availFroces;
				if ( theReinfArray[nParty].HasReinforcement( NDb::RT_GROUND_ATTACK_PLANES ) )
					availFroces.push_back( FT_AIR_GUNPLANE );
				if ( theReinfArray[nParty].HasReinforcement( NDb::RT_BOMBERS ) )
					availFroces.push_back( FT_AIR_BOMBER );
				if ( !availFroces.empty() )
				{
					float fMaxDistance = 0;
					for ( CUnitsIter<0,3> unitsIter( nParty, EDI_ENEMY, vCellCenter, fRadius ); !unitsIter.IsFinished(); unitsIter.Iterate() )
					{
						CAIUnit *pUnit = *unitsIter;
						CAIUnitInfoForGeneral* pInfo = pUnit->GetUnitInfoForGeneral();

						// если это враг и его видели не так давно		
						if ( curTime - pInfo->GetLastTimeOfVisibility() < pUnit->GetTimeToForget() )
						{
							// "забыть" о невидимых юнитах в этом радиусе
							if ( !pUnit->IsVisible( nParty ) )
								pUnit->SetLastVisibleTime( 0 );
						}
					}
					pAirForce->RequestForSupport( vCenter, availFroces[NRandom::Random(0, availFroces.size() -1)], (*iter).GetCellNumber() );
				}
			}
			else	
			{
				float fMaxDistance = 0;
				for ( CUnitsIter<0,3> unitsIter( nParty, EDI_ENEMY, vCellCenter, fRadius ); !unitsIter.IsFinished(); unitsIter.Iterate() )
				{
					CAIUnit *pUnit = *unitsIter;
					CAIUnitInfoForGeneral* pInfo = pUnit->GetUnitInfoForGeneral();

					// если это враг и его видели не так давно		
					if ( curTime - pInfo->GetLastTimeOfVisibility() < pUnit->GetTimeToForget() )
					{
						const float fDist = fabs( pUnit->GetCenterPlain() - vCenter );
						if ( fDist > fMaxDistance )
							fMaxDistance = fDist;

						// "забыть" о невидимых юнитах в этом радиусе
						if ( !pUnit->IsVisible( nParty ) )
							pUnit->SetLastVisibleTime( 0 );
					}
				}

				pGeneralArtillery->RequestForSupport( vCenter, fMaxDistance, bIsAntiArtilleryFight, (*iter).GetCellNumber() );
			}
		}
	}
}

void CGeneral::ArtilleryBombardment()
{
	cBombardmentType = 0;
	Bombardment();
}

void CGeneral::AviationBombardment()
{
	cBombardmentType = 1;
	Bombardment();
}

void CGeneral::BombardmentSegment()
{
	// allow tank swarm if weight is sufficient
	CResistancesContainer::iterator iter = resContainer.begin();
	if ( !iter.IsFinished() )
	{
		if ( curTime >= lastBombardmentCheck + 1000 )
		{
			const float fComparativeWeight = Min( SGeneralConsts::MIN_WEIGHT_TO_ARTILLERY_FIRE, SGeneralConsts::MIN_WEIGHT_TO_SEND_BOMBERS);
			const SResistance &maxCell = *iter;
			const float fRatio = maxCell.GetWeight() / fComparativeWeight;
			const CVec2 vCellCenter = SResistance::GetResistanceCellCenter( maxCell.GetCellNumber() );

			bSendReserves = maxCell.GetWeight() >= SGeneralConsts::MIN_WEIGHT_TO_SEND_SWARM;

			// check if best region have sufficient weight to bombard it
			if ( fRatio >= 1.0f )
			{
				const float fProbability = 
					Func( fRatio, 1 - SGeneralConsts::PROBABILITY_TO_SHOOT_AFTER_ARTILLERY_FIRE )
					+ 
					SGeneralConsts::PROBABILITY_TO_SHOOT_AFTER_ARTILLERY_FIRE;
				// делится на 1000, т.к. проверка раз в секунду
				const float fProbNow = fProbability >= 1.0f ? 1.0f : 1 - exp( 1.0f/((float)SGeneralConsts::TIME_TO_ARTILLERY_FIRE / 1000.0f) * log( 1.0f - fProbability ) );

				// нужно стрелять
				const float fRand = NRandom::Random( 0.0f, 1.0f );
				if ( fRand < fProbNow )
				{
					// check if artillery can reach
					if ( pGeneralArtillery->CanBombardRegion( vCellCenter ) )
					{
						// if it can - bombard region with artillery
						ArtilleryBombardment();
					}
					else
					{
						// else try send bombers or ground attack planes
						AviationBombardment();
					}
				}
			}			
		}
	}
}

bool CGeneral::IsMobileReinforcement( int nGroupID ) const
{
	return mobileReinforcementGroupIDs.find( nGroupID ) != mobileReinforcementGroupIDs.end();
}

void CGeneral::GiveNewUnits( const list<CCommonUnit*> &pUnits, bool bFromReinforcement )
{
	typedef hash_map<int, bool> Formations;
	Formations formations; // добавляемые формации
	
	// забрать все юниты, кроме солдат. солдат забирать формациями	
	for ( list<CCommonUnit*>::const_iterator iter = pUnits.begin(); iter != pUnits.end(); ++iter )
	{
		CCommonUnit *pUnit = *iter;
		if ( theDipl.GetDiplStatusForParties( pUnit->GetParty(), nParty ) == EDI_FRIEND )
		{
			if ( pUnit->IsInFormation() &&
					 pUnit->GetFormation()->GetState()->GetName() != EUSN_GUN_CREW_STATE )
				formations[pUnit->GetFormation()->GetUniqueId()] = true;
			else
			{
				Give( pUnit, bFromReinforcement );
				BalanceUpdate( BA_ADD_OWN, pUnit );
			}
		}
	}

	// забрать формации
	for( Formations::iterator it = formations.begin(); it != formations.end(); ++it )
	{
		if ( CCommonUnit *pUnit = GetObjectByUniqueIdSafe<CCommonUnit>( it->first ) )
		{
			Give( pUnit, bFromReinforcement );
			BalanceUpdate( BA_ADD_OWN, pUnit );
		}
	}
	
	// another oppotunity to ask for worker for tasks
	for ( Tasks::iterator it = tasks.begin(); it != tasks.end(); ++it )
		(*it)->AskForWorker( this, 0, true );
}

void CGeneral::Init()
{
	bSendReserves = false;
	SGeneralConsts::Init();
	pAirForce = new CGeneralAirForce( nParty, this );
	pGeneralArtillery = new CGeneralArtillery( this );
	pIntendant = new CGeneralIntendant( nParty, this );
	pIntendant->Init();

	tasks.push_back( new CGeneralTaskToSwarmToPoint( this ) );

	curProcessed = enemys.begin();

	InitRearManager();
}

void CGeneral::Init( const NDb::SAIGeneralSide &mapInfo )
{
	// создать список мобильных отрядов
	for ( int i = 0; i < mapInfo.mobileScriptIDs.size(); ++i )
		mobileReinforcementGroupIDs.insert( mapInfo.mobileScriptIDs[i] );
	nMaxAllowedMobileTanks = mapInfo.nMaxMobileTanks;

	Init();
	
	bool bReinforceCreated = false;

	for ( int i = 0; i < mapInfo.parcels.size(); ++i )
	{
		switch( mapInfo.parcels[i].eType )
		{
		case EPATCH_DEFENCE:								// защита патчей 
			{
				CGeneralTaskToDefendPatch * pTask  = new CGeneralTaskToDefendPatch;
				pTask->Init( mapInfo.parcels[i], this );
				pTask->AskForWorker( this, 0, true );
				tasks.push_back( pTask );
			}
			break;
		case EPATCH_REINFORCE:
			{
				bReinforceCreated = true;
				// манипулирование резервами
				CGeneralTaskToHoldReinforcement * pTask = new CGeneralTaskToHoldReinforcement;
				pTask->Init( mapInfo.parcels[i] );
				// notify Intendant about points to hold reinforcement.
				pIntendant->AddReiforcePositions( mapInfo.parcels[i] );
				pTask->AskForWorker( this, 0, true );
				tasks.push_back( pTask );
			}
			break;
		}
	}
	/*
	// there is no reinforcement positions. create one, MUST HAVE at leas one
	if ( !bReinforceCreated )
	{
		// манипулирование резервами
		CGeneralTaskToHoldReinforcement * pTask = new CGeneralTaskToHoldReinforcement;
		pTask->Init();
		pTask->AskForWorker( this, 0, true );
		tasks.push_back( pTask );
		pTask->SetEnemyConatiner( this );
	}*/
}

void CGeneral::SetAAVisible( class CAIUnit *pUnit, const bool bVisible )
{
	pAirForce->SetAAVisible( pUnit, bVisible );
	enemys[pUnit->GetUniqueId()] = CUnitTimeSeen( pUnit, bVisible ? -1 : curTime );
}

void CGeneral::SetUnitVisible( class CAIUnit *pUnit, const bool bVisible )
{
	const SUnitBaseRPGStats * pStats = pUnit->GetStats();
	if ( EDI_ENEMY == theDipl.GetDiplStatusForParties( pUnit->GetParty(), nParty ) )
	{	
		if ( !pStats->IsAviation() )
		{
			if ( pStats->IsArtillery() )
			{
				// invisible artillery is cannot be supplied

			}
			if ( pUnit->CanShootToPlanes() )
				SetAAVisible( pUnit, bVisible );
			else
			{
				NI_ASSERT( pUnit->GetUniqueId(), "unit has zero unique ID" );
				enemys[pUnit->GetUniqueId()] = CUnitTimeSeen(pUnit, bVisible ? -1 : curTime );
			}
		}
		BalanceUpdate( BA_ADD_ENEMY, pUnit );
	}
	if ( pUnit->GetParty() == theDipl.GetNeutralParty() && pStats->IsArtillery() )
	{
		pIntendant->SetArtilleryVisible( pUnit, bVisible );
	}
}

void CGeneral::RemoveResistance( const CVec2 &vCenter )
{
	resContainer.RemoveExcluded( vCenter );
}

void CGeneral::AddResistance( const CVec2 &vCenter, const float fRadius )
{
	resContainer.AddExcluded( vCenter, fRadius );
}

void CGeneral::GiveEnemies( IEnemyEnumerator *pEnumerator )
{
	for ( CEnemyVisibility::iterator it = enemys.begin();
				it != enemys.end() && pEnumerator->EnumEnemy( it->second.first ); ++it );
}

void CGeneral::UpdateEnemyUnitInfo( CAIUnitInfoForGeneral *pInfo,
	const NTimer::STime lastVisibleTimeDelta, const CVec2 &vLastVisiblePos,
	const NTimer::STime lastAntiArtTimeDelta, const CVec2 &vLastVisibleAntiArtCenter, const float fDistToLastVisibleAntiArt )
{
	resContainer.UpdateEnemyUnitInfo(
		pInfo, lastVisibleTimeDelta, vLastVisiblePos,	lastAntiArtTimeDelta, vLastVisibleAntiArtCenter, fDistToLastVisibleAntiArt );
}

void CGeneral::UnitDied( class CCommonUnit * pUnit )
{
	pIntendant->UnitDead( pUnit );
	CEnemyVisibility::iterator it = enemys.find( pUnit->GetUniqueId() );
	if ( it != enemys.end() )
	{
		if ( curProcessed == it )
			++curProcessed;
		BalanceUpdate( BA_REMOVE_ENEMY, pUnit );
		enemys.erase( it );
	}
	else if ( pUnit->GetPlayer() != BYTE(-1) && pUnit->GetParty() == nParty )				// Own unit died
	{
		BalanceUpdate( BA_REMOVE_OWN, pUnit );
	}
}

void CGeneral::UnitDied( CAIUnitInfoForGeneral *pInfo )
{
	UnitDied( pInfo->GetOwner() );

	if ( EDI_ENEMY == theDipl.GetDiplStatusForParties( pInfo->GetOwner()->GetParty(), nParty ) )
	{
		resContainer.UnitDied( pInfo );
		pAirForce->DeleteAA( pInfo->GetOwner() );
	}
	else
	{
		CAIUnit *pUnit = pInfo->GetOwner();
		CAIUnit *pTruck = pUnit->GetTruck();
		if ( pTruck && pTruck->IsRefValid() && pTruck->IsAlive() )
			Give( pTruck );			
	}
}

void CGeneral::UnitChangedPosition( class CCommonUnit * pUnit, const CVec2 &vNewPos )
{
	if ( pUnit->IsFormation() || !checked_cast<CAIUnit*>( pUnit )->GetStats()->IsAviation() )
	{
		pIntendant->UnitChangedPosition( pUnit, vNewPos );
	}
}

void CGeneral::UnitAskedForResupply( class CCommonUnit * pUnit, const EResupplyType eType, const bool bSet )
{
	pIntendant->UnitAskedForResupply( pUnit, eType, bSet );
}

void CGeneral::UnitChangedParty( CAIUnit *pUnit, const int nNewParty )
{
	BalanceUpdate( BA_REMOVE_OWN, pUnit );

	switch ( theDipl.GetDiplStatusForParties( nNewParty, nParty ) )
	{
	case EDI_ENEMY:
		{
			resContainer.UnitChangedParty( pUnit->GetUnitInfoForGeneral() );

			pAirForce->DeleteAA( pUnit );
			if ( !pUnit->GetStats()->IsAviation() )
				pIntendant->UnitDead( pUnit );

			BalanceUpdate( BA_ADD_ENEMY, pUnit );
		}

		break;
	case EDI_NEUTRAL:
		{
			const SUnitBaseRPGStats * pStats = pUnit->GetStats();
			if ( !pStats->IsAviation() && !pStats->IsArtillery() )
				pIntendant->UnitDead( pUnit );
		}

		break;
	case EDI_FRIEND:
		if ( !pUnit->GetStats()->IsAviation() )
			pIntendant->UnitDead( pUnit );
		
		break;
	}
}

void CGeneral::SetCellInUse( const int nResistanceCellNumber, bool bInUse )
{
	resContainer.SetCellInUse( nResistanceCellNumber, bInUse );
}

bool CGeneral::IsInResistanceCircle( const CVec2 &vPoint ) const
{
	return resContainer.IsInResistanceCircle( vPoint );
}

