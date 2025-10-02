#pragma once

#include "Common_RTS_AI/Terrain.h"
#include "StaticObjectRotation.h"




namespace NLock
{
struct SEntranceData
{
	CVec2 vPos;
	WORD wDir;
};

struct SStaticObjectLockInfo
{
	bool bNewLockingWay;
	CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > oldLock;
	const vector<SVector> *pNewLock;
	CArray2D<BYTE> &lockInfo;
	list<SObjTileInfo> &lockedTiles;

	SStaticObjectLockInfo ( CArray2D<BYTE> &_lockInfo, list<SObjTileInfo> &_lockedTiles )
		: lockInfo( _lockInfo ), lockedTiles( _lockedTiles ), bNewLockingWay( true ), pNewLock( 0 )
	{
	}
};

template <class TAccess>
void CreateStaticObjectLockedTilesInfo( list<SObjTileInfo> *pTiles, 
	vector<SEntranceData> &entrances, 
	EAIClasses ePassClass, 
	SStaticObjectLockInfo &lockInfo,
	TAccess & access )
{
	pTiles->clear();

	hash_set<SVector, STilesHash> entranceTiles;
	int nEntrances = entrances.size();
	for ( int i = 0; i < nEntrances; ++i )
	{
		const SEntranceData &entr = entrances[i];
		CVec2 vEntr = entr.vPos;
		CVec2 vEntrDir = GetVectorByDirection( entr.wDir ) * AI_TILE_SIZE * 0.5;
		SVector tile( access.GetTile( entr.vPos ) );
		for ( int j = 0; j < 10; ++j )
		{
			entranceTiles.insert( tile );
			vEntr += vEntrDir;
			tile = access.GetTile( vEntr );
		}
	}

	if ( lockInfo.bNewLockingWay == false )
	{
		CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > &pass = lockInfo.oldLock;

		lockInfo.lockInfo.SetSizes( (pass.GetMaxX() - pass.GetMinX() ) / SConsts::TILE_SIZE + 2, (pass.GetMaxY() - pass.GetMinY())/SConsts::TILE_SIZE +2 );
		lockInfo.lockInfo.FillZero();

		const SVector vStartTile( AICellsTiles::GetTile( pass.GetMinX(), pass.GetMinY() ) );
		for ( int x = pass.GetMinX(); x < pass.GetMaxX(); x += SConsts::TILE_SIZE )
		{
			for ( int y = pass.GetMinY(); y < pass.GetMaxY(); y += SConsts::TILE_SIZE )
			{
				const CVec2 vTileCenter( AICellsTiles::GetCenterOfTile( x, y ) );
				if ( pass.GetVal( vTileCenter ) )
				{
					const SVector tile( AICellsTiles::GetTile( x, y ) );				
					if ( !access.IsTileInside( tile ) )
						continue;

					const BYTE tileInfo = access.GetTileLockInfo( tile );
					BYTE lock = ~tileInfo & ePassClass & EAC_ANY;
					if ( entranceTiles.find( tile ) != entranceTiles.end() )
						lock &= ~EAC_HUMAN;
					lockInfo.lockInfo[tile.y - vStartTile.y][tile.x - vStartTile.x] = lock;
					pTiles->push_back( SObjTileInfo( tile, lock ) );
				}
			}
		}
	}
	else if ( lockInfo.pNewLock )
	{
		const vector<SVector> &tiles = *lockInfo.pNewLock;
		lockInfo.lockedTiles.clear();
		for ( int i = 0; i < tiles.size(); ++i )
		{
			if ( !access.IsTileInside( tiles[i] ) )
				continue;

			const BYTE tileInfo = access.GetTileLockInfo( tiles[i] );
			BYTE lock = ~tileInfo & ePassClass & EAC_ANY;
			if ( entranceTiles.find( tiles[i] ) != entranceTiles.end() )
				lock &= ~EAC_HUMAN;

			lockInfo.lockedTiles.push_back( SObjTileInfo( tiles[i], lock ) );
		}

		*pTiles = lockInfo.lockedTiles;
	}
}
};

