#pragma once

#include "MOUnit.h"

class CMOUnitInfantry: public CMOUnit
{
	OBJECT_NOCOPY_METHODS( CMOUnitInfantry );
	//
	ZDATA_(CMOUnit)
		CPtr<IMOSquad> pSquad;
		CPtr<IMOContainer> pTransport;
		int nParachuteID;
		bool bEntrenched;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CMOUnit*)this); f.Add(2,&pSquad); f.Add(3,&pTransport); f.Add(4,&nParachuteID); f.Add(5,&bEntrenched); return 0; }
private:
	const NDb::SInfantryRPGStats* GetStatsLocal() const { return checked_cast<const NDb::SInfantryRPGStats*>( GetStats() ); }
	//
	IClientUpdatableProcess *StartParadrop( const SParadropStartFinishUpdate *pUpdate, NDb::ESeason eSeason );
	IClientUpdatableProcess *FinishParadrop( const SParadropStartFinishUpdate *pUpdate, NDb::ESeason eSeason );
	bool IsFirstUnit() const;
	void GetActionsBy( CUserActions *pActions ) const;
protected:
	virtual void InitAttached( const NDb::ESeason eSeason, IChooseAttached *pChooseFunc );
	void FillIconsInfo( SSceneObjIconInfo &iconInfo );
	void SetVisible( const bool bVisible, const NDb::ESeason eSeason, const bool bIsNight );
public:
	CMOUnitInfantry(): nParachuteID( -1 ), bEntrenched( false ) {}
	//
	bool Create( const int nUniqueID, const SAIBasicUpdate *pUpdate, NDb::ESeason eSeason, const NDb::EDayNight eDayTime, bool bInEditor );
	bool CreateSceneObject( const int nUniqueID, const SAINewUnitUpdate *pUpdate, NDb::ESeason eSeason, bool bInEditor );
	void ChangeRPGStats( const struct SAIChangeDBIDUpdate &update, const NDb::ESeason eSeason );
	void GetStatus( SObjectStatus *pStatus ) const;

	void SetSquad( struct IMOSquad *_pSquad );
	struct IMOSquad* GetSquad() const { return pSquad; }
	virtual IB2MapObj* GetSquadB2MapObj() const { return pSquad; }
	//
	bool Load( struct IMOUnit *pMO, bool bEnter ) { return false; }
	bool LoadSquad( struct IMOSquad *pSquad, bool bEnter ) { return false; }
	void UpdatePassangers() { }
	void GetPassangers( std::vector<CMOSelectable*> *pBuffer ) const {}
	int GetPassangersCount() const { return 0; }
	int GetFreePlaces() const { return 0; }
	int GetFreeMechPlaces() const { return 0; }
	void AIUpdateShot( const struct SAINotifyBaseShot &shot, const NTimer::STime &currTime, IScene *pScene, NDb::ESeason eSeason );
	void AIUpdatePlacement( const struct SAINotifyPlacement &placement, struct IScene *pScene, struct ISoundScene *pSoundScene, NDb::ESeason eSeason );
	//
	void GetActions( CUserActions *pActions, EActionsType eActions ) const;
	void GetPossibleActions( CUserActions *pActions ) const;
	void GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const	{}
	void AIUpdateDeadUnit( const SAIDeadUnitUpdate *pUpdate, const NDb::ESeason eSeason, const bool bIsNight, struct ISoundScene *pSoundScene, struct IClientAckManager *pAckManager );
	void AIUpdateDissapear( const SAIDissapearObjUpdate *pUpdate, struct ISoundScene *pSoundScene, IClientAckManager *pAckManager );
	void AIUpdateAction( const SAIActionUpdate *pUpdate, const NDb::ESeason eSeason );
	class CMOProjectile* LaunchProjectile( const SAINewProjectileUpdate *pUpdate );
	void SetTransport( IMOContainer *_pTransport );
	IMOContainer* GetTransport() { return pTransport; }
	//
	IClientUpdatableProcess *AIUpdateStartFinishParadrop( const SParadropStartFinishUpdate *pUpdate, NDb::ESeason eSeason );
	void HideParachute();
	void SetupWeapon( NDb::ESeason eSeason );
	virtual bool IsInfantry() const { return true; }

  virtual void Select( bool bSelect );

	const CVec3 GetFirePoint( const int nPlatform, const int nGun ) const;
	void AddShotTrace( const CDBPtr<NDb::SWeaponRPGStats> pWeapon, const struct SAINotifyBaseShot &shot, const NTimer::STime &currTime, IScene *pScene );

  bool IsVisible() const;
  
  void SetEntrench( bool bEntrench );
	IMOContainer* GetContainer() const { return pTransport ? pTransport.GetPtr() : pSquad.GetPtr(); }
};


