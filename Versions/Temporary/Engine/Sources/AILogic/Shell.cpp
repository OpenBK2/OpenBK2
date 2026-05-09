#include "stdafx.h"

#include "System/Time.h"
#include "Shell.h"
#include "AIUnit.h"
#include "Randomize.h"
#include "UnitsIterators2.h"
#include "NewUpdater.h"
#include "Guns.h"
#include "HitsStore.h"
#include "StaticObject.h"
#include "CombatEstimator.h"
#include "GlobalWarFog.h"
#include "Weather.h"
#include "DifficultyLevel.h"
#include "Cheats.h"
#include "StaticObjectsIters.h"
#include "AIGeometry.h"
//#include "..\Scene\Scene.h"
#include "SimpleChecksumCalc.h"

#include "Common_RTS_AI/CheckSums.h"
#include "Common_RTS_AI/StaticMapHeights.h"
#include "DebugTools/DebugInfoManager.h"

REGISTER_SAVELOAD_CLASS( 0x1108D4E3, CHitInfo );
REGISTER_SAVELOAD_CLASS( 0x1108D446, CFakeBallisticTraj );
REGISTER_SAVELOAD_CLASS( 0x1108D447, CBombBallisticTraj );
REGISTER_SAVELOAD_CLASS( 0x1108D448, CBallisticTraj );
REGISTER_SAVELOAD_CLASS( 0x19184C00, CAARocketTraj );
REGISTER_SAVELOAD_CLASS( 0x1108D449, CVisShell );
REGISTER_SAVELOAD_CLASS( 0x1108D44A, CInvisShell );
REGISTER_SAVELOAD_CLASS( 0x1108D44B, CBurstExpl );
REGISTER_SAVELOAD_CLASS( 0x1108D44C, CCumulativeExpl );
REGISTER_SAVELOAD_CLASS( 0x11230D00, CFlameThrowerExpl );

extern CCombatEstimator theCombatEstimator;
extern CEventUpdater updater;
extern NTimer::STime curTime;
extern CStaticObjects theStatObjs;
extern CHitsStore theHitsStore;
CShellsStore theShellsStore;
extern CDiplomacy theDipl;
extern CGlobalWarFog theWarFog;
extern CWeather theWeather;
extern CDifficultyLevel theDifficultyLevel;
extern SCheats theCheats;

//*******************************************************************
//*														CHitInfo															*
//*******************************************************************

CHitInfo::CHitInfo( const class CExplosion *pExpl, CObjectBase *_pVictim, const enum SAINotifyHitInfo::EHitType &_eHitType, const CVec3 &_explCoord )
: pWeapon( pExpl->GetWeapon() ), wShell( pExpl->GetShellType() ), wDir( pExpl->GetAttackDir() ), 
	pVictim( _pVictim ), eHitType( _eHitType ), explCoord( _explCoord )
{
	SetUniqueIdForObjects();
}

void CHitInfo::GetHitInfo( SAINotifyHitInfo *pHitInfo ) const 
{ 
	pHitInfo->explCoord = explCoord;
	pHitInfo->nVictimUniqueID = 
		pVictim && pVictim->IsRefValid() ? dynamic_cast_ptr<CUpdatableObj*>(pVictim)->GetUniqueId() : 0;
	pHitInfo->pWeapon = pWeapon;
	pHitInfo->wDir = wDir;
	pHitInfo->wShell = wShell;
	pHitInfo->eHitType = eHitType;
}

//*******************************************************************
//*												CExplosion																*
//*******************************************************************

void CExplosion::Init(	CAIUnit *_pUnit, 
												const SWeaponRPGStats *_pWeapon, 
												const float fDispersion, 
												const float fDispRatio,
												const CVec3 &_explCoord, 
												const CVec3 &attackerPos, 
												const BYTE _nShellType, 
												const bool bRandomize, 
												const int _nPlayerOfShoot )
{
	pUnit = _pUnit;
	pWeapon = _pWeapon;
	nShellType = _nShellType;
	nPlayerOfShoot = _nPlayerOfShoot;

	CVec2 vRand( VNULL2 );
	const CVec3 vDiff( _explCoord - attackerPos );

	if ( bRandomize )
	{
		const float fFireRangeMax = GetFireRangeMax( pWeapon, pUnit );
		const float fDispRadius = GetDispByRadius( fDispersion, fFireRangeMax, fabs( vDiff ) );
		RandQuadrInCircle( fDispRadius, &vRand, fDispRatio, CVec2(vDiff.x, vDiff.y) );
	}

	explCoord = _explCoord + CVec3( vRand, 0 );
	attackDir = GetDirectionByVector( - vDiff.x, - vDiff.y );
}

CExplosion::CExplosion( CAIUnit *pUnit, const SWeaponRPGStats *pWeapon, const CVec3 &explCoord, const CVec3 &attackerPos, const BYTE nShellType, const bool bRandomize )
{
	if ( pUnit != 0 )
		Init( pUnit, pWeapon, pWeapon->fDispersion, 1, explCoord, attackerPos, nShellType, bRandomize, pUnit->GetPlayer() );
	else
		Init( pUnit, pWeapon, pWeapon->fDispersion, 1, explCoord, attackerPos, nShellType, bRandomize, theDipl.GetNeutralPlayer() );
}

CExplosion::CExplosion( CAIUnit *pUnit, const CBasicGun *pGun, const CVec3 &explCoord, const CVec3 &attackerPos, const BYTE nShellType, const bool bRandomize )
{
	float fDispRatio = pGun->GetDispRatio( nShellType, fabs(explCoord-attackerPos) );
	if ( pUnit != 0 )
		Init( pUnit, pGun->GetWeapon(), pGun->GetDispersion(), fDispRatio, explCoord, attackerPos, nShellType, bRandomize, pUnit->GetPlayer() );
	else
		Init( pUnit, pGun->GetWeapon(), pGun->GetDispersion(), fDispRatio, explCoord, attackerPos, nShellType, bRandomize, theDipl.GetNeutralPlayer() );
}

const SAINotifyHitInfo::EHitType CExplosion::ProcessExactHit( CAIUnit *pTarget, const SRect &combatRect, const CVec3 &explCoord, const int nRandPiercing, const int nRandArmor ) const
{
	// попали по комбат системе
	if ( combatRect.IsPointInside( CVec2( explCoord.x, explCoord.y )  ) )
	{
		// пробили
		if ( nRandPiercing >= nRandArmor && !pTarget->IsSavedByCover() )
			return SAINotifyHitInfo::EHT_HIT;
		else
			return SAINotifyHitInfo::EHT_REFLECT;
	}
	else
		return SAINotifyHitInfo::EHT_MISS;
}

const int CExplosion::GetRandomPiercing() const
{
	if ( pUnit )
		return pUnit->GetStatsModifier()->weaponPiercing.Get( pWeapon->shells[nShellType].GetRandomPiercing() );
	else
		return pWeapon->shells[nShellType].GetRandomPiercing();		//For non-weapon damage (e.g. mines)
}

float CExplosion::GetMaxDamage() const
{
	if ( pUnit )
		return pUnit->GetStatsModifier()->weaponDamage.Get( pWeapon->shells[nShellType].fDamagePower 
			+ pWeapon->shells[nShellType].nDamageRandom );
	else
		return pWeapon->shells[nShellType].fDamagePower + pWeapon->shells[nShellType].nDamageRandom;		//For non-weapon damage (e.g. mines)
}

const float CExplosion::GetRandomDamage() const
{
	if ( pUnit )
		return
		pUnit->GetStatsModifier()->weaponDamage.Get( pWeapon->shells[nShellType].GetRandomDamage() );
	else
		return pWeapon->shells[nShellType].GetRandomDamage();		//For non-weapon damage (e.g. mines)
}

const int CExplosion::GetPartyOfShoot() const 
{
	NI_ASSERT( nPlayerOfShoot != -1, "Invalid shooting player" );
	return theDipl.GetNParty( nPlayerOfShoot );
}

const int CExplosion::GetPlayerOfShoot() const
{
	NI_ASSERT( nPlayerOfShoot != -1, "Invalid shooting player" );
	return nPlayerOfShoot; 
}

bool CExplosion::ProcessSmokeScreenExplosion() const
{
	const SWeaponRPGStats::SShell &rShell = pWeapon->shells[nShellType];
	if ( rShell.eDamageType == NDb::SWeaponRPGStats::SShell::DAMAGE_FOG )
	{
		// большой радиус взрыва - радиус завесы,
		// nPiercing - прозрачность,
		// fDamage - время существования
		theStatObjs.AddNewSmokeScreen(
			GetExplCoordinates(),
			pWeapon->shells[nShellType].fArea,
			pWeapon->shells[nShellType].nPiercing,
			pWeapon->shells[nShellType].fDamagePower );

		return true;
	}

	return false;
}

void CExplosion::AddHitToSend( CHitInfo *pHit )
{
	//чтобы удалилось
	CPtr<CHitInfo> memHit = pHit;
	if ( pHitToSend == 0 || pHitToSend->eHitType != SAINotifyHitInfo::EHT_HIT )
		pHitToSend = pHit;
}

//*******************************************************************
//*												CCumulativeExpl														*
//*******************************************************************

CCumulativeExpl::CCumulativeExpl( CAIUnit *pUnit, const CBasicGun *pGun, const CVec3 &explCoord, const CVec3 &attackerPos, const BYTE nShellType, const bool bRandomize )
: CExplosion( pUnit, pGun, explCoord, attackerPos, nShellType, bRandomize )
{
	if ( pUnit && pUnit->GetZ() > GetExplCoordinates().z )
		nArmorDir = 2;
	else
		nArmorDir = 0;
}

const SAINotifyHitInfo::EHitType GetHitType( const CVec2 &vPoint )
{
	const SVector hitTile( AICellsTiles::GetTile( vPoint ) );
	const ETerrainTypes eType = GetTerrain()->GetTerrainType( hitTile.x, hitTile.y );

	switch ( eType )
	{
		case ETT_EARTH_TERRAIN:			return SAINotifyHitInfo::EHT_GROUND;
		case ETT_MARINE_TERRAIN:		return SAINotifyHitInfo::EHT_NONE;
		case ETT_WATER_TERRAIN:			return SAINotifyHitInfo::EHT_WATER;
	}

	return SAINotifyHitInfo::EHT_NONE;
}

void CCumulativeExpl::Explode()
{
	const CVec3 vExplCoord3D = GetExplCoordinates();
	const CVec2 vExplCoord( vExplCoord3D.x, vExplCoord3D.y );
	const float fExplTerrainZ = GetHeights()->GetVisZ( vExplCoord.x, vExplCoord.y );

	if ( ProcessSmokeScreenExplosion() ) 
	{
		updater.AddUpdate( 0, ACTION_NOTIFY_HIT, new CHitInfo( pWeapon, nShellType, attackDir, vExplCoord3D,	GetHitType( vExplCoord ) ), -1 );
		return;
	}

	theHitsStore.AddHit( vExplCoord, CHitsStore::EHT_OPEN_SIGHT );

	bool bHit = false;
	bool bSoldierHit = false;
	
	// по юнитам
	for ( CUnitsIter<0,0> iter( 0, ANY_PARTY, vExplCoord, 0.0f ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pTarget = *iter;
		if ( IsValidObj( pTarget ) && pUnit != pTarget )
		{
			if ( nShellType == NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE || nShellType == NDb::SWeaponRPGStats::SShell::TRAJECTORY_GRENADE )
				pTarget->Grazed( pUnit );
			
			// target жив, target не тот, кто стрелял и по высоте совпадает с высотой взрыва
			if ( !bSoldierHit || !pTarget->GetStats()->IsInfantry() )
			{
				// чтобы не пропускался вызов функции из-за оптимизации вычисления bool выражений
				const bool bExplResult = pTarget->ProcessCumulativeExpl( this, nArmorDir, false );
				bHit = bHit || bExplResult;

				bSoldierHit = bSoldierHit || bExplResult && pTarget->GetStats()->IsInfantry();
				
				if ( nShellType == NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE || nShellType == NDb::SWeaponRPGStats::SShell::TRAJECTORY_GRENADE )
				{
					CAIUnit *pWhoFire = GetWhoFire();
					if ( IsValidObj( pWhoFire ) )
						pWhoFire->WantedToReveal( pTarget );
				}
			}
		}
	}

	if ( InOnGround( fExplTerrainZ ) )
	{
		// нельзя создавать 2 итератора по статическим объектам, внутри ProcessCumulativeExpl
		// итератор нужен, значит здесь нельзя заводить итератор.
		std::list<CExistingObject*> hitObjects;
		
		// по статическим объектам
		for ( CStObjCircleIter<false> iter( vExplCoord, 0 ); !iter.IsFinished(); iter.Iterate() )
		{
			CExistingObject *pObj = *iter;
			if ( pObj->IsAlive() )
				hitObjects.push_back( pObj );
		}
		
		for ( std::list<CExistingObject*>::iterator it = hitObjects.begin(); it != hitObjects.end(); ++it )
		{
			// чтобы не пропускался вызов функции из-за оптимизации вычисления bool выражений			
			const bool bExplResult = (*it)->ProcessCumulativeExpl( this, nArmorDir, false );
			bHit = bHit || bExplResult;
		}
	}
	
	// ни в кого не попало
	if ( !bHit )
	{
		if ( InOnGround( fExplTerrainZ ) )
			updater.AddUpdate( 0, ACTION_NOTIFY_HIT, new CHitInfo( pWeapon, nShellType, attackDir, vExplCoord3D,	GetHitType( vExplCoord ) ), -1 );
		else
			updater.AddUpdate( 0, ACTION_NOTIFY_HIT, 
			new CHitInfo( pWeapon, nShellType, attackDir, vExplCoord3D,	SAINotifyHitInfo::EHT_AIR ), -1 );
	}
	else if ( pHitToSend != 0 )
		updater.AddUpdate( 0, ACTION_NOTIFY_HIT, pHitToSend, -1 );
}

//*******************************************************************
//*												CBurstExpl																*
//*******************************************************************

CBurstExpl::CBurstExpl( CAIUnit *pUnit, const CBasicGun *pGun, const CVec3 &explCoord, const CVec3 &attackerPos, const BYTE nShellType, const bool bRandomize, const int ArmorDir, const bool _bShowEffect )
: CExplosion( pUnit, pGun, explCoord, attackerPos, nShellType, bRandomize ), nArmorDir( ArmorDir ), bShowEffect( _bShowEffect )
{
	if ( pWeapon->shells[nShellType].etrajectory != NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE || (pUnit && pUnit->GetZ() > GetExplCoordinates().z) )
		nArmorDir = 2;
}

CBurstExpl::CBurstExpl( CAIUnit *pUnit, const SWeaponRPGStats *pWeapon, const CVec3 &explCoord, const CVec3 &attackerPos, const BYTE nShellType, const bool bRandomize, const int ArmorDir, const bool _bShowEffect )
: CExplosion( pUnit, pWeapon, explCoord, attackerPos, nShellType, bRandomize ), nArmorDir( ArmorDir ), bShowEffect( _bShowEffect )
{ 
	if ( pWeapon->shells[nShellType].etrajectory != NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE || (pUnit && pUnit->GetZ() > GetExplCoordinates().z) )
		nArmorDir = 2;
}

void CBurstExpl::Explode()
{
	const CVec3 vExplCoord = GetExplCoordinates();
	const CVec2 explCoord( vExplCoord.x, vExplCoord.y );
	const float fExplTerrainZ = GetHeights()->GetVisZ( vExplCoord.x, vExplCoord.y );

	if ( ProcessSmokeScreenExplosion() ) 
	{
		updater.AddUpdate( 0, ACTION_NOTIFY_HIT, new CHitInfo( pWeapon, nShellType, attackDir, vExplCoord, GetHitType( explCoord ) ), -1 );
		return;
	}
		
	if ( nArmorDir != 2 )
		theHitsStore.AddHit( explCoord, CHitsStore::EHT_OPEN_SIGHT );
	else
		theHitsStore.AddHit( explCoord, CHitsStore::EHT_OVER_SIGHT );
	
	const float &fRadius = pWeapon->shells[nShellType].fArea2;
	const float &fSmallRadius = pWeapon->shells[nShellType].fArea;
	NI_ASSERT( fRadius != 0, "Неверный тип взрыва" );

	bool bHit = false;
	// по юнитам
	for ( CUnitsIter<0,0> iter( 0, ANY_PARTY, explCoord, fRadius ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pTarget = *iter;
		if ( IsValidObj( pTarget ) )
		{
			if ( pTarget != pUnit &&
					 ( pWeapon->shells[nShellType].etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE || 
					 pWeapon->shells[nShellType].etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_GRENADE ) )
				pTarget->Grazed( pUnit );

			// чтобы не пропускался вызов функции из-за оптимизации вычисления bool выражений
			const bool bExplResult = pTarget->ProcessBurstExpl( this, nArmorDir, fRadius, fSmallRadius );
			bHit = bHit || bExplResult;

			if ( pWeapon->shells[nShellType].etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE || 
				pWeapon->shells[nShellType].etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_GRENADE )
			{
				CAIUnit *pWhoFire = GetWhoFire();
				if ( IsValidObj( pWhoFire ) )
					pWhoFire->WantedToReveal( pTarget );
			}
		}
	}

	if ( InOnGround( fExplTerrainZ ) )
	{	
		// по статическим объектам
		// нельзя создавать 2 итератора по статическим объектам, внутри ProcessCumulativeExpl
		// итератор нужен, значит здесь нельзя заводить итератор.
		std::list<CExistingObject*> hitObjects;
		
		// по статическим объектам
		for ( CStObjCircleIter<false> iter( explCoord, fSmallRadius + 300.0f ); !iter.IsFinished(); iter.Iterate() )
		{
			CExistingObject *pObj = *iter;
			if ( IsValidObj( pObj ) )
				hitObjects.push_back( pObj );
		}
		for ( std::list<CExistingObject*>::iterator it = hitObjects.begin(); it != hitObjects.end(); ++it )
		{
			// чтобы не пропускался вызов функции из-за оптимизации вычисления bool выражений			
			const bool bExplResult = (*it)->ProcessBurstExpl( this, nArmorDir, fRadius, fSmallRadius );
			bHit = bHit || bExplResult;
		}
	}

	// Torpedoes - special case - always do surface explosion
	if ( pWeapon->shells[nShellType].etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_TORPEDO )
		bHit = false;

	// так никуда и не попали
	int nExplodeShellType = nShellType;
	if ( !bShowEffect && pWeapon->shells[nShellType].etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_BOMB &&
		nShellType+1 < pWeapon->shells.size() && pWeapon->shells[nShellType+1].etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_BOMB )
		nExplodeShellType = nShellType + 1;
	if ( !bHit )
	{
		if ( InOnGround( fExplTerrainZ ) ) 
		{
			updater.AddUpdate( 0, ACTION_NOTIFY_HIT, new CHitInfo( pWeapon, nExplodeShellType, attackDir, CVec3( explCoord, fExplTerrainZ ), GetHitType( explCoord ) ), -1 );
			//CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW + 4, StrFmt( "CBurstExpl Miss Ground") );
		}
		else
		{
			updater.AddUpdate( 0, ACTION_NOTIFY_HIT, new CHitInfo( pWeapon, nExplodeShellType, attackDir, vExplCoord, SAINotifyHitInfo::EHT_AIR ), -1 );
			//CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW + 4, StrFmt( "CBurstExpl Miss Air") );
		}
	}
	else if ( pHitToSend != 0 )
	{
		updater.AddUpdate( 0, ACTION_NOTIFY_HIT, pHitToSend, -1 );
		//CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW + 4, StrFmt( "CBurstExpl Reflect") );
	}
}

// CFlameThrowerExpl

CFlameThrowerExpl::CFlameThrowerExpl( CAIUnit *pUnit, const class CBasicGun *pGun,
									const CVec3 &explCoord, const CVec3 &attackerPos, 
									const BYTE nShellType, const bool bRandomize )
									: CExplosion( pUnit, pGun, explCoord, 
																attackerPos, nShellType, bRandomize ),
									vShooterPos( attackerPos ), vTargetPos( explCoord )
{
}

void CFlameThrowerExpl::Explode()
{
	CVec3 vDir ( vTargetPos - vShooterPos );
	const float fLength( fabs( vDir ) );
	Normalize( &vDir );
	// create nuber of cumulative explosions along the trajectory and explode them
	const float &fRadius = pWeapon->shells[nShellType].fArea;

	
	for ( float fL = 0; fL < fLength; fL += fRadius )
	{
		const CVec3 vPos( GetHeights()->GetGroundPoint( vDir * fL + vShooterPos ) );
		CPtr<CExplosion> pE = new CBurstExpl( GetWhoFire(), GetWeapon(), vPos, vShooterPos, GetShellType(), true, 0, true );
		pE->Explode();
#ifndef _FINALRELEASE
		if ( NGlobal::GetVar( "flamethrower_explotions_show", 0 ) )
		{
			CSegment segm;
			segm.p1 = CVec2( vPos.x + 10, vPos.y + 10 );
			segm.p2 = CVec2( vPos.x - 10, vPos.y - 10 );
			segm.dir = segm.p2 - segm.p1;
			DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );
			segm.p1 = CVec2( vPos.x + 10, vPos.y - 10 );
			segm.p2 = CVec2( vPos.x - 10, vPos.y + 10 );
			segm.dir = segm.p2 - segm.p1;
			DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );
		}
#endif
	}

	const CVec3 vPos( GetHeights()->GetGroundPoint( vShooterPos + fLength * vDir ) );

	CPtr<CExplosion> pE = new CBurstExpl( GetWhoFire(), GetWeapon(), 
		vPos, vShooterPos, GetShellType(), true, 0, true );
	pE->Explode();
#ifndef _FINALRELEASE
	if ( NGlobal::GetVar( "flamethrower_explotions_show", 0 ) )
	{
		CSegment segm;
		segm.p1 = CVec2( vPos.x + 10, vPos.y + 10 );
		segm.p2 = CVec2( vPos.x - 10, vPos.y - 10 );
		segm.dir = segm.p2 - segm.p1;
		DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );
		segm.p1 = CVec2( vPos.x + 10, vPos.y - 10 );
		segm.p2 = CVec2( vPos.x - 10, vPos.y + 10 );
		segm.dir = segm.p2 - segm.p1;
		DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );
	}
#endif

}

//*******************************************************************
//*													CShell																	*
//*******************************************************************

CShell::CShell( const NTimer::STime &_explTime, CExplosion *_expl, const int _nGun )
: explTime( _explTime ), expl( _expl ), nGun( _nGun )
{
	CAIUnit *pWhoFire = expl->GetWhoFire();
	const CVec3 vOwnerCenter = ( pWhoFire == 0 ) ? expl->GetExplCoordinates() : pWhoFire->GetCenter();

	vStartVisZ = GetHeights()->GetVisZ( vOwnerCenter.x, vOwnerCenter.y );

	const CVec3 vExplCoord( expl->GetExplCoordinates() );
	vFinishVisZ = GetHeights()->GetVisZ( vExplCoord.x, vExplCoord.y );
}

CObjectBase* CShell::GetWhoFired() const 
{ 
	return expl->GetWhoFire(); 
}

//*******************************************************************
//*												CVisShell																	*
//*******************************************************************

CVisShell::CVisShell( CExplosion *_expl, IBallisticTraj *_pTraj, const int nGun, const int _nPlatform )
: CShell( _pTraj->GetExplTime(), _expl, nGun ), pTraj( _pTraj ),
	center( _pTraj->GetStartPoint() ), speed( VNULL3 ), bVisible( false ),
	nPlatform( _nPlatform )
{ 
	NI_ASSERT( pTraj != 0, "trajectory cannot be null" );
	SetUniqueIdForObjects(); 
}

void CVisShell::GetPlacement(  SAINotifyPlacement *pPlacement, const NTimer::STime timeDiff )
{
	pPlacement->bNewFormat = true;
	pPlacement->nObjUniqueID = GetUniqueId();

	CVec3 vSpeed3;
	GetSpeed3( &vSpeed3 );
	pPlacement->vPlacement = center - timeDiff * vSpeed3;
	CVec3 vNormale = (vSpeed3 ^ V3_AXIS_Z) ^ vSpeed3;

	MakeQuatBySpeedAndNormale( &pPlacement->rotation, vSpeed3, vNormale );
}

const bool CVisShell::IsVisibleByPlayer() const
{
	return bVisible;
}

void CVisShell::CalcVisibility()
{
	const bool bVisibleByPlayer = theWarFog.IsTileVisible( AICellsTiles::GetTile( center.x, center.y ), theDipl.GetMyParty() );
	if ( bVisible != bVisibleByPlayer )
	{
		bVisible = bVisibleByPlayer;
		updater.AddUpdate( 0, ACTION_NOTIFY_CHANGE_VISIBILITY, this, IsVisibleByPlayer() );
	}
}

void CVisShell::Segment()
{
	NI_ASSERT( pTraj != 0, "Trajectory can't be null!" );
	if ( pTraj == 0 ) 
		return;
	//
	const CVec3 oldCenter( center );
	center = pTraj->GetCoordinates();
	if ( center == oldCenter )
		return;
	speed = ( center - oldCenter ) / SConsts::AI_SEGMENT_DURATION;
	updater.AddUpdate( 0, ACTION_NOTIFY_PLACEMENT, this, -1 );
	CalcVisibility();
}

void CVisShell::GetProjectileInfo( SAINotifyNewProjectile *pProjectileInfo )
{
	pProjectileInfo->nObjUniqueID = GetUniqueId();

	CAIUnit *pUnit = checked_cast<CAIUnit*>(GetWhoFired());
	pProjectileInfo->nSourceUniqueID = pUnit->GetUniqueId();
	pProjectileInfo->nGun = GetNGun();
	pProjectileInfo->nShell = GetShellType();
	pProjectileInfo->timeToEqualizePos = GetExplTime() - GetStartTime();

	pProjectileInfo->vAIStartPos = center;
	pProjectileInfo->nPlatform = nPlatform;
}

float CVisShell::GetTerrainHeight( const float x, const float y, const NTimer::STime timeDiff ) const
{
	float fRatio;
	if ( curTime - timeDiff < GetStartTime() )
		fRatio = 0;
	else
		fRatio = float( curTime - timeDiff - GetStartTime() ) / float( GetExplTime() - GetStartTime() );
	
	if ( pTraj->GetTrajType() == NDb::SWeaponRPGStats::SShell::TRAJECTORY_BOMB )
		return GetFinishVisZ() * fRatio;
	else 
		return GetStartVisZ() * ( 1 - fRatio ) + GetFinishVisZ() * fRatio;
}

//*******************************************************************
//*								  CShellsStore																		*
//*******************************************************************

void CShellsStore::AddShell( CMomentShell &shell )
{
	shell.Explode();
	theCombatEstimator.AddShell( curTime, shell.GetMaxDamage() );
}

void CShellsStore::AddShell( CInvisShell *pShell )
{ 
	//DEBUG{
	NTimer::STime t1 = 0;
	if ( !invisShells.empty() )
		t1 = invisShells.top()->GetExplTime();
	//DEBUG}
	pShell->SetOrder( nNextInvisShellOrder++ );
	invisShells.push( pShell );
	theCombatEstimator.AddShell( curTime, pShell->GetMaxDamage() );
	
	//DEBUG{
	const NTimer::STime t2 = invisShells.top()->GetExplTime();
	//DEBUG}
}

void CShellsStore::AddShell( CVisShell *pShell )
{
	visShells.push_back( pShell );
	updater.AddUpdate( 0, ACTION_NOTIFY_NEW_PROJECTILE, pShell, -1 );

	if ( pShell->GetTrajectoryType() == NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE ||
			 pShell->GetTrajectoryType() == NDb::SWeaponRPGStats::SShell::TRAJECTORY_GRENADE )
		theCombatEstimator.AddShell( curTime, pShell->GetMaxDamage() );
}

void CShellsStore::Segment()
{
	// взорвать невидимые снаряды
	while ( !invisShells.empty() && invisShells.top()->GetExplTime() <= curTime + SConsts::AI_SEGMENT_DURATION / 2 )
	{
		invisShells.top()->Explode();
		invisShells.pop();
	}

	// обновить видимые
	CVisShellList::iterator iter = visShells.begin();
	while ( iter != visShells.end() )
	{
		CVisShell *shell = *iter;
		// долетел
		if ( shell->GetExplTime() <= curTime )
		{
			shell->Explode();
			updater.AddUpdate( 0, ACTION_NOTIFY_DEAD_PROJECTILE, shell, -1 );
			iter = visShells.erase( iter );
		}
		else
		{
			shell->Segment();
			++iter;
		}
	}
}

void CShellsStore::Clear()
{
	while ( !invisShells.empty() )
		invisShells.pop();

	visShells.clear();
	nNextInvisShellOrder = 0;
}

void CShellsStore::UpdateCheckSum( uLong *pCheckSum )
{
	using namespace NCheckSums;

	static SCheckSumBufferStorage checkSumBuf( 10000 );
	checkSumBuf.nCnt = 0;

	CInvisShells copyQueue = invisShells;
	while ( !copyQueue.empty() )
	{
		CInvisShell *pShell = copyQueue.top();
		copyQueue.pop();
		
		const CVec3 vExplCenter = pShell->GetExplCoordinates();
		const NTimer::STime explTime = pShell->GetExplTime();
		const int nOrder = pShell->GetOrder();

		CopyToBuf( &checkSumBuf, vExplCenter );
		CopyToBuf( &checkSumBuf, explTime );
		CopyToBuf( &checkSumBuf, nOrder );
	}

	for ( CVisShellList::iterator iter = visShells.begin(); iter != visShells.end(); ++iter )
	{
		CVisShell *pShell = *iter;
		const CVec3 vExplCenter = pShell->GetExplCoordinates();
		const CVec3 vCurCenter = pShell->GetCoordinates();
		const NTimer::STime explTime = pShell->GetExplTime();

		CopyToBuf( &checkSumBuf, vExplCenter );
		CopyToBuf( &checkSumBuf, vCurCenter );
		CopyToBuf( &checkSumBuf, explTime );
	}

	adler32( *pCheckSum, &(checkSumBuf.buf[0]), checkSumBuf.nCnt );
}

void CShellsStore::UpdateDebugChecksums(FILE* f)
{
	fprintf(f, "Explosions:\n");
	CInvisShells copyQueue = invisShells;
	while ( !copyQueue.empty() )
	{
		CInvisShell *pShell = copyQueue.top();
		copyQueue.pop();
		
		const CVec3 vExplCenter = pShell->GetExplCoordinates();
		const NTimer::STime explTime = pShell->GetExplTime();
		const int nOrder = pShell->GetOrder();

		uLong checksum = 12347;
		checksum = CalculateChecksum(checksum, vExplCenter.x, vExplCenter.y, vExplCenter.z, explTime, nOrder);

		fprintf(f, "\nExplosion[%d]: %lu\n", nOrder, checksum);
	}
	fprintf(f, "\n");

	fprintf(f, "Shells:\n");
	for ( CVisShellList::iterator iter = visShells.begin(); iter != visShells.end(); ++iter )
	{
		CVisShell *pShell = *iter;
		const CVec3 vExplCenter = pShell->GetExplCoordinates();
		const CVec3 vCurCenter = pShell->GetCoordinates();
		const NTimer::STime explTime = pShell->GetExplTime();
		uLong checksum = 71717;
		checksum = CalculateChecksum(checksum, vExplCenter.x, vExplCenter.y, vExplCenter.z, vCurCenter.x, vCurCenter.y, vCurCenter.z, explTime);
		
		fprintf(f, "\tPlayer[%d] Shell[%d]: %lu\n", (int)pShell->GetPlayer(), pShell->GetUniqueId(), checksum);
	}
	fprintf(f, "\n");
}

//*******************************************************************
//*											CBombBallisticTraj													*
//*******************************************************************

CBombBallisticTraj::CBombBallisticTraj( const CVec3 &_point, const CVec3 &_v, const NTimer::STime &_explTime, const CVec2 &_vRandAcc )
: point( _point ), v( _v ), wDir( GetDirectionByVector( CVec2( _v.x, _v.y ) ) ), 
	startTime( curTime ), explTime( _explTime ), vRandAcc( _vRandAcc )
{ 
}

CVec3 CBombBallisticTraj::CalcTrajectoryFinish( const CVec3 &vSourcePoint, const CVec3 &vInitialSpeed, const CVec2 &vRandAcc, const float fTimeOfFly )
{
	const float fTimeOfFly2 = sqr( fTimeOfFly );
	const float fCoeff = GetCoeff( fTimeOfFly );
	return GetHeights()->Get3DPoint( CVec2(vSourcePoint.x + vInitialSpeed.x * fCoeff + vRandAcc.x * fTimeOfFly2 / 2.0f, vSourcePoint.y + vInitialSpeed.y * fCoeff + vRandAcc.y * fTimeOfFly2 / 2.0f) );
}

const CVec3 CBombBallisticTraj::GetCoordinates() const
{
	const float timeDiff = curTime - startTime;
	const float timeDiff2 = sqr( timeDiff );
	const float fCoeff = GetCoeff( timeDiff );
	const float vPointX = v.x * fCoeff;
	const float vPointY = v.y * fCoeff;
	const float vPointZ = v.z * timeDiff - SConsts::TRAJECTORY_BOMB_G * timeDiff2 / 2;

	return CVec3( point.x + vPointX + vRandAcc.x * timeDiff2 / 2.0f, point.y + vPointY + vRandAcc.y * timeDiff2 / 2.0f, point.z + vPointZ );
}

float CBombBallisticTraj::GetCoeff( const float &timeDiff )
{
	return ( 1 - exp( -1.0f * SConsts::TRAJ_BOMB_ALPHA * timeDiff ) ) / SConsts::TRAJ_BOMB_ALPHA;
}

float CBombBallisticTraj::GetTimeOfFly( const float fZ, const float fZSpeed )
{
	return ( sqrt( sqr(fZSpeed) + 2 * SConsts::TRAJECTORY_BOMB_G * fZ ) + fZSpeed ) / SConsts::TRAJECTORY_BOMB_G;
}

//*******************************************************************
//*										CFakeBallisticTraj														*
//*******************************************************************

CFakeBallisticTraj::CFakeBallisticTraj( const CVec3 &_point, const CVec3 &_v, const NTimer::STime &_explTime, const float _A1, const float _A2 )
: point( _point ), v( _v ), wDir( GetDirectionByVector( CVec2( _v.x, _v.y ) ) ), 
	startTime( curTime ), explTime( _explTime ), A1( _A1 ), A2( _A2 ) 
{ 
}

const CVec3 CFakeBallisticTraj::GetCoordinates() const
{
	const NTimer::STime timeDiff = curTime - startTime;
	const CVec3 firstPoint = v * timeDiff;
	const float r = fabs( CVec2( firstPoint.x, firstPoint.y ) );

	return CVec3 ( point.x + firstPoint.x, point.y + firstPoint.y, 
								 point.z + firstPoint.z + A1 * sqr( r ) + A2 * r );
}
const NDb::SWeaponRPGStats::SShell::ETrajectoryType CFakeBallisticTraj::GetTrajType() const 
{ 
	return NDb::SWeaponRPGStats::SShell::TRAJECTORY_CANNON; 
}

//*******************************************************************
//*													CBallisticTraj													*
//*******************************************************************

CBallisticTraj::CBallisticTraj( const CVec3 &_vStart, const CVec3 &vFinish, float fV, const NDb::SWeaponRPGStats::SShell::ETrajectoryType _eType, WORD wMaxAngle, float fMaxRange )
: startTime( curTime ), vStart3D( _vStart ), eType( _eType )
{
	if ( eType == NDb::SWeaponRPGStats::SShell::TRAJECTORY_GRENADE )
		wMaxAngle = 65535 / 8;

	const CVec3 vDir3D( vFinish - vStart3D );
	vDir = CVec2( vDir3D.x, vDir3D.y );
	const float x0 = fabs( vDir );
	Normalize( &vDir );
	wDir = GetDirectionByVector( vDir );

	if ( eType == NDb::SWeaponRPGStats::SShell::TRAJECTORY_HOWITZER || eType == NDb::SWeaponRPGStats::SShell::TRAJECTORY_GRENADE )
	{
		wAngle = wMaxAngle + 65535 / 4 * 3;
		const CVec2 vSin = GetVectorByDirection( wAngle );
		fG = 2.0f * sqr( fV ) * vSin.x * vSin.y / x0;
		fVx = vSin.x * fV;
		fVy = vSin.y * fV;
	}
	else
	{	
		fV = sqr( fV );
		fG = fV / fMaxRange / 2;
		const float fCoeff = fG * x0;
		// добавить скорости, если не хватает
		if ( fV < fCoeff + 0.001f )
			fV = fCoeff + 0.001f;

		const float fSqrt1 = sqrt( fV + fCoeff );
		const float fSqrt2 = sqrt( fV - fCoeff );

		// крутая траектория
		/*if ( eType == NDb::SWeaponRPGStats::SShell::TRAJECTORY_GRENADE )
		{
 			fVx = 0.5f * ( fSqrt1 - fSqrt2 );
			fVy = 0.5f * ( fSqrt1 + fSqrt2 );
		}
		// пологая траектория
		else*/
		{
			fVx = 0.5f * ( fSqrt1 + fSqrt2 );
			fVy = 0.5f * ( fSqrt1 - fSqrt2 );
		}
		wAngle = GetDirectionByVector( fVx, fVy );
	}

	
	explTime = startTime + x0 / fVx;
}

const CVec3 CBallisticTraj::GetCoordinates() const
{
	const float fT = curTime - startTime;
	const CVec3 vRet = vStart3D + CVec3( vDir * fVx * fT, fVy * fT - fG * sqr( fT ) / 2 );
	return vRet;
}

WORD CBallisticTraj::GetTrajectoryZAngle( const CVec3 &vToAim, float fV, const NDb::SWeaponRPGStats::SShell::ETrajectoryType eType, WORD wMaxAngle, float fMaxRange )
{
	const CBallisticTraj traj( VNULL3, vToAim, fV, eType, wMaxAngle, fMaxRange );
	return traj.wAngle;
}

//*******************************************************************
//*													CAARocketTraj														*
//*******************************************************************

CAARocketTraj::CAARocketTraj( const CVec3 &vStart, const CVec3 &vFinish, float fV )
: startTime( curTime ), vStart3D( vStart )
{
	vSpeed = vFinish - vStart;
	CVec2 vDir( vSpeed.x, vSpeed.y );
	const float fDistance = fabs( vSpeed );
	Normalize( &vSpeed );
	Normalize( &vDir );
	wDir = GetDirectionByVector( vDir );
	explTime = startTime + fDistance / fV;
	vSpeed *= fV;
}

const CVec3 CAARocketTraj::GetCoordinates() const
{
	const float fT = curTime - startTime;
	const CVec3 vRet = vStart3D + vSpeed * fT;
	return vRet;
}



