#pragma once

#include "MouseTranslator.h"
#include "Selector.h"

#include "B2_M1_World/UpdatableWorld.h"
#include "Sound/TerrainSounds.h"

namespace NDb
{
	struct SMapInfo;
};

struct ITransceiver;
struct IVisualNotifications;
struct IAILogic;
struct IScenarioTracker;
struct IMissionSuperWeapon;

typedef	std::unordered_map< NDb::EUserAction, std::string, SEnumHash > CEventsMap;

enum EActionMode
{
	EAM_SELECT,
	EAM_REINF,
	EAM_SUPER_WEAPON,
};

class CWorldClient : public CUpdatableWorld, protected NInput::CGMORegContainer, public ITerrainSounds
{
	OBJECT_NOCOPY_METHODS( CWorldClient );
private:
	//
	//
	typedef bool (CWorldClient::*USER_ACTION)( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	struct SActionDesc
	{
		enum
		{
			AUTO		= 0x00000001,							// action can be initiated with respect to current pick and selection
			FORCED	= 0x00000002,							// action can be activated only from interface
			INSTANT	= 0x00000004							// action will be instantly performed in the moment of initiation
		};
		//
		USER_ACTION pfnAction;
		DWORD flags;
	};

	enum EBuildState
	{
		EBS_NONE,
		EBS_STARTED,
		EBS_FIRST_POINT_SELECTED,
		EBS_COMPLETE,
	};

	typedef std::unordered_map<int, SActionDesc> CActionsMap;
	
	struct SMapCommandAck
	{
		ZDATA
		int nUniqueID;
		CDBPtr<NDb::SModel> pModel;
		bool bPlaced;
		float fShowTime;
		CVec3 vPos;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&nUniqueID); f.Add(3,&pModel); f.Add(4,&bPlaced); f.Add(5,&fShowTime); f.Add(6,&vPos); return 0; }
		
		NTimer::STime nTime;
	public:
		SMapCommandAck() : nTime( 0 ) {}
		
		void Place( const CVec3 &vPos );
		void Place( const CVec3 &vPos, const CVec3 &vStartPos );
		void Update();
	};
	
	struct SUISelection;
	
	//
	CDBPtr<NDb::SMapInfo> pMapInfo;
	//
	CObj< CSelector > pSelector;
	CObj< CMouseTranslator > pMouseTranslator;
	CEventsMap	eventsMap;
	NDb::EUserAction eForcedAction;
	CPtr<CObjectBase> pForcedActionParam;
	CActionsMap userActionsMap;	// DON'T SAVE THIS FIELD
	std::list<int>	lastPickObjects;
	NTimer::STime lastPickTime;
	CVec2	vLastPickPos;
	CVec2	vFirstCommandPoint;
	CVec2 vSelectionFirstPoint;
	CVec2 vSelectionLastPoint;
	bool bFirstCommandPointDef;
//	bool bPlaceCommandInQuery;
	bool bActive;
	NDb::ESpecialAbilityParam eCurrentAbilityParam;
	CVec2 vMapSize;
	CObj<CCommandsSender> pCommandsSender;

	EBuildState eBuildObjectState;
	CVec2 vLastBuildPos;
	NTimer::STime lastUpdate;

	bool bAreasShown;
	bool bRangesShown;
	
	SMapCommandAck mapCommandAck;
	SMapCommandAck mapCommandAckDir;
	
	bool bCameraUpdated; // don't save
	CPtr<IAILogic> pAI;

	EActionMode eActionMode;

	bool bOnMinimap; // don't save
	bool bOnMinimapOff; // don't save
	std::vector<SSoundTerrainInfo> terrainSounds;
	
	typedef std::unordered_map< int, CPtr<CMOSelectable> > CSpecialGroup;
	CSpecialGroup sgFighters;
	CSpecialGroup sgGroundAttackPlanes;
	CSpecialGroup sgReconPlanes;
	CSpecialGroup sgSuperWeapons;

	CPtr<IScenarioTracker> pScenarioTracker;
	
	bool bUISelectionMode; // don't save
	bool bUISelectionDir; // don't save
	CVec2 vUISelectionStartDir; // don't save
	CVec2 vUISelectionMovePos; // don't save
	std::vector<int> uiVisSelections; // don't save

	typedef std::unordered_map< int, CPtr<CMapObj> > CObjMap;
	CObjMap ownAvia;
	
	struct SUnitIcon
	{
		ZDATA
		CPtr<IMOUnit> pMOUnit;
		float fLastTime;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pMOUnit); f.Add(3,&fLastTime); return 0; }
	};
	typedef std::unordered_map< int, SUnitIcon > CUnitIconMap;
	CUnitIconMap unitIcons;
	
	std::vector<CVec4> minimapColors;
	bool bIsOwnUnitsPresent;

	std::vector< std::unordered_map< int, int > > xrayUnits; // vector by players < id, time >, don't save
	NTimer::STime timeAbs; // don't save
	
	CPtr<IMissionSuperWeapon> pSuperWeapon;
private:
	void MsgScrollMap( const SGameMessage &msg );
	
	void MsgStartSelection( const SGameMessage &msg );
	void MsgUpdateSelection( const SGameMessage &msg );
	void MsgEndSelection( const SGameMessage &msg );
	void MsgCancelSelection( const SGameMessage &msg );
	
	void MsgSetTarget( const SGameMessage &msg );
	void MsgResetTarget( const SGameMessage &msg );
	void MsgUpdateDirection( const SGameMessage &msg );
	void MsgSetDestination( const SGameMessage &msg );
	void MsgSetDirection( const SGameMessage &msg );
	void MsgCancelDirection( const SGameMessage &msg );
	void MsgDoAction( const SGameMessage &msg );

	void MsgUpdateSelectedUnit( const SGameMessage &msg );

	void MsgSetSpecialAbility( const SGameMessage &msg );
	
//	bool MsgShiftDown( const SGameMessage &msg )	{ SetPlaceInQueue( true ); return false; }
//	bool MsgShiftUp( const SGameMessage &msg )	{ SetPlaceInQueue( false ); return false; }
	
	void MsgAreasUp( const SGameMessage &msg );
	void MsgAreasDown( const SGameMessage &msg );
	void MsgRangesUp( const SGameMessage &msg );
	void MsgRangesDown( const SGameMessage &msg );

	void MsgUserActionAttack( const SGameMessage &msg );
	void MsgUserActionRotate( const SGameMessage &msg );
	void MsgUserActionStop( const SGameMessage &msg );

	void MsgUnloadUnit( const SGameMessage &msg );
	void MsgSelectUnits( const SGameMessage &msg );
	void MsgSelectNextGroup( const SGameMessage &msg );
	void MsgSelectPrevGroup( const SGameMessage &msg );

	void MsgMinimapDown( const SGameMessage &msg );
	void MsgMinimapMove( const SGameMessage &msg );
	void MsgMinimapUp( const SGameMessage &msg );
	void MsgSetDestinationMinimap( const SGameMessage &msg );
	void MsgSetTargetMinimap( const SGameMessage &msg );
	void MsgAssignSelectionGroup( const SGameMessage &msg, int nParam );
	void MsgRestoreSelectionGroup( const SGameMessage &msg, int nParam );
	void MsgCenterSelectionGroup( const SGameMessage &msg, int nParam );
	void MsgCenterCurrentSelection( const SGameMessage &msg );
	
	void UpdateCursorObjects( const CVec2 &vPos );
	bool IsObstacleObject( CMapObj *pMO ) const;
	
	void PlaceMapCommandAck( const CVec2 &vTarget );
	void PlaceMapCommandAckDir( const CVec2 &vTarget );
	
	bool IsFighter( CMapObj *pMO ) const;
	bool IsGroundAttackPlane( CMapObj *pMO ) const;
	bool IsReconPlane( CMapObj *pMO ) const;
	bool IsSuperWeapon( CMapObj *pMO ) const;
	bool IsAviation( CMapObj *pMO ) const;
	// pMO == 0 - remove object
	void UpdateSpecialGroups( int nID, CMapObj *pMO );
	
	void CenterSelectionGroupPrivate( const std::vector<CMOSelectable*> &group );

	void OnSelectSlot( int nSlot, WORD wKeyboardFlags );
	void OnActionOnSlot( int nSlot, WORD wKeyboardFlags );
	
	void ShowSelectionDst( const CVec2 &vTarget );
	void ShowSelectionDst( SUISelection *pUISelection, const CVec2 &vMovePos, 
		const CVec2 &vStartDirPos, const CVec2 &vFinishDirPos, bool bFadeOut );
	void ClearSelectionDst();
	void GotoSelectionDst( SUISelection *pUISelection );

	enum ENotifyParams
	{
		ENP_NONE							= 0,
		ENP_CAMERA_POS				= 1, // pParam->vCenterCamera
		ENP_UNITS_POS					= 2,
		ENP_IDS								= 4, // vector<int> - misc data
		ENP_CAMERA_POS_SIMPLE	= 8, // CVec2( LOWORD( pUpdate->info.nParam ), HIWORD( pUpdate->info.nParam )
		ENP_UNITS							= 16,
	};
	void OnNotifyEvent( const struct SAIFeedbackUpdate *pUpdate, NDb::ENotificationEventType eType,
		ENotifyParams eParams );
	void OnObjectiveMoved( const struct SAIFeedbackUpdate *pUpdate );

	void UpdateXRayUnits( int nDeltaTime );
	bool IsXRayMode() const;
protected:
	void SetCursor( NDb::EUserAction _eForcedAction );
	void SetForcedAction( NDb::EUserAction eForcedAction );
	NDb::EUserAction GetForcedAction() const { return eForcedAction; } 
	bool IsForcedAction() const { return GetForcedAction() != NDb::USER_ACTION_UNKNOWN; }
	
	NDb::EUserAction DetermineBestAutoAction( CMapObj *_pMO, bool bAltMode );
	NDb::EUserAction DetermineBestCtrlAction( CMapObj *_pMO );

	void RegisterUserAction( NDb::EUserAction nAction, DWORD flags, USER_ACTION pfnUserAction );

	bool PerformGroupAction( const struct SAIUnitCmd *pCommand, bool bPlaceInQueue );
	bool PerformGroupAction( EActionCommand eActionCommand, bool bPlaceInQueue );
	bool PerformGroupAction( EActionCommand eActionCommand, float fParam, bool bPlaceInQueue );
	bool PerformGroupAction( EActionCommand eActionCommand, int nObjectID, bool bPlaceInQueue );
	bool PerformGroupAction( EActionCommand eActionCommand, int nObjectID, float fParam, bool bPlaceInQueue );
	bool PerformGroupAction( EActionCommand eActionCommand, const CVec2 &vPos, bool bPlaceInQueue );
	bool PerformGroupAction( EActionCommand eActionCommand, const CVec2 &vPos, float fParam, bool bPlaceInQueue );
	bool PerformGroupAction( EActionCommand eActionCommand, const CVec2 &vPos, int nObjectID, bool bPlaceInQueue );
	bool PerformGroupActionAutocast( const struct SAIUnitCmd *pCommand, bool bPlaceInQueue );

	USER_ACTION GetAction( NDb::EUserAction eUserAction, int nType );

	void ResetFirstCommandPoint()	{ bFirstCommandPointDef = false; }
	void SetFirstCommandPoint( const CVec2 &vPos )
	{
		vFirstCommandPoint = vPos;
		bFirstCommandPointDef= true;
	}
	const bool IsFirstCommandPointDefined() const { return bFirstCommandPointDef; }
	const CVec2 GetFirstCommandPoint() const { 
		NI_ASSERT( IsFirstCommandPointDefined(), "Trying recieve first point for command before its defenition." );
		return vFirstCommandPoint;
	}

	void DoAction( const CVec2 &vPos, bool bMiniMap, enum EKeyboardFlags eFlags );
	void DoAction( USER_ACTION pfnAction, NDb::EUserAction eUserAction, const CVec2 &vTarget, const CMapObj *pMO, 
		bool bSelfAction, bool bShiftState, bool bLikeForced );

//	void SetPlaceInQueue( bool bPlaceInQueue ) { bPlaceCommandInQuery = bPlaceInQueue; }
//	bool GetPlaceInQueue() const { return bPlaceCommandInQuery; }
	bool GetPlaceInQueue() const { return pMouseTranslator->IsShiftDown(); }

	void MsgResetForcedAction( const SGameMessage &msg );
	
	void InitPrivate();

	virtual void PlayerObjectiveChanged( const int nObjective, const EMissionObjectiveState eState );
	virtual void OnUpdateNotifyFeedback( const struct SAIFeedbackUpdate *pUpdate );
	void OnUpdateNewUnit( const SAINewUnitUpdate *pUpdate, CMapObj *pMO );
	void OnUpdateDiplomacy( CMapObj *pMO, const int nNewPlayer );
	void OnUpdateKeyObjectProgress( CMapObj *pMO, float fProgress, int nPlayer );
	int GetHPBarColorIndex( const CMapObj *pMO, const int nPlayer );
	void OnUpdateVisualStatus( const SUnitStatusUpdate *pUpdate, CMapObj *pMO );

	void OnUpdateSuperWeaponControl( const struct SSuperWeaponControl &update );
	void OnUpdateSuperWeaponRecycle( const struct SSuperWeaponRecycle &update );
	void OnReplaceSelectionGroup( CMOSelectable *pMOPattern, CMOSelectable *pMO );

	//{ CUpdatableWorld
	void OnNewMapObj( int nID, CMapObj *pMO );
	void OnDeadOrRemoveMapObj( int nID );
	//}
public:
	CWorldClient();
	CWorldClient( ITransceiver *pTransceiver, IVisualNotifications *pNotifications, IAILogic *pAI, 
		IScenarioTracker *pScenarioTracker, IMissionSuperWeapon *pSuperWeapon );
	~CWorldClient();

	void Update();
	//
	bool ProcessEvent( const struct SGameMessage &msg );
	bool PickMapObj( const CVec2 &vPos, std::list<int> *pPickObjects );
	CMapObj *PickTopMapObj( const CVec2 &vPos );
	// PickTopMapObj that can be actioned upon with eActionOn 
	CMapObj *PickTopMapObj( const CVec2 &vPos, const NDb::EUserAction eActionOn );

	// vPos - in AI coords
	bool IsInsideAIMap( const CVec2 &vPos );
	void ClampToAIMap( CVec2 &vPos );

  //
	void Select( CMapObj *pMapObj );
	void DeSelect( CMapObj *pMapObj );
	void DeSelectDead( CMapObj *pMapObj );
	bool IsActive( CMapObj *pMopObj );
	bool IsSuperActive( CMapObj *pMopObj );
	void DoUpdateSpecialAbility( CMapObj *pMO );
	void DoUpdateObjectStats( CMapObj *pMO );

	void RemoveFromSelectionGroup( CMapObj *pMopObj );
	void HideFromSelectionGroup( CMapObj *pMopObj );
	void UnHideFromSelectionGroup( CMapObj *pMopObj );
	void UpdateSpecialSelection( int nID, CMapObj *pMO );
	//
	bool ActionMove( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionReverse( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionAttack( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionRotate( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionMoveToGrid( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionSwarm( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionAmbush( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionFollow( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionStandGround( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionBoard( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionLeave( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionMechBoard( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	
	bool ActionLeaveOneSquad( const int nIndex );

	bool ActionInstall( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionUnInstall( const CVec2 &vPos, const CMapObj *pMO, bool bForced );

	bool ActionCaptureArtillery( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionHookArtillery( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionDeployArtillery( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	
	bool ActionEntrenchSelf( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionPlaceMines( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionClearMines( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionBuildEntrenchment( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionBuildFence( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionRepair( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionResupply( const CVec2 &vPos, const CMapObj *pMO, bool bForced );

	bool ActionChangeFormation0( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionChangeFormation1( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionChangeFormation2( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionChangeFormation3( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionChangeFormation4( const CVec2 &vPos, const CMapObj *pMO, bool bForced );

	bool ActionChangeShellDamage( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionChangeShellAgit( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionChangeShellSmoke( const CVec2 &vPos, const CMapObj *pMO, bool bForced );

	bool ActionStop( const CVec2 &vPos, const CMapObj *pMO, bool bForced );

	bool ActionCamoflage( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionAdvancedCamoflage( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionUseSpyGlass( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionUseThrow( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionUseLandMine( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionUseBlastingCharge( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionUseControlledCharge( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionDetonateControlledCharge( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionUseHoldSector( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionUseTrackTargeting( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionSupressFire( const CVec2 &vPos, const CMapObj *pMO, bool bForced );

	bool ActionCriticalTargetting( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionRapidFire( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionCaution( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionShootInMovement( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionCounterFire( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionSmokeShots( const CVec2 &vPos, const CMapObj *pMO, bool bForced );

	bool ActionDropBombs( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionExactShot( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionCoverFire( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionFirstAid( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionFillRU( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionZeroingIn( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionLinkedGrenades( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionSupportFire( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionPatrol( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionSpyMode( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionOverload( const CVec2 &vPos, const CMapObj *pMO, bool bForced );

	bool ActionGlobeBombMission( const CVec2 &vPos, const CMapObj *pMO, bool bForced );

	// call reinforcements
	bool ActionReinfCommon( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionReinfBomb( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionReinfParatroopers( const CVec2 &vPos, const CMapObj *pMO, bool bForced );
	bool ActionReinfNone( const CVec2 &vPos, const CMapObj *pMO, bool bForced );

	// super weapon
	bool ActionCallSuperWeapon( const CVec2 &vPos, const CMapObj *pMO, bool bForced );

	bool ActionPlaceMarker( const CVec2 &vPos, const CMapObj *pMO, bool bForced );

	void DoLButtonDown( const CVec2 &vPos );
	void DoLButtonUp( const CVec2 &vPos );
	void DoLButtonDblClk( const CVec2 &vPos );
	void DoRButtonDown( const CVec2 &vPos );
	void DoRButtonUp( const CVec2 &vPos );
	void DoRButtonDblClk( const CVec2 &vPos );
	void DoMouseMove( const CVec2 &vPos );
	//
	void LoadMap( const NDb::SMapInfo *pMapInfo );
	void AfterLoad( const NDb::SMapInfo *pMapInfo );
	void OnGetFocus( bool bFocus );
	//
	void OnLeave();
	void OnEnter();
	//
	void GetSelectionActions( CUserActions *pActions ) { return pSelector->GetActions( pActions ); }
	void GetSelectionDisabledActions( CUserActions *pActions ) { return pSelector->GetDisabledActions( pActions ); }
	void GetSelectionEnabledActions( CUserActions *pActions ) { return pSelector->GetEnabledActions( pActions ); }
	void SetMaxUnits( const int nMaxUnitSlots, const int nMaxUnitPerSlot ) { pSelector->SetMaxUnits( nMaxUnitSlots, nMaxUnitPerSlot ); }
	int GetAbilityTier( NDb::EUserAction eAction ) const { return pSelector->GetAbilityTier( eAction ); }
	const NDb::SUnitSpecialAblityDesc* GetAbilityDesc( NDb::EUserAction eAction ) const { return pSelector->GetAbilityDesc( eAction ); }
	//
	void ScreenToAI( CVec2 *pvAI, const CVec2 &vScreen );
	CVec3 ScreenToAI( const CVec2 &vScreen );
	// debug helpers
	void ToggleAIPassability( const SGameMessage &msg );
	void ToggleLockProfiles( const SGameMessage &msg );
	void PlayAllAnimations( const SGameMessage &msg );

	void MsgSetForcedAction( const SGameMessage &msg );
	void MsgSetForcedActionWithParam( const SGameMessage &msg, CObjectBase *pParam );
	
	bool IsCameraUpdated() const { return bCameraUpdated; }
	void ResetCameraUpdated() { bCameraUpdated = false; }

	const bool IsAreasShown() const { return bAreasShown; }
	void GetAreas( SShootAreas *pAreas ) { pSelector->GetAreas( pAreas ); }

	void UpdateIcons();

	int operator&( IBinSaver &saver );

	CCommandsSender* GetCommandsSender() const { return pCommandsSender.GetPtr(); }

	EActionMode GetActionMode() const { return eActionMode; }
	void SetActionMode( EActionMode eActionMode );
	
	bool IsOnMinimap() const { return bOnMinimap; }
	void SetOnMinimap( bool bOnMinimap );

	// for sound
	const NDb::SComplexSoundDesc * GetTerrainSound( int nTerrainType );
	const NDb::SComplexSoundDesc * GetTerrainCycleSound( int nTerrainType );
	void GetTerrainMassData( std::vector<SSoundTerrainInfo> *pData, int nMaxSize );
	float GetSoundVolume( int nTerrainType ) const;

	void OnClickMultiSelectUnit( int nSlot, WORD wKeyboardFlags );

	void OnSelectSpecialGroup( int nIndex );

	void CenterSelectedUnit();

	bool IsActiveOwnAvia() const;
	void GetOwnAvia( std::vector<CMapObj*> *pObjects ) const;

	void GetCameraByName( NCamera::CCameraPlacement *pCamera, const std::string &rszName ) const;
	void GetObjectPosByScriptID( CVec3 *pObjPos, int nScriptID ) const;
	bool GetMoviesData( NDb::SScriptMovies *pMoviesData ) const;

	void UpdateUnitIcons( float fDeltaTime );
	void SetMinimapColors( const std::vector<CVec4> &_minimapColors );
	
	bool IsOwnUnitsPresent() const { return bIsOwnUnitsPresent; }
	void GameResetForcedAction();
	void OnChangeForcedAction( NDb::EUserAction eOldForcedAction );
};


