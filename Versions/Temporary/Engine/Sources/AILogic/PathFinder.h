#pragma once

class CBasePathUnit;

interface IStaticPathFinder : public CAIObjectBase
{
	virtual void SetPathParameters( const int nBoundTileRadius, const BYTE aiClass, interface IPointChecking *pChecking, const CVec2 &startPoint, const CVec2 &finishPoint, const int upperLimit, const bool longPath, const SVector &lastKnownGoodTile ) = 0;

	// поиск пути без каких-либо улучшений
	virtual bool CalculatePath() = 0;	
	// поиск пути в точку без циклов
	virtual void CalculatePathWOCycles() = 0;
	virtual void SmoothPath() = 0;
	
	virtual const int GetPathLength()	const	= 0;
	virtual const SVector GetStopTile( int n ) const = 0;
	virtual const void GetStopTiles( void *buf, int len ) const = 0;
	
	virtual const SVector GetStartTile() const = 0;
	virtual const SVector GetFinishTile() const = 0;
};

interface IStaticPath* CreateStaticPathForAttack( CBasePathUnit *pUnit, class CAIUnit *pTarget, const float fRangeMin, const float fRangeMax, const float fRandomCant, const bool bIgnoreObstacles );
interface IStaticPath* CreateStaticPathForStObjAttack( CBasePathUnit *pUnit, class CStaticObject *pObj, const float fRangeMin, const float fRangeMax, const bool bIgnoreObstacles );
interface IStaticPath* CreateStaticPathForSideAttack( CBasePathUnit *pUnit, class CAIUnit *pTarget, const CVec2 &attackDir, const float fRangeMin, const float fRangeMax, const float fDistToPoint, const WORD wHalfAngle, const bool bIgnoreObstacles );
interface IStaticPath* CreatePathWithChecking( CBasePathUnit *pUnit, const SVector &vTargetTile, IPointChecking *pPointChecking );
bool CanUnitApproachToUnitByPath( const class CAIUnit *pMoving, const interface IStaticPath *pPath, const class CAIUnit *pStanding);
bool CanUnitApproachToPointByPath( const class CAIUnit *pMoving, const IStaticPath *pPath, const class CVec2 & point );
bool CanUnitApproachToObjectByPath( const class CAIUnit *pMoving, const IStaticPath *pPath, const class CStaticObject *pPoint );
bool IsUnitNearObject( const class CAIUnit * pUnit, const class CStaticObject * pObj );
bool IsUnitNearUnit( const class CAIUnit * pUnit1, const class CAIUnit * pUnit2 );
bool IsUnitNearPoint( const class CAIUnit * pUnit1, const class CVec2 & point, const int add = 0 );
bool IsPointNearPoint( const class CVec2 & point1, const class CVec2 & point2 );

