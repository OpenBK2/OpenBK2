#pragma once

#include "FreeFireManager.h"
#include "RailRoads.h"
#include "StatesFactory.h"
#include "StaticObject.h"
#include "System/Basic.h"
#include "Technics.h"
#include "UnitStates.h"

#include <cstdint>
#include <list>

//
//	Trains.h
//
// 02.07.2004 - 1. Single locomotive will go along a RailRoad VSO
// 15.09.2004 - 2. Started work on multi-car trains
//									a) all cars are linked to back of locomotive
//									b) no compensation for curves
//									c) all cars face the same way
//

//	CLocomotiveStatesFactory

class CLocomotiveStatesFactory : public IStatesFactory
{
	OBJECT_BASIC_METHODS( CLocomotiveStatesFactory );

	static CPtr<CLocomotiveStatesFactory> pFactory;

public:
	int operator&( IBinSaver &saver ) override
	{
		return 0;
	}

	static IStatesFactory *Instance();

	bool CanCommandBeExecuted( class CAICommand *pCommand ) override;

	struct IUnitState *ProduceState( class CQueueUnit *pObj, class CAICommand *pCommand ) override;
	struct IUnitState *ProduceRestState( class CQueueUnit *pUnit ) override;

	// for Saving/Loading of static members
	friend class CStaticMembers;
};

//	CTrainCarStatesFactory

class CTrainCarStatesFactory : public IStatesFactory
{
	OBJECT_BASIC_METHODS( CTrainCarStatesFactory );

	static CPtr<CTrainCarStatesFactory> pFactory;

public:
	int operator&( IBinSaver &saver ) override
	{
		return 0;
	}

	static IStatesFactory *Instance();

	bool CanCommandBeExecuted( class CAICommand *pCommand ) override;

	struct IUnitState *ProduceState( class CQueueUnit *pObj, class CAICommand *pCommand ) override;
	struct IUnitState *ProduceRestState( class CQueueUnit *pUnit ) override;

	// for Saving/Loading of static members
	friend class CStaticMembers;
};

//	CTrainLocomotive

class CTrainCar;

class CTrainLocomotive : public CMilitaryCar
{
	OBJECT_NOCOPY_METHODS( CTrainLocomotive );

	// List of cars
	typedef std::list<CPtr<CAIUnit>> CCarList;
	ZDATA_( CMilitaryCar )
	CCarList cars;

	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	int nTrack = {};
	bool bBackward = {}; // Front of the unit faces point 0 of the VSO
	float fTrackPos = {};
	float fTrackLimit = {};

	float fFrontOffset = {};
	float fBackOffset = {};
	float fTrainLength = {};
	float fBackLink = {};

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CMilitaryCar *)this );
		f.Add( 2, &cars );
		f.Add( 3, &nTrack );
		f.Add( 4, &bBackward );
		f.Add( 5, &fTrackPos );
		f.Add( 6, &fTrackLimit );
		f.Add( 7, &fFrontOffset );
		f.Add( 8, &fBackOffset );
		f.Add( 9, &fTrainLength );
		f.Add( 10, &fBackLink );
		return 0;
	}

	void Init( const CVec2 &center,
	           const int z,
	           const SUnitBaseRPGStats *pStats,
	           const float fHP,
	           const uint16_t dir,
	           const uint8_t player,
	           ICollisionsCollector *pCollisionsCollector ) override;

	IStatesFactory *GetStatesFactory() const override
	{
		return CLocomotiveStatesFactory::Instance();
	}

	void TakeDamage( const float fDamage,
	                 const SWeaponRPGStats::SShell *pShell,
	                 const int nPlayerOfShoot,
	                 CAIUnit *pShotUnit ) override;

	const bool CanRotate() const override
	{
		return false;
	}

	virtual bool CanTurnToFrontDir( const uint16_t wDir )
	{
		return false;
	}

	const bool CanShootInMovement() override
	{
		return true;
	}

	const bool NeedDeinstall() const override
	{
		return false;
	}

	class CArtillery *GetTowedArtillery() const override
	{
		return 0;
	}

	void Segment() override;

	// Train-specific
	const SRailRoadSystem::SRRInstance *GetTrack() const;

	float GetTrackPos() const
	{
		return fTrackPos;
	}

	float GetTrackLimit() const
	{
		return fTrackLimit;
	}

	const bool IsBackwards() const
	{
		return bBackward;
	}

	void SetToTrackPos( float fPos );
	void SetCollision( ICollision *pCollision, IPath *pPath ) override;
	const ECollidingType GetCollidingType( CBasePathUnit *pUnit ) const override;

	void Die( const bool fromExplosion, const float fDamage ) override;

	// Locomotive-specific
	// The complete list includes destroyed cars, so traversing a train never stops at a wreck.
	const std::list<CPtr<CAIUnit>> &GetCars() const
	{
		return cars;
	}

	void AddCar( CTrainCar *pNewCar );             // Add car to the end of the list
	void PassCommandToAll( CAICommand *pCommand ); // Pass command to all cars
	const float GetTrainLength();
	const bool TryFiringAll();
	void Stop() override; // Stop all cars
	CAIUnit *GetLastCar();
	void SetMovingTileLocks( bool bMoving );
};

//	CTrainCar

class CTrainCar : public CMilitaryCar
{
	friend class CTrainLocomotive;
	OBJECT_NOCOPY_METHODS( CTrainCar );

	ZDATA_( CMilitaryCar )

	CPtr<CTrainLocomotive> pLocomotive;
	CPtr<CTrainCar> pLinkFront;
	CPtr<CTrainCar> pLinkBack;
	int nTrack = {};
	bool bBackward = {}; // Front of the unit faces point 0 of the VSO
	float fTrackPos = {};
	float fTrackLimit = {};

	float fFrontOffset = {};
	float fBackOffset = {};
	float fFrontLink = {};
	float fBackLink = {};

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CMilitaryCar *)this );
		f.Add( 2, &pLocomotive );
		f.Add( 3, &pLinkFront );
		f.Add( 4, &pLinkBack );
		f.Add( 5, &nTrack );
		f.Add( 6, &bBackward );
		f.Add( 7, &fTrackPos );
		f.Add( 8, &fTrackLimit );
		f.Add( 9, &fFrontOffset );
		f.Add( 10, &fBackOffset );
		f.Add( 11, &fFrontLink );
		f.Add( 12, &fBackLink );
		return 0;
	}

private:
	void LinkToLocomotive( CTrainLocomotive *pLinkTo );

public:
	void Init( const CVec2 &center,
	           const int z,
	           const SUnitBaseRPGStats *pStats,
	           const float fHP,
	           const uint16_t dir,
	           const uint8_t player,
	           ICollisionsCollector *pCollisionsCollector ) override;

	IStatesFactory *GetStatesFactory() const override
	{
		return CTrainCarStatesFactory::Instance();
	}

	void TakeDamage( const float fDamage,
	                 const SWeaponRPGStats::SShell *pShell,
	                 const int nPlayerOfShoot,
	                 CAIUnit *pShotUnit ) override;

	const bool CanRotate() const override
	{
		return false;
	}

	virtual bool CanTurnToFrontDir( const uint16_t wDir )
	{
		return false;
	}

	const bool CanShootInMovement() override
	{
		return true;
	}

	const bool NeedDeinstall() const override
	{
		return false;
	}

	class CArtillery *GetTowedArtillery() const override
	{
		return 0;
	}

	void Segment() override;

	// Train-specific
	const SRailRoadSystem::SRRInstance *GetTrack() const;

	float GetTrackPos() const
	{
		return fTrackPos;
	}

	float GetTrackLimit() const
	{
		return fTrackLimit;
	}

	const bool IsBackwards() const
	{
		return bBackward;
	}

	void SetToTrackPos( float fPos );
	void SetCollision( ICollision *pCollision, IPath *pPath ) override;
	const ECollidingType GetCollidingType( CBasePathUnit *pUnit ) const override;

	void Die( const bool fromExplosion, const float fDamage ) override;

	// Train car-specific
	CTrainLocomotive *GetLocomotive() const
	{
		return pLocomotive;
	}

	CTrainCar *GetFrontCar() const
	{
		return pLinkFront;
	}

	CTrainCar *GetBackCar() const
	{
		return pLinkBack;
	}

	void LinkToCar( CMilitaryCar *pLinkTo );

	const float GetFrontLink() const
	{
		return fFrontLink;
	}

	const float GetBackLink() const
	{
		return fBackLink;
	}

	const float GetLength() const
	{
		return fFrontLink + fBackLink;
	}
};

//	CLocomotivePath

class CLocomotivePath : public ISmoothPath
{
	OBJECT_BASIC_METHODS( CLocomotivePath );

	ZDATA
	CVec2 vEndPoint;
	float fCurPos = {};
	float fEndPos = {};
	bool bFinished = {};
	CPtr<CTrainLocomotive> pUnit;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &vEndPoint );
		f.Add( 3, &fCurPos );
		f.Add( 4, &fEndPos );
		f.Add( 5, &bFinished );
		f.Add( 6, &pUnit );
		return 0;
	}

	CLocomotivePath() {}

	bool Init( CBasePathUnit *_pUnit, const CVec2 &_vTargetPoint );

	virtual const CVec2 &GetFinishPoint() const
	{
		return vEndPoint;
	}

	bool IsFinished() const override
	{
		return bFinished;
	}

	void Segment( const NTimer::STime timeDiff ) override;

	// ненужные функции ?
	bool Init( CBasePathUnit *pUnit, IPath *pPath, bool bSmoothTurn, bool bCheckTurn, CAIMap *pAIMap ) override
	{
		return false;
	}

	bool Init( IMemento *pMemento, CBasePathUnit *_pUnit, CAIMap *pAIMap ) override;

	bool InitByFormationPath( class CFormation *pFormation, CBasePathUnit *pUnit ) override
	{
		return false;
	}

	void Stop() override
	{
		fEndPos = fCurPos;
		bFinished = true;
	}

	const bool CanGoBackward() const override
	{
		return ( fCurPos >= fEndPos );
	}

	const bool CanGoForward() const override
	{
		return ( fCurPos <= fEndPos );
	}

	const CVec2 PeekPathPoint( const int nToShift ) const override
	{
		return pUnit->GetCenterPlain();
	}

	IMemento *CreateMemento() const override;

	const float GetSpeed() const override
	{
		return bFinished ? 0 : pUnit->GetSpeed();
	}
};

//	CTrainCarPath

class CTrainCarPath : public ISmoothPath
{
	OBJECT_BASIC_METHODS( CTrainCarPath );

	ZDATA
	CPtr<CTrainCar> pUnit;
	CPtr<CLocomotivePath> pOwner;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pUnit );
		f.Add( 3, &pOwner );
		return 0;
	}

public:
	CTrainCarPath() {}

	bool Init( CBasePathUnit *_pUnit, CLocomotivePath *pOwnerPath );

	virtual const CVec2 &GetFinishPoint() const
	{
		return pOwner->GetFinishPoint();
	}

	bool IsFinished() const override
	{
		return pOwner->IsFinished();
	}

	void Segment( const NTimer::STime timeDiff ) override {}

	// ненужные функции ?
	bool Init( CBasePathUnit *pUnit, IPath *pPath, bool bSmoothTurn, bool bCheckTurn, CAIMap *pAIMap ) override
	{
		return false;
	}

	bool Init( IMemento *pMemento, CBasePathUnit *_pUnit, CAIMap *pAIMap ) override
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
		return pOwner->CanGoBackward();
	}

	const bool CanGoForward() const override
	{
		return pOwner->CanGoForward();
	}

	const CVec2 PeekPathPoint( const int nToShift ) const override
	{
		return pUnit->GetCenterPlain();
	}

	IMemento *CreateMemento() const override
	{
		return 0;
	}

	const float GetSpeed() const override
	{
		return pOwner->GetSpeed();
	}
};

//	SLocomotivePathMemento

struct SLocomotivePathMemento : public IMemento
{
	OBJECT_BASIC_METHODS( SLocomotivePathMemento );

public:
	ZDATA
	float fCurPos = {};
	float fEndPos = {};
	CVec2 vEndPoint;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &fCurPos );
		f.Add( 3, &fEndPos );
		f.Add( 4, &vEndPoint );
		return 0;
	}
};

//	CTrainRestState

class CTrainRestState : public IUnitState, public CFreeFireManager
{
	OBJECT_BASIC_METHODS( CTrainRestState );

	ZDATA_( CFreeFireManager )
	CPtr<CAIUnit> pUnit;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CFreeFireManager *)this );
		f.Add( 2, &pUnit );
		return 0;
	}

	static IUnitState *Instance( CAIUnit *_pUnit );

	CTrainRestState()
		: pUnit( 0 )
	{
	}

	CTrainRestState( CAIUnit *_pUnit );

	void Segment() override;

	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand ) override;

	bool IsAttackingState() const override
	{
		return false;
	}

	const CVec2 GetPurposePoint() const override
	{
		return VNULL2;
	}

	bool IsRestState() const override
	{
		return true;
	}

	EUnitStateNames GetName() override
	{
		return EUSN_REST;
	}
};

//	CLocomotiveMoveState

class CLocomotiveMoveState : public IUnitState, public CFreeFireManager
{
	OBJECT_BASIC_METHODS( CLocomotiveMoveState );

	ZDATA_( CFreeFireManager )
	NTimer::STime startTime = {};
	CPtr<CTrainLocomotive> pUnit;
	bool bWaiting = {};
	CVec2 point;
	CPtr<CLocomotivePath> pPath;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CFreeFireManager *)this );
		f.Add( 2, &startTime );
		f.Add( 3, &pUnit );
		f.Add( 4, &bWaiting );
		f.Add( 5, &point );
		f.Add( 6, &pPath );
		return 0;
	}

	static IUnitState *Instance( CAIUnit *_pUnit, const CVec2 &_point );

	CLocomotiveMoveState()
		: pUnit( 0 )
	{
	}

	CLocomotiveMoveState( CTrainLocomotive *_pUnit, const CVec2 &point );

	void Segment() override;

	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand ) override;

	bool IsAttackingState() const override
	{
		return false;
	}

	const CVec2 GetPurposePoint() const override
	{
		return point;
	}

	EUnitStateNames GetName() override
	{
		return EUSN_MOVE;
	}

	CLocomotivePath *GetPath()
	{
		return pPath;
	}
};

//	CLocomotiveAttackUnitState

class CLocomotiveAttackUnitState : public IUnitAttackingState
{
	OBJECT_BASIC_METHODS( CLocomotiveAttackUnitState );

	enum
	{
		TIME_OF_WAITING = 200
	};

	enum EAttackStates
	{
		EAS_STARTING,
		EAS_MOVING,
		EAS_FIRING
	};

	ZDATA
	CPtr<CTrainLocomotive> pUnit;
	CPtr<CAIUnit> pTarget;
	EAttackStates eState = {};
	CPtr<CLocomotivePath> pPath;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pUnit );
		f.Add( 3, &pTarget );
		f.Add( 4, &eState );
		f.Add( 5, &pPath );
		return 0;
	}

private:
	const bool TryFiring();

public:
	static IUnitState *Instance( CAIUnit *_pUnit, CAIUnit *_pTarget );

	CLocomotiveAttackUnitState()
		: pUnit( 0 )
	{
	}

	CLocomotiveAttackUnitState( CTrainLocomotive *_pUnit, CAIUnit *_pTarget );

	void Segment() override;

	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand ) override;

	bool IsAttackingState() const override
	{
		return true;
	}

	const CVec2 GetPurposePoint() const override
	{
		return IsValid( pTarget ) ? pTarget->GetCenterPlain() : VNULL2;
	}

	bool IsAttacksUnit() const override
	{
		return true;
	}

	class CAIUnit *GetTargetUnit() const override
	{
		return pTarget;
	}

	EUnitStateNames GetName() override
	{
		return EUSN_ATTACK_UNIT;
	}

	CLocomotivePath *GetPath()
	{
		return pPath;
	}
};

//	CLocomotiveAttackObjState

class CLocomotiveAttackObjState : public IUnitAttackingState
{
	OBJECT_BASIC_METHODS( CLocomotiveAttackObjState );

	enum
	{
		TIME_OF_WAITING = 200
	};

	enum EAttackStates
	{
		EAS_STARTING,
		EAS_MOVING,
		EAS_FIRING
	};

	ZDATA
	CPtr<CTrainLocomotive> pUnit;
	CPtr<CStaticObject> pTarget;
	EAttackStates eState = {};
	CPtr<CLocomotivePath> pPath;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pUnit );
		f.Add( 3, &pTarget );
		f.Add( 4, &eState );
		f.Add( 5, &pPath );
		return 0;
	}

private:
	const bool TryFiring();

public:
	static IUnitState *Instance( CAIUnit *_pUnit, CStaticObject *_pTarget );

	CLocomotiveAttackObjState()
		: pUnit( 0 )
	{
	}

	CLocomotiveAttackObjState( CTrainLocomotive *_pUnit, CStaticObject *_pTarget );

	void Segment() override;

	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand ) override;

	bool IsAttackingState() const override
	{
		return true;
	}

	const CVec2 GetPurposePoint() const override
	{
		return IsValid( pTarget ) ? GetTargetCenter() : VNULL2;
	}

	bool IsAttacksUnit() const override
	{
		return true;
	}

	CStaticObject *GetTarget() const
	{
		return pTarget;
	}

	const CVec2 GetTargetCenter() const
	{
		return CVec2( pTarget->GetCenter().x, pTarget->GetCenter().y );
	}

	CAIUnit *GetTargetUnit() const override
	{
		return 0;
	}

	EUnitStateNames GetName() override
	{
		return EUSN_ATTACK_STAT_OBJECT;
	}

	CLocomotivePath *GetPath()
	{
		return pPath;
	}
};

//	CTrainCarMoveToState

class CTrainCarMoveToState : public IUnitState, public CFreeFireManager
{
	OBJECT_BASIC_METHODS( CTrainCarMoveToState );

	ZDATA_( CFreeFireManager )
	CPtr<CTrainCar> pUnit;
	bool bWaiting = {};
	CPtr<CTrainCarPath> pPath;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CFreeFireManager *)this );
		f.Add( 2, &pUnit );
		f.Add( 3, &bWaiting );
		f.Add( 4, &pPath );
		return 0;
	}

	static IUnitState *Instance( CAIUnit *_pUnit );

	CTrainCarMoveToState()
		: pUnit( 0 )
	{
	}

	CTrainCarMoveToState( CTrainCar *_pUnit );

	void Segment() override;

	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand ) override;

	bool IsAttackingState() const override
	{
		return false;
	}

	const CVec2 GetPurposePoint() const override
	{
		return pUnit->GetLocomotive()->GetState()->GetPurposePoint();
	}

	EUnitStateNames GetName() override
	{
		return EUSN_MOVE;
	}
};

//	CTrainCarAttackUnitState

class CTrainCarAttackUnitState : public IUnitState
{
	OBJECT_BASIC_METHODS( CTrainCarAttackUnitState );

	enum EAttackStates
	{
		EAS_STARTING,
		EAS_WAITING,
		EAS_FOLLOWING,
		EAS_FIRING
	};

	ZDATA
	CPtr<CTrainCar> pUnit;
	EAttackStates eState = {};
	CPtr<CTrainCarPath> pPath;
	CPtr<CAIUnit> pTarget;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pUnit );
		f.Add( 3, &eState );
		f.Add( 4, &pPath );
		f.Add( 5, &pTarget );
		return 0;
	}

	static IUnitState *Instance( CAIUnit *_pUnit, CAIUnit *_pTarget );

	CTrainCarAttackUnitState()
		: pUnit( 0 )
	{
	}

	CTrainCarAttackUnitState( CTrainCar *_pUnit, CAIUnit *_pTarget );

	void Segment() override;

	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand ) override;

	const CVec2 GetPurposePoint() const override
	{
		return IsValid( pTarget ) ? pTarget->GetCenterPlain() : VNULL2;
	}

	bool IsAttackingState() const override
	{
		return true;
	}

	virtual bool IsAttacksUnit() const
	{
		return true;
	}

	virtual class CAIUnit *GetTargetUnit() const
	{
		return pTarget;
	}

	EUnitStateNames GetName() override
	{
		return EUSN_ATTACK_UNIT;
	}

	// TrainCar-specific
	const bool TryFiring();
};

//	CTrainCarAttackObjectState

class CTrainCarAttackObjectState : public IUnitState
{
	OBJECT_BASIC_METHODS( CTrainCarAttackObjectState );

	enum EAttackStates
	{
		EAS_STARTING,
		EAS_WAITING,
		EAS_FOLLOWING,
		EAS_FIRING
	};

	ZDATA
	CPtr<CTrainCar> pUnit;
	EAttackStates eState = {};
	CPtr<CTrainCarPath> pPath;
	CPtr<CStaticObject> pTarget;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pUnit );
		f.Add( 3, &eState );
		f.Add( 4, &pPath );
		f.Add( 5, &pTarget );
		return 0;
	}

	static IUnitState *Instance( CAIUnit *_pUnit, CStaticObject *_pTarget );

	CTrainCarAttackObjectState()
		: pUnit( 0 )
	{
	}

	CTrainCarAttackObjectState( CTrainCar *_pUnit, CStaticObject *_pTarget );

	void Segment() override;

	const CVec2 GetPurposePoint() const override
	{
		return IsValid( pTarget ) ? GetTargetCenter() : VNULL2;
	}

	virtual bool IsAttacksUnit() const
	{
		return true;
	}

	CStaticObject *GetTarget() const
	{
		return pTarget;
	}

	const CVec2 GetTargetCenter() const
	{
		return CVec2( pTarget->GetCenter().x, pTarget->GetCenter().y );
	}

	virtual CAIUnit *GetTargetUnit() const
	{
		return 0;
	}

	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand ) override;

	bool IsAttackingState() const override
	{
		return true;
	}

	EUnitStateNames GetName() override
	{
		return EUSN_ATTACK_UNIT;
	}

	// TrainCar-specific
	const bool TryFiring();
};
