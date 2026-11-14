#include "stdafx.h"

#include "AICmdsAutoMagic.h"
#include "AILogic/B2AI.h"
#include "Client/ServerClientInterface.h"
#include "CommonCommands.h"
#include "DBConsts.h"
#include "DBGameRoot.h"
#include "GameXClassIDs.h"
#include "GetConsts.h"
#include "InterfaceMisc.h"
#include "InterfaceMissionInternal.h"
#include "InterfaceState.h"
#include "Main/AICmdsAutoMagicInterface.h"
#include "Main/CommandsHistory.hpp"
#include "Main/DBNetConsts.h"
#include "Main/MainLoop.h"
#include "Main/MainLoopCommands.h"
#include "Misc/StrProc.h"
#include "Misc/Win32Helper.h"
#include "MPPacketTraceLog.h"
#include "MPTransceiver.h"
#include "MultiplayerCommandManager.h"
#include "port/time.h"
#include "port/unicode.h"
#include "SaveLoadHelper.h"
#include "ScenarioTracker.h"
#include "SceneB2/Scene.h"
#include "Server_Client_Common/GamePackets.h"
#include "System/Commands.h"
#include "System/GlobalVars.h"

#include "GameX_export.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>

#include <float.h>
#include <fmt/format.h>

static int s_nInterruptTimeout = 5000; // No segments for this time before timeout is called

class CAISegmentFinishedPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CAISegmentFinishedPacket );

public:
	ZDATA
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	int nSegment = {};
	// uint32_t, through a local typedef for unsigned long until now: this is
	// the checksum every client compares, and it is serialized into the packet.
	uint32_t ulCheckSum = {};
	std::list<CMPTransceiver::SRawCommand> aiCommands;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &nSegment );
		f.Add( 3, &ulCheckSum );
		f.Add( 4, &aiCommands );
		return 0;
	}

	CAISegmentFinishedPacket()
		: CNetPacket( 0 )
	{
	}
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
	EType eType = {};

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &eType );
		return 0;
	}

	CTransciverCommonPacket() {}

	CTransciverCommonPacket( EType _eType )
		: CNetPacket( 0 )
		, eType( _eType )
	{
	}
};

REGISTER_SAVELOAD_CLASS( GAMEX, 111, CAISegmentFinishedPacket );
REGISTER_SAVELOAD_CLASS( GAMEX, 112, CTransciverCommonPacket );

// CMPTransceiver

CMPTransceiver::CMPTransceiver()
	: nGameSpeed( 0 )
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
	Scene()->ResetTimer( GetCurrentTimeMilliseconds() );
	pTimer->SetSpeed( nGameSpeed );
	pCmdsHistory->StartNewGame( pMapInfo );
	NMainLoop::Command( new CICMission( this ) );
	bWaiting = false;
}

void CMPTransceiver::SendCommand( IAILogicCommandB2 *pCommand )
{
	if ( !bIsGameRunning || bIsGameEnded )
	{
		return;
	}

	aiCommandsToSend.push_back( pCommand );
	//DebugTrace( "+++ Segment %d(%d) cmd from %d", nSegment, nCommonSegment, nMyLogicID );
}

void CMPTransceiver::PlayerRemoved( int nPlayer )
{
	//DebugTrace( "+++ Segment %d, removing player %d", nCommonSegment, nPlayer );
	if ( nPlayer < 0 || !IsPlayerPresent( nPlayer ) )
	{
		return;
	}

	const unsigned long ulMaskBefore = wMask;
	wMask &= ~( 1UL << nPlayer );
	wWaitMask &= ~( 1UL << nPlayer );
	NGameX::MatchPacketTrace_RecordDropApplied( nPlayer, nCommonSegment );
	NGameX::MatchPacketTrace_Log( nCommonSegment,
	                              "STATE",
	                              "PlayerRemoved",
	                              players[nPlayer].nClientID,
	                              fmt::format( "slot={} pre_active_mask={:08X} post_active_mask={:08X} wait_mask={:08X}",
	                                           nPlayer,
	                                           ulMaskBefore,
	                                           wMask,
	                                           wWaitMask ) );
	// A disconnect changes the simulation just like a player command. Record it at
	// the applied boundary, before this segment's orders, so replay also transfers the units.
	CDropPlayerCommand::RecordAndExecute( pCmdsHistory, nCommonSegment, nPlayer );
	NInput::PostEvent( "mission_remove_player", nPlayer, 0 );
}

bool CMPTransceiver::SchedulePlayerRemoval( int nPlayer, int nSegment )
{
	if ( bIsGameEnded || nPlayer < 0 || nPlayer >= playerRemovals.size() )
	{
		return false;
	}

	NMPPlayerDrop::SPlayerDrop &removal = playerRemovals[nPlayer];
	const int nPrevious = removal.GetRemovalSegment( nLatency );
	const NMPPlayerDrop::EResult result = removal.Schedule( nSegment, nCommonSegment, nLatency );
	if ( result == NMPPlayerDrop::TOO_LATE )
	{
		// There is no rollback: transferring now would preserve two different
		// histories. Refuse the correction before changing either membership mask.
		FailPlayerDrop( nPlayer, nSegment, "late_drop_boundary" );
		return false;
	}
	if ( result == NMPPlayerDrop::INVALID )
	{
		return false;
	}
	if ( result == NMPPlayerDrop::UNCHANGED )
	{
		return true;
	}

	StopWaitingForPlayer( nPlayer, nSegment );
	// Removal precedes orders at this boundary. Packet P's orders execute at
	// P + latency + pack size; required earlier packets still gate those ticks.
	const int nSimulationRemovalSegment = removal.GetRemovalSegment( nLatency );
	NGameX::MatchPacketTrace_RecordDropScheduled( nPlayer, nSimulationRemovalSegment );
	NGameX::MatchPacketTrace_Log( nCommonSegment,
	                              "DECISION",
	                              "SchedulePlayerRemoval",
	                              players[nPlayer].nClientID,
	                              fmt::format( "slot={} requested_seg={} sim_remove_seg={} queued_seg={} prev_seg={}",
	                                           nPlayer,
	                                           nSegment,
	                                           nSimulationRemovalSegment,
	                                           nSimulationRemovalSegment,
	                                           nPrevious ) );

	// Apply only at dispatch, after the manager has drained its packet queue.
	// Applying here would make simultaneous transfers follow packet arrival order.
	return !bPlayerDropError;
}

// perform segments for AI
void CMPTransceiver::DoSegments()
{
	NWin32Helper::MaskAllFloatingPointExceptions();
	// pClient is not owned here: the MP manager mode holds the owning CObj and
	// replaces the client on a network error. Releasing that CObj clears the
	// client in place (DestroyContents leaves a default-constructed object with
	// no connection), and the interface teardown that error requests runs only
	// on a later frame. Until then there is nothing to step or send to.
	// CheckRunGame and FinalizeSegmentPack, the other users of pClient, are only
	// reached from here.
	if ( !IsValid( pClient ) )
	{
		return;
	}
	pClient->Segment();
	// Membership recovery freezes simulation, not the network pump. It also
	// precedes loading and final-boundary handling so neither can bypass it.
	if ( bDepartureRecovery )
	{
		return;
	}
	if ( !IsGameRunning() )
	{
		CheckRunGame();
		return;
	}
	pTimer->BeginSegments(); //???this line does nothing.

	if ( bIsGameEnded )
	{
		return;
	}

	// A final boundary already reached must finish even while a local user pause
	// is active. Never enter another packet wait after reaching that boundary.
	if ( nFinalSegment >= 0 && nCommonSegment >= nFinalSegment )
	{
		ApplyScheduledPlayerRemovals();
		EndGame();
		return;
	}
	if ( bWaiting )
	{
		return;
	}

	while ( pTimer->CanStartNextSegment() )
	{
		if ( IsSegmentPackFinished() )
		{
			// прибавляется MAX_LATENCY, т.к. модуль от отрицательных чисел считается неправильно
			const int nPastSegment = ( nSegment - nLatency + pConsts->nMaxLatency ) % pConsts->nMaxLatency;
			if ( nCommonSegment < nLatency || IsSegmentFinishedByAll( nPastSegment ) )
			{
				// Commit all departures in slot order immediately before this tick's
				// orders, only once its required command prefix is available.
				ApplyScheduledPlayerRemovals();
				if ( bIsGameEnded )
				{
					return;
				}
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
		{
			ApplyScheduledPlayerRemovals();
			if ( bIsGameEnded )
			{
				return;
			}
			AdvanceToNextSegment();
		}

		// Catch-up can execute several segments in one frame. Stop on the agreed
		// tick, before asking an already-finished peer for another segment packet.
		if ( nFinalSegment >= 0 && nCommonSegment >= nFinalSegment )
		{
			ApplyScheduledPlayerRemovals();
			EndGame();
			return;
		}
	}
}

void CMPTransceiver::ApplyScheduledPlayerRemovals()
{
	for ( int i = 0; i < playerRemovals.size(); ++i )
	{
		NMPPlayerDrop::SPlayerDrop &removal = playerRemovals[i];
		const int nSegment = removal.GetRemovalSegment( nLatency );
		if ( !removal.IsPending() || nSegment > nCommonSegment )
		{
			continue;
		}
		if ( nSegment < nCommonSegment )
		{
			FailPlayerDrop( i, removal.GetSegment(), "missed_drop_boundary" );
			return;
		}
		if ( nSegment == nCommonSegment )
		{
			NGameX::MatchPacketTrace_Log( nCommonSegment,
			                              "STATE",
			                              "ApplyScheduledPlayerRemovals",
			                              players[i].nClientID,
			                              fmt::format( "slot={} target_seg={} apply_seg={}", i, nSegment, nCommonSegment ) );
			// Keep the applied decision so duplicates cannot requeue or erase it.
			removal.MarkApplied();
			PlayerRemoved( i );
		}
	}
}

void CMPTransceiver::FailPlayerDrop( int nPlayer, int nSegment, const char *szReason )
{
	if ( bPlayerDropError )
	{
		return;
	}
	bPlayerDropError = true;
	NGameX::MatchPacketTrace_Log( nCommonSegment,
	                              "ERROR",
	                              "PlayerDropSynchronizationFailed",
	                              players[nPlayer].nClientID,
	                              fmt::format( "slot={} requested_seg={} reason={}", nPlayer, nSegment, szReason ) );
	NGameX::MatchPacketTrace_Flush( "player_drop_error" );
	// The manager displays the network error on its next update. Stop simulation
	// immediately, including when the failure occurs inside a catch-up loop.
	Singleton<IScenarioTracker>()->MissionCancel();
	EndGame();
}

void CMPTransceiver::ExecuteCommands( int nFromSegment )
{
	NWin32Helper::MaskAllFloatingPointExceptions();
	bool bHasCommands = false;
	for ( int i = 0; i < cmds.GetSizeX(); ++i )
	{
		if ( IsPlayerPresent( i ) ) // Only for existing players
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
	return GetMissingSegmentPlayers( nSegment ) == 0;
}

uint32_t CMPTransceiver::GetCurrentMissingPlayers() const
{
	if ( !bIsGameRunning || bIsGameEnded || nCommonSegment < nLatency || nSegmentsPackSize <= 0 ||
	     nCommonSegment % nSegmentsPackSize != 0 || !pConsts || pConsts->nMaxLatency <= 0 )
	{
		return 0;
	}

	const int nPastSegment = ( nSegment - nLatency + pConsts->nMaxLatency ) % pConsts->nMaxLatency;
	if ( nPastSegment < 0 || nPastSegment >= segmFinished.size() )
	{
		return 0;
	}

	// Scheduled departures retain a separate command-prefix barrier, but no
	// longer participate in ordinary lag voting. Do not turn them into laggers.
	return GetMissingSegmentPlayers( nPastSegment ) & wWaitMask;
}

uint16_t CMPTransceiver::GetMissingSegmentPlayers( int nSegment ) const
{
	uint16_t wMissing = wWaitMask & ~segmFinished[nSegment];
	// A departure releases the ordinary receipt barrier immediately, but commands
	// that execute before its removal still require the departing player's packet.
	// Peer streams are ordered, so the absolute receipt counter proves that prefix.
	for ( int i = 0; i < playerRemovals.size(); ++i )
	{
		if ( i != nMyLogicID && IsPlayerPresent( i ) &&
		     playerRemovals[i].RequiresPacket(
						 nCommonSegment, nLatency, nSegmentsPackSize, players[i].nLastReceivedSegment ) )
		{
			wMissing |= 1U << i;
		}
	}
	return wMissing;
}

bool CMPTransceiver::IsSegmentPackFinished()
{
	return ( nCommonSegment % nSegmentsPackSize == 0 );
}

void CMPTransceiver::SetSegmentFinished( int nPlayer, int nSegment, uint32_t ulCheckSum )
{
	// Repeated receipt of one player's data must never erase other players' bits.
	// Consumed ring slots are cleared explicitly by DoSegments.
	segmFinished[nSegment] |= ( 1UL << nPlayer );
	checkSums[nPlayer][nSegment] = ulCheckSum;
	//DebugTrace( "+++ Player %d segment %d checksum %d", nPlayer, nSegment, ulCheckSum );
}

void CMPTransceiver::FinalizeSegmentPack()
{
	const uint32_t ulCheckSum = pCmdsHistory->GetLastChecksum();
#ifdef CHECKSUM_LIST_DEBUG
	myHistoryHashes.push_back( ulCheckSum );
#endif
	SetSegmentFinished( nMyLogicID, nSegment, ulCheckSum );
	CAISegmentFinishedPacket *pPacket = new CAISegmentFinishedPacket();
	pPacket->nSegment = nCommonSegment;
	pPacket->ulCheckSum = ulCheckSum;
	const int nSegmentToExecute = GetSegmentToExecute( nCommonSegment );

	for ( std::list<CPtr<IAILogicCommandB2>>::iterator it = aiCommandsToSend.begin(); it != aiCommandsToSend.end(); ++it )
	{
		CPtr<IAILogicCommandB2> pCommand = *it;
		const int nCommandID = pCmdsSerializer->GetCommandID( pCommand );
		if ( nCommandID < 0 )
		{
			// A command missing from the MP table must never run only on its originating client.
			NGameX::MatchPacketTrace_Log( nCommonSegment, "TX", "UnregisteredAICommand", -1, "command rejected" );
			continue;
		}
		cmds[nSegmentToExecute][nMyLogicID].push_back( pCommand );
		//DebugTrace( "+++ Plr %d cmd to execute on %d", nMyLogicID, nSegmentToExecute );
		SRawCommand &aiCommand = pPacket->aiCommands.emplace_back();
		aiCommand.nTypeID = nCommandID;
		{
			aiCommand.cmd.Seek( 0 );
			CObj<IBinSaver> pSaver = CreateBinSaver( &( aiCommand.cmd ), SAVER_MODE_WRITE );
			pSaver->AddPolymorphicBase( 1, pCommand );
		}
	}
	//DebugTrace( "+++ send SEG_PACK Plr %d, seg %d(%d), CS %u, %d cmds", nMyLogicID, nSegment, nCommonSegment, ulCheckSum, pPacket->aiCommands.size() );
	NGameX::MatchPacketTrace_Log( nCommonSegment,
	                              "TX",
	                              "CAISegmentFinishedPacket",
	                              players[nMyLogicID].nClientID,
	                              fmt::format( "segment={} checksum={} cmd_count={}",
	                                           pPacket->nSegment,
	                                           pPacket->ulCheckSum,
	                                           int( pPacket->aiCommands.size() ) ) );
	aiCommandsToSend.clear();
	nLastSentSegment = pPacket->nSegment;
	departurePackets.push_back( pPacket );
	// Producing P requires each survivor's P-latency, which in turn required our
	// P-2*latency. Everything older has already reached every active survivor.
	// Retain an extra pack on each side of that bound, including startup packets.
	const int64_t nOldest = int64_t( nLastSentSegment ) - 2 * int64_t( nLatency ) - 2 * int64_t( nSegmentsPackSize );
	while ( !departurePackets.empty() &&
	        static_cast<CAISegmentFinishedPacket *>( departurePackets.front().GetPtr() )->nSegment < nOldest )
	{
		departurePackets.pop_front();
	}
	pClient->SendGamePacket( pPacket, true );
}

int CMPTransceiver::GetDepartureSegment() const
{
	// The final produced packet executes at P + pack + latency. Remove at the
	// next pack boundary, exactly where peers would need the first unsent packet.
	const int64_t nDropSegment =
			nLastSentSegment < 0 ? nSegmentsPackSize : int64_t( nLastSentSegment ) + 2 * int64_t( nSegmentsPackSize );
	if ( nSegmentsPackSize <= 0 || nLatency < 0 || nDropSegment > ( std::numeric_limits<int>::max )() - nLatency )
	{
		return -1;
	}
	return int( nDropSegment );
}

void CMPTransceiver::SetDepartureRecovery( bool bRecovering )
{
	if ( bDepartureRecovery == bRecovering )
	{
		return;
	}
	bDepartureRecovery = bRecovering;
	if ( IsValid( pTimer ) )
	{
		// A user Resume or an ordinary input receipt must not release this pause.
		pTimer->Pause( bRecovering, PAUSE_TYPE_MP_DEPARTURE_RECOVERY );
	}
	if ( !bRecovering )
	{
		timeStartWaiting = 0;
		timeStartWaitingForRemovedPlayer = 0;
	}
}

void CMPTransceiver::PruneRecoveryCommands()
{
	// Prune against our executed frontier, not the newest received packet: a
	// faster peer's receipt must not discard input a slower survivor still needs.
	const int64_t nOldest = int64_t( nCommonSegment ) - 2 * int64_t( nLatency ) - 2 * int64_t( nSegmentsPackSize );
	for ( SPlayer &player : players )
	{
		while ( !player.recoveryPackets.empty() &&
		        static_cast<CAISegmentFinishedPacket *>( player.recoveryPackets.front().GetPtr() )->nSegment < nOldest )
		{
			player.recoveryPackets.pop_front();
		}
	}
}

void CMPTransceiver::GetRecoveryCommands( uint32_t dwPlayers, std::vector<SMPRecoveryCommand> *pCommands ) const
{
	if ( !pCommands )
	{
		return;
	}
	pCommands->clear();
	for ( int i = 0; i < players.size() && i < 32; ++i )
	{
		if ( ( dwPlayers & ( uint32_t( 1 ) << i ) ) == 0 )
		{
			continue;
		}
		// The originating client already queues its own commands directly; its
		// outgoing tail is the equivalent source of raw packets for another peer.
		const auto &packets = i == nMyLogicID ? departurePackets : players[i].recoveryPackets;
		for ( const CPtr<CNetPacket> &pPacket : packets )
		{
			SMPRecoveryCommand &command = pCommands->emplace_back();
			command.nPlayer = i;
			command.nSegment = static_cast<CAISegmentFinishedPacket *>( pPacket.GetPtr() )->nSegment;
			command.pPacket = pPacket;
		}
	}
}

static bool HaveSameRecoveryCommands( const CAISegmentFinishedPacket &first, const CAISegmentFinishedPacket &second )
{
	if ( first.ulCheckSum != second.ulCheckSum || first.aiCommands.size() != second.aiCommands.size() )
	{
		return false;
	}
	auto firstCommand = first.aiCommands.begin();
	auto secondCommand = second.aiCommands.begin();
	for ( ; firstCommand != first.aiCommands.end(); ++firstCommand, ++secondCommand )
	{
		if ( firstCommand->nTypeID != secondCommand->nTypeID ||
		     firstCommand->cmd.GetSize() != secondCommand->cmd.GetSize() ||
		     ( firstCommand->cmd.GetSize() != 0 &&
		       std::memcmp( firstCommand->cmd.GetBuffer(), secondCommand->cmd.GetBuffer(), firstCommand->cmd.GetSize() ) !=
		           0 ) )
		{
			return false;
		}
	}
	return true;
}

bool CMPTransceiver::SameRecoveryCommand( const SMPRecoveryCommand &first, const SMPRecoveryCommand &second )
{
	const auto *pFirst = dynamic_cast<const CAISegmentFinishedPacket *>( first.pPacket.GetPtr() );
	const auto *pSecond = dynamic_cast<const CAISegmentFinishedPacket *>( second.pPacket.GetPtr() );
	return pFirst && pSecond && first.nPlayer >= 0 && first.nSegment >= 0 && first.nPlayer == second.nPlayer &&
	       first.nSegment == second.nSegment && first.nSegment == pFirst->nSegment &&
	       second.nSegment == pSecond->nSegment && HaveSameRecoveryCommands( *pFirst, *pSecond );
}

bool CMPTransceiver::RestoreRecoveryCommands( const std::vector<SMPRecoveryCommand> &commands,
                                              uint32_t dwPlayers,
                                              const std::vector<int> &cutoffs )
{
	const auto Reject = [this]( const char *szReason, int nPlayer, int nPacket )
	{
		NGameX::MatchPacketTrace_Log( nCommonSegment,
		                              "ERROR",
		                              "RecoveryCommandsRejected",
		                              -1,
		                              fmt::format( "reason={} slot={} packet={}", szReason, nPlayer, nPacket ) );
		return false;
	};
	if ( !bDepartureRecovery || nSegmentsPackSize <= 0 || nLatency < 0 || players.size() > 32 )
	{
		return Reject( "invalid_recovery_state", -1, -1 );
	}
	const uint32_t dwKnownPlayers = players.size() == 32 ? ~uint32_t( 0 ) : ( uint32_t( 1 ) << players.size() ) - 1;
	if ( ( dwPlayers & ~dwKnownPlayers ) != 0 )
	{
		return Reject( "invalid_player_mask", -1, -1 );
	}
	for ( int i = 0; i < players.size(); ++i )
	{
		if ( ( dwPlayers & ( uint32_t( 1 ) << i ) ) != 0 && IsPlayerPresent( i ) &&
		     ( i >= cutoffs.size() || !NMPPlayerDrop::SPlayerDrop::IsValidSegment( cutoffs[i], nLatency ) ||
		       cutoffs[i] % nSegmentsPackSize != 0 ) )
		{
			return Reject( "invalid_cutoff", i, -1 );
		}
	}

	std::vector<const SMPRecoveryCommand *> ordered;
	ordered.reserve( commands.size() );
	for ( const SMPRecoveryCommand &command : commands )
	{
		const auto *pPacket = dynamic_cast<CAISegmentFinishedPacket *>( command.pPacket.GetPtr() );
		if ( command.nPlayer < 0 || command.nPlayer >= players.size() ||
		     ( dwPlayers & ( uint32_t( 1 ) << command.nPlayer ) ) == 0 || !pPacket || command.nSegment < 0 ||
		     command.nSegment % nSegmentsPackSize != 0 || command.nSegment != pPacket->nSegment )
		{
			return Reject( "invalid_packet", command.nPlayer, command.nSegment );
		}
		// Previously applied removals keep their history for other survivors, but
		// must never deserialize or execute those commands again on this client.
		if ( IsPlayerPresent( command.nPlayer ) && command.nSegment < cutoffs[command.nPlayer] - nSegmentsPackSize )
		{
			const auto &localPackets =
					command.nPlayer == nMyLogicID ? departurePackets : players[command.nPlayer].recoveryPackets;
			for ( const CPtr<CNetPacket> &pLocal : localPackets )
			{
				const auto *pLocalPacket = static_cast<CAISegmentFinishedPacket *>( pLocal.GetPtr() );
				if ( pLocalPacket->nSegment == command.nSegment && !HaveSameRecoveryCommands( *pLocalPacket, *pPacket ) )
				{
					return Reject( "conflicting_local_packet", command.nPlayer, command.nSegment );
				}
			}
			ordered.push_back( &command );
		}
	}
	std::sort( ordered.begin(),
	           ordered.end(),
	           []( const SMPRecoveryCommand *first, const SMPRecoveryCommand *second )
	           {
							 return first->nPlayer < second->nPlayer ||
		                  ( first->nPlayer == second->nPlayer && first->nSegment < second->nSegment );
						 } );

	// Validate every missing suffix before importing any player's data. A later
	// receipt counter cannot prove a gap that existed on a different peer.
	std::vector<const SMPRecoveryCommand *> missing;
	size_t nCommand = 0;
	for ( int i = 0; i < players.size(); ++i )
	{
		if ( ( dwPlayers & ( uint32_t( 1 ) << i ) ) == 0 || !IsPlayerPresent( i ) )
		{
			continue;
		}
		const int nLast = i == nMyLogicID ? nLastSentSegment : players[i].nLastReceivedSegment;
		int64_t nNext = nLast < 0 ? 0 : int64_t( nLast ) + nSegmentsPackSize;
		const SMPRecoveryCommand *pPrevious = nullptr;
		while ( nCommand < ordered.size() && ordered[nCommand]->nPlayer == i )
		{
			const SMPRecoveryCommand *pCommand = ordered[nCommand++];
			if ( pPrevious && pPrevious->nSegment == pCommand->nSegment )
			{
				if ( !HaveSameRecoveryCommands( *static_cast<CAISegmentFinishedPacket *>( pPrevious->pPacket.GetPtr() ),
				                                *static_cast<CAISegmentFinishedPacket *>( pCommand->pPacket.GetPtr() ) ) )
				{
					return Reject( "conflicting_packet", i, pCommand->nSegment );
				}
				continue;
			}
			pPrevious = pCommand;
			if ( pCommand->nSegment <= nLast )
			{
				continue;
			}
			if ( pCommand->nSegment != nNext || i == nMyLogicID ||
			     int64_t( pCommand->nSegment ) + nLatency + nSegmentsPackSize < nCommonSegment )
			{
				return Reject( "noncontiguous_prefix", i, int( nNext ) );
			}
			missing.push_back( pCommand );
			nNext += nSegmentsPackSize;
		}
		if ( nNext < int64_t( cutoffs[i] ) - nSegmentsPackSize )
		{
			return Reject( "missing_prefix", i, int( nNext ) );
		}
	}
	for ( const SMPRecoveryCommand *pCommand : missing )
	{
		auto *pPacket = static_cast<CAISegmentFinishedPacket *>( pCommand->pPacket.GetPtr() );
		const int nOriginalClientID = pPacket->nClientID;
		pPacket->nClientID = players[pCommand->nPlayer].nClientID;
		OnAISegmentFinishedPacket( pPacket );
		pPacket->nClientID = nOriginalClientID;
	}
	return true;
}

void CMPTransceiver::AdvanceToNextSegment()
{
	pTimer->NextSegment();
	pAI->Segment();
	++nCommonSegment;
	nSegment = nCommonSegment % pConsts->nMaxLatency;
	PruneRecoveryCommands();
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
	{
		return true;
	}
	SPlayer &player = players[nPlayer];
	// Deduplicate before both receipt accounting and command deserialization.
	// Otherwise an old packet can reset another peer's receipt or execute twice.
	if ( pPacket->nSegment < 0 || pPacket->nSegment <= player.nLastReceivedSegment )
	{
		NGameX::MatchPacketTrace_Log(
				nCommonSegment,
				"RX",
				"AISegmentFinishedIgnored",
				pPacket->nClientID,
				fmt::format( "slot={} segment={} last_received={}", nPlayer, pPacket->nSegment, player.nLastReceivedSegment ) );
		return true;
	}
	// StartMission must not discard packets already received during loading.
	player.nLastReceivedSegment = pPacket->nSegment;
	player.recoveryPackets.push_back( pPacket );
	PruneRecoveryCommands();
	NGameX::MatchPacketTrace_Log( nCommonSegment,
	                              "RX",
	                              "CAISegmentFinishedPacket",
	                              pPacket->nClientID,
	                              fmt::format( "from_slot={} segment={} checksum={} cmd_count={}",
	                                           nPlayer,
	                                           pPacket->nSegment,
	                                           pPacket->ulCheckSum,
	                                           int( pPacket->aiCommands.size() ) ) );
	int nPlayerSegment = pPacket->nSegment % pConsts->nMaxLatency;
	//DebugTrace( "+++ receive SEG_PACK Plr %d, seg %d(%d), CS %u, %d cmds", nPlayer, nPlayerSegment, pPacket->nSegment, pPacket->ulCheckSum, pPacket->aiCommands.size() );
	// Pending departures may still need earlier command packets. Keep receipt
	// accounting until simulation removal, even after the manager stops waiting.
	SetSegmentFinished( nPlayer, nPlayerSegment, pPacket->ulCheckSum );
	const int nSegmentToExecuteCommand = GetSegmentToExecute( pPacket->nSegment );
	for ( std::list<SRawCommand>::iterator it = pPacket->aiCommands.begin(); it != pPacket->aiCommands.end(); ++it )
	{
		SRawCommand &rawCommand = *it;
		CObjectBase *pObj = pCmdsSerializer->MakeCommand( rawCommand.nTypeID );
		CPtr<IAILogicCommandB2> pCommand = dynamic_cast<IAILogicCommandB2 *>( pObj );
		NI_VERIFY( pCommand, "Wrong command type id", return true );
		{
			rawCommand.cmd.Seek( 0 );
			CObj<IBinSaver> pSaver = CreateBinSaver( &( rawCommand.cmd ), SAVER_MODE_READ );
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

	if ( nPlayer < 0 )
	{
		return false;
	}

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
		{
			return i;
		}
	}
	return -1;
}

bool CMPTransceiver::IsPlayerPresent( int nPlayer ) const
{
	return wMask & ( 1UL << nPlayer );
}

bool CMPTransceiver::IsPlayerWaitedFor( int nPlayer ) const
{
	return wWaitMask & ( 1UL << nPlayer );
}

void CMPTransceiver::StopWaitingForPlayer( int nPlayer, int nDropSegment )
{
	if ( nPlayer < 0 || nPlayer >= players.size() )
	{
		return;
	}
	if ( !IsPlayerWaitedFor( nPlayer ) )
	{
		return;
	}

	const unsigned long ulWaitMaskBefore = wWaitMask;
	wWaitMask &= ~( 1UL << nPlayer );
	NGameX::MatchPacketTrace_Log(
			nCommonSegment,
			"STATE",
			"PlayerStoppedWaiting",
			players[nPlayer].nClientID,
			fmt::format( "slot={} drop_seg={} pre_wait_mask={:08X} post_wait_mask={:08X} active_mask={:08X}",
	                 nPlayer,
	                 nDropSegment,
	                 ulWaitMaskBefore,
	                 wWaitMask,
	                 wMask ) );
}

bool CMPTransceiver::IsGameRunning() const
{
	return bIsGameRunning;
}

ICheckSumLog *CMPTransceiver::GetCheckSumLogger()
{
	return pCmdsHistory;
}

const NDb::SMapInfo *CMPTransceiver::GetMap() const
{
	return pMapInfo;
}

void CMPTransceiver::CheckRunGame()
{
	if ( bIsGameRunning )
	{
		return;
	}

	if ( !players[nMyLogicID].bLoaded )
	{
		players[nMyLogicID].bLoaded = true;
		pClient->SendGamePacket( new CTransciverCommonPacket( CTransciverCommonPacket::ET_LOADED ), true );
		bWaiting = false; // So that the following call works
		return;
	}

	bool bCanStart = true;
	uint32_t dwWaitFor = 0;
	for ( int i = 0; i < players.size(); ++i )
	{
		SPlayer &player = players[i];

		// A departure committed during loading releases the network barrier now;
		// its deterministic simulation removal runs once the survivors start.
		if ( IsPlayerWaitedFor( i ) && !player.bLoaded )
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
	Scene()->ResetTimer( GetCurrentTimeMilliseconds() );
}

void CMPTransceiver::EndGame()
{
	if ( bIsGameEnded )
	{
		return;
	}
	bIsGameEnded = true;
	SetDepartureRecovery( false );
	CommandTimeOut( false );
	SetLagState( 0, false );
	NGameX::MatchPacketTrace_Log( nCommonSegment,
	                              "STATE",
	                              "TransceiverEndGame",
	                              players[nMyLogicID].nClientID,
	                              fmt::format( "final_segment={}", nFinalSegment ) );
}

void CMPTransceiver::LeaveOutOfSync()
{
	//	WriteToPipe( PIPE_CHAT, StrFmt( "You were kicked because you were out of sync" ) );
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_DEBUG_WINDOW,
	                                         "You were kicked because you were out of sync" );
	NGameX::MatchPacketTrace_Log(
			nCommonSegment, "STATE", "LeaveOutOfSync", players[nMyLogicID].nClientID, "async_detected=1" );
	NGameX::MatchPacketTrace_SetFinalState( 0, 0, wWaitMask );
	NGameX::MatchPacketTrace_Flush( "async" );
	Singleton<IScenarioTracker>()->MissionCancel();
	EndGame();
}

const bool CMPTransceiver::IsGamePacket( const CNetPacket *pPacket )
{
	const CAISegmentFinishedPacket *pSegmentPacket = dynamic_cast<const CAISegmentFinishedPacket *>( pPacket );
	if ( pSegmentPacket )
	{
		return true;
	}
	const CTransciverCommonPacket *pInfoPacket = dynamic_cast<const CTransciverCommonPacket *>( pPacket );
	if ( pInfoPacket )
	{
		return true;
	}
	return false;
}

void CMPTransceiver::Init( IServerClient *_pClient, const SB2StartGameParams &params, int nMySlot )
{
	NI_ASSERT( !params.clients.empty(), "Empty clients list" );
	SReplayInfo replay;
	bCommandsFromHistory = false;
	bIsGameRunning = false;
	bIsGameEnded = false;
	bPlayerDropError = false;
	SetDepartureRecovery( false );
	departurePackets.clear();
	nLastSentSegment = -1;
	timeStartWaitingForRemovedPlayer = 0;
	pClient = _pClient;
	pCmdsSerializer = CreateAICmdsAutoMagic();
	pConsts = NGameX::GetNetConsts();
	pCmdsHistory = CreateCommandsHistory( replay );
	pAI = Singleton<IAILogic>();
	pTimer = Singleton<IGameTimer>();
	wMask = 0;
	wWaitMask = 0;
	nGameID = params.nGameID;
	int nCurrentPlayers = params.clients.size();
	pMapInfo = params.pMapInfo;
	players.assign( pMapInfo->players.size(), SPlayer() );
	DebugTrace( "+++ MPT: start game, %d plrs, me = %d", nCurrentPlayers, nMySlot );
	for ( int i = 0; i < nCurrentPlayers; ++i )
	{
		const SB2StartGameParams::SClient &client = params.clients[i];
		SPlayer &player = players[client.nPlayer];
		player.nClientID = client.nClientID;
		player.nTeam = client.nTeam;
		player.nLastReceivedSegment = -1;
		wMask |= ( 1UL << client.nPlayer );
		wWaitMask |= ( 1UL << client.nPlayer );
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
	playerRemovals.assign( players.size(), NMPPlayerDrop::SPlayerDrop() );
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
		const auto message = fmt::format( "WAIT({}/{})", segmFinished[nSegment], wWaitMask );
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_DEBUG_WINDOW + 4, message.c_str() );
		NTimer::STime curTime = pTimer->GetAbsTime();
		const uint16_t wMissingRemovedPlayers = GetMissingSegmentPlayers( nSegment ) & ~wWaitMask;
		if ( wMissingRemovedPlayers == 0 )
		{
			timeStartWaitingForRemovedPlayer = 0;
		}
		else if ( timeStartWaitingForRemovedPlayer == 0 )
		{
			timeStartWaitingForRemovedPlayer = curTime;
		}
		else if ( curTime - timeStartWaitingForRemovedPlayer > s_nInterruptTimeout )
		{
			// A disconnected peer cannot be kicked again to recover missing input.
			// End explicitly instead of skipping its commands or waiting forever.
			for ( int i = 0; i < players.size(); ++i )
			{
				if ( ( wMissingRemovedPlayers & ( 1U << i ) ) != 0 )
				{
					FailPlayerDrop( i, nCommonSegment, "missing_pre_drop_commands" );
					return;
				}
			}
		}
		if ( timeStartWaiting == 0 )
		{
			timeStartWaiting = curTime;
		}
		else
		{
			if ( curTime - timeStartWaiting > s_nInterruptTimeout )
			{
				ReportLags( wWaitMask & ~segmFinished[nSegment], false );
			}
		}
	}
	else
	{
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_DEBUG_WINDOW + 4, "" );
		ReportLags( 0, false );
		timeStartWaiting = 0;
		timeStartWaitingForRemovedPlayer = 0;
	}
}

void CMPTransceiver::ReportLags( uint32_t dwLaggers, bool bInitial )
{
	Singleton<IMPToUIManager>()->AddUIMessage( new SMPUILagMessage( dwLaggers, bInitial ) );
}

void CMPTransceiver::ReportAsnycToFile( int segment )
{
	FILE *fl = fopen( "last_async.txt", "w" );

	if ( !fl )
	{
		return;
	}

	fprintf( fl, "Segment: %d\n", segment );
	fprintf( fl, "PlayerID,\tPlayerClientID,\tPlayerNick,\tPlayerTeam,\tLastChecksum\n" );

	auto scenario = Singleton<IScenarioTracker>();

	std::string checksumsTxt = "";
	for ( int nPlayerIndex = 0; nPlayerIndex < 16; ++nPlayerIndex )
	{
		if ( !IsPlayerWaitedFor( nPlayerIndex ) )
		{
			continue;
		}

		const auto &playerInfos = scenario->GetMultiplayerInfo()->players;
		if ( nPlayerIndex >= playerInfos.size() )
		{
			continue;
		}

		std::string playerName = WideToUTF8( playerInfos[nPlayerIndex].wszName );
		checksumsTxt += fmt::format( "{},\t\t{},\t\t{},\t\t{},\t\t{}\n",
		                             nPlayerIndex,
		                             players[nPlayerIndex].nClientID,
		                             playerName.c_str(),
		                             players[nPlayerIndex].nTeam,
		                             checkSums[nPlayerIndex][segment] );
	}
	fprintf( fl, "%s", checksumsTxt.c_str() );
	fclose( fl );

	pAI->WriteDetailedChecksumInfo( "last_async_detailed_debug.txt" );
	pCmdsHistory->WriteCommandsHistoryToFile( "last_async_cmds.txt" );

#ifdef CHECKSUM_LIST_DEBUG
	FILE *fd = fopen( "last_async_debug.txt", "w" );

	if ( !fd )
	{
		return;
	}

	fprintf( fd, "seg,\thash\n" );
	for ( int i = 0; i < myHistoryHashes.size(); i++ )
	{
		fprintf( fd, "%d,\t%d\n", i, myHistoryHashes[i] );
	}

	fclose( fd );
#endif
}

bool CMPTransceiver::IsAsyncDetected( int nSegment )
{
	if ( nCommonSegment <= nLatency )
	{
		return false;
	}

	std::string szCheckSums = fmt::format( "Segment {}: ", nSegment );
	const uint32_t nMyCheckSum = checkSums[nMyLogicID][nSegment];
	int nOutOfSyncs = 0;
	for ( int nPlayerIndex = 0; nPlayerIndex < 16; ++nPlayerIndex )
	{
		if ( !IsPlayerWaitedFor( nPlayerIndex ) )
		{
			continue;
		}

		szCheckSums += fmt::format( "{},", checkSums[nPlayerIndex][nSegment] );
		if ( checkSums[nPlayerIndex][nSegment] != nMyCheckSum )
		{
			++nOutOfSyncs;
		}
	}
	if ( nOutOfSyncs > 0 )
	{
		if ( NGlobal::GetVar( "save_on_async", 0 ) != 0 )
		{
			NSaveLoad::SSaveInfo info;
			std::string szFilename = fmt::format( "async_{:1d}_{:03d}", nMyLogicID, nSegment );
			NMainLoop::Command( CreateICSave( szFilename ) );
			info.Write( NSaveLoad::GetSavePath() + szFilename + NSaveLoad::INFO_FILE_EXTENSION,
			            NStr::ToUnicode( szCheckSums ),
			            false,
			            false,
			            false );
			NGlobal::SetVar( "save_on_async", 0 );
			NMainLoop::Command( ML_COMMAND_MESSAGE_BOX,
			                    CICMessageBox::MakeConfigString( "MessageBoxWindowOk", L"ASYNC<br>Save created" ).c_str() );
			//pAI->DumpAfterAssinc();

			ReportAsnycToFile( nSegment );
		}

		if ( NGlobal::GetVar( "pause_on_async", 0 ) != 0 )
		{
			if ( NGlobal::GetVar( "MP_async", 0 ) == 0 )
			{
				NGlobal::SetVar( "MP_async", 2 );
			}
			pTimer->Pause( true, PAUSE_TYPE_USER_PAUSE );
		}
		else
		{
			LeaveOutOfSync();
		}
		return true;
	}
	return false;
}

int CMPTransceiver::ScheduleGameEnd( const int _nSegment )
{
	// Negative means a local decision; zero is a valid segment received from a peer.
	const int nFinishOnSegment = ( _nSegment >= 0 ) ? _nSegment : nCommonSegment;
	const int nRequestedFinalSegment = ( nFinishOnSegment - nFinishOnSegment % nSegmentsPackSize ) + nLatency;
	// Several teammates can detect the same loss on different frames. A later
	// announcement must never extend the wait beyond a peer's earlier finish.
	if ( nFinalSegment < 0 || nRequestedFinalSegment < nFinalSegment )
	{
		nFinalSegment = nRequestedFinalSegment;
	}
	NGameX::MatchPacketTrace_Log(
			nCommonSegment,
			"DECISION",
			"ScheduleGameEnd",
			players[nMyLogicID].nClientID,
			fmt::format( "requested_segment={} final_segment={}", nFinishOnSegment, nFinalSegment ) );

	return nFinalSegment - nLatency;
}

CMPTransceiver *CreateMPTransceiver( IServerClient *pClient, const SB2StartGameParams &params, int nMySlot )
{
	CMPTransceiver *pTransceiver = new CMPTransceiver();
	pTransceiver->Init( pClient, params, nMySlot );

	return pTransceiver;
}

REGISTER_SAVELOAD_CLASS( GAMEX, 0x1713C440, CMPTransceiver );

START_REGISTER( MPTransceiver )
REGISTER_VAR_EX( "multiplayer_pause_timeout", NGlobal::VarIntHandler, &s_nInterruptTimeout, 5000, STORAGE_NONE );
FINISH_REGISTER
