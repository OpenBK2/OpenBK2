#pragma once

#include "MapObj.h"
#include "Notifications.h"

#include "../SceneB2/CameraInternal.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CAllAnimationsPlayer;
class CLaserMarkTrace;
interface ICommonB2M1AI;
typedef hash_map<int, NDb::SReinforcementPosition> CReinforcementPositions;
typedef list<CDBPtr<NDb::SReinforcement> > CEnabledReinforcements;
typedef hash_map<int, CPtr<CMapObj> > CKeyBuildings;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CCombatMusic
{
	enum ECombatMusicState 
	{
		ESSS_IDLE,
		ESSS_TO_COMBAT,
		ESSS_COMBAT,
		ESSS_TO_IDLE,
	};
	ZDATA
	NTimer::STime timeLastCombatNotify;
	ECombatMusicState eState;
	NTimer::STime combatPlayWONotify;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&timeLastCombatNotify); f.Add(3,&eState); f.Add(4,&combatPlayWONotify); return 0; }
public: 
	CCombatMusic();
	void Update( bool bCombat );
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CUpdatableWorld : public virtual CObjectBase, public IUpdatableClient
{
	typedef hash_map<int, ObjectFactoryNewFunc> CNewFuncsMap;
	CNewFuncsMap newFuncs;
	typedef hash_map<int, CObj<CMapObj> > CMapObjMap;
	CMapObjMap objects;
	NDb::ESeason eSeason;
	NDb::EDayNight eDayTime;					// needed to determine if headlights are on
	
	NTimer::STime currTime;

	// every segment update functions
	typedef void (CUpdatableWorld::*EVERY_SEGMENT_UPDATE)();
	typedef vector<EVERY_SEGMENT_UPDATE> CEverySegment;
	CEverySegment everySegment;

	// updates by type
	typedef void (CUpdatableWorld::*UPDATE_TYPE)( const SAIBasicUpdate *pUpdate );
	typedef hash_map<int, UPDATE_TYPE> CUpdateType;
public:
	CUpdateType updateType;
private:
	// Temporary vis objects for building (IDs)
	typedef vector<int> CTempMapObjList;
	CTempMapObjList ghostObjects;

	// for animations testing
	CPtr<CAllAnimationsPlayer> pAllAnimationsPlayer;
	
	int nLastRangeAreasTime;

	typedef hash_map<int /*UnitID*/, NTimer::STime> CWaitingCorpses;

	CWaitingCorpses graveyard;

	CPtr<IVisualNotifications> pNotifications;
	CPtr<ICommonB2M1AI> pAI;
	
	// reinforcements
	CReinforcementPositions reinforcementPositions;
	CEnabledReinforcements enabledReinforcements;
	bool bReinfEnabled;
	NTimer::STime reinfTimeRecycleStart; // момент времени, с которого ждать разрешения на следующий вызов подкреплений
	NTimer::STime reinfTimeRecycleEnd; // промежуток времени, через который будут разрешены подкрепления
	float fReinfRecycleProgress;
	int nReinfCallsLeft;
	CKeyBuildings keyBuildings;
	CCombatMusic combatMusic;

	hash_map<int, CObj<CLaserMarkTrace> > laserMarks;
	hash_map<int, CObj<CObjectBase> > laserMarksMeshes;

#ifndef _FINALRELEASE
	vector<CObj<CPtrFuncBase<NGScene::CObjectInfo> > > badObjects;
	vector<CObj<CObjectBase> > badObjectsMeshes;
#endif
private:
	void InitPrivate();

	void InitEverySegmentFunctions();
	void InitUpdateTypeFunctions();
	// EVERY_SEGMENT_UPDATE
	void UpdateBasicUpdates();
	void UpdateAcknowledgemets();
	void UpdatePlayAllAnimations();
	void UpdateClientData();

	void AIUpdateCombatSituationInfo();

	// UPDATE_TYPE
	void AIUpdateAreas( const SAIBasicUpdate *_pUpdate );

	void UpdateDeadPlane( const SAIBasicUpdate * _pUpdate );
	void UpdateNewUnits( const SAIBasicUpdate *_pUpdate );
	void UpdateNewBridgeSpan( const SAIBasicUpdate *_pUpdate );
	void UpdatePlacement( const SAIBasicUpdate *_pUpdate );
	void UpdateRPGChanged( const SAIBasicUpdate *_pUpdate );
	void UpdateSelection( const SAIBasicUpdate *_pUpdate );
	void UpdateSideChanged( const SAIBasicUpdate *_pUpdate );
	void UpdateNewFormation( const SAIBasicUpdate * _pUpdate );
	void UpdateEntranceState( const SAIBasicUpdate * _pUpdate );
	void UpdateDeadProjectile( const SAIBasicUpdate * _pUpdate );
	void UpdateTurretTurn( const SAIBasicUpdate * _pUpdate );
	void UpdateMechShoot( const SAIBasicUpdate * _pUpdate );
	void UpdateInfantryShoot( const SAIBasicUpdate * _pUpdate );
	void UpdateDiplomacy( const SAIBasicUpdate * _pUpdate );
	void UpdateChageDBID( const SAIBasicUpdate * _pUpdate );

	void UpdateShootBuilding( const SAIBasicUpdate * _pUpdate );
	void UpdateThrowbuilding( const SAIBasicUpdate * _pUpdate );
	void UpdateHit( const SAIBasicUpdate * _pUpdate );
	void UpdateDeleteObject( const SAIBasicUpdate * _pUpdate );
	void UpdateSilentDeath( const SAIBasicUpdate * _pUpdate );
	void UpdateDissapearObj( const SAIBasicUpdate * _pUpdate );
	void UpdateRevealArtillery( const SAIBasicUpdate * _pUpdate );
	void UpdateSetCamouflage( const SAIBasicUpdate * _pUpdate );
	void UpdateRemoveCamouflage( const SAIBasicUpdate * _pUpdate );
	void UpdateSetAmbush( const SAIBasicUpdate * _pUpdate );
	void UpdateRemoveAmbush( const SAIBasicUpdate * _pUpdate );
	void UpdateBreakTrack( const SAIBasicUpdate * _pUpdate );
	void UpdateRepairTrack( const SAIBasicUpdate * _pUpdate );
	void UpdateRepairStateBegin( const SAIBasicUpdate * _pUpdate );
	void UpdateRepairStateEnd( const SAIBasicUpdate * _pUpdate );
	void UpdateResuplyStateBegin( const SAIBasicUpdate * _pUpdate );
	void UpdateResuplyStateEnd( const SAIBasicUpdate * _pUpdate );
	void UpdateChangeVisibility( const SAIBasicUpdate * _pUpdate );
	void UpdateStateChanged( const SAIBasicUpdate * _pUpdate );
	void UpdateServedArtillery( const SAIBasicUpdate * _pUpdate );
	//void UpdateSelectChecked( const SAIBasicUpdate * _pUpdate );
	void UpdateSelectionGroup( const SAIBasicUpdate * _pUpdate );
	void UpdateNotifyFeedback( const SAIBasicUpdate * _pUpdate );
	void UpdateShootAreas( const SAIBasicUpdate *_pUpdate );
	void UpdateRangeAreas( const SAIBasicUpdate *_pUpdate );

	void UpdateSpecialAbility( const SAIBasicUpdate * _pUpdate );

	void UpdateTreeBroken( const SAIBasicUpdate * _pUpdate );

	void UpdateDeadUnit( const SAIBasicUpdate * _pUpdate );
	void UpdatePreDisappear( const SAIBasicUpdate * _pUpdate );
	void UpdateNewEntrenchment( const SAIBasicUpdate *_pUpdate );
	void UpdateAnimationChanged( const SAIBasicUpdate *_pUpdate );	
	
	void UpdateModifyEntranceState( const SAIBasicUpdate *pUpdate );
	// helper functions
	void UpdateSelectable( const SAIBasicUpdate * _pUpdate );

	void UpdateDelayedShoot( const SAIBasicUpdate * _pUpdate );
	void UpdateStop( const SAIBasicUpdate * _pUpdate );
	void UpdateMove( const SAIBasicUpdate * _pUpdate );
	void UpdateIdleTrench( const SAIBasicUpdate * _pUpdate );

	void UpdateReinfTypeAvail( const SAIBasicUpdate * _pUpdate );
	void UpdateReinfRecycle( const SAIBasicUpdate * _pUpdate );
	void UpdateReinforcmentPoint( const SAIBasicUpdate * _pUpdate );
	void SendUpdateReinfPoints( int nFactoryID );
	//
	void UpdateAction( const SAIBasicUpdate * _pUpdate );
	void UpdateObjectsUnderConstruction( const SAIBasicUpdate * _pUpdate );

	void UpdateKeyBuildingCaptured( const SAIBasicUpdate * _pUpdate );
	void UpdateKeyBuildingLost( const SAIBasicUpdate * _pUpdate );
	void UpdateNewKeyBuilding( const SAIBasicUpdate * _pUpdate );
	void UpdateKeyBuildingCaptureProgress( const SAIBasicUpdate * _pUpdate );

	void UpdateSwitchLightFX( const SAIBasicUpdate * _pUpdate );
	//
	void UpdateStartFinishParadrop( const SAIBasicUpdate * _pUpdate );

	void UpdateDamage( const SAIBasicUpdate * _pUpdate );

	void UpdateScriptCameraRun( const SAIBasicUpdate * _pUpdate );
	void UpdateScriptCameraReset( const SAIBasicUpdate * _pUpdate );

	void UpdateScriptCameraStartMovie( const SAIBasicUpdate * _pUpdate );
	void UpdateScriptCameraStopMovie( const SAIBasicUpdate * _pUpdate );

	void UpdateWeatherChanged( const SAIBasicUpdate * _pUpdate );
	void UpdatePlaneReturns( const SAIBasicUpdate * _pUpdate );

	void UpdateParentOfAtomObj( const SAIBasicUpdate *pUpdate );

	void CreateNewObject( const int nUniquieID, const int nTypeID, const SAIBasicUpdate *_pUpdate );

	void UpdateNotifyDisableAction( const SAIBasicUpdate *pUpdate );
	void UpdateNotifyEnableAction( const SAIBasicUpdate *pUpdate );

	void UpdatePlayEffect( const SAIBasicUpdate *pUpdate );
	void UpdateStatus( const SAIBasicUpdate *pUpdate );

	void UpdateSuperWeaponControl( const SAIBasicUpdate *_pUpdate );
	void UpdateSuperWeaponRecycle( const SAIBasicUpdate *_pUpdate );
	void UpdateDiveBomberDive( const SAIBasicUpdate *_pUpdate );
protected:
	void AddMapObj( int nID, CMapObj *pMO );
	void RemoveMapObj( int nID, bool bGlobalRemove );
	
	virtual void OnNewMapObj( int nID, CMapObj *pMO ) {}
	virtual void OnDeadOrRemoveMapObj( int nID ) {}
	//
	const NDb::ESeason GetSeason() const { return eSeason; }
	const NDb::EDayNight GetDayTime() const { return eDayTime; }
	//
	virtual void PlayerObjectiveChanged( const int nObjective, const enum EMissionObjectiveState eState ) = 0;
	IVisualNotifications* GetNotifications() const { return pNotifications; }
	
	virtual void OnUpdateNotifyFeedback( const struct SAIFeedbackUpdate *pUpdate ) {}
	virtual void OnUpdateNewUnit( const SAINewUnitUpdate *pUpdate, CMapObj *pMO ) {}
	virtual void OnUpdateDiplomacy( CMapObj *pMO, const int nNewPlayer ) = 0;
	virtual void OnUpdateKeyObjectProgress( CMapObj *pMO, float fProgress, int nPlayer ) {}
	virtual void OnUpdateVisualStatus( const SUnitStatusUpdate *pUpdate, CMapObj *pMO ) {}

	virtual void OnUpdateSuperWeaponControl( const struct SSuperWeaponControl &update ) {}
	virtual void OnUpdateSuperWeaponRecycle( const struct SSuperWeaponRecycle &update ) {}
	virtual void OnReplaceSelectionGroup( CMOSelectable *pMOPattern, CMOSelectable *pMO ) {}

	void ObjectPlayAttachedEffect( const SAIBasicUpdate *_pUpdate );	
	void ObjectStopAttachedEffect( const SAIBasicUpdate *_pUpdate );	

	//
	CUpdatableWorld();
	CUpdatableWorld( IVisualNotifications *pNotifications, ICommonB2M1AI *pAI );
	~CUpdatableWorld();
public:
	bool bEditor;
	void SetSeason( const NDb::ESeason _eSeason, const NDb::EDayNight _eDayTime ) { eSeason = _eSeason; eDayTime = _eDayTime; }
	void AfterLoad();
	
	virtual void Update();
	//
	CMapObj* GetMapObj( int nID ) const
	{
		CMapObjMap::const_iterator pos = objects.find( nID );
		return pos == objects.end() ? 0 : pos->second;
	}
	//
	virtual void Select( CMapObj *pMapObj ) = 0;
	virtual void DeSelect( CMapObj *pMapObj ) = 0;
	virtual void DeSelectDead( CMapObj *pMapObj ) = 0;
	virtual void RemoveFromSelectionGroup( CMapObj *pMopObj ) = 0;
	virtual void HideFromSelectionGroup( CMapObj *pMapObj ) { RemoveFromSelectionGroup( pMapObj ); } 
	virtual void UnHideFromSelectionGroup( CMapObj *pMapObj ) {} 
	virtual void UpdateSpecialSelection( int nID, CMapObj *pMO ) {}
	virtual bool IsActive( CMapObj *pMopObj ) = 0;
	virtual bool IsSuperActive( CMapObj *pMopObj ) = 0;
	virtual void DoUpdateSpecialAbility( CMapObj *pMO ) = 0;
	virtual void DoUpdateObjectStats( CMapObj *pMO ) = 0;

	//
	int operator&( IBinSaver &saver );

	void PlayAllObjectsAnimations();
	//
	// updates
	virtual void ProcessUpdate( const struct SAIPointLightUpdate *pUpdate );
	virtual void ProcessUpdate( const struct SAIHeadLightUpdate *pUpdate );
	virtual void ProcessUpdate( const struct SAIToggleDayNightWindowsUpdate *pUpdate );
	virtual void ProcessUpdate( const struct SAIBreakWindowUpdate *pUpdate );
	virtual void ProcessUpdate( const struct SAINewProjectileUpdate *pUpdate );
	virtual void ProcessUpdate( const struct SAIDeadProjectileUpdate *pUpdate );
//	virtual void ProcessUpdate( const struct SAINewProjectileM1 *pUpdate );
	virtual void ProcessUpdate( const struct SExplodeProjectileUpdate *pUpdate );
	virtual void ProcessUpdate( const struct SScriptCameraRunUpdate *pUpdate ) {};
	virtual void ProcessUpdate( const struct SScriptCameraResetUpdate *pUpdate ) {};
	virtual void ProcessUpdate( const struct SScriptCameraStartMovieUpdate *pUpdate ) {};
	virtual void ProcessUpdate( const struct SScriptCameraStopMovieUpdate *pUpdate ) {};
	virtual void ProcessUpdate( const struct SClientUpdateButtonsUpdate *pUpdate );
	virtual void ProcessUpdate( const struct SClientUpdateSingleUnitUpdate *pUpdate );
	virtual void ProcessUpdate( const struct SWeatherChangedUpdate *pUpdate ) {};
	virtual void ProcessUpdate( const struct SLaserMarkUpdate *pUpdate );
	virtual void ProcessUpdate( const struct SChatMessageUpdate *pUpdate );
	virtual void ProcessUpdate( const struct SWinLoseUpdate *pUpdate );
	virtual void ProcessUpdate( const struct SStartStopSequenceUpdate *pUpdate );
	virtual void ProcessUpdate( const struct SMSChangePlaylistUpdate *pUpdate );
//	virtual void ProcessUpdate( const struct SMSPlayVoiceUpdate *pUpdate );
	virtual void ProcessUpdate( const struct SMSSetVolumeUpdate *pUpdate );
	virtual void ProcessUpdate( const struct SMSPauseMusicUpdate *pUpdate );
	virtual void ProcessUpdate( const struct SMoneyChangedUpdate *pUpdate );
	virtual void ProcessUpdate( const struct SObjectiveChanged *pUpdate );
	virtual void ProcessUpdate( const struct SEnableAirStrike *pUpdate );
	virtual void ProcessUpdate( const struct SMemCameraPos *pUpdate ) { }
	virtual void ProcessUpdate( const struct SSetCameraToMemPos *pUpdate ) { }
	virtual void ProcessUpdate( const struct SChatClear *pUpdate );

	void RegisterProcess( IClientUpdatableProcess *pProcess );

	void PerformUpdate( CObjectBase *pRawUpdate );

	// reinforcements
	const CEnabledReinforcements& GetEnabledReinfs() const { return enabledReinforcements; }
	const CReinforcementPositions& GetReinfPositions() const { return reinforcementPositions; }
	bool IsReinfEnabled() const { return bReinfEnabled; }
	const float GetReinfRecycleProgress() const { return fReinfRecycleProgress; }
	NTimer::STime GetReinfRecycleEnd() const { return reinfTimeRecycleEnd; }
	int GetReinfCallsLeft() const { return nReinfCallsLeft; }
	const CKeyBuildings& GetKeyBuildings() const { return keyBuildings; }

	// script cameras
	virtual void GetCameraByName( NCamera::CCameraPlacement *pCamera, const string &rszName ) const {}
	virtual void GetObjectPosByScriptID( CVec3 *pObjPos, int nScriptID ) const {}
	virtual bool GetMoviesData( NDb::SScriptMovies *pMoviesData ) const { return false; }

	void GetObjects( list<int> *pObjects ) const;
	void GetObjects( vector<IB2MapObj*> *pObjects ) const;
	void GetObjects( vector<CMapObj*> *pObjects ) const;
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
