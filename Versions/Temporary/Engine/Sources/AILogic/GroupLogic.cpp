#include "stdafx.h"

#include "unitssegments.h"
#include "System/Time.h"
#include "GroupLogic.h"
#include "Commands.h"
#include "Soldier.h"
#include "UnitsIterators.h"
#include "NewUpdater.h"
#include "UnitStates.h"
#include "Formation.h"
#include "Shell.h"
#include <float.h>
#include "ScanLimiter.h"
#include "Building.h"
#include "GridCreation.h"
#include "B2AI.h"
#include "System/FastMath.h"
#include "Artillery.h"

//#include "..\Common_RTS_AI\CollisionInternal.h"

// for profiling
#include "TimeCounter.h"
// for debug
//#include "..\Scene\Statistics.h"
#include "ExecutorContainer.h"
#include "CommandRegistratorForScript.h"
#include "GroupMover.h"

EXTERNVAR CExecutorContainer theExecutorContainer;
CGroupLogic theGroupLogic;
extern CEventUpdater updater;
extern NTimer::STime curTime;
extern CDiplomacy theDipl;
extern CShellsStore theShellsStore;
extern CScanLimiter theScanLimiter;
extern CCommandRegistratorForScript theCommandTrackerForScript;

extern NAI::CTimeCounter timeCounter;

//*******************************************************************
//*														CGroupLogic														*
//*******************************************************************

void CGroupLogic::Init( ICollisionsCollector *_pCollisionsCollector )
{
	lastSegmTime = curTime - curTime % SConsts::AI_SEGMENT_DURATION;
	segmUnits.resize( 2 );

	registeredGroups.clear();
	lastAmbushCheck = 0;
	pCollisionsCollector = _pCollisionsCollector;
}

void CGroupLogic::AddUnitToGroup( CCommonUnit *pGroupUnit, const int nGroup )
{
	if ( pGroupUnit->nGroup != nGroup )
	{
		// если юнит был в какой-то группе, убрать его оттуда
		DelUnitFromGroup( pGroupUnit );

		pGroupUnit->nGroup = nGroup;
		pGroupUnit->nPos = groupUnits.Push( nGroup, pGroupUnit );
	}

}

void CGroupLogic::DelUnitFromGroup( CCommonUnit *pUnit )
{
	if ( pUnit->nGroup != 0 )
	{
		groupUnits.Erase( pUnit->nGroup, pUnit->nPos );
		pUnit->nGroup = 0;
		pUnit->nPos = 0;
	}
}

void CGroupLogic::DelUnitFromSpecialGroup( CCommonUnit *pUnit )
{
	if ( pUnit->nSpecialGroup != 0 )
	{
		if ( groupUnits.begin( pUnit->nSpecialGroup ) != groupUnits.end() )
			groupUnits.Erase( pUnit->nSpecialGroup, pUnit->nSpecialPos );

		if ( groupUnits.begin( pUnit->nSpecialGroup ) == groupUnits.end() )
			UnregisterSpecialGroup( pUnit->nSpecialGroup );
		pUnit->nSpecialGroup = 0;
	}
}

void CGroupLogic::DelGroup( const int nGroup )
{
	// известить юнитов
	for ( int i = groupUnits.begin( nGroup ); i != groupUnits.end(); i = groupUnits.GetNext( i ) )
	{
		groupUnits.GetEl( i )->nGroup = 0;
		groupUnits.GetEl( i )->nPos = 0;
	}
	groupUnits.DelQueue( nGroup );
}

WORD CGroupLogic::GetGroupNumberByID( const WORD wID )
{
	return ( ( wID << 4 ) | BYTE( theDipl.GetMyNumber() ) ) << 1;
}

WORD CGroupLogic::GetSpecialGroupNumberByID( const WORD wID )
{
	return ( ( ( wID << 4 ) | BYTE( theDipl.GetMyNumber() ) ) << 1 ) | 1;
}

WORD CGroupLogic::GetIdByGroupNumber( const WORD wGroup )
{
	return wGroup >> 5;
}

WORD CGroupLogic::GetPlayerByGroupNumber( const WORD wGroup )
{
	return ( wGroup >> 1 ) & 0xf;
}

void CGroupLogic::RegisterGroup( CObjectBase **pUnitsBuffer, const int nLen, const WORD wGroup )
{
	std::vector<int> aviaIndexes( nLen, 0 );
	CVec2 vPos = VNULL2;
	int nAvia = 0;
	for ( int i = 0; i < nLen; ++i )
	{
		CCommonUnit *pCommonUnit = dynamic_cast<CCommonUnit*>( pUnitsBuffer[i] );
		if ( pCommonUnit && !pCommonUnit->IsFormation() )
		{
			CAIUnit * pAIUnit = checked_cast<CAIUnit*>( pCommonUnit );
			if ( pAIUnit->GetStats()->IsAviation() )
			{				
				vPos += pAIUnit->GetCenterPlain();
				aviaIndexes[nAvia++] = i;
			}
		}
	}
	if ( nAvia != 0 )
		vPos /= nAvia;
	for ( int i = 0; i < nAvia; ++i )
	{
		CAIUnit * pAvia = dynamic_cast<CAIUnit*>( pUnitsBuffer[aviaIndexes[i]] );
		CVec2 vShift( pAvia->GetCenterPlain() - vPos );
		if ( fabs( vShift ) > 1000 )
		{
			Normalize( &vShift );
			vShift *= 1000;
		}
		pAvia->SetGroupShift( vShift );
	}

	if ( wGroup >= groupUnits.GetQueuesNum() )
		groupUnits.IncreaseQueuesNum( wGroup * 1.5 );

	if ( groupUnits.GetSize( wGroup ) != 0 )
	{
		DelGroup( wGroup );

		if ( GetPlayerByGroupNumber( wGroup ) == theDipl.GetMyNumber() )
			groupIds.Return( GetIdByGroupNumber( wGroup ) );
	}

	registeredGroups.insert( wGroup );

	for ( int i = 0; i < nLen; ++i )
	{
		if ( pUnitsBuffer[i] != 0 && dynamic_cast<CCommonUnit*>(pUnitsBuffer[i]) != 0 )
		{
			CCommonUnit *pUnit = checked_cast<CCommonUnit*>( pUnitsBuffer[i] );
			CArtillery *pArtillery = dynamic_cast<CArtillery *>( pUnit );
			const bool bNotHookedArtillery = pArtillery == 0 || !pArtillery->IsBeingHooked();
			if ( pUnit->IsRefValid() && pUnit->IsAlive() && bNotHookedArtillery )
			{
				CCommonUnit *pGroupUnit;
				if ( pUnit->IsInFormation() )
					pGroupUnit = pUnit->GetFormation();
				else
					pGroupUnit = pUnit;

				if ( pGroupUnit->nGroup != wGroup )				
				{
					DelUnitFromGroup( pGroupUnit );

					pGroupUnit->nGroup = wGroup;
					pGroupUnit->nPos = groupUnits.Push( wGroup, pGroupUnit );
				}
			}
		}
	}
}

void CGroupLogic::RegisterGroup( const std::vector<int> &vIDs, const WORD wGroup )
{
	typedef CObjectBase* LPObjectBase;
	LPObjectBase *objects = new LPObjectBase[ vIDs.size() ];
	// for planes calculate group shift

	CVec2 vPos = VNULL2;
	int nAvia = 0;
	for ( int i = 0; i < vIDs.size(); ++i )
		objects[i] = (CObjectBase *)CLinkObject::GetObjectByUniqueIdSafe( vIDs[i] );

	RegisterGroup( objects, vIDs.size(), wGroup );
	delete[] objects;
}

const WORD CGroupLogic::GenerateGroupNumber()
{
	return GetGroupNumberByID( groupIds.Get() );
}

void CGroupLogic::UnregisterSpecialGroup( const WORD wSpecialGroup )
{
	if ( registeredGroups.find( wSpecialGroup ) != registeredGroups.end() )
	{
		for ( int i = groupUnits.begin( wSpecialGroup ); i != groupUnits.end(); i = groupUnits.GetNext( i ) )
			groupUnits.GetEl( i )->nSpecialGroup = 0;

		groupUnits.DelQueue( wSpecialGroup );
		groupIds.Return( GetIdByGroupNumber( wSpecialGroup ) );

		registeredGroups.erase( wSpecialGroup );
	}
}

void CGroupLogic::UnregisterGroup( const WORD wGroup ) 
{ 	
	if ( registeredGroups.find( wGroup ) != registeredGroups.end() )
	{
		DelGroup( wGroup );

		registeredGroups.erase( wGroup );
	}
	if ( GetPlayerByGroupNumber( wGroup ) == theDipl.GetMyNumber() )
		groupIds.Return( GetIdByGroupNumber( wGroup ) );

}

void CGroupLogic::DivideBySubGroups2( const SAIUnitCmd &command, const int nGroup )
{
	CVec2 vGroupCenter( VNULL2 );
	int nCount = 0;
  for ( int i = groupUnits.begin( nGroup ); i != groupUnits.end(); i = groupUnits.GetNext( i ) )
	{
		vGroupCenter += groupUnits.GetEl( i )->GetCenterPlain();
		++nCount;
	}
	if ( nCount == 0 )
		return;
	vGroupCenter /= nCount;
	bool bTooFar = false;
	for ( int i = groupUnits.begin( nGroup ); i != groupUnits.end() && !bTooFar; i = groupUnits.GetNext( i ) )
		bTooFar = fabs2( vGroupCenter - groupUnits.GetEl( i )->GetCenterPlain() ) > sqr( SConsts::GROUP_DISTANCE );
	if ( bTooFar )
	{
		for ( int i = groupUnits.begin( nGroup ); i != groupUnits.end(); i = groupUnits.GetNext( i ) )
		{
			groupUnits.GetEl( i )->nSubGroup = -1;
			groupUnits.GetEl( i )->vShift = VNULL2;
		}
	}
	else
	{
		for ( int i = groupUnits.begin( nGroup ); i != groupUnits.end(); i = groupUnits.GetNext( i ) )
		{
			groupUnits.GetEl( i )->nSubGroup = -1;
			groupUnits.GetEl( i )->vShift = groupUnits.GetEl( i )->GetCenterPlain() - vGroupCenter;
		}
	}
}

void CGroupLogic::DivideBySubGroups( const SAIUnitCmd &command, const int nGroup )
{
	float fMinX = 1e10, fMaxX = -1e10, fMinY = 1e10, fMaxY = -1e10;
	for ( int i = groupUnits.begin( nGroup ), n = 0; i != groupUnits.end(); i = groupUnits.GetNext( i ), ++n )
	{
		const CVec2& center = groupUnits.GetEl( i )->GetCenterPlain();

		fMinX = (std::min)( fMinX, center.x );
		fMaxX = (std::max)( fMaxX, center.x );
		fMinY = (std::min)( fMinY, center.y );
		fMaxY = (std::max)( fMaxY, center.y );
	}

	int numRows = ceil( ( fMaxX - fMinX ) / SConsts::GROUP_DISTANCE );
	int numColumns = ceil( ( fMaxY - fMinY ) / SConsts::GROUP_DISTANCE );
	numRows = (std::max)( numRows, 1 );
	numColumns = (std::max)( numColumns, 1 );
	std::vector<CVec2> centers( numRows * numColumns );
	std::vector<int> nums( numRows * numColumns );

	memset( &(centers[0]), 0, centers.size() * sizeof( SVector ) );
	memset( &(nums[0]), 0, nums.size() * sizeof( int ) );

	for ( int i = groupUnits.begin( nGroup ); i != groupUnits.end(); i = groupUnits.GetNext( i ) )
	{
		CCommonUnit *pUnit = groupUnits.GetEl( i );

		const int nX = ( pUnit->GetCenter().x - fMinX ) / SConsts::GROUP_DISTANCE;
		const int nY = ( pUnit->GetCenter().y - fMinY ) / SConsts::GROUP_DISTANCE;

		const int nSubGroup = nY * numRows + nX;
		pUnit->nSubGroup = nSubGroup;
		centers[nSubGroup] += pUnit->GetCenterPlain();
		++nums[nSubGroup];
	}

	for ( int i = 0; i != centers.size(); ++i )
	{
		if ( nums[i] != 0 )
			centers[i] /= nums[i];
	}

	for ( int i = groupUnits.begin( nGroup ); i != groupUnits.end(); i = groupUnits.GetNext( i ) )
	{
		CCommonUnit *pUnit = groupUnits.GetEl( i );
		pUnit->vShift = pUnit->GetCenterPlain() - centers[pUnit->nSubGroup];
		if ( command.nCmdType == ACTION_COMMAND_MOVE_TO || command.nCmdType == ACTION_COMMAND_SWARM_TO )
		{
			const CVec2 vAngle( NMath::Cos( command.fNumber ), NMath::Sin( command.fNumber ) );
			pUnit->vShift ^= vAngle;
		}
	}
}

void CGroupLogic::CreateSpecialGroup( const WORD wGroup )
{
	if ( wGroup != 0 )
	{
		const WORD wSpecialGroup = GetSpecialGroupNumberByID( groupIds.Get() );
		registeredGroups.insert( wSpecialGroup );

		if ( wSpecialGroup >= groupUnits.GetQueuesNum() )
			groupUnits.IncreaseQueuesNum( wSpecialGroup * 1.5 );

		int nLen = 0;
		for ( int i = groupUnits.begin( wGroup ); i != groupUnits.end(); i = groupUnits.GetNext( i ) )
		{
			CCommonUnit *pUnit = groupUnits.GetEl( i );

			if ( pUnit->IsFormation() )
			{
				CFormation *pForm = checked_cast<CFormation *>(pUnit);
				int nSold = pForm->Size();
				for ( int i=0; i< nSold; ++i )
				{
					CSoldier *pSold = (*pForm)[i];

					DelUnitFromSpecialGroup( pSold );

					pSold->nSpecialGroup = wSpecialGroup;
					pSold->nSpecialPos = groupUnits.Push( wSpecialGroup, pSold );
					++nLen;
				}
			}
			else
			{
				DelUnitFromSpecialGroup( pUnit );

				pUnit->nSpecialGroup = wSpecialGroup;
				pUnit->nSpecialPos = groupUnits.Push( wSpecialGroup, pUnit );
				++nLen;
			}
		}
	}
}

void CGroupLogic::EraseFromAmbushGroups( const SAIUnitCmd &command, const WORD wGroup )
{
	if ( !command.bFromAI || command.nCmdType == ACTION_COMMAND_AMBUSH )
	{
		for ( int i = groupUnits.begin( wGroup ); i != groupUnits.end(); i = groupUnits.GetNext( i ) )
		{
			CCommonUnit *pUnit = groupUnits.GetEl( i );
			ambushUnits.erase( pUnit->GetUniqueId() );
		}
	}
}

void CGroupLogic::CreateAmbushGroup( const WORD wGroup )
{
	ambushGroups.emplace_front();
	for ( int i = groupUnits.begin( wGroup ); i != groupUnits.end(); i = groupUnits.GetNext( i ) )
	{
		CCommonUnit *pUnit = groupUnits.GetEl( i );
		const int nUniqueId = pUnit->GetUniqueId();
		ambushGroups.front().push_back( SAmbushInfo( nUniqueId ) );
		ambushUnits.insert( nUniqueId );
	}
}

void CGroupLogic::SetToAmbush( CAmbushGroups::iterator &iter )
{
	std::list< std::pair<CCommonUnit*, int> > oldUnitsGroups;
	
	std::vector<int> objects;
	int nLen = 0;
	for ( std::list<SAmbushInfo>::iterator innerIter = iter->begin(); innerIter != iter->end(); ++innerIter )
	{
		CCommonUnit *pUnit = checked_cast<CCommonUnit*>( CLinkObject::GetObjectByUniqueIdSafe( innerIter->nUniqueId ) );
		innerIter->bGivenCommandToRestore = false;
		objects.push_back( pUnit->GetUniqueId() );

		oldUnitsGroups.push_back( std::pair<CCommonUnit*, int>( pUnit, pUnit->GetNGroup() ) );
	}

	const int nGroup = GetSpecialGroupNumberByID( groupIds.Get() );
	Singleton<IAILogic>()->RegisterGroup( objects, nGroup );

	SAIUnitCmd command( ACTION_COMMAND_AMBUSH );
	Singleton<IAILogic>()->GroupCommand( &command, nGroup, false );
	Singleton<IAILogic>()->UnregisterGroup( nGroup );

	for ( std::list< std::pair<CCommonUnit*, int> >::iterator iter = oldUnitsGroups.begin(); iter != oldUnitsGroups.end(); ++iter )
		theGroupLogic.AddUnitToGroup( iter->first, iter->second );
}

void CGroupLogic::UnitSetToAmbush( CCommonUnit *pUnit )
{
	for ( CAmbushGroups::iterator iter = ambushGroups.begin(); iter != ambushGroups.end(); ++iter )
	{
		for ( std::list<SAmbushInfo>::iterator innerIter = iter->begin(); innerIter != iter->end(); ++innerIter )
		{
			const int nUniqueID = innerIter->nUniqueId;
			CLinkObject *pObject = CLinkObject::GetObjectByUniqueIdSafe( nUniqueID );

			if ( pObject == pUnit )
			{
				innerIter->vAmbushCenter = pUnit->GetCenterPlain();
				innerIter->wAmbushDir = pUnit->GetDirection();
			}
		}
	}
}

void CGroupLogic::ProcessAmbushGroups()
{
	if ( lastAmbushCheck + 5000 < curTime )
	{
		lastAmbushCheck = curTime;
		std::unordered_set<int> checkedUnits;

		CAmbushGroups::iterator iter = ambushGroups.begin();
		while ( iter != ambushGroups.end() )
		{
			bool bCanSetToAmbush = true;
			std::list<SAmbushInfo>::iterator innerIter = iter->begin();
			while ( innerIter != iter->end() )
			{
				const int nUniqueId = innerIter->nUniqueId;
				// уже в другой ambush группе, or deleted from ambush groups, удалить из этой
				if ( checkedUnits.find( nUniqueId ) != checkedUnits.end() || 
						 ambushUnits.find( nUniqueId ) == ambushUnits.end() )
					innerIter = iter->erase( innerIter );
				else
				{
					// юнит просмотрен
					checkedUnits.insert( nUniqueId );

					CLinkObject *pObject = CLinkObject::GetObjectByUniqueIdSafe( nUniqueId );
					// юнит умер, удалить из ambush groups
					if ( !IsValidObj( pObject ) )
					{
						ambushUnits.erase( nUniqueId );
						innerIter = iter->erase( innerIter );
					}
					else
					{
						// unit alive, check if can be set to ambush
						CCommonUnit *pUnit = checked_cast<CCommonUnit*>( pObject );

						if ( pUnit->GetState() && innerIter->vAmbushCenter.x != -1.0f )
						{
							const bool bRest = 
								pUnit->GetState()->IsRestState() && pUnit->IsIdle() &&
								( !pUnit->IsFormation() || checked_cast<CFormation*>(pUnit)->IsEveryUnitResting() );

							if ( bRest )
							{
								if ( !innerIter->bGivenCommandToRestore &&
										 ( fabs2( pUnit->GetCenterPlain() - innerIter->vAmbushCenter ) >= sqr( SConsts::TILE_SIZE * 2 ) ||
											 DirsDifference( pUnit->GetFrontDirection(), innerIter->wAmbushDir ) >= 3000 ) )
								{
									innerIter->bGivenCommandToRestore = true;
									UnitCommand( SAIUnitCmd( ACTION_COMMAND_SWARM_TO, innerIter->vAmbushCenter , 0 ), pUnit, false );
									UnitCommand( SAIUnitCmd( ACTION_COMMAND_ROTATE_TO_DIR, GetVectorByDirection( innerIter->wAmbushDir ) ), pUnit, true );

									bCanSetToAmbush = false;
								}
								else
								{
									const int nParty = pUnit->GetParty();
									bCanSetToAmbush = 
										bCanSetToAmbush && ( pUnit->GetLastChangeStateTime() + 5000 < curTime ) &&
										nParty < 2 && !pUnit->IsVisible( 1 - nParty );
								}
							}
							else
								bCanSetToAmbush = false;
						}
						else
							bCanSetToAmbush = false;

						++innerIter;
					}
				}
			}

			if ( bCanSetToAmbush )
			{
				// group isn't empty, set to ambush
				if ( !iter->empty() )
				{
					SetToAmbush( iter );
					++iter;
				}
				else
					// group is empty, erase from ambush groups
					iter = ambushGroups.erase( iter );
			}
			else
				++iter;
		}
	}
}

void CGroupLogic::GroupCommand( const SAIUnitCmd &command, const WORD wGroup, bool bPlaceInQueue )
{
	theCommandTrackerForScript.Called( command.nCmdType );

	/*Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_DEBUG_WINDOW, 
		StrFmt( "CMD(time %d): ID = %d, (%.2f, %.2f), %d, %.2f", curTime, command.nCmdType, command.vPos.x, command.vPos.y, 
		command.nObjectID, command.fNumber ) );
	Singleton<IGameTimer>()->Pause( true, 0 );*/

	// т.к. часть юнитов может исчезнуть, например, когда один из игроков в multiplayer вышел
 	if ( registeredGroups.find( wGroup ) != registeredGroups.end() )
	{
		// пойти с сохранением относительной позиции
		if ( command.nCmdType == ACTION_COMMAND_MOVE_TO || command.nCmdType == ACTION_COMMAND_SWARM_TO || 
					command.nCmdType == ACTION_COMMAND_PLACEMINE || command.nCmdType == ACTION_COMMAND_CLEARMINE || 
					command.nCmdType == ACTION_COMMAND_DEPLOY_ARTILLERY || command.nCmdType == ACTION_COMMAND_UNLOAD ||
					command.nCmdType == ACTION_COMMAND_RESUPPLY_HR || command.nCmdType == ACTION_COMMAND_RESUPPLY ||
					command.nCmdType == ACTION_COMMAND_REPAIR )
			DivideBySubGroups2( command, wGroup );
		else
		{
			for ( int i = groupUnits.begin( wGroup ); i != groupUnits.end(); i = groupUnits.GetNext( i ) )
			{
				groupUnits.GetEl( i )->nSubGroup = -1;
				groupUnits.GetEl( i )->vShift = VNULL2;
			}
		}
		//EraseFromAmbushGroups( command, wGroup );

		/*if ( command.nCmdType == ACTION_COMMAND_AMBUSH )
		{
			CreateSpecialGroup( wGroup );
			CreateAmbushGroup( wGroup );
		}*/
		
		std::vector<CCommonUnit*> groups( groupUnits.GetSize( wGroup ) );

		int nGroupsIter = 0;
		for ( int i = groupUnits.begin( wGroup ); i != groupUnits.end(); i = groupUnits.GetNext( i ) )
			groups[nGroupsIter++] = groupUnits.GetEl( i );
/*
		if ( command.nCmdType == ACTION_COMMAND_SWARM_TO )
		{
			string str = "ACTION_COMMAND_SWARM_TO: ";
			if ( groups.size() == 0 )
				str += "empty group !!!";
			else
			{
				str += StrFmt( "%d", groups[0]->GetUniqueID() );
				for ( int i = 1; i < groups.size(); ++i )
					str += StrFmt( ", %d", groups[i]->GetUniqueID() );
			}
			DebugTrace( str.c_str() );
		}
*/
		// этот рандом вызывать нужно независимо от command.bFromAI - для правильной работы 
		// мультиплера
		if ( groupUnits.GetSize(wGroup) != 0 )
		{
			int nRandom = NRandom::Random( groupUnits.GetSize(wGroup) );
			if ( !command.bFromAI )
			{
				// выбрать по рандому юнит, который будет говорить аск на подтверждение команды
				groups[nRandom]->SendAcknowledgement( ACK_POSITIVE, true );
			}
		}

		if ( command.nCmdType == ACTION_COMMAND_MOVE_TO_GRID )
			ProcessGridCommand( command.vPos, CVec2( NMath::Cos(command.fNumber), NMath::Sin(command.fNumber) ), wGroup, bPlaceInQueue );
		else
		{
			CPtr<CAICommand> pCommand = new CAICommand( command );
			for ( std::vector<CCommonUnit*>::iterator iter = groups.begin(); iter != groups.end(); ++iter )
			{
				CCommonUnit *pUnit = *iter;
				pCommand->AddUnit( pUnit );
			}

			//выравнивание скорости только для команд ACTION_COMMAND_MOVE_TO и ACTION_COMMAND_SWARM_TO
			if ( NGlobal::GetVar( "adjust_group_speed", 0 ) )
			{
				float fDesGroupSpeed = -1.0f;
				if ( command.nCmdType == ACTION_COMMAND_MOVE_TO || command.nCmdType == ACTION_COMMAND_SWARM_TO )
				{
					fDesGroupSpeed = FP_MAX_VALUE;
					for ( std::vector<CCommonUnit*>::iterator iter = groups.begin(); iter != groups.end(); ++iter )
					{
						CCommonUnit *pUnit = *iter;

						if ( pUnit->CanMove() && pUnit->GetMaxPossibleSpeed() > 0.0f && pUnit->GetMaxPossibleSpeed() < fDesGroupSpeed )
							fDesGroupSpeed = pUnit->GetMaxPossibleSpeed();
					}
				}
				for ( std::vector<CCommonUnit*>::iterator iter = groups.begin(); iter != groups.end(); ++iter )
          (*iter)->SetDesiredSpeed( fDesGroupSpeed );
			}

			std::unordered_set<int> memFormationIDs;
			//DebugTrace( "Command from AI: %s", pCommand->IsFromAI() ? "true" : "false" );
			for ( std::vector<CCommonUnit*>::iterator iter = groups.begin(); iter != groups.end(); ++iter )
			{
				CCommonUnit *pUnit = *iter;

				bool bSendCommand = true;
				if ( command.nCmdType == ACTION_COMMAND_FORM_FORMATION && pUnit->IsFormation() )
				{
					CFormation *pFormation = checked_cast<CFormation*>( pUnit );
					if ( pFormation->Size() == 1 && (*pFormation)[0]->GetMemFormation() != 0 )
					{
						const int nMemFormationID = (*pFormation)[0]->GetMemFormation()->GetUniqueId();
						if ( memFormationIDs.find( nMemFormationID ) == memFormationIDs.end() )
							memFormationIDs.insert( nMemFormationID );
						else
							bSendCommand = false;
					}
				}
				
				if ( bSendCommand )
					pUnit->TryExecuteCommand( pCommand, bPlaceInQueue, false );
			}
		}
	}
}

void CGroupLogic::UnitCommand( const SAIUnitCmd &command, CCommonUnit *pGroupUnit, bool bPlaceInQueue  )
{
	if ( pGroupUnit->IsAlive() )
	{
		if ( command.nCmdType == ACTION_COMMAND_DIE )
			pGroupUnit->Die( command.bFromExplosion, command.fNumber );
		else
		{
			CAICommand *pCommand = new CAICommand( command );
			pGroupUnit->UnitCommand( pCommand, bPlaceInQueue, true );
		}
	}
}

void CGroupLogic::InsertUnitCommand( const SAIUnitCmd &command, CCommonUnit *pUnit )
{
	CAICommand *pCmd = new CAICommand( command );
	pUnit->InsertUnitCommand( pCmd );
}

void CGroupLogic::PushFrontUnitCommand( const SAIUnitCmd &command, CCommonUnit *pUnit  )
{
	CAICommand *pCmd = new CAICommand( command );
	pUnit->PushFrontUnitCommand( pCmd );
}

void CGroupLogic::SegmentFollowingUnits()
{
	while ( !followingUnits.empty() )
	{
		CCommonUnit *pUnit = followingUnits.front();
		CCommonUnit *pHeadUnit = pUnit->GetFollowedUnit();
		followingUnits.pop_front();

		const float fHeadUnitSpeed = pHeadUnit->GetSpeedForFollowing();
		if ( fHeadUnitSpeed != 0.0f )
			pUnit->SetDesirableSpeed( fHeadUnitSpeed );
	}
}

void CGroupLogic::StayTimeSegment()
{
	int nCntNotRestUnits = 0;
	
	for ( CGlobalIter iter( 0, ANY_PARTY ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pUnit = (*iter);

		if ( pUnit->IsRefValid() )
			pUnit->AnimationSegment();
	}
}

void CGroupLogic::Segment() 
{
	updater.ClearInterpolatable();

	const NTimer::STime roundedCurTime = curTime - curTime % SConsts::AI_SEGMENT_DURATION;
	_control87( _RC_NEAR, _MCW_RC );

	// states юнитов
	NSegmObjs::Segment( lastSegmTime, roundedCurTime, segmUnits[0], (CStateSegments*)0 );
	NSegmObjs::Segment( lastSegmTime, roundedCurTime, segmUnits[1], (CStateSegments*)0 );
	NSegmObjs::Segment( lastSegmTime, roundedCurTime, freezeUnits, (CFreezeSegments*)0 );

	// взрывы
	theShellsStore.Segment();

	// скорости юнитов в follow
	SegmentFollowingUnits();

	// поиск коллизий
	NSegmObjs::Segment( lastSegmTime, roundedCurTime, firstPathUnits, (CFirstPathSegments*)0 );
	theScanLimiter.SegmentsFinished();

	// обработка застрявших из-за коллизий юнитов (это обрабатывается в CBasePathUnit::FirstSegment)
	// NSegmObjs::SegmentWOMove( lastSegmTime, roundedCurTime, secondPathUnits, (CStayTimeSegments*)0 );
	// нефига перебирать каждый юнит, будем вызывать в сегменте самого юнита
	//StayTimeSegment();

	// обработка коллизий
	pCollisionsCollector->HandOutCollisions( GetAIMap() );

	// движение юнитов вдоль пути
	// inside CSecondPathSegments there is Float2Int is used, so we have to set processor control word
	NSegmObjs::Segment( lastSegmTime, roundedCurTime, secondPathUnits, (CSecondPathSegments*)0 );

	lastSegmTime = curTime - curTime % SConsts::AI_SEGMENT_DURATION + SConsts::AI_SEGMENT_DURATION;

	ProcessAmbushGroups();
	NI_ASSERT( (_MCW_RC  & _control87( 0, 0 )) == 0, "something changed processor control word" );
}

void CGroupLogic::UpdateAllAreas( const std::vector<int> &units, const EActionNotify eAction )
{
	for ( int i = 0; i < units.size(); ++i )
	{
		CCommonUnit *pUnit = dynamic_cast<CCommonUnit*>( CLinkObject::GetObjectByUniqueIdSafe( units[i] ) );
		if ( pUnit && pUnit->IsRefValid() )
			pUnit->UpdateArea( eAction );
	}
}

void CGroupLogic::RegisterSegments( CCommonUnit *pUnit, const bool bInitialization, bool bAllInfo )
{
	segmUnits[pUnit->IsInfantry()].RegisterSegments( pUnit, bInitialization );
	if ( bAllInfo )
		freezeUnits.RegisterSegments( pUnit, bInitialization );
}

void CGroupLogic::RegisterPathSegments( CAIUnit *pUnit, const bool bInitialization )
{
	firstPathUnits.RegisterSegments( pUnit, bInitialization );
	secondPathUnits.RegisterSegments( pUnit, bInitialization );
}

void CGroupLogic::UnregisterSegments( CCommonUnit *pUnit )
{
	segmUnits[pUnit->IsInfantry()].UnregisterSegments( pUnit );
}

void CGroupLogic::UnregisterPathSegments( CAIUnit *pUnit )
{
	firstPathUnits.UnregisterSegments( pUnit );
	secondPathUnits.UnregisterSegments( pUnit );
}

const CVec2 GetGoPointByCommand( const SAIUnitCmd &cmd )
{
	// -1 - нет точки, 0 - cmd.vPos, 1 - центр юнита GetObjectByCmd( cmd ), 2 - центр статич. объекта GetObjectByCmd( cmd )
	int nType = -1;

	switch ( cmd.nCmdType )
	{
		case ACTION_COMMAND_MOVE_TO:						nType =  0; break;
		case ACTION_COMMAND_ATTACK_UNIT:				nType =  1; break;
		case ACTION_COMMAND_ATTACK_OBJECT:			nType =  2; break;
		case ACTION_COMMAND_SWARM_TO:						nType =  0; break;
		case ACTION_COMMAND_LOAD:								nType =  1; break;
		case ACTION_COMMAND_UNLOAD:							nType = -1; break;
		case ACTION_COMMAND_ENTER:							nType =  2; break;
		case ACTION_COMMAND_LEAVE:							nType =  0; break;
		case ACTION_COMMAND_ROTATE_TO:					nType = -1; break;
		case ACTION_COMMAND_ROTATE_TO_DIR:			nType = -1; break;
		case ACTION_COMMAND_STOP:								nType = -1; break;
		case ACTION_COMMAND_PARADE:							nType = -1; break;
		case ACTION_COMMAND_PLACEMINE:					nType =  0; break;
		case ACTION_COMMAND_CLEARMINE:					nType =  0; break;
		case ACTION_COMMAND_GUARD:							nType =  0; break;
		case ACTION_COMMAND_AMBUSH:							nType = -1; break;
		case ACTION_COMMAND_RANGE_AREA:					nType = -1; break;
		case ACTION_COMMAND_ART_BOMBARDMENT:		nType = -1; break;
		case ACTION_COMMAND_INSTALL:						nType = -1; break;
		case ACTION_COMMAND_UNINSTALL:					nType = -1; break;
		case ACTION_COMMAND_RESUPPLY:						nType = -1; break;
		case ACTION_COMMAND_REPAIR:							nType = -1; break;
		case ACTION_COMMAND_BUILD_FENCE_BEGIN:	nType =  0; break;
		case ACTION_COMMAND_ENTRENCH_BEGIN:			nType =  0; break;
		case ACTION_COMMAND_CATCH_ARTILLERY:		nType =  1; break;
		case ACTION_COMMAND_USE_SPYGLASS:				nType = -1; break;
		case ACTION_COMMAND_TAKE_ARTILLERY:			nType = -1; break;
		case ACTION_COMMAND_DEPLOY_ARTILLERY:		nType = -1; break;
		case ACTION_COMMAND_PLACE_ANTITANK:			nType =  0; break;
		case ACTION_COMMAND_DISBAND_FORMATION:	nType = -1; break;
		case ACTION_COMMAND_FORM_FORMATION:			nType = -1; break;
	}

	CVec2 vResult( -1.0f, -1.0f );
	switch ( nType )
	{
		case -1:
			break;
		case 0:
			vResult = cmd.vPos;
			break;
		case 1:
			NI_ASSERT( dynamic_cast<CCommonUnit*>(GetObjectByCmd( cmd )) != 0, StrFmt( "Non-compliance for command %d", cmd.nCmdType ) );
			vResult = checked_cast<CCommonUnit*>(GetObjectByCmd( cmd ))->GetCenterPlain();
			break;
		case 2:
			{
				NI_ASSERT( dynamic_cast<CStaticObject*>(GetObjectByCmd( cmd )) != 0, StrFmt( "Non-compliance for command %d", cmd.nCmdType ) );
				CStaticObject* pObj = dynamic_cast<CStaticObject*>(GetObjectByCmd( cmd ));
				if ( pObj->GetObjectType() == ESOT_BUILDING )
				{
					CBuilding *pBuilding = checked_cast<CBuilding*>(pObj);
					if ( pBuilding->GetNEntrancePoints() > 0 )
						vResult = pBuilding->GetEntrancePoint( 0 );
					else
						vResult = CVec2(pObj->GetCenter().x,pObj->GetCenter().y);
				}
				else
					vResult = CVec2(pObj->GetCenter().x,pObj->GetCenter().y);
			}

			break;
	}

	return vResult;
}

void CGroupLogic::AddFollowingUnit( CCommonUnit *pUnit )
{
	followingUnits.push_back( pUnit );
}

void CGroupLogic::ProcessGridCommand( const CVec2 &vGridCenter, const CVec2 &vGridDir, const int nGroup, bool bPlaceInQueue )
{
	CGrid grid( vGridCenter, nGroup, vGridDir );

	SAIUnitCmd moveToCmd( ACTION_COMMAND_MOVE_TO_GRID, VNULL2, GetDirectionByVector( vGridDir ) );
	for ( int i = 0; i < grid.GetNUnitsInGrid(); ++i )
	{
		CCommonUnit *pUnit = grid.GetUnit( i );

		if ( pUnit->GetBehaviourMoving() == SBehaviour::EMHoldPos )
			pUnit->SetBehaviourMoving( SBehaviour::EMRoaming );

		moveToCmd.vPos = grid.GetUnitCenter( i );
		UnitCommand( moveToCmd, pUnit, bPlaceInQueue );
	}
}
#include "Common_RTS_AI/CheckSums.h"
#include "SimpleChecksumCalc.h"

void CGroupLogic::GetCheckSum( unsigned long *ulChecksum )
{
	for ( int n = 0; n < NSegmObjs::SEGM_UNITS_SIZE; ++n )
	{
		for ( int i = segmUnits[1].begin( n ); i != segmUnits[1].end(); i = segmUnits[1].GetNext( i ) )
		{
			CCommonUnit * pUnit = segmUnits[1].GetEl( i );
			int id = pUnit->GetUniqueId();
			*ulChecksum = adler32( *ulChecksum, (BYTE*)&id, 4 );
		}
	}
}


void CGroupLogic::UpdateDebugChecksums(FILE* f)
{
	fprintf(f, "GroupSegmentUnits:\n");
	uLong baseChecksum = 555111;
	for ( int n = 0; n < NSegmObjs::SEGM_UNITS_SIZE; ++n )
	{
		for ( int i = segmUnits[1].begin( n ); i != segmUnits[1].end(); i = segmUnits[1].GetNext( i ) )
		{
			CCommonUnit * pUnit = segmUnits[1].GetEl( i );
			int id = pUnit->GetUniqueId();
			CVec2 pos = pUnit->GetCenterPlain();
			WORD dir = pUnit->GetDirection();
			
			BYTE player = pUnit->GetPlayer();
			uLong checksum = CalculateChecksum(baseChecksum, id, pos.x, pos.y, dir);

			fprintf(f, "\tPlayer[%d] SegmUnit[%d]: %lu\n", (int)player, pUnit->GetUniqueID(), checksum);
		}
	}
	fprintf(f, "\n");
}

