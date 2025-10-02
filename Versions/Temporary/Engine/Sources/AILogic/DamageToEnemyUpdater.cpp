#include "stdafx.h"

#include <float.h>
#include "AIUnit.h"
#include "Guns.h"
#include "DamageToEnemyUpdater.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*												CDamageToEnemyUpdater											*
//*******************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDamageToEnemyUpdater::SetDamageToEnemy( CAIUnit *pOwner, CAIUnit *pEnemy, const DWORD dwGuns )
{
	UnsetDamageFromEnemy( pCurEnemy );
	pCurEnemy = pEnemy;
	if ( nTakenDamageUpdated == 0 && fabs2( pOwner->GetCenter() - pEnemy->GetCenter() ) <= sqr( 1.3f * pOwner->GetMaxFireRange() ) )
	{
		nTakenDamageUpdated = 2;

		fTakenDamagePower = pOwner->GetKillSpeed( pEnemy, dwGuns );
		pEnemy->UpdateTakenDamagePower( fTakenDamagePower );
		NI_ASSERT( _finite( fTakenDamagePower ) != 0, "Wrong fTakenDamagePower" );
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDamageToEnemyUpdater::SetDamageToEnemy( CAIUnit *pOwner, CAIUnit *pEnemy, CBasicGun *pGun )
{
	if ( pCurEnemy == pEnemy )
		return; // nothing to do

	UnsetDamageFromEnemy( pCurEnemy );
	pCurEnemy = pEnemy;
	if ( nTakenDamageUpdated == 0 && fabs2( pOwner->GetCenter() - pEnemy->GetCenter() ) <= sqr( 1.3f * pOwner->GetMaxFireRange() ) && pGun != 0 )
	{
		if ( pGun->IsGrenade() )
		{
			nTakenDamageUpdated = 1;
			pEnemy->UpdateNAttackingGrenages( 1 );
		}
		else
			nTakenDamageUpdated = 2;

		fTakenDamagePower = pOwner->GetKillSpeed( pEnemy );
		pEnemy->UpdateTakenDamagePower( fTakenDamagePower );

		NI_ASSERT( _finite( fTakenDamagePower ) != 0, "Wrong fTakenDamagePower" );
	}
	else
		nTakenDamageUpdated = 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDamageToEnemyUpdater::UnsetDamageFromEnemy( CAIUnit *pEnemy )
{
	if ( nTakenDamageUpdated != 0 && pEnemy && pEnemy->IsAlive() && pEnemy == pCurEnemy )
	{
		if ( nTakenDamageUpdated == 1 )
			pEnemy->UpdateNAttackingGrenages( -1 );

		NI_ASSERT( _finite( fTakenDamagePower ) != 0, "Wrong fTakenDamagePower" );

		pEnemy->UpdateTakenDamagePower( -fTakenDamagePower );
		nTakenDamageUpdated = 0;
	}
	pCurEnemy = 0;
}

