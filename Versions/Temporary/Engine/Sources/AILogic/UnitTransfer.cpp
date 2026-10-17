#include "stdafx.h"

#include "AILogicInternal.h"
#include "Artillery.h"
#include "Formation.h"
#include "GroupLogic.h"
#include "NewUpdater.h"
#include "ScenarioTracker.h"
#include "Soldier.h"
#include "Technics.h"
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
} // namespace

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
	if ( !transfers.empty() )
	{
		// Every simulation emits the same feedback. Only the recipient's presentation layer displays it.
		CPtr<SFeedBackUnitsArray> pNotice = new SFeedBackUnitsArray;
		pNotice->unitIDs = transfers;
		pNotice->nParam = nTargetPlayer;
		CCommonUnit *pFirstUnit = dynamic_cast<CCommonUnit *>( CLinkObject::GetObjectByUniqueIdSafe( transfers.front() ) );
		pNotice->vCenterCamera = pFirstUnit->GetCenterPlain();
		updater.AddUpdate( EFB_UNITS_GIVEN, -1, pNotice );
	}
}
