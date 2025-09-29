#pragma once
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "AIClasses.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
interface IPointChecking;
class CAIMap;
class CTerrain;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CCommonPathFinder : public CAIObjectBase
{
	OBJECT_BASIC_METHODS( CCommonPathFinder );
public:
	enum { tidTypeID = 0x3008B3C0 };
	int operator&( IBinSaver &saver );
private:
	CPtr<IPointChecking> pChecking;

	int nBoundTileRadius;
	EAIClasses aiClass;

	int upperLimit;
	bool longPath;
	SVector startPoint, finishPoint;
	SVector lastKnownGoodTile;

	CVec2 vStartPoint, vFinishPoint;

	int nLength, nStart;

	int minDistance, minPointNum;
	// нашли точку
	bool bFinished;
	int nCyclePoints;

	vector<SVector> stopPoints, addPoints;
	int mapBufIndex;
	CArray2D<BYTE> mapBuf;
	vector<int> cyclePoints, segmBegin;
	bool bPathFound;

	int nBestDist;
	SVector vBestPoint;
	CPtr<CAIMap> pAIMap;
	CPtr<CTerrain> pTerrain;

	//
	void LineSmoothing( const int STEP_LENGTH_THERE, const int MAX_NUM_OF_ATTEMPTS_THERE,
		const int STEP_LENGTH_BACK, const int MAX_NUM_OF_ATTEMPTS_BACK );

	const SVector CalculateHandPath( const SVector &blockPoint, const SVector &dir, const SVector &finish );
	const SVector CalculateSimplePath( const SVector &blockPoint, const SVector &dir, const SVector &finish );
	bool CanGoTowardPoint( const SVector &start, const SVector &finish );

	bool Walkable( const SVector &start, const SVector &finish );
	const int SavePathThere( const SVector &start, const SVector &finish, const int nLen );
	const int SavePathBack( const SVector& start, const SVector& finish, const int nLen );

	bool CheckFakePath( const SVector point );
	const SVector LookForFakePathBegin();

	void EraseCycles();
	void AnalyzePoint( const SVector &point, const int num );
	// если юнит сейчас на непроходимом тайле, то можно длина пути от pointFrom то lastKnownGoodTile, а потом искать путь
	const int GetAdditionalPathLength( const SVector &pointFrom );

	bool CalculatePath();
	void SmoothPath();

	const SVector GetStopTile( int n ) const { NI_ASSERT( n >= 0 && n < nLength, "Wrong number of stop point" ); return addPoints[n]; }

	void CheckBestPoint( const SVector &point )
	{
		const int nDist = mDistance( point, finishPoint );
		if ( nDist < nBestDist )
		{
			vBestPoint = point;
			nBestDist = nDist;
		}
	}

	const enum EFreeTileInfo CanUnitGoByDir( const int nBoundTileRadius, const EAIClasses aiClass, const SVector &tile, const SVector &dir );
	void NextMapBufIndex();

	//DEBUG{
	int nRedMarker, nGreenMarker, nBlueMarker;
	//DEBUG}
public:
	CCommonPathFinder();
	void Init();

	void SetPathParameters( 
		const int nBoundTileRadius,
		const EAIClasses aiClass,
		const CVec2 &vStartPoint, const CVec2 &vFinishPoint,
		const SVector &lastKnownGoodTile, CAIMap *pAIMap );

	void SetLimitedPathParameters( 
		const int nBoundTileRadius,
		const EAIClasses aiClass,
		const CVec2 &vStartPoint, const CVec2 &vFinishPoint,
		const SVector &lastKnownGoodTile, const int nUpperLimit, CAIMap *pAIMap );

	void SetCheckingPathParameters(
		const int nBoundTileRadius,
		const EAIClasses aiClass,
		const CVec2 &vStartPoint, const CVec2 &vFinishPoint,
		const SVector &lastKnownGoodTile, interface IPointChecking *pChecking, CAIMap *pAIMap );

	bool DoesPathExist();
	// Specially to not realculate path twice. Calculate path after call DoesPathExist
	interface IStaticPath* CreatePathAfterExistCheck();
	interface IStaticPath* CreatePath( const bool bCreateNullPath );

	const CVec2& GetStartPoint() const { return vStartPoint; }
	const SVector& GetStartTile() const {  return startPoint; }
	const CVec2& GetFinishPoint();
	const SVector& GetFinishTile() const { return finishPoint; }

	const int GetPathLength()	const { return nLength; }
	void GetTiles( void *pBuf, const int nLen ) const
	{ 
		NI_ASSERT( nLength > 0 && nLen > 0 && nLen <= nLength, "Wrong number of stop points" ); 
		memcpy( pBuf, &(addPoints[0]), nLen * sizeof(SVector) );
	}
	void GetStopPoints( void *pBuf, const int nLen ) const
	{ 
		NI_ASSERT( nLength > 0 && nLen > 0 && nLen <= nLength, "Wrong number of stop points" ); 
		memcpy( pBuf, &(stopPoints[0]), nLen * sizeof(SVector) );
	}

	void MarkPathTiles();
	void MarkPoints();
	const bool IsPathFound() const { return bPathFound; }
};
