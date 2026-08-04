#include "stdafx.h"

#include "System/FastMath.h"
#include "System/Time.h"
#include "Misc/bresenham.h"
#include "Bridge.h"
#include "NewUpdater.h"
#include "UnitsIterators2.h"
#include "AIUnit.h"
#include "Cheats.h"
#include "Statistics.h"
#include "GlobalWarFog.h"
#include "Scripts.h"
#include "Graveyard.h"
#include "StaticObjectsIters.h"
#include "Common_RTS_AI/StaticMapHeights.h"
#include "DebugTools/DebugInfoManager.h"

REGISTER_SAVELOAD_CLASS_NM( 0x1108D4D1, SSpanLock, CFullBridge );
REGISTER_SAVELOAD_CLASS( 0x1108D4D0, CFullBridge );
REGISTER_SAVELOAD_CLASS( 0x1108D49F, CBridgeSpan );

CBridgeHeightRemover theBridgeHeightsRemover;

extern CStaticObjects theStatObjs;
extern CGlobalWarFog theWarFog;
extern CStaticObjects theStatObjs;
extern CEventUpdater updater;
extern SCheats theCheats;
extern CDiplomacy theDipl;
extern CStatistics theStatistics;
extern CScripts *pScripts;
extern CGraveyard theGraveyard;

// CBridgeHeightRemover

void CBridgeHeightRemover::Clear()
{
	heightsOrder.clear();
	heightToRemove.clear();
}

void CBridgeHeightRemover::RegisterOrder( const int nHeightID )
{
	heightsOrder.push_front( nHeightID );
}

void CBridgeHeightRemover::RemoveHeight( const int nHeightID ) 
{ 
	heightToRemove[nHeightID] = true; 
}

void CBridgeHeightRemover::Segment()
{
	if ( heightToRemove.empty() )
		return;

	for ( CHeightsOrder::iterator it = heightsOrder.begin(); it != heightsOrder.end(); ++it )
	{
		if ( heightToRemove.find( *it ) != heightToRemove.end() )
		{
			SVector tileUp, tileDown;
			GetHeights()->RestoreHeights( *it );
			if ( GetHeights()->GetLocalHeightsInfo( &tileUp, &tileDown, *it ) )
				theWarFog.SynchronizeHeights( tileUp, tileDown );
		}
	}
	heightToRemove.clear();
}

//BASIC_REGISTER_CLASS( CBridgeSpan );

//*******************************************************************
//*														CBridgeSpan														*
//*******************************************************************

CBridgeSpan::CBridgeSpan( const SBridgeRPGStats *_pStats, const CVec3 &center, const float _fHP, const WORD _nDir, const int nFrameIndex )
: CGivenPassabilityStObject( center, _fHP, _nDir, nFrameIndex ), pStats( _pStats ), 
	bNewBuilt( false ), bLocked( false ), bDeletingAround( false ),
	nScriptID( -1 )
{
	Init();
	CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > pass;
	GetPassability( &pass );

	// под всем мостом запретить строить окопы.
	std::list<SVector> tiles;
	GetCoveredTiles( &tiles );
	GetTerrain()->AddUndigableTiles( tiles );

#ifndef _FINALRELEASE
	nTilesMarkerID = NDebugInfo::OBJECT_ID_GENERATE;
#endif
}

void CBridgeSpan::GetPlacement( SAINotifyPlacement *pPlacement, const NTimer::STime timeDiff )
{
	pPlacement->nObjUniqueID = GetUniqueId();
	pPlacement->dir = GetDir();
	pPlacement->center = CVec2(GetCenter().x,GetCenter().y);
	pPlacement->z = GetCenter().z;
	pPlacement->fSpeed = 0;
	pPlacement->dwNormal = GetHeights()->GetNormal( -1, -1 );
}

void CBridgeSpan::GetTilesForVisibilityInternal( CTilesSet *pTiles ) const
{
	SRect rect;
	GetBoundRect( &rect );
	GetAIMap()->GetTilesCoveredByRectSides( rect, pTiles );
}

void CBridgeSpan::GetTilesForVisibility( CTilesSet *pTiles ) const
{
	if ( pFullBridge == 0 )
		GetTilesForVisibilityInternal( pTiles );
	else
		pFullBridge->GetTilesForVisibility( pTiles );
}

void CBridgeSpan::GetCoveredTiles( std::list<SVector> *pTiles ) const
{
	SRect rect;
	GetBoundRect( &rect );
	GetAIMap()->GetTilesCoveredByRect( rect, pTiles );
}

bool CBridgeSpan::ShouldSuspendAction( const EActionNotify &eAction ) const
{
	return
		( eAction == ACTION_NOTIFY_RPG_CHANGED || 
			eAction == ACTION_NOTIFY_UPDATE_DIPLOMACY ||
			eAction == ACTION_NOTIFY_CHANGE_FRAME_INDEX ||
			eAction == ACTION_NOTIFY_NEW_ST_OBJ && bNewBuilt );
}

void CBridgeSpan::Build()
{
	bNewBuilt = true;
	SetHitPoints( 0 );
	LockTiles();
	//theStatObjs.UpdateAllPartiesStorages( false, true );
	pFullBridge->SpanBuilt( this );
}

void CBridgeSpan::GetVisibility( CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > *visibility ) const
{
	const CVec3 vCenter( GetCenter() );
	visibility->Init( pStats->GetPassability( GetFrameIndex() ), GetDir(), pStats->GetVisOrigin( GetFrameIndex() ), CVec2( vCenter.x, vCenter.y ) );
}

void CBridgeSpan::GetPassability( CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > *passability ) const
{
	const CVec3 vCenter( GetCenter() );
	passability->Init( pStats->GetPassability( GetFrameIndex() ), GetDir(), pStats->GetOrigin( GetFrameIndex() ), CVec2( vCenter.x, vCenter.y ) );
}

void CBridgeSpan::SetHeights()
{
	CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > pass;
	GetPassability( &pass );
	
	CArray2D<bool> newHeightsInfo;
	const float fBridgeZ = GetCenter().z;
	//DebugTrace( "Set bridge's heights at %2.3f + %2.3f = %2.3f", fBridgeZ, pStats->fHeight, fBridgeZ + pStats->fHeight );

	newHeightsInfo.SetSizes( (pass.GetMaxX() - pass.GetMinX() ) / SConsts::TILE_SIZE + 1, (pass.GetMaxY() - pass.GetMinY())/SConsts::TILE_SIZE +1);
	newHeightsInfo.FillEvery( false );

	const SVector vStartTile( AICellsTiles::GetTile( pass.GetMinX(), pass.GetMinY() ) );
	for ( int x = pass.GetMinX(); x < pass.GetMaxX(); x += SConsts::TILE_SIZE )
	{
		for ( int y = pass.GetMinY(); y < pass.GetMaxY(); y += SConsts::TILE_SIZE )
		{
			const CVec2 vTileCenter( AICellsTiles::GetCenterOfTile( x, y ) );
			const SVector tile( GetAIMap()->GetTile( x - pass.GetMinX(), y - pass.GetMinY()) );

			if ( pass.GetVal( vTileCenter ) )
			{
				GetTerrain()->AddBridgeTile( AICellsTiles::GetTile( x, y ) );
				newHeightsInfo[tile.y][tile.x] = true; //pStats->fHeight + fBridgeZ;
			}
		}
	}
	const SVector tileLeftDown( GetAIMap()->GetTile( CVec2( pass.GetMinX(), pass.GetMinY() ) ) );
	nOldHeightsID = GetHeights()->UpdateLocalHeights( tileLeftDown.x, tileLeftDown.y, newHeightsInfo, pStats->fHeight + fBridgeZ );
	SVector tileUp, tileDown;
	if ( GetHeights()->GetLocalHeightsInfo( &tileUp, &tileDown, nOldHeightsID ) )
		theWarFog.SynchronizeHeights( tileUp, tileDown );

	const CVec3 vCenter( GetCenter() );
	for ( CUnitsIter<0,0> iter( 0, ANY_PARTY, CVec2( vCenter.x, vCenter.y ), SConsts::TILE_SIZE * (newHeightsInfo.GetSizeX() + newHeightsInfo.GetSizeY() ) ); 
				!iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit * pUnit = *iter;
		if ( IsValidObj( pUnit ) && !pUnit->GetStats()->IsAviation() )
			updater.AddUpdate( 0, ACTION_NOTIFY_PLACEMENT, pUnit, -1 );
	}

	theBridgeHeightsRemover.RegisterOrder( nOldHeightsID );
#ifndef _FINALRELEASE
	DisplayBridgeTiles();
#endif
}

#ifndef _FINALRELEASE
void CBridgeSpan::DisplayBridgeTiles()
{
	std::vector<SVector> tiles;

	//Get tiles
	CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > pass;
	GetPassability( &pass );

	for ( int x = pass.GetMinX(); x < pass.GetMaxX(); x += SConsts::TILE_SIZE )
	{
		for ( int y = pass.GetMinY(); y < pass.GetMaxY(); y += SConsts::TILE_SIZE )
		{
			const CVec2 vTileCenter( AICellsTiles::GetCenterOfTile( x, y ) );

			if ( pass.GetVal( vTileCenter ) )
			{
				tiles.push_back( SVector( x / SConsts::TILE_SIZE, y / SConsts::TILE_SIZE ) );
			}
		}
	}

	if ( NGlobal::GetVar( "bridge_tile_markers", 0 ) )
		nTilesMarkerID = DebugInfoManager()->CreateMarker( nTilesMarkerID, tiles, DebugInfoManager()->GetCycleColor() );
}
#endif

void CBridgeSpan::CreateLockedTilesInfo( std::list<SObjTileInfo> *pTiles )
{
	pTiles->clear();

	CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > pass;
	GetPassability( &pass );
	for ( int x = pass.GetMinX(); x < pass.GetMaxX(); x += SConsts::TILE_SIZE )
	{
		for ( int y = pass.GetMinY(); y < pass.GetMaxY(); y += SConsts::TILE_SIZE )
		{
			const CVec2 vTileCenter( AICellsTiles::GetCenterOfTile( x, y ) );
			const SVector tile( GetAIMap()->GetTile( x, y ) );
			const BYTE val = pass.GetVal( vTileCenter );
			if ( GetAIMap()->IsTileInside( tile ) && val )
			{
				EAIClasses aiClassLock = EAC_NONE;
				if ( val & 0x01 )
					aiClassLock = (EAIClasses)( aiClassLock | EAC_TERRAIN );
				if ( val & 0x10 )
					aiClassLock = (EAIClasses)( aiClassLock | EAC_WATER );

				if ( aiClassLock != EAC_NONE )
					pTiles->push_back( SObjTileInfo( tile, aiClassLock ) );
			}
		}
	}
}

void CBridgeSpan::LockTiles()
{
	if ( !oldTilesInfo.empty() )
		return;

	CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > pass;
	GetPassability( &pass );
	oldTilesInfo.clear();
	for ( int x = pass.GetMinX(); x < pass.GetMaxX(); x += SConsts::TILE_SIZE )
	{
		for ( int y = pass.GetMinY(); y < pass.GetMaxY(); y += SConsts::TILE_SIZE )
		{
			const CVec2 vTileCenter( AICellsTiles::GetCenterOfTile( x, y ) );
			const SVector tile( GetAIMap()->GetTile( x, y ) );
			const BYTE val = pass.GetVal( vTileCenter );
			if ( GetAIMap()->IsTileInside( tile ) && val )
			{
				const EAIClasses aiClassLock = ( val & 0x01 ) ? EAC_TERRAIN : EAC_NONE;
				oldTilesInfo.push_back( SObjTileInfo( tile, GetTerrain()->GetTileLockInfo( tile ) | aiClassLock ) );
			}
		}
	}
}

void CBridgeSpan::RealLockTiles()
{
	if ( fHP <= 0 || bLocked )
		return;

	CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > pass;
	GetPassability( &pass );

	bLocked = true;

	const EAIClasses aiClass = (EAIClasses)pStats->nAIPassabilityClass;

	std::list<SObjTileInfo> unlockTiles;
	for ( int x = pass.GetMinX(); x < pass.GetMaxX(); x += SConsts::TILE_SIZE )
	{
		for ( int y = pass.GetMinY(); y < pass.GetMaxY(); y += SConsts::TILE_SIZE )
		{
			const CVec2 vTileCenter( AICellsTiles::GetCenterOfTile( x, y ) );
			const SVector tile( GetAIMap()->GetTile( x, y ) );
			const BYTE val = pass.GetVal( vTileCenter );
			if ( GetAIMap()->IsTileInside( tile ) && val )
			{
				EAIClasses aiClassUnlock = EAC_NONE;
				if ( val & 0x02 )
					aiClassUnlock = aiClass;

				if ( aiClassUnlock != EAC_NONE )
					unlockTiles.push_back( SObjTileInfo( tile, aiClassUnlock ) );
			}
		}
	}
	GetTerrain()->RemoveStaticObjectTilesForBridge( unlockTiles );

	std::list<SObjTileInfo> lockTiles;
	CreateLockedTilesInfo( &lockTiles );
	GetTerrain()->AddStaticObjectTiles( lockTiles );
}

void CBridgeSpan::UnlockTiles() 
{
	if ( fHP < 0 || !bLocked )
		return;
	bLocked = false;

	if ( fHP == 0.0 )
	{
		if ( IsValid( pFullBridge ) )
			pFullBridge->NeedRepair();
	}

	std::list<SObjTileInfo> unlockTiles;
	for ( std::list<SObjTileInfo>::iterator it = oldTilesInfo.begin(); it != oldTilesInfo.end(); ++it )
		unlockTiles.push_back( SObjTileInfo( it->tile, EAC_ANY ) );

	GetTerrain()->RemoveStaticObjectTiles( unlockTiles );
	GetTerrain()->AddStaticObjectTiles( oldTilesInfo );
	// RemoveHeights();

#ifndef _FINALRELEASE
	DisplayBridgeTiles();
#endif
}

void CBridgeSpan::RemoveHeights()
{
	if ( -1 != nOldHeightsID )
	{
		theBridgeHeightsRemover.RemoveHeight( nOldHeightsID );
	}
	nOldHeightsID = -1;
}

void CBridgeSpan::SetHitPoints( const float fNewHP )
{
	if ( fHP < GetStats()->fMaxHP && fNewHP == GetStats()->fMaxHP )
		LockTiles();
	else if ( fHP > 0 && fNewHP == 0 )
		UnlockTiles();

	if ( fHP != fNewHP )
	{
		fHP = (std::min)( fNewHP, GetStats()->fMaxHP );
		SetAlive( fHP > 0.0f );
		updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, this, -1 );
	}
}

void CBridgeSpan::Die( const float fDamage )
{
	if ( bDeletingAround ) 
		return;

	SRect	boundRect;
	GetBoundRect( &boundRect );
	theGraveyard.DelKilledUnitsFromBridge( boundRect );

	UnlockTiles();

	updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, this, -1 );
	fHP = 0.0f;

	CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > pass;
	GetPassability( &pass );

	// убить юнитов, стоявших на этом пролёте
	const CVec2 rectCenter( ( pass.GetMaxX() + pass.GetMinX() ) * 0.5f, ( pass.GetMinY() + pass.GetMaxY() ) * 0.5f );
	const CVec2 vAABBHalfSize( ( pass.GetMaxX() - pass.GetMinX() ) * 0.5f + SConsts::MAX_UNIT_TILE_RADIUS, 
		( pass.GetMaxY() - pass.GetMinY() ) * 0.5f + SConsts::MAX_UNIT_TILE_RADIUS );

	std::list<CAIUnit*> deadUnits;

	std::list<CExistingObject*> deadObjects;
	{
		STerrainModeSetter terrainMode( ELM_STATIC, GetTerrain() );
		for ( CUnitsIter<0,0> iter( 0, ANY_PARTY, rectCenter, (std::max)( vAABBHalfSize.x, vAABBHalfSize.y ) ); !iter.IsFinished(); iter.Iterate() )
		{
			CAIUnit *pUnit = *iter;
			if ( pUnit && pUnit->IsAlive() && !pUnit->GetStats()->IsAviation() )
			{
				const SRect unitRect( pUnit->GetUnitRect() );
				if ( boundRect.IsIntersected( unitRect ) )
					deadUnits.push_back( pUnit );
			}
		}

		for ( CStObjCircleIter<false> iter( rectCenter, vAABBHalfSize.x + vAABBHalfSize.y ); !iter.IsFinished(); iter.Iterate() )
		{
			CExistingObject *pObj = *iter;
			if ( pObj && pObj->IsAlive() && pObj->GetObjectType() != ESOT_BRIDGE_SPAN )
			{
				SRect objectRect;
				pObj->GetBoundRect( &objectRect );
				if ( boundRect.IsIntersected( objectRect ) || GetTerrain()->IsLocked( GetAIMap()->GetTile( CVec2( pObj->GetCenter().x, pObj->GetCenter().y ) ), EAC_TERRAIN ) )
					deadObjects.push_back( pObj );
			}
		}
	}

	for ( std::list<CAIUnit*>::iterator iter = deadUnits.begin(); iter != deadUnits.end(); ++iter )
	{
		CAIUnit *pUnit = *iter;
		theStatistics.UnitDead( pUnit );
		pUnit->Disappear();
	}

	bDeletingAround = true;
	for ( std::list<CExistingObject*>::iterator it = deadObjects.begin(); it != deadObjects.end(); ++it )
	{
		CExistingObject *pObj = *it;
		if ( pObj->IsAlive() )
			pObj->Die( 0 );
	}
	bDeletingAround = false;
}

void CBridgeSpan::TakeDamage( const float fDamage, const bool bFromExplosion, const int nPlayerOfShoot, CAIUnit *pShotUnit )
{
	NI_ASSERT( pFullBridge != 0, "Check your map!!! Bridge span without full bridge." );
	if ( bFromExplosion && fHP > 0 && pFullBridge && pFullBridge->CanTakeDamage() && !theCheats.GetImmortals(0) )
	{
		fHP = (std::max)( 0.0f, fHP - fDamage );

		if ( theCheats.GetFirstShoot( theDipl.GetNParty( nPlayerOfShoot) ) == 1 )
			fHP = 0;

		if ( fHP == 0  )
			Die( fDamage );
		else
		{
			WasHit();
			updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, this, -1 );
		}
	
		pFullBridge->DamageTaken( this, fDamage, bFromExplosion, nPlayerOfShoot, pShotUnit );
	}
}

bool CBridgeSpan::IsPointInside( const CVec2 &point ) const
{
	SRect boundRect;
	GetBoundRect( &boundRect );

	return boundRect.IsPointInside( point );
}

void CBridgeSpan::SetFullBrige( CFullBridge *_pFullBridge )
{
	pFullBridge = _pFullBridge;
}

void CBridgeSpan::SetTransparencies()
{
	SetTransparenciesInt( pFullBridge ? pFullBridge->GetUniqueId() : GetUniqueId() );
}

void CBridgeSpan::RemoveTransparencies()
{
	RemoveTransparenciesInt( pFullBridge ? pFullBridge->GetUniqueId() : GetUniqueId() );
}

//*******************************************************************
//*														CFullBridge														*
//*******************************************************************

struct SOnlyDirNeed
{
	bool operator()( const int nTest, const int nDesire) const
	{ return nTest == nDesire; }
};

class CTilesColl
{
public:
	std::list<SVector> *pTiles;
	CTilesColl( std::list<SVector> *_pTiles ) : pTiles( _pTiles ) { }

	void operator()( const int x, const int y )
	{ 
		if ( GetAIMap()->IsTileInside( x, y ) )
			pTiles->push_back( SVector( x, y ) );
	}
};

static void GetTilesUnderRectSide( const SRect &rect, std::list<SVector> *pTiles, const WORD wDir, SOnlyDirNeed need )
{
	CTilesColl a( pTiles );

	//перейдм к DWORD чтобы не переполнялось
	DWORD arDir[4];
	DWORD dwDir = wDir;
	arDir[0] = GetDirectionByVector( (rect.v1 +rect.v2)/2 - rect.center );
	arDir[1] = GetDirectionByVector( (rect.v2 +rect.v3)/2 - rect.center );
	arDir[2] = GetDirectionByVector( (rect.v3 +rect.v4)/2 - rect.center );
	arDir[3] = GetDirectionByVector( (rect.v4 +rect.v1)/2 - rect.center );

	int iMin = 0;
	DWORD dwMin = 65535;
	for( int i=0; i< 4; ++i )
	{
		const WORD wDirsDiff = DirsDifference( arDir[i], dwDir );
		if( dwMin >  wDirsDiff )
		{
			iMin = i;
			dwMin  = wDirsDiff;
		}
	}

	const int nTileSize = GetAIMap()->GetTileSize();

	if ( need(iMin,0) )
	{//wDir не лежит между arDir[0] и arDir[1]
		MakeLine2( rect.v1.x/nTileSize, rect.v1.y/nTileSize, rect.v2.x/nTileSize, rect.v2.y/nTileSize, a );
	}

	if ( need(iMin,1) )
	{//wDir не лежит между arDir[1] и arDir[2]
		MakeLine2( rect.v2.x/nTileSize, rect.v2.y/nTileSize, rect.v3.x/nTileSize, rect.v3.y/nTileSize, a );
	}

	if ( need(iMin,2) )
	{//wDir no лежит между arDir[1] и arDir[2]
		MakeLine2( rect.v3.x/nTileSize, rect.v3.y/nTileSize, rect.v4.x/nTileSize, rect.v4.y/nTileSize, a );
	}

	if ( need(iMin,3) )
	{//wDir no лежит между arDir[1] и arDir[2]
		MakeLine2( rect.v4.x/nTileSize, rect.v4.y/nTileSize, rect.v1.x/nTileSize, rect.v1.y/nTileSize, a );
	}
}

CFullBridge::SSpanLock::SSpanLock( CBridgeSpan * pSpan, const WORD wDir )
: pSpan( pSpan )
{
	// найти тайлы. запомнить состояние залоченности.
	SRect rect; 
	pSpan->GetBoundRect( &rect );

	SOnlyDirNeed a;
	GetTilesUnderRectSide( rect, &tiles, wDir + 65535/2, a );

	std::list<SObjTileInfo> tilesInfo;
	// разлокать
	for ( std::list<SVector>::const_iterator it = tiles.begin(); it != tiles.end(); ++it )
	{
		const EAIClasses lockInfo = GetTerrain()->GetTileLockInfo( *it );
		formerTiles.push_back( lockInfo );

		tilesInfo.push_back( SObjTileInfo( *it, lockInfo ) );

	}
	GetTerrain()->RemoveStaticObjectTiles( tilesInfo );

	// залокать для всех
	for ( std::list<SObjTileInfo>::iterator iter = tilesInfo.begin(); iter != tilesInfo.end(); ++iter )
		iter->lockInfo = EAC_TERRAIN;
	GetTerrain()->AddStaticObjectTiles( tilesInfo );
}

void CFullBridge::SSpanLock::Unlock()
{
	// разлокать для всех
	std::list<SObjTileInfo> tilesInfo;
	for ( std::list<SVector>::iterator iter = tiles.begin(); iter != tiles.end(); ++iter )
		tilesInfo.push_back( SObjTileInfo( *iter, EAC_TERRAIN ) );
	GetTerrain()->RemoveStaticObjectTiles( tilesInfo );

	// залокать как было
	std::list<EAIClasses>::const_iterator lockedIter = formerTiles.begin();
	std::list<SObjTileInfo>::iterator tilesInfoIter = tilesInfo.begin();
	for ( std::list<SVector>::const_iterator it = tiles.begin(); it != tiles.end(); ++it )
	{
		tilesInfoIter->lockInfo = *lockedIter;
		++lockedIter;
		++tilesInfoIter;
	}
	GetTerrain()->AddStaticObjectTiles( tilesInfo );
	
	// забыть тайлы
	tiles.clear();
	formerTiles.clear();
}

//*******************************************************************
//*														CFullBridge														*
//*******************************************************************

//BASIC_REGISTER_CLASS( CFullBridge );

void CFullBridge::AddSpan( CBridgeSpan *pSpan )
{
	if ( pSpan->GetHitPoints() < 0.0f )
		projectedSpans.push_back( pSpan );
	else
	{
		spans.push_back( pSpan );
	}
	++nSpans;
}

void CFullBridge::SpanBuilt( CBridgeSpan * pSpan )
{
	for ( std::list<CBridgeSpan*>::iterator it = projectedSpans.begin(); it != projectedSpans.end(); ++it )
	{
		if ( *it == pSpan )
		{
			projectedSpans.erase( it );
			AddSpan( pSpan );
			return;
		}
	}
}

const float CFullBridge::GetHPPercent() const
{
	NI_ASSERT( !spans.empty(), "no spans" );
	return (*spans.begin())->GetHitPoints() / (*spans.begin())->GetStats()->fMaxHP;
}

bool CFullBridge::CanTakeDamage() const
{
	return true;
}

void CFullBridge::DamageTaken( CBridgeSpan *pDamagedSpan, const float fDamage, const bool bFromExplosion, const int nPlayerOfShoot, CAIUnit *pShotUnit )
{
	if ( !bGivingDamage )
	{
		bGivingDamage = true;
		const float fNewHPPercent = pDamagedSpan->GetHitPoints() / pDamagedSpan->GetStats()->fMaxHP;

		for ( std::list<CBridgeSpan*>::iterator iter = spans.begin(); iter != spans.end(); ++iter )
		{
			CBridgeSpan *pSpan = *iter;
			if ( pSpan != pDamagedSpan )
			{
				// раздать всем damage, чтобы уравнять процентное соотношение всех HP
				const float fCurMaxHP = pSpan->GetStats()->fMaxHP;
				const float fCurHPPercent = pSpan->GetHitPoints() / fCurMaxHP;
				if ( fCurHPPercent > fNewHPPercent )
				{
					const float fDamage = (fCurHPPercent - fNewHPPercent) * fCurMaxHP;
					pSpan->TakeDamage( fDamage, bFromExplosion, nPlayerOfShoot, pShotUnit );
				}
			}
		}

		bGivingDamage = false;
		theStatistics.ObjectDestroyed( nPlayerOfShoot );
	}
}

void CFullBridge::UnlockAllSpans()
{
	for ( LockedSpans::iterator it = lockedSpans.begin(); it != lockedSpans.end(); )
	{
		(*it)->Unlock();
		it = lockedSpans.erase( it );
	}
}

void CFullBridge::LockSpan( CBridgeSpan * pSpan, const WORD wDir )
{
	lockedSpans.push_back( new SSpanLock( pSpan, wDir ) );
}

void CFullBridge::UnlockSpan( CBridgeSpan * pSpan )
{
	for ( LockedSpans::iterator it = lockedSpans.begin(); it != lockedSpans.end(); )
	{
		if ( (*it)->GetSpan() == pSpan )
		{
			(*it)->Unlock();
			it = lockedSpans.erase( it );
		}
		else
			++it;
	}
}

const int CFullBridge::GetNSpans() const
{
	return nSpans;
}

void CFullBridge::EnumSpans( std::vector< CObj<CBridgeSpan> > *pSpans )
{
	for ( std::list<CBridgeSpan*>::iterator it = spans.begin(); it != spans.end(); ++it )
		pSpans->push_back( *it );
	for ( std::list<CBridgeSpan*>::iterator it = projectedSpans.begin(); it != projectedSpans.end(); ++it )
		pSpans->push_back( *it );
}

const bool CFullBridge::IsVisible( const BYTE cParty ) const
{
	CTilesSet tiles;
	GetTilesForVisibility( &tiles );

	for ( CTilesSet::const_iterator iter = tiles.begin(); iter != tiles.end(); ++iter )
	{
		if ( theWarFog.IsTileVisible( *iter, cParty ) )
			return true;
	}

	return false;
}

void CFullBridge::GetTilesForVisibility( CTilesSet *pTiles ) const
{
	for ( std::list<CBridgeSpan*>::const_iterator it = spans.begin(); it != spans.end(); ++it )
		(*it)->GetTilesForVisibilityInternal( pTiles );
}

void CFullBridge::InitEntireBridge()
{
	det_set<int> ids;
	for ( std::list<CBridgeSpan*>::const_iterator it = spans.begin(); it != spans.end(); ++it )
		ids.insert( (*it)->GetUniqueId() );
	theWarFog.ReplaceStaticObjects( GetUniqueId(), ids );

	if ( bLockingBridge )
	{
		for ( std::list<CBridgeSpan*>::const_iterator it = spans.begin(); it != spans.end(); ++it )
			(*it)->RealLockTiles();
	}
}


