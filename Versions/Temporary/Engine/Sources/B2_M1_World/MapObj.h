#pragma once

#include "B2_M1_World_export.h"


#include "../misc/2darray.h"
#include "../zlib/zconf.h"
#include "../stats_b2_m1/actioncommand.h"
#include "../stats_b2_m1/specialabilities.h"
#include "../stats_b2_m1/iconsset.h"
#include "B2MapObj.h"
#include "../Stats_B2_M1/DBVisObj.h"
#include "UpdatableProcess.h"

#include "../SceneB2/Scene.h"
#include "../Stats_B2_M1/AIUpdates.h"
#include "../Misc/HashFuncs.h"

const int ARMOR_FRONT				= 0;
const int ARMOR_SIDE_1			= 1;
const int ARMOR_BACK				= 2;
const int ARMOR_SIDE_2			= 3;
const int ARMOR_TOP					= 4;
const int ARMOR_BOTTOM			= 5;
const int ARMOR_COUNT				= 6;

extern const float SOLID_ICON_ALPHA;
extern float FADED_ICON_ALPHA;

interface IMOUnit;
interface IMOContainer;

typedef list< CPtr<IMOUnit> >CUnitsList;

struct SAIBasicUpdate;
namespace NDb
{
	struct SHPObjectRPGStats;
	enum EUnitAckType;
	struct SAnimB2;
}
namespace NAnimation
{
	interface ISkeletonAnimator;
}

enum EObjectStatusArmor
{
	EOS_ARMOR_FRONT,
	EOS_ARMOR_SIDE,
	EOS_ARMOR_BACK,
	EOS_ARMOR_TOP,

	EOS_ARMOR_COUNT,
};

struct B2_M1_WORLD_EXPORT SObjectStatus
{
	struct SWeapon
	{
		ZDATA
		CDBPtr<NDb::SWeaponRPGStats> pWeaponID;
		int nCount;
		bool bPrimary;
		wstring szLocalizedName;
		int nDamage;
		int nPenetration;
		int nAmmo;
		int nMaxAmmo;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pWeaponID); f.Add(3,&nCount); f.Add(4,&bPrimary); f.Add(5,&szLocalizedName); f.Add(6,&nDamage); f.Add(7,&nPenetration); f.Add(8,&nAmmo); f.Add(9,&nMaxAmmo); return 0; }
		
		bool operator==( const SWeapon &weapon ) const
		{
			return pWeaponID == weapon.pWeaponID;
		}
	};

	ZDATA
	wstring szLocalizedName;

	int nHP;
	int nMaxHP;
	
	int nSupply;
	ZSKIP //int nSecondaryAmmo;
	
	ZSKIP //int nPrimaryGunCount;
	ZSKIP //int nSecondaryGunCount;

	vector<int> armors;
	vector<SWeapon> weapons;

	CDBPtr< NDb::SArmorPattern > pArmorPattern;
	
	bool bIsTransport;
	ZSKIP //bool bIsResourcesCarrier;
	
	float fFuel;
	
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szLocalizedName); f.Add(3,&nHP); f.Add(4,&nMaxHP); f.Add(5,&nSupply); f.Add(9,&armors); f.Add(10,&weapons); f.Add(11,&pArmorPattern); f.Add(12,&bIsTransport); f.Add(14,&fFuel); return 0; }
	
	SObjectStatus();
	
	void Clear();
	void AddWeapon( const SWeapon &weapon );
};

struct SIconsSetInfo
{
	ZDATA
		float fRaising;
		float fHPBarLen;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&fRaising); f.Add(3,&fHPBarLen); return 0; }
	
	SIconsSetInfo() {}
	SIconsSetInfo( float _fRaising, float _fHPBarLen ) : 
		fRaising( _fRaising ), fHPBarLen( _fHPBarLen ) {}
};

struct SAbilityInfo 
{
	SAbilitySwitchState	abilityState;
	float	fParam;
	SAbilityInfo():
	abilityState( EASS_READY_TO_ON ), fParam( 0.0f ) {}
	SAbilityInfo( SAbilitySwitchState _abilityState, float	_fParam ):
	abilityState( _abilityState ), fParam( _fParam ) {}
};
typedef hash_map< NDb::EUnitSpecialAbility, SAbilityInfo, SEnumHash > CAbilityInfo;

B2_M1_WORLD_EXPORT void CombineAbilities( CAbilityInfo *pAbilities, NDb::EUnitSpecialAbility eAbility, const SAbilityInfo &abilityInfo );
B2_M1_WORLD_EXPORT void CombineAbilities( CAbilityInfo *pAbilities, const CAbilityInfo &abilities );

inline const NDb::SModel* GetExactModel( const NDb::SVisObj *pVisObj, NDb::ESeason eSeason )
{
	if ( pVisObj )
	{
		for ( vector<NDb::SVisObj::SSingleObj>::const_iterator it = pVisObj->models.begin(); it != pVisObj->models.end(); ++it )
		{
			if ( it->eSeason == eSeason ) 
				return it->pModel;
		}
	}

	return 0;
}

inline const NDb::SModel* GetExactLowLevelModel( const NDb::SVisObj *pVisObj, NDb::ESeason eSeason )
{
	if ( pVisObj )
	{
		for ( vector<NDb::SVisObj::SSingleObj>::const_iterator it = pVisObj->models.begin(); it != pVisObj->models.end(); ++it )
		{
			if ( it->eSeason == eSeason ) 
				return it->pLowLevelModel;
		}
	}

	return 0;
}

inline const NDb::SModel* GetModel( const NDb::SVisObj *pVisObj, NDb::ESeason eSeason )
{
	const NDb::SModel *pExactModel = GetExactModel( pVisObj, eSeason );
	return pExactModel != 0 ? pExactModel : GetExactModel( pVisObj, NDb::SEASON_SUMMER );
}

inline const NDb::SModel* GetLowLevelModel( const NDb::SVisObj *pVisObj, NDb::ESeason eSeason )
{
	const NDb::SModel *pExactModel = GetExactLowLevelModel( pVisObj, eSeason );
	return pExactModel != 0 ? pExactModel : GetExactLowLevelModel( pVisObj, NDb::SEASON_SUMMER );
}

enum EActionsType 
{
	ACTIONS_WITH,
	ACTIONS_BY,
	ACTIONS_ALL
};

class B2_M1_WORLD_EXPORT CMapObj : public IB2MapObj
{
	// CPtr<CObjectBase> pAIObj;
	CDBPtr<NDb::SHPObjectRPGStats> pStats;
	CDBPtr<NDb::SModel> pModel;
	CVec3 vPos;
	CQuat qRot;
	CVec3 vScale;
	float fHP;
	int nID;
	bool bVisible;
	//int nObjectID;
	EDiplomacyInfo eDiplomacy;
	bool bLoopedAnimation;
	bool bIsSilentlyDead;

	vector<WORD> attachedSounds;
	//
	bool bHasMoveAnimation;
	float fAnimationSpeed;
	
	int nKeyObjectPlayer;
	int nParentID;
	int nPlayer;
	int nColorIndex;
	//
protected:

	NDb::ESeason eSeason;
	bool bIsNight;

	enum EAttachedSoundType
	{
		EAST_MOVEMENT,
		EAST_FALLING,
		EAST_PLANE_DIVE,
		EAST_IDLE,
		__EAST_COUNTER__,
	};

	void SetHasMoveAnimation( bool _bHasMoveAnimation ) { bHasMoveAnimation = _bHasMoveAnimation; }
	bool HasMoveAnimation() const { return bHasMoveAnimation; }
	float GetAnimSpeed() const { return fAnimationSpeed; }
	void SetAnimSpeed( float _fAnimationSpeed ) { fAnimationSpeed = _fAnimationSpeed; }

	int CommonUpdateHP( const float fNewHP, const struct SAINotifyRPGStats &stats, interface IScene * pScene, NDb::ESeason eSeason );
	const NDb::SVisObj* ChooseVisObjForHP( float fHP );
	void SetStats( const NDb::SHPObjectRPGStats *_pStats ) { pStats = _pStats; }
	void SetID( int _nID ) { nID = _nID; }
	//
	void SetModel( const NDb::SModel *_pModel ) { pModel = _pModel; }
	virtual void ChangeModelToDamaged( const int nDamaged, const NDb::SModel *pNewModel, const NDb::ESeason eSeason );
	virtual void ChangeModelToUsual( const NDb::SModel *pNewModel, const NDb::ESeason eSeason );
	virtual void ChangeModelToAnimable( const NDb::SModel *pNewModel, const NDb::ESeason eSeason );
	virtual void ChangeModelToTransportable( const NDb::SModel *pNewModel, const NDb::ESeason eSeason );

	void AttachSound( EAttachedSoundType eType, const NDb::SComplexSoundDesc *pSound, bool bLooped );
	void DetachSound( EAttachedSoundType eType );
	bool IsSoundAttached( EAttachedSoundType eType ) const { return attachedSounds[eType] != 0; }

	void SetDeathState();

	virtual NDb::EUserAction GetBestAutoAction( const CUserActions &actionsBy, CUserActions *pActionsWith, 
		bool bAltMode ) const;

	virtual void FillIconsInfo( SSceneObjIconInfo &iconInfo ) {}
	virtual bool CanShowIcons() const { return false; }
	
	void UpdateVisibility( bool bForced );
public:
	CMapObj();
	virtual ~CMapObj();
private:
	CMapObj( const CMapObj &a ) { ASSERT( 0 ); }
	const CMapObj & operator = ( const CMapObj &a ) { ASSERT( 0 ); return *this; }

public:
	//
	virtual bool Create( const int nUniqueID, const SAIBasicUpdate *_pUpdate, NDb::ESeason eSeason, const NDb::EDayNight eDayTime, bool bInEditor );
	virtual bool CreateSceneObject( const int nUniqueID, const SAINewUnitUpdate *pUpdate, NDb::ESeason eSeason, bool bInEditor ) = 0;
	int GetID() const { return nID; }
	//
	const NDb::SAnimB2* GetAnimation( const int nAnimation );
	// used in IB2MapObj, virtual
	virtual int GetUniqueID() const { return GetID(); }

	void AfterLoad();
	
	virtual void SwitchLightFX( const bool bNewState, const NDb::ESeason eSeason, const bool bIsNight, const bool bInEditor = false );
	void ForceSwitchLightFX( const bool bNewState, const bool bInEditor );
	void ForceSwitchLightFX( int nEffect, const bool bNewState, const bool bInEditor );

	const NDb::SHPObjectRPGStats *GetStats() const { return pStats; }
	const NDb::SModel* GetModelDesc() const { return pModel; }
	virtual void SetPlacement( const CVec3 &_vPos, const CQuat &_qRot ) { vPos = _vPos; qRot = _qRot; }
	virtual void SetScale( const CVec3 &_vScale ) { vScale = _vScale; }
	void GetPlacement( CVec3 *pvPos, CQuat *pqRot, CVec3 *pvScale ) const { *pvPos = vPos; *pqRot = qRot; *pvScale = vScale; }

  const CVec3 &GetCenter() const { return vPos; }
	const CQuat &GetOrientation() const { return qRot; } 

	void SetDiplomacy( const EDiplomacyInfo diplomacy ) { eDiplomacy = diplomacy; }
	EDiplomacyInfo GetDiplomacy() const { return eDiplomacy; }
	bool IsEnemy() const { return GetDiplomacy() == EDI_ENEMY; }
	virtual bool IsFriend() const { return GetDiplomacy() == EDI_FRIEND; }
	bool IsNeutral() const { return GetDiplomacy() == EDI_NEUTRAL; }
	int GetTypeID()	const { return GetStats()->GetTypeID(); }
	const CDBID &GetDBID() const { return GetStats()->GetDBID(); }
	const NDb::STexture *GetIcon() { return GetStats()->pIconTexture;	}

	virtual void AIUpdateState( const int nParam ) {};
	virtual void AINewUnitInfo( const struct SNewUnitInfo &info, interface IScene *pScene, interface ISoundScene *pSoundScene, NDb::ESeason eSeason );
	virtual void AIUpdatePlacement( const struct SAINotifyPlacement &placement, interface IScene *pScene, interface ISoundScene *pSoundScene, NDb::ESeason eSeason );
	virtual IClientUpdatableProcess* AIUpdateMovement( const NTimer::STime &time, const bool _bMove, IScene *pScene, interface ISoundScene *pSoundScene  ) { return 0; }
	virtual IClientUpdatableProcess* AIUpdateRPGStats( const struct SAINotifyRPGStats &stats, interface IClientAckManager *pAckManager, NDb::ESeason eSeason ) = 0;

	virtual IClientUpdatableProcess* AIUpdateDamage( int nProjectileID, float fDamage, const list<int> &probableHitAttached, interface IScene *pScene, NDb::ESeason eSeason, bool bFromAIUpdate ) { return 0; }
	virtual void AIUpdateTurretTurn( const struct SAINotifyTurretTurn &turn, const NTimer::STime &currTime, IScene *pScene, const bool bHorTurn ) {}
	virtual bool AIUpdateDiplomacy( const struct SAINotifyDiplomacy &diplomacy );
	virtual bool AIUpdateSpecialAbility( const struct SAISpecialAbilityUpdate &update ) { return false; }
	virtual void AIUpdateAction( const SAIActionUpdate *pUpdate, const NDb::ESeason eSeason ) {  }
	virtual int GetAbilityTier( NDb::EUserAction eAction ) const { return -1; }
	void AIUpdateAnimationChanged( const NDb::SAnimB2 *pAnim, const NTimer::STime startTime );
	// for animations testing
	// returns: if nAnimation >= number of anims, when -1, else <length of animation, is anim looped>
	pair<int,bool> PlayAnimation( const int nAnimation ); 
	// get all actions for unit, so we know visible buttons
	virtual void GetActions( CUserActions *pActions, EActionsType eActions ) const = 0;
	// get all possible actions for object
	virtual void GetPossibleActions( CUserActions *pActions ) const { GetActions( pActions, ACTIONS_BY ); }
	// get actions that currently unaviable, so we know disabled buttons
	virtual void GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const = 0;
	// get actions that currently available
	virtual void GetEnabledActions( CUserActions *pActions, EActionsType eActions ) const;
	NDb::EUserAction GetBestAutoAction( const CUserActions &actionsBy, bool bAltMode ) const;
	NDb::EUserAction GetBestSelfAction( const CUserActions &actionsBy, bool bAltMode ) const;
	float GetHP() const { return fHP; } // [0..1]
	float GetMaxHP() const { return GetStats()->fMaxHP; }
	virtual float GetVisualHPFraction() const { return fHP; }
	bool IsAlive() const { return GetHP() > 0; }
	//
	virtual bool CanSelect() const { return false; }
	
	virtual void GetStatus( SObjectStatus *pStatus ) const;

	void UpdateIcons();

	bool HasLoopedAnimation() const { return bLoopedAnimation; }
	void SetLoopedAnimation( bool bLooped ) { bLoopedAnimation = bLooped; }
  //
  // selection
  float GetSelectionScale() const { return pStats->fSelectionScale; }
  NDb::ESelectionType GetSelectionType() const { return pStats->eSelectionType; }
  virtual void SetVisible( const bool bVisible, const NDb::ESeason eSeason, const bool bIsNight );
  virtual bool IsVisible() const { return bVisible; }
  bool IsSilentlyDead() const { return bIsSilentlyDead; }
  void SetSilentlyDead() { bIsSilentlyDead = true; }
	//
	// set fire/put out
	void ChangeLight( const int nLight, NDb::ESeason eSeason, bool bLight );
	void AIUpdateHit( const NDb::SComplexEffect *pEffect, WORD wDir, NTimer::STime time );
	//
	int GetKeyObjectPlayer() const { return nKeyObjectPlayer; }
	bool IsKeyObject() const { return nKeyObjectPlayer != -1; }
	//
	virtual void AIUpdateKeyObject( const struct SAINotifyKeyBuilding &update );
	virtual void AIUpdateKeyObjectCaptureProgress( float fProgress, int nColorIndex ) {}
	virtual void AIUpdateDissapear( const SAIDissapearObjUpdate *pUpdate, interface ISoundScene *pSoundScene, interface IClientAckManager *pAckManager );

	void SetParentID( int _nParentID ) { nParentID = _nParentID; }
	int GetParentID() const { return nParentID; }

	virtual bool IsPlaceMapCommandAck( NDb::EUserAction eUserAction ) const { return false; }
	
	virtual bool AIUpdateAction( bool bEnable, EActionCommand eAction ) { return false; }
	
	int GetPlayer() const { return nPlayer; }
	int GetColorIndex() const { return nColorIndex; }
	void SetColorIndex( int nColorIndex, bool bForceUpdate );

	virtual void UpdateVisualStatus( const struct SUnitStatusUpdate &update ) {}
	//
	virtual int operator&( IBinSaver &saver );
};

class B2_M1_WORLD_EXPORT CMOSelectable : public CMapObj
{
	bool bSelected;
	bool bCanSelect;
	int nSelectionGroup;
	bool bIconHitbar;
  NDb::SVisObjIconsSet::SVisObjIcon::EVisObjIconType eIconGroup;
	SIconsSetInfo iconsSetInfo;
	bool bDisableIcons;
	bool bIsMousePicked;
	bool bHighlighted;
	DWORD dwVisualStatus;
	float fVisualRadius;
private:
	bool HasVisualGroup( enum EUnitStatus eGroup ) const;
	void SetVisualGroup( enum EUnitStatus eGroup );
	void ClearVisualGroup( enum EUnitStatus eGroup );
protected:
	void FillIconsInfo( SSceneObjIconInfo &iconInfo );
	void SetIconsHitbar( bool bHitbar, bool bHighlighted );
	void SetIconsGroup( int nGroup, bool bHighlighted );
	
	void SetIconsSetInfo( const SIconsSetInfo &info ) { iconsSetInfo = info; }

	NDb::EUserAction GetBestAutoAction( const CUserActions &actionsBy, CUserActions *pActionsWith, 
		bool bAltMode ) const;
public:
	CMOSelectable() : bSelected( false ), bCanSelect( true ), nSelectionGroup( -1 ),
		bIconHitbar( false ),
    eIconGroup( NDb::SVisObjIconsSet::SVisObjIcon::VOIT_NONE ),
    bDisableIcons( false ),
		bIsMousePicked( false ),
		bHighlighted( false ),
		dwVisualStatus( 0 ),
		fVisualRadius( -1.0f )
	{
	}
	//check, is this object selected?
	virtual bool IsSelected() const { return bSelected; }
	virtual void Select( bool bSelect );
	virtual bool CanSelect() const;
	virtual void SetCanSelect( bool _bCanSelect ) { bCanSelect = _bCanSelect; }
	virtual const bool IsMousePicked() const { return bIsMousePicked; }
	virtual void SetMousePicked( const bool _bIsMousePicked ) { bIsMousePicked = _bIsMousePicked; }
	int GetSelectionGroup() const { return nSelectionGroup; }
	virtual void SetSelectionGroup( int nSelectionGroup );
	//sends selection acknowlegdement
	//virtual void SendAcknowledgement( interface IClientAckManager *pAckManager, const EUnitAckType eAckType, const int nSet ) {  }
	const SIconsSetInfo& GetIconsSetInfo() const { return iconsSetInfo; }
	float GetIconsRaising() const { return iconsSetInfo.fRaising; }
	float GetIconsHPBarLen() const { return iconsSetInfo.fHPBarLen; }
	void DisableIcons( bool bDisable );
	bool CanShowIcons() const;
	virtual void GetAbilityInfo( CAbilityInfo &abilityList ) const {}
	void UpdateVisualStatus( const struct SUnitStatusUpdate &update );
	virtual IMOContainer* GetContainer() const { return 0; }
	IMOContainer* GetTopContainer() const;
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, checked_cast<CMapObj*>(this) );
		saver.Add( 2, &bSelected );
		saver.Add( 3, &bCanSelect );
		saver.Add( 4, &nSelectionGroup );
		saver.Add( 6, &eIconGroup );
		saver.Add( 9, &iconsSetInfo );
		saver.Add( 11, &bIconHitbar );
		saver.Add( 12, &bDisableIcons );
		saver.Add( 13, &dwVisualStatus );
		saver.Add( 14, &fVisualRadius );
		return 0;
	}
};

interface IMOContainer : public CMOSelectable
{
protected:
	void FillIconsInfo( SSceneObjIconInfo &iconInfo );
public:
	ZDATA_(CMOSelectable)
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CMOSelectable*)this); return 0; }
	// load unit onboard or unload it
	virtual bool Load( interface IMOUnit *pMO, bool bEnter ) = 0;
	virtual bool LoadSquad( interface IMOSquad *pSquad, bool bEnter ) = 0;

	// show icons of the passangers
	virtual void UpdatePassangers() = 0;
	// get all passangers from container.
	virtual void GetPassangers( vector<CMOSelectable*> *pBuffer ) const = 0;
	virtual void GetPassangers( vector<IB2MapObj*> *pPassangers ) const;
	
	virtual int GetPassangersCount() const = 0;
	// get free places
	virtual int GetFreePlaces() const = 0;
	// get free places for mech units
	virtual int GetFreeMechPlaces() const = 0;
	// firing... (from container of by himself)
	virtual void AIUpdateShot( const struct SAINotifyBaseShot &shot, const NTimer::STime &currTime, IScene *pScene, NDb::ESeason eSeason ) = 0;
	// нужно ли показывать пассажиров
	virtual bool NeedShowInterrior() const = 0;
	virtual void AIUpdateModifyEntranceState( bool bOpen ) = 0;
	virtual bool IsOpen() const = 0;
	virtual void SendAcknowledgement( interface IClientAckManager *pAckManager, const NDb::EUnitAckType eAck ) { NI_ASSERT( false, "wrong call" ); }
};

interface IMOSquad : public IMOContainer
{
	ZDATA_(IMOContainer)
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(IMOContainer*)this); return 0; }

	virtual IMOUnit* GetServedGun() const = 0;

	virtual bool IsFirstUnit( const IMOUnit *pUnit ) const = 0;
	virtual void FillIconsInfoForFirstUnit( SSceneObjIconInfo &iconInfo ) = 0;
	virtual void UpdateSquadIcons() = 0;
	virtual void SetContainer( IMOContainer *_pContainer ) = 0;
	virtual IMOContainer* GetContainer() const = 0;
};

interface IMOUnit : public IMOContainer
{
	ZDATA_(IMOContainer)
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(IMOContainer*)this); return 0; }
	//	virtual const bool IsVisible() const = 0;
	//
	//	virtual void AssignSelectionGroup( const int nGroupID ) = 0;
	//
	//	virtual void SetContainer( IMOContainer *pContainer ) = 0;
	//	virtual IMOContainer* GetContainer() const = 0;
	virtual void SetSquad( interface IMOSquad *_pSquad ) = 0;
	virtual interface IMOSquad* GetSquad() const = 0;
	// general update. called if this unit in the 'update' list. return true to remove unit from update list
	// general update. called if this unit in the 'update' list. return true to remove unit from update list
	//	virtual bool Update( const NTimer::STime &currTime ) = 0;
	// unit's updates
	//	virtual void AIUpdateAiming( const struct AIUpdateAiming &aiming ) = 0;
	//
	//	virtual IMapObj* AIUpdateFireWithProjectile( const struct SAINotifyNewProjectile &projectile, const NTimer::STime &currTime, interface IVisObjBuilder *pVOB ) = 0;
	// CRAP{ for animations testing
	//	virtual void AddAnimation( const SUnitBaseRPGStats::SAnimDesc *pDesc ) = 0;
	// CRAP}
	// for acks
	virtual void SendAcknowledgement( interface IClientAckManager *pAckManager, const NDb::EUnitAckType eAck ) = 0;
	virtual void AIUpdateAcknowledgement( const NDb::EUnitAckType eAck, interface IClientAckManager *pAckManager, const int nSet ) = 0;
	virtual void AIUpdateBoredAcknowledgement( const struct SAIBoredAcknowledgement &ack, interface IClientAckManager *pAckManager ) = 0;
	// remove all sounds that attached to this unit
	//	virtual void RemoveSounds(interface IScene * pScene ) = 0;
	//
	virtual void AIUpdateDeadUnit( const SAIDeadUnitUpdate *pUpdate, const NDb::ESeason eSeason, const bool bIsNight, interface ISoundScene *pSoundScene, interface IClientAckManager *pAckManager  ) = 0;
	virtual void AIUpdateDissapear( const SAIDissapearObjUpdate *pUpdate, interface ISoundScene *pSoundScene, IClientAckManager *pAckManager ) = 0;
	virtual class CMOProjectile* LaunchProjectile( const SAINewProjectileUpdate *pUpdate ) = 0;
	
	virtual void SetUnitLevel( int nLevel, bool bShowUnitRank ) = 0;
	virtual void SetNewAbility( bool bNewAbility ) = 0;

	virtual NDb::EReinforcementType GetReinfType() const = 0;
	virtual void SetReinfType( NDb::EReinforcementType eType ) = 0;
	//
	virtual IClientUpdatableProcess *AIUpdateStartFinishParadrop( const SParadropStartFinishUpdate *pUpdate, NDb::ESeason eSeason ) { return 0; }
	
	virtual void SetPointer( const NDb::SModel *pModel ) = 0;
	virtual void ClearPointer() = 0;
};

void PlaceCrater( const NDb::SCraterSet *pCrater, NDb::ESeason eSeason, const CVec2 &vPos );
int PlayComplexEffect( const int nID, const NDb::SComplexEffect *pEffect, NTimer::STime timeStart, const SHMatrix &mPlace );
int PlayComplexEffect( const int nID, const NDb::SComplexEffect *pEffect, NTimer::STime timeStart, const CVec3 &vPos );
void PlaySoundEffect( const int nID, const NDb::SComplexSoundDesc *pEffect, NTimer::STime timeStart, const CVec3 &vPos );
void PlayComplexEffect( const int nID, const string &szBoneName, ESceneSubObjType eType, const NDb::SComplexEffect *pEffect, NTimer::STime timeStart, ESceneAttachMode eMode );
int PlayComplexSeasonedEffect( const int nID, const NDb::SComplexSeasonedEffect *pEffect, NTimer::STime timeStart, const CVec3 &vPos, NDb::ESeason eSeason );
int PlayComplexSeasonedEffect( const int nID, const NDb::SComplexSeasonedEffect *pEffect, NTimer::STime timeStart, const CVec3 &vPos, const CQuat &qRot, NDb::ESeason eSeason );
int PlayComplexSeasonedEffect( const int nID, const NDb::SComplexSeasonedEffect *pEffect, NTimer::STime timeStart, const SHMatrix &mPlace, NDb::ESeason eSeason );

bool RunDefaultObjectAnimation( const NDb::SSkeleton *pSkeleton, NAnimation::ISkeletonAnimator *pAnimator );

void GetPlacementFromUpdate( CVec3 *pvPos, CQuat *pqRot, const SAINewUnitUpdate *pUpdate );

B2_M1_WORLD_EXPORT bool AddAnimation( const NDb::SAnimB2 *pAnim, const NTimer::STime timeStart, NAnimation::ISkeletonAnimator *pAnimator, bool bLooped, float fSpeed = 1.0f );
B2_M1_WORLD_EXPORT bool AddAnimation( const NDb::SAnimB2 *pAnim, const NTimer::STime timeStart, NAnimation::ISkeletonAnimator *pAnimator );



