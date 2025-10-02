#include "stdafx.h"

#include "Soldier.h"
#include "GunsInternal.h"
#include "UnitGuns.h"
#include "Turret.h"
#include "PathFinder.h"
#include "Weather.h"

extern NTimer::STime curTime;
extern CWeather theWeather;

//*******************************************************************
//*										CUnitGuns																			*
//*******************************************************************

REGISTER_SAVELOAD_CLASS( 0x1108D4C7, CMechUnitGuns );
REGISTER_SAVELOAD_CLASS( 0x1108D4C8, CInfantryGuns );
REGISTER_SAVELOAD_CLASS( 0x1108D4C9, SCommonGunInfo );
BASIC_REGISTER_CLASS( CUnitGuns );

bool CUnitGuns::AddGun( const interface IGunsFactory &gunsFactory, const int nPlatform, const int nGunInStats, const SWeaponRPGStats *pWeapon, int *nGuns, const int nAmmo )
{
	NI_VERIFY( pWeapon != 0, "Gun w/o weapon! See next assert for unit ID", return false );
	//
	const int nCommonGun = gunsFactory.GetNCommonGun();
	if ( commonGunsInfo.size() <= nCommonGun )
		commonGunsInfo.resize( nCommonGun + 1 );
	if ( gunsBegins.size() <= nCommonGun + 1 )
		gunsBegins.resize( nCommonGun + 2, 0 );

	commonGunsInfo[nCommonGun] = new SCommonGunInfo( false, nAmmo, nPlatform, nGunInStats );

	gunsBegins[nCommonGun] = *nGuns;
	gunsBegins[nCommonGun+1] = *nGuns + pWeapon->shells.size();

	if ( nCommonGuns < nCommonGun + 1 )
		nCommonGuns = nCommonGun + 1;

	if ( guns.size() < *nGuns + pWeapon->shells.size() )
		guns.resize( *nGuns + pWeapon->shells.size() );
	for ( int i = 0; i < pWeapon->shells.size(); ++i )
	{
		CBasicGun *pGun;
		
		if ( pWeapon->shells[i].etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_BOMB )
			pGun = gunsFactory.CreateGun( IGunsFactory::PLANE_GUN, i, commonGunsInfo[nCommonGun] );//when create bomb
		else if ( pWeapon->shells[i].etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE )
		{
			if ( pWeapon->shells[i].fArea2 == 0 )
				pGun = gunsFactory.CreateGun( IGunsFactory::MOMENT_CML_GUN, i, commonGunsInfo[nCommonGun] );
			else
				pGun = gunsFactory.CreateGun( IGunsFactory::MOMENT_BURST_GUN, i, commonGunsInfo[nCommonGun] );
		}
		else if ( pWeapon->shells[i].etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_TORPEDO )
		{
			pGun = gunsFactory.CreateGun( IGunsFactory::TORPEDO_GUN, i, commonGunsInfo[nCommonGun] );
		}
		else if ( pWeapon->shells[i].etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_ROCKET )
		{
			pGun = gunsFactory.CreateGun( IGunsFactory::ROCKET_GUN, i, commonGunsInfo[nCommonGun] );
		}
		else if ( pWeapon->shells[i].etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_FLAME_THROWER )
			pGun = gunsFactory.CreateGun( IGunsFactory::FLAME_GUN, i, commonGunsInfo[nCommonGun] );
		else
		{
			if ( pWeapon->shells[i].fArea2 == 0 )
				pGun = gunsFactory.CreateGun( IGunsFactory::VIS_CML_BALLIST_GUN, i, commonGunsInfo[nCommonGun] );
			else
				pGun = gunsFactory.CreateGun( IGunsFactory::VIS_BURST_BALLIST_GUN, i, commonGunsInfo[nCommonGun] );
		}

		guns[(*nGuns)++] = pGun;
		if ( pWeapon->fRangeMax > fMaxFireRange )
			fMaxFireRange = pWeapon->fRangeMax;

		if ( pWeapon->nCeiling > 0 )
			bCanShootToPlanes = true;

		if ( i == 0 && pGun->GetGun().nPriority == 0 )
			nMainGun = (*nGuns) - 1;
	}
	return true;
}

void CUnitGuns::Segment()
{
	for ( int i = 0; i < guns.size(); ++i )
		guns[i]->Segment();
}

void CUnitGuns::FindTimeToTurn( CAIUnit *pOwner, const WORD wWillPower, CTurret *pTurret, CAIUnit *pEnemy, const SVector &finishTile, const bool bIsEnemyInFireRange, NTimer::STime *pTimeToTurn ) const
{
	*pTimeToTurn = 0;
	// нужно учесть время на развороты
	if ( pOwner->CanRotate() && ( !bIsEnemyInFireRange || pTurret == 0 ) )
	{
		const WORD finishToEnemyDir = GetDirectionByVector( (pEnemy->GetCenterTile() - finishTile).ToCVec2() );
		const WORD dirsDiff( DirsDifference( pOwner->GetDirection(), finishToEnemyDir ) );

		if ( dirsDiff > wWillPower )
			*pTimeToTurn = dirsDiff / pOwner->GetTurnSpeed();
	}

	if ( pTurret != 0 )
	{
		WORD startAngle = pTurret->GetHorCurAngle() + pOwner->GetFrontDirection();

		WORD finalAngle;
		if ( !bIsEnemyInFireRange )
			finalAngle = GetDirectionByVector( (pEnemy->GetCenterTile() - finishTile).ToCVec2() );
		else
			finalAngle = GetDirectionByVector( (pEnemy->GetCenterTile() - pOwner->GetCenterTile()).ToCVec2() );

		const WORD dirsDiff = DirsDifference( finalAngle, startAngle );

		if ( dirsDiff > wWillPower )
		{
			const WORD wHorizontalRotationSpeed = pTurret->GetHorRotateSpeed();
			NI_ASSERT( wHorizontalRotationSpeed != 0, StrFmt("horizontal rotation speed == 0 for \"%s\"", pOwner->GetStats()->szKeyName.c_str()) );
			*pTimeToTurn += DirsDifference( startAngle, finalAngle ) / wHorizontalRotationSpeed;
		}
	}
}

bool CUnitGuns::FindTimeToStatObjGo( CAIUnit *pUnit, CStaticObject *pObj, const SWeaponRPGStats *pStats, CUnitGuns::SWeaponPathInfo &info ) const
{
	const float fFireRangeMax = GetFireRangeMax( pStats, pUnit );
	CPtr<IStaticPath> pPath = CreateStaticPathForStObjAttack( pUnit, pObj, pStats->fRangeMin, fFireRangeMax, false );

	if ( !IsValid( pPath ) || pPath->GetLength() == -1 )
		return false;

	info.fRadius = fFireRangeMax;
	info.time = pPath->GetLength() * SConsts::TILE_SIZE * pUnit->GetStats()->fSpeed;
	info.pStaticPath = pPath;
	return true;
}

CBasicGun* CUnitGuns::ChooseGunForStatObj( CAIUnit *pOwner, CStaticObject *pObj, NTimer::STime *pTime )
{
	if ( pOwner->GetNGuns() == 0 )
		return 0;
	
	*pTime = 0;
	int nGun = -1;
	
	list< SWeaponPathInfo > pathInfo666( 0 );
	const SWeaponRPGStats *pWStats = pOwner->GetGun(0)->GetWeapon();
	int i = 0;
	do
	{
		CBasicGun *pGun = pOwner->GetGun(i);
		const SWeaponRPGStats::SShell &shell = pGun->GetShell();
		// разрывными снарядами и возможно дострелить
		if ( shell.eDamageType != NDb::SWeaponRPGStats::SShell::DAMAGE_HEALTH || shell.fArea2 <= 0 )
			pGun->SetRejectReason( ACK_NEGATIVE );
		else if ( pGun->CanShootToObject( pObj ) )
		{
			SWeaponPathInfo info;			
			if ( !pOwner->CanMove() || FindTimeToStatObjGo( pOwner, pObj, pWStats, info ) )
			{
				// если мы в формации и нельзя выходить за её пределы, а чтобы стрелять в юнита, придётся				
				if ( pOwner->CanMove() && !pOwner->CanGoToPoint( info.pStaticPath->GetFinishPoint() ) )
					continue;

				NTimer::STime time;
				if ( pOwner->CanMove() )
					time = info.time;
				else
					time = 0;

				// выстрелов, чтобы убить
				const float fShotsToKill = pObj->GetHitPoints() / pGun->GetDamage();
				// очередей
				const int nBursts = ceil( fShotsToKill / pWStats->nAmmoPerBurst );

				// кол-во очередей * ( время прицеливания + ( кол-во выстрелов - 1 ) * время между выстрелами )
				time += nBursts * ( pWStats->nAimingTime + ( pWStats->nAmmoPerBurst - 1 ) * pGun->GetFireRate() );

				if ( time < *pTime || nGun == -1 )
				{
					*pTime = time;
					nGun = i;
				}
			}
		}
	
		++i;
		if ( i < GetNTotalGuns() )
			pWStats = pOwner->GetGun(i)->GetWeapon();
	} while ( i < pOwner->GetNGuns() );

	if ( nGun == -1 )
		return 0;
	else
		return pOwner->GetGun( nGun );
}

void CUnitGuns::SetOwner( CAIUnit *pUnit )
{
	for ( int i = 0; i < guns.size(); ++i )
		guns[i]->SetOwner( pUnit );
}

const SBaseGunRPGStats& CUnitGuns::GetCommonGunStats( const int nCommonGun ) const
{
	NI_ASSERT( nCommonGun < nCommonGuns, StrFmt( "Wrong number of gun (%d), total number of guns (%d)", nCommonGun, nCommonGuns ) );
	return guns[gunsBegins[nCommonGun]]->GetGun();
}

int CUnitGuns::GetNAmmo( const int nCommonGun ) const
{
	NI_ASSERT( nCommonGun < nCommonGuns, StrFmt( "Wrong number of gun (%d), total number of guns (%d)", nCommonGun, nCommonGuns ) );
	return commonGunsInfo[nCommonGun]->nAmmo;
}

// nAmmo со знаком
void CUnitGuns::ChangeAmmo( const int nCommonGun, const int nAmmo )
{
	NI_ASSERT( nCommonGun < nCommonGuns, StrFmt( "Wrong number of gun (%d), total number of guns (%d)", nCommonGun, nCommonGuns ) );
	commonGunsInfo[nCommonGun]->nAmmo += nAmmo;
	commonGunsInfo[nCommonGun]->nAmmo = Clamp( commonGunsInfo[nCommonGun]->nAmmo, 0, GetNAmmo( nCommonGun ) );
}

const EUnitAckType CUnitGuns::GetRejectReason() const
{
	int nMaxPriority = 10000;
	EUnitAckType eReason = ACK_NONE;
	float fMaxFireRange = -1.0f;

	for ( int i = 0; i < guns.size(); ++i )
	{
		const EUnitAckType &eGunReason = guns[i]->GetRejectReason();
		if ( eGunReason != ACK_NONE )
		{
			const int nPriority = guns[i]->GetGun().nPriority;
			if ( nPriority < nMaxPriority )
			{
				nMaxPriority = guns[i]->GetGun().nPriority;
				fMaxFireRange = guns[i]->GetFireRange( 0.0f );

				eReason = eGunReason;
			}
			else if ( nPriority == nMaxPriority )
			{
				const float fFireRange = guns[i]->GetFireRange( 0.0f );
				if ( fFireRange > fMaxFireRange )
				{
					fMaxFireRange = fFireRange;
					eReason = eGunReason;
				}
			}
		}
	}

	return eReason;
}

bool CUnitGuns::DoesExistRejectReason( const EUnitAckType &ackType ) const
{
	for ( int i = 0; i < guns.size(); ++i )
	{
		if ( guns[i]->GetRejectReason() == ackType )
			return true;
	}

	return false;
}

CBasicGun* CUnitGuns::GetMainGun() const
{
	if ( guns.size() != 0 )
		return guns[nMainGun];
	else
		return 0;
}

float CUnitGuns::GetMaxFireRange( const CAIUnit *pOwner ) const
{
	return fMaxFireRange;
}

//*******************************************************************
//*											CMechUnitGuns																*
//*******************************************************************

void CMechUnitGuns::Init( CCommonUnit *pCommonUnit )
{
	CAIUnit *pUnit = checked_cast<CAIUnit*>(pCommonUnit);
	const SMechUnitRPGStats *pStats = checked_cast<const SMechUnitRPGStats*>( pUnit->GetStats() );

	int nGuns = 0;
	float fMaxRevealRadius = 0;
	int nCommonGun = 0;

	for ( int i = 0; i < pStats->GetPlatformsSize( pUnit->GetUniqueId() ); ++i )
	{
		for ( int j = 0; j < pStats->GetGunsSize( pUnit->GetUniqueId(), i ); ++j )
		{
			const SBaseGunRPGStats &gun = pStats->GetGun( pUnit->GetUniqueId(), i, j );
			if ( gun.pWeapon  )
			{
				AddGun( CUnitsGunsFactory( pUnit, nCommonGun++, i-1 ), i, j, gun.pWeapon, &nGuns, gun.nAmmo );

				if ( gun.pWeapon->fRevealRadius > fMaxRevealRadius )
					fMaxRevealRadius = gun.pWeapon->fRevealRadius;
			}
		}
	}

	if ( fMaxRevealRadius > 0 || pUnit->GetStats()->eDBtype == NDb::DB_RPG_TYPE_ART_AAGUN )
		pUnit->CreateAntiArtillery( fMaxRevealRadius );

	int i = 0;
	while ( i < GetNGuns() && 
					(
						NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE == GetGun(i)->GetShell().etrajectory ||
						GetGun(i)->GetShell().eDamageType == NDb::SWeaponRPGStats::SShell::DAMAGE_MORALE ||
						GetGun(i)->GetShell().eDamageType == NDb::SWeaponRPGStats::SShell::DAMAGE_FOG
					)
				)
		++i;

	nFirstArtGun = ( i < GetNGuns() ) ? i : -1;
	
	for ( int i = 1; i < GetNGuns(); ++i )
	{
		if ( GetGun( i )->GetGun().nPriority == 0 )
		{
			int j = 0;
			while ( j < i && 
							( GetGun( j )->GetGun().nPriority != 0 || 
							  GetGun( j )->GetCommonGunNumber() == GetGun( i )->GetCommonGunNumber() ) )
				++j;

			if ( j < i )
			{
				GetGun( j )->AddParallelGun( GetGun( i ) );
				GetGun( i )->SetToParallelGun();
			}
		}
	}
}

bool CMechUnitGuns::SetActiveShellType( const NDb::SWeaponRPGStats::SShell::EShellDamageType eShellType )
{
	int i = 0;
	while ( i < GetNGuns() && 
					(
					NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE == GetGun(i)->GetShell().etrajectory ||
					GetGun(i)->GetShell().eDamageType != eShellType
					)
				)
	{
		++i;
	}
	if ( i < GetNGuns() && nFirstArtGun != i )
	{
		nFirstArtGun = i ;
		return true;
	}
	return false;
}

const int CMechUnitGuns::GetActiveShellType() const
{
	if ( nFirstArtGun == -1 )
		return NDb::SWeaponRPGStats::SShell::DAMAGE_HEALTH;
	else
		return GetGun(nFirstArtGun)->GetShell().eDamageType;
}

class CBasicGun* CMechUnitGuns::GetFirstArtilleryGun() const
{ 
	if ( nFirstArtGun >= 0 ) 
		return GetGun( nFirstArtGun ); 
	else return 0; 
}

//*******************************************************************
//*											CInfantryGuns																*
//*******************************************************************

void CInfantryGuns::Init( CCommonUnit *pCommonUnit )
{
	CSoldier *pUnit = checked_cast<CSoldier*>(pCommonUnit);
	const SInfantryRPGStats *pStats = checked_cast<const SInfantryRPGStats*>( pUnit->GetStats() );

	int nGuns = 0;
	const int nUnitUniqueID = pUnit->GetUniqueId();
	for ( int i = 0; i < pStats->GetGunsSize( nUnitUniqueID, 0 ); ++i )
	{
		bool bSuccess = AddGun( CUnitsGunsFactory( pUnit, i, -1 ), 0, i, pStats->GetGun( nUnitUniqueID, 0, i ).pWeapon, &nGuns, pStats->GetGun( nUnitUniqueID, 0, i ).nAmmo );
		NI_ASSERT( bSuccess, StrFmt("Can't add gun to unit \"%s\"", NDb::GetResName(pStats)) );
	}
}

