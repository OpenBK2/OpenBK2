#include "stdafx.h"

#include "Stats_B2_M1/DBClientConsts.h"
#include "b2_m1_world/MapObj.h"
#include "3DLib/Transform.h"
#include "Misc/StrProc.h"
#include "WorldClient.h"
#include "Stats_B2_M1/AIUnitCmd.h"

#include "SceneB2/Cursor.h"
#include "Sound/SoundScene.h"
#include "Stats_B2_M1/ActionsRemap.h"
#include "AILogic/AIDebugInfo.h"
#include "AILogic/B2AI.h"
#include "Misc/Win32Random.h"
#include "SceneB2/TerraGen.h"
#include "UISpecificB2/WindowMiniMap.h"
#include "B2_M1_World/MOEntrenchment.h"
#include "Stats_B2_M1/DBCameraConsts.h"
#include "CommandsSender.h"
#include "ScenarioTracker.h"
#include "InterfaceState.h"
#include "System/BinaryResources.h"
#include "GetConsts.h"
#include "GameXClassIDs.h"
#include "AILogic/FeedbackSystem.h"
#include "B2_M1_World/MissionObjectiveStates.h"
#include "B2_M1_World/MOBridge.h"
#include "Utils.h"

#include "DebugTools/DebugInfoManager.h"
#include "SuperWeapon.h"
#include "Stats_B2_M1/SuperWeaponUpdates.h"

const int CURSOR_FADE_HALF_SIZE_X = 150;
const int CURSOR_FADE_HALF_SIZE_Y = 150;

const int CLIENT_UNIQUE_ID_MAP_COMMAND_ACK = -2; // look for other CLIENT_UNIQUE_ID_xxx (криво, но менять поздно)
const int CLIENT_UNIQUE_ID_MAP_COMMAND_ACK_DIR = -3; // look for other CLIENT_UNIQUE_ID_xxx (криво, но менять поздно)

const float SELECTION_FADE_IN_TIME = 1.0f;
const float SELECTION_FADE_OUT_TIME = 2.0f;

const float NEW_ABILITY_ICON_SHOW_TIME = 60.0f;

const CVec4 COVERED_OBJECT_COLOR( 1.0f, 0.0f, 0.0f, 1.0f );

static bool s_bFadeMode = false;
static bool s_bXRayMode = true;
static bool s_bXRayCursorOnlyFilter = false; // CRAP - just for experiment
static bool s_bXRayFastFade = false; // CRAP - just for experiment

const int MAX_PLAYERS = 17; // CRAP - absolute limit
const int MAX_XRAY_TIME = 500; // keep units at x-ray list for this time

static bool bForcedXRayMode = false;

static void MsgXRay( const SGameMessage &msg, int nPressed );

// нужно для сохранения порядка сортировки равноправных объектов
struct SPickObject
{
	CMapObj *pMO;
	int nOrder; // чем меньше nOrder, тем ближе объект
};

// 1.level: hp > 0?
// 2.level: unit, building, entrenchment, mine, object
// 3.level: enemy, friend, neutral
// 4.level: can select, can't select
struct SMapObjectLessFunctional
{
	bool operator()( const SPickObject &pickObj1, const SPickObject &pickObj2 ) const
	{
		CMapObj *pMO1 = pickObj1.pMO;
		CMapObj *pMO2 = pickObj2.pMO;
		if ( (pMO1->GetHP() > 0) && (pMO2->GetHP()> 0) ) 
		{
			if ( pMO1->GetStats()->eGameType == pMO2->GetStats()->eGameType )
			{
				if ( pMO1->GetDiplomacy() == pMO2->GetDiplomacy() )
				{
					if ( pMO1->CanSelect() == pMO2->CanSelect() ) 
					{
						if ( pMO1->GetStats()->eVisType == pMO2->GetStats()->eVisType )
							return pickObj1.nOrder < pickObj2.nOrder;
						else
							return pMO1->GetStats()->eVisType == pMO2->GetStats()->eVisType;
					}
					else
						return pMO1->CanSelect() > pMO2->CanSelect();
				}
				else
					return pMO1->GetDiplomacy() > pMO2->GetDiplomacy();
			}
			else
				return pMO1->GetStats()->eGameType < pMO2->GetStats()->eGameType;
		}
		else
			return pMO1->GetHP() > pMO2->GetHP();
	}
};

int GetObjectRate( const CMapObj &pMapObject )
{
	switch( pMapObject.GetTypeID() ) 
	{
	case NDb::SBuildingRPGStats::typeID:
		return pMapObject.IsFriend() ? 2:1;
	case NDb::SMechUnitRPGStats::typeID:
	case NDb::SInfantryRPGStats::typeID:
		return pMapObject.IsFriend() ? 6:5;
	default:
		return 0;
	}
}

static void BuildScreenRect( CVec2 *pMin, CVec2 *pMax, const CVec2 &vPoint1, const CVec2 &vPoint2 )
{
	*pMin = CVec2( (std::min)( vPoint1.x, vPoint2.x ), (std::min)( vPoint1.y, vPoint2.y ) );
	*pMax = CVec2( (std::max)( vPoint1.x, vPoint2.x ), (std::max)( vPoint1.y, vPoint2.y ) );
}

static bool DoScreenRectsOverlap( const CVec2 &vMin1, const CVec2 &vMax1, const CVec2 &vMin2, const CVec2 &vMax2 )
{
	return vMin1.x <= vMax2.x && vMax1.x >= vMin2.x &&
				 vMin1.y <= vMax2.y && vMax1.y >= vMin2.y;
}

static bool ProjectWorldPointToScreen( CVec2 *pScreenPos, const CTransformStack &transform, const CVec2 &vScreenRect, const CVec3 &vWorldPos )
{
	CVec4 vProjected;
	transform.Get().forward.RotateHVector( &vProjected, vWorldPos );

	const float fW = fabs( vProjected.w );
	if ( fW < 0.000001f )
		return false;

	pScreenPos->x = vProjected.x / fW * vScreenRect.x * 0.5f + vScreenRect.x * 0.5f;
	pScreenPos->y = -vProjected.y / fW * vScreenRect.y * 0.5f + vScreenRect.y * 0.5f;
	return _finite( pScreenPos->x ) != 0 && _finite( pScreenPos->y ) != 0;
}

static bool AddProjectedPointToScreenRect( CVec2 *pMin, CVec2 *pMax, bool *pHasRect,
																					 const CTransformStack &transform, const CVec2 &vScreenRect, const CVec3 &vWorldPos )
{
	CVec2 vScreenPos;
	if ( !ProjectWorldPointToScreen( &vScreenPos, transform, vScreenRect, vWorldPos ) )
		return false;

	if ( !*pHasRect )
	{
		*pMin = vScreenPos;
		*pMax = vScreenPos;
		*pHasRect = true;
	}
	else
	{
		pMin->Minimize( vScreenPos );
		pMax->Maximize( vScreenPos );
	}
	return true;
}

static bool DoesMapObjectOverlapScreenRect( const CMapObj *pMO, const CVec2 &vSelectionMin, const CVec2 &vSelectionMax,
																						const CTransformStack &transform, const CVec2 &vScreenRect )
{
	NI_ASSERT( pMO != 0, "Map object is missing" );

	CVec2 vObjectMin, vObjectMax;
	bool bHasProjectedRect = false;

	if ( const NDb::SModel *pModel = pMO->GetModelDesc() )
	{
		if ( pModel->pGeometry )
		{
			CVec3 vPos, vScale;
			CQuat qRot;
			pMO->GetPlacement( &vPos, &qRot, &vScale );

			// CMapObj placement is stored in AI space; project the same visual-space placement used by scene rendering.
			CVec3 vVisPos;
			AI2Vis( &vVisPos, vPos );

			SFBTransform objectTransform;
			MakeMatrix( &objectTransform, vScale, vVisPos, qRot );

			const CVec3 vCenter = pModel->pGeometry->vCenter;
			const CVec3 vHalfSize = pModel->pGeometry->vSize * 0.5f;

			// Project all object-box corners, then collapse them to a plain 2D screen AABB.
			for ( int iX = 0; iX < 2; ++iX )
			{
				for ( int iY = 0; iY < 2; ++iY )
				{
					for ( int iZ = 0; iZ < 2; ++iZ )
					{
						const CVec3 vLocalCorner(
							vCenter.x + ( iX == 0 ? -vHalfSize.x : vHalfSize.x ),
							vCenter.y + ( iY == 0 ? -vHalfSize.y : vHalfSize.y ),
							vCenter.z + ( iZ == 0 ? -vHalfSize.z : vHalfSize.z ) );

						CVec3 vWorldCorner;
						objectTransform.forward.RotateHVector( &vWorldCorner, vLocalCorner );
						AddProjectedPointToScreenRect( &vObjectMin, &vObjectMax, &bHasProjectedRect, transform, vScreenRect, vWorldCorner );
					}
				}
			}
		}
	}

	if ( !bHasProjectedRect )
	{
		// Some map objects do not have a model/geometry descriptor; use their 3D center as a point-sized box.
		CVec3 vVisCenter;
		AI2Vis( &vVisCenter, pMO->GetCenter() );
		AddProjectedPointToScreenRect( &vObjectMin, &vObjectMax, &bHasProjectedRect, transform, vScreenRect, vVisCenter );
	}

	return bHasProjectedRect && DoScreenRectsOverlap( vObjectMin, vObjectMax, vSelectionMin, vSelectionMax );
}

// CWorldClient::SUISelection

struct CWorldClient::SUISelection
{
	struct SObject
	{
		CMOSelectable *pMO;
		CVec2 vPos;
		bool bCanMove;
		bool bCanRotate;
		CVec2 vMovePos;
		CVec2 vRotatePos;
		
		int operator&( IBinSaver &saver ) { NI_ASSERT( 0, "Wrong call" ); return 0; }
	};
	
	std::vector<SObject> objects;
	CVec2 vCenter;
	CVec2 vMovePos;
	CVec2 vRotatePos;
	
	int operator&( IBinSaver &saver ) { NI_ASSERT( 0, "Wrong call" ); return 0; }
	
	void CalcCurrentState( const std::vector<CMOSelectable*> &selection );
	void CalcTargetState( const CVec2 &vMovePos, const CVec2 &vStartRotatePos, const CVec2 &vFinishRotatePos );
};

void CWorldClient::SUISelection::CalcCurrentState( const std::vector<CMOSelectable*> &selection )
{
	objects.clear();
	objects.reserve( selection.size() );
	
	vCenter = VNULL2;
	int nCount = 0;
	for ( int i = 0; i < selection.size(); ++i )
	{
		SObject object;
		object.pMO = selection[i];

		CUserActions actionsBy;
		object.pMO->GetEnabledActions( &actionsBy, ACTIONS_BY );

		object.bCanMove = actionsBy.HasAction( NDb::USER_ACTION_MOVE );
		object.bCanRotate = actionsBy.HasAction( NDb::USER_ACTION_ROTATE );

		if ( !object.bCanMove && !object.bCanRotate )
			continue;
		
		if ( object.bCanMove )
		{
			if ( CDynamicCast<IMOSquad> pSquad = object.pMO )
			{
				std::vector<CMOSelectable*> members;
				pSquad->GetPassangers( &members );

				object.vPos = VNULL2;
				if ( !members.empty() )
				{
					for ( int j = 0; j < members.size(); ++j )
					{
						const CVec3 &center = members[j]->GetCenter();
						object.vPos.x += center.x;
						object.vPos.y += center.y;
					}
					object.vPos /= members.size();
				}
				else
					continue;

				vCenter += object.vPos;
				nCount++;
			}
			else
			{
				const CVec3 &center = object.pMO->GetCenter();
				object.vPos.x = center.x;
				object.vPos.y = center.y;

				vCenter += object.vPos;
				nCount++;
			}
		}

		objects.push_back( object );
	}
	if ( nCount > 0 )
		vCenter /= nCount;
}

void CWorldClient::SUISelection::CalcTargetState( const CVec2 &vMovePos, 
	const CVec2 &vStartRotatePos, const CVec2 &vFinishRotatePos )
{
	float fStartAngle = atan2f( vStartRotatePos.y - vMovePos.y, vStartRotatePos.x - vMovePos.x );
	float fFinishAngle = atan2f( vFinishRotatePos.y - vMovePos.y, vFinishRotatePos.x - vMovePos.x );
	float fDeltaAngle = fFinishAngle - fStartAngle;
	float fSin = sinf( fDeltaAngle );
	float fCos = cosf( fDeltaAngle );

	for ( int i = 0; i < objects.size(); ++i )
	{
		SObject &object = objects[i];

		CVec2 v = object.vPos - vCenter;
		CVec2 vDir( v.x * fCos - v.y * fSin, v.x * fSin + v.y * fCos );
		object.vMovePos = vDir + vMovePos;

		object.vRotatePos = object.vMovePos + (vFinishRotatePos - vMovePos);
	}
}

// CWorldClient

CWorldClient::CWorldClient()
{
	InitPrivate();
}

CWorldClient::CWorldClient( ITransceiver *pTransceiver, IVisualNotifications *pNotifications, IAILogic *_pAI, 
	IScenarioTracker *_pScenarioTracker, IMissionSuperWeapon *_pSuperWeapon ) : 
	pAI( _pAI ),
	CUpdatableWorld( pNotifications, _pAI ),
	pScenarioTracker( _pScenarioTracker ),
	pSuperWeapon( _pSuperWeapon )
{
	InitPrivate();

	pCommandsSender = new CCommandsSender( pTransceiver );

	const NDb::SClientGameConsts *pClient = NGameX::GetClientConsts();
	mapCommandAck.nUniqueID = CLIENT_UNIQUE_ID_MAP_COMMAND_ACK;
	if ( const NDb::SVisObj *pVisObj = pClient->mapCommandAck.pVisObj )
	{
		if ( !pVisObj->models.empty() )
			mapCommandAck.pModel = pVisObj->models.front().pModel;
	}
	mapCommandAck.bPlaced = false;
	mapCommandAck.fShowTime = pClient->mapCommandAck.fShowTime;
	// dir
	mapCommandAckDir.nUniqueID = CLIENT_UNIQUE_ID_MAP_COMMAND_ACK_DIR;
	if ( const NDb::SVisObj *pVisObj = pClient->mapCommandAckDir.pVisObj )
	{
		if ( !pVisObj->models.empty() )
			mapCommandAckDir.pModel = pVisObj->models.front().pModel;
	}
	mapCommandAckDir.bPlaced = false;
	mapCommandAckDir.fShowTime = pClient->mapCommandAckDir.fShowTime;

	eActionMode = EAM_SELECT;
}

CWorldClient::~CWorldClient()
{
}

void CWorldClient::InitPrivate()
{
	AddObserver( "minimap_down", &CWorldClient::MsgMinimapDown );
	AddObserver( "minimap_move", &CWorldClient::MsgMinimapMove );
	AddObserver( "minimap_up", &CWorldClient::MsgMinimapUp );
	
	AddObserver( "set_destination_minimap", &CWorldClient::MsgSetDestinationMinimap );
	AddObserver( "set_target_minimap", &CWorldClient::MsgSetTargetMinimap );

	AddObserver( "scroll_map_true", &CWorldClient::MsgScrollMap );

	AddObserver( "start_selection", &CWorldClient::MsgStartSelection );
	AddObserver( "update_selection", &CWorldClient::MsgUpdateSelection );
	AddObserver( "end_selection", &CWorldClient::MsgEndSelection );
	AddObserver( "cancel_selection", &CWorldClient::MsgCancelSelection );

	AddObserver( "set_target", &CWorldClient::MsgSetTarget );
	AddObserver( "reset_target2", &CWorldClient::MsgResetTarget );
	AddObserver( "set_destination", &CWorldClient::MsgSetDestination );

	AddObserver( "update_direction", &CWorldClient::MsgUpdateDirection );
	AddObserver( "set_direction", &CWorldClient::MsgSetDirection );
	AddObserver( "cancel_direction", &CWorldClient::MsgCancelDirection );
	AddObserver( "do_action", &CWorldClient::MsgDoAction );
	//
	AddObserver( "set_forced_action", &CWorldClient::MsgSetForcedAction );
	AddObserver( "game_reset_forced_action", &CWorldClient::MsgResetForcedAction );
	AddObserver( "set_special_ability", &CWorldClient::MsgSetSpecialAbility );
	//
//	AddObserver( "shift_down", MsgShiftDown  );
//	AddObserver( "shift_up", MsgShiftUp  );
	//
	AddObserver( "show_areas_down", &CWorldClient::MsgAreasDown  );
	AddObserver( "show_areas_up", &CWorldClient::MsgAreasUp  );
	AddObserver( "show_ranges_down", &CWorldClient::MsgRangesDown  );
	AddObserver( "show_ranges_up", &CWorldClient::MsgRangesUp  );
	AddObserver( "show_xray_down", MsgXRay, 1 );
	AddObserver( "show_xray_up", MsgXRay, 0 );
	//
	AddObserver( "unload_unit", &CWorldClient::MsgUnloadUnit );
	AddObserver( "select_units", &CWorldClient::MsgSelectUnits );
	AddObserver( "select_next_group", &CWorldClient::MsgSelectNextGroup );
	AddObserver( "select_prev_group", &CWorldClient::MsgSelectPrevGroup );
	//
	AddObserver( "update_selected_unit", &CWorldClient::MsgUpdateSelectedUnit );
	//
	AddObserver( "show_objects_profile", &CWorldClient::ToggleLockProfiles );

	AddObserver( "play_all_animations", &CWorldClient::PlayAllAnimations );

	AddObserver( "assign_selection_group_0", &CWorldClient::MsgAssignSelectionGroup, 0 );
	AddObserver( "assign_selection_group_1", &CWorldClient::MsgAssignSelectionGroup, 1 );
	AddObserver( "assign_selection_group_2", &CWorldClient::MsgAssignSelectionGroup, 2 );
	AddObserver( "assign_selection_group_3", &CWorldClient::MsgAssignSelectionGroup, 3 );
	AddObserver( "assign_selection_group_4", &CWorldClient::MsgAssignSelectionGroup, 4 );
	AddObserver( "assign_selection_group_5", &CWorldClient::MsgAssignSelectionGroup, 5 );
	AddObserver( "assign_selection_group_6", &CWorldClient::MsgAssignSelectionGroup, 6 );
	AddObserver( "assign_selection_group_7", &CWorldClient::MsgAssignSelectionGroup, 7 );
	AddObserver( "assign_selection_group_8", &CWorldClient::MsgAssignSelectionGroup, 8 );
	AddObserver( "assign_selection_group_9", &CWorldClient::MsgAssignSelectionGroup, 9 );
	AddObserver( "restore_selection_group_0", &CWorldClient::MsgRestoreSelectionGroup, 0 );
	AddObserver( "restore_selection_group_1", &CWorldClient::MsgRestoreSelectionGroup, 1 );
	AddObserver( "restore_selection_group_2", &CWorldClient::MsgRestoreSelectionGroup, 2 );
	AddObserver( "restore_selection_group_3", &CWorldClient::MsgRestoreSelectionGroup, 3 );
	AddObserver( "restore_selection_group_4", &CWorldClient::MsgRestoreSelectionGroup, 4 );
	AddObserver( "restore_selection_group_5", &CWorldClient::MsgRestoreSelectionGroup, 5 );
	AddObserver( "restore_selection_group_6", &CWorldClient::MsgRestoreSelectionGroup, 6 );
	AddObserver( "restore_selection_group_7", &CWorldClient::MsgRestoreSelectionGroup, 7 );
	AddObserver( "restore_selection_group_8", &CWorldClient::MsgRestoreSelectionGroup, 8 );
	AddObserver( "restore_selection_group_9", &CWorldClient::MsgRestoreSelectionGroup, 9 );
	AddObserver( "center_selection_group_0", &CWorldClient::MsgCenterSelectionGroup, 0 );
	AddObserver( "center_selection_group_1", &CWorldClient::MsgCenterSelectionGroup, 1 );
	AddObserver( "center_selection_group_2", &CWorldClient::MsgCenterSelectionGroup, 2 );
	AddObserver( "center_selection_group_3", &CWorldClient::MsgCenterSelectionGroup, 3 );
	AddObserver( "center_selection_group_4", &CWorldClient::MsgCenterSelectionGroup, 4 );
	AddObserver( "center_selection_group_5", &CWorldClient::MsgCenterSelectionGroup, 5 );
	AddObserver( "center_selection_group_6", &CWorldClient::MsgCenterSelectionGroup, 6 );
	AddObserver( "center_selection_group_7", &CWorldClient::MsgCenterSelectionGroup, 7 );
	AddObserver( "center_selection_group_8", &CWorldClient::MsgCenterSelectionGroup, 8 );
	AddObserver( "center_selection_group_9", &CWorldClient::MsgCenterSelectionGroup, 9 );

	AddObserver( "center_current_selection", &CWorldClient::MsgCenterCurrentSelection );

	RegisterUserAction( NDb::USER_ACTION_MOVE, SActionDesc::AUTO | SActionDesc::FORCED, &CWorldClient::ActionMove );
	RegisterUserAction( NDb::USER_ACTION_REVERSE, SActionDesc::FORCED, &CWorldClient::ActionReverse );
	RegisterUserAction( NDb::USER_ACTION_MOVE_TO_GRID, SActionDesc::FORCED, &CWorldClient::ActionMoveToGrid );
	RegisterUserAction( NDb::USER_ACTION_MOVE_LIKE_TERRAIN, SActionDesc::AUTO, &CWorldClient::ActionMove );
	RegisterUserAction( NDb::USER_ACTION_MOVE_TRACK, SActionDesc::AUTO, &CWorldClient::ActionMove );
	RegisterUserAction( NDb::USER_ACTION_MOVE_WHELL, SActionDesc::AUTO, &CWorldClient::ActionMove );
	RegisterUserAction( NDb::USER_ACTION_MOVE_HUMAN, SActionDesc::AUTO, &CWorldClient::ActionMove );
	RegisterUserAction( NDb::USER_ACTION_SWARM, SActionDesc::AUTO | SActionDesc::FORCED, &CWorldClient::ActionSwarm );
	RegisterUserAction( NDb::USER_ACTION_ROTATE, SActionDesc::FORCED, &CWorldClient::ActionRotate );
	RegisterUserAction( NDb::USER_ACTION_ATTACK, SActionDesc::AUTO | SActionDesc::FORCED, &CWorldClient::ActionAttack );
	RegisterUserAction( NDb::USER_ACTION_FOLLOW, SActionDesc::AUTO | SActionDesc::FORCED, &CWorldClient::ActionFollow );
	RegisterUserAction( NDb::USER_ACTION_STAND_GROUND, SActionDesc::INSTANT, &CWorldClient::ActionStandGround );

	RegisterUserAction( NDb::USER_ACTION_FORMATION_0, SActionDesc::INSTANT, &CWorldClient::ActionChangeFormation0 );
	RegisterUserAction( NDb::USER_ACTION_FORMATION_1, SActionDesc::INSTANT, &CWorldClient::ActionChangeFormation1 );
	RegisterUserAction( NDb::USER_ACTION_FORMATION_2, SActionDesc::INSTANT, &CWorldClient::ActionChangeFormation2 );
	RegisterUserAction( NDb::USER_ACTION_FORMATION_3, SActionDesc::INSTANT, &CWorldClient::ActionChangeFormation3 );
	RegisterUserAction( NDb::USER_ACTION_FORMATION_4, SActionDesc::INSTANT, &CWorldClient::ActionChangeFormation4 );

	RegisterUserAction( NDb::USER_ACTION_USE_SHELL_DAMAGE, SActionDesc::INSTANT, &CWorldClient::ActionChangeShellDamage );
	RegisterUserAction( NDb::USER_ACTION_USE_SHELL_AGIT, SActionDesc::INSTANT, &CWorldClient::ActionChangeShellAgit );
	RegisterUserAction( NDb::USER_ACTION_USE_SHELL_SMOKE, SActionDesc::INSTANT, &CWorldClient::ActionChangeShellSmoke );

	RegisterUserAction( NDb::USER_ACTION_BOARD, SActionDesc::AUTO | SActionDesc::FORCED, &CWorldClient::ActionBoard );
	RegisterUserAction( NDb::USER_ACTION_LEAVE, SActionDesc::AUTO | SActionDesc::FORCED, &CWorldClient::ActionLeave );
	RegisterUserAction( NDb::USER_ACTION_MECH_BOARD, SActionDesc::AUTO | SActionDesc::FORCED, &CWorldClient::ActionMechBoard );

	RegisterUserAction( NDb::USER_ACTION_INSTALL, SActionDesc::INSTANT, &CWorldClient::ActionInstall );
	RegisterUserAction( NDb::USER_ACTION_UNINSTALL, SActionDesc::INSTANT, &CWorldClient::ActionUnInstall );

	RegisterUserAction( NDb::USER_ACTION_CAPTURE_ARTILLERY, SActionDesc::AUTO, &CWorldClient::ActionCaptureArtillery );
	RegisterUserAction( NDb::USER_ACTION_HOOK_ARTILLERY, SActionDesc::AUTO | SActionDesc::FORCED, &CWorldClient::ActionHookArtillery );
	RegisterUserAction( NDb::USER_ACTION_DEPLOY_ARTILLERY, SActionDesc::AUTO | SActionDesc::FORCED, &CWorldClient::ActionDeployArtillery );

	RegisterUserAction( NDb::USER_ACTION_ENTRENCH_SELF, SActionDesc::AUTO | SActionDesc::INSTANT, &CWorldClient::ActionEntrenchSelf );
	RegisterUserAction( NDb::USER_ACTION_ENGINEER_PLACE_MINES, SActionDesc::FORCED, &CWorldClient::ActionPlaceMines );
	RegisterUserAction( NDb::USER_ACTION_ENGINEER_CLEAR_MINES, SActionDesc::FORCED, &CWorldClient::ActionClearMines );
	RegisterUserAction( NDb::USER_ACTION_ENGINEER_BUILD_ENTRENCHMENT, SActionDesc::FORCED, &CWorldClient::ActionBuildEntrenchment );
	RegisterUserAction( NDb::USER_ACTION_ENGINEER_BUILD_FENCE, SActionDesc::FORCED, &CWorldClient::ActionBuildFence );
	RegisterUserAction( NDb::USER_ACTION_ENGINEER_REPAIR, SActionDesc::AUTO | SActionDesc::FORCED, &CWorldClient::ActionRepair );

	RegisterUserAction( NDb::USER_ACTION_STOP, SActionDesc::INSTANT, &CWorldClient::ActionStop );

	RegisterUserAction( NDb::USER_ACTION_SUPPORT_RESUPPLY, SActionDesc::AUTO | SActionDesc::FORCED, &CWorldClient::ActionResupply );
// Special abilities
	RegisterUserAction( NDb::USER_ACTION_CAMOFLAGE, SActionDesc::INSTANT, &CWorldClient::ActionCamoflage );
	RegisterUserAction( NDb::USER_ACTION_ADVANCED_CAMOFLAGE, SActionDesc::INSTANT, &CWorldClient::ActionAdvancedCamoflage );
	RegisterUserAction( NDb::USER_ACTION_AMBUSH, SActionDesc::INSTANT, &CWorldClient::ActionAmbush );
	RegisterUserAction( NDb::USER_ACTION_SPYGLASS, SActionDesc::FORCED, &CWorldClient::ActionUseSpyGlass );
	RegisterUserAction( NDb::USER_ACTION_THROW, SActionDesc::FORCED, &CWorldClient::ActionUseThrow );
//	RegisterUserAction( NDb::USER_ACTION_LAND_MINE, SActionDesc::FORCED, &CWorldClient::ActionUseLandMine );
	RegisterUserAction( NDb::USER_ACTION_BLASTING_CHARGE, SActionDesc::FORCED, &CWorldClient::ActionUseBlastingCharge );
	RegisterUserAction( NDb::USER_ACTION_CONTROLLED_CHARGE, SActionDesc::FORCED, &CWorldClient::ActionUseControlledCharge );
	RegisterUserAction( NDb::USER_ACTION_DETONATE, SActionDesc::INSTANT, &CWorldClient::ActionDetonateControlledCharge );
	RegisterUserAction( NDb::USER_ACTION_HOLD_SECTOR, SActionDesc::FORCED, &CWorldClient::ActionUseHoldSector );
	RegisterUserAction( NDb::USER_ACTION_TRACK_TARGETING, SActionDesc::FORCED, &CWorldClient::ActionUseTrackTargeting );
	RegisterUserAction( NDb::USER_ACTION_SUPPRESS, SActionDesc::FORCED, &CWorldClient::ActionSupressFire );
	RegisterUserAction( NDb::USER_ACTION_FIRE_ROCKETS, SActionDesc::FORCED, &CWorldClient::ActionFireRockets );
	RegisterUserAction( NDb::USER_ACTION_USE_FLAMETHROWER, SActionDesc::FORCED, &CWorldClient::ActionUseFlamethrower );

	RegisterUserAction( NDb::USER_ACTION_CRITICAL_TARGETTING, SActionDesc::INSTANT, &CWorldClient::ActionCriticalTargetting );
	RegisterUserAction( NDb::USER_ACTION_RAPID_FIRE, SActionDesc::INSTANT, &CWorldClient::ActionRapidFire );
	RegisterUserAction( NDb::USER_ACTION_CAUTION, SActionDesc::INSTANT, &CWorldClient::ActionCaution );
	RegisterUserAction( NDb::USER_ACTION_MOVING_SHOOT, SActionDesc::INSTANT, &CWorldClient::ActionShootInMovement );
	RegisterUserAction( NDb::USER_ACTION_COUNTER_FIRE, SActionDesc::FORCED, &CWorldClient::ActionCounterFire );

	RegisterUserAction( NDb::USER_ACTION_DROP_BOMB, SActionDesc::FORCED, &CWorldClient::ActionDropBombs );
	RegisterUserAction( NDb::USER_ACTION_EXACT_SHOT, SActionDesc::FORCED, &CWorldClient::ActionExactShot );
	RegisterUserAction( NDb::USER_ACTION_COVER_FIRE, SActionDesc::FORCED, &CWorldClient::ActionCoverFire );
	RegisterUserAction( NDb::USER_ACTION_FIRST_AID, SActionDesc::FORCED, &CWorldClient::ActionFirstAid );
	RegisterUserAction( NDb::USER_ACTION_FILL_RU, SActionDesc::AUTO | SActionDesc::FORCED, &CWorldClient::ActionFillRU );

	RegisterUserAction( NDb::USER_ACTION_GLOBE_BOMB_MISSION, SActionDesc::FORCED, &CWorldClient::ActionGlobeBombMission );

	RegisterUserAction( NDb::USER_ACTION_SMOKE_SHOTS, SActionDesc::INSTANT, &CWorldClient::ActionSmokeShots );
	RegisterUserAction( NDb::USER_ACTION_LINKED_GRENADES, SActionDesc::INSTANT, &CWorldClient::ActionLinkedGrenades );
	RegisterUserAction( NDb::USER_ACTION_ZEROING_IN, SActionDesc::FORCED, &CWorldClient::ActionZeroingIn );
	RegisterUserAction( NDb::USER_ACTION_SUPPORT_FIRE, SActionDesc::AUTO | SActionDesc::FORCED, &CWorldClient::ActionSupportFire );
	RegisterUserAction( NDb::USER_ACTION_PATROL, SActionDesc::FORCED, &CWorldClient::ActionPatrol );
	RegisterUserAction( NDb::USER_ACTION_SPY_MODE, SActionDesc::FORCED, &CWorldClient::ActionSpyMode );
	RegisterUserAction( NDb::USER_ACTION_KAMIKAZE, SActionDesc::INSTANT, &CWorldClient::ActionOverload );

	RegisterUserAction( NDb::USER_ACTION_REINF_COMMON, SActionDesc::FORCED, &CWorldClient::ActionReinfCommon );
	RegisterUserAction( NDb::USER_ACTION_REINF_BOMB, SActionDesc::FORCED, &CWorldClient::ActionReinfBomb );
	RegisterUserAction( NDb::USER_ACTION_REINF_PARATROOPERS, SActionDesc::FORCED, &CWorldClient::ActionReinfParatroopers );
	RegisterUserAction( NDb::USER_ACTION_REINF_NONE, SActionDesc::FORCED, &CWorldClient::ActionReinfNone );

	RegisterUserAction( NDb::USER_ACTION_SUPER_WEAPON_MODE, SActionDesc::FORCED, &CWorldClient::ActionCallSuperWeapon );
	RegisterUserAction( NDb::USER_ACTION_PLACE_MARKER, SActionDesc::FORCED, &CWorldClient::ActionPlaceMarker );

	pSelector = new CSelector();
	if ( NGlobal::GetVar( "History.Playing", 0 ) )
		pMouseTranslator = new CMouseTranslatorB2Replay( pSelector );
	else
		pMouseTranslator = new CMouseTranslatorB2Game( pSelector );

//	bPlaceCommandInQuery = false;

	eForcedAction = NDb::USER_ACTION_UNKNOWN;
	eCurrentAbilityParam = NDb::PARAM_ABILITY_OFF;
	vFirstCommandPoint = CVec2( -1.0f, -1.0f );

	bActive = true;
	ResetFirstCommandPoint();

	eBuildObjectState = EBS_NONE;

	bAreasShown = false;
	bRangesShown = false;
	
	bCameraUpdated = false;
	
	bOnMinimap = false;
	bOnMinimapOff = false;
	
	bUISelectionMode = false;
	bUISelectionDir = false;
	
	bIsOwnUnitsPresent = false;
	
	xrayUnits.resize( MAX_PLAYERS );
	timeAbs = 0;
}

void CWorldClient::Update()
{
	NTimer::STime timeCurrAbs = Singleton<IGameTimer>()->GetAbsTime();
	int nDeltaTime = (timeAbs == 0) ? 0 : timeCurrAbs - timeAbs;
	timeAbs = timeCurrAbs;

	CUpdatableWorld::Update();

	mapCommandAck.Update();
	mapCommandAckDir.Update();
	pSelector->Segment();

	UpdateXRayUnits( nDeltaTime );
}

bool CWorldClient::ProcessEvent( const struct SGameMessage &msg )
{
	if ( CGMORegContainer::ProcessEvent( msg, this ) == false )
		return pMouseTranslator->ProcessEvent( msg );
	else
		return true;
}

// CRAP
void CWorldClient::MsgUserActionAttack( const SGameMessage &msg )
{
	NInput::PostEvent( "set_forced_action", NDb::USER_ACTION_ATTACK, 0 );
}

void CWorldClient::MsgUserActionRotate( const SGameMessage &msg )
{
	NInput::PostEvent( "set_forced_action", NDb::USER_ACTION_ROTATE, 0 );
}

void CWorldClient::MsgUserActionStop( const SGameMessage &msg )
{
	NInput::PostEvent( "set_forced_action", NDb::USER_ACTION_STOP, 0 );
}

void CWorldClient::LoadMap( const NDb::SMapInfo *_pMapInfo )
{
	pMapInfo = _pMapInfo;
	SetSeason( pMapInfo->eSeason, pMapInfo->eDayTime );
	Cursor()->Show( true );
	{
		const NDb::SCameraLimits *pClient = NGameX::GetClientConsts()->pCamera;
		const NCamera::SCameraLimits pitchLimit = NCamera::SCameraLimits( pClient->pitchLimit.fMin, pClient->pitchLimit.fMax, pClient->pitchLimit.fAve, 
																																			pClient->pitchLimit.fAutoSpeed, pClient->pitchLimit.fManualSpeed, pClient->pitchLimit.bCyclic );
																																			Camera()->SetLimits( NCamera::CAMERA_LIMITS_PITCH, pitchLimit );

		const NCamera::SCameraLimits yawLimit = NCamera::SCameraLimits( pClient->yawLimit.fMin, pClient->yawLimit.fMax, pClient->yawLimit.fAve, 
																																		pClient->yawLimit.fAutoSpeed, pClient->yawLimit.fManualSpeed, pClient->yawLimit.bCyclic );
																																		Camera()->SetLimits( NCamera::CAMERA_LIMITS_YAW, yawLimit );

		const NCamera::SCameraLimits distanceLimit = NCamera::SCameraLimits( pClient->distanceLimit.fMin, pClient->distanceLimit.fMax, pClient->distanceLimit.fAve, 
																																				 pClient->distanceLimit.fAutoSpeed, pClient->distanceLimit.fManualSpeed, pClient->distanceLimit.bCyclic );
																																				 Camera()->SetLimits( NCamera::CAMERA_LIMITS_DISTANCE, distanceLimit );
	}

	//
	IScene *pScene = Scene();
	pScene->SetLight( pMapInfo->pLight );
	// setup terrain
	ITerraManager *pTerraManager = pScene->GetTerraManager();
	//const string szDataPath;// = Singleton<IMainLoop>()->GetBaseDir() + "Data\\";
	//pTerrain->SetStreamPathes( szDataPath, szDataPath );
	const int nSizeX = pMapInfo->nNumPatchesX * AI_TILES_IN_PATCH;
	const int nSizeY = pMapInfo->nNumPatchesY * AI_TILES_IN_PATCH;
	pTerraManager->SetAIObserver( pAI->CreateTerraAIObserver( nSizeX, nSizeY ) );
	const std::string szMapFilePath = NDb::GetFolderName( pMapInfo->GetDBID() );
	NScene::LoadTerrain( pTerraManager, pMapInfo, szMapFilePath );

	ISoundScene *pSoundScene = Singleton<ISoundScene>();
	pSoundScene->SetSoundSceneMode( ESSM_INGAME );
	pSoundScene->Init( nSizeX * AI_TILE_SIZE, nSizeY * AI_TILE_SIZE,
										 0, 1, // listener is always on ground level
										 AI_TILE_SIZE );

	//if ( !pMapInfo->players.empty() ) 
	//{
	//	if ( pMapInfo->players[0].vCameraAnchor == VNULL3 && pMapInfo->players[0].camera.vAnchor != VNULL3 )
	//		Camera()->SetAnchor( pMapInfo->players[0].camera.vAnchor );
	//	else
	//		Camera()->SetAnchor( pMapInfo->players[0].vCameraAnchor );
	//	if ( !pMapInfo->players[0].camera.bUseAnchorOnly )
	//		Camera()->SetPlacement( pMapInfo->players[0].camera.fDist, pMapInfo->players[0].camera.fPitch, pMapInfo->players[0].camera.fYaw );
	//}
	//else
	//	Camera()->SetAnchor( CVec3(10, 10, 0) );

	vMapSize.x = pMapInfo->nNumPatchesX * AI_TILES_IN_PATCH * AI_TILE_SIZE;
	vMapSize.y = pMapInfo->nNumPatchesY * AI_TILES_IN_PATCH * AI_TILE_SIZE;

	Camera()->SetAnchorLimits( CTRect<float>( 0.0f, 0.0f, 
																						pMapInfo->nNumPatchesX * VIS_TILES_IN_PATCH * VIS_TILE_SIZE, 
																						pMapInfo->nNumPatchesY * VIS_TILES_IN_PATCH * VIS_TILE_SIZE ) );

	Camera()->SetScriptMutatorsHolder( 0 );
}

void CWorldClient::AfterLoad( const NDb::SMapInfo *pMapInfo )
{
	CUpdatableWorld::AfterLoad();
	
	Cursor()->Show( true );
	//Scene()->SetLight( pMapInfo->pLight );
//	const string szDataPath = Singleton<IMainLoop>()->GetBaseDir() + "Data\\";
//	pTerrain->SetStreamPathes( szDataPath, szDataPath );
//	pTerrain->Load( pMapInfo );

	pSelector->AfterLoad();
}

void CWorldClient::OnGetFocus( bool bFocus )
{
	pMouseTranslator->OnGetFocus( bFocus );
//	if ( bFocus ) 
//		RegisterObservers();
//	else
//		UnRegisterObservers();
}

void CWorldClient::Select( CMapObj *pMapObj )
{
	if ( pSelector->CanSelect( pMapObj ) )
		if ( pSelector->Select( checked_cast<CMOSelectable*>(pMapObj), true ) )
  		pSelector->UpdateSelection( true, false );
}

void CWorldClient::DeSelect( CMapObj *pMapObj )
{
	if ( pSelector->CanSelect( pMapObj ) )
	{
		if ( pSelector->Select( checked_cast<CMOSelectable*>(pMapObj), false ) )
			pSelector->UpdateSelection( true, false );
		if ( pSelector->IsEmpty() && eActionMode == EAM_SELECT )
			SetForcedAction( NDb::USER_ACTION_UNKNOWN );
	}
}

void CWorldClient::DeSelectDead( CMapObj *pMapObj )
{
	if ( pSelector->DeSelectDead( pMapObj ) )
	{
		pSelector->UpdateSelection( true, true );
		if ( pSelector->IsEmpty() && eActionMode == EAM_SELECT )
			SetForcedAction( NDb::USER_ACTION_UNKNOWN );
	}
	else
	{
		if ( pSelector->IsSelectedOrInSelectedContainer( pMapObj ) )
			pSelector->DoUpdateSelectedUnits();
		if ( pSelector->IsPreselectedOrInPreselectedContainer( pMapObj ) )
			pSelector->DoUpdatePreselectedUnits();
	}
}

bool CWorldClient::IsActive( CMapObj *pMapObj )
{
	return pSelector->CanSelect( pMapObj ) ? pSelector->IsActive( checked_cast<CMOSelectable*>(pMapObj) ) : false;
}

bool CWorldClient::IsSuperActive( CMapObj *pMapObj )
{
	return pSelector->CanSelect( pMapObj ) ? pSelector->IsSuperActive( checked_cast<CMOSelectable*>(pMapObj) ) : false;
}

void CWorldClient::DoUpdateSpecialAbility( CMapObj *pMO )
{
	pSelector->DoUpdateSpecialAbility( pMO ); // обновить доступнvе абилити можно только через селектор
}

void CWorldClient::DoUpdateObjectStats( CMapObj *pMO )
{
	pSelector->DoUpdateStats( pMO ); // обновить статv объекта можно только через селектор
	if ( IsSuperWeapon( pMO ) )
		NInput::PostEvent( "mission_update_super_weapon_stats", pMO->GetID(), 0 );
}

void CWorldClient::RemoveFromSelectionGroup( CMapObj *pMapObj )
{
	if ( pSelector->CanSelect( pMapObj ) )
		pSelector->RemoveFromGroups( checked_cast<CMOSelectable*>(pMapObj) );
}

void CWorldClient::HideFromSelectionGroup( CMapObj *pMapObj )
{
	if ( pSelector->CanSelect( pMapObj ) )
		pSelector->HideFromGroups( checked_cast<CMOSelectable*>(pMapObj) );
}

void CWorldClient::UnHideFromSelectionGroup( CMapObj *pMapObj )
{
	if ( pSelector->CanSelect( pMapObj ) )
		pSelector->UnHideForGroups( checked_cast<CMOSelectable*>(pMapObj) );
}

void CWorldClient::RegisterUserAction( NDb::EUserAction nAction, DWORD flags, USER_ACTION pfnUserAction )
{
	NI_ASSERT( (flags != 0) && (pfnUserAction), StrFmt("Can't register action %d with NULL functions and/or flags", nAction) );
	SActionDesc &action = userActionsMap[nAction];
	action.flags = flags;
	action.pfnAction = pfnUserAction;
}

NDb::EUserAction CWorldClient::DetermineBestAutoAction( CMapObj *pMO, bool bAltMode )
{
	if ( !pMO ) 
	{
		if ( bAltMode )
			return NDb::USER_ACTION_MOVE;
		return NDb::USER_ACTION_UNKNOWN;
	}
	
	CUserActions actionsBy;
	pSelector->GetEnabledActions( &actionsBy );

	pSelector->FilterActions( &actionsBy, pMO );
	
	if ( pSelector->IsSuperActive( dynamic_cast<CMOSelectable*>( pMO ) ) )
		return pMO->GetBestSelfAction( actionsBy, bAltMode );
	else
		return pMO->GetBestAutoAction( actionsBy, bAltMode );
}

NDb::EUserAction CWorldClient::DetermineBestCtrlAction( CMapObj *pMO )
{
	CUserActions actions;
	pSelector->GetEnabledSuperActiveActions( &actions );
	if ( actions.HasAction( NDb::USER_ACTION_SUPPRESS ) )
		return NDb::USER_ACTION_SUPPRESS;

	CUserActions actionsBy;
	pSelector->GetEnabledActions( &actionsBy );

	if ( pMO )
	{
		pSelector->FilterActions( &actionsBy, pMO );
		if ( actionsBy.HasAction( NDb::USER_ACTION_ATTACK ) )
			return NDb::USER_ACTION_ATTACK;
	}
	else
	{
		if ( actionsBy.HasAction( NDb::USER_ACTION_SWARM ) )
			return NDb::USER_ACTION_SWARM;
	}

	if ( actionsBy.HasAction( NDb::USER_ACTION_MOVE ) )
		return NDb::USER_ACTION_MOVE;

	return NDb::USER_ACTION_UNKNOWN;
}

void CWorldClient::SetCursor( NDb::EUserAction _eAction )
{
	NDb::EUserAction eCursor = _eAction;
	if ( !bActive )
	{
		if ( eActionMode == EAM_REINF )
		{
			if ( !IsOnMinimap() )
				eCursor = NDb::USER_ACTION_UNKNOWN;
		}
		else
		{
			eCursor = NDb::USER_ACTION_UNKNOWN;
		}
	}
	Cursor()->SetMode( eCursor );
}

void CWorldClient::SetForcedAction( NDb::EUserAction _eForcedAction )
{
	NI_ASSERT( _eForcedAction == NDb::USER_ACTION_UNKNOWN || GetAction(_eForcedAction, SActionDesc::FORCED) != 0, StrFmt("Can't set user action %d as forced - no execution function", _eForcedAction) );
	NDb::EUserAction eOldForcedAction = eForcedAction;
	if ( _eForcedAction != NDb::USER_ACTION_UNKNOWN && GetAction(_eForcedAction, SActionDesc::FORCED) == 0 ) 
		eForcedAction = NDb::USER_ACTION_UNKNOWN;
	else
		eForcedAction = _eForcedAction;
	if ( eOldForcedAction != eForcedAction )
		OnChangeForcedAction( eOldForcedAction );
	ResetFirstCommandPoint();
	//If starting to build, send command requesting "ghost objects"
	switch ( eForcedAction ) 
	{
		case NDb::USER_ACTION_LAND_MINE:
			{
				eBuildObjectState = EBS_STARTED;
			}
			break;
		default:
			eBuildObjectState = EBS_NONE;
	}
	SetCursor( GetForcedAction() );
}

void CWorldClient::MsgSetForcedAction( const SGameMessage &msg )
{
	if ( GetForcedAction() != static_cast<NDb::EUserAction>( msg.nParam1 ) )
	{
		USER_ACTION pfnAction = GetAction( static_cast<NDb::EUserAction>( msg.nParam1 ), SActionDesc::INSTANT );
		if ( pfnAction )
		{
			if ( !(this->*pfnAction)( VNULL2, 0, true ) )
			{
				// INSTANT action cannot be done ?
			}
			SetForcedAction( NDb::USER_ACTION_UNKNOWN );
		}
		else SetForcedAction( static_cast<NDb::EUserAction>( msg.nParam1 ) );
	}
}

void CWorldClient::MsgSetForcedActionWithParam( const SGameMessage &msg, CObjectBase *pParam )
{
	pForcedActionParam = pParam;
	MsgSetForcedAction( msg );
}

void CWorldClient::MsgResetForcedAction( const SGameMessage &msg )
{
	GameResetForcedAction();
}

void CWorldClient::GameResetForcedAction()
{
	SetForcedAction( NDb::USER_ACTION_UNKNOWN );
}

void CWorldClient::OnChangeForcedAction( NDb::EUserAction eOldForcedAction )
{
	if ( eBuildObjectState != EBS_NONE )		//Were building something
	{
		switch ( eOldForcedAction )
		{
		case NDb::USER_ACTION_ENGINEER_BUILD_ENTRENCHMENT:
			{
				SAIUnitCmd cmd( ACTION_COMMAND_ENTRENCH_END );
				cmd.nNumber = -1;		//Cancel
				PerformGroupAction( &cmd, false );
				pAI->RequestBuildPreview( ACTION_COMMAND_ENTRENCH_END, VNULL2, VNULL2, true );
			}
			break;
		case NDb::USER_ACTION_ENGINEER_BUILD_FENCE:
			{
				SAIUnitCmd cmd( ACTION_COMMAND_BUILD_FENCE_END );
				cmd.nNumber = -1;		//Cancel
				PerformGroupAction( &cmd, false );
				pAI->RequestBuildPreview( ACTION_COMMAND_BUILD_FENCE_END, VNULL2, VNULL2, true );
			}
			break;
		case NDb::USER_ACTION_ENGINEER_PLACE_MINES:
			{
				SAIUnitCmd cmd( ACTION_COMMAND_PLACEMINE );
				cmd.nNumber = -1;		//Cancel
				PerformGroupAction( &cmd, false );
				pAI->RequestBuildPreview( ACTION_COMMAND_PLACEMINE, VNULL2, VNULL2, true );
			}
			break;
		}
	}
}

void CWorldClient::MsgSetSpecialAbility( const SGameMessage &msg )
{
	eCurrentAbilityParam = (NDb::ESpecialAbilityParam)msg.nParam2;
	switch ( eCurrentAbilityParam )
	{
	case NDb::PARAM_ABILITY_ON:
		{
			USER_ACTION pfnAction = GetAction( static_cast<NDb::EUserAction>( msg.nParam1 ), SActionDesc::INSTANT );
			if ( pfnAction && (this->*pfnAction)( VNULL2, 0, true ) )
				SetForcedAction( NDb::USER_ACTION_UNKNOWN );
			else
			{
				USER_ACTION pfnAction = GetAction( static_cast<NDb::EUserAction>( msg.nParam1 ), SActionDesc::FORCED );
				if ( pfnAction && (this->*pfnAction)( VNULL2, 0, true ) )
					SetForcedAction( static_cast<NDb::EUserAction>( msg.nParam1 ) );
			}
		}
		break;
	case NDb::PARAM_ABILITY_OFF:
		{
			USER_ACTION pfnAction = GetAction( static_cast<NDb::EUserAction>( msg.nParam1 ), SActionDesc::INSTANT | SActionDesc::FORCED );
			if ( pfnAction && (this->*pfnAction)( VNULL2, 0, true ) )
				SetForcedAction( NDb::USER_ACTION_UNKNOWN );
			else
				SetForcedAction( static_cast<NDb::EUserAction>( msg.nParam1 ) );
		}
		break;
	case NDb::PARAM_ABILITY_AUTOCAST_ON:
	case NDb::PARAM_ABILITY_AUTOCAST_OFF:
		{
			NDb::EUserAction eAction = NDb::EUserAction( msg.nParam1 );
			SAIUnitCmd command( ( eAction == NDb::USER_ACTION_THROW ) ? ACTION_COMMAND_THROW_GRENADE : GetCommandByAction( eAction ) );
			//Hack: two THROW commands...
			SAbilitySwitchState state( EASS_CHANGE_AUTOCAST_ONLY );
			state.bAutocast = (eCurrentAbilityParam == NDb::PARAM_ABILITY_AUTOCAST_ON);
			command.nNumber = state.dwStateValue;
			command.fNumber = eCurrentAbilityParam;
			PerformGroupActionAutocast( &command, GetPlaceInQueue() );
		}
		break;
	default:
		break;
	}
}

CWorldClient::USER_ACTION CWorldClient::GetAction( NDb::EUserAction eUserAction, int nType )
{
	NI_ASSERT( eUserAction != NDb::USER_ACTION_UNKNOWN, "Illegal user action" );
	CActionsMap::const_iterator pos = userActionsMap.find( eUserAction );
	if ( pos == userActionsMap.end() ) return 0;
	return pos->second.flags & nType ? pos->second.pfnAction : 0;
}

// ************************************************************************************************************************ //
// **
// ** DoAction
// **
// **
// **
// ************************************************************************************************************************ //

void CWorldClient::DoAction( const CVec2 &vPos, bool bMiniMap, enum EKeyboardFlags eFlags )
{
	USER_ACTION pfnAction = 0;
	NDb::EUserAction eUserAction;

	bool bSelfAction = false;
	bool bLikeForced = false;
	if ( IsForcedAction() ) 
	{
		eUserAction = GetForcedAction();
		pfnAction = GetAction( eUserAction, SActionDesc::FORCED );
		NI_ASSERT( pfnAction != 0, StrFmt( "Forced action (%d) not registred", GetForcedAction() ) );
	}
	else
	{
		CMapObj *pMO = ( PickTopMapObj( vPos ) );
		if ( (eFlags & EKF_CTRL) != 0 )
		{
			eUserAction = DetermineBestCtrlAction( pMO );
			bLikeForced = true;
		}
		else
			eUserAction = DetermineBestAutoAction( pMO, (eFlags & EKF_ALT) != 0 );
		if ( eUserAction == NDb::USER_ACTION_UNKNOWN )
		{
			if ( pMO && pMO->GetTypeID() == NDb::SObjectRPGStats::typeID )
				eUserAction = NDb::USER_ACTION_MOVE;
			else if ( pMO && pMO->GetTypeID() == NDb::SBuildingRPGStats::typeID )
				eUserAction = NDb::USER_ACTION_MOVE;
			else
				return;
		}
		pfnAction = GetAction( eUserAction, (eFlags & EKF_CTRL) != 0 ? SActionDesc::FORCED : SActionDesc::AUTO );
		NI_ASSERT( pfnAction != 0, StrFmt( "Auto action (%d) not registred", eUserAction ) );
		bSelfAction = pSelector->IsSelected( pMO );
	}

	if ( pfnAction != 0 ) 
	{
		const CMapObj *pMO = (bMiniMap || eUserAction == NDb::USER_ACTION_MOVE || eUserAction == NDb::USER_ACTION_REVERSE || eUserAction == NDb::USER_ACTION_SUPPRESS || eUserAction == NDb::USER_ACTION_FIRE_ROCKETS || eUserAction == NDb::USER_ACTION_USE_FLAMETHROWER) ?
											0 : PickTopMapObj( vPos, eUserAction );
		CVec2 vTarget;
		if ( bMiniMap )
			vTarget = vPos;
		else
			ScreenToAI( &vTarget, vPos );

		DoAction( pfnAction, eUserAction, vTarget, pMO, bSelfAction, (eFlags & EKF_SHIFT) != 0, 
			IsForcedAction() || bLikeForced );
	}
}

void CWorldClient::DoAction( USER_ACTION pfnAction, NDb::EUserAction eUserAction, const CVec2 &vTarget, 
	const CMapObj *pMO, bool bSelfAction, bool bShiftState, bool bLikeForced )
{
	if ( (this->*pfnAction)( vTarget, pMO, bLikeForced ) )
	{
		if ( !pMO )
		{
			if ( !bSelfAction )
				PlaceMapCommandAck( vTarget );
		}
		else
		{
			if ( pMO->IsPlaceMapCommandAck( eUserAction ) )
				PlaceMapCommandAck( vTarget );
		}
		if ( IsForcedAction() ) 
		{
			if ( !bShiftState )
				NInput::PostEvent( "new_reset_forced_action", 0, 0 );
			else
				NInput::PostEvent( "notify_forced_action", 0, 0 );
		}
		if ( pMO == 0 && (eUserAction == NDb::USER_ACTION_MOVE || eUserAction == NDb::USER_ACTION_REVERSE || eUserAction == NDb::USER_ACTION_ATTACK) )
		{
			ShowSelectionDst( vTarget );
		}
	}
	else
	{
		const NDb::SComplexSoundDesc *pSound = InterfaceState()->GetSoundEntry( "SOUND_CANT_PERFORM_COMMAND" );
		if ( pSound )
			SoundScene()->AddSound( pSound, VNULL3, SFX_INTERFACE, SAM_ADD_N_FORGET, 0, 1.0f );
	}
}

void CWorldClient::ShowSelectionDst( const CVec2 &vTarget )
{
	std::vector<CMOSelectable*> selection;
	pSelector->GetSelection( &selection );

	SUISelection uiSelection;
	uiSelection.CalcCurrentState( selection );
	uiSelection.CalcTargetState( vTarget, vTarget, vTarget );

	for ( int i = 0; i < uiSelection.objects.size(); ++i )
	{
		SUISelection::SObject &object = uiSelection.objects[i];
		int nResult = Scene()->AddSelection( -1, CVec3( object.vMovePos.x, object.vMovePos.y, 0.0f ), 
			object.pMO->GetSelectionScale(), NDb::SELECTION_TYPE_GROUND, SELECTION_FADE_IN_TIME, -1.0f );

		uiVisSelections.push_back( nResult );
	}
}

void CWorldClient::ShowSelectionDst( SUISelection *pUISelection, const CVec2 &vMovePos, 
	const CVec2 &vStartDirPos, const CVec2 &vFinishDirPos, bool bFadeOut )
{
	std::vector<CMOSelectable*> selection;
	pSelector->GetSelection( &selection );

	pUISelection->CalcCurrentState( selection );
	pUISelection->CalcTargetState( vMovePos, vStartDirPos, vFinishDirPos );

	for ( int i = 0; i < pUISelection->objects.size(); ++i )
	{
		SUISelection::SObject &object = pUISelection->objects[i];

		int nResult = Scene()->AddSelection( -1, CVec3( object.vMovePos.x, object.vMovePos.y, 0.0f ), 
			object.pMO->GetSelectionScale(), NDb::SELECTION_TYPE_GROUND, 0.0f, bFadeOut ? SELECTION_FADE_OUT_TIME : -1.0f );
		uiVisSelections.push_back( nResult );
	}
}

void CWorldClient::ClearSelectionDst()
{
	for ( int i = 0; i < uiVisSelections.size(); ++i )
	{
		Scene()->RemoveSelection( uiVisSelections[i] );
	}
	uiVisSelections.clear();
}

void CWorldClient::GotoSelectionDst( SUISelection *pUISelection )
{
	for ( int i = 0; i < pUISelection->objects.size(); ++i )
	{
		SUISelection::SObject &object = pUISelection->objects[i];

		{
		std::vector<int> buffer;
		buffer.push_back( object.pMO->GetID() );

		SAIUnitCmd command( ACTION_COMMAND_MOVE_TO );
		command.vPos = object.vMovePos;
		const WORD wAIGroup = pCommandsSender->CommandRegisterGroup( buffer );
		pCommandsSender->CommandGroupCommand( &command, wAIGroup, GetPlaceInQueue(), ML_COMMAND_SAVE_GAME );
		pCommandsSender->CommandUnregisterGroup( wAIGroup );
		}

		if ( !pCommandsSender->LastCommandSkipped() )
		{
		std::vector<int> buffer;
		buffer.push_back( object.pMO->GetID() );

		SAIUnitCmd command( ACTION_COMMAND_ROTATE_TO );
		command.vPos = object.vRotatePos;
		const WORD wAIGroup = pCommandsSender->CommandRegisterGroup( buffer );
		pCommandsSender->CommandGroupCommand( &command, wAIGroup, true, ML_COMMAND_SAVE_GAME );
		pCommandsSender->CommandUnregisterGroup( wAIGroup );
		}
	}
}

bool CWorldClient::IsInsideAIMap( const CVec2 &vPos )
{
	return (vPos.x >= 0.0f) && (vPos.x < vMapSize.x) && (vPos.y >= 0.0f) && (vPos.y < vMapSize.y);
}

void CWorldClient::ClampToAIMap( CVec2 &vPos )
{
	if ( vPos.x < 0.0f )
		vPos.x = 0.0f;
	else if ( vPos.x > vMapSize.x )
		vPos.x = vMapSize.x;

	if ( vPos.y < 0.0f )
		vPos.y = 0.0f;
	else if ( vPos.y > vMapSize.y )
		vPos.y = vMapSize.y;
}

// ************************************************************************************************************************ //
// **
// ** messages from mouse translator
// **
// ** 
// ** 
// **
// ************************************************************************************************************************ //

void CWorldClient::MsgStartSelection( const SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	const CVec2 vPos = UnPackCoords( msg.nParam1 );
	vSelectionFirstPoint = vPos;
}

void CWorldClient::MsgUpdateSelection( const SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	const CVec2 vPos = UnPackCoords( msg.nParam1 );
	vSelectionLastPoint = vPos;
}

void CWorldClient::MsgEndSelection( const SGameMessage &msg )
{
	if ( msg.nParam1 & SA_SELECT_BY_RECT )
	{
		std::list<int> ids;
		std::list<CMapObj*> objects;
		int nCurrentSelectionRate = 0;
		CVec2 vSelectionMin, vSelectionMax;
		BuildScreenRect( &vSelectionMin, &vSelectionMax, vSelectionFirstPoint, vSelectionLastPoint );
		const CVec2 vScreenRect = Scene()->GetScreenRect();
		const CTransformStack &cameraTransform = Camera()->GetTransform();
		Scene()->PickObjects( ids, vSelectionFirstPoint, vSelectionLastPoint, IScene::PO_CENTER_INSIDE );
		for ( std::list<int>::iterator it = ids.begin(); it != ids.end(); ++it )
		{
			CMapObj *pMO = GetMapObj( *it );
			if ( pMO && pSelector->CanSelect( pMO ) )
			{
				const bool bOverlapsSelectionRect = DoesMapObjectOverlapScreenRect( pMO, vSelectionMin, vSelectionMax, cameraTransform, vScreenRect );
				if ( !bOverlapsSelectionRect )
					continue;

				const int nSelectionRate = GetObjectRate( *pMO );
				if ( nSelectionRate > nCurrentSelectionRate )
				{
					objects.clear();
					objects.push_back( pMO );
					nCurrentSelectionRate = nSelectionRate;
				}
				else if ( nSelectionRate == 6 ) // 6 is friendly unit
				{
					objects.push_back( pMO );
				}
			}
		}

		if ( (msg.nParam1 & SA_PREVIOS_SELECTION) == SA_CLEAR_ALWAYS )
			pSelector->Empty();

		if ( objects.empty() )
			return;

		if ( (msg.nParam1 & SA_PREVIOS_SELECTION) == SA_CLEAR_IF_NEW_NOT_EMPTY )
			pSelector->Empty();

		for ( std::list<CMapObj*>::iterator it = objects.begin(); it != objects.end(); ++it )
		{
			CMOSelectable *pMOSel = checked_cast< CMOSelectable * >( *it );
			pSelector->Select( pMOSel, true );
		}
		pSelector->DoneSelection( true, false );
	}
	else
	{
		CMapObj *pMO = PickTopMapObj( vSelectionFirstPoint );
		
		if ( (msg.nParam1 & SA_PREVIOS_SELECTION) == SA_CLEAR_ALWAYS )
			pSelector->Empty();

		if ( ( pMO == 0 ) || ( !pSelector->CanSelect( pMO ) ) )
		{
			const NDb::SComplexSoundDesc *pSound = InterfaceState()->GetSoundEntry( "SOUND_CANT_PERFORM_COMMAND" );
			if ( pSound )
				SoundScene()->AddSound( pSound, VNULL3, SFX_INTERFACE, SAM_ADD_N_FORGET, 0, 1.0f );
			return;
		}

		if ( (msg.nParam1 & SA_PREVIOS_SELECTION) == SA_CLEAR_IF_NEW_NOT_EMPTY )
			pSelector->Empty();

		CMOSelectable *pMOSel = checked_cast< CMOSelectable * >( pMO );
		if ( ( msg.nParam1 & SA_PREVIOS_SELECTION ) == SA_INVERSE )
		{
			pSelector->Select( pMOSel, !pSelector->IsSelected( pMOSel ) );
      pSelector->DoneSelection( true, false );
		}
		else
		{
			pSelector->Select( pMOSel, true );
			if ( msg.nParam1 & SA_ONE_TYPE )	
			{
				std::list<int> ids;
				if ( msg.nParam1 & SA_ON_WORLD )
					GetObjects( &ids );
				else
					Scene()->PickObjects( ids, CVec2( 0, 0 ), Scene()->GetScreenRect(), IScene::PO_CENTER_INSIDE );
					
				std::list<CMapObj*> objects;
				for ( std::list<int>::iterator it = ids.begin(); it != ids.end(); ++it )
				{
					CMapObj *pMO = GetMapObj( *it );
					if ( pMO && pSelector->CanSelect( pMO ) && CSelector::IsSameType( pMOSel, pMO ) )
					{
						objects.push_back( pMO );
					}
				}
				
				for ( std::list<CMapObj*>::iterator it = objects.begin(); it != objects.end(); ++it )
				{
					CMOSelectable *pMOSel = checked_cast< CMOSelectable * >( *it );
					pSelector->Select( pMOSel, true );
				}
			}
			pSelector->DoneSelection( false, false );
		}
	}

	pSelector->SetShowAreas( ACTION_NOTIFY_SHOOT_AREA, false );
	pSelector->SetShowAreas( ACTION_NOTIFY_RANGE_AREA, false );
	Scene()->ClearMarkers( ESMT_SHOOT_AREA, -1 );

	if ( bAreasShown )
		pSelector->SetShowAreas( ACTION_NOTIFY_SHOOT_AREA, true );
	if ( bRangesShown )
		pSelector->SetShowAreas( ACTION_NOTIFY_RANGE_AREA, true );

}

void CWorldClient::MsgCancelSelection( const SGameMessage &msg )
{

}

void CWorldClient::MsgSetTarget( const SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	const CVec2 vPos = UnPackCoords( msg.nParam1 );
	DoAction( vPos, false, (EKeyboardFlags)( msg.nParam2 ) );
}

void CWorldClient::MsgResetTarget( const SGameMessage &msg )
{
	if ( IsForcedAction() ) 
	{
		NInput::PostEvent( "new_reset_forced_action", 0, 0 );
	}
}

void CWorldClient::MsgSetDestination( const SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	const CVec2 vPos = UnPackCoords( msg.nParam1 );
	CVec2 vTarget;
	ScreenToAI( &vTarget, vPos );
	bool bResult = PerformGroupAction( ACTION_COMMAND_MOVE_TO, vTarget, GetPlaceInQueue() );
	if ( bResult )
	{
		PlaceMapCommandAck( vTarget );
		ShowSelectionDst( vTarget );
	}
	bUISelectionMode = true;
	bUISelectionDir = false;
	vUISelectionMovePos = vTarget;
}

void CWorldClient::MsgUpdateDirection( const SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	CVec2 vPos = UnPackCoords( msg.nParam1 );
	bool bIsDownSamePos = (msg.nParam2 != 0);

	if ( !bIsDownSamePos )
	{
		CVec2 vTarget;
		ScreenToAI( &vTarget, vPos );

		PlaceMapCommandAckDir( vTarget );

		if ( !bUISelectionDir )
			vUISelectionStartDir = vTarget;
		bUISelectionDir = true;
		
		SUISelection uiSelection;
		ClearSelectionDst();
		ShowSelectionDst( &uiSelection, vUISelectionMovePos, vUISelectionStartDir, vTarget, false );
	}
	else
	{
		bUISelectionDir = false;
	}
}

void CWorldClient::MsgSetDirection( const SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	const CVec2 vPos = UnPackCoords( msg.nParam1 );
	CVec2 vTarget;
	ScreenToAI( &vTarget, vPos );

//	bool bResult = PerformGroupAction( ACTION_COMMAND_ROTATE_TO, vTarget, true );
//	if ( bResult )
		PlaceMapCommandAckDir( vTarget );

	if ( !bUISelectionDir )
		vUISelectionStartDir = vTarget;

	bUISelectionMode = false;

	SUISelection uiSelection;
	ClearSelectionDst();
	ShowSelectionDst( &uiSelection, vUISelectionMovePos, vUISelectionStartDir, vTarget, true );
	GotoSelectionDst( &uiSelection );
}

void CWorldClient::MsgCancelDirection( const SGameMessage &msg )
{
	ClearSelectionDst();
	bUISelectionMode = false;
}

void CWorldClient::MsgDoAction( const SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	const CVec2 vPos = UnPackCoords( msg.nParam1 );
	DoAction( vPos, false, (EKeyboardFlags)( msg.nParam2 ) );
}

bool CWorldClient::PickMapObj( const CVec2 &vPos, std::list<int> *pPickObjects )
{
	if ( vPos != vLastPickPos || GameTimer()->GetGameTime() != lastPickTime )
	{
		std::list<int> pickObjects;
		Scene()->PickObjects( pickObjects, vPos );

		std::vector<SPickObject> aliveObjects;
		aliveObjects.reserve( 20 );
		int nOrder = 0;
		for ( std::list<int>::iterator it = pickObjects.begin(); it != pickObjects.end(); ++it )
		{
			CMapObj *pMO = GetMapObj( *it );
			if ( pMO && ( pMO->IsAlive() || dynamic_cast<CMOBridge*>(pMO) != 0 ) )
			{
				const NDb::SHPObjectRPGStats *pStats = pMO->GetStats();
				if ( pStats && pStats->eGameType != NDb::SGVOGT_TERRAOBJ &&
											 pStats->eGameType != NDb::SGVOGT_TANK_PIT ) // exclude TerraObj & TankPits
				{
					SPickObject pickObj;
					pickObj.pMO = pMO;
					pickObj.nOrder = nOrder;
					aliveObjects.push_back( pickObj );
					nOrder++;
				}
			}
		}
		sort( aliveObjects.begin(), aliveObjects.end(), SMapObjectLessFunctional() );

		lastPickObjects.clear();
		for ( std::vector<SPickObject>::iterator it = aliveObjects.begin(); it != aliveObjects.end(); ++it )
		{
			SPickObject &pickObj = *it;
			if ( CMOEntrenchmentPart *pMOPart = dynamic_cast<CMOEntrenchmentPart*>( pickObj.pMO ) )
			{
				if ( pMOPart->GetParent() != 0 )
					lastPickObjects.push_back( pMOPart->GetParent()->GetID() );
			}
			else
				lastPickObjects.push_back( pickObj.pMO->GetID() );
		}
	}
	*pPickObjects = lastPickObjects;
	lastPickTime = GameTimer()->GetGameTime();
	vLastPickPos = vPos;
	return ( !pPickObjects->empty() );
}

CMapObj *CWorldClient::PickTopMapObj( const CVec2 &vPos, const NDb::EUserAction eAction )
{
	std::list<int> objects;
	if ( !PickMapObj( vPos, &objects ) )
		return 0;
	else 
	{
		CUserActions actions;
		for ( std::list<int>::const_iterator it = objects.begin(); it != objects.end(); ++it )
		{
			if ( CMapObj *pObj = GetMapObj( *it ) )
			{
				pObj->GetEnabledActions( &actions, ACTIONS_WITH );
				if ( actions.HasAction( eAction ) )
					return pObj;
			}
		}
	}
	return 0;
}


CMapObj *CWorldClient::PickTopMapObj( const CVec2 &vPos )
{
	std::list<int> objects;
	if ( !PickMapObj( vPos, &objects ) )
		return 0;
	else 
		return GetMapObj( objects.front() );
}

void CWorldClient::ScreenToAI( CVec2 *pvAI, const CVec2 &vScreen )
{
	CVec3 v3;
	Scene()->PickTerrain( &v3, vScreen );
	pvAI->x = v3.x;
	pvAI->y = v3.y;
}

CVec3 CWorldClient::ScreenToAI( const CVec2 &vScreen )
{
	CVec3 v3;
	Scene()->PickTerrain( &v3, vScreen );
	return v3;
}

// ************************************************************************************************************************ //
// **
// ** messages handlers
// **
// ** Theres no differences beetwen MiniMap click and MainMap click yet.
// **
// ************************************************************************************************************************ //

#ifndef _FINALRELEASE
#include "B2_M1_World/MOUnit.h"
#endif

void CWorldClient::DoMouseMove( const CVec2 &vPos )
{
	UpdateCursorObjects( vPos );
	pMouseTranslator->DoMouseMove( vPos, true );
	if ( !IsForcedAction() )
	{
		if ( pMouseTranslator->IsShiftDown() )
		{
			NDb::EUserAction eUserAction = NDb::USER_ACTION_UNKNOWN;
			if ( CMapObj *pMO = PickTopMapObj( vPos ) )
			{
				if ( pSelector->CanSelect( pMO ) )
				{
					if ( !pSelector->IsSelected( pMO ) )
						eUserAction = NDb::USER_ACTION_ADD_UNIT;
					else 
						eUserAction = NDb::USER_ACTION_REMOVE_UNIT;
				}
			}
			SetCursor( eUserAction );
		}
		else if ( !pSelector->IsEmpty() )
		{
			CMapObj *pMO = PickTopMapObj( vPos );
#ifndef _FINALRELEASE
			const int nDebugWindow = NGlobal::GetVar( "mouse_over_unit_details", 0 );
			if ( nDebugWindow )
			{
				CMOUnit * pUnit = dynamic_cast<CMOUnit*>( pMO );
				if ( pUnit )
				{
					Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_DEBUG_WINDOW + nDebugWindow, 
						StrFmt( "UnitID = %i, pos = (%.2f, %.2f, %.2f)", 
						pUnit->GetID(), pUnit->GetCenter().x, pUnit->GetCenter().y, pUnit->GetCenter().z ) );
				}
			}
#endif
			SetCursor( pMouseTranslator->IsCtrlDown() ? DetermineBestCtrlAction( pMO ) :
				DetermineBestAutoAction( pMO, pMouseTranslator->IsAltDown() ) );
		}
	}

	// preselection: show info about the object under cursor
	if ( bActive )
		pSelector->SetPreselection( PickTopMapObj( vPos ) );
	else
		pSelector->SetPreselection( 0 );

	switch ( eBuildObjectState )
	{
	case EBS_STARTED:
	case EBS_FIRST_POINT_SELECTED:
		{
			if ( abs( long( GameTimer()->GetAbsTime() - lastUpdate ) ) <= 100 )		//Ten times a second!
				break;

			CVec2 vAIPos;
			ScreenToAI( &vAIPos, vPos );

			if ( fabs2( vAIPos - vLastBuildPos ) < 50 )		//Some distance
				break;

			CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW + 2, StrFmt("TerrainPos ( %f, %f )", vAIPos.x, vAIPos.y ) );

			ClampToAIMap( vAIPos );												// Ensure it is inside AI map

			const EActionCommand eBuildCommand = GetCommandByAction( GetForcedAction() );
			if ( IsFirstCommandPointDefined() )
				pAI->RequestBuildPreview( eBuildCommand, GetFirstCommandPoint(), vAIPos, false );	//For long objects
			else	//CRAP: later add position and direction (for buildings)
				pAI->RequestBuildPreview( eBuildCommand, vAIPos, vAIPos, false );	//For simple objects

			lastUpdate = GameTimer()->GetAbsTime();
			vLastBuildPos = vAIPos;
		}
		break;
	default:
		{
			//EBS_COMPLETE or EBS_NONE
			//no action required, now
		}
	}
	if ( eActionMode == EAM_REINF )
	{
		SetCursor( GetForcedAction() );
	}
	else if ( eActionMode == EAM_SUPER_WEAPON )
	{
		SetCursor( GetForcedAction() );
	}
}

void CWorldClient::DoLButtonDown( const CVec2 &vPos )
{
	pMouseTranslator->DoLButtonDown( vPos, PickTopMapObj( vPos ), IsForcedAction() );
}

void CWorldClient::DoLButtonUp( const CVec2 &vPos )
{
	pMouseTranslator->DoLButtonUp( vPos, PickTopMapObj( vPos ), IsForcedAction() );
}

void CWorldClient::DoLButtonDblClk( const CVec2 &vPos )
{
	pMouseTranslator->DoLButtonDblClk( vPos, PickTopMapObj( vPos ), IsForcedAction() );
}

void CWorldClient::DoRButtonDown( const CVec2 &vPos )
{
	pMouseTranslator->DoRButtonDown( vPos, PickTopMapObj( vPos ), IsForcedAction() );
}

void CWorldClient::DoRButtonUp( const CVec2 &vPos )
{
	pMouseTranslator->DoRButtonUp( vPos, PickTopMapObj( vPos ), IsForcedAction() );
}

void CWorldClient::DoRButtonDblClk( const CVec2 &vPos )
{

}

void CWorldClient::OnLeave()
{
	bActive = false;
	SetCursor( GetForcedAction() );
	pSelector->SetPreselection( 0 );
}

void CWorldClient::OnEnter()
{
	bActive = true;
	SetCursor( GetForcedAction() );
}

void CWorldClient::MsgUnloadUnit( const SGameMessage &msg )
{
	ActionLeaveOneSquad( msg.nParam1 );
}

void CWorldClient::MsgSelectUnits( const SGameMessage &msg )
{
//	OnSelectSlot( msg.nParam1, false );
}

void CWorldClient::MsgSelectNextGroup( const SGameMessage &msg )
{
	pSelector->SelectNextGroup();
	NInput::PostEvent( "reset_target2", 0, 0 );
}

void CWorldClient::MsgSelectPrevGroup( const SGameMessage &msg )
{
	pSelector->SelectPrevGroup();
	NInput::PostEvent( "reset_target2", 0, 0 );
}

void CWorldClient::MsgMinimapMove( const SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	CVec2 vPos = UnPackCoords( msg.nParam1 );
	pMouseTranslator->DoMouseMove( vPos, false );
}

void CWorldClient::MsgMinimapDown( const SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	CVec2 vPos = UnPackCoords( msg.nParam1 );
	if ( msg.nParam2 == MSTATE_BUTTON1 )
		pMouseTranslator->DoMinimapLButtonDown( vPos, IsForcedAction() );
	else if ( msg.nParam2 == MSTATE_BUTTON2 )
		pMouseTranslator->DoMinimapRButtonDown( vPos, IsForcedAction() );
}

void CWorldClient::MsgMinimapUp( const SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	CVec2 vPos = UnPackCoords( msg.nParam1 );
	if ( msg.nParam2 == MSTATE_BUTTON1 )
		pMouseTranslator->DoLButtonUp( CVec2(-1,-1), 0, IsForcedAction() );
	else if ( msg.nParam2 == MSTATE_BUTTON2 )
		pMouseTranslator->DoRButtonUp( CVec2(-1,-1), 0, IsForcedAction() );
}

void CWorldClient::MsgSetDestinationMinimap( const SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	CVec2 vPos = UnPackCoords( msg.nParam1 );
	bool bResult = PerformGroupAction( ACTION_COMMAND_MOVE_TO, vPos, GetPlaceInQueue() );
	if ( bResult )
	{
		PlaceMapCommandAck( vPos );
		ShowSelectionDst( vPos );
	}
}

void CWorldClient::MsgSetTargetMinimap( const SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	CVec2 vPos = UnPackCoords( msg.nParam1 );
	DoAction( vPos, true, (EKeyboardFlags)( msg.nParam2 ) );
}

void CWorldClient::MsgScrollMap( const SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	const CVec2 vPos = UnPackCoords( msg.nParam1 );
	CVec2 vAnc;
	AI2Vis( &vAnc, UnPackCoords( msg.nParam1 ) );
	Camera()->SetAnchor( CVec3( vAnc.x,  vAnc.y, 0 ) );
//	Camera()->Update(); // нужно, чтобv сохранить правильнvй угол камерv при обновлении _нитов на минимапе - заменим на задержку в 1 сегмент
	bCameraUpdated = true;
}

void CWorldClient::MsgUpdateSelectedUnit( const SGameMessage &msg )
{
	pSelector->DoUpdateSelectedUnits();
}

void CWorldClient::MsgAssignSelectionGroup( const SGameMessage &msg, int nParam )
{
	if ( eActionMode != EAM_SELECT )
		return;
		
	pSelector->AssignSelectionToGroup( nParam );
}

void CWorldClient::MsgRestoreSelectionGroup( const SGameMessage &msg, int nParam )
{
	if ( eActionMode != EAM_SELECT )
		return;
		
	pSelector->AssignGroupToSelection( nParam, false );

	pSelector->SetShowAreas( ACTION_NOTIFY_SHOOT_AREA, false );
	pSelector->SetShowAreas( ACTION_NOTIFY_RANGE_AREA, false );
	Scene()->ClearMarkers( ESMT_SHOOT_AREA, -1 );

	if ( bAreasShown )
		pSelector->SetShowAreas( ACTION_NOTIFY_SHOOT_AREA, true );
	if ( bRangesShown )
		pSelector->SetShowAreas( ACTION_NOTIFY_RANGE_AREA, true );
}

void CWorldClient::MsgCenterSelectionGroup( const SGameMessage &msg, int nParam )
{
	if ( eActionMode != EAM_SELECT )
		return;

	std::vector<CMOSelectable*> members;
	pSelector->GetGroupMembers( nParam, &members );

	CenterSelectionGroupPrivate( members );
}

void CWorldClient::MsgCenterCurrentSelection( const SGameMessage &msg )
{
	CenterSelectedUnit();
}

void CWorldClient::CenterSelectionGroupPrivate( const std::vector<CMOSelectable*> &group )
{
	// TODO: надо сделать нормальное центрирование, чтобv в поле видимости (лучше в центр) попадал хотя бv один _нит
	
	if ( group.empty() )
		return;

	CVec3 vCenter( 0.0f, 0.0f, 0.0f );
	for ( std::vector<CMOSelectable*>::const_iterator it = group.begin(); it != group.end(); ++it )
	{
		CMOSelectable *pSO = *it;
		CVec3 vPos;
		CQuat qRot;
		CVec3 vScale;
		pSO->GetPlacement( &vPos, &qRot, &vScale );
		vCenter += vPos;
		break;
	}
//	vCenter /= group.size();

	vCenter = CorrectPosByCameraAndHeight( vCenter );

	AI2Vis( &vCenter );
	Camera()->SetAnchor( vCenter );
}

bool CWorldClient::IsActiveOwnAvia() const
{
	return !ownAvia.empty();
}	

void CWorldClient::GetOwnAvia( std::vector<CMapObj*> *pObjects ) const
{
	pObjects->clear();
	pObjects->reserve( ownAvia.size() );
	for ( CObjMap::const_iterator it = ownAvia.begin(); it != ownAvia.end(); ++it )
	{
		CMapObj *pMO = it->second;
		pObjects->push_back( pMO );
	}
}

void CWorldClient::MsgAreasUp( const SGameMessage &msg )
{
	pSelector->SetShowAreas( ACTION_NOTIFY_SHOOT_AREA, false );
	Scene()->ClearMarkers( ESMT_SHOOT_AREA, -1 );
	bAreasShown = false;
}

void CWorldClient::MsgAreasDown( const SGameMessage &msg )
{
	pSelector->SetShowAreas( ACTION_NOTIFY_SHOOT_AREA, true );	
	bAreasShown = true;
}

void CWorldClient::MsgRangesUp( const SGameMessage &msg )
{
	pSelector->SetShowAreas( ACTION_NOTIFY_RANGE_AREA, false );
	Scene()->ClearMarkers( ESMT_SHOOT_AREA, -1 );
	bRangesShown = false;
}

void CWorldClient::MsgRangesDown( const SGameMessage &msg )
{
	pSelector->SetShowAreas( ACTION_NOTIFY_RANGE_AREA, false );
	bRangesShown = true;
}

struct SObstacleObjectFilter : public SObjectFilter
{
	const CWorldClient *pClient;

	SObstacleObjectFilter( CWorldClient *_pClient ) : pClient( _pClient ) {}
	virtual bool operator() ( int nID ) const
	{
		CMapObj *pMO = pClient->GetMapObj( nID );
		if ( !pMO )
			return false;
		const NDb::SHPObjectRPGStats *pStats = pMO->GetStats();
		if ( !pStats || pStats->eGameType == NDb::SGVOGT_TERRAOBJ )
			return false;
		if ( dynamic_cast<IMOUnit*>( pMO ) )
			return false;
		if ( dynamic_cast<CMOBridge*>( pMO ) )
			return false;
		return true;
	};
};

struct SCanSelectObjectFilter : public SObjectFilter
{
	const CWorldClient *pClient;
	const CSelector *pSelector;

	SCanSelectObjectFilter( CWorldClient *_pClient, CSelector *_pSelector ) : pClient( _pClient ), pSelector( _pSelector ) {}
	virtual bool operator() ( int nID ) const
	{
		CMapObj *pMO = pClient->GetMapObj( nID );
		return pMO && pSelector->CanSelect( pMO ) && dynamic_cast<IMOUnit*>( pMO );
	};
};

struct SCoveredObjectFilter : public SObjectFilter
{
	const CWorldClient *pClient;

	SCoveredObjectFilter( CWorldClient *_pClient ) : pClient( _pClient ) {}
	virtual bool operator() ( int nID ) const
	{
		CMapObj *pMO = pClient->GetMapObj( nID );
		return pMO && dynamic_cast<IMOUnit*>( pMO ) && pMO->IsAlive() && pMO->IsVisible() && !pMO->IsNeutral();
	};
};

void CWorldClient::UpdateCursorObjects( const CVec2 &vPos )
{
	if ( !s_bFadeMode )
	{
		Scene()->SetFadedObjects( std::list<int>() );
	}
	else
	{
		CVec2 rect = Scene()->GetScreenRect();
		int nX = CURSOR_FADE_HALF_SIZE_X * rect.x / 1024;
		int nY = CURSOR_FADE_HALF_SIZE_Y * rect.y / 768;
		CVec2 vScreenPos1 = vPos + CVec2( -nX, -nY );
		CVec2 vScreenPos2 = vPos + CVec2( nX, nY );

		std::list<int> obstacles;
		Scene()->GetObstacleObjects( &obstacles, vScreenPos1, vScreenPos2, SCanSelectObjectFilter(this, pSelector), SObstacleObjectFilter(this) );
		
		Scene()->SetFadedObjects( obstacles );
	}

	if ( !IsXRayMode() )
	{
		Scene()->ClearPostEffectObjects();
	}
	else
	{
		std::list<int> coveredList;
		Scene()->GetCoveredObjects( &coveredList, SCoveredObjectFilter(this), SObstacleObjectFilter(this) );

		//{ CRAP - just for experiment
		if ( s_bXRayCursorOnlyFilter )
		{
			CVec2 rect = Scene()->GetScreenRect();
			int nX = CURSOR_FADE_HALF_SIZE_X * rect.x / 1024;
			int nY = CURSOR_FADE_HALF_SIZE_Y * rect.y / 768;
			CVec2 vScreenPos1 = vPos + CVec2( -nX, -nY );
			CVec2 vScreenPos2 = vPos + CVec2( nX, nY );

			std::list<int> nearObjects;
			Scene()->PickObjects( nearObjects, vScreenPos1, vScreenPos2, IScene::PO_CENTER_INSIDE, IScene::EPOC_OBJECTS );
			
			for ( std::list<int>::iterator it = coveredList.begin(); it != coveredList.end(); )
			{
				std::list<int>::const_iterator itNearObjects = find( nearObjects.begin(), nearObjects.end(), *it );
				if ( itNearObjects == nearObjects.end() )
					it = coveredList.erase( it );
				else
					++it;
			}
		}
		if ( s_bXRayFastFade )
		{
			for ( int nPlayer = 0; nPlayer < xrayUnits.size(); ++nPlayer )
			{
				std::unordered_map< int, int > &units = xrayUnits[nPlayer];
				units.clear();
			}
		}
		//}

		for ( std::list<int>::iterator it = coveredList.begin(); it != coveredList.end(); ++it )
		{
			CMapObj *pCurMO = GetMapObj( *it );
			int nPlayer = pCurMO->GetPlayer();
			if ( nPlayer >= 0 && nPlayer < xrayUnits.size() )
			{
				std::unordered_map< int, int > &units = xrayUnits[nPlayer];
				units[*it] = 0; // reset unit's timer
			}
		}

		Scene()->ClearPostEffectObjects();

		for ( int nPlayer = 0; nPlayer < xrayUnits.size(); ++nPlayer )
		{
			std::unordered_map< int, int > &units = xrayUnits[nPlayer];

			std::list<int> objects;
			for ( std::unordered_map< int, int >::iterator it = units.begin(); it != units.end(); ++it )
			{
				objects.push_back( it->first );
			}

			CVec4 vColor = COVERED_OBJECT_COLOR;
			if ( nPlayer >= 0 && nPlayer < minimapColors.size() )
				vColor = minimapColors[nPlayer];
			Scene()->AddPostEffectObjects( objects, vColor );
		}

		/*
		for( list< list<int> >::iterator iObstacles = obstaclesList.begin(); iObstacles != obstaclesList.end(); ++iObstacles )
		{
			list<int> &curObstacles = *iObstacles;

			int nCurID = curObstacles.back();
			for ( list<int>::iterator it = curObstacles.begin(); it != curObstacles.end(); ++it )
			{
				int nID = *it;
				if ( nID == nCurID )
					break;
				CMapObj *pMO = GetMapObj( *it );
				if ( IsObstacleObject( pMO ) )
				{
					CMapObj *pCurMO = GetMapObj( nCurID );
					int nPlayer = pCurMO->GetPlayer();
					if ( nPlayer >= 0 && nPlayer < xrayUnits.size() )
					{
						hash_map< int, int > &units = xrayUnits[nPlayer];
						units[nCurID] = 0; // reset unit's timer
					}
					break;
				}
			}
		}

		Scene()->ClearPostEffectObjects();
		for ( int nPlayer = 0; nPlayer < xrayUnits.size(); ++nPlayer )
		{
			hash_map< int, int > &units = xrayUnits[nPlayer];

			list<int> objects;
			for ( hash_map< int, int >::iterator it = units.begin(); it != units.end(); ++it )
			{
				objects.push_back( it->first );
			}

			CVec4 vColor = COVERED_OBJECT_COLOR;
			if ( nPlayer >= 0 && nPlayer < minimapColors.size() )
				vColor = minimapColors[nPlayer];
			Scene()->AddPostEffectObjects( objects, vColor );
		}
		*/
	}
}

bool CWorldClient::IsObstacleObject( CMapObj *pMO ) const
{
	if ( !pMO )
		return false;
	const NDb::SHPObjectRPGStats *pStats = pMO->GetStats();
	if ( !pStats || pStats->eGameType == NDb::SGVOGT_TERRAOBJ )
		return false;
	if ( dynamic_cast<IMOUnit*>( pMO ) )
		return false;
	if ( dynamic_cast<CMOBridge*>( pMO ) )
		return false;
	return true;
}

void CWorldClient::SetMinimapColors( const std::vector<CVec4> &_minimapColors )
{
	minimapColors = _minimapColors;
}

void CWorldClient::PlaceMapCommandAck( const CVec2 &vTarget )
{
	CVec3 vPos( vTarget.x, vTarget.y, pAI->GetZ( vTarget ) );
	mapCommandAck.Place( vPos );
}

void CWorldClient::PlaceMapCommandAckDir( const CVec2 &vTarget )
{
	CVec3 vPos( vTarget.x, vTarget.y, pAI->GetZ( vTarget ) );
	mapCommandAckDir.Place( vPos, mapCommandAck.vPos );
}

bool CWorldClient::IsFighter( CMapObj *pMO ) const
{
	if ( CDynamicCast<const NDb::SUnitBaseRPGStats> pStats = pMO->GetStats() )
	{
		if ( pStats->eDBtype == NDb::DB_RPG_TYPE_AVIA_FIGHTER )
			return true;
	}
	return false;
}

bool CWorldClient::IsGroundAttackPlane( CMapObj *pMO ) const
{
	if ( CDynamicCast<const NDb::SUnitBaseRPGStats> pStats = pMO->GetStats() )
	{
		if ( pStats->eDBtype == NDb::DB_RPG_TYPE_AVIA_ATTACK )
			return true;
	}
	return false;
}

bool CWorldClient::IsReconPlane( CMapObj *pMO ) const
{
	if ( CDynamicCast<const NDb::SUnitBaseRPGStats> pStats = pMO->GetStats() )
	{
		if ( pStats->eDBtype == NDb::DB_RPG_TYPE_AVIA_SCOUT )
			return true;
	}
	return false;
}

bool CWorldClient::IsSuperWeapon( CMapObj *pMO ) const
{
	if ( CDynamicCast<const NDb::SUnitBaseRPGStats> pStats = pMO->GetStats() )
	{
		if ( pStats->eDBtype == NDb::DB_RPG_TYPE_ART_SUPER ||
			pStats->eDBtype == NDb::DB_RPG_TYPE_SPG_SUPER ||
			pStats->eDBtype == NDb::DB_RPG_TYPE_ARM_SUPER ||
			pStats->eDBtype == NDb::DB_RPG_TYPE_AVIA_SUPER ||
			pStats->eDBtype == NDb::DB_RPG_TYPE_TRAIN_SUPER )
			return true;
	}
	return false;
}

bool CWorldClient::IsAviation( CMapObj *pMO ) const
{
	if ( CDynamicCast<const NDb::SUnitBaseRPGStats> pStats = pMO->GetStats() )
		return pStats->IsAviation();
	return false;
}

void CWorldClient::UpdateSpecialGroups( int nID, CMapObj *pMO )
{
	CDynamicCast<CMOSelectable> pSO = pMO;
		
	if ( pSO && pSO->CanSelect() )
	{
		if ( IsFighter( pSO ) )
		{
			sgFighters[pSO->GetID()] = pSO;
			NInput::PostEvent( "mission_update_special_select_btn", 0, 1 );
		}
		if ( IsGroundAttackPlane( pSO ) )
		{
			sgGroundAttackPlanes[pSO->GetID()] = pSO;
			NInput::PostEvent( "mission_update_special_select_btn", 1, 1 );
		}
		if ( IsReconPlane( pSO ) )
		{
			sgReconPlanes[pSO->GetID()] = pSO;
			NInput::PostEvent( "mission_update_special_select_btn", 2, 1 );
		}
		if ( IsSuperWeapon( pSO ) )
		{
			sgSuperWeapons[nID] = pSO;
			NInput::PostEvent( "mission_update_special_select_btn", 3, 1 );
		}
	}
	else
	{
		if ( !sgFighters.empty() )
		{
			sgFighters.erase( nID );
			if ( sgFighters.empty() )
				NInput::PostEvent( "mission_update_special_select_btn", 0, 0 );
		}
		if ( !sgGroundAttackPlanes.empty() )
		{
			sgGroundAttackPlanes.erase( nID );
			if ( sgGroundAttackPlanes.empty() )
				NInput::PostEvent( "mission_update_special_select_btn", 1, 0 );
		}
		if ( !sgReconPlanes.empty() )
		{
			sgReconPlanes.erase( nID );
			if ( sgReconPlanes.empty() )
				NInput::PostEvent( "mission_update_special_select_btn", 2, 0 );
		}
		if ( !sgSuperWeapons.empty() )
		{
			sgSuperWeapons.erase( nID );
			if ( sgSuperWeapons.empty() )
				NInput::PostEvent( "mission_update_special_select_btn", 3, 0 );
		}
	}
}

void CWorldClient::ToggleLockProfiles( const SGameMessage &msg )
{
	NAIVisInfo::ToggleLockProfiles();
}

void CWorldClient::PlayAllAnimations( const SGameMessage &msg )
{
	PlayAllObjectsAnimations();
}

void CWorldClient::UpdateIcons()
{
	std::vector<CMapObj*> objects;
	GetObjects( &objects );
	for ( std::vector<CMapObj*>::iterator it = objects.begin(); it != objects.end(); ++it )
	{
		CMapObj *pMO = *it;
		pMO->UpdateIcons();
	}
}

void CWorldClient::PlayerObjectiveChanged( const int nObjective, const EMissionObjectiveState eState )
{
	pScenarioTracker->SetObjectiveState( nObjective, eState );
}

void CWorldClient::OnUpdateNotifyFeedback( const struct SAIFeedbackUpdate *pUpdate )
{
	IVisualNotifications *pNotifications = GetNotifications();
	
	switch ( pUpdate->info.feedBackType )
	{
		case EFB_OBJECTIVE_CHANGED:
		{
			int nObjective = pUpdate->info.nParam >> 8;
			int nValue = pUpdate->info.nParam & 0xff;
			EMissionObjectiveState eState = (EMissionObjectiveState)nValue;

			PlayerObjectiveChanged( nObjective, eState );
			switch ( eState )
			{
				case EMOS_RECEIVED:
					pNotifications->Notify( EVNT_RECEIVE_OBJECTIVE, nObjective, VNULL2 );
					break;
				case EMOS_COMPLETED:
					pNotifications->Notify( EVNT_COMPLETE_OBJECTIVE, nObjective, VNULL2 );
					break;
				case EMOS_FAILED:
					pNotifications->Notify( EVNT_FAIL_OBJECTIVE, nObjective, VNULL2 );
					break;
				case EMOS_WAITING:
					pNotifications->Notify( EVNT_REMOVE_OBJECTIVE, nObjective, VNULL2 );
					break;
			};
		}
		break;
		//
		case EFB_REINFORCEMENT_CENTER_LOCAL_PLAYER:
		{
			CVec2 vPos( LOWORD( pUpdate->info.nParam ), HIWORD( pUpdate->info.nParam ) );
			pNotifications->Notify( EVNT_REINFORCEMENT_ARRIVED, -1, vPos );
		}
		break;
		
		case EFB_REMOVE_FEEDBACK:
		{
			pNotifications->OnRemoveEvent( (NDb::ENotificationEventType)( -1 ), pUpdate->info.nParam );
			break;
		}
		
		case EFB_KEYBUILDING_ATTACKED:
		{
			OnNotifyEvent( pUpdate, NDb::NEVT_KEY_POINT_ATTACKED, ENP_CAMERA_POS );
			break;
		}

		case EFB_COMMANDER_LEVELUP:
		{
			// nParam - reinforcement type
			OnNotifyEvent( pUpdate, NDb::NEVT_REINF_LEVELUP, ENP_NONE );

			const SFeedBackUnitsArray *pParam = checked_cast_ptr<SFeedBackUnitsArray*>( pUpdate->info.pParam );

			IScenarioTracker *pST = Singleton<IScenarioTracker>();
			const IScenarioTracker::SLeaderInfo *pLeader = pST->GetLeaderInfo( 
				(NDb::EReinforcementType)( pUpdate->info.nParam ) );
			for ( std::vector<int>::const_iterator it = pParam->unitIDs.begin(); it != pParam->unitIDs.end(); ++it )
			{
				int nID = *it;
				
				IMOUnit *pMOUnit = dynamic_cast< IMOUnit* >( GetMapObj( nID ) );
				if ( pMOUnit )
				{
					SUnitIcon unitIcon;
					unitIcon.pMOUnit = pMOUnit;
					unitIcon.fLastTime = NEW_ABILITY_ICON_SHOW_TIME;
					unitIcons[nID] = unitIcon;

					if ( pLeader )
					{
						pMOUnit->SetUnitLevel( pLeader->info.nRank, pST->GetGameType() != IScenarioTracker::EGT_SINGLE );
						DoUpdateObjectStats( pMOUnit );
					}

					pMOUnit->SetNewAbility( true );
					pMOUnit->UpdateIcons();
				}
			}
			break;
		}

		case EFB_ENEMY_AVIATION_CALLED:
		{
			OnNotifyEvent( pUpdate, NDb::NEVT_ENEMY_AVIA_DETECTED, (ENotifyParams)( ENP_UNITS | ENP_CAMERA_POS ) );
			break;
		};
		
		case EFB_AAGUN_FIRED:
		{
			OnNotifyEvent( pUpdate, NDb::NEVT_ENEMY_AA_DETECTED, (ENotifyParams)( ENP_UNITS | ENP_CAMERA_POS ) );
			break;
		}

		case EFB_HOWITZER_GUN_FIRED:
		{
			OnNotifyEvent( pUpdate, NDb::NEVT_ENEMY_ART_DETECTED, (ENotifyParams)( ENP_UNITS | ENP_CAMERA_POS ) );
			break;
		}

		case EFB_ENEMY_SIGHTED:
		{
			// switching-off because of useless
//			OnNotifyEvent( pUpdate, NDb::NEVT_ENEMY_UNIT_DETECTED, (ENotifyParams)( ENP_UNITS | ENP_CAMERA_POS ) );
			break;
		}

		case EFB_UNDER_ATTACK:
		{
			OnNotifyEvent( pUpdate, NDb::NEVT_UNIT_ATTACKED, (ENotifyParams)( ENP_UNITS | ENP_CAMERA_POS ) );
			break;
		}

		case EFB_TRACK_BROKEN:
		{
			OnNotifyEvent( pUpdate, NDb::NEVT_UNIT_BLOWUP_AT_MINE, (ENotifyParams)( ENP_UNITS | ENP_CAMERA_POS ) );
			break;
		}

		case EFB_NO_AMMO:
		{
			OnNotifyEvent( pUpdate, NDb::NEVT_UNIT_OUT_OF_AMMUNITION, (ENotifyParams)( ENP_UNITS | ENP_CAMERA_POS ) );
			break;
		}

		case EFB_MINE_SIGHTED:
		{
			OnNotifyEvent( pUpdate, NDb::NEVT_ENGINEERING_MINE_DETECTED, (ENotifyParams)( ENP_UNITS | ENP_CAMERA_POS ) );
			break;
		}

		case EFB_ENGINEER_WORK_FINISHED:
		{
			OnNotifyEvent( pUpdate, NDb::NEVT_ENGINEERING_COMPLETED, (ENotifyParams)( ENP_UNITS | ENP_CAMERA_POS ) );
			break;
		}

		case EFB_WORK_TERMINATED:
		{
			OnNotifyEvent( pUpdate, NDb::NEVT_ENGINEERING_INTERRUPTED, ENP_CAMERA_POS_SIMPLE );
			break;
		}

		case EFB_GAIN_EXP:
		{
			break;
		}
		case EFB_RIVER_POINT:
		{
			// add sound to specific point
			const CVec3 vPos( LOWORD( pUpdate->info.nParam ), HIWORD( pUpdate->info.nParam ), 0 );
			const NDb::SComplexSoundDesc * pDesc = Singleton<IInterfaceState>()->GetSoundEntry( "SOUND_RIVER_CYCLE" );
			//DEBUG{
#ifndef _FINALRELEASE
			if ( NGlobal::GetVar( "show_river_sounds", 0 ) )
			{
				CSegment segm;
				segm.p1 = CVec2( vPos.x + 10, vPos.y + 10 );
				segm.p2 = CVec2( vPos.x - 10, vPos.y - 10 );
				segm.dir = segm.p2 - segm.p1;
				DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );
				segm.p1 = CVec2( vPos.x + 10, vPos.y - 10 );
				segm.p2 = CVec2( vPos.x - 10, vPos.y + 10 );
				segm.dir = segm.p2 - segm.p1;
				DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );
			}
#endif
			//DEBUG}
			if ( pDesc == 0 )
				break;
			SoundScene()->AddSoundToMap( pDesc, vPos );

			break;
		}
		
		case EFB_OBJECTIVE_MOVED:
		{
			OnObjectiveMoved( pUpdate );
			break;
		}

		case EFB_UNITS_GIVEN:
		{
			OnNotifyEvent( pUpdate, NDb::NEVT_UNITS_GIVEN, (ENotifyParams)( ENP_UNITS | ENP_CAMERA_POS ) );
			break;
		}

		case EFB_PLACE_MARKER:
		{
			const CVec2 vPos( LOWORD( pUpdate->info.nParam ), HIWORD( pUpdate->info.nParam ) );
			GetNotifications()->PlaceMarker( vPos );
			/*CSegment segm;
			segm.p1 = CVec2( vPos.x + 50, vPos.y + 50 );
			segm.p2 = CVec2( vPos.x - 50, vPos.y - 50 );
			segm.dir = segm.p2 - segm.p1;
			DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );
			segm.p1 = CVec2( vPos.x + 50, vPos.y - 50 );
			segm.p2 = CVec2( vPos.x - 50, vPos.y + 50 );
			segm.dir = segm.p2 - segm.p1;
			DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );*/

			break;
		}

	}
}

int CWorldClient::GetHPBarColorIndex( const CMapObj *pMO, const int nPlayer )
{
	if (nPlayer == -2)
		return 20;

	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	if ( !pST )
		return 0;
	const IScenarioTracker::SPlayerColor &color = pST->GetPlayerColor( nPlayer );
	int nColour = color.nColorIndex;
	if ( pST->GetGameType() == IScenarioTracker::EGT_MULTI_FLAG_CONTROL && pMO->IsKeyObject() )
	{
		const int nTeam = pST->GetPlayerSide( nPlayer );
		nColour = 3;
		if ( nTeam == 0 )
			nColour = 0;
		else if ( nTeam == 1 )
			nColour = 2;
	}

	return nColour;
}

void CWorldClient::OnUpdateNewUnit( const SAINewUnitUpdate *pUpdate, CMapObj *pMO )
{
	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	if ( CDynamicCast<IMOUnit> pUnit = pMO )
	{
		pUnit->SetReinfType( pUpdate->info.eReinfType );
		const int eType = pUpdate->info.eReinfType;
		if ( eType == NDb::RT_ENGINEERING
			|| eType == NDb::RT_RECON
			|| eType == NDb::RT_SUPER_WEAPON
			// --- Extra Max Level Ground ---
			|| eType == NDb::RT_EXTRA_MAXLVL_GROUND_1
			|| eType == NDb::RT_EXTRA_MAXLVL_GROUND_2
			|| eType == NDb::RT_EXTRA_MAXLVL_GROUND_3
			|| eType == NDb::RT_EXTRA_MAXLVL_GROUND_4
			|| eType == NDb::RT_EXTRA_MAXLVL_GROUND_5
			// --- Extra Max Level Air ---
			|| eType == NDb::RT_EXTRA_MAXLVL_AIR_1
			|| eType == NDb::RT_EXTRA_MAXLVL_AIR_2
			|| eType == NDb::RT_EXTRA_MAXLVL_AIR_3
			|| eType == NDb::RT_EXTRA_MAXLVL_AIR_4
			|| eType == NDb::RT_EXTRA_MAXLVL_AIR_5
			// --- Extra Max Level Mixed ---
			|| eType == NDb::RT_EXTRA_MAXLVL_MIXED_1
			|| eType == NDb::RT_EXTRA_MAXLVL_MIXED_2
			|| eType == NDb::RT_EXTRA_MAXLVL_MIXED_3
			|| eType == NDb::RT_EXTRA_MAXLVL_MIXED_4
			|| eType == NDb::RT_EXTRA_MAXLVL_MIXED_5 )
			pUnit->SetUnitLevel( pUpdate->info.nExpLevel, false );
		else if ( pST )
		{
			if ( pST->GetGameType() == IScenarioTracker::EGT_SINGLE )
			{
				if ( const IScenarioTracker::SLeaderInfo *pLeader = pST->GetLeaderInfo( pUpdate->info.eReinfType ) )
					pUnit->SetUnitLevel( pLeader->info.nRank, false );
				else
					pUnit->SetUnitLevel( pUpdate->info.nExpLevel, false );
			}
			else
			{
				pUnit->SetUnitLevel( pUpdate->info.nExpLevel, true );
			}
		}
		else	// in main menu
			pUnit->SetUnitLevel( 0, false );
		pUnit->UpdateIcons();
	}
	pMO->SetColorIndex( GetHPBarColorIndex( pMO, pUpdate->info.nPlayer ), true );
}

void CWorldClient::OnUpdateDiplomacy( CMapObj *pMO, const int nNewPlayer )
{
	pMO->SetColorIndex( GetHPBarColorIndex( pMO, nNewPlayer ), true );
}

void CWorldClient::OnUpdateKeyObjectProgress( CMapObj *pMO, float fProgress, int nPlayer )
{
	pMO->AIUpdateKeyObjectCaptureProgress( fProgress, GetHPBarColorIndex( pMO, nPlayer ) );
}

void CWorldClient::OnUpdateVisualStatus( const SUnitStatusUpdate *pUpdate, CMapObj *pMO )
{
	pMO->UpdateVisualStatus( *pUpdate );
}

void CWorldClient::OnNewMapObj( int nID, CMapObj *pMO )
{
	UpdateSpecialGroups( nID, pMO );

	if ( IsAviation( pMO ) && pMO->IsFriend() )
		ownAvia[pMO->GetID()] = pMO;
}

void CWorldClient::OnDeadOrRemoveMapObj( int nID )
{
	UpdateSpecialGroups( nID, 0 );

	if ( !ownAvia.empty() )
	{
		ownAvia.erase( nID );
		if ( ownAvia.empty() )
		{
			GetNotifications()->OnRemoveEvent( NDb::NEVT_AVIA_AVAILABLE, -1 );
		}
	}
	CUnitIconMap::iterator it = unitIcons.find( nID );
	if ( it != unitIcons.end() )
	{
		SUnitIcon &unitIcon = it->second;

		unitIcon.pMOUnit->SetNewAbility( false );
		unitIcon.pMOUnit->UpdateIcons();

		unitIcons.erase( it );
	}
}

void CWorldClient::UpdateSpecialSelection( int nID, CMapObj *pMO )
{
	UpdateSpecialGroups( nID, pMO );
}

void CWorldClient::SetActionMode( EActionMode _eActionMode )
{
	eActionMode = _eActionMode;
}

int CWorldClient::operator&( IBinSaver &saver )
{
	saver.Add( 1, checked_cast<CUpdatableWorld*>(this) );
	saver.Add( 2, &pMouseTranslator );
	saver.Add( 3, &pSelector );
	saver.Add( 5, &eventsMap );
	saver.Add( 6, &eForcedAction );
//	saver.Add( 7, &userActionsMap );
	saver.Add( 8, &eCurrentAbilityParam );
	saver.Add( 9, &vMapSize );
	saver.Add( 10, &bFirstCommandPointDef);
	saver.Add( 11, &vFirstCommandPoint );
	saver.Add( 12, &pCommandsSender );
	//saver.Add( 13, &bFadeMode );
	saver.Add( 15, &eBuildObjectState );
	saver.Add( 16, &lastUpdate );
	saver.Add( 17, &vLastBuildPos );
	saver.Add( 18, &pForcedActionParam );
	saver.Add( 19, &bAreasShown );
	saver.Add( 20, &bRangesShown );
	saver.Add( 21, &mapCommandAck );
	saver.Add( 22, &pAI );
	saver.Add( 23, &eActionMode );
	saver.Add( 24, &mapCommandAckDir );
	saver.Add( 25, &terrainSounds );
	saver.Add( 26, &sgFighters );
	saver.Add( 27, &sgGroundAttackPlanes );
	saver.Add( 28, &sgReconPlanes );
	saver.Add( 29, &sgSuperWeapons );
	saver.Add( 30, &pMapInfo );
	saver.Add( 31, &pScenarioTracker );
	saver.Add( 32, &ownAvia );
	saver.Add( 33, &unitIcons );
	saver.Add( 34, &minimapColors );
	saver.Add( 35, &bIsOwnUnitsPresent );

	return 0;
}

// SMapCommandAck

void CWorldClient::SMapCommandAck::Place( const CVec3 &_vPos )
{
	if ( !pModel )
		return;

	vPos = _vPos;
	if ( bPlaced )
		Scene()->RemoveObject( nUniqueID );
	Scene()->AddObject( nUniqueID, pModel, vPos, 
		QNULL, CVec3(1, 1, 1), OBJ_ANIM_MODE_FORCE_ANIMATED_STATIC_ABS, 0 );
	if ( pModel->pSkeleton != 0 ) 
	{
		if ( NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( nUniqueID ) )
		for ( int i = 0; i < pModel->pSkeleton->animations.size(); ++i )
		{
			const NDb::SAnimB2 *pAnim = checked_cast_ptr<const NDb::SAnimB2*>( pModel->pSkeleton->animations[i] );
			if ( pAnim->eType == NDb::ANIMATION_IDLE )
			{
				AddAnimation( pAnim, Singleton<IGameTimer>()->GetAbsTime(), pAnimator, false/*pAnim->bLooped*/ );
				break;
			}
		}
	}
	bPlaced = true;
	nTime = Singleton<IGameTimer>()->GetAbsTime();
}

void CWorldClient::SMapCommandAck::Place( const CVec3 &_vPos, const CVec3 &vStartPos )
{
	Place( _vPos );
	if ( bPlaced )
	{
		float fAngle = atan2f( vPos.y - vStartPos.y, vPos.x - vStartPos.x );
		CQuat q;
		q.FromAngleAxis( fAngle, CVec3( 0.0f, 0.0f, 1.0f ) );
		Scene()->MoveObject( nUniqueID, vPos, q );
	}
}

void CWorldClient::SMapCommandAck::Update()
{
	if ( !bPlaced )
		return;
		
	NTimer::STime nCurrentTime = Singleton<IGameTimer>()->GetAbsTime();
	if ( nTime == 0 )
	{
		nTime = nCurrentTime;

		// уберем объект после загрузки игрv (то, что он может убраться в самом начале игрv можно проигнорировать)
		Scene()->RemoveObject( nUniqueID );
		bPlaced = false;
		return;
	}
	
	if ( nCurrentTime - nTime >= (int)(fShowTime * 1000.0f) )
	{
		Scene()->RemoveObject( nUniqueID );
		bPlaced = false;
	}
}

void CWorldClient::SetOnMinimap( bool _bOnMinimap )
{
	if ( _bOnMinimap )
	{
		bOnMinimap = true;
		bOnMinimapOff = false;
	}
	else
	{
		if ( bOnMinimapOff )
			bOnMinimap = _bOnMinimap;
		bOnMinimapOff = true;
	}
}

void CWorldClient::GetTerrainMassData( std::vector<SSoundTerrainInfo> *pData, int nMaxSize )
{
	ITerraManager * pTerraManager = Scene()->GetTerraManager();
	CVec3 vPos;
	const CVec2 vScreenSize = Scene()->GetScreenRect();
	Scene()->PickZeroHeight( &vPos, vScreenSize * 0.5f );
	const float fRadius = 0.5f * ( vScreenSize.x + vScreenSize.y );
	
	CArray2D<BYTE> areaTypes;
	pTerraManager->GetAreaTileTypes( &areaTypes, (vPos.x - fRadius) / VIS_TILE_SIZE,
																							 (vPos.y - fRadius) / VIS_TILE_SIZE, 
																							 (vPos.x + fRadius) / VIS_TILE_SIZE,
																							 (vPos.y + fRadius) / VIS_TILE_SIZE );
	std::unordered_map<int, SSoundTerrainInfo> infos;
	for ( int x = 0; x < areaTypes.GetSizeX(); ++x )
	{
		for ( int y = 0; y < areaTypes.GetSizeY(); ++y )
		{
			SSoundTerrainInfo &info = infos[areaTypes[y][x]];
			info.fWeight += 1;
			info.vPos += CVec3( (std::max)(0.0f, vPos.x - fRadius) + x * VIS_TILE_SIZE, (std::max)(0.0f, vPos.y - fRadius) + y * VIS_TILE_SIZE, 0 );
		}
	}
	pData->resize( infos.size() );
	int i = 0;
	for ( std::unordered_map<int, SSoundTerrainInfo>::const_iterator it = infos.begin(); it != infos.end(); ++it )
	{
		(*pData)[i].fWeight = it->second.fWeight;
		(*pData)[i].vPos = it->second.vPos / it->second.fWeight;
		(*pData)[i].nTerrainType = it->first;
	}

	// сначала отсортировать по массе
	SSoundTerrainInfo::CPrSoundsMassSort prMassSort;
	sort( pData->begin(), pData->end(), prMassSort );
	// удалить все с нулевой массой
	SSoundTerrainInfo::CPrZeroMass prZeroMass;
	std::vector<SSoundTerrainInfo>::iterator firstZeromass = find_if( pData->begin(), pData->end(), prZeroMass );
	int nSize = (std::min<int>)( nMaxSize, firstZeromass - pData->begin() );
	pData->resize( nSize );
	// оставшееся отсортировать по TerrainType
	SSoundTerrainInfo::CPrTerrainTypeSort prTerrainType;
	sort( pData->begin(), pData->end(), prTerrainType );
}

float CWorldClient::GetSoundVolume( int nTerrainType ) const
{
	// ask terrain stats about sound volume
	return 1.0f;
}

const NDb::SComplexSoundDesc * CWorldClient::GetTerrainSound( int nTerrainType )
{
	ITerraManager * pTerraManager = Scene()->GetTerraManager();
	// ask terrain stats about sounds
	const std::vector<CDBPtr<NDb::STGTerraType> > &terraTypes = pTerraManager->GetDesc()->pTerraSet->terraTypes;
	if ( nTerrainType >= terraTypes.size() )
		return 0;
	return terraTypes[nTerrainType]->pSound;
}

const NDb::SComplexSoundDesc * CWorldClient::GetTerrainCycleSound( int nTerrainType )
{
	ITerraManager * pTerraManager = Scene()->GetTerraManager();
	// ask terrain stats about sounds
	const std::vector<CDBPtr<NDb::STGTerraType> > &terraTypes = pTerraManager->GetDesc()->pTerraSet->terraTypes;
	if ( nTerrainType >= terraTypes.size() )
		return 0;
	return terraTypes[nTerrainType]->pCycledSound;
}

void CWorldClient::OnSelectSlot( int nSlot, WORD wKeyboardFlags )
{
	NI_VERIFY( nSlot >= 0, "Wrong slot", return );

	if ( pSelector->IsUnloadMode() )
		ActionLeaveOneSquad( nSlot );
	else
	{
		pSelector->SetShowAreas( ACTION_NOTIFY_SHOOT_AREA, false );
		pSelector->SetShowAreas( ACTION_NOTIFY_RANGE_AREA, false );
		Scene()->ClearMarkers( ESMT_SHOOT_AREA, -1 );

		if ( (wKeyboardFlags & EKF_SHIFT) != 0 )
			pSelector->UnselectSlot( nSlot );
		else if ( (wKeyboardFlags & EKF_CTRL) != 0 )
			pSelector->SelectSameType( nSlot );
		else
			pSelector->SelectSameSlots( nSlot );

		if ( bAreasShown )
			pSelector->SetShowAreas( ACTION_NOTIFY_SHOOT_AREA, true );
		if ( bRangesShown )
			pSelector->SetShowAreas( ACTION_NOTIFY_RANGE_AREA, true );
	}
}

void CWorldClient::OnClickMultiSelectUnit( int nSlot, WORD wKeyboardFlags )
{
	if ( !IsForcedAction() )
		OnSelectSlot( nSlot, wKeyboardFlags );
	else
		OnActionOnSlot( nSlot, wKeyboardFlags );
}

void CWorldClient::OnActionOnSlot( int nSlot, WORD wKeyboardFlags )
{
	const CMapObj *pMO = pSelector->GetFirstSlotUnit( nSlot );
	if ( !pMO )
		return;
		
	NDb::EUserAction eUserAction = GetForcedAction();
	USER_ACTION pfnAction = GetAction( eUserAction, SActionDesc::FORCED );
	NI_ASSERT( pfnAction != 0, StrFmt( "Forced action (%d) not registred", GetForcedAction() ) );

	DoAction( pfnAction, eUserAction, VNULL2, pMO, false, (wKeyboardFlags & EKF_SHIFT) != 0, IsForcedAction() );
}

void CWorldClient::OnSelectSpecialGroup( int nIndex )
{
	CSpecialGroup *pGroup = 0;
	switch ( nIndex )
	{
		case 0:
			pGroup = &sgFighters;
		break;
		
		case 1:
			pGroup = &sgGroundAttackPlanes;
		break;

		case 2:
			pGroup = &sgReconPlanes;
		break;

		case 3:
			pGroup = &sgSuperWeapons;
		break;
	}
	if ( !pGroup )
		return;

	pSelector->SetShowAreas( ACTION_NOTIFY_SHOOT_AREA, false );
	pSelector->SetShowAreas( ACTION_NOTIFY_RANGE_AREA, false );
	Scene()->ClearMarkers( ESMT_SHOOT_AREA, -1 );

	std::vector<CMOSelectable*> prevSelection;
	if ( eActionMode == EAM_SELECT )
		pSelector->GetSelectionMembers( &prevSelection );

	pSelector->Empty();
	for ( CSpecialGroup::iterator it = pGroup->begin(); it != pGroup->end(); ++it )
	{
		CMOSelectable *pMOSel = it->second;
		pSelector->Select( pMOSel, true );
	}
	pSelector->DoneSelection( true, false );

	if ( eActionMode == EAM_SELECT )
	{
		std::vector<CMOSelectable*> selection;
		pSelector->GetSelectionMembers( &selection );

		if ( selection == prevSelection )
			CenterSelectionGroupPrivate( selection );
	}

	if ( bAreasShown )
		pSelector->SetShowAreas( ACTION_NOTIFY_SHOOT_AREA, true );
	if ( bRangesShown )
		pSelector->SetShowAreas( ACTION_NOTIFY_RANGE_AREA, true );
}

void CWorldClient::CenterSelectedUnit()
{
	if ( eActionMode == EAM_SELECT )
	{
		std::vector<CMOSelectable*> selection;
		pSelector->GetSelectionMembers( &selection );

		if ( !selection.empty() )
			CenterSelectionGroupPrivate( selection );
	}
}

void CWorldClient::GetCameraByName( NCamera::CCameraPlacement *pCamera, const std::string &rszName ) const
{
	for ( std::vector<NDb::SScriptCameraPlacement>::const_iterator it = pMapInfo->scriptMovies.scriptCameraPlacements.begin(); it < pMapInfo->scriptMovies.scriptCameraPlacements.end(); ++it )
	{
		const NDb::SScriptCameraPlacement &cameraPos = (*it);
		if ( cameraPos.szName == rszName )
		{
			NCamera::CCameraPlacement newCamera( cameraPos.vPosition, cameraPos.fYaw, cameraPos.fPitch, cameraPos.fFOV, 0.0f );
			newCamera.szName = cameraPos.szName;

			(*pCamera) = newCamera;
			return;
		}
	}
}

void CWorldClient::GetObjectPosByScriptID( CVec3 *pObjPos, int nScriptID ) const
{
	for ( std::vector<NDb::SMapObjectInfo>::const_iterator it = pMapInfo->objects.begin(); it < pMapInfo->objects.end(); ++it )
	{
		const NDb::SMapObjectInfo &object = (*it);
		if ( object.nScriptID == nScriptID )
		{
			CVec3 newObjPos;
			newObjPos = object.vPos;
			(*pObjPos) = newObjPos;
			return;
		}
	}
}

bool CWorldClient::GetMoviesData( NDb::SScriptMovies *pMoviesData ) const
{
	if ( pMapInfo->scriptMovies.scriptMovieSequences.size() > 0 )
	{
		(*pMoviesData) = pMapInfo->scriptMovies;
		return true;
	}

	return false;
}

void CWorldClient::OnNotifyEvent( const struct SAIFeedbackUpdate *pUpdate, NDb::ENotificationEventType eType,
	ENotifyParams eParams )
{
	const SFeedBackUnitsArray *pParam = checked_cast_ptr<SFeedBackUnitsArray*>( pUpdate->info.pParam );
	// CRAP - use assert instead
	NI_VERIFY( pParam || (eParams & ENP_CAMERA_POS_SIMPLE) == ENP_CAMERA_POS_SIMPLE, "Programmers: pParam = 0", return );
	IVisualNotifications::SEventParams params;
	params.nID = pUpdate->info.nParam;
	params.eEventType = eType;
	if ( (eParams & ENP_CAMERA_POS) != 0 )
	{
		params.positions.push_back( pParam->vCenterCamera );
	}
	if ( (eParams & ENP_UNITS_POS) != 0 )
	{
		params.positions.reserve( pParam->unitIDs.size() );
		for ( int i = 0; i < pParam->unitIDs.size(); ++i )
		{
			const CMapObj *pMO = GetMapObj( pParam->unitIDs[i] );
			if ( pMO )
			{
				const CVec3 &vCenter = pMO->GetCenter();
				params.positions.push_back( CVec2( vCenter.x, vCenter.y ) );
			}
		}
	}
	if ( (eParams & ENP_IDS) != 0 )
	{
		params.ids = pParam->unitIDs;
	}
	if ( (eParams & ENP_CAMERA_POS_SIMPLE) != 0 )
	{
		CVec2 vPos( LOWORD( pUpdate->info.nParam ), HIWORD( pUpdate->info.nParam ) );
		params.positions.push_back( vPos );
	}
	if ( (eParams & ENP_UNITS) != 0 )
	{
		params.objects.reserve( pParam->unitIDs.size() );
		for ( int i = 0; i < pParam->unitIDs.size(); ++i )
		{
			CMapObj *pMO = GetMapObj( pParam->unitIDs[i] );
			if ( pMO )
				params.objects.push_back( pMO );
		}
	}
	GetNotifications()->OnEvent( params );
}

void CWorldClient::OnObjectiveMoved( const struct SAIFeedbackUpdate *pUpdate )
{
	const SFeedBackUnitsArray *pParam = checked_cast_ptr<SFeedBackUnitsArray*>( pUpdate->info.pParam );
	NI_ASSERT( pParam != 0, "Programmers: pParam = 0" );
	if ( !pParam )
		return;

	int nID = pUpdate->info.nParam;
	//{ CRAP
	if ( nID < 0 )
		nID = 0;
	//}
	
	std::vector< CMapObj* > objects;
	objects.reserve( pParam->unitIDs.size() );
	for ( int i = 0; i < pParam->unitIDs.size(); ++i )
	{
		CMapObj *pMO = GetMapObj( pParam->unitIDs[i] );
		if ( pMO )
			objects.push_back( pMO );
		CDynamicCast<IMOUnit> pUnit = pMO;

		const NDb::SClientGameConsts *pClientConsts = NGameX::GetClientConsts();
		const NDb::SVisObj *pVisObj = (pClientConsts != 0) ? pClientConsts->pMapPointer : 0;
		const NDb::SModel *pModel = (pVisObj != 0 && !pVisObj->models.empty()) ? pVisObj->models.front().pModel : 0;

		if ( pUnit )
			pUnit->SetPointer( pModel );
	}
	Singleton<IScenarioTracker>()->SetObjectiveObjects( nID, objects );

	IVisualNotifications::SEventParams params;
	params.nID = nID;
	//{ CRAP
	if ( params.nID < 0 )
		params.nID = 0;
	//}
	params.eEventType = NDb::NEVT_OBJECTIVE_MOVED;

	GetNotifications()->OnEvent( params );
}

void CWorldClient::UpdateUnitIcons( float fDeltaTime )
{
	std::list<int> removed;
	for ( CUnitIconMap::iterator it = unitIcons.begin(); it != unitIcons.end(); ++it )
	{
		SUnitIcon &unitIcon = it->second;
		
		unitIcon.fLastTime -= fDeltaTime;
		if ( unitIcon.fLastTime <= 0.0f )
		{
			unitIcon.pMOUnit->SetNewAbility( false );
			unitIcon.pMOUnit->UpdateIcons();
			removed.push_back( it->first );
		}
	}
	for ( std::list<int>::iterator it = removed.begin(); it != removed.end(); ++it )
	{
		unitIcons.erase( *it );
	}
}

void CWorldClient::UpdateXRayUnits( int nDeltaTime )
{
	for ( int nPlayer = 0; nPlayer < xrayUnits.size(); ++nPlayer )
	{
		std::unordered_map< int, int > &units = xrayUnits[nPlayer];
		if ( !IsXRayMode() )
		{
			units.clear();
			continue;
		}
		
		std::vector<int> toRemoveUnits;
		toRemoveUnits.reserve( units.size() );
		for ( std::unordered_map< int, int >::iterator it = units.begin(); it != units.end(); ++it )
		{
			it->second += nDeltaTime;
			if ( it->second > MAX_XRAY_TIME )
				toRemoveUnits.push_back( it->first );
		}
		for ( std::vector<int>::iterator it = toRemoveUnits.begin(); it != toRemoveUnits.end(); ++it )
		{
			units.erase( *it );
		}
	}
}

bool CWorldClient::IsXRayMode() const
{
	return s_bXRayMode || bForcedXRayMode;
}

void CWorldClient::OnUpdateSuperWeaponControl( const struct SSuperWeaponControl &update )
{
	if ( pSuperWeapon )
	{
		IScenarioTracker *pST = Singleton<IScenarioTracker>();
		if ( pST && update.nPlayer == pST->GetLocalPlayer() )
		{
			CMapObj *pMO = 0;
			if ( update.nUnitID != -1 )
				pMO = GetMapObj( update.nUnitID );
			pSuperWeapon->OnUpdateSuperWeaponControl( pMO, update.pUnit, update.bEnabled );
		}
	}
}

void CWorldClient::OnUpdateSuperWeaponRecycle( const struct SSuperWeaponRecycle &update )
{
	if ( pSuperWeapon )
	{
		IScenarioTracker *pST = Singleton<IScenarioTracker>();
		if ( pST && update.nPlayer == pST->GetLocalPlayer() )
		{
			pSuperWeapon->OnUpdateSuperWeaponRecycle( update.fPartComplete );
		}
	}
}

void CWorldClient::OnReplaceSelectionGroup( CMOSelectable *pMOPattern, CMOSelectable *pMO )
{
	pSelector->ReplaceSelectionGroup( pMOPattern, pMO );
}

REGISTER_SAVELOAD_CLASS( 0x10078340, CWorldClient );



//
//
//		Passability markers are used in development configurations
//
//


#include "DebugTools/DebugInfoManagerInternal.h"
#include "Misc/StrProc.h"
#include "Common_RTS_AI/AIClasses.h"
#include "Common_RTS_AI/Terrain.h"
#include "Common_RTS_AI/TerraAIObserver.h"
#include "System/Commands.h"
#ifndef _FINALRELEASE

static void CommandDumpMaxes( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	if ( paramsSet.size() != 2 )
	{
		csSystem << "usage: " << szID << "marker_color file_name ai_class" << endl;
		csSystem << "\tfile_name:\tname for .tga file with maxes (file will be saved in debug_images folder with .tga extension)" << endl;
		csSystem << "\tai_class:\t\tHUMAN, WHEEL, TRACK, SEA, RIVER, TERRAIN, WATER, ANY" << endl;
		return;
	}

	std::string szParam;

	NStr::ToMBCS( &szParam, paramsSet[1] );
	NStr::ToLowerASCII( &szParam );
	EAIClasses aiClass = EAC_NONE;
	if (szParam == "human" )
		aiClass = EAC_HUMAN;
	else if ( szParam == "wheel" )
		aiClass = EAC_WHELL;
	else if ( szParam == "track" )
		aiClass = EAC_TRACK;
	else if ( szParam == "river" )
		aiClass = EAC_RIVER;
	else if ( szParam == "sea" )
		aiClass = EAC_SEA;
	else if ( szParam == "terrain" )
		aiClass = EAC_TERRAIN;
	else if ( szParam == "water" )
		aiClass = EAC_WATER;
	else if ( szParam == "any" )
		aiClass = EAC_ANY;

	NStr::ToMBCS( &szParam, paramsSet[0] );
	NStr::ToLowerASCII( &szParam );
	Scene()->GetTerraManager()->GetAIObserver()->DumpMaxes( szParam, (int)aiClass );
}

static void CommandPassMarker( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	if ( paramsSet.size() < 2 || paramsSet.size() > 4 )
	{
		csSystem << "usage: " << szID << "marker_color ai_class [free_class] [radius]" << endl;
		csSystem << "\tmarker_color:\tRED, GREEN or BLUE" << endl;
		csSystem << "\tai_class:\t\tNONE (turn off marker), HUMAN, WHEEL, TRACK, SEA, RIVER, TERRAIN, WATER, ANY" << endl;
		csSystem << "\tfree_class:\t\tNONE, TERRAIN, WATER, ANY; default: NONE" << endl;
		csSystem << "\tradius:\t\t\tBound tile radius; defaut: 0" << endl;
		return;
	}

	std::string szParam;
	NStr::ToMBCS( &szParam, paramsSet[0] );
	NStr::ToLowerASCII( &szParam );
	NDebugInfo::EColor color = NDebugInfo::GREEN;
	if ( szParam == "red" )
		color = NDebugInfo::RED;
	else if ( szParam == "green" )
		color = NDebugInfo::GREEN;
	else if ( szParam == "blue" )
		color = NDebugInfo::BLUE;
	else
	{
		csSystem << "error: invalid color, use RED, GREEN or BLUE" << endl;
		return;
	}

	NStr::ToMBCS( &szParam, paramsSet[1] );
	NStr::ToLowerASCII( &szParam );
	EAIClasses aiClass = EAC_TERRAIN;
	if (szParam == "human" )
		aiClass = EAC_HUMAN;
	else if ( szParam == "wheel" )
		aiClass = EAC_WHELL;
	else if ( szParam == "track" )
		aiClass = EAC_TRACK;
	else if ( szParam == "river" )
		aiClass = EAC_RIVER;
	else if ( szParam == "sea" )
		aiClass = EAC_SEA;
	else if ( szParam == "terrain" )
		aiClass = EAC_TERRAIN;
	else if ( szParam == "water" )
		aiClass = EAC_WATER;
	else if ( szParam == "any" )
		aiClass = EAC_ANY;
	else
		aiClass = EAC_NONE;

	EFreeTileInfo freeClass = FREE_NONE;
	if ( paramsSet.size() > 2 )
	{
		NStr::ToMBCS( &szParam, paramsSet[2] );
		NStr::ToLowerASCII( &szParam );
		if ( szParam == "none" )
			freeClass = FREE_NONE;
		else if ( szParam == "terrain" )
			freeClass = FREE_TERRAIN;
		else if ( szParam == "water" )
			freeClass = FREE_WATER;
		else
			freeClass = FREE_ANY;
	}

	int nBoundTileRadius = 0;
	if ( paramsSet.size() > 3 )
	{
		NStr::ToMBCS( &szParam, paramsSet[3] );
		if ( !NStr::IsDecNumber( szParam ) )
		{
			csSystem << "error: invalid radius, must be integer value" << endl;
			return;
		}
		nBoundTileRadius = NStr::ToInt( szParam );
		//if ( nBoundTileRadius < 0 || nBoundTileRadius > pAIMap->GetMaxUnitTileRadius() )
		if ( nBoundTileRadius < 0 )
		{
			csSystem << "error: invalid radius, must be integer value between 0 and " << endl;
			return;
		}
	}

	Scene()->GetTerraManager()->GetAIObserver()->SetPassMarkers( (int)color, (int)aiClass, (int)freeClass, nBoundTileRadius );
}

#endif // _FINALRELEASE
void MsgXRay( const SGameMessage &msg, int nPressed )
{
	bForcedXRayMode = (nPressed == 1);
}

START_REGISTER( GameX_PassMarkers )
REGISTER_VAR_EX( "cursor_fade_mode", NGlobal::VarBoolHandler, &s_bFadeMode, false, STORAGE_USER );
REGISTER_VAR_EX( "xray_mode", NGlobal::VarBoolHandler, &s_bXRayMode, true, STORAGE_USER );
REGISTER_VAR_EX( "xray_cursor_only_filter", NGlobal::VarBoolHandler, &s_bXRayCursorOnlyFilter, false, STORAGE_USER );
REGISTER_VAR_EX( "xray_fast_fade", NGlobal::VarBoolHandler, &s_bXRayFastFade, false, STORAGE_USER );
#ifndef _FINALRELEASE
REGISTER_CMD( "pass_marker", CommandPassMarker )
REGISTER_CMD( "dump_maxes", CommandDumpMaxes );
#endif
FINISH_REGISTER



