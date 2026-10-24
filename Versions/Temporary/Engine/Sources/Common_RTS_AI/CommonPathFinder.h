#pragma once

// Defensive member defaults; explicit constructor values still take precedence.

#include "AIClasses.h"

#include "Common_RTS_AI_export.h"

#include <cstdint>

enum EFreeTileInfo : int;
struct IPointChecking;
class CAIMap;
class CTerrain;

class COMMON_RTS_AI_EXPORT CCommonPathFinder : public CAIObjectBase
{
	OBJECT_BASIC_METHODS( CCommonPathFinder );

public:
	enum
	{
		tidTypeID = 0x3008B3C0
	};

	int operator&( IBinSaver &saver ) override;

private:
	CPtr<IPointChecking> pChecking;

	int nBoundTileRadius = 0;
	EAIClasses aiClass{};

	int upperLimit = 0;
	bool longPath = false;
	SVector startPoint, finishPoint;
	SVector lastKnownGoodTile;

	CVec2 vStartPoint, vFinishPoint;

	int nLength = 0, nStart = 0;

	int minDistance = 0, minPointNum = 0;
	// нашли точку
	bool bFinished = false;
	int nCyclePoints = 0;

	std::vector<SVector> stopPoints, addPoints;
	int mapBufIndex = 0;
	CArray2D<uint8_t> mapBuf;
	std::vector<int> cyclePoints, segmBegin;
	bool bPathFound = false;

	int nBestDist = 0;
	SVector vBestPoint;
	CPtr<CAIMap> pAIMap;
	CPtr<CTerrain> pTerrain;

	//
	void LineSmoothing( const int STEP_LENGTH_THERE,
	                    const int MAX_NUM_OF_ATTEMPTS_THERE,
	                    const int STEP_LENGTH_BACK,
	                    const int MAX_NUM_OF_ATTEMPTS_BACK );

	const SVector CalculateHandPath( const SVector &blockPoint, const SVector &dir, const SVector &finish );
	const SVector CalculateSimplePath( const SVector &blockPoint, const SVector &dir, const SVector &finish );
	bool CanGoTowardPoint( const SVector &start, const SVector &finish );

	bool Walkable( const SVector &start, const SVector &finish );
	const int SavePathThere( const SVector &start, const SVector &finish, const int nLen );
	const int SavePathBack( const SVector &start, const SVector &finish, const int nLen );

	bool CheckFakePath( const SVector point );
	const SVector LookForFakePathBegin();

	void EraseCycles();
	void AnalyzePoint( const SVector &point, const int num );
	// если юнит сейчас на непроходимом тайле, то можно длина пути от pointFrom то lastKnownGoodTile, а потом искать путь
	const int GetAdditionalPathLength( const SVector &pointFrom );

	bool CalculatePath();
	void SmoothPath();

	const SVector GetStopTile( int n ) const
	{
		// The bound that has to hold is against addPoints, which is what this indexes, and not
		// against nLength, which is the path length. The two are different quantities: Init() sizes
		// both point vectors to LONG_PATH_LENGTH + 1 and SetLimitedPathParameters() caps upperLimit,
		// and so nLength, at LONG_PATH_LENGTH, which is why indexing by a path position is in range
		// at all. The single caller derives n from GetPathLength() after rejecting a length of zero.
		// Cast so that the condition still compiles clean if _DO_ASSERT_SLOW ever turns it back on:
		// an argument this macro drops is never type-checked either.
		NI_ASSERT( n >= 0 && n < static_cast<int>( addPoints.size() ), "Wrong number of stop point" );
		return addPoints[n];
	}

	void CheckBestPoint( const SVector &point )
	{
		const int nDist = mDistance( point, finishPoint );
		if ( nDist < nBestDist )
		{
			vBestPoint = point;
			nBestDist = nDist;
		}
	}

	const enum EFreeTileInfo
	CanUnitGoByDir( const int nBoundTileRadius, const EAIClasses aiClass, const SVector &tile, const SVector &dir );
	void NextMapBufIndex();

	//DEBUG{
	int nRedMarker = 0, nGreenMarker = 0, nBlueMarker = 0;
	//DEBUG}
public:
	CCommonPathFinder();
	void Init();

	void SetPathParameters( const int nBoundTileRadius,
	                        const EAIClasses aiClass,
	                        const CVec2 &vStartPoint,
	                        const CVec2 &vFinishPoint,
	                        const SVector &lastKnownGoodTile,
	                        CAIMap *pAIMap );

	void SetLimitedPathParameters( const int nBoundTileRadius,
	                               const EAIClasses aiClass,
	                               const CVec2 &vStartPoint,
	                               const CVec2 &vFinishPoint,
	                               const SVector &lastKnownGoodTile,
	                               const int nUpperLimit,
	                               CAIMap *pAIMap );

	void SetCheckingPathParameters( const int nBoundTileRadius,
	                                const EAIClasses aiClass,
	                                const CVec2 &vStartPoint,
	                                const CVec2 &vFinishPoint,
	                                const SVector &lastKnownGoodTile,
	                                struct IPointChecking *pChecking,
	                                CAIMap *pAIMap );

	bool DoesPathExist();
	// Specially to not realculate path twice. Calculate path after call DoesPathExist
	struct IStaticPath *CreatePathAfterExistCheck();
	struct IStaticPath *CreatePath( const bool bCreateNullPath );

	const CVec2 &GetStartPoint() const
	{
		return vStartPoint;
	}

	const SVector &GetStartTile() const
	{
		return startPoint;
	}

	const CVec2 &GetFinishPoint();

	const SVector &GetFinishTile() const
	{
		return finishPoint;
	}

	const int GetPathLength() const
	{
		return nLength;
	}

	// Both of these copied nLen points out under an assertion that bounded nLen against nLength, the
	// path length, while the copy reads addPoints or stopPoints, which are separate vectors. That is
	// a bound on a different quantity from the one that matters, and NI_ASSERT does not evaluate its
	// argument in this configuration, so neither bound was ever applied. The source side is in range
	// today because Init() sizes both vectors to LONG_PATH_LENGTH + 1 while nLength cannot pass
	// LONG_PATH_LENGTH, not because of anything either function did; the clamps below state that
	// invariant against the vector each one actually reads, and are unreachable while it holds.
	//
	// pBuf carries no size, so the destination stays the caller's responsibility either way.
	void GetTiles( void *pBuf, const int nLen ) const
	{
		const int nCopy = (std::min)( nLen, static_cast<int>( addPoints.size() ) );
		if ( nCopy <= 0 )
		{
			return;
		}
		memcpy( pBuf, &( addPoints[0] ), nCopy * sizeof( SVector ) );
	}

	void GetStopPoints( void *pBuf, const int nLen ) const
	{
		const int nCopy = (std::min)( nLen, static_cast<int>( stopPoints.size() ) );
		if ( nCopy <= 0 )
		{
			return;
		}
		memcpy( pBuf, &( stopPoints[0] ), nCopy * sizeof( SVector ) );
	}

	void MarkPathTiles();
	void MarkPoints();

	const bool IsPathFound() const
	{
		return bPathFound;
	}
};
