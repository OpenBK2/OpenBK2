#pragma once

//
//	Home for torpedoes (CUnitTorpedo class, states factory and states)
//

#include "AIUnit.h"
#include "StatesFactory.h"
#include "UnitStates.h"

#include <cstdint>

struct ICollisionsCollector;

class CTorpedoStatesFactory : public IStatesFactory
{
	OBJECT_BASIC_METHODS( CTorpedoStatesFactory );

	static CPtr<CTorpedoStatesFactory> pFactory;

public:
	int operator&( IBinSaver &saver ) override
	{
		return 0;
	}

	static IStatesFactory *Instance();

	bool CanCommandBeExecuted( class CAICommand *pCommand ) override;

	struct IUnitState *ProduceState( class CQueueUnit *pUnit, class CAICommand *pCommand ) override;
	struct IUnitState *ProduceRestState( class CQueueUnit *pUnit ) override;

	// for Saving/Loading of static members
	friend class CStaticMembers;
};

class CUnitTorpedo : public CAIUnit
{
	OBJECT_NOCOPY_METHODS( CUnitTorpedo );

	ZDATA_( CAIUnit )
	CDBPtr<SWeaponRPGStats> pShooterStats;
	CDBPtr<SMechUnitRPGStats> pTorpedoStats;
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	float fSpeed = {};
	NTimer::STime timeLaunched = {};
	CPtr<CAIUnit> pOwner;
	CVec2 vContactPoint;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CAIUnit *)this );
		f.Add( 2, &pShooterStats );
		f.Add( 3, &pTorpedoStats );
		f.Add( 4, &fSpeed );
		f.Add( 5, &timeLaunched );
		f.Add( 6, &pOwner );
		f.Add( 7, &vContactPoint );
		return 0;
	}

protected:
	void InitGuns() override {}

	const class CUnitGuns *GetGuns() const override
	{
		return 0;
	}

	class CUnitGuns *GetGuns() override
	{
		return 0;
	}

public:
	CUnitTorpedo()
		: pShooterStats( 0 )
		, pTorpedoStats( 0 )
		, fSpeed( 0 )
		, timeLaunched( 0 )
		, pOwner( 0 )
		, vContactPoint( VNULL2 )
	{
	}

	virtual void Init( const CVec2 &center,
	                   const int z,
	                   CAIUnit *_pOwner,
	                   const SWeaponRPGStats *_pShooterStats,
	                   const SMechUnitRPGStats *_pTorpedoStats,
	                   const float fHP,
	                   const uint16_t dir,
	                   const uint8_t player,
	                   ICollisionsCollector *pCollisionsCollector );

	// This Init() does nothing
	void Init( const CVec2 &center,
	           const int z,
	           const SUnitBaseRPGStats *_pStats,
	           const float fHP,
	           const uint16_t dir,
	           const uint8_t player,
	           ICollisionsCollector *pCollisionsCollector ) override
	{
	}

	const SUnitBaseRPGStats *GetStats() const override
	{
		return pTorpedoStats;
	}

	IStatesFactory *GetStatesFactory() const override
	{
		return CTorpedoStatesFactory::Instance();
	}

	virtual const float GetSpeed()
	{
		return fSpeed;
	}

	void Die( const bool fromExplosion, const float fDamage ) override;

	virtual const uint8_t GetParty()
	{
		return 2;
	}

	// Inherited from AIUnit
	const EActionNotify GetShootAction() const override
	{
		return ACTION_NOTIFY_MECH_SHOOT;
	}

	const EActionNotify GetAimAction() const override
	{
		return ACTION_NOTIFY_AIM;
	}

	const EActionNotify GetDieAction() const override
	{
		return ACTION_NOTIFY_DIE;
	}

	const EActionNotify GetIdleAction() const override
	{
		return ACTION_NOTIFY_IDLE;
	}

	const EActionNotify GetMovingAction() const override
	{
		return ACTION_NOTIFY_MOVE;
	}

	const bool CanUnitTrampled( const CBasePathUnit *pTramplerUnit ) const override
	{
		return false;
	}

	const bool CanGoBackward() const override
	{
		return false;
	}

	class CBasicGun *ChooseGunForStatObj( class CStaticObject *pObj, NTimer::STime *pTime ) override
	{
		return 0;
	}

	float GetMaxFireRange() const override
	{
		return 0.0f;
	}

	bool IsMech() const override
	{
		return true;
	}

	const bool CanShootToPlanes() const override
	{
		return false;
	}

	int GetNGuns() const override
	{
		return 0;
	}

	const int GetNTurrets() const override
	{
		return 0;
	}

	class CBasicGun *GetGun( const int n ) const override
	{
		return 0;
	}

	const bool IsInfantry() const override
	{
		return true;
	}

	const bool IterateUnits( const CVec2 &vCenter,
	                         const float fRadius,
	                         const bool bOnlyMech,
	                         const SIterateUnitsCallback &callback ) const override;

	const ECollidingType GetCollidingType( CBasePathUnit *pUnit ) const override
	{
		return ECT_NONE;
	}

	const bool CanLockTiles() const override
	{
		return false;
	}

	void Segment() override;
};

class CTorpedoPath : public ISmoothPath
{
	OBJECT_BASIC_METHODS( CTorpedoPath );
	CVec2 vCurPoint;
	CVec2 vEndPoint;
	CVec2 vSpeed;

	CBasePathUnit *pUnit = {};

public:
	CTorpedoPath() {}

	bool Init( CBasePathUnit *_pUnit, const class CVec2 &_vCurPoint, const CVec2 &_vEndPoint, const float fSpeed );

	virtual const CVec2 &GetFinishPoint() const
	{
		return VNULL2;
	}

	bool IsFinished() const override
	{
		return false;
	}

	void Segment( const NTimer::STime timeDiff ) override;
	int operator&( IBinSaver &saver ) override;

	//ненужные функции
	bool Init( CBasePathUnit *pUnit, IPath *pPath, bool bSmoothTurn, bool bCheckTurn, CAIMap *pAIMap ) override
	{
		return false;
	}

	bool Init( IMemento *pMemento, CBasePathUnit *pUnit, CAIMap *pAIMap ) override
	{
		return false;
	}

	bool InitByFormationPath( class CFormation *pFormation, CBasePathUnit *pUnit ) override
	{
		return false;
	}

	void Stop() override {}

	const bool CanGoBackward() const override
	{
		return true;
	}

	const bool CanGoForward() const override
	{
		return true;
	}

	const CVec2 PeekPathPoint( const int nToShift ) const override;

	IMemento *CreateMemento() const override
	{
		return 0;
	}

	const float GetSpeed() const override
	{
		return -1.0f;
	}
};

class CTorpedoAttackState : public IUnitState
{
	OBJECT_BASIC_METHODS( CTorpedoAttackState );

	ZDATA
	CPtr<CUnitTorpedo> pTorpedo;
	CPtr<CTorpedoPath> pPath;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pTorpedo );
		f.Add( 3, &pPath );
		return 0;
	}

public:
	CTorpedoAttackState( CUnitTorpedo *pUnit, const CVec2 &_vPoint );

	CTorpedoAttackState() {}

	void Segment() override;

	EUnitStateNames GetName() override
	{
		return EUSN_SWARM;
	}

	bool IsRestState() const override
	{
		return false;
	}

	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand ) override
	{
		return TSIR_NO_COMMAND_INCOMPATIBLE;
	}

	bool IsAttackingState() const override
	{
		return true;
	}

	const CVec2 GetPurposePoint() const override
	{
		return VNULL2;
	}
};
