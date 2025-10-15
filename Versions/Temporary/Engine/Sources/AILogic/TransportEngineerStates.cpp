#include "stdafx.h"

#include <float.h>

#include "PathFinder.h"
#include "TransportStates.h"
#include "UnitsIterators2.h"
#include "Technics.h"
#include "GroupLogic.h"
#include "UnitCreation.h"
#include "NewUpdater.h"
#include "Building.h"
#include "Formation.h"
#include "Soldier.h"
#include "Artillery.h"
#include "FormationStates.h"
#include "EntrenchmentCreation.h"
#include "ComplexObstacleCreation.h"
#include "BridgeCreation.h"
#include "Bridge.h"
#include "StaticObjectsIters.h"
#include "Common_RTS_AI/PathFinder.h"
#include "AIGeometry.h"
#include "FeedBackSystem.h"
#include "DebugTools/DebugInfoManager.h"

#include "AILogic_export.h"

REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D470, CTransportRepairState );
REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D471, CTransportResupplyState );
REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D472, CTransportHookArtilleryState );
REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D473, CTransportUnhookArtilleryState );

extern CFeedBackSystem theFeedBackSystem;
extern CDiplomacy theDipl;
extern CEventUpdater updater;
extern CUnitCreation theUnitCreation;
extern CGroupLogic theGroupLogic;
extern CUnits units;
extern NTimer::STime curTime;
extern CStaticObjects theStatObjs;

//*******************************************************************
//*											CTransportResupplyState											*
//*******************************************************************

IUnitState* CTransportResupplyState::Instance( CAITransportUnit *pTransport, const CVec2 &vServePoint, CAIUnit *_pPreferredUnit )
{
	return new CTransportResupplyState( pTransport, vServePoint, _pPreferredUnit );
}

CTransportResupplyState::CTransportResupplyState( CAITransportUnit *_pTransport, const CVec2 &vServePoint, CAIUnit *_pPreferredUnit )
: CTransportServeState( _pTransport, vServePoint, _pPreferredUnit )
{
	SetStatus( EUS_RESUPPLY );
}

bool CTransportResupplyState::FindUnitToServe( bool *pIsNotEnoughRU )
{
	CFormationServeUnitState::CFindFirstUnitPredicate pred;
	CFormationResupplyUnitState::FindUnitToServe( pTransport->GetCenterPlain(), 
																								pTransport->GetPlayer(), 
																								(std::min)(pTransport->GetResursUnitsLeft(), SConsts::ENGINEER_RU_CARRY_WEIGHT),
																								pLoaderSquad, &pred, pPreferredUnit );
	*pIsNotEnoughRU = pred.IsNotEnoughRu();
	return pred.HasUnit();
}

void CTransportResupplyState::UpdateActionBegin()
{
	if ( !bUpdatedActionsBegin )
		pTransport->SendAcknowledgement( ACK_START_SERVICE_RESUPPLY );
	bUpdatedActionsBegin = true;
}

void CTransportResupplyState::SendLoaders()
{
	theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_RESUPPLY_UNIT, pPreferredUnit ? pPreferredUnit->GetUniqueId() : 0 ), pLoaderSquad, false );
	theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_SET_HOME_TRANSPORT, pTransport->GetUniqueId()), pLoaderSquad, true );
	theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_CATCH_TRANSPORT, pTransport->GetUniqueId()), pLoaderSquad, true );
}

//*******************************************************************
//*											CTransportRepairState													*
//*******************************************************************

IUnitState* CTransportRepairState::Instance( CAITransportUnit *pTransport, const CVec2 &vServePoint, CAIUnit *_pPreferredUnit )
{
	return new CTransportRepairState( pTransport, vServePoint, _pPreferredUnit );
}

CTransportRepairState::CTransportRepairState( CAITransportUnit *pTransport, const CVec2 &vServePoint, CAIUnit *_pPreferredUnit )
: CTransportServeState( pTransport, vServePoint, _pPreferredUnit )
{
	SetStatus( EUS_REPAIR );
}

void CTransportRepairState::UpdateActionBegin()
{
	if ( !bUpdatedActionsBegin )
		pTransport->SendAcknowledgement( ACK_START_SERVICE_REPAIR );
	bUpdatedActionsBegin = true;
}

bool CTransportRepairState::FindUnitToServe( bool *pIsNotEnoughRU )
{
	CFormationServeUnitState::CFindFirstUnitPredicate pred;
	CFormationRepairUnitState::FindUnitToServe( pTransport->GetCenterPlain(), 
																							pTransport->GetPlayer(), 
																							pTransport->GetResursUnitsLeft(), 
																							pLoaderSquad, &pred,
																							pPreferredUnit );
	*pIsNotEnoughRU |= pred.IsNotEnoughRu();
	return pred.HasUnit();
}

void CTransportRepairState::SendLoaders()
{
	theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_REPAIR_UNIT, pPreferredUnit ? pPreferredUnit->GetUniqueId() : 0 ), pLoaderSquad, false );
	theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_SET_HOME_TRANSPORT, pTransport->GetUniqueId()), pLoaderSquad, true );
	theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_CATCH_TRANSPORT, pTransport->GetUniqueId()), pLoaderSquad, true );
}

//*******************************************************************
//*											CTransportLoadRuState												*
//*******************************************************************

IUnitState* CTransportLoadRuState::Instance( CAITransportUnit *pTransport, const bool bSubState, CBuilding *_pPreferredStorage )
{
	return new CTransportLoadRuState( pTransport, bSubState, _pPreferredStorage );
}

CTransportLoadRuState::CTransportLoadRuState ( CAITransportUnit *_pTransport, const bool bSubState, CBuilding *_pPreferredStorage )
: CStatusUpdatesHelper( EUS_FILL_RESOURCE, _pTransport ), pTransport( _pTransport ), eState( ETLRS_SEARCH_FOR_STORAGE ), nEntrance( -1 ), bSubState( bSubState ),
	pStorage( _pPreferredStorage )
{
	if ( pTransport->GetResursUnitsLeft() == SConsts::TRANSPORT_RU_CAPACITY )
		pTransport->SetCommandFinished();
}

CBuilding* CTransportLoadRuState::FindNearestSource()
{
	// для поиска ближайшего хранилища
	class CFindNearestConnected : public CStaticObjects::IEnumStoragesPredicate 
	{
		CPtr<CBuilding> pNearest;
		float fPathLength;
	public:
		CFindNearestConnected() : fPathLength( 0 ) {  }
		virtual bool OnlyConnected() const { return true; }
		// true - закончить, то, что нужно уже нашлось
		virtual bool AddStorage( class CBuilding * pStorage, const float _fPathLength )
		{
			if ( pStorage->IsAlive() && ( !pNearest || fPathLength > _fPathLength ) )
			{
				fPathLength = _fPathLength;
				pNearest = pStorage;
			}
			return false;
		}
		class CBuilding * GetNearest() { return pNearest; };
	};
	
	CFindNearestConnected pred;
	theStatObjs.EnumStoragesInRange( pTransport->GetCenterPlain(), 
																	 pTransport->GetParty(),
																	 SConsts::RESUPPLY_MAX_PATH,
																	 SConsts::TRANSPORT_LOAD_RU_DISTANCE,
																	 pTransport,
																	 &pred );
	return pred.GetNearest();
}

void CTransportLoadRuState::Segment()
{
	if ( ETLRS_SEARCH_FOR_STORAGE != eState && ( !IsValidObj( pStorage ) || theDipl.GetNParty(pStorage->GetPlayer()) != pTransport->GetParty() ) )
	{
		Interrupt();
		return;
	}

	switch( eState )
	{
	case ETLRS_SEARCH_FOR_STORAGE:
		{
			if ( (
							IsValidObj( pStorage ) &&	// preferred storage exists
							theDipl.GetNParty(pStorage->GetPlayer()) == pTransport->GetParty()	// and of our party
					 ) ||
					 ( 
							//theStatObjs.IsPointUnderSupply( pTransport->GetPlayer(), pTransport->GetCenter() ) &&
							(pStorage = FindNearestSource()) 
					 )
				 )
			{
				if ( -1 != (nEntrance = CFormationRepairBuildingState::SendToNearestEntrance( pTransport, pStorage ))  )
				{
					if ( bSubState )
						pTransport->SendAcknowledgement( ACK_GOING_TO_STORAGE, true );
					eState = ETLRS_APPROACHING_STORAGE;
				}
				else //if ( bSubState )
					pTransport->SendAcknowledgement( ACK_NO_RESOURCES_CANT_FIND_DEPOT, true );
			}
			else //if ( bSubState )
				pTransport->SendAcknowledgement( ACK_NO_RESOURCES_CANT_FIND_DEPOT, true );

			if ( ETLRS_APPROACHING_STORAGE != eState )
			{
				if ( pTransport->GetPlayer() == theDipl.GetMyNumber() )
					updater.AddUpdate( EFB_WORK_TERMINATED, MAKELONG( pTransport->GetCenterPlain().x, pTransport->GetCenterPlain().y), 0 );
				// почистить очередь команд у транспорта
				theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_GUARD), pTransport, false );
				Interrupt();
			}
			else
				InitStatus();
		}	

		break;
	case ETLRS_APPROACHING_STORAGE:
		if ( !IsValidObj( pStorage ) )
			eState = ETLRS_SEARCH_FOR_STORAGE;
		else if ( fabs2( pTransport->GetCenterPlain()-pStorage->GetEntrancePoint(nEntrance)) < sqr(SConsts::TRANSPORT_LOAD_RU_DISTANCE) )
		{
			pTransport->Stop();
			eState = ETLRS_START_LOADING_RU;
		}
		else if ( pTransport->IsIdle() )
			eState = ETLRS_START_LOADING_RU;

		break;
	case ETLRS_START_LOADING_RU:
		// send loaders 
		if ( IsValidObj( pStorage ) )
		{
			if ( pTransport->UpdateExternalLoaders() )
			{
				//create squad of resupply engineers
				CreateSquad();

				if ( pLoaderSquad )
				{
					CAITransportUnit::PrepareLoaders( pLoaderSquad, pTransport );
					theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_LOAD_RU, pStorage->GetUniqueId() ), pLoaderSquad, false );
					theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_SET_HOME_TRANSPORT, pTransport->GetUniqueId()), pLoaderSquad, true );
					theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_CATCH_TRANSPORT, pTransport->GetUniqueId(), SConsts::ENGINEER_RU_CARRY_WEIGHT ), pLoaderSquad, true );
					eState = ETLRS_LOADING_RU;
				}
			}
		}
		else
			eState = ETLRS_SEARCH_FOR_STORAGE;

		break;
	case ETLRS_LOADING_RU:
		// wait utill resurs are full
		if ( !IsValidObj( pLoaderSquad ) )
		{
			const float fRes = pTransport->GetResursUnitsLeft();
			if ( fRes < SConsts::TRANSPORT_RU_CAPACITY )
			{
				// send again.
				eState = ETLRS_START_LOADING_RU;
			}
			else 
				Interrupt();
		}

		break;
	case ETLRS_WAIT_FOR_LOADERS:
		if ( IsValidObj( pLoaderSquad ) )
		{
			if ( pLoaderSquad->IsInTransport() )
			{
				pTransport->Unlock();
				pLoaderSquad->Disappear();
				pTransport->SetCommandFinished();
			}
		}
		else
			pTransport->SetCommandFinished();
		
		break;

	}
}

void CTransportLoadRuState::Interrupt()
{
	if ( bSubState )
		eState  = ETLRS_SUBSTATE_FINISHED;
	else 
		TryInterruptState( 0 );
}

void CTransportLoadRuState::CreateSquad()
{
	pTransport->Unlock();
	if ( IsValidObj( pLoaderSquad ) )
		pLoaderSquad->Disappear();
	
	if ( pTransport->GetPlayer() == theDipl.GetNeutralPlayer() )
	{
		pLoaderSquad = 0;
		pTransport->SetCommandFinished();
	}
	else
	{
		pLoaderSquad = theUnitCreation.CreateResupplyEngineers( pTransport );
		//pTransport->Lock( pLoaderSquad );
	}
}

ETryStateInterruptResult CTransportLoadRuState::TryInterruptState( class CAICommand *pCommand )
{
	// если грузчики еще не в транспорте, то послать их догонять.
	if ( pCommand && pTransport->IsAlive() )
	{
		if ( IsValidObj( pLoaderSquad ) )
		{
			if ( pLoaderSquad->IsInTransport() )
				pLoaderSquad->Disappear();
			else
			{
				theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_CATCH_TRANSPORT, pTransport->GetUniqueId()), pLoaderSquad, false );
				pTransport->AddExternLoaders( pLoaderSquad );
			}
		}

		pTransport->Unlock();
		pTransport->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}
	else
	{
		CAITransportUnit::FreeLoaders( pLoaderSquad, pTransport );
		pTransport->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}

}

//*******************************************************************
//*											CTransportServeState												*
//*******************************************************************

CTransportServeState::CTransportServeState( class CAITransportUnit *_pTransport, const CVec2 & _vServePoint, CAIUnit *_pPreferredUnit )
: CStatusUpdatesHelper( EUS_UNDEFINED, _pTransport ), pTransport( _pTransport ),eState( ETRS_WAIT_FOR_UNLOCK ),timeLastUpdate ( curTime ),
	vServePoint( _vServePoint + _pTransport->GetGroupShift() ), bUpdatedActionsBegin( false ),
	pPreferredUnit( _pPreferredUnit ), bWaitForPath( false ), bSendFinishFeedback( false )
{
	if ( IsValidObj( pPreferredUnit ) )
		vServePoint = pPreferredUnit->GetCenterPlain();

	// for artillery crew
	if ( IsValidObj( pPreferredUnit ) )
	{
		const SUnitBaseRPGStats *pStats = pPreferredUnit->GetStats();
		if ( pStats->IsInfantry() )
		{
			IUnitState *pCurrentState = static_cast_ptr<CSoldier*>(pPreferredUnit)->GetFormation()->GetState();
			if ( pCurrentState && pCurrentState->GetName() == EUSN_GUN_CREW_STATE )
			{
				CFormationGunCrewState *pGunCrewState = checked_cast<CFormationGunCrewState*>( pCurrentState );
				pPreferredUnit = pGunCrewState->GetArtillery();
			}
		}
	}
}

void CTransportServeState::Segment()
{
	switch (eState)
	{
	case ETRS_WAIT_FOR_UNLOCK:
		if ( !pTransport->IsLocked() )
		{
			if ( pTransport->GetPlayer() == theDipl.GetNeutralPlayer() )
			{
				pLoaderSquad = 0;
				pTransport->SetCommandFinished();
			}
			else
			{
				eState = ETRS_INIT;
				InitStatus();
			}
		}

		break;
	case ETRS_INIT:
		eState = ETRS_START_APPROACH;

		break;
	case ETRS_GOING_TO_STORAGE:
		theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_MOVE_LOAD_RU), pTransport );

		break;
	case ETRS_START_APPROACH:
		{
			if ( pTransport->GetResursUnitsLeft() == 0.0f )
				eState = ETRS_GOING_TO_STORAGE;
			else
			{
				if ( IsUnitNearPoint( pTransport, vServePoint, SConsts::TRANSPORT_RESUPPLY_OFFSET ) )
				{
					eState = ETRS_APPROACHING;
				}
				else
				{
					CPtr<IStaticPath> pStaticPath = CreateStaticPathToPoint( vServePoint, pTransport->GetGroupShift(), pTransport, true, GetAIMap() );

					if ( pStaticPath )
					{
						// if path is presize - wait for finish
						bWaitForPath = fabs2( pStaticPath->GetFinishPoint() - vServePoint ) < sqr( int(SConsts::TILE_SIZE) );
						pTransport->SendAlongPath( pStaticPath, pTransport->GetGroupShift(), true );
						timeLastUpdate = curTime;
						eState = ETRS_APPROACHING;
					}
					else
					{
						pTransport->SendAcknowledgement( ACK_NEGATIVE_NOTIFICATION );
						TryInterruptState( 0 );
					}
				}
			}
		}

		break;
	case ETRS_APPROACHING:
		{
			// preferred unit moved
			if ( IsValidObj( pPreferredUnit ) && fabs2( vServePoint- pPreferredUnit->GetCenterPlain() ) > sqr( SConsts::RESUPPLY_RADIUS ) )
			{
				eState = ETRS_START_APPROACH;
				vServePoint = pPreferredUnit->GetCenterPlain();
			}
			else if ( bWaitForPath ? pTransport->IsIdle() : IsUnitNearPoint( pTransport, vServePoint, SConsts::TRANSPORT_RESUPPLY_OFFSET ) )
			{
				pTransport->Stop();
				eState = ETRS_CREATE_SQUAD;
			}
			else if ( pTransport->IsIdle() )
			{
				pTransport->SendAcknowledgement( ACK_NEGATIVE_NOTIFICATION );
				TryInterruptState( 0 );
			}
		}			
		break;
	case ETRS_CREATE_SQUAD:
		//if ( pTransport->UpdateExternalLoaders() )
		{
			//create squad of resupply engineers
			CreateSquad();

			if ( pLoaderSquad )
			{
				eState = ETRS_FINDING_UNIT_TO_SERVE;
				CAITransportUnit::PrepareLoaders( pLoaderSquad, pTransport );
			}
		}

		break;
	case ETRS_FINDING_UNIT_TO_SERVE:
		{
			if ( IsValidObj( pPreferredUnit ) && fabs2( vServePoint- pPreferredUnit->GetCenterPlain() ) > sqr( SConsts::RESUPPLY_RADIUS ) )
			{
				eState = ETRS_START_APPROACH;
				theFeedBackSystem.RemoveFeedback( pTransport->GetUniqueId(), EFB_ENGINEER_WORK_FINISHED );
				vServePoint = pPreferredUnit->GetCenterPlain();
				break;
			}
 			bool isNotEnoughRU = false;
			bool bFound = FindUnitToServe( &isNotEnoughRU );
			if ( !isNotEnoughRU )
			{
				UpdateActionBegin();
				SendLoaders();
				eState = ETRS_LOADERS_INROUTE;
				theFeedBackSystem.RemoveFeedback( pTransport->GetUniqueId(), EFB_ENGINEER_WORK_FINISHED );
			}
			else if ( isNotEnoughRU && pTransport->GetResursUnitsLeft() != SConsts::TRANSPORT_RU_CAPACITY )
			{
				eState = ETRS_GOING_TO_STORAGE;
				theFeedBackSystem.RemoveFeedback( pTransport->GetUniqueId(), EFB_ENGINEER_WORK_FINISHED );
			}
		}

		break;
	case ETRS_WAIT_FOR_UNIT_TO_SERVE:
		// стоим и ждем не понадобятся ли услуги еще.
		if ( curTime - timeLastUpdate > pTransport->GetBehUpdateDuration() )
		{
			if ( !bSendFinishFeedback )
			{
				if ( pTransport->GetPlayer() == theDipl.GetMyNumber() )
					theFeedBackSystem.AddFeedback( pTransport->GetUniqueId(), pTransport->GetCenterPlain(), EFB_ENGINEER_WORK_FINISHED, -1 );
				bSendFinishFeedback = true;
			}
			eState = ETRS_FINDING_UNIT_TO_SERVE;
		}
		break;
	case ETRS_LOADERS_INROUTE:
		if ( IsValidObj( pPreferredUnit ) && fabs2( vServePoint- pPreferredUnit->GetCenterPlain() ) > sqr( SConsts::RESUPPLY_RADIUS ) )
		{
			eState = ETRS_START_APPROACH;
			vServePoint = pPreferredUnit->GetCenterPlain();
		}

		if ( curTime - timeLastUpdate < pTransport->GetBehUpdateDuration() )
		{
		}
		else 
		{
			timeLastUpdate = curTime;
			if ( !IsValidObj( pLoaderSquad ) || pLoaderSquad->IsInTransport() )
				eState = ETRS_CREATE_SQUAD;
		}			

		break;
	case ETRS_WAIT_FOR_LOADERS:
		if ( IsValidObj( pLoaderSquad ) )
		{
			if ( pLoaderSquad->IsInTransport() )
			{
				pTransport->Unlock();
				pLoaderSquad->Disappear();
				pTransport->SetCommandFinished();
			}
		}
		else
			pTransport->SetCommandFinished();
		
		break;
	}
}

void CTransportServeState::CreateSquad()
{
	if ( !pTransport || !pTransport->IsRefValid() || !pTransport->IsAlive() )
	{
		pLoaderSquad = 0;
		return;
	}

	pTransport->Unlock();
	if ( IsValidObj( pLoaderSquad ) )
		pLoaderSquad->Disappear();

	if ( pTransport->GetPlayer() == theDipl.GetNeutralPlayer() )
	{
		pLoaderSquad = 0;
		pTransport->SetCommandFinished();
	}
	else
	{
		pLoaderSquad = theUnitCreation.CreateResupplyEngineers( pTransport );
		//pTransport->Lock( pLoaderSquad );
	}
}

ETryStateInterruptResult CTransportServeState::TryInterruptState( class CAICommand *pCommand )
{
	// если грузчики еще не в транспорте, то послать их догонять.
	if ( pTransport->IsRefValid() && pTransport->IsAlive() )
	{
		if ( IsValidObj( pLoaderSquad ) )
		{
			if ( pLoaderSquad->IsInTransport() )
				pLoaderSquad->Disappear();
			else
			{
				theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_CATCH_TRANSPORT, pTransport->GetUniqueId()), pLoaderSquad, false );
				pTransport->AddExternLoaders( pLoaderSquad );
			}
		}

		pTransport->Unlock();
		pTransport->SetCommandFinished();

		return TSIR_YES_IMMIDIATELY;
	}
	else
	{
		if ( pTransport->IsRefValid() && pTransport->GetPlayer() == theDipl.GetMyNumber() )
			updater.AddUpdate( EFB_WORK_TERMINATED, MAKELONG( pTransport->GetCenterPlain().x, pTransport->GetCenterPlain().y), 0 );
		CAITransportUnit::FreeLoaders( pLoaderSquad, pTransport );
		pTransport->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}
}

//*******************************************************************
//*											CTransportBuildState												*
//*******************************************************************

CTransportBuildState::CTransportBuildState ( class CAITransportUnit *pTransport, const class CVec2 & vDestPoint )
: CStatusUpdatesHelper( EUS_UNDEFINED, pTransport ), eState( ETBS_ESTIMATE ), pUnit( pTransport ), vStartPoint( vDestPoint + pTransport->GetGroupShift() )
{
}

void CTransportBuildState::Segment()
{
	switch( eState )
	{
	case ETBS_ESTIMATE:
		InitStatus();
		if ( IsEndPointNeeded() )
			eState = ETBS_WAIT_FOR_ENDPOINT;
		else
			eState = ETBS_END_POINT_READY;

		break;
	case ETBS_END_POINT_READY:
		if ( IsWorkDone() )
		{
			pUnit->SendAcknowledgement( ACK_NEGATIVE_NOTIFICATION );
			pUnit->SetCommandFinished();
		}
		else if ( IsEnoughResources() )
			eState = ETBS_START_APPROACH;
		else
			eState = ETBS_LOADING_RESOURCES;

		break;
	case ETBS_WAIT_FOR_ENDPOINT:
		if ( pUnit->GetNextCommand() )
		{
			TryInterruptState( 0 );
		}

		break;
	case ETBS_START_APPROACH:
		if ( !HaveToSendEngeneersNow() )
		{
			SendTransportToBuildPoint();
			eState = ETBS_APROACHING_BUILDPOINT;
		}
		else
		{
			pUnit->SendAcknowledgement( NDb::ACK_START_SERVICE_BUILDING, pUnit->GetPlayer() == theDipl.GetMyNumber() );
			eState = ETBS_CREATE_SQUAD;
		}
		break;
	case ETBS_LOADING_RESOURCES:
		if ( !pLoadRuSubState )
			pLoadRuSubState = checked_cast<CTransportLoadRuState*>(CTransportLoadRuState::Instance( pUnit, true ));
		pLoadRuSubState->Segment();
		if ( pLoadRuSubState->IsSubStateFinished() )
		{
			pLoadRuSubState = 0;
			if ( IsEnoughResources() )
				eState = ETBS_START_APPROACH;
			else
				TryInterruptState( 0 );
		}

		break;
	case ETBS_APROACHING_BUILDPOINT:
		if ( HaveToSendEngeneersNow() )
		{
			pUnit->SendAcknowledgement( NDb::ACK_START_SERVICE_BUILDING, pUnit->GetPlayer() == theDipl.GetMyNumber() );
			pUnit->Stop();
			eState = ETBS_CREATE_SQUAD;
		}
		else if ( pUnit->IsIdle() )
		{
			if ( MustSayNegative() )
				pUnit->SendAcknowledgement( ACK_NEGATIVE_NOTIFICATION );
			if ( pUnit->GetPlayer() == theDipl.GetMyNumber() )
				theFeedBackSystem.AddFeedbackAndForget( pUnit->GetUniqueID(), pUnit->GetCenterPlain(), EFB_ENGINEER_WORK_FINISHED, -1 );
			TryInterruptState( 0 );
		}
		
		break;
	case ETBS_CREATE_SQUAD:
		{	
			pUnit->Unlock();
			if ( IsValidObj( pEngineers ) )
				pEngineers->Disappear();

			if ( pUnit->GetPlayer() == theDipl.GetNeutralPlayer() )
			{
				pEngineers = 0;
				if ( pUnit->GetPlayer() == theDipl.GetMyNumber() )
					theFeedBackSystem.AddFeedbackAndForget( pUnit->GetUniqueID(), pUnit->GetCenterPlain(), EFB_ENGINEER_WORK_FINISHED, -1 );

				TryInterruptState( 0 );
			}
			else
			{
				pEngineers = theUnitCreation.CreateResupplyEngineers( pUnit );
				pUnit->Lock( pEngineers );
			}
		}

		if ( pEngineers )
		{
			CAITransportUnit::PrepareLoaders( pEngineers, pUnit );
			SendEngineers();
			eState = ETBS_WAIT_FINISH_BUILD;
		}

		break;
	case ETBS_WAIT_FINISH_BUILD:
		if ( !IsValidObj( pEngineers ) || pEngineers->IsInTransport() )
		{
			pUnit->Unlock();
			if ( IsValidObj( pEngineers ) )
				pEngineers->Disappear();
			
			if ( IsWorkDone() )
			{
				if ( pUnit->GetPlayer() == theDipl.GetMyNumber() )
				{
					if ( MustSayNegative() )
						updater.AddUpdate( EFB_WORK_TERMINATED, MAKELONG( pUnit->GetCenterPlain().x, pUnit->GetCenterPlain().y), 0 );
					else
						theFeedBackSystem.AddFeedbackAndForget( pUnit->GetUniqueID(), pUnit->GetCenterPlain(), EFB_ENGINEER_WORK_FINISHED, -1 );
				}

				TryInterruptState( 0 );
			}
			else if ( !IsEnoughResources() )
			{
				NotifyGoToStorage();
				eState = ETBS_LOADING_RESOURCES;
			}
			else
				eState = ETBS_CREATE_SQUAD;
		}
			
		break;
	case ETBS_WAIT_FOR_LOADERS:
		if ( IsValidObj( pEngineers ) )
		{
			if ( pEngineers->IsInTransport() )
			{
				pUnit->Unlock();
				pEngineers->Disappear();
				if ( pUnit->GetPlayer() == theDipl.GetMyNumber() )
					theFeedBackSystem.AddFeedbackAndForget( pUnit->GetUniqueID(), pUnit->GetCenterPlain(), EFB_ENGINEER_WORK_FINISHED, -1 );
				TryInterruptState( 0 );
			}
		}
		else
		{
			if ( pUnit->GetPlayer() == theDipl.GetMyNumber() )
				theFeedBackSystem.AddFeedbackAndForget( pUnit->GetUniqueID(), pUnit->GetCenterPlain(), EFB_ENGINEER_WORK_FINISHED, -1 );
			TryInterruptState( 0 );
		}

		break;
	}
}

void CTransportBuildState::SetEndPoint( const CVec2& _vEndPoint )
{
	NI_ASSERT( eState == ETBS_WAIT_FOR_ENDPOINT, "wrong states sequence" );
	if ( eState != ETBS_WAIT_FOR_ENDPOINT )
	{
		TryInterruptState( 0 );
		return;
	}
	vEndPoint = _vEndPoint + pUnit->GetGroupShift();
	eState = ETBS_END_POINT_READY;
}

ETryStateInterruptResult CTransportBuildState::TryInterruptState( class CAICommand *pCommand )
{
	if ( IsValidObj( pUnit ) )
	{
		if ( IsValidObj( pEngineers ) )
		{
			if ( pEngineers->IsInTransport() )
				pEngineers->Disappear();
			else
			{
				theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_CATCH_TRANSPORT, pUnit->GetUniqueId()), pEngineers, false );
				pUnit->AddExternLoaders( pEngineers );
			}
		}

		pUnit->Unlock();
		pUnit->SetCommandFinished();

		return TSIR_YES_IMMIDIATELY;
	}
	else
	{
		if ( pUnit->IsRefValid() && pUnit->GetPlayer() == theDipl.GetMyNumber() )
			updater.AddUpdate( EFB_WORK_TERMINATED, MAKELONG( pUnit->GetCenterPlain().x, pUnit->GetCenterPlain().y), 0 );
		CAITransportUnit::FreeLoaders( pEngineers, pUnit );
		pUnit->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}
}

//*******************************************************************
//*											CTransportBuildLongObjectState							*
//*******************************************************************

CTransportBuildLongObjectState::CTransportBuildLongObjectState ( class CAITransportUnit *pTransport, const class CVec2 & vDestPoint, class CLongObjectCreation *_pCreation )
: pCreation( _pCreation ), CTransportBuildState( pTransport, vDestPoint ) 
{
}

void CTransportBuildLongObjectState::SetEndPoint( const CVec2& _vEndPoint )
{
	CTransportBuildState::SetEndPoint( _vEndPoint );
	pUnit->UnlockTiles();
	//
	PreCreate();
}

bool CheckThePoint( CVec2 *vPoint, const CVec2 &vStart, const CVec2 &vEnd, float fMinDist )
{
	vPoint->x = Clamp( vPoint->x, 128.0f, float( ( GetAIMap()->GetSizeX() - 4 ) * SConsts::TILE_SIZE) );
	vPoint->y = Clamp( vPoint->y, 128.0f, float( ( GetAIMap()->GetSizeY() - 4 ) * SConsts::TILE_SIZE) );
	
	return GetDistanceToSegment( vStart, vEnd, *vPoint ) > fMinDist;
}

void CTransportBuildLongObjectState::SendTransportToBuildPoint()
{
	const float fCurDist = GetDistanceToSegment( vStartPoint, vEndPoint, pUnit->GetCenterPlain() );
	const float fRadius = pUnit->GetStats()->vAABBHalfSize.x + pUnit->GetStats()->vAABBHalfSize.y + 7 * SConsts::TILE_SIZE;
	const float fMaxRadius = SConsts::TRANSPORT_RESUPPLY_OFFSET > fRadius ? SConsts::TRANSPORT_RESUPPLY_OFFSET : fRadius * 2;

	if ( fCurDist > fMaxRadius )
	{
		// подъезжать нужно
		CPtr<IStaticPath> pPath = CreateStaticPathToPoint( vStartPoint, VNULL2, pUnit, true, GetAIMap() );
		if ( pPath )
			pUnit->SendAlongPath( pPath, VNULL2, true );
	}
	else if ( fCurDist < fRadius )
	{
		for ( float fDist = fRadius + SConsts::TILE_SIZE * 7; fDist < fMaxRadius; fDist += SConsts::TILE_SIZE )
		{
			for ( int nAngle = 0; nAngle < 8; ++nAngle )
			{
				CVec2 vPoint( (vStartPoint +vEndPoint) * 0.5f + GetVectorByDirection( 65535 / 8 * nAngle ) * fDist );
#ifndef _FINALRELEASE
				if ( NGlobal::GetVar( "track_place_markers", 0 ) )
				{
					CSegment segm;
					segm.p1 = CVec2( vPoint.x + 10, vPoint.y + 10 );
					segm.p2 = CVec2( vPoint.x - 10, vPoint.y - 10 );
					segm.dir = segm.p2 - segm.p1;
					DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );
					segm.p1 = CVec2( vPoint.x + 10, vPoint.y - 10 );
					segm.p2 = CVec2( vPoint.x - 10, vPoint.y + 10 );
					segm.dir = segm.p2 - segm.p1;
					DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );
				}
#endif
				if ( CheckThePoint( &vPoint, vStartPoint, vEndPoint, fRadius ) )
				{
					CPtr<IStaticPath> pPath = CreateStaticPathToPoint( vPoint, VNULL2, pUnit, true, GetAIMap() );
					if ( pPath )
					{
						pUnit->SendAlongPath( pPath, VNULL2, true );
						return;
					}
				}
			}
		}
	}
	else
	{
		// уже на месте.
	}
}

bool CTransportBuildLongObjectState::HaveToSendEngeneersNow() 
{
	const float fDist = GetDistanceToSegment( vStartPoint, vEndPoint, pUnit->GetCenterPlain() );
	const float fRadius = pUnit->GetStats()->vAABBHalfSize.x + pUnit->GetStats()->vAABBHalfSize.y + 7 * SConsts::TILE_SIZE;
	const float fMaxRadius = SConsts::TRANSPORT_RESUPPLY_OFFSET > fRadius ? SConsts::TRANSPORT_RESUPPLY_OFFSET : fRadius * 2;

	return fDist > fRadius && fDist <= fMaxRadius;
}

bool CTransportBuildLongObjectState::IsEnoughResources() const
{
	return pUnit->GetResursUnitsLeft() > 0.0f; //pUnit->GetResursUnitsLeft() >= pCreation->GetPrice();
}

bool CTransportBuildLongObjectState::MustSayNegative() const
{
	return pCreation->GetCurIndex() != pCreation->GetMaxIndex();
}

bool CTransportBuildLongObjectState::IsWorkDone() const
{
	return pCreation->GetCurIndex() == pCreation->GetMaxIndex() || !pCreation->CanBuildNext();
}

void CTransportBuildLongObjectState::SendEngineers()
{
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_BUILD_LONGOBJECT, pCreation->GetUniqueId()), pEngineers, false );
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_SET_HOME_TRANSPORT, pUnit->GetUniqueId() ), pEngineers, true );
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_CATCH_TRANSPORT, pUnit->GetUniqueId()), pEngineers, true );
}

//*******************************************************************
//*											CTransportBuildFenceState										*
//*******************************************************************

IUnitState* CTransportBuildFenceState::Instance( class CAITransportUnit *pTransport, const class CVec2 &vStartPoint )
{
	return new CTransportBuildFenceState( pTransport, vStartPoint );
}

CTransportBuildFenceState::CTransportBuildFenceState( class CAITransportUnit *pTransport, const class CVec2 & vStartPoint )
	: CTransportBuildLongObjectState( pTransport, vStartPoint, NLongObjectCreation::Create<CComplexObstacleCreation>( vStartPoint, pTransport->GetPlayer(), true ) )
{
	SetStatus( EUS_BUILD_OBSTACLES );
}

void CTransportBuildFenceState::PreCreate()
{
	NLongObjectCreation::PreCreate<CComplexObstacleCreation>( vEndPoint, &pCreation, true );
}

//*******************************************************************
//*											CTransportBuildEntrenchmentState						*
//*******************************************************************

CTransportBuildEntrenchmentState::CTransportBuildEntrenchmentState( class CAITransportUnit *pTransport, const class CVec2 & vStartPoint )
: CTransportBuildLongObjectState( pTransport, vStartPoint, NLongObjectCreation::Create<CEntrenchmentCreation>( vStartPoint, pTransport->GetPlayer(), true ) )
{
	SetStatus( EUS_DIG_TRENCHES );
}

IUnitState* CTransportBuildEntrenchmentState::Instance( class CAITransportUnit *pTransport, const class CVec2 & vStartPoint )
{
	return new CTransportBuildEntrenchmentState( pTransport, vStartPoint );
}
void CTransportBuildEntrenchmentState::PreCreate()
{
	NLongObjectCreation::PreCreate<CEntrenchmentCreation>( vEndPoint, &pCreation, true );
}

//*******************************************************************
//*											CTransportClearMineState										*
//*******************************************************************

CTransportClearMineState::CTransportClearMineState( class CAITransportUnit *pTransport, const class CVec2 & vDestPoint )
	: CTransportBuildState( pTransport, vDestPoint ), timeNextCheck( curTime ), bWorkDone( false )
{  
	// посчитать время, которое нужно для того, чтобы проехать 1 длину грузовичка
	SetStatus( EUS_DEMINE );
	const SMechUnitRPGStats * pStats = checked_cast<const SMechUnitRPGStats *>(pTransport->GetStats());
	timeCheckPeriod = 100;//SConsts::MINE_VIS_RADIUS / pStats->fSpeed / 2;
}

void CTransportClearMineState::SendTransportToBuildPoint()
{
	CPtr<IStaticPath> pPath = CreateStaticPathToPoint( vStartPoint, VNULL2, pUnit, true, GetAIMap() );
	if ( pPath )
		pUnit->SendAlongPath( pPath, VNULL2, true );
}

bool CTransportClearMineState::HaveToSendEngeneersNow() 
{
	if ( curTime >= timeNextCheck )
	{
		timeNextCheck = curTime + timeCheckPeriod;
		const int nParty = pUnit->GetParty();
 		for ( CMinesIter iter( pUnit->GetCenterPlain(), SConsts::MINE_CLEAR_RADIUS*2, nParty, true ); !iter.IsFinished(); iter.Iterate() )
		{
			if ( (*iter)->WillExplodeUnder( pUnit ) ) 
				return true;
		}
	}
	return false;
}

bool CTransportClearMineState::IsEnoughResources() const
{
	return true;
}

bool CTransportClearMineState::IsWorkDone() const
{
	return bWorkDone;
}

void CTransportClearMineState::SendEngineers()
{
	bWorkDone = true;
	const CVec2 vUnitCenter( pUnit->GetCenterPlain() );
	CVec2 vDir = vStartPoint - vUnitCenter;
	const float fMaxDist = fabs2( vDir );
	Normalize( &vDir );

	bool bToQueue = false;
	for ( CVec2 vCurPoint = vUnitCenter + vDir * SConsts::MINE_CLEAR_RADIUS;
				fabs2( vCurPoint - vUnitCenter ) < fMaxDist; 
				vCurPoint += vDir * SConsts::MINE_CLEAR_RADIUS )
	{
		theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_CLEARMINE, vCurPoint), pEngineers, bToQueue );	
		bToQueue = true;
	}

	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_CLEARMINE, vStartPoint ), pEngineers, bToQueue );
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_CATCH_TRANSPORT, pUnit->GetUniqueId() ), pEngineers, true );
}

bool CTransportClearMineState::IsEndPointNeeded() const
{
	return false;
}

IUnitState* CTransportClearMineState::Instance( class CAITransportUnit *pTransport, const class CVec2 & vStartPoint )
{
	return new CTransportClearMineState( pTransport, vStartPoint );
}

//*******************************************************************
//*											CTransportPlaceMineState										*
//*******************************************************************

void CTransportPlaceMineState::SendTransportToBuildPoint()
{
	CPtr<IStaticPath> pPath = CreateStaticPathToPoint( vStartPoint, VNULL2, pUnit, true, GetAIMap() );
	if ( pPath )
		pUnit->SendAlongPath( pPath, VNULL2, true );
	bTransportSent = true;
}

bool CTransportPlaceMineState::HaveToSendEngeneersNow() 
{
//	const SMechUnitRPGStats * pStats = checked_cast<const SMechUnitRPGStats*>( pUnit->GetStats() );

	if ( pUnit->IsIdle() && fabs2(pUnit->GetCenterPlain() - vStartPoint) <= sqr(SConsts::TRANSPORT_RESUPPLY_OFFSET * 2) )
		return true;

	if ( !pUnit->IsIdle() && fabs2(pUnit->GetCenterPlain() - vStartPoint) <= sqr(SConsts::TRANSPORT_RESUPPLY_OFFSET) )
		return true;

	if ( bTransportSent && pUnit->IsIdle() )
		return true;
	return false;
}

bool CTransportPlaceMineState::IsEnoughResources() const
{
	return pUnit->GetResursUnitsLeft() >= SConsts::MINE_RU_PRICE[eCurrType];
}

bool CTransportPlaceMineState::IsWorkDone() const
{
	return bWorkDone;
}

void CTransportPlaceMineState::SendEngineers()
{
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_PLACEMINE, vCorner + CVec2( nSqI * SAIConsts::TILE_SIZE, nSqJ * SAIConsts::TILE_SIZE ), float(eCurrType) ), pEngineers, false );
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_SET_HOME_TRANSPORT, pUnit->GetUniqueId()), pEngineers, true );
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_CATCH_TRANSPORT, pUnit->GetUniqueId()), pEngineers, true );
	eCurrType = MT_INFANTRY_AND_TECHNICS;
	bWorkDone = true;
}

bool CTransportPlaceMineState::IsEndPointNeeded() const
{
	return false;
}
CTransportPlaceMineState::CTransportPlaceMineState( class CAITransportUnit *pTransport, const class CVec2 & vDestPoint )
: CTransportBuildState( pTransport, vDestPoint ), bWorkDone( false ), bTransportSent( false ), eCurrType( MT_TECHNICS ), nSqI( 0 ), nSqJ( 0 ), nSquareSize( 2 )
{
	const float fShift = float( SAIConsts::TILE_SIZE * ( nSquareSize - 1 ) );
	vCorner.Set( vDestPoint.x - fShift, vDestPoint.y - fShift );
	SetStatus( EUS_SET_MINE_FIELD );
}

IUnitState* CTransportPlaceMineState::Instance( class CAITransportUnit *pTransport, const class CVec2 & vStartPoint )
{
	return new CTransportPlaceMineState( pTransport, vStartPoint );
}

//*******************************************************************
//*											CTransportPlaceAntitankState								*
//*******************************************************************

IUnitState* CTransportPlaceAntitankState::Instance( class CAITransportUnit *pTransport, const class CVec2 & vStartPoint )
{
	return new CTransportPlaceAntitankState( pTransport, vStartPoint );
}

CTransportPlaceAntitankState::CTransportPlaceAntitankState( class CAITransportUnit *pTransport, const class CVec2 & vDestPoint )
	: CTransportBuildState( pTransport, vDestPoint ), bWorkFinished ( false ), bSent( false )
{
}

void CTransportPlaceAntitankState::SendTransportToBuildPoint()
{
	CPtr<IStaticPath> pPath = CreateStaticPathToPoint( vStartPoint, VNULL2, pUnit, true, GetAIMap() );
	if ( pPath )
		pUnit->SendAlongPath( pPath, VNULL2, true );
	bSent = true;
}

bool CTransportPlaceAntitankState::HaveToSendEngeneersNow() 
{
//	const SMechUnitRPGStats * pStats = checked_cast<const SMechUnitRPGStats*>( pUnit->GetStats() );
	if ( pUnit->IsIdle() && fabs2(pUnit->GetCenterPlain() - vStartPoint) <= sqr(SConsts::TRANSPORT_RESUPPLY_OFFSET * 2) )
		return true;

	if ( !pUnit->IsIdle() && fabs2(pUnit->GetCenterPlain() - vStartPoint) <= sqr(SConsts::TRANSPORT_RESUPPLY_OFFSET) )
		return true;

	if ( bSent && pUnit->IsIdle() )
		return true;
	return false;
}

bool CTransportPlaceAntitankState::IsEnoughResources() const
{
	return pUnit->GetResursUnitsLeft() >= SConsts::ANTITANK_RU_PRICE;
}

bool CTransportPlaceAntitankState::IsWorkDone() const
{
	return bWorkFinished;
}

void CTransportPlaceAntitankState::SendEngineers()
{
	bWorkFinished = true;
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_PLACE_ANTITANK, vStartPoint), pEngineers, false );
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_SET_HOME_TRANSPORT, pUnit->GetUniqueId()), pEngineers, true );
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_CATCH_TRANSPORT, pUnit->GetUniqueId()), pEngineers, true );
}

bool CTransportPlaceAntitankState::IsEndPointNeeded() const
{
	return false;
}

//*******************************************************************
//*											CTransportRepairBridgeState									*
//*******************************************************************

IUnitState* CTransportRepairBridgeState::Instance( class CAITransportUnit *pTransport, class CBridgeSpan *pFullBridge )
{
	return new CTransportRepairBridgeState( pTransport, pFullBridge );
}

CTransportRepairBridgeState::CTransportRepairBridgeState( class CAITransportUnit *pTransport, class CBridgeSpan *pFullBridge )
: CTransportBuildState( pTransport, VNULL2 ), pBridgeToRepair( pFullBridge ), bSentToBuildPoint( false )
{
	std::vector< CObj<CBridgeSpan> > spans;
	pBridgeToRepair->GetFullBridge()->EnumSpans( &spans );
	SetStartPoint( CBridgeCreation::SortBridgeSpans( &spans, pTransport ) );
	SetStatus( EUS_REPAIR_OBJECT );
}

void CTransportRepairBridgeState::SendTransportToBuildPoint()
{
	CPtr<IStaticPath> pPath = CreateStaticPathToPoint( vStartPoint, VNULL2, pUnit, true, GetAIMap() );
	bSentToBuildPoint = true;
	if ( pPath )
		pUnit->SendAlongPath( pPath, VNULL2, true );
}

bool CTransportRepairBridgeState::HaveToSendEngeneersNow() 
{
	return bSentToBuildPoint && pUnit->IsIdle();
}

bool CTransportRepairBridgeState::IsEnoughResources() const
{
	return 1.0f < pUnit->GetResursUnitsLeft();
}

bool CTransportRepairBridgeState::IsWorkDone() const
{
	return pBridgeToRepair->GetFullBridge()->GetHPPercent() == 1.0f;
}

void CTransportRepairBridgeState::SendEngineers()
{
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_REPAIR_BRIDGE, pBridgeToRepair->GetUniqueId() ), pEngineers, false );
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_SET_HOME_TRANSPORT, pUnit->GetUniqueId()), pEngineers, true );
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_CATCH_TRANSPORT, pUnit->GetUniqueId()), pEngineers, true );
}

bool CTransportRepairBridgeState::IsEndPointNeeded() const 
{
	return false;
}

//*******************************************************************
//*											CTransportRepairBridgeState									*
//*******************************************************************

IUnitState* CTransportBuildBridgeState::Instance( class CAITransportUnit *pTransport, class CFullBridge *pFullBridge )
{
	return new CTransportBuildBridgeState( pTransport, pFullBridge );
}

CTransportBuildBridgeState::CTransportBuildBridgeState( class CAITransportUnit *_pTransport, class CFullBridge *_pFullBridge )
: CTransportBuildState( _pTransport, VNULL2 ), pFullBridge( _pFullBridge ), pCreation( new CBridgeCreation(_pFullBridge, _pTransport, true ) ),
	bTransportSent( false )
{
	SetStartPoint( pCreation->GetStartPoint() );
}

void CTransportBuildBridgeState::SendTransportToBuildPoint()
{
	CPtr<IStaticPath> pPath = CreateStaticPathToPoint( vStartPoint, VNULL2, pUnit, true, GetAIMap() );

	bTransportSent = true;
	if ( pPath )
		pUnit->SendAlongPath( pPath, VNULL2, true );
}

bool CTransportBuildBridgeState::HaveToSendEngeneersNow() 
{
	return bTransportSent && pUnit->IsIdle();
}

bool CTransportBuildBridgeState::IsEnoughResources() const
{
	return pUnit->GetResursUnitsLeft() > 0.0f;//return pCreation->GetPrice() <= pUnit->GetResursUnitsLeft();
}

bool CTransportBuildBridgeState::IsWorkDone() const
{
	return pCreation->GetCurIndex() >= pCreation->GetMaxIndex();
}

void CTransportBuildBridgeState::SendEngineers()
{
	bool bToQueue = false;
	if ( pCreation->IsFirstSegmentBuilt() )
	{
		theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_REPAIR_BRIDGE, pFullBridge->GetUniqueId() ), pEngineers, false );
		theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_SET_HOME_TRANSPORT, pUnit->GetUniqueId()), pEngineers, true );
		bToQueue = true;
	}
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_BUILD_LONGOBJECT, pCreation->GetUniqueId() ), pEngineers, bToQueue );
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_SET_HOME_TRANSPORT, pUnit->GetUniqueId()), pEngineers, true );
	
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_CATCH_TRANSPORT, pUnit->GetUniqueId()), pEngineers, true );
}

bool CTransportBuildBridgeState::IsEndPointNeeded() const 
{
	return false;
}

//*******************************************************************
//*											CTransportRepairBuildingState								*
//*******************************************************************

IUnitState* CTransportRepairBuildingState::Instance( class CAITransportUnit *pTransport, class CBuilding *pStaticObject )
{
	return new CTransportRepairBuildingState( pTransport, pStaticObject );
}

CTransportRepairBuildingState::CTransportRepairBuildingState( class CAITransportUnit *pTransport, class CBuilding *pStaticObject )
: CTransportBuildState( pTransport, CVec2(pStaticObject->GetCenter().x,pStaticObject->GetCenter().y) ), pBuilding( pStaticObject ), bSentToBuildPoint( false )
{
	SetStatus( EUS_REPAIR_OBJECT );
}

void CTransportRepairBuildingState::SendTransportToBuildPoint()
{
	int nEntrance = CFormationRepairBuildingState::SendToNearestEntrance( pUnit, pBuilding );
	if ( nEntrance != -1 )
	{
		pUnit->SendAcknowledgement( ACK_START_SERVICE_REPAIR, true );
		bSentToBuildPoint = true;
	}
	else
	{
		pUnit->SendAcknowledgement( ACK_CANNOT_FIND_PATH_TO_TARGET, true );
		TryInterruptState( 0 );
	}
}

bool CTransportRepairBuildingState::HaveToSendEngeneersNow() 
{
	const SMechUnitRPGStats * pStats = checked_cast<const SMechUnitRPGStats*>( pUnit->GetStats() );
	if ( bSentToBuildPoint && pUnit->IsIdle() || fabs2(pUnit->GetCenterPlain() - vStartPoint) <= sqr(pStats->vAABBHalfSize.y * 2) )
		return true;
	return false;
}

bool CTransportRepairBuildingState::IsEnoughResources() const
{
	return pBuilding->GetStats()->fRepairCost * SConsts::REPAIR_COST_ADJUST <= pUnit->GetResursUnitsLeft();
}

bool CTransportRepairBuildingState::IsWorkDone() const
{
	return pBuilding->GetHitPoints() == pBuilding->GetStats()->fMaxHP;
}

void CTransportRepairBuildingState::SendEngineers()
{
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_REPAIR_BUILDING, pBuilding->GetUniqueId() ), pEngineers, false );
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_SET_HOME_TRANSPORT, pUnit->GetUniqueId()), pEngineers, true );
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_CATCH_TRANSPORT, pUnit->GetUniqueId()), pEngineers, true );
}

bool CTransportRepairBuildingState::IsEndPointNeeded() const 
{
	return false;
}


