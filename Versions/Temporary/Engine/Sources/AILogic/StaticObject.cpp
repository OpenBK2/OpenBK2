#include "stdafx.h"

#include "System/Time.h"
#include "Misc/bresenham.h"
#include "StaticObject.h"
#include "NewUpdater.h"
#include "Diplomacy.h"
#include "Shell.h"
#include "Cheats.h"
#include "GlobalWarFog.h"
#include "Common_RTS_AI/StaticMapHeights.h"
#include "StaticObjectsIters.h"
#include "Common_RTS_AI/AIMap.h"
#include "ExecutorContainer.h"
#include "ObjectProfile.h"
#include "AIDebugInfo.h"
#include "System/FastMath.h"
#include "AILock.h"
#include "FeedBackSystem.h"
#include "ExecutorBurningFuel.h"
extern bool g_bNewLock;

extern CFeedBackSystem theFeedBackSystem;


class CTerrainAccess
{
public:
	bool IsTileInside( const SVector &vPos ) const
	{
		return GetAIMap()->IsTileInside( vPos );
	}
	SVector GetTile( const CVec2& vPos ) const
	{
		return AICellsTiles::GetTile( vPos );
	}
	EAIClasses GetTileLockInfo( const SVector &tile ) const
	{
		return GetTerrain()->GetTileLockInfo( tile );
	}
};

EXTERNVAR CExecutorContainer theExecutorContainer;

int ConvertToNAngle( const WORD _wAngle ) 
{
	return int( float(_wAngle) / 16384 + 0.5f) % 4;
}

int CExistingObjectModifyAI::bProhibited = 0;

REGISTER_SAVELOAD_CLASS( 0x1108D44F, CSimpleStaticObject );
REGISTER_SAVELOAD_CLASS( 0x110B8BC0, CTerraMeshStaticObject );

extern CGlobalWarFog theWarFog;
extern CEventUpdater updater;
extern CStaticObjects theStatObjs;
extern CDiplomacy theDipl;
extern SCheats theCheats;
extern NTimer::STime curTime;

//*******************************************************************
//*												  CStaticObject														*
//*******************************************************************

BASIC_REGISTER_CLASS( CStaticObject );

const BYTE CStaticObject::GetPlayer() const 
{ 
	return theDipl.GetNeutralPlayer();
}

const bool CStaticObject::IsVisible( const BYTE cParty ) const
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

void CStaticObject::GetTilesForVisibility( CTilesSet *pTiles ) const
{
	SRect rect;
	GetBoundRect( &rect );
	GetAIMap()->GetTilesCoveredByRectSides( rect, pTiles );
}

bool CStaticObject::ShouldSuspendAction( const EActionNotify &eAction ) const
{
	return
		(	eAction == ACTION_NOTIFY_RPG_CHANGED || 
			eAction == ACTION_NOTIFY_UPDATE_DIPLOMACY ||
			eAction == ACTION_NOTIFY_TREE_BROKEN ||
			eAction == ACTION_NOTIFY_CHANGE_FRAME_INDEX ||
			eAction == ACTION_NOTIFY_DISSAPEAR_OBJ ||
			//eAction == ACTION_NOTIFY_DELETED_ST_OBJ ||
			eAction == ACTION_NOTIFY_SILENT_DEATH );
}

//*******************************************************************
//*												  CExistingObject													*
//*******************************************************************

BASIC_REGISTER_CLASS( CExistingObject );

unsigned long CExistingObject::globalMark = 0;

//DEBUG{
#ifndef _FINALRELEASE
extern bool bShowUnderConstruction ;
#endif
//DEBUG}

void CExistingObject::UpdateGlobalMark() 
{ 
	if ( !CExistingObjectModifyAI::IsProhibited() )
		++globalMark; 
}

void CExistingObject::SetGlobalUpdated()
{ 
	if ( !CExistingObjectModifyAI::IsProhibited() )
		mark = globalMark; 
}

void CExistingObject::GetPlacement( SAINotifyPlacement *pPlacement, const NTimer::STime timeDiff )
{
	pPlacement->nObjUniqueID = GetUniqueId();
	pPlacement->dir = GetDir();
	pPlacement->center = CVec2(GetCenter().x,GetCenter().y);
	pPlacement->z = GetTerrainHeight( pPlacement->center.x, pPlacement->center.y, timeDiff ) + GetCenter().z;
	pPlacement->fSpeed = 0;
	pPlacement->dwNormal = GetHeights()->GetNormal( -1, -1 );
}

void CExistingObject::GetNewUnitInfo( SNewUnitInfo *pNewUnitInfo )
{
	GetPlacement( pNewUnitInfo, 0 );

	pNewUnitInfo->pStats = GetStats();
	pNewUnitInfo->eDipl = EDI_NEUTRAL;
	pNewUnitInfo->nFrameIndex = GetFrameIndex();
	pNewUnitInfo->fHitPoints = GetHitPoints();
	pNewUnitInfo->fResize = 1.0f;
	pNewUnitInfo->nPlayer = GetPlayer();
}

void CExistingObject::SetNewPlacement( const CVec3 &center, const WORD dir )	
{ 
	UnlockTiles();	

	SetNewPlaceWithoutMapUpdate( center, dir );
	updater.AddUpdate( 0, ACTION_NOTIFY_ST_OBJ_PLACEMENT, this, -1 );

	LockTiles();
}

void CExistingObject::Delete()
{
	if ( !IsTrampled() )
		updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, this, -1 );

	theStatObjs.DeleteInternalObjectInfo( this );
	theFeedBackSystem.RemovedAllFeedbacks( nUniqueID );
	// if object has weapon that should shoot upon destruction, create damaging executor
	const NDb::SHPObjectRPGStats * pHPStats = GetStats();
	const NDb::SStaticObjectRPGStats * pSStats = dynamic_cast<const NDb::SStaticObjectRPGStats *>( pHPStats );
	if ( pSStats && pSStats->pShootOnDestruction )
	{
		const CVec3 vCen = GetCenter();
		theExecutorContainer.Add( new CExecutorBurningFuel( GetHeights()->Get3DPoint( CVec2( vCen.x, vCen.y ) ), pSStats->pShootOnDestruction ) );
	}
}

const int CExistingObject::GetRandArmorByDir( const int nArmorDir, const WORD wAttackDir )
{
	switch ( nArmorDir )
	{
		case 0:
			{
				SRect boundRect;
				GetBoundRect( &boundRect );
				return GetStats()->GetRandomArmor( boundRect.GetSide( wAttackDir ) );
			}
		case 1: return GetStats()->GetRandomArmor( RPG_BOTTOM );
		case 2: return GetStats()->GetRandomArmor( RPG_TOP );
		default: NI_VERIFY( false, "Wrong armor dir", return 0 );
	}

	return 0;
}

bool CExistingObject::ProcessCumulativeExpl( CExplosion *pExpl, const int nArmorDir, const bool bFromExpl )
{
	const CVec3 vExpl( pExpl->GetExplCoordinates() );
	const CVec2 vExpl2( vExpl.x, vExpl.y );
	const CVec2 vCntr( GetAttackCenter( vExpl2 ) );
	const float fAreaRadius = pExpl->GetArea();
	const bool bCanDamageByArea = ( fabs( vCntr - vExpl2 ) < fAreaRadius ) && GetStats();	

	if ( vExpl.z - GetHeights()->GetVisZ( GetCenter().x, GetCenter().y ) <= AI_TILE_SIZE * 2.0f && ( IsPointInside( vExpl2 ) || bCanDamageByArea ) )
	{
		SRect targetRect; 
		GetBoundRect( &targetRect );

		if ( pExpl->GetRandomPiercing() >= GetRandArmorByDir( nArmorDir, pExpl->GetAttackDir() ) || theCheats.GetFirstShoot( pExpl->GetPlayerOfShoot() ) == 1 )
		{
			TakeDamage( pExpl->GetRandomDamage(), bFromExpl, pExpl->GetPlayerOfShoot(), pExpl->GetWhoFire() );
			pExpl->AddHitToSend( new CHitInfo( pExpl, this, SAINotifyHitInfo::EHT_HIT, vExpl ) );
		}
		else
			pExpl->AddHitToSend( new CHitInfo( pExpl, this, SAINotifyHitInfo::EHT_REFLECT, vExpl ) );

		return true;
	}

	return false;
}

bool CExistingObject::ProcessBurstExpl( CExplosion *pExpl, const int nArmorDir, const float fRadius, const float fSmallRadius )
{
	if ( !ProcessCumulativeExpl( pExpl, nArmorDir, true ) )
	{
		ProcessAreaDamage( pExpl, nArmorDir, fRadius, fSmallRadius );
		return false;
	}
	else
		return IsPointInside( CVec2( pExpl->GetExplCoordinates().x, pExpl->GetExplCoordinates().y )  );
}

void CExistingObject::BurnSegment()
{
	const float fDamage = GetStats()->fMaxHP * SConsts::BURNING_SPEED * 0.01f * SConsts::AI_SEGMENT_DURATION;
	fHP = (std::max)( GetMinHP(), fHP - fDamage );
	SetAlive( GetHitPoints() > 0.0f );
	if ( fHP <= 0 )
		Die( fDamage );

	updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, this, -1 );

	if ( curTime >= burningEnd )
		theStatObjs.EndBurning( this );
}

void CExistingObject::WasHit()
{
	if ( checked_cast<const SStaticObjectRPGStats*>(GetStats())->bBurn && fHP <= GetStats()->fMaxHP / 2.0f )
	{
		burningEnd = curTime + 10000;
		theStatObjs.StartBurning( this );
	}
}

//*******************************************************************
//*												 CGivenPassabilityStObject								*
//*******************************************************************

bool CGivenPassabilityStObject::CheckStaticObject( const SObjectBaseRPGStats *pStats, const CVec2 &vPos, const WORD wDir, const int nFrameIndex )
{
	if ( g_bNewLock == 0 )
	{
		const CVec3 vCenter( vPos, 0 );
		const CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > pass( pStats->GetPassability( nFrameIndex ), wDir, pStats->GetOrigin( nFrameIndex ), CVec2( vCenter.x, vCenter.y ) );
		for ( int x = pass.GetMinX(); x < pass.GetMaxX(); x += SConsts::TILE_SIZE )
		{
			for ( int y = pass.GetMinY(); y < pass.GetMaxY(); y += SConsts::TILE_SIZE )
			{
				const CVec2 vTileCenter( AICellsTiles::GetCenterOfTile( x, y ) );
				const SVector tile( AICellsTiles::GetTile( x, y ) );
				if ( pass.GetVal( vTileCenter ) && GetTerrain()->IsLocked( tile, EAC_TERRAIN ) )
					return false;
			}
		}
	}
	else
	{
		const bool bForceThickLock = dynamic_cast<const SFenceRPGStats*>( pStats ) != 0;
		CPtr<CObjectProfile> pPassProfile = new CObjectProfile( pStats->GetPassProfile( nFrameIndex ), CVec3( vPos, 0.0f ), wDir, bForceThickLock );
		const std::vector<SVector> &tiles = pPassProfile->GetTilesUnder();
		for ( int i = 0; i < tiles.size(); ++i )
		{
			if ( GetTerrain()->IsLocked( tiles[i], EAC_TERRAIN ) )
				return false;
		}
	}

	return true;
}

void CGivenPassabilityStObject::RotateFence( const CVec3 &vNewCenter )
{
	center = vNewCenter;
	wDir += 65536 / 2;
}
//DEBUG{
//DEBUG}

CGivenPassabilityStObject::CGivenPassabilityStObject( const CVec3 &_center, const float _fHP,
																										  const WORD _wAngle, const int nFrameIndex )
: CExistingObject( nFrameIndex, _fHP ), center( _center ), bTransparencySet( false ), wDir( _wAngle )
{ 
	// unitialized hunt
	boundRect.InitRect( VNULL2, V2_AXIS_X, 0, 0, 0 );
}

void CGivenPassabilityStObject::Init()
{
	const CVec3 vCenter( GetCenter() );
	const SStaticObjectRPGStats* pStats = checked_cast<const SStaticObjectRPGStats*>( GetStats() );

	const CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > pass( pStats->GetPassability( GetFrameIndex() ), wDir, pStats->GetOrigin( GetFrameIndex() ), CVec2( vCenter.x, vCenter.y ) );
	boundRect.InitRect( CVec2( (pass.GetMinX() + pass.GetMaxX()) * 0.5f,
														 (pass.GetMinY() + pass.GetMaxY()) * 0.5f), 
														 CVec2( 1, 0 ), 
														 0.5f * ( pass.GetMaxX() - pass.GetMinX() ),
														 0.5f * ( pass.GetMaxY() - pass.GetMinY() ) );

	const bool bForceThickLock = dynamic_cast<const SFenceRPGStats*>( pStats ) != 0;
	pPassProfile = new CObjectProfile( pStats->GetPassProfile( GetFrameIndex() ), GetCenter(), wDir, bForceThickLock );
	pVisProfile = new CObjectProfile( pStats->GetVisProfile( GetFrameIndex() ), GetCenter(), wDir, bForceThickLock );
}

void CGivenPassabilityStObject::GetVisibility( CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > *visibility ) const
{
	const SStaticObjectRPGStats* pStats = checked_cast<const SStaticObjectRPGStats*>( GetStats() );	
	const CVec3 vCenter( GetCenter() );
	visibility->Init( pStats->GetVisibility( GetFrameIndex() ), wDir, pStats->GetVisOrigin( GetFrameIndex() ), CVec2( vCenter.x, vCenter.y ) );
}

void CGivenPassabilityStObject::GetPassability( CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > *passability ) const
{
	const SStaticObjectRPGStats* pStats = checked_cast<const SStaticObjectRPGStats*>( GetStats() );	
	const CVec3 vCenter( GetCenter() );
	passability->Init( pStats->GetPassability( GetFrameIndex() ), wDir, pStats->GetOrigin( GetFrameIndex() ), CVec2( vCenter.x, vCenter.y ) );
}

int CGivenPassabilityStObject::GetHeight() const
{
	if ( const SObjectBaseRPGStats* pStats = dynamic_cast<const SObjectBaseRPGStats*>( GetStats() ) ) 
		return pStats->nObjectHeight;
	else
		return 32;
}
const EAIClasses CGivenPassabilityStObject::GetPassabilityClass() const
{
	const SStaticObjectRPGStats* pStats = checked_cast<const SStaticObjectRPGStats*>( GetStats() );	
	return (EAIClasses)pStats->nAIPassabilityClass;
}

void CGivenPassabilityStObject::GetRPGStats( SAINotifyRPGStats *pStats ) 
{ 
	pStats->nObjUniqueID = GetUniqueId();
	pStats->fHitPoints = fHP;
	pStats->time = curTime;
}

void CGivenPassabilityStObject::CreateLockedTilesInfo( std::list<SObjTileInfo> *pTiles )
{
	std::vector<NLock::SEntranceData> entrances;
	int nEntrances = GetNEntrancePoints();
	entrances.resize( nEntrances );
	for ( int i = 0; i < nEntrances; ++i )
		GetEntranceData( &entrances[i].vPos, &entrances[i].wDir, i );

	CTerrainAccess access;
	NLock::SStaticObjectLockInfo info( lockInfo, lockedTiles );
	if ( g_bNewLock == 0 )
	{
		info.bNewLockingWay = false;
		GetPassability( &info.oldLock );
	}
	else if ( GetPassProfile() != 0 )
	{
		info.bNewLockingWay = true;
		info.pNewLock = &GetPassProfile()->GetTilesUnder();
	}

	NLock::CreateStaticObjectLockedTilesInfo( pTiles, entrances, GetPassabilityClass(), info, access );
}

void CGivenPassabilityStObject::LockTiles()
{
	std::list<SObjTileInfo> tiles;
	CreateLockedTilesInfo( &tiles );
	GetTerrain()->AddStaticObjectTiles( tiles );

#if !defined(_FINALRELEASE)
	const SStaticObjectRPGStats* pStats = checked_cast<const SStaticObjectRPGStats*>( GetStats() );
	NAIVisInfo::AddProfile( GetUniqueId(), GetCenter(), GetDir(), pStats->GetPassProfile( GetFrameIndex() ) );
#endif
}

void CGivenPassabilityStObject::UnlockTiles() 
{
	if ( g_bNewLock == 0 )
	{
		CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > pass;
		GetPassability( &pass );

		const SVector vStartTile( AICellsTiles::GetTile( pass.GetMinX(), pass.GetMinY() ) );
		std::list<SObjTileInfo> tiles;
		
		// object lock mast changed since save created
		if ( pass.GetMaxX() - pass.GetMinX() >= lockInfo.GetSizeX() ||
				 pass.GetMaxY() - pass.GetMinY() >= lockInfo.GetSizeY() )
				 return;

		for ( int x = pass.GetMinX(); x < pass.GetMaxX(); x += SConsts::TILE_SIZE )
		{
			for ( int y = pass.GetMinY(); y < pass.GetMaxY(); y += SConsts::TILE_SIZE )
			{
				const CVec2 vTileCenter( AICellsTiles::GetCenterOfTile( x, y ) );
				if ( pass.GetVal( vTileCenter ) )
				{
					const SVector tile( AICellsTiles::GetTile( x, y ) );				
					if ( !GetAIMap()->IsTileInside( tile ) )
						continue;
					tiles.push_back( SObjTileInfo( tile, lockInfo[tile.y - vStartTile.y][tile.x - vStartTile.x] ) );
				}
			}
		}
		GetTerrain()->RemoveStaticObjectTiles( tiles );
	}
	else if ( GetPassProfile() != 0 )
		GetTerrain()->RemoveStaticObjectTiles( lockedTiles );

#if !defined(_FINALRELEASE)
	NAIVisInfo::RemoveProfile( GetUniqueId() );
#endif
}

void CGivenPassabilityStObject::SetTransparenciesInt( const int nUniqueID )
{
	if ( !GetStats() || GetStats()->GetTypeID() != NDb::SBuildingRPGStats::typeID && GetStats()->GetTypeID() != NDb::SBridgeRPGStats::typeID )
		return;
	//special for (c) Alexander Vinnikov's method of visibility
	CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > pass;
	GetVisibility( &pass );

	const SVector vStartTile( AICellsTiles::GetTile( pass.GetMinX(), pass.GetMinY() ) );
	std::list<SObjTileInfo> tiles;

	for ( int x = pass.GetMinX(); x < pass.GetMaxX(); x += SConsts::TILE_SIZE )
	{
		for ( int y = pass.GetMinY(); y < pass.GetMaxY(); y += SConsts::TILE_SIZE )
		{
			const CVec2 vTileCenter( AICellsTiles::GetCenterOfTile( x, y ) );
			const SVector tile( AICellsTiles::GetTile( x, y ) );				

			if ( GetAIMap()->IsTileInside( tile ) && pass.GetVal( vTileCenter ) )
				theWarFog.AddStaticObjectTile( tile, nUniqueID, GetHeight() );
		}
	}
	
	bTransparencySet = true;
}

void CGivenPassabilityStObject::RemoveTransparenciesInt( const int nUniqueID )
{
	if ( !GetStats() || GetStats()->GetTypeID() != NDb::SBuildingRPGStats::typeID && GetStats()->GetTypeID() != NDb::SBridgeRPGStats::typeID )
		return;
	//special for (c) Alexander Vinnikov's method of visibility
	CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > pass;
	GetVisibility( &pass );

	const SVector vStartTile( AICellsTiles::GetTile( pass.GetMinX(), pass.GetMinY() ) );
	std::list<SObjTileInfo> tiles;

	for ( int x = pass.GetMinX(); x < pass.GetMaxX(); x += SConsts::TILE_SIZE )
	{
		for ( int y = pass.GetMinY(); y < pass.GetMaxY(); y += SConsts::TILE_SIZE )
		{
			const CVec2 vTileCenter( AICellsTiles::GetCenterOfTile( x, y ) );
			const SVector tile( AICellsTiles::GetTile( x, y ) );				

			if ( GetAIMap()->IsTileInside( tile ) && pass.GetVal( vTileCenter ) )
			{
				theWarFog.RemoveStaticObjectTile( tile, nUniqueID );
			}
		}
	}

	bTransparencySet = false;

	const CVec3 vCenter( GetCenter() );
	SExecutorEventParam param( EID_ADD_TO_RESTORE_TRANSPARENCY_QUEUE, 0, 0 );
	CExecutorEventAddToRestoreTransparencyQueue event( param, GetUniqueId() );
	theExecutorContainer.RaiseEvent( event );
}

void CGivenPassabilityStObject::RestoreTransparenciesImmidiately()
{
	if ( bTransparencySet )
		SetTransparencies();
}

bool CGivenPassabilityStObject::IsPointInside( const CVec2 &point ) const
{
	if ( boundRect.IsPointInside( point ) )
	{
		const SHPObjectRPGStats *pStats = GetStats();
		if ( pStats == 0 ) 
			return false;
		else if ( checked_cast<const SStaticObjectRPGStats*>(pStats)->GetPassability( GetFrameIndex() ).IsEmpty() )
			return true;
		else
		{
			bool bRet = false;
			if ( g_bNewLock == 0 || GetPassProfile() == 0 )
			{
				CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > pass;
				GetPassability( &pass );
				bRet = pass.GetVal( point );
			}
			else
				bRet = GetPassProfile()->IsPointInside( point );

			if ( bRet )
			{
				//CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW + 1, "Hit" );
			}
			else
			{
				//CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW + 1, "Miss tiles" );
			}
			return bRet;
		}
	}
	else
	{
		//CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW + 1, "Miss Bound Rect" );
		return false;
	}
}

void CGivenPassabilityStObject::GetCoveredTiles( std::list<SVector> *pTiles ) const
{
	if ( g_bNewLock == 0 || GetPassProfile() == 0 )
	{
		const SStaticObjectRPGStats* pStats = checked_cast<const SStaticObjectRPGStats*>( GetStats() );	

		const CVec3 vCenter( GetCenter() );
		const CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > pass( pStats->GetPassability( GetFrameIndex() ), wDir, pStats->GetOrigin( GetFrameIndex() ), CVec2( vCenter.x, vCenter.y ) );
		for ( int x = pass.GetMinX(); x < pass.GetMaxX(); x += SConsts::TILE_SIZE )
		{
			for ( int y = pass.GetMinY(); y < pass.GetMaxY(); y += SConsts::TILE_SIZE )
			{
				const CVec2 vTileCenter( AICellsTiles::GetCenterOfTile( x, y ) );
				if ( pass.GetVal( vTileCenter ) )
				{
					const SVector tile( AICellsTiles::GetTile( x, y ) );				
					if ( !GetAIMap()->IsTileInside( tile ) )
						continue;
					pTiles->push_back( tile );
				}
			}
		}
	}
	else
	{
		const std::vector<SVector> &tiles = GetPassProfile()->GetTilesUnder();
		pTiles->clear();
		pTiles->assign( tiles.begin(), tiles.end() );
	}
}

const CVec2 CGivenPassabilityStObject::GetAttackCenter( const CVec2 &vPoint ) const
{
	std::list<SVector> tiles;
	GetCoveredTiles( &tiles );
	CVec2 vBestPoint;
	CVec2 vMidPoint( 0, 0 );

	if ( tiles.empty() )
		vBestPoint = boundRect.center;
	else 
	{
		float fMinDist2 = -1.0f;
		int nTiles = 0;
		for ( std::list<SVector>::const_iterator iter = tiles.begin(); iter != tiles.end(); ++iter )
		{
			++nTiles;
			const CVec2 vIteratingPoint = AICellsTiles::GetPointByTile( *iter );
			const float fDist2 = fabs2( vIteratingPoint - vPoint );
			vMidPoint += vIteratingPoint;
			if ( fMinDist2 == -1.0f || fDist2 < fMinDist2 )
			{
				fMinDist2 = fDist2;
				vBestPoint = vIteratingPoint;
			}
		}
		// move attack point a bit to the middle
		vMidPoint /= nTiles;
		CVec2 vDirToMid( vMidPoint - vBestPoint );
		if ( vDirToMid != VNULL2 )
		{
			Normalize( &vDirToMid );
			vBestPoint += vDirToMid * SConsts::TILE_SIZE;
		}
	}

	return vBestPoint;
}

//*******************************************************************
//*												 CCommonStaticObject											*
//*******************************************************************

BASIC_REGISTER_CLASS( CCommonStaticObject );

void CCommonStaticObject::Die( const float fDamage )
{
	fHP = 0;
	SetAlive( GetHitPoints() > 0.0f );
	Delete();
}

void CCommonStaticObject::TakeDamage( const float fDamage, const bool bFromExplosion, const int nPlayerOfShoot, CAIUnit *pShotUnit )
{
	if ( bFromExplosion && IsAlive() && fHP > 0 )
	{
		fHP -= fDamage;
		SetAlive( GetHitPoints() > 0.0f );
		if ( fHP <= 0 || theCheats.GetFirstShoot( nPlayerOfShoot ) == 1 )
		{
			updater.AddUpdate( 0, ACTION_NOTIFY_DIE, this, -1 );			
			Die( fDamage );
		}
		else
		{
			updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, this, -1 );
			WasHit();
		}
	}
}

bool CCommonStaticObject::ProcessAreaDamage( const class CExplosion *pExpl, const int nArmorDir, const float fRadius, const float fSmallRadius )
{
	if ( bFallen )
		return false;
	SRect boundRect;
	GetBoundRect( &boundRect );
	const CVec3 vExpl( pExpl->GetExplCoordinates() );
	if ( boundRect.IsIntersectCircle( CVec2(vExpl.x, vExpl.y), fRadius ) )
	{
		if ( GetRandArmorByDir( nArmorDir, pExpl->GetAttackDir() ) <= SConsts::ARMOR_FOR_AREA_DAMAGE )
		{
			TakeDamage( SConsts::AREA_DAMAGE_COEFF * pExpl->GetRandomDamage(), true, pExpl->GetPlayerOfShoot(), pExpl->GetWhoFire() );
			return true;
		}
	}

	return false;
}

void CCommonStaticObject::AnimateFalling( const CVec2 &vFallTo )
{
	NI_ASSERT( CanFall(), "This object cannot fall!" );
	if ( bFallen )
		return;
	bFallen = true;
	CVec2 vIt = CVec2(GetCenter().x,GetCenter().y);
	const float fStartHeight = GetHeights()->GetVisZ( vIt.x, vIt.y );
	vIt = CVec2(GetCenter().x,GetCenter().y) + vFallTo * AI_TILE_SIZE;
	float fMaxTG = ( GetHeights()->GetVisZ( vIt.x, vIt.y ) - fStartHeight ) / AI_TILE_SIZE;
	const float fSearchRadius = GetHeight();
	for ( float i = AI_TILE_SIZE * 2.0f; i <= fSearchRadius; i += AI_TILE_SIZE )
	{
		vIt = CVec2(GetCenter().x,GetCenter().y) + vFallTo * i;
		float fNewTG = ( GetHeights()->GetVisZ( vIt.x, vIt.y ) - fStartHeight ) / i;
		if ( fNewTG > fMaxTG ) 
			fMaxTG = fNewTG;
	}
	SAITreeBrokenUpdate *pUpdate = new SAITreeBrokenUpdate;
	pUpdate->fTg = fMaxTG;
	pUpdate->nObjUniqueID = GetUniqueId();
	pUpdate->vDir = vFallTo;
	updater.AddUpdate( pUpdate, ACTION_NOTIFY_TREE_BROKEN, this, -1 );
}

//*******************************************************************
//*											CSimpleStaticObject													*
//*******************************************************************

bool CSimpleStaticObject::CanUnitGoThrough( const EAIClasses &eClass ) const
{
	return ( pStats->nAIPassabilityClass & eClass ) == 0;
}

bool CSimpleStaticObject::ShouldSuspendAction( const EActionNotify &eAction ) const
{
	return 
		(	CCommonStaticObject::ShouldSuspendAction( eAction ) ||
		eAction == ACTION_NOTIFY_TREE_BROKEN ||
		eAction == ACTION_NOTIFY_NEW_ST_OBJ && bDelayedUpdate );
}

bool CSimpleStaticObject::CanFall()
{
	if ( const SObjectBaseRPGStats *pLocalStats = dynamic_cast_ptr<const SObjectBaseRPGStats*>(pStats) ) 
		return pLocalStats->bCanFall;
	else
		return false;
}

//*******************************************************************
//*											CTerraMeshStaticObject											*
//*******************************************************************

bool CTerraMeshStaticObject::CanUnitGoThrough( const EAIClasses &eClass ) const
{
	return ( pStats->nAIPassabilityClass & eClass ) == 0;
}

void CTerraMeshStaticObject::SetNewPlacement( const CVec3 &center, const WORD dir )
{
	CCommonStaticObject::SetNewPlacement( center, dir );
	wDir = dir;
}

void CTerraMeshStaticObject::GetPlacement( SAINotifyPlacement *pPlacement, const NTimer::STime timeDiff )
{
	CCommonStaticObject::GetPlacement( pPlacement, timeDiff );
	pPlacement->dir = GetDir();
}

bool CTerraMeshStaticObject::CanFall()
{
	if ( const SObjectBaseRPGStats *pLocalStats = dynamic_cast_ptr<const SObjectBaseRPGStats*>(pStats) ) 
		return pLocalStats->bCanFall;
	else
		return false;
}


