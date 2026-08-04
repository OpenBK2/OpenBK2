#include "stdafx.h"

#include "Misc/bresenham.h"
#include "System/Time.h"
#include "Stats_B2_M1/AbilityActions.h"
#include "TechnicsStates.h"
#include "NewUpdater.h"
#include "GroupLogic.h"
#include "UnitCreation.h"
#include "StaticObjects.h"
#include "EntrenchmentCreation.h"
#include "UnitsIterators2.h"
#include "Artillery.h"
#include "SerializeOwner.h"
#include "Technics.h"
#include "Commands.h"
#include "Guns.h"
#include "PresizePath.h"
#include "Common_RTS_AI/PathFinder.h"
#include "Common_RTS_AI/StaticMapHeights.h"
#include "DebugTools/DebugInfoManager.h"


extern CDiplomacy theDipl;
extern CStaticObjects theStatObjs;
extern NTimer::STime curTime;
extern CUnitCreation theUnitCreation;
extern CGroupLogic theGroupLogic;
extern CEventUpdater updater;

//*******************************************************************
//*										CMechUnitEntrenchSelfState											*
//*******************************************************************

IUnitState* CTankPitLeaveState::Instance( class CAIUnit *pTank )
{
	return new CTankPitLeaveState( pTank );
}

CTankPitLeaveState::CTankPitLeaveState( class CAIUnit  *pTank )
: eState( TLTPS_ESTIMATING ), pUnit( pTank ), timeStartLeave( curTime )
{
}

void CTankPitLeaveState::Segment()
{
	switch( eState )
	{
		
	case TLTPS_ESTIMATING:
		updater.AddUpdate( 0, ACTION_NOTIFY_ENTRENCHMENT_STARTED, pUnit, 2 );
		if ( curTime - timeStartLeave > SConsts::AA_BEH_UPDATE_DURATION )
		{
			updater.AddUpdate( 0, ACTION_NOTIFY_START_LEAVE_PIT, pUnit, -1 );
			eState = TLTPS_MOVING;
		}

		break;
	case TLTPS_MOVING:
		pUnit->SetOffTankPit();
		pUnit->SetCommandFinished();

		CPtr<SAISpecialAbilityUpdate> pUpdate = new SAISpecialAbilityUpdate;
		pUpdate->info.nAbilityType = NDb::ABILITY_ENTRENCH_SELF;
		pUpdate->info.state = EASS_READY_TO_ON;
		pUpdate->info.fCurValue = 0.0f;
		pUpdate->info.nObjUniqueID = pUnit->GetUniqueId();
		updater.AddUpdate( pUpdate, ACTION_NOTIFY_SPECIAL_ABLITY, pUnit, -1 );

		break;
	}
}

ETryStateInterruptResult CTankPitLeaveState::TryInterruptState( class CAICommand *pCommand )
{
	return TSIR_YES_WAIT;
}

//*******************************************************************
//*										CMechUnitEntrenchSelfState*
//*******************************************************************

IUnitState* CMechUnitEntrenchSelfState::Instance( class CAIUnit * pUnit )
{
	return new CMechUnitEntrenchSelfState( pUnit );
}

CMechUnitEntrenchSelfState::CMechUnitEntrenchSelfState( class CAIUnit * _pUnit ) 
: CStatusUpdatesHelper( EUS_ENTRENCH, _pUnit ), pUnit( _pUnit ), eState( ESHD_ESTIMATE ), timeStartBuild( 0 ), fOldProgress( 0.0f )
{  
	if ( pUnit->IsInTankPit() )
	{
		eState = ESHD_END;
	}
	else if ( pUnit->GetStats()->IsArtillery() && pUnit->NeedDeinstall() && pUnit->IsUninstalled() )
	{
		eState = ESHD_WAIT_FOR_INSTALL;
		pUnit->Stop();
	}
	else
		pUnit->Stop();
}

bool CMechUnitEntrenchSelfState::CheckInfantry( const CAIUnit * pUnit, const SRect &rect ) const
{
	const CFormation *pFormation = 0;
	if ( pUnit->GetStats()->IsArtillery() )
		pFormation = checked_cast<const CArtillery*>(pUnit)->GetCrew();

	for ( CUnitsIter<0,1> iter( 0, ANY_PARTY, pUnit->GetCenterPlain(), rect.width + rect.lengthAhead + rect.lengthBack );
				!iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pSoldier = *iter;
	
		if ( pSoldier->IsFree() && pSoldier->GetStats()->IsInfantry() && rect.IsPointInside( pSoldier->GetCenterPlain() ) )
		{
			if ( pFormation != pSoldier->GetFormation() )
				return false;
		}
	}
	return true;
}

bool CMechUnitEntrenchSelfState::CheckTrenches( const CAIUnit * pUnit, const SRect &rectToTest ) const
{
	return !CEntrenchmentCreation::SearchTrenches( pUnit->GetCenterPlain(), rectToTest );
}

struct SVectorHash
{
	int operator()( const SVector & v) const
	{
		return (v.x * 65535) + v.y;
	}
};
typedef det_map<SVector, bool, SVectorHash> CSVectorHash;

void CMechUnitEntrenchSelfState::GetTilesNextToRect( const SRect &rect, const WORD wDirExclude )
{
	// криво! как-то нужно подумать и написать получше
	std::list<SVector> tilesUnderRect;
	GetAIMap()->GetTilesCoveredByRect( rect, &tilesUnderRect );

	CSVectorHash tilesUnderRect1;
	for ( std::list<SVector>::iterator i = tilesUnderRect.begin(); i != tilesUnderRect.end(); ++i )
		tilesUnderRect1[*i] = true;

	CSVectorHash adjustedTiles;
	for ( std::list<SVector>::iterator i = tilesUnderRect.begin(); i != tilesUnderRect.end(); ++i )
	{
		if ( tilesUnderRect1.find( SVector(i->x+1, i->y) ) == tilesUnderRect1.end() )
			adjustedTiles[ SVector(i->x+1, i->y) ];

		if ( tilesUnderRect1.find( SVector(i->x-1, i->y) ) == tilesUnderRect1.end() )
			adjustedTiles[ SVector(i->x-1, i->y) ];

		if ( tilesUnderRect1.find( SVector(i->x, i->y+1) ) == tilesUnderRect1.end() )
			adjustedTiles[ SVector(i->x, i->y+1) ];

		if ( tilesUnderRect1.find( SVector(i->x, i->y-1) ) == tilesUnderRect1.end() )
			adjustedTiles[ SVector(i->x, i->y-1) ];
	}

	for ( CSVectorHash::iterator i = adjustedTiles.begin(); i != adjustedTiles.end(); ++i )
		tiles.push_back( i->first );

	for ( std::list<SObjTileInfo>::iterator it = tiles.begin(); it != tiles.end(); )
	{
		const CVec2 vec = GetAIMap()->GetPointByTile( *it );
		const WORD wDirToTile = GetDirectionByVector( vec - rect.center );
		if ( DirsDifference( wDirToTile, wDirExclude ) < (65535 / 8) )
			it = tiles.erase( it );
		else 
			++it;
	}
}

void CMechUnitEntrenchSelfState::Segment()
{
	switch( eState )
	{
	case ESHD_END:
		pUnit->SetCommandFinished();

		break;
	case ESHD_WAIT_FOR_INSTALL:
		if ( !pUnit->IsOperable() )
			eState = ESHD_END;
		else if ( pUnit->IsInstalled() )
			eState = ESHD_ESTIMATE;

		break;
	case ESHD_ESTIMATE:
		{
			pUnit->UnlockTiles();
			// determine weather we can build tank pit from sand bags or can hull doun into ground
			const SRect unitRect = pUnit->GetUnitRect();
			std::list<SVector> unitTiles;
			GetAIMap()->GetTilesCoveredByRect( unitRect, &unitTiles );

			bool bCanDig = true;
			for ( std::list<SVector>::iterator i = unitTiles.begin(); i != unitTiles.end(); ++i )
			{
				if ( !GetTerrain()->CanDigEntrenchment( i->x, i->y ) )
				{
					bCanDig = false;
					break;
				}
			}

			float fResize;
			const SMechUnitRPGStats * pUnitStats = checked_cast<const SMechUnitRPGStats*>( pUnit->GetStats() );
			pStats = theUnitCreation.GetRandomTankPit( pUnitStats->vAABBHalfSize, bCanDig, &fResize );

			const CVec2 vRelativePosUnit(  pUnitStats->vAABBHalfSize.y + pUnitStats->vAABBCenter.y, pUnitStats->vAABBCenter.x );
			const CVec2 vRelativePosPit( fResize * ( pStats->vAABBHalfSize.y + pStats->vAABBCenter.y), fResize * (pStats->vAABBCenter.x) );
			vTankPitCenter = pUnit->GetCenterPlain() + ( ( vRelativePosUnit - vRelativePosPit )^pUnit->GetDirectionVector() );

			vHalfSize.x = pUnitStats->vAABBHalfSize.x ;
			vHalfSize.y = pUnitStats->vAABBHalfSize.y ;

			SRect rect;
			SRect rectToCheck;
			const CVec2 vFrontDir( GetVectorByDirection(pUnit->GetFrontDirection()) );
			rect.InitRect( pUnit->GetCenterPlain(), vFrontDir, vHalfSize.y, vHalfSize.x );
			rectToCheck.InitRect( pUnit->GetCenterPlain(), vFrontDir, vHalfSize.y + SConsts::TILE_SIZE, vHalfSize.x + SConsts::TILE_SIZE );
			
			GetTilesNextToRect( rect, 65535 / 2 + pUnit->GetFrontDirection() );
			
			// проверить, не залоканы ли тайлы под TankPit
			bool bCanAdd = true;
			for ( std::list<SObjTileInfo> ::iterator i = tiles.begin(); i != tiles.end(); ++i )
			{
				if ( GetTerrain()->IsLocked( i->tile, EAC_TERRAIN ) )
				{
					bCanAdd = false;
					break;
				}
			}
			
			if ( bCanAdd ) // проверить, нет ли под TankPit окопов
				bCanAdd = CheckTrenches( pUnit, rectToCheck );
			if ( bCanAdd )
				bCanAdd = CheckInfantry( pUnit, rectToCheck ); 

			if ( bCanAdd )
			{
				//lock tiles where tankpit will be build
				//
				GetTerrain()->AddStaticObjectTiles( tiles );
				eState = ESHD_START_BUILD;
				updater.AddUpdate( 0, ACTION_NOTIFY_ENTRENCHMENT_STARTED, pUnit, -1 );
				InitStatus();
			}
			else
			{
				pUnit->SendAcknowledgement( ACK_NEGATIVE );
				pUnit->SetCommandFinished();
			}
			float fCoeff = 1.0f;
			int nDefaultTime = SConsts::ENTRENCH_SELF_TIME;
			for ( int i = 0; i < (std::min<int>) ( pUnit->GetStats()->GetActions()->specialAbilities.size(), pUnit->GetAbilityLevel() ); ++i )
			{
				const int nAbility = pUnit->GetStats()->GetActions()->specialAbilities[i]->eName;
				if ( nAbility == NDb::ABILITY_MOBILE_FORTRESS ) 
				{
					const NDb::SUnitSpecialAblityDesc *pSA = pUnit->GetStats()->GetActions()->specialAbilities[i];
					NI_ASSERT( pSA, "Ability desc (Mobile Fortress) not found");
					if ( pSA )
						fCoeff = pSA->fParameter;
				}
				else if ( nAbility == NDb::ABILITY_ENTRENCH_SELF ) 
				{
					const NDb::SUnitSpecialAblityDesc *pSA = pUnit->GetStats()->GetActions()->specialAbilities[i];
					NI_ASSERT( pSA, "Ability desc (Entrench Self) not found");
					if ( pSA )
						nDefaultTime = pSA->nSwitchOnTime;
				}
			}
			timeEndBuild = curTime + fCoeff * nDefaultTime;
			timeStartBuild = curTime;
		}

		break;
	case ESHD_START_BUILD:
		if ( curTime > timeEndBuild )
		{
			updater.AddUpdate( 0, ACTION_NOTIFY_START_BUILD_PIT, pUnit, -1 );
			eState = ESHD_BUILD_PIT;

			CPtr<SAISpecialAbilityUpdate> pUpdate = new SAISpecialAbilityUpdate;
			pUpdate->info.nAbilityType = NDb::ABILITY_ENTRENCH_SELF;
			pUpdate->info.state = EASS_ACTIVE;
			pUpdate->info.fCurValue = 0.0f;
			pUpdate->info.nObjUniqueID = pUnit->GetUniqueId();
			updater.AddUpdate( pUpdate, ACTION_NOTIFY_SPECIAL_ABLITY, pUnit, -1 );
			fOldProgress = -1.0f;
		}
		else
		{
			float fNewValue = float( curTime - timeStartBuild ) / ( timeEndBuild - timeStartBuild );

			if ( fabs( fOldProgress - fNewValue ) > 0.1f )
			{
				CPtr<SAISpecialAbilityUpdate> pUpdate = new SAISpecialAbilityUpdate;
				pUpdate->info.nAbilityType = NDb::ABILITY_ENTRENCH_SELF;
				pUpdate->info.state = EASS_SWITCHING_ON;
				pUpdate->info.fCurValue = fNewValue;
				pUpdate->info.nObjUniqueID = pUnit->GetUniqueId();
				updater.AddUpdate( pUpdate, ACTION_NOTIFY_SPECIAL_ABLITY, pUnit, -1 );

				fOldProgress = fNewValue;
			}
		}

		break;
	case ESHD_BUILD_PIT:
		{
			GetTerrain()->RemoveStaticObjectTiles( tiles );

			pUnit->SetInTankPit( theStatObjs.AddNewTankPit( pStats, CVec3(vTankPitCenter,0.0f), pUnit->GetFrontDirection(), 0, vHalfSize, tiles, pUnit ) );
			pUnit->SendAcknowledgement( ACK_BUILDING_FINISHED, true );
		
			pUnit->SetCommandFinished();
			updater.AddUpdate( 0, ACTION_NOTIFY_ENTRENCHMENT_STARTED, pUnit, 1 );
		}

		break;
	}
}

ETryStateInterruptResult CMechUnitEntrenchSelfState::TryInterruptState( class CAICommand *pCommand )
{
	if ( eState != ESHD_ESTIMATE )		//  UNLOCK LoCKED TILES
		GetTerrain()->RemoveStaticObjectTiles( tiles );

	if ( pUnit && pUnit->IsAlive() )
	{
		pUnit->SetCommandFinished();

		CPtr<SAISpecialAbilityUpdate> pUpdate = new SAISpecialAbilityUpdate;
		pUpdate->info.nAbilityType = NDb::ABILITY_ENTRENCH_SELF;
		pUpdate->info.state = EASS_READY_TO_ON;
		pUpdate->info.fCurValue = -1.0f;
		pUpdate->info.nObjUniqueID = pUnit->GetUniqueId();
		updater.AddUpdate( pUpdate, ACTION_NOTIFY_SPECIAL_ABLITY, pUnit, -1 );
	}

	return TSIR_YES_IMMIDIATELY;
}

IUnitState* CSoldierEnterHoldSectorState::Instance( class CAIUnit * pUnit )
{
	return new CSoldierEnterHoldSectorState( pUnit );
}

CSoldierEnterHoldSectorState::CSoldierEnterHoldSectorState( class CAIUnit *_pUnit ) 
: pUnit( _pUnit )
{
	nTimeStart = curTime;
}

void CSoldierEnterHoldSectorState::Segment()
{
	if ( nTimeStart + SConsts::TIME_TO_ENTER_HOLD_SECTOR < curTime )
	{
		pUnit->SetHoldSector();
		pUnit->SetCommandFinished();
	}
	else
	{
		CPtr< SAISpecialAbilityUpdate > pUpdate = new SAISpecialAbilityUpdate;
		pUpdate->info.nAbilityType = NDb::ABILITY_HOLD_SECTOR;
		pUpdate->info.state = EASS_SWITCHING_ON;
		pUpdate->info.nObjUniqueID = pUnit->GetUniqueId();
		pUpdate->info.fCurValue = ( curTime - nTimeStart ) / SConsts::TIME_TO_ENTER_HOLD_SECTOR;
		updater.AddUpdate( pUpdate, ACTION_NOTIFY_SPECIAL_ABLITY, pUnit, -1 );
	}
}

ETryStateInterruptResult CSoldierEnterHoldSectorState::TryInterruptState( class CAICommand *pCommand )
{
	if ( pUnit->IsRefValid() && pUnit->IsAlive() )
	{
		//reset ui
	}
	return TSIR_YES_IMMIDIATELY;
}

CMechUnitInsideMechUnitState::CMechUnitInsideMechUnitState( class CAIUnit *_pUnit, class CMilitaryCar *_pTransport, const bool bImmidiate ) 
: pUnit( _pUnit ), pTransport( _pTransport ), timeNextCheck( curTime ), vEntrancePoint( VNULL2 ), eState( EMEM_APPROACHING ),
vDestination( VNULL2 )
{
	if ( !pTransport->CanBoard( pUnit ) )
	{
		//CRAP{
		// send acknowledgement about cannot board
		//CRAP}
		TryInterruptState( 0 );
	}
	else
	{
		pTransport->AddBoardingMechUnit( pUnit );
		if ( bImmidiate )
			BoardNow();
		else
		{
			// will start to move to unit
			// when approach the unit closely, state is finished
			SendToEntrance();
			eState = EMEM_APPROACHING;
		}
	}
}

void CMechUnitInsideMechUnitState::BoardNow()
{
	eState = EMEM_REST_INSIDE;
	pTransport->SetOnBoard( pUnit, true );
	pPath = checked_cast<CMechUnitRestOnBoardPath*>( pUnit->GetSmoothPath() );
	pUnit->SetSelectable( false, true );
	updater.AddUpdate( 0, ACTION_NOTIFY_ENTRANCE_STATE, pUnit, -1 );
	pUnit->SetRestInside( true, pTransport );
	pUnit->SetGoForward( true );
	pTransport->RemoveBoardingMechUnit( pUnit );
}

void CMechUnitInsideMechUnitState::SendToEntrance()
{
		vEntrancePoint = pTransport->GetEntrancePoint();
		CPtr<IStaticPath> pPath = CreateStaticPathToPoint( vEntrancePoint, VNULL2, pUnit, true, GetAIMap() );
		if ( pPath != 0 )
			pUnit->SendAlongPath( pPath, VNULL2, true );
		else
		{
			//CRAP{
			// send acknowledgement about cannot find path
			//CRAP}
			TryInterruptState( 0 );
		}
}

void CMechUnitInsideMechUnitState::AttackTarget( CAIUnit * _pEnemy )
{
	eState = EMEM_ATTACKING;
	pEnemy = _pEnemy;
}

void CMechUnitInsideMechUnitState::Unload( const CVec2 &vPoint )
{
	eState = EMEM_EXITTING;
	timeNextCheck = curTime;
	vDestination = vPoint;
}

bool CMechUnitInsideMechUnitState::WillUnitIntersectOtherUnits( CAIUnit * pUnit, const CVec2 &vCenter, const CVec2 &vDir )
{
	// check if unit rect intersect with any oter unit rect if being placed to vCenter with wDir
	const CVec2 &vAABBHalfSize = pUnit->GetStats()->vAABBHalfSize;
	const CVec2 &vAABBCenter = pUnit->GetStats()->vAABBCenter;
	const float length = vAABBHalfSize.y * SConsts::BOUND_RECT_FACTOR;
	const float width = vAABBHalfSize.x * SConsts::BOUND_RECT_FACTOR;

	SRect unitRect;

	const CVec2 dirPerp( vDir.y, -vDir.x );
	const CVec2 vCenterShift( vDir * vAABBCenter.y + dirPerp * vAABBCenter.x );
	unitRect.InitRect( vCenter + vCenterShift, vDir, length, width );


	const float fRadius = SConsts::TILE_SIZE * 10;
	for ( CUnitsIter<0,2> iter( 0, ANY_PARTY, vCenter, fRadius ); !iter.IsFinished(); iter.Iterate() )
	{
		if ( (*iter) != pUnit && !(*iter)->GetStats()->IsInfantry() && !(*iter)->GetStats()->IsAviation() )
		{
			const SRect rect = (*iter)->GetUnitRect();
			if ( unitRect.IsIntersected( rect ) )
				return true;
		}
	}
	return false;
}

void CMechUnitInsideMechUnitState::Segment()
{
	if ( !IsValidObj( pTransport ) )
	{
		TryInterruptState( 0 );
		return;
	}

	switch( eState )
	{
	case EMEM_APPROACHING:
		if ( fabs2( pUnit->GetCenterTile() - pTransport->GetEntrancePoint() ) < sqr( pUnit->GetBoundTileRadius() * SConsts::TILE_SIZE) ) 
		{ // if unit is near entrance
			// board now, set state "on board"
			BoardNow();
		}
		else if ( !pTransport->CanBoard( pUnit ) ) // cannot board 
			TryInterruptState( 0 );
		else if ( timeNextCheck > curTime && pTransport->GetEntrancePoint() != vEntrancePoint ) // transport moved
		{
			timeNextCheck = curTime + CHECK_PERIOID;
			SendToEntrance();
		}
		else if ( pUnit->IsIdle() ) // unit idle - resent again
			SendToEntrance();

		break;
	case EMEM_REST_INSIDE:

		pUnit->AnalyzeTargetScan( 0, false, true );
		break;
	case EMEM_ATTACKING:
		if ( IsValidObj( pEnemy ) )
		{
			pUnit->ResetShootEstimator( pEnemy, false, pUnit->GetForbiddenGuns() );
			CBasicGun * pGun = pUnit->GetBestShootEstimatedGun();
			if ( pGun )
			{
				if ( !pGun->IsFiring() && pGun->GetRestTimeOfRelax() == 0 )
					pGun->StartEnemyBurst( pEnemy, true );
			}
			else
				eState = EMEM_REST_INSIDE;
		}
		else
			eState = EMEM_REST_INSIDE;

		break;
	case EMEM_EXITTING:
		// wait untill unit can be placed on unlocked position
		// or choose unlocked position
		if ( timeNextCheck <= curTime )
		{
			// check while drop point is free from units
			const CVec2 vEntrancePoint( pTransport->GetEntrancePoint() );
			const SVector tile( AICellsTiles::GetTile( vEntrancePoint ) );
			const CVec2 vTransportCenter( pTransport->GetCenterPlain() );

			CVec2 vDir( vEntrancePoint - vTransportCenter );
			Normalize( &vDir );
			
			CVec2 vDropPoint = vEntrancePoint;
			
			if ( GetTerrain()->CanUnitGo( pUnit->GetBoundTileRadius(), tile, pUnit->GetAIPassabilityClass() ) == FREE_TERRAIN  &&
				   !WillUnitIntersectOtherUnits( pUnit, vEntrancePoint, vDir ) )
			{
				// drop unit, have him to move a bit from drop point
				SendToDestination( vDropPoint, vDir );
				TryInterruptState( 0 );
			}
			// check if unit can land in some radius from drop point
			else if ( FindAllowedDropPoint( pUnit, &vDropPoint ) )
			{
				SendToDestination( vDropPoint, vDir );
				TryInterruptState( 0 );
			}
			timeNextCheck = curTime + NRandom::Random( 100, 1000 ); RecordRandomCall();
		}
		break;
	}
	if ( pPath )
		pPath->Advance();
}

void CMechUnitInsideMechUnitState::SendToDestination( const CVec2 &vDropPoint, const CVec2 &vDir )
{
	pTransport->SetOnBoard( pUnit, false );
	pPath = 0;
	pUnit->SetSelectable( theDipl.GetMyNumber() == pUnit->GetPlayer(), true );
	updater.AddUpdate( 0, ACTION_NOTIFY_ENTRANCE_STATE, pUnit, -1 );
	pUnit->SetRestInside( false, 0 );
	pUnit->Stop();
	pUnit->SetCenter( GetHeights()->Get3DPoint( vDropPoint ) );

	CVec2 vSendTo = vDropPoint + 200 * vDir;

	if ( vDestination != VNULL2 )
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_MOVE_TO, vDestination ), pUnit, true );

	//if ( vDestination == VNULL2 || fabs2( vDropPoint - vSendTo ) < fabs2( vDropPoint - vDestination ) )
	theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_MOVE_TO, vSendTo ), pUnit, true );
}

bool CMechUnitInsideMechUnitState::FindAllowedDropPoint( CAIUnit *pUnit, CVec2 *vDropPoint )
{
	// search by radius
	WORD wStartDir = NRandom::Random( 0, 65535 ); RecordRandomCall();
	CVec2 vCenterPoint = *vDropPoint;
	const int nIterations = 8;
	for ( float fRadius = 150; fRadius < 3000; fRadius += 50 )
	{
		for ( int i = 0; i < nIterations; ++i )
		{
			WORD wDir = wStartDir +(65535 / nIterations * i);
			const CVec2 vDir = GetVectorByDirection( wDir );
			const CVec2 vPoint(  vCenterPoint + vDir * fRadius );
			const SVector vTile( AICellsTiles::GetTile( vPoint ) );

#ifndef _FINALRELEASE
			if ( NGlobal::GetVar( "unload_tries_markers", 0 ) )
			{
				CSegment segm;
				segm.p1 = CVec2( vPoint.x + 10, vPoint.y + 10 );
				segm.p2 = CVec2( vPoint.x - 10, vPoint.y - 10 );
				segm.dir = segm.p2 - segm.p1;
				DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );
				segm.p1 = CVec2( vPoint.x + 10, vPoint.y - 10 );
				segm.p2 = CVec2( vPoint.x - 10, vPoint.y + 10 );
				segm.dir = segm.p2 - segm.p1;
				DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );
			}
#endif

			if ( GetTerrain()->CanUnitGo( pUnit->GetBoundTileRadius(), vTile, pUnit->GetAIPassabilityClass() ) == FREE_TERRAIN )
			{
				if ( !WillUnitIntersectOtherUnits( pUnit, vPoint, vDir ) )
				{
					*vDropPoint = vPoint;
					return true;
				}
			}
		}
	}
	return false;
}

ETryStateInterruptResult CMechUnitInsideMechUnitState::TryInterruptState( class CAICommand *pCommand )
{
	if ( pCommand && pCommand->ToUnitCmd().nCmdType == ACTION_COMMAND_UNLOAD )
	{
		Unload( pUnit->GetCenterPlain() );
		return TSIR_YES_MANAGED_ALREADY;
	}
	if ( pUnit->GetCurCmd() != 0 )
		pUnit->GetCurCmd()->ToUnitCmd().fNumber = 1;

	pUnit->SetCommandFinished();
	if ( pCommand && pCommand->ToUnitCmd().nCmdType != ACTION_MOVE_ONBOARD_ATTACK_UNIT )
		pUnit->SetRestInside( false, 0 );
	if ( IsValidObj( pTransport ) && pUnit->IsAlive() )
		pTransport->RemoveBoardingMechUnit( pUnit );
	return TSIR_YES_IMMIDIATELY;
}

void CMechUnitInsideMechUnitState::OnSerialize( IBinSaver &saver )
{
	SerializeOwner( 2, &pUnit, &saver );
}

REGISTER_SAVELOAD_CLASS( 0x11123380, CMechUnitInsideMechUnitState );
REGISTER_SAVELOAD_CLASS( 0x1108D4E5, CMechUnitEntrenchSelfState );
REGISTER_SAVELOAD_CLASS( 0x1108D4C2, CTankPitLeaveState );

