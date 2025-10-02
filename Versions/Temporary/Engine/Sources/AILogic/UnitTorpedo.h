#pragma once

//
//	Home for torpedoes (CUnitTorpedo class, states factory and states)
//

#include "AIUnit.h"
#include "UnitStates.h"
#include "StatesFactory.h"
interface ICollisionsCollector;

class CTorpedoStatesFactory : public IStatesFactory
{
	OBJECT_BASIC_METHODS( CTorpedoStatesFactory );

	static CPtr<CTorpedoStatesFactory> pFactory;
public:
	int operator&( IBinSaver &saver )
	{
		return 0;
	}
	static IStatesFactory* Instance();

	virtual bool CanCommandBeExecuted( class CAICommand *pCommand );

	virtual interface IUnitState* ProduceState( class CQueueUnit *pUnit, class CAICommand *pCommand );
	virtual interface IUnitState* ProduceRestState( class CQueueUnit *pUnit );

	// for Saving/Loading of static members
	friend class CStaticMembers;
};

class CUnitTorpedo : public CAIUnit
{
	OBJECT_NOCOPY_METHODS( CUnitTorpedo );

	ZDATA_(CAIUnit)
	CDBPtr<SWeaponRPGStats> pShooterStats;
	CDBPtr<SMechUnitRPGStats> pTorpedoStats;
	float fSpeed;
	NTimer::STime timeLaunched;
	CPtr<CAIUnit> pOwner;
	CVec2 vContactPoint;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CAIUnit*)this); f.Add(2,&pShooterStats); f.Add(3,&pTorpedoStats); f.Add(4,&fSpeed); f.Add(5,&timeLaunched); f.Add(6,&pOwner); f.Add(7,&vContactPoint); return 0; }

protected:
	virtual void InitGuns() {}
	virtual const class CUnitGuns* GetGuns() const { return 0; }
	virtual class CUnitGuns* GetGuns() { return 0; }

public:
	CUnitTorpedo(): pShooterStats(0), pTorpedoStats(0), fSpeed(0), timeLaunched(0), pOwner(0), vContactPoint(VNULL2) {}
	virtual void Init( const CVec2 &center, const int z, CAIUnit *_pOwner, const SWeaponRPGStats *_pShooterStats, const SMechUnitRPGStats *_pTorpedoStats, const float fHP, const WORD dir, const BYTE player, ICollisionsCollector *pCollisionsCollector );

	// This Init() does nothing
	virtual void Init( const CVec2 &center, const int z, const SUnitBaseRPGStats *_pStats, const float fHP, const WORD dir, const BYTE player, ICollisionsCollector *pCollisionsCollector ) { }

	virtual const SUnitBaseRPGStats* GetStats() const { return pTorpedoStats; }	
	virtual IStatesFactory* GetStatesFactory() const { return CTorpedoStatesFactory::Instance(); }
	virtual const float GetSpeed() { return fSpeed; }
	virtual void Die( const bool fromExplosion, const float fDamage );
	virtual const BYTE GetParty() { return 2; }

	// Inherited from AIUnit
	virtual const EActionNotify GetShootAction() const { return ACTION_NOTIFY_MECH_SHOOT; }
	virtual const EActionNotify GetAimAction() const { return ACTION_NOTIFY_AIM; }
	virtual const EActionNotify GetDieAction() const { return ACTION_NOTIFY_DIE; }
	virtual const EActionNotify GetIdleAction() const { return ACTION_NOTIFY_IDLE; }
	virtual const EActionNotify GetMovingAction() const { return ACTION_NOTIFY_MOVE; }
	virtual const bool CanUnitTrampled( const CBasePathUnit *pTramplerUnit ) const { return false; }
	virtual const bool CanGoBackward() const { return false; }
	virtual class CBasicGun* ChooseGunForStatObj( class CStaticObject *pObj, NTimer::STime *pTime ) { return 0; }
	virtual float GetMaxFireRange() const { return 0.0f; }
	virtual bool IsMech() const { return true; }
	virtual const bool CanShootToPlanes() const { return false; }
	virtual int GetNGuns() const { return 0; }
	virtual const int GetNTurrets() const { return 0; }
	virtual class CBasicGun* GetGun( const int n ) const { return 0; }
	virtual const bool IsInfantry() const { return true; }

	virtual const bool IterateUnits( const CVec2 &vCenter, const float fRadius,	const bool bOnlyMech, const SIterateUnitsCallback &callback ) const;
	const ECollidingType GetCollidingType( CBasePathUnit *pUnit ) const { return ECT_NONE; }
	const bool CanLockTiles() const { return false; }

	virtual void Segment();
};

class CTorpedoPath : public ISmoothPath
{
	OBJECT_BASIC_METHODS( CTorpedoPath );
	CVec2 vCurPoint;
	CVec2 vEndPoint;
	CVec2 vSpeed;

	CBasePathUnit *pUnit;
public:
	CTorpedoPath() { }
	bool Init( CBasePathUnit *_pUnit, const class CVec2 &_vCurPoint, const CVec2 &_vEndPoint, const float fSpeed );

	virtual const CVec2& GetFinishPoint() const { return VNULL2; }
	virtual bool IsFinished() const { return false; }
	virtual void Segment( const NTimer::STime timeDiff );
	int operator&( IBinSaver &saver );

	//ненужные функции
	virtual bool Init( CBasePathUnit *pUnit, IPath *pPath, bool bSmoothTurn, bool bCheckTurn, CAIMap *pAIMap ) { return false; }
	virtual bool Init( IMemento *pMemento, CBasePathUnit *pUnit, CAIMap *pAIMap ) { return false; }
	virtual bool InitByFormationPath( class CFormation *pFormation, CBasePathUnit *pUnit ) { return false; }
	virtual void Stop() { }

	virtual const bool CanGoBackward() const { return true; }
	virtual const bool CanGoForward() const { return true; }
	virtual const CVec2 PeekPathPoint( const int nToShift ) const;
	virtual IMemento* CreateMemento() const { return 0; }
	virtual const float GetSpeed() const { return -1.0f; }
};

class CTorpedoAttackState : public IUnitState
{
	OBJECT_BASIC_METHODS( CTorpedoAttackState );

	ZDATA
	CPtr<CUnitTorpedo> pTorpedo;
	CPtr<CTorpedoPath> pPath;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTorpedo); f.Add(3,&pPath); return 0; }
public:
	CTorpedoAttackState( CUnitTorpedo *pUnit, const CVec2 &_vPoint );
	CTorpedoAttackState() { }

	virtual void Segment();

	virtual EUnitStateNames GetName() { return EUSN_SWARM; }
	virtual bool IsRestState() const { return false; }
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand ) { return TSIR_NO_COMMAND_INCOMPATIBLE; }
	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const { return VNULL2; }
};
