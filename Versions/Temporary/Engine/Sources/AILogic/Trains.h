#pragma once

#include "Technics.h"
#include "UnitStates.h"
#include "StatesFactory.h"
#include "RailRoads.h"
#include "FreeFireManager.h"
#include "StaticObject.h"

#include <cstdint>

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
	int operator&( IBinSaver &saver )
	{
		return 0;
	}
	static IStatesFactory* Instance();

	virtual bool CanCommandBeExecuted( class CAICommand *pCommand );

	virtual struct IUnitState* ProduceState( class CQueueUnit *pObj, class CAICommand *pCommand );
	virtual struct IUnitState* ProduceRestState( class CQueueUnit *pUnit );

	// for Saving/Loading of static members
	friend class CStaticMembers;
};

//	CTrainCarStatesFactory

class CTrainCarStatesFactory : public IStatesFactory
{
	OBJECT_BASIC_METHODS( CTrainCarStatesFactory );

	static CPtr<CTrainCarStatesFactory> pFactory;
public:
	int operator&( IBinSaver &saver )
	{
		return 0;
	}
	static IStatesFactory* Instance();

	virtual bool CanCommandBeExecuted( class CAICommand *pCommand );

	virtual struct IUnitState* ProduceState( class CQueueUnit *pObj, class CAICommand *pCommand );
	virtual struct IUnitState* ProduceRestState( class CQueueUnit *pUnit );

	// for Saving/Loading of static members
	friend class CStaticMembers;
};

//	CTrainLocomotive

class CTrainCar;
class CTrainLocomotive : public CMilitaryCar
{
	OBJECT_NOCOPY_METHODS( CTrainLocomotive );

	// List of cars
	typedef std::list< CPtr<CAIUnit> > CCarList;
	ZDATA_(CMilitaryCar)
	CCarList cars;

	int		nTrack;
	bool	bBackward;				// Front of the unit faces point 0 of the VSO
	float fTrackPos;
	float fTrackLimit;

	float fFrontOffset;
	float fBackOffset;
	float fTrainLength;
	float fBackLink;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CMilitaryCar*)this); f.Add(2,&cars); f.Add(3,&nTrack); f.Add(4,&bBackward); f.Add(5,&fTrackPos); f.Add(6,&fTrackLimit); f.Add(7,&fFrontOffset); f.Add(8,&fBackOffset); f.Add(9,&fTrainLength); f.Add(10,&fBackLink); return 0; }

	virtual void Init( const CVec2 &center, const int z, const SUnitBaseRPGStats *pStats, const float fHP, const uint16_t dir, const uint8_t player, ICollisionsCollector *pCollisionsCollector );
	virtual IStatesFactory* GetStatesFactory() const { return CLocomotiveStatesFactory::Instance(); }

	virtual void TakeDamage( const float fDamage, const SWeaponRPGStats::SShell *pShell, const int nPlayerOfShoot, CAIUnit *pShotUnit );
	virtual const bool CanRotate() const { return false; }
	virtual bool CanTurnToFrontDir( const uint16_t wDir ) { return false; }
	virtual const bool CanShootInMovement() { return true; }

	virtual const bool NeedDeinstall() const { return false; }

	virtual class CArtillery* GetTowedArtillery() const { return 0; }

	virtual void Segment();

	// Train-specific
	const SRailRoadSystem::SRRInstance *GetTrack() const;
	float GetTrackPos() const { return fTrackPos; }
	float GetTrackLimit() const { return fTrackLimit; }
	const bool IsBackwards() const { return bBackward; }
	void SetToTrackPos( float fPos );
	virtual void SetCollision( ICollision *pCollision, IPath *pPath );
	virtual const ECollidingType GetCollidingType( CBasePathUnit *pUnit ) const;

	virtual void Die( const bool fromExplosion, const float fDamage );

	// Locomotive-specific
	void AddCar( CTrainCar *pNewCar );								// Add car to the end of the list
	void PassCommandToAll( CAICommand *pCommand );		// Pass command to all cars
	const float GetTrainLength();
	const bool TryFiringAll();
	virtual void Stop();															// Stop all cars
	CAIUnit *GetLastCar();
	void SetMovingTileLocks( bool bMoving );
};

//	CTrainCar

class CTrainCar : public CMilitaryCar
{
	friend class CTrainLocomotive;
	OBJECT_NOCOPY_METHODS( CTrainCar );

	ZDATA_(CMilitaryCar)

	CPtr<CTrainLocomotive> pLocomotive;
	CPtr<CTrainCar> pLinkFront;
	CPtr<CTrainCar> pLinkBack;
	int		nTrack;
	bool	bBackward;				// Front of the unit faces point 0 of the VSO
	float fTrackPos;
	float fTrackLimit;

	float fFrontOffset;
	float fBackOffset;
	float fFrontLink;
	float fBackLink;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CMilitaryCar*)this); f.Add(2,&pLocomotive); f.Add(3,&pLinkFront); f.Add(4,&pLinkBack); f.Add(5,&nTrack); f.Add(6,&bBackward); f.Add(7,&fTrackPos); f.Add(8,&fTrackLimit); f.Add(9,&fFrontOffset); f.Add(10,&fBackOffset); f.Add(11,&fFrontLink); f.Add(12,&fBackLink); return 0; }
private:

	void LinkToLocomotive( CTrainLocomotive *pLinkTo );
public:

	virtual void Init( const CVec2 &center, const int z, const SUnitBaseRPGStats *pStats, const float fHP, const uint16_t dir, const uint8_t player, ICollisionsCollector *pCollisionsCollector );
	virtual IStatesFactory* GetStatesFactory() const { return CTrainCarStatesFactory::Instance(); }

	virtual void TakeDamage( const float fDamage, const SWeaponRPGStats::SShell *pShell, const int nPlayerOfShoot, CAIUnit *pShotUnit );
	virtual const bool CanRotate() const { return false; }
	virtual bool CanTurnToFrontDir( const uint16_t wDir ) { return false; }
	virtual const bool CanShootInMovement() { return true; }

	virtual const bool NeedDeinstall() const { return false; }

	virtual class CArtillery* GetTowedArtillery() const { return 0; }

	virtual void Segment();

	// Train-specific
	const SRailRoadSystem::SRRInstance *GetTrack() const;
	float GetTrackPos() const { return fTrackPos; }
	float GetTrackLimit() const { return fTrackLimit; }
	const bool IsBackwards() const { return bBackward; }
	void SetToTrackPos( float fPos );
	virtual void SetCollision( ICollision *pCollision, IPath *pPath );
	virtual const ECollidingType GetCollidingType( CBasePathUnit *pUnit ) const;

	virtual void Die( const bool fromExplosion, const float fDamage );

	// Train car-specific
	CTrainLocomotive *GetLocomotive() const { return pLocomotive; }
	CTrainCar *GetFrontCar() const { return pLinkFront; }
	CTrainCar *GetBackCar() const { return pLinkBack; }
	void LinkToCar( CMilitaryCar *pLinkTo );
	const float GetFrontLink() const { return fFrontLink; }
	const float GetBackLink() const { return fBackLink; }
	const float GetLength() const { return fFrontLink + fBackLink; }
};

//	CLocomotivePath

class CLocomotivePath : public ISmoothPath
{
	OBJECT_BASIC_METHODS( CLocomotivePath );

	ZDATA
	CVec2 vEndPoint;
	float fCurPos;
	float fEndPos;
	bool bFinished;
	CPtr<CTrainLocomotive> pUnit;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&vEndPoint); f.Add(3,&fCurPos); f.Add(4,&fEndPos); f.Add(5,&bFinished); f.Add(6,&pUnit); return 0; }
	CLocomotivePath() { }
	bool Init( CBasePathUnit *_pUnit, const CVec2 &_vTargetPoint );

	virtual const CVec2& GetFinishPoint() const { return vEndPoint; }
	virtual bool IsFinished() const { return bFinished; }
	virtual void Segment( const NTimer::STime timeDiff );

	// ненужные функции ?
	virtual bool Init( CBasePathUnit *pUnit, IPath *pPath, bool bSmoothTurn, bool bCheckTurn, CAIMap *pAIMap ) { return false; }
	virtual bool Init( IMemento *pMemento, CBasePathUnit *_pUnit, CAIMap *pAIMap );
	virtual bool InitByFormationPath( class CFormation *pFormation, CBasePathUnit *pUnit ) { return false; }
	virtual void Stop() { fEndPos = fCurPos; bFinished = true; }

	virtual const bool CanGoBackward() const { return ( fCurPos >= fEndPos ); }
	virtual const bool CanGoForward() const { return ( fCurPos <= fEndPos ); }
	virtual const CVec2 PeekPathPoint( const int nToShift ) const { return pUnit->GetCenterPlain(); }
	virtual IMemento* CreateMemento() const;
	virtual const float GetSpeed() const { return bFinished ? 0 : pUnit->GetSpeed(); }
};

//	CTrainCarPath

class CTrainCarPath : public ISmoothPath
{
	OBJECT_BASIC_METHODS( CTrainCarPath );

	ZDATA
	CPtr<CTrainCar> pUnit;
	CPtr<CLocomotivePath> pOwner;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&pOwner); return 0; }
public:
	CTrainCarPath() { }
	bool Init( CBasePathUnit *_pUnit, CLocomotivePath *pOwnerPath );

	virtual const CVec2& GetFinishPoint() const { return pOwner->GetFinishPoint(); }
	virtual bool IsFinished() const { return pOwner->IsFinished(); }
	virtual void Segment( const NTimer::STime timeDiff ) {}

	// ненужные функции ?
	virtual bool Init( CBasePathUnit *pUnit, IPath *pPath, bool bSmoothTurn, bool bCheckTurn, CAIMap *pAIMap ) { return false; }
	virtual bool Init( IMemento *pMemento, CBasePathUnit *_pUnit, CAIMap *pAIMap ) { return false; }
	virtual bool InitByFormationPath( class CFormation *pFormation, CBasePathUnit *pUnit ) { return false; }
	virtual void Stop() { }

	virtual const bool CanGoBackward() const { return pOwner->CanGoBackward(); }
	virtual const bool CanGoForward() const { return pOwner->CanGoForward(); }
	virtual const CVec2 PeekPathPoint( const int nToShift ) const { return pUnit->GetCenterPlain(); }
	virtual IMemento* CreateMemento() const { return 0; }
	virtual const float GetSpeed() const { return pOwner->GetSpeed(); }
};

//	SLocomotivePathMemento

struct SLocomotivePathMemento : public IMemento
{
	OBJECT_BASIC_METHODS( SLocomotivePathMemento );

public:
	ZDATA
	float fCurPos;
	float fEndPos;
	CVec2 vEndPoint;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&fCurPos); f.Add(3,&fEndPos); f.Add(4,&vEndPoint); return 0; }

};

//	CTrainRestState

class CTrainRestState : public IUnitState, public CFreeFireManager
{
	OBJECT_BASIC_METHODS( CTrainRestState );

	ZDATA_(CFreeFireManager)
	CPtr<CAIUnit> pUnit;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CFreeFireManager*)this); f.Add(2,&pUnit); return 0; }
	static IUnitState* Instance( CAIUnit *_pUnit );

	CTrainRestState() : pUnit( 0 ) { }
	CTrainRestState( CAIUnit *_pUnit );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return VNULL2; }
	virtual bool IsRestState() const { return true; }

	virtual EUnitStateNames GetName() { return EUSN_REST; }
};

//	CLocomotiveMoveState

class CLocomotiveMoveState : public IUnitState, public CFreeFireManager
{
	OBJECT_BASIC_METHODS( CLocomotiveMoveState );

	ZDATA_(CFreeFireManager)
	NTimer::STime startTime;
	CPtr<CTrainLocomotive> pUnit;
	bool bWaiting;
	CVec2 point;
	CPtr<CLocomotivePath> pPath;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CFreeFireManager*)this); f.Add(2,&startTime); f.Add(3,&pUnit); f.Add(4,&bWaiting); f.Add(5,&point); f.Add(6,&pPath); return 0; }
	static IUnitState* Instance( CAIUnit *_pUnit, const CVec2 &_point );

	CLocomotiveMoveState() : pUnit( 0 ) { }
	CLocomotiveMoveState( CTrainLocomotive *_pUnit, const CVec2 &point );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return point; }

	virtual EUnitStateNames GetName() { return EUSN_MOVE; }

	CLocomotivePath *GetPath() { return pPath; }
};

//	CLocomotiveAttackUnitState

class CLocomotiveAttackUnitState : public IUnitAttackingState
{
	OBJECT_BASIC_METHODS( CLocomotiveAttackUnitState );

	enum { TIME_OF_WAITING = 200 };

	enum EAttackStates { EAS_STARTING, EAS_MOVING, EAS_FIRING };

	ZDATA
	CPtr<CTrainLocomotive> pUnit;
	CPtr<CAIUnit> pTarget;
	EAttackStates eState;
	CPtr<CLocomotivePath> pPath;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&pTarget); f.Add(4,&eState); f.Add(5,&pPath); return 0; }
private:
	const bool TryFiring();
public:

	static IUnitState* Instance( CAIUnit *_pUnit, CAIUnit *_pTarget );

	CLocomotiveAttackUnitState() : pUnit( 0 ) { }
	CLocomotiveAttackUnitState( CTrainLocomotive *_pUnit, CAIUnit *_pTarget );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const { return IsValid(pTarget)?pTarget->GetCenterPlain():VNULL2; }

	virtual bool IsAttacksUnit() const { return true;	}
	virtual class CAIUnit* GetTargetUnit() const { return pTarget; }

	virtual EUnitStateNames GetName() { return EUSN_ATTACK_UNIT; }

	CLocomotivePath *GetPath() { return pPath; }
};

//	CLocomotiveAttackObjState

class CLocomotiveAttackObjState : public IUnitAttackingState
{
	OBJECT_BASIC_METHODS( CLocomotiveAttackObjState );

	enum { TIME_OF_WAITING = 200 };

	enum EAttackStates { EAS_STARTING, EAS_MOVING, EAS_FIRING };

	ZDATA
	CPtr<CTrainLocomotive> pUnit;
	CPtr<CStaticObject> pTarget;
	EAttackStates eState;
	CPtr<CLocomotivePath> pPath;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&pTarget); f.Add(4,&eState); f.Add(5,&pPath); return 0; }
private:
	const bool TryFiring();

public:
	static IUnitState* Instance( CAIUnit *_pUnit, CStaticObject *_pTarget );

	CLocomotiveAttackObjState() : pUnit( 0 ) { }
	CLocomotiveAttackObjState( CTrainLocomotive *_pUnit, CStaticObject *_pTarget );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const { return IsValid(pTarget) ? GetTargetCenter() : VNULL2; }

	virtual bool IsAttacksUnit() const { return true;	}
	CStaticObject* GetTarget() const { return pTarget; }
	const CVec2 GetTargetCenter() const { return CVec2( pTarget->GetCenter().x, pTarget->GetCenter().y ); }
	virtual CAIUnit* GetTargetUnit() const { return 0; }

	virtual EUnitStateNames GetName() { return EUSN_ATTACK_STAT_OBJECT; }

	CLocomotivePath *GetPath() { return pPath; }
};

//	CTrainCarMoveToState

class CTrainCarMoveToState : public IUnitState, public CFreeFireManager
{
	OBJECT_BASIC_METHODS( CTrainCarMoveToState );

	ZDATA_(CFreeFireManager)
	CPtr<CTrainCar> pUnit;
	bool bWaiting;
	CPtr<CTrainCarPath> pPath;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CFreeFireManager*)this); f.Add(2,&pUnit); f.Add(3,&bWaiting); f.Add(4,&pPath); return 0; }
	static IUnitState* Instance( CAIUnit *_pUnit );

	CTrainCarMoveToState() : pUnit( 0 ) { }
	CTrainCarMoveToState( CTrainCar *_pUnit );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return pUnit->GetLocomotive()->GetState()->GetPurposePoint(); }

	virtual EUnitStateNames GetName() { return EUSN_MOVE; }
};

//	CTrainCarAttackUnitState

class CTrainCarAttackUnitState : public IUnitState
{
	OBJECT_BASIC_METHODS( CTrainCarAttackUnitState );

	enum EAttackStates { EAS_STARTING, EAS_WAITING, EAS_FOLLOWING, EAS_FIRING };

	ZDATA
	CPtr<CTrainCar> pUnit;
	EAttackStates eState;
	CPtr<CTrainCarPath> pPath;
	CPtr<CAIUnit> pTarget;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&eState); f.Add(4,&pPath); f.Add(5,&pTarget); return 0; }
	static IUnitState* Instance( CAIUnit *_pUnit, CAIUnit *_pTarget );

	CTrainCarAttackUnitState() : pUnit( 0 ) { }
	CTrainCarAttackUnitState( CTrainCar *_pUnit, CAIUnit *_pTarget );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual const CVec2 GetPurposePoint() const { return IsValid( pTarget ) ? pTarget->GetCenterPlain() : VNULL2; }
	virtual bool IsAttackingState() const { return true; }
	virtual bool IsAttacksUnit() const { return true;	}
	virtual class CAIUnit* GetTargetUnit() const { return pTarget; }

	virtual EUnitStateNames GetName() { return EUSN_ATTACK_UNIT; }

	// TrainCar-specific
	const bool TryFiring();
};

//	CTrainCarAttackObjectState

class CTrainCarAttackObjectState : public IUnitState
{
	OBJECT_BASIC_METHODS( CTrainCarAttackObjectState );

	enum EAttackStates { EAS_STARTING, EAS_WAITING, EAS_FOLLOWING, EAS_FIRING };

	ZDATA
		CPtr<CTrainCar> pUnit;
	EAttackStates eState;
	CPtr<CTrainCarPath> pPath;
	CPtr<CStaticObject> pTarget;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&eState); f.Add(4,&pPath); f.Add(5,&pTarget); return 0; }
	static IUnitState* Instance( CAIUnit *_pUnit, CStaticObject *_pTarget );

	CTrainCarAttackObjectState() : pUnit( 0 ) { }
	CTrainCarAttackObjectState( CTrainCar *_pUnit, CStaticObject *_pTarget );

	virtual void Segment();

	virtual const CVec2 GetPurposePoint() const { return IsValid(pTarget) ? GetTargetCenter() : VNULL2; }

	virtual bool IsAttacksUnit() const { return true;	}
	CStaticObject* GetTarget() const { return pTarget; }
	const CVec2 GetTargetCenter() const { return CVec2( pTarget->GetCenter().x, pTarget->GetCenter().y ); }
	virtual CAIUnit* GetTargetUnit() const { return 0; }
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return true; }

	virtual EUnitStateNames GetName() { return EUSN_ATTACK_UNIT; }

	// TrainCar-specific
	const bool TryFiring();
};

