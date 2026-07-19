#include "stdafx.h"

#include "Selector.h"
#include "GameXClassIDs.h"

#include "B2_M1_World/ClientAckManager.h"
#include "CommandsSender.h"
#include "AILogic/B2AI.h"
#include "Input/Bind.h"
#include "Stats_B2_M1/ActionsRemap.h"
#include "Stats_B2_M1/AIUnitCmd.h"
#include "Stats_B2_M1/AbilityActions.h"
#include "B2_M1_World/MOBuilding.h"
#include "B2_M1_World/MOUnitInfantry.h"
#include "SceneB2/StatSystem.h"
//#include "..\System\GlobalVars.h"
#include "System/Commands.h"

#include <algorithm>

static bool s_bShowAllObjectsInfo = false;
static bool s_bShowBuildingsInfo = false;

const NDb::EUserAction uaCommomActions[] = { NDb::USER_ACTION_ATTACK, NDb::USER_ACTION_MOVE, NDb::USER_ACTION_STOP, NDb::USER_ACTION_UNKNOWN };

const NDb::EUserAction uaMultiClassActionsList[] = { NDb::USER_ACTION_ATTACK, NDb::USER_ACTION_BOARD, NDb::USER_ACTION_MECH_BOARD, NDb::USER_ACTION_UNKNOWN };

enum EObjectClass
{
	EOC_GROUND,
	EOC_AIR,
	EOC_WATER,
};

static EObjectClass GetObjectClass( CMapObj *pMO )
{
	if ( CDynamicCast<const NDb::SUnitBaseRPGStats> pStats = pMO->GetStats() )
	{
		if ( pStats->IsAviation() )
			return EOC_AIR;
		if ( pStats->eSelectionType == NDb::SELECTION_TYPE_WATER )
			return EOC_WATER;
	}
	return EOC_GROUND;
}

static bool IsAquatic( CMapObj *pMO )
{
	if ( CDynamicCast<const NDb::SUnitBaseRPGStats> pStats = pMO->GetStats() )
		return pStats->eSelectionType == NDb::SELECTION_TYPE_WATER;
	return false;
}

// CSelector

CUserActions CSelector::MakeCommonActions()
{
	CUserActions actions;
	for ( int i = 0; uaCommomActions[i] != NDb::USER_ACTION_UNKNOWN; ++i )
	{
		actions.SetAction( uaCommomActions[i] );
	}
	return actions;
}

CUserActions CSelector::MakeMultiClassActions()
{
	CUserActions actions;
	for ( int i = 0; uaMultiClassActionsList[i] != NDb::USER_ACTION_UNKNOWN; ++i )
	{
		actions.SetAction( uaMultiClassActionsList[i] );
	}
	return actions;
}

bool CSelector::IsAllUnitsCommand( EActionCommand eCommand )
{
	const bool bAllUnitsCommand = ( 
		eCommand == ACTION_COMMAND_MOVE_TO ||
		eCommand == ACTION_COMMAND_FOLLOW ||
		eCommand == ACTION_COMMAND_SWARM_TO ||
		eCommand == ACTION_COMMAND_ART_BOMBARDMENT ||
		eCommand == ACTION_COMMAND_RANGE_AREA ||
		eCommand == ACTION_COMMAND_ATTACK_UNIT ||
		eCommand == ACTION_COMMAND_ATTACK_OBJECT ||
		eCommand == ACTION_COMMAND_ROTATE_TO ||
		eCommand == ACTION_COMMAND_MOVE_TO_GRID ||
		eCommand == ACTION_COMMAND_STAND_GROUND ||
		eCommand == ACTION_COMMAND_STOP ||
		eCommand == ACTION_COMMAND_ENTRENCH_SELF ||
		// eCommand == ACTION_COMMAND_AMBUSH ||

		eCommand == ACTION_COMMAND_UNLOAD ||
		eCommand == ACTION_COMMAND_LEAVE ||
		
		eCommand == ACTION_COMMAND_ENTER ||
		eCommand == ACTION_COMMAND_LOAD ||
		eCommand == ACTION_COMMAND_MECH_ENTER ||
		
		eCommand == ACTION_COMMAND_PARADE
	);
	return bAllUnitsCommand;
}

CSelector::CSelector()
: groups( 10 ), eSelectionState( ssNone ), nCurrentAbilityGroup( -1 ), bUnloadMode( false ),
	uaAllCommonActions( MakeCommonActions() ),
	uaMultiClassActions( MakeMultiClassActions() ),
	nMaxUnitSlots( 0 ), 
	nMaxUnitPerSlot( 0 ),
	bNeedUpdateSelectedUnits( false ),
	bNeedUpdatePreselectedObject( false ),
	bNeedUpdateAbilityIcons( false ),
	bNeedUpdateActionButtons( false ),
	bNeedUpdateSelectionInterior( false )
{ 
	for ( int i = 0; i < groups.size(); ++i ) 
		groups[i].SetID( i );
		
	bSupportSingleSelection = NGlobal::GetVar( "mission_single_selection", 0 ) != 0;
}

bool CSelector::Select( CMapObj *pMO, bool bSelect )
{
	return SelectPrivate( pMO, bSelect, false );
}

bool CSelector::DeSelectDead( CMapObj *pMO )
{

	return SelectPrivate( pMO, false, true );
}

bool CSelector::SelectPrivate( CMapObj *_pMO, bool bSelect, bool bDeath )
{
	CDynamicCast<CMOSelectable> pMO( _pMO );
	if ( !pMO )
		return false;
	
	if ( bSelect && !CanSelect( pMO ) )
		return false;

	if ( bSelect && !pMO->IsFriend() ) 
		return false;

	IMOSquad *pSquad = 0;
	if ( pMO->GetTypeID() == NDb::SInfantryRPGStats::typeID )
	{
		IMOUnit *pUnit = checked_cast<IMOUnit *>(pMO.GetPtr());
		if ( pUnit )
			pSquad = pUnit->GetSquad();
		if ( !bDeath && pSquad )
		{
			pMO = pSquad;
		}
	}

	if ( bSelect )
	{
		const ESelectionState eNewState = ( pMO->GetTypeID() == NDb::SBuildingRPGStats::typeID ) ? ssBuilding : ( pMO->IsFriend() ? ssUnits : ssEnemy );
		if ( bSelect && ( eNewState != ssUnits || eSelectionState != ssUnits ) )
			Empty();
		eSelectionState = eNewState;
	}

	if ( bSelect == IsSelected( pMO ) )
		return false;

	if ( bSelect ) 
	{
		if ( !CanAddObject( pMO ) )
			return false;
		AddObj( pMO );
		newObjList.push_back( pMO.GetPtr() );
		pMO->Select( true );

		return true;
	}
	else 
	{
		newObjList.remove( pMO.GetPtr() );
		for ( CMapObjectsVector::iterator it = objList.begin(); it != objList.end(); ++it)
		{
			if ( pMO == *it )
			{
				objList.erase( it );
				pMO->Select( false );

				return true;
			}
			if ( pSquad && pSquad == *it )
			{
				pMO->Select( false );
				if ( pSquad->GetPassangersCount() == 0 )
				{
					objList.erase( it );
					pSquad->Select( false );
				}

				return true;
			}
		}
		return false;
	}
}

void CSelector::AddObj( CMOSelectable *pSO )
{
	if ( NGlobal::GetVar( "History.Playing", 0 ) )
		return;

	objList.push_back( pSO );
	std::sort( objList.begin(), objList.end(), SObjectsSort() );
	CalcSlots();

	NGlobal::SetVar( "temp.TutorialOnly.SelectedUnits", static_cast<int>(slots.size()) );

#ifndef _FINALRELEASE
	const int nDisplay = NGlobal::GetVar( "display_selection_ids", 0 );
	if ( nDisplay )
	{
		std::string szResult = "Selction IDs: ";
		for ( int i = 0; i < objList.size(); ++i )
			szResult += StrFmt( "%i   ", objList[i]->GetID() );
		Singleton<IStatSystem>()->UpdateEntry( "SelectionIDs", szResult.c_str() );
	}

	if ( objList.size() == 1 )
		Singleton<IStatSystem>()->UpdateEntry( "SelectionDBID", pSO->GetStats() ? pSO->GetStats()->GetDBID().ToString().c_str() : "" );
#endif
}

void CSelector::CalcSlots()
{
	slots.clear();
	CalcSlots( &slots, objList );
}

void CSelector::CalcSlots( CSlotVector *pSlots, const CMapObjectsVector &objects ) const
{
	for ( CMapObjectsVector::const_iterator iObj = objects.begin(); iObj != objects.end(); ++iObj )
	{
		CMOSelectable *pSO = *iObj;
		CSlotVector::iterator it = find_if( pSlots->begin(), pSlots->end(), SSameSlotCompare( pSO ) );
		if ( it == pSlots->end() || objects.size() <= nMaxUnitSlots )
		{
			SSlot slot;
			slot.objects.push_back( pSO );
			pSlots->push_back( slot );
		}
		else
		{
			while ( true )
			{
				SSlot &slot = *it;
				if ( slot.objects.size() < nMaxUnitPerSlot )
				{
					slot.objects.push_back( pSO );
					break;
				}
				++it;
				it = find_if( it, pSlots->end(), SSameSlotCompare( pSO ) );
				if ( it == pSlots->end() )
				{
					SSlot slot;
					slot.objects.push_back( pSO );
					pSlots->push_back( slot );
					break;
				}
			}
		}
	}
}

bool CSelector::CanAddObject( CMOSelectable *pSO ) const
{
	if ( slots.size() < nMaxUnitSlots )
		return true;

	CMapObjectsVector testObjects = objList;
	testObjects.push_back( pSO );

	CSlotVector testSlots;

	CalcSlots( &testSlots, testObjects );

	return testSlots.size() <= nMaxUnitSlots;
}

void CSelector::SendUpdateSelection()
{
	if ( !bSupportSingleSelection )
	{
		NInput::PostEvent( "mission_multi_select_mode", 0, 0 );
	}
	else
	{
		if ( objList.size() == 1 )
		{
			NInput::PostEvent( "mission_single_select_mode", 0, 0 );
			CMapObj *pMO = objList.front();
			NInput::PostEvent( "mission_update_single_unit", pMO->GetID(), 0 );
		}
		else
		{
			NInput::PostEvent( "mission_multi_select_mode", 0, 0 );
		}
	}
}

void CSelector::SendUpdateSelectionInterior()
{
	bNeedUpdateSelectionInterior = true;
}

void CSelector::Empty()
{
	newObjList.clear();
	for ( CMapObjectsVector::iterator it = objList.begin(); it != objList.end(); ++it)
		(*it)->Select( false );

	objList.clear();
	slots.clear();
	UpdateSelection( false, false );
}

void CSelector::DoneSelection( const bool bPreserveGroup, bool bKeepNumbers )
{
	NInput::PostEvent( "close_reinforcement", 0, 0 );
	NInput::PostEvent( "mission_close_reinforcement", 0, 0 );

	NInput::PostEvent( "new_reset_forced_action", 0, 0 );
	
	UpdateSelection( bPreserveGroup, bKeepNumbers );
}

void CSelector::UpdateSelection( const bool bPreserveGroup, bool bKeepNumbers )
{
	if ( !bKeepNumbers )
	{
		for ( CMapObjectsVector::iterator it = objList.begin(); it != objList.end(); ++it)
		{
			CMOSelectable *pMO = *it;
			pMO->SetSelectionGroup( -1 );
		}
  }
	
	if ( !newObjList.empty() ) 
	{
		for ( CMapObjectsList::iterator it = newObjList.begin(); it != newObjList.end(); ++it ) 
		{
			// do smth for every new object in list
		}
		// selection acknowledgement
		CMapObj *pMO = *newObjList.begin();
		if ( pMO->GetTypeID() == NDb::SMechUnitRPGStats::typeID || pMO->GetTypeID() == NDb::SInfantryRPGStats::typeID )
		{
			IMOUnit *pMOUnit = checked_cast<IMOUnit*>( pMO );
			if ( pMOUnit )
				pMOUnit->SendAcknowledgement( AckManager(), NDb::ACK_SELECTED );
		}
		else if ( pMO->GetTypeID() == NDb::SSquadRPGStats::typeID )
		{
			IMOSquad *pMOSquad = checked_cast<IMOSquad*>( pMO );
			if ( pMOSquad )
				pMOSquad->SendAcknowledgement( AckManager(), NDb::ACK_SELECTED );
		}
		newObjList.clear();
	}
	sort( objList.begin(), objList.end(), SObjectsSort() );
	CalcSlots();
	vAbilityGroups.clear();

 	if ( !pPreselectedObject || (objList.size() == 1 && objList.front() == pPreselectedObject) )
		SendUpdateSelection();

	if ( objList.empty() )
	{
		nCurrentAbilityGroup = 0;
		superActives.clear();
		uaCurrentAbility.Clear();
		bUnloadMode = false;
		SendUpdateSelectionInterior();
		SetSelectionGroup( 0 );
	}
	else
	{
		if ( objList.size() == 1 )
		{
			CMapObj *pMO = *(objList.begin());
			if ( pMO->GetTypeID() == NDb::SMechUnitRPGStats::typeID || pMO->GetTypeID() == NDb::SBuildingRPGStats::typeID )
			{
				IMOContainer *pContainer = checked_cast<IMOContainer *>(pMO);
				int nCount = pContainer->GetPassangersCount();
				if ( nCount != 0 )
				{
					bUnloadMode = true;
					vAbilityGroups.push_back( std::pair<int, int>( 0, 0 ) );
					SendUpdateSelectionInterior();
					SetSelectionGroup( 0 );
					return;
				}
			}
		}

		bUnloadMode = false;
		int nSame = CalcAbilityGroups();
			
		SendUpdateSelectionInterior();
		SetSelectionGroup( bPreserveGroup ? nSame : 0 );
	}
}

int CSelector::CalcAbilityGroups()
{
	int nIndex = 0, nFirst = 0, nSame = 0;
	CUserActions actionsFirst;
	for ( CSlotVector::iterator it = slots.begin(); it != slots.end(); ++it )
	{
		SSlot &slot = *it;
		CUserActions actions;
		GetPossibleActions( &actions, slot.objects );

		if ( nIndex == 0 )
			actionsFirst = actions;
		else if ( actionsFirst != actions )
		{
			vAbilityGroups.push_back( std::pair<int, int>( nFirst, nIndex-1 ) );
			if ( uaCurrentAbility == actionsFirst )
				nSame = vAbilityGroups.size()-1;
			actionsFirst = actions;
			nFirst = nIndex;
		}  
		nIndex++;
	}

	vAbilityGroups.push_back( std::pair<int, int>( nFirst, nIndex-1 ) );
	if ( uaCurrentAbility == actionsFirst )
		nSame = vAbilityGroups.size()-1;
	return nSame;
}

void CSelector::UpdateAbilityIcons( const std::vector< CPtr<CMOSelectable> > &objects ) const
{
	CAbilityInfo allAbilities;
	for ( std::vector< CPtr<CMOSelectable> >::const_iterator it = objects.begin(); it != objects.end(); ++it )
	{
		CMOSelectable *pSO = *it;

		CAbilityInfo abilities;
		pSO->GetAbilityInfo( abilities );
		CombineAbilities( &allAbilities, abilities );
	}
	for ( CAbilityInfo::const_iterator it = allAbilities.begin(); it != allAbilities.end(); ++it )
	{
		NDb::EUnitSpecialAbility eAbility = it->first;
		const SAbilityInfo &ability = it->second;
		NInput::PostEvent( "set_ability_state", eAbility, ability.abilityState.dwStateValue );
		NInput::PostEvent( "set_ability_param", eAbility, ability.fParam * 1024 );
	}

	// update ability button: ABILITY_RADIO_CONTROLLED_MODE
	CAbilityInfo::const_iterator itRadioControlledMode = allAbilities.find( NDb::ABILITY_RADIO_CONTROLLED_MODE );
	CAbilityInfo::const_iterator itPlaceControlled = allAbilities.find( NDb::ABILITY_PLACE_CONTROLLED_CHARGE );
	CAbilityInfo::const_iterator itDetonate = allAbilities.find( NDb::ABILITY_DETONATE );
	if ( itRadioControlledMode != allAbilities.end() &&
		itPlaceControlled != allAbilities.end() &&
		itDetonate != allAbilities.end() )
	{
		const SAbilityInfo &placeControlled = itPlaceControlled->second;
		const SAbilityInfo &detonate = itDetonate->second;

		SAbilitySwitchState	abilityState;
		abilityState.dwStateValue = placeControlled.abilityState.dwStateValue;
		if ( abilityState.eState == EASS_DISABLE )
			abilityState.dwStateValue = detonate.abilityState.dwStateValue;
		NInput::PostEvent( "set_ability_state", NDb::ABILITY_RADIO_CONTROLLED_MODE, abilityState.dwStateValue );
		NInput::PostEvent( "set_ability_param", NDb::ABILITY_RADIO_CONTROLLED_MODE, 0 );
	}
}

void CSelector::SetPreselection( CMapObj *_pMO )
{
	CMapObj *pMO = _pMO;

	// ignore uninteresting objects
	if ( CDynamicCast<IMOUnit>( pMO ) )
	{
	}
	else if ( CDynamicCast<CMOBuilding> pBuilding = pMO )
	{
		if ( !s_bShowAllObjectsInfo && !s_bShowBuildingsInfo )
		{
			if ( pBuilding->GetPassangersCount() == 0 && !pBuilding->IsKeyObject() )
				pMO = 0;
		}		
	}
	else
	{
		if ( !s_bShowAllObjectsInfo )
			pMO = 0;
	}

	if ( pPreselectedObject == pMO )
		return;

	// show squad or served gun for infantry
	if ( pMO && pMO->GetTypeID() == NDb::SInfantryRPGStats::typeID )
	{
		if ( CDynamicCast<IMOUnit> pUnit = pMO )
		{
			if ( IMOSquad *pSquad = pUnit->GetSquad() )
			{
				pMO = pSquad;
				if ( IMOUnit *pGun = pSquad->GetServedGun() )
				{
					if ( pGun->IsVisible() )
						pMO = pGun;
				}
			}
		}
	}

	if ( pMO )
	{
		NInput::PostEvent( "mission_preselect_mode", 0, 0 );
		NInput::PostEvent( "mission_update_single_unit", pMO->GetID(), 0 );
		NInput::PostEvent( "mission_update_unit_stats", pMO->GetID(), 0 );

		if ( pMO->IsKeyObject() )
		{
			Scene()->AddKeyPointArea( CVec2(pMO->GetCenter().x, pMO->GetCenter().y), 
				NGlobal::GetVar("AI.TransportAndResupply.TakeStorageOwnershipRadius", 100), CVec3(0.5, 0.5, 0.5) );
		}
	}
	else
	{
		SendUpdateSelection();
		SendUpdateSelectionInterior();

		Scene()->ClearKeyPointAreas();
	}

	pPreselectedObject = pMO;
}

void CSelector::UpdateUnitsIcons( bool bPreselection )
{
	NInput::PostEvent( "update_icon", -1, -1 );
	
	if ( objList.empty() )
	{
		bUnloadMode = false;
		return;
	}

	bool bShowInterrior = false;
	CMapObj *pMO = objList.front();
	
	if ( objList.size() == 1 )
	{
		IMOContainer *pContainer = dynamic_cast<IMOContainer*>( pMO );
		if ( pContainer )
			bShowInterrior = pContainer->NeedShowInterrior();
	}
	
	if ( bShowInterrior )
	{
		std::vector<CMOSelectable*> units;
		IMOContainer *pContainer = checked_cast<IMOContainer *>(pMO);
		pContainer->GetPassangers( &units );
		if ( !units.empty() )
		{
			for ( std::vector<CMOSelectable*>::iterator it = units.begin(); it != units.end(); ++it )
				NInput::PostEvent( "update_icon", (*it)->GetID(), 1 );
			bUnloadMode = true;
			NInput::PostEvent( "mission_update_unit_stats", units.front()->GetID(), 0 );
			return;
		}
	}

	bUnloadMode = false;
	for ( CSlotVector::const_iterator it = slots.begin(); it != slots.end(); ++it )
	{
		const SSlot &slot = *it;
		NInput::PostEvent( "update_icon", slot.objects[0]->GetID(), slot.objects.size() );
	}
	
	if ( !bPreselection )
		SetSelectionGroup( nCurrentAbilityGroup );
}

void CSelector::SendUpdateActionButtons()
{
	bNeedUpdateActionButtons = true;
}

void CSelector::UpdateSelectedUnits()
{
	SendUpdateSelectionInterior();
	SendUpdateActionButtons();
}

void CSelector::DoUpdateSelectedUnits()
{
	bNeedUpdateSelectedUnits = true;
}

void CSelector::DoUpdatePreselectedUnits()
{
	bNeedUpdatePreselectedObject = true;
}

void CSelector::DoUpdateSpecialAbility( CMapObj *pMO )
{
	if ( !superActives.empty() )
	{
		bNeedUpdateAbilityIcons = true;
	}
}

void CSelector::DoUpdateStats( CMapObj *pMO )
{
	if ( CMOSelectable *pSelectable = dynamic_cast<CMOSelectable*>( pMO ) )
	{
		if ( pSelectable->IsSelected() )
			bNeedUpdateSelectedUnits = true;
	}

	if ( pPreselectedObject && pPreselectedObject == pMO )
		bNeedUpdatePreselectedObject = true;
}

void CSelector::Segment()
{
	if ( bNeedUpdateActionButtons )
	{
		bNeedUpdateActionButtons = false;
		NInput::PostEvent( "update_buttons", 0, 0 );
		bNeedUpdateAbilityIcons = true;
	}
	if ( bNeedUpdateAbilityIcons )
	{
		bNeedUpdateAbilityIcons = false;
		UpdateAbilityIcons( superActives );
	}

	if ( bNeedUpdateSelectedUnits )
	{
		bNeedUpdateSelectedUnits = false;
		UpdateSelectedUnits();
	}
	if ( bNeedUpdatePreselectedObject )
	{
		bNeedUpdatePreselectedObject = false;
		NInput::PostEvent( "mission_update_unit_stats", pPreselectedObject->GetID(), 0 );
	}
	if ( bNeedUpdateSelectionInterior )
	{
		bNeedUpdateSelectionInterior = false;
		if ( !bSupportSingleSelection )
		{
			UpdateUnitsIcons( false );
		}
		else
		{
			if ( objList.size() == 1 )
			{
				NInput::PostEvent( "mission_update_unit_stats", objList.front()->GetID(), 0 );
			}
			else
				UpdateUnitsIcons( false );
		}
	}
}

void CSelector::AssignSelectionToGroup( int nIndex )
{
	while ( !groups[nIndex].GetList().empty() )
	{
		RemoveObjectFromGroup( groups[nIndex].GetList().front(), nIndex );
	}
	groups[nIndex].Clear();
	for ( CMapObjectsVector::iterator it = objList.begin(); it != objList.end(); ++it) 
	{
		CMOSelectable *pMO = *it;
		AddObjectToGroup( pMO, nIndex );
		pMO->SetSelectionGroup( nIndex );
	}
}

void CSelector::AssignGroupToSelection( int nIndex, bool bAddToCurrent )
{
	if ( groups[nIndex].GetList().empty() )
		return; // выбор группы без юнитов не должен делать ничего, просто игнорируется
		
	if ( !bAddToCurrent ) 
	{
		Empty();
	}
	for ( CMapObjectsList::const_iterator it = groups[nIndex].GetList().begin(); it != groups[nIndex].GetList().end(); ++it )
		Select( *it, true );
	DoneSelection( false, false );
	for ( CMapObjectsList::const_iterator it = groups[nIndex].GetList().begin(); it != groups[nIndex].GetList().end(); ++it )
	{
		CMOSelectable *pMO = *it;
		pMO->SetSelectionGroup( nIndex );
	}
}

void CSelector::RemoveFromGroups( CMOSelectable *pMO )
{
	for ( int i = 0; i < groups.size(); ++i )
	{
		groups[i].Remove( pMO );
	}
	pMO->SetSelectionGroup( -1 );
}

void CSelector::HideFromGroups( CMOSelectable *_pMO )
{
	CMOSelectable *pMO = _pMO;
	if ( CDynamicCast<IMOUnit> pUnit = pMO )
	{
		if ( IMOSquad *pSquad = pUnit->GetSquad() )
			pMO = pSquad;
	}
	for ( int i = 0; i < groups.size(); ++i )
	{
		groups[i].Hide( pMO );
	}
	pMO->SetSelectionGroup( -1 );
}

void CSelector::UnHideForGroups( CMOSelectable *_pMO )
{
	CMOSelectable *pMO = _pMO;
	if ( CDynamicCast<IMOUnit> pUnit = pMO )
	{
		if ( IMOSquad *pSquad = pUnit->GetSquad() )
			pMO = pSquad;
	}
	for ( int i = 0; i < groups.size(); ++i )
	{
		groups[i].UnHide( pMO );
	}
}

void CSelector::ReplaceSelectionGroup( CMOSelectable *pMOPattern, CMOSelectable *pMO )
{
	int nIndex = FindSelectionGroup( pMOPattern );
	int nIndexCurrent = pMOPattern->GetSelectionGroup();

	if ( nIndex >= 0 )
	{
		RemoveObjectFromGroup( pMOPattern, nIndex );
		RemoveObjectFromGroup( pMO, nIndex );
		AddObjectToGroup( pMO, nIndex );
	}
	
	if ( pMOPattern->IsSelected() )
	{
		Select( pMOPattern, false );
		if ( CanSelect( pMO ) )
			Select( pMO, true );
	}
	else
	{
		Select( pMO, false );
	}

	pMO->SetSelectionGroup( nIndexCurrent );

	UpdateSelection( true, true );
}

int CSelector::FindSelectionGroup( CMOSelectable *pMO ) const
{
	int nIndex = -1;
	for ( int i = 0; i < groups.size(); ++i )
	{
		const SSelectionGroup &group = groups[i];
		if ( group.HasObject( pMO ) )
		{
			nIndex = i;
			break;
		}
	}
	return nIndex;
}

void CSelector::GetGroupMembers( int nIndex, std::vector<CMOSelectable*> *pMembers )
{
	pMembers->clear();
	for ( CMapObjectsList::const_iterator it = groups[nIndex].GetList().begin(); 
		it != groups[nIndex].GetList().end(); ++it )
	{
		CMOSelectable *pSO = *it;
		if ( CDynamicCast<IMOSquad> pSquad = pSO )
		{
			std::vector<CMOSelectable*> passangers;
			pSquad->GetPassangers( &passangers );
			for ( std::vector<CMOSelectable*>::iterator iPass = passangers.begin(); iPass != passangers.end(); ++iPass )
			{
				CMOSelectable *pPassanger = *iPass;
				pMembers->push_back( pPassanger );
			}
		}
		else
		{
			pMembers->push_back( pSO );
		}
	}
}

bool CSelector::DoGroupCommand( CCommandsSender *pCommandsSender, 
															 const struct SAIUnitCmd *pCommand, bool bPlaceInQueue )
{
	if ( IsEmpty() || eSelectionState == ssEnemy )
		return false;

	const bool bAllUnitsCommand = IsAllUnitsCommand( pCommand->nCmdType );

	std::vector<int> buffer;
	buffer.reserve( nMaxUnitSlots * nMaxUnitPerSlot );
	const int nBegin = bAllUnitsCommand ? 0 : vAbilityGroups[nCurrentAbilityGroup].first;
	const int nEnd = bAllUnitsCommand ? slots.size()-1 : vAbilityGroups[nCurrentAbilityGroup].second;

	for ( int i = nBegin; i <= nEnd; ++i )
	{
		SSlot &slot = slots[i];
		for ( std::vector< CPtr<CMOSelectable> >::iterator it = slot.objects.begin(); it != slot.objects.end(); ++it )
		{
			CMOSelectable *pSO = *it;
			
			buffer.push_back( pSO->GetID() );
		}
	}

	const WORD wAIGroup = pCommandsSender->CommandRegisterGroup( buffer );
	pCommandsSender->CommandGroupCommand( pCommand, wAIGroup, bPlaceInQueue, ML_COMMAND_SAVE_GAME );
	pCommandsSender->CommandUnregisterGroup( wAIGroup );
	return true;
}

bool CSelector::DoGroupCommandAutocast( class CCommandsSender *pCommandsSender, const struct SAIUnitCmd *pCommand, bool bPlaceInQueue )
{
	if ( IsEmpty() || eSelectionState == ssEnemy )
		return false;

	const bool bAllUnitsCommand = IsAllUnitsCommand( pCommand->nCmdType );

	std::vector<int> buffer;
	buffer.reserve( nMaxUnitSlots * nMaxUnitPerSlot );
	const int nBegin = bAllUnitsCommand ? 0 : vAbilityGroups[nCurrentAbilityGroup].first;
	const int nEnd = bAllUnitsCommand ? slots.size()-1 : vAbilityGroups[nCurrentAbilityGroup].second;

	for ( int i = nBegin; i <= nEnd; ++i )
	{
		SSlot &slot = slots[i];
		for ( std::vector< CPtr<CMOSelectable> >::iterator it = slot.objects.begin(); it != slot.objects.end(); ++it )
		{
			CMOSelectable *pMO = *it;

			buffer.push_back( pMO->GetID() );
			CObj<SAISpecialAbilityUpdate> pUpdate = new SAISpecialAbilityUpdate();
			pUpdate->info.nAbilityType = GetAbilityByCommand( pCommand->nCmdType );
			pUpdate->info.fCurValue = pCommand->fNumber;
			pUpdate->info.state.dwStateValue = pCommand->nNumber;
			if ( pMO->AIUpdateSpecialAbility( *pUpdate ) )
			{
				DoUpdateSpecialAbility( pMO );
			}
		}
	}

	const WORD wAIGroup = pCommandsSender->CommandRegisterGroup( buffer );
	pCommandsSender->CommandGroupCommand( pCommand, wAIGroup, bPlaceInQueue, ML_COMMAND_SAVE_GAME );
	pCommandsSender->CommandUnregisterGroup( wAIGroup );
	return true;
}

void CSelector::SetShowAreas( EActionNotify eType, bool bOn )
{
	std::vector<int> buffer;
	buffer.reserve( nMaxUnitSlots * nMaxUnitPerSlot );
	if ( !bOn )
	{
		Singleton<IAILogic>()->ShowAreas( buffer, eType, false );		//The group is not needed anyway
		return;
	}

	for ( int i = 0; i < objList.size(); ++i )
	{
		CMOSelectable *pSO = objList[i];
		if ( CDynamicCast<CMOBuilding> pMOBuilding = pSO )
		{
			std::vector<CMOSelectable*> passengers;
			pMOBuilding->GetPassangers( &passengers );
			for ( int j = 0; j < passengers.size(); ++j )
			{
				CMOSelectable *pMOPassenger = passengers[j];
				buffer.push_back( pMOPassenger->GetID() );
			}
		}
		else
			buffer.push_back( pSO->GetID() );
	}

	//const WORD wAIGroup = pCommandsSender->CommandRegisterGroup( buffer );
	Singleton<IAILogic>()->ShowAreas( buffer, eType, bOn );
}

void CSelector::GetAreas( SShootAreas *pAreas )
{
	pAreas->areas.clear();
	for ( int i = 0; i < objList.size(); ++i )
	{
		Singleton<IAILogic>()->GetShootAreas( objList[i]->GetID(), pAreas );
	}
}

int CSelector::GetSelection( std::vector<CMOSelectable*> *pBuffer ) const
{
	int nResult = 0;
	for ( CMapObjectsVector::const_iterator it = objList.begin(); it != objList.end(); ++it )
	{
		if ( pBuffer )
			pBuffer->push_back( *it );
		nResult++;
	}
	return nResult;
}

void CSelector::GetSelectionMembers( std::vector<CMOSelectable*> *pBuffer ) const
{
	if ( !pBuffer )
		return;
	pBuffer->clear();
	for ( CMapObjectsVector::const_iterator it = objList.begin(); it != objList.end(); ++it )
	{
		CMOSelectable *pSO = *it;
		if ( CDynamicCast<IMOSquad> pSquad = pSO )
		{
			std::vector<CMOSelectable*> passangers;
			pSquad->GetPassangers( &passangers );
			for ( std::vector<CMOSelectable*>::iterator iPass = passangers.begin(); iPass != passangers.end(); ++iPass )
			{
				CMOSelectable *pPassanger = *iPass;
				pBuffer->push_back( pPassanger );
			}
		}
		else
		{
			pBuffer->push_back( pSO );
		}
	}
}

void CSelector::GetActionsPrivate( CUserActions *pActions, bool bEnabledOnly )
{
	CUserActions actionsActive;
	if ( !superActives.empty() )
	{
		if ( bEnabledOnly )
			GetEnabledActions( &actionsActive, superActives, ACTIONS_BY );
		else
			GetActions( &actionsActive, superActives, ACTIONS_BY );
	}

	CUserActions actionsCommon;
	for ( CMapObjectsVector::iterator it = objList.begin(); it != objList.end(); ++it )
	{
		CMOSelectable *pMO = *it;
		
		CUserActions actions;
		if ( bEnabledOnly )
			pMO->GetEnabledActions( &actions, ACTIONS_BY );
		else
			pMO->GetActions( &actions, ACTIONS_BY );
		actionsCommon |= actions;
	}
	actionsCommon &= uaAllCommonActions;
	*pActions |= actionsCommon;
	*pActions |= actionsActive;
}

void CSelector::GetActions( CUserActions *pActions )
{
	GetActionsPrivate( pActions, false );
}

void CSelector::GetDisabledActions( CUserActions *pActions )
{
	CUserActions actionsActive;
	if ( !superActives.empty() )
	{
		GetDisabledActions( &actionsActive, superActives, ACTIONS_BY );
	}

	CUserActions actionsCommon;
	for ( CMapObjectsVector::iterator it = objList.begin(); it != objList.end(); ++it )
	{
		CMOSelectable *pMO = *it;
		
		CUserActions actions;
		pMO->GetDisabledActions( &actions, ACTIONS_BY );
		actionsCommon |= actions;
	}
	actionsCommon &= uaAllCommonActions;
	*pActions |= actionsCommon;
	*pActions |= actionsActive;
}

void CSelector::GetEnabledActions( CUserActions *pActions )
{
	GetActionsPrivate( pActions, true );
}

void CSelector::GetEnabledSuperActiveActions( CUserActions *pActions )
{
	GetEnabledActions( pActions, superActives, ACTIONS_BY );
}

void CSelector::AddObjectToGroup( CMOSelectable *pMO, int nGroup )
{
	groups[nGroup].Add( pMO );
//	pMO->SetSelectionGroup( nGroup );
}

void CSelector::RemoveObjectFromGroup( CMOSelectable *pMO, int nGroup )
{
	groups[nGroup].Remove( pMO );
//	pMO->SetSelectionGroup( FindObjectGroup( pMO ) );
}

int CSelector::FindObjectGroup( CMOSelectable *pMO ) const
{
	for ( int i = 0; i < groups.size(); ++i )
	{
		CMapObjectsList::const_iterator it = find( groups[i].GetList().begin(),
			groups[i].GetList().end(), pMO );
		if ( it != groups[i].GetList().end() )
			return i;
	}
	return -1;
}

void CSelector::GetActions( CUserActions *pActions, const std::vector< CPtr<CMOSelectable> > &objects, EActionsType eActions )
{
	for ( std::vector< CPtr<CMOSelectable> >::const_iterator it = objects.begin(); it != objects.end(); ++it )
	{
		CMOSelectable *pSO = *it;
		CUserActions actions;
		pSO->GetActions( &actions, eActions );
		if ( it == objects.begin() )
			*pActions = actions;
		else
			*pActions |= actions;
	}
}

void CSelector::GetPossibleActions( CUserActions *pActions, const std::vector< CPtr<CMOSelectable> > &objects )
{
	for ( std::vector< CPtr<CMOSelectable> >::const_iterator it = objects.begin(); it != objects.end(); ++it )
	{
		CMOSelectable *pSO = *it;
		CUserActions actions;
		pSO->GetPossibleActions( &actions );
		if ( it == objects.begin() )
			*pActions = actions;
		else
			*pActions |= actions;
	}
}

void CSelector::GetEnabledActions( CUserActions *pActions, const std::vector< CPtr<CMOSelectable> > &objects, EActionsType eActions )
{
	for ( std::vector< CPtr<CMOSelectable> >::const_iterator it = objects.begin(); it != objects.end(); ++it )
	{
		CMOSelectable *pSO = *it;
		CUserActions actions;
		pSO->GetEnabledActions( &actions, eActions );
		if ( it == objects.begin() )
			*pActions = actions;
		else
			*pActions |= actions;
	}
	CUserActions actions;
	GetActions( &actions, objects, eActions );
	*pActions &= actions;
}

void CSelector::GetDisabledActions( CUserActions *pActions, const std::vector< CPtr<CMOSelectable> > &objects, EActionsType eActions )
{
	for ( std::vector< CPtr<CMOSelectable> >::const_iterator it = objects.begin(); it != objects.end(); ++it )
	{
		CMOSelectable *pSO = *it;
		CUserActions actions;
		pSO->GetDisabledActions( &actions, eActions );
		*pActions |= actions;
	}
}

int CSelector::GetAbilityTier( const std::vector< CPtr<CMOSelectable> > &objects, NDb::EUserAction eAction )
{
	for ( std::vector< CPtr<CMOSelectable> >::const_iterator it = objects.begin(); it != objects.end(); ++it )
	{
		CMOSelectable *pSO = *it;
		int nTier = pSO->GetAbilityTier( eAction );
		if ( nTier != -1 )
			return nTier;
	}
	return -1;
}

const NDb::SUnitSpecialAblityDesc* CSelector::GetAbilityDesc( const std::vector< CPtr<CMOSelectable> > &objects, NDb::EUserAction eAction )
{
	// The action panel has one button per action, so use the first matching selected descriptor.
	for ( std::vector< CPtr<CMOSelectable> >::const_iterator it = objects.begin(); it != objects.end(); ++it )
	{
		CMOSelectable *pSO = *it;
		const NDb::SUnitSpecialAblityDesc *pDesc = pSO->GetAbilityDesc( eAction );
		if ( pDesc )
			return pDesc;
	}
	return 0;
}


void CSelector::SetSelectionGroup( const int nIndex )
{
	if ( 0 <= nIndex && nIndex < vAbilityGroups.size() )
	{
		if ( !bUnloadMode )
			NInput::PostEvent( "highlight_units", vAbilityGroups[nIndex].first, vAbilityGroups[nIndex].second );
		else
			NInput::PostEvent( "highlight_units", -1, -1 );
		nCurrentAbilityGroup = nIndex;
		superActives.clear();
		for ( int i = vAbilityGroups[nIndex].first; i <= vAbilityGroups[nIndex].second; ++i )
		{
			SSlot &slot = slots[i];
			superActives.insert( superActives.end(), slot.objects.begin(), slot.objects.end() );
		}
		uaCurrentAbility.Clear();
		GetPossibleActions( &uaCurrentAbility, superActives );
		SendUpdateActionButtons();
	}
	else
	{
		superActives.clear();
		SendUpdateActionButtons();
	}

	if ( !superActives.empty() )
		NInput::PostEvent( "mission_update_unit_stats", superActives.front()->GetID(), 0 );
}

void CSelector::SelectSameSlots( const int nSlot )
{
	int nAbilityGroup = -1;
	for ( int i = 0; i < vAbilityGroups.size(); ++i )
	{
		if ( vAbilityGroups[i].first <= nSlot && nSlot <= vAbilityGroups[i].second )
		{
			nAbilityGroup = i;
			break;
		}
	}
	if ( nAbilityGroup == -1 )
		return;

	if ( nAbilityGroup == nCurrentAbilityGroup && objList.size() != 1 && nSlot < slots.size() )
	{
		std::vector< CPtr<CMOSelectable> > tmpObjects;
		tmpObjects.resize( slots[nSlot].objects.size() );
		for ( int i = 0; i < slots[nSlot].objects.size(); ++i ) 
		{
			tmpObjects[i] = slots[nSlot].objects[i];
		}
		Empty();
		for ( int i = 0; i < tmpObjects.size(); ++i )
		{
			Select( tmpObjects[i], true );
		}
		DoneSelection( false, false );
	}
	else
	{
		SetSelectionGroup( nAbilityGroup );
	}
}

void CSelector::SelectSameType( int nSlot )
{
	if ( nSlot < 0 || nSlot >= slots.size() )
		return;

	CMOSelectable *pMO = slots[nSlot].objects.front();

	std::vector< CPtr<CMOSelectable> > tmpObjects;
	tmpObjects.reserve( nMaxUnitSlots * nMaxUnitPerSlot );
	for ( int i = 0; i < slots.size(); ++i )
	{
		SSlot &slot = slots[i];
		for ( int j = 0; j < slot.objects.size(); ++j )
		{
			CMOSelectable *pMO2 = slot.objects[j];
			if ( IsSameType( pMO, pMO2 ) )
				tmpObjects.push_back( pMO2 );
		}
	}

	Empty();
	for ( int i = 0; i < tmpObjects.size(); ++i )
	{
		Select( tmpObjects[i], true );
	}
	DoneSelection( false, false );
}

void CSelector::SelectNextGroup()
{
	if ( vAbilityGroups.empty() || vAbilityGroups.size() == 1 )
		return;
	
	const int nIndex = (nCurrentAbilityGroup ==  vAbilityGroups.size()-1) ? 0 : nCurrentAbilityGroup+1;
	SetSelectionGroup( nIndex );
}

void CSelector::SelectPrevGroup()
{
	if ( vAbilityGroups.empty() || vAbilityGroups.size() == 1 )
		return;
	
	const int nIndex = (nCurrentAbilityGroup > 0) ? nCurrentAbilityGroup - 1 : vAbilityGroups.size() - 1;
	SetSelectionGroup( nIndex );
}

void CSelector::UnselectSlot( int nSlot )
{
	if ( nSlot >= 0 && nSlot < slots.size() )
	{
		std::vector< CPtr<CMOSelectable> > objects = slots[nSlot].objects;
		for ( std::vector< CPtr<CMOSelectable> >::iterator it = objects.begin(); it != objects.end(); ++it )
		{
			CMOSelectable *pSO = *it;
			Select( pSO, false );
		}
		DoneSelection( true, false );
	}
}

void CSelector::SetMaxUnits( const int _nMaxUnitSlots, const int _nMaxUnitPerSlot )
{
	nMaxUnitSlots = _nMaxUnitSlots;
	nMaxUnitPerSlot = _nMaxUnitPerSlot;
	objList.reserve( nMaxUnitSlots * nMaxUnitPerSlot );
	vAbilityGroups.reserve( nMaxUnitSlots );
}

int CSelector::GetAbilityTier( NDb::EUserAction eAction ) const
{
	return GetAbilityTier( superActives, eAction );
}

const NDb::SUnitSpecialAblityDesc* CSelector::GetAbilityDesc( NDb::EUserAction eAction ) const
{
	return GetAbilityDesc( superActives, eAction );
}

bool CSelector::IsSameType( const CMapObj *_pMO, const CMapObj *_pMO2 )
{
	const CMapObj *pMO = _pMO;
	const CMapObj *pMO2 = _pMO2;
	if ( CDynamicCast<CMOUnitInfantry> pInfantry = pMO )
		pMO = pInfantry->GetSquad();
	if ( CDynamicCast<CMOUnitInfantry> pInfantry = pMO2 )
		pMO2 = pInfantry->GetSquad();
	return pMO->GetTypeID() == pMO2->GetTypeID() && 
		pMO->GetStats() == pMO2->GetStats();
}

bool CSelector::IsSelected( const CMapObj *pMO ) const
{
	CDynamicCast<CMOSelectable> pSO = pMO;
	if ( !pSO )
		return false;
	bool bSelected = pSO->IsSelected();
	NI_ASSERT( !bSelected || CanSelect( pMO ), "Unselectable object is selected" );
	return bSelected;
}

bool CSelector::IsSelectedOrInSelectedContainer( const CMapObj *pMO ) const
{
	CDynamicCast<CMOSelectable> pSO = pMO;
	if ( !pSO )
		return false;
	bool bSelected = pSO->IsSelected();
	NI_ASSERT( !bSelected || CanSelect( pMO ), "Unselectable object is selected" );
	if ( bSelected )
		return true;
	if ( IMOContainer *pContainer = pSO->GetTopContainer() )
		bSelected = pContainer->IsSelected();
	return bSelected;
}

bool CSelector::IsPreselectedOrInPreselectedContainer( const CMapObj *pMO ) const
{
	if ( !pMO )
		return false;
	if ( pMO == pPreselectedObject )
		return true;
	CDynamicCast<CMOSelectable> pSO = pMO;
	if ( !pSO )
		return false;
	if ( IMOContainer *pContainer = pSO->GetTopContainer() )
	{
		if ( pContainer == pPreselectedObject )
			return true;
	}
	return false;
}

bool CSelector::IsActive( const CMOSelectable *pMO ) const
{
	if ( !IsSelected( pMO ) )
		return false;
	for ( int i = vAbilityGroups[nCurrentAbilityGroup].first; i <=  vAbilityGroups[nCurrentAbilityGroup].second; ++i )
	{
		const SSlot &slot = slots[i];
		for ( std::vector< CPtr<CMOSelectable> >::const_iterator it = slot.objects.begin(); it != slot.objects.end(); ++it )
		{
			const CMOSelectable *pSO = *it;
			if ( pSO == pMO )
				return true;
		}
	}
	return false;
}

bool CSelector::IsSuperActive( const CMOSelectable *pMO ) const
{
	return find( superActives.begin(), superActives.end(), pMO ) != superActives.end();
}

const CMOSelectable* CSelector::GetFirstSlotUnit( int nSlot ) const
{
	if ( nSlot < 0 || nSlot >= slots.size() )
		return 0;
		
	const SSlot &slot = slots[nSlot];
	if ( slot.objects.empty() )
		return 0;
		
	return slot.objects.front();
}

void CSelector::FilterActions( CUserActions *pActionsBy, CMapObj *pMO ) const
{
	if ( !pMO || objList.empty() )
		return;

	EObjectClass eObjClass1 = GetObjectClass( pMO );
	bool bIsSameClass = false;
	for ( CMapObjectsVector::const_iterator it = objList.begin(); it != objList.end(); ++it )
	{
		CMOSelectable *pObj = *it;
		if ( eObjClass1 == GetObjectClass( pObj ) )
		{
			bIsSameClass = true;
			break;
		}
	}
		
	if ( !bIsSameClass )
	{
		*pActionsBy &= uaMultiClassActions;
	}
	
	if ( !pMO->IsFriend() )
	{
		pActionsBy->RemoveAction( NDb::USER_ACTION_FILL_RU );
	}
}

void CSelector::AfterLoad()
{
	std::vector<CMOSelectable*> units;
	GetSelection( &units );
	for ( std::vector<CMOSelectable*>::iterator it = units.begin(); it != units.end(); ++it )
		(*it)->Select( true );

	SetPreselection( 0 );
	
	Segment();
}

int CSelector::operator&( IBinSaver &saver )
{
	saver.Add( 1, &objList );
	saver.Add( 2, &newObjList );
	saver.Add( 3, &groups );
	saver.Add( 6, &eSelectionState );
	saver.Add( 7, &vAbilityGroups );
	saver.Add( 8, &nCurrentAbilityGroup );
	saver.Add( 9, &uaCurrentAbility );
//	saver.Add( 10, &nMaxUnits );
//	saver.Add( 11, &pSuperActive );
	saver.Add( 12, &bUnloadMode );
//	saver.Add( 13, &uaAllUnitActions );
	saver.Add( 14, &bSupportSingleSelection );
//	saver.Add( 15, &uaAllCommonActions ); // создается каждый раз заново
	saver.Add( 17, &slots );
	saver.Add( 18, &nMaxUnitSlots );
	saver.Add( 19, &nMaxUnitPerSlot );
//	saver.Add( 20, &superActive );
	saver.Add( 21, &superActives );
	saver.Add( 22, &bNeedUpdateSelectedUnits );
	saver.Add( 23, &bNeedUpdatePreselectedObject );
	saver.Add( 24, &bNeedUpdateAbilityIcons );
	saver.Add( 25, &bNeedUpdateActionButtons );
	saver.Add( 26, &bNeedUpdateSelectionInterior );
	saver.Add( 27, &pPreselectedObject );
	
	return 0;
}

START_REGISTER(SelectorCommands)

REGISTER_VAR_EX( "show_all_objects_info", NGlobal::VarBoolHandler, &s_bShowAllObjectsInfo, false, STORAGE_NONE );
REGISTER_VAR_EX( "show_buildings_info", NGlobal::VarBoolHandler, &s_bShowBuildingsInfo, false, STORAGE_NONE );

FINISH_REGISTER

REGISTER_SAVELOAD_CLASS( 0x15078B00, CSelector );


