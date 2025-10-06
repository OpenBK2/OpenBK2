#include "stdafx.h"
#include "./playerreinforcement.h"
#include "SingleReinforcement.h"
#include "NewUpdater.h"
#include "ScenarioTracker.h"
#include "Diplomacy.h"
#include "GroupLogic.h"
#include "CommonUnit.h"
#include "Stats_B2_M1/ReinfUpdates.h"
#include "Common_RTS_AI/AIMap.h"
#include "DBAIConsts.h"
#include "AILogicInternal.h"
#include "System/RandomGen.h"

extern NTimer::STime curTime;
extern CEventUpdater updater;
CPlayerReinforcementArray theReinfArray;
extern NTimer::STime curTime;
extern CDiplomacy theDipl;
extern CGroupLogic theGroupLogic;

CPlayerReinforcement::CPlayerReinforcement() 
: bReinfButtonEnabled( false ), timeReinfButtonEnable( curTime ), nMapReinforcementBonus( 0 ), fRecycleTimeCoeff( 1.0f ),
timeToCall( 0 ), fStoredProgress( 0.0f ), timeReinfIncrease( 0 ), superWeaponType( NDb::SUPER_WEAPON_BOMBER ), nSuperWeaponShots( -1 ),
timeSuperWeaponRecycleTime( 0 ), timeSuperWeaponFlyTime( 0 ), nSuperWeaponShotsLeft( -1 ), vSuperWeaponCallPoint( VNULL2 ),
timeSuperWeaponFire( 0 ), superWeaponState( SWS_WAIT_FOR_CALL )
{
	recycleTimes.clear();
}

void CPlayerReinforcement::InitPlayerReinforcement( int _nPlayer, const NDb::SMapInfo * pMapInfo, const NDb::SAIGameConsts *pConsts )
{
	if ( !GetScenarioTracker() )
		return;

	nPlayer = _nPlayer;
	recycleTimes.clear();
	for ( int i = 0; i < pConsts->reinforcementRecycleTime.size(); ++i )
		recycleTimes[pConsts->reinforcementRecycleTime[i].eType] = pConsts->reinforcementRecycleTime[i].nTime * 1000;

	IAIScenarioTracker *pTracker = GetScenarioTracker();
	for ( int i = 0; i < NDb::_RT_NONE; ++i )
	{
		const NDb::SReinforcement * pReinf = pTracker->GetReinforcement( nPlayer, (NDb::EReinforcementType)i );
		if ( pReinf )
			AddReinforcement( pReinf );
	}

	// send human player reinforcement to client
	if ( theDipl.GetMyNumber() == nPlayer )
	{
		for ( CInfos::const_iterator it = reinforcementInfos.begin();  it != reinforcementInfos.end(); ++it )
		{
			CPtr<SAIAvailableReinfUpdate> pUpdate = new SAIAvailableReinfUpdate;
			pUpdate->bEnabled = true;
			pUpdate->pReinf = it->second;
			pUpdate->nReinforcementCallsLeft = pTracker->GetReinforcementCallsLeft( nPlayer );
			updater.AddUpdate( pUpdate, ACTION_NOTIFY_AVAIL_REINF, 0, -1 );		
		}
	}

	nMapReinforcementBonus = pMapInfo->players[nPlayer].nReinforcementCalls;
	fRecycleTimeCoeff = (std::max)( 0.0f, pMapInfo->players[nPlayer].fRecycleTimeCoefficient );
	// reinforcement points
	for ( int i = 0; i < pMapInfo->players[nPlayer].reinforcementPoints.size(); ++i )
		AddPosition( pMapInfo->players[nPlayer].reinforcementPoints[i], i, pMapInfo->players[nPlayer].reinforcementPoints[i].bIsDefault );

	// Fill unit availability array
	unitAvailability.SetSizes( NDb::DB_RPG_TYPE_COUNT, NDb::_RT_NONE );
	unitAvailability.FillZero();
	for ( CInfos::iterator it = reinforcementInfos.begin(); it != reinforcementInfos.end(); ++it )
	{
		if ( !it->second )
			continue;

		const NDb::EReinforcementType eType = it->second->eType;
		if ( eType == NDb::RT_SUPER_WEAPON )
		{
			superWeaponType = pMapInfo->players[nPlayer].superWeapon.eSuperWeaponType;
			nSuperWeaponShots = pMapInfo->players[nPlayer].superWeapon.nCount;
			timeSuperWeaponRecycleTime = pMapInfo->players[nPlayer].superWeapon.fRecycleTime * 1000;
			timeSuperWeaponFlyTime =  pMapInfo->players[nPlayer].superWeapon.fFlyTime * 1000;
		}

		for ( int j = 0; j < it->second->entries.size(); ++j )
		{
			const NDb::SReinforcementEntry &entry = it->second->entries[j];
			//if ( it->first >= NDb::_RT_NONE )
				//continue;

			if ( entry.pMechUnit )
			{
				unitAvailability[it->first][entry.pMechUnit->eDBtype] += 1;
			}
			else if ( entry.pSquad )
			{
				NDb::EReinforcementType eRType = NDb::_RT_NONE;
				switch ( entry.pSquad->eSquadType ) 
				{
				case NDb::Assault_Squad :
					eRType = NDb::RT_ASSAULT_INFANTRY;
					break;
				case NDb::Special_Squad :
					eRType = NDb::RT_PARATROOPS;
					break;
				case NDb::SQUAD_TYPE_UNKNOWN :
					break;
				default:
					eRType = NDb::RT_MAIN_INFANTRY;
				}

				if ( eRType == NDb::_RT_NONE )
					continue;

				for ( int k = 0; k < entry.pSquad->members.size(); ++k )
					unitAvailability[eRType][entry.pSquad->members[k]->eDBtype] += 1;
			}
		}
	}

	if ( pTracker->GetGameType() == IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL )
	{
		fRecycleTimeCoeff = pTracker->GetRecycleSpeedCoeff( pTracker->GetPlayerSide( nPlayer ) );
		timeReinfIncrease = curTime + 1000.0f * fRecycleTimeCoeff;
		pTracker->SetReinforcementXP( nPlayer, NDb::_RT_NONE, ( timeReinfIncrease - curTime ) / 1000.0f );
	}
}

void CPlayerReinforcement::ModifyReinforcement( const NDb::SReinforcement *pReinforcement )
{
	CInfos::iterator pos = reinforcementInfos.find( pReinforcement->eType );
	if ( pos != reinforcementInfos.end() )
		pos->second = pReinforcement;
}

void CPlayerReinforcement::AddReinforcement( const NDb::SReinforcement *pReinforcement )
{
	reinforcementInfos[pReinforcement->eType] = pReinforcement;
}

void CPlayerReinforcement::AddPosition( const NDb::SReinforcementPosition &point, int nID, bool bEnabled )
{
	positions[nID].first = point;
	positions[nID].second = bEnabled;
	if ( bEnabled )
		UpdateAddPosition( nID, point );
}

void CPlayerReinforcement::EnablePosition( int nPositionID, bool bEnable )
{
	CPositions::iterator posPositions = positions.find( nPositionID );
	if ( posPositions != positions.end() )
	{
		posPositions->second.second = bEnable;
		if ( !bEnable )
			UpdateDeletePosition( nPositionID );
		else
			UpdateAddPosition( nPositionID, posPositions->second.first );
	}
}

const NDb::SDeployTemplate * CPlayerReinforcement::GetDeployTemplate( const NDb::SReinforcementPosition &positionToDeploy, NDb::EReinforcementType eType ) 
{
	// first - use default
	CDBPtr<NDb::SDeployTemplate> pTemplate =	positionToDeploy.pTemplate;
	
	// choose from consts
	const NDb::SAIGameConsts * pConsts = static_cast<CAILogic*>( Singleton<IAILogic>() )->GetAIConsts();
	for ( int i = 0; i < pConsts->typedTemplates.size(); ++i )
	{
		if ( eType == pConsts->typedTemplates[i].eType )
		{
			NI_ASSERT( pConsts->typedTemplates[i].pTemplate != 0, StrFmt( "Map Design Error: typed deploy template for reinforcement type %i is NULL", eType ) );
			if ( pConsts->typedTemplates[i].pTemplate )
			{
				pTemplate = pConsts->typedTemplates[i].pTemplate;
				break;
			}
		}
	}
	// choose from map (overwrite)
	for ( int i = 0; i < positionToDeploy.typedTemplates.size(); ++i )
	{
		if ( eType == positionToDeploy.typedTemplates[i].eType )
		{
			NI_ASSERT( positionToDeploy.typedTemplates[i].pTemplate != 0, StrFmt( "Map Design Error: typed deploy template for reinforcement type %i is NULL", eType ) );
			if ( positionToDeploy.typedTemplates[i].pTemplate )
			{
				pTemplate = positionToDeploy.typedTemplates[i].pTemplate;
				break;
			}
		}
	}
	return pTemplate;
}

void CPlayerReinforcement::AddCallReinforcementCommand( NDb::EReinforcementType eType, const CVec2 &vPoint )
{
	commands.push_back( SCallReinforcementCommand(eType, vPoint ) );
}

void CPlayerReinforcement::SendReinforcementToPoint( std::list< std::pair<int, CObjectBase*> > &objects, const NDb::EReinforcementType eType, const CVec2 &vPoint, const bool bIsParatroops, const float fCmdParam )
{
	std::vector<int> ids;
	for ( std::list< std::pair<int, CObjectBase*> >::iterator it = objects.begin(); it != objects.end(); ++it )
	{
		CCommonUnit * pUnit = checked_cast<CCommonUnit*>( it->second );
		if ( pUnit->IsEmptyCmdQueue() )
			ids.push_back( checked_cast<CLinkObject*>( it->second )->GetUniqueId() );
	}
	const int nGroup = theGroupLogic.GenerateGroupNumber();
	theGroupLogic.RegisterGroup( ids, nGroup );

	SAIUnitCmd cmd;
	if ( bIsParatroops )
	{
		cmd.nCmdType = ACTION_COMMAND_UNLOAD;
		cmd.fNumber = float(int(ALP_POSITION_VALID));
		cmd.vPos = vPoint;
		theGroupLogic.GroupCommand( cmd, nGroup, false );
	}
	else
	{
		cmd.nCmdType = ACTION_COMMAND_MOVE_TO;
		cmd.vPos = vPoint;
		cmd.fNumber = fCmdParam;
		theGroupLogic.GroupCommand( cmd, nGroup, false );

		cmd.nCmdType = ACTION_COMMAND_SWARM_TO;
		cmd.vPos = vPoint;
		cmd.fNumber = fCmdParam;
		theGroupLogic.GroupCommand( cmd, nGroup, false );
	}
	theGroupLogic.UnregisterGroup( nGroup );
	Singleton<IAILogic>()->SetNeedNewGroupNumber();
}

void CPlayerReinforcement::CallReinforcement( NDb::EReinforcementType eType, const CVec2 &vPoint, int nScriptID )
{
	// find nearest reinforcement point
	int nPointID = -1;
	CInfos::iterator pos = reinforcementInfos.find( eType );
	const NDb::SReinforcement *pReinf = pos == reinforcementInfos.end() ? 0 : pos->second;
	
	float fMinRange = 10000000;
	bool bOnWater = false;
	bool bAviation = pReinf && pReinf->IsAviation();
	for ( CPositions::const_iterator it = positions.begin(); it != positions.end(); ++it )
	{
		const float fRange = fabs( ( bAviation ? it->second.first.vAviationPosition : it->second.first.vPosition ) - vPoint );
		if ( it->second.second && fMinRange > fRange )
		{
			fMinRange = fRange;
			nPointID = it->first;
			SVector vTile = GetAIMap()->GetTile( it->second.first.vPosition );
			if ( GetTerrain()->GetTerrainType( vTile ) == ETT_WATER_TERRAIN )
				bOnWater = true;
			else
				bOnWater = false;
		}
	}

	if ( nPointID == -1 )
	{
		CONSOLE_BUFFER_LOG( CONSOLE_STREAM_CONSOLE, "No point enabled" );
	}
	else
	{
		std::list< std::pair<int, CObjectBase*> > objects;
		CallReinforcement( eType, nPointID, nScriptID, &objects, bOnWater, vPoint );
		if ( bOnWater && !bAviation )
			return;
		SendReinforcementToPoint( objects, eType, vPoint, ( eType == NDb::RT_ELITE_INFANTRY || eType == NDb::RT_PARATROOPS ) && pReinf->HasPlanes(), 0.0f );
	}
}

void CPlayerReinforcement::UpdateButtonsAfterCall( const NDb::EReinforcementType eType )
{
	GetScenarioTracker()->RegisterReinforcementCall( nPlayer, eType );

	// set recycle time
	if ( eType == NDb::RT_SUPER_WEAPON )
		timeToCall = timeSuperWeaponRecycleTime;
	else
		timeToCall = recycleTimes[eType];
	if ( GetScenarioTracker()->GetGameType() == IAIScenarioTracker::EGT_SINGLE )
	{
		if ( theDipl.GetNParty( nPlayer ) == 1 )
			timeReinfButtonEnable = curTime + timeToCall * fRecycleTimeCoeff * GetScenarioTracker()->GetEnemyDifficultyRTimeModifier();
		else
			timeReinfButtonEnable = curTime + timeToCall * fRecycleTimeCoeff;
	}
	else
	{
		timeReinfButtonEnable = curTime + timeToCall;
	}
	UpdateReinfButtonState( false, timeReinfButtonEnable, 0.0f );
}

void CPlayerReinforcement::CallReinforcement( NDb::EReinforcementType eType, int nPointID, int nScriptID, std::list< std::pair<int, CObjectBase*> > *pObjects, const bool bOnWater, const CVec2 &vTarget )
{
	if ( !GetScenarioTracker() )
		return;

	if ( curTime < timeReinfButtonEnable )
	{
		CONSOLE_BUFFER_LOG( CONSOLE_STREAM_CONSOLE, StrFmt( "Reinforcement cannot be called, wait %i seconds", (timeReinfButtonEnable - curTime) / 1000 + 1 ) );
		return;
	}
	CInfos::iterator pos = reinforcementInfos.find( eType );
	CPositions::iterator posPositions =  positions.find( nPointID );
	if ( pos != reinforcementInfos.end() && posPositions != positions.end() && pos->second != 0 )
	{
		const NDb::SReinforcement *pReinf = pos->second;
		bool bAviaReinf = pReinf->IsAviation();
		if ( !posPositions->second.second )
		{
			CONSOLE_BUFFER_LOG( CONSOLE_STREAM_CONSOLE, StrFmt( "Point %i is disabled", nPointID) );
			return;
		}

		const CVec2 vCallPoint = bAviaReinf ?
															posPositions->second.first.vAviationPosition :
															posPositions->second.first.vPosition;
		WORD wDirection =bAviaReinf ? 
															GetDirectionByVector( vTarget - vCallPoint ) :
															posPositions->second.first.nDirection;

		const NDb::SReinforcementPosition &positionToDeploy = posPositions->second.first;

		// choose deploy template
		CDBPtr<NDb::SDeployTemplate> pTemplate = GetDeployTemplate( positionToDeploy, eType );

		// call reinforcement
		if ( !bOnWater || bAviaReinf )
			NReinforcement::PlaceSingleLandReinforcement( nPlayer, pReinf, pReinf->eType, pTemplate, vCallPoint, wDirection, nScriptID, pObjects, false );
		else
		{
			// Check if the reinforcement has any ships
			const NDb::SReinforcement *pReinf = pos->second;
			if ( pReinf->transports.size() == 0 )
			{
				CONSOLE_BUFFER_LOG( CONSOLE_STREAM_CONSOLE, StrFmt( "Reinforcement #%i has no transport ships.", eType ) );
				NI_ASSERT( 0, StrFmt( "DESIGN(CHAPTER): Reinforcement \"%s\" is called from sea, but has no transports", NDb::GetResName(pReinf) ) );
				return;
			}

			// Call ships
			NReinforcement::PlaceSingleSeaReinforcement( nPlayer, pos->second, pTemplate,	vCallPoint, posPositions->second.first.nDirection, nScriptID, vTarget );
		}

		UpdateButtonsAfterCall( eType );
		CONSOLE_BUFFER_LOG( CONSOLE_STREAM_CONSOLE, StrFmt( "Player %i called reinf #%i to point %i. %i calls left.", nPlayer, eType, nPointID, Singleton<IAIScenarioTracker>()->GetReinforcementCallsLeft( nPlayer ) + nMapReinforcementBonus ) );
	}
	else 
		CONSOLE_BUFFER_LOG( CONSOLE_STREAM_CONSOLE, StrFmt( "CallReinforcement: cannot call reinforcement type %i to point %i for player %i (no point or no reinforcement)", eType, nPointID, nPlayer ) );
}

void CPlayerReinforcement::ShotSuperWeapon()
{
	if ( superWeaponType == NDb::SUPER_WEAPON_ARTILLERY )
		timeSuperWeaponFire = curTime + timeSuperWeaponFlyTime;
 	else
		timeSuperWeaponFire = curTime;

	superWeaponState = SWS_WAIT_FOR_FLY;
}

void CPlayerReinforcement::CallSuperWeapon()
{
	// get reinforcment
	CInfos::iterator posInfo = reinforcementInfos.find( NDb::RT_SUPER_WEAPON );
	if ( posInfo == reinforcementInfos.end() )
		return;
	CDBPtr<NDb::SReinforcement> pReinforcement = posInfo->second;

	NI_ASSERT( pReinforcement && pReinforcement->entries.size() > 0 && pReinforcement->entries[0].pMechUnit, "Invalid SuperWeapon reinforcement" );

	if ( !pReinforcement || pReinforcement->entries.size() == 0 || pReinforcement->entries[0].pMechUnit == 0 )
		return;

	// find position
	CPositions::const_iterator pos = positions.end();
	float fMinRange = 0.0f;
	for ( CPositions::const_iterator it = positions.begin(); it != positions.end(); ++it )
	{
		const CVec2 vPoint = it->second.first.vAviationPosition;
		const float fRange = fabs( vPoint - vSuperWeaponCallPoint );
		if ( it->second.second && ( fMinRange > fRange || pos == positions.end() ) )
		{
			fMinRange = fRange;
			pos = it;
		}
	}
	if ( pos == positions.end() )
		return;

	// call reinforcement
	CVec2 vTarget( vSuperWeaponCallPoint );
	if ( superWeaponType == NDb::SUPER_WEAPON_ARTILLERY )
	{
		CDBPtr<NDb::SMechUnitRPGStats> pReinfUnit = pReinforcement->entries[0].pMechUnit;
		NI_VERIFY( pReinfUnit->platforms.size() > 0 && pReinfUnit->platforms[0].guns.size() > 0 && pReinfUnit->platforms[0].guns[0].pWeapon, "Invalid weapon for SuperWeapon shell", return );
		CDBPtr<NDb::SWeaponRPGStats> pWeapon = pReinfUnit->platforms[0].guns[0].pWeapon;
		if ( pWeapon )
			vTarget += GetVectorByDirection( NRandom::Random( (int)0, (int)65535 ) )*NRandom::Random( pWeapon->fDispersion );
	}
	const bool bOnWater = GetTerrain()->GetTerrainType( GetAIMap()->GetTile( pos->second.first.vPosition ) ) == ETT_WATER_TERRAIN;
	const CVec2 vDeployPoint = pos->second.first.vAviationPosition;
	const WORD wDirection = GetDirectionByVector( vTarget - vDeployPoint );
	CDBPtr<NDb::SDeployTemplate> pTemplate = GetDeployTemplate( pos->second.first, NDb::RT_SUPER_WEAPON );
	std::list< std::pair<int, CObjectBase*> > objects;
	NReinforcement::PlaceSingleLandReinforcement( nPlayer, pReinforcement, pReinforcement->eType, pTemplate, vDeployPoint, wDirection, -1, &objects, nSuperWeaponShotsLeft != nSuperWeaponShots );
	SendReinforcementToPoint( objects, NDb::RT_SUPER_WEAPON, vTarget, false, superWeaponType == NDb::SUPER_WEAPON_BOMBER ? 0.0f : 1.0f );

	// update super weapon times
	--nSuperWeaponShotsLeft;
	if ( nSuperWeaponShotsLeft <= 0 )
	{
		pSuperWeaponShell = 0;
		superWeaponState = SWS_WAIT_FOR_CALL;
	}
	else
	{
		pSuperWeaponShell = dynamic_cast<CCommonUnit*>( objects.front().second );
		superWeaponState = SWS_FLYING;
	}

}

void CPlayerReinforcement::Segment()
{
	// Reinforcement counter recycle
	IAIScenarioTracker *pScenarioTracker = GetScenarioTracker();
	if ( pScenarioTracker->GetGameType() == IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL )
	{
		if ( curTime > timeReinfIncrease )
		{
			timeReinfIncrease += 1000 * fRecycleTimeCoeff;
			pScenarioTracker->IncreaseReinforcementCallsLeft( nPlayer, 1 );
			// Send update
			CPtr<SAIAvailableReinfUpdate> pUpdate = new SAIAvailableReinfUpdate;
			pUpdate->nReinforcementCallsLeft = pScenarioTracker->GetReinforcementCallsLeft( nPlayer );
			updater.AddUpdate( pUpdate, ACTION_NOTIFY_AVAIL_REINF, 0, -1 );
		}
		pScenarioTracker->SetReinforcementXP( nPlayer, NDb::_RT_NONE, ( timeReinfIncrease - curTime ) / 1000.0f );
	}

	CheckReinfButton();

	for ( int i = 0; i < commands.size(); ++i )
	{
		if ( commands[i].eType == NDb::RT_SUPER_WEAPON )
		{
			vSuperWeaponCallPoint = commands[i].vPoint;
			nSuperWeaponShotsLeft = nSuperWeaponShots;
			ShotSuperWeapon();
			UpdateButtonsAfterCall( NDb::RT_SUPER_WEAPON );
		}
		else
			CallReinforcement( commands[i].eType, commands[i].vPoint, -1 );
	}
	commands.clear();

	if ( superWeaponState == SWS_WAIT_FOR_FLY && curTime >= timeSuperWeaponFire )
		CallSuperWeapon();
	if ( superWeaponState == SWS_FLYING && ( !IsValid( pSuperWeaponShell ) || !(pSuperWeaponShell->IsAlive()) ) )
		ShotSuperWeapon();
}

void CPlayerReinforcement::UpdateDeletePosition( int nUniqueID ) const
{
	if ( nPlayer != theDipl.GetMyNumber() )
		return;

	CPtr<SAIReinfPointUpdate> pUpdate = new SAIReinfPointUpdate;

	pUpdate->bEnable = false;
	pUpdate->nPointID = nUniqueID;
	updater.AddUpdate( pUpdate, ACTION_NOTIFY_REINF_POINT, 0, -1 );		
}

void CPlayerReinforcement::UpdateAddPosition( int nID, const NDb::SReinforcementPosition &point ) const
{
	if ( nPlayer != theDipl.GetMyNumber() )
		return;

	CPtr<SAIReinfPointUpdate> pUpdate = new SAIReinfPointUpdate;

	pUpdate->bEnable = true;
	pUpdate->nPointID = nID;
	pUpdate->position = point;

	updater.AddUpdate( pUpdate, ACTION_NOTIFY_REINF_POINT, 0, -1 );		
}

void CPlayerReinforcement::UpdateReinfButtonState( bool bEnable, NTimer::STime timeWhenEnabled, float fPercentComplete )
{
	bReinfButtonEnabled = bEnable;

	if ( nPlayer != theDipl.GetMyNumber() )
		return;

	CPtr<SAIReinfRecycleUpdate> pUpdate = new SAIReinfRecycleUpdate;
	pUpdate->bEnabled = bEnable;
	pUpdate->timeRecycleEnd = timeWhenEnabled;
	pUpdate->fProgress = fPercentComplete;
	updater.AddUpdate( pUpdate, ACTION_NOTIFY_REINF_RECYCLE, 0, -1 );		
}

void CPlayerReinforcement::SetRecycleCoeff( const float fNewCoeff )
{
	if ( fRecycleTimeCoeff == fNewCoeff )
		return;

	if ( GetScenarioTracker()->GetGameType() == IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL )
	{
		timeReinfIncrease = ( timeReinfIncrease - curTime ) / fRecycleTimeCoeff * fNewCoeff + curTime;
		GetScenarioTracker()->SetReinforcementXP( nPlayer, NDb::_RT_NONE, ( timeReinfIncrease - curTime ) / 1000.0f );
		//DebugTrace( "Recycle Coeff changed from %f to %f", fRecycleTimeCoeff, fNewCoeff );
		fRecycleTimeCoeff = fNewCoeff;
		return;
	}

	if ( reinforcementInfos.size() == 0 || positions.size() == 0 )		// No reinfs or no positions
	{
		fRecycleTimeCoeff = fNewCoeff;
		return;
	}

	if ( !bReinfButtonEnabled )						// Only do calculations if recycling now
	{
		if ( fRecycleTimeCoeff < 0.0f )
		{
			if ( fNewCoeff >= 0.0f )		// Progress restarts
			{
				timeReinfButtonEnable = curTime + timeToCall * fNewCoeff * ( 1.0f - fStoredProgress );
				UpdateReinfButtonState( false, timeReinfButtonEnable, fStoredProgress );
			}
		}
		else
		{
			fStoredProgress = 1.0f - ( timeReinfButtonEnable - curTime ) / ( timeToCall * fRecycleTimeCoeff );
			if ( fNewCoeff < 0.0f )			// Progress stops
			{
				timeReinfButtonEnable = 0;
				UpdateReinfButtonState( false, 0, fStoredProgress );
			}
			else
			{
				timeReinfButtonEnable = curTime + timeToCall * fNewCoeff * ( 1.0f - fStoredProgress );
				UpdateReinfButtonState( false, timeReinfButtonEnable, fStoredProgress );
			}
		}
	}
	fRecycleTimeCoeff = fNewCoeff;
}

const NDb::SReinforcementPosition * CPlayerReinforcement::GetPosition( int nPositionID ) const
{
	CPositions::const_iterator pos = positions.find( nPositionID );
	if ( pos != positions.end() )
		return &pos->second.first;
	return 0;
}

int CPlayerReinforcement::GetReinforcementCallsLeft() const
{
	if ( !GetScenarioTracker() )
		return 0;

	return GetScenarioTracker()->GetReinforcementCallsLeft( nPlayer );
}

bool CPlayerReinforcement::HasGroundReinforcements() const
{
	for ( CInfos::const_iterator it = reinforcementInfos.begin(); it != reinforcementInfos.end(); ++it )
	{
		if ( it->second->eType != RT_FIGHTERS &&
				 it->second->eType != RT_BOMBERS &&
				 it->second->eType != RT_GROUND_ATTACK_PLANES &&
				 it->second->eType != RT_RECON &&
				 it->second->eType != RT_PARATROOPS )
		{
			return true;
		}
	}
	return false;
}

bool CPlayerReinforcement::CanCallNow() const
{
	if ( curTime < timeReinfButtonEnable )
		return false;

	const int nCallsLeft = GetReinforcementCallsLeft();
	const bool bCanBeCalled = !reinforcementInfos.empty() && 
		( nCallsLeft > 0 || nCallsLeft == IAIScenarioTracker::INFINITE_CALLS );

	if ( !bCanBeCalled )
		return false;

	for ( CPositions::const_iterator it = positions.begin(); it != positions.end(); ++it )
		if ( it->second.second )
			return true;

	return false;
}

int CPlayerReinforcement::GetRandomPointID() const
{
	std::vector<int> pointIDs;
	for ( CPositions::const_iterator it = positions.begin(); it != positions.end(); ++it )
	{
		if ( it->second.second )
			pointIDs.push_back( it->first );
	}
	if ( pointIDs.empty() )
		return -1;
	
	return pointIDs[NRandom::Random( pointIDs.size() )];
}

CVec2 CPlayerReinforcement::GetRandomPoint() const
{
	return GetPosition( GetRandomPointID() )->vPosition;
}

bool CPlayerReinforcement::HasReinforcement( NDb::EReinforcementType eType ) const
{
	if ( eType == _RT_NONE )
	{
		return !reinforcementInfos.empty();
	}
	else
	{
		CInfos::const_iterator pos = reinforcementInfos.find( eType );
		return pos != reinforcementInfos.end();
	}
}

NTimer::STime CPlayerReinforcement::GetRecycleTimeLeft() const
{
	return timeReinfButtonEnable < curTime ? 0 : timeReinfButtonEnable - curTime;
}

int CPlayerReinforcement::GetNEntries( NDb::EReinforcementType eType ) const
{
	CInfos::const_iterator pos = reinforcementInfos.find( eType );
	if ( pos != reinforcementInfos.end() )
		return pos->second->entries.size();
	return 0;
}

const NDb::SHPObjectRPGStats * CPlayerReinforcement::GetUnitSample( NDb::EReinforcementType eType, int nEntry ) const
{
	CInfos::const_iterator pos = reinforcementInfos.find( eType );
	if ( pos != reinforcementInfos.end() )
	{
		if ( nEntry < pos->second->entries.size() )
		{
			if ( pos->second->entries[nEntry].pMechUnit)
				return pos->second->entries[nEntry].pMechUnit;
			if ( pos->second->entries[nEntry].pSquad )
				pos->second->entries[nEntry].pSquad;
		}
	}
	return 0;
}

const int CPlayerReinforcement::GetUnitAvailability( const NDb::EDBUnitRPGType eUnit, const NDb::EReinforcementType eReinf )
{ 
	if ( unitAvailability.GetSizeX() > eUnit && unitAvailability.GetSizeY() > eReinf )
		return unitAvailability[eReinf][eUnit]; 
	return 0;
}

void CPlayerReinforcement::PlaceInitialUnits()
{
	const NDb::SReinforcement *pReinf = GetScenarioTracker()->GetStartUnits( nPlayer );

	if ( !pReinf )
		return;

	CPositions::iterator pos = positions.begin();
	/*for ( ; pos != positions.end(); ++pos )
	{
		if ( pos->second.second )
			break;
	}*/

	if ( pos == positions.end() )
		return;

	this->EnablePosition( pos->first, true );

	const NDb::SReinforcementPosition &positionToDeploy = pos->second.first;

	CDBPtr<NDb::SDeployTemplate> pTemplate = GetDeployTemplate( positionToDeploy, NDb::_RT_NONE );		// Get default template

	NReinforcement::PlaceSingleLandReinforcement( nPlayer, pReinf, NDb::_RT_NONE, pTemplate, positionToDeploy.vPosition, positionToDeploy.nDirection, -1, 0, false );
}

void CPlayerReinforcement::GiveReinforcementCalls( int nCalls, bool bResetCounter )
{
	IAIScenarioTracker *pScenarioTracker = GetScenarioTracker();

	if ( pScenarioTracker->GetGameType() != IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL )
		return;

	if ( NGlobal::GetVar( "multiplayer_flag_calls_bonus", 0 ) != 1 )
		return;

	int nOldCalls = pScenarioTracker->GetReinforcementCallsLeft( nPlayer );

	if ( nCalls > 0 )
		pScenarioTracker->IncreaseReinforcementCallsLeft( nPlayer, nCalls );
	else if ( nCalls < 0 )
		pScenarioTracker->DecreaseReinforcementCallsLeft( nPlayer, -nCalls );

	if ( bResetCounter )
	{
		timeReinfIncrease = curTime + 1000 * fRecycleTimeCoeff;
		pScenarioTracker->SetReinforcementXP( nPlayer, NDb::_RT_NONE, ( timeReinfIncrease - curTime ) / 1000.0f );
	}
}

void CPlayerReinforcement::CheckReinfButton()
{
	const bool bNotYetTime = timeReinfButtonEnable > 0 && ( curTime < timeReinfButtonEnable );
	const int nCallsLeft = GetReinforcementCallsLeft();
	const bool bNoReinfs = reinforcementInfos.empty() || ( nCallsLeft <= 0 && nCallsLeft != IAIScenarioTracker::INFINITE_CALLS );

	bool bNoPositions = true;
	for ( CPositions::const_iterator it = positions.begin(); it != positions.end(); ++it )
	{
		if ( it->second.second )
		{
			bNoPositions = false;
			break;
		}
	}

	if ( bNoReinfs || bNoPositions )
	{
		if ( bReinfButtonEnabled || timeReinfButtonEnable > 0 )
		{
			timeReinfButtonEnable = 0;
			UpdateReinfButtonState( false, 0, 0.0f );
		}
		return;
	}

	if ( bNotYetTime )
	{
		float fCoeff = 1.0f;
		if ( GetScenarioTracker()->GetGameType() == IAIScenarioTracker::EGT_SINGLE )
			fCoeff = fRecycleTimeCoeff;

		if ( timeToCall > 0 )
			fStoredProgress = 1.0f - ( timeReinfButtonEnable - curTime ) / ( fCoeff * timeToCall );
		else
			fStoredProgress = 0.0f;
		UpdateReinfButtonState( false, timeReinfButtonEnable, fStoredProgress );
	}
	else if ( !bReinfButtonEnabled )
		UpdateReinfButtonState( true, 0, 0.0f );
}

// CPlayerReinforcementArray

void CPlayerReinforcementArray::InitPlayerReinforcementArray( const NDb::SMapInfo *pMapInfo, const NDb::SAIGameConsts *pConsts )
{
	resize( pMapInfo->players.size() );
	for ( int i = 0; i < pMapInfo->players.size(); ++ i )
	{
		operator[]( i ).InitPlayerReinforcement( i, pMapInfo, pConsts );
	}
}

int CPlayerReinforcementArray::operator&( IBinSaver &saver )
{
	saver.Add( 1, (std::vector<CPlayerReinforcement>*)(this) );
	return 0;
}

void CPlayerReinforcementArray::Segment()
{
	for ( std::vector<CPlayerReinforcement>::iterator it = begin(); it != end(); ++it )
		it->Segment();
}

void CPlayerReinforcementArray::SetRecycleCoeff( const int nPlayer, const float fNewCoeff )
{
	if ( nPlayer >= 0 && nPlayer < size() )
		operator[]( nPlayer ).SetRecycleCoeff( fNewCoeff );
}

void CPlayerReinforcementArray::PlaceInitialUnits()
{
	for ( std::vector<CPlayerReinforcement>::iterator it = begin(); it != end(); ++it )
		it->PlaceInitialUnits();
}

void CPlayerReinforcementArray::GiveReinforcementCalls( const int nPlayer, const int nCalls, const bool bResetCounter )
{
	if ( nPlayer >= 0 && nPlayer < size() )
		operator[]( nPlayer ).GiveReinforcementCalls( nCalls, bResetCounter );
}


