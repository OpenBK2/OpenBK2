#include "stdafx.h"

#include "../Stats_B2_M1/AnimationFromAction.h"
#include "Guns.h"
#include "GunsInternal.h"
#include "Shell.h"
#include "Soldier.h"
#include "Randomize.h"
#include "NewUpdater.h"
#include "Cheats.h"
#include "Turret.h"
#include "Aviation.h"
#include "UnitStates.h"
#include "Weather.h"
#include "DifficultyLevel.h"
#include "Diplomacy.h"
#include "Formation.h"
#include "TimeCounter.h"
#include "../Common_RTS_AI/StaticMapHeights.h"
#include "../Common_RTS_AI/AIMap.h"
#include "AIGeometry.h"
#include "../DebugTools/DebugInfoManager.h"
#include "UnitCreation.h"
#include "Building.h"
#include "../System/Commands.h"
#include "GlobalWarFog.h"

REGISTER_SAVELOAD_CLASS( 0x1108D4B5, CBaseGun );
REGISTER_SAVELOAD_CLASS( 0x1108D4B7, CTurretGun );

static int g_nEffectsForBombs = 4;

START_REGISTER(EffectForBombs)
REGISTER_VAR_EX( "bomb_with_effect", NGlobal::VarIntHandler, &g_nEffectsForBombs, 4, STORAGE_NONE );
FINISH_REGISTER

extern CEventUpdater updater;
extern NTimer::STime curTime;
extern CShellsStore theShellsStore;
extern SCheats theCheats;
extern CWeather theWeather;
extern CDifficultyLevel theDifficultyLevel;
extern CDiplomacy theDipl;
extern CUnitCreation theUnitCreation;
extern CGlobalWarFog theWarFog;
extern NAI::CTimeCounter timeCounter;

float GetDispByRadius( const float fDispRadius, const float fRangeMax, const float fDist )
{
	return fDispRadius / fRangeMax * fDist;
}

float GetDispByRadius( const CBasicGun *pGun, const float fDist )
{
	return GetDispByRadius( pGun->GetDispersion(), pGun->GetFireRangeMax(), fDist );
}

float GetDispByRadius( const CBasicGun *pGun, const CVec2 &attackerPos, const CVec2 &explCoord )
{
	return GetDispByRadius( pGun->GetDispersion(), pGun->GetFireRangeMax(), fabs( attackerPos - explCoord ) );
}

BASIC_REGISTER_CLASS( CBasicGun );

//*******************************************************************
//*													  CGun																	*
//*******************************************************************

CBasicGun::CBasicGun( class CAIUnit *_pOwner, const BYTE _nShellType, SCommonGunInfo *_pCommonGunInfo, const IGunsFactory::EGunTypes _eType )
: pOwner( _pOwner ), shootState( EST_REST ), nShellType( _nShellType ), bAngleLocked( false ), bCanShoot( true ), pCommonGunInfo( _pCommonGunInfo ), eType( _eType ),
	eRejectReason( ACK_NONE ), bWaitForReload( false ), lastCheck( 0 ), bParallelGun( false ), lastCheckTurnTime( 0 ), 
	nShotsLast( 0 ), vLastShotPoint( VNULL3 ), target( VNULL2 ), bAim( false ), lastEnemyPos( VNULL2 ), bCanShootToUnitWOMove( false )
{
	SetUniqueIdForObjects();
	bGrenade = pOwner && pOwner->GetStats()->IsInfantry() && pCommonGunInfo->nGun == 1;
	bIgnoreObstacles = pOwner && ( pOwner->GetStats()->IsInfantry() || pOwner->GetStats()->IsAviation() ||
										             eType == IGunsFactory::VIS_CML_BALLIST_GUN ||
																 eType == IGunsFactory::TORPEDO_GUN ||
										             eType == IGunsFactory::VIS_BURST_BALLIST_GUN );
	pEnemy = 0;
	z = 0;
	
	InitRandoms();

	nOwnerParty = pOwner ? pOwner->GetParty() : 0;

	if ( pOwner )
		pWeapon = pOwner->GetStats()->GetGun( pOwner->GetUniqueID(), pCommonGunInfo->nPlatform, pCommonGunInfo->nGun ).pWeapon;
}

void CBasicGun::InitRandoms()
{
	fRandom4Aim = NRandom::Random( 1.0f, SConsts::COEFF_FOR_RANDOM_DELAY );
	fRandom4Relax = NRandom::Random( 1.0f, SConsts::COEFF_FOR_RANDOM_DELAY );
}

void CBasicGun::GetMechShotInfo( SAINotifyMechShot *pMechShotInfo, const NTimer::STime &time ) const
{
	pOwner->GetShotInfo( pMechShotInfo );

	pMechShotInfo->cGun = pCommonGunInfo->nGun;
	pMechShotInfo->cPlatform = pCommonGunInfo->nPlatform;
	pMechShotInfo->cShell = nShellType;
	pMechShotInfo->time = time;
	pMechShotInfo->vDestPos = vLastShotPoint;
}

void CBasicGun::GetInfantryShotInfo( SAINotifyInfantryShot *pInfantryShotInfo, const NTimer::STime &time ) const
{
	NI_ASSERT( pOwner->GetStats()->IsInfantry(), "Wrong unit type" );

	if ( pCommonGunInfo->nGun == 1 )
		checked_cast_ptr<CSoldier*>(pOwner)->GetThrowInfo( pInfantryShotInfo );
	else
		pOwner->GetShotInfo( pInfantryShotInfo );

	pInfantryShotInfo->cShell = nShellType;
	pInfantryShotInfo->pWeapon = pWeapon;
	pInfantryShotInfo->time = time;
	pInfantryShotInfo->vDestPos = vLastShotPoint;
}

const NTimer::STime CBasicGun::GetActionPoint() const
{
	if ( pOwner->GetStats()->IsInfantry() && pCommonGunInfo->nGun == 1 )
	{
		const int nAnim = GetAnimationFromAction( checked_cast_ptr<CSoldier*>(pOwner)->GetThrowAction() );
		const NTimer::STime nAnimLength = pOwner->GetStats()->GetAnimTime( nAnim );
		const NTimer::STime nAnimAP = pOwner->GetStats()->GetAnimActionTime( nAnim );
		return nAnimAP > nAnimLength ? 0 : nAnimAP;					//CRAP - if APoint is too big, use 0
	}
	else
		return pOwner->GetStats()->GetAnimActionTime( GetAnimationFromAction( pOwner->GetShootAction() ) );
}

bool CBasicGun::CanBreakArmor( CAIUnit *pTarget ) const
{
	int nSide ;
	if ( pOwner->GetZ() > pTarget->GetZ() ) // стрельба из самолета по крышам 
	{
		nSide = RPG_TOP;
	}
	else
	{
		nSide = pTarget->GetUnitRect().GetSide( pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform ) );
	}
	return CanBreach( pTarget, nSide );
}

bool CBasicGun::CanShootByHeight( const float fZ ) const
{
	const float fOwnerZ = GetOwner()->GetZ();
	const bool bCeilingOK = fOwnerZ > fZ || sqr( fOwnerZ - fZ ) <= sqr( pWeapon->nCeiling ) || 
													pOwner->GetStats()->IsAviation();
	return bCeilingOK;
}

bool CBasicGun::CanShootByHeight( CAIUnit *pTarget ) const
{
	const float fTargetZ = pTarget->GetZ();
	return pOwner->GetStats()->IsAviation() || CanShootByHeight( fTargetZ );
}

float CBasicGun::GetFireRangeMax() const
{
	return pWeapon->fRangeMax;
}

float CBasicGun::GetFireRange( float fZ ) const
{
	const SUnitBaseRPGStats * pStats = GetOwner()->GetStats();

	if (	GetShell().etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE &&
				!pStats->IsAviation() && ( pStats->etype != RPG_TYPE_ART_AAGUN || fZ == 0.0f ) )
	{
		return Min( GetFireRangeMax(), SConsts::MAX_FIRE_RANGE_TO_SHOOT_BY_LINE );
	}
	else
		return GetFireRangeMax();
}

bool CBasicGun::InFireRange( const CVec3 &vPoint ) const
{
	const float fDist = fabs2( (pOwner->GetCenterTile() - AICellsTiles::GetTile( vPoint.x, vPoint.y )).ToCVec2() );

	float fMaxRange = GetFireRange( vPoint.z );
	fMaxRange /= SConsts::TILE_SIZE;

	return 
		fDist <= sqr( fMaxRange ) &&
		( pOwner->GetZ() <= 0.0f && vPoint.z > 0.0f || fDist >= sqr( pWeapon->fRangeMin / SConsts::TILE_SIZE ) );
}

bool CBasicGun::InFireRange( CAIUnit *pTarget ) const
{
	const CVec2 vPoint = pTarget->GetCenterPlain();
	float fDist = fabs( (pOwner->GetCenterTile() - AICellsTiles::GetTile( vPoint )).ToCVec2() );

	float fMaxRange;
	if ( pTarget->GetStats()->IsAviation() || pOwner->GetStats()->IsAviation() )
		fMaxRange = GetFireRangeMax() / SConsts::TILE_SIZE;
	else
	{
		if ( GetShell().etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE )
			fMaxRange = Min( GetFireRange( pTarget->GetZ() ), SConsts::MAX_FIRE_RANGE_TO_SHOOT_BY_LINE );
		else
			fMaxRange = GetFireRange( pTarget->GetZ() );

		fMaxRange /= SConsts::TILE_SIZE;
	}

	float fDist4MaxRange;
	const bool bIsTargetInGunCrew = 
		pTarget->GetFormation() && pTarget->GetFormation()->GetState() && pTarget->GetFormation()->GetState()->GetName() == EUSN_GUN_CREW_STATE;

	if ( pOwner->IsMoving() && pTarget->CanMove() && !pTarget->NeedDeinstall() && !bIsTargetInGunCrew )
	{
		fDist4MaxRange = sqr( fDist + 3.0f );
		fDist *= fDist;
	}
	else
		fDist = fDist4MaxRange = sqr( fDist );

	return 
		fDist4MaxRange <= sqr( fMaxRange ) && 
		( pOwner->GetZ() <= 0.0f && pTarget->GetZ() > 0.0f || fDist >= sqr( pWeapon->fRangeMin / SConsts::TILE_SIZE ) );
}

bool CBasicGun::InGoToSideRange( const CAIUnit *pTarget ) const
{
	const float fDist = fabs2( pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform ) - pTarget->GetCenterPlain() );
	return ( fDist <= sqr( 2 * GetFireRange(pTarget->GetZ() ) ) );
}

bool CBasicGun::TooCloseToFire( const CAIUnit *pTarget ) const
{
	if ( pOwner->GetZ() <= 0.0f && pTarget->GetZ() > 0.0f )
		return false;
	else
	{
		const float fDist = SquareOfDistance( pOwner->GetCenterTile(), pTarget->GetCenterTile() );
		return ( fDist < fabs2( pWeapon->fRangeMin / SConsts::TILE_SIZE ) );
	}
}

bool CBasicGun::TooCloseToFire( const CVec3 &vPoint ) const
{
	if ( pOwner->GetZ() <= 0.0f && vPoint.z > 0.0f )
		return false;
	else
	{
		const float fDist = SquareOfDistance( pOwner->GetCenterTile(), AICellsTiles::GetTile( vPoint.x, vPoint.y ) );
		return ( fDist < fabs2( pWeapon->fRangeMin / SConsts::TILE_SIZE ) );
	}
}

void CBasicGun::ToRestStateWOParallel()
{
	shootState = EST_REST;
	lastCheck = curTime;
	pCommonGunInfo->bFiring = false;
	target = VNULL2;

	StopTracing();

	if ( CTurret *pTurret = GetTurret() )
	{
		if ( !pTurret->IsLocked( this ) )
			pTurret->StopTurning();
	}

	pEnemy = 0;
}

void CBasicGun::ToRestState()
{
	shootState = EST_REST;
	lastCheck = curTime;
	pCommonGunInfo->bFiring = false;
	target = VNULL2;

	StopTracing();
	
	if ( CTurret *pTurret = GetTurret() )
	{
		if ( !pTurret->IsLocked( this ) )
			pTurret->StopTurning();
	}

	for ( CParallelGuns::iterator iter = parallelGuns.begin(); iter != parallelGuns.end(); ++iter )
		(*iter)->StopFire();
/*
	if ( IsValidObj( pEnemy ) )
		TraceAim( pEnemy );
*/
	pEnemy = 0;
}

void CBasicGun::Turning()
{
	bool bRightDir = false;
	CVec2 pos2Turn;
	
	if ( 	eType == IGunsFactory::TORPEDO_GUN ) 
	{
		if ( lastEnemyPos == VNULL2 || lastCheckTurnTime + 500 < curTime )	// recheck 2 times a second
		{
			pos2Turn = GetShootingPoint();
			lastCheckTurnTime = curTime;
		}
		else
			pos2Turn = lastEnemyPos;			// Cheat to allow the torpedo to shoot
	}
	else 
		pos2Turn = GetShootingPoint();

	if ( pEnemy != 0 )
		bRightDir = TurnGunToEnemy( pos2Turn, pEnemy->GetZ() - pOwner->GetZ() );
	else
		bRightDir = TurnGunToEnemy( pos2Turn, z );

	lastEnemyPos = pos2Turn;

	if ( bRightDir )
	{
		lastCheck = curTime;
		shootState = EST_AIMING;
		if ( !pOwner->IsInfantry() && pCommonGunInfo->nGun != 1 )
			updater.AddUpdate( 0, pOwner->GetAimAction(), pOwner, -1 );

		lastEnemyPos = VNULL2;
		if ( 	eType == IGunsFactory::TORPEDO_GUN ) 			// Cheat to allow the torpedo to shoot with lead
		{
			pEnemy = 0;
			target = pos2Turn;
		}
	}
}

bool CBasicGun::IsFiring() const 
{ 
	//return shootState != EST_REST;
	if ( shootState != EST_REST )
		return true;
	for ( CParallelGuns::const_iterator iter = parallelGuns.begin(); iter != parallelGuns.end(); ++iter )
	{
		if ( (*iter)->IsFiring() )
			return true;
	}
	return false;
}

void CBasicGun::Aiming()
{
	// враг убежал из радиуса обстрела
	if ( pEnemy != 0 && !InFireRange( pEnemy ) )
		StopFire();
	// враг убежал из прицела
	else if ( !CanShootWOGunTurn( 1, z ) )
	{
		bAim = true;
		shootState = EST_TURNING;
	}
	// прицелились и перезарядились
	else 
	{
		// чтобы не сразу стрелять, а перезаряжаться после подвоза патронов
		if ( GetNAmmo() == 0 )
		{
			if ( pOwner->GetStats()->IsAviation() )			// but PLANES mustn't wait
				shootState = EST_REST;
			lastCheck = curTime;
		}

		const int nAimingTime = GetAimTime();
		const bool b1 = curTime - lastCheck >= nAimingTime * bAim ;
		const bool b2 = pCommonGunInfo->lastShoot == 0 || 
			curTime - pCommonGunInfo->lastShoot >= GetRelaxTime() + pWeapon->nAimingTime * bAim;

		if (  b1 && b2 && bCanShoot && GetNAmmo() > 0 )
		{
			updater.AddUpdate( 0, ACTION_NOTIFY_DELAYED_SHOOT, pOwner, -1 );

			NI_ASSERT( pEnemy == 0 || pEnemy->IsRefValid() && pEnemy->IsAlive(), "Dead enemy" );
			if ( pEnemy != 0 )
			{
				target = pEnemy->GetCenterPlain();
				z = pEnemy->GetZ();

				if ( eType == IGunsFactory::ROCKET_GUN )			// Add deviation to z for rocket guns
				{
					const float fZ = GetHeights()->GetVisZ( target.x, target.y );
					const float fDisp = GetDispByRadius( this, fabs( pOwner->GetCenterPlain() - target ) );
					z = Max( fZ, NRandom::Random( z - fDisp, z + fDisp ) );
				}
			}


			shootState = WAIT_FOR_ACTION_POINT;
			lastCheck = curTime;

			// It's a grenade, run animation in advance
			if ( pOwner->GetStats()->IsInfantry() && pCommonGunInfo->nGun == 1 )
			{
				updater.AddUpdate( 0, ACTION_NOTIFY_INFANTRY_SHOOT, this, -1 );
			}
		}
	}
}

void CBasicGun::WaitForActionPoint()
{
	if ( curTime - lastCheck >= GetActionPoint() )
	{
		shootState = EST_SHOOTING;
		pOwner->RemoveCamouflage( ECRR_SELF_SHOOT );
		nShotsLast = pWeapon->nAmmoPerBurst;
		pCommonGunInfo->lastShoot = curTime - GetFireRate();

		vLastShotPoint = CVec3( target, z );

		if ( pOwner->GetStats()->IsInfantry() )	
		{
			if ( pCommonGunInfo->nGun != 1 )	// animation for grenades is played in advance
				updater.AddUpdate( 0, ACTION_NOTIFY_INFANTRY_SHOOT, this, -1 );
		}
		// для зениток выстрел нужно присылать на каждый снаряд, action point отсутствует
		else if ( pOwner->GetStats()->etype != RPG_TYPE_ART_AAGUN && pOwner->GetStats()->etype != RPG_TYPE_ART_ROCKET )
			updater.AddUpdate( 0, ACTION_NOTIFY_MECH_SHOOT, this, -1 );
	}
}

void CBasicGun::Shooting()
{
	if ( pEnemy && ( !IsValidObj( pEnemy ) || !pEnemy->IsAlive() ) )
	{
		shootState = EST_REST;
		lastCheck = curTime;
		pCommonGunInfo->bFiring = false;
		return;
	}

	// время для выстрела и ещё есть патроны в очереди

	while ( curTime - pCommonGunInfo->lastShoot >= GetFireRate() && nShotsLast > 0 && pCommonGunInfo->nAmmo > 0 )
	{
		bool bShowBombEffect = true;
		if ( pOwner && pOwner->GetStats() &&
			( pOwner->GetStats()->etype == NDb::RPG_TYPE_AVIA_BOMBER || pOwner->GetStats()->etype == NDb::RPG_TYPE_AVIA_SUPER ) &&
			nShotsLast%g_nEffectsForBombs != 0 )
			bShowBombEffect = false;
		// check if actual shooting can be done ( some buildings, movement during aim can change conditions)
		if ( fabs( z ) < SConsts::TILE_SIZE && eType != IGunsFactory::ROCKET_GUN )
		{
			const float fZ = GetHeights()->GetVisZ( target.x, target.y ); 
			if ( CanShotBecauseOfObstacles( target, fZ ) )
				Fire( target, fZ, bShowBombEffect );
			else
			{
				ToRestStateWOParallel();
				return;
			}
		}
		else
		{
			if ( CanShotBecauseOfObstacles( target, z ) )
				Fire( target, z, bShowBombEffect );
			else
			{
				ToRestStateWOParallel();
				return;
			}
		}

		if ( pOwner->GetStats()->etype == RPG_TYPE_ART_AAGUN || pOwner->GetStats()->etype == RPG_TYPE_ART_ROCKET )
			updater.AddUpdate( 0, ACTION_NOTIFY_MECH_SHOOT, this, -1 );

		--nShotsLast;

		pCommonGunInfo->lastShoot += GetFireRate();
		
		// Subtract appropriate number of shells
		pCommonGunInfo->nAmmo -= Min( pCommonGunInfo->nAmmo, GetOwner()->GetMultiShot() );
		pOwner->CheckAmmoStatus();

		CPtr<CBuilding> pBuilding = GetMountBuilding();
		if ( pBuilding )
			updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, pBuilding, -1 );

		updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, GetOwner(), -1 );
	}
	if ( nShotsLast == 0 || pCommonGunInfo->nAmmo == 0 )
	{
		bWaitForReload = true;
		ToRestStateWOParallel();

		//if ( !bParallelGun )
		//	ToRestStateWOParallel();
		//else
		//{
		//	shootState = EST_REST;
		//	lastCheck = curTime;
		//	pCommonGunInfo->bFiring = false;

		//	if ( pEnemy != 0 )
		//	{
		//		if ( IsValidObj( pEnemy ) )
		//			StartEnemyBurst( pEnemy, false );
		//	}
		//	else
		//		StartPointBurst( CVec3( target, z ), false );
		//}
	}
}

const CVec2 CBasicGun::GetShootingPoint() const
{
	if ( !pEnemy )
		return target;

	CVec2 vEnemyPos = pEnemy->GetCenterPlain();

	// Torpedoes shoot with lead (range correction)
	if ( eType == IGunsFactory::TORPEDO_GUN ) 
	{
		const CVec2 vOwnPos = pOwner->GetCenterPlain();
		const float fEnemySpeed = pEnemy->GetSpeed();
		const float fTorpedoSpeed = GetShell().fSpeed;
		CVec2 vEnemySpeed = pEnemy->GetDirectionVector();
		Normalize( &vEnemySpeed );
		if ( fEnemySpeed > 0.1f )		// If the enemy is moving, shoot for the nose
			vEnemyPos += vEnemySpeed * pEnemy->GetAABBHalfSize().y;
		vEnemySpeed *= fEnemySpeed;

		if ( fEnemySpeed != 0.0f )
		{
			// now we need to solve a quadratic equation
			const float f2A = 2.0f * ( fEnemySpeed * fEnemySpeed - fTorpedoSpeed * fTorpedoSpeed );
			const float fDx = vEnemyPos.x - vOwnPos.x;
			const float fDy = vEnemyPos.y - vOwnPos.y;
			const float fB = 2.0f * ( fDx * vEnemySpeed.x + fDy * vEnemySpeed.y );
			const float fC = fDx * fDx + fDy * fDy;

			float fTimeOfHit = 0.0f;

			// Check if speeds are different enough (to avoid large numbers)
			if ( fabs( f2A ) < 0.1f )
			{
				fTimeOfHit = - fC / fB;
			}
			else
			{
				float fDisc = fB * fB - 2.0f * f2A * fC;

				if ( fDisc < 0.0f )
					return vEnemyPos;			// If cannot hit, shoot directly

				fDisc = sqrt( fDisc );

				const float fT1 = ( - fB + fDisc ) / f2A;
				const float fT2 = ( - fB - fDisc ) / f2A;

				if ( fT1 <= 0.0f && fT2 <= 0.0f )
					return vEnemyPos;			// If cannot hit, shoot directly
				else if ( fT1 <= 0.0f && fT2 > 0.0f )
					fTimeOfHit = fT2;			// Get the positive one
				else if ( fT1 > 0.0f && fT2 <= 0.0f )
					fTimeOfHit = fT1;			// Get the positive one
				else
					fTimeOfHit = ( fT1 > fT2 ) ? fT2 : fT1;		// Get the smallest of 2 positives
			}

			// Calculate position
			CVec2 vPlaceOfHit( vEnemyPos + vEnemySpeed * fTimeOfHit );

			return vPlaceOfHit;
		}
	}

	return vEnemyPos;
}

WORD CBasicGun::GetVisAngleOfAim() const
{
	if ( pEnemy == 0 )
		return 0;
	else
		return GetVisibleAngle( pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform ), pEnemy->GetUnitRect() ) / 2;
}

bool CBasicGun::CanShootWOGunTurn( const BYTE cDeltaAngle, const float fZ )
{
	return pEnemy ? CanShootWOGunTurn( pEnemy, cDeltaAngle ) :
		( IsGoodAngle( target, 0, fZ, cDeltaAngle ) && CanShootToPointWOMove( target, fZ ) );
}

bool CBasicGun::CanShootToTargetWOMove()
{
	return	pEnemy ? CanShootToUnitWOMove( pEnemy ) : CanShootToPointWOMove( target, z );
}

void CBasicGun::SetOwner( CAIUnit *_pOwner )
{
	pOwner = _pOwner;
	nOwnerParty = pOwner ? pOwner->GetParty() : 0;
	if ( pOwner )
	{
		pWeapon = GetWeapon();
		if ( !pWeapon )
			pWeapon = pOwner->GetStats()->GetGun( pOwner->GetUniqueID(), pCommonGunInfo->nPlatform, pCommonGunInfo->nGun ).pWeapon;
	}
}

void CBasicGun::Segment()
{
	SetAlive( GetOwner()->IsAlive() );
	NI_ASSERT( !pOwner || nOwnerParty == pOwner->GetParty(), "Wrong owner party" );

	// врага убили
	if ( shootState != EST_REST && IsValid( pEnemy ) && !pEnemy->IsAlive() )
	{
		// момент выпускания очереди - стрельбу не прерывать 
		if ( shootState == EST_SHOOTING || shootState == WAIT_FOR_ACTION_POINT )
			pEnemy = 0;
		else
			ToRestState();
		return;
	}
	
	if ( shootState != EST_REST && shootState != EST_TURNING && shootState != EST_AIMING &&
			 GetGun().nPriority == 0 && !pOwner->CanShootInMovement() && !pOwner->IsIdle() )
	{
		ToRestState();
		return;
	}

	if ( pEnemy )
	{
		pCanShootCachedEnemy = 0;
		bCanShootToUnitWOMove = CanShootToUnitWOMove( pEnemy );
		pCanShootCachedEnemy = pEnemy;
	}
	else
	{
		pCanShootCachedEnemy = 0;
		bCanShootToUnitWOMove = false;
	}

	bool bTryFastShot = false;
	switch ( shootState )
	{
		case EST_REST:
			Rest();

			break;
		case EST_TURNING:
			if ( pEnemy != 0 && !pEnemy->IsVisible( nOwnerParty ) )
				ToRestState();
			else
			{
				OnTurningState();
				if ( EST_TURNING != shootState )
					bTryFastShot = true;
			}

			break;
		case EST_AIMING:
			if ( pEnemy != 0 && !pEnemy->IsVisible( nOwnerParty ) )
				ToRestState();
			else
			{
				OnAimState();
				if ( EST_AIMING != shootState )
					bTryFastShot = true;
			}

			break;
		case WAIT_FOR_ACTION_POINT:
			OnWaitForActionPointState();
			if ( WAIT_FOR_ACTION_POINT != shootState )
				bTryFastShot = true;

			break;
		case EST_SHOOTING:
			Shooting();

			break;
	}

	if ( bTryFastShot )
	{
		if ( EST_AIMING == shootState )
		{
			OnAimState();
		}

		if ( WAIT_FOR_ACTION_POINT == shootState )
			OnWaitForActionPointState();

		if ( EST_SHOOTING == shootState )
			Shooting();
	}
}

void CBasicGun::OnWaitForActionPointState()
{
	WaitForActionPoint();
}

void CBasicGun::OnTurningState()
{
	if ( AnalyzeTurning() )
		shootState = EST_AIMING;
}

void CBasicGun::OnAimState()
{
	if ( !CanShootToTargetWOMove() )
		StopFire();
	else
		Aiming();
}

void CBasicGun::StartPlaneBurst( CAIUnit *_pEnemy, bool bReAim )
{
	if ( _pEnemy && _pEnemy->IsRefValid() && _pEnemy->IsAlive() )
	{
		pEnemy = _pEnemy;
		lastCheck = curTime;
		bAim = bReAim;
		lastEnemyPos = VNULL2;
		target = pEnemy->GetCenterPlain();
		z = pEnemy->GetZ();

		StopTracing();	
		
		if ( IsGoodAngle( pEnemy->GetCenterPlain(), 
											GetVisibleAngle( pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform ), pEnemy->GetUnitRect() ) / 2, 
											pEnemy->GetZ(), 
											!bReAim || pOwner->GetStats()->IsAviation() ) )
		{
			shootState = EST_AIMING;
			if ( bAim || curTime - pCommonGunInfo->lastShoot < GetRelaxTime() )
				updater.AddUpdate( 0, pOwner->GetAimAction(), pOwner, -1 );
		}
	}
}

void CBasicGun::StartPointBurst( const CVec3 &_target, bool bReAim )
{
	if ( !(pCommonGunInfo->bFiring) && ( shootState == EST_REST || pEnemy != 0 || CVec3( target, z ) != _target ) )
	{
		target.x = _target.x;
		target.y = _target.y;
		z = _target.z;
		lastCheck = curTime;
		bAim = bReAim;
		lastEnemyPos = VNULL2;
		pCommonGunInfo->bFiring = true;

		StopTracing();

		if ( IsGoodAngle( target, 0, z, !bReAim || pOwner->GetStats()->IsAviation() ) )
		{
			shootState = EST_AIMING;

			if ( bAim || curTime - pCommonGunInfo->lastShoot < GetRelaxTime() )
				updater.AddUpdate( 0, pOwner->GetAimAction(), pOwner, -1 );
		}
		else
		{
			bAim = true;
			shootState = EST_TURNING;
		}

		pEnemy = 0;
		
		for ( CParallelGuns::iterator iter = parallelGuns.begin(); iter != parallelGuns.end(); ++iter )
			(*iter)->StartPointBurst( _target, bReAim );
	}
}

void CBasicGun::StartPointBurst( const CVec2 &_target, bool bReAim )
{
	if ( !(pCommonGunInfo->bFiring) && ( shootState == EST_REST || pEnemy != 0 || target != _target ) )
	{
		target = _target;
		z = 0;
		lastCheck = curTime;
		bAim = bReAim;
		lastEnemyPos = VNULL2;
		pCommonGunInfo->bFiring = true;

		StopTracing();

		if ( IsGoodAngle( target, 0, 0, !bReAim || pOwner->GetStats()->IsAviation() ) )
		{
			shootState = EST_AIMING;

			if ( bAim || curTime - pCommonGunInfo->lastShoot < GetRelaxTime() )
				updater.AddUpdate( 0, pOwner->GetAimAction(), pOwner, -1 );
		}
		else
		{
			bAim = true;
			shootState = EST_TURNING;
		}

		pEnemy = 0;

		for ( CParallelGuns::iterator iter = parallelGuns.begin(); iter != parallelGuns.end(); ++iter )
			(*iter)->StartPointBurst( _target, bReAim );
	}
}

void CBasicGun::StartEnemyBurst( CAIUnit *_pEnemy, bool bReAim )
{
	if ( IsValidObj( _pEnemy ) && !(pCommonGunInfo->bFiring) && ( shootState == EST_REST || pEnemy != _pEnemy ) )
	{
		pEnemy = _pEnemy;
		lastCheck = curTime;
		bAim = bReAim;
		lastEnemyPos = VNULL2;
		pCommonGunInfo->bFiring = true;
		target = pEnemy->GetCenterPlain();
		z = pEnemy->GetZ();

		StopTracing();

		if ( IsGoodAngle( target, 
			GetVisibleAngle( pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform ), 
			pEnemy->GetUnitRect() ) / 2, z, 
			!bReAim || pOwner->GetStats()->IsAviation() ) )
		{
			shootState = EST_AIMING;

			if ( bAim || curTime - pCommonGunInfo->lastShoot < GetRelaxTime() )
			{
				if ( !pOwner->IsInfantry() && pCommonGunInfo->nGun != 1 )
					updater.AddUpdate( 0, pOwner->GetAimAction(), pOwner, -1 );
			}
		}
		else
		{
			bAim = true;
			shootState = EST_TURNING;
		}

		for ( CParallelGuns::iterator iter = parallelGuns.begin(); iter != parallelGuns.end(); ++iter )
			(*iter)->StartEnemyBurst( _pEnemy, bReAim );
	}
}

const SBaseGunRPGStats& CBasicGun::GetGun() const
{
	NI_ASSERT( pOwner->IsRefValid(), "Wrong owner. Can't get gun info" );
	return pOwner->GetStats()->GetGun( pOwner->GetUniqueID(), pCommonGunInfo->nPlatform, pCommonGunInfo->nGun );
}

const SWeaponRPGStats* CBasicGun::GetWeapon() const 
{ 
	return pWeapon;
}

const SWeaponRPGStats::SShell& CBasicGun::GetShell() const 
{ 
	return pWeapon->shells[nShellType];
}

bool CBasicGun::IsRelaxing() const
{
	return curTime - pCommonGunInfo->lastShoot < GetRelaxTime();
}

bool CBasicGun::CanShootWOGunTurn( CAIUnit *pEnemy, const BYTE cDeltaAngle )
{
	return
		CanShootToUnitWOMove( pEnemy ) &&
		( 
			GetOwner()->GetStats()->IsAviation() ? 
			IsGoodAngle( pEnemy->GetCenterPlain(), GetVisibleAngle( pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform ), pEnemy->GetUnitRect() ) / 2, pEnemy->GetCenter().z, cDeltaAngle ) :
			IsGoodAngle( pEnemy->GetCenterPlain(), GetVisibleAngle( pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform ), pEnemy->GetUnitRect() ) / 2, pEnemy->GetZ(), cDeltaAngle )
		);
}

const NTimer::STime CBasicGun::GetRestTimeOfRelax() const
{
	return 
		Max( GetRelaxTime() - ( curTime - pCommonGunInfo->lastShoot ), 0.0f );
}

bool CBasicGun::AnalyzeLimitedAngle( class CCommonUnit *pUnit, const CVec2 &point ) const
{
	NI_ASSERT( dynamic_cast<CSoldier*>(pUnit) != 0, "Wrong unit to analyze limited angle" );
	CSoldier *pSoldier = static_cast<CSoldier*>( pUnit );

	if ( pSoldier->IsAngleLimited() )
		return IsInTheAngle( GetDirectionByVector( point - pSoldier->GetCenterPlain() ), pSoldier->GetMinAngle(), pSoldier->GetMaxAngle() );

	return true;
}

bool CBasicGun::CanShootToUnitWOMove( CAIUnit *pTarget )
{
	if ( pCanShootCachedEnemy == pTarget )
		return bCanShootToUnitWOMove;

	if ( !pTarget || !pTarget->IsAlive() || pTarget == GetOwner() ||  pTarget->GetState()->GetName() == EUSN_PARTROOP )
	{
		SetRejectReason( ACK_INVALID_TARGET );
		return false;
	}

	if( GetShell().etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_TORPEDO )
	{
		if ( GetTerrain()->IsLocked( pTarget->GetCenterTile(), EAC_WATER ) )
		{
			SetRejectReason( ACK_INVALID_TARGET );
			return false;
		}
	}

	if ( pOwner->GetStats()->IsAviation() )
	{
		const CVec3 vEnemyCenter( pTarget->GetCenter() );
		const CVec3 vOwnerCenter( pOwner->GetCenter() );
		if ( !CanShootToPointWOMove( pTarget->GetCenterPlain(), vEnemyCenter.z, GetVisibleAngle( pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform ), pTarget->GetUnitRect() ) / 2, vEnemyCenter.z - vOwnerCenter.z, pTarget ) )
			return false;
	}
	else 
	{
		if ( !CanShootToPointWOMove( pTarget->GetCenterPlain(), pTarget->GetZ(), GetVisibleAngle( pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform ), pTarget->GetUnitRect() ) / 2, pTarget->GetZ() - pOwner->GetZ(), pTarget ) )
			return false;
	}

	if ( pTarget->GetStats()->IsInfantry() && !AnalyzeLimitedAngle( pTarget, pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform ) ) )
	{
		SetRejectReason( ACK_NOT_IN_ATTACK_ANGLE );
		return false;
	}

	if ( !CanShootByHeight( pTarget ) )
	{
		SetRejectReason( ACK_NOT_IN_FIRE_RANGE );
		return false;
	}

	if ( !CanBreakArmor( pTarget ) && !pOwner->IsTargetingTrack() )	// Track Targetting ignores armor
	{
		SetRejectReason( ACK_CANNOT_PIERCE );
		return false;
	}

	return true;
}

bool CBasicGun::CanShootToUnit( CAIUnit *pEnemy )
{
	if ( !pEnemy || !pEnemy->IsAlive() || pEnemy == GetOwner() )
	{
		SetRejectReason( ACK_INVALID_TARGET );
		return false;
	}
	
	if ( !pOwner->CanMove() || pOwner->NeedDeinstall() || pOwner->IsLocked( this ) )
		return CanShootToUnitWOMove( pEnemy );
	
	if ( GetNAmmo() == 0 && pOwner->CanMove() && 
		   !pOwner->NeedDeinstall() && 
			 pWeapon->shells[nShellType].etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE )
	{
		SetRejectReason( ACK_NO_AMMO );
		return false;
	}

	if ( !CanShootByHeight( pEnemy ) )
	{
		SetRejectReason( ACK_NOT_IN_FIRE_RANGE );
		return false;
	}
	
	if ( !CanBreach( pEnemy ) )
	{
		SetRejectReason( ACK_CANNOT_PIERCE );
		return false;
	}

	return true;
}

bool CBasicGun::CanShootToObjectWOMove( CStaticObject *pObj )
{
	if ( !pObj->IsRefValid() || !pObj->IsAlive() )
	{
		SetRejectReason( ACK_INVALID_TARGET );
		return false;
	}

	const CVec2 vOwnerCenter( pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform ) );
	const CVec2 vAttackCenter( pObj->GetAttackCenter( vOwnerCenter ) );
	if ( !CanShootToPointWOMove( vAttackCenter, 0.0f ) )
		return false;
	
	const SVector vAttackTile( AICellsTiles::GetTile( vAttackCenter ) );
	if ( !theWarFog.IsTileVisible( vAttackTile, pOwner->GetParty() ) )
		return false;

	// проверка на возможность пробивания брони
	{
		SRect boundRect;
		pObj->GetBoundRect( &boundRect );
		const WORD wDir2Obj( GetDirectionByVector( pObj->GetAttackCenter( vOwnerCenter ) - vOwnerCenter ) );
		
		const int nSide = IsBallisticTrajectory() ? RPG_TOP : boundRect.GetSide( wDir2Obj );
		if ( GetMaxPossiblePiercing() < pObj->GetStats()->GetMinPossibleArmor( nSide ) )
		{
			//SetRejectReason( ACK_CANNOT_PIERCE );
			// don't want to hear this ack on buildings
			SetRejectReason( ACK_NEGATIVE );
			return false;
		}
	}

	return true;
}

bool CBasicGun::CanShootToObject( CStaticObject *pObj )
{
	if ( !pObj->IsRefValid() || !pObj->IsAlive() )
	{
		SetRejectReason( ACK_INVALID_TARGET );
		return false;
	}

	if ( !pOwner->CanMove() || pOwner->NeedDeinstall() || pOwner->IsLocked( this ) )
		return CanShootToObjectWOMove( pObj );

	const NDb::SWeaponRPGStats::SShell::ETrajectoryType eTraj = pWeapon->shells[nShellType].etrajectory;

	if ( GetNAmmo() == 0 && pOwner->CanMove() && !pOwner->NeedDeinstall() && 
		   eTraj == NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE )
	{
		SetRejectReason( ACK_NO_AMMO );
		return false;
	}

	const int nMaxPossiblePiercing = GetMaxPossiblePiercing();
	if ( eTraj != NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE && 
		   nMaxPossiblePiercing < pObj->GetStats()->defences[RPG_TOP].nArmorMin ||
			 eTraj == NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE && 
			 nMaxPossiblePiercing < pObj->GetStats()->GetMinPossibleArmor( RPG_FRONT ) )
	{
		//SetRejectReason( ACK_CANNOT_PIERCE );
		// don't want to hear this ack on buildings
		SetRejectReason( ACK_NEGATIVE );
		return false;
	}

	SetRejectReason( ACK_NONE );
	return true;
}

bool CBasicGun::CanShotBecauseOfObstacles( const CVec2 &point, const float fZ )
{
	if ( bIgnoreObstacles )
		return true;
	//
	const SVector vAimTile = GetAIMap()->GetTile( point.x, point.y );
	const SVector vAimVisTile( vAimTile.x / AI_TILES_IN_VIS_TILE, vAimTile.y / AI_TILES_IN_VIS_TILE );
	const SVector vUnitTile( pOwner->GetCenterTile() );
	const SVector vUnitVisTile( vUnitTile.x / AI_TILES_IN_VIS_TILE, vUnitTile.y / AI_TILES_IN_VIS_TILE );
	// check for out-of-map objects	
	if ( GetAIMap()->IsTileInside( vAimTile ) == false || GetAIMap()->IsTileInside( vUnitTile ) == false )
		return false;
	// check traceability
	if ( !theWarFog.IsTraceable( vUnitVisTile, vAimVisTile ) )
	{
		SetRejectReason( ACK_NOT_IN_FIRE_RANGE );
		return false;
	}
	return true;
}

bool CBasicGun::CanShootToPointWOMove( const CVec2 &point, const float fZ, const WORD wHorAddAngle, const WORD wVertAddAngle, CAIUnit *pEnemy )
{
	const CVec3 v3DTarget( point, fZ );

	if ( !IsBallisticTrajectory() && !CanShotBecauseOfObstacles( point, fZ ) )
	{
		SetRejectReason( ACK_NOT_IN_FIRE_RANGE );
		return false;
	}

	if( GetShell().etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_TORPEDO )
	{
		if ( GetTerrain()->IsLocked( GetAIMap()->GetTile( point ), EAC_WATER ) )
		{
			SetRejectReason( ACK_INVALID_TARGET );
			return false;
		}
	}

	if ( !pEnemy && !InFireRange( v3DTarget ) || pEnemy && !InFireRange( pEnemy ) )
	{
		SetRejectReason( ACK_NOT_IN_FIRE_RANGE );
		return false;
	}

	// disallow shooting to static object under war fog
	const NDb::SWeaponRPGStats::SShell::ETrajectoryType eTraj = pWeapon->shells[nShellType].etrajectory;
	if ( !pEnemy && eTraj == NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE )
	{
		const SVector vAttackTile( AICellsTiles::GetTile( point ) );
		if ( !theWarFog.IsTileVisible( vAttackTile, pOwner->GetParty() ) )
		{
			SetRejectReason( ACK_NOT_IN_FIRE_RANGE );
			return false;
		}
	}

	if ( !CanShootByHeight( fZ ) )
	{
		SetRejectReason( ACK_NOT_IN_FIRE_RANGE );
		return false;
	}

	// нельзя вращать базу
	if ( !pOwner->CanRotate() && !pOwner->CanMove() || pOwner->NeedDeinstall() || pOwner->IsLocked( this ) )
	{
		if ( !IsOnTurret() || IsOnTurret() && GetTurret()->IsLocked( this ) ) // нельзя вращать turret, или gun на базе
		{
			if ( !IsGoodAngle( point, wHorAddAngle, fZ, 1 ) )
			{
				SetRejectReason( ACK_NOT_IN_ATTACK_ANGLE );
				return false;
			}
		}
		else if ( !IsInShootCone( point, wHorAddAngle ) ) 
		{
			SetRejectReason( ACK_NOT_IN_ATTACK_ANGLE );
			return false;
		}
	}

	if ( pOwner->GetStats()->IsInfantry() && !AnalyzeLimitedAngle( pOwner, point ) )
	{
		SetRejectReason( ACK_NOT_IN_ATTACK_ANGLE );
		return false;
	}

	if ( GetNAmmo() == 0 && pOwner->CanMove() && !pOwner->NeedDeinstall() && 
		   eTraj == NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE )
	{
		SetRejectReason( ACK_NO_AMMO );
		return false;
	}

	SetRejectReason( ACK_NONE );
	return true;
}

bool CBasicGun::CanShootToPoint( const CVec2 &point, const float fZ, const WORD wHorAddAngle, const WORD wVertAddAngle )
{
	if ( !pOwner->CanMove() || pOwner->NeedDeinstall() || pOwner->IsLocked( this ) )
		return CanShootToPointWOMove( point, fZ, wHorAddAngle, wVertAddAngle );

	if ( GetNAmmo() == 0 && pOwner->CanMove() && !pOwner->NeedDeinstall() && 
		   pWeapon->shells[nShellType].etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE )
	{
		SetRejectReason( ACK_NO_AMMO );
		return false;
	}

	SetRejectReason( ACK_NONE );	
	return true;
}

bool CBasicGun::IsInShootCone( const CVec2 &point, const WORD wAddAngle ) const
{
	if ( !pOwner->InVisCone( point ) )
		return false;
	else
	{
		const WORD dirToPoint = GetDirectionByVector( point - pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform ) );

		if ( IsOnTurret() && !GetTurret()->IsLocked( this ) )
			return DirsDifference( dirToPoint, pOwner->GetFrontDirection() ) <= GetHorTurnConstraint() + (int)wAddAngle + (int)pWeapon->wDeltaAngle;
		else
			return DirsDifference( dirToPoint, pOwner->GetFrontDirection() - GetGun().wDirection ) <= (int)wAddAngle + (int)pWeapon->wDeltaAngle;
	}
}

const float CBasicGun::GetDispRatio( BYTE nShellType, const float fDist ) const
{
	int eTraj = pWeapon->shells[nShellType].etrajectory;
	float fMax = GetFireRangeMax();
	float fMin = pWeapon->fRangeMin;

	float fMaxDisp = SConsts::dispersionRatio[eTraj][0];
	float fMinDisp = SConsts::dispersionRatio[eTraj][1];

	return fMinDisp + (fDist-fMin)/(fMax-fMin)*(fMaxDisp-fMinDisp);
}

const float CBasicGun::GetDispersion() const
{
	return pOwner->GetStatsModifier()->weaponDispersion.Get( pWeapon->fDispersion );
}

const float CBasicGun::GetAimTime( bool bRandomize ) const 
{
	const float fAimTime = pOwner->GetStatsModifier()->weaponAimTime.Get( pWeapon->nAimingTime );
	if ( bRandomize )
		return fAimTime * fRandom4Aim;
	return fAimTime;
}

const float CBasicGun::GetRelaxTime( bool bRandomize ) const
{
	const float fRelaxTime = pOwner->GetStatsModifier()->weaponRelaxTime.Get( pWeapon->shells[nShellType].nRelaxTime );
	if ( bRandomize )
		return fRelaxTime * fRandom4Relax;
	return fRelaxTime;
}

const int CBasicGun::GetFireRate() const
{
	return pWeapon->shells[nShellType].nFireRate * pOwner->GetFireRateBonus();
}

bool CBasicGun::CanBreach( const CCommonUnit *pTarget ) const
{
	if ( pOwner->GetZ() > pTarget->GetZ() ) // сирельба из самолета по крышам 
	{
		return GetMaxPossiblePiercing() >= pTarget->GetArmor( RPG_TOP );
	}
	else if ( pWeapon->shells[nShellType].etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE )
		return GetMaxPossiblePiercing() >= pTarget->GetMinArmor();
	else
		return GetMaxPossiblePiercing() >= pTarget->GetArmor( RPG_TOP );
}

bool CBasicGun::CanBreach( const SHPObjectRPGStats *pStats, const int nSide ) const
{
	if ( pWeapon->shells[nShellType].etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE )
		return GetMaxPossiblePiercing() >= pStats->GetMinPossibleArmor( nSide );
	else
		return GetMaxPossiblePiercing() >= pStats->GetMinPossibleArmor( RPG_TOP );
}

bool CBasicGun::CanBreach( const CCommonUnit *pTarget, const int nSide ) const
{
	if ( pWeapon->shells[nShellType].etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE )
		return GetMaxPossiblePiercing() >= pTarget->GetMinPossibleArmor( nSide );
	else
		return GetMaxPossiblePiercing() >= pTarget->GetMinPossibleArmor( RPG_TOP );
}

bool CBasicGun::IsCommonEqual( const CBasicGun *pGun ) const
{
	return pGun != 0 && pOwner == pGun->GetOwner() && GetCommonGunNumber() == pGun->GetCommonGunNumber();
}

IBallisticTraj* CBasicGun::CreateTraj( const CVec3 &vTarget ) const
{
	switch ( eType )
	{
		case IGunsFactory::MOMENT_CML_GUN:
		case IGunsFactory::TORPEDO_GUN:
		case IGunsFactory::MOMENT_BURST_GUN:
			return 0;

		case IGunsFactory::ROCKET_GUN:
		case IGunsFactory::FLAME_GUN:
			return new CAARocketTraj( CVec3( pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform ), pOwner->GetZ() ), vTarget, pOwner->GetStatsModifier()->weaponShellSpeed.Get( pWeapon->shells[nShellType].fSpeed ) );

		case IGunsFactory::VIS_CML_BALLIST_GUN:
		case IGunsFactory::VIS_BURST_BALLIST_GUN:
			return new CBallisticTraj( GetHeights()->Get3DPoint( pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform ) ), vTarget, pOwner->GetStatsModifier()->weaponShellSpeed.Get( GetShell().fSpeed ), pWeapon->shells[nShellType].etrajectory, GetVerTurnConstraint(), GetFireRange(z) );
			break;
		case IGunsFactory::PLANE_GUN:
			NI_ASSERT( false, "wrong call " );
			break;
	}

	return 0;
}

void CBasicGun::Fire( const CVec2 &target, const float z, const bool bShowBombEffect )
{
#ifndef _FINALRELEASE
	if ( NGlobal::GetVar( "gunfire_markers", 0 ) )
	{
		const CVec2 vOwnerCenter = pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform );
		CSegment segm;
		segm.p1 = vOwnerCenter;
		segm.p2 = target;
		segm.dir = segm.p2 - segm.p1;
		DebugInfoManager()->DeleteObject( GetUniqueId() );
		DebugInfoManager()->CreateSegment( GetUniqueId(), segm, 1, NDebugInfo::WHITE );
	}
#endif
	const CVec2 vOwnerCenter = pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform );
	const float fOwnerZ( pOwner->GetZ() );
	const int nShotsAtOnce = Min( GetOwner()->GetMultiShot(), GetNAmmo() );
	for ( int i = 0; i < nShotsAtOnce; ++i )
	{
		switch ( eType )
		{
		case IGunsFactory::FLAME_GUN:
			{
				CVec2 vDir ( target - vOwnerCenter );
				Normalize( &vDir );
				const CVec2 vExplosion( vOwnerCenter + vDir * GetFireRangeMax() );
				// to disallow kill themselves
				const CVec2 vStart ( vOwnerCenter + vDir * ( 64 + Min( GetWeapon()->fRangeMin, pWeapon->shells[nShellType].fArea ) ) );
				
				CPtr<CFlameThrowerExpl> pExpl = new CFlameThrowerExpl( pOwner, this, CVec3( vExplosion, z ), CVec3( vStart, fOwnerZ ), nShellType );
				theShellsStore.AddShell( new CVisShell( pExpl, CreateTraj( pExpl->GetExplCoordinates() ), GetCommonGunNumber(), GetPlatform() ) );
			}

			break;
		case IGunsFactory::MOMENT_CML_GUN:
			theShellsStore.AddShell( 
				new CInvisShell( 
				curTime + fabs( fabs(target - vOwnerCenter), fOwnerZ - z ) / pOwner->GetStatsModifier()->weaponShellSpeed.Get( pWeapon->shells[nShellType].fSpeed ), 
				new CCumulativeExpl( pOwner, this, CVec3(target, z), CVec3( vOwnerCenter, fOwnerZ), nShellType ), 
				GetCommonGunNumber() 
				)
				);

			break;
		case IGunsFactory::MOMENT_BURST_GUN:
			theShellsStore.AddShell( 
				new CInvisShell( 
				curTime + fabs( fabs(target - vOwnerCenter), fOwnerZ - z ) / pOwner->GetStatsModifier()->weaponShellSpeed.Get( pWeapon->shells[nShellType].fSpeed ), 
				new CBurstExpl( pOwner, this, CVec3(target, z), CVec3(vOwnerCenter,fOwnerZ), nShellType, true, 0, true ),
				GetCommonGunNumber() 
				)
				);

			break;
		case IGunsFactory::VIS_CML_BALLIST_GUN:
			{
				CPtr<CCumulativeExpl> pExpl = new CCumulativeExpl( pOwner, this, CVec3( target, z ), CVec3( vOwnerCenter, fOwnerZ ), nShellType );
				theShellsStore.AddShell( new CVisShell( pExpl, CreateTraj( pExpl->GetExplCoordinates() ), GetCommonGunNumber(), GetPlatform() ) );
			}

			break;
		case IGunsFactory::VIS_BURST_BALLIST_GUN:
			{
				CPtr<CBurstExpl> pExpl = new CBurstExpl( pOwner, this, CVec3( target, z ), CVec3( vOwnerCenter, fOwnerZ ), nShellType, true, 0, true );
				theShellsStore.AddShell( new CVisShell( pExpl, CreateTraj( pExpl->GetExplCoordinates() ), GetCommonGunNumber(), GetPlatform() ) );
			}

			break;
		case IGunsFactory::TORPEDO_GUN:
			{
				CVec2 vTorpedoSource = pOwner->GetCenterPlain();
				CVec2 vOffset = GetVectorByDirection( pOwner->GetDirection() ) * pOwner->GetAABBHalfSize().y;
				vTorpedoSource += vOffset;
				CAIUnit *pTorpedo = theUnitCreation.CreateTorpedo( pOwner, pWeapon, vTorpedoSource, target );
				if ( pTorpedo ) 
					pTorpedo->FreezeByState( false );

			}
			break;
		case IGunsFactory::ROCKET_GUN:
			{
				CPtr<CBurstExpl> pExpl = new CBurstExpl( pOwner, this, CVec3( target, z ), CVec3( vOwnerCenter, fOwnerZ ), nShellType, true, 0, true );
				theShellsStore.AddShell( new CVisShell( pExpl, CreateTraj( pExpl->GetExplCoordinates() ), GetCommonGunNumber(), GetPlatform() ) );
			}
			break;
		case IGunsFactory::PLANE_GUN:
			{
				NI_ASSERT( dynamic_cast_ptr<CAviation*>(pOwner)!=0, StrFmt("unit \"%s\" weapon \"%s\": only planes can shoot with bombs", NDb::GetResName(pOwner->GetStats()), NDb::GetResName(pWeapon)) );
				CAviation *pAvia = checked_cast_ptr<CAviation*>(pOwner);
				CVec3 vSpeed3 ( pAvia->GetSpeedB2() );
				const CVec3 vOwnerCenter3D( pAvia->GetPosB2() );

				CVec3 vTrajFinish ( vOwnerCenter, 0.0f );
				float fTimeToFly = 0;
				float fDispRadius = 0;
				float fAcceleration = 0;
				CVec2 vRandAcc = VNULL2;


				for ( int i = 0; i < 3; ++ i )
				{
					fTimeToFly = CBombBallisticTraj::GetTimeOfFly( vOwnerCenter3D.z - vTrajFinish.z, vSpeed3.z );
					fDispRadius = GetDispersion() * ( pOwner->GetZ() - vTrajFinish.z ) / GetFireRangeMax();
					fAcceleration = fDispRadius * 2 / sqr( fTimeToFly );
					RandQuadrInCircle( fAcceleration, &vRandAcc );
					vTrajFinish = CBombBallisticTraj::CalcTrajectoryFinish( vOwnerCenter3D, vSpeed3, vRandAcc, fTimeToFly );
				}

				CPtr<IBallisticTraj> pTraj = new CBombBallisticTraj( vOwnerCenter3D, vSpeed3, curTime + fTimeToFly, vRandAcc );

				CPtr<CBurstExpl> pExpl = new CBurstExpl( pOwner, this, vTrajFinish, vOwnerCenter3D, nShellType, false, 2, bShowBombEffect );
				CPtr<CVisShell> pShell = new CVisShell( pExpl, pTraj, GetCommonGunNumber(), GetPlatform() );
				theShellsStore.AddShell( pShell	);
			}

			break;
		}
	}				// end for( multiple shots )

	if ( z > GetHeights()->GetVisZ( target.x, target.y ) )
		pOwner->Fired( - ( GetAIMap()->GetSizeX() + GetAIMap()->GetSizeY() ), pCommonGunInfo->nGun );	
	//CRAP?: if shot above ground, give negative radius to indicate that it is an AA gun
	else
		pOwner->Fired( pWeapon->fRevealRadius, pCommonGunInfo->nGun );

	InitRandoms();
}

WORD CBasicGun::GetTrajectoryZAngle( const CVec3 &vToAim ) const
{
	if ( eType == IGunsFactory::VIS_CML_BALLIST_GUN || eType == IGunsFactory::VIS_BURST_BALLIST_GUN )
		return CBallisticTraj::GetTrajectoryZAngle( vToAim, pOwner->GetStatsModifier()->weaponShellSpeed.Get( pWeapon->shells[nShellType].fSpeed ), pWeapon->shells[nShellType].etrajectory, GetVerTurnConstraint(), GetFireRange(vToAim.z) );
	else
		return 16384 * 3;
}

void CBasicGun::SetRejectReason( const EUnitAckType &eReason ) 
{ 
	eRejectReason = eReason;
}

const int CBasicGun::GetPiercing() const
{
	return pOwner->GetStatsModifier()->weaponPiercing.Get( pWeapon->shells[nShellType].nPiercing );
}

const int CBasicGun::GetPiercingRandom() const
{
	return pOwner->GetStatsModifier()->weaponPiercing.Get( pWeapon->shells[nShellType].nPiercingRandom );
}

const int CBasicGun::GetMaxPossiblePiercing() const
{
	return pOwner->GetStatsModifier()->weaponPiercing.Get( pWeapon->shells[nShellType].GetMaxPossiblePiercing() );
}

const int CBasicGun::GetMinPossiblePiercing() const
{
	return pOwner->GetStatsModifier()->weaponPiercing.Get( pWeapon->shells[nShellType].GetMinPossiblePiercing() );
}

const int CBasicGun::GetRandomPiercing() const
{
	return pOwner->GetStatsModifier()->weaponPiercing.Get( pWeapon->shells[nShellType].GetRandomPiercing() );
}

const float CBasicGun::GetDamage() const
{
	return pOwner->GetStatsModifier()->weaponDamage.Get( pWeapon->shells[nShellType].fDamagePower );
}

const float CBasicGun::GetDamageRandom() const
{
	return pOwner->GetStatsModifier()->weaponDamage.Get( pWeapon->shells[nShellType].nDamageRandom );
}

const float CBasicGun::GetRandomDamage() const
{
	return pOwner->GetStatsModifier()->weaponDamage.Get( GetShell().GetRandomDamage() );
}

bool CBasicGun::IsBallisticTrajectory() const
{
	const NDb::SWeaponRPGStats::SShell::ETrajectoryType eTraj = GetShell().etrajectory;
	return eTraj == NDb::SWeaponRPGStats::SShell::TRAJECTORY_HOWITZER || 
				 eTraj == NDb::SWeaponRPGStats::SShell::TRAJECTORY_ROCKET || 
				 eTraj == NDb::SWeaponRPGStats::SShell::TRAJECTORY_AA_ROCKET || 
				 eTraj == NDb::SWeaponRPGStats::SShell::TRAJECTORY_TORPEDO || // a cheat to allow over-horizon torpedo launch
				 eTraj == NDb::SWeaponRPGStats::SShell::TRAJECTORY_CANNON ||
				 eTraj == NDb::SWeaponRPGStats::SShell::TRAJECTORY_FLAME_THROWER;
}

//*******************************************************************
//*													 CTurretGun															*
//*******************************************************************

CTurretGun::CTurretGun( CAIUnit *pOwner, const BYTE nShellType, SCommonGunInfo *pCommonGunInfo, const IGunsFactory::EGunTypes eType, const int nTurret )
: CBasicGun( pOwner, nShellType, pCommonGunInfo, eType ), bCircularAttack( false ), bTurnByBestWay( false ),
	wBestWayDir( 0 )
{ 
	pTurret = pOwner->GetTurret( nTurret );
}

void CTurretGun::TraceAim( CAIUnit *pUnit )
{
	GetTurret()->TraceAim( pUnit, this );

	for ( CParallelGuns::iterator iter = parallelGuns.begin(); iter != parallelGuns.end(); ++iter )
		(*iter)->TraceAim( pUnit );
}

void CTurretGun::StopTracing()
{
	GetTurret()->StopTracing();
/*
	for ( CParallelGuns::iterator iter = parallelGuns.begin(); iter != parallelGuns.end(); ++iter )
		(*iter)->StopTracing();
*/
}

bool CTurretGun::CanShootByHeight( CAIUnit *pTarget ) const
{
	if ( GetTurret()->DoesRotateVert() && GetShell().etrajectory != NDb::SWeaponRPGStats::SShell::TRAJECTORY_ROCKET )
	{
		const float fTargetZ = pTarget->GetZ();
		const float fOwnerZ = pOwner->GetZ();
		if ( fTargetZ > fOwnerZ && GetZAngle( pTarget->GetCenterPlain() - pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform ), fTargetZ - fOwnerZ ) > GetTurret()->GetVerTurnConstraint() )
			return false;
	}

	return CBasicGun::CanShootByHeight( pTarget );
}	

WORD CTurretGun::CalcVerticalAngle( const class CVec3 &pt ) const
{
	const WORD wZDesiredAngle = GetZAngle( pt ) + GetTrajectoryZAngle( pt );

	const WORD wConstraint = GetVerTurnConstraint() + 65535/4 * 3;
	return Min( wConstraint, wZDesiredAngle );
}

bool CTurretGun::TurnByVer( const CVec2 &vEnemyCenter, const float zDiff )
{
	WORD wZAngle = CalcVerticalAngle( CVec3( vEnemyCenter-pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform ), zDiff ) );
	CTurret *pTurret = GetTurret();

	bool bTurned = false;
	if ( pTurret->DoesRotateVert() )
	{
		if ( pTurret->IsVerFinished() && wZAngle != pTurret->GetVerCurAngle() || 
			!pTurret->IsVerFinished() && wZAngle != pTurret->GetVerFinalAngle() )
		{
			pTurret->TurnVer( wZAngle );
			bTurned = ( pTurret->GetVerEndTime() <= curTime + SConsts::AI_SEGMENT_DURATION );
		}
		else
			bTurned = ( pTurret->GetVerCurAngle() == wZAngle );

		if ( bTurned )
			pTurret->StopVerTurning();
	}
	else
		bTurned = true;

	return bTurned;
}

bool CTurretGun::TurnArtilleryToEnemy( const CVec2 &vEnemyCenter )
{
	const WORD wToEnemy = GetDirectionByVector( vEnemyCenter - pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform ) ) - GetGun().wDirection;
	WORD wDesirableAngle = WORD( wToEnemy - pOwner->GetFrontDirection() );
	CTurret *pTurret = GetTurret();

	bool bTurned = false;
	const WORD wHorConstraint = GetHorTurnConstraint();
	// желаемый угол вне contraints на поворот
	if ( wHorConstraint == 0 && wDesirableAngle != 0 ||
			 wDesirableAngle > wHorConstraint && wDesirableAngle < 65535 - wHorConstraint )
	{
		if ( DirsDifference( wDesirableAngle, wHorConstraint ) < DirsDifference( wDesirableAngle, -wHorConstraint ) )
			wDesirableAngle = wHorConstraint;
		else
			wDesirableAngle = -wHorConstraint;
	}

	if ( pTurret->IsHorFinished() && wDesirableAngle != pTurret->GetHorCurAngle() ||
 		  !pTurret->IsHorFinished() && wDesirableAngle != pTurret->GetHorFinalAngle() )
	{
		pTurret->TurnHor( wDesirableAngle );
		bTurned = ( pTurret->GetHorEndTime() <= curTime + SConsts::AI_SEGMENT_DURATION / 2 );
	}
	else
		bTurned = ( pTurret->GetHorCurAngle() == wDesirableAngle );

	if ( bTurned )
		pTurret->StopHorTurning();

	return bTurned;
}

bool CTurretGun::TurnByBestWay( const WORD wDirToEnemy )
{
	bTurnByBestWay = true;	
	
	const WORD wFrontDir = pOwner->GetFrontDirection();
	const WORD wBaseTurn = DirsDifference( wFrontDir, wDirToEnemy );
	const WORD wTurretCurAngle = GetTurret()->GetHorCurAngle() + GetGun().wDirection;
	const WORD wTurretGlobalAngle = wFrontDir + wTurretCurAngle;

	const float fBaseSpeed = pOwner->GetTurnSpeed();
	const float fTurretSpeed = GetTurret()->GetHorRotateSpeed();

	WORD wFinalTurretDir;
	if ( IsInTheMinAngle( wTurretGlobalAngle, wFrontDir, wDirToEnemy ) )
	{
		const WORD wCommonTurn = DirsDifference( wTurretGlobalAngle, wDirToEnemy );
		const WORD wResultBaseTurn = fBaseSpeed * wCommonTurn / ( fBaseSpeed + fTurretSpeed );
		const WORD wResultGunTurn = fTurretSpeed * wCommonTurn / ( fBaseSpeed + fTurretSpeed );

		if ( WORD(wDirToEnemy - wTurretGlobalAngle) == wCommonTurn )
		{
			wFinalTurretDir = wTurretCurAngle + wResultGunTurn - GetGun().wDirection;

			if ( DirsDifference( wFinalTurretDir, 0 ) > GetHorTurnConstraint() )
			{
				wBestWayDir = wDirToEnemy - GetHorTurnConstraint() - GetGun().wDirection;
				wFinalTurretDir = GetHorTurnConstraint();
			}
			else
				wBestWayDir = wFrontDir + wResultBaseTurn;
		}
		else
		{
			wFinalTurretDir = wTurretCurAngle - wResultGunTurn - GetGun().wDirection;

			if ( DirsDifference( wFinalTurretDir, 0 ) > GetHorTurnConstraint() )
			{
				wBestWayDir = wDirToEnemy + GetHorTurnConstraint() - GetGun().wDirection;
				wFinalTurretDir = -GetHorTurnConstraint();
			}
			else
				wBestWayDir = wFrontDir - wResultBaseTurn;
		}
	}
	else
	{
		const float fBaseTurnTime = (float)wBaseTurn / fBaseSpeed;
		const float fTurretTurnTime = (float)DirsDifference( wTurretCurAngle, 0 ) / fTurretSpeed;
		const float fTogetherTime = Max( fBaseTurnTime, fTurretTurnTime );

		const WORD wTurretTurn = DirsDifference( wTurretGlobalAngle, wDirToEnemy );
		const float fTurretOnlyTime = (float)wTurretTurn / fTurretSpeed;

		// поворачиваем вместе
		if ( fTogetherTime <= fTurretOnlyTime && DirsDifference( 0, -GetGun().wDirection ) <= GetHorTurnConstraint() )
		{
			wBestWayDir = wDirToEnemy;
			wFinalTurretDir = -GetGun().wDirection;
		}
		// только пушку
		else if ( DirsDifference( wDirToEnemy - wFrontDir - GetGun().wDirection, 0 ) <= GetHorTurnConstraint() )
		{
			wBestWayDir = wFrontDir;
			wFinalTurretDir = wDirToEnemy - wFrontDir - GetGun().wDirection;
		}
		else if ( DirsDifference( GetTurret()->GetHorCurAngle(), GetHorTurnConstraint() ) < DirsDifference( GetTurret()->GetHorCurAngle(), -GetHorTurnConstraint() ) )
		{
			wBestWayDir = wDirToEnemy - GetHorTurnConstraint() - GetGun().wDirection;
			wFinalTurretDir = GetHorTurnConstraint();
		}
		else
		{
			wBestWayDir = wDirToEnemy + GetHorTurnConstraint() - GetGun().wDirection;
			wFinalTurretDir = -GetHorTurnConstraint();
		}
	}

	const WORD wTurretTurnWOBase = wDirToEnemy - wFrontDir - GetGun().wDirection;
	if ( DirsDifference( wFrontDir, wBestWayDir ) <= SConsts::MIN_ROTATE_ANGLE && DirsDifference( wTurretTurnWOBase, 0 ) <= GetHorTurnConstraint() )
	{
		wBestWayDir = wFrontDir;
		wFinalTurretDir = wTurretTurnWOBase;
	}

	if ( /* !pOwner->CanTurnToFrontDir( wBestWayDir ) || */
			( pOwner->GetBehaviourMoving() != SBehaviour::EMRoaming && pOwner->GetBehaviourMoving() != SBehaviour::EMHoldPos ) ||
		( pOwner->CanShootInMovement() /*&& pOwner->IsMoving()*/ ) )
	{
		wBestWayDir = wFrontDir;
		wFinalTurretDir = wTurretTurnWOBase;
	}

	// wDeltaAngle didn't work. It is a fix
	bool bRet = GetTurret()->TurnHor( wFinalTurretDir );
	bool bRet2 = true;
	if ( wBestWayDir != wFrontDir )
		bRet2 = pOwner->TurnToDirection( wBestWayDir, false, true );
	return bRet && bRet2;
}

bool CTurretGun::TurnGunToEnemy( const CVec2 &vEnemyCenter, const float zDiff )
{
	// пушка
	if ( pOwner->NeedDeinstall() || pOwner->IsLocked( this ) || !pOwner->CanRotate() || pOwner->CanShootInMovement() )
	{
		StopTracing();
		
		const bool bHor = TurnArtilleryToEnemy( vEnemyCenter );
		const bool bVer = TurnByVer( vEnemyCenter, zDiff );
		return bHor && bVer;
	}
	else if ( lastEnemyPos != vEnemyCenter || !bTurnByBestWay || lastCheckTurnTime + 3000 < curTime )
	{
		StopTracing();
		
		lastCheckTurnTime = curTime;
		lastEnemyPos = vEnemyCenter;

		const bool bHor = TurnByBestWay( GetDirectionByVector( vEnemyCenter - pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform ) ) );
		const bool bVer = TurnByVer( vEnemyCenter, zDiff );
		return bHor && bVer;
	}
	else
		return ( pOwner->IsMoving() ? true : pOwner->TurnToDirection( wBestWayDir, false, true ) ) && 
			GetTurret()->IsFinished();
}

bool CTurretGun::IsGoodAngle( const CVec2 &point, const WORD addAngle, const float z, const BYTE cDeltaAngle ) const
{
	const WORD wDesirableAngle = WORD( GetDirectionByVector( point - pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform ) ) - GetGun().wDirection - pOwner->GetFrontDirection() );
	const WORD wVerAngle = CalcVerticalAngle( CVec3( point-pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform ), z-pOwner->GetZ() ) );

	CTurret *pTurret = GetTurret();

	// "10 +" is a bug fix - tiger (wDeltaAngle == 0) cannot shoot by line. because of inaccuracy of CalcVerticalAngle
	return 
		DirsDifference( wDesirableAngle, pTurret->GetHorCurAngle() ) <= 
				( (int)pWeapon->wDeltaAngle + (int)addAngle ) * cDeltaAngle + SConsts::AI_SEGMENT_DURATION * pTurret->GetHorRotateSpeed()
		&&
		DirsDifference( wVerAngle, pTurret->GetVerCurAngle() ) <= 
				( 10 + pWeapon->wDeltaAngle )* cDeltaAngle + SConsts::AI_SEGMENT_DURATION * pTurret->GetVerRotateSpeed();
}

void CTurretGun::Rest()
{
	if ( !bAngleLocked && lastCheck != 0 &&
			 curTime - lastCheck >= SConsts::TIME_TO_RETURN_GUN && curTime - lastCheck >= GetRelaxTime( false ) )
	{
		if ( !GetTurret()->IsLocked( this ) )
			GetTurret()->SetCanReturn();
		lastCheck = 0;
	}
}

const NTimer::STime CTurretGun::GetTimeToShoot( const CVec3 &vPoint ) const 
{ 
	const NTimer::STime nAimingTime = CBasicGun::GetAimTime( false );
	const SWeaponRPGStats::SShell &shell = GetShell();

	const CVec3 vUnitCenter( pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform ), pOwner->GetZ() );
	const float xDiff = vPoint.x - vUnitCenter.x;
	const float yDiff = vPoint.y - vUnitCenter.y;

	// time is rounded to segment duration
	return (nAimingTime +
					GetActionPoint() + 
	  			fabs( fabs(xDiff, yDiff), vPoint.z - vUnitCenter.z ) / pOwner->GetStatsModifier()->weaponShellSpeed.Get( shell.fSpeed )  ) / SConsts::AI_SEGMENT_DURATION * SConsts::AI_SEGMENT_DURATION;
}

const NTimer::STime CTurretGun::GetTimeToShootToPoint( const CVec3 &vPoint ) const
{
	const float fVertRotSpeed = pTurret->GetVerRotateSpeed();
	const float fHorRotSpeed = pTurret->GetHorRotateSpeed();

	const WORD wCurVerAngle = pTurret->GetVerCurAngle();
	const WORD wCurHorAngle = pTurret->GetHorCurAngle() + pOwner->GetFrontDirection();

	const CVec3 vUnitCenter( pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform ), pOwner->GetZ() );

	const float xDiff = vPoint.x - vUnitCenter.x;
	const float yDiff = vPoint.y - vUnitCenter.y;
	const float zDiff = vPoint.z - vUnitCenter.z;

	const NTimer::STime timeHorTurn = 
		1.0f * ( DirsDifference( wCurHorAngle, GetDirectionByVector( xDiff,yDiff ) )) / fHorRotSpeed;
	const NTimer::STime timeVerTurn = 
		1.0f * ( DirsDifference( GetDirectionByVector( fabs(xDiff,yDiff), zDiff ), wCurVerAngle ) ) / fVertRotSpeed;
	const NTimer::STime timeToTurn = Max( timeVerTurn, timeHorTurn );

	const float fTime = 
					timeToTurn + GetTimeToShoot( vPoint );

	// time is rounded to segment duration
	return Max( fTime, GetRelaxTime( false ) + CBasicGun::GetAimTime( false ) ) / SConsts::AI_SEGMENT_DURATION * SConsts::AI_SEGMENT_DURATION;
}

bool CTurretGun::AnalyzeTurning()
{
	if ( !CanShootToTargetWOMove() )
	{
		if ( !GetTurret()->IsLocked( this ) )
			GetTurret()->StopTurning();

		StopFire();
	}
	else
	{
		if ( CanShootWOGunTurn( bAim ? 0 : 1, z ) )
		{
			return true;
		}
		else if ( GetTurret()->IsLocked( this ) )
			StopFire();
		else
			Turning();
	}

	return false;
}

void CTurretGun::StopFire()
{
	pOwner->Unlock( this );
	GetTurret()->Unlock( this );
	ToRestState();
}

const WORD CTurretGun::GetGlobalDir() const
{
	return pOwner->GetDirection() + GetTurret()->GetHorCurAngle() + GetGun().wDirection;
}

void CTurretGun::TurnToRelativeDir( const WORD wAngle )
{
	GetTurret()->TurnHor( wAngle );
}

const float CTurretGun::GetRotateSpeed() const
{
	return GetTurret()->GetHorRotateSpeed();
}

WORD CTurretGun::GetHorTurnConstraint() const
{
	if ( !bCircularAttack )
		return GetTurret()->GetHorTurnConstraint();
	else
		return 32768;
}

WORD CTurretGun::GetVerTurnConstraint() const
{
	return GetTurret()->GetVerTurnConstraint();
}

void CTurretGun::SetCircularAttack( const bool bCanAttack )
{
	bCircularAttack = bCanAttack;	
}

void CTurretGun::StartPointBurst( const CVec3 &target, bool bReAim )
{
	bTurnByBestWay = false;
	CBasicGun::StartPointBurst( target, bReAim );
}

void CTurretGun::StartPointBurst( const CVec2 &target, bool bReAim )
{
	bTurnByBestWay = false;
	CBasicGun::StartPointBurst( target, bReAim );
}

void CTurretGun::StartEnemyBurst( class CAIUnit *pEnemy, bool bReAim )
{
	bTurnByBestWay = false;
	CBasicGun::StartEnemyBurst( pEnemy, bReAim );
}

//*******************************************************************
//*													CBaseGun																*
//*******************************************************************

bool CBaseGun::TurnGunToEnemy( const CVec2 &vEnemyCenter, const float zDiff )
{
	if ( pOwner->IsMoving() )
		return false;

	return pOwner->TurnToDirection( GetDirectionByVector( vEnemyCenter - pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform ) ) - GetGun().wDirection, false, true );
}

bool CBaseGun::IsGoodAngle( const CVec2 &point, const WORD addAngle, const float z, const BYTE cDeltaAngle  ) const
{
	const SUnitBaseRPGStats *pStats = pOwner->GetStats();
	
	if ( pStats->IsInfantry() && !AnalyzeLimitedAngle( pOwner, point ) )
		return false;

	const CVec2 vOwnerCenter( pOwner->GetGunCenter( pCommonGunInfo->nGun, pCommonGunInfo->nPlatform ) );
	const int wWeaponDeltaAngle = (int)pWeapon->wDeltaAngle;

	if ( pStats->IsAviation() )
	{
		const CAviation * pPlane = checked_cast_ptr<const CAviation*>( pOwner );
		// check vertical angle
		const WORD wDesiredVAngle = GetDirectionByVector( fabs(point - vOwnerCenter), z - pOwner->GetZ() );
		CVec3 vSpeed;
		pPlane->GetSpeed3( &vSpeed );
		const WORD wCurrentVAngle = GetDirectionByVector ( fabs( vSpeed.x, vSpeed.y ), vSpeed.z );
		if ( DirsDifference( wCurrentVAngle, wDesiredVAngle ) > 2 * ( wWeaponDeltaAngle + (int)addAngle ) * cDeltaAngle )
			return false;

		const WORD wDesirableAngle = GetDirectionByVector( point - vOwnerCenter ) - GetGun().wDirection;
		return DirsDifference( wDesirableAngle, GetDirectionByVector( vSpeed.x, vSpeed.y ) ) <= 
			( wWeaponDeltaAngle + (int)addAngle ) * cDeltaAngle;
	}

	const WORD wDesirableAngle = GetDirectionByVector( point - vOwnerCenter ) - GetGun().wDirection;
	return DirsDifference( wDesirableAngle, pOwner->GetFrontDirection() ) <= 
				( wWeaponDeltaAngle + (int)addAngle ) * cDeltaAngle;
}

bool CBaseGun::AnalyzeTurning()
{
	if ( !CanShootToTargetWOMove() )
		StopFire();

	if ( CanShootWOGunTurn( bAim ? 0 : 1, z ) )
		return true;
	else if ( pOwner->IsLocked( this ) || pOwner->GetStats()->IsAviation() || !pOwner->CanRotate() )
		StopFire();
	else
		Turning();

	return false;
}

void CBaseGun::StopFire()
{
	pOwner->Unlock( this );
	ToRestState();
}

const WORD CBaseGun::GetGlobalDir() const
{
	return pOwner->GetDirection() + GetGun().wDirection;
}

const float CBaseGun::GetRotateSpeed() const
{
	return pOwner->GetTurnSpeed();
}

const float GetFireRangeMax( const SWeaponRPGStats *pStats, CAIUnit *pOwner )
{
	return pStats->fRangeMax;
}


