#pragma once

#include "MOUnit.h"
#include "../SceneB2/AnimMutators.h"
#include "SmokeTrailEffect.h"

namespace NAnimation
{
	struct ISkeletonAnimator;
}
namespace NDb
{
	enum EAnimationType;
}

class CMOUnitMechanical : public CMOUnit
{
	OBJECT_NOCOPY_METHODS( CMOUnitMechanical );

	CObj<IClientUpdatableProcess> pIdleProcess;
	ZDATA_( CMOUnit )
		ZONSERIALIZE
		bool bArtilleryHooked;
		vector< CPtr<CMOSelectable> > vPassangers;
		CPtr<IMechUnitJoggingMutator> pJoggingMutator;
		bool bMoved;
		vector<CVec3> lastTrackPoints;
		vector<CVec3> trackPoints;
		float fTrackWidth;
		WORD wLastTrackDir;
		int nLastTrackTime;
		bool bForwardMoving;
		CPtr<IMOContainer> pTransport;
		bool bTrackBroken;
		vector< CObj<CSmokeTrailEffect> > smokeTrails;
		CPtr<IMOUnit> pOneFromCrew;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CMOUnit *)this); OnSerialize( f ); f.Add(2,&bArtilleryHooked); f.Add(3,&vPassangers); f.Add(4,&pJoggingMutator); f.Add(5,&bMoved); f.Add(6,&lastTrackPoints); f.Add(7,&trackPoints); f.Add(8,&fTrackWidth); f.Add(9,&wLastTrackDir); f.Add(10,&nLastTrackTime); f.Add(11,&bForwardMoving); f.Add(12,&pTransport); f.Add(13,&bTrackBroken); f.Add(14,&smokeTrails); f.Add(15,&pOneFromCrew); return 0; }
	void OnSerialize( IBinSaver &f );
private:
	
	bool IsInside( const int nID );
	const NDb::SMechUnitRPGStats* GetStatsLocal() const { return checked_cast<const NDb::SMechUnitRPGStats*>( GetStats() ); }
	int GetMechPassangersCount() const;
	void PlayDieAnimation( const SAIDeadUnitUpdate *pUpdate );
	const NDb::SAnimB2* CMOUnitMechanical::GetAnimB2( 
		const NDb::SModel *pModel, const vector<NDb::Svector_AnimDescs> &animdescs, 
		const NDb::EAnimationType eAnimType, const int nAnimID );

	void PlayAnimDesc( NAnimation::ISkeletonAnimator *pAnimator, const NDb::SModel *pModel, 
										 const vector<NDb::Svector_AnimDescs> &animdescs,
										 const int nStartTime, const NDb::EAnimationType eAnimType, const int nAnimID );
	void PlayAnimDescForAttached( IAttachedObject *pObject, const vector<NDb::Svector_AnimDescs> &animdescs,
																const int nStartTime, const NDb::EAnimationType eAnimType, const int nAnimID );
protected:
	virtual void InitAttached( const NDb::ESeason eSeason, IChooseAttached *pChooseFunc );
	void FillIconsInfo( SSceneObjIconInfo &iconInfo );
public:
	virtual bool Create( const int nUniqueID, const SAIBasicUpdate *pUpdate, NDb::ESeason eSeason, const NDb::EDayNight eDayTime, bool bInEditor );

	void GetStatus( SObjectStatus *pStatus ) const;
	//
	bool Load( struct IMOUnit *pMO, bool bEnter );
	bool LoadSquad( struct IMOSquad *pSquad, bool bEnter );
	void UpdatePassangers() { }
	void GetPassangers( vector<CMOSelectable*> *pBuffer ) const;
	int GetPassangersCount() const { return vPassangers.size(); }
	int GetFreePlaces() const;
	int GetFreeMechPlaces() const;

	const CVec3 GetFirePoint( const int nPlatform, const int nGun ) const;
	
	void AIUpdateState( const int nParam );
	void AIUpdateShot( const struct SAINotifyBaseShot &shot, const NTimer::STime &currTime, IScene *pScene, NDb::ESeason eSeason );
	IClientUpdatableProcess* AIUpdateMovement( const NTimer::STime &time, const bool _bMove, IScene *pScene, ISoundScene *pSoundScene  );
	void AIUpdateTurretTurn( const struct SAINotifyTurretTurn &turn, const NTimer::STime &currTime, IScene *pScene, const bool bHorTurn );
	void AIUpdatePlacement( const struct SAINotifyPlacement &placement, struct IScene *pScene, struct ISoundScene *pSoundScene, NDb::ESeason eSeason );

	//
	virtual void SendAcknowledgement( struct IClientAckManager *pAckManager, const NDb::EUnitAckType eAck );
	void GetActions( CUserActions *pActions, EActionsType eActions ) const;
	void GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const;
	void AIUpdateDeadUnit( const SAIDeadUnitUpdate *pUpdate, const NDb::ESeason eSeason, const bool bIsNight, struct ISoundScene *pSoundScene, struct IClientAckManager *pAckManager );
	void AIUpdateDissapear( const SAIDissapearObjUpdate *pUpdate, struct ISoundScene *pSoundScene, IClientAckManager *pAckManager );
	void AIUpdateAction( const SAIActionUpdate *pUpdate, const NDb::ESeason eSeason );
	virtual class CMOProjectile* LaunchProjectile( const SAINewProjectileUpdate *pUpdate );
	void SetTransport( IMOContainer *_pTransport );
	void AIUpdateDeadPlane( const SAIActionUpdate *pUpdate );

	//
  virtual void Select( bool bSelect );
	virtual bool IsMechUnit() const { return true; }
  //
	virtual bool NeedShowInterrior() const;

	virtual void SetTrackBroken( const bool bNewValue );
	void AddShotTrace( const CDBPtr<NDb::SWeaponRPGStats> pWeapon, const struct SAINotifyBaseShot &shot, const CVec3 &vStart, const NTimer::STime &currTime, IScene *pScene );
	void SetDiveSound( bool bDive );

  bool IsVisible() const { return CMOUnit::IsVisible() && pTransport == 0; }
	IMOContainer* GetContainer() const { return pTransport; }
	void SetCrewSoldier( IMOUnit *pCrewSoldier ) { pOneFromCrew = pCrewSoldier; }
};


