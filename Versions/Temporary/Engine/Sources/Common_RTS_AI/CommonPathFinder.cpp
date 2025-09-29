#include "stdafx.h"

#include "CommonPathFinder.h"
#include "ImportedStruct.h"
#include "PointChecking.h"
#include "StaticPathInternal.h"
#include "../Common_RTS_AI/Terrain.h"
#include "../Common_RTS_AI/AIMap.h"
#include "../Misc/Bresenham.h"
#include "../System/Commands.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const int LONG_PATH_LENGTH = 2000;
static const int STEP_LENGTH_THERE = 30;
static const int MAX_NUM_OF_ATTEMPTS_THERE = 4;
static const int STEP_LENGTH_BACK = 10;
static const int MAX_NUM_OF_ATTEMPTS_BACK = 12;
static const int STEP_LENGTH_THERE_SHORT = 5;
static const int MAX_NUM_OF_ATTEMPTS_THERE_SHORT = 4;
static const int STEP_LENGTH_BACK_SHORT = 2;
static const int MAX_NUM_OF_ATTEMPTS_BACK_SHORT = 12;
static const int TOLERANCE = 64;
static const int TOLERANCE_SHORT = 16;
static const int MAX_MAPBUFINDEX = 254;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static float fWarFogBoundWidth = 256.0f;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
REGISTER_SAVELOAD_CLASS( 0x3008B3C0, CCommonPathFinder );
BASIC_REGISTER_CLASS( CCommonPathFinder );
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CCommonPathFinder::Init()
{
	bPathFound = false;
	nBoundTileRadius = -1;
	aiClass = EAC_NONE; 
	upperLimit = -1;
	longPath = false;
	lastKnownGoodTile = SVector(-1,-1);
	finishPoint = SVector(-1,-1);
	startPoint = SVector(-1,-1);
	vStartPoint =  vFinishPoint = VNULL2;
	nLength = nStart = -1;

	minDistance = minPointNum = 1;
	bFinished = false;
	nCyclePoints = 0;

	//mapBufIndex = -1;
	bPathFound = false;
	nBestDist = -1;
	vBestPoint = SVector(-1,-1);

	mapBufIndex = 1;

	cyclePoints.resize( 2*LONG_PATH_LENGTH + 1, 0 );

	stopPoints.resize( LONG_PATH_LENGTH + 1 );
	addPoints.resize( LONG_PATH_LENGTH + 1 );
	segmBegin.resize( LONG_PATH_LENGTH + 1 );
	mapBuf.Clear();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CCommonPathFinder::CCommonPathFinder()
{
	Init();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CCommonPathFinder::SetPathParameters( const int nBoundTileRadius, const EAIClasses aiClass,
	const CVec2 &vStartPoint, const CVec2 &vFinishPoint, const SVector &lastKnownGoodTile, CAIMap *pAIMap )
{
	SetLimitedPathParameters( nBoundTileRadius, aiClass, vStartPoint, vFinishPoint, lastKnownGoodTile, LONG_PATH_LENGTH, pAIMap );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CCommonPathFinder::SetLimitedPathParameters( 
	const int _nBoundTileRadius,
	const EAIClasses _aiClass,
	const CVec2 &_vStartPoint, const CVec2 &_vFinishPoint,
	const SVector &_lastKnownGoodTile, const int _nUpperLimit, CAIMap *_pAIMap )
{
	pAIMap = _pAIMap;
	pTerrain = pAIMap->GetTerrain();

	const int nMaxSize = Max( pAIMap->GetSizeX(), pAIMap->GetSizeY() );
	if ( nMaxSize > mapBuf.GetSizeX() )
	{
		mapBuf.SetSizes( nMaxSize, nMaxSize );
		mapBuf.FillZero();
		mapBufIndex = 1;
	}

	vStartPoint = _vStartPoint;
	vFinishPoint = CVec2( Clamp( _vFinishPoint.x, fWarFogBoundWidth, pAIMap->GetSizeX() * pAIMap->GetTileSize() - fWarFogBoundWidth ),
		Clamp( _vFinishPoint.y, fWarFogBoundWidth, pAIMap->GetSizeY() * pAIMap->GetTileSize() - fWarFogBoundWidth ) );
	//vFinishPoint = _vFinishPoint;

	bFinished = false;
	startPoint = pAIMap->GetTile( vStartPoint.x, vStartPoint.y );
	startPoint.x = Clamp( startPoint.x, 0, pAIMap->GetSizeX() - 1 );
	startPoint.y = Clamp( startPoint.y, 0, pAIMap->GetSizeY() - 1 );
	finishPoint = pAIMap->GetTile( vFinishPoint.x, vFinishPoint.y );
	upperLimit = Min( _nUpperLimit, LONG_PATH_LENGTH );

	longPath = ( upperLimit == LONG_PATH_LENGTH );
	lastKnownGoodTile = _lastKnownGoodTile;

	nBoundTileRadius = _nBoundTileRadius;
	aiClass = _aiClass;
	bPathFound = false;
	pChecking = 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CCommonPathFinder::SetCheckingPathParameters(
	const int nBoundTileRadius,
	const EAIClasses aiClass,
	const CVec2 &vStartPoint, const CVec2 &vFinishPoint,
	const SVector &lastKnownGoodTile, interface IPointChecking *_pChecking, CAIMap *pAIMap )
{
	SetPathParameters( nBoundTileRadius, aiClass, vStartPoint, vFinishPoint, lastKnownGoodTile, pAIMap );
	pChecking = _pChecking;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const EFreeTileInfo CCommonPathFinder::CanUnitGoByDir( const int nBoundTileRadius, const EAIClasses aiClass, const SVector &tile, const SVector &dir )
{
	const SVector tileToGo( tile + dir );
	return pTerrain->CanUnitGo( nBoundTileRadius, tileToGo, aiClass );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CCommonPathFinder::AnalyzePoint( const SVector &point, const int num )
{
	const int mDist = mDistance( point, finishPoint );
	if ( mDist < minDistance )
	{
		CheckBestPoint( point );
		minDistance = mDist;
		minPointNum = num;
		if ( !bFinished && pChecking && pChecking->IsGoodTile( point ) )
			bFinished = true;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const SVector CCommonPathFinder::CalculateHandPath( const SVector &blockPoint, const SVector &dir, const SVector &finish )
{
	//DebugTrace( "CCommonPathFinder::CalculateHandPath( %d x %d, %d x %d, %d x %d )", blockPoint.x, blockPoint.y, dir.x, dir.y, finish.x, finish.y );
	const CLine blockLine( blockPoint, finish );
	const CLine perpLine( blockLine.GetPerpendicular( blockPoint ) );
	const CLine perpLine1( blockLine.GetPerpendicular( finish) );
	const int startLen = nLength;

	SVector dirLeft = dir, dirRight = dir;
	SVector curRightPoint = blockPoint, curLeftPoint = blockPoint;

	int cnt = 0;
	do
	{
		dirRight.TurnLeftUntil45();
		++cnt;
	} while ( cnt < 10 && CanUnitGoByDir( nBoundTileRadius, aiClass, curRightPoint, dirRight ) == FREE_NONE );

	if ( cnt >= 10 )
	{
		nLength = startLen;
		return SVector( -1, -1 );
	}

	cnt = 0;
	do
	{
		dirLeft.TurnRightUntil45();
		++cnt;
	} while ( cnt < 10 && CanUnitGoByDir( nBoundTileRadius, aiClass, curLeftPoint, dirLeft ) == FREE_NONE );

	if ( cnt >= 10 )
	{
		nLength = startLen;
		return SVector( -1, -1 );
	}

	// шагнуть вперёд, если на углу
	if ( CanUnitGoByDir( nBoundTileRadius, aiClass, curRightPoint, dirRight ) != FREE_NONE && 
		CanUnitGoByDir( nBoundTileRadius, aiClass, curLeftPoint, dirLeft ) != FREE_NONE )
	{
		stopPoints[nLength] = curRightPoint;
		curRightPoint += dirRight;

		addPoints[nLength++] = curLeftPoint;
		curLeftPoint += dirLeft;
	}

	while ( nCyclePoints < LONG_PATH_LENGTH )
	{
		// правая рука
		dirRight.x = -dirRight.x;
		dirRight.y = -dirRight.y;
		int cnt = 0;
		do 
		{
			++cnt;
			dirRight.TurnLeftUntil45();
		} while( cnt < 9 && CanUnitGoByDir( nBoundTileRadius, aiClass, curRightPoint, dirRight ) == FREE_NONE );
		if ( cnt == 9 )
		{
			nLength = startLen;
			return SVector( -1, -1 );
		}

		stopPoints[nLength] = curRightPoint;
		CheckBestPoint( curRightPoint );
		SVector nextPoint = curRightPoint + dirRight;

		if ( blockLine.IsSegmIntersectLine( curRightPoint, nextPoint ) )
		{
/*LOG{
	было:
		if ( ... * ... > 0 && ... * ... > 0 )
	почему:
		проход через необходимую точку не считался обходом
}LOG*/
			// обошли
			if ( perpLine.GetHPLineSign( nextPoint ) * perpLine.GetHPLineSign( finish ) >= 0  &&
				perpLine1.GetHPLineSign( nextPoint ) * perpLine1.GetHPLineSign( blockPoint ) >= 0 )
			{
				++nLength;				
				for ( int i = startLen; i < nLength; ++i )
				{
					// проверка на цикл
					if ( mapBuf[stopPoints[i].x][stopPoints[i].y] == mapBufIndex )
					{
//						if ( nCyclePoints >= LONG_PATH_LENGTH )
#ifndef _FINALRELEASE
							NI_VERIFY( nCyclePoints < cyclePoints.size(), StrFmt( "Index out of range, %d x %d - %d x %d", startPoint.x, startPoint.y, finishPoint.x, finishPoint.y ), cyclePoints.resize( cyclePoints.size() * 2 ) );
#endif							
							cyclePoints[nCyclePoints++] = i;
					}
					else
						mapBuf[stopPoints[i].x][stopPoints[i].y] = mapBufIndex;
					AnalyzePoint( stopPoints[i], i );
				}
				
				//DebugTrace( "return %d x %d at right-hand-path", nextPoint.x, nextPoint.y );
				return nextPoint;
			}
		}

		curRightPoint = nextPoint;

		// ----------------------------------------------------------------------------------------------

		// левая рука

		dirLeft.x = -dirLeft.x;
		dirLeft.y = -dirLeft.y;
		cnt = 0;
		do 
		{
			++cnt;
			dirLeft.TurnRightUntil45();
		} while( cnt < 9 && CanUnitGoByDir( nBoundTileRadius, aiClass, curLeftPoint, dirLeft ) == FREE_NONE );
		if ( cnt == 9 )
		{
			nLength = startLen;
			return SVector( -1, -1 );
		}

		addPoints[nLength] = curLeftPoint;
		CheckBestPoint( curLeftPoint );
		nextPoint = curLeftPoint + dirLeft;

		if ( blockLine.IsSegmIntersectLine( curLeftPoint, nextPoint ) )
		{
			// обошли
			if ( perpLine.GetHPLineSign( nextPoint ) * perpLine.GetHPLineSign( finish ) >= 0 &&
				perpLine1.GetHPLineSign( nextPoint ) * perpLine1.GetHPLineSign( blockPoint ) >= 0 )
			{
				++nLength;
				for ( int i = startLen; i < nLength; i++ )
				{
					// проверка на цикл
					if ( mapBuf[addPoints[i].x][addPoints[i].y] == mapBufIndex )
					{
//						if ( nCyclePoints >= LONG_PATH_LENGTH )
#ifndef _FINALRELEASE
							NI_VERIFY( nCyclePoints < cyclePoints.size(), "Index out of range", cyclePoints.resize( cyclePoints.size() * 2 ) );
#endif							
							cyclePoints[nCyclePoints++] = i;
					}
					else
						mapBuf[addPoints[i].x][addPoints[i].y] = mapBufIndex;
					AnalyzePoint( addPoints[i], i );
				}
				memcpy( &(stopPoints[0]) + startLen, &(addPoints[0]) + startLen, sizeof(SVector) * ( nLength - startLen ) );

				//DebugTrace( "return %d x %d at left-hand-path", nextPoint.x, nextPoint.y );
				return nextPoint;
			}
		}
		curLeftPoint = nextPoint;


		if ( nLength >= upperLimit )
		{
			nLength = startLen;
			return SVector( -1, -1 );
		}

		++nLength;
	}
	return SVector( -1, -1 );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CCommonPathFinder::CanGoTowardPoint( const SVector &start, const SVector &finish )
{
	if ( mDistance( start, finish ) <= 2*nBoundTileRadius+1 )
		return true;
	CBres bres;
	bres.Init( start, finish );
	bres.MakeStep();

//	return CanUnitGoByDir( nBoundTileRadius, aiClass, start, bres.GetDirection() ) != FREE_NONE;

	if ( CanUnitGoByDir( nBoundTileRadius, aiClass, start, bres.GetDirection() ) != FREE_NONE )
	{
		bres.MakeStep();
		return CanUnitGoByDir( nBoundTileRadius, aiClass, start, bres.GetDirection() ) != FREE_NONE;
	}
	return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const SVector CCommonPathFinder::CalculateSimplePath( const SVector &blockPoint, const SVector &dir, const SVector &finish )
{
	const int startLen = nLength; 

	SVector dirLeft = dir, dirRight = dir;
	SVector curRightPoint = blockPoint, curLeftPoint = blockPoint;

	int cnt = 0;
	do
	{
		dirRight.TurnLeftUntil45();
		++cnt;
	} while ( cnt < 10 && CanUnitGoByDir( nBoundTileRadius, aiClass, curRightPoint, dirRight ) == FREE_NONE);
	if ( cnt >= 10 )
	{
		nLength = startLen;
		return SVector( -1, -1 );
	}

	cnt = 0;
	do
	{
		++cnt;
		dirLeft.TurnRightUntil45();
	} while ( cnt <10 && CanUnitGoByDir( nBoundTileRadius, aiClass, curLeftPoint, dirLeft ) == FREE_NONE );
	if ( cnt >= 10 )
	{
		nLength = startLen;
		return SVector( -1, -1 );
	}

	// шагнуть вперёд, если на углу
	if ( CanUnitGoByDir( nBoundTileRadius, aiClass, curRightPoint, dirRight ) != FREE_NONE &&
		CanUnitGoByDir( nBoundTileRadius, aiClass, curLeftPoint, dirLeft ) != FREE_NONE )
	{
		stopPoints[nLength] = curRightPoint;
		curRightPoint += dirRight;

		addPoints[nLength++] = curLeftPoint;
		curLeftPoint += dirLeft;
	}

	while ( nLength < upperLimit )
	{
		// ----------------------- правая рука ---------------------------------

		SVector dirTemp = dirRight;
		dirRight.TurnRightUntil90();
		int cnt = 0;
		while ( cnt < 8 && CanUnitGoByDir( nBoundTileRadius, aiClass, curRightPoint, dirRight ) == FREE_NONE )
		{
			++cnt;
			dirRight.TurnLeftUntil45();
		}

		if ( cnt >= 8 )
		{
			nLength = startLen;
			return SVector( -1, -1 );
		}

		if ( cnt <= 3 )
		{
			const SVector dir1 = finish-curRightPoint;
			if ( ( dirTemp * dir1 ) >= 0 && ( dirRight * dir1 ) >= 0 && CanGoTowardPoint( curRightPoint, finish ) )
			{
				for ( int i = startLen; i < nLength; ++i )
				{
					// проверка на цикл
					if ( mapBuf[stopPoints[i].x][stopPoints[i].y] == mapBufIndex )
					{
#ifndef _FINALRELEASE
							NI_VERIFY( nCyclePoints < cyclePoints.size(), "Index out of range", cyclePoints.resize( cyclePoints.size() * 2 ) );
#endif							
						cyclePoints[nCyclePoints++] = i;
					}
					else
						mapBuf[stopPoints[i].x][stopPoints[i].y] = mapBufIndex;
					AnalyzePoint( stopPoints[i], i );
				}
				return curRightPoint;
			}
		}

		stopPoints[nLength] = curRightPoint;
		CheckBestPoint( curRightPoint );
		curRightPoint += dirRight;
		// --------------------- левая рука -----------------------
		dirTemp = dirLeft;
		dirLeft.TurnLeftUntil90();
		cnt = 0;
		while ( cnt < 8 && CanUnitGoByDir( nBoundTileRadius, aiClass, curLeftPoint, dirLeft ) == FREE_NONE )
		{
			++cnt;
			dirLeft.TurnRightUntil45();
		}

		if ( cnt >= 8 )
		{
			nLength = startLen;
			return SVector( -1, -1 );
		}

		if ( cnt <= 3 )
		{
			const SVector dir1 = finish-curLeftPoint;			
			if ( ( dirTemp * dir1 ) >= 0  &&  ( dirLeft * dir1 ) >= 0  && CanGoTowardPoint( curLeftPoint, finish ) )
			{
				for ( int i = startLen; i < nLength; i++ )
				{
					// проверка на цикл
					if ( mapBuf[addPoints[i].x][addPoints[i].y] == mapBufIndex )
					{
#ifndef _FINALRELEASE
							NI_VERIFY( nCyclePoints < cyclePoints.size(), "Index out of range", cyclePoints.resize( cyclePoints.size() * 2 ) );
#endif							
						cyclePoints[nCyclePoints++] = i;
					}
					else
						mapBuf[addPoints[i].x][addPoints[i].y] = mapBufIndex;
					AnalyzePoint( addPoints[i], i );
				}
				memcpy( &(stopPoints[0]) + startLen, &(addPoints[0]) + startLen, sizeof(SVector) * ( nLength - startLen ) );

				return curLeftPoint;
			}
		}

		addPoints[nLength] = curLeftPoint;
		CheckBestPoint( curLeftPoint );
		curLeftPoint += dirLeft;

		++nLength;		
	}

	nLength = startLen;
	return SVector( -1, -1 );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CCommonPathFinder::CheckFakePath( const SVector point )
{
	if ( pTerrain->CanUnitGo( nBoundTileRadius, point, aiClass ) != FREE_NONE )
	{
		CBres bres;
		bres.InitPoint( startPoint, point );

		do
		{
			stopPoints[nLength++] = bres.GetDirection();
			bres.MakePointStep();
		} while ( bres.GetDirection() != point );

		return true;
	}

	return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const int CCommonPathFinder::GetAdditionalPathLength( const SVector &pointFrom )
{
	if ( pointFrom == lastKnownGoodTile )
		return 0;
	else
	{
		const SVector oldStart = startPoint;
		const SVector oldFinish = finishPoint;
		IPointChecking *pOldPointChecking = pChecking;

		startPoint = pointFrom;
		finishPoint = lastKnownGoodTile;
		pChecking = 0;

		int nPathLength = -1;
		if ( CalculatePath() )
		{
			if ( GetPathLength() == 0 || GetStopTile( GetPathLength() - 1 ) != lastKnownGoodTile )
				nPathLength = -1;
			else
				nPathLength = GetPathLength();
		}

		startPoint = oldStart;
		finishPoint = oldFinish;
		pChecking = pOldPointChecking;

		return nPathLength;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const SVector CCommonPathFinder::LookForFakePathBegin()
{
	const int nMaxFails = 5;
	int nFails = 0;

	int nBestPathLength = -1;
	SVector bestPoint( 0, 0 );
	SVector firstPointToGo( -1, -1 );
	for ( int i = 0; i < 6 && nBestPathLength == -1 && nFails < nMaxFails; ++i )
	{
		for ( int j = startPoint.y - i; j <= startPoint.y + i && nFails < nMaxFails; ++j )
		{
			const SVector point( startPoint.x - i, j );
			if ( pTerrain->CanUnitGo( nBoundTileRadius, point, aiClass ) != FREE_NONE )
			{
				if ( firstPointToGo.x == -1 || mDistance( firstPointToGo, startPoint ) > mDistance( point, startPoint ) )
					firstPointToGo = point;

				const int nLocalPathLength = GetAdditionalPathLength( point );
				if ( nLocalPathLength != -1 && ( nBestPathLength == -1 || nLocalPathLength < nBestPathLength ) )
				{
					nBestPathLength = nLocalPathLength;
					bestPoint = point;
				}
				else if ( nLocalPathLength == -1 )
					++nFails;
			}

			const SVector point1( startPoint.x + i, j );
			if ( pTerrain->CanUnitGo( nBoundTileRadius, point1, aiClass ) != FREE_NONE )
			{
				if ( firstPointToGo.x == -1 || mDistance( firstPointToGo, startPoint ) > mDistance( point, startPoint ) )
					firstPointToGo = point;

				const int nLocalPathLength = GetAdditionalPathLength( point1 );
				if ( nLocalPathLength != -1 && ( nBestPathLength == -1 || nLocalPathLength < nBestPathLength ) )
				{
					nBestPathLength = nLocalPathLength;
					bestPoint = point1;
				}
				else if ( nLocalPathLength == -1 )
					++nFails;
			}
		}

		for ( int j = startPoint.x - i; j < startPoint.x + i && nFails < nMaxFails; ++j )
		{
			const SVector point( j, startPoint.y - i );
			if ( pTerrain->CanUnitGo( nBoundTileRadius, point, aiClass ) != FREE_NONE )
			{
				if ( firstPointToGo.x == -1 || mDistance( firstPointToGo, startPoint ) > mDistance( point, startPoint ) )
					firstPointToGo = point;

				const int nLocalPathLength = GetAdditionalPathLength( point );
				if ( nLocalPathLength != -1 && ( nBestPathLength == -1 || nLocalPathLength < nBestPathLength ) )
				{
					nBestPathLength = nLocalPathLength;
					bestPoint = point;
				}
				else if ( nLocalPathLength == -1 )
					++nFails;
			}

			const SVector point1( j, startPoint.y + i );
			if ( pTerrain->CanUnitGo( nBoundTileRadius, point1, aiClass ) != FREE_NONE )
			{
				if ( firstPointToGo.x == -1 || mDistance( firstPointToGo, startPoint ) > mDistance( point, startPoint ) )
					firstPointToGo = point;

				const int nLocalPathLength = GetAdditionalPathLength( point1 );
				if ( nLocalPathLength != -1 && ( nBestPathLength == -1 || nLocalPathLength < nBestPathLength ) )
				{
					nBestPathLength = nLocalPathLength;
					bestPoint = point1;
				}
				else if ( nLocalPathLength == -1 )
					++nFails;
			}
		}
	}

	nLength = 0;
	if ( nBestPathLength == -1 )
	{
		if ( firstPointToGo.x != -1 )
			return firstPointToGo;
		else
			return startPoint;

//		return startPoint;
	}
	else if ( CheckFakePath( bestPoint ) )
		return bestPoint;
	else
		return startPoint;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CCommonPathFinder::CalculatePath( )
{
	nBestDist = 0x7FFFFFFF;
	vBestPoint = SVector( -1, -1 );

	nLength = 0;
	nCyclePoints = 1;
	minDistance = mDistance( startPoint, finishPoint );
	minPointNum = 0;
	bFinished = false;

	NI_ASSERT( finishPoint.x >= 0 && finishPoint.y >= 0, "Wrong finish point" );

	if ( finishPoint.x < 0 || finishPoint.y < 0 )
	{
		finishPoint = startPoint;
		return false;
	}

	if ( startPoint == finishPoint )
	{
		nLength = 1;
		stopPoints[0] = startPoint;
		//DebugTrace( ">>>>>> Path trivial: startPoint == finishPoint" );
		return true;
	}

	// {CRAP: чтобы не застревали	
	SVector startSearchPoint;
	if ( pTerrain->CanUnitGo( nBoundTileRadius, startPoint, aiClass ) == FREE_NONE )
	{
		startSearchPoint = lastKnownGoodTile;
		startPoint = lastKnownGoodTile;
	}
	//		startSearchPoint = LookForFakePathBegin();
	else
		startSearchPoint = startPoint;
	//SVector startSearchPoint;
	//startSearchPoint = startPoint;
	// CRAP}


	// проверить, что можно хоть куда-нибудь идти
	bool bCanGo = false;
	for ( int i = -1; i <= 1 && !bCanGo; ++i )
	{
		for ( int j = -1; j <= 1  && !bCanGo; ++j )
		{
			if ( i != 0 || j != 0 )
				bCanGo = ( CanUnitGoByDir( nBoundTileRadius, aiClass, startSearchPoint, SVector( i, j ) ) != FREE_NONE );
		}
	}

	if ( !bCanGo )
	{
		finishPoint = startPoint;
		//DebugTrace( ">>>>>> Cannot find path: sitting on locked tile" );
		return false;
	}

	SVector curPoint(startSearchPoint);

	CBres bres;
	bres.Init( startSearchPoint, finishPoint );

	while ( curPoint != finishPoint && upperLimit >= 0 )
	{
		bres.MakeStep();

		// сходить
		if ( CanUnitGoByDir( nBoundTileRadius, aiClass, curPoint, bres.GetDirection() ) == FREE_NONE )
		{
			if ( curPoint + bres.GetDirection() == finishPoint )
			{
				CheckBestPoint( curPoint );
				finishPoint = curPoint;
				return true;
			}

			if ( mapBuf[curPoint.x][curPoint.y] != mapBufIndex )
			{
				SVector point( CalculateSimplePath( curPoint, bres.GetDirection(), finishPoint ) );
				if ( point.x == -1 )
				{
					point = CalculateHandPath( curPoint, bres.GetDirection(), finishPoint );

					if ( point.x == -1  || mDistance( point, curPoint ) >= 2*nBoundTileRadius+1 )
						curPoint = point;
					else
					{
						CheckBestPoint( curPoint );
						finishPoint = curPoint;
						return true;
					} 
				}
				else
					curPoint = point;
			}
			else 
			{

#ifndef _FINALRELEASE
				NI_VERIFY( nCyclePoints < cyclePoints.size(), "Index out of range", cyclePoints.resize( cyclePoints.size() * 2 ) );
#endif							
				cyclePoints[nCyclePoints++] = nLength;				
				SVector point( CalculateHandPath( curPoint, bres.GetDirection(), finishPoint ) );
				if ( point.x == -1 || mDistance( point, curPoint ) >= 2*nBoundTileRadius+1 )
					curPoint = point;
				else
				{
					CheckBestPoint( curPoint );
					finishPoint = curPoint;
					//DebugTrace( ">>>>>> Path found (1): finishPoint = %d x %d, nLength = %d", finishPoint.x, finishPoint.y, nLength );
					return true;
				}
			}

			if ( curPoint.x != -1 )
				bres.Init( curPoint, finishPoint );
		}
		else
		{
			mapBuf[curPoint.x][curPoint.y] = mapBufIndex;
			AnalyzePoint( curPoint, nLength );
			stopPoints[nLength++] = curPoint;
			curPoint += bres.GetDirection();
		}

		// дошли до точки, откуда можно производить нужные действия
		if ( bFinished )
		{
			finishPoint = curPoint;
			//DebugTrace( ">>>>>> Path found (2): finishPoint = %d x %d, nLength = %d", finishPoint.x, finishPoint.y, nLength );
			return true;
		}
		// путь не найден
		if ( curPoint.x == -1 )
		{
			finishPoint = stopPoints[nLength];
			//DebugTrace( ">>>>>> Path not found (1): finishPoint = %d x %d, nLength = %d", finishPoint.x, finishPoint.y, nLength );
			return false;
		}
		if ( nLength >= upperLimit )
		{
			finishPoint = stopPoints[upperLimit];
			//DebugTrace( ">>>>>> Path not found (2): finishPoint = %d x %d, nLength = %d", finishPoint.x, finishPoint.y, nLength );
			return false;
		}
	}

	//DebugTrace( ">>>>>> Path found (4): startPoint = %d x %d, lastKnownGoodTile = %d x %d, finishPoint = %d x %d, nLength = %d, upperLimit = %d, mapBufIndex = %d", startPoint.x, startPoint.y, lastKnownGoodTile.x, lastKnownGoodTile.y, finishPoint.x, finishPoint.y, nLength, upperLimit, mapBufIndex );
	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const CVec2& CCommonPathFinder::GetFinishPoint()
{
	if ( pAIMap->GetTile( vFinishPoint ) != finishPoint )
		vFinishPoint = pAIMap->GetPointByTile( finishPoint );

	return vFinishPoint;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CCommonPathFinder::EraseCycles()
{
	if ( minDistance > 1 || pTerrain->CanUnitGo( nBoundTileRadius, finishPoint, aiClass ) == FREE_NONE )
	{
		if ( nLength > minPointNum )
			nLength = minPointNum;

		finishPoint = stopPoints[nLength];
	}

	nStart = 0;

	int i = nLength - 1;
	int cycleNum = nCyclePoints - 1;

	// ищем конец ближайшего цикла
	while ( cycleNum > 0 && cyclePoints[cycleNum] > i - nStart )
		--cycleNum;

	while ( i - nStart >= 0  && cycleNum > 0 )
	{
		// сдвигаемся до конца циклаe
		while ( i - nStart >= cyclePoints[cycleNum] )
		{
			stopPoints[i] = stopPoints[i - nStart];
			// очистка буфера карты
			// mapBuf[stopPoints[i - nStart].x][stopPoints[i - nStart].y] = 0;
			--i;
		}

		// пропуск цикла
		while ( i - nStart >= 0 && stopPoints[i + 1] != stopPoints[i - nStart] )
		{
			// очистка буфера карты
			// mapBuf[stopPoints[i - nStart].x][stopPoints[i - nStart].y] = 0;
			++nStart;
		}
		++nStart;

		// ищем конец ближайшего цикла
		while ( cycleNum > 0 && cyclePoints[cycleNum] > i - nStart )
			--cycleNum;
	}
	//	--nStart;
	nLength -= nStart;

	while ( i - nStart  >= 0 )
	{
		stopPoints[i] = stopPoints[i - nStart];
		// mapBuf[stopPoints[i - nStart].x][stopPoints[i - nStart].y] = 0;
		--i;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CCommonPathFinder::Walkable( const SVector &start, const SVector &finish )
{
	CBres bres;
	bres.InitPoint( start, finish );

	while ( bres.GetDirection() != finish )
	{
		bres.MakePointStep();
		if ( pTerrain->CanUnitGo( nBoundTileRadius, bres.GetDirection(), aiClass ) == FREE_NONE )
			return false;
	}

	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const int CCommonPathFinder::SavePathThere( const SVector &start, const SVector &finish, const int nLen )
{
	CBres bres;
	bres.InitPoint( start, finish );

	int res = 0;
	do
	{
		addPoints[nLen+res++] = bres.GetDirection();		
		bres.MakePointStep();
	} while ( bres.GetDirection() != finish );

	return res;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const int CCommonPathFinder::SavePathBack( const SVector &start, const SVector &finish, const int nLen )
{
	CBres bres;
	bres.InitPoint( start, finish );

	int res = 0;
	do
	{
		bres.MakePointStep();		
		stopPoints[nLen+res++] = bres.GetDirection();		
	} while ( bres.GetDirection() != finish );

	return res;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CCommonPathFinder::LineSmoothing( const int STEP_LENGTH_THERE, const int MAX_NUM_OF_ATTEMPTS_THERE,
																			const int STEP_LENGTH_BACK, const int MAX_NUM_OF_ATTEMPTS_BACK )
{
	if ( nLength < 0 ) 
		return;

	stopPoints[nStart+nLength++] = finishPoint;

	// вперёд
	int curNum = 1, i = 1; 
	int checkNum = 0, numOfAttempts = 0, addLen = 0;

	while ( i < nLength-1 )
	{
		const int j = Min( i+STEP_LENGTH_THERE, nLength )-1;

		if ( numOfAttempts > MAX_NUM_OF_ATTEMPTS_THERE  ||  
			!Walkable( stopPoints[checkNum + nStart], stopPoints[j + nStart] ) )
		{
			addLen += SavePathThere( stopPoints[checkNum + nStart], stopPoints[i + nStart], addLen );

			checkNum = i;
			curNum = ++i;
			numOfAttempts = 0;
		}
		else
		{
			i = j;
			++numOfAttempts;
		}
	}

	addLen += SavePathThere( stopPoints[checkNum + nStart], stopPoints[nStart + nLength - 1], addLen );
	addPoints[addLen] = finishPoint;
	nLength = addLen+1;

	// назад
	i = nLength-2; 
	checkNum = nLength-1; 
	curNum = nLength-2; 
	numOfAttempts = addLen = 0;
	int nSegm = 0;
	while ( i > 0 )
	{
		const int j = Max( i-STEP_LENGTH_BACK, 0 );

		if ( numOfAttempts > MAX_NUM_OF_ATTEMPTS_BACK || !Walkable( addPoints[j], addPoints[checkNum] ) )
		{
			segmBegin[nSegm++] = addLen;			
			addLen += SavePathBack( addPoints[i], addPoints[checkNum], addLen );

			checkNum = i;
			curNum = --i;
			numOfAttempts = 0;
		}
		else
		{
			i = j;
			++numOfAttempts;
		}
	}

	// по сегментам
	segmBegin[nSegm++] = addLen;
	addLen += SavePathBack( addPoints[0], addPoints[checkNum], addLen );
	segmBegin[nSegm] = addLen;

	if ( nSegm == 1 )
	{
		addPoints[0] = startPoint;
		memcpy( &(addPoints[0]) + 1, &(stopPoints[0]), nLength * sizeof( SVector ) );
		addPoints[addLen] = finishPoint;
		nLength = addLen + 1;
	}
	else
	{

		// go through control points
		addPoints[0] = startPoint;	
		nLength = addLen = 1;
		i = nSegm-1;
		//	int up, down, mid;

		while ( i >= 0 )
		{

			//	binary search
			//		up = Min( i - 1, (int)TOLERANCE )+2;
			//			down = 1;

			//			while ( down != up )
			//			{
			//				mid = ( up+down) >> 1;
			//				if ( Walkable( pathPoints[segmBegin[i]], pathPoints[segmBegin[i-mid+1]-1] ) )
			//					down = mid+1;
			//				else
			//					up = mid;
			//			}

			//			j = up-1;


			//  simple bisections	
			if ( longPath )
			{
				int j = Min( i - 1, (int)TOLERANCE )+1;
				while ( j > 0  &&  !Walkable( stopPoints[segmBegin[i]], stopPoints[segmBegin[i-j+1]-1] ) )
					j >>= 1;

				if ( !j )
				{
					memcpy( &(addPoints[0]) + addLen, &(stopPoints[0]) + segmBegin[i], sizeof(SVector)*(segmBegin[i-j+1]-segmBegin[i]) );
					addLen += segmBegin[i-j+1]-segmBegin[i];
				}
				else
				{
					addLen += SavePathThere( stopPoints[segmBegin[i]], stopPoints[segmBegin[i-j+1]-1], addLen );
					addPoints[addLen++] = stopPoints[segmBegin[i-j+1]-1];
				}

				i -= j+1;
			}
			else
				//	sequential search	
			{
				int j = Max(1, i-TOLERANCE);
				while ( j <= i && !Walkable( stopPoints[segmBegin[i]], stopPoints[segmBegin[j]-1] ) )
					++j;

				addLen += SavePathThere( stopPoints[segmBegin[i]], stopPoints[segmBegin[j]-1], addLen );
				addPoints[addLen++] = stopPoints[segmBegin[j]-1];
				i = j-2;
			}
		}

		nLength = addLen;

		//	// for sequential search
		//		// записать путь
		//		stopPoints[0] = GetCode(pathPoints[segmBegin[nSegm-1]]-startPoint);
		//		nLength = 1;
		//		for ( i = nSegm-1; i > 0; --i )
		//		{
		//			for ( k = segmBegin[i]; k < segmBegin[i+1]-1; ++k )
		//				stopPoints[nLength++] = GetCode( pathPoints[k+1]-pathPoints[k] );

		//			stopPoints[nLength++] = GetCode( pathPoints[segmBegin[i - 1]]-pathPoints[segmBegin[i+1]-1] );
		//		}
		//		for ( k = segmBegin[0]; k < segmBegin[1]-1; ++k )
		//			stopPoints[nLength++] = GetCode( pathPoints[k+1]-pathPoints[k]);

	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CCommonPathFinder::DoesPathExist()
{
	const int nLineDistance = mDistance( startPoint, finishPoint );
	NextMapBufIndex();
	bPathFound = CalculatePath();
/*LOG{
	было:
		if ( bPathFound ...
	почему:
		во время отладки "гуляющей пехоты" убрал, потому что, искался путь на залоканый тайл, не находился, и finishPoint был ОЧЕНЬ
		не там где надо, в то время как vBestPoint был очень даже ничего
}LOG*/
	if ( nLength > nLineDistance*10 && vBestPoint.x != -1 && vBestPoint != finishPoint )
	{
		//DebugTrace( ">>>>>> Second chance: searchig path to point %d x %d", vBestPoint.x, vBestPoint.y );
		finishPoint = vBestPoint;
		NextMapBufIndex();
		bPathFound = CalculatePath();
	}

	return bPathFound;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IStaticPath* CCommonPathFinder::CreatePathAfterExistCheck()
{
	EraseCycles();

	if ( nLength < 0 )
		return 0;
	else
	{
		if ( longPath )
			LineSmoothing( STEP_LENGTH_THERE, MAX_NUM_OF_ATTEMPTS_THERE, STEP_LENGTH_BACK, MAX_NUM_OF_ATTEMPTS_BACK );
		else 
			LineSmoothing( STEP_LENGTH_THERE_SHORT, MAX_NUM_OF_ATTEMPTS_THERE_SHORT, STEP_LENGTH_BACK_SHORT, MAX_NUM_OF_ATTEMPTS_BACK_SHORT );

		return new CCommonStaticPath( this, pAIMap );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IStaticPath* CCommonPathFinder::CreatePath( const bool bCreateNullPath )
{
	if ( DoesPathExist() || nLength > 0 )
		return CreatePathAfterExistCheck();
	else
	{
		if ( bCreateNullPath )
		{
			nLength = 1;
			addPoints[0] = startPoint;
			return new CCommonStaticPath( this, pAIMap );
		}
		else
			return 0;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CCommonPathFinder::NextMapBufIndex()
{
	++mapBufIndex;
	if ( mapBufIndex >= MAX_MAPBUFINDEX )
	{
		mapBuf.FillZero();
		mapBufIndex = 1;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CCommonPathFinder::operator&( IBinSaver &saver )
{
	if ( saver.IsReading() )
		Init();


	return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CCommonPathFinder::MarkPathTiles()
{
	vector<SVector> terrainTiles;

	for ( int i = 0; i < GetPathLength(); ++i )
		terrainTiles.push_back( addPoints[i] );

	nGreenMarker = DebugInfoManager()->CreateMarker( nGreenMarker, terrainTiles, NDebugInfo::GREEN );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CCommonPathFinder::MarkPoints()
{
	vector<SVector> add, stop;
	add.reserve( nLength );
	stop.reserve( nLength );

	for ( int i = 0; i < nLength; ++i )
	{
		add.push_back( addPoints[i] );
		stop.push_back( stopPoints[i] );
	}

	nGreenMarker = DebugInfoManager()->CreateMarker( NDebugInfo::OBJECT_ID_GENERATE, add, NDebugInfo::GREEN );
	nRedMarker = DebugInfoManager()->CreateMarker( NDebugInfo::OBJECT_ID_GENERATE, stop, NDebugInfo::RED );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
START_REGISTER(PathFinderVars)
REGISTER_VAR_EX( "warfog_bound_width", NGlobal::VarFloatHandler, &fWarFogBoundWidth, 256.0f, STORAGE_NONE );
FINISH_REGISTER
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
