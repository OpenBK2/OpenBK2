#include "stdafx.h"

#include "Misc/bresenham.h"
#include "System/Time.h"
#include "Entrenchment.h"
#include "Soldier.h"
#include "NewUpdater.h"
#include "StaticObjects.h"
#include "Diplomacy.h"
#include "Cheats.h"
#include "UnitStates.h"
#include "Statistics.h"
#include "Common_RTS_AI/AIMap.h"
#include "Common_RTS_AI/StaticMapHeights.h"
#include "Misc/Checker.h"
#include "DebugTools/DebugInfoManager.h"
#include "GlobalWarFog.h"

#include <algorithm>
#include <cstdint>

REGISTER_SAVELOAD_CLASS( 0x1108D4CC, CFullEntrenchment );
REGISTER_SAVELOAD_CLASS( 0x1108D4C1, CEntrenchmentTankPit );
REGISTER_SAVELOAD_CLASS( 0x1108D497, CEntrenchment );
REGISTER_SAVELOAD_CLASS( 0x1108D452, CEntrenchmentPart );

extern CEventUpdater updater;
extern CStaticObjects theStatObjs;
extern NTimer::STime curTime;
extern CDiplomacy theDipl;
extern SCheats theCheats;
extern CGlobalWarFog theWarFog;
extern CStatistics theStatistics;

//*******************************************************************
//*											CEntrenchmentPart														*
//*******************************************************************

//BASIC_REGISTER_CLASS( CEntrenchmentPart );
//BASIC_REGISTER_CLASS( CEntrenchmentTankPit );

CEntrenchmentPart::CEntrenchmentPart( const SEntrenchmentRPGStats *_pStats, const CVec3 &_center, const uint16_t _dir, const int nFrameIndex,  float fHP, int nPlayer, bool _bPlayerCreates )
:	CExistingObject( nFrameIndex, fHP ), pStats( _pStats ), center( _center ), dir( _dir ),
	bDigBySegment( false ), bOwnerChanged( false ), bVisible( false ), nextSegmTime( 0 ), 
	bSuspendedAppear( nPlayer != theDipl.GetMyNumber() ),
	bPlayerCreates( _bPlayerCreates )
{
	boundRect.InitRect( VNULL2, VNULL2, 0, 0, 0 );
}

const int CEntrenchmentPart::GetNDefenders() const 
{ 
	NI_ASSERT( pOwner != 0, "entrenchment part without entrenchment" );
	if ( pOwner )
		return pOwner->GetNDefenders(); 
	return 0;
}

void CEntrenchmentPart::Init()
{
	boundRect = CalcBoundRect( CVec2(GetCenter().x, GetCenter().y), dir, GetSegmStats() );
	//DEBUG{
#ifndef _FINALRELEASE
	if ( NGlobal::GetVar( "show_entrenchment_boundrects", 0 ) )
	{
		CSegment segm;
		segm.p1 = boundRect.v1;
		segm.p2 = boundRect.v2;
		segm.dir = segm.p2 - segm.p1;
		DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 1, NDebugInfo::WHITE );
		segm.p1 = boundRect.v2;
		segm.p2 = boundRect.v3;
		segm.dir = segm.p2 - segm.p1;
		DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 1, NDebugInfo::WHITE );
		segm.p1 = boundRect.v3;
		segm.p2 = boundRect.v4;
		segm.dir = segm.p2 - segm.p1;
		DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 1, NDebugInfo::WHITE );
		segm.p1 = boundRect.v4;
		segm.p2 = boundRect.v1;
		segm.dir = segm.p2 - segm.p1;
		DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 1, NDebugInfo::WHITE );
	}
#endif
	//DEBUG}
	bVisible = false;
	bOwnerChanged = false;
	GetAIMap()->GetTilesCoveredByRect( boundRect, &coveredTiles );

	if ( !bPlayerCreates && !bSuspendedAppear )
	{
		bVisible = true;
		updater.AddUpdate( 0, ACTION_NOTIFY_NEW_ST_OBJ, this, -1 );
		updater.AddUpdate( 0, ACTION_NOTIFY_NEW_ENTRENCHMENT, this, 1 );
	}
	nextSegmTime = 0;
}

bool CEntrenchmentPart::CanUnregister() const
{
	for ( std::list<SVector>::const_iterator iter = coveredTiles.begin(); iter != coveredTiles.end(); ++iter )
	{
		const SVector &tile = *iter;		
		if ( !theWarFog.IsTileVisible( tile, 0 ) || !theWarFog.IsTileVisible( tile, 1 ) )
			return false;
	}

	return true;
}

void CEntrenchmentPart::Segment()
{
	// все действия здесь должны быть const, т.к. они различны на разных компах, для multiplayer
	if ( !bVisible )
	{
		for ( std::list<SVector>::const_iterator iter = coveredTiles.begin(); iter != coveredTiles.end(); ++iter )
		{
			if ( theWarFog.IsTileVisible( *iter, theDipl.GetMyParty() ) )
			{
				if ( pFullEntrenchment )
					pFullEntrenchment->SetVisible();
				else if ( pOwner )
					pOwner->SetVisible();
				else
					NI_ASSERT( false, StrFmt( "Entrenchment part without parents, link ID %d", GetLink() ) );
				break;
			}
		}
	}

	// unregister только если виден всеми сторонами - для multiplayer	
	if ( CanUnregister() )
		theStatObjs.UnregisterSegment( this );

	// random вызывать всегда - для mutliplayer
	nextSegmTime = curTime + NRandom::Random( 500, 2000 ); RecordRandomCall();
}

void CEntrenchmentPart::GetPlacement( SAINotifyPlacement *pPlacement, const NTimer::STime timeDiff )
{
	pPlacement->nObjUniqueID = GetUniqueId();
	pPlacement->dir = GetDir();
	pPlacement->center = CVec2(GetCenter().x, GetCenter().y);
	pPlacement->z = GetTerrainHeight( pPlacement->center.x, pPlacement->center.y, timeDiff ) + center.z;
	pPlacement->fSpeed = 0;
	pPlacement->dwNormal = GetHeights()->GetNormal( pPlacement->center.x, pPlacement->center.y );
}

SRect CEntrenchmentPart::CalcBoundRect( const CVec2 & center, const uint16_t _dir, const SEntrenchmentRPGStats::SEntrenchSegmentRPGStats& stats)
{
	SRect r;
	const CVec2 vDir( GetVectorByDirection( _dir + 65535/4 ) );
	const CVec2 dirPerp( vDir.y, -vDir.x );
	const CVec2 vShift( vDir * (stats.vAABBCenter.x /*-stats.vAABBHalfSize.x*/) + dirPerp * stats.vAABBCenter.y );
	r.InitRect( center + vShift, vDir, stats.vAABBHalfSize.x, stats.vAABBHalfSize.y );
	return r;
}

void CEntrenchmentPart::SetNewPlaceWithoutMapUpdate( const CVec3 &_center, const uint16_t _dir )
{ 
	center = _center; 
	dir = _dir; 
}

void CEntrenchmentPart::GetRPGStats( SAINotifyRPGStats *pStats ) 
{ 
	pStats->nObjUniqueID = GetUniqueId();
	pStats->fHitPoints = 1;
	pStats->time = curTime;
}

CVec2 CEntrenchmentPart::GetShift( const CVec2 &vPoint, const CVec2 &vDir ) 
{
	const CVec2 dirPerp( vDir.y, -vDir.x );
	return CVec2( vDir * vPoint.y + dirPerp * vPoint.x );
}

void CEntrenchmentPart::GetCoveredTiles( std::list<SVector> *pTiles ) const
{
	SRect boundRect;
	GetBoundRect( &boundRect );

	GetAIMap()->GetTilesCoveredByRect( boundRect, pTiles );
}

void CEntrenchmentPart::TakeDamage( const float fDamage, const bool bFromExplosion, const int nPlayerOfShoot, CAIUnit *pShotUnit )
{
	// trenches must not die
	if ( pOwner )
		pOwner->TakeDamage( fDamage, bFromExplosion, nPlayerOfShoot, pShotUnit );
}

bool CEntrenchmentPart::IsPointInside( const CVec2 &point ) const
{
	return boundRect.IsPointInside( point );
}

void CEntrenchmentPart::SetVisible( const bool bLastSegment /*=false*/)
{
	if ( !bVisible )
	{
		bVisible = true;
		if ( bSuspendedAppear )
		{
			bDigBySegment = false;
			bSuspendedAppear = false;
		}
		updater.AddUpdate( 0, ACTION_NOTIFY_NEW_ST_OBJ, this, -1 );
	}
	if ( bOwnerChanged )
	{
				
		bOwnerChanged = false;
		int nParam = ( bLastSegment ? 1 : -1 ) * ( bDigBySegment ? 2 : 1 ); 

		if ( GetType() == EST_TERMINATOR && bDigBySegment )			//CRAP: force terminators to dig every time when digging in-game
			nParam = 2;

		updater.AddUpdate( 0, ACTION_NOTIFY_NEW_ENTRENCHMENT, this, nParam );
		bDigBySegment = false;
	}
}

const NTimer::STime CEntrenchmentPart::GetNextSegmentTime() const 
{ 
	return nextSegmTime > curTime ? nextSegmTime : curTime; 
}

//*******************************************************************
//*													CEntrenchment														*
//*******************************************************************

//BASIC_REGISTER_CLASS( CEntrenchment );

CEntrenchment::CEntrenchment( CObjectBase** _segments, const int nLen, CFullEntrenchment *pFullEntrenchment, const bool bPiecewise )
: nBusyFireplaces( 0 ), nextSegmTime( 0 )
{
	SetAlive( true );
	NI_ASSERT( nLen != 0, "Окоп из нуля сегментов" );
	SetUniqueIdForObjects();
	
	insiders.clear();
	fireplaces.reserve( 0 );

	float fLengthAhead = -1, fLengthBack = 0.0f, fWidth = 0.0f;
	CVec2 vDir( VNULL2 ), vDirPerp( VNULL2 ), center( VNULL2 );
	for ( int i = 0; i < nLen; ++i )
	{
		NI_ASSERT( dynamic_cast<const CEntrenchmentPart*>( _segments[i] ) != 0, "Wrong part of entrenchment" );
		CEntrenchmentPart *pPart = checked_cast<CEntrenchmentPart*>( _segments[i] );
	//	NI_ASSERT( pPart->GetOwner() == 0, "Segment of entrenchment exists in two sections" );
		const CVec2 vPartCenter( pPart->GetCenter().x, pPart->GetCenter().y );
		vDir = GetVectorByDirection( pPart->GetDir() );
		vDirPerp = CVec2( -vDir.y, vDir.x );

		if ( pPart->GetType() == EST_FIREPLACE || pPart->GetType() == EST_LINE )
		{
			if ( fLengthAhead == -1 )
			{
				fLengthAhead = pPart->GetSegmStats().vAABBHalfSize.x;
				fLengthBack = pPart->GetSegmStats().vAABBHalfSize.x;
				fWidth = pPart->GetSegmStats().vAABBHalfSize.y;

				center = vPartCenter + GetShift( pPart->GetSegmStats().vAABBCenter, vDir );
				z = 2 * pPart->GetSegmStats().vAABBHalfSize.z;
				pStats = checked_cast<const SEntrenchmentRPGStats*>(pPart->GetStats());
			}
			else
			{
				const float fToCenter = ( vPartCenter + GetShift( pPart->GetSegmStats().vAABBCenter, vDir ) - center ) * vDirPerp;

				float fLength = fToCenter + pPart->GetSegmStats().vAABBHalfSize.y;
				if ( fLength > 0 && fLength > fLengthAhead )
					fLengthAhead = fLength;

				fLength = fToCenter - pPart->GetSegmStats().vAABBHalfSize.y;
				if ( fLength < 0 && -fLength > fLengthBack )
					fLengthBack = -fLength;
			}
			
			for ( int i = 0; i < pPart->GetSegmStats().fireplaces.size(); ++i )
				fireplaces.push_back( SFireplaceInfo( vPartCenter + GetShift( pPart->GetSegmStats().fireplaces[i], vDir ), 0, pPart->GetFrameIndex() ) );
		}

		pPart->SetOwner( this, bPiecewise );
		pPart->SetFullEntrench( pFullEntrenchment );

		segments.push_back( pPart );
		
		theStatObjs.RegisterSegment( pPart );
	}

	if ( pFullEntrenchment )
		pFullEntrenchment->AddEntrenchmentSection( this );

	NI_ASSERT( fLengthAhead != -1, "Окоп без линейных сегментов и fireplaces" );

	// чтобы центр прямоульника был в середине
	const float fLength = ( fLengthAhead + fLengthBack ) / 2;
	rect.InitRect( center + vDirPerp * ( -fLengthBack + fLength ), vDirPerp, fLength, fWidth );

#ifndef _FINALRELEASE
	if ( NGlobal::GetVar( "entrenchment_markers", 0 ) != 0 )
	{
		CSegment segm;
		segm.p1 = rect.v1;
		segm.p2 = rect.v2;
		segm.dir = segm.p2 - segm.p1;
		DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );
		segm.p1 = rect.v2;
		segm.p2 = rect.v3;
		segm.dir = segm.p2 - segm.p1;
		DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );
		segm.p1 = rect.v3;
		segm.p2 = rect.v4;
		segm.dir = segm.p2 - segm.p1;
		DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );
		segm.p1 = rect.v4;
		segm.p2 = rect.v1;
		segm.dir = segm.p2 - segm.p1;
		DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );
	}
#endif
}

const NTimer::STime CEntrenchment::GetNextSegmentTime() const 
{ 
	return nextSegmTime > curTime ? nextSegmTime : curTime; 
}

CAIUnit* CEntrenchment::GetIteratedUnit() 
{ 
	NI_VERIFY( !IsIterateFinished(), "Wrong fire unit to get", return 0 ); 
	return iter->pUnit; 
}

CVec2 CEntrenchment::GetShift( const CVec2 &vPoint, const CVec2 &vDir )
{
	const CVec2 dirPerp( vDir.y, -vDir.x );
	return CVec2( vDir * vPoint.y + dirPerp * vPoint.x );
}

void CEntrenchment::AddSoldier( CSoldier *pUnit )
{
#ifndef _FINALRELEASE
	const int nID = pUnit->GetUniqueId();
#endif
	pUnit->ApplyStatsModifier( pStats->pInnerUnitBonus, true );
	if ( nBusyFireplaces == 0 )
	{
		nextSegmTime = curTime + SConsts::AI_SEGMENT_DURATION - 1;		
		theStatObjs.RegisterSegment( this );
	}
	
	int nFireplace = -1;
	if ( nBusyFireplaces < fireplaces.size() )
	{
		++nBusyFireplaces;
		
		// найти ближайшее к центру юнита свободное fireplace 
		int i = 0;
		float fBestDist = 1e10;
		while ( i < fireplaces.size() )
		{
			if ( fireplaces[i].pUnit == 0 )
			{
				const float fDist = fabs2( pUnit->GetCenterPlain() - fireplaces[i].center );
				if ( fDist < fBestDist )
				{
					fBestDist = fDist;
					nFireplace = i;
				}
			}

			++i;
		}

		NI_ASSERT( nFireplace < fireplaces.size(), "Wrong fireplace number" );

		fireplaces[nFireplace].pUnit = pUnit;
		pUnit->MoveToEntrenchFireplace( CVec3( fireplaces[nFireplace].center, GetHeights()->GetZ( fireplaces[nFireplace].center ) - z ), fireplaces[nFireplace].nFrameIndex );
		insiders.push_front( SInsiderInfo( pUnit, nFireplace ) );
	}
	else
	{
		pUnit->SetNSlot( -1 );
		pUnit->SetToSolidPlace();
		insiders.push_back( SInsiderInfo( pUnit, nFireplace ) );
	}

	CRotatingFireplacesObject::AddUnit( pUnit, nFireplace );
}

void CEntrenchment::AddSoldierToFirePlace( CSoldier *pUnit, const int nFirePlace )
{
	if ( nBusyFireplaces == 0 )
	{
		nextSegmTime = curTime + SConsts::AI_SEGMENT_DURATION - 1;		
		theStatObjs.RegisterSegment( this );
	}

	NI_ASSERT( nBusyFireplaces < fireplaces.size(), "Not enough fireplaces" );
	
	++nBusyFireplaces;
	fireplaces[nFirePlace].pUnit = pUnit;
	CVec3 vNewCenter = GetHeights()->Get3DPoint( fireplaces[nFirePlace].center );
	vNewCenter.z -= z;
	pUnit->MoveToEntrenchFireplace( vNewCenter, fireplaces[nFirePlace].nFrameIndex );
	pUnit->ApplyStatsModifier( pStats->pInnerUnitBonus, true );
	insiders.push_front( SInsiderInfo( pUnit, nFirePlace ) );
	CRotatingFireplacesObject::AddUnit( pUnit, nFirePlace );
}

const CVec2 CEntrenchment::GetFirePlaceCoord( const int nFirePlace )
{
	CheckFixedRange( nFirePlace, GetNFirePlaces(), "entrehcment" );
	return fireplaces[nFirePlace].center;
}

void CEntrenchment::ProcessEmptyFireplace( const int nFireplace )
{
	// есть юниты в резерве, проверка на нахождение в окопе, т.к. он оттуда может уже собираться выходить
	if ( !insiders.empty() && insiders.back().nFireplace == -1 && insiders.back().pUnit->IsInEntrenchment() )
	{
		CSoldier *pUnit = insiders.back().pUnit;
		
		insiders.back().nFireplace = nFireplace;
		fireplaces[nFireplace].pUnit = pUnit;

		pUnit->MoveToEntrenchFireplace( CVec3( fireplaces[nFireplace].center, GetHeights()->GetZ( fireplaces[nFireplace].center ) - z ), fireplaces[nFireplace].nFrameIndex );

		insiders.push_front( insiders.back() );
		insiders.pop_back();
	}
	else
	{
		NI_ASSERT( nBusyFireplaces > 0, "Wrong number of fireplaces" );
		--nBusyFireplaces;
	}
}

void CEntrenchment::DelSoldier( CSoldier *pUnit, const bool bFillEmptyFireplace )
{
	std::list< SInsiderInfo >::iterator iter = insiders.begin();
	while ( iter != insiders.end() && iter->pUnit != pUnit )
		++iter;

	if ( iter != insiders.end() )
	{
		int nFireplace = iter->nFireplace;
		// сидит в fireplace
		if ( nFireplace != -1 )
			fireplaces[nFireplace].pUnit = 0;

		insiders.erase( iter );

		if ( nFireplace != -1 && bFillEmptyFireplace )
			ProcessEmptyFireplace( nFireplace );
		if ( nFireplace != -1 && !bFillEmptyFireplace )
			--nBusyFireplaces;

		CRotatingFireplacesObject::DeleteUnit( pUnit );
		pUnit->ApplyStatsModifier( pStats->pInnerUnitBonus, false );
		pUnit->SetNSlot( -1 );
	}
}

void CEntrenchment::TakeDamage( const float fDamage, const bool bFromExplosion, const int nPlayerOfShoot, CAIUnit *pShotUnit )
{
	// Designers requested this feature removed - 06.04-2005
	return;

	/*list< CPtr<CSoldier> > dead;	
	
	// все убиты
	if ( fDamage >= pStats->fMaxHP || theCheats.GetFirstShoot( nPlayerOfShoot ) == 1 )
	{
		for ( list<SInsiderInfo>::iterator iter = insiders.begin(); iter != insiders.end(); ++iter )
			dead.push_back( iter->pUnit );
	}
	else
	{
		const float fProbability = fDamage / pStats->fMaxHP;

		for ( list<SInsiderInfo>::iterator iter = insiders.begin(); iter != insiders.end(); ++iter )
		{
			RecordRandomCall();
			if ( NRandom::Random( 0.0f, 1.0f ) < fProbability )
				dead.push_back( iter->pUnit );
		}
	}

	for ( list< CPtr<CSoldier> >::iterator iter = dead.begin(); iter != dead.end(); ++iter )
	{
		CSoldier *pSoldier = *iter;

		if ( pShotUnit )
			pShotUnit->EnemyKilled( pSoldier );
		
		theStatistics.UnitKilled( nPlayerOfShoot, pSoldier->GetPlayer(), pSoldier->GetStats()->fExpPrice, pShotUnit ? pShotUnit->GetReinforcementType() : NDb::_RT_NONE, pSoldier->GetReinforcementType() );
		theMPInfo.UnitsKilled( nPlayerOfShoot, pSoldier->GetStats()->fPrice, pSoldier->GetPlayer() );

		pSoldier->Die( false, 0 );
	}*/
}

void CEntrenchment::GetCoveredTiles( std::list<SVector> *pTiles ) const
{
	GetAIMap()->GetTilesCoveredByRect( rect, pTiles );
}

void CEntrenchment::Iterate() 
{ 
	if ( iter != insiders.end() )
	{
		++iter;
		while ( iter != insiders.end() && iter->nFireplace == -1 ) 
			++iter; 
	}
}

const int CEntrenchment::GetNDefenders() const
{
	return insiders.size();
}

CSoldier* CEntrenchment::GetUnit( const int n ) const
{
	NI_ASSERT( n < GetNDefenders(), "Wrong unit to get from entrenchment" );

	std::list<SInsiderInfo>::const_iterator iter = insiders.begin();
	advance( iter, n );
	return iter->pUnit;
}

const uint8_t CEntrenchment::GetPlayer() const
{
	if ( GetNDefenders() ==0 || insiders.empty() )
		return theDipl.GetNeutralPlayer();
	else
		return insiders.begin()->pUnit->GetPlayer();
}

void CEntrenchment::Segment() 
{ 
	nextSegmTime = curTime + NRandom::Random( 200, 1000 ); RecordRandomCall();

	if ( !IsUnitsInside() )
		theStatObjs.UnregisterSegment( this );
	else
	{
		if ( !CStormableObject::Segment() )
			CRotatingFireplacesObject::Segment();
	}
}

void CEntrenchment::Delete()
{
	theStatObjs.DeleteInternalEntrenchmentInfo( this );
}

void CEntrenchment::GetClosestPoint( const CVec2 &vPoint, CVec2 *pvResult ) const
{
	const CSegment rectSegment( rect.center - rect.dir * rect.lengthBack, rect.center + rect.dir * rect.lengthAhead );
	rectSegment.GetClosestPoint( vPoint, pvResult );
}

bool CEntrenchment::CanRotateSoldier( CSoldier *pSoldier ) const
{
	return pSoldier->GetState() && pSoldier->GetState()->IsRestState();
}

void CEntrenchment::ExchangeUnitToFireplace( CSoldier *pSoldier, int nFirePlace )
{
	NI_ASSERT( nFirePlace < GetNFirePlaces(), StrFmt( "Wrong number of fireplace (%d), number of fireplaces (%d)", nFirePlace, GetNFirePlaces() ) );

	CSoldier *pDeletedSoldier = GetSoldierInFireplace( nFirePlace );

	if ( pDeletedSoldier )
		DelSoldier( pDeletedSoldier, false );
	DelSoldier( pSoldier, false );


	AddSoldierToFirePlace( pSoldier, nFirePlace );
	if ( pDeletedSoldier )
		AddSoldier( pDeletedSoldier );
}

const int CEntrenchment::GetNFirePlaces() const
{
	return fireplaces.size();
}

CSoldier* CEntrenchment::GetSoldierInFireplace( const int nFireplace) const
{
	return fireplaces[nFireplace].pUnit;
}

const CVec3& CEntrenchment::GetCenter() const 
{ 
	static CVec3 vCenter;
	vCenter = CVec3( rect.center, z );
	return vCenter; 
}

void CEntrenchment::SetVisible()
{
	CEntrenchmentPart *pPrevIter = 0;
	for ( CSegmentList::iterator iter = segments.begin(); iter != segments.end(); ++iter )
	{
		if ( pPrevIter ) 
			pPrevIter->SetVisible();

		pPrevIter = (*iter);
	}

	if ( pPrevIter )			//Display last part
		pPrevIter->SetVisible( true );
}

//*******************************************************************
//*											CEntrenchmentTankPit												*
//*******************************************************************

CEntrenchmentTankPit::CEntrenchmentTankPit( const SMechUnitRPGStats *_pStats,
												const CVec3& center,
												const uint16_t dir,
												const int nFrameIndex,  const class CVec2 &vHalfSize,
												const std::list<SObjTileInfo> &tiles,
												class CAIUnit *_pOwner )
: wDir( dir ), vHalfSize( vHalfSize ), pStats( _pStats ), pOwner( _pOwner ), tilesToLock( tiles ),
	CGivenPassabilityStObject( center, 1.0f, dir, nFrameIndex )
{
	boundRect.InitRect( CVec2(center.x, center.y), GetVectorByDirection(dir), vHalfSize.y, vHalfSize.x );
	SetUniqueIdForObjects();
}

CEntrenchmentTankPit::~CEntrenchmentTankPit()
{
}

void CEntrenchmentTankPit::GetTilesForVisibility( CTilesSet *pTiles ) const
{
	pTiles->clear();
	for ( std::list<SObjTileInfo>::const_iterator i = tilesToLock.begin(); tilesToLock.end() != i; ++i )
	{
		const SVector &v = i->tile;
		if ( GetAIMap()->IsTileInside( v ) )
			pTiles->push_back( v );
	}
}

void CEntrenchmentTankPit::GetCoveredTiles( std::list<SVector> *pTiles ) const
{
	pTiles->clear();
	for ( std::list<SObjTileInfo>::const_iterator i = tilesToLock.begin(); tilesToLock.end() != i; ++i )
	{
		const SVector &v = i->tile;
		if ( GetAIMap()->IsTileInside( v ) )
			pTiles->push_back( v );
	}
}

void CEntrenchmentTankPit::GetNewUnitInfo( struct SNewUnitInfo *pNewUnitInfo )
{
	CGivenPassabilityStObject::GetNewUnitInfo( pNewUnitInfo );
	pNewUnitInfo->fResize = vHalfSize.y / pStats->vAABBHalfSize.y;
	pNewUnitInfo->dwNormal = GetHeights()->GetNormal( pNewUnitInfo->center.x, pNewUnitInfo->center.y );
	pNewUnitInfo->nExpLevel = 0;
}

void CEntrenchmentTankPit::CreateLockedTilesInfo( std::list<SObjTileInfo> *pTiles )
{
	*pTiles = tilesToLock;
}

void CEntrenchmentTankPit::LockTiles()
{
	GetTerrain()->AddStaticObjectTiles( tilesToLock );
}

bool CEntrenchmentTankPit::ShouldSuspendAction( const EActionNotify &eAction ) const
{
	return //eAction == ACTION_NOTIFY_NEW_ST_OBJ ||
		CGivenPassabilityStObject::ShouldSuspendAction( eAction );
}

void CEntrenchmentTankPit::UnlockTiles() 
{
	GetTerrain()->RemoveStaticObjectTiles( tilesToLock );
}

void CEntrenchmentTankPit::Die( const float fDamage )
{
	Delete();
}

void CEntrenchmentTankPit::TakeDamage( const float fDamage, const bool bFromExplosion, const int nPlayerOfShoot, CAIUnit *pShotUnit )
{
	//не разрушается
}

//*******************************************************************
//*												 CFullEntrenchment												*
//*******************************************************************

//BASIC_REGISTER_CLASS( CFullEntrenchment );

void CFullEntrenchment::AddEntrenchmentSection( class CEntrenchment *pSection )
{
	sections.push_back( pSection );
}

void CFullEntrenchment::SetVisible()
{
	for ( CSectionList::iterator it = sections.begin(); it != sections.end(); ++it )
	{
		CEntrenchment *pSection = *it;

		pSection->SetVisible();
	}
}


