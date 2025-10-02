#pragma once

#include "MapObj.h"

namespace NDb
{
	struct SAnimB2;
	struct SAttachedModelVisObj;
	struct SModel;
};

class CMOUnit : public IMOUnit
{
	struct SAmmo
	{
		ZDATA
		CDBPtr<NDb::SWeaponRPGStats> pWeapon;
		int nWeaponCount;
		int nAmmo;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pWeapon); f.Add(3,&nWeaponCount); f.Add(4,&nAmmo); return 0; }
	};
	struct SAmmoCompare
	{
		const NDb::SWeaponRPGStats *pWeapon;

		SAmmoCompare( const NDb::SWeaponRPGStats *_pWeapon ) { pWeapon = _pWeapon; }
		bool operator()( const SAmmo &ammo )
		{
			return pWeapon == ammo.pWeapon;
		}
	};
	
	CAbilityInfo abilityMap;
	vector<SShootAreas> oldAreas;
	int nOldAreasTime;
	bool bOpen;
	int nLevel;
	vector<SAmmo> ammos;
	int nSupply;
	int nMaxAmmo;
	int nCurAmmo;
	bool bNewAbility;
	NDb::EReinforcementType eReinfType;
	float fFuel;
	bool bPointer;
	bool bShowUnitRank;

private:
	//
	const NDb::SUnitBaseRPGStats* GetStatsLocal() const { return checked_cast<const NDb::SUnitBaseRPGStats*>( GetStats() ); }
	void CopyAreas( const vector<SShootAreas> &newAreas );
	bool AreasChanged( const vector<SShootAreas> &newAreas ) const;

public:
	struct IChooseAttached : public CObjectBase
	{
		virtual const NDb::SVisObj* Choose( const NDb::SAttachedModelVisObj *pAttachedVisObj ) = 0;
	};
protected:
	struct SChooseAttachedByDefault : public IChooseAttached
	{
		OBJECT_NOCOPY_METHODS( SChooseAttachedByDefault )
	public:
		virtual const NDb::SVisObj* Choose( const NDb::SAttachedModelVisObj *pAttachedVisObj );
	};

	virtual void InitAttached( const NDb::ESeason eSeason, IChooseAttached *pChooseFunc ) = 0;
	void TryToAttach( const NDb::SAttachedModelVisObj *pAttachedObj, IChooseAttached *pChooseFunc, 
										const NDb::ESeason eSeason, const string &szLocator, const ESceneSubObjType eType, const int nNumber );

	virtual void ChangeModelToDamaged( const int nDamaged, const NDb::SModel *pNewModel, const NDb::ESeason eSeason );
	virtual void ChangeModelToUsual( const NDb::SModel *pNewModel, const NDb::ESeason eSeason );
	virtual void ChangeModelToAnimable( const NDb::SModel *pNewModel, const NDb::ESeason eSeason );
	virtual void ChangeModelToTransportable( const NDb::SModel *pNewModel, const NDb::ESeason eSeason );

	void SetAnimation( const NDb::SAnimB2 *pAnimation, const NTimer::STime startTime );
	int GetSupply() const { return nSupply; }
	int GetWeaponAmmo( const NDb::SWeaponRPGStats *pWeapon ) const;
	int GetWeaponAmmoTotal() const;
	void FillIconsInfo( SSceneObjIconInfo &iconInfo );
public:
	typedef list< SAbilityInfo > CAbilityInfoList;

	bool Create( const int nUniqueID, const SAIBasicUpdate *_pUpdate, NDb::ESeason eSeason, const NDb::EDayNight eDayTime, bool bInEditor );
	virtual bool CreateSceneObject( const int nUniqueID, const SAINewUnitUpdate *pUpdate, NDb::ESeason eSeason, bool bInEditor );

	void GetStatus( SObjectStatus *pStatus ) const;

	IClientUpdatableProcess* AIUpdateRPGStats( const SAINotifyRPGStats &stats, struct IClientAckManager *pAckManager, NDb::ESeason eSeason );
	bool AIUpdateSpecialAbility( const struct SAISpecialAbilityUpdate &update );
	int GetAbilityTier( NDb::EUserAction eAction ) const;
	void GetAbilityInfo( CAbilityInfo &abilityList ) const;
	void SetSquad( struct IMOSquad *_pSquad ) { }
	virtual struct IMOSquad* GetSquad() const {	return 0; }
	//
	virtual bool NeedShowInterrior() const;
	
	virtual void SendAcknowledgement( struct IClientAckManager *pAckManager, const NDb::EUnitAckType eAck );
	void AIUpdateAcknowledgement( const NDb::EUnitAckType eAck, struct IClientAckManager *pAckManager, const int nSet );
	void AIUpdateBoredAcknowledgement( const struct SAIBoredAcknowledgement &ack, struct IClientAckManager *pAckManager );
	void AIUpdateDeadUnit( const SAIDeadUnitUpdate *pUpdate, const NDb::ESeason eSeason, const bool bIsNight, ISoundScene *pSoundScene, IClientAckManager *pAckManager );
	void AIUpdateDissapear( const SAIDissapearObjUpdate *pUpdate, struct ISoundScene *pSoundScene, IClientAckManager *pAckManager );
	void AIUpdateShootAreas( const SAIShootAreaUpdate *pUpdate );

	bool IsSelected() const
	{ 
		return (GetSquad()) ? (GetSquad()->IsSelected()) : (CMOSelectable::IsSelected());
	}
	void GetActions( CUserActions *pActions, EActionsType eActions ) const;
	void GetPossibleActions( CUserActions *pActions ) const;
	void GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const;

	void AIUpdateModifyEntranceState( bool bOpen );
	bool IsOpen() const { return bOpen; }

	void SetUnitLevel( int nLevel, bool bShowUnitRank );
	void SetNewAbility( bool bNewAbility );

	NDb::EReinforcementType GetReinfType() const { return eReinfType; }
	void SetReinfType( NDb::EReinforcementType eType ) { eReinfType = eType; }

	virtual const CVec3 GetFirePoint( const int nPlatform, const int nGun ) const { return VNULL3; }

	void SetPointer( const NDb::SModel *pModel );
	void ClearPointer();

	//
	int operator&( IBinSaver &saver );
};


