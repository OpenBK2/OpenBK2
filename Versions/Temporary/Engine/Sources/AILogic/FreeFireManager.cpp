#include "stdafx.h"

#include "FreeFireManager.h"
#include "Guns.h"
#include "UnitsIterators2.h"
#include "Turret.h"
#include "AIUnit.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern NTimer::STime curTime;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CFreeFireManager::CFreeFireManager( CCommonUnit *pOwner ) 
: shootInfo( 2 * pOwner->GetNGuns() ), lastCheck( 0 ) 
{
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFreeFireManager::Analyze( CCommonUnit *pUnit, CBasicGun *pActiveGun )
{
	if ( curTime - lastCheck >= TIME_TO_CHECK )
	{
		const int nRandom = NRandom::Random( 0, 300 );
		if ( nRandom < curTime )
			lastCheck = curTime - NRandom::Random( 0, 500 );
		else
			lastCheck = curTime;
		
		const float fSightRadius = pUnit->GetSightRadius();
		DWORD dwForbidden = 0;

		int nTriedGuns = 0;
		for ( int i = 0; i < pUnit->GetNGuns(); ++i )
		{
			CBasicGun *pGun = pUnit->GetGun( i );

			// нельзя анализировать
			if ( pGun->IsCommonEqual( pActiveGun ) || pGun->IsCommonGunFiring() || 
				pGun->GetGun().nPriority == 0 && !pUnit->IsIdle() && !pUnit->CanShootInMovement() || pGun->GetNAmmo() == 0 )
			{
				dwForbidden |= ( 1 << i );
				++nTriedGuns;
			}
		}

		while ( nTriedGuns < pUnit->GetNGuns() )
		{
			pUnit->ResetShootEstimator( 0, false, dwForbidden );
			for ( CUnitsIter<1,3> iter( pUnit->GetParty(), EDI_ENEMY, pUnit->GetCenterPlain(), fSightRadius ); !iter.IsFinished(); iter.Iterate() )
			{
				if ( (*iter)->IsAlive() && (*iter)->IsNoticableByUnit( pUnit, fSightRadius ) )
					pUnit->AddUnitToShootEstimator( *iter );
			}

			CAIUnit *pTarget = pUnit->GetBestShootEstimatedUnit();

			if ( pTarget != 0 )
			{
				CBasicGun *pGun = pUnit->GetBestShootEstimatedGun();
				int nGun = pUnit->GetNumOfBestShootEstimatedGun();

				if ( CTurret *pTurret = pGun->GetTurret() )
				{
					if ( !pTurret->IsLocked( pGun ) )
						pTurret->Lock( pGun );
				}

				pGun->StartEnemyBurst( pTarget, shootInfo[nGun].NeedAim( pTarget, pGun ) );
				shootInfo[nGun].SetInfo( pTarget, pGun );

				++nTriedGuns;
				dwForbidden |= ( 1 << nGun );
			}
			else
				return;
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CFreeFireManager::SShotInfo::NeedAim( CAIUnit *pNewTarget, CBasicGun *pGun ) const
{
	return pNewTarget != pTarget || shootingPos != pNewTarget->GetCenterPlain() || unitDir != pNewTarget->GetFrontDirection() || gunDir != pGun->GetGlobalDir();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFreeFireManager::SShotInfo::SetInfo( CAIUnit *pNewTarget, CBasicGun *pGun )
{
	pTarget = pNewTarget;
	shootingPos = pNewTarget->GetCenterPlain();
	unitDir = pNewTarget->GetFrontDirection();
	gunDir = pGun->GetGlobalDir();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
