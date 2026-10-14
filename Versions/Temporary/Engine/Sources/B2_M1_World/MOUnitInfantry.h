#pragma once

// Defensive member defaults; explicit constructor values still take precedence.

#include "MOUnit.h"

class CMOUnitInfantry : public CMOUnit
{
	OBJECT_NOCOPY_METHODS( CMOUnitInfantry );
	//
	ZDATA_( CMOUnit )
	CPtr<IMOSquad> pSquad;
	CPtr<IMOContainer> pTransport;
	int nParachuteID = 0;
	bool bEntrenched = false;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CMOUnit *)this );
		f.Add( 2, &pSquad );
		f.Add( 3, &pTransport );
		f.Add( 4, &nParachuteID );
		f.Add( 5, &bEntrenched );
		return 0;
	}

private:
	const NDb::SInfantryRPGStats *GetStatsLocal() const
	{
		return checked_cast<const NDb::SInfantryRPGStats *>( GetStats() );
	}

	//
	IClientUpdatableProcess *StartParadrop( const SParadropStartFinishUpdate *pUpdate, NDb::ESeason eSeason );
	IClientUpdatableProcess *FinishParadrop( const SParadropStartFinishUpdate *pUpdate, NDb::ESeason eSeason );
	bool IsFirstUnit() const;
	void GetActionsBy( CUserActions *pActions ) const;

protected:
	void InitAttached( const NDb::ESeason eSeason, IChooseAttached *pChooseFunc ) override;
	void FillIconsInfo( SSceneObjIconInfo &iconInfo ) override;
	void SetVisible( const bool bVisible, const NDb::ESeason eSeason, const bool bIsNight ) override;

public:
	CMOUnitInfantry()
		: nParachuteID( -1 )
		, bEntrenched( false )
	{
	}

	//
	bool Create( const int nUniqueID,
	             const SAIBasicUpdate *pUpdate,
	             NDb::ESeason eSeason,
	             const NDb::EDayNight eDayTime,
	             bool bInEditor ) override;
	bool CreateSceneObject( const int nUniqueID,
	                        const SAINewUnitUpdate *pUpdate,
	                        NDb::ESeason eSeason,
	                        bool bInEditor ) override;
	void ChangeRPGStats( const struct SAIChangeDBIDUpdate &update, const NDb::ESeason eSeason );
	void GetStatus( SObjectStatus *pStatus ) const override;

	void SetSquad( struct IMOSquad *_pSquad ) override;

	struct IMOSquad *GetSquad() const override
	{
		return pSquad;
	}

	virtual IB2MapObj *GetSquadB2MapObj() const
	{
		return pSquad;
	}

	//
	bool Load( struct IMOUnit *pMO, bool bEnter ) override
	{
		return false;
	}

	bool LoadSquad( struct IMOSquad *pSquad, bool bEnter ) override
	{
		return false;
	}

	void UpdatePassangers() override {}

	void GetPassangers( std::vector<CMOSelectable *> *pBuffer ) const override {}

	int GetPassangersCount() const override
	{
		return 0;
	}

	int GetFreePlaces() const override
	{
		return 0;
	}

	int GetFreeMechPlaces() const override
	{
		return 0;
	}

	void AIUpdateShot( const struct SAINotifyBaseShot &shot,
	                   const NTimer::STime &currTime,
	                   IScene *pScene,
	                   NDb::ESeason eSeason ) override;
	void AIUpdatePlacement( const struct SAINotifyPlacement &placement,
	                        struct IScene *pScene,
	                        struct ISoundScene *pSoundScene,
	                        NDb::ESeason eSeason ) override;
	//
	void GetActions( CUserActions *pActions, EActionsType eActions ) const override;
	void GetPossibleActions( CUserActions *pActions ) const override;

	void GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const override {}

	void AIUpdateDeadUnit( const SAIDeadUnitUpdate *pUpdate,
	                       const NDb::ESeason eSeason,
	                       const bool bIsNight,
	                       struct ISoundScene *pSoundScene,
	                       struct IClientAckManager *pAckManager ) override;
	void AIUpdateDissapear( const SAIDissapearObjUpdate *pUpdate,
	                        struct ISoundScene *pSoundScene,
	                        IClientAckManager *pAckManager ) override;
	void AIUpdateAction( const SAIActionUpdate *pUpdate, const NDb::ESeason eSeason ) override;
	class CMOProjectile *LaunchProjectile( const SAINewProjectileUpdate *pUpdate ) override;
	void SetTransport( IMOContainer *_pTransport );

	IMOContainer *GetTransport()
	{
		return pTransport;
	}

	//
	IClientUpdatableProcess *AIUpdateStartFinishParadrop( const SParadropStartFinishUpdate *pUpdate,
	                                                      NDb::ESeason eSeason ) override;
	void HideParachute();
	void SetupWeapon( NDb::ESeason eSeason );

	virtual bool IsInfantry() const
	{
		return true;
	}

	void Select( bool bSelect ) override;

	const CVec3 GetFirePoint( const int nPlatform, const int nGun ) const override;
	void AddShotTrace( const CDBPtr<NDb::SWeaponRPGStats> pWeapon,
	                   const struct SAINotifyBaseShot &shot,
	                   const NTimer::STime &currTime,
	                   IScene *pScene );

	bool IsVisible() const override;

	void SetEntrench( bool bEntrench );

	IMOContainer *GetContainer() const override
	{
		return pTransport ? pTransport.GetPtr() : pSquad.GetPtr();
	}
};
