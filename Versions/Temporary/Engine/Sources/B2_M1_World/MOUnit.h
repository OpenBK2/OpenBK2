#pragma once

// Defensive member defaults; explicit constructor values still take precedence.

#include "MapObj.h"

namespace NDb
{
struct SAnimB2;
struct SAttachedModelVisObj;
struct SModel;
}; // namespace NDb

class CMOUnit : public IMOUnit
{
	struct SAmmo
	{
		ZDATA
		CDBPtr<NDb::SWeaponRPGStats> pWeapon;
		int nWeaponCount = 0;
		int nAmmo = 0;

		ZEND int operator&( IBinSaver &f )
		{
			f.Add( 2, &pWeapon );
			f.Add( 3, &nWeaponCount );
			f.Add( 4, &nAmmo );
			return 0;
		}
	};

	struct SAmmoCompare
	{
		const NDb::SWeaponRPGStats *pWeapon = nullptr;

		SAmmoCompare( const NDb::SWeaponRPGStats *_pWeapon )
		{
			pWeapon = _pWeapon;
		}

		bool operator()( const SAmmo &ammo )
		{
			return pWeapon == ammo.pWeapon;
		}
	};

	CAbilityInfo abilityMap;
	std::vector<SShootAreas> oldAreas;
	int nOldAreasTime = 0;
	bool bOpen = false;
	int nLevel = 0;
	std::vector<SAmmo> ammos;
	int nSupply = 0;
	int nMaxAmmo = 0;
	int nCurAmmo = 0;
	bool bNewAbility = false;
	NDb::EReinforcementType eReinfType{};
	float fFuel = 0.0f;
	bool bPointer = false;
	bool bShowUnitRank = false;

private:
	//
	const NDb::SUnitBaseRPGStats *GetStatsLocal() const
	{
		return checked_cast<const NDb::SUnitBaseRPGStats *>( GetStats() );
	}

	void CopyAreas( const std::vector<SShootAreas> &newAreas );
	bool AreasChanged( const std::vector<SShootAreas> &newAreas ) const;

public:
	struct IChooseAttached : public CObjectBase
	{
		virtual const NDb::SVisObj *Choose( const NDb::SAttachedModelVisObj *pAttachedVisObj ) = 0;
	};

protected:
	struct SChooseAttachedByDefault : public IChooseAttached
	{
		OBJECT_NOCOPY_METHODS( SChooseAttachedByDefault )
	public:
		const NDb::SVisObj *Choose( const NDb::SAttachedModelVisObj *pAttachedVisObj ) override;
	};

	virtual void InitAttached( const NDb::ESeason eSeason, IChooseAttached *pChooseFunc ) = 0;
	void TryToAttach( const NDb::SAttachedModelVisObj *pAttachedObj,
	                  IChooseAttached *pChooseFunc,
	                  const NDb::ESeason eSeason,
	                  const std::string &szLocator,
	                  const ESceneSubObjType eType,
	                  const int nNumber );

	void ChangeModelToDamaged( const int nDamaged, const NDb::SModel *pNewModel, const NDb::ESeason eSeason ) override;
	void ChangeModelToUsual( const NDb::SModel *pNewModel, const NDb::ESeason eSeason ) override;
	void ChangeModelToAnimable( const NDb::SModel *pNewModel, const NDb::ESeason eSeason ) override;
	void ChangeModelToTransportable( const NDb::SModel *pNewModel, const NDb::ESeason eSeason ) override;

	void SetAnimation( const NDb::SAnimB2 *pAnimation, const NTimer::STime startTime );

	int GetSupply() const
	{
		return nSupply;
	}

	int GetWeaponAmmo( const NDb::SWeaponRPGStats *pWeapon ) const;
	int GetWeaponAmmoTotal() const;
	void FillIconsInfo( SSceneObjIconInfo &iconInfo ) override;

public:
	typedef std::list<SAbilityInfo> CAbilityInfoList;

	bool Create( const int nUniqueID,
	             const SAIBasicUpdate *_pUpdate,
	             NDb::ESeason eSeason,
	             const NDb::EDayNight eDayTime,
	             bool bInEditor ) override;
	bool CreateSceneObject( const int nUniqueID,
	                        const SAINewUnitUpdate *pUpdate,
	                        NDb::ESeason eSeason,
	                        bool bInEditor ) override;

	void GetStatus( SObjectStatus *pStatus ) const override;

	IClientUpdatableProcess *AIUpdateRPGStats( const SAINotifyRPGStats &stats,
	                                           struct IClientAckManager *pAckManager,
	                                           NDb::ESeason eSeason ) override;
	bool AIUpdateSpecialAbility( const struct SAISpecialAbilityUpdate &update ) override;
	int GetAbilityTier( NDb::EUserAction eAction ) const override;
	const NDb::SUnitSpecialAblityDesc *GetAbilityDesc( NDb::EUserAction eAction ) const override;
	void GetAbilityInfo( CAbilityInfo &abilityList ) const override;

	void SetSquad( struct IMOSquad *_pSquad ) override {}

	struct IMOSquad *GetSquad() const override
	{
		return 0;
	}

	//
	bool NeedShowInterrior() const override;

	void SendAcknowledgement( struct IClientAckManager *pAckManager, const NDb::EUnitAckType eAck ) override;
	void AIUpdateAcknowledgement( const NDb::EUnitAckType eAck,
	                              struct IClientAckManager *pAckManager,
	                              const int nSet ) override;
	void AIUpdateBoredAcknowledgement( const struct SAIBoredAcknowledgement &ack,
	                                   struct IClientAckManager *pAckManager ) override;
	void AIUpdateDeadUnit( const SAIDeadUnitUpdate *pUpdate,
	                       const NDb::ESeason eSeason,
	                       const bool bIsNight,
	                       ISoundScene *pSoundScene,
	                       IClientAckManager *pAckManager ) override;
	void AIUpdateDissapear( const SAIDissapearObjUpdate *pUpdate,
	                        struct ISoundScene *pSoundScene,
	                        IClientAckManager *pAckManager ) override;
	void AIUpdateShootAreas( const SAIShootAreaUpdate *pUpdate );

	bool IsSelected() const override
	{
		return ( GetSquad() ) ? ( GetSquad()->IsSelected() ) : ( CMOSelectable::IsSelected() );
	}

	void GetActions( CUserActions *pActions, EActionsType eActions ) const override;
	void GetPossibleActions( CUserActions *pActions ) const override;
	void GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const override;

	void AIUpdateModifyEntranceState( bool bOpen ) override;

	bool IsOpen() const override
	{
		return bOpen;
	}

	void SetUnitLevel( int nLevel, bool bShowUnitRank ) override;
	void SetNewAbility( bool bNewAbility ) override;

	NDb::EReinforcementType GetReinfType() const override
	{
		return eReinfType;
	}

	void SetReinfType( NDb::EReinforcementType eType ) override
	{
		eReinfType = eType;
	}

	virtual const CVec3 GetFirePoint( const int nPlatform, const int nGun ) const
	{
		return VNULL3;
	}

	void SetPointer( const NDb::SModel *pModel ) override;
	void ClearPointer() override;

	//
	int operator&( IBinSaver &saver ) override;
};
