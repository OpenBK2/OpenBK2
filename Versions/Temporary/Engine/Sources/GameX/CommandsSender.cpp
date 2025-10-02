#include "stdafx.h"

#include "CommandsSender.h"
#include "AILogicCommandInternal.h"
#include "../AILogic/B2AI.h"
#include "Transceiver.h"
#include "../Main/MainLoop.h"
#include "ScenarioTracker.h"

#ifndef _FINALRELEASE
static int AUTOSAVE_ID = 0;
#endif

const int MIN_DISTANCE_BETWEEN_DIFFERENT_COMMANDS = 300;
const int MIN_TIME_BETWEEN_DIFFERENT_COMMANDS = 1000;

CCommandsSender::CCommandsSender( ITransceiver *_pTransciver ) : nLastGroupID( -1 )  
{
	pTransciver = _pTransciver;
	bHistoryPlaying = false;
	lastCommandTime = Singleton<IGameTimer>()->GetAbsTime();
	bGroupChanged = true;
	bLastCommandSkipped = false;
}

// register group of units to AI
int CCommandsSender::CommandRegisterGroup( const vector<int> &_vIDs )
{
	if ( !bHistoryPlaying )
	{
		vector<int> vIDs = _vIDs;
		sort( vIDs.begin(), vIDs.end() );

		if ( !Singleton<IAILogic>()->NeedNewGroupNumber() && vIDs == lastGroup )
			return nLastGroupID;
		
		pTransciver->SendCommand( new CUnregisterGroupCommand( nLastGroupID ) );
		const int nID = Singleton<IAILogic>()->GenerateGroupNumber();
		Singleton<IAILogic>()->ResetNeedNewGroupNumber();
		pTransciver->SendCommand( new CRegisterGroupCommand( vIDs, nID ) );

		lastGroup.swap( vIDs );
		nLastGroupID = nID;
		bGroupChanged = true; 

		return nID;
	}
	else
		return 0;
}

// unregister group 
void CCommandsSender::CommandUnregisterGroup( const WORD wGroup )
{
//	if ( !bHistoryPlaying )
//		pTransciver->SendCommand( new CUnregisterGroupCommand( wGroup ) );
}

// send command to group of units
void CCommandsSender::CommandGroupCommand( const SAIUnitCmd *pCommand, const WORD wGroup, bool bPlaceInQueue, const int nCommandSaveID )
{
	if ( !bHistoryPlaying )
	{
		NTimer::STime curTime = Singleton<IGameTimer>()->GetAbsTime();

		if ( !bPlaceInQueue && !pCommand->bFromExplosion )
		{
			if ( !bGroupChanged && pCommand->nCmdType == lastCommand.nCmdType
				&& pCommand->nObjectID == lastCommand.nObjectID && pCommand->nNumber == lastCommand.nNumber
				&& pCommand->fNumber == lastCommand.fNumber )
			{
				if ( fabs( pCommand->vPos - lastCommand.vPos ) < MIN_DISTANCE_BETWEEN_DIFFERENT_COMMANDS 
					&& curTime - lastCommandTime < MIN_TIME_BETWEEN_DIFFERENT_COMMANDS )
				{
					bLastCommandSkipped = true;
					return;
				}
			}
			bGroupChanged = false;
			lastCommandTime = curTime;
			lastCommand = *pCommand;
		}
		bLastCommandSkipped = false;
		SAIUnitCmd cmd( *pCommand );
		cmd.bFromAI = false;

#ifndef _FINALRELEASE
		if ( NGlobal::GetVar( "m1", 0 ) == 0 && NGlobal::GetVar( "autosave_enable", 0 ) )
			NMainLoop::Command( nCommandSaveID, StrFmt( "autosave_%04d", ++AUTOSAVE_ID ) );
#endif

		IAILogicCommandB2 *pAICmd = new CB2GroupCommand( &cmd, wGroup, bPlaceInQueue );
		pTransciver->SendCommand( pAICmd );
	}
}

// set single command to call planes, reinforcements, etc. returns group number, which was created
int CCommandsSender::CommandUnitCommand( const SAIUnitCmd *pCommand )
{
	if ( !bHistoryPlaying )
	{
		const int nID = Singleton<IAILogic>()->GenerateGroupNumber();
		SAIUnitCmd cmd( *pCommand );
		cmd.bFromAI = false;
		IAILogicCommandB2 *pAICmd = new CUnitCommand( &cmd, nID, Singleton<IScenarioTracker>()->GetLocalPlayer() );

		pTransciver->SendCommand( pAICmd );

		return nID;
	}
	else
		return 0;
}

void CCommandsSender::CommandGeneralCommand( const SAIUnitCmd *pCommand )
{
	if ( !bHistoryPlaying )
	{
		SAIUnitCmd cmd( *pCommand );
		cmd.bFromAI = false;
		IAILogicCommandB2 *pAICmd = new CUnitCommand( &cmd, 0, Singleton<IScenarioTracker>()->GetLocalPlayer() );

		pTransciver->SendCommand( pAICmd );
	}
}

void CCommandsSender::SendCommand( IAILogicCommandB2 *pCmd )
{
	pTransciver->SendCommand( pCmd );
}

REGISTER_SAVELOAD_CLASS( 0x3009E581, CCommandsSender );

