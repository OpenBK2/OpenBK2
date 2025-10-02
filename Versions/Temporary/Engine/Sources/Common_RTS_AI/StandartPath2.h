#pragma once

#include "Common_RTS_AI_export.h"

#include "AIClasses.h"
#include "Path.h"

class CCommonPathFinder;
class CCommonStaticPath;
class CBasePathUnit;
struct IStaticPath;

class COMMON_RTS_AI_EXPORT CStandartPath2 : public IPath
{
	OBJECT_NOCOPY_METHODS( CStandartPath2 )
	ZDATA
		CPtr<CCommonPathFinder> pPathFinder;
		CPtr<CAIMap> pAIMap;
		int nBoundTileRadius;
		EAIClasses aiClass;

		CPtr<CCommonStaticPath> pStaticPath;
		int nCurStaticPathTile;
		
		CVec2 vStartPoint;
		CVec2 vFinishPoint;
		SVector vFinishTile;
		CVec2 vShift;
		SVector vShiftTile;

		vector<SVector> insTiles;
		int nCurInsTile;

		vector<SVector> pathTiles;
		int nCurPathTile;
		int nLastPathTile;

		int nUnitID;

		CPtr<IStaticPath> pPathHistory1; //for debug purpose only
		CPtr<IStaticPath> pPathHistory2; //for debug purpose only
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pPathFinder); f.Add(3,&pAIMap); f.Add(4,&nBoundTileRadius); f.Add(5,&aiClass); f.Add(6,&pStaticPath); f.Add(7,&nCurStaticPathTile); f.Add(8,&vStartPoint); f.Add(9,&vFinishPoint); f.Add(10,&vFinishTile); f.Add(11,&vShift); f.Add(12,&vShiftTile); f.Add(13,&insTiles); f.Add(14,&nCurInsTile); f.Add(15,&pathTiles); f.Add(16,&nCurPathTile); f.Add(17,&nLastPathTile); f.Add(18,&nUnitID); f.Add(19,&pPathHistory1); f.Add(20,&pPathHistory2); return 0; }
private:
	const bool CanUnitGo( const SVector &vTile ) const;
	const SVector GetTileWithShift( const SVector &vTile ) const;
	const SVector GetTile( const CVec2 &vPoint ) const;
	const CVec2 GetPoint( const SVector &vTile ) const;

	void CopyPath( const int nLength );
	const bool CalculatePath( const bool bShift, const SVector &vLastKnownGoodTile );

	void SetFinishPoint( const CVec2 &_vFinishPoint ) { vFinishPoint = _vFinishPoint; vFinishTile = GetTile( vFinishPoint ); } 
	void SetFinishTile( const SVector &_vFinishTile ) { vFinishTile = _vFinishTile; vFinishPoint = GetPoint( vFinishTile ); } 

	void InitByStaticPath( CCommonStaticPath *pStaticPath, const CVec2 &vStartPoint, const CVec2 &vFinishPoint, const bool bResetShift );
public:
	CStandartPath2();
	CStandartPath2( const CBasePathUnit *pUnit, IStaticPath *pStaticPath, const CVec2 &vStartPoint, const CVec2 &vFinishPoint, CAIMap *pAIMap );

	const CVec2 PeekPoint( const int nShift ) const;
	void Shift( const int nShift );
	bool IsFinished() const { return nCurPathTile == nLastPathTile; }

	const CVec2& GetFinishPoint() const { return vFinishPoint; }
	const CVec2& GetStartPoint() const { return vStartPoint; }

	//! восстановить (пересчитать новый статический путь) путь из новой точки ( vPoint )
	void RecoverPath( const CVec2 &vPoint, const bool bIsPointAtWater, const SVector &vLastKnownGoodTile );
	//! пересчитать путь из новой точки ( vPoint )
	void RecalcPath( const CVec2 &vPoint, const bool bIsPointAtWater, const SVector &vLastKnownGoodTile );
	//! добавить тайлы в начало пути, для нормального продолжения после InsertTiles необходимо вызвать RecoverPath
	void InsertTiles( const list<SVector> &tiles );
	//! можно ли проехать весь путь задом
	const bool CanGoBackward( const CBasePathUnit *pUnit ) const;
  //! необходимо проверить можно ли развернуться для того, что бы ехать по данному пути
	const bool ShouldCheckTurn() const { return true; }
	//! можно ли для этого пути построить сложный разворот (все равно разворотов по окружности ПОКА нет)
	const bool CanBuildComplexTurn() const { return false; }
	//! отметить путь, и, может быть, вывести различную информацию
	void MarkPath( const int nID, const NDebugInfo::EColor color ) const;
};
