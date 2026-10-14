#pragma once

// Defensive member defaults; explicit constructor values still take precedence.

#include "BasePathUnit.h"

#include "Common_RTS_AI_export.h"

class COMMON_RTS_AI_EXPORT CStandartDirPath : public IPath
{
	OBJECT_NOCOPY_METHODS( CStandartDirPath );

	ZDATA
	CVec2 dir;
	CVec2 startPoint, finishPoint;
	CVec2 curPoint;
	int nTileSize = 0;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &dir );
		f.Add( 3, &startPoint );
		f.Add( 4, &finishPoint );
		f.Add( 5, &curPoint );
		f.Add( 6, &nTileSize );
		return 0;
	}

private:
public:
	CStandartDirPath()
		: dir( VNULL2 )
		, startPoint( VNULL2 )
		, finishPoint( VNULL2 )
		, curPoint( VNULL2 )
		, nTileSize( -1 )
	{
	}

	CStandartDirPath( const CVec2 &startPoint, const CVec2 &dir, const CVec2 &finishPoint, const int nTileSize );
	CStandartDirPath( const CVec2 &startPoint, const CVec2 &finishPoint, const int nTileSize );

	bool IsFinished() const override;

	const CVec2 &GetStartPoint() const override
	{
		return startPoint;
	}

	const CVec2 &GetFinishPoint() const override
	{
		return finishPoint;
	}

	const CVec2 PeekPoint( const int nShift ) const override;
	void Shift( const int nShift ) override;

	void RecalcPath( const CVec2 &vPoint, const bool bIsPointAtWater, const SVector &lastKnownGoodTile ) override;
	void RecoverPath( const CVec2 &vPoint, const bool bIsPointAtWater, const SVector &lastKnownGoodTile ) override;

	void InsertTiles( const std::list<SVector> &tiles ) override {}

	const bool CanGoBackward( const CBasePathUnit *pUnit ) const override
	{
		return pUnit->CanGoBackward();
	}

	const bool ShouldCheckTurn() const override
	{
		return false;
	}

	const bool CanBuildComplexTurn() const override
	{
		return false;
	}

	void MarkPath( const int nID, const NDebugInfo::EColor color ) const override;
};
