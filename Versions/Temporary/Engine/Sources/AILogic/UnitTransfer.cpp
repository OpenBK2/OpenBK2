#include "stdafx.h"

#include "AILogicInternal.h"
#include "Artillery.h"
#include "Formation.h"
#include "GroupLogic.h"
#include "NewUpdater.h"
#include "ScenarioTracker.h"
#include "Soldier.h"
#include "Technics.h"
#include "UnitStates.h"
#include "UnitTransfer.h"

extern CGroupLogic theGroupLogic;
extern CEventUpdater updater;

namespace
{
void AddRelatedUnit( CCommonUnit *pUnit, std::vector<int> *pIDs )
{
	if ( pUnit && pUnit->IsAlive() )
	{
		pIDs->push_back( pUnit->GetUniqueID() );
	}
}

bool InspectTransferUnit( int nID, int nSourcePlayer, std::vector<int> *pRelated )
{
	CCommonUnit *pUnit = dynamic_cast<CCommonUnit *>( CLinkObject::GetObjectByUniqueIdSafe( nID ) );
	if ( !pUnit || !pUnit->IsAlive() || pUnit->GetPlayer() != nSourcePlayer )
	{
		return false;
	}
	if ( CFormation *pFormation = dynamic_cast<CFormation *>( pUnit ) )
	{
		for ( int i = 0; i < pFormation->Size(); ++i )
		{
			AddRelatedUnit( ( *pFormation )[i], pRelated );
		}
		for ( int i = 0; i < pFormation->VirtualUnitsSize(); ++i )
		{
			AddRelatedUnit( pFormation->GetVirtualUnit( i ), pRelated );
		}
	}
	if ( CAIUnit *pAIUnit = dynamic_cast<CAIUnit *>( pUnit ) )
	{
		AddRelatedUnit( pAIUnit->GetFormation(), pRelated );
		AddRelatedUnit( pAIUnit->GetArtilleryIfCrew(), pRelated );
		AddRelatedUnit( pAIUnit->GetContainingUnit(), pRelated );
	}
	AddRelatedUnit( pUnit->GetTruck(), pRelated );
	if ( CSoldier *pSoldier = dynamic_cast<CSoldier *>( pUnit ) )
	{
		AddRelatedUnit( pSoldier->GetVirtualFormation(), pRelated );
	}
	if ( CMilitaryCar *pCar = dynamic_cast<CMilitaryCar *>( pUnit ) )
	{
		for ( int i = 0; i < pCar->GetNPassengers(); ++i )
		{
			AddRelatedUnit( pCar->GetPassenger( i ), pRelated );
		}
		for ( int i = 0; i < pCar->GetNBoarded(); ++i )
		{
			AddRelatedUnit( pCar->GetBoarded( i ), pRelated );
		}
		AddRelatedUnit( pCar->GetTowedArtillery(), pRelated );
		AddRelatedUnit( pCar->GetTowedArtilleryCrew(), pRelated );
	}
	if ( CArtillery *pArtillery = dynamic_cast<CArtillery *>( pUnit ) )
	{
		AddRelatedUnit( pArtillery->GetCrew(), pRelated );
	}
	return true;
}

bool IsTransferSelectionRestricted( const CCommonUnit *pUnit )
{
	if ( !pUnit->IsAlive() || pUnit->GetTruck() )
	{
		return true;
	}
	const CFormation *pFormation = dynamic_cast<const CFormation *>( pUnit );
	if ( pFormation && pFormation->IsInTransport() )
	{
		return true;
	}
	if ( IUnitState *pState = pUnit->GetState() )
	{
		const EUnitStateNames eState = pState->GetName();
		if ( eState == EUSN_PARTROOP || eState == EUSN_BEING_TOWED ||
		     ( eState == EUSN_GUN_CREW_STATE && !( pFormation && pFormation->HasMortar() ) ) )
		{
			return true;
		}
	}
	if ( const CAIUnit *pAIUnit = dynamic_cast<const CAIUnit *>( pUnit ) )
	{
		if ( pAIUnit->GetContainingUnit() )
		{
			return true;
		}
		// A formation's state can prohibit selecting its individual soldiers, too.
		if ( const CFormation *pOwnerFormation = pAIUnit->GetFormation() )
		{
			if ( IsTransferSelectionRestricted( pOwnerFormation ) )
			{
				return true;
			}
		}
		const SUnitBaseRPGStats *pStats = pAIUnit->GetStats();
		if ( pStats && pStats->etype == RPG_TYPE_AVIA_BOMBER )
		{
			return true;
		}
	}
	if ( const CSoldier *pSoldier = dynamic_cast<const CSoldier *>( pUnit ) )
	{
		return pSoldier->IsInBuilding() || pSoldier->IsInTransport();
	}
	if ( const CArtillery *pArtillery = dynamic_cast<const CArtillery *>( pUnit ) )
	{
		return pArtillery->IsBeingHooked();
	}
	return false;
}
} // namespace

void RestoreTransferredUnitSelection( CCommonUnit *pUnit )
{
	// Never copy IsSelectable() from the donor: that flag differs between clients.
	// Only suppress selection here; ChangePlayer already applied the recipient's local ownership check.
	if ( pUnit && IsTransferSelectionRestricted( pUnit ) )
	{
		pUnit->SetSelectable( false, true );
	}
}

void CAILogic::TransferUnits( const std::vector<int> &unitIDs, int nSourcePlayer, int nTargetPlayer )
{
	IAIScenarioTracker *pTracker = GetScenarioTracker();
	if ( !CanTransferUnitsToPlayer( pTracker, nSourcePlayer, nTargetPlayer ) )
	{
		return;
	}
	const std::vector<int> transfers =
			CollectUnitTransfer( unitIDs,
	                         [nSourcePlayer]( int nID, std::vector<int> *pRelated )
	                         { return InspectTransferUnit( nID, nSourcePlayer, pRelated ); } );
	// Snapshot every soldier before ChangePlayer recursively changes formation and artillery crew owners.
	for ( int nID : transfers )
	{
		CCommonUnit *pCommonUnit = dynamic_cast<CCommonUnit *>( CLinkObject::GetObjectByUniqueIdSafe( nID ) );
		theGroupLogic.DetachTransferredUnit( pCommonUnit );
		if ( CAIUnit *pUnit = dynamic_cast<CAIUnit *>( pCommonUnit ) )
		{
			pUnit->PrepareTransfer( nTargetPlayer );
		}
	}
	for ( int nID : transfers )
	{
		CCommonUnit *pUnit = dynamic_cast<CCommonUnit *>( CLinkObject::GetObjectByUniqueIdSafe( nID ) );
		if ( pUnit && pUnit->GetPlayer() == nSourcePlayer )
		{
			pUnit->ChangePlayer( nTargetPlayer );
		}
	}
	// ChangePlayer makes every owned unit selectable, including attached crew and passengers.
	// Run after the entire transfer so a later formation/crew ChangePlayer cannot undo these restrictions.
	for ( int nID : transfers )
	{
		RestoreTransferredUnitSelection( dynamic_cast<CCommonUnit *>( CLinkObject::GetObjectByUniqueIdSafe( nID ) ) );
	}
	if ( !transfers.empty() )
	{
		// Detached groups cannot be reused even if a shortcut selects exactly the same unit IDs again.
		SetNeedNewGroupNumber();
		// Every simulation emits the same feedback. Only the recipient's presentation layer displays it.
		CPtr<SFeedBackUnitsArray> pNotice = new SFeedBackUnitsArray;
		pNotice->unitIDs = transfers;
		pNotice->nParam = nTargetPlayer;
		CCommonUnit *pFirstUnit = dynamic_cast<CCommonUnit *>( CLinkObject::GetObjectByUniqueIdSafe( transfers.front() ) );
		pNotice->vCenterCamera = pFirstUnit->GetCenterPlain();
		updater.AddUpdate( EFB_UNITS_GIVEN, -1, pNotice );
	}
}
