#include "stdafx.h"

#include "AIMap.h"
#include "BasePathUnit.h"
#include "CommonPathFinder.h"
#include "StandartPath2.h"
#include "StaticPathInternal.h"
#include "Terrain.h"

#include "Misc/Bresenham.h"

//максимальное количество точек, на которое будет смотреться вперед (в функции PeekPoint)
static const int MAX_LOOK_FORWARD_POINTS = 7;
//максимальная длина короткого пути в тайлах
static const int MAX_PATH_TILES_COUNT = 64;
//а вообще путь будет строиться именно на столько тайлов
static const int SMALL_PATH_TILES_COUNT = MAX_PATH_TILES_COUNT/4;
//стандартный сдвиг точек на статическом пути
static const int STATIC_PATH_SHIFT = 10;
// p.s. максимальное количество тайлов, которое можно записать - MAX_PATH_TILES_COUNT - ( 2 x MAX_LOOK_FORWARD_POINTS )
static std::vector<SVector> pathBuffer( MAX_PATH_TILES_COUNT );

#define INVALID_TILE SVector( -1, -1 )

inline const bool IsValidTile( const SVector &vTile )
{
	return vTile.x >= 0;
}

// возвращает true, если vPoint ближе к vEndPoint1, чем к vEndPoint2
inline const bool CompareDistance( const SVector &vEndPoint1, const SVector &vEndPoint2, const SVector &vPoint )
{
	return mDistance( vEndPoint1, vPoint ) < mDistance( vEndPoint2, vPoint );
}

CStandartPath2::CStandartPath2() : pPathFinder( 0 ), pAIMap( 0 ), nBoundTileRadius( -1 ), aiClass( EAC_NONE ), pStaticPath( 0 ),
  nCurStaticPathTile( -1 ), vStartPoint( VNULL2 ), vFinishPoint( VNULL2 ), vShift( VNULL2 ), nCurInsTile( 0 ), nCurPathTile( 0 ),
	nLastPathTile( -1 ), vShiftTile( 0, 0 ), vFinishTile( 0, 0 ), nUnitID( -1 )
{
	pathTiles.resize( MAX_PATH_TILES_COUNT, SVector( 0, 0 ) );
}

CStandartPath2::CStandartPath2( const CBasePathUnit *pUnit, IStaticPath *_pStaticPath, const CVec2 &_vStartPoint, const CVec2 &_vFinishPoint, CAIMap *_pAIMap )
: pPathFinder( pUnit->GetPathFinder() ), pAIMap( _pAIMap ), nBoundTileRadius( pUnit->GetBoundTileRadius() ), aiClass( pUnit->GetAIPassabilityClass() ),
	nCurInsTile( 0 ), vFinishTile( 0, 0 ), nUnitID( pUnit->GetUniqueID() )
{
	const int nUnitID = pUnit->GetUniqueID();
	pathTiles.resize( MAX_PATH_TILES_COUNT );
	InitByStaticPath( dynamic_cast<CCommonStaticPath *>( _pStaticPath ), _vStartPoint, _vFinishPoint, false );
}

void CStandartPath2::InitByStaticPath( CCommonStaticPath *_pStaticPath, const CVec2 &_vStartPoint, const CVec2 &_vFinishPoint, const bool bResetShift )
{
	pStaticPath = _pStaticPath;
	vStartPoint = _vStartPoint;
	SetFinishPoint( _vFinishPoint );
	vShift = ( bResetShift || pStaticPath == 0 ) ? VNULL2 : vFinishPoint - pStaticPath->GetFinishPoint();
	vShiftTile.x = vShift.x/pAIMap->GetTileSize();
	vShiftTile.y = vShift.y/pAIMap->GetTileSize();
	nCurPathTile = 0;
	nLastPathTile = 1;
	nCurStaticPathTile = 0;
	pathTiles[1] = pathTiles[0] = GetTile( vStartPoint );

	if ( pStaticPath == 0 )
		SetFinishPoint( vStartPoint );
	else
		CalculatePath( true, INVALID_TILE );
}

const bool CStandartPath2::CanUnitGo( const SVector &vTile ) const
{
	STerrainModeSetter oldMode( ELM_STATIC, pAIMap->GetTerrain() );
	return pAIMap->GetTerrain()->CanUnitGo( nBoundTileRadius, vTile, aiClass ) != FREE_NONE;
}

const SVector CStandartPath2::GetTile( const CVec2 &vPoint ) const
{
	return pAIMap->GetTile( vPoint );
}

const CVec2 CStandartPath2::GetPoint( const SVector &vTile ) const
{
	return pAIMap->GetPointByTile( vTile );
}

// вернет тайл в отрицательной области (INVALID_TILE) если точка не найдена, результат можно проверить функцией IsValidTile
const SVector CStandartPath2::GetTileWithShift( const SVector &vTile ) const
{
	const SVector vEndTile( vTile + vShiftTile );

	CBres bres;
	bres.InitPoint( vTile, vEndTile );

	bool bCanUnitGo = CanUnitGo( bres.GetDirection() );
	if ( !bCanUnitGo )
		return INVALID_TILE;

	SVector vResult = vTile;
	while ( bCanUnitGo && bres.GetDirection() != vEndTile )
	{
		vResult = bres.GetDirection();
		bres.MakePointStep();
		bCanUnitGo = CanUnitGo( bres.GetDirection() );
	}

	return vResult;
}

void CStandartPath2::CopyPath( const int nLength )
{
	if ( nLength < 0 )
		return;
	pPathFinder->GetTiles( &(pathBuffer[0]), nLength );
	const int nCopyTiles = (std::min)( nLength, MAX_PATH_TILES_COUNT - nLastPathTile );
	memcpy( &(pathTiles[0]) + nLastPathTile, &(pathBuffer[0]), sizeof(SVector) * (std::min)( nLength, MAX_PATH_TILES_COUNT - nLastPathTile ) );
	if ( nLength > nCopyTiles )
		memcpy( &(pathTiles[0]), &(pathBuffer[0]) + nCopyTiles, sizeof(SVector) * ( nLength - nCopyTiles ) );
	nLastPathTile = (nLastPathTile+nLength)%MAX_PATH_TILES_COUNT;
	// чтоб можно было спрашивать
	pathTiles[nLastPathTile] = pathBuffer[nLength-1];
}

const bool CStandartPath2::CalculatePath( const bool bShift, const SVector &vLastKnownGoodTile )
{
	// нечего считать, пора заканчивать
	if ( pathTiles[nLastPathTile] == vFinishTile ) 
		return false;

	const int nPrevStaticPathTile = nCurStaticPathTile;
	// выбираем точку на текущем статическом пути
	if ( bShift )
		nCurStaticPathTile = (std::min)( nCurStaticPathTile + STATIC_PATH_SHIFT, pStaticPath->GetLength()-2 );
	
	SVector vNextTile = GetTileWithShift( pStaticPath->GetTile( nCurStaticPathTile ) );
	bool bNextTileValid = true;
	if ( !IsValidTile( vNextTile ) )
	{
		int nShift = 1;
		while ( nShift < STATIC_PATH_SHIFT && !IsValidTile( vNextTile ) )
		{
			if ( nCurStaticPathTile + nShift < pStaticPath->GetLength()-1 )
				vNextTile = GetTileWithShift( pStaticPath->GetTile( nCurStaticPathTile + nShift ) );
			if ( IsValidTile( vNextTile ) )
				nCurStaticPathTile += nShift;
			else if ( nCurStaticPathTile - nShift > nPrevStaticPathTile )
			{
				vNextTile = GetTileWithShift( pStaticPath->GetTile( nCurStaticPathTile - nShift ) );
				if ( IsValidTile( vNextTile ) )
					nCurStaticPathTile -= nShift;
			}
			++nShift;
		}
		if ( !IsValidTile( vNextTile ) )
		{
			vNextTile = pStaticPath->GetTile( nCurStaticPathTile );
			bNextTileValid = false;
		}
	}

	//а вот теперь считаем маленький путь, конечный тайл - vNextTile, хорош ли он - bNextTileValid, его индекс - nCurStaticPathTile
	if ( IsValidTile( vLastKnownGoodTile ) )
	{
		//DebugTrace( ">>>> CalculatePath: %d x %d - %d x %d (%s) (valid: %d x %d) for unit %d (%d & %d)", pathTiles[nLastPathTile].x, pathTiles[nLastPathTile].y, vNextTile.x, vNextTile.y, bNextTileValid ? "true" : "false", vLastKnownGoodTile.x, vLastKnownGoodTile.y, nUnitID, aiClass, nBoundTileRadius );
		pPathFinder->SetPathParameters( nBoundTileRadius, aiClass, GetPoint( pathTiles[nLastPathTile] ), GetPoint( vNextTile ), vLastKnownGoodTile, pAIMap );
	}
	else
	{
		//DebugTrace( ">>>> CalculatePath: %d x %d - %d x %d (%s) (valid: none) for unit %d (%d & %d)", pathTiles[nLastPathTile].x, pathTiles[nLastPathTile].y, vNextTile.x, vNextTile.y, bNextTileValid ? "true" : "false", nUnitID, aiClass, nBoundTileRadius );
		pPathFinder->SetPathParameters( nBoundTileRadius, aiClass, GetPoint( pathTiles[nLastPathTile] ), GetPoint( vNextTile ), pathTiles[nLastPathTile], pAIMap );
	}
	CPtr<IStaticPath> pFoundStaticPath = pPathFinder->CreatePath( false );
	pPathHistory2 = pPathHistory1;
	pPathHistory1 = pFoundStaticPath;

	// если путь не найден - ничего не делаем, или на следующий раз посчитается, или путь закончится
	// переделывать путь смысла нет - все равно не найдется
	if ( pFoundStaticPath == 0 )
		return false;

	if ( pFoundStaticPath->GetStartTile() == pFoundStaticPath->GetFinishTile() )
		return false;

	if ( pFoundStaticPath->GetFinishTile() != vNextTile )
		bNextTileValid = false;

	// если этот путь не слишком длинный - копируем его
	if ( pFoundStaticPath->GetLength() <= SMALL_PATH_TILES_COUNT )
	{
		CopyPath( pFoundStaticPath->GetLength() );
		// это был последний шаг - корректируем конечную точку
		if ( nCurStaticPathTile == pStaticPath->GetLength()-2 )
			SetFinishTile( pathTiles[nLastPathTile] );
	}
	else
	{
		// подбираем nCurStaticPathTile и тайл на пути, чтобы они были максимально близки
		int nLength = (std::min)( pFoundStaticPath->GetLength()-1, MAX_PATH_TILES_COUNT - 2*MAX_LOOK_FORWARD_POINTS );
		bool bCloseToFinish = false;
		do
		{
			bCloseToFinish = CompareDistance( vFinishTile, vNextTile, pFoundStaticPath->GetTile( nLength ) );
			if ( !bCloseToFinish )
				--nLength;
		} while( nLength > SMALL_PATH_TILES_COUNT && !bCloseToFinish );
    CopyPath( nLength );

		// попалась точка, которая ближе к финишу, чем к желаемой
		if ( bCloseToFinish )
		{
			int nBestStaticPathTile = nCurStaticPathTile;
			int nBestDistance = mDistance( pStaticPath->GetTile( nBestStaticPathTile ) + vShiftTile, pathTiles[nLastPathTile] );
			for ( int i = nCurStaticPathTile+1; i < pStaticPath->GetLength()-1; ++i )
			{
				const int nThisDistance = mDistance( pStaticPath->GetTile( i ) + vShiftTile, pathTiles[nLastPathTile] );
				if ( nThisDistance < nBestDistance )
				{
					nBestDistance = nThisDistance;
					nBestStaticPathTile = i;
				}
			}
			nCurStaticPathTile = nBestStaticPathTile;
		}
		// это был последний шаг - корректируем конечную точку
		if ( nCurStaticPathTile == pStaticPath->GetLength()-2 )
			SetFinishTile( pFoundStaticPath->GetFinishTile() );
	}
	return true;
}

const CVec2 CStandartPath2::PeekPoint( const int _nShift ) const
{
	if ( IsFinished() )
		return vFinishPoint;

	if ( nCurInsTile + _nShift < insTiles.size() )
		return GetPoint( insTiles[nCurInsTile+_nShift] );

	int nShift = nCurInsTile + _nShift - insTiles.size();
	NI_VERIFY( nShift <= MAX_LOOK_FORWARD_POINTS, "Cannot predict point. Shift too far", nShift = MAX_LOOK_FORWARD_POINTS );
  
	const int nLastPathTile2 = ( nCurPathTile < nLastPathTile ) ? nLastPathTile : nLastPathTile + MAX_PATH_TILES_COUNT; 
	if ( nCurPathTile + nShift > nLastPathTile2 )
		return vFinishPoint;

	const int nTile = ( nCurPathTile + nShift )%MAX_PATH_TILES_COUNT;
	return GetPoint( pathTiles[nTile] );
}

void CStandartPath2::Shift( const int _nShift )
{
	if ( IsFinished() )
		return;
	if ( nCurInsTile + _nShift < insTiles.size() )
	{
		nCurInsTile += _nShift;
		return;
	}

	const int nShift = nCurInsTile + _nShift - insTiles.size();
	nCurInsTile = insTiles.size();

	nCurPathTile = ( nCurPathTile + nShift )%MAX_PATH_TILES_COUNT;
	const int nLastPathTile2 = ( nCurPathTile < nLastPathTile ) ? nLastPathTile : nLastPathTile + MAX_PATH_TILES_COUNT; 
	if ( nCurPathTile + MAX_LOOK_FORWARD_POINTS >= nLastPathTile2 )
	{
		CalculatePath( true, INVALID_TILE );
	}
}

void CStandartPath2::InsertTiles( const std::list<SVector> &tiles )
{
	insTiles.clear();
	for ( std::list<SVector>::const_iterator it = tiles.begin(); it != tiles.end(); ++it )
		insTiles.push_back( *it );
	nCurInsTile = 0;
}

const bool CStandartPath2::CanGoBackward( const CBasePathUnit *pUnit ) const
{ 
	return pUnit->CanGoBackward() && pStaticPath->GetLength() * float( pAIMap->GetTileSize() ) <= pUnit->GetUnitProfile().GetHalfLength() * 5.0f;
}

void CStandartPath2::RecoverPath( const CVec2 &vPoint, const bool bIsPointAtWater, const SVector &vLastKnownGoodTile )
{
	//DebugTrace( "(%d) CStandartPath2::RecoverPath( %2.3f x %2.3f, %s, %d x %d )", nUnitID, vPoint.x, vPoint.y, bIsPointAtWater ? "true" : "false", vLastKnownGoodTile.x, vLastKnownGoodTile.y );
	CPtr<IStaticPath> pNewStaticPath = 0;
	{
		STerrainModeSetter modeSetter( ELM_STATIC, pAIMap->GetTerrain() );
		pPathFinder->SetPathParameters( nBoundTileRadius, aiClass, vPoint, vFinishPoint, vLastKnownGoodTile, pAIMap );
		pNewStaticPath = pPathFinder->CreatePath( false );
	}
	if ( IsValid( pStaticPath ) && IsValid( pNewStaticPath ) && nCurStaticPathTile < pStaticPath->GetLength()-2 )
	{
		CPtr<IStaticPath> pBackStaticPath = 0;
		STerrainModeSetter modeSetter( ELM_STATIC, pAIMap->GetTerrain() );
		pPathFinder->SetPathParameters( nBoundTileRadius, aiClass, vPoint, GetPoint( pStaticPath->GetTile( nCurStaticPathTile ) ), vLastKnownGoodTile, pAIMap );
		pBackStaticPath = pPathFinder->CreatePath( false );
		if ( IsValid( pBackStaticPath ) && pBackStaticPath->GetLength() + pStaticPath->GetLength() - nCurStaticPathTile < pNewStaticPath->GetLength()+STATIC_PATH_SHIFT )
		{
			if ( pBackStaticPath->MergePath( pStaticPath, nCurStaticPathTile ) )
				pNewStaticPath = pBackStaticPath;
		}
	}

	// pNewStaticPath can still be null.. so just pull some crap out to prevent this rare crash
	// idk if this approach is good or not, but it's better than crash!
	if ( !pNewStaticPath )
		pNewStaticPath = pPathFinder->CreatePath( false );

	InitByStaticPath( dynamic_cast_ptr<CCommonStaticPath*>( pNewStaticPath ), vPoint, pNewStaticPath->GetFinishPoint(), true );
}

void CStandartPath2::RecalcPath( const CVec2 &vPoint, const bool bIsPointAtWater, const SVector &vLastKnownGoodTile )
{
	vStartPoint = vPoint;
	nCurPathTile = 0;
	nLastPathTile = 1;
	pathTiles[0] = pathTiles[1] = pAIMap->GetTile( vPoint );

	if ( !CalculatePath( false, vLastKnownGoodTile ) )
		nLastPathTile = 0;
}

void CStandartPath2::MarkPath( const int nID, const NDebugInfo::EColor color ) const
{
	std::vector<SVector> tiles;

	if ( IsFinished() )
		return;

	if ( nCurInsTile < insTiles.size() )
	{
		for ( int i = nCurInsTile; i < insTiles.size(); ++i )
			tiles.push_back( insTiles[i] );
	}

	int i = nCurPathTile;
	while ( i != nLastPathTile )
	{
		tiles.push_back( pathTiles[i] );
		++i;
		if ( i == MAX_PATH_TILES_COUNT ) 
			i = 0;
	}
	DebugInfoManager()->CreateMarker( nID, tiles, color );
}

REGISTER_SAVELOAD_CLASS( COMMON_RTS_AI, 0x3121AC40, CStandartPath2 );


