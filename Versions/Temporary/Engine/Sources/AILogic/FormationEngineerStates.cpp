#include "stdafx.h"

#include <float.h>

#include "FormationStates.h"
#include "Formation.h"
#include "Commands.h"
#include "Guns.h"
#include "GroupLogic.h"
#include "NewUpdater.h"
#include "Technics.h"
#include "BridgeCreation.h"
#include "Building.h"
#include "Artillery.h"
#include "Turret.h"
#include "Soldier.h"
#include "PathFinder.h"
#include "UnitsIterators2.h"
#include "UnitCreation.h"
#include "ArtilleryPaths.h"
#include "ObstacleInternal.h"
#include "Bridge.h"
#include "General.h"
#include "Statistics.h"
#include "Common_RTS_AI/StaticMapHeights.h"
#include "Common_RTS_AI/PathFinder.h"
#include "Common_RTS_AI/StandartDirPath.h"
#include "FeedBackSystem.h"
#include "EntrenchmentCreation.h"

#include "AILogic_export.h"

#include <algorithm>
#include <cstdint>

#include <fmt/format.h>

REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D474, CFormationRepairUnitState );
REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D475, CFormationLoadRuState );
REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D476, CFormationResupplyUnitState );
REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D477, CFormationPlaceAntitankState );
REGISTER_SAVELOAD_CLASS( AILOGIC, 0x33230B40, CFormationBuildEntrenchmentState )

extern CFeedBackSystem theFeedBackSystem;
extern CSupremeBeing theSupremeBeing;
extern CUnitCreation theUnitCreation;
extern CStaticObjects theStatObjs;
extern NTimer::STime curTime;
extern CGroupLogic theGroupLogic;
extern CEventUpdater updater;
extern CDiplomacy theDipl;
extern CUnits units;
extern CStatistics theStatistics;

float Heal( const float fMaxHP, const float fCurHP, const float fRepCost,
																			 float *pfWorkAccumulator, float *pfWorkLeft, class CAITransportUnit *pHomeTransport )
{
	if (  fMaxHP !=  fCurHP )
	{
		const float hpRepeared = *pfWorkAccumulator / fRepCost;
		const float dh =  (std::min)( fMaxHP - fCurHP, hpRepeared );
		{
			*pfWorkAccumulator -= dh * fRepCost;
			*pfWorkLeft -= dh * fRepCost ;
			pHomeTransport->DecResursUnitsLeft( dh * fRepCost );
		}
		return fCurHP + dh;
	}
	return fCurHP;
}

//*******************************************************************
//*												CFormationPlaceMine												*
//*******************************************************************

IUnitState* CFormationPlaceMine::Instance( CFormation *pFormation, const CVec2 &point, const enum EMineType eType )
{
	return new CFormationPlaceMine( pFormation, point, eType );
}

CFormationPlaceMine::CFormationPlaceMine( CFormation *_pFormation, const CVec2 &_point, const enum EMineType _eType )
: pFormation( _pFormation ), point( _point ), eType( _eType ), eState( EPM_WAIT_FOR_HOMETRANSPORT )
{
}

void CFormationPlaceMine::SetHomeTransport( class CAITransportUnit *pTransport )
{
	NI_ASSERT( eState == EPM_WAIT_FOR_HOMETRANSPORT, "wrong state" );
	eState = EPM_START;
	pHomeTransport = pTransport;
}

void CFormationPlaceMine::Segment()
{
	if ( !IsValidObj( pHomeTransport ) )
		TryInterruptState( 0 );

	switch ( eState )
	{
		case EPM_WAIT_FOR_HOMETRANSPORT:
			if ( pFormation->GetNextCommand() )
				pFormation->SetCommandFinished();

			break;
		case EPM_START:
			if ( CPtr<IStaticPath> pStaticPath = CreateStaticPathToPoint( point, VNULL2, pFormation, true, GetAIMap() ) )
			{
				pFormation->SendAlongPath( pStaticPath, pFormation->GetGroupShift(), true );
				eState = EPM_MOVE;
			}
			else
			{
				pFormation->SendAcknowledgement( ACK_NEGATIVE );
				pFormation->SetCommandFinished();
			}
	
			break;
		case EPM_MOVE:
			{
//				const float fDist = fabs2( pFormation->GetCenter() - point - pFormation->GetGroupShift() );

				if ( pFormation->IsIdle() )
				{
					pFormation->StopFormationCenter();
					// все юниты добежали и встали в строй
					if ( pFormation->IsIdle() )
					{
						// если был послан один солдат, то нужно положить мину точно куда указали
						if ( pFormation->Size() == 1 && pFormation->GetGroupShift() == VNULL2 )
						{
							if ( SConsts::MINE_RU_PRICE[eType] <= pHomeTransport->GetResursUnitsLeft() )
							{
								pHomeTransport->DecResursUnitsLeft( SConsts::MINE_RU_PRICE[eType] );
								theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_PLACEMINE_NOW, point.x, point.y, float(eType) ), (*pFormation)[0], false );
							}
						}
						else
						{
							for ( int i = 0; i < pFormation->Size(); ++i )
							{
								if ( SConsts::MINE_RU_PRICE[eType] <= pHomeTransport->GetResursUnitsLeft() )
								{
									pHomeTransport->DecResursUnitsLeft( SConsts::MINE_RU_PRICE[eType] );
									const CVec2 point = (*pFormation)[i]->GetCenterPlain() + (*pFormation)[i]->GetDirectionVector() * SConsts::TILE_SIZE * 0.5f;
									theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_PLACEMINE_NOW, point.x, point.y, float(int(eType)) ), (*pFormation)[i], false );
								}
							}
						}
					}

					pFormation->SetToWaitingState();
					eState = EPM_WAITING;
				}
			}

			break;
		case EPM_WAITING:
			if ( pFormation->IsEveryUnitResting() )
				pFormation->SetCommandFinished();

			pFormation->UnsetFromWaitingState();

			break;
	}
}

ETryStateInterruptResult CFormationPlaceMine::TryInterruptState( class CAICommand *pCommand )
{
	pFormation->UnsetFromWaitingState();
	pFormation->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

//*******************************************************************
//*												CFormationClearMine												*
//*******************************************************************

IUnitState* CFormationClearMine::Instance( CFormation *pFormation, const CVec2 &point )
{
	return new CFormationClearMine( pFormation, point );
}

CFormationClearMine::CFormationClearMine( CFormation *_pFormation, const CVec2 &_point )
: pFormation( _pFormation ), point( _point ), eState( EPM_START )
{
}

void CFormationClearMine::Segment()
{
	switch ( eState )
	{
		case EPM_START:
			if ( CPtr<IStaticPath> pStaticPath = pFormation->GetCurCmd()->CreateStaticPath( pFormation ) )
			{
				pFormation->SendAlongPath( pStaticPath, pFormation->GetGroupShift(), true );
				eState = EPM_MOVE;
			}
			else
			{
				pFormation->SendAcknowledgement( ACK_NEGATIVE );
				pFormation->SetCommandFinished();
			}

			break;
		case EPM_MOVE:
			if ( pFormation->IsIdle() )
			{
				pFormation->StopFormationCenter();
				for ( int i = 0; i < pFormation->Size(); ++i )
					theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_CLEARMINE_RADIUS, (*pFormation)[i]->GetCenter().x, (*pFormation)[i]->GetCenter().y ), (*pFormation)[i], false );

				pFormation->SetToWaitingState();
				eState = EPM_WAIT;
			}

			break;

		case EPM_WAIT:
			if ( pFormation->IsEveryUnitResting() )
				pFormation->SetCommandFinished();

			pFormation->UnsetFromWaitingState();

			break;
	}
}

ETryStateInterruptResult CFormationClearMine::TryInterruptState( class CAICommand *pCommand )
{
	pFormation->UnsetFromWaitingState();
	pFormation->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

//*******************************************************************
//*										CFormationRepairUnitState::CFindBestStorageToRepearPredicate *
//*******************************************************************

bool CFormationRepairUnitState::CFindFirstStorageToRepearPredicate::AddStorage( class CBuilding * pStorage, const float fPathLenght )
{
	const SBuildingRPGStats * pStats = checked_cast<const SBuildingRPGStats *>(pStorage->GetStats());
	if ( TYPE_TEMP_RU_STORAGE == pStats->etype &&
			 pStorage->GetHitPoints() != pStats->fMaxHP ) // нужна починка
	{
		if ( pStats->fRepairCost * SConsts::REPAIR_COST_ADJUST <= fMaxRu )						// можем починить хоть немного
			bHasStor = true;
		else
			bNotEnoughRu = true;
	}
	return bHasStor;
}

//*******************************************************************
//*										CFormationRepairUnitState											*
//*******************************************************************

IUnitState* CFormationRepairUnitState::Instance( class CFormation *_pUnit, CAIUnit *_pPreferredUnit )
{
	return new CFormationRepairUnitState( _pUnit, _pPreferredUnit );
}

CFormationRepairUnitState::CFormationRepairUnitState( class CFormation *pUnit, CAIUnit *_pPreferredUnit )
: CFormationServeUnitState( _pPreferredUnit ), pUnit( pUnit ), 
	lastRepearTime( curTime ), vPointInQuestion( VNULL2 ), fRepCost( 0 ),	bNearTruck( true )
{
}

bool CFormationRepairUnitState::CheckUnit( CAIUnit *pU, CFormationServeUnitState::CFindUnitPredicate * pPred, const float fResurs, const CVec2 &vCenter )
{
	if ( pU && pU->IsRefValid() && pU->IsAlive() &&
		fabs2( pU->GetCenterPlain() - vCenter ) < sqr( SConsts::RESUPPLY_RADIUS ) )
	{
		const EUnitRPGType type = pU->GetStats()->etype;
		const SHPObjectRPGStats * pStats = pU->GetStats();
		
		bool bCannotReachUnit = false; // до юнита нельзя дойти
		float fTrackHP = 0;

		// у танков - проверить гусеницу.
		if ( ::IsArmor( type ) || ::IsSPG( type ) )
		{
			//CRAP{ OTKAZALSA OT PUTI, TORMOZILO
			/*CPtr<IStaticPath> pStaticPath = CreateStaticPathToPoint( pU->GetCenter(), VNULL2, pLoaderSquad, true );
			SRect unitRect = pU->GetUnitRect();
			unitRect.Compress( 1.2f );
			if ( !pStaticPath || !unitRect.IsPointInside( pStaticPath->GetFinishPoint() ) )
			{
				bCannotReachUnit = true;
			}*/
			//CRAP}
			if ( !bCannotReachUnit )
			{
				CTank * pTank = checked_cast<CTank*>( pU );
				// повреждена гусеница и хватит ресурсов, чтобы ее починить
				if ( pTank->IsTrackDamaged() )
				{
					if ( pStats->fRepairCost * SConsts::TANK_TRACK_HIT_POINTS * SConsts::REPAIR_COST_ADJUST <= fResurs )
						fTrackHP = SConsts::TANK_TRACK_HIT_POINTS;
					else
					{
						pPred->SetNotEnoughRu();
						return false;
					}
				}
			}
		}

		if ( (::IsArmor(type) || ::IsArtillery(type) || ::IsTrain(type) || ::IsTransport(type) || ::IsSPG(type) ) && 
				pU->IsIdle() )
		{
			//CRAP{ OTKAZALSA OT PUTI, TORMOZILO
			/*CPtr<IStaticPath> pStaticPath = CreateStaticPathToPoint( pU->GetCenter(), VNULL2, pLoaderSquad, true );
			SRect unitRect = pU->GetUnitRect();
			unitRect.Compress( 1.2f );
			if ( pStaticPath && unitRect.IsPointInside( pStaticPath->GetFinishPoint() ) )*/

			//CRAP}
			{
				const float curHP = pU->GetStats()->fMaxHP + fTrackHP - pU->GetHitPoints();
				if ( curHP >  0.0f )
				{
					if ( pStats->fRepairCost * SConsts::REPAIR_COST_ADJUST > fResurs )
						pPred->SetNotEnoughRu();
					else
					{
						//CRAP{ OTKAZALSA OT PUTI, TORMOZILO
						/*if ( pPred->SetUnit( pU, curHP, pStaticPath->GetLength() * SAIConsts::TILE_SIZE ) )
							return;*/
						//CRAP}
						if ( pPred->SetUnit( pU, curHP, fabs( pU->GetCenterPlain() - vCenter )) )
							return true;
					}
				}
			}
		}
	}
	return false;
}

void CFormationRepairUnitState::FindUnitToServe( const CVec2 &vCenter, 
																									int nPlayer, 
																									const float fResurs, 
																									CCommonUnit * pLoaderSquad, 
																									CFormationServeUnitState::CFindUnitPredicate * pPred,
																									CAIUnit *_pPreferredUnit )
{
	// first - check prefered unit
	if ( CheckUnit( _pPreferredUnit, pPred, fResurs, vCenter ) || pPred->HasUnit() )
		return;

	CUnitsIter<0,2> iter( theDipl.GetNParty(nPlayer), EDI_FRIEND, vCenter, SConsts::RESUPPLY_RADIUS );

	while ( !iter.IsFinished() )
	{
		if ( CheckUnit( (*iter), pPred, fResurs, vCenter ) )
			return;
		
		iter.Iterate();
	}
}

void CFormationRepairUnitState::Segment()
{
	// реакция на смещение юнита
	if ( EFRUS_WAIT_FOR_HOME_TRANSPORT != eState )
	{
		if ( !IsValidObj( pHomeTransport ) )
			TryInterruptState( 0 );
		if ( ( !IsValidObj( pUnitInQuiestion ) || 	//реакция на смерть юнита
				pUnitInQuiestion->GetCenterPlain() != vPointInQuestion ) ) // да пошел! еще бегать за ним.
		{
			eState = EFRUS_FIND_UNIT_TO_SERVE;
		}
		else if ( pUnitInQuiestion->GetHitPoints() == pUnitInQuiestion->GetStats()->fMaxHP )
		{
			const EUnitRPGType type = pUnitInQuiestion->GetStats()->etype;
			// у танков - проверить гусеницу.
			if ( !::IsArmor( type ) && !::IsSPG( type ) && !static_cast_ptr<CTank*>( pUnitInQuiestion )->IsTrackDamaged() )
			{
				eState = EFRUS_FIND_UNIT_TO_SERVE;
			}
		}
	}

	switch ( eState )
	{
	case EFRUS_WAIT_FOR_HOME_TRANSPORT:
		if ( pUnit->GetNextCommand() )
			pUnit->SetCommandFinished();		

		break;
	case EFRUS_WAIT_FOR_UNIT_TO_SERVE:
		if ( !bNearTruck )
		{
			Interrupt();
			break;
		}
		if ( curTime - lastRepearTime > 3000 )
			eState = EFRUS_FIND_UNIT_TO_SERVE;

		break;
	case EFRUS_FIND_UNIT_TO_SERVE:
		{
			CFormationServeUnitState::CFindBestUnitPredicate pred;
			FindUnitToServe( pHomeTransport->GetCenterPlain(), pHomeTransport->GetPlayer(), fWorkLeft, pUnit, &pred, pPreferredUnit );
			if ( pred.HasUnit() )
			{
				pUnitInQuiestion = pred.GetUnit();
				vPointInQuestion = pUnitInQuiestion->GetCenterPlain();
				EUnitRPGType type = pUnitInQuiestion->GetStats()->etype;
				if (::IsArmor(type) || ::IsSPG(type) || ::IsTrain(type) )
					pTank = static_cast_ptr< CTank* >( pUnitInQuiestion );
				eState = EFRUS_START_APPROACH;
				fRepCost = pUnitInQuiestion->GetStats()->fRepairCost * SConsts::REPAIR_COST_ADJUST;
				break;
			}
			else if ( pred.IsNotEnoughRu() || !bNearTruck ||
								fWorkLeft != (std::min)( SConsts::ENGINEER_RU_CARRY_WEIGHT, pHomeTransport->GetResursUnitsLeft() ) )
				Interrupt();
			else
			{
				lastRepearTime = curTime;
				eState = EFRUS_WAIT_FOR_UNIT_TO_SERVE;
			}
		}

		break;
	case EFRUS_START_APPROACH:
		{
			CPtr<IStaticPath> pStaticPath = CreateStaticPathToPoint( pUnitInQuiestion->GetCenterPlain(), VNULL2, pUnit, true, GetAIMap() );
			if( pStaticPath && pStaticPath->GetLength() * GetAIMap()->GetTileSize() < 3 * SConsts::TRANSPORT_RESUPPLY_OFFSET )
				pUnit->SendAlongPath( pStaticPath, VNULL2, true );
			else
			{
				SRect unitRect = pUnitInQuiestion->GetUnitRect();
				
				for ( int i = 0; i < pUnit->Size(); ++i )
				{
					CVec2 vPoint;
					if ( !GetRectBeamIntersection( &vPoint, (*pUnit)[i]->GetCenterPlain(), pUnitInQuiestion->GetCenterPlain() - (*pUnit)[i]->GetCenterPlain(), unitRect ) )
						vPoint = pUnitInQuiestion->GetCenterPlain();
					CPtr<CArtilleryCrewPath> pPath = new CArtilleryCrewPath( (*pUnit)[i], (*pUnit)[i]->GetCenterPlain(), vPoint, (*pUnit)[i]->GetMaxPossibleSpeed() );
					(*pUnit)[i]->SetSmoothPath( pPath );
				}
			}

			bNearTruck = false;
			eState = EFRUS_APPROACHING;
		}

		break;
	case EFRUS_APPROACHING:
		if ( pUnit->IsIdle() )
		{
			eState = EFRUS_START_SERVICE;
			for ( int i = 0; i < pUnit->Size(); ++i )
				(*pUnit)[i]->RestoreSmoothPath();
		}

		break;
	case EFRUS_START_SERVICE:
		for ( int i = 0; i < pUnit->Size(); ++i )
			theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_COMMAND_USE, (float)ACTION_NOTIFY_USE_UP ), (*pUnit)[i] );

		eState = EFRUS_SERVICING;
		lastRepearTime = curTime;
		fWorkAccumulator = 0;

		break;
	case EFRUS_SERVICING:
		if ( curTime - lastRepearTime > SConsts::TIME_QUANT )
		{
			fWorkAccumulator += pUnit->Size() * 
													SConsts::RU_PER_QUANT *
													( curTime - lastRepearTime ) / SConsts::TIME_QUANT;

			fWorkAccumulator = (std::min)( fWorkAccumulator, fWorkLeft );

			// гусеницу - в первую очередь
			if ( pTank && pTank->IsTrackDamaged() && // если повреждена и достаточно ресурсов
					fWorkLeft >= fRepCost * SConsts::TANK_TRACK_HIT_POINTS ) 
			{
				if ( fRepCost * SConsts::TANK_TRACK_HIT_POINTS <= fWorkAccumulator )
				{
					fWorkAccumulator -= fRepCost * SConsts::TANK_TRACK_HIT_POINTS;
					fWorkLeft -= fRepCost * SConsts::TANK_TRACK_HIT_POINTS ;
					pHomeTransport->DecResursUnitsLeft( fRepCost * SConsts::TANK_TRACK_HIT_POINTS );
					pTank->RepairTrack();
				}
			}
			else
			{
				const float maxHP = pUnitInQuiestion->GetStats()->fMaxHP;
				const float curHP = pUnitInQuiestion->GetHitPoints();
				const float fRepCost = pUnitInQuiestion->GetStats()->fRepairCost * SConsts::REPAIR_COST_ADJUST;
			
				const float fNewHP = Heal( maxHP, curHP, fRepCost, &fWorkAccumulator, &fWorkLeft, pHomeTransport );
				pUnitInQuiestion->IncreaseHitPoints( fNewHP - curHP );
				if ( pUnitInQuiestion->GetStats()->fMaxHP == pUnitInQuiestion->GetHitPoints() )//починили
				{
					theSupremeBeing.UnitAskedForResupply( pUnitInQuiestion, ERT_REPAIR, false );
					eState = EFRUS_FIND_UNIT_TO_SERVE;
				}
				else if ( fWorkLeft + fWorkAccumulator < fRepCost )
					eState = EFRUS_FIND_UNIT_TO_SERVE;
			}

			lastRepearTime = curTime;
		}

		break;
	}
}

void CFormationRepairUnitState::Interrupt()
{
	if ( !pUnit->IsIdle() )
	{
		for ( int i = 0; i < pUnit->Size(); ++i )
			(*pUnit)[i]->RestoreSmoothPath();
		pUnit->Stop();
	}

	pUnit->SetCommandFinished();
}

ETryStateInterruptResult CFormationRepairUnitState::TryInterruptState( class CAICommand *pCommand )
{
	if ( !pCommand || pCommand->ToUnitCmd().nCmdType == ACTION_MOVE_CATCH_TRANSPORT )
	{
		Interrupt();
		return TSIR_YES_IMMIDIATELY;
	}
	else if ( pCommand->ToUnitCmd().nCmdType == ACTION_MOVE_SET_HOME_TRANSPORT ||
						pCommand->ToUnitCmd().nCmdType == ACTION_COMMAND_LOAD )
	{
		return TSIR_YES_WAIT;
	}
	return TSIR_NO_COMMAND_INCOMPATIBLE;
}


//*******************************************************************
//*										CFormationServeUnitState										*
//*******************************************************************


void CFormationServeUnitState::SetHomeTransport( class CAITransportUnit *pTransport )
{
	NI_ASSERT( eState == EFRUS_WAIT_FOR_HOME_TRANSPORT, "wrong state" );
	eState = EFRUS_FIND_UNIT_TO_SERVE;
	pHomeTransport = pTransport;
	fWorkLeft = (std::min)( SConsts::ENGINEER_RU_CARRY_WEIGHT, pTransport->GetResursUnitsLeft() );
}

//*******************************************************************
//*										CFormationResupplyUnitState										*
//*******************************************************************

IUnitState* CFormationResupplyUnitState::Instance( class CFormation *_pUnit, class CAIUnit *_pPreferredUnit )
{
	return new CFormationResupplyUnitState( _pUnit, _pPreferredUnit );
}

CFormationResupplyUnitState::CFormationResupplyUnitState( class CFormation *pUnit, class CAIUnit *_pPreferredUnit )
: pUnit( pUnit ), vPointInQuestion( VNULL2 ), CFormationServeUnitState( _pPreferredUnit ), bNearTruck ( true )
{
}

void CFormationResupplyUnitState::Segment()
{
	// реакция на смещение юнита
	if( EFRUS_WAIT_FOR_HOME_TRANSPORT != eState )
	{
		if ( !IsValidObj( pUnitInQuiestion ) ||//реакция на смерть юнита
					vPointInQuestion != pUnitInQuiestion->GetCenterPlain() // да пошел! еще бегать за ним.
				) 
		{
			eState = EFRUS_FIND_UNIT_TO_SERVE;
		}
		if ( !IsValidObj( pHomeTransport ) )
			TryInterruptState( 0 );
	}
	
	switch ( eState )
	{
	case EFRUS_WAIT_FOR_HOME_TRANSPORT:
		if ( pUnit->GetNextCommand() )
			pUnit->SetCommandFinished();		

		break;
	case EFRUS_WAIT_FOR_UNIT_TO_SERVE:
		if ( !bNearTruck )		// go back to truck
		{
			Interrupt();
			break;
		}
		if ( curTime - lastResupplyTime > 3000 )
			eState = EFRUS_FIND_UNIT_TO_SERVE;

		break;
	case EFRUS_FIND_UNIT_TO_SERVE:
		{
			CFormationServeUnitState::CFindBestUnitPredicate pred;
			FindUnitToServe( pHomeTransport->GetCenterPlain(), pHomeTransport->GetPlayer(), fWorkLeft, pUnit, &pred, pPreferredUnit );
			if ( pred.HasUnit() )
			{
				pUnitInQuiestion = pred.GetUnit();
				vPointInQuestion = pUnitInQuiestion->GetCenterPlain();
//				EUnitRPGType type = pUnitInQuiestion->GetStats()->etype;
				CPtr<CSoldier> pSold;
				if (	::IsInfantry( pUnitInQuiestion->GetStats()->etype ) &&
							( pSold = static_cast_ptr<CSoldier*>(pUnitInQuiestion) ) &&
							( pSquadInQuestion = pSold->GetFormation()) )
				{
					pUnitInQuiestion = (*pSquadInQuestion)[0];
					vPointInQuestion = pUnitInQuiestion->GetCenterPlain();
					iCurUnitInFormation = 0;
				}
				eState = EFRUS_START_APPROACH;
			}
			else if ( pred.IsNotEnoughRu() && SConsts::ENGINEER_RU_CARRY_WEIGHT == fWorkLeft )
			{
				NI_ASSERT( false, "RESUPPLY SQUAD: cannot carry 1(!) shell, please adjust shell price or ENGINEER_RU_CARRY_WEIGHT" );
			}
			else if ( (pred.IsNotEnoughRu() || !bNearTruck ) && fWorkLeft != pHomeTransport->GetResursUnitsLeft() )
			{
				Interrupt();
			}
			else
			{
				lastResupplyTime = curTime;
				eState = EFRUS_WAIT_FOR_UNIT_TO_SERVE;
			}
		}

		break;
	case EFRUS_START_APPROACH:
		{
			CPtr<IStaticPath> pStaticPath = CreateStaticPathToPoint( pUnitInQuiestion->GetCenterPlain(), VNULL2, pUnit, true, GetAIMap() );
			if( pStaticPath && pStaticPath->GetLength() * GetAIMap()->GetTileSize() < 3 * SConsts::TRANSPORT_RESUPPLY_OFFSET )
				pUnit->SendAlongPath( pStaticPath, VNULL2, true );
			else
			{
				SRect unitRect = pUnitInQuiestion->GetUnitRect();

				for ( int i = 0; i < pUnit->Size(); ++i )
				{
					CVec2 vPoint;
					if ( !GetRectBeamIntersection( &vPoint, (*pUnit)[i]->GetCenterPlain(), pUnitInQuiestion->GetCenterPlain() - (*pUnit)[i]->GetCenterPlain(), unitRect ) )
						vPoint = pUnitInQuiestion->GetCenterPlain();
					CPtr<CArtilleryCrewPath> pPath = new CArtilleryCrewPath( (*pUnit)[i], (*pUnit)[i]->GetCenterPlain(), vPoint, (*pUnit)[i]->GetMaxPossibleSpeed() );
					(*pUnit)[i]->SetSmoothPath( pPath );
				}
			}

			bNearTruck = false;
			eState = EFRUS_APPROACHING;
		}

		break;
	case EFRUS_APPROACHING:
		if ( pUnit->IsIdle() )
		{
			eState = EFRUS_START_SERVICE;
			for ( int i = 0; i < pUnit->Size(); ++i )
				(*pUnit)[i]->RestoreSmoothPath();
		}

		break;
/*
	case EFRUS_START_APPROACH:
		{
			CPtr<IStaticPath> pStaticPath = CreateStaticPathToPoint( pUnitInQuiestion->GetCenterPlain(), VNULL2, pUnit, true, GetAIMap() );
			if ( pStaticPath )
			{
				if ( IsUnitNearPoint( pUnitInQuiestion, pStaticPath->GetFinishPoint(), SConsts::TILE_SIZE * 5 ) )
				{
					bNearTruck = false;
					pUnit->SendAlongPath( pStaticPath, VNULL2, true );
					eState = EFRUS_APPROACHING;
				}
			//CRAP{ IN CHOOSING - PATH REMOVED, SO REMOVE CHECKING THERE
			//	else
				//	Interrupt();
			}
			//else
				//Interrupt();
			eState = EFRUS_APPROACHING;
			//CRAP}
		}

		break;
	case EFRUS_APPROACHING:
		if ( pUnit->IsIdle() )
			eState = EFRUS_START_SERVICE;

		break;
*/
	case EFRUS_START_SERVICE:
		for ( int i = 0; i < pUnit->Size(); ++i )
			theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_COMMAND_USE, (float)ACTION_NOTIFY_USE_UP ), (*pUnit)[i] );

		bSayAck = false;
		for ( int i = 0; i < pUnitInQuiestion->GetNCommonGuns(); ++ i )
		{
			if ( pUnitInQuiestion->GetNAmmo( i ) == 0 )
			{
				bSayAck = true;
				break;
			}
		}
		eState = EFRUS_SERVICING;
		fWorkAccumulator = 0;
		lastResupplyTime = curTime ;

		break;
	case EFRUS_SERVICING:
		if ( curTime - lastResupplyTime > SConsts::TIME_QUANT )
		{
			//такую работу произвели инженеры
			fWorkAccumulator += pUnit->Size() * 
													SConsts::RU_PER_QUANT * 
													(curTime - lastResupplyTime) / SConsts::TIME_QUANT;
			fWorkAccumulator = (std::min)( fWorkAccumulator, fWorkLeft );
			lastResupplyTime = curTime ;
			float fWorkPerformed = 0;
			int nGunsResupplied = 0;
			//resupply each gun
			float min1AmmoCost = 0;

			for ( int i = 0; i < pUnitInQuiestion->GetNCommonGuns(); ++i )
			{
				const SBaseGunRPGStats &rStats = pUnitInQuiestion->GetCommonGunStats( i );
				int iAmmoNeeded = rStats.nAmmo - pUnitInQuiestion->GetNAmmo( i );
				if ( 0 != iAmmoNeeded )
				{//need resupply
					min1AmmoCost = ( min1AmmoCost == 0 || rStats.fReloadCost < min1AmmoCost ) ? rStats.fReloadCost : min1AmmoCost;
					int nAmmoToAdd = (std::min)( iAmmoNeeded, int( fWorkAccumulator / rStats.fReloadCost ) );
					float fWorkNeeded = (std::min)( iAmmoNeeded * rStats.fReloadCost, nAmmoToAdd * rStats.fReloadCost );
					if ( fWorkNeeded == 0 )
					{
						continue;
					}
					pUnitInQuiestion->ChangeAmmo( i, nAmmoToAdd );
					fWorkPerformed += fWorkNeeded;
					fWorkAccumulator -= fWorkNeeded;
				}
				else
					++nGunsResupplied;
			}
			// забрать ресурсы из транспорта
			pHomeTransport->DecResursUnitsLeft( fWorkPerformed );
			fWorkLeft -= fWorkPerformed;

			if ( nGunsResupplied == pUnitInQuiestion->GetNCommonGuns() || // все перезарядили
					 min1AmmoCost > fWorkLeft )// в транспорте больше не осталось ресурсов
			{
				if ( nGunsResupplied == pUnitInQuiestion->GetNCommonGuns() )
				{
					theSupremeBeing.UnitAskedForResupply( pUnitInQuiestion, ERT_RESUPPLY, false );
					theFeedBackSystem.RemoveFeedback( pUnitInQuiestion->GetUniqueID(), EFB_NO_AMMO );
				}

				// если перезаряжали Squad, то взять следующего солдата
				if ( pSquadInQuestion && ++iCurUnitInFormation < pSquadInQuestion->Size() )
				{
					pUnitInQuiestion = (*pSquadInQuestion)[iCurUnitInFormation];
					vPointInQuestion = pUnitInQuiestion->GetCenterPlain();
				}
				else
					eState = EFRUS_FIND_UNIT_TO_SERVE;
			}
		}

		break;
	}
}

bool CFormationResupplyUnitState::CheckUnit( CAIUnit *pU, CFormationServeUnitState::CFindUnitPredicate * pPred, const float fResurs, const CVec2 &vCenter )
{
	if ( pU && pU->IsRefValid() && pU->IsAlive() &&
			/*pU->GetPlayer() == pU->GetPlayer() &&*/ 
			pU->IsIdle() && pU->IsFree() &&
			fabs2( pU->GetCenterPlain() - vCenter ) < sqr( SConsts::RESUPPLY_RADIUS ) )
	{
//			const EUnitRPGType type = pU->GetStats()->etype;

		//CRAP{ OTKAZALSA OT PUTI, TORMOZILO
		//CPtr<IStaticPath> pStaticPath = CreateStaticPathToPoint( pU->GetCenter(), VNULL2, pLoaderSquad, true );
		//SRect unitRect = pU->GetUnitRect();
		//unitRect.Compress( 1.2f );
		//if ( pStaticPath && unitRect.IsPointInside( pStaticPath->GetFinishPoint() ) )
		//CRAP}
		{
			float fWorkPresent = 0; //в рублях стоимость снарядов у юнита
			float fWorkTotal = 0;		// в рублях стоимость максимального боезапаса
			float fMinReloadCost = 0;
			for ( int i = 0; i < pU->GetNCommonGuns(); ++i )
			{
				const SBaseGunRPGStats& rStats = pU->GetCommonGunStats( i );
				int iAmmoPresent = pU->GetNAmmo( i );
				fWorkPresent += iAmmoPresent * rStats.fReloadCost;
				fWorkTotal += rStats.nAmmo * rStats.fReloadCost;
				if ( iAmmoPresent != rStats.nAmmo )
					fMinReloadCost = ( (fMinReloadCost == 0 || fMinReloadCost > rStats.fReloadCost) ) ? rStats.fReloadCost : fMinReloadCost;
			}
			//find unit with lowest ammo percentage.
			const float curAmmo = fWorkTotal - fWorkPresent;
			if (  curAmmo > 0.0f )  // юниту нужны патроны
			{
				if ( fMinReloadCost != 0  && fMinReloadCost > fResurs ) // ne достаточно ресурсов, чтобы дать хотя-бы 1 патрон
					pPred->SetNotEnoughRu();
				//CRAP{ OTKAZALSA OT PUTI, TORMOZILO
				/*else if ( pPred->SetUnit( pU, curAmmo, pStaticPath->GetLength()) )
					return;*/
				//CRAP}
				else if ( pPred->SetUnit( pU, curAmmo, fabs( pU->GetCenterPlain() - vCenter ) ) )
					return true;
			}
		}			
	}
	return false;
}

void CFormationResupplyUnitState::FindUnitToServe( const CVec2 &vCenter, 
																												int nPlayer, 
																												const float fResurs, 
																												CCommonUnit * pLoaderSquad,  
																												CFormationServeUnitState::CFindUnitPredicate * pPred, 
																												CAIUnit *_pPreferredUnit )
{
	// first - check prefered unit
	if ( CheckUnit( _pPreferredUnit, pPred, fResurs, vCenter ) || pPred->HasUnit() )
		return;

	CUnitsIter<0,2> iter( theDipl.GetNParty(nPlayer), EDI_FRIEND, vCenter, SConsts::RESUPPLY_RADIUS );

	//найти юнит, наиболее требующий перезарядки
	while ( !iter.IsFinished() )
	{
		if ( CheckUnit( (*iter), pPred, fResurs, vCenter ) )
			return;
		iter.Iterate();
	}
}

void CFormationResupplyUnitState::Interrupt()
{
	if ( !pUnit->IsIdle() )
		pUnit->Stop();
	for ( int i = 0; i < pUnit->Size(); ++i )
		(*pUnit)[i]->RestoreSmoothPath();
	for ( int i = 0; i < pUnit->Size(); ++i )
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_STOP_THIS_ACTION), (*pUnit)[i], false );

	pUnit->SetCommandFinished();
}

ETryStateInterruptResult CFormationResupplyUnitState::TryInterruptState( class CAICommand *pCommand )
{
	if ( !pCommand || pCommand->ToUnitCmd().nCmdType == ACTION_MOVE_CATCH_TRANSPORT )
	{
		Interrupt();
		return TSIR_YES_IMMIDIATELY;
	}
	else if ( 
				pCommand->ToUnitCmd().nCmdType == ACTION_MOVE_SET_HOME_TRANSPORT ||
				pCommand->ToUnitCmd().nCmdType == ACTION_COMMAND_LOAD )
	{
		Interrupt();
		return TSIR_YES_IMMIDIATELY;
	}
	return TSIR_NO_COMMAND_INCOMPATIBLE;
}


//*******************************************************************
//*										CFormationLoadRuState*
//*******************************************************************

IUnitState* CFormationLoadRuState::Instance( class CFormation *pUnit, class CBuilding *pStorage)
{
	return new CFormationLoadRuState( pUnit, pStorage );
}

CFormationLoadRuState::CFormationLoadRuState( class CFormation *pUnit, class CBuilding *pStorage)
: pUnit( pUnit ), pStorage( pStorage ), nEntrance( -1 ), CFormationServeUnitState( 0 )
{
}

void CFormationLoadRuState::Segment()
{
	//реакция на смерть склада
	if ( !IsValidObj( pStorage ) )
	{
		Interrupt();
		return;
	}

	switch ( eState )
	{
	case EFRUS_WAIT_FOR_HOME_TRANSPORT:
		if ( pUnit->GetNextCommand() )
			pUnit->SetCommandFinished();		

		break;
	case EFRUS_FIND_UNIT_TO_SERVE:
		eState = EFRUS_START_APPROACH;
		break;
	case EFRUS_START_APPROACH:
		{
			nEntrance = CFormationRepairBuildingState::SendToNearestEntrance( pUnit, pStorage );
			if ( -1 != nEntrance )
				eState = EFRUS_APPROACHING;
			else
				Interrupt();
		}

		break;
	case EFRUS_APPROACHING:
		if ( pUnit->IsIdle() )
		{
			const CVec2 vEntrance = pStorage->GetEntrancePoint( nEntrance );
			for ( int i = 0; i < pUnit->Size(); ++i )
			{
				RecordRandomCall();
				const CVec2 vDestPoint = vEntrance + GetVectorByDirection( NRandom::Random(0,65535) ) * 32.0f;
				CPtr<IStaticPath> pStaticPath = CreateStaticPathToPoint( vDestPoint, VNULL2, (*pUnit)[i], true, GetAIMap() );
				if ( pStaticPath )
					(*pUnit)[i]->SendAlongPath( pStaticPath, VNULL2, true );
				else
				{
					CPtr<IPath> pPath = new CStandartDirPath( (*pUnit)[i]->GetCenterPlain(), vDestPoint, GetAIMap()->GetTileSize() );
					(*pUnit)[i]->SendAlongPath( pPath );
				}
			}
			eState = EFRUS_APPROACHING2;
		}

		break;
	case EFRUS_APPROACHING2:
		{
			bool bAllIdle = true;
			for ( int i = 0; i < pUnit->Size(); ++i )
				if ( !(*pUnit)[i]->IsIdle() )
					bAllIdle = false;
			if ( bAllIdle )
			{
				const CVec2 vEntrance = pStorage->GetEntrancePoint( nEntrance );
				for ( int i = 0; i < pUnit->Size(); ++i )
					(*pUnit)[i]->TurnToTarget( vEntrance );
				eState = EFRUS_START_SERVICE;
			}
		}

		break;
	case EFRUS_START_SERVICE:
		for ( int i = 0; i < pUnit->Size(); ++i )
			theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_COMMAND_USE, (float)ACTION_NOTIFY_USE_UP ), (*pUnit)[i] );

		eState = EFRUS_SERVICING;
		fWorkAccumulator =0;
		lastResupplyTime = curTime ;

		break;
	case EFRUS_SERVICING:
		if ( curTime - lastResupplyTime > SConsts::TIME_QUANT )
		{
			//такую работу произвели инженеры
			fWorkAccumulator += pUnit->Size() * 
													SConsts::RU_PER_QUANT * 
													(curTime - lastResupplyTime)/SConsts::TIME_QUANT;
			lastResupplyTime = curTime ;
			
			if ( fWorkAccumulator > SConsts::ENGINEER_RU_CARRY_WEIGHT )
				Interrupt();
		}
		break;
	}
}

void CFormationLoadRuState::Interrupt()
{
	pUnit->SetCommandFinished();
}

ETryStateInterruptResult CFormationLoadRuState::TryInterruptState( class CAICommand *pCommand )
{
	if ( !pCommand )
	{
		Interrupt();
		return TSIR_YES_IMMIDIATELY;
	}
	else if ( pCommand->ToUnitCmd().nCmdType == ACTION_MOVE_CATCH_TRANSPORT )
	{
		pCommand->ToUnitCmd().fNumber = fWorkAccumulator/pUnit->Size();
		Interrupt();
		return TSIR_YES_IMMIDIATELY;
	}
	else if ( pCommand->ToUnitCmd().nCmdType == ACTION_MOVE_SET_HOME_TRANSPORT ||
						pCommand->ToUnitCmd().nCmdType == ACTION_COMMAND_LOAD )
	{
		return TSIR_YES_WAIT;
	}
	return TSIR_NO_COMMAND_INCOMPATIBLE;

}

//*										CFormationPlaceAntitankState*
//*******************************************************************

IUnitState* CFormationPlaceAntitankState::Instance( class CFormation *_pUnit, const CVec2 &vDesiredPoint )
{
	return new CFormationPlaceAntitankState( _pUnit, vDesiredPoint );
}

CFormationPlaceAntitankState::CFormationPlaceAntitankState( class CFormation *_pUnit, const CVec2 &vDesiredPoint )
: pUnit(_pUnit), eState( FPAS_WAIT_FOR_HOMETRANSPORT), vDesiredPoint( vDesiredPoint )
{
}

void CFormationPlaceAntitankState::SetHomeTransport( class CAITransportUnit *pTransport )
{
	NI_ASSERT( eState == FPAS_WAIT_FOR_HOMETRANSPORT, "wrong state" );
	eState = FPAS_ESITMATING ;
	pHomeTransport = pTransport;
}

void CFormationPlaceAntitankState::Segment()
{

	if ( !IsValidObj( pHomeTransport ) )
		TryInterruptState( 0 );

	switch( eState )
	{
	case FPAS_WAIT_FOR_HOMETRANSPORT:
		if ( pUnit->GetNextCommand() )
			pUnit->SetCommandFinished();

		break;
	case FPAS_ESITMATING:
		if ( fabs2( pUnit->GetCenterPlain() - vDesiredPoint) > SConsts::TILE_SIZE*SConsts::TILE_SIZE )
		{
			CPtr<IStaticPath> pPath = CreateStaticPathToPoint( vDesiredPoint, VNULL2, pUnit, true, GetAIMap() );
			if ( pPath )
			{
				pUnit->SendAlongPath( pPath, VNULL2, true );
				eState = FPAS_APPROACHING;
			}
			else
			{
				pUnit->SendAcknowledgement( ACK_NEGATIVE_NOTIFICATION );
				TryInterruptState( 0 );
			}
		}
		else
			eState = FPAS_START_BUILD;

		break;
	case FPAS_APPROACHING:
		if ( fabs2(pUnit->GetCenterPlain() - vDesiredPoint) < SConsts::TILE_SIZE*SConsts::TILE_SIZE )
		{
			const int nSize = pUnit->Size();
			for ( int i = 0; i < nSize; ++i )
			{
				RecordRandomCall();
				const CVec2 vShift( GetVectorByDirection( uint16_t(NRandom::Random(0, 65535) ) * SConsts::TILE_SIZE ) );
				CPtr<IStaticPath> pPath = CreateStaticPathToPoint( vDesiredPoint, vShift, (*pUnit)[i], true, GetAIMap() );
				if ( pPath )
					(*pUnit)[i]->SendAlongPath( pPath, vShift, true );
			}
			eState = FPAS_APPROACHING_2;
		}
		else if ( pUnit->IsIdle() )
		{
			pUnit->SendAcknowledgement( ACK_NEGATIVE_NOTIFICATION );
			TryInterruptState( 0 );
		}

		break;
	case FPAS_APPROACHING_2:
		{
			bool bEveryIsIdle = true;
			for ( int i = 0; i < pUnit->Size(); ++i )
			{
				if ( !(*pUnit)[i]->IsIdle() )
				{
					bEveryIsIdle = false;
					break;
				}
			}
			if ( bEveryIsIdle )
				eState = FPAS_START_BUILD;
		}

		break;
	case FPAS_START_BUILD:
		{
			pUnit->Stop();
			eState = FPAS_START_BUILD_2;
		}

		break;
	case FPAS_START_BUILD_2:
		{
			for ( int i=0; i < pUnit->Size(); ++i )
				theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_COMMAND_USE, (float)ACTION_NOTIFY_USE_UP ), (*pUnit)[i] );

			fWorkAccumulator = 0.0f;
			eState = FPAS_BUILDING;
			timeBuild = curTime;

			const SObjectBaseRPGStats * pStats = theUnitCreation.GetRandomAntitankObject();

			if ( CGivenPassabilityStObject::CheckStaticObject( pStats, vDesiredPoint, 0, 0 ) )
			{
				pAntitank = new CSimpleStaticObject( pStats, CVec3(vDesiredPoint,0), 0, pStats->fMaxHP, 0, ESOT_COMMON, pUnit->GetPlayer(), true );
				pAntitank->Mem2UniqueIdObjs();
				pAntitank->Init();
				pAntitank->LockTiles();
				eState = FPAS_BUILDING;
			}
			else
			{
				pUnit->SendAcknowledgement( ACK_NEGATIVE_NOTIFICATION );
				TryInterruptState( 0 );
			}
		}

		break;
	case FPAS_BUILDING:
		if ( curTime - timeBuild > SConsts::TIME_QUANT )
		{
			fWorkAccumulator += SConsts::RU_PER_QUANT * ( curTime - timeBuild ) / SConsts::TIME_QUANT;
			timeBuild = curTime;

			if ( fWorkAccumulator >= pAntitank->GetHitPoints() )
			{
				CObstacleStaticObject *pObstacle = new CObstacleStaticObject( pAntitank );
				theStatObjs.AddStaticObject( pAntitank, true );
				theStatObjs.AddObstacle( pObstacle );
				pUnit->SendAcknowledgement( ACK_BUILDING_FINISHED, true );

				pHomeTransport->DecResursUnitsLeft( SConsts::ANTITANK_RU_PRICE );

				pUnit->SetCommandFinished();
			}
		}

		break;
	}
}

ETryStateInterruptResult CFormationPlaceAntitankState::TryInterruptState( class CAICommand *pCommand )
{
	pUnit->SetCommandFinished();

	if ( pAntitank )
		pAntitank->UnlockTiles();
	return TSIR_YES_IMMIDIATELY;
}

//*******************************************************************
//*										CFormationBuildLongObjectState*
//*******************************************************************

IUnitState * CFormationBuildLongObjectState::Instance( class CFormation *pUnit, class CLongObjectCreation *pCreation  )
{
	return new CFormationBuildLongObjectState( pUnit, pCreation );
}

CFormationBuildLongObjectState::CFormationBuildLongObjectState( class CFormation *pUnit, class CLongObjectCreation *pCreation )
: pUnit(pUnit), 
	eState(ETBS_WAITING_FOR_HOMETRANSPORT), 
	lastTime(curTime), 
	pCreation( pCreation ),
	fCompletion( 0 )
{
	nCurrentSegment = pCreation->GetCurIndex();
}

void CFormationBuildLongObjectState::SetHomeTransport( class CAITransportUnit *pTransport )
{
	NI_ASSERT( eState == ETBS_WAITING_FOR_HOMETRANSPORT, "wrong state" );
	eState = FBFS_READY_TO_START;
	pHomeTransport = pTransport;
	fWorkLeft = (std::min)( SConsts::ENGINEER_RU_CARRY_WEIGHT, pTransport->GetResursUnitsLeft() );
}

ETryStateInterruptResult CFormationBuildLongObjectState::TryInterruptState( CAICommand *pCommand )
{
	pUnit->SetCommandFinished();
	for ( int i = 0; i < pUnit->Size(); ++i )
		(*pUnit)[i]->RestoreSmoothPath();

	return TSIR_YES_IMMIDIATELY;
}

void CFormationBuildLongObjectState::Segment()
{
	switch ( eState )
	{
	case ETBS_WAITING_FOR_HOMETRANSPORT:
		if ( pUnit->GetNextCommand() )
			pUnit->SetCommandFinished();

		break;
	case FBFS_READY_TO_START:
		{
			if ( pCreation->GetCurIndex() != 0 )
			{
				const CVec3 vCenter( pUnit->GetCenter() );
				if ( fabs(CVec2(vCenter.x, vCenter.y) - pCreation->GetNextPoint( 0, 1 )) > SConsts::TILE_SIZE * 5 )
				{
					CPtr<IStaticPath> pStaticPath = CreateStaticPathToPoint( pCreation->GetNextPoint(0,1), VNULL2, pUnit, true, GetAIMap() );

					if ( pStaticPath )
					{
						pUnit->SendAlongPath( pStaticPath, VNULL2, true );
						eState = FBFS_APPROACHING_STARTPOINT;
					}
				}
				eState = FBFS_APPROACHING_STARTPOINT;
			}
			else if ( pCreation->GetMaxIndex() != 0 )
			{
				CPtr<IStaticPath> pStaticPath = CreateStaticPathToPoint( pCreation->GetNextPoint(0,1), VNULL2, pUnit, true, GetAIMap() );

				if ( pStaticPath )
				{
					pUnit->SendAlongPath( pStaticPath, VNULL2, true );
					eState = FBFS_APPROACHING_STARTPOINT;
				}
			}
			if ( FBFS_READY_TO_START == eState )
			{
				pUnit->SendAcknowledgement( ACK_NEGATIVE );
				TryInterruptState( 0 );
			}
		}
		
		break;
	case FBFS_APPROACHING_STARTPOINT:
		if ( pUnit->IsIdle() )
		{
			eState = FBFS_NEXT_SEGMENT;
			const int nSize = pUnit->Size();
			for ( int i = 0; i < nSize; ++i )
			{
				theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_IDLE), (*pUnit)[i], false );
				(*pUnit)[i]->Stop();
			}
		}

		break;
	case FBFS_BUILD_SEGMENT:
		if ( pCreation->GetCurIndex() != nCurrentSegment || pCreation->GetCurIndex() >= pCreation->GetMaxIndex() )
		{
			// someone build this segment already
			eState = FBFS_NEXT_SEGMENT;
		}
		else if ( curTime - lastTime > SConsts::TIME_QUANT )
		{
			const float fAddWork = (std::min)( 1.0f * pUnit->Size() *
				SConsts::RU_PER_QUANT *
														float( curTime - lastTime ) / SConsts::TIME_QUANT,
														fWorkLeft );
			lastTime = curTime;
			float fDecResource = fAddWork;
			// work finished
			if ( fAddWork + pCreation->GetWorkDone() >= pCreation->GetPrice() )
			{
				fDecResource = pCreation->GetPrice() - pCreation->GetWorkDone();
				pCreation->BuildNext();
				eState = FBFS_NEXT_SEGMENT;
			}
			else 
				pCreation->AddWork( fAddWork );

			pHomeTransport->DecResursUnitsLeft( fDecResource );
			fWorkLeft -= fDecResource;

			if ( 0.0f == fWorkLeft )
				pUnit->SetCommandFinished();
		}

		break;
	case FBFS_NEXT_SEGMENT:
		// если нужно еще строить, то FBFS_ADVANCE_TO_SEGMENT
		if ( pCreation->GetCurIndex() < pCreation->GetMaxIndex() )
		{
			if ( fWorkLeft == 0 )
			{
				pUnit->SetCommandFinished();
			}
			else if ( pCreation->CanBuildNext() )
			{
				eState = FBFS_CHECK_FOR_UNITS_PREVENTING;
				pCreation->LockNext();
			}
			else
			{
				pCreation->LockCannotBuild();
				eState = FBFS_CANNOT_BUILD_ANYMORE;
			}
		}
		else
		{
			if ( pCreation->CannotFinish() )
				pUnit->SendAcknowledgement( ACK_CANNOT_FINISH_BUILD, true );
			else
				pUnit->SendAcknowledgement( ACK_BUILDING_FINISHED, true );
			pUnit->SetCommandFinished();
		}

		break;
	case FBFS_CANNOT_BUILD_ANYMORE:
		pCreation->LockCannotBuild();
		pUnit->SetCommandFinished();
		pUnit->SendAcknowledgement( ACK_CANNOT_FINISH_BUILD, true );
		pUnit->SetCenter( GetHeights()->Get3DPoint( (*pUnit)[0]->GetCenterPlain() ) );

		break;
	case FBFS_CHECK_FOR_UNITS_PREVENTING:
		{
			pCreation->GetUnitsPreventing( &unitsPreventing );
			SendUnitsAway( &unitsPreventing );
			eState = FBFS_WAIT_FOR_UNITS;
			lastTime = curTime;
		}

		break;
	case FBFS_WAIT_FOR_UNITS:
		// дождаться, когда все мешающие уедут
		if ( !pCreation->IsAnyUnitPrevent() )
			eState = FBFS_START_APPROACH_SEGMENT;
		else if ( curTime - lastTime > 15000 ) // ждали достаточно
			eState = FBFS_CANNOT_BUILD_ANYMORE;

		break;
	case FBFS_START_APPROACH_SEGMENT:
		{
			const int nSize = pUnit->Size();
			CVec2 vNewFormationCenter;
			for ( int i = 0; i < nSize; ++i )
			{
				CAIUnit * pSoldier= (*pUnit)[i];
				vNewFormationCenter = pCreation->GetNextPoint( i, nSize );
				if ( pCreation->IsCheatPath() )
				{
					pSoldier->SetSmoothPath( new CArtilleryCrewPath( pSoldier, pSoldier->GetCenterPlain(), vNewFormationCenter, pSoldier->GetMaxPossibleSpeed()/2 ) );
				}
				else
				{
					CPtr<IStaticPath> pStaticPath = CreateStaticPathToPoint( vNewFormationCenter, VNULL2, pSoldier, true, GetAIMap() );
					if ( pStaticPath )
						pSoldier->SendAlongPath( pStaticPath, VNULL2, true );
					else
					{
						CPtr<IPath> pPath = new CStandartDirPath( pSoldier->GetCenterPlain(), vNewFormationCenter, GetAIMap()->GetTileSize() );
						pSoldier->SendAlongPath( pPath );
					}
				}
			}
			pUnit->SetCenter( GetHeights()->Get3DPoint( vNewFormationCenter ) );
			lastTime = curTime;
			eState = FBFS_APPROACH_SEGMENT;
		}

		break;
	case FBFS_APPROACH_SEGMENT:
		{
			bool bEveryIsOnPlace = true;
			for ( int i = 0; i < pUnit->Size(); ++i )
			{
				CAIUnit * pSold = (*pUnit)[i];
				if ( !pSold->IsIdle() )
				{
					bEveryIsOnPlace = false;
					break;
				}
			}
			if ( bEveryIsOnPlace ) 
			{
				eState = FBFS_BUILD_SEGMENT;
				nCurrentSegment = pCreation->GetCurIndex();
				lastTime = curTime;
				fCompletion = 0;
				for ( int i = 0; i < pUnit->Size(); ++i )
				{
					CAIUnit *pSoldier = (*pUnit)[i];
					pSoldier->RestoreSmoothPath();
					theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_COMMAND_USE, (float)ACTION_NOTIFY_USE_UP ), pSoldier );
				}
			}
		}
		break;
	}
}

void CFormationBuildLongObjectState::SendUnitsAway( std::list<CPtr<CAIUnit> > *pUnitsPreventing )
{
	CLine2 line = pCreation->GetCurLine();
	CVec2 vAway( line.a, line.b );
	Normalize( &vAway );
	for ( std::list<CPtr<CAIUnit> >::iterator it = pUnitsPreventing->begin(); it != pUnitsPreventing->end(); ++it )
	{
		CAIUnit * pUnit = *it;
		int nSign = - line.GetSign( pUnit->GetCenterPlain() );
		nSign = nSign != 0 ? nSign : 1;
		const CVec2 vTo = pUnit->GetCenterPlain() + 
					pUnit->GetBoundTileRadius() * 10 * SConsts::TILE_SIZE * nSign * vAway;
		theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_COMMAND_MOVE_TO, vTo ), pUnit );
	}
	pUnitsPreventing->clear();
}


//*******************************************************************
//*										CFormationBuildEntrenchmentState*
//*******************************************************************

IUnitState * CFormationBuildEntrenchmentState::Instance( CFormation *pUnit, CLongObjectCreation *pCreation, const CVec2 &vStartPoint )
{
	return new CFormationBuildEntrenchmentState( pUnit, pCreation, vStartPoint );
}

CFormationBuildEntrenchmentState::CFormationBuildEntrenchmentState( CFormation *pUnit, CLongObjectCreation *_pCreation, const CVec2 &_vStartPoint )
: pFormation( pUnit ), eState( EBES_START_APPROACHING ), lastTime( curTime ), fCompletion( 0.0f ), bEndPointSelected( false ),
vStartPoint( _vStartPoint ) 
{
	DebugTrace( ">>>> CFormationBuildEntrenchmentState::CFormationBuildEntrenchmentState( %d, ..., %2.3f x %2.3f )", pUnit->GetUniqueID(), _vStartPoint.x, _vStartPoint.y );
	pCreation = checked_cast<CEntrenchmentCreation*>( _pCreation );
}

ETryStateInterruptResult CFormationBuildEntrenchmentState::TryInterruptState( CAICommand *pCommand )
{
	pFormation->BalanceCenter();
	pFormation->SetCommandFinished();
	for ( int i = 0; i < pFormation->Size(); ++i )
		(*pFormation)[i]->RestoreSmoothPath();

	return TSIR_YES_IMMIDIATELY;
}

const int nBuildersPerSegment = 3;
void CFormationBuildEntrenchmentState::SetEndPoint( const CVec2 &vPos )
{
	vEndPoint = vPos;
	CObj<CLongObjectCreation> pLongObjectCreation = static_cast<CLongObjectCreation*>( pCreation.GetPtr() );
	NLongObjectCreation::PreCreate<CEntrenchmentCreation>( vPos, &pLongObjectCreation, true );
	pCreation = checked_cast<CEntrenchmentCreation*>( pLongObjectCreation.GetPtr() );
	if ( pCreation->GetCurIndex() < 0 )
	{
		TryInterruptState( 0 );
		return;
	}
	bEndPointSelected = true;
	const int nSize = pFormation->Size();
	nMaxIndex = (std::min)( pCreation->GetMaxIndex(), pCreation->GetCurIndex() + ( nSize + 1 ) / nBuildersPerSegment );
}

void CFormationBuildEntrenchmentState::Segment()
{
	if ( !bEndPointSelected )
	{
		if ( pFormation->GetNextCommand() )
			pFormation->SetCommandFinished();
		return;
	}
	switch ( eState )
	{
	case EBES_START_APPROACHING:
		{
			if ( nMaxIndex == 0 )
			{
				TryInterruptState( 0 );
				return;
			}
			for ( int i = pCreation->GetCurIndex(); i < nMaxIndex; ++i )
			{
				if ( !pCreation->CanBuildByIndexSlow( i ) )
				{
					nMaxIndex = i;
					if ( nMaxIndex == pCreation->GetCurIndex() )
					{
						TryInterruptState( 0 );
            return;
					}
					break;
				}
			}
			const int nSize = pFormation->Size();
			for ( int i = 0; i < nSize; ++i )
			{
				CSoldier* pSoldier = (*pFormation)[i];
				const int nIndex = ( ( i / nBuildersPerSegment + pCreation->GetCurIndex() ) >= nMaxIndex ) ?
					( nMaxIndex - 1 ) : ( i / nBuildersPerSegment + pCreation->GetCurIndex() );
				const bool bIsLastSegment = nIndex == nMaxIndex - 1;
				int nBuilders = bIsLastSegment ? 
					( nSize - nBuildersPerSegment * ( nMaxIndex - 1 - pCreation->GetCurIndex() ) ) : nBuildersPerSegment;
				const int nMyNumber = bIsLastSegment ? 
					( i - nBuildersPerSegment * ( nMaxIndex - 1 - pCreation->GetCurIndex() ) ) : ( i % nBuildersPerSegment );
				NI_VERIFY( nBuilders != 0, "Error in formula for number of builders", nBuilders = nBuildersPerSegment );
				const CVec2 vTargetPoint = pCreation->GetBuildPointForIndex( nMyNumber, nBuilders, nIndex );
				targetPoints[pSoldier->GetUniqueID()] = vTargetPoint;
				pSoldier->UnitCommand( new CAICommand( SAIUnitCmd( ACTION_COMMAND_MOVE_TO, vTargetPoint ) ), false, false );
				pSoldier->UnitCommand( new CAICommand( SAIUnitCmd( ACTION_COMMAND_USE, (float)ACTION_NOTIFY_USE_DOWN ) ), true, true );
			}
			eState = EBES_CHECK_FOR_UNITS_PREVENTING;
			break;
		}
	case EBES_CHECK_FOR_UNITS_PREVENTING:
		{
			for ( int i = pCreation->GetCurIndex(); i < nMaxIndex; ++i )
			{
				std::list<CPtr<CAIUnit> > unitsPreventing;
				pCreation->GetUnitsPreventingByIndex( &unitsPreventing, i );
				if ( !unitsPreventing.empty() )
					SendUnitsAway( &unitsPreventing );
			}
			eState = EBES_APPROACHING_BUILDPOINT;
			break;
		}
	case EBES_APPROACHING_BUILDPOINT:
		{
			bool bAllUnitsOnPlace = true;
			const int nSize = pFormation->Size();
      for ( int i = 0; i < nSize; ++i )
			{
				CSoldier* pSoldier = (*pFormation)[i];
				if ( pSoldier->GetState()->GetName() == EUSN_USE )
				{
					const float fDist2 = fabs2( targetPoints[pSoldier->GetUniqueID()] - pSoldier->GetCenterPlain() );
					const float fMaxRadius2 = fabs2( 4 * SConsts::TILE_SIZE );
					if ( fDist2 > fMaxRadius2 )
					{
						TryInterruptState( 0 );
						return;
					}
				}
				else
					bAllUnitsOnPlace = false;
			}
			if ( bAllUnitsOnPlace )
				eState = EBES_BUILD;
			break;
		}
	case EBES_BUILD:
		{
			const int nSize = pFormation->Size();
			const float fAddWork = nSize * SConsts::RU_PER_QUANT * float( curTime - lastTime ) / SConsts::TIME_QUANT / 3.0f;
			lastTime = curTime;
			fCompletion += fAddWork;
			if ( fCompletion > pCreation->GetPrice() * ( nMaxIndex - pCreation->GetCurIndex() ) )
			{
				pCreation->BuildAll( pCreation->GetCurIndex(), nMaxIndex );
				if ( nMaxIndex < pCreation->GetMaxIndex() )
				{
					pCreation->LockCannotBuild();
				}
        TryInterruptState( 0 );
				pFormation->UnitCommand( new CAICommand( SAIUnitCmd( ACTION_COMMAND_ENTER, pCreation->GetEntrenchmentID() ) ), false, false );
				return;
			}
			break;
		}
	}
}

void CFormationBuildEntrenchmentState::SendUnitsAway( std::list<CPtr<CAIUnit> > *pUnitsPreventing )
{
	CLine2 line( vStartPoint, vEndPoint );
	CVec2 vAway = vEndPoint - vStartPoint;
	vAway = CVec2( vAway.y, -vAway.x );
	Normalize( &vAway );
	for ( std::list<CPtr<CAIUnit> >::iterator it = pUnitsPreventing->begin(); it != pUnitsPreventing->end(); ++it )
	{
		CAIUnit * pUnit = *it;
		int nSign = line.GetSign( pUnit->GetCenterPlain() );
		nSign = nSign != 0 ? nSign : 1;
		const CVec2 vTo = pUnit->GetCenterPlain() + 
			pUnit->GetBoundTileRadius() * 3 * SConsts::TILE_SIZE * nSign * vAway;
		if ( pUnit->IsIdle() )
			pUnit->UnitCommand( new CAICommand( SAIUnitCmd( ACTION_COMMAND_MOVE_TO, vTo ) ), false, false );
	}
	pUnitsPreventing->clear();
}

//*******************************************************************
//*										CFormationGunCrewState												*
//*******************************************************************

void CFormationGunCrewState::SUnit::UpdateAction()
{
	if ( bForce || eNewAction != eAction )
	{
		NI_ASSERT( IsAlive(), "wrong call" );

		bForce = false;
		eAction = eNewAction;

		if ( eAction != ACTION_NOTIFY_NONE  )
		{
			if ( eAction == ACTION_NOTIFY_USE_UP || eAction == ACTION_NOTIFY_USE_DOWN )
				theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_USE, (float)eAction ), pUnit, false );
			else if ( eAction == ACTION_NOTIFY_IDLE )
				theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_STOP ), pUnit, false );
		}
//		else
//			theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_STOP ), pUnit, false );
	}

	if ( timeNextUpdate < curTime && eAction != ACTION_NOTIFY_NONE )
	{
		//pUnit->TurnToDir( wDirection );
		pUnit->SetDirection( wDirection );
		timeNextUpdate = curTime + 500;
	}
}

void CFormationGunCrewState::SUnit::ResetAnimation()
{
	bForce = true;
}

void CFormationGunCrewState::SUnit::SetAction( const CFormationGunCrewState::SCrewAnimation &rNewAnim, bool force )
{
	eNewAction = rNewAnim.eAction;
	wDirection = rNewAnim.wDirection;
	bForce |= force;
}

bool CFormationGunCrewState::SUnit::IsAlive() const 
{
	return IsValidObj( pUnit );
}

CFormationGunCrewState::SUnit::SUnit()
: eAction( ACTION_NOTIFY_IDLE ), eNewAction( ACTION_NOTIFY_NONE ), bForce( true ), vServePoint( VNULL2 ), wDirection(0), timeNextUpdate( 0 )
{
}

CFormationGunCrewState::SUnit::SUnit( class CSoldier * pUnit, const CVec2 &vServePoint, const EActionNotify eAction)
: pUnit( pUnit ), eAction( eAction ), eNewAction( ACTION_NOTIFY_NONE ), bForce( true ), vServePoint( vServePoint ), wDirection(0),
timeNextUpdate( 0 )
{
}

CFormationGunCrewState::SCrewMember::SCrewMember()
: bOnPlace(false)
{
}

CFormationGunCrewState::SCrewMember::SCrewMember( const CVec2 &vServePoint, CSoldier *pUnit, const EActionNotify eAction)
: SUnit( pUnit, vServePoint, eAction ), bOnPlace(false)
{
}

IUnitState* CFormationGunCrewState::Instance( class CFormation *pUnit, CArtillery *pArtillery)
{
	return new CFormationGunCrewState( pUnit, pArtillery );
}

CFormationGunCrewState::CFormationGunCrewState( class CFormation *_pUnit, CArtillery * _pArtillery)
: pUnit( _pUnit ), pArtillery( _pArtillery ), startTime(curTime),
	fReloadProgress( 0 ),
	eGunState( EGSS_OPERATE ),
	timeLastUpdate( curTime ),
	nFormationSize( 0 ),
	nReloadPhaze( 0 )
{
	pUnit->SetSelectable( false, true );
	pArtillery->SetSelectable( pArtillery->GetPlayer()==theDipl.GetMyNumber(), true );
	pStats = checked_cast<const SMechUnitRPGStats*>( pArtillery->GetStats() );
	const int nSize = pUnit->Size();
	for ( int i = 0; i < nSize; ++i )
	{
		(*pUnit)[i]->AllowLieDown( false );
		(*pUnit)[i]->ApplyStatsModifier( pStats->pInnerUnitBonus, true );
	}
	b360DegreesRotate = pStats->GetPlatform( pArtillery->GetUniqueId(), 1 ).constraint.wMax >= 65535;
	ClearState();
	updater.AddUpdate( 0, ACTION_NOTIFY_SERVED_ARTILLERY, pUnit, pArtillery->GetUniqueId() );
	
	//capturnig enemy artillery.
	if ( EDI_ENEMY == theDipl.GetDiplStatus( pArtillery->GetInitialPlayer(), pUnit->GetPlayer() ) )
	{
		theStatistics.UnitCaptured( pUnit->GetPlayer() );
		pArtillery->SetInitialPlayer( pUnit->GetPlayer() );
		updater.AddUpdate( 0, ACTION_NOTIFY_CHANGE_SCENARIO_INDEX, pArtillery, -1 );
	}
}

bool CFormationGunCrewState::ClearState()
{
	bReloadInProgress = false;

	freeUnits.clear();
	nFormationSize = pUnit->Size();
	const CVec2 vGunCenter = pArtillery->GetCenterPlain();
	for ( int i = 0; i < nFormationSize; ++i )
	{
		CSoldier *pSold = (*pUnit)[i];
		theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_IDLE), pSold, false );
		pSold->SetFree();
		pSold->RestoreSmoothPath();
		pSold->Stop();
		freeUnits.push_back( SUnit( pSold, vGunCenter ) );
	}

	// для движения миномета - особый случай
	if ( (pStats->etype == RPG_TYPE_ART_MORTAR ||pStats->etype == RPG_TYPE_ART_HEAVY_MG)&& eGunState == EGSS_MOVE )
	{
		pUnit->SetCarryedMortar( pArtillery );
		pUnit->SetGroupShift( pArtillery->GetGroupShift() );
		pUnit->SetSubGroup( pArtillery->GetSubGroup() );
		pUnit->InitWCommands( pArtillery );	// очередь команд миномета скопировать в очередь SQUAD
		pUnit->SetSelectable( pArtillery->GetPlayer() == theDipl.GetMyNumber(), true );
		updater.AddUpdate( 0, ACTION_SET_SELECTION_GROUP, pUnit, pArtillery->GetUniqueId() );
		updater.AddUpdate( 0, ACTION_NOTIFY_DISABLE_ACTION, pUnit, ACTION_COMMAND_CATCH_ARTILLERY );
		//updater.AddUpdate( ACTION_NOTIFY_SELECT_CHECKED, pUnit, pArtillery->GetUniqueId() );
		updater.AddUpdate( 0, ACTION_NOTIFY_SERVED_ARTILLERY, pUnit, -1 );
		pArtillery->SetOffTankPit();
		pArtillery->Disappear();
		pUnit->SetCommandFinished();
		
		return true;
	}
	else
	{
		crew.clear();
		// определить сколько мест нам нужно, чтобы распределить всю команду
		NI_ASSERT( pStats->gunners.size() == EGSS_MOVE + 1, fmt::format("gunners structure has wrong size ({})", pStats->gunners.size()) );
		crew.resize( pStats->gunners[eGunState].gunners.size() );
		NI_ASSERT( !crew.empty(), fmt::format( "locators for gunner places in artillery {} are not exist", pStats->GetParentName()) );

		pArtillery->SetOperable( 1.0f );
		wGunTurretDir =  pArtillery->GetFrontDirection() + pArtillery->GetTurret( 0 )->GetHorCurAngle();
	}
	return false;
}

void CFormationGunCrewState::Segment()
{
	if ( !IsValidObj( pArtillery ) )
	{
		pArtillery->DelCrew();	
		TryInterruptState( 0 );
		return;
	}
	CTurret *pTurret = pArtillery->GetTurret( 0 );
	const uint16_t wCurTurretHorDir = pTurret->GetHorCurAngle();
	const uint16_t wCurTurretVerDir = pTurret->GetVerCurAngle();

	const bool bNoAnimation = EGSS_MOVE != eGunState && b360DegreesRotate;
	
	const uint16_t wCurTuttetDir = pArtillery->GetFrontDirection() + wCurTurretHorDir;
	const CVec2 vTurretDir = GetVectorByDirection( wGunTurretDir );
	
	const bool bReloaderRotatesWithTurret = b360DegreesRotate && pStats->etype != RPG_TYPE_ART_AAGUN;

	const CVec2 vGunDir =  bReloaderRotatesWithTurret ? vTurretDir : GetVectorByDirection( pArtillery->GetFrontDirection() );

	const uint16_t wCurBaseDir = bReloaderRotatesWithTurret ? wGunTurretDir : pArtillery->GetFrontDirection();
	const CVec2 vCurGunPos = pArtillery->GetCenterPlain();
	bool bRecalcPoints = wCurTuttetDir != wGunTurretDir || wGunBaseDir != wCurBaseDir || vCurGunPos != vGunPos;
	vGunPos = vCurGunPos;
	wGunTurretDir = wCurTuttetDir;
	wGunBaseDir = wCurBaseDir;
	
	EGunServeState eDesiredState = EGSS_OPERATE;
	if ( pArtillery->GetState()->GetName() == EUSN_MOVE && pArtillery->IsUninstalled() )
		eDesiredState = EGSS_MOVE;
	//else if ( /*pArtillery->IsInInstallAction() ||*/ pArtillery->IsUninstalled() )
	//	eDesiredState = EGSS_ROTATE;

	bool bFinishState = false;
	if ( eGunState != eDesiredState || nFormationSize < pUnit->Size() ) // change state
	{
		bRecalcPoints = true;
		eGunState = eDesiredState;
		bFinishState = ClearState();
	}

	if ( bFinishState )
		return;


	// общие действия
	if ( bRecalcPoints )
		RecountPoints( vGunDir, vTurretDir );
	RefillCrew();
	const int iUnitsOnPlace = CheckThatAreOnPlace();
	SendThatAreNotOnPlace( bNoAnimation );
	
	// работа с пушкой
	switch ( eGunState )
	{
	case EGSS_OPERATE:
		{
			bool bStartReload = false;
			int nGun = pArtillery->GetNGuns();
			bool bNoAmmo = true;
			if ( !bStartReload )
			{
				for ( int i = 0; i< nGun; ++i )
				{
					CBasicGun *pGun = pArtillery->GetGun( i );
					bStartReload = pGun->IsRelaxing(); //some gun is waiting for reload
					if ( bStartReload && !bReloadInProgress )
						fReloadPrice = pGun->GetRelaxTime( false );
					bNoAmmo &= ( 0==pGun->GetNAmmo() );
				}
			}
			if ( bStartReload )
			{
				if ( !bReloadInProgress ) // только начали перезарядку
				{
					bReloadInProgress  = true;
					fReloadProgress = 0;
				}
			}

			eGunOperateSubState = EGOSS_RELAX;
			if ( bNoAmmo )
				fReloadProgress = 0;
			else if ( bReloadInProgress )
			{
				eGunOperateSubState = EGOSS_RELOAD;
				fReloadProgress += 1.0f * (curTime - startTime) *
														iUnitsOnPlace / pStats->gunners[eGunState].gunners.size();
				// 3 фазы прерзарядки ( для анимаций )
				nReloadPhaze = int ( fReloadProgress / (fReloadPrice / 3.0f) );
				if ( fReloadProgress >= fReloadPrice )
				{
					pArtillery->ClearWaitForReload();
					eGunOperateSubState = EGOSS_RELAX;
					bReloadInProgress = false;
				}
			}
			else if ( abs( wCurTurretVerDir - wTurretVerDir ) > 0 )
			{
				if ( abs( wCurTurretHorDir - wTurretHorDir ) > 0 )
					eGunOperateSubState = EGOSS_AIM;
				else
					eGunOperateSubState = EGOSS_AIM_VERTICAL;
			}
		}
		break;
	case EGSS_ROTATE:
		bReloadInProgress = false;
		break;
	case EGSS_MOVE:
		bReloadInProgress = false;
		break;
	}

	UpdateAnimations();
	
	pArtillery->SetOperable( 1.0f );

	startTime = curTime ;
	wTurretHorDir = wCurTurretHorDir;
	wTurretVerDir = wCurTurretVerDir;
}

void CFormationGunCrewState::RefillCrew()
{
	for ( int i = 0; i < crew.size(); ++i )
	{
		if ( !crew[i].IsAlive() ) //кого-то убили
		{
			nFormationSize = pUnit->Size();
			if ( !freeUnits.empty() ) // если есть запасные, взять из запасных
			{
				crew[i].pUnit = freeUnits.front().pUnit;
				freeUnits.pop_front();
				crew[i].bOnPlace = false;
				crew[i].ResetAnimation();
				crew[i].pUnit->SetSmoothPath( new CArtilleryCrewPath( crew[i].pUnit, crew[i].pUnit->GetCenterPlain(), crew[i].vServePoint, 0 ) );
			}
			else	// перераспределить команду.
			{
				for ( int j = crew.size() - 1; j > i; --j ) // найти живого на менее приоритетном месте
				{
					if ( crew[j].IsAlive() )
					{
	  				crew[i].pUnit->SetSmoothPath( new CArtilleryCrewPath( crew[i].pUnit, crew[i].pUnit->GetCenterPlain(), crew[i].vServePoint, 0 ) );
						crew[i].pUnit = crew[j].pUnit;
						crew[j].pUnit = 0;
						crew[i].bOnPlace = false;
						crew[i].ResetAnimation();
						break;
					}
				}
			}
		}
	}
}

void CFormationGunCrewState::RecountPoints( const CVec2 &vGunDir, const CVec2 &vTurretDir )
{
	const CVec2 vCenter ( pArtillery->GetCenterPlain() );

	const int nCrew = crew.size();
	const int nDesiredSize = pStats->gunners[eGunState].gunners.size();
	NI_ASSERT( nDesiredSize != 0, fmt::format("{} in state {} has 0 gunners", pStats->szKeyName, int( eGunState )) );

	for ( int i = 0; i < nCrew; ++i )
	{
		const CVec2 vCrew( pStats->gunners[eGunState].gunners[i%nDesiredSize] );
		const int nOffs = i / nDesiredSize;
		const CVec2 pt( vCrew.y, - vCrew.x * ( 1 + 1.1f * nOffs ) );
		// 1 - st gunner is near ammo box - rotates with gun, not with turret
		crew[i].vServePoint = vCenter + ( pt ^ ( i == 1 ? vGunDir: vTurretDir ) );
	}

	// свободных слать только если они слишком далеко от точек, где должны быть
	int i = 0;
	for ( std::list< SUnit >::iterator it = freeUnits.begin(); it != freeUnits.end(); ++it )
	{
		const CVec2 freePoint( -pStats->vAABBHalfSize.y + pStats->vAABBCenter.y - SConsts::TILE_SIZE, i * SConsts::TILE_SIZE/2 );	
		const CVec2 vServePoint = vCenter + ( freePoint ^ vGunDir );
		(*it).vServePoint = vServePoint;
		++i;
	}
}				

uint16_t CFormationGunCrewState::CalcDirToAmmoBox( int nCrewNumber ) const
{
	return GetDirectionByVector( CVec2(pArtillery->GetAmmoBoxCoordinates().x,pArtillery->GetAmmoBoxCoordinates().y) - crew[nCrewNumber].pUnit->GetCenterPlain() );
}

uint16_t CFormationGunCrewState::CalcDirFromTo( int nCrewNumberFrom, int nCrewNumberTo ) const
{
	const CVec2 vCenter ( pArtillery->GetCenterPlain() );
	const CVec2 ptTo( pStats->gunners[eGunState].gunners[nCrewNumberTo].y, - pStats->gunners[eGunState].gunners[nCrewNumberTo].x );
	const CVec2 ptFrom( pStats->gunners[eGunState].gunners[nCrewNumberFrom].y, - pStats->gunners[eGunState].gunners[nCrewNumberFrom].x );
	const CVec2 vGunDir( GetVectorByDirection( wGunTurretDir ) );

	return GetDirectionByVector( (ptTo ^ vGunDir)  - (ptFrom ^ vGunDir) );
}

CFormationGunCrewState::SCrewAnimation CFormationGunCrewState::CalcAniamtionForMG( int iUnitNumber ) const
{
	SCrewAnimation animation;

	animation.wDirection = wGunTurretDir;
	switch( iUnitNumber )
	{
		case 0: 

			if ( EGSS_OPERATE == eGunState && IsGunAttacking() )
				animation.eAction = ACTION_NOTIFY_USE_UP;
			else if ( EGSS_ROTATE == eGunState )
				animation.eAction = ACTION_NOTIFY_USE_UP;
			else
				animation.eAction = ACTION_NOTIFY_USE_DOWN;

			break;
		case 1: 
			animation.eAction = ACTION_NOTIFY_USE_DOWN; 
			break;
		default:
			animation.eAction = ACTION_NOTIFY_NONE; 
			break;
	}

	return animation;
}

bool CFormationGunCrewState::IsGunAttacking() const 
{
	return 	pArtillery->IsInstalled() &&
					pArtillery->GetState() &&
					( 
						pArtillery->GetState()->IsAttackingState() || 
						(b360DegreesRotate ? false : pArtillery->GetTurret( 0 )->GetHorCurAngle() != 0 )
					) &&
					0 != pArtillery->GetGun( 0 )->GetNAmmo();
}

CFormationGunCrewState::SCrewAnimation CFormationGunCrewState::CalcNeededAnimation( int iUnitNumber ) const
{
	SCrewAnimation animation;

	if ( RPG_TYPE_ART_HEAVY_MG == pStats->etype )
		return CalcAniamtionForMG( iUnitNumber );

	switch( eGunState )
	{
	case EGSS_OPERATE:
		{
			switch( eGunOperateSubState )
			{
			case EGOSS_AIM_VERTICAL:
				switch ( iUnitNumber )
				{
				case 0:
					animation.eAction = ACTION_NOTIFY_USE_UP;
					animation.wDirection = wGunTurretDir;
					break;
				case 1:
					animation.eAction = ACTION_NOTIFY_IDLE;
					animation.wDirection = wGunBaseDir;
					break;
				case 2:
					animation.eAction = ACTION_NOTIFY_IDLE;
					animation.wDirection = wGunTurretDir;
					break;
				}

				break;
			case EGOSS_AIM:
				switch( iUnitNumber )
				{
				case 0:
					animation.wDirection = wGunTurretDir;
					animation.eAction = ACTION_NOTIFY_USE_UP;
					break;
				case 1:
					animation.eAction = ACTION_NOTIFY_MOVE;
					animation.wDirection = wGunBaseDir;
					break;
				case 2:
					animation.eAction = ACTION_NOTIFY_MOVE;
					animation.wDirection = wGunTurretDir;
					break;
				}
				
				break;
			
			case EGOSS_RELOAD:
				{
					switch( nReloadPhaze )
					{
					case 0: // достать из ящика патрон
						switch ( iUnitNumber )
						{
						case 0: 
							animation.wDirection = wGunTurretDir; 
							if ( pStats->etype == RPG_TYPE_ART_MORTAR ) // у миномета передают патрон сидя
								animation.eAction = ACTION_NOTIFY_USE_DOWN; 
							else
								animation.eAction = ACTION_NOTIFY_USE_DOWN; 
							break;
						case 2: 
							animation.wDirection = wGunTurretDir; 
							if ( pStats->etype == RPG_TYPE_ART_MORTAR ) // у миномета передают патрон сидя
								animation.eAction = ACTION_NOTIFY_USE_DOWN;
							else 
								animation.eAction = ACTION_NOTIFY_IDLE; 

							break;
						case 1: 
							animation.wDirection = CalcDirToAmmoBox( 1 );
							animation.eAction = ACTION_NOTIFY_USE_DOWN; 
							
							break;
						default: 
							animation.eAction = ACTION_NOTIFY_IDLE; 
							animation.wDirection = wGunTurretDir; 
							
							break;
						}

						break;
					case 1:	// передать заряжающему
						switch ( iUnitNumber )
						{
						case 0: 
							animation.wDirection = wGunTurretDir;
							if ( pStats->etype == RPG_TYPE_ART_MORTAR ) // у миномета передают патрон сидя
								animation.eAction = ACTION_NOTIFY_USE_DOWN; 
							else
								animation.eAction = ACTION_NOTIFY_USE_DOWN; 
							
							break;
						case 2: // заряжаюший
							animation.wDirection = CalcDirFromTo( 2, 1 );
							if ( pStats->etype == RPG_TYPE_ART_MORTAR ) // у миномета передают патрон сидя
								animation.eAction = ACTION_NOTIFY_USE_DOWN;
							else
								animation.eAction = ACTION_NOTIFY_USE_UP;

							break;
						case 1: // у ящика
							animation.wDirection = CalcDirFromTo( 1, 2 );
							if ( pStats->etype == RPG_TYPE_ART_MORTAR ) // у миномета передают патрон сидя
								animation.eAction = ACTION_NOTIFY_USE_DOWN;
							else
								animation.eAction = ACTION_NOTIFY_USE_UP; 

							break;
						default: 
							animation.wDirection = wGunTurretDir; 
							animation.eAction = ACTION_NOTIFY_IDLE; 
							
							break;
						}

						break;
					case 2:	// зарядить патрон в пушку
					default:
						switch ( iUnitNumber )
						{
						case 0: // при зарядке миномета встать
							animation.wDirection = wGunTurretDir; 
							if ( pStats->etype == RPG_TYPE_ART_MORTAR ) 
								animation.eAction = ACTION_NOTIFY_USE_UP;
							else
								animation.eAction = ACTION_NOTIFY_USE_DOWN; 

							break;
						case 1: // подающий у миномета сидит
							animation.wDirection = CalcDirToAmmoBox( 1 ); 
							animation.eAction = ACTION_NOTIFY_USE_DOWN; 

							break;
						case 2: 
							animation.wDirection = wGunTurretDir; 
							if ( pStats->etype == RPG_TYPE_ART_MORTAR ) 
								animation.eAction = ACTION_NOTIFY_USE_DOWN; 
							else
								animation.eAction = ACTION_NOTIFY_USE_UP; 

							break;
						default: 
							animation.wDirection = wGunTurretDir; 
							animation.eAction = ACTION_NOTIFY_IDLE; 
	
							break;
						}
						
						break;
					}
				}

				break;
			case EGOSS_RELAX:
			default:
				{
					if ( iUnitNumber == 1 )
						animation.wDirection = wGunBaseDir;
					else
						animation.wDirection = wGunTurretDir;
					NI_ASSERT( 1 >= pArtillery->GetNCommonGuns(), fmt::format("artillery ({}) with {} guns, error", NDb::GetResName(pArtillery->GetStats()), pArtillery->GetNCommonGuns()) );
					const bool bAttackingNow =	IsGunAttacking();
					if ( !bAttackingNow )
						animation.eAction = ACTION_NOTIFY_IDLE;
					else
					{
						if ( pStats->etype == RPG_TYPE_ART_MORTAR && iUnitNumber >= 0 && iUnitNumber < 3  )  
							animation.eAction = ACTION_NOTIFY_USE_DOWN; // у миномета рассчет сидит во время боя
						else
						{
							switch ( iUnitNumber )
							{
							case 0: 
								animation.wDirection = wGunTurretDir;
								if ( pStats->etype == RPG_TYPE_ART_MORTAR ) 
									animation.eAction = ACTION_NOTIFY_USE_UP;
								else
									animation.eAction = ACTION_NOTIFY_USE_DOWN; 
								break;
							case 1: 
								animation.wDirection = CalcDirToAmmoBox( 1 );
								animation.eAction = ACTION_NOTIFY_USE_DOWN; 

								break;
							case 2: 
								animation.wDirection = wGunTurretDir; 
								if ( pStats->etype == RPG_TYPE_ART_MORTAR ) 
									animation.eAction = ACTION_NOTIFY_USE_DOWN; 
								else
									animation.eAction = ACTION_NOTIFY_IDLE; 

								break;
							default: 
								animation.wDirection = wGunTurretDir; 
								animation.eAction = ACTION_NOTIFY_IDLE; 

								break;
							}
						}
					}
				}
				break;
			}
		}

		break;
	case EGSS_ROTATE:
		//animation.eAction = ACTION_NOTIFY_NONE;
		animation.eAction = ACTION_NOTIFY_USE_UP;

		break;
	case EGSS_MOVE:
		animation.eAction = ACTION_NOTIFY_MOVE;
		animation.wDirection = wGunTurretDir;

		break;
	default:
		animation.wDirection = wGunTurretDir;
		animation.eAction = ACTION_NOTIFY_IDLE;

		break;
	}
	return animation;
}

int CFormationGunCrewState::CheckThatAreOnPlace()
{
	int iOnPlace = 0;
	for ( int i = 0; i < crew.size(); ++i )
	{
		SCrewMember &crewMember = crew[i];
		if ( crewMember.IsAlive() )
		{
			const CVec2 vDiff = crewMember.vServePoint - crewMember.pUnit->GetCenterPlain();
			if ( fabs2( vDiff ) > 0.1f )
			{
				crewMember.bOnPlace = false;
				crewMember.SetAction( SCrewAnimation( ACTION_NOTIFY_NONE, GetDirectionByVector( vDiff ) ) );
			}
			else if ( crewMember.bOnPlace )
			{
				++iOnPlace;
				crewMember.SetAction( CalcNeededAnimation( i ) );
			}
			else 
			{
				++iOnPlace;
				crewMember.bOnPlace = true;
				crewMember.SetAction( CalcNeededAnimation( i ), true );
			}
		}
	}

	for ( std::list<SUnit>::iterator it = freeUnits.begin(); it != freeUnits.end(); )
	{
		SUnit crewUnit( *it );
		if ( crewUnit.IsAlive() )
		{
			if ( crewUnit.pUnit->IsIdle() )
			{
				crewUnit.pUnit->Stop();
				crewUnit.SetAction( CalcNeededAnimation( -1 ) );
			}
			++it;
		}
		else
			it = freeUnits.erase( it );
	}

	return iOnPlace;
}

void CFormationGunCrewState::SendThatAreNotOnPlace( const bool bNoAnimation )
{
	for ( int i = 0; i < crew.size() ; ++i )
	{
		SCrewMember & crewMember = crew[i];
		if ( crewMember.IsAlive() )
		{
			if ( eGunState == EGSS_ROTATE )
			{
				if ( !crewMember.pUnit->IsInSolidPlace() )
					units.UnitChangedPosition( crewMember.pUnit, crewMember.vServePoint );
				crewMember.pUnit->SetDirection( wGunBaseDir );
				crewMember.pUnit->SetCenter( GetHeights()->Get3DPoint( crewMember.vServePoint ), false );
				updater.AddUpdate( 0, ACTION_NOTIFY_PLACEMENT, crewMember.pUnit, -1 );
				continue;
			}
			else if ( bNoAnimation )
			{
				const float fDiff2 = fabs2( crewMember.vServePoint - crewMember.pUnit->GetCenterPlain() );
				if ( fDiff2 > 0 )					// is not on place
				{
					CArtilleryCrewPath* pPath = checked_cast<CArtilleryCrewPath*>( crewMember.pUnit->GetSmoothPath() );
					const float fSpeed = pArtillery->GetSpeed();
					
					// to cure not smooth moving
					// const float fCurrentSpeed = Max( fSpeed, crewMember.pUnit->GetMaxPossibleSpeed() );
					pPath->SetParams( crewMember.vServePoint, 0 );//fCurrentSpeed );

					if ( !crewMember.pUnit->IsInSolidPlace() )
						units.UnitChangedPosition( crewMember.pUnit, crewMember.vServePoint );
					crewMember.pUnit->Stop();
					crewMember.pUnit->SetDirection( wGunBaseDir );
					crewMember.pUnit->SetCenter( GetHeights()->Get3DPoint( crewMember.vServePoint ), false );
					updater.AddUpdate( 0, ACTION_NOTIFY_PLACEMENT, crewMember.pUnit, -1 );
				}
				else
					crewMember.SetAction( CalcNeededAnimation( i ) );
				continue;
			}
			else if ( !crewMember.bOnPlace )
			{
				const CVec2 vDiff = crewMember.vServePoint - crewMember.pUnit->GetCenterPlain();
				const float fDiff2 = fabs2( vDiff );
				if ( fDiff2 >= 0.01f ) //far from desired place
				{
					if ( crewMember.pUnit->IsInFirePlace() )
						crewMember.pUnit->SetFree();
					//DEBUG{
					{
						CArtilleryCrewPath *pArtPath = dynamic_cast<CArtilleryCrewPath*>( crewMember.pUnit->GetSmoothPath() );
					}
					//DEBUG}
					NI_ASSERT( dynamic_cast<CArtilleryCrewPath*>(crewMember.pUnit->GetSmoothPath()) != 0, "wrong path" );
					CArtilleryCrewPath* pPath = checked_cast<CArtilleryCrewPath*>( crewMember.pUnit->GetSmoothPath() );
					const float fSpeed = pArtillery->GetSpeed();
					pPath->SetParams( crewMember.vServePoint, (std::max)( fSpeed, crewMember.pUnit->GetMaxPossibleSpeed() )  );
					crewMember.pUnit->SetDirectionVec( vDiff );
					continue;
				}
				
				if ( fDiff2 < 0.01f ) // near to desired place
				{
					NI_ASSERT( dynamic_cast<CArtilleryCrewPath*>(crewMember.pUnit->GetSmoothPath()) != 0, "wrong path" );
					CArtilleryCrewPath* pPath = checked_cast<CArtilleryCrewPath*>( crewMember.pUnit->GetSmoothPath() );
					pPath->SetParams( crewMember.vServePoint, sqrt(fDiff2) / SConsts::AI_SEGMENT_DURATION );

					if ( !crewMember.pUnit->IsInSolidPlace() )
						units.UnitChangedPosition( crewMember.pUnit, crewMember.vServePoint );
					crewMember.pUnit->SetCenter( GetHeights()->Get3DPoint( crewMember.vServePoint ), false );

					crewMember.pUnit->SetDirectionVec( vDiff );
					continue;
				}
			}

			if( !crewMember.pUnit->IsInFirePlace() )
			{
				crewMember.pUnit->SetToFirePlace();
				crewMember.SetAction( CalcNeededAnimation( i ), true );
				crewMember.pUnit->AllowLieDown( false );
			}
			else
				crewMember.SetAction( CalcNeededAnimation( i ) );
		}
	}
	
	// послать свободных по обычному пути
	for ( std::list< SUnit >::iterator it = freeUnits.begin(); it != freeUnits.end(); ++it )
	{
		const CVec2 &vServePoint =  (*it).vServePoint ;
		const CVec2 vCenter =  (*it).pUnit->GetCenterPlain() ;
		if (	(*it).pUnit->IsIdle() &&
					fabs2( vCenter - vServePoint ) > sqr( static_cast<int>(SConsts::TILE_SIZE) ) )
		{
			CPtr<IStaticPath> pPath = CreateStaticPathToPoint( (*it).vServePoint, VNULL2, (*it).pUnit, true, GetAIMap() );
			if ( pPath )
			{
				(*it).pUnit->RestoreSmoothPath();
				(*it).pUnit->SendAlongPath( pPath, VNULL2, true );
				(*it).SetAction( SCrewAnimation(ACTION_NOTIFY_MOVE, (*it).pUnit->GetDirection()) );
			}
		}
	}
}

void CFormationGunCrewState::UpdateAnimations()
{
	for ( int i = 0; i < crew.size(); ++i )
	{
		if ( crew[i].IsAlive() )
		{
			if ( crew[i].bOnPlace )
				crew[i].SetAction( CalcNeededAnimation( i ) );
			crew[i].UpdateAction();
		}
	}
	for ( CFreeUnits::iterator it = freeUnits.begin(); it != freeUnits.end(); ++it )
	{
		if ( it->IsAlive() )
		{
			it->SetAction( CalcNeededAnimation( -1 ) );
			it->UpdateAction();
		}
	}
}

void CFormationGunCrewState::SetAllAnimation( EActionNotify action, bool force )
{
	for ( int i=0; i< crew.size(); ++i )
		crew[i].SetAction( SCrewAnimation(action, crew[i].pUnit->GetDirection()), force );
}				

void CFormationGunCrewState::Interrupt()
{
	if	( pUnit->IsIdle() )
	{
		pUnit->Stop();
	}
	const int nSize = pUnit->Size();
	for	( int i = 0; i < nSize; ++i )
	{
		CSoldier *pSold = (*pUnit)[i];
		(*pUnit)[i]->ApplyStatsModifier( pStats->pInnerUnitBonus, false );
		pSold->SetFree();
		pSold->AllowLieDown( true );
		pSold->RestoreSmoothPath();
	}

	pUnit->SetCommandFinished();
	pArtillery->SetOperable( false );
	pArtillery->DelCrew();
	
	updater.AddUpdate( 0, ACTION_NOTIFY_SERVED_ARTILLERY, pUnit, -1 );
}

bool CFormationGunCrewState::CanInterrupt() 
{
	return false;
}

ETryStateInterruptResult CFormationGunCrewState::TryInterruptState( class CAICommand *pCommand )
{
	if ( !pCommand || 
				pCommand->ToUnitCmd().nCmdType == ACTION_MOVE_CATCH_TRANSPORT ||
				pCommand->ToUnitCmd().nCmdType == ACTION_COMMAND_LEAVE ||
				pCommand->ToUnitCmd().nCmdType == ACTION_COMMAND_LOAD_NOW ||
				pCommand->ToUnitCmd().nCmdType == ACTION_COMMAND_LOAD ||
				pCommand->ToUnitCmd().nCmdType == ACTION_MOVE_ENTER_TRANSPORT_CHEAT_PATH )
	{
		Interrupt();
		return TSIR_YES_IMMIDIATELY;
	}
	else
		return TSIR_YES_WAIT;
}

const CVec2 CFormationGunCrewState::GetPurposePoint() const
{
	if ( IsValidObj( pArtillery ) )
		return pArtillery->GetCenterPlain();
	else
		return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*										CFormationInstallMortarState												*
//*******************************************************************

IUnitState* CFormationInstallMortarState::Instance( class CFormation *pUnit )
{
	return new CFormationInstallMortarState( pUnit );
}

CFormationInstallMortarState::CFormationInstallMortarState( class CFormation *pUnit )
: pUnit( pUnit ), timeInstall( curTime + 200 ), nStage( 0 )
{
	CAIUnit *pMortar = pUnit->InstallCarryedMortar();
	if ( pMortar )
	{
		// миномету послать апдейт на то, что он анинсталлирован
		pArt = checked_cast<CArtillery *>(pMortar);
		pArt->InstallAction( ACTION_NOTIFY_INSTALL_MOVE, true );
	}
	else
		pUnit->SetCommandFinished();
}

void CFormationInstallMortarState::Segment()
{
	if ( curTime > timeInstall )
	{
		if ( nStage == 0 )
		{
			if ( pArt && pArt->IsRefValid() && pArt->IsAlive() )
			{
				pArt->SetCrew( pUnit, false );
				pArt->SetSelectable( pUnit->IsSelectable(), true );
				updater.AddUpdate( 0, ACTION_SET_SELECTION_GROUP, pArt, pUnit->GetUniqueId() );
				updater.AddUpdate( 0, ACTION_NOTIFY_ENABLE_ACTION, pUnit, ACTION_COMMAND_CATCH_ARTILLERY );
				//updater.AddUpdate( ACTION_NOTIFY_SELECT_CHECKED, pArt, pUnit->GetUniqueId() );

				// и команду на начало инсталляции
				theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_INSTALL), pArt, false );
			}
			nStage = 1;
			timeInstall = curTime + 200;
		}
		else if ( nStage == 1 )
		{
			theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_CATCH_ARTILLERY, pArt->GetUniqueId()), pUnit, false );
			pUnit->SetSelectable( false, true );
			pUnit->SetCommandFinished();
		}
	}
	//NI_ASSERT( false, "WRONG CALL" );
}

ETryStateInterruptResult CFormationInstallMortarState::TryInterruptState( class CAICommand *pCommand )
{
	return TSIR_YES_WAIT;
}

//*******************************************************************
//*										CFormationBuildFenceState											*
//*******************************************************************

IUnitState* CFormationUseSpyglassState::Instance( CFormation *pFormation, const CVec2 &point )
{
	return new CFormationUseSpyglassState( pFormation, point );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
CFormationUseSpyglassState::CFormationUseSpyglassState( CFormation *_pFormation, const CVec2 &point )
: pFormation( _pFormation )
{
	bool bHoldPos = false;
	for ( int i = 0; i < pFormation->Size(); ++i )
	{
		CSoldier *pSoldier = (*pFormation)[i];

		if ( pSoldier->GetStats()->HasCommand( ACTION_COMMAND_USE_SPYGLASS ) &&
				 ( pSoldier->IsFree() || pSoldier->IsInFirePlace() ) )
		{
			theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_USE_SPYGLASS, point ), pSoldier, false );
			bHoldPos = true;
		}
	}

	if ( bHoldPos )
	{
		pFormation->Stop();	
		pFormation->SetBehaviourMoving( SBehaviour::EMHoldPos );
		for ( int i = 0; i < pFormation->Size(); ++i )
		{
			CSoldier *pSoldier = (*pFormation)[i];

			pSoldier->Stop();
			pSoldier->SetBehaviourMoving( SBehaviour::EMHoldPos );
		}
	}

	pFormation->SetToWaitingState();
}

void CFormationUseSpyglassState::Segment()
{
}

ETryStateInterruptResult CFormationUseSpyglassState::TryInterruptState( class CAICommand *pCommand )
{
	pFormation->UnsetFromWaitingState();
	pFormation->SetCommandFinished();

	return TSIR_YES_IMMIDIATELY;
}

const CVec2 CFormationUseSpyglassState::GetPurposePoint() const
{
	return pFormation->GetCenterPlain();
}

//*******************************************************************
//*										CFormationCaptureArtilleryState								*
//*******************************************************************

IUnitState* CFormationCaptureArtilleryState::Instance( CFormation *pUnit, CArtillery *pArtillery, const bool _bUseFormationPart )
{
	return new CFormationCaptureArtilleryState( pUnit, pArtillery, _bUseFormationPart );
}

CFormationCaptureArtilleryState::CFormationCaptureArtilleryState( class CFormation *_pUnit, CArtillery *_pArtillery, const bool bUseFormationPart )
: CStatusUpdatesHelper( EUS_TAKE_ARTILLERY, _pUnit ), pUnit( _pUnit ), pArtillery( _pArtillery ), eState( FCAS_ESTIMATING )
{
	if ( ( pArtillery->IsBeingCaptured() && pArtillery->GetCapturedUnit() != pUnit ) ||
			 ( pArtillery->HasServeCrew() && pArtillery->GetCrew() != pUnit ) ||
			  !pArtillery->MustHaveCrewToOperate() )
	{
		// artillery doesn't need crew
		eState = FCAS_EXITTTING;
	}
	else
	{
		// artillery is free and need crew
		if ( bUseFormationPart )
		{
			// check if artillery in sight
			if ( pUnit->GetSightRadius() < fabs( pUnit->GetCenterPlain() - pArtillery->GetCenterPlain() ) )
			{
				CVec2 vDirection = pArtillery->GetCenterPlain() - pUnit->GetCenterPlain();
				Normalize( &vDirection );
				const CVec2 vDestination = pArtillery->GetCenterPlain() - vDirection * pUnit->GetSightRadius() / 2.0f;
				theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_MOVE_TO, vDestination ), pUnit, false );
				theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_CATCH_ARTILLERY, pArtillery->GetUniqueId(), float(bUseFormationPart) ), pUnit, true );
				pArtillery = 0;
				return;
			}
			// create new squad to send it to capture artillery.
			CPtr<CFormation> pCrew = theUnitCreation.CreateCrew( pArtillery, -1, CVec3( pUnit->GetCenterPlain(), 0.0f ), pUnit->GetPlayer(), false );
			if ( !pCrew )
			{
				pUnit->SetCommandFinished();
				return;
			}

			const bool bUseOfficer = pCrew->Size() >= pUnit->Size();

			// do not use officer if possible.
			std::vector< CPtr<CSoldier> > soldiersToUse;
			
			for ( int i = 0; i < pUnit->Size(); ++i )
			{
				if ( bUseOfficer || RPG_TYPE_OFFICER != (*pUnit)[i]->GetStats()->etype )
					soldiersToUse.push_back( (*pUnit)[i] );
			}
			
			// place soldier from crew on their place
//			const int nSize = pCrew->Size();
			std::vector< CPtr<CSoldier> > soldiersToDelete;
			for ( int i = 0; i < pCrew->Size(); ++i )
			{
				if ( i >= soldiersToUse.size() )
					soldiersToDelete.push_back( (*pCrew)[i] );
				else
				{
					CSoldier *pOldSoldier = soldiersToUse[i];
					CSoldier *pNewSoldier = (*pCrew)[i];

					const uint16_t wDirection = pOldSoldier->GetDirection();
					const CVec2 vPos = pOldSoldier->GetCenterPlain();

					pNewSoldier->SetDirection( wDirection );
					pNewSoldier->NullCreationTime();
					pNewSoldier->SetCenter( GetHeights()->Get3DPoint( vPos ), false );
					pNewSoldier->CalcVisibility( true );
					updater.AddUpdate( 0, ACTION_NOTIFY_PLACEMENT, pNewSoldier, -1 );
					usedSoldiers.push_back( pOldSoldier );
				}
			}
			
			for ( int nSold = 0; nSold < soldiersToDelete.size(); ++nSold ) // crew has more memebers that is needed
				soldiersToDelete[nSold]->Disappear();
		}
		else
		{
			pUnit = _pUnit;
		}
	}
}

void CFormationCaptureArtilleryState::Segment()
{
	if ( !IsValidObj( pArtillery ) )
	{
		pUnit->SetCommandFinished();
		pUnit->SetSelectable( pUnit->GetPlayer() == theDipl.GetMyNumber(), true );
		return;
	}
	
	switch ( eState )
	{
	case FCAS_EXITTTING:
		pUnit->SetCommandFinished();

		break;
	case FCAS_ESTIMATING:
		{
			if ( !usedSoldiers.empty() )
			{
				for ( int i = 0; i < usedSoldiers.size(); ++i )
					usedSoldiers[i]->Disappear();
				usedSoldiers.clear();
				pUnit->SetCommandFinished();
				return;
			}
			
			InitStatus();
			CPtr<IStaticPath> pPath = CreateStaticPathToPoint( pArtillery->GetCenterPlain(), VNULL2, pUnit, true, GetAIMap() );
			if ( pPath )
			{
				pUnit->SendAlongPath( pPath, VNULL2, true );
				eState = FCAS_APPROACHING;
			}
			else 
			{
				pUnit->SendAcknowledgement( ACK_NEGATIVE );
				pUnit->SetCommandFinished();
			}
		}

		break;
	case FCAS_APPROACHING:
		if ( pUnit->IsIdle() )
		{
			pUnit->SetCommandFinished();
			pArtillery->SetCrew( pUnit ); // отдались
			pArtillery->InstallBack( false );
		}
		
		break;
	}
}

ETryStateInterruptResult CFormationCaptureArtilleryState::TryInterruptState( class CAICommand *pCommand )
{
	pUnit->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

//*******************************************************************
//*										CFormationRepairBridgeState*
//*******************************************************************

IUnitState* CFormationRepairBridgeState::Instance( class CFormation *pFormation, class CBridgeSpan *pBridge )
{
	return new CFormationRepairBridgeState( pFormation, pBridge );
}

CFormationRepairBridgeState::CFormationRepairBridgeState( class CFormation *pFormation, class CBridgeSpan *pBridge )
: pBridgeToRepair( pBridge->GetFullBridge() ), pUnit( pFormation ), eState( FRBS_WAIT_FOR_HOMETRANSPORT ),
fWorkLeft(0), fWorkDone(0)
{
	pBridge->GetFullBridge()->EnumSpans( &bridgeSpans );
	// убрать все непостроенные сегменты ( если команда на починку недостроенного моста )
	for( int i = 0; i < bridgeSpans.size(); ++i )
	{
		if ( bridgeSpans[i]->GetHitPoints() < 0.0f )
		{
			bridgeSpans.resize( i );
			break;
		}
	}
	CBridgeCreation::SortBridgeSpans( &bridgeSpans, pUnit );
}

void CFormationRepairBridgeState::Segment()
{
	if ( !IsValidObj( pHomeTransport ) )
		TryInterruptState( 0 );

	switch( eState )
	{
	case FRBS_WAIT_FOR_HOMETRANSPORT:
		if ( pUnit->GetNextCommand() )
			pUnit->SetCommandFinished();
		
		break;
	case FRBS_START_APPROACH:
		{
			if ( bridgeSpans.empty() ) 
				pUnit->SetCommandFinished();
			else
			{
				CPtr<IStaticPath> pPath = CreateStaticPathToPoint( bridgeSpans[0]->GetAttackCenter( pUnit->GetCenterPlain() ), VNULL2, pUnit, true, GetAIMap() );
				if ( pPath )
					pUnit->SendAlongPath( pPath, VNULL2, true );
				eState = FRBS_APPROACH;
			}
		}
		
		//find first damaged segment
		break;
	case FRBS_APPROACH:
		if ( pUnit->IsIdle() )
		{
			eState = FRBS_REPEAR;
			fWorkDone = 0;
			timeLastCheck = curTime;
		}

		break;
	case FRBS_REPEAR:
		if ( curTime - timeLastCheck > SConsts::TIME_QUANT )
		{
			fWorkDone = 1.0f * ( curTime - timeLastCheck ) / SConsts::TIME_QUANT * SConsts::RU_PER_QUANT;
			timeLastCheck = curTime;
			fWorkDone = (std::min)( fWorkLeft, fWorkDone );
			float fMissedWork = 0;
			int nSpansWithMissedHP = 0;
			// calc missed work

			for ( int i = 0; i < bridgeSpans.size(); ++i )
			{
				const SHPObjectRPGStats * pStats = bridgeSpans[i]->GetStats();
				const float fDWork = (pStats->fMaxHP - bridgeSpans[i]->GetHitPoints()) * pStats->fRepairCost * SConsts::REPAIR_COST_ADJUST;
				fMissedWork += fDWork;
				nSpansWithMissedHP += (fDWork != 0);
			}

			if ( fWorkDone >= fMissedWork || 0 == nSpansWithMissedHP )
			{
				for ( int i = 0; i < bridgeSpans.size(); ++i )
				{
					const SHPObjectRPGStats * pStats = bridgeSpans[i]->GetStats();
					bridgeSpans[i]->SetHitPoints( pStats->fMaxHP );
					bridgeSpans[i]->LockTiles();
				}
				fWorkLeft -= fMissedWork;
				pHomeTransport->DecResursUnitsLeft( fMissedWork );
				pUnit->SetCommandFinished();
				pBridgeToRepair->FinishRepair();
				for ( int i = 0; i < bridgeSpans.size(); ++i )
					bridgeSpans[i]->RealLockTiles();
			}
			else
			{
				const float fWorkPerSpan = fWorkDone / nSpansWithMissedHP;
				for ( int i = 0; i < bridgeSpans.size(); ++i )
				{
					const SHPObjectRPGStats * pStats = bridgeSpans[i]->GetStats();
					bridgeSpans[i]->SetHitPoints( bridgeSpans[i]->GetHitPoints() + fWorkPerSpan / (pStats->fRepairCost * SConsts::REPAIR_COST_ADJUST));
					fWorkLeft -= fWorkPerSpan;
					pHomeTransport->DecResursUnitsLeft( fWorkPerSpan );	
				}
			}
			
			fWorkDone = 0;
			if ( 1.0f >= fWorkLeft )
				pUnit->SetCommandFinished();
		}

		break;
	}
}

ETryStateInterruptResult CFormationRepairBridgeState::TryInterruptState( class CAICommand *pCommand )
{
	return TSIR_YES_IMMIDIATELY;
}

void CFormationRepairBridgeState::SetHomeTransport( class CAITransportUnit *pTransport )
{
	NI_ASSERT( eState == FRBS_WAIT_FOR_HOMETRANSPORT, "wrong state sequence" );
	eState = FRBS_START_APPROACH;
	pHomeTransport = pTransport;
	fWorkLeft = (std::min)( SConsts::ENGINEER_RU_CARRY_WEIGHT, pTransport->GetResursUnitsLeft() );
}

//*******************************************************************
//*										CFormationRepairBridgeState*
//*******************************************************************

IUnitState* CFormationRepairBuildingState::Instance( class CFormation *pFormation, class CBuilding *pBuilding )
{
	return new CFormationRepairBuildingState( pFormation, pBuilding );
}

CFormationRepairBuildingState::CFormationRepairBuildingState( class CFormation *pFormation, class CBuilding *pBuilding )
: pUnit( pFormation ), eState( EFRBS_WAIT_FOR_HOME_TRANSPORT ), pBuilding( pBuilding ), pHomeTransport( 0 ),
	fWorkAccumulator( 0.0f ), fWorkLeft( 0.0f ), lastRepearTime( 0 )
{
}

int CFormationRepairBuildingState::SendToNearestEntrance( CCommonUnit *pTransport, CBuilding * pStorage )
{
	//
	const int nEntrances = pStorage->GetNEntrancePoints();
	if ( nEntrances <= 0 )
		return -1;
	//NI_ASSERT( nEntrances != 0, "Storage without entrances on map!" );
	CPtr<IStaticPath> pShortestPath;
	float fPathLen = 1000000;
	int nNearestEntrance = -1;
	for ( int i = 0; i < nEntrances; ++i )
	{
		const CVec2 &vEntrance = pStorage->GetEntrancePoint(i);
		CPtr<IStaticPath> pStaticPath = CreateStaticPathToPoint( vEntrance, VNULL2, pTransport, true, GetAIMap() );
		if ( pStaticPath && fabs(pStaticPath->GetFinishPoint() - vEntrance) < sqr(SConsts::TRANSPORT_RESUPPLY_OFFSET) && pStaticPath->GetLength() < fPathLen )
		{
			fPathLen = pStaticPath->GetLength();
			pShortestPath = pStaticPath;
			nNearestEntrance = i;
		}
	}

	if ( pShortestPath )
	{
		pTransport->SendAlongPath( pShortestPath, VNULL2, true );
		return nNearestEntrance;
	}

	return -1;
}

void CFormationRepairBuildingState::Segment()
{
		// реакция на смещение юнита
	if ( EFRBS_WAIT_FOR_HOME_TRANSPORT != eState )
	{
		if ( !IsValidObj( pHomeTransport ) )
			TryInterruptState( 0 );
		if ( pBuilding->GetHitPoints() == pBuilding->GetStats()->fMaxHP )
			pUnit->SetCommandFinished();
	}

	switch ( eState )
	{
	case EFRBS_WAIT_FOR_HOME_TRANSPORT:
		if ( pUnit->GetNextCommand() )
			pUnit->SetCommandFinished();		

		break;
	case EFRBS_START_APPROACH:
		{
			const int nEntrance = SendToNearestEntrance( pUnit, pBuilding );
			if ( -1 != nEntrance )
			{
				eState = EFRBS_APPROACHING;
				break;
			}
			else
			{
				pUnit->SendAcknowledgement( ACK_NEGATIVE );
				Interrupt();
			}
		}

		break;
	case EFRBS_APPROACHING:
		if ( pUnit->IsIdle() )
			eState = EFRBS_START_SERVICE;

		break;
	case EFRBS_START_SERVICE:
		for ( int i = 0; i < pUnit->Size(); ++i )
			theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_COMMAND_USE, (float)ACTION_NOTIFY_USE_UP ), (*pUnit)[i] );

		eState = EFRBS_SERVICING;
		lastRepearTime = curTime;
		fWorkAccumulator =0;

		break;
	case EFRBS_SERVICING:
		if ( curTime - lastRepearTime > SConsts::TIME_QUANT )
		{
			fWorkAccumulator += pUnit->Size() * SConsts::RU_PER_QUANT * ( curTime - lastRepearTime ) / SConsts::TIME_QUANT;
			fWorkAccumulator = (std::min)( fWorkAccumulator, fWorkLeft );

			const float maxHP = pBuilding->GetStats()->fMaxHP;
			const float curHP = pBuilding->GetHitPoints();
			const float fRepCost = pBuilding->GetStats()->fRepairCost * SConsts::REPAIR_COST_ADJUST;
			
			const float fNewHP = Heal( maxHP, curHP, fRepCost, &fWorkAccumulator, &fWorkLeft, pHomeTransport );
			pBuilding->SetHitPoints( fNewHP );
			if ( pBuilding->GetStats()->fMaxHP == pBuilding->GetHitPoints() ||  //починили
					 fWorkLeft + fWorkAccumulator < fRepCost )
			{
				pUnit->SetCommandFinished();
			}

			lastRepearTime = curTime;
		}

		break;
	}
}

void CFormationRepairBuildingState::Interrupt()
{
	if ( !pUnit->IsIdle() )
		pUnit->Stop();

	pUnit->SetCommandFinished();
}

ETryStateInterruptResult CFormationRepairBuildingState::TryInterruptState( class CAICommand *pCommand )
{
	if ( !pCommand || pCommand->ToUnitCmd().nCmdType == ACTION_MOVE_CATCH_TRANSPORT )
	{
		Interrupt();
		return TSIR_YES_IMMIDIATELY;
	}
	else if ( pCommand->ToUnitCmd().nCmdType == ACTION_MOVE_SET_HOME_TRANSPORT ||
						pCommand->ToUnitCmd().nCmdType == ACTION_COMMAND_LOAD )
	{
		return TSIR_YES_WAIT;
	}
	return TSIR_NO_COMMAND_INCOMPATIBLE;
}

void CFormationRepairBuildingState::SetHomeTransport( class CAITransportUnit *pTransport )
{
	NI_ASSERT( eState == EFRBS_WAIT_FOR_HOME_TRANSPORT, "wrong state" );
	eState = EFRBS_START_APPROACH;
	pHomeTransport = pTransport;
	fWorkLeft = (std::min)( SConsts::ENGINEER_RU_CARRY_WEIGHT, pTransport->GetResursUnitsLeft() );
}


