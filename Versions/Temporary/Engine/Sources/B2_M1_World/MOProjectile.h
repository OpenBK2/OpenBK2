#pragma once

#include "MapObj.h"
#include "SmokeTrailEffect.h"

class CMOProjectile : public CMapObj
{
	OBJECT_NOCOPY_METHODS( CMOProjectile );	

	ZDATA_( CMapObj )
		CVec3 vPosDiff;
		NTimer::STime timeToEqualizePos;
		NTimer::STime startTime;

		ZSKIP
		ZSKIP
		ZSKIP
		CQuat qStartRot;
		ZSKIP
		CDBPtr<NDb::SProjectile> pProjectile;
		CPtr<CMapObj> pTarget;
		float fDamage;
		bool bModelExists;
		bool bHitTarget;
		CDBPtr<NDb::SWeaponRPGStats> pWeapon;
		int nShell;
		bool bTraceTargetIntersection;
		CObj<CSmokeTrailEffect> pTrailEffect;
		CDBPtr<NDb::SComplexEffect> pTrajectoryEffect;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CMapObj *)this); f.Add(2,&vPosDiff); f.Add(3,&timeToEqualizePos); f.Add(4,&startTime); f.Add(8,&qStartRot); f.Add(10,&pProjectile); f.Add(11,&pTarget); f.Add(12,&fDamage); f.Add(13,&bModelExists); f.Add(14,&bHitTarget); f.Add(15,&pWeapon); f.Add(16,&nShell); f.Add(17,&bTraceTargetIntersection); f.Add(18,&pTrailEffect); f.Add( 19, &pTrajectoryEffect ); return 0; }

	//
	void CreateAttachedEffect();
	void InitSmokyExhaustInfo(const CVec3 &vVisPos, const CQuat &qRot, NTimer::STime currTime );
	//
	~CMOProjectile();
public:
	CMOProjectile() : fDamage( -1 ), bModelExists( false ), bHitTarget( false ), nShell( 0 ), bTraceTargetIntersection( false ) { }

	bool Create( const struct SAINewProjectileUpdate *pUpdate, const NDb::SProjectile *pProjectile, const CVec3 &vVisPos, const CQuat &qRot, const NDb::SComplexEffect *_pTrajectoryEffect );
	bool CreateSceneObject( const int nUniqueID, const SAINewUnitUpdate *pUpdate, NDb::ESeason eSeason, bool bInEditor ) { return true; }

	void GetStatus( SObjectStatus *pStatus ) const;

	void AIUpdatePlacement( const struct SAINotifyPlacement &placement, interface IScene *pScene, interface ISoundScene *pSoundScene, NDb::ESeason eSeason );
	virtual IClientUpdatableProcess* AIUpdateRPGStats( const struct SAINotifyRPGStats &stats, interface IClientAckManager *pAckManager, NDb::ESeason eSeason ) { return 0; }
	virtual void GetActions( CUserActions *pActions, EActionsType eActions ) const { }
	virtual void GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const { }
	void Explode( SAINotifyHitInfo::EHitType eHitType, NDb::ESeason eSeason, const CVec3 &vCenter, const CVec3 &vDir );

	void SetM1Info( CMapObj *_pTarget, const NDb::SWeaponRPGStats *pWeapon, int nShell, bool bTraceTargetIntersection, float fDamage );

	virtual void GetPassangers( vector<IB2MapObj*> *pPassangers ) const { }
	virtual int GetPassangersCount() const { return 0; }
};



