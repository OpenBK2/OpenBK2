#pragma once

#include "Common_RTS_AI/AIClasses.h"
#include "DebugTools/DebugInfoManager.h"

class CBasePathUnit;
class CAIMap;

//! вот бы избавиться от этого...
struct IMemento : public CAIObjectBase
{
};

//! путь юнита
struct IPath : public CAIObjectBase
{
	virtual bool IsFinished() const = 0;

	virtual const CVec2 PeekPoint( const int nShift ) const = 0;
	virtual void Shift( const int nShift ) = 0;

	virtual const CVec2& GetFinishPoint() const = 0;
	virtual const CVec2& GetStartPoint() const = 0;

	//! восстановить путь из новой точки ( vPoint )
	virtual void RecoverPath( const CVec2 &vPoint, const bool bIsPointAtWater, const SVector &vLastKnownGoodTile ) = 0;
	//! пересчитать путь из новой точки ( vPoint )
	virtual void RecalcPath( const CVec2 &vPoint, const bool bIsPointAtWater, const SVector &vLastKnownGoodTile ) = 0;
	//! добавить тайлы в начало пути
	virtual void InsertTiles( const std::list<SVector> &tiles ) = 0;
	//! можно ли проехать весь путь задом
	virtual const bool CanGoBackward( const CBasePathUnit *pUnit ) const = 0;
	virtual const bool ShouldCheckTurn() const = 0;
	//! можно ли для этого пути построить сложный разворот
	virtual const bool CanBuildComplexTurn() const = 0;

	virtual void MarkPath( const int nID, const NDebugInfo::EColor color ) const = 0;
};

//! сглаженный путь юнита, именно вдоль этого пути и происходит движение юнита
struct ISmoothPath : public CAIObjectBase
{
	// возвращает - пошёл юнит по пути или нет
	virtual bool Init( CBasePathUnit *pUnit, IPath *pPath, bool bSmoothTurn, bool bCheckTurn, CAIMap *pAIMap ) = 0;
	virtual bool Init( IMemento *pMemento, CBasePathUnit *pUnit, CAIMap *pAIMap ) = 0;

	virtual bool IsFinished() const = 0;

	virtual void Stop() = 0;

	virtual void Segment( const NTimer::STime timeDiff ) = 0;

	virtual const bool CanGoBackward() const = 0;
	virtual const bool CanGoForward() const = 0;

	virtual const CVec2 PeekPathPoint( const int nShift ) const = 0;
	virtual IMemento* CreateMemento() const = 0;

	virtual const float GetSpeed() const { return -1.0f; }

	virtual bool InitByFormationPath( class CFormation *pFormation, CBasePathUnit *pUnit ) { return true; }
};


