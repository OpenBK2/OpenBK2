#include "stdafx.h"

#include "MPManagerMode.h"
#include "MPPacketTraceLog.h"
#include "MPTransceiver.h"
#include "MultiplayerNetPackets.h"

#include <algorithm>
#include <limits>
#include <map>
#include <utility>

#include <fmt/format.h>

bool CMPManagerMode::IsDepartureRecoveryActive() const
{
	return dwRecoveryRound != dwCompletedRecovery;
}

uint32_t CMPManagerMode::GetKnownDepartures() const
{
	uint32_t dwResult = dwRecoveryRound;
	for ( size_t i = 0; i < playerDrops.size() && i < 32; ++i )
	{
		if ( playerDrops[i].IsDisconnected() || playerDrops[i].GetSegment() >= 0 )
		{
			dwResult |= uint32_t( 1 ) << i;
		}
	}
	return dwResult & dwInitialPlayers;
}

int CMPManagerMode::GetRecoveryCoordinator( uint32_t dwDeparted ) const
{
	for ( size_t i = 0; i < slots.size() && i < 32; ++i )
	{
		if ( ( dwInitialPlayers & ~dwDeparted & ( uint32_t( 1 ) << i ) ) != 0 )
		{
			return int( i );
		}
	}
	return -1;
}

void CMPManagerMode::BeginDepartureRecovery( uint32_t dwDeparted )
{
	if ( GetMode() != ENM_NIVAL_NET || !IsGameRunning() || !IsValid( pTransceiver ) )
	{
		return;
	}
	dwDeparted = ( dwDeparted | GetKnownDepartures() ) & dwInitialPlayers;
	if ( dwDeparted == dwRecoveryRound || dwDeparted == 0 )
	{
		return;
	}
	if ( nOwnSlot < 0 || nOwnSlot >= slots.size() || slots.size() > 32 )
	{
		return;
	}
	if ( ( dwDeparted & ( uint32_t( 1 ) << nOwnSlot ) ) != 0 )
	{
		// A timeout decision may reach a still-running excluded client. It must
		// leave, rather than run a separate simulation outside the survivor cohort.
		LoseGame();
		return;
	}
	const NTimer::STime timeNow = GameTimer()->GetAbsTime();
	if ( !IsDepartureRecoveryActive() )
	{
		for ( int i = 0; i < lags.size(); ++i )
		{
			AccountLagTime( i, timeNow );
		}
	}
	dwRecoveryRound = dwDeparted;
	recoveryReports.assign( slots.size(), nullptr );
	pTransceiver->SetDepartureRecovery( true );
	// No simulation, outcome checks or ordinary kick voting may run between
	// this snapshot and the matching commit. A larger failed set starts afresh.
	CPtr<CB2DepartureRecoveryPacket> request = new CB2DepartureRecoveryPacket;
	request->nGameID = nGameID;
	request->dwDeparted = dwRecoveryRound;
	BroadcastGameControlPacket( request );

	CPtr<CB2DepartureRecoveryPacket> report = new CB2DepartureRecoveryPacket;
	report->nClientID = GetOwnClientID();
	report->nGameID = nGameID;
	report->ePhase = CB2DepartureRecoveryPacket::SNAPSHOT;
	report->dwDeparted = dwRecoveryRound;
	report->nCommonSegment = pTransceiver->GetCurrentCommonSegment();
	report->cutoffs.resize( slots.size(), -1 );
	for ( size_t i = 0; i < playerDrops.size(); ++i )
	{
		report->cutoffs[i] = playerDrops[i].GetSegment();
	}
	if ( pRecoveryCommit )
	{
		// A former coordinator may have delivered a commit to only some peers.
		// Reports carry its complete repair input even after ordinary cache expiry.
		report->commands = pRecoveryCommit->commands;
	}
	std::vector<SMPRecoveryCommand> recentCommands;
	pTransceiver->GetRecoveryCommands( dwRecoveryRound, &recentCommands );
	report->commands.insert( report->commands.end(), recentCommands.begin(), recentCommands.end() );
	recoveryReports[nOwnSlot] = report;
	BroadcastGameControlPacket( report );
	NGameX::MatchPacketTrace_Log( report->nCommonSegment,
	                              "STATE",
	                              "DepartureRecoveryBegin",
	                              GetOwnClientID(),
	                              fmt::format( "departed={:08X} coordinator_slot={}",
	                                           dwRecoveryRound,
	                                           GetRecoveryCoordinator( dwRecoveryRound ) ) );
	ShowWaitWindow( true );
	SendLagInfo();
}

bool CMPManagerMode::OnDepartureRecoveryPacket( CB2DepartureRecoveryPacket *pPacket )
{
	if ( GetMode() != ENM_NIVAL_NET || !IsGameRunning() || !IsValid( pTransceiver ) || pPacket->nGameID != nGameID ||
	     pPacket->dwDeparted == 0 || ( pPacket->dwDeparted & ~dwInitialPlayers ) != 0 || slots.size() > 32 )
	{
		return true;
	}
	const int nSender = GetSlotByClientID( pPacket->nClientID );
	if ( nSender < 0 || ( dwInitialPlayers & ( uint32_t( 1 ) << nSender ) ) == 0 ||
	     ( pPacket->dwDeparted & ( uint32_t( 1 ) << nSender ) ) != 0 )
	{
		return true;
	}
	if ( pPacket->ePhase != CB2DepartureRecoveryPacket::REQUEST &&
	     pPacket->ePhase != CB2DepartureRecoveryPacket::SNAPSHOT &&
	     pPacket->ePhase != CB2DepartureRecoveryPacket::COMMIT )
	{
		return true;
	}
	// A regular peer cannot invent another player's departure. It may repeat
	// locally confirmed membership loss. The existing host may also commit a
	// timeout vote; its deterministic replacement may report loss of that host.
	uint32_t dwAllowed = GetKnownDepartures();
	if ( pPacket->nClientID == nHostClientID )
	{
		dwAllowed |= pPacket->dwDeparted;
	}
	else if ( pPacket->nClientID == GetReplacementHostClientID( nHostClientID ) )
	{
		const int nHostSlot = GetSlotByClientID( nHostClientID );
		if ( nHostSlot >= 0 )
		{
			dwAllowed |= uint32_t( 1 ) << nHostSlot;
		}
	}
	if ( ( pPacket->dwDeparted & ~dwAllowed ) != 0 )
	{
		return true;
	}
	const uint32_t dwMerged = pPacket->dwDeparted | GetKnownDepartures();
	if ( dwMerged != dwRecoveryRound )
	{
		BeginDepartureRecovery( dwMerged );
	}
	if ( IsGameRunning() && pPacket->dwDeparted == dwRecoveryRound &&
	     pPacket->ePhase == CB2DepartureRecoveryPacket::REQUEST )
	{
		// A peer may have rejected our earlier snapshot before its ordered
		// membership notification arrived. Reply with the same frozen snapshot,
		// never a newly sampled tick or newly arrived command tail.
		if ( !IsDepartureRecoveryActive() && pRecoveryCommit && nOwnSlot == GetRecoveryCoordinator( dwRecoveryRound ) )
		{
			BroadcastGameControlPacket( pRecoveryCommit );
		}
		else if ( nOwnSlot >= 0 && nOwnSlot < recoveryReports.size() && recoveryReports[nOwnSlot] )
		{
			BroadcastGameControlPacket( recoveryReports[nOwnSlot] );
		}
	}
	if ( !IsGameRunning() || !IsDepartureRecoveryActive() || pPacket->dwDeparted != dwRecoveryRound )
	{
		return true;
	}
	NGameX::MatchPacketTrace_Log( pTransceiver->GetCurrentCommonSegment(),
	                              "RX",
	                              "DepartureRecoveryPacket",
	                              pPacket->nClientID,
	                              fmt::format( "phase={} departed={:08X} frozen_seg={} packets={}",
	                                           int( pPacket->ePhase ),
	                                           pPacket->dwDeparted,
	                                           pPacket->nCommonSegment,
	                                           pPacket->commands.size() ) );
	if ( pPacket->ePhase == CB2DepartureRecoveryPacket::SNAPSHOT )
	{
		if ( pPacket->nCommonSegment < 0 || pPacket->cutoffs.size() != slots.size() )
		{
			return true;
		}
		// Each survivor publishes exactly one frozen snapshot for this failed set.
		if ( !recoveryReports[nSender] )
		{
			recoveryReports[nSender] = pPacket;
		}
	}
	else if ( pPacket->ePhase == CB2DepartureRecoveryPacket::COMMIT &&
	          nSender == GetRecoveryCoordinator( dwRecoveryRound ) )
	{
		AcceptDepartureRecovery( pPacket );
	}
	return true;
}

void CMPManagerMode::UpdateDepartureRecovery()
{
	if ( !IsDepartureRecoveryActive() || !IsValid( pTransceiver ) || pTransceiver->GetSegmentsPackSize() <= 0 ||
	     nOwnSlot != GetRecoveryCoordinator( dwRecoveryRound ) )
	{
		return;
	}
	const uint32_t dwSurvivors = dwInitialPlayers & ~dwRecoveryRound;
	CPtr<CB2DepartureRecoveryPacket> commit = new CB2DepartureRecoveryPacket;
	commit->nClientID = GetOwnClientID();
	commit->nGameID = nGameID;
	commit->ePhase = CB2DepartureRecoveryPacket::COMMIT;
	commit->dwDeparted = dwRecoveryRound;
	commit->cutoffs.assign( slots.size(), -1 );
	std::vector<int> lastPackets( slots.size(), -1 );
	std::map<std::pair<int, int>, SMPRecoveryCommand> commands;
	for ( size_t i = 0; i < slots.size(); ++i )
	{
		if ( ( dwSurvivors & ( uint32_t( 1 ) << i ) ) == 0 )
		{
			continue;
		}
		if ( i >= recoveryReports.size() || !recoveryReports[i] )
		{
			return;
		}
		const CB2DepartureRecoveryPacket &report = *recoveryReports[i];
		commit->nCommonSegment = (std::max)( commit->nCommonSegment, report.nCommonSegment );
		for ( size_t nPlayer = 0; nPlayer < slots.size(); ++nPlayer )
		{
			const int nPrevious = report.cutoffs[nPlayer];
			if ( nPrevious < 0 )
			{
				continue;
			}
			if ( !NMPPlayerDrop::SPlayerDrop::IsValidSegment( nPrevious, pTransceiver->GetLatency() ) ||
			     ( commit->cutoffs[nPlayer] >= 0 && commit->cutoffs[nPlayer] != nPrevious ) )
			{
				// Never rewrite an accepted decision after another survivor may
				// have executed it. Conflicting history must remain frozen.
				return;
			}
			commit->cutoffs[nPlayer] = nPrevious;
		}
		for ( const SMPRecoveryCommand &command : report.commands )
		{
			if ( command.nPlayer < 0 || command.nPlayer >= slots.size() || command.nSegment < 0 ||
			     command.nSegment % pTransceiver->GetSegmentsPackSize() != 0 ||
			     ( dwRecoveryRound & ( uint32_t( 1 ) << command.nPlayer ) ) == 0 ||
			     !CMPTransceiver::SameRecoveryCommand( command, command ) )
			{
				return;
			}
			lastPackets[command.nPlayer] = (std::max)( lastPackets[command.nPlayer], command.nSegment );
			const auto inserted = commands.emplace( std::make_pair( command.nPlayer, command.nSegment ), command );
			if ( !inserted.second && !CMPTransceiver::SameRecoveryCommand( inserted.first->second, command ) )
			{
				return;
			}
		}
	}
	for ( size_t i = 0; i < slots.size(); ++i )
	{
		if ( ( dwRecoveryRound & ( uint32_t( 1 ) << i ) ) == 0 || commit->cutoffs[i] >= 0 )
		{
			continue;
		}
		const int nPack = pTransceiver->GetSegmentsPackSize();
		const int64_t nCutoff = lastPackets[i] < 0 ? nPack : int64_t( lastPackets[i] ) + 2 * int64_t( nPack );
		if ( nPack <= 0 || nCutoff > ( std::numeric_limits<int>::max )() - pTransceiver->GetLatency() ||
		     nCutoff + pTransceiver->GetLatency() < commit->nCommonSegment )
		{
			return;
		}
		commit->cutoffs[i] = int( nCutoff );
	}
	// Check the union against every frozen observer before anyone can resume.
	// A new target's cache includes its highest actual receipt; an already
	// accepted cutoff has previously passed this same complete-prefix check.
	for ( size_t nObserver = 0; nObserver < slots.size(); ++nObserver )
	{
		if ( ( dwSurvivors & ( uint32_t( 1 ) << nObserver ) ) == 0 )
		{
			continue;
		}
		const CB2DepartureRecoveryPacket &report = *recoveryReports[nObserver];
		for ( size_t nPlayer = 0; nPlayer < slots.size(); ++nPlayer )
		{
			if ( ( dwRecoveryRound & ( uint32_t( 1 ) << nPlayer ) ) == 0 || report.cutoffs[nPlayer] >= 0 )
			{
				continue;
			}
			int nLast = -1;
			for ( const SMPRecoveryCommand &command : report.commands )
			{
				if ( command.nPlayer == nPlayer )
				{
					nLast = (std::max)( nLast, command.nSegment );
				}
			}
			const int nPack = pTransceiver->GetSegmentsPackSize();
			int64_t nNext = nLast < 0 ? 0 : int64_t( nLast ) + nPack;
			while ( nNext < int64_t( commit->cutoffs[nPlayer] ) - nPack )
			{
				if ( commands.find( std::make_pair( int( nPlayer ), int( nNext ) ) ) == commands.end() )
				{
					return;
				}
				nNext += nPack;
			}
		}
	}
	for ( const auto &entry : commands )
	{
		const SMPRecoveryCommand &command = entry.second;
		if ( command.nSegment < commit->cutoffs[command.nPlayer] - pTransceiver->GetSegmentsPackSize() )
		{
			commit->commands.push_back( command );
		}
	}
	// Queue the whole decision before resuming locally. If this client dies in
	// the recipient loop, every recipient that accepted it retains the payload
	// and reports it in the next round; old-round commits are fenced on freeze.
	BroadcastGameControlPacket( commit );
	AcceptDepartureRecovery( commit );
}

bool CMPManagerMode::AcceptDepartureRecovery( CB2DepartureRecoveryPacket *pPacket )
{
	if ( pPacket->cutoffs.size() != slots.size() || pPacket->nCommonSegment < pTransceiver->GetCurrentCommonSegment() )
	{
		return false;
	}
	for ( size_t i = 0; i < slots.size(); ++i )
	{
		if ( ( dwRecoveryRound & ( uint32_t( 1 ) << i ) ) == 0 )
		{
			continue;
		}
		const int nCutoff = pPacket->cutoffs[i];
		const int nPrevious = playerDrops[i].GetSegment();
		if ( !NMPPlayerDrop::SPlayerDrop::IsValidSegment( nCutoff, pTransceiver->GetLatency() ) ||
		     ( nPrevious >= 0 && nCutoff != nPrevious ) ||
		     ( nPrevious < 0 && nCutoff + pTransceiver->GetLatency() < pTransceiver->GetCurrentCommonSegment() ) )
		{
			return false;
		}
	}
	if ( !pTransceiver->RestoreRecoveryCommands( pPacket->commands, dwRecoveryRound, pPacket->cutoffs ) )
	{
		NGameX::MatchPacketTrace_Log( pTransceiver->GetCurrentCommonSegment(),
		                              "ERROR",
		                              "DepartureRecoveryMissingPrefix",
		                              GetOwnClientID(),
		                              fmt::format( "departed={:08X}", dwRecoveryRound ) );
		return false;
	}
	pRecoveryCommit = pPacket;
	for ( size_t i = 0; i < slots.size(); ++i )
	{
		if ( ( dwRecoveryRound & ( uint32_t( 1 ) << i ) ) != 0 && playerDrops[i].GetSegment() < 0 )
		{
			ScheduleSynchronizedPlayerDrop( int( i ), pPacket->cutoffs[i] );
		}
	}
	const int nOldHost = nHostClientID;
	const int nHostSlot = GetSlotByClientID( nHostClientID );
	if ( nHostSlot >= 0 && ( dwRecoveryRound & ( uint32_t( 1 ) << nHostSlot ) ) != 0 )
	{
		PromoteGameControlHostAfterRemoval( nOldHost );
	}
	dwCompletedRecovery = dwRecoveryRound;
	pTransceiver->SetDepartureRecovery( false );
	const NTimer::STime timeNow = GameTimer()->GetAbsTime();
	for ( size_t i = 0; i < lags.size(); ++i )
	{
		SLagInfo &lag = lags[i];
		lag.ResetKickVotes();
		if ( ( dwLaggersOld & ( uint32_t( 1 ) << i ) ) != 0 )
		{
			lag.timeStartLag = timeNow;
		}
	}
	UpdateLagState( true );
	NGameX::MatchPacketTrace_Log( pTransceiver->GetCurrentCommonSegment(),
	                              "STATE",
	                              "DepartureRecoveryCommitted",
	                              GetOwnClientID(),
	                              fmt::format( "departed={:08X}", dwRecoveryRound ) );
	return true;
}
