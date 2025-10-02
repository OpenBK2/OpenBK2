#include "StdAfx.h"

#include "SinglePlayerTransceiver.h"

#include "AILogic/AILogicCommand.h"
#include "AILogic/B2AI.h"
#include "Input/Bind.h"
#include "ScenarioTracker.h"

#include "SceneB2/Scene.h"

// CSinglePlayerTransceiver

CSinglePlayerTransceiver::CSinglePlayerTransceiver()
: bCommandsFromHistory( false )
{
	nAdjustedGameSpeed = 0;
	nGameSpeed = 0;
	nStartCountingTime = 0;
	nFrames = 0;
}

CSinglePlayerTransceiver::CSinglePlayerTransceiver( ICommandsHistory *_pCommandHistory, IAILogic *_pAI )
: bCommandsFromHistory( false )
{
	pCmdsHistory = _pCommandHistory;
	pAI = _pAI;
	nAdjustedGameSpeed = Singleton<IGameTimer>()->GetSpeed();
	nGameSpeed = nAdjustedGameSpeed;
	nStartCountingTime = Singleton<IGameTimer>()->GetAbsTime();
	nFrames = 0;
}

void CSinglePlayerTransceiver::StartMission( const NDb::SMapInfo *_pMap, IAILogic *_pAI )
{
	pAI = _pAI;
	//
	NGlobal::RemoveVar( "MultiplayerGame" );	
	// reset timer
	Scene()->ResetTimer( GetTickCount() );

	nCommonSegment = 0;
	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	if ( !pST || pST->GetGameType() == IAIScenarioTracker::EGT_SINGLE ) 
		pAI->SetNetGame( false );
	else
		pAI->SetNetGame( true );

	pCmdsHistory->StartNewGame( _pMap );
}

void CSinglePlayerTransceiver::AdjustGameSpeed( const int nDelta )
{
	IGameTimer *pTimer = Singleton<IGameTimer>();
	const int nSpeedLimit = NGlobal::GetVar( "AI_SLOWDOWN_SPEED_LIMIT", 2 );
	nAdjustedGameSpeed = Clamp( nAdjustedGameSpeed + nDelta, nGameSpeed - nSpeedLimit, nGameSpeed );
	pTimer->SetSpeed( nAdjustedGameSpeed );
	nAdjustedGameSpeed = pTimer->GetSpeed();
}

// perform segments for AI
void CSinglePlayerTransceiver::DoSegments()
{
	// just skip all net chat in single-player mode
	IConsoleBuffer *pBuffer = Singleton<IConsoleBuffer>();
	wstring szString;
	while ( ReadFromPipe( PIPE_NET_CHAT, &szString, 0 ) )
	{
		DebugTrace( "Got type: %S\n", szString.c_str() );
		if ( ReadFromPipe( PIPE_NET_CHAT, &szString, 0 ) )
			DebugTrace( "Got msg: %S\n", szString.c_str() );
		else
			DebugTrace( "Error: no msg" );
	}
	// segment calling through segment timer
	IGameTimer *pTimer = Singleton<IGameTimer>();

	if ( NGlobal::GetVar( "AI_SLOWDOWN", 0 ) == 1 )
	{
		const int nCurrentGameSpeed = pTimer->GetSpeed();	
		const int nUserSpeedDelta = nCurrentGameSpeed - nAdjustedGameSpeed;
		const NTimer::STime nCurTime = pTimer->GetAbsTime();
		if ( nFrames == 0 )
		{
			nStartCountingTime = nCurTime;
		}
		++nFrames;
		if ( nUserSpeedDelta != 0 )
		{
			nGameSpeed = Clamp( nGameSpeed + nUserSpeedDelta, pTimer->GetMinSpeed(), pTimer->GetMaxSpeed() );
			nAdjustedGameSpeed = nCurrentGameSpeed;
		}
		else
		{
			const int nFPSCountInterval = NGlobal::GetVar( "AI_SLOWDOWN_FPS_COUNT_INTERVAL", 500 );
			if ( nCurTime > nStartCountingTime + nFPSCountInterval )
			{
				const float fFPS = float( nFrames - 1 ) / ( nCurTime - nStartCountingTime ) * 1000;
				const float fSlowdownFPS = NGlobal::GetVar( "AI_SLOWDOWN_FPS", 15.0f );
				if ( fFPS > fSlowdownFPS )
					AdjustGameSpeed( +1 );
				else
					AdjustGameSpeed( -1 );
				nFrames = 0;
			}
		}
	}

	// no segments to do
	if ( !pTimer->BeginSegments() )
		return;

	if ( nCommonSegment > 14 )
		pAI->NetGameStarted();

	while ( pTimer->NextSegment() )
	{
		if ( !pAI->IsSuspended() )
		{
			bCommandsFromHistory = true;
			pCmdsHistory->ExecuteSegmentCommands( nCommonSegment, this );			
			bCommandsFromHistory = false;
			pAI->Segment();
			++nCommonSegment;
		}
	}
}

void CSinglePlayerTransceiver::SendCommand( IAILogicCommandB2 *pCmd )
{
	pCmdsHistory->AddCommand( nCommonSegment, pCmd );

	if ( NGlobal::GetVar( "History.Playing", 0 ) == 0 || bCommandsFromHistory )
		pCmd->Execute();
}

void CSinglePlayerTransceiver::CommandClientTogglePause() 
{ 
	NInput::PostEvent( "toggle_pause", 0, 0 );
}

void CSinglePlayerTransceiver::CommandClientSpeed( const int nChange ) 
{ 
	NInput::PostEvent( "game_speed", nChange, 0 );
}

const NDb::SMapInfo *CSinglePlayerTransceiver::GetMap() const 
{
	return pCmdsHistory->GetMap(); 
}

ITransceiver *CreateSinglePlayerTransceiver( const SReplayInfo &replay, IAILogic *pAI )
{
	ICommandsHistory *pHistory = CreateCommandsHistory( replay );
	return new CSinglePlayerTransceiver( pHistory, pAI );
}

BASIC_REGISTER_CLASS( IAILogicCommandB2 )
REGISTER_SAVELOAD_CLASS( 0x1007AB40, CSinglePlayerTransceiver )

