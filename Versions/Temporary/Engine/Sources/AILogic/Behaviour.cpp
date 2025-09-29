#include "stdafx.h"

#include "Behaviour.h"
#include "UnitsIterators2.h"
#include "Guns.h"
#include "Aviation.h"
#include "HitsStore.h"
#include "Turret.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern NTimer::STime curTime;
extern CDiplomacy theDipl;
extern CGroupLogic theGroupLogic;
extern CHitsStore theHitsStore;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*												CShootEstimatorLighAA											*
//*******************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CShootEstimatorLighAA::Init( CCommonUnit *_pUnit )
{
	pUnit = _pUnit;
	
	party = pUnit->GetParty();
	
	pResult = 0;
	fWorstDamage = -1;
	bestTime = 100000;
	fMinDistance = 1e10;
	pGun = 0;
	bCanShootNow = false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CShootEstimatorLighAA::Init( class CCommonUnit *pUnit, CBasicGun *_pGun )
{
	Init( pUnit );
	pGun = _pGun;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CShootEstimatorLighAA::AddUnit( CAIUnit *pTarget )
{
	if ( IsValidObj( pTarget ) && pTarget->GetStats()->IsAviation() )
	{
		if ( pGun != 0 && ( !pGun->CanShootByHeight( pTarget ) || !pGun->CanBreakArmor( pTarget ) ) ) return;
		
		CBasicGun *pChosenGun = pGun;
		// gun не задан, нужно его выбрать и найти время для убиения врага
		if ( pChosenGun == 0 )
		{
			int i = 0;
			while ( i < pUnit->GetNGuns() && !pUnit->GetGun(i)->CanBreach( pTarget ) )
				++i;
			if ( i >= pUnit->GetNGuns() )
				return;
			//CRAP{ считаем, что у зениток 1 пушка
			pChosenGun = pUnit->GetGun( i );
			//CRAP}
		}


		const float fDistance = fabs2( pUnit->GetCenter() - pTarget->GetCenter() );
		const float fDamage = pTarget->GetMaxDamage( pUnit );
		
		//его можно прострелить
		if ( pChosenGun != 0 )
		{
			static NTimer::STime timeToShoot = DirsDifference( GetDirectionByVector( pTarget->GetCenterPlain() - pUnit->GetCenterPlain() ), pChosenGun->GetGlobalDir() );
			const bool bCanShoot = pChosenGun->CanShootToUnit( pTarget );
			if ( pResult == 0 )
			{
				bCanShootNow = bCanShoot;
				pResult = pTarget;
				bestTime = timeToShoot;
				fWorstDamage = fDamage;
				fMinDistance = fDistance;
			}
			else
			{
				// он может нанести больший damage 
				// или может стрелять (исходя из дистанции)
				// или он нанесёт такой же damage, но его быстрее пристрелить
				// или damage и время совпадают, но он ближе
				if (	fDamage > fWorstDamage ||
							fDamage == fWorstDamage && bCanShoot && !bCanShootNow ||
							fDamage == fWorstDamage && bCanShoot == bCanShootNow && bestTime > timeToShoot ||
							fDamage == fWorstDamage && bestTime == timeToShoot && fDistance < fMinDistance )
				{
					bCanShootNow = bCanShoot;
					pResult = pTarget;
					fWorstDamage = fDamage;
					bestTime = timeToShoot;
					fMinDistance = fDistance;
				}
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAIUnit* CShootEstimatorLighAA::GetBestUnit()
{
	return pResult;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*												CStandartBehaviour												*
//*******************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAIUnit* CStandartBehaviour::LookForTargetInFireRange( CCommonUnit *pUnit )
{
	return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CStandartBehaviour::ResetTime( CCommonUnit *pUnit )
{
	underFireAnalyzeTime = 0;
	lastTimeOfRotate = NTimer::STime(-1);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CStandartBehaviour::UponFire( class CCommonUnit *pUnit, class CAIUnit *pWho, class CAICommand *pCommand )
{
/*
	if ( IsValidObj( pWho ) )
	{
		SBehaviour &beh = pUnit->GetBehaviour();
		// если return fire
		if ( beh.fire == SBehaviour::EFReturn )
		{
			// если свободное блуждание или нет команд и не нужно держать позицию и в радиусе видимости
			if ( ( beh.moving == SBehaviour::EMRoaming ) || ( pCommand == 0 && beh.moving != SBehaviour::EMHoldPos )
					 && pWho->IsVisible( pUnit->GetParty() ) && pUnit->InVisSector( pWho) )
				theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_COMMAND_ATTACK_UNIT, pWho ), pUnit, true );
			// если follow the path и в радиусе огня
			if ( beh.moving == SBehaviour::EMFollow && pUnit->InFireRange( pWho ) )
				theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_COMMAND_ATTACK_UNIT, pWho ), pUnit, true );
		}
	}
*/
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CStandartBehaviour::TryToTraceEnemy( CAIUnit *pUnit )
{
	const int nParty = pUnit->GetParty();	
	if ( curTime - fleeTraceEnemyTime >= 3000 + NRandom::Random( 0, 1000 ) && pUnit->GetNGuns() > 0 )
	{
		fleeTraceEnemyTime = curTime;

		int nBestPiercing = -1;
		CBasicGun *pBestGun = 0;
		for ( int i = 0; i < pUnit->GetNGuns(); ++i )
		{
			CBasicGun *pGun = pUnit->GetGun( i );
			const int nPiercing = pGun->GetPiercing();
			if ( pGun->GetNAmmo() != 0 && pGun->GetTurret() != 0 && ( nPiercing > nBestPiercing || pBestGun == 0 ) )
			{
				nBestPiercing = nPiercing;
				pBestGun = pUnit->GetGun( i );
			}
		}

		if ( pBestGun == 0 )
			return false;

		const float r = pUnit->GetSightRadius();
		const float r2 = sqr( r );
		int nBestSides = -1;
		CAIUnit *pBestUnit = 0;
		const CVec2 vCenter = pUnit->GetCenterPlain();

		for ( CUnitsIter<1,3> iter( pUnit->GetParty(), EDI_ENEMY, pUnit->GetCenterPlain(), r ); !iter.IsFinished() && nBestSides < 3; iter.Iterate() )
		{
			CAIUnit *pEnemy = *iter;
			if ( !pEnemy->GetStats()->IsAviation() && pEnemy->IsVisible( nParty ) && fabs2( pEnemy->GetCenterPlain() - vCenter ) < r2 )
			{
				int nSides = 0;
				for ( int i = 0; i < 4; ++i )
				{
					if ( pBestGun->CanBreach( pEnemy, i ) )
						++nSides;
				}

				if ( nSides > nBestSides || pBestUnit == 0 )
				{
					nBestSides = nSides;
					pBestUnit = pEnemy;
				}
			}
		}

		if ( pBestUnit )
		{
			pBestGun->GetTurret()->TraceAim( pBestUnit, pBestGun );
			return true;
		}
		else
		{
			pBestGun->GetTurret()->StopTracing();			
			return false;
		}
	}
	else
	{
		for ( int i = 0; i < pUnit->GetNTurrets(); ++i )
		{
			CAIUnit *pTracedUnit = pUnit->GetTurret( i )->GetTracedUnit();
			if ( pTracedUnit && !pTracedUnit->IsVisible( nParty ) )
				pUnit->GetTurret( i )->StopTracing();
		}

		return false;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CStandartBehaviour::AnalyzeUnderFire( CAIUnit *pUnit )
{
	if ( curTime >= underFireAnalyzeTime )
	{
		if ( pUnit->IsOperable() && pUnit->GetNTurrets() != 0 )
		{
			if ( TryToTraceEnemy( pUnit ) )
				lastTimeOfRotate = curTime;
			else if ( pUnit->GetTurret( 0 )->IsFinished() )
			{
			 if ( ( theHitsStore.WasHit( pUnit->GetCenterPlain(), 2 * SConsts::RADIUS_OF_HIT_NOTIFY, CHitsStore::EHT_ANY ) ||
							lastTimeOfRotate != NTimer::STime( -1 ) && curTime - lastTimeOfRotate < 10000 + NRandom::Random( 0, 1000 ) ) )
				{
					// если танк с перебитой гусеницей или зенитка
					if ( pUnit->GetTurret( 0 )->GetHorTurnConstraint() != 0 && 
							( !pUnit->CanMove() && pUnit->GetStats()->IsArmor() || pUnit->GetStats()->etype == RPG_TYPE_ART_AAGUN ) 
						 )
					{
						if ( NRandom::Random( 0.0f, 1.0f ) > 0.7f )
						{
							CTurret *pTurret = pUnit->GetTurret( 0 );
							const WORD wTurretAngle = pTurret->GetHorCurAngle();
							
							WORD wRotateAngle;
							if ( pUnit->GetStats()->IsArmor() )
								wRotateAngle = Min( (WORD)15000, (WORD)pTurret->GetHorTurnConstraint() );
							else
								wRotateAngle = Min( (WORD)32768, (WORD)pTurret->GetHorTurnConstraint() );

							const WORD wMinRotateAngle = wRotateAngle / 4;
							const WORD wMaxRotateAngle = Max( DirsDifference( wTurretAngle, wRotateAngle ), DirsDifference( wTurretAngle, -wRotateAngle ) );
							
							if ( wMaxRotateAngle > 0 && (int)wMaxRotateAngle - (int)wMinRotateAngle + 1 != 0 )
							{
								WORD wRotate = NRandom::Random( wMinRotateAngle, wMaxRotateAngle );

								bool bClockWise = false;
								bool bCounterClockWise = false;

								if ( DirsDifference( wTurretAngle + wRotate, 0 ) <= wRotateAngle )
									bClockWise = true;
								if ( DirsDifference( wTurretAngle - wRotate, 0 ) <= wRotateAngle )
									bCounterClockWise = true;
								
								int nSign = 0;
								if ( bClockWise && !bCounterClockWise )
									nSign = 1;
								if ( !bClockWise && bCounterClockWise )
									nSign = -1;
								if ( bClockWise && bCounterClockWise )
								{
									if ( nLastSign == 1 )
										nSign = ( NRandom::Random( 0.0f, 1.0f ) >= 0.6f ) ? 1 : -1;
									else
										nSign = ( NRandom::Random( 0.0f, 1.0f ) >= 0.4f ) ? 1 : -1;
								}
								if ( !bClockWise && !bCounterClockWise )
								{
									wRotate = wTurretAngle;
									nSign = -1;
								}
								nLastSign = nSign;

								const WORD wResAngle = wTurretAngle + nSign * wRotate;
								pTurret->TurnHor( wResAngle );
							}
						}

						if ( theHitsStore.WasHit( pUnit->GetCenterPlain(), 2 * SConsts::RADIUS_OF_HIT_NOTIFY, CHitsStore::EHT_ANY ) )
							lastTimeOfRotate = curTime;
					}
				}
				else
					pUnit->GetTurret( 0 )->SetCanReturn();
			}
		}

		underFireAnalyzeTime = curTime + SConsts::TIME_OF_HIT_NOTIFY + NRandom::Random( 0, 1000 );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
