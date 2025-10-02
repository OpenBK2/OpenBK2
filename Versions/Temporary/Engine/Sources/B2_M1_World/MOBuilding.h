#pragma once

#include "MapObj.h"

class CMOBuilding : public IMOContainer
{
	OBJECT_NOCOPY_METHODS( CMOBuilding );

	vector< CPtr<CMOSelectable> > vPassangers;

	enum EWindowState { EWS_NONE, EWS_DAY, EWS_NIGHT, EWS_DESTROYED };
	vector<EWindowState> attachedWindows;
	typedef list< pair< int, int > > CAttachedObjIDs;
	vector< CAttachedObjIDs > attachedObjects;
	vector< float > attachedObjectsHP;
	float fMaxDistance;
	float fBuildingHP;
	WORD wAmbientSound;
	WORD wCycledSound;
	//WORD wAmbientSoundTimed;
	WORD wCycledSoundTimed;
	bool bStorage;

	hash_set<int> projectilesAlreadyHit;
	int nOldModelStage;
	int nCurrentAmmo;
	float fCapturingProgress;
	int nCapturingColorIndex;
	bool bCanEnter;
private:
	bool IsInside( const int nID );
	const NDb::SBuildingRPGStats* GetStats() const;

	const NDb::SVisObj* GetWindowObj( const int nWindow, const EWindowState eState );
	void DetachWindow( const int nWindow, const NDb::ESeason eSeason );
	const int AttachWindow( const int nWindow, const NDb::ESeason eSeason, const EWindowState eState );
	void AddDynamicDebris( const NDb::SModel *pDeadModel, const CVec3 &vPos, const CQuat &qRot, NDb::ESeason eSeason );
	void AttachEffectToSlot( const int nSlot, const ESceneSubObjType eType, const NDb::SEffect *pEffect, const NTimer::STime _time, const bool bTurnWithWind, const bool bRemoveObject );
	void AttachComplexEffectToSlot( const int nSlot, const ESceneSubObjType eType, const NDb::SComplexEffect *pEffect, const NTimer::STime _time, const bool bRemoveObject );
	const int AttachSubObjectToSlot( const int nSlot, const ESceneSubObjType eType, const NDb::SModel *pSubModel );
	void RemoveSubObjectsFromSlot( const int nSlot, const ESceneSubObjType eType );
	const int GetSlotHPIndex( const int nWindow, const EWindowState eState ) const;
	bool IsHitbarVisible() const;
protected:
	void FillIconsInfo( SSceneObjIconInfo &iconInfo );
	NDb::EUserAction GetBestAutoAction( const CUserActions &actionsBy, CUserActions *pActionsWith, bool bAltMode ) const;
public:
	CMOBuilding();
	//
	virtual bool Create( const int nUniqueID, const SAIBasicUpdate *pUpdate, NDb::ESeason eSeason, const NDb::EDayNight eDayTime, bool bInEditor );
	bool CreateSceneObject( const int nUniqueID, const SAINewUnitUpdate *pUpdate, NDb::ESeason eSeason, bool bInEditor );
	void GetStatus( SObjectStatus *pStatus ) const;
	IClientUpdatableProcess* AIUpdateRPGStats( const SAINotifyRPGStats &stats, struct IClientAckManager *pAckManager, NDb::ESeason eSeason );

	virtual IClientUpdatableProcess* AIUpdateDamage( int nProjectileID, float fDamage, const list<int> &probableHitAttached, struct IScene *pScene, NDb::ESeason eSeason, bool bFromAIUpdate );
	void AIUpdateKeyObject( const struct SAINotifyKeyBuilding &update );
	void AIUpdateKeyObjectCaptureProgress( float fProgress, int nColorIndex );

	// Create day/night dependent stuff (windows)
	virtual void SwitchLightFX( const bool bNewState, const NDb::ESeason eSeason, const bool bIsNight, const bool bInEditor = false );

	// break window
	const int BreakWindow( const int nWindow, const NDb::ESeason eSeason );
	void BreakAllWindows( const NDb::ESeason eSeason );
	// on/of night windows
	void ToggleNightWindows( const bool bNightOn, const NDb::ESeason eSeason );
	//
	bool Load( struct IMOUnit *pMO, bool bEnter ) { return false; }
	bool LoadSquad( struct IMOSquad *pSquad, bool bEnter );
	void UpdatePassangers() { }
	void GetPassangers( vector<CMOSelectable*> *pBuffer ) const;

	int GetPassangersCount() const { return vPassangers.size(); }

	int GetFreePlaces() const;
	int GetFreeMechPlaces() const { return 0; }
	void AIUpdateShot( const struct SAINotifyBaseShot &shot, const NTimer::STime &currTime, IScene *pScene, NDb::ESeason eSeason );
	//
	virtual bool NeedShowInterrior() const;
	//
	void GetActions( CUserActions *pActions, EActionsType eActions ) const;
	void GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const;
	//void GetSpecialAbilities( CSpecialAbilities *pAbilities ) const {}
  //
  virtual void Select( bool bSelect );
	void SetCanSelect( bool bCanSelect );

	void AIUpdateModifyEntranceState( bool _bOpen ) { bCanEnter = _bOpen; }
	virtual void AIUpdatePlacement( const struct SAINotifyPlacement &placement, struct IScene *pScene, struct ISoundScene *pSoundScene, NDb::ESeason eSeason );
	bool IsOpen() const { return true; }
	
	bool IsStorage() const { return bStorage; }
	virtual bool IsBuilding() const { return true; }
	//
	int operator&( IBinSaver &saver );
};


