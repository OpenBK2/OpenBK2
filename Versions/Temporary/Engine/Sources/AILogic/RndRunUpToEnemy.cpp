#include "stdafx.h"

#include "RndRunUpToEnemy.h"
#include "../Common_RTS_AI/PathFinder.h"
#include "Soldier.h"
#include "Guns.h"
#include "SerializeOwner.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern NTimer::STime curTime;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CRndRunUpToEnemy::CRndRunUpToEnemy( CAIUnit *pOwner, CAIUnit *pEnemy, bool bCanMove )
{
	Init( pOwner, pEnemy, bCanMove );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CRndRunUpToEnemy::Init( CAIUnit *_pOwner, CAIUnit *_pEnemy, bool bCanMove )
{
	pEnemy = _pEnemy;
	bRunningToEnemy = false;
	bForceStaying = !bCanMove;
	vLastOwnerPos = _pOwner->GetCenterPlain();
	checkTime = 0;

	if ( _pOwner->GetStats()->IsInfantry() && _pOwner->GetStats()->etype != RPG_TYPE_SNIPER && _pOwner->GetBehaviourMoving() != SBehaviour::EMHoldPos )
	{
		bCheck = true;
		pOwner = checked_cast<CSoldier*>( _pOwner );
	}
	else
	{
		pOwner = 0;
		bCheck = false;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CRndRunUpToEnemy::SendOwnerToRandomRun()
{
	if ( bForceStaying )
		return;
	NI_ASSERT( !bForceStaying, "Wrong force staying value ( false expected )" );
	
	const CVec2 vDirToEnemy = pEnemy->GetCenterPlain() - pOwner->GetCenterPlain();
	const WORD wDirToEnemy = GetDirectionByVector( vDirToEnemy );

	const WORD wRandomAngle = NRandom::Random( 0, 65536 / 5 );
	WORD wResultDir;
	if ( NRandom::Random( 0.0f, 1.0f ) < 0.5f )
		wResultDir = wDirToEnemy - wRandomAngle;
	else
		wResultDir = wDirToEnemy + wRandomAngle;

	float fRandomDist;
	// ползти
	if ( NRandom::Random( 0.0f, 1.0f ) < 0.7f )
	{
		fRandomDist = NRandom::Random( float( 0.4f * SConsts::TILE_SIZE ), float( 2.0f * SConsts::TILE_SIZE ) );
		bForceStaying = false;
	}
	else
	{
		fRandomDist = NRandom::Random( float( 2.0f * SConsts::TILE_SIZE ), float( 4.0f * SConsts::TILE_SIZE ) );
		bForceStaying = true;
	}

	const CVec2 vPointToRunUp = pOwner->GetCenterPlain() + GetVectorByDirection( wResultDir ) * fRandomDist;

	if ( CPtr<IStaticPath> pStaticPath = CreateStaticPathToPoint( vPointToRunUp, VNULL2, pOwner, true, GetAIMap() ) )
	{
		// путь не слишком длинный и конечная точка не слишком далека от нужной нам
		if ( ( bForceStaying && pStaticPath->GetLength() <= 5 ||
 				   !bForceStaying && pStaticPath->GetLength() <= 3 ) &&
				 fabs2( pStaticPath->GetFinishPoint() - vPointToRunUp ) < sqr( 3.0f * SConsts::TILE_SIZE / 4.0f ) )
		{
			bRunningToEnemy = true;

			if ( bForceStaying )
				pOwner->AllowLieDown( false );

			pOwner->SendAlongPath( pStaticPath, VNULL2, false );
		}
		else
		{
			bRunningToEnemy = false;	
			bForceStaying = false;
		}
	}
	else
	{
		bRunningToEnemy = false;	
		bForceStaying = false;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CRndRunUpToEnemy::Segment()
{
	if ( bForceStaying )
		return;
	if (
			 bCheck &&
		   pOwner->IsFree() && IsValidObj( pEnemy ) &&
			 ( bRunningToEnemy || fabs2( pOwner->GetCenter() - pEnemy->GetCenter() ) >= sqr(0.7f) * sqr(pOwner->GetGun( 0 )->GetFireRange( 0 )) ) 
		 )
	{
		if ( !bRunningToEnemy )
		{
			if ( curTime >= checkTime )
			{
				if ( pOwner->IsIdle() && vLastOwnerPos == pOwner->GetCenterPlain() && NRandom::Random( 0.0f, 1.0f ) <= 0.7f )
					SendOwnerToRandomRun();

				if ( !bRunningToEnemy )
				{
					vLastOwnerPos = pOwner->GetCenterPlain();
					checkTime = curTime + NRandom::Random( 2000, 5000 );
				}
			}
		}
		else if ( pOwner->IsIdle() )
		{
			bRunningToEnemy = false;
			vLastOwnerPos = pOwner->GetCenterPlain();
			checkTime = curTime + NRandom::Random( 2000, 5000 );

			if ( bForceStaying )
			{
				pOwner->AllowLieDown( true );
				bForceStaying = false;
			}
		}
	}

	NI_ASSERT( bRunningToEnemy || !bForceStaying, "Wrong force staying value ( false expected )" );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CRndRunUpToEnemy::Finish()
{
	if ( pOwner )
		pOwner->AllowLieDown( true );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CRndRunUpToEnemy::OnSerialize( IBinSaver &saver )
{
	SerializeOwner( 1, &pOwner, &saver );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
