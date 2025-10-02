#include "stdafx.h"

#include "ShootEstimatorInternal.h"
#include "Turret.h"
#include "Diplomacy.h"
#include "Units.h"
#include "ScanLimiter.h"
#include "Soldier.h"
#include "Building.h"
#include "Guns.h"
#include "Artillery.h"
#include "General.h"

#include "UnitCreation.h"
extern CUnitCreation theUnitCreation;
// for profiling
#include "TimeCounter.h"
REGISTER_SAVELOAD_CLASS( 0x1108D4DF, CTankShootEstimator );
REGISTER_SAVELOAD_CLASS( 0x1108D4E0, CSoldierShootEstimator );
REGISTER_SAVELOAD_CLASS( 0x1108D4E1, CPlaneDeffensiveFireShootEstimator );
REGISTER_SAVELOAD_CLASS( 0x1108D4E2, CPlaneShturmovikShootEstimator );
REGISTER_SAVELOAD_CLASS( 0x11144380, CShootEstimatorSupportAAGun );

extern CDiplomacy theDipl;
extern CUnits units;
extern CScanLimiter theScanLimiter;

extern NAI::CTimeCounter timeCounter;
extern CSupremeBeing theSupremeBeing;

//*******************************************************************
//*											 CTankShootEstimator												*
//*******************************************************************

//BASIC_REGISTER_CLASS( CTankShootEstimator );
BASIC_REGISTER_CLASS( IShootEstimator );
//BASIC_REGISTER_CLASS( CPlaneDeffensiveFireShootEstimator );
//BASIC_REGISTER_CLASS( CPlaneShturmovikShootEstimator );

const float F( const float fHPPercent, const float fT0, const float fT1, const float fT2, const float fPrice )
{
	static const float F_LIMIT_TIME = 1000.0f;	

	static const float fAlphaAttack1 = 1.0f;
	static const float fAlphaAttack2 = 0.3f;
	static const float fAlphaGo = 0.005f;
	static const float fAlphaKill= 1.0f;
	static const float fAlphaPrice = 1.0f;

	return
		( 0.8f + fHPPercent * 0.2f ) * fAlphaAttack1 * Min( 0.0f, fT2 - F_LIMIT_TIME ) -
		( 0.8f + fHPPercent * 0.2f ) * fAlphaAttack2 * Max( 0.0f, fT2 - F_LIMIT_TIME ) -
		fAlphaGo * fT0 - 
		fAlphaKill * fT1 +
		fPrice * fAlphaPrice;
}

CTankShootEstimator::CTankShootEstimator( CAIUnit *_pOwner ) 
: pOwner( _pOwner ), fBestRating( -1.0f ), bDamageToCurTargetUpdated( false ), nBestGun( -1 )
{
	dwForbidden = 0;
	dwDefaultForbidden = 0;
	for ( int i = 1; i < pOwner->GetNGuns(); ++i )
	{
		if ( pOwner->GetGun( i )->GetGun().nPriority == 0 )
		{
			int j = 0;
			while ( j < i && 
							( pOwner->GetGun( j )->GetGun().nPriority != 0 || 
							  pOwner->GetGun( j )->GetCommonGunNumber() == pOwner->GetGun( i )->GetCommonGunNumber() ) )
				++j;

			if ( j < i )
				dwDefaultForbidden |= ( 1UL << i );
		}
		if ( _pOwner->GetStats()->etype == RPG_TYPE_ART_AAGUN && pOwner->GetGun( i )->GetShellType() == 1 )
			dwDefaultForbidden |= ( 1UL << i );
	}
	
	pMosinStats = theUnitCreation.GetMosinStats();
}

const float FindTimeToTurnToPoint( const CVec2 &vPoint, class CCommonUnit *pOwner, CBasicGun *pGun )
{
	const WORD wUnitDir = pOwner->GetDirection();		
	const WORD wDirToEnemy = GetDirectionByVector( vPoint - pOwner->GetCenterPlain() );
	const bool bMovingOwner = pOwner->CanRotate() && !pOwner->NeedDeinstall();
	const WORD wDeltaAngle = pGun->GetWeapon()->wDeltaAngle;
	CTurret *pTurret = pGun->GetTurret();

	if ( pTurret != 0 )
	{
		int nTimeVerticalAim = 0;
		CTurret * pTurret = pGun->GetTurret();
		if ( pGun->IsBallisticTrajectory() )
			nTimeVerticalAim = DirsDifference( pGun->GetVerTurnConstraint(), pTurret->GetVerCurAngle() ) / pTurret->GetVerRotationSpeed();

		const WORD wGunDir = pTurret->GetHorCurAngle() + pGun->GetGun().wDirection + wUnitDir;
		const WORD wTurnAngle = DirsDifference( wGunDir, wDirToEnemy );

		if ( wTurnAngle < wDeltaAngle )
			return nTimeVerticalAim;
		else
		{
			float fTurnSpeed = pTurret->GetHorRotationSpeed();
			if ( bMovingOwner )
				fTurnSpeed = Min( fTurnSpeed, pOwner->GetTurnSpeed() );

			return nTimeVerticalAim + float( wTurnAngle ) / fTurnSpeed;
		}
	}
	else
	{
		if ( !bMovingOwner )
			return 0.0f;
		else
		{
			const WORD wGunDir = pGun->GetGun().wDirection + wUnitDir;
			const WORD wTurnAngle = DirsDifference( wGunDir, wDirToEnemy );
			if ( wTurnAngle < wDeltaAngle )
				return 0.0f;
			else
				return float( wTurnAngle ) / pOwner->GetTurnSpeed();
		}
	}
}


void CTankShootEstimator::Reset( CAIUnit *pCurEnemy, const bool bDamageUpdated, const DWORD _dwForbidden )
{
	pCurTarget = pCurEnemy;
	bDamageToCurTargetUpdated = bDamageUpdated;
	pBestUnit = 0;
	pBestGun = 0;
	nBestGun = 0;
	dwForbidden = _dwForbidden;

	if ( IsValidObj( pCurTarget ) )
		AddUnit( pCurTarget );
}

const float CTankShootEstimator::GetRating( const SUnitBaseRPGStats *pStats, const CVec2 &vCenter, CBasicGun *pGun ) const
{
	float fTimeToGo = 0;

	if ( pOwner->CanMove() && !pOwner->NeedDeinstall() )
		fTimeToGo = fabs( pOwner->GetCenterPlain() - vCenter ) / pOwner->GetStats()->fSpeed;
	fTimeToGo += FindTimeToTurnToPoint( vCenter, pOwner, pGun ) + 
		(!pGun->CanShootToPointWOMove( vCenter, 0 ) ? fTimeToGo * 100.0f : 0.0f);// to diminish targeting impossible to shoot enemies

	const float fEnemyKillUsTime = 0.0f;

	const float fKillEnemySpeed = pOwner->GetKillSpeed( pStats, vCenter, pGun );
	const float fKillEnemyTime = pStats->fMaxHP / fKillEnemySpeed;
	const float fEnemyHPPercent = 1.0f;

	return
		F( fEnemyHPPercent, fTimeToGo, fEnemyKillUsTime, fKillEnemyTime, pStats->fPrice );
}

const float CTankShootEstimator::GetRating( CAIUnit *pEnemy, CBasicGun *pGun ) const
{
	float fTimeToGo = 0;

	if ( pOwner->CanMove() && !pOwner->NeedDeinstall() )
		fTimeToGo = fabs( pOwner->GetCenter() - pEnemy->GetCenter() ) / pOwner->GetStats()->fSpeed;
	//fTimeToGo += FindTimeToTurnToPoint( pEnemy->GetCenterPlain(), pOwner, pGun );
	fTimeToGo += FindTimeToTurnToPoint( pEnemy->GetCenterPlain(), pOwner, pGun ) + 
		(!pGun->CanShootToUnitWOMove( pEnemy ) ? fTimeToGo * 100.0f : 0.0f);// to diminish targeting impossible to shoot enemies


	const float fEnemyKillUsSpeed = pEnemy->GetKillSpeed( pOwner );
	float fEnemyKillUsTime;
	if ( fEnemyKillUsSpeed == 0.0f )
		fEnemyKillUsTime = 0.0f;
	else
		fEnemyKillUsTime = pOwner->GetStats()->fMaxHP / fEnemyKillUsSpeed;

	float fKillEnemySpeed;
	if ( !bDamageToCurTargetUpdated )
		fKillEnemySpeed = pEnemy->GetTakenDamagePower() + pOwner->GetKillSpeed( pEnemy, pGun );
	else
	{
		if ( pEnemy != pCurTarget )
			fKillEnemySpeed = pEnemy->GetTakenDamagePower() + pOwner->GetKillSpeed( pEnemy, pGun );
		else
			fKillEnemySpeed = pEnemy->GetTakenDamagePower();
	}

	const float fKillEnemyTime = pEnemy->GetStats()->fMaxHP / fKillEnemySpeed;
	const float fEnemyHPPercent = pEnemy->GetHitPoints() / pEnemy->GetStats()->fMaxHP;

	return
		F( fEnemyHPPercent, fTimeToGo, fEnemyKillUsTime, fKillEnemyTime, pEnemy->GetStats()->fPrice );
}

void CTankShootEstimator::ChooseGun( CBasicGun **pBestGun, int *nBestGun, CAIUnit *pEnemy )
{
	float fBestTime = 0;
	*pBestGun = 0;
	float fLargestArea2 = 0;

	CVec2 vDirToEnemy = pEnemy->GetCenterPlain() - pOwner->GetCenterPlain();
	const float fDist = fabs( vDirToEnemy );
	Normalize( &vDirToEnemy );
	const bool bOverHorizont = fDist > pOwner->GetSightRadius();
	const bool bMovingOwner = pOwner->CanRotate() && pOwner->CanMove() && !pOwner->NeedDeinstall();

	const bool bArtilleryMech = pOwner->GetFirstArtilleryGun() != 0;

	const SRect enemyRect = pEnemy->GetUnitRect();
	// площать combat rect
	const float fSEnemyRect = 
		2 * enemyRect.width * ( enemyRect.lengthAhead + enemyRect.lengthBack ) / sqr( pEnemy->GetRemissiveCoeff() );

	if ( pOwner->GetUnitAbilityDesc( NDb::ABILITY_COVER_FIRE ) == 0 || !pEnemy->IsInfantry() )
		dwForbidden |= dwDefaultForbidden;
	const int nGuns = pOwner->GetNGuns();
	const CVec2 vCenterPlain ( pOwner->GetCenterPlain() );

	for ( int i = 0; i < nGuns; ++i )
	{
		CBasicGun *pGun = pOwner->GetGun( i );
		const NDb::SWeaponRPGStats::SShell &shell = pGun->GetShell();

		if ( ( dwForbidden & ( 1UL << i ) ) == 0 && pGun->GetNAmmo() > 0 && 
			   shell.eDamageType == NDb::SWeaponRPGStats::SShell::DAMAGE_HEALTH )
		{
/*
			// либо за горизонт, либо миномёт, либо прямой наводкой
			if ( bOverHorizont ||
					 pOwner->GetStats()->etype == RPG_TYPE_ART_MORTAR ||
 					 !bOverHorizont && pGun->GetShell().etrajectory == TRAJECTORY_LINE )
*/
			{
				// можно выстрелить
				if ( /*!pGun->TooCloseToFire( pEnemy ) &&*/
						 (
							!bArtilleryMech && pGun->CanShootToUnit( pEnemy ) ||
							bArtilleryMech && ( !pGun->IsBallisticTrajectory() && pGun->CanShootToUnit( pEnemy ) && !bOverHorizont || pGun->CanShootToUnitWOMove( pEnemy ) ) 
						  )
						)
				{
					if ( bArtilleryMech && pGun->IsBallisticTrajectory() && bOverHorizont || 
						   pGun->GetShell().etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_TORPEDO )
					{
						*pBestGun = pGun;
						*nBestGun = i;

						return;
					}
					
					// нельзя стрелять по очагам сопротивления
					if ( theDipl.IsAIPlayer( pOwner->GetPlayer() ) && pGun->IsBallisticTrajectory() && 
							 theSupremeBeing.IsInResistanceCircle( pEnemy->GetCenterPlain(), pOwner->GetParty() ) )
						continue;
					
					// нельзя двигаться или нужно идти достаточно далеко, чтобы примерно дойти до точки, откуда можно стрелять
					const float fFireRange = pGun->GetFireRange( pEnemy->GetCenter().z );
					const float fDistToGo = Max( 0.0f, fDist - fFireRange );
					const CVec2 vPoint = vCenterPlain + vDirToEnemy * fDistToGo;

	
					if ( !bMovingOwner || fDistToGo == 0 || pOwner->CanGoToPoint( vPoint ) )
					{
						float fTime = 0;
						if ( bMovingOwner )
							fTime = fDistToGo / pOwner->GetStats()->fSpeed;

						fTime += FindTimeToTurnToPoint( pEnemy->GetCenterPlain(), pOwner, pGun );

						// макс. броня врага не пробивается оружием
						if ( pGun->GetMaxPossiblePiercing() < pEnemy->GetMaxArmor() )
							// поворот на 30 градусов ( 65536 / 360 * 30 )
							fTime += 5461.0f * pOwner->GetTurnSpeed();

						const float fDispRadius = GetDispByRadius( pGun, pOwner->GetCenterPlain(), pEnemy->GetCenterPlain() );
						float fR;
						// dispersion умножается на 0.56, т.к. у нас не равномерное распр. при попадании снаряда,
						// а что-то типа равномерного в квадрате. Число 0.56 взято из экспериментов
						if ( pEnemy->GetMaxArmor() == 0 )
						{
							// может быть area damage
							// если техника, то по малому радиусу взрыва
							if ( !pEnemy->GetStats()->IsInfantry() )
								fR = 0.56 * fDispRadius - shell.fArea;
							// если солдат и свободен, то по большому радиусу
							else if ( pEnemy->IsFree() )
								fR = 0.56 * fDispRadius - shell.fArea2;
							else
								// если солдат и не свободен, то по малому радиусу
								fR = 0.56 * fDispRadius - shell.fArea;
						}
						else
							// только точное попадание
							fR = 0.56 * fDispRadius;

						// вероятность попасть ( примерная... )
						float fProbToHit;
						if ( fR <= 0 )
							fProbToHit = 1;
						else
							fProbToHit = Min( 1.0f, fSEnemyRect / ( FP_PI * sqr( fR ) ) );

						// probability to pierce
						const float fPierceProb = 1.0f / Clamp( sqr( float( pGun->GetMaxPossiblePiercing() - pEnemy->GetMinArmor() ) / float( pGun->GetMaxPossiblePiercing() - pGun->GetMinPossiblePiercing() ) ), 0.01f, 1.0f );

						const CDBPtr<NDb::SWeaponRPGStats> pWeapon = pGun->GetWeapon();

						const bool bPreferThisGun = pEnemy->IsInfantry() &&
							( pWeapon->eWeaponType == NDb::SWeaponRPGStats::WEAPON_MACHINEGUN || pWeapon->eWeaponType == NDb::SWeaponRPGStats::WEAPON_SUBMACHINEGUN ) &&
							fDist < pWeapon->fRangeMax;
						// выстрелов, чтобы убить с вероятностью 80%		
						const float fShotsToKill = bPreferThisGun ? 0.0f : fPierceProb * 0.8 * pEnemy->GetHitPoints() / pGun->GetDamage();
						const float fProbShotsToKill = fShotsToKill / fProbToHit;
						// очередей

						const int nAmmoPerBurst = pGun->GetWeapon()->nAmmoPerBurst;
						const int nBursts = ceil( fProbShotsToKill / nAmmoPerBurst );
						
						fTime += pGun->GetAimTime( false ) + ( nBursts - 1 ) * pGun->GetRelaxTime( false ) + nBursts * ( ( nAmmoPerBurst - 1 ) * pGun->GetFireRate() );

						if ( *pBestGun == 0 || fTime < fBestTime )
						{
							*pBestGun = pGun;
							*nBestGun = i;
							fBestTime = fTime;
							fLargestArea2 = shell.fArea2;
						}
						else if ( pOwner->GetUnitAbilityDesc( NDb::ABILITY_COVER_FIRE ) != 0 && pEnemy->IsInfantry() )
						{
							// Workaround for Cover Fire ability
							// choose the gun with the largest Area2
							if ( shell.fArea2 > fLargestArea2 )
							{
								*pBestGun = pGun;
								*nBestGun = i;
								fBestTime = fTime;
								fLargestArea2 = shell.fArea2;
							}
						}
					}
				}
			}
		}
	}
}

void CTankShootEstimator::AddUnit( CAIUnit *pEnemy )
{
	theScanLimiter.TargetScanning( pOwner->GetStats()->etype );
	
	CBasicGun *pBestLocalGun = 0;
	int nBestLocalGun = 0;
	const CVec2 vEnemyCenter = pEnemy->GetCenterPlain();

	bool bChoose = true;
	int nSoldiersCache = -1;
	if ( pOwner->IsFreeEnemySearch() && pOwner->GetFirstArtilleryGun() != 0 )
	{
		const float fDistToEnemy2 = fabs2( pOwner->GetCenterPlain() - vEnemyCenter );
		// враг за горизонтом
		if ( fDistToEnemy2 >= sqr( pOwner->GetSightRadius() ) )
		{
			const float fDispersion = pOwner->GetFirstArtilleryGun()->GetDispersion();

			if ( pEnemy->GetStats()->IsInfantry() && 
					 ( nSoldiersCache = units.GetNSoldiers( vEnemyCenter, fDispersion * 1.2f, 1 - pOwner->GetParty() ) ) < 10 )
				bChoose = false;

			if ( !pEnemy->GetStats()->IsInfantry() &&
					 units.GetNUnits( vEnemyCenter, fDispersion, 1 - pOwner->GetParty() ) < 4 )
				bChoose = false;
		}
	}

	if ( bChoose )
		ChooseGun( &pBestLocalGun, &nBestLocalGun, pEnemy );

	if ( pBestLocalGun != 0 )
	{
		const float fSoldierRating = GetRating( pMosinStats, vEnemyCenter, pBestLocalGun );
		const int nSoldiers = nSoldiersCache == -1 ? units.GetNSoldiers( vEnemyCenter, pBestLocalGun->GetDispersion(), 1 - pOwner->GetParty() ) : nSoldiersCache;
		const float fRating = GetRating( pEnemy, pBestLocalGun ) + nSoldiers * fSoldierRating;

		if ( pBestUnit == 0 )
		{
			pBestUnit = pEnemy;
			pBestGun = pBestLocalGun;
			nBestGun = nBestLocalGun;
			fBestRating = fRating;
		}
		else
		{
			const float fNewEnemyKillUsTime = pEnemy->GetKillSpeed( pOwner );
			const float fEnemyKillUsTime = pBestUnit->GetKillSpeed( pOwner );

			if ( fEnemyKillUsTime == 0.0f && fNewEnemyKillUsTime != 0.0f ||
					 fRating > fBestRating && 
					 (	
							fEnemyKillUsTime != 0.0f && fNewEnemyKillUsTime != 0.0f ||
							fEnemyKillUsTime == 0.0f && fNewEnemyKillUsTime == 0.0f 
					 )
				 )
			{
				pBestUnit = pEnemy;
				pBestGun = pBestLocalGun;
				nBestGun = nBestLocalGun;
				fBestRating = fRating;
			}
		}
	}
}

CAIUnit* CTankShootEstimator::GetBestUnit() const
{
	return pBestUnit;
}

CBasicGun* CTankShootEstimator::GetBestGun() const
{
	return pBestGun;
}

const int CTankShootEstimator::GetNumberOfBestGun() const
{
	return nBestGun;
}

//*******************************************************************
//*											 CSoldierShootEstimator											*
//*******************************************************************

const int CSoldierShootEstimator::N_GOOD_NUMBER_ATTACKING_GRENADES = 6;

CSoldierShootEstimator::CSoldierShootEstimator( CAIUnit *_pOwner ) 
: pOwner( _pOwner ), bHasGrenades ( pOwner->GetNGuns() >= 2 ), 
	bUseGrenadeAutocast( false ), bUseGrenadeFixed( false ),
	fBestRating( -1.0f ), bDamageToCurTargetUpdated( false ), nBestGun( -1 ), bThrowGrenade( false ),
	dwForbidden( 0 )
{ 
	pMosinStats = theUnitCreation.GetMosinStats();
}

void CSoldierShootEstimator::Reset( CAIUnit *pCurEnemy, const bool bDamageUpdated, const DWORD _dwForbidden )
{
	pCurTarget = pCurEnemy;
	bDamageToCurTargetUpdated = bDamageUpdated;
	bThrowGrenade = false;
	pBestUnit = 0;
	pBestGun = 0;
	nBestGun = 0;

	dwForbidden = _dwForbidden;

	if ( IsValidObj( pCurTarget ) )
		AddUnit( pCurTarget );
}

const float CSoldierShootEstimator::GetRating( CAIUnit *pEnemy, CBasicGun *pGun ) const
{
	float fTimeToGo = fabs( pOwner->GetCenter() - pEnemy->GetCenter() ) / pOwner->GetStats()->fSpeed;

	const float fEnemyKillUsSpeed = pEnemy->GetKillSpeed( pOwner );
	float fEnemyKillUsTime;
	if ( fEnemyKillUsSpeed == 0.0f )
		fEnemyKillUsTime = 0.0f;
	else
		fEnemyKillUsTime = pOwner->GetStats()->fMaxHP / fEnemyKillUsSpeed;

	float fKillEnemySpeed;
	if ( !bDamageToCurTargetUpdated )
		fKillEnemySpeed = pEnemy->GetTakenDamagePower() + pOwner->GetKillSpeed( pEnemy, pGun );
	else
	{
		if ( pEnemy != pCurTarget )
			fKillEnemySpeed = pEnemy->GetTakenDamagePower() + pOwner->GetKillSpeed( pEnemy, pGun );
		else
			fKillEnemySpeed = pEnemy->GetTakenDamagePower();
	}

	const float fKillEnemyTime = pEnemy->GetStats()->fMaxHP / fKillEnemySpeed;
	const float fEnemyHPPercent = pEnemy->GetHitPoints() / pEnemy->GetStats()->fMaxHP;

	return
		F( fEnemyHPPercent, fTimeToGo, fEnemyKillUsTime, fKillEnemyTime, pEnemy->GetStats()->fPrice );
}

const float CSoldierShootEstimator::GetRating( const SUnitBaseRPGStats *pStats, const CVec2 &vCenter, CBasicGun *pGun ) const
{
	float fTimeToGo = 0;

	if ( pOwner->CanMove() && !pOwner->NeedDeinstall() )
		fTimeToGo = fabs( pOwner->GetCenterPlain() - vCenter ) / pOwner->GetStats()->fSpeed;
	fTimeToGo += FindTimeToTurnToPoint( vCenter, pOwner, pGun );

//	const float fEnemyKillUsSpeed = 0.0f;
	const float fEnemyKillUsTime = 0.0f;

	const float fKillEnemySpeed = pOwner->GetKillSpeed( pStats, vCenter, pGun );
	const float fKillEnemyTime = pStats->fMaxHP / fKillEnemySpeed;
	const float fEnemyHPPercent = 1.0f;

	return
		F( fEnemyHPPercent, fTimeToGo, fEnemyKillUsTime, fKillEnemyTime, pStats->fPrice );
}

void CSoldierShootEstimator::ChooseGun( CBasicGun **pBestGun, int *nBestGun, CAIUnit *pEnemy )
{
	float fBestTime = 0;
	*pBestGun = 0;

	const float fDist = fabs( pOwner->GetCenter() - pEnemy->GetCenter() );

	// не внутри объекта, есть только ружьё или стреляем по солдатам
	if ( pOwner->IsFree() && ( ( !bUseGrenadeAutocast && !bUseGrenadeFixed ) || ( ( dwForbidden & 1 ) == 0 && ( pOwner->GetNGuns() == 1 || pEnemy->GetStats()->IsInfantry() ) ) ) )
	{
		CBasicGun *pGun = pOwner->GetGun( 0 );
		if ( pGun->GetNAmmo() > 0 && 
			   pGun->GetShell().eDamageType == NDb::SWeaponRPGStats::SShell::DAMAGE_HEALTH && 
				 pGun->CanShootToUnit( pEnemy ) )
		{
			*pBestGun = pGun;
			*nBestGun = 0;
		}
	}
	else
	{
		//const SRect enemyRect = pEnemy->GetUnitRect();
		const CVec2 vEnemySize = pEnemy->GetStats()->vAABBHalfSize;
		// площать combat rect
		const float fSEnemyRect = 
			4.0f * vEnemySize.y * ( vEnemySize.x ) / sqr( pEnemy->GetRemissiveCoeff() );

		CVec2 vDirToEnemy = pEnemy->GetCenterPlain() - pOwner->GetCenterPlain();
		Normalize( &vDirToEnemy );
		

		const int nGuns = pOwner->GetNGuns();
		const CVec2 vCeneterPlain( pOwner->GetCenterPlain() );
		for ( int i = 0; i < nGuns; ++i )
		{
			CBasicGun *pGun = pOwner->GetGun( i );
			const bool bTooFar = 
				( i == 1 ) && sqr(fDist) > sqr( SConsts::MAX_DISTANCE_TO_THROW_GRENADE ) &&
				pEnemy->GetStats()->IsArtillery() &&
				checked_cast<CArtillery*>(pEnemy)->GetCrew() != 0;
			
			const NDb::SWeaponRPGStats::SShell &shell = pGun->GetShell();
			if ( !bTooFar && ( dwForbidden & ( 1 << i ) ) == 0 && 
				   shell.eDamageType == NDb::SWeaponRPGStats::SShell::DAMAGE_HEALTH &&
					 pGun->GetNAmmo() > 0 && pGun->CanShootToUnit( pEnemy ) )
			{
				// нельзя двигаться или можно идти достаточно далеко, чтобы примерно дойти до точки, откуда можно стрелять
				const float fDistToGo = Max( 0.0f, fDist - pGun->GetFireRange( pEnemy->GetCenter().z ) );
				const CVec2 vPoint = vCeneterPlain + vDirToEnemy * fDistToGo;

				if ( pOwner->CanGoToPoint( vPoint ) || fDistToGo == 0 )
				{
					float fTime = fDistToGo / pOwner->GetStats()->fSpeed;

					const float fDispRadius = GetDispByRadius( pGun, pOwner->GetCenterPlain(), pEnemy->GetCenterPlain() );
					float fR;
					// dispersion умножается на 0.56, т.к. у нас не равномерное распр. при попадании снаряда,
					// а что-то типа равномерного в квадрате. Число 0.56 взято из экспериментов
					if ( pEnemy->GetMaxArmor() == 0 )
					{
						// может быть area damage
						// если техника, то по малому радиусу взрыва
						if ( !pEnemy->GetStats()->IsInfantry() )
							fR = 0.56 * fDispRadius - shell.fArea;
						// если солдат и свободен, то по большому радиусу
						else if ( pEnemy->IsFree() )
							fR = 0.56 * fDispRadius - shell.fArea2;
						else
							// если солдат и не свободен, то по малому радиусу
							fR = 0.56 * fDispRadius - shell.fArea;
					}
					else
						// только точное попадание
						fR = 0.56 * fDispRadius;

					// вероятность попасть ( примерная... )
					float fProbToHit;
					if ( fR <= 0 )
						fProbToHit = 1;
					else
						fProbToHit = Min( 1.0f, fSEnemyRect / ( FP_PI * sqr( fR ) ) );

					// выстрелов, чтобы убить с вероятностью 80%		
					const float fShotsToKill = 0.8 * pEnemy->GetHitPoints() / pGun->GetDamage();
					const float fProbShotsToKill = fShotsToKill / fProbToHit;

					// очередей
					const int nAmmoPerBurst = pGun->GetWeapon()->nAmmoPerBurst;
					const int nBursts = ceil( fProbShotsToKill / nAmmoPerBurst );

					fTime += pGun->GetAimTime( false ) +
									 ( nBursts - 1 ) * pGun->GetRelaxTime( false ) +
									 nBursts * ( ( nAmmoPerBurst - 1 ) * pGun->GetFireRate() );

					if ( *pBestGun == 0 || fTime < fBestTime )
					{
						*pBestGun = pGun;
						*nBestGun = i;
						fBestTime = fTime;
					}
				}
			}
		}
	}
}

void CSoldierShootEstimator::SetGrenadeAutocast( bool bOn )
{
	bUseGrenadeAutocast = bOn;
}

void CSoldierShootEstimator::SetGrenadeFixed( bool bOn )
{
	bUseGrenadeFixed = bOn;
}

void CSoldierShootEstimator::AddUnit( CAIUnit *pEnemy )
{
	theScanLimiter.TargetScanning( pOwner->GetStats()->etype );	
	
	CBasicGun *pBestLocalGun = 0;
	// а не кинуть ли гранату?``
	if ( bHasGrenades && ( bUseGrenadeFixed || bUseGrenadeAutocast ) )
	{
		// enemy - mech. unit, который может двигаться
		if ( bUseGrenadeFixed || pOwner->GetGun( 1 )->CanShootToUnit( pEnemy ) /*( !pEnemy->GetStats()->IsInfantry() && pEnemy->CanMove() && !pEnemy->NeedDeinstall() )*/ )
		{
			int nAttackingGrenades = pEnemy->GetNAttackingGrenages();
			// сколько гранат, не учитывая нас
			if ( bDamageToCurTargetUpdated && pEnemy == pCurTarget )
				--nAttackingGrenades;

			// гранат не хватает и можно кинуть гранату по юниту
			if ( ( bUseGrenadeFixed || nAttackingGrenades < N_GOOD_NUMBER_ATTACKING_GRENADES ) /*&& pOwner->GetGun( 1 )->CanShootToUnit( pEnemy )*/ )
			{
				const float fDistToEnemy2 = fabs2( pOwner->GetCenter() - pEnemy->GetCenter() );

				// юнит недалеко
				if ( bUseGrenadeFixed || fDistToEnemy2 <= SConsts::MAX_DISTANCE_TO_THROW_GRENADE )
				{
					// по лучшему уже кидаем гранату
					if ( bThrowGrenade )
					{
						// если ближе, чем best unit
						if ( pBestUnit == 0 || fDistToEnemy2 < fabs2( pBestUnit->GetCenter() - pOwner->GetCenter() ) )
						{
							pBestUnit = pEnemy;
							pBestGun = pOwner->GetGun( 1 );
							nBestGun = 1;
							bUseGrenadeFixed = false;
						}
					}
					else
					{
						bThrowGrenade = true;
						pBestUnit = pEnemy;
						pBestGun = pOwner->GetGun( 1 );
						nBestGun = 1;
						bUseGrenadeFixed = false;
					}
				}
			}
		}
	}
	
	if ( !bThrowGrenade )
	{
		int nBestLocalGun = 0;
		if ( !bUseGrenadeFixed )
			ChooseGun( &pBestLocalGun, &nBestLocalGun, pEnemy );

		if ( pBestLocalGun != 0 )
		{
			const float fSoldierRating = GetRating( pMosinStats, pEnemy->GetCenterPlain(), pBestLocalGun );
			const int nSoldiers = units.GetNSoldiers( pEnemy->GetCenterPlain(), pBestLocalGun->GetDispersion(), 1 - pOwner->GetParty() );
			const float fRating = GetRating( pEnemy, pBestLocalGun ) + nSoldiers * fSoldierRating;

			if ( pBestUnit == 0 )
			{
				pBestUnit = pEnemy;
				pBestGun = pBestLocalGun;
				nBestGun = nBestLocalGun;
				fBestRating = fRating;
			}
			else
			{
				const float fNewEnemyKillUsTime = pEnemy->GetKillSpeed( pOwner );
				const float fEnemyKillUsTime = pBestUnit->GetKillSpeed( pOwner );

				if ( fEnemyKillUsTime == 0.0f && fNewEnemyKillUsTime != 0.0f ||
						 fRating > fBestRating && 
						 (	
								fEnemyKillUsTime != 0.0f && fNewEnemyKillUsTime != 0.0f ||
								fEnemyKillUsTime == 0.0f && fNewEnemyKillUsTime == 0.0f 
						 )
					 )
				{
					pBestUnit = pEnemy;
					pBestGun = pBestLocalGun;
					nBestGun = nBestLocalGun;
					fBestRating = fRating;
				}
			}
		}
	}
}

CAIUnit* CSoldierShootEstimator::GetBestUnit() const
{
	return pBestUnit;
}

CBasicGun* CSoldierShootEstimator::GetBestGun() const
{
	return pBestGun;
}

const int CSoldierShootEstimator::GetNumberOfBestGun() const
{
	return nBestGun;
}

//*******************************************************************
//*								CPlaneDeffensiveFireShootEstimator								*
//*******************************************************************

const float CPlaneDeffensiveFireShootEstimator::CalcTimeToOpenFire( class CAIUnit *pEnemy, CBasicGun *pGun ) const
{
	if ( pGun->IsInShootCone( pEnemy->GetCenterPlain() ) )
	{
		return ( pGun->InFireRange( pEnemy ) ? 0 : 10000 );
	}
	else
	{
		return 100000;
	}
}

const float CPlaneDeffensiveFireShootEstimator::CalcRating( CAIUnit *pEnemy, CBasicGun *pGun ) const
{
	const float fTimeToGo = CalcTimeToOpenFire( pEnemy, pGun );
	const float fEnemyKillUsSpeed = pEnemy->GetKillSpeed( pOwner );
	float fEnemyKillUsTime;
	if ( fEnemyKillUsSpeed == 0.0f )
		fEnemyKillUsTime = 0.0f;
	else
		fEnemyKillUsTime = pOwner->GetStats()->fMaxHP / fEnemyKillUsSpeed;

	float fKillEnemySpeed;
	if ( !bDamageToCurTargetUpdated )
		fKillEnemySpeed = pEnemy->GetTakenDamagePower() + pOwner->GetKillSpeed( pEnemy, pGun );
	else
	{
		if ( pEnemy != pCurTarget )
			fKillEnemySpeed = pEnemy->GetTakenDamagePower() + pOwner->GetKillSpeed( pEnemy, pGun );
		else
			fKillEnemySpeed = pEnemy->GetTakenDamagePower();
	}

	const float fKillEnemyTime = pEnemy->GetStats()->fMaxHP / fKillEnemySpeed;
	const float fEnemyHPPercent = pEnemy->GetHitPoints() / pEnemy->GetStats()->fMaxHP;

	return
		F( fEnemyHPPercent, fTimeToGo, fEnemyKillUsTime, fKillEnemyTime, pEnemy->GetStats()->fPrice );
}

void CPlaneDeffensiveFireShootEstimator::SetGun( CBasicGun *_pGun )
{
	pGun = _pGun;
}

CPlaneDeffensiveFireShootEstimator::CPlaneDeffensiveFireShootEstimator( class CAIUnit *pOwner )
: pOwner( pOwner ), fBestRating( -1.0f ), bDamageToCurTargetUpdated( false )
{
	
}

void CPlaneDeffensiveFireShootEstimator::Reset( class CAIUnit *pCurEnemy, const bool bDamageUpdated, const DWORD dwForbidden )
{
	pCurTarget = pCurEnemy;
	bDamageToCurTargetUpdated = bDamageUpdated;
	pBestUnit = 0;

	if ( IsValidObj( pCurTarget ) )
		AddUnit( pCurTarget );
}

void CPlaneDeffensiveFireShootEstimator::AddUnit( class CAIUnit *pEnemy )
{
	// enemy plane must be in celling of deffensive fire

	const int nGuns = pOwner->GetNGuns();
	bool bCanShootByHeight = false;

	for ( int i = 0; i < nGuns; ++i )
	{
		if ( pOwner->GetGun( i )->CanShootByHeight( pEnemy ) )
		{
			bCanShootByHeight = true;
			break;
		}
	}
	
	if ( !bCanShootByHeight )
		return;
		
		// приоритеты при защитном огне 
	const float fTempRating = CalcRating( pEnemy, pGun );

	if ( !pCurTarget || fTempRating > fBestRating )
	{
		pCurTarget = pEnemy;	
		fBestRating = fTempRating;
	}
}

class CAIUnit* CPlaneDeffensiveFireShootEstimator::GetBestUnit() const
{
	return pCurTarget;
};

CBasicGun* CPlaneDeffensiveFireShootEstimator::GetBestGun() const
{
	NI_ASSERT( false, "wrong call");
	return 0;
}

const int CPlaneDeffensiveFireShootEstimator::GetNumberOfBestGun() const
{
	NI_ASSERT( false, "wrong call");
	return 0;
}

//*******************************************************************
//*											CPlaneShturmovikShootEstimator							*
//*******************************************************************

CPlaneShturmovikShootEstimator::CPlaneShturmovikShootEstimator( class CAIUnit *pOwner )
: pOwner( pOwner ), vCenter( VNULL2 )
{
}

void CPlaneShturmovikShootEstimator::Reset( class CAIUnit *_pCurEnemy, const bool bDamageUpdated, const DWORD dwForbidden )
{
	bestAviation.Reset();
	bestForBombs.Reset();
	bestForGuns.Reset();
	if ( _pCurEnemy && _pCurEnemy->IsRefValid() && _pCurEnemy->IsAlive() )
	{
		pCurEnemy = _pCurEnemy;
		AddUnit( pCurEnemy );
	}
	buildings.clear();
}

void CPlaneShturmovikShootEstimator::AddUnit( CAIUnit *pTry )
{
	if ( !pTry || !pTry->IsRefValid() || !pTry->IsAlive() ) return;

	// if infantry is in buildlings, consider building instead of infantry.
	if ( !pTry->IsFree() && pTry->GetStats()->IsInfantry() )
	{
		NI_ASSERT( dynamic_cast<CSoldier*>( pTry ) != 0, "not soldier, but isn't free" );
		CSoldier * pSoldier = checked_cast<CSoldier*>( pTry );
		if ( pSoldier->IsInBuilding() )
		{
			CBuilding * pBuilding = pSoldier->GetBuilding();
			if ( pBuilding && pBuilding->GetStats() )
			{
				const SBuildingRPGStats * pStats = checked_cast<const SBuildingRPGStats*>( pBuilding->GetStats() );
				if ( pStats->etype != TYPE_MAIN_RU_STORAGE )
					buildings.insert( pBuilding->GetUniqueId() );
			}
		}
		return;
	}
	
	// убедиться, что какой-то из небомбовых ганов может пробить цель ( и есть патроны )
	bool bCanBreak = false;
	bool bOnlyBombs = true;
	int nGuns = pOwner->GetNGuns();
	DWORD dwPossibleGuns = 0;
	for ( int i = 0; i < nGuns; ++i )
	{
		CBasicGun *pGun = pOwner->GetGun( i );
		if ( 0 != pGun->GetNAmmo() && pGun->CanBreakArmor( pTry ) )
		{
			if ( NDb::SWeaponRPGStats::SShell::TRAJECTORY_BOMB != pGun->GetShell().etrajectory )
			{
				if( DirsDifference( pGun->GetGlobalDir(), pOwner->GetDirection() ) < 500 )
				{
					dwPossibleGuns |= 1<<i;
					bCanBreak = true;
					bOnlyBombs = false;
				}
			}
			else
			{
				bCanBreak = true;
				dwPossibleGuns |= 1<<i;
			}
		}
	}

	if ( !bCanBreak )
		return;

	if ( pTry->GetStats()->IsAviation() )
	{
		if ( !bOnlyBombs )
			CollectTarget( &bestAviation, pTry, dwPossibleGuns );	
	}
	else if ( bOnlyBombs )
		CollectTarget( &bestForBombs, pTry, dwPossibleGuns );		
	else
		CollectTarget( &bestForGuns, pTry, dwPossibleGuns );
}

class CAIUnit* CPlaneShturmovikShootEstimator::GetBestUnit() const
{
	if ( bestForGuns.pTarget )
		return bestForGuns.pTarget ;
	else if ( bestForBombs.pTarget )
		return bestForBombs.pTarget;
	else
		return bestAviation.pTarget;
};

void CPlaneShturmovikShootEstimator::CalcBestBuilding()
{
	float fRating = 0;

	for ( CBuildings::iterator it = buildings.begin(); it != buildings.end(); ++it )
	{
		CBuilding *pBuilding = checked_cast<CBuilding*>( CLinkObject::GetObjectByUniqueIdSafe( *it ) );
		const SBuildingRPGStats *pBuildingStats = checked_cast<const SBuildingRPGStats*>( pBuilding->GetStats() );

		int nGuns = pOwner->GetNGuns();
		DWORD dwPossibleGuns = 0;
		bool bCanBreak = false;
		for ( int i = 0; i < nGuns; ++i )
		{
			CBasicGun *pGun = pOwner->GetGun( i );
			if ( 0 != pGun->GetNAmmo() && pGun->CanBreach( pBuildingStats, RPG_TOP ) )
			{
				if ( NDb::SWeaponRPGStats::SShell::TRAJECTORY_BOMB != pGun->GetShell().etrajectory ||
							DirsDifference( pGun->GetGlobalDir(), pOwner->GetDirection() ) < 500 )
				{
					dwPossibleGuns |= 1<<i;
					bCanBreak = true;
				}
			}
		}

		float fCurrentRating = 0.0f;
		if ( bCanBreak )
		{
			const float fKillBuildingSpeed = pOwner->GetKillSpeed( pBuildingStats, pBuilding->GetAttackCenter( pOwner->GetCenterPlain() ), dwPossibleGuns );
			const float fKillBuildingTime = fKillBuildingSpeed == 0.0f ? 0.0f : 1.0f / fKillBuildingSpeed;
			
			float fTotalPrice = 0;
			const int nDefenders = pBuilding->GetNDefenders();
			for ( int i = 0; i < nDefenders; ++i )
				fTotalPrice += pBuilding->GetUnit( i )->GetPriceMax();
			fCurrentRating = F( pBuilding->GetHitPoints() / pBuildingStats->fMaxHP, 0, 0, fKillBuildingTime, fTotalPrice );
		}
		if ( !pBestBuilding || fCurrentRating > fRating )
		{
			fRating = fCurrentRating;
			pBestBuilding = pBuilding;
		}
	}
}

const float CPlaneShturmovikShootEstimator::CalcTimeToOpenFire( CAIUnit *pEnemy ) const
{
	const WORD wDir = pOwner->GetDirection();
	const WORD wDirToEnemy = GetDirectionByVector( pEnemy->GetCenterPlain() - pOwner->GetCenterPlain() );
	return float( DirsDifference( wDir, wDirToEnemy ) ) / 65535.0f;
}

const float CPlaneShturmovikShootEstimator::CalcRating( CAIUnit *pEnemy, const DWORD dwPossibleGuns ) const
{
	const float fEnemyKillUsSpeed = pEnemy->GetKillSpeed( pOwner );
	const float fKillEnemy = pOwner->GetKillSpeed( pEnemy, dwPossibleGuns );
	const float fKillEnemyByOthers = (pEnemy == pCurEnemy ? pEnemy->GetTakenDamagePower() - fKillEnemy : pEnemy->GetTakenDamagePower());

	float fPrice = pEnemy->GetPriceMax();
	fPrice = 0.005f * ( fPrice == 0.0f ? 0.0f : 1.0f - 1.0f / fPrice );

	return fKillEnemy == 0 ? 0 : ( fEnemyKillUsSpeed * 100000 + ( 5 * fKillEnemy - fKillEnemyByOthers + fPrice ) );
}

void CPlaneShturmovikShootEstimator::CollectTarget( CPlaneShturmovikShootEstimator::STargetInfo * pInfo, class CAIUnit *pTarget, const DWORD dwPossibleGuns )
{
	const float fRating = CalcRating( pTarget, dwPossibleGuns );

	if (!pInfo->pTarget || fRating > pInfo->fRating ) 
	{
		pInfo->pTarget = pTarget;
		pInfo->fRating = fRating;
	}
}

//*******************************************************************
//*											 CShootEstimatorForObstacles*
//*******************************************************************

bool CShootEstimatorForObstacles::AddObstacle( interface IObstacle *pObstacle )
{
	// уничтожать только вражеские препятствия
	if ( theDipl.GetDiplStatus( pObstacle->GetPlayer(), pOwner->GetPlayer() ) != EDI_ENEMY )
		return false;

	//учитывать C
	// - время поворота
	// - скорость уничтожения
	// - время подъезда
	// - количество оставшегося здоровья

	NTimer::STime timeToKill = 0;
	CBasicGun *pGun = pObstacle->ChooseGunToShootToSelf( pOwner, &timeToKill );
	if ( pGun )
	{
		float fTimeToGo = 0;
		/*if ( pOwner->CanMove() && !pOwner->NeedDeinstall() )
			fTimeToGo = fabs( pOwner->GetCenter() - pObstacle->GetCenter() );*/
		fTimeToGo += FindTimeToTurnToPoint( CVec2(pObstacle->GetCenter().x, pObstacle->GetCenter().y), pOwner, pGun );

		float fRating = F( pObstacle->GetHPPercent(), fTimeToGo, 0, timeToKill, 0 );
		
		if ( !pBest || fCurRating < fRating )
		{
			pBest = pObstacle;
			fCurRating = fRating;
		}
	}
	return false;
}

interface IObstacle * CShootEstimatorForObstacles::GetBest() const
{
	return pBest;
}

//*******************************************************************
//*											 CShootEstimatorForObstacles*
//*******************************************************************

CShootEstimatorSupportAAGun::CShootEstimatorSupportAAGun ( class CAIUnit *_pOwner )
: pOwner( _pOwner ), fBestRating( 0.f ), dwForbidden( 0 ), bDamageToCurTargetUpdated ( false )
{
}

void CShootEstimatorSupportAAGun::Reset( class CAIUnit *pCurEnemy, const bool bDamageUpdated, const DWORD _dwForbidden )
{
	pBestTarget = pCurEnemy;
	bDamageToCurTargetUpdated = bDamageUpdated;
	dwForbidden = _dwForbidden;
	fBestRating = 0.0f;

	if ( IsValidObj( pBestTarget ) )
		AddUnit( pBestTarget );
}

void CShootEstimatorSupportAAGun::AddUnit( class CAIUnit *pEnemy )
{
	const int nGuns = pOwner->GetNGuns();
	float fTempRating = 0.0f;

	for ( int i = 0; i < nGuns; ++i )
	{
		CBasicGun * pGun = pOwner->GetGun( i );
		if ( !(dwForbidden & (1<<i) ) && pGun->CanShootByHeight( pEnemy ) )
			fTempRating += CalcRating( pEnemy, pGun );
	}

	if ( fTempRating == 0.0f )
		return;

	// приоритеты при защитном огне 
	if ( !pBestTarget || fTempRating > fBestRating )
	{
		pBestTarget = pEnemy;	
		fBestRating = fTempRating;
	}
}

const float CShootEstimatorSupportAAGun::CalcRating( CAIUnit *pEnemy, CBasicGun *pGun ) const
{
	const float fTimeToGo = fabs( pEnemy->GetCenterPlain() - pOwner->GetGunCenter( pGun->GetCommonGunNumber(), pGun->GetPlatform() ) ) / pEnemy->GetSpeed();

	float fKillEnemySpeed;
	if ( !bDamageToCurTargetUpdated )
		fKillEnemySpeed = pEnemy->GetTakenDamagePower() + pOwner->GetKillSpeed( pEnemy, pGun );
	else
		fKillEnemySpeed = pEnemy->GetTakenDamagePower();

	const float fKillEnemyTime = pEnemy->GetStats()->fMaxHP / fKillEnemySpeed;
	const float fEnemyHPPercent = pEnemy->GetHitPoints() / pEnemy->GetStats()->fMaxHP;

	return
		F( fEnemyHPPercent, fTimeToGo, 0, fKillEnemyTime, pEnemy->GetStats()->fPrice );
}

