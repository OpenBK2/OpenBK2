#include "StdAfx.h"

#include "MPTransceiver.h"
#include "../Client/ServerClientInterface.h"
#include "../Server_Client_Common/GamePackets.h"
#include "../AILogic/B2AI.h"
#include "../Main/DBNetConsts.h"
#include "CommonCommands.h"
#include <float.h>
#include "../Main/AICmdsAutoMagicInterface.h"
#include "../Main/CommandsHistory.hpp"
#include "ScenarioTracker.h"
#include "SaveLoadHelper.h"
#include "InterfaceMissionInternal.h"
#include "DBGameRoot.h"
#include "DBConsts.h"
#include "InterfaceState.h"
#include "GameXClassIDs.h"
#include "MultiplayerCommandManager.h"
#include "../Misc/StrProc.h"
#include "../Main/MainLoop.h"
#include "../Main/MainLoopCommands.h"
#include "../System/GlobalVars.h"
#include "../System/Commands.h"
#include "GetConsts.h"
#include "InterfaceMisc.h"

#include "../SceneB2/Scene.h"

IAICmdsAutoMagic *CreateAICmdsAutoMagic();

static int s_nInterruptTimeout = 5000;		// No segments for this time before timeout is called

class CAISegmentFinishedPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CAISegmentFinishedPacket );
public:
	typedef unsigned long ulong;
	ZDATA
	int nSegment;
	ulong ulCheckSum;
	list<CMPTransceiver::SRawCommand> aiCommands;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nSegment); f.Add(3,&ulCheckSum); f.Add(4,&aiCommands); return 0; }

	CAISegmentFinishedPacket() : CNetPacket( 0 ) {}
};

class CTransciverCommonPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CTransciverCommonPacket );
public:
	enum EType
	{
		ET_LOADED,
	};

	ZDATA
		EType eType;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eType); return 0; }

	CTransciverCommonPacket() {}
	CTransciverCommonPacket( EType _eType ) : CNetPacket( 0 ), eType( _eType ) {}
};

REGISTER_SAVELOAD_CLASS( 111, CAISegmentFinishedPacket );
REGISTER_SAVELOAD_CLASS( 112, CTransciverCommonPacket );

// CMPTransceiver

CMPTransceiver::CMPTransceiver() : nGameSpeed( 0 )
{
	REGISTER_PACKET_PROCESSOR( &CMPTransceiver::OnTransciverCommonPacket );
	REGISTER_PACKET_PROCESSOR( &CMPTransceiver::OnAISegmentFinishedPacket );
}

void CMPTransceiver::StartMission()
{
	nLatency = 2;
	nSegmentsPackSize = nLatency / 2;
	nSegment = 0;
	nCommonSegment = 0;
	nFinalSegment = -1;
	pAI->NetGameStarted();
	Scene()->ResetTimer( GetTickCount() );
	pTimer->SetSpeed( nGameSpeed );
	pCmdsHistory->StartNewGame( pMapInfo );
	NMainLoop::Command( new CICMission( this ) );
	bWaiting = false;
}

void CMPTransceiver::SendCommand( IAILogicCommandB2 *pCommand )
{
	if ( !bIsGameRunning || bIsGameEnded )
		return;

	aiCommandsToSend.push_back( pCommand );
	//DebugTrace( "+++ Segment %d(%d) cmd from %d", nSegment, nCommonSegment, nMyLogicID );
}

void CMPTransceiver::PlayerRemoved( int nPlayer )
{
	//DebugTrace( "+++ Segment %d, removing player %d", nCommonSegment, nPlayer );
	if ( nPlayer < 0 || !IsPlayerPresent( nPlayer ) )
		return;

	wMask &= ~( 1UL << nPlayer );
	NInput::PostEvent( "mission_remove_player", nPlayer, 0 );
}

// perform segments for AI
void CMPTransceiver::DoSegments()
{
	_control87( _EM_INVALID | _EM_ZERODIVIDE | _EM_OVERFLOW | _EM_UNDERFLOW | _EM_INEXACT | _EM_DENORMAL | _PC_24, 0xfffff );
	pClient->Segment();
	if ( !IsGameRunning() )
	{
		CheckRunGame();
		return;
	}
	pTimer->BeginSegments();			//???this line does nothing.

	if ( bWaiting || bIsGameEnded )
		return;

	if ( nFinalSegment > 0 && nCommonSegment >= nFinalSegment )
	{
		EndGame();
	}

	while ( pTimer->CanStartNextSegment() )
	{
		if ( IsSegmentPackFinished() )
		{
			// прибавляется MAX_LATENCY, т.к. модуль от отрицательных чисел считается неправильно
			const int nPastSegment = ( nSegment - nLatency + pConsts->nMaxLatency ) % pConsts->nMaxLatency;
			if ( nCommonSegment < nLatency || IsSegmentFinishedByAll( nPastSegment ) )
			{
				SetLagState( nPastSegment, false );

				bCommandsFromHistory = true;
				pCmdsHistory->ExecuteSegmentCommands( nCommonSegment, this );
				bCommandsFromHistory = false;

				ExecuteCommands( nPastSegment );
				FinalizeSegmentPack();
				if ( IsAsyncDetected( nPastSegment ) )
				{
					return;
				}
				AdvanceToNextSegment();
				segmFinished[nPastSegment] = 0;
			}
			else
			{
				SetLagState( nPastSegment, true );
				return;
			}
		}
		else
			AdvanceToNextSegment();
	}
}

void CMPTransceiver::ExecuteCommands( int nFromSegment )
{
	_control87( _EM_INVALID | _EM_ZERODIVIDE | _EM_OVERFLOW | _EM_UNDERFLOW | _EM_INEXACT | _EM_DENORMAL | _PC_24, 0xfffff );
	bool bHasCommands = false;
	for ( int i = 0; i < cmds.GetSizeX(); ++i )
	{
		if ( IsPlayerPresent( i ) )			// Only for existing players
		{
			//DebugTrace( "+++ Execute Plr %d, %d cmds", i,  cmds[nFromSegment][i].size() );
			for ( CAILogicCommandsList::iterator it = cmds[nFromSegment][i].begin(); it != cmds[nFromSegment][i].end(); ++it )
			{
				IAILogicCommandB2 *pCmd = *it;
				pCmd->Execute();
				//DebugTrace( "+++ Executed cmd plr %d: %s", i, pCmd->GetDebugInfo().c_str() );
				pCmdsHistory->AddCommand( nCommonSegment, pCmd );
				bHasCommands = true;
			}
		}
		else
		{
			//DebugTrace( "+++ Execute Plr %d, not present", i );
		}
		cmds[nFromSegment][i].clear();
	}
	if ( bHasCommands )
	{
		//DebugTrace( "+++ Executed commands from segment %d ***", nFromSegment );
	}
}

bool CMPTransceiver::IsSegmentFinishedByAll( int nSegment )
{
	return ( segmFinished[nSegment] & wMask ) == wMask;
}

bool CMPTransceiver::IsSegmentPackFinished()
{
	return ( nCommonSegment % nSegmentsPackSize == 0 );
}

void CMPTransceiver::SetSegmentFinished( int nPlayer, int nSegment, unsigned long ulCheckSum )
{
	if ( segmFinished[nSegment] & ( 1UL << nPlayer ) )
		segmFinished[nSegment] = 0;
	segmFinished[nSegment] |= ( 1UL << nPlayer );
	checkSums[nPlayer][nSegment] = ulCheckSum;
	//DebugTrace( "+++ Player %d segment %d checksum %d", nPlayer, nSegment, ulCheckSum );
}

void CMPTransceiver::FinalizeSegmentPack()
{
	unsigned long ulCheckSum = pCmdsHistory->GetLastChecksum();
	SetSegmentFinished( nMyLogicID, nSegment, ulCheckSum );
	CAISegmentFinishedPacket *pPacket = new CAISegmentFinishedPacket();
	pPacket->nSegment = nCommonSegment;	
	pPacket->ulCheckSum = ulCheckSum;		
	const int nSegmentToExecute = GetSegmentToExecute( nCommonSegment );

	for ( list< CPtr<IAILogicCommandB2> >::iterator it = aiCommandsToSend.begin(); it != aiCommandsToSend.end(); ++it )
	{
		CPtr<IAILogicCommandB2> pCommand = *it;
		cmds[nSegmentToExecute][nMyLogicID].push_back( pCommand );
		//DebugTrace( "+++ Plr %d cmd to execute on %d", nMyLogicID, nSegmentToExecute );
		SRawCommand &aiCommand = *( pPacket->aiCommands.insert( pPacket->aiCommands.end() ) );
		aiCommand.nTypeID = pCmdsSerializer->GetCommandID( pCommand );
		{
			aiCommand.cmd.Seek( 0 );
			CObj<IBinSaver> pSaver = CreateBinSaver( &(aiCommand.cmd), SAVER_MODE_WRITE );
			pSaver->AddPolymorphicBase( 1, pCommand );
		}
	}
	//DebugTrace( "+++ send SEG_PACK Plr %d, seg %d(%d), CS %u, %d cmds", nMyLogicID, nSegment, nCommonSegment, ulCheckSum, pPacket->aiCommands.size() );
	aiCommandsToSend.clear();
	pClient->SendGamePacket( pPacket, true );
}

void CMPTransceiver::AdvanceToNextSegment()
{
	pTimer->NextSegment();
	pAI->Segment();
	++nCommonSegment;
	nSegment = nCommonSegment % pConsts->nMaxLatency;
	//DebugTrace( "+++ Local Player %d move to segment %d (%d)", nMyLogicID, nSegment, nCommonSegment );
}

int CMPTransceiver::GetSegmentToExecute( const int nSegment ) const
{
	return ( nSegment + nSegmentsPackSize ) % pConsts->nMaxLatency;  
}

bool CMPTransceiver::OnAISegmentFinishedPacket( class CAISegmentFinishedPacket *pPacket )
{
	const int nPlayer = GetPlayerByClient( pPacket->nClientID );
	if ( nPlayer < 0 || !IsPlayerPresent( nPlayer ) )
		return true;
	int nPlayerSegment = pPacket->nSegment % pConsts->nMaxLatency;
	//DebugTrace( "+++ receive SEG_PACK Plr %d, seg %d(%d), CS %u, %d cmds", nPlayer, nPlayerSegment, pPacket->nSegment, pPacket->ulCheckSum, pPacket->aiCommands.size() );
	SetSegmentFinished( nPlayer, nPlayerSegment, pPacket->ulCheckSum );
	const int nSegmentToExecuteCommand = GetSegmentToExecute( pPacket->nSegment );
	for ( list<SRawCommand>::iterator it = pPacket->aiCommands.begin(); it != pPacket->aiCommands.end(); ++it )
	{
		SRawCommand &rawCommand = *it;
		CObjectBase *pObj = pCmdsSerializer->MakeCommand( rawCommand.nTypeID );
		CPtr<IAILogicCommandB2> pCommand = dynamic_cast<IAILogicCommandB2*>( pObj );
		NI_VERIFY( pCommand, "Wrong command type id", return true );
		{
			rawCommand.cmd.Seek( 0 );
			CObj<IBinSaver> pSaver = CreateBinSaver( &(rawCommand.cmd), SAVER_MODE_READ );
			pSaver->AddPolymorphicBase( 1, pCommand );
		}
		if ( IsPlayerPresent( nPlayer ) )
		{
			cmds[nSegmentToExecuteCommand][nPlayer].push_back( pCommand );
			//DebugTrace( "+++ Plr %d cmd to execute on %d", nPlayer, nSegmentToExecuteCommand );
		}
	}
	return true;
}

bool CMPTransceiver::OnTransciverCommonPacket( class CTransciverCommonPacket *pPacket )
{
	int nPlayer = GetPlayerByClient( pPacket->nClientID );

	switch ( pPacket->eType )
	{
	case CTransciverCommonPacket::ET_LOADED:
    players[nPlayer].bLoaded = true;
		return true;
	};

	return false;
}

int CMPTransceiver::GetPlayerByClient( int nClient ) const
{
	for ( int i = 0; i < players.size(); ++i )
	{
		if ( IsPlayerPresent( i ) && players[i].nClientID == nClient )
			return i;
	}
	return -1;
}

bool CMPTransceiver::IsPlayerPresent( int nPlayer ) const
{
	return wMask & ( 1UL << nPlayer );
}

bool CMPTransceiver::IsGameRunning() const
{
	return bIsGameRunning;
}

ICheckSumLog* CMPTransceiver::GetCheckSumLogger()
{
	return pCmdsHistory;
}

const NDb::SMapInfo* CMPTransceiver::GetMap() const
{
	return pMapInfo;
}

void CMPTransceiver::CheckRunGame()
{
	if ( bIsGameRunning )
		return;

	if ( !players[nMyLogicID].bLoaded )
	{
		players[nMyLogicID].bLoaded = true;
		pClient->SendGamePacket( new CTransciverCommonPacket( CTransciverCommonPacket::ET_LOADED ), true );
		bWaiting = false;	// So that the following call works
		return;
	}

	bool bCanStart = true;
	DWORD dwWaitFor = 0;
	for ( int i = 0; i < players.size(); ++i )
	{
		SPlayer &player = players[i];

		if ( IsPlayerPresent( i ) && !player.bLoaded )
		{
			bCanStart = false;
			dwWaitFor = dwWaitFor | ( 1UL << i );
		}
	}

	if ( !bCanStart )
	{
		ReportLags( dwWaitFor, true );
		return;
	}

	bIsGameRunning = true;
	ReportLags( 0, false );
	pAI->NetGameStarted();
	Scene()->ResetTimer( GetTickCount() );
}

void CMPTransceiver::EndGame()
{
	if ( bIsGameEnded )
		return;
	bIsGameEnded = true;
}

void CMPTransceiver::LeaveOutOfSync()
{
//	WriteToPipe( PIPE_CHAT, StrFmt( "You were kicked because you were out of sync" ) );
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_DEBUG_WINDOW, StrFmt( "You were kicked because you were out of sync" ) );
	Singleton<IScenarioTracker>()->MissionCancel();
	EndGame();
}

const bool CMPTransceiver::IsGamePacket( const CNetPacket *pPacket )
{
	const CAISegmentFinishedPacket *pSegmentPacket = dynamic_cast<const CAISegmentFinishedPacket*>( pPacket );
	if ( pSegmentPacket )
		return true;
	const CTransciverCommonPacket *pInfoPacket = dynamic_cast<const CTransciverCommonPacket*>( pPacket );
	if ( pInfoPacket )
		return true;
	return false;
}

void CMPTransceiver::Init( IServerClient *_pClient, const SB2StartGameParams &params, int nMySlot )
{
	NI_ASSERT( !params.clients.empty(), "Empty clients list" );
	SReplayInfo replay;
	bCommandsFromHistory = false;
	bIsGameRunning = false;
	bIsGameEnded = false;
	pClient = _pClient;
	pCmdsSerializer = CreateAICmdsAutoMagic();
	pConsts = NGameX::GetNetConsts();
	pCmdsHistory = CreateCommandsHistory( replay );
	pAI = Singleton<IAILogic>();
	pTimer = Singleton<IGameTimer>();
	wMask = 0;
	nGameID = params.nGameID;
	int nCurrentPlayers = params.clients.size();
	pMapInfo = params.pMapInfo;
	players.resize( pMapInfo->players.size() );
	DebugTrace( "+++ MPT: start game, %d plrs, me = %d", nCurrentPlayers, nMySlot );
	for ( int i = 0; i < nCurrentPlayers; ++i )
	{
		const SB2StartGameParams::SClient &client = params.clients[i];
		SPlayer &player = players[client.nPlayer];
		player.nClientID = client.nClientID;
		player.nTeam = client.nTeam;
		wMask |= (1UL << client.nPlayer);
		DebugTrace( "+++ MPT: %d - plr %d, CID %d, team %d", i, client.nPlayer, client.nClientID, client.nTeam );
	}
	nMyLogicID = nMySlot;

	pAI->ClearAI();
	pAI->SetNPlayers( players.size() );
	pAI->SetNetGame( true );
	pAI->SetMyDiplomacyInfo( players[nMyLogicID].nTeam, nMyLogicID ); 
	nGameSpeed = params.nSpeedAdjustment;
	checkSums.SetSizes( pConsts->nMaxLatency, players.size() );
	segmFinished.resize( pConsts->nMaxLatency, 0 );
	cmds.Clear();
	cmds.SetSizes( players.size(), pConsts->nMaxLatency );
	checkSums.FillZero();
}

void CMPTransceiver::CommandTimeOut( bool bSet )
{ 
	bWaiting = bSet; 
	pTimer->Pause( bSet, PAUSE_TYPE_USER_PAUSE );
}

void CMPTransceiver::SetLagState( int nSegment, bool bOn )
{
	pTimer->Pause( bOn, PAUSE_TYPE_MP_NO_SEGMENT_DATA );
	if ( bOn )
	{
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_DEBUG_WINDOW + 4, StrFmt( "WAIT(%d/%d)", segmFinished[nSegment], wMask ) );
		NTimer::STime curTime = pTimer->GetAbsTime();
		if ( timeStartWaiting == 0 )
			timeStartWaiting = curTime;
		else
		{
			if ( curTime - timeStartWaiting > s_nInterruptTimeout )
				ReportLags( wMask & ~segmFinished[nSegment], false );
		}
	}
	else
	{
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_DEBUG_WINDOW + 4, StrFmt( "" ) );
		ReportLags( 0, false );
		timeStartWaiting = 0;
	}
}

void CMPTransceiver::ReportLags( DWORD dwLaggers,	bool bInitial )
{
	Singleton<IMPToUIManager>()->AddUIMessage( new SMPUILagMessage( dwLaggers, bInitial ) );
}

bool CMPTransceiver::IsAsyncDetected( int nSegment )
{
	if ( nCommonSegment <= nLatency )
		return false;

	string szCheckSums = StrFmt( "Segment %d: ", nSegment );
	unsigned long nMyCheckSum = checkSums[nMyLogicID][nSegment];
	int nOutOfSyncs = 0;
	for ( int nPlayerIndex = 0; nPlayerIndex < 16; ++nPlayerIndex )
	{
		if ( !IsPlayerPresent( nPlayerIndex ) )
			continue;

		szCheckSums += StrFmt( "%d,", checkSums[nPlayerIndex][nSegment] );
		if ( checkSums[nPlayerIndex][nSegment] != nMyCheckSum )
			++nOutOfSyncs;
	}
	if ( nOutOfSyncs > 0 )
	{
		if ( NGlobal::GetVar( "save_on_async", 0 ) != 0 )
		{
			NSaveLoad::SSaveInfo info;
			string szFilename = StrFmt( "async_%1d_%03d", nMyLogicID, nSegment );
			NMainLoop::Command( CreateICSave( szFilename ) );
			info.Write( NSaveLoad::GetSavePath() + szFilename + NSaveLoad::INFO_FILE_EXTENSION, NStr::ToUnicode( szCheckSums ), false, false, false );
			NGlobal::SetVar( "save_on_async", 0 );
			NMainLoop::Command( ML_COMMAND_MESSAGE_BOX, CICMessageBox::MakeConfigString( "MessageBoxWindowOk", L"ASYNC<br>Save created" ).c_str() );
			//pAI->DumpAfterAssinc();
		}

		if ( NGlobal::GetVar( "pause_on_async", 0 ) != 0 )
		{
			if ( NGlobal::GetVar( "MP_async", 0 ) == 0 )
				NGlobal::SetVar( "MP_async", 2 );
			pTimer->Pause( true, PAUSE_TYPE_USER_PAUSE );
		}
		else
			LeaveOutOfSync();
		return true;
	}
	return false;
}

int CMPTransceiver::ScheduleGameEnd( const int _nSegment )
{
	const int nFinishOnSegment = ( _nSegment > 0 ) ? _nSegment : nCommonSegment;
	nFinalSegment = ( nFinishOnSegment - nFinishOnSegment % nSegmentsPackSize ) + nLatency;

	return nFinishOnSegment;
}

CMPTransceiver* CreateMPTransceiver( IServerClient *pClient, const SB2StartGameParams &params, int nMySlot )
{
	CMPTransceiver *pTransceiver = new CMPTransceiver();
	pTransceiver->Init( pClient, params, nMySlot );

	return pTransceiver;
}

REGISTER_SAVELOAD_CLASS( 0x1713C440, CMPTransceiver );

START_REGISTER(MPTransceiver)
REGISTER_VAR_EX( "multiplayer_pause_timeout", NGlobal::VarIntHandler, &s_nInterruptTimeout, 5000, STORAGE_NONE );
FINISH_REGISTER
