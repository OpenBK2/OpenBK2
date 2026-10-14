#pragma once

// Defensive member defaults; explicit constructor values still take precedence.

#include "MOUnit.h"
#include "SceneB2/AnimMutators.h"
#include "SmokeTrailEffect.h"

#include <cstdint>

namespace NAnimation
{
struct ISkeletonAnimator;
}

namespace NDb
{
enum EAnimationType : int;
}

class CMOUnitMechanical : public CMOUnit
{
	OBJECT_NOCOPY_METHODS( CMOUnitMechanical );

	CObj<IClientUpdatableProcess> pIdleProcess;
	ZDATA_( CMOUnit )
	ZONSERIALIZE
	bool bArtilleryHooked = false;
	std::vector<CPtr<CMOSelectable>> vPassangers;
	CPtr<IMechUnitJoggingMutator> pJoggingMutator;
	bool bAmphibianInWater = false;
	bool bAmphibianWaterEffectsActive = false;
	int nJoggingBasisBoneIndex = 0;
	int nJoggingMode = 0;
	int nLastWaterMoveEffectTime = 0;
	CVec2 vLastWaterMoveEffectPos;
	bool bMoved = false;
	std::vector<CVec3> lastTrackPoints;
	std::vector<CVec3> trackPoints;
	float fTrackWidth = 0.0f;
	uint16_t wLastTrackDir = 0;
	int nLastTrackTime = 0;
	bool bForwardMoving = false;
	CPtr<IMOContainer> pTransport;
	bool bTrackBroken = false;
	std::vector<CObj<CSmokeTrailEffect>> smokeTrails;
	CPtr<IMOUnit> pOneFromCrew;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CMOUnit *)this );
		OnSerialize( f );
		f.Add( 2, &bArtilleryHooked );
		f.Add( 3, &vPassangers );
		f.Add( 4, &pJoggingMutator );
		f.Add( 5, &bMoved );
		f.Add( 6, &lastTrackPoints );
		f.Add( 7, &trackPoints );
		f.Add( 8, &fTrackWidth );
		f.Add( 9, &wLastTrackDir );
		f.Add( 10, &nLastTrackTime );
		f.Add( 11, &bForwardMoving );
		f.Add( 12, &pTransport );
		f.Add( 13, &bTrackBroken );
		f.Add( 14, &smokeTrails );
		f.Add( 15, &pOneFromCrew );
		f.Add( 16, &bAmphibianInWater );
		f.Add( 17, &nJoggingBasisBoneIndex );
		f.Add( 18, &nJoggingMode );
		f.Add( 19, &nLastWaterMoveEffectTime );
		f.Add( 20, &vLastWaterMoveEffectPos );
		f.Add( 21, &bAmphibianWaterEffectsActive );
		return 0;
	}

	void OnSerialize( IBinSaver &f );

private:
	bool IsInside( const int nID );

	const NDb::SMechUnitRPGStats *GetStatsLocal() const
	{
		return checked_cast<const NDb::SMechUnitRPGStats *>( GetStats() );
	}

	int GetMechPassangersCount() const;
	IClientUpdatableProcess *CreateIdleEffectProcess() const;
	void SetJoggingMode( const int nMode, const bool bPlay );
	void UpdateAmphibianJogging( const bool bMoving );
	void AttachComplexEffectToLocators( IScene *pScene,
	                                    const std::vector<std::string> &locators,
	                                    const NDb::SComplexEffect *pComplexEffect,
	                                    const NTimer::STime time ) const;
	void UpdateWaterMoveEffects( const struct SAINotifyPlacement &placement, IScene *pScene );
	void PlayDieAnimation( const SAIDeadUnitUpdate *pUpdate );
	const NDb::SAnimB2 *GetAnimB2( const NDb::SModel *pModel,
	                               const std::vector<NDb::Svector_AnimDescs> &animdescs,
	                               const NDb::EAnimationType eAnimType,
	                               const int nAnimID,
	                               bool skipAnimdescs = false );

	void PlayAnimDesc( NAnimation::ISkeletonAnimator *pAnimator,
	                   const NDb::SModel *pModel,
	                   const std::vector<NDb::Svector_AnimDescs> &animdescs,
	                   const int nStartTime,
	                   const NDb::EAnimationType eAnimType,
	                   const int nAnimID );
	void PlayAnimDescForAttached( IAttachedObject *pObject,
	                              const std::vector<NDb::Svector_AnimDescs> &animdescs,
	                              const int nStartTime,
	                              const NDb::EAnimationType eAnimType,
	                              const int nAnimID );

protected:
	// Derived vehicle visuals can extend the shared pose mutator and placement without duplicating mechanical-unit behavior.
	IMechUnitJoggingMutator *GetOrCreateJoggingMutator();
	virtual void SetupSpecialAnimationMutator( NAnimation::ISkeletonAnimator *pAnimator );
	virtual void AdjustVisualPlacement( struct SAINotifyPlacement *pPlacement );

	bool HasMoved() const
	{
		return bMoved;
	}

	void InitAttached( const NDb::ESeason eSeason, IChooseAttached *pChooseFunc ) override;
	void FillIconsInfo( SSceneObjIconInfo &iconInfo ) override;

public:
	bool Create( const int nUniqueID,
	             const SAIBasicUpdate *pUpdate,
	             NDb::ESeason eSeason,
	             const NDb::EDayNight eDayTime,
	             bool bInEditor ) override;

	void GetStatus( SObjectStatus *pStatus ) const override;
	//
	bool Load( struct IMOUnit *pMO, bool bEnter ) override;
	bool LoadSquad( struct IMOSquad *pSquad, bool bEnter ) override;

	void UpdatePassangers() override {}

	void GetPassangers( std::vector<CMOSelectable *> *pBuffer ) const override;

	int GetPassangersCount() const override
	{
		return vPassangers.size();
	}

	int GetFreePlaces() const override;
	int GetFreeMechPlaces() const override;

	const CVec3 GetFirePoint( const int nPlatform, const int nGun ) const override;

	void AIUpdateState( const int nParam ) override;
	void AIUpdateShot( const struct SAINotifyBaseShot &shot,
	                   const NTimer::STime &currTime,
	                   IScene *pScene,
	                   NDb::ESeason eSeason ) override;
	IClientUpdatableProcess *
	AIUpdateMovement( const NTimer::STime &time, const bool _bMove, IScene *pScene, ISoundScene *pSoundScene ) override;
	void AIUpdateTurretTurn( const struct SAINotifyTurretTurn &turn,
	                         const NTimer::STime &currTime,
	                         IScene *pScene,
	                         const bool bHorTurn ) override;
	void AIUpdatePlacement( const struct SAINotifyPlacement &placement,
	                        struct IScene *pScene,
	                        struct ISoundScene *pSoundScene,
	                        NDb::ESeason eSeason ) override;

	//
	void SendAcknowledgement( struct IClientAckManager *pAckManager, const NDb::EUnitAckType eAck ) override;
	void GetActions( CUserActions *pActions, EActionsType eActions ) const override;
	void GetPossibleActions( CUserActions *pActions ) const override;
	void GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const override;
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
	void AIUpdateDeadPlane( const SAIActionUpdate *pUpdate );

	//
	void Select( bool bSelect ) override;

	virtual bool IsMechUnit() const
	{
		return true;
	}

	//
	bool NeedShowInterrior() const override;

	virtual void SetTrackBroken( const bool bNewValue );
	void AddShotTrace( const CDBPtr<NDb::SWeaponRPGStats> pWeapon,
	                   const struct SAINotifyBaseShot &shot,
	                   const CVec3 &vStart,
	                   const NTimer::STime &currTime,
	                   IScene *pScene );
	void SetDiveSound( bool bDive );

	bool IsVisible() const override
	{
		return CMOUnit::IsVisible() && pTransport == 0;
	}

	IMOContainer *GetContainer() const override
	{
		return pTransport;
	}

	void SetCrewSoldier( IMOUnit *pCrewSoldier )
	{
		pOneFromCrew = pCrewSoldier;
	}
};
