#include "stdafx.h"

#include "Client/ServerClientInterface.h"
#include "MPLANTest.h"
#include "MPManagerMode.h"
#include "MPPacketTraceLog.h"
#include "MPTransceiver.h"
#include "MultiplayerNetPackets.h"
#include "ScenarioTracker.h"
#include "Server_Client_Common/GamePackets.h"

#include <cstdint>

#include <fmt/format.h>

// CMPManagerMode - client management

bool CMPManagerMode::HostAssignNewClient( int nClientID )
{
	if ( !IsGameHost() )
	{
		return false;
	}

	if ( nSlotsUsed == slots.size() )
	{
		CConnectGameFailed *pFailPkt = new CConnectGameFailed( nClientID, CConnectGameFailed::ER_MAX_PLAYERS_REACHED );
		pClient->SendPacket( pFailPkt );
		return false;
	}

	for ( int i = 0; i < slots.size(); ++i )
	{
		if ( !slots[i].bPresent && !slots[i].bAccept ) // Look for an empty slot that is not locked
		{
			slots[i].bPresent = true;
			slots[i].nClientID = nClientID;

			// Send slot number
			CPtr<CSlotNumberPacket> pIDPacket = new CSlotNumberPacket( nClientID, i, ulGameCheckSum );
			pClient->SendGamePacket( pIDPacket, false );
			//pClient->SendPacket( pIDPacket );
			++nSlotsUsed;

			// Send own slot info, the rest it should get from others
			//pClient->SendGamePacket( new CB2SlotInfoPacket( nClientID, nOwnSlot, slots[nOwnSlot] ), false );
			UpdateMyConnectivityMask(); // Sends own slot info inside

			//Send game info
			CPtr<CB2GameSpecificInfoPacket> pGameInfoPkt = new CB2GameSpecificInfoPacket;
			pGameInfoPkt->nClientID = nClientID;
			pGameInfoPkt->info = gameDesc;
			pClient->SendGamePacket( pGameInfoPkt, false );

			// Send closed slots
			for ( int nSlot = 0; nSlot < slots.size(); ++nSlot )
			{
				if ( !slots[nSlot].bPresent && slots[nSlot].bAccept )
				{
					pClient->SendGamePacket( new CB2SlotInfoPacket( nClientID, nSlot, slots[nSlot] ), false );
				}
			}

			return true;
		}
	}
	return false;
}

void CMPManagerMode::ClientAssignNewClient( int nClientID )
{
	// Send reply to new client with own name
	// Since we are "client", we already know own slot, and can send it
	pClient->SendGamePacket( new CB2SlotInfoPacket( nClientID, nOwnSlot, slots[nOwnSlot] ), false );
}

void CMPManagerMode::JoiningClientAssignNewClient( int nClientID )
{
	pendingClients.push_back( nClientID );
}

void CMPManagerMode::SendLagInfo()
{
	if ( !bWaitWindowShown )
	{
		return;
	}

	bool bOwnLag = ( dwUserPausedPlayers & ( 1UL << nOwnSlot ) ) != 0;
	NTimer::STime curTime = GameTimer()->GetAbsTime();
	CPtr<SMPUILagInfoMessage> pInfoMsg = new SMPUILagInfoMessage( bOwnLag );

	// Fill the info
	if ( bOwnLag )
	{
		const int nTimeLeft = (std::max)( GetRemainingLagTime( nOwnSlot, curTime ) - GetLagReserveMilliseconds(), 0 );
		pInfoMsg->nOwnTimeLeft = nTimeLeft / 1000;
	}
	for ( int i = 0; i < slots.size(); ++i )
	{
		if ( i == nOwnSlot || !IsPlayerLagging( i ) )
		{
			continue;
		}

		int nTimeLeft = GetRemainingLagTime( i, curTime );
		if ( ( dwUserPausedPlayers & ( 1UL << i ) ) != 0 )
		{
			// An explicit pause ends at the network reserve, not when the entire
			// disconnect allowance expires. Every observer shows that same limit.
			nTimeLeft = (std::max)( nTimeLeft - GetLagReserveMilliseconds(), 0 );
		}
		SMPUILagInfoMessage::SLagItem newItem;
		newItem.szName = slots[i].szName;
		newItem.nSecondsLeft = bInitialLoadInProgress ? -1 : nTimeLeft / 1000;
		pInfoMsg->lags.push_back( newItem );
	}

	PushMessage( pInfoMsg );
}

bool CMPManagerMode::IsPlayerPresent( int nPlayer )
{
	if ( nPlayer < 0 || nPlayer >= slots.size() )
	{
		return false;
	}
	return slots[nPlayer].bPresent;
}

uint32_t CMPManagerMode::GetPresentMask() const
{
	uint32_t dwMask = 0;
	for ( int i = 0; i < slots.size(); ++i )
	{
		if ( slots[i].bPresent )
		{
			dwMask |= ( 1UL << i );
		}
	}
	return dwMask;
}

bool CMPManagerMode::IsPlayerLagging( int nPlayer )
{
	if ( !IsPlayerPresent( nPlayer ) )
	{
		return false;
	}
	return ( dwLaggers & ( 1UL << nPlayer ) );
}

int CMPManagerMode::GetSlotByClientID( int nClientID ) const
{
	for ( int i = 0; i < slots.size(); ++i )
	{
		if ( slots[i].nClientID == nClientID )
		{
			return i;
		}
	}
	return -1;
}

int CMPManagerMode::GetSlotClientID( int nSlot )
{
	if ( nSlot < 0 || nSlot >= slots.size() )
	{
		return -1;
	}

	if ( nSlot == nOwnSlot )
	{
		const int nOwnClientID = GetOwnClientID();
		if ( nOwnClientID >= 0 )
		{
			return nOwnClientID;
		}
	}

	return slots[nSlot].nClientID;
}

bool CMPManagerMode::IsGameControlHost()
{
	const int nOwnClientID = GetOwnClientID();
	return nOwnClientID >= 0 && nOwnClientID == nHostClientID;
}

int CMPManagerMode::GetReplacementHostClientID( int nRemovedClientID )
{
	for ( int i = 0; i < slots.size(); ++i )
	{
		const SMPSlot &slot = slots[i];
		const int nClientID = GetSlotClientID( i );
		// A received drop stops lockstep waiting immediately, but the slot can
		// remain visible until its announced segment. Never elect that leaver.
		const bool bDropScheduled = i < playerDrops.size() && playerDrops[i].GetSegment() >= 0;
		if ( slot.bPresent && !bDropScheduled && nClientID >= 0 && nClientID != nRemovedClientID )
		{
			return nClientID;
		}
	}
	return -1;
}

void CMPManagerMode::PromoteGameControlHostAfterRemoval( int nRemovedClientID )
{
	if ( nRemovedClientID != nHostClientID )
	{
		return;
	}

	const int nOldHostClientID = nHostClientID;
	for ( NMPPlayerDrop::SPlayerDrop &drop : playerDrops )
	{
		drop.RememberCorrectionAuthority( nOldHostClientID );
	}
	const int nOldHostSlot = GetSlotByClientID( nOldHostClientID );
	if ( nOldHostSlot >= 0 && nOldHostSlot < playerDrops.size() )
	{
		// Backend disconnect and the host's final announcement use different routes.
		// Keep authority for that host's own departure after promoting its successor.
		playerDrops[nOldHostSlot].RememberHost( nOldHostClientID );
	}
	nHostClientID = GetReplacementHostClientID( nRemovedClientID );
	NGameX::MatchPacketTrace_Log(
			IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
			"STATE",
			"PromoteGameControlHostAfterRemoval",
			GetOwnClientID(),
			fmt::format( "old_host={} new_host={} removed={}", nOldHostClientID, nHostClientID, nRemovedClientID ) );
	OnGameControlHostChanged( nOldHostClientID, nHostClientID, nRemovedClientID );
}

bool CMPManagerMode::IsAuthoritativeDropPacket( const class CB2DropPlayerAtSegmentPacket *pPacket )
{
	if ( !pPacket )
	{
		return false;
	}

	const int nSlotToDrop = pPacket->nSlotToDrop;
	if ( nSlotToDrop < 0 || nSlotToDrop >= slots.size() || nSlotToDrop >= playerDrops.size() ||
	     !IsValid( pTransceiver ) ||
	     !NMPPlayerDrop::SPlayerDrop::IsValidSegment( pPacket->nSegment, pTransceiver->GetLatency() ) )
	{
		return false;
	}

	const int nTargetClientID = GetSlotClientID( nSlotToDrop );
	const int nReplacementHostClientID =
			nTargetClientID == nHostClientID ? GetReplacementHostClientID( nHostClientID ) : -1;
	if ( !playerDrops[nSlotToDrop].IsAuthoritative(
					 pPacket->nClientID, nHostClientID, nTargetClientID, nReplacementHostClientID ) )
	{
		return false;
	}

	if ( nTargetClientID == nHostClientID )
	{
		const int nOldHostClientID = nHostClientID;
		for ( NMPPlayerDrop::SPlayerDrop &drop : playerDrops )
		{
			drop.RememberCorrectionAuthority( nOldHostClientID );
		}
		playerDrops[nSlotToDrop].RememberHost( nOldHostClientID );
		nHostClientID = nReplacementHostClientID;
		NGameX::MatchPacketTrace_Log( IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
		                              "STATE",
		                              "PromoteGameControlHostFromDropPacket",
		                              GetOwnClientID(),
		                              fmt::format( "old_host={} new_host={} slot={} target_seg={}",
		                                           nOldHostClientID,
		                                           nHostClientID,
		                                           nSlotToDrop,
		                                           pPacket->nSegment ) );
		OnGameControlHostChanged( nOldHostClientID, nHostClientID, nOldHostClientID );
	}

	return true;
}

void CMPManagerMode::BroadcastSynchronizedPlayerDrop( int nSlot, int nSegment, const char *szReason )
{
	if ( !IsGameControlHost() || !IsValid( pTransceiver ) || pTransceiver->HasPlayerDropError() ||
	     !NMPPlayerDrop::SPlayerDrop::IsValidSegment( nSegment, pTransceiver->GetLatency() ) )
	{
		return;
	}

	NGameX::MatchPacketTrace_Log(
			nSegment,
			"TX",
			"CB2DropPlayerAtSegmentPacket",
			GetOwnClientID(),
			fmt::format( "slot={} target_seg={} reason={}", nSlot, nSegment, szReason ? szReason : "" ) );
	BroadcastGameControlPacket( new CB2DropPlayerAtSegmentPacket( 0, nSlot, nSegment ) );
}

bool CMPManagerMode::ScheduleSynchronizedPlayerDrop( int nSlot, int nSegment )
{
	if ( nSlot < 0 || nSlot >= slots.size() || !IsValid( pTransceiver ) )
	{
		return false;
	}

	// Remember and apply the earliest authoritative drop segment for this slot.
	if ( playerDrops.size() != slots.size() )
	{
		playerDrops.assign( slots.size(), NMPPlayerDrop::SPlayerDrop() );
	}
	const int nCurrent = playerDrops[nSlot].GetSegment();
	// Validate in the transceiver before changing membership or the manager's
	// retained decision. It refuses to rewrite a simulation tick already executed.
	if ( !pTransceiver->SchedulePlayerRemoval( nSlot, nSegment ) )
	{
		return false;
	}
	if ( playerDrops[nSlot].Schedule( nSegment, pTransceiver->GetCurrentCommonSegment(), pTransceiver->GetLatency() ) !=
	     NMPPlayerDrop::SCHEDULED )
	{
		return false;
	}

	const uint32_t dwLaggersBefore = dwLaggers;
	const uint32_t dwLaggersOldBefore = dwLaggersOld;
	// Stop local lag accounting for this slot immediately to avoid repeated kick scheduling.
	dwNetworkLaggers &= ~( 1UL << nSlot );
	dwLaggers &= ~( 1UL << nSlot );
	dwLaggersOld &= ~( 1UL << nSlot );
	dwUserPausedPlayers &= ~( 1UL << nSlot );
	lags[nSlot].ResetKickVotes();
	lags[nSlot].timeStartLag = 0;
	for ( SLagInfo &lag : lags )
	{
		// A removed player is no longer a voter for any other pending timeout.
		lag.dwHatedBy &= ~( 1UL << nSlot );
	}
	if ( dwLaggers == 0 )
	{
		ShowWaitWindow( false );
	}
	NGameX::MatchPacketTrace_RecordDropScheduled( nSlot, nSegment );
	NGameX::MatchPacketTrace_Log(
			IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
			"DECISION",
			"ScheduleSynchronizedPlayerDrop",
			GetOwnClientID(),
			fmt::format(
					"slot={} target_seg={} prev_target_seg={} pre_laggers={:08X} post_laggers={:08X} pre_laggers_old={:08X} post_laggers_old={:08X}",
					nSlot,
					nSegment,
					nCurrent,
					dwLaggersBefore,
					dwLaggers,
					dwLaggersOldBefore,
					dwLaggersOld ) );

	// A removed local player must not retain a pause whose ownership bit was cleared above.
	if ( nSlot == nOwnSlot )
	{
		pTransceiver->CommandTimeOut( false );
	}
	if ( bWinFromPlayerDrops )
	{
		// Keep the final simulation boundary aligned with corrected departures.
		// Other victory reasons retain their independently announced finish tick.
		ScheduleWinGame();
	}
	return true;
}

void CMPManagerMode::ApplyScheduledSlotDrops()
{
	if ( !IsGameRunning() || !IsValid( pTransceiver ) )
	{
		return;
	}
	if ( playerDrops.size() != slots.size() )
	{
		return;
	}

	const int nCommonSegment = pTransceiver->GetCurrentCommonSegment();
	for ( int i = 0; i < slots.size(); ++i )
	{
		const int nDropSegment = playerDrops[i].GetSegment();
		if ( nDropSegment < 0 || nDropSegment > nCommonSegment )
		{
			continue;
		}

		// Retain the authoritative boundary after applying it: duplicate removals
		// stay idempotent and surviving teammates use it for the same match-end tick.
		if ( IsPlayerPresent( i ) )
		{
			const uint32_t dwPrePresentMask = GetPresentMask();
			slots[i].bPresent = false;
			const uint32_t dwPostPresentMask = GetPresentMask();
			NGameX::MatchPacketTrace_Log(
					nCommonSegment,
					"STATE",
					"ApplyScheduledSlotDrops",
					GetOwnClientID(),
					fmt::format( "slot={} target_seg={} applied_seg={} pre_present={:08X} post_present={:08X}",
			                 i,
			                 nDropSegment,
			                 nCommonSegment,
			                 dwPrePresentMask,
			                 dwPostPresentMask ) );
			NGameX::MatchPacketTrace_RecordDropApplied( i, nCommonSegment );
		}
	}
}

void CMPManagerMode::ResolvePendingHostDrops()
{
	// Each iteration retires one known host. Do not skip the authority chain:
	// peers that have not received a later disconnect must validate the same handoffs.
	for ( size_t i = 0; i < slots.size() && IsValid( pTransceiver ) && !pTransceiver->HasPlayerDropError(); ++i )
	{
		const int nHostSlot = GetSlotByClientID( nHostClientID );
		if ( nHostSlot < 0 || nHostSlot >= playerDrops.size() )
		{
			return;
		}
		const int nOldHost = nHostClientID;
		NMPPlayerDrop::SPlayerDrop &drop = playerDrops[nHostSlot];
		const int nDeferredSegment = drop.TakeDeferredSegment();
		if ( nDeferredSegment >= 0 )
		{
			// This self-announcement was authenticated when received, but its sender
			// had not become our host yet. It can now pass the ordinary authority check.
			CPtr<CB2DropPlayerAtSegmentPacket> pPacket =
					new CB2DropPlayerAtSegmentPacket( nOldHost, nHostSlot, nDeferredSegment );
			OnB2DropPlayerAtSegmentPacket( pPacket );
		}
		else if ( drop.IsDisconnected() )
		{
			RemoveClient( nOldHost, false );
		}
		if ( nHostClientID == nOldHost )
		{
			return;
		}
	}
}

void CMPManagerMode::RemoveClient( int nClientID, bool bKicked )
{
	NGameX::MatchPacketTrace_Log( IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
	                              "DECISION",
	                              "RemoveClient",
	                              nClientID,
	                              fmt::format( "kicked={} running={} in_room={} room_host={} control_host={} host_id={}",
	                                           bKicked ? 1 : 0,
	                                           IsGameRunning() ? 1 : 0,
	                                           IsInGameRoom() ? 1 : 0,
	                                           IsGameHost() ? 1 : 0,
	                                           IsGameControlHost() ? 1 : 0,
	                                           nHostClientID ) );

	if ( IsInGameRoom() && !IsGameHost() && nClientID == nHostClientID )
	{
		// Game Host left, close everything
		PushMessage( new SMPUIGameRoomInitMessage( SMPUIGameRoomInitMessage::ERR_GAME_KILLED ) );
		OnLeaveGame();
		return;
	}
	// Otherwise, remove client from slot
	for ( int i = 0; i < slots.size(); ++i )
	{
		if ( slots[i].nClientID != nClientID )
		{
			continue;
		}

		// In active game, never mutate lockstep membership here.
		// The current game-control host announces deterministic drops. If that host
		// vanished, the deterministic replacement host takes over that job.
		if ( IsGameRunning() )
		{
			const bool bRemovingGameControlHost = ( nClientID == nHostClientID );
			const int nReplacementHostClientID = bRemovingGameControlHost ? GetReplacementHostClientID( nClientID ) : -1;
			const bool bOwnsDropAuthority =
					IsGameControlHost() || ( bRemovingGameControlHost && GetOwnClientID() == nReplacementHostClientID );
			if ( IsValid( pTransceiver ) )
			{
				if ( playerDrops.size() != slots.size() )
				{
					playerDrops.assign( slots.size(), NMPPlayerDrop::SPlayerDrop() );
				}

				// Retain a disconnect observed before this client becomes control host.
				// A later handoff must not elect it and wait for a second notification.
				playerDrops[i].RememberDisconnect();

				// Schedule by lockstep membership, not slot UI state.
				// Slot presence can already be false here while transceiver still waits for this player.
				if ( playerDrops[i].GetSegment() < 0 && ( pTransceiver->GetPlayerMask() & ( 1UL << i ) ) != 0 )
				{
					const int nDropSegment = pTransceiver->GetCurrentCommonSegment();
					if ( bOwnsDropAuthority )
					{
						if ( bRemovingGameControlHost )
						{
							PromoteGameControlHostAfterRemoval( nClientID );
						}

						ScheduleSynchronizedPlayerDrop( i, nDropSegment );

						if ( IsGameControlHost() )
						{
							BroadcastSynchronizedPlayerDrop(
									i, nDropSegment, bRemovingGameControlHost ? "host_removed" : "remove_client" );
						}
						else
						{
							NGameX::MatchPacketTrace_Log( nDropSegment,
							                              "DECISION",
							                              "RemoveClientLocalDropFallback",
							                              GetOwnClientID(),
							                              fmt::format( "slot={} target_seg={}", i, nDropSegment ) );
						}
					}
					else
					{
						NGameX::MatchPacketTrace_Log(
								nDropSegment,
								"DECISION",
								"RemoveClientAwaitingDropAuthority",
								GetOwnClientID(),
								fmt::format(
										"slot={} authority={}", i, bRemovingGameControlHost ? nReplacementHostClientID : nHostClientID ) );
					}
				}
				else if ( bRemovingGameControlHost && GetOwnClientID() == nReplacementHostClientID )
				{
					PromoteGameControlHostAfterRemoval( nClientID );
				}
			}
			return;
		}

		slots[i].bPresent = false;
		if ( IsInGameRoom() )
		{
			SMPSlot emptySlot;
			if ( bKicked )
			{
				emptySlot.bAccept = slots[i].bAccept;
			}
			slots[i] = emptySlot;
			slots[i].nClientID = -1;
			--nSlotsUsed;

			SMPUIUpdateSlotMessage *pUpdateMsg = new SMPUIUpdateSlotMessage;
			pUpdateMsg->nSlot = i;
			pUpdateMsg->info = emptySlot;
			PushMessage( pUpdateMsg );
			if ( pLanTester )
			{
				pLanTester->ClientRemoved( nClientID );
			}

			OnGameRoomClientRemoved();
			UpdateMyConnectivityMask();
		}
		return;
	}
}

void CMPManagerMode::UpdateMyConnectivityMask()
{
	uint16_t &wMask = slots[nOwnSlot].wConnectedTo;
	uint16_t wNewMask = 0;
	for ( int i = 0; i < slots.size(); ++i )
	{
		if ( slots[i].bPresent )
		{
			wNewMask |= ( 1 << i );
		}
	}
	if ( wNewMask != wMask )
	{
		DebugTrace( "+++ Connectivity Mask change %d -> %d ", wMask, wNewMask );
		wMask = wNewMask;
		pClient->SendGamePacket( new CB2SlotInfoPacket( nHostClientID, nOwnSlot, slots[nOwnSlot] ), IsGameHost() );
	}
	CheckJoinGameConditions();
}
