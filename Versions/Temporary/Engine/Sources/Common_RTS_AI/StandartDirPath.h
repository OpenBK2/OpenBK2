#pragma once

#include "Common_RTS_AI_export.h"


#include "BasePathUnit.h"

class COMMON_RTS_AI_EXPORT CStandartDirPath : public IPath
{
	OBJECT_NOCOPY_METHODS( CStandartDirPath );

	ZDATA
		CVec2 dir;
		CVec2 startPoint, finishPoint;
		CVec2 curPoint;
		int nTileSize;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&dir); f.Add(3,&startPoint); f.Add(4,&finishPoint); f.Add(5,&curPoint); f.Add(6,&nTileSize); return 0; }
private:
public:
	CStandartDirPath() : dir( VNULL2 ), startPoint( VNULL2 ), finishPoint( VNULL2 ), curPoint( VNULL2 ), nTileSize( -1 ) { }
	CStandartDirPath( const CVec2 &startPoint, const CVec2 &dir, const CVec2 &finishPoint, const int nTileSize );
	CStandartDirPath( const CVec2 &startPoint, const CVec2 &finishPoint, const int nTileSize );

	virtual bool IsFinished() const;

	virtual const CVec2& GetStartPoint() const { return startPoint; }
	virtual const CVec2& GetFinishPoint() const { return finishPoint; };

	virtual const CVec2 PeekPoint( const int nShift ) const;
	virtual void Shift( const int nShift );

	virtual void RecalcPath( const CVec2 &vPoint, const bool bIsPointAtWater, const SVector &lastKnownGoodTile );
	virtual void RecoverPath( const CVec2 &vPoint, const bool bIsPointAtWater, const SVector &lastKnownGoodTile );

	virtual void InsertTiles( const list<SVector> &tiles ) {};

	virtual const bool CanGoBackward( const CBasePathUnit *pUnit ) const { return pUnit->CanGoBackward(); }
	virtual const bool ShouldCheckTurn() const { return false; }
	virtual const bool CanBuildComplexTurn() const { return false; }
	virtual void MarkPath( const int nID, const NDebugInfo::EColor color ) const;
};


