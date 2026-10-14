#pragma once
#include "MapObj.h"

class CMOSquad : public IMOSquad
{
	OBJECT_NOCOPY_METHODS( CMOSquad );

	ZDATA_( IMOSquad )
	CUnitsList units;
	CPtr<IMOUnit> pServedGun;
	ZSKIP //bool bCanCatchArtillery;
			CPtr<IMOContainer>
					pContainer;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (IMOSquad *)this );
		f.Add( 2, &units );
		f.Add( 3, &pServedGun );
		f.Add( 5, &pContainer );
		return 0;
	}

private:
	bool IsInSquad( struct IMOUnit *pUnit );
	void UpdateServedGunCrew( const bool bClearCrew );

public:
	CMOSquad();
	~CMOSquad( void ) override;

	void AIUpdatePlacement( const struct SAINotifyPlacement &placement,
	                        struct IScene *pScene,
	                        struct ISoundScene *pSoundScene,
	                        NDb::ESeason eSeason ) override
	{
	}

	IClientUpdatableProcess *AIUpdateRPGStats( const struct SAINotifyRPGStats &stats,
	                                           struct IClientAckManager *pAckManager,
	                                           NDb::ESeason eSeason ) override
	{
		return 0;
	}

	bool Create( const int nUniqueID,
	             const SAIBasicUpdate *_pUpdate,
	             NDb::ESeason eSeason,
	             const NDb::EDayNight eDayTime,
	             bool bInEditor ) override;

	bool CreateSceneObject( const int nUniqueID,
	                        const SAINewUnitUpdate *pUpdate,
	                        NDb::ESeason eSeason,
	                        bool bInEditor ) override
	{
		return true;
	}

	//void GetSpecialAbilities( CSpecialAbilities *pAbilities ) const;
	bool AIUpdateSpecialAbility( const struct SAISpecialAbilityUpdate &update ) override;
	void GetStatus( SObjectStatus *pStatus ) const override;
	// load unit onboard or unload it
	bool Load( struct IMOUnit *pUnit, bool bEnter ) override;

	bool LoadSquad( struct IMOSquad *pSquad, bool bEnter ) override
	{
		return false;
	}

	// show icons of the passangers
	void UpdatePassangers() override {}

	void GetPassangers( std::vector<CMOSelectable *> *pBuffer ) const override;

	int GetPassangersCount() const override
	{
		return units.size();
	}

	// get free places
	int GetFreePlaces() const override
	{
		return 0;
	}

	int GetFreeMechPlaces() const override
	{
		return 0;
	}

	// firing... (from container of by himself)
	void AIUpdateShot( const struct SAINotifyBaseShot &shot,
	                   const NTimer::STime &currTime,
	                   IScene *pScene,
	                   NDb::ESeason eSeason ) override
	{
	}

	//
	bool NeedShowInterrior() const override;
	void SendAcknowledgement( struct IClientAckManager *pAckManager, const NDb::EUnitAckType eAck ) override;
	//
	void GetActions( CUserActions *pActions, EActionsType eActions ) const override;
	void GetPossibleActions( CUserActions *pActions ) const override;
	void GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const override;
	void GetEnabledActions( CUserActions *pActions, EActionsType eActions ) const override;
	int GetAbilityTier( NDb::EUserAction eAction ) const override;
	const NDb::SUnitSpecialAblityDesc *GetAbilityDesc( NDb::EUserAction eAction ) const override;
	void GetAbilityInfo( CAbilityInfo &abilityList ) const override;
	void Select( bool bSelect ) override;
	void SetSelectionGroup( int nSelectionGroup ) override;

	void AIUpdateModifyEntranceState( bool bOpen ) override {}

	bool IsOpen() const override
	{
		return true;
	}

	void AIUpdateDissapear( const SAIDissapearObjUpdate *pUpdate,
	                        struct ISoundScene *pSoundScene,
	                        IClientAckManager *pAckManager ) override;
	void AIUpdateServedArtillery( IMOUnit *pMOUnit );

	IMOUnit *GetServedGun() const override
	{
		return pServedGun;
	}

	bool AIUpdateAction( bool bEnable, EActionCommand eAction ) override;

	virtual bool IsSquad() const
	{
		return true;
	}

	float GetVisualHPFraction() const override;
	float GetVisualHPFractionSmooth() const;

	bool IsFirstUnit( const IMOUnit *pUnit ) const override;
	void FillIconsInfoForFirstUnit( SSceneObjIconInfo &iconInfo ) override;
	void UpdateSquadIcons() override;

	void SetContainer( IMOContainer *_pContainer ) override
	{
		pContainer = _pContainer;
	}

	IMOContainer *GetContainer() const override
	{
		return pContainer;
	}

	// Инициализирует параметры, зависящие только от базы.
	static void InitByDB();
};
