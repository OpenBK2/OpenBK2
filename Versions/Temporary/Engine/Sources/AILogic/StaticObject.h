#pragma once

#include "LinkObject.h"
#include "StaticObjectRotation.h"
#include "Common_RTS_AI/Terrain.h"
#include "Stats_b2_m1/RPGStats.h"

class CAIUnit;
class CObjectProfile;
namespace NDb
{
	struct SHPObjectRPGStats;
	struct SObjectBaseRPGStats;
	struct SStaticObjectRPGStats;
}

enum EStaticObjType 
{ 
	ESOT_COMMON, 
	ESOT_BUILDING, 
	ESOT_MINE, 
	ESOT_ENTR_PART, 
	ESOT_ENTRENCHMENT, 
	ESOT_TERRA, 
	ESOT_BRIDGE_SPAN,
	ESOT_TANKPIT,
	ESOT_FENCE,
	ESOT_SMOKE_SCREEN,
	ESOT_FLAG,
	ESOT_ARTILLERY_BULLET_STORAGE,
	ESOT_FAKE_CORPSE,
	ESOT_CANT_CRUSH,
};

//*******************************************************************
//*												  CStaticObject														*
//*******************************************************************

class CStaticObject : public CLinkObject
{
	// if this flag is set, then static object will terminate all executors
	ZDATA_(CLinkObject)
	bool bTerminateSegmentFlag;			
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CLinkObject*)this); f.Add(2,&bTerminateSegmentFlag); return 0; }
public:
	CStaticObject() : bTerminateSegmentFlag( false ) {  }
	void SetTerminateExecutorFlag( const bool _bTerminateSegmentFlag ) { bTerminateSegmentFlag = _bTerminateSegmentFlag; }
	bool IsTerminateExecutors() const { return bTerminateSegmentFlag; }

	virtual const SHPObjectRPGStats* GetStats() const = 0;
	
	// расположение объекта
	virtual const CVec3& GetCenter() const = 0;
	virtual const CVec2 GetAttackCenter( const CVec2 &vPoint ) const = 0;
	virtual void GetBoundRect( SRect *pRect ) const = 0;
	virtual void GetCoveredTiles( std::list<SVector> *pTiles ) const = 0;
	virtual bool IsPointInside( const CVec2 &point ) const = 0;
	virtual const WORD GetDir() const = 0;

	// hit points и damage
	virtual const float GetHitPoints() const = 0;
	virtual void SetHitPoints( const float fNewHP ) { NI_ASSERT(false, "wrong call,CStaticObject::SetHitPoints"); }

	virtual void TakeDamage( const float fDamage, const bool bFromExplosion, const int nPlayerOfShoot, CAIUnit *pShotUnit ) = 0;

	// сегмент для обработки некоторой внутренней логики объекта
	virtual void Segment() = 0;
	virtual const NTimer::STime GetNextSegmentTime() const { return 0; }

	virtual EStaticObjType GetObjectType() const = 0;
	virtual const BYTE GetPlayer() const;
		
	virtual bool IsContainer() const = 0;
	virtual const int GetNDefenders() const = 0;
	virtual class CSoldier* GetUnit( const int n ) const = 0;
	
	// для suspended updates
	virtual const bool IsVisible( const BYTE cParty ) const;
	virtual void GetTilesForVisibility( CTilesSet *pTiles ) const;
	virtual bool ShouldSuspendAction( const EActionNotify &eAction ) const;

	// может ли юнит класса eClass проехать сквозь объект
	virtual bool CanUnitGoThrough( const EAIClasses &eClass ) const = 0;
};

//*******************************************************************
//*													ILoadableObject													*
//*******************************************************************

// объект, внутри которого могут быть юниты
struct ILoadableObject
{
	// итерирование по fire slots
	virtual void StartIterate() = 0;
	virtual void Iterate() = 0;
	virtual bool IsIterateFinished() = 0;
	virtual class CAIUnit* GetIteratedUnit() = 0;
};

// check IsProhibited before do anything that can change sinchronization
class CExistingObjectModifyAI
{
	static int bProhibited;
public:
	CExistingObjectModifyAI()
	{
		++bProhibited;
	}
	~CExistingObjectModifyAI()
	{
		--bProhibited;
	}
	static void Clear()
	{
		bProhibited = 0;
	}
	static bool IsProhibited()  { return bProhibited; }
};

// объект, реально находящийся на карте
class CExistingObject : public CStaticObject
{
	typedef unsigned long ulong;
	ZDATA_( CStaticObject )
	ulong mark;

	NTimer::STime burningEnd;
	bool bTrampled;
	int nFrameIndex;
protected:

	float fHP;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CStaticObject *)this); f.Add(2,&mark); f.Add(3,&burningEnd); f.Add(4,&bTrampled); f.Add(5,&nFrameIndex); f.Add(6,&fHP); return 0; }
	static unsigned long globalMark;

	//
	const int GetRandArmorByDir( const int nArmorDir, const WORD wAttackDir );
public:
	virtual void SetNewPlaceWithoutMapUpdate( const CVec3 &center, const WORD dir = 0 ) = 0;
	static void Init()
	{
		globalMark = 0;	
	}
	static void Clear() 
	{ 
	}
	CExistingObject() : bTrampled( false ) { }
	CExistingObject( const int _nFrameIndex, const float _fHP ) 
		: nFrameIndex( _nFrameIndex ), fHP( _fHP ), mark( 0 ), burningEnd( 0 ), bTrampled( false )
	{ 
		SetAlive( fHP > 0.0f );
		if ( !CExistingObjectModifyAI::IsProhibited() )
			SetUniqueIdForObjects(); 
	}

	void SetTrampled() { bTrampled = true; }
	bool IsTrampled() const { return bTrampled; }

	// информация об объекте
	const int GetFrameIndex() const { return nFrameIndex; }

	// hit points и damage
	const float GetHitPoints() const { return fHP; }
	void SetHitPoints( const float fNewHP ) { fHP = fNewHP; SetAlive( fHP > 0.0f ); }

	// выполнение необходимых действий при обращении HitPoints в ноль
	virtual void Die( const float fDamage ) = 0;

	// расположение объекта
	virtual void SetNewPlacement( const CVec3 &center, const WORD dir );

	virtual void LockTiles() = 0;
	virtual void CreateLockedTilesInfo( std::list<SObjTileInfo> *pTiles ) = 0;
	virtual void UnlockTiles() = 0;
	virtual void SetTransparencies() = 0;
	virtual void RemoveTransparencies() = 0;
	// поставить прозрачность ещё раз, если она сейчас поставлена
	virtual void RestoreTransparenciesImmidiately() = 0;
	
	// для updater-а
	virtual void GetPlacement( struct SAINotifyPlacement *pPlacement, const NTimer::STime timeDiff );
	virtual void GetNewUnitInfo( struct SNewUnitInfo *pNewUnitInfo );
	virtual void GetRPGStats( struct SAINotifyRPGStats *pStats ) = 0;

	virtual void Delete();

	// для iterator-а
	virtual bool IsGlobalUpdated() const { return mark == globalMark; }
	virtual void SetGlobalUpdated();
	// for debug
	const int GetMark() const { return mark; }

	static void UpdateGlobalMark();
	
	// true при попадании
	virtual bool ProcessCumulativeExpl( class CExplosion *pExpl, const int nArmorDir, const bool bFromExpl );
	virtual bool ProcessBurstExpl( class CExplosion *pExpl, const int nArmorDir, const float fRadius, const float fSmallRadius );
	virtual bool ProcessAreaDamage( const class CExplosion *pExpl, const int nArmorDir, const float fRadius, const float fSmallRadius ) { return false; }

	// сегмент для горения объекта
	virtual void BurnSegment();
	// попадание по объекту с damage (вызывать только после списывания damage)
	virtual void WasHit();
	// минимальное количество HP для объекта (для всех 0, кроме ключувых зданий)
	virtual const float GetMinHP() const { return 0.0f; }
	
	//if object falls on destruction
	virtual bool CanFall() { return false; }
	//fall to direction
	virtual void AnimateFalling( const CVec2 &vFallTo ) { NI_ASSERT( false, "Wrong call of CExistingObject::AnimateFalling()!" ); }
	virtual bool HasFallen() { return false; }

	virtual CObjectProfile* GetPassProfile() const = 0;
	void SetFrameIndex( const int _nFrameIndex ) { nFrameIndex = _nFrameIndex; }

	friend class CStaticMembers;
};

//*******************************************************************
//*											CGivenPassabilityStObject										*
//*******************************************************************

// объект с заданной проходимостью
class CGivenPassabilityStObject : public CExistingObject
{
	ZDATA_(CExistingObject)
		CVec3 center;
		SRect boundRect;
		CArray2D<BYTE> lockInfo;
		bool bTransparencySet;
		SAIAngle wDir;
		CPtr<CObjectProfile> pPassProfile;
		CPtr<CObjectProfile> pVisProfile;
		std::list<SObjTileInfo> lockedTiles;
		public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CExistingObject*)this); f.Add(2,&center); f.Add(3,&boundRect); f.Add(4,&lockInfo); f.Add(5,&bTransparencySet); f.Add(6,&wDir); f.Add(7,&pPassProfile); f.Add(8,&pVisProfile); f.Add(9,&lockedTiles); return 0; }
protected:
	void RotateFence( const CVec3 &vNewCenter );

	virtual void GetVisibility( CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > *visibity ) const;
	virtual void GetPassability( CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > *passability ) const;

	void SetPassProfile( CObjectProfile * _pPassProfile ) { pPassProfile = _pPassProfile; }
	void SetVisProfile( CObjectProfile * _pVisProfile ) { pVisProfile = _pVisProfile; }

	CObjectProfile* GetVisProfile() const { return pVisProfile; }

	virtual int GetHeight() const;

	void SetTransparenciesInt( const int nUniqueID );
	void RemoveTransparenciesInt( const int nUniqueID );
public:
	virtual void SetNewPlaceWithoutMapUpdate( const CVec3 &_center, const WORD dir = 0 ) { center = _center; }
	CGivenPassabilityStObject() : bTransparencySet( false ) { }
	CGivenPassabilityStObject( const CVec3 &center, const float fHP, const WORD wAngle, const int nFrameIndex );
	virtual void Init();
	virtual const EAIClasses GetPassabilityClass() const;

	virtual const CVec3& GetCenter() const { return center; }
	virtual const CVec2 GetAttackCenter( const CVec2 &vPoint ) const;
	virtual const WORD GetDir() const { return wDir; }

	virtual void GetRPGStats( struct SAINotifyRPGStats *pStats );

	virtual void GetCoveredTiles( std::list<SVector> *pTiles ) const;
	virtual void GetBoundRect( SRect *pRect ) const { *pRect = boundRect; }
	virtual bool IsPointInside( const CVec2 &point ) const;

	virtual void LockTiles();
	virtual void UnlockTiles();
	virtual void CreateLockedTilesInfo( std::list<SObjTileInfo> *pTiles );
	virtual void SetTransparencies() { SetTransparenciesInt( GetUniqueId() ); }
	virtual void RemoveTransparencies() { RemoveTransparenciesInt( GetUniqueId() ); }
	virtual void RestoreTransparenciesImmidiately();
	// true если объект можно добавить в данную точку
	static bool CheckStaticObject( const SObjectBaseRPGStats * pStats, const CVec2 & vPos, const WORD wDir, const int nFrameIndex );
	virtual const int GetNEntrancePoints() const { return 0; }
	virtual void GetEntranceData( CVec2 *pvPoint, WORD *pwDir, int nIndex ) const { return; }

	virtual CObjectProfile* GetPassProfile() const { return pPassProfile; }
};

//*******************************************************************
//*												 CCommonStaticObject											*
//*******************************************************************

class CCommonStaticObject : public CGivenPassabilityStObject
{

	ZDATA_(CGivenPassabilityStObject)
	EStaticObjType eType;
	bool bFallen;
	public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CGivenPassabilityStObject*)this); f.Add(2,&eType); f.Add(3,&bFallen); return 0; }
public:
	CCommonStaticObject() { }
	CCommonStaticObject( const CVec3 &center, const float fHP, const WORD wDir, const int nFrameIndex, EStaticObjType _eType )
		: CGivenPassabilityStObject( center, fHP, wDir, nFrameIndex ), eType( _eType ), bFallen( false ) 
	{
		//CLinkObject::SetUniqueIdForObjects();
	}

	virtual void TakeDamage( const float fDamage, const bool bFromExplosion, const int nPlayerOfShoot, CAIUnit *pShotUnit );
	virtual void Die( const float fDamage );
	
	virtual void Segment() { }

	virtual EStaticObjType GetObjectType() const { return eType; }

	virtual bool IsContainer() const { return false; }
	virtual const int GetNDefenders() const { return 0; }
	virtual class CSoldier* GetUnit( const int n ) const { return 0; }

	virtual bool ProcessAreaDamage( const class CExplosion *pExpl, const int nArmorDir, const float fRadius, const float fSmallRadius );
	void AnimateFalling( const CVec2 &vFallTo );
	bool HasFallen() { return bFallen; }
};

class CSimpleStaticObject : public CCommonStaticObject
{
	OBJECT_BASIC_METHODS( CSimpleStaticObject );
	ZDATA_(CCommonStaticObject)
	CDBPtr<SStaticObjectRPGStats> pStats;
	int nPlayer;
	bool bDelayedUpdate;									// if true then update for new object is delayed
	SAIAngle wDir;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CCommonStaticObject*)this); f.Add(2,&pStats); f.Add(3,&nPlayer); f.Add(4,&bDelayedUpdate); f.Add(5,&wDir); return 0; }
public:
	CSimpleStaticObject() { }
	CSimpleStaticObject( const SStaticObjectRPGStats *_pStats, const CVec3 &center, const WORD _wDir, const float fHP, const int nFrameIndex, EStaticObjType eType, const int nPlayer = -1, const bool bDelayedUpdate = false )
		: pStats( _pStats ), CCommonStaticObject( center, fHP, _wDir, nFrameIndex, eType ), nPlayer( nPlayer ), bDelayedUpdate( bDelayedUpdate ), wDir( _wDir ) { }

	virtual const BYTE GetPlayer() const { return nPlayer == -1 ? CCommonStaticObject::GetPlayer() : nPlayer; }
	virtual const SHPObjectRPGStats* GetStats() const { return pStats; }
	virtual bool CanUnitGoThrough( const EAIClasses &eClass ) const;
	virtual bool ShouldSuspendAction( const EActionNotify &eAction ) const;
	virtual const WORD GetDir() const { return wDir; }
	bool CanFall();
};

class CTerraMeshStaticObject : public CCommonStaticObject
{
	OBJECT_BASIC_METHODS( CTerraMeshStaticObject );
	ZDATA_(CCommonStaticObject)
	CDBPtr<SStaticObjectRPGStats> pStats;	
	SAIAngle wDir;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CCommonStaticObject*)this); f.Add(2,&pStats); f.Add(3,&wDir); return 0; }
public:
	CTerraMeshStaticObject() { }
	CTerraMeshStaticObject( const SStaticObjectRPGStats *_pStats, const CVec3 &center, const WORD _wDir, const float fHP, const int nFrameIndex, EStaticObjType eType )
		: pStats( _pStats ), wDir( _wDir ), CCommonStaticObject( center, fHP, _wDir, nFrameIndex, eType ) { }
	
	virtual const SHPObjectRPGStats *GetStats() const { return pStats; }
	virtual bool CanUnitGoThrough( const EAIClasses &eClass ) const;

	virtual const WORD GetDir() const { return wDir; }

	virtual void SetNewPlacement( const CVec3 &center, const WORD dir );
	virtual void GetPlacement( struct SAINotifyPlacement *pPlacement, const NTimer::STime timeDiff );
	bool CanFall();
};


