#pragma once

#include "MOUnit.h"

namespace NAnimation
{
	struct ISkeletonAnimatior;
}
namespace NDb
{
	enum EAnimationType;
	struct SMechUnitRPGStats;
	struct SM1UnitHelicopter;
}
struct IMechUnitJoggingMutator;
struct IWingScaleMutator;
class CSmokeTrailEffect;

class CMOUnitHelicopter :public CMOUnit
{
	struct SPropellerInfo
	{
		ZDATA
			CPtr<NAnimation::ISkeletonAnimator> pAnimator;
			CPtr<IWingScaleMutator> pScaleMutator;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pAnimator); f.Add(3,&pScaleMutator); return 0; }

		SPropellerInfo() : pAnimator( 0 ), pScaleMutator( 0 ) {}
	};
	OBJECT_NOCOPY_METHODS( CMOUnitHelicopter );
	ZDATA_( CMOUnit )
		CDBPtr<NDb::SMechUnitRPGStats> pStats;
		CDBPtr<NDb::SM1UnitHelicopter> pHelicopterStats;
		std::vector< CPtr<CMOSelectable> > vPassangers;
		std::vector<SPropellerInfo> vPropellers;
		float fPropSpeed;
		std::vector< CObj<CSmokeTrailEffect> > smokeTrails;
		bool bMove;
		bool bDeadPlane;
		CPtr<IMechUnitJoggingMutator> pJoggingMutator;
		CQuat qSmoothRotationStart;
		CQuat qSmoothRotationTarget;
		NTimer::STime timeSmoothRotationStart;
		NTimer::STime timeSmoothRotationFinish;
		bool bSmoothRotationActive;
		bool bSmoothRotationProcessRegistered;
		bool bSmoothRotationInitialized;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CMOUnit *)this); f.Add(2,&pStats); f.Add(3,&pHelicopterStats); f.Add(4,&vPassangers); f.Add(5,&vPropellers); f.Add(6,&fPropSpeed); f.Add(7,&smokeTrails); f.Add(8,&bMove); f.Add(9,&bDeadPlane); f.Add(10,&qSmoothRotationStart); f.Add(11,&qSmoothRotationTarget); f.Add(12,&timeSmoothRotationStart); f.Add(13,&timeSmoothRotationFinish); f.Add(14,&bSmoothRotationActive); f.Add(15,&bSmoothRotationProcessRegistered); f.Add(16,&bSmoothRotationInitialized); return 0; }
private:
	const int CMOUnitHelicopter::GetMechPassangersCount() const;
	bool IsInside( const int nID );
	void SetPropellersSpeed( const float fPropSpeed, NDb::ESeason eSeason );
	void StartSmoothRotation( const CQuat &qStart, const CQuat &qTarget );
	/*
	void SetPropeller( const int nAxis, const EPropellerState eState, const NDb::ESeason eSeason );
	void SetRotationSpeed( const int nAxis, const float fSpeed );
	void SetAxisScale( const int nAxis, const float fScale );
	*/
	const NDb::SMechUnitRPGStats* GetStatsLocal() const { return checked_cast<const NDb::SMechUnitRPGStats*>( GetStats() ); }
protected:
	void InitAttached( const NDb::ESeason eSeason, IChooseAttached *pChooseFunc );
	//virtual void UpdateIcons();

public:
	bool Create( const int nUniqueID, const SAIBasicUpdate *pUpdate, NDb::ESeason eSeason, const NDb::EDayNight eDayTime, bool bInEditor );
	bool UpdateSmoothRotation( const NTimer::STime &time );

//passangers functions
	bool Load( struct IMOUnit *pMO, bool bEnter );
	bool LoadSquad( struct IMOSquad *pSquad, bool bEnter );
	void UpdatePassangers() {}
	void GetPassangers( std::vector<CMOSelectable*> *pBuffer ) const;
	int GetPassangersCount() const { return vPassangers.size(); }
	int GetFreePlaces() const;
	int GetFreeMechPlaces() const;

//AI updates
	void AIUpdateShot( const struct SAINotifyBaseShot &shot, const NTimer::STime &currTime, IScene *pScene, NDb::ESeason eSeason );
	void AIUpdatePlacement( const struct SAINotifyPlacement &placement, struct IScene *pScene, ISoundScene *pSoundScene, NDb::ESeason eSeason );
	void AIUpdateDeadPlane( const SAIActionUpdate *pUpdate, NDb::ESeason eSeason );
	void AIUpdateDissapear( const SAIDissapearObjUpdate *pUpdate, struct ISoundScene *pSoundScene, IClientAckManager *pAckManager );
	IClientUpdatableProcess* AIUpdateMovement( const NTimer::STime &time, const bool _bMove, IScene *pScene, ISoundScene *pSoundScene  );
	IClientUpdatableProcess* AIUpdateRPGStats( const SAINotifyRPGStats &stats, struct IClientAckManager *pAckManager, NDb::ESeason eSeason );

	virtual void Select( bool bSelect );
	bool IsMechUnit() const { return true; }
	

	class CMOProjectile* LaunchProjectile( const SAINewProjectileUpdate *pUpdate );
};

