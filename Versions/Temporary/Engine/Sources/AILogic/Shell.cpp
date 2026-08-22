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

#include "AILogic_export.h"

#include <cstdint>

REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D4E3, CHitInfo );
REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D446, CFakeBallisticTraj );
REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D447, CBombBallisticTraj );
REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D448, CBallisticTraj );
REGISTER_SAVELOAD_CLASS( AILOGIC, 0x19184C00, CAARocketTraj );
REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D449, CVisShell );
REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D44A, CInvisShell );
REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D44B, CBurstExpl );
REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D44C, CCumulativeExpl );
REGISTER_SAVELOAD_CLASS( AILOGIC, 0x11230D00, CFlameThrowerExpl );

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

namespace NAsyncExplosionDebug
{
	enum { AREA_DAMAGE_TRACE_SIZE = 256 };

	struct SAreaDamageContext
	{
		bool bValid;
		const CExplosion *pExpl;
		CAIUnit *pTarget;
		int nRawIndex;
		int nSortedIndex;
		int nDuplicateCount;
		int nRawCount;
		int nUniqueCount;

		SAreaDamageContext() : bValid( false ), pExpl( 0 ), pTarget( 0 ), nRawIndex( -1 ), nSortedIndex( -1 ),
			nDuplicateCount( 0 ), nRawCount( 0 ), nUniqueCount( 0 ) { }
	};

	struct SAreaDamageRecord
	{
		unsigned int nSeq;
		NTimer::STime nTime;
		const char *szStage;

		int nShooterUID;
		int nShooterPlayer;
		int nShellType;
		int nTrajectory;
		CVec3 vExpl;
		float fRadius;
		float fSmallRadius;

		int nTargetUID;
		int nTargetPlayer;
		int nTargetParty;
		CVec2 vTarget;
		float fTargetZ;

		int nRawIndex;
		int nSortedIndex;
		int nDuplicateCount;
		int nRawCount;
		int nUniqueCount;

		int nArmorDir;
		float fDist2;
		float fZDiff;
		bool bPreconditionsPassed;
		bool bCoverCalled;
		bool bSavedByCover;
		bool bCircleHit;
		bool bArmorPassed;
		bool bDamageApplied;
		uint64_t nRngBefore;
		uint64_t nRngAfter;

		SAreaDamageRecord() : nSeq( 0 ), nTime( 0 ), szStage( "" ), nShooterUID( 0 ), nShooterPlayer( -1 ),
			nShellType( -1 ), nTrajectory( -1 ), vExpl( VNULL3 ), fRadius( 0 ), fSmallRadius( 0 ),
			nTargetUID( 0 ), nTargetPlayer( -1 ), nTargetParty( -1 ), vTarget( VNULL2 ), fTargetZ( 0 ),
			nRawIndex( -1 ), nSortedIndex( -1 ), nDuplicateCount( 0 ), nRawCount( 0 ), nUniqueCount( 0 ),
			nArmorDir( 0 ), fDist2( 0 ), fZDiff( 0 ), bPreconditionsPassed( false ), bCoverCalled( false ),
			bSavedByCover( false ), bCircleHit( false ), bArmorPassed( false ), bDamageApplied( false ),
			nRngBefore( 0 ), nRngAfter( 0 ) { }
	};

	static SAreaDamageContext s_areaDamageContext;
	static SAreaDamageRecord s_areaDamageRecords[AREA_DAMAGE_TRACE_SIZE];
	static unsigned int s_nextAreaDamageSeq = 1;
	static int s_nextAreaDamageRecord = 0;

	uint64_t GetRandomCallCounter()
	{
		return NRandom::GetRandomCallsCounter();
	}

	void SetAreaDamageCandidate( const CExplosion *pExpl, CAIUnit *pTarget, int nRawIndex, int nSortedIndex, int nDuplicateCount, int nRawCount, int nUniqueCount )
	{
		s_areaDamageContext.bValid = true;
		s_areaDamageContext.pExpl = pExpl;
		s_areaDamageContext.pTarget = pTarget;
		s_areaDamageContext.nRawIndex = nRawIndex;
		s_areaDamageContext.nSortedIndex = nSortedIndex;
		s_areaDamageContext.nDuplicateCount = nDuplicateCount;
		s_areaDamageContext.nRawCount = nRawCount;
		s_areaDamageContext.nUniqueCount = nUniqueCount;
	}

	void ClearAreaDamageCandidate()
	{
		s_areaDamageContext = SAreaDamageContext();
	}

	void RecordAreaDamageTrace( const char *szStage, const CExplosion *pExpl, CAIUnit *pTarget, int nArmorDir, float fRadius, float fSmallRadius,
		bool bPreconditionsPassed, bool bCoverCalled, bool bSavedByCover, bool bCircleHit, bool bArmorPassed, bool bDamageApplied,
		float fDist2, float fZDiff, uint64_t nRngBefore, uint64_t nRngAfter )
	{
		SAreaDamageRecord &record = s_areaDamageRecords[s_nextAreaDamageRecord];
		s_nextAreaDamageRecord = ( s_nextAreaDamageRecord + 1 ) % AREA_DAMAGE_TRACE_SIZE;

		record = SAreaDamageRecord();
		record.nSeq = s_nextAreaDamageSeq++;
		record.nTime = curTime;
		record.szStage = szStage ? szStage : "";
		record.nArmorDir = nArmorDir;
		record.fRadius = fRadius;
		record.fSmallRadius = fSmallRadius;
		record.bPreconditionsPassed = bPreconditionsPassed;
		record.bCoverCalled = bCoverCalled;
		record.bSavedByCover = bSavedByCover;
		record.bCircleHit = bCircleHit;
		record.bArmorPassed = bArmorPassed;
		record.bDamageApplied = bDamageApplied;
		record.fDist2 = fDist2;
		record.fZDiff = fZDiff;
		record.nRngBefore = nRngBefore;
		record.nRngAfter = nRngAfter;

		if ( pExpl )
		{
			CAIUnit *pShooter = pExpl->GetWhoFire();
			if ( IsValidObj( pShooter ) )
			{
				record.nShooterUID = pShooter->GetUniqueId();
				record.nShooterPlayer = pShooter->GetPlayer();
			}
			record.nShellType = pExpl->GetShellType();
			record.nTrajectory = pExpl->GetTrajectoryType();
			record.vExpl = pExpl->GetExplCoordinates();
		}

		if ( IsValidObj( pTarget ) )
		{
			record.nTargetUID = pTarget->GetUniqueId();
			record.nTargetPlayer = pTarget->GetPlayer();
			record.nTargetParty = pTarget->GetParty();
			record.vTarget = pTarget->GetCenterPlain();
			record.fTargetZ = pTarget->GetVisZ();
		}

		if ( s_areaDamageContext.bValid && s_areaDamageContext.pExpl == pExpl && s_areaDamageContext.pTarget == pTarget )
		{
			record.nRawIndex = s_areaDamageContext.nRawIndex;
			record.nSortedIndex = s_areaDamageContext.nSortedIndex;
			record.nDuplicateCount = s_areaDamageContext.nDuplicateCount;
			record.nRawCount = s_areaDamageContext.nRawCount;
			record.nUniqueCount = s_areaDamageContext.nUniqueCount;
		}
	}

	void ResetAreaDamageTrace()
	{
		s_areaDamageContext = SAreaDamageContext();
		for ( int i = 0; i < AREA_DAMAGE_TRACE_SIZE; ++i )
			s_areaDamageRecords[i] = SAreaDamageRecord();
		s_nextAreaDamageSeq = 1;
		s_nextAreaDamageRecord = 0;
	}

	void DumpAreaDamageTrace( FILE* f )
	{
		if ( !f )
			return;

		fprintf( f, "Area damage trace (last %d records):\n", AREA_DAMAGE_TRACE_SIZE );
		for ( int i = 0; i < AREA_DAMAGE_TRACE_SIZE; ++i )
		{
			const int nIndex = ( s_nextAreaDamageRecord + i ) % AREA_DAMAGE_TRACE_SIZE;
			const SAreaDamageRecord &record = s_areaDamageRecords[nIndex];
			if ( record.nSeq == 0 )
				continue;

			fprintf( f,
				"\t#%u time=%d stage=%s shooterUID=%d shooterPlayer=%d shell=%d traj=%d expl=(%f,%f,%f) radius=(%f,%f) targetUID=%d targetPlayer=%d targetParty=%d target=(%f,%f,%f) raw=%d sorted=%d dup=%d counts=(%d,%d) armorDir=%d dist2=%f zDiff=%f pre=%d cover=%d saved=%d circle=%d armor=%d damage=%d rng=%I64u->%I64u\n",
				record.nSeq, (int)record.nTime, record.szStage, record.nShooterUID, record.nShooterPlayer,
				record.nShellType, record.nTrajectory, record.vExpl.x, record.vExpl.y, record.vExpl.z,
				record.fRadius, record.fSmallRadius, record.nTargetUID, record.nTargetPlayer, record.nTargetParty,
				record.vTarget.x, record.vTarget.y, record.fTargetZ, record.nRawIndex, record.nSortedIndex,
				record.nDuplicateCount, record.nRawCount, record.nUniqueCount, record.nArmorDir, record.fDist2,
				record.fZDiff, record.bPreconditionsPassed, record.bCoverCalled, record.bSavedByCover,
				record.bCircleHit, record.bArmorPassed, record.bDamageApplied, record.nRngBefore, record.nRngAfter );
		}
		fprintf( f, "\n" );
	}
}

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
												const uint8_t _nShellType,
												const bool bRandomize, 
												const int _nPlayerOfShoot )
{
	pUnit = _pUnit;
	pWeapon = _pWeapon;
	nShellType = _nShellType;
	nPlayerOfShoot = _nPlayerOfShoot;
	// Snapshot shot-time weapon modifiers so delayed shells keep the bonuses used when fired.
	weaponDamageModifier = NDb::SUnitStatsModifier::SParameterModifier();
	weaponPiercingModifier = NDb::SUnitStatsModifier::SParameterModifier();
	weaponAreaModifier = NDb::SUnitStatsModifier::SParameterModifier();
	weaponArea2Modifier = NDb::SUnitStatsModifier::SParameterModifier();
	if ( pUnit )
	{
		weaponDamageModifier = pUnit->GetStatsModifier()->weaponDamage;
		weaponPiercingModifier = pUnit->GetStatsModifier()->weaponPiercing;
		weaponAreaModifier = pUnit->GetStatsModifier()->weaponArea;
		weaponArea2Modifier = pUnit->GetStatsModifier()->weaponArea2;
	}

	CVec2 vRand( VNULL2 );
	const CVec3 vDiff( _explCoord - attackerPos );

	if ( bRandomize )
	{
		const float fFireRangeMax = GetFireRangeMax( pWeapon, pUnit );
		const float fDispRadius = GetDispByRadius( fDispersion, fFireRangeMax, fabs( vDiff ) );
		RandQuadrInCircle( fDispRadius, &vRand, fDispRatio, CVec2(vDiff.x, vDiff.y) ); RecordRandomCall(); RecordRandomCall();
	}

	explCoord = _explCoord + CVec3( vRand, 0 );
	attackDir = GetDirectionByVector( - vDiff.x, - vDiff.y );
}

CExplosion::CExplosion( CAIUnit *pUnit, const SWeaponRPGStats *pWeapon, const CVec3 &explCoord, const CVec3 &attackerPos, const uint8_t nShellType, const bool bRandomize )
{
	if ( pUnit != 0 )
		Init( pUnit, pWeapon, pWeapon->fDispersion, 1, explCoord, attackerPos, nShellType, bRandomize, pUnit->GetPlayer() );
	else
		Init( pUnit, pWeapon, pWeapon->fDispersion, 1, explCoord, attackerPos, nShellType, bRandomize, theDipl.GetNeutralPlayer() );
}

CExplosion::CExplosion( CAIUnit *pUnit, const CBasicGun *pGun, const CVec3 &explCoord, const CVec3 &attackerPos, const uint8_t nShellType, const bool bRandomize )
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
	return weaponPiercingModifier.Get( pWeapon->shells[nShellType].GetRandomPiercing() );
}

float CExplosion::GetMaxDamage() const
{
	return weaponDamageModifier.Get( pWeapon->shells[nShellType].fDamagePower + pWeapon->shells[nShellType].nDamageRandom );
}

const float CExplosion::GetRandomDamage() const
{
	return weaponDamageModifier.Get( pWeapon->shells[nShellType].GetRandomDamage() );
}
const float CExplosion::GetArea() const
{
	return weaponAreaModifier.Get( pWeapon->shells[nShellType].fArea );
}

const float CExplosion::GetArea2() const
{
	return weaponArea2Modifier.Get( pWeapon->shells[nShellType].fArea2 );
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
			GetArea(),
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

CCumulativeExpl::CCumulativeExpl( CAIUnit *pUnit, const CBasicGun *pGun, const CVec3 &explCoord, const CVec3 &attackerPos, const uint8_t nShellType, const bool bRandomize )
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
	// Ensure that the units within explosion are iterated deterministically by UniqueID
	std::vector<CAIUnit*> units;
	units.reserve(32);
	CUnitsIter<0,0> iter( 0, ANY_PARTY, vExplCoord, 0.0f );
	while (!iter.IsFinished())
	{
		units.push_back(*iter);
		iter.Iterate();
	}
	// sort ensures the deterministic iteration
	std::sort(units.begin(), units.end(), [](CAIUnit* u1, CAIUnit* u2) {
		return u1->GetUniqueId() > u2->GetUniqueId();
	});
	// Clear out the potential duplicates
	units.erase(std::unique(units.begin(), units.end()), units.end());

	for ( size_t i = 0; i < units.size(); i++ )
	{
		CAIUnit *pTarget = units[i];
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
		std::vector<CExistingObject*> hitObjects;
		hitObjects.reserve(32);
		
		// по статическим объектам
		for ( CStObjCircleIter<false> iter( vExplCoord, 0 ); !iter.IsFinished(); iter.Iterate() )
		{
			CExistingObject *pObj = *iter;
			if ( pObj->IsAlive() )
				hitObjects.push_back( pObj );
		}
		// Ensure deterministic iteration
		std::sort(hitObjects.begin(), hitObjects.end(), [](CExistingObject* o1, CExistingObject* o2) {
			return o1->GetUniqueId() > o2->GetUniqueId();
		});
		// Clear out the potential duplicates
		hitObjects.erase(std::unique(hitObjects.begin(), hitObjects.end()), hitObjects.end());

		for ( std::vector<CExistingObject*>::iterator it = hitObjects.begin(); it != hitObjects.end(); ++it )
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

CBurstExpl::CBurstExpl( CAIUnit *pUnit, const CBasicGun *pGun, const CVec3 &explCoord, const CVec3 &attackerPos, const uint8_t nShellType, const bool bRandomize, const int ArmorDir, const bool _bShowEffect )
: CExplosion( pUnit, pGun, explCoord, attackerPos, nShellType, bRandomize ), nArmorDir( ArmorDir ), bShowEffect( _bShowEffect )
{
	if ( pWeapon->shells[nShellType].etrajectory != NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE || (pUnit && pUnit->GetZ() > GetExplCoordinates().z) )
		nArmorDir = 2;
}

CBurstExpl::CBurstExpl( CAIUnit *pUnit, const SWeaponRPGStats *pWeapon, const CVec3 &explCoord, const CVec3 &attackerPos, const uint8_t nShellType, const bool bRandomize, const int ArmorDir, const bool _bShowEffect )
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
	
	const float fRadius = GetArea2();
	const float fSmallRadius = GetArea();
	NI_ASSERT( fRadius != 0, "Неверный тип взрыва" );

	bool bHit = false;
	// по юнитам
	// Ensure that the units within explosion are iterated deterministically by UniqueID
	std::vector<CAIUnit*> units;
	units.reserve(32);
	CUnitsIter<0,0> iter( 0, ANY_PARTY, explCoord, fRadius );
	while (!iter.IsFinished())
	{
		units.push_back(*iter);
		iter.Iterate();
	}
	std::vector<CAIUnit*> rawUnits = units;
	// sort ensures the deterministic iteration
	std::sort(units.begin(), units.end(), [](CAIUnit* u1, CAIUnit* u2) {
		return u1->GetUniqueId() > u2->GetUniqueId();
	});
	// Clear out the potential duplicates
	units.erase(std::unique(units.begin(), units.end()), units.end());

	for ( size_t i = 0; i < units.size(); i++ )
	{
		CAIUnit *pTarget = units[i];
		if ( IsValidObj( pTarget ) )
		{
			int nRawIndex = -1;
			int nDuplicateCount = 0;
			for ( int nRaw = 0; nRaw < (int)rawUnits.size(); ++nRaw )
			{
				if ( rawUnits[nRaw] == pTarget )
				{
					if ( nRawIndex == -1 )
						nRawIndex = nRaw;
					++nDuplicateCount;
				}
			}

			if ( pTarget != pUnit &&
					 ( pWeapon->shells[nShellType].etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE || 
					 pWeapon->shells[nShellType].etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_GRENADE ) )
				pTarget->Grazed( pUnit );

			// чтобы не пропускался вызов функции из-за оптимизации вычисления bool выражений
			NAsyncExplosionDebug::SetAreaDamageCandidate( this, pTarget, nRawIndex, (int)i, nDuplicateCount, (int)rawUnits.size(), (int)units.size() );
			const bool bExplResult = pTarget->ProcessBurstExpl( this, nArmorDir, fRadius, fSmallRadius );
			NAsyncExplosionDebug::ClearAreaDamageCandidate();
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
		std::vector<CExistingObject*> hitObjects;
		hitObjects.reserve(32);

		// по статическим объектам
		for ( CStObjCircleIter<false> iter( explCoord, fSmallRadius + 300.0f ); !iter.IsFinished(); iter.Iterate() )
		{
			CExistingObject *pObj = *iter;
			if ( IsValidObj( pObj ) )
				hitObjects.push_back( pObj );
		}
		// Ensure deterministic iteration
		std::sort(hitObjects.begin(), hitObjects.end(), [](CExistingObject* o1, CExistingObject* o2) {
			return o1->GetUniqueId() > o2->GetUniqueId();
		});
		// Clear out the potential duplicates
		hitObjects.erase(std::unique(hitObjects.begin(), hitObjects.end()), hitObjects.end());

		for ( std::vector<CExistingObject*>::iterator it = hitObjects.begin(); it != hitObjects.end(); ++it )
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
									const uint8_t nShellType, const bool bRandomize )
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
	const float fRadius = GetArea();

	
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
	nPlatform( _nPlatform ), nOrder( 0 )
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
	// Visible shells can share an explosion time; order keeps processing stable.
	pShell->SetOrder( nNextVisShellOrder++ );
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
	visShells.sort( SVisShellCompare() );
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
	nNextVisShellOrder = 0;
	NAsyncExplosionDebug::ResetAreaDamageTrace();
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

	CVisShellList sortedVisShells = visShells;
	sortedVisShells.sort( SVisShellCompare() );
	for ( CVisShellList::iterator iter = sortedVisShells.begin(); iter != sortedVisShells.end(); ++iter )
	{
		CVisShell *pShell = *iter;
		const CVec3 vExplCenter = pShell->GetExplCoordinates();
		const CVec3 vCurCenter = pShell->GetCoordinates();
		const NTimer::STime explTime = pShell->GetExplTime();
		const int nOrder = pShell->GetOrder();

		CopyToBuf( &checkSumBuf, vExplCenter );
		CopyToBuf( &checkSumBuf, vCurCenter );
		CopyToBuf( &checkSumBuf, explTime );
		CopyToBuf( &checkSumBuf, nOrder );
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
	CVisShellList sortedVisShells = visShells;
	sortedVisShells.sort( SVisShellCompare() );
	for ( CVisShellList::iterator iter = sortedVisShells.begin(); iter != sortedVisShells.end(); ++iter )
	{
		CVisShell *pShell = *iter;
		const CVec3 vExplCenter = pShell->GetExplCoordinates();
		const CVec3 vCurCenter = pShell->GetCoordinates();
		const NTimer::STime explTime = pShell->GetExplTime();
		const int nOrder = pShell->GetOrder();
		uLong checksum = 71717;
		checksum = CalculateChecksum(checksum, vExplCenter.x, vExplCenter.y, vExplCenter.z, vCurCenter.x, vCurCenter.y, vCurCenter.z, explTime, nOrder);
		
		fprintf(f, "\tPlayer[%d] Shell[%d] Order[%d]: %lu\n", (int)pShell->GetPlayer(), pShell->GetUniqueId(), nOrder, checksum);
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

CBallisticTraj::CBallisticTraj( const CVec3 &_vStart, const CVec3 &vFinish, float fV, const NDb::SWeaponRPGStats::SShell::ETrajectoryType _eType, uint16_t wMaxAngle, float fMaxRange )
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

uint16_t CBallisticTraj::GetTrajectoryZAngle( const CVec3 &vToAim, float fV, const NDb::SWeaponRPGStats::SShell::ETrajectoryType eType, uint16_t wMaxAngle, float fMaxRange )
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



