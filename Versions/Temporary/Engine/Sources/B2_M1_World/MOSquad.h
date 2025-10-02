#pragma once
#include "mapobj.h"

class CMOSquad : public IMOSquad
{
  OBJECT_NOCOPY_METHODS( CMOSquad );
  
  ZDATA_(IMOSquad)
	CUnitsList units;
	CPtr<IMOUnit> pServedGun;
	ZSKIP //bool bCanCatchArtillery;
	CPtr<IMOContainer> pContainer;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(IMOSquad*)this); f.Add(2,&units); f.Add(3,&pServedGun); f.Add(5,&pContainer); return 0; }
private:
	bool IsInSquad( interface IMOUnit *pUnit );
	void UpdateServedGunCrew( const bool bClearCrew );
public:
	CMOSquad();
	~CMOSquad(void);

	void AIUpdatePlacement( const struct SAINotifyPlacement &placement, interface IScene *pScene, interface ISoundScene *pSoundScene, NDb::ESeason eSeason ) {}
	virtual IClientUpdatableProcess* AIUpdateRPGStats( const struct SAINotifyRPGStats &stats, interface IClientAckManager *pAckManager, NDb::ESeason eSeason ) { return 0; }

	virtual bool Create( const int nUniqueID, const SAIBasicUpdate *_pUpdate, NDb::ESeason eSeason, const NDb::EDayNight eDayTime, bool bInEditor );
	bool CreateSceneObject( const int nUniqueID, const SAINewUnitUpdate *pUpdate, NDb::ESeason eSeason, bool bInEditor ) { return true; }
	//void GetSpecialAbilities( CSpecialAbilities *pAbilities ) const;
	bool AIUpdateSpecialAbility( const struct SAISpecialAbilityUpdate &update );
	void GetStatus( SObjectStatus *pStatus ) const;
	// load unit onboard or unload it
	bool Load( interface IMOUnit *pUnit, bool bEnter );
	bool LoadSquad( interface IMOSquad *pSquad, bool bEnter ) { return false; }
	// show icons of the passangers
	void UpdatePassangers() {}
	void GetPassangers( vector<CMOSelectable*> *pBuffer ) const;
	int GetPassangersCount() const { return units.size(); }
	// get free places
	int GetFreePlaces() const { return 0; }
	int GetFreeMechPlaces() const { return 0; }
	// firing... (from container of by himself)
	void AIUpdateShot( const struct SAINotifyBaseShot &shot, const NTimer::STime &currTime, IScene *pScene, NDb::ESeason eSeason ) { }
	//
	virtual bool NeedShowInterrior() const;
	virtual void SendAcknowledgement( interface IClientAckManager *pAckManager, const NDb::EUnitAckType eAck );
	//
	void GetActions( CUserActions *pActions, EActionsType eActions ) const;
	void GetPossibleActions( CUserActions *pActions ) const;
	void GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const;
	void GetEnabledActions( CUserActions *pActions, EActionsType eActions ) const;
	int GetAbilityTier( NDb::EUserAction eAction ) const;
	void GetAbilityInfo( CAbilityInfo &abilityList ) const;
	virtual void Select( bool bSelect );
	void SetSelectionGroup( int nSelectionGroup );

	void AIUpdateModifyEntranceState( bool bOpen ) {}
	bool IsOpen() const { return true; }

	void AIUpdateDissapear( const SAIDissapearObjUpdate *pUpdate, interface ISoundScene *pSoundScene, IClientAckManager *pAckManager );
	void AIUpdateServedArtillery( IMOUnit *pMOUnit );
	
	IMOUnit* GetServedGun() const { return pServedGun; }

	bool AIUpdateAction( bool bEnable, EActionCommand eAction );
	virtual bool IsSquad() const { return true; }

	float GetVisualHPFraction() const;
	float GetVisualHPFractionSmooth() const;
	
	bool IsFirstUnit( const IMOUnit *pUnit ) const;
	void FillIconsInfoForFirstUnit( SSceneObjIconInfo &iconInfo );
	void UpdateSquadIcons();
	void SetContainer( IMOContainer *_pContainer ) { pContainer = _pContainer; }
	IMOContainer* GetContainer() const { return pContainer; }

	// Инициализирует параметры, зависящие только от базы.
	static void InitByDB();
};


