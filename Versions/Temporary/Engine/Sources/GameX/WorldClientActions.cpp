#include "stdafx.h"

#include "WorldClient.h"
#include "GameXClassIDs.h"
#include "Stats_B2_M1/AIUnitCmd.h"

#include "CommandsSender.h"
#include "AILogic/B2AI.h"
#include "Common_RTS_AI/AIClasses.h"

// ************************************************************************************************************************ //
// **
// ** PerformAction(s)
// **
// **
// **
// ************************************************************************************************************************ //

bool CWorldClient::PerformGroupAction( const struct SAIUnitCmd *pCommand, bool bPlaceInQueue )
{
	return pSelector->DoGroupCommand( pCommandsSender, pCommand, bPlaceInQueue );
}

bool CWorldClient::PerformGroupAction( EActionCommand eActionCommand, bool bPlaceInQueue )
{
	SAIUnitCmd command( eActionCommand );
	return PerformGroupAction( &command, bPlaceInQueue );
}

bool CWorldClient::PerformGroupAction( EActionCommand eActionCommand, int nObjectID, bool bPlaceInQueue )
{
	SAIUnitCmd command( eActionCommand );
	command.nObjectID = nObjectID;
	return PerformGroupAction( &command, bPlaceInQueue );
}

bool CWorldClient::PerformGroupAction( EActionCommand eActionCommand, int nObjectID, float fParam, bool bPlaceInQueue )
{
	SAIUnitCmd command( eActionCommand );
	command.nObjectID = nObjectID;
	command.fNumber = fParam;
	return PerformGroupAction( &command, bPlaceInQueue );
}

bool CWorldClient::PerformGroupAction( EActionCommand eActionCommand, float fParam, bool bPlaceInQueue )
{
	SAIUnitCmd command( eActionCommand, fParam );
	return PerformGroupAction( &command, bPlaceInQueue );
}

bool CWorldClient::PerformGroupAction( EActionCommand eActionCommand, const CVec2 &vPos, bool bPlaceInQueue )
{
	if ( !IsInsideAIMap( vPos ) )
		return false;
	
	SAIUnitCmd command( eActionCommand );
	command.vPos = vPos;
	return PerformGroupAction( &command, bPlaceInQueue );
}

bool CWorldClient::PerformGroupAction( EActionCommand eActionCommand, const CVec2 &vPos, int nObjectID, bool bPlaceInQueue )
{
	if ( !IsInsideAIMap( vPos ) )
		return false;
	
	SAIUnitCmd command( eActionCommand );
	command.vPos = vPos;
	command.nObjectID = nObjectID;
	return PerformGroupAction( &command, bPlaceInQueue );
}

bool CWorldClient::PerformGroupAction( EActionCommand eActionCommand, const CVec2 &vPos, float fParam, bool bPlaceInQueue )
{
	if ( !IsInsideAIMap( vPos ) )
		return false;
	
	SAIUnitCmd command( eActionCommand );
	command.vPos = vPos;
	command.fNumber = fParam;
	return PerformGroupAction( &command, bPlaceInQueue );
}

bool CWorldClient::PerformGroupActionAutocast( const struct SAIUnitCmd *pCommand, bool bPlaceInQueue )
{
	return pSelector->DoGroupCommandAutocast( pCommandsSender, pCommand, bPlaceInQueue );
}

// ************************************************************************************************************************ //
// **
// ** USER_ACTION
// **
// **
// **
// ************************************************************************************************************************ //

bool CWorldClient::ActionMove( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	if ( bForced ) 
	{
		if ( pMO )
		{
			switch( pMO->GetTypeID() ) 
			{
			case NDb::SMechUnitRPGStats::typeID:
			case NDb::SInfantryRPGStats::typeID:
				return PerformGroupAction( ACTION_COMMAND_FOLLOW, vPos, pMO->GetID(), GetPlaceInQueue() );
			}
		}
	}
	bool bResult = PerformGroupAction( ACTION_COMMAND_MOVE_TO, vPos, GetPlaceInQueue() );
//	if ( bResult )
//		PlaceMapCommandAck( CVec3( vPos.x, vPos.y, Singleton<IAILogic>()->GetZ( vPos ) ) );
	return bResult;
}

bool CWorldClient::ActionAttack( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	if ( !pMO )
	{
		bool bResult = PerformGroupAction( ACTION_COMMAND_SWARM_TO, vPos, GetPlaceInQueue() );
//		if ( bResult ) 
//			PlaceMapCommandAck( CVec3( vPos.x, vPos.y, Singleton<IAILogic>()->GetZ( vPos ) ) );
//		if ( PerformGroupAction( ACTION_COMMAND_ART_BOMBARDMENT, vPos, GetPlaceInQueue() ) ) 
//			bResult = true;
		return bResult;
	}
	else
	{
		if ( bForced || pMO->GetDiplomacy() == EDI_ENEMY )
		{
			switch( pMO->GetTypeID() ) 
			{
			case NDb::SMechUnitRPGStats::typeID:
			case NDb::SInfantryRPGStats::typeID:
				return PerformGroupAction( ACTION_COMMAND_ATTACK_UNIT, vPos, pMO->GetID(), GetPlaceInQueue() );
			case NDb::SBuildingRPGStats::typeID:
			case NDb::SObjectRPGStats::typeID:
			case NDb::SBridgeRPGStats::typeID:
			case NDb::SFenceRPGStats::typeID:
				return PerformGroupAction( ACTION_COMMAND_ATTACK_OBJECT, vPos, pMO->GetID(), GetPlaceInQueue() );
			}
		}
	}
	return false;
}

bool CWorldClient::ActionRotate( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_ROTATE_TO, vPos, GetPlaceInQueue() );
}

bool CWorldClient::ActionMoveToGrid( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_MOVE_TO_GRID, vPos, GetPlaceInQueue() );
}

bool CWorldClient::ActionSwarm( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_SWARM_TO, vPos, GetPlaceInQueue() );
}

bool CWorldClient::ActionAmbush( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_AMBUSH, (float)eCurrentAbilityParam, GetPlaceInQueue() );
}

bool CWorldClient::ActionFollow( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	if ( pMO && ( ( pMO->GetTypeID() == NDb::SMechUnitRPGStats::typeID ) || ( pMO->GetTypeID() == NDb::SInfantryRPGStats::typeID ) ) )
		return PerformGroupAction( ACTION_COMMAND_FOLLOW, vPos, pMO->GetID(), GetPlaceInQueue() );
	return false;
}

bool CWorldClient::ActionStandGround( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_STAND_GROUND, GetPlaceInQueue() );
}

bool CWorldClient::ActionBoard( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	if ( !pMO )
		return false;

	switch ( pMO->GetTypeID() )
	{
		case NDb::SEntrenchmentRPGStats::typeID:
		case NDb::SBuildingRPGStats::typeID:
			return PerformGroupAction( ACTION_COMMAND_ENTER, pMO->GetID(), GetPlaceInQueue() );
		case NDb::SMechUnitRPGStats::typeID:
		{
			bool bResult = PerformGroupAction( ACTION_COMMAND_LOAD, pMO->GetID(), GetPlaceInQueue() );
			bResult = bResult || PerformGroupAction( ACTION_COMMAND_MECH_ENTER, pMO->GetID(), GetPlaceInQueue() ); // attempt to load rest of selection (mech units)
			return bResult;
		}
		case NDb::SInfantryRPGStats::typeID:
			if ( pSelector->GetSelectionState() == ssBuilding )
			{
				std::vector< CMOSelectable* >selection;
				pSelector->GetSelection( &selection );
				const int nBuildingID = (*(selection.begin()))->GetID();

				std::vector<int> squad;
				const IMOUnit *pUnit = dynamic_cast<const IMOUnit *>( pMO );
				if ( !pUnit )
					return false;
				squad.push_back( pUnit->GetSquad()->GetID() );

				const int groupID = pCommandsSender->CommandRegisterGroup( squad );
				SAIUnitCmd command;		
				command.nCmdType = ACTION_COMMAND_ENTER;
				command.nObjectID = nBuildingID;
				pCommandsSender->CommandGroupCommand( &command, groupID, GetPlaceInQueue(), ML_COMMAND_SAVE_GAME );
				pCommandsSender->CommandUnregisterGroup( groupID );
				return true;
			}
			else
			{

			}
		default:
			return false;
	}
}

bool CWorldClient::ActionMechBoard( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	if ( !pMO )
		return false;

	if ( pMO->GetTypeID() == NDb::SMechUnitRPGStats::typeID )
	{
		bool bResult = PerformGroupAction( ACTION_COMMAND_MECH_ENTER, pMO->GetID(), GetPlaceInQueue() );
		bResult = bResult || PerformGroupAction( ACTION_COMMAND_LOAD, pMO->GetID(), GetPlaceInQueue() ); // attempt to load rest of selection (infantry)
		return bResult;
	}

	return false;
}

bool CWorldClient::ActionLeave( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	if ( pSelector->GetSelectionState() == ssBuilding )
	{
		std::vector< CMOSelectable* >selection;
		pSelector->GetSelection( &selection );

		for ( std::vector< CMOSelectable* >::iterator it = selection.begin(); it != selection.end(); ++it )
		{
			IMOContainer *pContainer = checked_cast<IMOContainer *>( *it );
			std::vector<CMOSelectable*> passengers;
			pContainer->GetPassangers( &passengers );
			std::vector<int> units;
			units.resize( passengers.size() );
			for ( int i = 0; i < passengers.size(); ++i )
			{
				units[i] = passengers[i]->GetID();
			}
	
			const int groupID = pCommandsSender->CommandRegisterGroup( units );
			SAIUnitCmd command;		
			command.nCmdType = ACTION_COMMAND_LEAVE;
			command.vPos = vPos;
			command.fNumber = ALP_POSITION_VALID;
			pCommandsSender->CommandGroupCommand( &command, groupID, GetPlaceInQueue(), ML_COMMAND_SAVE_GAME );
			pCommandsSender->CommandUnregisterGroup( groupID );
		}
		return true;
	}
	else
		return PerformGroupAction( ACTION_COMMAND_UNLOAD, vPos, GetPlaceInQueue() );
}

// unload only one squad
bool CWorldClient::ActionLeaveOneSquad( const int nIndex )
{
	if ( pSelector->GetSelectionState() == ssBuilding )
	{
		std::vector< CMOSelectable* >selection;
		pSelector->GetSelection( &selection );
		for ( std::vector< CMOSelectable* >::iterator it = selection.begin(); it != selection.end(); ++it )
		{
			IMOContainer *pContainer = checked_cast<IMOContainer *>( *it );

			std::vector<CMOSelectable*> passangers;
			pContainer->GetPassangers( &passangers );
			if ( nIndex < 0 || nIndex >= passangers.size() ) 
				return false;
			const int nUnitIndex = passangers[nIndex]->GetID();

			std::vector<int> units;
			units.push_back( nUnitIndex );

			const int groupID = pCommandsSender->CommandRegisterGroup( units );
			SAIUnitCmd command;		
			command.nCmdType = ACTION_COMMAND_LEAVE;
			command.fNumber = ALP_POSITION_INVALID;
			command.vPos = CVec2( 300.0f, 300.0f );
			pCommandsSender->CommandGroupCommand( &command, groupID, GetPlaceInQueue(), ML_COMMAND_SAVE_GAME );
			pCommandsSender->CommandUnregisterGroup( groupID );
		}
		return true;
	}
	else
	{
//		return PerformGroupAction( ACTION_COMMAND_UNLOAD, CVec2( 300.0f, 300.0f ) ,(float)ALP_POSITION_INVALID, GetPlaceInQueue() );

		std::vector< CMOSelectable* >selection;
		pSelector->GetSelection( &selection );
		for ( std::vector< CMOSelectable* >::iterator it = selection.begin(); it != selection.end(); ++it )
		{
			IMOContainer *pContainer = checked_cast<IMOContainer *>( *it );

			std::vector<CMOSelectable*> passangers;
			pContainer->GetPassangers( &passangers );
			if ( nIndex < 0 || nIndex >= passangers.size() ) 
				return false;
				
			const int nUnitIndex = passangers[nIndex]->GetID();

			std::vector<int> group;
			group.push_back( pContainer->GetID() );

			SAIUnitCmd command( ACTION_COMMAND_UNLOAD );
			command.nObjectID = nUnitIndex;
			command.fNumber = (float)ALP_POSITION_INVALID;
			command.vPos = CVec2( 300.0f, 300.0f );
			const int groupID = pCommandsSender->CommandRegisterGroup( group );
			pCommandsSender->CommandGroupCommand( &command, groupID, GetPlaceInQueue(), ML_COMMAND_SAVE_GAME );
			pCommandsSender->CommandUnregisterGroup( groupID );
			return true;
		}
		return false;
	}
}

bool CWorldClient::ActionInstall( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_INSTALL, GetPlaceInQueue() );
}

bool CWorldClient::ActionUnInstall( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_UNINSTALL, GetPlaceInQueue() );
}

bool CWorldClient::ActionCaptureArtillery( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	if ( pMO && pMO->IsAlive() && pMO->IsNeutral() )
		return PerformGroupAction( ACTION_COMMAND_CATCH_ARTILLERY, pMO->GetID(), 1.1f, GetPlaceInQueue() );
	return false;
}

bool CWorldClient::ActionHookArtillery( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	if ( pMO )
		return PerformGroupAction( ACTION_COMMAND_TAKE_ARTILLERY, pMO->GetID(), GetPlaceInQueue() );
	else
		return PerformGroupAction( ACTION_COMMAND_TAKE_ARTILLERY, vPos, 0.0f, GetPlaceInQueue() );
	return false;
}

bool CWorldClient::ActionDeployArtillery( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_DEPLOY_ARTILLERY, vPos, bForced ? 0.0f : 1.0f , GetPlaceInQueue() );
}

bool CWorldClient::ActionEntrenchSelf( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_ENTRENCH_SELF, GetPlaceInQueue() );
}

bool CWorldClient::ActionPlaceMines( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	CSegment marker;

	SAIUnitCmd cmd( ACTION_COMMAND_PLACEMINE, vPos );
	cmd.nNumber = 1;								//Need visualisation
	PerformGroupAction( &cmd, true );
	Singleton<IAILogic>()->RequestBuildPreview( ACTION_COMMAND_PLACEMINE, VNULL2, VNULL2, true );
	eBuildObjectState = EBS_NONE;
	return true;
}

bool CWorldClient::ActionClearMines( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_CLEARMINE, vPos, GetPlaceInQueue() );
}

bool CWorldClient::ActionBuildEntrenchment( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	CSegment marker;

	if ( IsFirstCommandPointDefined() )
	{
		SAIUnitCmd cmd( ACTION_COMMAND_ENTRENCH_BEGIN, GetFirstCommandPoint() );
		cmd.nNumber = 1;								//Need visualisation
		PerformGroupAction( &cmd, false );
		cmd.nCmdType = ACTION_COMMAND_ENTRENCH_END;
		cmd.vPos = vPos;
		PerformGroupAction( &cmd, true );
		Singleton<IAILogic>()->RequestBuildPreview( ACTION_COMMAND_ENTRENCH_END, VNULL2, VNULL2, true );
		ResetFirstCommandPoint();
		eBuildObjectState = EBS_NONE;
	}
	else 
	{
		SetFirstCommandPoint( vPos );		
		eBuildObjectState = EBS_FIRST_POINT_SELECTED;

		lastUpdate = 0;
		vLastBuildPos.Set( 0, 0 );

		return false;
	}
	return true;
}

bool CWorldClient::ActionBuildFence( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	CSegment marker;

	if ( IsFirstCommandPointDefined() )
	{
		SAIUnitCmd cmd( ACTION_COMMAND_BUILD_FENCE_BEGIN, GetFirstCommandPoint() );
		cmd.nNumber = 1;								//Need visualisation
		PerformGroupAction( &cmd, false );
		cmd.nCmdType = ACTION_COMMAND_BUILD_FENCE_END;
		cmd.vPos = vPos;
		PerformGroupAction( &cmd, true );
		Singleton<IAILogic>()->RequestBuildPreview( ACTION_COMMAND_BUILD_FENCE_END, VNULL2, VNULL2, true );
		ResetFirstCommandPoint();
		eBuildObjectState = EBS_NONE;
	}
	else 
	{
		SetFirstCommandPoint( vPos );		
		eBuildObjectState = EBS_FIRST_POINT_SELECTED;

		lastUpdate = 0;
		vLastBuildPos.Set( 0, 0 );

		return false;
	}
	return true;
}

bool CWorldClient::ActionRepair( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	if ( pMO )
	{
		CUserActions actions;
		pMO->GetActions( &actions, ACTIONS_WITH );
		if ( actions.HasAction( NDb::USER_ACTION_ENGINEER_REPAIR ) ) 
		{
			switch ( pMO->GetTypeID() )
			{
				case NDb::SBuildingRPGStats::typeID:
				case NDb::SBridgeRPGStats::typeID:
					return PerformGroupAction( ACTION_COMMAND_REPEAR_OBJECT, pMO->GetID(), GetPlaceInQueue() );
				default:
					return PerformGroupAction( ACTION_COMMAND_REPAIR, pMO->GetID(), GetPlaceInQueue() );
			}
		}
		return false;
	}
	else
		return PerformGroupAction( ACTION_COMMAND_REPAIR, vPos, GetPlaceInQueue() );
}

bool CWorldClient::ActionResupply( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	if ( pMO )
	{
		CUserActions actions;
		pMO->GetActions( &actions, ACTIONS_WITH );
		if ( actions.HasAction( NDb::USER_ACTION_SUPPORT_RESUPPLY ) ) 
			return PerformGroupAction( ACTION_COMMAND_RESUPPLY, pMO->GetID(), GetPlaceInQueue() );
		return false;
	}
	else
		return PerformGroupAction( ACTION_COMMAND_RESUPPLY, vPos, GetPlaceInQueue() );
}

bool CWorldClient::ActionChangeFormation0( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_PARADE, float(NDb::SSquadRPGStats::SFormation::DEFAULT), GetPlaceInQueue() );
}

bool CWorldClient::ActionChangeFormation1( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_PARADE, float(NDb::SSquadRPGStats::SFormation::MOVEMENT), GetPlaceInQueue() );
}

bool CWorldClient::ActionChangeFormation2( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_PARADE, float(NDb::SSquadRPGStats::SFormation::DEFENSIVE), GetPlaceInQueue() );
}

bool CWorldClient::ActionChangeFormation3( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_PARADE, float(NDb::SSquadRPGStats::SFormation::OFFENSIVE), GetPlaceInQueue() );
}

bool CWorldClient::ActionChangeFormation4( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_PARADE, float(NDb::SSquadRPGStats::SFormation::SNEAK), GetPlaceInQueue() );
}

bool CWorldClient::ActionChangeShellDamage( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_CHANGE_SHELLTYPE, 0.0f, GetPlaceInQueue() );
}

bool CWorldClient::ActionChangeShellAgit( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_CHANGE_SHELLTYPE, 1.0f, GetPlaceInQueue() );
}

bool CWorldClient::ActionChangeShellSmoke( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_CHANGE_SHELLTYPE, 2.0f, GetPlaceInQueue() );
}

bool CWorldClient::ActionStop( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	bool bRes = PerformGroupAction( ACTION_COMMAND_STOP, GetPlaceInQueue() );
//	// set Hold Ground mode then unit stopped by user
//	PerformGroupAction( ACTION_COMMAND_STAND_GROUND, true );
	return bRes;
}

bool CWorldClient::ActionCamoflage( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_CAMOFLAGE_MODE, (float)eCurrentAbilityParam, GetPlaceInQueue() );
}

bool CWorldClient::ActionAdvancedCamoflage( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_ADAVNCED_CAMOFLAGE_MODE, (float)eCurrentAbilityParam, GetPlaceInQueue() );
}

bool CWorldClient::ActionUseSpyGlass( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_USE_SPYGLASS, vPos, (float)eCurrentAbilityParam, GetPlaceInQueue() );
}

bool CWorldClient::ActionUseThrow( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	if ( pMO )
	{
		switch( pMO->GetTypeID() ) 
		{
			case NDb::SMechUnitRPGStats::typeID:
			case NDb::SInfantryRPGStats::typeID:
			{
				SAIUnitCmd command( ACTION_COMMAND_THROW_GRENADE );
				//command.nNumber = (int)EASS_READY_TO_ON;
				command.nObjectID = pMO->GetID();
				command.nNumber = (int)ATGP_ATACK_UNIT;
				return PerformGroupAction( &command, GetPlaceInQueue() );
			}
			case NDb::SBuildingRPGStats::typeID:
			case NDb::SObjectRPGStats::typeID:
			{
				SAIUnitCmd command( ACTION_COMMAND_THROW_GRENADE );
				//command.nNumber = (int)EASS_READY_TO_ON;
				command.nObjectID = pMO->GetID();
				command.nNumber = (int)ATGP_ATACK_OBJECT;
				return PerformGroupAction( &command, GetPlaceInQueue() );
			}
			default:
				return false;
		}
	}
	else
	{
		SAIUnitCmd command( ACTION_COMMAND_THROW_GRENADE );
		//command.nNumber = (int)EASS_READY_TO_ON;
		command.vPos = vPos;
		command.nNumber = (int)ATGP_ATACK_POINT;
		return PerformGroupAction( &command, GetPlaceInQueue() );
	}
}

bool CWorldClient::ActionUseLandMine( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return true;
}

bool CWorldClient::ActionUseBlastingCharge( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
/*	if ( pMO )
	{
		switch( pMO->GetTypeID() ) 
		{
			case NDb::SMechUnitRPGStats::typeID:
			case NDb::SInfantryRPGStats::typeID:
			{
				SAIUnitCmd command( ACTION_COMMAND_BLASTING_CHARGE );
				command.nNumber = (int)EASS_READY_TO_ON;
				command.nObjectID = pMO->GetID();
				command.fNumber = (float)ATGP_ATACK_UNIT;
				return PerformGroupAction( &command, GetPlaceInQueue() );
			}
			case NDb::SBuildingRPGStats::typeID:
			case NDb::SObjectRPGStats::typeID:
			{
				SAIUnitCmd command( ACTION_COMMAND_BLASTING_CHARGE );
				command.nNumber = (int)EASS_READY_TO_ON;
				command.nObjectID = pMO->GetID();
				command.fNumber = (float)ATGP_ATACK_OBJECT;
				return PerformGroupAction( &command, GetPlaceInQueue() );
			}
			default:
				return false;
		}
	}
	else*/
	{
		SAIUnitCmd command( ACTION_COMMAND_PLACE_CHARGE );
		command.nNumber = (int)EASS_READY_TO_ON;
		command.vPos = vPos;
		command.fNumber = 0.0f;
		return PerformGroupAction( &command, GetPlaceInQueue() );
	}
}

bool CWorldClient::ActionUseControlledCharge( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	if ( pMO )
	{
		/*switch( pMO->GetTypeID() ) 
		{
			case NDb::SMechUnitRPGStats::typeID:
			case NDb::SInfantryRPGStats::typeID:
			{
				SAIUnitCmd command( ACTION_COMMAND_CONTROLLED_CHARGE );
				command.nNumber = (int)EASS_READY_TO_ON;
				command.nObjectID = pMO->GetID();
				command.fNumber = (float)ATGP_ATACK_UNIT;
				return PerformGroupAction( &command, GetPlaceInQueue() );
			}
			case NDb::SBuildingRPGStats::typeID:
			case NDb::SObjectRPGStats::typeID:
			{
				SAIUnitCmd command( ACTION_COMMAND_CONTROLLED_CHARGE );
				command.nNumber = (int)EASS_READY_TO_ON;
				command.nObjectID = pMO->GetID();
				command.fNumber = (float)ATGP_ATACK_OBJECT;
				return PerformGroupAction( &command, GetPlaceInQueue() );
			}
			default:
				return false;
		}*/
		NI_ASSERT( false, "Placing charge on an object" );
		return false;
	}
	else
	{
		SAIUnitCmd command( ACTION_COMMAND_PLACE_CONTROLLED_CHARGE );
		command.nNumber = (int)EASS_READY_TO_ON;
		command.vPos = vPos;
		command.fNumber = 0.0f;
		return PerformGroupAction( &command, GetPlaceInQueue() );
	}
}

bool CWorldClient::ActionDetonateControlledCharge( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_DETONATE, GetPlaceInQueue() );
}

bool CWorldClient::ActionUseHoldSector( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_HOLD_SECTOR, vPos, (float)eCurrentAbilityParam, GetPlaceInQueue() );
}

bool CWorldClient::ActionSupportFire( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	if ( pMO )
	{
		switch( pMO->GetTypeID() ) 
		{
		case NDb::SMechUnitRPGStats::typeID:
		case NDb::SInfantryRPGStats::typeID:
			{
				SAIUnitCmd command( ACTION_COMMAND_SUPPORT_FIRE );
				command.nNumber = (int)EASS_READY_TO_ON;
				command.nObjectID = pMO->GetID();
				command.fNumber = (float)ATGP_ATACK_UNIT;
				return PerformGroupAction( &command, GetPlaceInQueue() );
			}
		default:
			return false;
		}
	}
	else
	{
	}
	return false;
}

bool CWorldClient::ActionPatrol( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_PATROL, vPos, GetPlaceInQueue() );
}

bool CWorldClient::ActionSpyMode( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	if ( pMO )
	{
		switch( pMO->GetTypeID() ) 
		{
		case NDb::SInfantryRPGStats::typeID:
			{
				SAIUnitCmd command( ACTION_COMMAND_SPY_MODE );
				command.nObjectID = pMO->GetID();
				//command.pTarget = pMO->GetStats();
				return PerformGroupAction( &command, GetPlaceInQueue() );
			}
		default:
			return false;
		}

	}
	else if ( vPos == VNULL2 )			// Ability off
	{
		SAIUnitCmd command( ACTION_COMMAND_SPY_MODE );
		command.fNumber = NDb::PARAM_ABILITY_OFF;
		return PerformGroupAction( &command, GetPlaceInQueue() );
	}
	return false;
}

bool CWorldClient::ActionUseTrackTargeting( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	if ( pMO )
	{
		switch( pMO->GetTypeID() ) 
		{
			case NDb::SMechUnitRPGStats::typeID:			// Can use TT only on MechUnits
			{
				const int nPassClass = checked_cast<const NDb::SMechUnitRPGStats*>(pMO->GetStats())->nAIPassabilityClass;
				if ( nPassClass & EAC_TRACK )
				{
					SAIUnitCmd command( ACTION_COMMAND_TRACK_TARGETING );
					command.nNumber = (int)EASS_READY_TO_ON;
					command.nObjectID = pMO->GetID();
					command.fNumber = (float)ATGP_ATACK_UNIT;
					return PerformGroupAction( &command, GetPlaceInQueue() );
				}
			}
			case NDb::SInfantryRPGStats::typeID:
			case NDb::SBuildingRPGStats::typeID:
			case NDb::SObjectRPGStats::typeID:
			{
				/*SAIUnitCmd command( ACTION_COMMAND_TRACK_TARGETING );
				command.nNumber = (int)EASS_READY_TO_ON;
				command.nObjectID = pMO->GetID();
				command.fNumber = (float)ATGP_ATACK_OBJECT;
				return PerformGroupAction( &command, GetPlaceInQueue() );*/
			}
			default:
				return false;
		}
	}
	else
	{
		/*SAIUnitCmd command( ACTION_COMMAND_TRACK_TARGETING );
		command.nNumber = (int)EASS_READY_TO_ON;
		command.vPos = vPos;
		command.fNumber = (float)ATGP_ATACK_POINT;
		return PerformGroupAction( &command, GetPlaceInQueue() );*/
	}
	return false;
}

bool CWorldClient::ActionSupressFire( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	if ( pMO )
	{
		return false;
	}
	else
	{
		bool bResult = PerformGroupAction( ACTION_COMMAND_ART_BOMBARDMENT, vPos, GetPlaceInQueue() );
		return bResult;
	}
}

bool CWorldClient::ActionCriticalTargetting( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_CRITICAL_TARGETING, (float)eCurrentAbilityParam, GetPlaceInQueue() );
}

bool CWorldClient::ActionRapidFire( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_RAPID_FIRE_MODE, (float)eCurrentAbilityParam, GetPlaceInQueue() );
}


bool CWorldClient::ActionCounterFire( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	CSegment marker;

	SAIUnitCmd command( ACTION_COMMAND_COUNTER_FIRE );
	command.nNumber = (int)EASS_READY_TO_ON;
	command.vPos = vPos;
	command.fNumber = -1.0f;

	/*marker.p1 = CVec2( vPos.x - 10, vPos.y );
	marker.p2 = CVec2( vPos.x + 10, vPos.y );
	marker.dir = marker.p2 - marker.p1;
	DebugInfoManager()->CreateSegment( OBJECT_ID_FORGET, marker, 1, NAIVisInfo::GREEN );
	marker.p1 = CVec2( vPos.x, vPos.y - 10 );
	marker.p2 = CVec2( vPos.x, vPos.y + 10 );
	marker.dir = marker.p2 - marker.p1;
	DebugInfoManager()->CreateSegment( OBJECT_ID_FORGET, marker, 1, NAIVisInfo::GREEN );*/
	
	return PerformGroupAction( &command, GetPlaceInQueue() );
}

bool CWorldClient::ActionShootInMovement( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_MOBILE_SHOOT, (float)eCurrentAbilityParam, GetPlaceInQueue() );
}

bool CWorldClient::ActionOverload( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_OVERLOAD_MODE, (float)eCurrentAbilityParam, GetPlaceInQueue() );
}

bool CWorldClient::ActionSmokeShots( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_SMOKE_SHOTS, (float)eCurrentAbilityParam, GetPlaceInQueue() );
}

bool CWorldClient::ActionLinkedGrenades( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_LINKED_GRENADES, (float)eCurrentAbilityParam, GetPlaceInQueue() );
}

bool CWorldClient::ActionCaution( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_CAUTION, (float)eCurrentAbilityParam, GetPlaceInQueue() );
}

bool CWorldClient::ActionDropBombs( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	int nID = 0;
	if ( pMO )
	{
		switch ( pMO->GetTypeID() ) 
		{
			case NDb::SMechUnitRPGStats::typeID:
			case NDb::SInfantryRPGStats::typeID:
			case NDb::SBuildingRPGStats::typeID:
				nID = pMO->GetID();
		}
		return PerformGroupAction( ACTION_COMMAND_DROP_BOMBS, vPos, nID, GetPlaceInQueue() );
	}
	return PerformGroupAction( ACTION_COMMAND_DROP_BOMBS, vPos, nID, GetPlaceInQueue() );
}

bool CWorldClient::ActionExactShot( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	if ( pMO )
	{
		switch ( pMO->GetTypeID() ) 
		{
			case NDb::SMechUnitRPGStats::typeID:
			case NDb::SInfantryRPGStats::typeID:
				return PerformGroupAction( ACTION_COMMAND_EXACT_SHOT, pMO->GetID(), GetPlaceInQueue() );
		}
	}
	return false;
}

bool CWorldClient::ActionCoverFire( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_COVER_FIRE, vPos, GetPlaceInQueue() );
}

bool CWorldClient::ActionZeroingIn( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	return PerformGroupAction( ACTION_COMMAND_RANGE_AREA, vPos, GetPlaceInQueue() );
}

bool CWorldClient::ActionFirstAid( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	if ( pMO )
	{
		switch ( pMO->GetTypeID() ) 
		{
			case NDb::SInfantryRPGStats::typeID:
				return PerformGroupAction( ACTION_COMMAND_FIRST_AID, pMO->GetID(), GetPlaceInQueue() );
		}
	}
	return false;
}

bool CWorldClient::ActionFillRU( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	if ( pMO )
		return PerformGroupAction( ACTION_COMMAND_FILL_RU, pMO->GetID(), GetPlaceInQueue() );
	return false;
}

bool CWorldClient::ActionGlobeBombMission( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	NI_ASSERT( pForcedActionParam != 0, "Can't find bomb mission param" );
	if ( pForcedActionParam == 0 )
		return false;

	NInput::PostEvent( "game_reset_forced_action", 0, 0 );

	return true;
}

bool CWorldClient::ActionReinfCommon( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	NInput::PostEvent( "forced_action_call_reinf", PackCoords( vPos ), 0 );

	return true;
}

bool CWorldClient::ActionReinfBomb( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	NInput::PostEvent( "forced_action_call_reinf", PackCoords( vPos ), 0 );

	return true;
}

bool CWorldClient::ActionReinfParatroopers( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	NInput::PostEvent( "forced_action_call_reinf", PackCoords( vPos ), 0 );

	return true;
}

bool CWorldClient::ActionReinfNone( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	NInput::PostEvent( "forced_action_call_no_reinf", PackCoords( vPos ), 0 );

	return false;
}

bool CWorldClient::ActionCallSuperWeapon( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	NInput::PostEvent( "forced_action_call_super_weapon", PackCoords( vPos ), 0 );

	return true;
}

bool CWorldClient::ActionPlaceMarker( const CVec2 &vPos, const CMapObj *pMO, bool bForced )
{
	SAIUnitCmd cmd;
	cmd.nCmdType = ACTION_COMMAND_PLACE_MARKER;
	cmd.vPos = vPos;
	GetCommandsSender()->CommandGeneralCommand( &cmd );

	return true;
}


