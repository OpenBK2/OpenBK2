#include "stdafx.h"

#include "Misc/bresenham.h"
#include "Terrain.h"
#include "Common_RTS_AI/AIMap.h"
#include "Image/Image.h"
#include "Image/ImageTGA.h"
#include "DebugTools/DebugInfoManager.h"
#include "Misc/Bresenham.h"

#include <algorithm>
#include <cstdint>

//*******************************************************************
//*												  CTerrain																*
//*******************************************************************

static const SVector offsets[4] = { SVector( 1, 0 ), SVector( 0, -1 ), SVector( -1, 0 ), SVector( 0, 1 ) };

CTerrain::CTerrain()
: eMode( ELM_ALL ), bInitMode( false ), nTmpLockUnitID( -1 )
{
}

CTerrain::CTerrain( CAIMap *_pAIMap, const bool _bInitMode )
: eMode( ELM_ALL ), bInitMode( false ), nTmpLockUnitID( -1 )
{
	pAIMap = _pAIMap;
	eMode = ELM_ALL;
	bInitMode = _bInitMode;

	const int nSizeX = pAIMap->GetSizeX();
	const int nSizeY = pAIMap->GetSizeY();

	digImpossible.SetSizes( nSizeX, nSizeY );
	digImpossible.FillZero();

	buf.SetSizes( nSizeX, nSizeY );
	buf.FillEvery( EAC_NONE );

	passTypes.SetSizes( nSizeX / 2 + 1, nSizeY / 2 + 1 );
	passTypes.FillZero();
	passClasses.resize( 1, 0 );
	passabilities.resize( 1, 1.0f );

	unitsBuf.resize( 2 );
	for ( int i = 0; i < unitsBuf.size(); ++i )
	{
		unitsBuf[i].SetSizes( nSizeX, nSizeY );
		unitsBuf[i].FillZero();
	}

	bridgeTiles.SetSizes( nSizeX, nSizeY );
	bridgeTiles.FillZero();

	riverCliffTiles.SetSizes( nSizeX, nSizeY );
	riverCliffTiles.FillZero();

	terrainTypes.SetSizes( nSizeX, nSizeY );
	terrainTypes.FillZero();

	soil.SetSizes( nSizeX, nSizeY );
	soil.FillZero();

	InitMaxesDefault( nSizeX, nSizeY );

	pAIMap->RecreateCircles();

	classIndices.resize( (int)EAC_ANY+1, -1 );
	classIndices[(int)EAC_WHELL]   = GetClassIndex( EAC_WHELL );
	classIndices[(int)EAC_TRACK]   = GetClassIndex( EAC_TRACK );
	classIndices[(int)EAC_HUMAN]   = GetClassIndex( EAC_HUMAN );
	classIndices[(int)EAC_RIVER]   = GetClassIndex( EAC_RIVER );
	classIndices[(int)EAC_SEA]     = GetClassIndex( EAC_SEA );
	classIndices[(int)EAC_WATER]   = GetClassIndex( EAC_WATER );
	classIndices[(int)EAC_TERRAIN] = GetClassIndex( EAC_TERRAIN );
	classIndices[(int)EAC_ANY]     = GetClassIndex( EAC_ANY );

#ifndef _FINALRELEASE
	debugLockInfo.resize( 1024 );
	nDebugLockInfoCount = 0;
	nDebugLockInfoPos = 0;
#endif
}

const bool CTerrain::CanDigEntrenchment( const int x, const int y ) const
{
	return !digImpossible.GetData( x, y );
}

const void CTerrain::AddUndigableTiles( const std::list<SVector> &tiles )
{
	for ( std::list<SVector>::const_iterator it = tiles.begin(); it != tiles.end(); ++it )
		digImpossible.SetData( it->x, it->y );
}

void CTerrain::PrepareTerraTypes( const int nCount )
{
	tileDigImpossible.resize( nCount, 0 );
	passClasses.resize( nCount, 0 );
	passabilities.resize( nCount, 0 );
	soilParams.resize( nCount, 0 );
}

void CTerrain::SetTerraTypes( const int nIndex, const float fPass, const uint32_t passClass, const uint8_t soilType, const uint8_t digImpossible )
{
	passabilities[nIndex] = fPass;
	passClasses[nIndex] = passClass;
	soilParams[nIndex] = soilType;
	tileDigImpossible[nIndex] = digImpossible;
}

void CTerrain::UpdateTypes( const int nX1, const int nY1, const int nX2, const int nY2, const CArray2D<uint8_t> &tiles )
{
	for ( int y = nY1; y < nY2; ++y )
	{
		for ( int x = nX1; x < nX2; ++x )
		{
			const int rightX = x;
			const int rightY = y;

			uint8_t tileType = tiles[y][x];
			//CRAP{ until water is 0xFF
			if ( tileType == 0xFF )
			{
				const EAIClasses aiClass = EAC_TERRAIN;
				LockTile( rightX * 2,			rightY * 2		, aiClass );
				LockTile( rightX * 2,			rightY * 2 + 1, aiClass );
				LockTile( rightX * 2 + 1, rightY * 2		, aiClass );
				LockTile( rightX * 2 + 1, rightY * 2 + 1, aiClass );

				int nX = Clamp( rightX * 2, 0, terrainTypes.GetSizeX() - 1 );
				int nY = Clamp( rightY * 2, 0, terrainTypes.GetSizeY() - 1 );
				terrainTypes.SetData( nX, nY, ETT_WATER_TERRAIN );

				nX = Clamp( rightX * 2, 0, terrainTypes.GetSizeX() - 1 );
				nY = Clamp( rightY * 2 + 1, 0, terrainTypes.GetSizeY() - 1 );
				terrainTypes.SetData( nX, nY, ETT_WATER_TERRAIN );

				nX = Clamp( rightX * 2 + 1, 0, terrainTypes.GetSizeX() - 1 );
				nY = Clamp( rightY * 2, 0, terrainTypes.GetSizeY() - 1 );
				terrainTypes.SetData( nX, nY, ETT_WATER_TERRAIN );

				nX = Clamp( rightX * 2 + 1, 0, terrainTypes.GetSizeX() - 1 );
				nY = Clamp( rightY * 2 + 1, 0, terrainTypes.GetSizeY() - 1 );
				terrainTypes.SetData( nX, nY, ETT_WATER_TERRAIN );
				/*				
				soil[rightY * 2][rightX * 2 ] = soil[rightY * 2][rightX * 2 + 1] = 
					soil[rightY * 2 + 1][rightX * 2	] = soil[rightY * 2 + 1][rightX * 2 + 1] = SVectorStripeObject::ESP_DUST;
				*/

				soil[rightY * 2][rightX * 2 ] = soil[rightY * 2][rightX * 2 + 1] = 
					soil[rightY * 2 + 1][rightX * 2	] = soil[rightY * 2 + 1][rightX * 2 + 1] = 0;
				
				continue;
			}
			else if ( tileType >= soilParams.size() ) 
			{
				tileType = 0;
			}
			//CRAP}

			// soil type
			const uint8_t cSoilType = soilParams[tileType];
			soil[rightY * 2][rightX * 2 ] = soil[rightY * 2][rightX * 2 + 1] = 
				soil[rightY * 2 + 1][rightX * 2	] = soil[rightY * 2 + 1][rightX * 2 + 1] = cSoilType;

			// инициализировать возможность строительства окопов

			if ( tileDigImpossible[tileType] )
			{
				digImpossible.SetData( rightX * 2,			rightY * 2 );
				digImpossible.SetData( rightX * 2 + 1,	rightY * 2 );
				digImpossible.SetData( rightX * 2,			rightY * 2 + 1 );
				digImpossible.SetData( rightX	* 2 + 1,  rightY * 2 + 1 );
			}

			// passability
			passTypes[rightY][rightX] = tileType;

			//CRAP{ unitl passClasses undefined
			//const EAIClasses aiClass = passabilities[tileType] == 0.0f ? EAC_ANY : (EAIClasses)(passClasses[tileType]);
			const EAIClasses aiClass = EAC_WATER;
			//CRAP}

			LockTile( rightX * 2,			rightY * 2		, aiClass );
			LockTile( rightX * 2,			rightY * 2 + 1, aiClass );
			LockTile( rightX * 2 + 1, rightY * 2		, aiClass );
			LockTile( rightX * 2 + 1, rightY * 2 + 1, aiClass );

			// инициализировать типы terrain для воронок
			if ( passClasses[ tileType ] & EAC_WATER )
			{				
				int nX = Clamp( rightX * 2, 0, terrainTypes.GetSizeX() - 1 );
				int nY = Clamp( rightY * 2, 0, terrainTypes.GetSizeY() - 1 );
				terrainTypes.SetData( nX, nY, ETT_WATER_TERRAIN );

				nX = Clamp( rightX * 2, 0, terrainTypes.GetSizeX() - 1 );
				nY = Clamp( rightY * 2 + 1, 0, terrainTypes.GetSizeY() - 1 );
				terrainTypes.SetData( nX, nY, ETT_WATER_TERRAIN );

				nX = Clamp( rightX * 2 + 1, 0, terrainTypes.GetSizeX() - 1 );
				nY = Clamp( rightY * 2, 0, terrainTypes.GetSizeY() - 1 );
				terrainTypes.SetData( nX, nY, ETT_WATER_TERRAIN );

				nX = Clamp( rightX * 2 + 1, 0, terrainTypes.GetSizeX() - 1 );
				nY = Clamp( rightY * 2 + 1, 0, terrainTypes.GetSizeY() - 1 );
				terrainTypes.SetData( nX, nY, ETT_WATER_TERRAIN );
			}
		}
	}

	InitExplosionTerrainTypes();
	InitMaxes( nX1, nX2, nY1, nY2 );
}

const ETerrainTypes CTerrain::GetTerrainType( const int nX, const int nY ) const
{
	if ( !pAIMap->IsTileInside( nX, nY ) || IsBridge( SVector( nX, nY ) ) )
		return ETT_EARTH_TERRAIN;
	else
		return ETerrainTypes( terrainTypes.GetData( nX, nY ) );
}

void CTerrain::AddWaterTiles( const std::list<SVector> &tiles )
{
	for ( std::list<SVector>::const_iterator iter = tiles.begin(); iter != tiles.end(); ++iter )
	{
		const SVector &tile = *iter;
		if ( pAIMap->IsTileInside(tile) )
			terrainTypes.SetData( tile.x, tile.y, ETT_WATER_TERRAIN );
	}
}

void CTerrain::AddTiles( const std::list<SVector> vTiles, const EAIClasses aiPassClass, const float fPassability, const int nSoilType, const bool bCanEntrench )
{
	const ELockMode oldMode = eMode;

	// if new passability isn't set yet, then add it to passability array.
	int nPassabilityIndex = -1;
	for ( int i = 0; i < passabilities.size(); ++i )
	{
		if ( passabilities[i] == fPassability )
		{
			nPassabilityIndex = i;
			break;
		}
	}
	if ( nPassabilityIndex == -1 )
	{
		passabilities.push_back( fPassability );
		nPassabilityIndex = passabilities.size() - 1 ;
	}

	const EAIClasses aiClass = ( passabilities[nPassabilityIndex] == 0.0f ) ? EAC_ANY : aiPassClass;
	const uint8_t cSoilParams = nSoilType;

	int downX = pAIMap->GetSizeX() - 1, downY = pAIMap->GetSizeY() - 1, upX = 0, upY = 0;

	for( std::list<SVector>::const_iterator it = vTiles.begin(); it != vTiles.end(); ++it )
	{
		const SVector tile = *it;
		if ( pAIMap->IsTileInside( tile ) )
		{
			if ( !bCanEntrench )
				digImpossible.SetData( tile.x, tile.y );
			else
				digImpossible.RemoveData( tile.x, tile.y );

			// влияние на скорость юнитов
			passTypes[tile.y / 2][tile.x / 2] = nPassabilityIndex;

			// проходимость
			UnlockTile( tile.x, tile.y, EAC_ANY );
			LockTile( tile.x, tile.y, aiClass );

			soil[tile.y][tile.x] = cSoilParams;

			downX = (std::min)( downX, tile.x - pAIMap->GetMaxUnitTileRadius() - 1 );
			downY = (std::min)( downY, tile.y - pAIMap->GetMaxUnitTileRadius() - 1 );
			upX = (std::max)( upX, tile.x + pAIMap->GetMaxUnitTileRadius() + 1 );
			upY = (std::max)( upY, tile.y + pAIMap->GetMaxUnitTileRadius() + 1 );

			if ( ( aiClass & EAC_WATER ) != EAC_WATER )
			{				
				int nX = Clamp( tile.x, 0, terrainTypes.GetSizeX() - 1 );
				int nY = Clamp( tile.y, 0, terrainTypes.GetSizeY() - 1 );
				terrainTypes.SetData( nX, nY, ETT_WATER_TERRAIN );
			}
		}
	}

	downX = (std::max)( 0, downX );
	downY = (std::max)( 0, downY );
	upX = (std::min)( pAIMap->GetSizeX() - 1, upX );
	upY = (std::min)( pAIMap->GetSizeY() - 1, upY );

	if ( aiClass == EAC_NONE )
	{
		SetMode( ELM_STATIC );
		UpdateMaxesForRemovedTiles( downX, upX, downY, upY, EAC_TERRAIN );
		UpdateMaxesForRemovedTiles( downX, upX, downY, upY, EAC_WATER );
		UpdateMaxesForRemovedTiles( downX, upX, downY, upY, EAC_ANY );
		SetMode( ELM_ALL );
		UpdateMaxesForRemovedTiles( downX, upX, downY, upY, EAC_TERRAIN );
		UpdateMaxesForRemovedTiles( downX, upX, downY, upY, EAC_WATER );
		UpdateMaxesForRemovedTiles( downX, upX, downY, upY, EAC_ANY );
	}
	else
	{
		if ( ( aiClass & EAC_WATER) == EAC_WATER )
		{
			SetMode( ELM_STATIC );
			UpdateMaxesForAddedTiles( downX, upX, downY, upY, EAC_WATER );
			SetMode( ELM_ALL );
			UpdateMaxesForAddedTiles( downX, upX, downY, upY, EAC_WATER );
		}
		if ( ( aiClass & EAC_TERRAIN) == EAC_TERRAIN )
		{
			SetMode( ELM_STATIC );
			UpdateMaxesForAddedTiles( downX, upX, downY, upY, EAC_TERRAIN );
			SetMode( ELM_ALL );
			UpdateMaxesForAddedTiles( downX, upX, downY, upY, EAC_TERRAIN );
		}
		SetMode( ELM_STATIC );
		UpdateMaxesForAddedTiles( downX, upX, downY, upY, EAC_ANY );
		SetMode( ELM_ALL );
		UpdateMaxesForAddedTiles( downX, upX, downY, upY, EAC_ANY );
	}

	SetMode( oldMode );
}

void CTerrain::AddMarineTiles( const std::list<SVector> coastTiles, const uint8_t coastSoilType, const std::list<SVector> waterTiles, const uint8_t waterSoilType )
{
	for ( std::list<SVector>::const_iterator it = coastTiles.begin(); it != coastTiles.end(); ++it )
	{
		if ( pAIMap->IsTileInside( *it ) )
		{
			LockTile( it->x, it->y, EAC_WATER );
			UnlockTile( it->x, it->y, EAC_TERRAIN );
			terrainTypes.SetData( it->x, it->y, ETT_EARTH_TERRAIN );
			soil[it->y][it->x] = coastSoilType;
		}
	}

	for ( std::list<SVector>::const_iterator it = waterTiles.begin(); it != waterTiles.end(); ++it )
	{
		if ( pAIMap->IsTileInside( *it ) )
		{
			UnlockTile( it->x, it->y, EAC_TERRAIN );
			terrainTypes.SetData( it->x, it->y, ETT_EARTH_TERRAIN );
			soil[it->y][it->x] = waterSoilType;
		}
	}
}

void CTerrain::ClearRiverCliffTiles()
{
	if ( !riverCliffTiles.IsEmpty() )
		riverCliffTiles.FillZero();
}


void CTerrain::AddRiverCliffTiles( const std::list<SVector> &tiles )
{
	for ( std::list<SVector>::const_iterator it = tiles.begin(); it != tiles.end(); ++it )
	{
		if ( pAIMap->IsTileInside( *it ) )
			riverCliffTiles.SetData( it->x, it->y );
	}
}


void CTerrain::InitMaxesDefault( const int nSizeX, const int nSizeY )
{
	maxes.SetSizes( GetClassIndex(EAC_COUNT), 2 );
	maxesSmooth.SetSizes( GetClassIndex(EAC_COUNT), 2 );
	for ( int i = 0; i < GetClassIndex(EAC_COUNT); ++i )
	{
		maxes[0][i].SetSizes( nSizeX, nSizeY );	
		maxes[1][i].SetSizes( nSizeX, nSizeY );
		maxes[0][i].FillZero();
		maxes[1][i].FillZero();

		maxesSmooth[0][i].SetSizes( nSizeX, nSizeY );	
		maxesSmooth[1][i].SetSizes( nSizeX, nSizeY );
		maxesSmooth[0][i].FillZero();
		maxesSmooth[1][i].FillZero();
	}

	maskForSmooth.SetSizes( nSizeX, nSizeY );
	maskForSmooth.FillZero();
	for ( int x = 0; x < nSizeX - 1; ++x )
	{
		maskForSmooth.SetData( x, 0 );
		maskForSmooth.SetData( x, nSizeY - 1 );
	}
	for ( int y = 1; y < nSizeY - 2; ++y )
	{
		maskForSmooth.SetData( 0, y );
		maskForSmooth.SetData( nSizeX - 1, y );
	}
}

void CTerrain::InitMaxes( const int _nX1, const int _nX2, const int _nY1, const int _nY2 )
{
	const int nX1 = (std::max)( 0, 2*_nX1 - pAIMap->GetMaxUnitTileRadius() );
	const int nX2 = (std::min)( pAIMap->GetSizeX(), 2*_nX2 + pAIMap->GetMaxUnitTileRadius() );
	const int nY1 = (std::max)( 0, 2*_nY1 - pAIMap->GetMaxUnitTileRadius() );
	const int nY2 = (std::min)( pAIMap->GetSizeY(), 2*_nY2 + pAIMap->GetMaxUnitTileRadius() );

	for ( int y = nY1; y < nY2; ++y )
	{
		for ( int x = nX1; x < nX2; ++x )
		{
			const int dX = (std::min)( x, pAIMap->GetSizeX() - x - 1 );
			const int dY = (std::min)( y, pAIMap->GetSizeY() - y - 1 );

			if ( dX < pAIMap->GetMaxUnitTileRadius() || dY < pAIMap->GetMaxUnitTileRadius() )
			{
				for ( int i = 0; i < GetClassIndex(EAC_COUNT); ++i )
				{
					maxes[0][i].SetData( x, y, (std::min)( dX, dY ) + 1 );
					maxes[1][i].SetData( x, y, (std::min)( dX, dY ) + 1 );
					maxesSmooth[0][i].SetData( x, y, (std::min)( dX, dY ) + 1 );
					maxesSmooth[1][i].SetData( x, y, (std::min)( dX, dY ) + 1 );
				}
			}
			else
			{
				for ( int i = 0; i < GetClassIndex(EAC_COUNT); ++i )
				{
					maxes[0][i].SetData( x, y, pAIMap->GetMaxUnitTileRadius() + 1 );
					maxes[1][i].SetData( x, y, pAIMap->GetMaxUnitTileRadius() + 1 );
					maxesSmooth[0][i].SetData( x, y, pAIMap->GetMaxUnitTileRadius() + 1 );
					maxesSmooth[1][i].SetData( x, y, pAIMap->GetMaxUnitTileRadius() + 1 );
				}
			}
		}
	}
}

void CTerrain::LockUnitProfile( const SUnitProfile &profile, const int id, SVector *pDownTile, SVector *pUpTile, const bool bWater )
{
	RemoveTemporaryUnlocking( id );

	CUnitsRects::iterator pos = unitsRects.find( id );
	if ( pos != unitsRects.end() )
		return;

	unitsRects[id] = SUnitTileInfo( profile, bWater );

	std::vector<SVector> tiles;
	if ( profile.bRect )
	{
		const SRect &rect( profile.rect );
		pAIMap->GetTilesCoveredByQuadrangle( rect.v1(), rect.v2(), rect.v3(), rect.v4(), &tiles );
	}
	else
	{
		const CCircle &circle( profile.circle );
		pAIMap->GetTilesCoveredByCircle( circle.center, circle.r, &tiles );
	}

	int downX = 2 * pAIMap->GetMaxMapSize(); 
	int downY = 2 * pAIMap->GetMaxMapSize(); 

	int upX = -10, upY = -10;

	const int nWater = ( bWater ) ? 1 : 0; 

	// lock tiles
	//DebugTrace( "CTerrain::LockUnitProfile( %d ( %2.3f x %2.3f, %2.3f ), %d, ..., %s )", tiles.size(), profile.GetCenter().x, profile.GetCenter().y, profile.GetRadius(), id, bWater ? "true" : "false" );
	for ( std::vector<SVector>::const_iterator iter = tiles.begin(); iter != tiles.end(); ++iter )
	{
		const SVector &tile( *iter );

		if ( GetTerrainType( tile ) == ETT_MARINE_TERRAIN )
		{
			++unitsBuf[1 - nWater][tile.y][tile.x];
			if ( unitsBuf[1 - nWater][tile.y][tile.x] == 1 )
			{
				downX = (std::min)( downX, tile.x );
				downY = (std::min)( downY, tile.y );
				upX = (std::max)( upX, tile.x );
				upY = (std::max)( upY, tile.y );
			}
		}
		++unitsBuf[nWater][tile.y][tile.x];
		if ( unitsBuf[nWater][tile.y][tile.x] == 1 )
		{
			downX = (std::min)( downX, tile.x );
			downY = (std::min)( downY, tile.y );
			upX = (std::max)( upX, tile.x );
			upY = (std::max)( upY, tile.y );
		}
	}

	pDownTile->x = downX;
	pDownTile->y = downY;
	pUpTile->x = upX;
	pUpTile->y = upY;
}

bool CTerrain::UnlockUnitProfile( const int id, SVector *pDownTile, SVector *pUpTile, bool *pbWater )
{
	RemoveTemporaryUnlocking( id );

	CUnitsRects::iterator pos = unitsRects.find( id );
	if ( pos != unitsRects.end() )
	{
		std::vector<SVector> tiles;
		if ( pos->second.profile.bRect )
		{
			const SRect &rect( pos->second.profile.rect );
			pAIMap->GetTilesCoveredByQuadrangle( rect.v1(), rect.v2(), rect.v3(), rect.v4(), &tiles );
		}
		else
		{
			const CCircle &circle( pos->second.profile.circle );
			pAIMap->GetTilesCoveredByCircle( circle.center, circle.r, &tiles );
		}

		int downX = 2 * pAIMap->GetMaxMapSize(); 
		int downY = 2 * pAIMap->GetMaxMapSize(); 

		int upX = -10, upY = -10;

		*pbWater = pos->second.bWater;
		const int nWater = pos->second.bWater ? 1 : 0; 

		//DebugTrace( "CTerrain::UnlockUnitProfile( %d ( %2.3f x %2.3f, %2.3f ), %d, ..., %s )", tiles.size(), pos->second.profile.GetCenter().x, pos->second.profile.GetCenter().y, pos->second.profile.GetRadius(), id, *pbWater ? "true" : "false" );
		for ( std::vector<SVector>::const_iterator iter = tiles.begin(); iter != tiles.end(); ++iter )
		{
			const SVector &tile( *iter );

			if ( GetTerrainType( tile ) == ETT_MARINE_TERRAIN )
			{
				--unitsBuf[1 - nWater][tile.y][tile.x];
				if ( unitsBuf[1 - nWater][tile.y][tile.x] == 0 )
				{
					downX = (std::min)( downX, tile.x );
					downY = (std::min)( downY, tile.y );
					upX = (std::max)( upX, tile.x );
					upY = (std::max)( upY, tile.y );
				}
			}
			--unitsBuf[nWater][tile.y][tile.x];
			if ( unitsBuf[nWater][tile.y][tile.x] == 0 )
			{
				downX = (std::min)( downX, tile.x );
				downY = (std::min)( downY, tile.y );
				upX = (std::max)( upX, tile.x );
				upY = (std::max)( upY, tile.y );
			}
		}

		pDownTile->x = downX;
		pDownTile->y = downY;
		pUpTile->x = upX;
		pUpTile->y = upY;
		unitsRects.erase( id );
		return true;
	}
	else
		return false;
}

struct SUpDownFinder
{
	SVector &downTile, &upTile;
	CAIMap *pAIMap;

	SUpDownFinder( SVector &_downTile, SVector &_upTile, CAIMap *_pAIMap )
		: downTile( _downTile ), upTile( _upTile ), pAIMap( _pAIMap )
	{
		downTile.x = downTile.y = 2 * pAIMap->GetMaxMapSize();
		//
		upTile.x = upTile.y = -10;
	}

	void operator()( const SObjTileInfo &el )
	{
		downTile.x = (std::min)( downTile.x, el.tile.x );
		downTile.y = (std::min)( downTile.y, el.tile.y );
		upTile.x = (std::max)( upTile.x, el.tile.x );
		upTile.y = (std::max)( upTile.y, el.tile.y );
	}
	void operator()( const SVector &el )
	{
		downTile.x = (std::min)( downTile.x, el.x );
		downTile.y = (std::min)( downTile.y, el.y );
		upTile.x = (std::max)( upTile.x, el.x );
		upTile.y = (std::max)( upTile.y, el.y );
	}
};

void CTerrain::LockTiles( const std::list<SObjTileInfo> &tiles, SVector *pDownTile, SVector *pUpTile )
{
	for ( std::list<SObjTileInfo>::const_iterator iter = tiles.begin(); iter != tiles.end(); ++iter )
		LockTile( *iter );

	SUpDownFinder finder( *pDownTile, *pUpTile, pAIMap );
	for_each( tiles.begin(), tiles.end(), finder );
}

void CTerrain::UnlockTiles( const std::list<SObjTileInfo> &tiles, SVector *pDownTile, SVector *pUpTile )
{
	for ( std::list<SObjTileInfo>::const_iterator iter = tiles.begin(); iter != tiles.end(); ++iter )
		UnlockTile( *iter );

	SUpDownFinder finder( *pDownTile, *pUpTile, pAIMap );
	for_each( tiles.begin(), tiles.end(), finder );
}

void CTerrain::InitExplosionTerrainTypes()
{
	CArray2D4Bit terrainTypesOld( terrainTypes );

	for ( int i = 0; i < terrainTypes.GetSizeY(); ++i )
	{
		for ( int j = 0; j < terrainTypes.GetSizeX(); ++j )
		{
			bool bRiverNear = false;
			bool bTerrainNear = false;
			for ( int di = -1; di <= 1; ++di )
			{
				for ( int dj = -1; dj <= 1; ++dj )
				{
					if ( di != 0 && dj != 0 )
					{
						const int nI = Clamp( i + di, 0, terrainTypes.GetSizeY() - 1 );
						const int nJ = Clamp( j + dj, 0, terrainTypes.GetSizeX() - 1 );

						if ( terrainTypesOld.GetData( nJ, nI ) == ETT_WATER_TERRAIN )
							bRiverNear = true;
						if ( terrainTypesOld.GetData( nJ, nI ) == ETT_EARTH_TERRAIN	)
							bTerrainNear = true;
					}
				}
			}

			if ( terrainTypesOld.GetData( j, i ) == ETT_WATER_TERRAIN && bTerrainNear )
				terrainTypes.SetData( j, i, ETT_MARINE_TERRAIN );
			if ( terrainTypesOld.GetData( j, i ) == ETT_EARTH_TERRAIN && bRiverNear )
				terrainTypes.SetData( j, i, ETT_MARINE_TERRAIN );
		}
	}
}

bool CTerrain::TemporaryUnlockUnitProfile( const int id, const SUnitProfile &unitProfile, const int nDecrease, const bool bWater )
{
	std::unordered_map< int, std::pair< bool, CTmpLockInfoBuf > >::iterator pos = tmpUnlockUnitsMap.find( id );
	
	if ( pos == tmpUnlockUnitsMap.end() )
	{
		const int nWater = bWater ? 1 : 0;
		std::list<SVector> tiles;

		SUnitProfile profile = unitProfile;
		profile.Compress( 1.25f );
		if ( profile.bRect )
		{
			const SRect &rect( profile.rect );
			pAIMap->GetTilesCoveredByQuadrangle( rect.v1(), rect.v2(), rect.v3(), rect.v4(), &tiles );
		}
		else
		{
			const CCircle &circle( profile.circle );
			pAIMap->GetTilesCoveredByCircle( circle.center, circle.r, &tiles );
		}

		CTmpLockInfoBuf tmpUnlockUnitsBuf;

		for ( std::list<SVector>::iterator iter = tiles.begin(); iter != tiles.end(); ++iter )
		{
			const SVector tile = *iter;

			if ( pAIMap->IsTileInside( tile ) )
			{
				tmpUnlockUnitsBuf.emplace_back();
				tmpUnlockUnitsBuf.back().tile = tile;
				tmpUnlockUnitsBuf.back().nUnitsBuf = unitsBuf[nWater][tile.y][tile.x];
				tmpUnlockUnitsBuf.back().aiClass = buf[tile.y][tile.x];
				unitsBuf[nWater][tile.y][tile.x] = (std::max)( 0, unitsBuf[nWater][tile.y][tile.x] - nDecrease );
				//buf[tile.y][tile.x] = EAC_NONE;
			}
		}

		tmpUnlockUnitsMap[id] = std::pair< bool, CTmpLockInfoBuf >( bWater, tmpUnlockUnitsBuf );

		return true;
	}
	else
		return false;
}

void CTerrain::RemoveTemporaryUnlocking( const int id )
{
	std::unordered_map< int, std::pair< bool, CTmpLockInfoBuf > >::iterator pos = tmpUnlockUnitsMap.find( id );

	if ( pos != tmpUnlockUnitsMap.end() )
	{
		const int nWater = pos->second.first ? 1 : 0;

		for ( CTmpLockInfoBuf::iterator iter = pos->second.second.begin(); iter != pos->second.second.end(); ++iter )
		{
			const int x = iter->tile.x;
			const int y = iter->tile.y;

			unitsBuf[nWater][y][x] = iter->nUnitsBuf;
			buf[y][x] = iter->aiClass;
		}

		tmpUnlockUnitsMap.erase( pos );
	}
}

const int CTerrain::GetTerrainPassabilityType( const int nX, const int nY ) const
{
	if ( !pAIMap->IsTileInside( nX, nY ) )
		return -1;
	else
		return passTypes[nY >> 1][nX >> 1];
}

void CTerrain::RemoveTerrainPassability( const int nX, const int nY )
{
	if ( pAIMap->IsTileInside( nX, nY ) )
	{
		const EAIClasses aiClass = GetPass( nX, nY ) == 0.0f ? EAC_ANY : (EAIClasses)(passClasses[ GetTerrainPassabilityType( nX, nY ) ] );
		UnlockTile( nX, nY, aiClass );
	}
}

void CTerrain::SetTerrainPassability( const int nX, const int nY, const int nTerrainType )
{
	if ( pAIMap->IsTileInside( nX, nY ) && nTerrainType != 0xff /*water, just crap*/ )
	{
		passTypes[nY >> 1][nX >> 1] = nTerrainType;		
		const EAIClasses aiClass = passabilities[nTerrainType] == 0.0f ? EAC_ANY : (EAIClasses)(passClasses[nTerrainType]);
		LockTile( nX, nY, aiClass );
	}
}

void CTerrain::UpdateTerrainPassabilityRect( const int nMinX, const int nMinY, const int nMaxX, const int nMaxY, bool bRemove )
{
	for ( int x = nMinX; x <= nMaxX; ++x )
	{
		for ( int y = nMinY; y <= nMaxY; ++y )
		{
			if ( bRemove )
				RemoveTerrainPassability( x, y );
			else
				SetTerrainPassability( x, y, GetTerrainPassabilityType( x, y ) );
		}
	}
}

const bool CTerrain::IsBridge( const SVector &tile ) const
{
	if ( pAIMap->IsTileInside( tile.x, tile.y ) )
		return bridgeTiles.GetData( tile.x, tile.y );
	else
		return false;
}

void CTerrain::AddBridgeTile( const SVector &tile )
{
	if ( pAIMap->IsTileInside( tile.x, tile.y ) )
	{
		bridgeTiles.SetData( tile.x, tile.y );
	}
}

void CTerrain::RemoveBridgeTile( const SVector &tile )
{
	if ( pAIMap->IsTileInside( tile.x, tile.y ) )
		bridgeTiles.RemoveData( tile.x, tile.y );
}

bool CTerrain::HasRiverCliffTiles( const SVector &tile, const int nBoundTileRadius ) const
{
	if ( riverCliffTiles.IsEmpty() )
		return false;

	const int nMinX = (std::max)( 0, tile.x - nBoundTileRadius );
	const int nMinY = (std::max)( 0, tile.y - nBoundTileRadius );
	const int nMaxX = (std::min)( pAIMap->GetSizeX() - 1, tile.x + nBoundTileRadius );
	const int nMaxY = (std::min)( pAIMap->GetSizeY() - 1, tile.y + nBoundTileRadius );

	for ( int y = nMinY; y <= nMaxY; ++y )
	{
		for ( int x = nMinX; x <= nMaxX; ++x )
		{
			const SVector checkTile( x, y );
			if ( riverCliffTiles.GetData( x, y ) && !IsBridge( checkTile ) )
				return true;
		}
	}

	return false;
}

bool CTerrain::IsStaticLocked( const int x, const int y, const EAIClasses aiClass ) const
{ 
	return ( !pAIMap->IsTileInside( x, y ) || ( (buf[y][x] & aiClass) == aiClass ) );
}

bool CTerrain::IsLocked( const int x, const int y, const EAIClasses aiClass ) const 
{ 
	return IsStaticLocked( x, y, aiClass ) || 
		( ( eMode == ELM_ALL ) && ( ( unitsBuf[0][y][x] > 0 && ( aiClass & EAC_TERRAIN ) ) || ( unitsBuf[1][y][x] > 0 && ( aiClass & EAC_WATER ) ) ) );
}

bool CTerrain::IsStaticLockedWOBoundaryCheck( const int x, const int y, const EAIClasses aiClass ) const
{ 
	return (buf[y][x] & aiClass) == aiClass;
}

bool CTerrain::IsLocked4ClassWOBoundaryCheck( const int x, const int y, const EAIClasses aiClass ) const
{ 
	if ( IsStaticLockedWOBoundaryCheck( x, y, aiClass ) )
		return true;
	if ( eMode == ELM_ALL )
		return ( ( unitsBuf[0][y][x] > 0 ) && ( ( aiClass & EAC_TERRAIN ) == EAC_TERRAIN ) ) || ( ( unitsBuf[1][y][x] > 0 ) && ( ( aiClass & EAC_WATER ) == EAC_WATER ) );
	else
		return false;
}

bool CTerrain::IsLocked4Class( const int x, const int y, const EAIClasses aiClass ) const
{ 
	if ( IsStaticLocked( x, y, aiClass ) )
		return true;
	if ( eMode == ELM_ALL )
		return ( ( unitsBuf[0][y][x] > 0 ) && ( ( aiClass & EAC_TERRAIN ) == EAC_TERRAIN ) ) || ( ( unitsBuf[1][y][x] > 0 ) && ( ( aiClass & EAC_WATER ) == EAC_WATER ) );
	else
		return false;
}

EFreeTileInfo CTerrain::CanUnitGo( const int nBoundTileRadius, const SVector &tile, const EAIClasses aiClass ) const 	
{
	if ( !pAIMap->IsTileInside( tile.x, tile.y ) )
		return FREE_NONE;

	// River visual cliffs are only a problem for amphibious units trying to switch
	// between water and terrain; pure land and pure water classes keep normal rules.
	if ( ( aiClass & EAC_WATER ) && ( aiClass & EAC_TERRAIN ) && HasRiverCliffTiles( tile, nBoundTileRadius ) )
		return FREE_NONE;

	EFreeTileInfo eResult = FREE_NONE;
	if ( aiClass & EAC_WATER )
	{
		const int nClassIndexStatic = GetClassIndexFast( aiClass & EAC_WATER );
		const int nClassIndexDynamic = ( aiClass & EAC_TERRAIN ) ? GetClassIndexFast( EAC_ANY ) : GetClassIndexFast( EAC_WATER );

		if ( maxesSmooth[eMode][ nClassIndexStatic ].GetData( tile.x, tile.y ) >= nBoundTileRadius + 1 && 
			maxesSmooth[eMode][ nClassIndexDynamic ].GetData( tile.x, tile.y ) >= nBoundTileRadius + 1 )
		{
			eResult = (EFreeTileInfo)((int)eResult | (int)FREE_WATER);
		}
	}
	if ( aiClass & EAC_TERRAIN )
	{
		const int nClassIndexStatic = GetClassIndexFast( aiClass & EAC_TERRAIN );
		const int nClassIndexDynamic = ( aiClass & EAC_WATER ) ? GetClassIndexFast( EAC_ANY ) : GetClassIndexFast( EAC_TERRAIN );

		if ( maxesSmooth[eMode][ nClassIndexStatic ].GetData( tile.x, tile.y ) >= nBoundTileRadius + 1 && 
				maxesSmooth[eMode][ nClassIndexDynamic ].GetData( tile.x, tile.y ) >= nBoundTileRadius + 1 )
		{
			eResult = (EFreeTileInfo)((int)eResult | (int)FREE_TERRAIN);
		}
	}
	if ( eResult == FREE_NONE && ( aiClass & EAC_WATER ) && ( aiClass & EAC_TERRAIN ) )
	{
		const int nClassIndexAny = GetClassIndexFast( EAC_ANY );

		// Amphibious units can straddle land/water borders, where neither pure side
		// has enough clearance for the whole footprint.
		if ( maxesSmooth[eMode][ nClassIndexAny ].GetData( tile.x, tile.y ) >= nBoundTileRadius + 1 )
		{
			eResult = ( GetTerrainType( tile ) == ETT_WATER_TERRAIN ) ? FREE_WATER : FREE_TERRAIN;
		}
	}
	return eResult;
}

EFreeTileInfo CTerrain::CanUnitGoToPoint( const int nBoundTileRadius, const CVec2 &point, const EAIClasses aiClass, CAIMap *pAIMap ) const
{
	const SVector tileToGo( pAIMap->GetTile( point ) );
	return CanUnitGo( nBoundTileRadius, tileToGo, aiClass );
}

const CVec2 CTerrain::GetValidPoint( const int nBoundTileRadius, const CVec2 &vStart, const CVec2 &vEnd, const EAIClasses aiClass, const bool bFindWayBack, CAIMap *pAIMap ) const
{
	const SVector vStartTile( pAIMap->GetTile( vStart ) );
	const SVector vEndTile( pAIMap->GetTile( vEnd ) );
	// первый тайл свободный, ищем с начала
	if ( CanUnitGo( nBoundTileRadius, vStartTile, aiClass ) != FREE_NONE )
	{
		CVec2 vResult = vStart;
		CBres bres;
		bres.InitPoint( vStartTile, vEndTile );
		while ( bres.GetDirection() != vEndTile && CanUnitGo( nBoundTileRadius, bres.GetDirection(), aiClass ) != FREE_NONE )
		{
			vResult = pAIMap->GetPointByTile( bres.GetDirection() );
			bres.MakePointStep();
		}
		if ( bres.GetDirection() == vEndTile && CanUnitGo( nBoundTileRadius, bres.GetDirection(), aiClass ) != FREE_NONE ) 
			return vEnd;
		return vResult;
	}
	// первый тайл занят, ищем первый подходящий тайл с конца
	else if ( bFindWayBack )
	{
		CBres bres;
		bres.InitPoint( vEndTile, vStartTile );
		while ( bres.GetDirection() != vStartTile && CanUnitGo( nBoundTileRadius, bres.GetDirection(), aiClass ) == FREE_NONE )
			bres.MakePointStep();
		if ( bres.GetDirection() == vEndTile ) 
			return vEnd;
		else if ( bres.GetDirection() == vStartTile )
			return vStart;
		return pAIMap->GetPointByTile( bres.GetDirection() );
	}
	return vStart;
}

uint8_t CTerrain::GetSoilType( const SVector &tile ) const
{ 
	const int nX = Clamp( tile.x, 0, soil.GetSizeX() - 1 );
	const int nY = Clamp( tile.y, 0, soil.GetSizeY() - 1 );
	return soil[nY][nX];
}

const float CTerrain::GetPass( const int nX, const int nY ) const
{
	if ( !pAIMap->IsPointInside( nX, nY ) )
		return 1.0f;
	else
	{
		SVector visTile = pAIMap->GetTile( nX, nY );
		visTile.x >>= 1;
		visTile.y >>= 1;
		//CRAP{ мостовые проблемы
		const float fResult = passabilities[ passTypes[visTile.y][visTile.x] ];
		return ( fResult == 0.0f ) ? 1.0f : fResult;
		//CRAP}
	}
}

// залокать от статического объекта
void CTerrain::LockTile( const int x, const int y, const EAIClasses aiClasses )
{
	//const EAIClasses aiClass = buf[y][x];
	buf[y][x] = (EAIClasses)( (uint8_t)buf[y][x] | (uint8_t)aiClasses );
	/*
	if ( x == 113 && y == 167 )
		DebugTrace( "CTerrain::LockTile( %d, %d, 0x%04x ) (0x%04x -> 0x%04x)", x, y, aiClasses, aiClass, buf[y][x] );
	*/
}

// разлокать от статич. объекта
void CTerrain::UnlockTile( const int x, const int y, const EAIClasses aiClasses )
{ 
	//const EAIClasses aiClass = buf[y][x];
	buf[y][x] = (EAIClasses)( (uint8_t)buf[y][x] & ~(uint8_t)aiClasses );
	/*
	if ( x == 71 && y == 83 )
		DebugTrace( "CTerrain::UnlockTile( %d, %d, 0x%04x ) (0x%04x -> 0x%04x)", x, y, aiClasses, aiClass, buf[y][x] );
	*/
}

void IncreaseUpdateRect( int *pDownX, int *pUpX, int *pDownY, int *pUpY, CAIMap *pAIMap )
{
	*pDownX = (std::max)( *pDownX - pAIMap->GetMaxUnitTileRadius() - 1, 0 );
	*pDownY = (std::max)( *pDownY - pAIMap->GetMaxUnitTileRadius() - 1, 0 );
	*pUpX = (std::min)( *pUpX + pAIMap->GetMaxUnitTileRadius() + 1, pAIMap->GetSizeX() - 1 );
	*pUpY = (std::min)( *pUpY + pAIMap->GetMaxUnitTileRadius() + 1, pAIMap->GetSizeY() - 1 );
}

void CTerrain::UpdateMaxesForRemovedTiles( int downX, int upX, int downY, int upY, const EAIClasses aiClass )
{
	if ( bInitMode )
		return;

	const int nClassIndex = GetClassIndex( aiClass );
	if ( nClassIndex < 0 )
		return;

	IncreaseUpdateRect( &downX, &upX, &downY, &upY, pAIMap );

	const int nSizeX = pAIMap->GetSizeX();
	const int nSizeY = pAIMap->GetSizeY();
	const int nMaxUnitTileRadius = pAIMap->GetMaxUnitTileRadius();

	for ( int y = downY; y <= upY; ++y )
	{
		for ( int x = downX; x <= upX; ++x )
		{
			if ( IsLocked4ClassWOBoundaryCheck( x, y, aiClass ) )
				maxes[eMode][nClassIndex].SetData( x, y, 0 );
			else
			{
				//тайл на границе
				if ( x == 0 || y == 0 )
					maxes[eMode][nClassIndex].SetData( x, y, 1 );
				//тайл не залокан
				else if ( !IsLocked4ClassWOBoundaryCheck( x, y, aiClass ) )
				{
					const uint8_t mxy_1 = maxes[eMode][nClassIndex].GetData( x, y-1 );

					if ( maxes[eMode][nClassIndex].GetData( x-1, y ) < mxy_1 && maxes[eMode][nClassIndex].GetData( x, y ) < mxy_1 )
					{
						//локаем тайлы ближе к границе
						if ( y + mxy_1 - 1 >= nSizeY )
							maxes[eMode][nClassIndex].SetData( x, y, mxy_1 - 1 );
						else
						{
							maxes[eMode][nClassIndex].SetData( x, y, mxy_1 );
							for ( int xx = x - mxy_1 + 1; xx <= x + mxy_1 - 1; ++xx )
							{
								if ( IsLocked4ClassWOBoundaryCheck( xx, y + mxy_1 - 1, aiClass ) )
								{
									maxes[eMode][nClassIndex].SetData( x, y, mxy_1 - 1 );
									break;
								}
							}
						}
					}
					else if ( maxes[eMode][nClassIndex].GetData( x-1, y ) == maxes[eMode][nClassIndex].GetData( x  ,y-1 ) &&
						maxes[eMode][nClassIndex].GetData( x  , y ) < maxes[eMode][nClassIndex].GetData( x-1, y ) + 1 )
					{
						const uint8_t mx_1y = maxes[eMode][nClassIndex].GetData( x-1, y );

						if ( mx_1y == 0 )
							maxes[eMode][nClassIndex].SetData( x, y, 1 );
						else if ( IsLocked4ClassWOBoundaryCheck( x + mx_1y - 1, y + mx_1y - 1, aiClass ) )
							maxes[eMode][nClassIndex].SetData( x, y, mx_1y - 1 );
						else if ( x + mx_1y >= nSizeX || y + mx_1y >= nSizeY || 
							IsLocked4ClassWOBoundaryCheck( x - mx_1y, y - mx_1y, aiClass ) || mx_1y > nMaxUnitTileRadius )
							maxes[eMode][nClassIndex].SetData( x, y, mx_1y );
						else
						{
							maxes[eMode][nClassIndex].SetData( x, y, mx_1y + 1 );
							for ( int xx = x - mx_1y; xx <= x + mx_1y; ++xx )
							{
								if ( IsLocked4ClassWOBoundaryCheck( xx, y + mx_1y, aiClass ) )
								{
									maxes[eMode][nClassIndex].SetData( x, y, mx_1y );
									break;
								}
							}

							if ( maxes[eMode][nClassIndex].GetData( x, y ) == mx_1y + 1 )
							{
								for ( int yy = y - mx_1y; yy <= y + mx_1y; ++yy )
								{
									if ( IsLocked4ClassWOBoundaryCheck( x + mx_1y, yy, aiClass ) )
									{
										maxes[eMode][nClassIndex].SetData( x, y, mx_1y );
										break;
									}
								}
							}
						}
					}
					else if ( maxes[eMode][nClassIndex].GetData( x, y ) < maxes[eMode][nClassIndex].GetData( x-1, y ) )
					{
						const uint8_t mx_1y = maxes[eMode][nClassIndex].GetData( x-1, y );

						if ( x + mx_1y - 1 >= nSizeX )
							maxes[eMode][nClassIndex].SetData( x, y, mx_1y - 1 );
						else
						{
							maxes[eMode][nClassIndex].SetData( x, y, mx_1y );
							for ( int yy = y - mx_1y + 1; yy <= y + mx_1y - 1; ++yy )
							{
								if ( IsLocked4ClassWOBoundaryCheck( x + mx_1y - 1, yy, aiClass ) )
								{
									maxes[eMode][nClassIndex].SetData( x, y, mx_1y - 1 );
									break;
								}
							}
						}
					}
				}
			}
		}
	}
	SmoothLock( downX, downY, upX, upY, aiClass );
}

void CTerrain::UpdateMaxesForAddedTiles( int downX, int upX, int downY, int upY, const EAIClasses aiClass )
{
	if ( bInitMode )
		return;

	const int nClassIndex = GetClassIndex( aiClass );
	if ( nClassIndex < 0 )
		return;

	IncreaseUpdateRect( &downX, &upX, &downY, &upY, pAIMap );

	const int nSizeX = pAIMap->GetSizeX();
	const int nSizeY = pAIMap->GetSizeY();
	const int nMaxUnitTileRadius = pAIMap->GetMaxUnitTileRadius();

	for ( int y = downY; y <= upY; ++y )
	{
		for ( int x = downX; x <= upX; ++x )
		{
			if ( IsLocked4ClassWOBoundaryCheck( x, y, aiClass ) )
				maxes[eMode][nClassIndex].SetData( x, y, 0 );
			else
			{
				if ( x == 0 || y == 0 )
					maxes[eMode][nClassIndex].SetData( x, y, 1 );
				else
				{
					if ( maxes[eMode][nClassIndex].GetData( x-1, y ) < maxes[eMode][nClassIndex].GetData( x, y-1 ) )
					{
						const uint8_t mxy_1 = maxes[eMode][nClassIndex].GetData( x, y-1 );

						if ( y + mxy_1 - 1 >= nSizeY )
							maxes[eMode][nClassIndex].SetData( x, y, mxy_1 - 1 );
						else
						{
							maxes[eMode][nClassIndex].SetData( x, y, mxy_1 );
							for ( int xx = x - mxy_1 + 1; xx <= x + mxy_1 - 1; ++xx )
							{
								if ( IsLocked4ClassWOBoundaryCheck( xx, y + mxy_1 - 1, aiClass ) )
								{
									maxes[eMode][nClassIndex].SetData( x, y, mxy_1 - 1 );
									break;
								}
							}
						}
					}

					else if ( maxes[eMode][nClassIndex].GetData( x-1, y ) == maxes[eMode][nClassIndex].GetData( x, y-1 ) )
					{
						const uint8_t mx_1y = maxes[eMode][nClassIndex].GetData( x-1, y );

						if ( mx_1y == 0 )
							maxes[eMode][nClassIndex].SetData( x, y, 1 );
						else if ( IsLocked4ClassWOBoundaryCheck( x + mx_1y - 1, y + mx_1y - 1, aiClass ) )
							maxes[eMode][nClassIndex].SetData( x, y, mx_1y - 1 );
						else if ( x + mx_1y >= nSizeX || y + mx_1y >= nSizeY || 
							IsLocked4ClassWOBoundaryCheck( x - mx_1y, y - mx_1y, aiClass ) || mx_1y > nMaxUnitTileRadius )
							maxes[eMode][nClassIndex].SetData( x, y, mx_1y );
						else
						{
							maxes[eMode][nClassIndex].SetData( x, y, mx_1y + 1 );
							for ( int xx = x - mx_1y; xx <= x + mx_1y; ++xx )
							{
								if ( IsLocked4ClassWOBoundaryCheck( xx, y + mx_1y, aiClass ) )
								{
									maxes[eMode][nClassIndex].SetData( x, y, mx_1y );
									break;
								}
							}

							if ( maxes[eMode][nClassIndex].GetData( x, y ) == mx_1y + 1 )
							{
								for ( int yy = y - mx_1y; yy <= y + mx_1y; ++yy )
								{
									if ( IsLocked4ClassWOBoundaryCheck( x + mx_1y, yy, aiClass ) )
									{
										maxes[eMode][nClassIndex].SetData( x, y, mx_1y );
										break;
									}
								}
							}
						}
					}
					else
					{
						const uint8_t mx_1y = maxes[eMode][nClassIndex].GetData( x-1, y );

						if ( x + mx_1y - 1 >= nSizeX )
							maxes[eMode][nClassIndex].SetData( x, y, mx_1y - 1 );
						else
						{
							maxes[eMode][nClassIndex].SetData( x, y, mx_1y );
							for ( int yy = y - mx_1y + 1; yy <= y + mx_1y - 1; ++yy )
							{
								if ( IsLocked4ClassWOBoundaryCheck( x + mx_1y - 1, yy, aiClass ) )
								{
									maxes[eMode][nClassIndex].SetData( x, y, mx_1y - 1 );
									break;
								}
							}
						}
					}
				}
			}
		}
	}
	SmoothLock( downX, downY, upX, upY, aiClass );
}

bool IsLockAround( const CArray2D4Bit &thisMaxes, const int x, const int y, const uint8_t bValue )
{
	int nCount = 0;
	if ( thisMaxes.GetData( x - 1, y ) < bValue )
		++nCount;
	if ( thisMaxes.GetData( x + 1, y ) < bValue )
		++nCount;
	if ( thisMaxes.GetData( x, y - 1 ) < bValue )
		++nCount;
	if ( thisMaxes.GetData( x, y + 1 ) < bValue )
		++nCount;
	return nCount >= 3;
}

bool IsFreeAround( const CArray2D4Bit &thisMaxes, const int x, const int y, const uint8_t bValue )
{
	int nCount = 0;
	if ( thisMaxes.GetData( x - 1, y ) > bValue )
		++nCount;
	if ( thisMaxes.GetData( x + 1, y ) > bValue )
		++nCount;
	if ( thisMaxes.GetData( x, y - 1 ) > bValue )
		++nCount;
	if ( thisMaxes.GetData( x, y + 1 ) > bValue )
		++nCount;
	return nCount >= 4;
}

void CTerrain::SmoothLock( const int _xMin, const int _yMin, const int _xMax, const int _yMax, const EAIClasses aiClass )
{
	const int nAIClassIndex = GetClassIndexFast( aiClass );
	CArray2D4Bit &thisMaxes = maxes[eMode][nAIClassIndex];
	CArray2D4Bit &thisMaxesSmooth = maxesSmooth[eMode][nAIClassIndex];
	std::list<SVector> tiles;

	for ( int y = _yMin; y < _yMax; ++y )
		for ( int x = _xMin; x < _xMax; ++x )
			thisMaxesSmooth.SetData( x, y, thisMaxes.GetData( x, y ) );

	const int xMin = (std::max)( _xMin, 1 );
	const int yMin = (std::max)( _yMin, 1 );
	const int xMax = (std::min)( _xMax, pAIMap->GetSizeX() - 2 );
	const int yMax = (std::min)( _yMax, pAIMap->GetSizeY() - 2 );
	
	// избавляемся от "левых" залоканных мест
  for ( int y = yMin; y <= yMax; ++y )
		for ( int x = xMin; x <= xMax; ++x )
		{
			const uint8_t bValue = thisMaxesSmooth.GetData( x, y );
			if ( IsFreeAround( thisMaxesSmooth, x, y, bValue ) )  
			{
				tiles.push_back( SVector( x, y ) );
				maskForSmooth.SetData( x, y );
			}
		}
	
	while ( !tiles.empty() )
	{
		const SVector tile = *(tiles.begin());
		tiles.pop_front();
		const uint8_t bValue = thisMaxesSmooth.GetData( tile.x, tile.y );
		thisMaxesSmooth.SetData( tile.x, tile.y, bValue + 1 );
		for ( int i = 0; i < 4; ++i )  
		{
			const int xx = tile.x + offsets[i].x;
			const int yy = tile.y + offsets[i].y;
			if ( maskForSmooth.GetData( xx, yy ) == 0 && thisMaxesSmooth.GetData( xx, yy ) == bValue && IsFreeAround( thisMaxesSmooth, xx, yy, bValue ) )  
			{
				tiles.push_back( SVector( xx, yy ) );
				maskForSmooth.SetData( xx, yy );
			}
		}
		maskForSmooth.RemoveData( tile.x, tile.y );
	}

	// избавляемся от "левых" разлоканых мест
	for ( int y = yMin; y <= yMax; ++y )
		for ( int x = xMin; x <= xMax; ++x )
		{
			const uint8_t bValue = thisMaxesSmooth.GetData( x, y );
			if ( IsLockAround( thisMaxesSmooth, x, y, bValue ) )  
			{
				tiles.push_back( SVector( x, y ) );
				maskForSmooth.SetData( x, y );
			}
		}

	while ( !tiles.empty() )
	{
		const SVector tile = *(tiles.begin());
		tiles.pop_front();
		const uint8_t bValue = thisMaxesSmooth.GetData( tile.x, tile.y );
		thisMaxesSmooth.SetData( tile.x, tile.y, bValue - 1 );
		for ( int i = 0; i < 4; ++i )  
		{
			const int xx = tile.x + offsets[i].x;
			const int yy = tile.y + offsets[i].y;
			if ( maskForSmooth.GetData( xx, yy ) == 0 && thisMaxesSmooth.GetData( xx, yy ) == bValue && IsLockAround( thisMaxesSmooth, xx, yy, bValue ) )  
			{
				tiles.push_back( SVector( xx, yy ) );
				maskForSmooth.SetData( xx, yy );
			}
		}
		maskForSmooth.RemoveData( tile.x, tile.y );
	}
}

void CTerrain::SmoothLock()
{
	{
		STerrainModeSetter modeSetter( ELM_STATIC, this );
		SmoothLock( 0, 0, pAIMap->GetSizeX(), pAIMap->GetSizeY(), EAC_HUMAN );
		SmoothLock( 0, 0, pAIMap->GetSizeX(), pAIMap->GetSizeY(), EAC_WHELL );
		SmoothLock( 0, 0, pAIMap->GetSizeX(), pAIMap->GetSizeY(), EAC_TRACK );
		SmoothLock( 0, 0, pAIMap->GetSizeX(), pAIMap->GetSizeY(), EAC_SEA );
		SmoothLock( 0, 0, pAIMap->GetSizeX(), pAIMap->GetSizeY(), EAC_RIVER );
		SmoothLock( 0, 0, pAIMap->GetSizeX(), pAIMap->GetSizeY(), EAC_TERRAIN );
		SmoothLock( 0, 0, pAIMap->GetSizeX(), pAIMap->GetSizeY(), EAC_WATER );
	}
	{
		STerrainModeSetter modeSetter( ELM_ALL, this );
		SmoothLock( 0, 0, pAIMap->GetSizeX(), pAIMap->GetSizeY(), EAC_HUMAN );
		SmoothLock( 0, 0, pAIMap->GetSizeX(), pAIMap->GetSizeY(), EAC_WHELL );
		SmoothLock( 0, 0, pAIMap->GetSizeX(), pAIMap->GetSizeY(), EAC_TRACK );
		SmoothLock( 0, 0, pAIMap->GetSizeX(), pAIMap->GetSizeY(), EAC_SEA );
		SmoothLock( 0, 0, pAIMap->GetSizeX(), pAIMap->GetSizeY(), EAC_RIVER );
		SmoothLock( 0, 0, pAIMap->GetSizeX(), pAIMap->GetSizeY(), EAC_TERRAIN );
		SmoothLock( 0, 0, pAIMap->GetSizeX(), pAIMap->GetSizeY(), EAC_WATER );
	}
}

void CTerrain::UpdateMaxesForAddedStObject( const SVector &downTile, const SVector &upTile )
{
	const int downX = downTile.x;
	const int downY = downTile.y;
	const int upX = upTile.x;
	const int upY = upTile.y;

	const ELockMode oldMode = eMode;

	eMode = ELM_STATIC;
	UpdateMaxesForAddedTiles( downX, upX, downY, upY, EAC_TERRAIN );
	UpdateMaxesForAddedTiles( downX, upX, downY, upY, EAC_WATER );
	UpdateMaxesForAddedTiles( downX, upX, downY, upY, EAC_ANY );
	for ( int index = 1, count = 0; count < EAC_COUNT; index *= 2, ++count )
		UpdateMaxesForAddedTiles( downX, upX, downY, upY, (EAIClasses)index );

	eMode = ELM_ALL;
	UpdateMaxesForAddedTiles( downX, upX, downY, upY, EAC_TERRAIN );
	UpdateMaxesForAddedTiles( downX, upX, downY, upY, EAC_WATER );
	UpdateMaxesForAddedTiles( downX, upX, downY, upY, EAC_ANY );
	for ( int index = 1, count = 0; count < EAC_COUNT; index *= 2, ++count )
		UpdateMaxesForAddedTiles( downX, upX, downY, upY, (EAIClasses)index );

	eMode = oldMode;
}

void CTerrain::UpdateMaxesForRemovedStObject( const SVector &downTile, const SVector &upTile )
{
	const int downX = downTile.x;
	const int downY = downTile.y;
	const int upX = upTile.x;
	const int upY = upTile.y;

	const ELockMode oldMode = eMode;

	eMode = ELM_STATIC;
	UpdateMaxesForRemovedTiles( downX, upX, downY, upY, EAC_TERRAIN );
	UpdateMaxesForRemovedTiles( downX, upX, downY, upY, EAC_WATER );
	UpdateMaxesForRemovedTiles( downX, upX, downY, upY, EAC_ANY );
	for ( int index = 1, count = 0; count < EAC_COUNT; index *= 2, ++count )
		UpdateMaxesForRemovedTiles( downX, upX, downY, upY, (EAIClasses)index );

	eMode = ELM_ALL;
	UpdateMaxesForRemovedTiles( downX, upX, downY, upY, EAC_TERRAIN );
	UpdateMaxesForRemovedTiles( downX, upX, downY, upY, EAC_WATER );
	UpdateMaxesForRemovedTiles( downX, upX, downY, upY, EAC_ANY );
	for ( int index = 1, count = 0; count < EAC_COUNT; index *= 2, ++count )
		UpdateMaxesForRemovedTiles( downX, upX, downY, upY, (EAIClasses)index );

	eMode = oldMode;
}

void CTerrain::AddStaticObjectTiles( const std::list<SObjTileInfo> &tiles )
{
	SVector downTile, upTile;
	LockTiles( tiles, &downTile, &upTile );

	UpdateMaxesForAddedStObject( downTile, upTile );
}

void CTerrain::RemoveStaticObjectTiles( const std::list<SObjTileInfo> &tiles )
{
	SVector downTile, upTile;
	UnlockTiles( tiles, &downTile, &upTile );
	UpdateTerrainPassabilityRect( downTile.x, downTile.y, upTile.x, upTile.y, false );

	UpdateMaxesForRemovedStObject( downTile, upTile );
}

void CTerrain::RemoveStaticObjectTilesForBridge( const std::list<SObjTileInfo> &tiles )
{
	SVector downTile, upTile;
	UnlockTiles( tiles, &downTile, &upTile );

	UpdateMaxesForRemovedStObject( downTile, upTile );
}

void CTerrain::AddUnitTiles( const int id, const SRect &rect, const bool bWater )
{
//	DebugTrace( "CTerrain::AddUnitTiles( %d, ... )", id );

	SVector downTile, upTile;
	LockUnitProfile( SUnitProfile( rect ), id, &downTile, &upTile, bWater );

	UpdateMaxesForAddedTiles( downTile.x, upTile.x, downTile.y, upTile.y, EAC_TERRAIN );
	UpdateMaxesForAddedTiles( downTile.x, upTile.x, downTile.y, upTile.y, EAC_WATER );
	UpdateMaxesForAddedTiles( downTile.x, upTile.x, downTile.y, upTile.y, EAC_ANY );
}

#ifndef _FINALRELEASE
void CTerrain::AddLockInfo( const int nUnitID, const bool bLock, const SUnitProfile &profile )
{
	debugLockInfo[nDebugLockInfoPos].nCount = nDebugLockInfoCount;
	debugLockInfo[nDebugLockInfoPos].nUnitID = nUnitID;
	debugLockInfo[nDebugLockInfoPos].bLock = bLock;
	debugLockInfo[nDebugLockInfoPos].profile = profile;

	++nDebugLockInfoCount;
	++nDebugLockInfoPos;
	if ( nDebugLockInfoPos == debugLockInfo.size() )
		nDebugLockInfoPos = 0;
}

#endif

void CTerrain::DumpLockInfo() const
{
#ifndef _FINALRELEASE
	for ( std::vector<STerrainLockInfo>::const_iterator it = debugLockInfo.begin(); it != debugLockInfo.end(); ++it )
	{
		if ( it->bLock )
		{
			if ( it->profile.bRect )
				DebugTrace( "[CTerrain] %d\t%d\tlock\trect: %2.3f x %2.3f, %2.3f x %2.3f, %2.3f, %2.3f, %2.3f", it->nCount, it->nUnitID, it->profile.rect.center.x, it->profile.rect.center.y, it->profile.rect.dir.x, it->profile.rect.dir.y, it->profile.rect.lengthAhead, it->profile.rect.lengthBack, it->profile.rect.width );
			else
				DebugTrace( "[CTerrain] %d\t%d\tlock\tcircle: %2.3f x %2.3f, %2.3f", it->nCount, it->nUnitID, it->profile.circle.center.x, it->profile.circle.center.y, it->profile.circle.r );
		}
		else
			DebugTrace( "[CTerrain] %d\t%d\tunlock", it->nCount, it->nUnitID );
	}
#else
	DebugTrace( "[CTerrain] DumpLockInfo not works in FINAL RELEASE" );
#endif
}

void CTerrain::AddUnitTiles( const int id, const SUnitProfile &profile, const bool bWater )
{
//	DebugTrace( "CTerrain::AddUnitTiles( %d, ... )", id );
#ifndef _FINALRELEASE
	AddLockInfo( id, true, profile );
#endif

	SVector downTile, upTile;
	LockUnitProfile( profile, id, &downTile, &upTile, bWater );

	UpdateMaxesForAddedTiles( downTile.x, upTile.x, downTile.y, upTile.y, EAC_TERRAIN );
	UpdateMaxesForAddedTiles( downTile.x, upTile.x, downTile.y, upTile.y, EAC_WATER );
	UpdateMaxesForAddedTiles( downTile.x, upTile.x, downTile.y, upTile.y, EAC_ANY );
}

void CTerrain::RemoveUnitTiles( const int id )
{
//	DebugTrace( "CTerrain::RemoveUnitTiles( %d, ... )", id );
#ifndef _FINALRELEASE
	AddLockInfo( id, false, SUnitProfile() );
#endif

	SVector downTile, upTile;
	bool bWater;
	if ( UnlockUnitProfile( id, &downTile, &upTile, &bWater ) )
	{
		UpdateMaxesForRemovedTiles( downTile.x, upTile.x, downTile.y, upTile.y, EAC_TERRAIN );
		UpdateMaxesForRemovedTiles( downTile.x, upTile.x, downTile.y, upTile.y, EAC_WATER );
		UpdateMaxesForRemovedTiles( downTile.x, upTile.x, downTile.y, upTile.y, EAC_ANY );
	}
}

void CTerrain::DumpStaticLock( const EAIClasses aiClass, const std::string &szFileName )
{
	CArray2D<uint32_t> image;
	image.SetSizes( pAIMap->GetSizeX(), pAIMap->GetSizeY() );
	image.FillZero();
	for ( int x = 0;  x < image.GetSizeX(); ++x )
		for ( int y = 0; y < image.GetSizeY(); ++y )
		{
			if ( IsStaticLocked( x, y, aiClass ) )
				image[y][x] = NImage::SColor( 255, 255, 0, 0 );
			else if ( IsLocked4ClassWOBoundaryCheck( x, y, aiClass	) )
				image[y][x] = NImage::SColor( 255, 255, 128, 0 );
			else if ( IsLocked( x, y, aiClass	) )
				image[y][x] = NImage::SColor( 255, 255, 255, 0 );
		}

	CFileStream stream( szFileName, CFileStream::WIN_CREATE );
	NImage::SaveImageAsTGA( &stream, image );
}

void CTerrain::DumpMaxes( const ELockMode lockMode, const EAIClasses aiClass, const std::string &szFileName )
{
	std::vector<SVector> tiles;
	DumpMaxes( lockMode, aiClass, szFileName, tiles );
}

void CTerrain::DumpMaxes( const ELockMode lockMode, const EAIClasses aiClass, const std::string &szFileName, const std::vector<SVector> &markTiles )
{
	std::vector<SVector> tiles;
	DumpMaxes( lockMode, aiClass, szFileName, markTiles, tiles, false );
}

void ShowMarkedTiles( CArray2D<uint32_t> *pImage, const int nCellSize, std::vector<SVector> tiles, const NImage::SColor color )
{
	if ( tiles.empty() )
		return;
	for ( int i = 1; i < tiles.size(); ++i )
	{
		const int x0 = tiles[i].x*nCellSize;
		const int y0 = tiles[i].y*nCellSize;
		for ( int n = 1; n < nCellSize; ++n )
		{
			(*pImage)[y0+n][x0+n] = color;
			(*pImage)[y0+nCellSize-n][x0+n] = color;
		}
	}
}

void ShowLinkedTiles( CArray2D<uint32_t> *pImage, const int nCellSize, std::vector<SVector> tiles, const NImage::SColor color )
{
	if ( tiles.empty() )
		return;
	if ( tiles.size() == 1 )
		ShowMarkedTiles(pImage, nCellSize, tiles, color );
	for ( int i = 1; i < tiles.size(); ++i )
	{
		const SVector vStart( tiles[i-1].x*nCellSize + nCellSize/2, tiles[i-1].y*nCellSize + nCellSize/2 );
		const SVector vFinish( tiles[i].x*nCellSize + nCellSize/2, tiles[i].y*nCellSize + nCellSize/2 );

		CBres bres;
		bres.InitPoint( vStart, vFinish );
		while ( bres.GetDirection() != vFinish )
		{
			(*pImage)[bres.GetDirection().y][bres.GetDirection().x] = color;
			bres.MakePointStep();
		}
	}
}

void CTerrain::DumpMaxes( const ELockMode lockMode, const EAIClasses aiClass, const std::string &szFileName, const std::vector<SVector> &tiles1, const std::vector<SVector> &tiles2, const bool bLink )
{
	const int IMAGE_CELL_SIZE = 10;
	CArray2D<uint32_t> image;
	image.SetSizes( IMAGE_CELL_SIZE*pAIMap->GetSizeX(), IMAGE_CELL_SIZE*pAIMap->GetSizeY() );
	image.FillZero();
	const int nIndex = GetClassIndex( aiClass );
	if ( nIndex < 0 )
		return;
	for ( int x = 0;  x < pAIMap->GetSizeX(); ++x )
		for ( int y = 0; y < pAIMap->GetSizeY(); ++y )
		{
			const uint8_t value = maxesSmooth[lockMode][nIndex].GetData( x, y );
			NImage::SColor color = NImage::SColor( 0, 0, 0, 0 );
			if ( value == 1 )
				color = NImage::SColor( 255, 255, 0, 0 );
			else if ( value == 2 )
				color = NImage::SColor( 255, 255, 128, 0 );
			else if ( value == 3 )
				color = NImage::SColor( 255, 255, 255, 0 );
			else if ( value == pAIMap->GetMaxUnitTileRadius() )
				color = NImage::SColor( 255, 0, 255, 0 );
			else if ( value == pAIMap->GetMaxUnitTileRadius() + 1 )
				color = NImage::SColor( 255, 255, 255, 255 );
			else if ( value > 0 )
				color = NImage::SColor( 255, 120+value*10, 120+value*10, 0 );
			const int x0 = x*IMAGE_CELL_SIZE;
			const int y0 = y*IMAGE_CELL_SIZE;
			for ( int i = 0; i < IMAGE_CELL_SIZE; ++i )
				for ( int n = 0; n < IMAGE_CELL_SIZE; ++n )
					image[y0+n][x0+i] = color;
		}

		for ( int x = 1; x < pAIMap->GetSizeX(); ++x )
		{
			const int x0 = x*IMAGE_CELL_SIZE;
			if ( x%10 == 0 )
			{
				for ( int y = 0; y < image.GetSizeY(); ++y )
					image[y][x0] = NImage::SColor( 255, 0, 0, 128 );
			}
			else
			{
				for ( int y = 0; y < image.GetSizeY(); ++y )
					if ( y%2 == 0 )
						image[y][x0] = NImage::SColor( 255, 0, 0, 128 );
			}
		}

		for ( int y = 1; y < pAIMap->GetSizeY(); ++y )
		{
			const int y0 = y*IMAGE_CELL_SIZE;
			if ( y%10 == 0 )
			{
				for ( int x = 0; x < image.GetSizeX(); ++x )
					image[y0][x] = NImage::SColor( 255, 0, 0, 128 );
			}
			else
			{
				for ( int x = 0; x < image.GetSizeX(); ++x )
					if ( x%2 == 0 )
						image[y0][x] = NImage::SColor( 255, 0, 0, 128 );
			}
		}

		if ( bLink )
		{
			ShowLinkedTiles( &image, IMAGE_CELL_SIZE, tiles1, NImage::SColor( 255, 0, 0, 255 ) );
			ShowLinkedTiles( &image, IMAGE_CELL_SIZE, tiles2, NImage::SColor( 255, 255, 0, 255 ) );
		}
		else
		{
			ShowMarkedTiles( &image, IMAGE_CELL_SIZE, tiles1, NImage::SColor( 255, 0, 0, 255 ) );
			ShowMarkedTiles( &image, IMAGE_CELL_SIZE, tiles2, NImage::SColor( 255, 255, 0, 255 ) );
		}

		CFileStream imageStream( szFileName, CFileStream::WIN_CREATE );
		NImage::SaveImageAsTGA( &imageStream, image );
}

void CTerrain::DumpPassability( const std::string &szFileName )
{
	CArray2D<uint32_t> image;
	image.SetSizes( pAIMap->GetSizeX(), pAIMap->GetSizeY() );
	image.FillZero();
	for ( int x = 0;  x < image.GetSizeX(); ++x )
		for ( int y = 0; y < image.GetSizeY(); ++y )
		{
			const int value = 255*GetPass( x, y );
			image[y][x] = NImage::SColor( 255, value, value, value );
		}

	CFileStream imageStream( szFileName, CFileStream::WIN_CREATE );
	NImage::SaveImageAsTGA( &imageStream, image );
}

void CTerrain::DumpBridges( const std::string &szFileName )
{
	CArray2D<uint32_t> image;
	image.SetSizes( pAIMap->GetSizeX(), pAIMap->GetSizeY() );
	image.FillZero();
	for ( int x = 0;  x < image.GetSizeX(); ++x )
		for ( int y = 0; y < image.GetSizeY(); ++y )
		{
			if ( IsBridge( SVector( x, y ) ) )
				image[y][x] = NImage::SColor( 255, 255, 255, 255 );
		}

	CFileStream imageStream( szFileName, CFileStream::WIN_CREATE );
	NImage::SaveImageAsTGA( &imageStream, image );
}

void CTerrain::DumpUnitsBuf( const std::string &szFileName )
{
	CArray2D<uint32_t> image;
	image.SetSizes( pAIMap->GetSizeX(), pAIMap->GetSizeY() );
	image.FillZero();
	for ( int x = 0;  x < image.GetSizeX(); ++x )
		for ( int y = 0; y < image.GetSizeY(); ++y )
		{
			int green = 0, blue = 0;
			if ( unitsBuf[0][y][x] == 1 )
				green = 64;
			else if ( unitsBuf[0][y][x] == 2 )
				green = 128;
			else if ( unitsBuf[0][y][x] > 2 )
				green = 255;
			if ( unitsBuf[1][y][x] == 1 )
				blue = 64;
			else if ( unitsBuf[1][y][x] == 2 )
				blue = 128;
			else if ( unitsBuf[1][y][x] > 2 )
				blue = 255;

			image[y][x] = NImage::SColor( 255, 0, green, blue );
		}

		CFileStream imageStream( szFileName, CFileStream::WIN_CREATE );
		NImage::SaveImageAsTGA( &imageStream, image );
}

void CTerrain::FinishInitMode()
{
	bInitMode = false;

	const SVector downTile( 0, 0 );
	const SVector upTile( pAIMap->GetSizeX() - 1, pAIMap->GetSizeY() - 1 );

	UpdateMaxesForAddedStObject( downTile, upTile );
	SmoothLock();
}

EAIClasses CTerrain::GetTileLockInfo( const int x, const int y ) const
{
	if ( x < 0 || y < 0 || x >= pAIMap->GetSizeX() || y >= pAIMap->GetSizeY() )
		return EAC_NONE;
	else
		return (EAIClasses)buf[y][x];
}

void CTerrain::GetLockedTiles( std::vector<SVector> *pTiles, const EAIClasses aiClass, const EFreeTileInfo tileInfo )
{
	STerrainModeSetter modeSetter( ELM_ALL, this );
	pTiles->clear();
	for ( int x = 0; x < pAIMap->GetSizeX(); ++x )
	{
		for ( int y = 0; y < pAIMap->GetSizeY(); ++y )
		{
			const SVector tile( x, y );
			if ( CanUnitGo( 0, tile, aiClass ) == tileInfo )
				pTiles->push_back( tile );
		}
	}
}

int CTerrain::ShowUnitLock( const int nUnitID, const int nMarkerID ) const
{
	CUnitsRects::const_iterator pos = unitsRects.find( nUnitID );
	if ( pos != unitsRects.end() )
	{
		std::vector<SVector> tiles;
		if ( pos->second.profile.bRect )
		{
			const SRect &rect( pos->second.profile.rect );
			pAIMap->GetTilesCoveredByQuadrangle( rect.v1(), rect.v2(), rect.v3(), rect.v4(), &tiles );
		}
		else
		{
			const CCircle &circle( pos->second.profile.circle );
			pAIMap->GetTilesCoveredByCircle( circle.center, circle.r, &tiles );
		}
		return DebugInfoManager()->CreateMarker( nMarkerID, tiles, NDebugInfo::RED );
	}

	return -1;
}

int CTerrain::TemporaryLockUnitProfile( const SUnitProfile &profile, const EAIClasses aiClass )
{
	std::list<SObjTileInfo> tiles;
	if ( profile.bRect )
	{
		const SRect &rect( profile.rect );
		pAIMap->GetTilesCoveredByQuadrangle( rect.v1(), rect.v2(), rect.v3(), rect.v4(), &tiles );
	}
	else
	{
		const CCircle &circle( profile.circle );
		pAIMap->GetTilesCoveredByCircle( circle.center, circle.r, &tiles );
	}
	for ( std::list<SObjTileInfo>::iterator it = tiles.begin(); it != tiles.end(); ++it )
		it->lockInfo = aiClass;

	return TemporaryLockTiles( tiles );
}

int CTerrain::TemporaryLockTiles( std::list<SObjTileInfo> &tiles )
{
	CTmpLockInfoBuf2 tmpLockInfo;
	for ( std::list<SObjTileInfo>::iterator it = tiles.begin(); it != tiles.end(); ++it )
		tmpLockInfo.push_back( STmpLockInfo2( SVector( it->tile.x, it->tile.y ), buf[it->tile.y][it->tile.x] ) );

	AddStaticObjectTiles( tiles );

	nTmpLockUnitID++;
	tmpLockUnitsMap[nTmpLockUnitID] = tmpLockInfo;
	return nTmpLockUnitID;
}

void CTerrain::RemoveTemporaryLock( const int nLockID )
{
	std::unordered_map< int, CTmpLockInfoBuf2 >::iterator pos = tmpLockUnitsMap.find( nLockID );
	if ( pos != tmpLockUnitsMap.end() )
	{
		SVector downTile( 2 * pAIMap->GetMaxMapSize(), 2 * pAIMap->GetMaxMapSize() ), upTile( -10, -10 );
		for ( std::list<STmpLockInfo2>::iterator it = pos->second.begin(); it != pos->second.end(); ++it )
		{
			buf[it->tile.y][it->tile.x] = it->aiClass;
			downTile.x = (std::min)( downTile.x, it->tile.x );
			downTile.y = (std::min)( downTile.y, it->tile.y );
			upTile.x = (std::max)( upTile.x, it->tile.x );
			upTile.y = (std::max)( upTile.y, it->tile.y );

		}
		tmpLockUnitsMap.erase( pos );
		UpdateTerrainPassabilityRect( downTile.x, downTile.y, upTile.x, upTile.y, false );
		UpdateMaxesForRemovedStObject( downTile, upTile );
	}
}

void CTerrain::LockTile( const SObjTileInfo &tileInfo )
{
	LockTile( tileInfo.tile.x, tileInfo.tile.y, tileInfo.lockInfo );
}

void CTerrain::UnlockTile( const SObjTileInfo &tileInfo )
{
	UnlockTile( tileInfo.tile.x, tileInfo.tile.y, tileInfo.lockInfo );
}

//*******************************************************************
//*												CTemporaryUnitProfileUnlocker   					*
//*******************************************************************

CTemporaryUnitProfileUnlocker::CTemporaryUnitProfileUnlocker( const int nUnitID, const SUnitProfile &unitProfile, const int nDecrease, const bool bWater, CTerrain *_pTerrain )
{
	nID = nUnitID;
	pTerrain = _pTerrain;
	bLocking = pTerrain->TemporaryUnlockUnitProfile( nUnitID, unitProfile, nDecrease, bWater );
}

CTemporaryUnitProfileUnlocker::~CTemporaryUnitProfileUnlocker()
{
	if ( bLocking )
		pTerrain->RemoveTemporaryUnlocking( nID );
}

//*******************************************************************
//*												STerrainModeSetter												*
//*******************************************************************

STerrainModeSetter::STerrainModeSetter( const ELockMode &eMode, CTerrain *_pTerrain )
{
	pTerrain = _pTerrain;

	eMemMode = pTerrain->eMode;
	pTerrain->SetMode( eMode );
}

STerrainModeSetter::~STerrainModeSetter()
{
	pTerrain->SetMode( eMemMode );
}

/************************************************************************/
/* CTemporaryUnitProfileLocker                                          */
/************************************************************************/

CTemporaryUnitProfileLocker::CTemporaryUnitProfileLocker( const SUnitProfile &profile, const EAIClasses aiClass, CTerrain *_pTerrain )
{
	pTerrain = _pTerrain;
	nLockID = pTerrain->TemporaryLockUnitProfile( profile, aiClass );
}

CTemporaryUnitProfileLocker::~CTemporaryUnitProfileLocker()
{
	pTerrain->RemoveTemporaryLock( nLockID );
}

REGISTER_SAVELOAD_CLASS( COMMON_RTS_AI, 0x3015A481, CTerrain )

