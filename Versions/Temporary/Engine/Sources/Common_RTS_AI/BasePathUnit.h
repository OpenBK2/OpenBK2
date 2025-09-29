#pragma once
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "..\Common_RTS_AI\Terrain.h"

#include "Path.h"
#include "StaticPath.h"
#include "Collision.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CAIMap;
class CStaticMapHeights;
class CCollisionsCollector;
class CCommonPathFinder;
interface IPointChecking;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! параметры для выравнивания скорости
enum EAdjustSpeedParam
{
	ADJUST_SLOW,	//! уменьшить скорость в указанное количество раз
	ADJUST_FAST,	//! увеличить скорость в указанное количество раз
	ADJUST_SET,		//! задать скорость
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! "плоскость" движения юнита
enum EMovementPlane
{
	PLANE_TERRAIN,
	PLANE_WATER,
	PLANE_AIR,
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! варианты с кем может коллизиться юнит
enum ECollidingType
{
	ECT_ALL,
	ECT_INTERNAL,
	ECT_NONE,
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CBasePathUnit;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! callback для итерирования по юнитам
struct SIterateUnitsCallback
{
	virtual bool Iterate( CBasePathUnit *unit ) const = 0;
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! Абстрактный класс, представляющий "перемещаемые" юниты. Все что может двигаться в AI, должно наследоваться от
//! CBasePathUnit. Также предоставляет необходимую статическую информации о юните для функций поиска пути.
class CBasePathUnit
{
	CBasePathUnit *pLastPushUnit;
	CBasePathUnit *pLastPusherUnit;
	CPtr<CCommonPathFinder> pPathFinder;

	ZDATA
		ZONSERIALIZE
		ZSKIP

		SAIAngle wDirection;					//! направление куда едем
		SAIAngle wFrontDirection;			//! направление переда

		SAIAngle wLastDirection;			//! предыдущее направление
		bool bGoForward;					//! едем передом

		bool bTurning;						//! поворачиваем
		bool bTurnCalled;					//! на этом сегменте поворачивались

		CVec3 vCenter;						//! положение юнита (с учетом высоты)
		SVector vTile;						//! положение юнита на плоскости карты в AI тайлах
		SVector vLastKnownGoodTile;

		float fSpeed;							//! скорость юнита
		float fDesiredSpeed;			//! желаемая скорость движения

		bool bLocking;						//! юнит залокал тайлы под собой
		bool bOnLockedTiles;			//! юнит на залоканых тайлах
		bool bIdle;								//! нет пути по которому нужно ехать
		bool bFixUnlocking;				

		int nCollisionsCount;

		CPtr<ISmoothPath> pSmoothPath;
		CPtr<ISmoothPath> pDefaultPath;

		CPtr<IMemento>		pPathMemento;

		CPtr<ICollision>  pCurrentCollision;
		CPtr<ICollision>  pInterruptedCollision;
		bool bNoCollision;

		NTimer::STime stayTime;
		NTimer::STime collStayTime;
		NTimer::STime nextSecondPathSegmTime;
		NTimer::STime checkOnLockedTime;

		bool bMaxSlowed, bMinSlowed, bNotified;

		bool bTurningToDirContinuesly;
		SAIAngle wDirToContinueslyTurn;

		EMovementPlane eMovementPlane;
		CPtr<CAIMap> pAIMap;
		CPtr<CStaticMapHeights> pHeights;
		CPtr<ICollisionsCollector> pCollisionsCollector;

		CPtr<ISmoothPath> pInterruptedPath;
		ZSKIP
		bool bPlacementUpdated;
		bool bStoppedSent;
		ZSKIP
		CVec3 vOldPlacement;
		WORD wOldDirection;
	public:
	ZEND int operator&( IBinSaver &f ) { OnSerialize( f ); f.Add(3,&wDirection); f.Add(4,&wFrontDirection); f.Add(5,&wLastDirection); f.Add(6,&bGoForward); f.Add(7,&bTurning); f.Add(8,&bTurnCalled); f.Add(9,&vCenter); f.Add(10,&vTile); f.Add(11,&vLastKnownGoodTile); f.Add(12,&fSpeed); f.Add(13,&fDesiredSpeed); f.Add(14,&bLocking); f.Add(15,&bOnLockedTiles); f.Add(16,&bIdle); f.Add(17,&bFixUnlocking); f.Add(18,&nCollisionsCount); f.Add(19,&pSmoothPath); f.Add(20,&pDefaultPath); f.Add(21,&pPathMemento); f.Add(22,&pCurrentCollision); f.Add(23,&pInterruptedCollision); f.Add(24,&bNoCollision); f.Add(25,&stayTime); f.Add(26,&collStayTime); f.Add(27,&nextSecondPathSegmTime); f.Add(28,&checkOnLockedTime); f.Add(29,&bMaxSlowed); f.Add(30,&bMinSlowed); f.Add(31,&bNotified); f.Add(32,&bTurningToDirContinuesly); f.Add(33,&wDirToContinueslyTurn); f.Add(34,&eMovementPlane); f.Add(35,&pAIMap); f.Add(36,&pHeights); f.Add(37,&pCollisionsCollector); f.Add(38,&pInterruptedPath); f.Add(40,&bPlacementUpdated); f.Add(41,&bStoppedSent); f.Add(43,&vOldPlacement); f.Add(44,&wOldDirection); return 0; }
	private:
	
	void OnSerialize( IBinSaver &f );
	const bool MakeTurnToDirection( const WORD wDirection );

	const bool IterateUnitsProc( const CBasePathUnit *unit ) const;
	const SRect GetUnitModifiedRect( const float fCompress ) const;
	void CalculateIdle();
	void SetNextSecondPathSegmTime( const NTimer::STime time ) { nextSecondPathSegmTime = time; }

protected:
	//! обновить позицию юнита, это должен делать кто-то из AI
	//virtual void UpdatePlacement( const CVec3 &vOldPosition )	{ NI_ASSERT( false, "Illegal call of CBasePathUnit::UpdatePlacement" ); }
	virtual void UpdatePlacement( const CVec3 &vOldPosition, const WORD wOldDirection, const bool bNeedUpdate )	= 0;
	virtual void UpdateTile() = 0;
	//! создать путь при инициализации юнита (для формации создаются свои пути)
	virtual ISmoothPath *CreateSmoothPath();
	//! проверить, не сломали ли чего-нибудь
	virtual void CheckForDestroyedObjects( const CVec2 &vCenter ) const = 0;
	virtual void NullSegmTime() = 0;
	virtual void AdjustSpeed();

	// called when path finished
	virtual void OnIdle() {}
	// called when unit stopped (no placement update)
	virtual void OnStopped() {}
public:
	CBasePathUnit();
	void Init( const CVec3 &vCenter, const WORD wDirection, CAIMap *pAIMap, ICollisionsCollector *pCollisionsCollector, CCommonPathFinder *pPathFinder );

	//! информация о позиции юнита в пространстве (3D)
	const CVec3 & GetCenter() const {	return vCenter; }
	//! информация о позиции юнита на плоскости (2D)
	// hack to increase FPS :)
	const CVec2 & GetCenterPlain() const { return *( reinterpret_cast<const CVec2 *>( &vCenter ) ); }

	CCommonPathFinder* GetPathFinder() const { return pPathFinder; }
	
	//! информация о позиции юнита в AI тайлах
	const SVector GetCenterTile() const { return vTile; }
	//! последнйи тайл, на котором юнит ни кому не мешался
	const SVector GetLastKnownGoodTile() const { return vLastKnownGoodTile; }

	//! проверить, что юнит не будет выходить за пределы карты в новой позиции
	const bool IsValidCenter( const CVec3 &vCenter );
	//! изменить центр юнита, bNeedUpdate - показывает необходимость вызова UpdatePlacement
	virtual void SetCenter( const CVec3 &vCenter, const bool bNeedUpdate = true );
  
	//! проверить, что юнит не будет выходить за пределы карты в новом направлении
	const bool IsValidDirection( const WORD wDirection );
	//! проверить, что юнит не будет выходить за пределы карты в новом направлении (направление задается вектором)
	const bool IsValidDirection( const CVec2 &vDirection ) { return IsValidDirection( GetDirectionByVector( vDirection ) ); }
	//! получить направление движения юнита, 
	const WORD GetDirection() const { return wDirection; }
	const float GetDir() const { return float(wFrontDirection) / 65536.0f * FP_2PI; }

	//! куда смотрит перед юнита
	const WORD GetFrontDirection() const { return wFrontDirection; }
	//! получить напраление движения юнита в векторе
	const CVec2 GetDirectionVector() const { return GetVectorByDirection( GetDirection() ); }
	//! получить, куда собирается ехать юнит (например на поворотах может несколько отличаться от направления движения)
	const CVec2 GetMoveDirection() const;
	//! куда смотрит перед юнита в векторе
	const CVec2 GetFrontDirectionVector() const { return GetVectorByDirection( GetFrontDirection() ); }
	//! установить направление движения юнита
	virtual void SetDirection( const WORD wDirection, const bool bNeedUpdate = true );
	//! установить направление движения юнита вектором
	void SetDirectionVec( const CVec2 &vDirection ) { SetDirection( GetDirectionByVector( vDirection ) ); }
	//! повернуться в указанном направлении
	virtual const bool TurnToDirection( const WORD wDirection, const bool bCanBackward, const bool bCanForward );
	//! повернуться на указанную точку
	virtual const bool TurnToTarget( const CVec2 &vTarget );

	//! можно ли передом повернуться в указанном направлении
	virtual const bool CanTurnToFrontDir( const WORD wDirection ) const;
	//! можно ли повернуться в указанном направлении
	virtual const bool CanTurnTo( const WORD wDirection, const bool bCanRebuildPath = true );
	//! можно ли развернуться на 180 градусов
	bool CanMake180DegreesTurn( SRect rect ) const;

	//! проверить, можно ли развернуться в направлении vDir, fRectCoeff - коеффициент "растяжения" профайла юнита
	virtual const bool CheckTurn( const float fRectCoeff, const CVec2 &vDir, const bool bWithUnits, const bool bCanGoBackward ) const;
	//! установить скорость
	void SetSpeed( const float _fSpeed ) { fSpeed = _fSpeed; }
	//! установить скорость, при помощи параметры eAdjust можно определить как именно менять скорость
	void SetSpeed( const EAdjustSpeedParam eAdjust, const float fValue );
	//! получить текущую скорость
	const float GetSpeed() const { return fSpeed; }
	//! задать желаемую скорость
	void SetDesiredSpeed( const float _fSpeed ) {	fDesiredSpeed = _fSpeed; }
	//! желаемая скорость
	const float GetDesiredSpeed() const { return fDesiredSpeed; }
	//! установить желаемую скорость равную максимальной
	void ResetDesiredSpeed() { fDesiredSpeed = GetMaxPossibleSpeed(); }
	//! максимальная скорость движения в указанной точке. bAdjust - выровнять скорость с учетом желаемой
	const float GetMaxSpeedHere( const CVec2 &vPosition, const bool bAdjust ) const;
	//! максимальная скорость движения в точке, где находиться центр юнита. Результат выравнивается с учетом желаемой скорости
	const float GetMaxSpeedHere() const { return GetMaxSpeedHere( GetCenterPlain(), true ); }
	//! юнит никуда не собирается ехать
	virtual const bool IsIdle() const { return bIdle; }
	//! юнит никуда не едет
	virtual const bool IsMoving() const { return /* GetSmoothPath()->IsUnitMoving() || */ GetSpeed() != 0.0f; }
	//! юнит поворацивается
	virtual const bool IsTurning() const { return bTurning; }
	//! юнит едет передом
	virtual const bool IsGoForward() const { return bGoForward; }
	//! установить направление движения юнита (передом или задом)
	void SetGoForward( const bool bGoForward );
	//! принудительно заставить юнит двигаться передом
	void ForceGoForward() { bGoForward = true; }  

	//! получить "плоскость" движения юнита (вода/земля)
	virtual const EMovementPlane GetMovementPlane() const { return eMovementPlane; }
	//! получить наиболее вероятную "плоскость" движения юнита в заданном AI тайле (вода/земля)
	virtual const EMovementPlane GetProbablePlane( const SVector &vPos ) const;

	//! остановить юнит
	virtual void Stop();
	//! остановить поворот юнита
	virtual void StopTurning() { bTurning = false; }

	//! залокать тайлы под юнитом
	void LockTiles();
	//! залокать тайлы под юнитом (в том числе и под пехотой)
	void ForceLockTiles();
	//! разлокать тайлы под юнитом
	void UnlockTiles();
	void StaticLockTiles() const;
	//! блокирует ли юнит тайлы под собой
	const bool IsLockingTiles() const { return bLocking; }
	//! запретить залокивание тайлов (не работает для ForceLockTiles)
	void FixUnlocking() { bFixUnlocking = true; }
	//! разрешить залокивание тайлов (не работает для ForceLockTiles)
	void UnfixUnlocking() { bFixUnlocking = false; }
	//! может ли юнит локать тайлы
	virtual const bool CanLockTiles() const { return !bLocking; }
	//! проверить, будет ли указанный профайл (profile) стоять на залоканных тайлах
	const bool IsOnLockedTiles( const SUnitProfile &profile ) const;
	//! восстановить залоканость тайлов, вызывается после load'а
	void RestoreLock();
	 
	//! первый сегмент перемещения юнита: разрешение коллизий
	virtual void FirstSegment( const NTimer::STime timeDiff );
	//! второй сегмент перемещения юнита: движение вдоль пути
	virtual void SecondSegment( const NTimer::STime timeDiff );
	//! промежуток времени, через который стоит вызвать второй сегмент для юнита
	virtual const NTimer::STime GetNextSecondPathSegmTime() const { return nextSecondPathSegmTime; }
	void CallUpdatePlacement();

	//! гладкий путь, полученный при создании юнита
	virtual ISmoothPath *GetDefaultPath() const { return pDefaultPath; }
	//! гладкий путь, по которому движется юнит в данный момент
	virtual ISmoothPath *GetSmoothPath() const { return pSmoothPath; }
	//! коллизия в которой участвует юнит 
	virtual ICollision *GetCollision() const { return pCurrentCollision; }
	//! задать коллизию для юнита
	virtual void SetCollision( ICollision *pCollision, IPath *pPath );
	virtual void SetCollisionOld( ICollisionOld *pCollision, IPath *pPath ) {}

	const int GetStayTime() const { return stayTime; }
	void UpdateCollisionStayTime( const NTimer::STime time ) { collStayTime = Max( collStayTime, Min( stayTime, time ) ); }

	void SetLastPushUnit( CBasePathUnit *pUnit ) { pLastPushUnit = pUnit; }
	CBasePathUnit *GetLastPushUnit() const { return pLastPushUnit; }
	CBasePathUnit *GetLastPusherUnit() const { return pLastPusherUnit; }

	void IncCollisionsCount() { ++nCollisionsCount; }
	void ResetCollisionsCount() { nCollisionsCount = 0; }
	const int GetCollisionsCount() { return nCollisionsCount; }

	virtual void SetSmoothPath( ISmoothPath *_pSmoothPath )
	{
		pSmoothPath = _pSmoothPath;
	}
	virtual void RestoreSmoothPath();

	const bool IsPathFinished() const { return GetSmoothPath()->IsFinished(); }  
	//! эта функция ДОЛЖНА быть переопределена в классах - потомках
	virtual const int GetUniqueID() const = 0;

	//! различные статы юнита, по необходимости (т.е. скорее всего) переопределяются потомками
	virtual const float GetMaxPossibleSpeed() const = 0;
	virtual const EAIClasses GetAIPassabilityClass() const = 0;
	virtual const float GetTurnRadius() const = 0;
	virtual const float GetTurnSpeed() const = 0;
	virtual const SUnitProfile &GetUnitProfile() const = 0;
	virtual const SRect & GetUnitRect() const = 0;
	virtual const CVec2 &GetAABBHalfSize() const = 0;
	virtual const CVec2 GetCenterShift() const = 0;
	virtual const bool CanGoBackward() const = 0;
	virtual const bool CanTurnRound() const = 0;
	virtual const bool IsRound() const = 0;
	virtual const int GetBoundTileRadius() const = 0;
	virtual const float GetSmoothTurnThreshold() const = 0;
	virtual const float GetPassability() const = 0;
	virtual const bool CanGoToPoint( const CVec2 &point ) const = 0;
	virtual const bool CanTurnInstant() const { return false; }
	virtual const bool CanChangeSpeedInstand() const { return false; }
	virtual const bool ShouldScatter() const { return false; }
	virtual const bool IsInfantry() const { return false; }
	virtual const bool IsAviation() const { return false; }
	

	//! может ли юнит двигаться
	virtual const bool CanMove() const { return true; }
	//! может ли юнит двигаться если ОЧЕНЬ надо
	virtual const bool CanMoveCritical() const { return true; }
	//! может ли юнит поворачиваться
	virtual const bool CanRotate() const { return true; }


	//! юнит задавлен другим юнитом (pTramplerUnit - юнит, который задавил this)
	virtual void UnitTrampled( const CBasePathUnit *pTramplerUnit ) = 0;
	
	//! юнит может быть задавлен другим юнитом (pTramplerUnit - тот кто собирается давить this)
	//! что-то вроде this - пехота и враг для pTramplerUnit
	virtual const bool CanUnitTrampled( const CBasePathUnit *pTramplerUnit ) const = 0;

	virtual const bool IsDangerousDirExist() const = 0;
	virtual const WORD GetDangerousDir() const = 0;

	//! с кем может коллизиться юнит
	virtual const ECollidingType GetCollidingType( CBasePathUnit *pUnit ) const { return ECT_ALL; }

	//! провести итерацию по юнитам CUnitsIter< 0, 0 >, для каждого юнита будет вызвана функция iterFunc,
	//! итерирование продолжается до тех пор пока есть юниты и пока iterFunc возвращает true. Возвращаемое
	//! значение равно последнему значение iterFunc, сам юнит (this) никогда не пройдет через итератор
	virtual const bool IterateUnits( const CVec2 &vCenter, const float fRadius,	const bool bOnlyMech, const SIterateUnitsCallback &callback ) const = 0;

	virtual void TurnToDirectionContinuesly( const WORD _wDirection );

	//! послать юнит вдоль пути, возвращает true, если юнит поехал
	virtual const bool SendAlongPath( IPath *pPath );
	//! послать юнит вдоль пути, возвращает true, если юнит поехал
	virtual const bool SendAlongPath( IStaticPath *pStaticPath, const CVec2 &vShift, const bool bSmoothTurn );
	//! послать юнит вдоль пути, возвращает true, если юнит поехал
	virtual void SendAlongSmoothPath( ISmoothPath *pPath );

	virtual IStaticPath *CreateBigStaticPath( const CVec2 &vStartPoint, const CVec2 &vFinishPoint, IPointChecking *pChecking );
	virtual const NTimer::STime GetCollStayTime() { return collStayTime; }
	virtual void ResetCollStayTime() { collStayTime = 0; }

	virtual void NotifyAboutClosestThreat( const CBasePathUnit *pUnit, const float fDistance );
	virtual const bool IsStaticUnit() const = 0;
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! CBasePathUnit не является потомком CObjectBase, поэтому для его сериализации нельзя использовать operator&, необходимо
//! пользоваться SerializeBasePathUnit( ... )
void SerializeBasePathUnit( IBinSaver &saver, const int nChunkID, CBasePathUnit **pUnit );
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
