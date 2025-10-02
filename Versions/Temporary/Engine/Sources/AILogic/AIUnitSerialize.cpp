#include "stdafx.h"

#include "Soldier.h"
#include "Artillery.h"
#include "SerializeOwner.h"


int CAIUnit::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CCommonUnit*>(this) );
	saver.Add( 5, &timeToDeath );
	if ( !saver.IsChecksum() )
		saver.Add( 6, &player );
	saver.Add( 11, &fCamoflage );
	saver.Add( 12, &wVisionAngle );
	saver.Add( 15, &fHitPoints );
//	saver.Add( 16, &pPathUnit );
	saver.Add( 17, &pAntiArtillery );
	saver.Add( 22, &pTankPit );
	saver.Add( 23, &camouflateTime );
//	if ( !saver.IsChecksum() )
	//	saver.Add( 24, &bVisibleByPlayer );
	saver.Add( 25, &fTakenDamagePower );
	saver.Add( 26, &nGrenades );
	saver.Add( 27, &targetScanRandom );
	saver.Add( 32, &pUnitInfoForGeneral );
	
	saver.Add( 40, &bFreeEnemySearch );
	saver.Add( 41, &pAnimUnit );
	saver.Add( 42, &creationTime );
	
	if ( !saver.IsChecksum() )
	{
		saver.Add( 43, &bAlwaysVisible );
		saver.Add( 44, &bCountToDissapear );
		saver.Add( 46, &lastTimeOfVis );
		
		saver.Add( 47, &bRevealed );
		saver.Add( 48, &bQueredToReveal );
		saver.Add( 49, &nextRevealCheck );
		saver.Add( 50, &vPlaceOfReveal );
		saver.Add( 51, &visible4Party );
		if ( visible4Party.empty() )
		{
			visible4Party.resize( 3, false );
		}
		
		saver.Add( 52, &nVisIndexInUnits );
	}
	saver.Add( 53, &bTargetingTrack );
	saver.Add( 54, &bVirtualTankPit );
	saver.Add( 55, &eReinforcementType );
	saver.Add( 56, &bRestInside );
	SerializeOwner( 57, &pObjInside, &saver );
	saver.Add( 58, &pStatsModifiers );
	saver.Add( 59, &fCamoflage );
	saver.Add( 60, &pShootInMovementExecutor );
	saver.Add( 61, &dwForbiddenGuns );
	saver.Add( 62, &bTrampled );
	saver.Add( 63, &bIgnoreAABBCoeff );
	saver.Add( 64, &bHoldingSector );
	saver.Add( 65, &lastAckTime );
	saver.Add( 66, &nMultipleShots );

	if ( !saver.IsChecksum() )
	{
		saver.Add( 67, &timeOfReveal );
		saver.Add( 68, &timeLastAttackedAck );
	}

	saver.Add( 69, &unitProfile );
	//saver.Add( 70, &nAbilityLevel );
	saver.Add( 71, &targetsCache );
	saver.Add( 72, &lastScanTime );
	saver.Add( 73, &realScanDuration );
	saver.Add( 74, &bIsInTankPit );
	saver.Add( 75, &timeLastAttacked );

	return 0;
}

void CSoldier::OnSerialize( IBinSaver &saver )
{
	if ( !saver.IsReading() && !saver.IsChecksum() )
	{
		if ( IsFree() )
			pObjInside = 0;
	}
	SerializeOwner( 2, &pObjInside, &saver );
}

int CArtillery::operator&( IBinSaver &f )
{
	f.Add( 1, static_cast<CAIUnit*>(this) ); 
	f.Add(2,&pStats); f.Add(3,&nInitialPlayer); f.Add(4,&pGuns); f.Add(5,&turrets); 
	f.Add(6,&eCurInstallAction); f.Add(7,&eNextInstallAction); 
	f.Add(8,&eCurrentStateOfInstall); f.Add(9,&bInstalled); 
	f.Add(10,&installActionTime); f.Add(11,&bInstallActionInstant); f.Add(12,&pStaticPathToSend); 
	f.Add(13,&vShift); f.Add(14,&pIPathToSend); f.Add(15,&pCapturingUnit); 
	f.Add(16,&pCrew); f.Add(17,&fOperable); f.Add(18,&pSlaveTransport); 
	f.Add(19,&pHookingTransport); f.Add(20,&pBulletStorage); 
	
	if ( !f.IsChecksum() )
		f.Add(21,&bBulletStorageVisible); 

	f.Add(22,&lastCheckToInstall); f.Add(23,&behUpdateDuration);
	return 0;
}
#ifndef _FINALRELEASE
extern NTimer::STime curTime;
#endif

int CCommonUnit::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CLinkObject*>(this) );
	saver.Add( 2, static_cast<CGroupUnit*>(this) );
	saver.Add( 3, static_cast<CQueueUnit*>(this) );

	saver.Add( 4, &beh );
	saver.Add( 5, &lastBehTime );
	saver.Add( 7, &pLockingGun );
	saver.Add( 9, &wReserveDir );
	if ( !saver.IsChecksum() )
		saver.Add( 10, &bSelectable );

	saver.Add( 11, &fDesirableSpeed );
	saver.Add( 12, &pFollowedUnit );
	saver.Add( 14, &fMinFollowingSpeed );
	saver.Add( 15, &vFollowShift );
	saver.Add( 16, &pShootEstimator );
	saver.Add( 17, &pTruck );
	saver.Add( 18, &vBattlePos );

	saver.Add( 20, static_cast<CBasePathUnit*>(this) );

//	saver.Add( 22, &pScenarioUnit );
	saver.Add( 23, &bCanBeFrozenByState );
	saver.Add( 24, &bCanBeFrozenByScan );
	saver.Add( 25, &nextFreezeScan );
	saver.Add( 26, &fPrice );


	saver.Add( 27, &vOldPlacement );
	saver.Add( 28, &qStart );
	saver.Add( 29, &qFinish );

	int nSaverCRAP = 666;
	saver.Add( 30, &nSaverCRAP );
	if ( saver.IsReading() && nSaverCRAP != 666 )
	{
		vOldPlacement = GetCenter();
		qStart.FromAngleAxis( ToRadian( float( GetFrontDirection() ) / 65536.0f * 360.0f ), 0, 0, 1 );
		MakeOrientation( &qStart, V3_AXIS_Z );
		qFinish = qStart;
	}
	if ( saver.IsReading() )
		nCommandLeft = -1;
	return 0;
}


