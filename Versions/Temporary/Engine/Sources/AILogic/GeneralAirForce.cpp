#include "stdafx.h"

#include "EnemyRememberer.h"
#include "GeneralHelper.h"
#include "GeneralAirForce.h"
#include "UnitCreation.h"
#include "General.h"
#include "UnitsIterators2.h"
#include "UnitsIterators.h"
#include "Aviation.h"
#include "Guns.h"
#include "GlobalWarFog.h"
#include "GeneralConsts.h"
#include "B2AI.h"
#include "PlayerREinforcement.h"
#include "Stats_B2_M1/AIUnitCmd.h"

#include <algorithm>
#include <random>

extern CPlayerReinforcementArray theReinfArray;
// time to wait for reinforcement system to process aviation call 
static const int TIME_WAIT_FOR_REINFORCE_SYSTEM = 1000;


extern CGlobalWarFog theWarFog;
extern CDiplomacy theDipl;
extern CUnitCreation theUnitCreation;
extern NTimer::STime curTime;
extern CSupremeBeing theSupremeBeing;

REGISTER_SAVELOAD_CLASS( 0x1508D4AE, CGeneralAirForce );

//BASIC_REGISTER_CLASS(CGeneralAirForce);

//*******************************************************************
//*											CGeneralAirForce*
//*******************************************************************

CGeneralAirForce::CGeneralAirForce( const int nParty, IEnemyContainer *pEnemyContainer ) 
	: pEnemyContainer( pEnemyContainer ), 
	nParty( nParty ), bOurTurn( false ),
	nCurrentRequest( FT_NONE ), timeWaitForReinforceSystem( curTime )
{  
	for ( int i = 0; i < theDipl.GetNPlayers(); ++i )
	{
		if ( theDipl.GetNParty(i) == nParty )
		{
			players.push_back( i );
		}
	}
	requests.clear();
}

void CGeneralAirForce::PassTurn()
{
	bOurTurn = true;
}

bool CGeneralAirForce::TurnReturned() const
{
	return !bOurTurn;
}

void CGeneralAirForce::Segment()
{
	CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW + 5, (bOurTurn ? "aviation's turn" : "ground general's turn") );
	if ( !bOurTurn )
		return;

	for ( int i = 0; i < players.size(); ++i )
	{
		if (  players[i] != theDipl.GetMyNumber() )
		{
			const int nPlayer = players[i];

			if ( !theReinfArray[nPlayer].HasReinforcement( NDb::RT_FIGHTERS ) &&
					 !theReinfArray[nPlayer].HasReinforcement( NDb::RT_GROUND_ATTACK_PLANES ) &&
					 !theReinfArray[nPlayer].HasReinforcement( NDb::RT_BOMBERS ) )
			{
				bOurTurn = false;
			}

			if ( !createdAviation.empty() )									// reinforcement created
			{
				// launch aviation that was created
				GiveOrders( nPlayer );
				createdAviation.clear();
				bOurTurn = false;
			}
			else if ( timeWaitForReinforceSystem > curTime ) // reinforcement already ordered, wait
				break;

			if ( theReinfArray[nPlayer].CanCallNow() && theReinfArray[nPlayer].HasReinforcement( NDb::RT_FIGHTERS ) )
			{
				if ( PrepeareFighters( nPlayer ) )
					CallReinforcement( nPlayer, NDb::RT_FIGHTERS, &requests, FT_AIR_FIGHTER );
			}

			if ( theReinfArray[nPlayer].CanCallNow() && theReinfArray[nPlayer].HasReinforcement( NDb::RT_GROUND_ATTACK_PLANES ) )
				CallReinforcement( nPlayer, NDb::RT_GROUND_ATTACK_PLANES, &requests, FT_AIR_GUNPLANE );

			if ( theReinfArray[nPlayer].CanCallNow() && theReinfArray[nPlayer].HasReinforcement( NDb::RT_BOMBERS ) )
				CallReinforcement( nPlayer, NDb::RT_BOMBERS, &requests, FT_AIR_BOMBER );

			//CRAP{ UNTILL SCOUTS IN REINFORCEMENTS
			//if ( theReinfArray[nPlayer].CanCallNow() && theReinfArray[nPlayer].HasReinforcement( NDb::RT_GROUND_ATTACK_PLANES ) )
				//LaunchScoutFree( nPlayer, NDb::RT_GROUND_ATTACK_PLANES );
			//CRAP}
			if ( theReinfArray[nPlayer].CanCallNow() )
				bOurTurn = false;
		}
	}
	std::list<int> deleted;
	for ( AntiAviation::iterator it = antiAviation.begin(); antiAviation.end() != it; ++it )
	{
		if ( it->second->IsTimeToForget() )
			deleted.push_back( it->first );
	}

	while( !deleted.empty() )
	{
		antiAviation.erase( deleted.front() );
		deleted.pop_front();
	}
}

bool CGeneralAirForce::PrepeareFighters( int nPlayer )
{
	bool bFound = false;
	for ( CPlanesIter iter; !iter.IsFinished(); iter.Iterate() )
	{
		CAviation *pPlane = *iter;
		if ( pPlane->IsHelicopter() )
			continue;

		if ( pPlane->IsAlive() &&
			RPG_TYPE_AVIA_FIGHTER != pPlane->GetStats()->etype && 
			EDI_FRIEND == theDipl.GetDiplStatus( pPlane->GetPlayer(), nPlayer ) )
		{
			// don't call new fighters
			return false;
		}
	}
	for ( CPlanesIter iter; !iter.IsFinished(); iter.Iterate() )
	{
		CAviation *pPlane = *iter;
		if ( pPlane->IsHelicopter() )
			continue;

		if ( pPlane->IsAlive() &&
				 RPG_TYPE_AVIA_FIGHTER != pPlane->GetStats()->etype && 
				 EDI_ENEMY == theDipl.GetDiplStatus( pPlane->GetPlayer(), nPlayer ) )
		{
			RequestForSupport( pPlane->GetCenterPlain(), FT_AIR_FIGHTER, -1 );
			bFound = true;
		}
	}
	return bFound;
}

void CGeneralAirForce::GiveOrders( const int nPlayer )
{
	if ( nCurrentRequest == -1 )
		return;

	std::list<CVec2> vPoints;
	Requests *pRequest = &requests[nCurrentRequest];
	
	for ( Requests::iterator it = pRequest->begin(); it != pRequest->end(); ++it )
		vPoints.push_back( it->second.vPoint );

	CAviation * pSamplePlane = checked_cast_ptr<CAviation*>( createdAviation[0] );

	// проредить.
	SSameEnemyPointPredicate pr1;
	std::list<CVec2>::iterator firstSame = unique( vPoints.begin(), vPoints.end(), pr1 );
	vPoints.erase( firstSame, vPoints.end() );

	const float fFlyHeight( pSamplePlane->GetZ() );

	// проверить каждую линию на безопасность.
	CVec2 vCurStartPoint = pSamplePlane->GetCenterPlain();
	for ( std::list<CVec2>::iterator it = vPoints.begin(); it != vPoints.end();  )
	{
		if ( 0 == CheckLineForSafety( vCurStartPoint, *it, fFlyHeight ) )
		{
			vCurStartPoint = *it;
			++it;
		}
		else
			it = vPoints.erase( it );
	}

	LaunchPlane( nCurrentRequest, vPoints, nPlayer );

	for ( Requests::iterator it = pRequest->begin(); pRequest->end() != it; ++it )
	{
		requestsID.Return( it->first );
	}
	pRequest->clear();
	nCurrentRequest = FT_NONE;
}

void CGeneralAirForce::CallReinforcement( const int nPlayer, NDb::EReinforcementType eType, RequestsByForceType *pRequests, EForceType eForceType )
{
	RequestsByForceType::const_iterator pos = pRequests->find( eForceType );
	if ( pos == pRequests->end() )
		return;

	timeWaitForReinforceSystem = curTime + TIME_WAIT_FOR_REINFORCE_SYSTEM;
	nCurrentRequest = eForceType;
	Requests::const_iterator posReq = pos->second.begin();
	if ( posReq != pos->second.end() )
	{
		theReinfArray[nPlayer].AddCallReinforcementCommand( eType, posReq->second.vPoint );
	}
}

float CGeneralAirForce::CheckLineForSafety( const CVec2 &vStart, const CVec2 &vFinish, const float fFlyHeight )
{
	CLine2 line( vStart, vFinish );
	const CVec2 vLineCenter( ( vStart + vFinish ) / 2 );

	for ( AntiAviation::iterator it = antiAviation.begin(); it != antiAviation.end(); ++it )
	{
		CAIUnit *pUnit = GetObjectByUniqueIdSafe<CAIUnit>( it->first );
		
		if ( !pUnit || !pUnit->IsRefValid() || !pUnit->IsAlive() ) continue;

		const CVec2 vCenter = pUnit->GetCenterPlain();
		// find gun that can shoot to planes and it's range
		const int nGuns = pUnit->GetNGuns();
		for ( int i = 0; i < nGuns; ++i )
		{
			CBasicGun *pGun = pUnit->GetGun( i );
			const SWeaponRPGStats *pStats = pGun->GetWeapon();
			if ( pStats->nCeiling > fFlyHeight )
			{
				const float fDistToCenter = fabs2( vCenter - vLineCenter );
				const float fDist1 = fabs2( vStart - vCenter );
				float fMinDist;

				if ( fDistToCenter > fDist1 ) // нормаль от точки не падает на отрезок
					fMinDist = fDist1;
				else 
				{
					const float fDist2 = fabs2( vCenter - vFinish );
					if ( fDistToCenter > fDist2 )
						fMinDist = fDist2;
					else
						fMinDist = sqr( fabs( line.DistToPoint( vCenter ) ) );

				}

				if ( sqr( pGun->GetFireRangeMax() ) > fMinDist )
					return 1;
			}
		}
	}
	return 0;
}

void CGeneralAirForce::LaunchPlane( EForceType eType, const std::list<CVec2> &vPoints, const int nPlayer )
{
	bOurTurn = false;

	if ( vPoints.empty() ) 
		return;
	
	SAIUnitCmd cmd;
	std::list<CVec2>::const_iterator it = vPoints.begin();
	const int nGroupID = Singleton<IAILogic>()->GenerateGroupNumber();
	cmd.fNumber = 0;	// allow unit creation disable planes.

	cmd.vPos = *it;
	switch( eType )
	{
	case FT_AIR_SCOUT:
		cmd.nCmdType = ACTION_COMMAND_SWARM_TO;
		break;
	case FT_AIR_FIGHTER:
		cmd.nCmdType = ACTION_COMMAND_SWARM_TO;
		break;
	case FT_AIR_BOMBER:
		cmd.nCmdType = ACTION_COMMAND_ART_BOMBARDMENT;
		break;
	case FT_AIR_GUNPLANE:
		cmd.nCmdType = ACTION_COMMAND_SWARM_TO;
		break;
	}

	std::vector<int> ids;
	for ( int i = 0; i < createdAviation.size(); ++i )
		ids.push_back( createdAviation[i]->GetUniqueId() );
	Singleton<IAILogic>()->RegisterGroup( ids, nGroupID );

	// если вызвались
	for ( ; it != vPoints.end(); ++it )
	{
		cmd.vPos = *it;
		Singleton<IAILogic>()->GroupCommand( &cmd, nGroupID, true );
	}
	Singleton<IAILogic>()->UnregisterGroup( nGroupID );
	Singleton<IAILogic>()->SetNeedNewGroupNumber();
}

void CGeneralAirForce::LaunchScoutFree( const int nPlayer, const NDb::EReinforcementType eType )
{
	const float fSizeX = GetAIMap()->GetSizeX();
	const float fSizeY = GetAIMap()->GetSizeY();
	const int nParty = theDipl.GetNParty( nPlayer );

	std::vector<CVec2> points;
	
	const int nStep = (std::max)( 2.0f, fSizeX / SGeneralConsts::SCOUT_POINTS );

	for ( int x = nStep / 2; x < fSizeX; x += nStep )
	{
		for ( int y = nStep / 2; y < fSizeY; y += nStep )
		{
			if ( !theWarFog.IsTileVisible( SVector(x,y), nParty ) )
				points.push_back( CVec2( x, y ) );
		}
	}
	std::random_device rd;
	std::mt19937 g(rd());

	std::shuffle( points.begin(), points.end(), g );
	if ( points.empty() )
	{
		bOurTurn = false;
		return;
	}

	theReinfArray[nPlayer].CallReinforcement( eType, points[0], -1 );
	//NI_ASSERT( !createdAviation.empty(), "cannot call scouts" );
	if ( createdAviation.empty() )
	{
		bOurTurn = false;
		return;
	}

	CAviation *pSamplePlane = checked_cast_ptr<CAviation*>( createdAviation[0] );

	const CVec2 vAppearPoint( pSamplePlane->GetCenterPlain() );
	const float fFlyHeight( pSamplePlane->GetZ() );
	
	std::list<CVec2> vPointsToFly;
	const float fCheckRadius = (std::min)( static_cast<int>( SGeneralConsts::SCOUT_FREE_POINT ),
																	static_cast<int>( nStep * SConsts::TILE_SIZE ) );
	
	CVec2 vCurStartPoint = vAppearPoint;
	
	for ( int i = 0; i < points.size(); ++i )
	{
		bool bCan = true;
		// find our units in range near scout
		for ( CUnitsIter<0,3> iter( nParty, EDI_FRIEND, points[i], fCheckRadius );
					!iter.IsFinished(); iter.Iterate() )
		{
			CAIUnit * pUnit = *iter;
			if ( pUnit->IsAlive() && sqr(fCheckRadius) > fabs2(points[i]-pUnit->GetCenterPlain()) )
			{
				bCan = false;
				break;
			}
		}
		if ( bCan )
		{
			const CVec2 vCurPointToAdd = points[i] * SConsts::TILE_SIZE ;
			if ( 0 == CheckLineForSafety( vCurStartPoint, vCurPointToAdd, fFlyHeight ) )
			{
				vPointsToFly.push_back( vCurPointToAdd );
				vCurStartPoint = vCurPointToAdd;
			}
		}
	}
	// вообще-то круто было бы проверить и возврат, но это невозможно
	if ( !vPointsToFly.empty() )
		LaunchPlane( FT_AIR_SCOUT, vPointsToFly, nPlayer );
}

int /*request ID*/CGeneralAirForce::RequestForSupport( const CVec2 &vSupportCenter, enum EForceType eType, int nResistanceCellNumber )
{
	if ( nResistanceCellNumber != -1 )
	{
		std::list<int> delRequest;
		for ( Requests::iterator iter = requests[eType].begin(); iter != requests[eType].end(); ++iter )
		{
			if ( iter->second.nResistanceCellNumber != -1 )
				delRequest.push_back( iter->first );
		}
		for ( std::list<int>::iterator iter = delRequest.begin(); iter != delRequest.end(); ++iter )
			CancelRequest( *iter, eType );
	}

	const int nID = requestsID.Get();
	SSupportInfo info;
	info.vPoint = vSupportCenter;
	info.nResistanceCellNumber = nResistanceCellNumber;
	
	requests[eType][nID] = info;
	return nID;
}

void CGeneralAirForce::CancelRequest( int nRequestID, enum EForceType eType )
{
	if ( 0 != nRequestID )
	{
		NI_ASSERT( requests[eType].find( nRequestID ) != requests[eType].end(), "wrong cancel request" );
		requests[eType].erase( nRequestID );
		requestsID.Return( nRequestID );
	}
}

void CGeneralAirForce::Give( CCommonUnit *pWorker )
{
	CAIUnit *pUnit = checked_cast<CAIUnit*>( pWorker );
	if ( pUnit->IsHelicopter() )
		return;

	createdAviation.push_back( pUnit );
}

void CGeneralAirForce::DeleteAA( CAIUnit *pUnit )
{
	antiAviation.erase( pUnit->GetUniqueId() );
}

void CGeneralAirForce::SetAAVisible( CAIUnit *pUnit, const bool bVisible )
{
	AntiAviation::iterator it = antiAviation.find( pUnit->GetUniqueId() );
	if (  it == antiAviation.end() )
	{
		// создать
		CEnemyRememberer *pEnemy = new CEnemyRememberer( SGeneralConsts::TIME_SONT_SEE_AA_BEFORE_FORGET );
		antiAviation[pUnit->GetUniqueId()] = pEnemy;
	}

	antiAviation[pUnit->GetUniqueId()]->SetVisible( pUnit, bVisible );
}

