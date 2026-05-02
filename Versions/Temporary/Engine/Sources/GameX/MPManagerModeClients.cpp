#include "stdafx.h"

#include "MPManagerMode.h"
#include "Server_Client_Common/GamePackets.h"
#include "Client/ServerClientInterface.h"
#include "MultiplayerNetPackets.h"
#include "MPTransceiver.h"
#include "ScenarioTracker.h"
#include "MPPacketTraceLog.h"

#include "MPLANTest.h"

// CMPManagerMode - client management

bool CMPManagerMode::HostAssignNewClient( int nClientID )
{
	if ( !IsGameHost() )
		return false;

	if ( nSlotsUsed == slots.size() )
	{
		CConnectGameFailed *pFailPkt = new CConnectGameFailed( nClientID, CConnectGameFailed::ER_MAX_PLAYERS_REACHED );
		pClient->SendPacket( pFailPkt );
		return false;
	}

	for ( int i = 0; i < slots.size(); ++i )
	{
		if ( !slots[i].bPresent && !slots[i].bAccept )			// Look for an empty slot that is not locked
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
			UpdateMyConnectivityMask();		// Sends own slot info inside

			//Send game info
			CPtr<CB2GameSpecificInfoPacket> pGameInfoPkt = new CB2GameSpecificInfoPacket;
			pGameInfoPkt->nClientID = nClientID;
			pGameInfoPkt->info = gameDesc;
			pClient->SendGamePacket( pGameInfoPkt, false );

			// Send closed slots
			for ( int i = 0; i < slots.size(); ++i )
			{
				if ( !slots[i].bPresent && slots[i].bAccept )
					pClient->SendGamePacket( new CB2SlotInfoPacket( nClientID, i, slots[i] ), false );
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
		return;

	bool bOwnLag = IsPlayerLagging( nOwnSlot );
	NTimer::STime curTime = GameTimer()->GetAbsTime();
	CPtr<SMPUILagInfoMessage> pInfoMsg = new SMPUILagInfoMessage( bOwnLag );

	// Fill the info
	if ( bOwnLag )
	{
		int nTimeLeft = lags[nOwnSlot].nLagLeft - ( curTime - lags[nOwnSlot].timeStartLag );
		pInfoMsg->nOwnTimeLeft = nTimeLeft / 1000;
	}
	for ( int i = 0; i < slots.size(); ++i )
	{
		if ( i == nOwnSlot || !IsPlayerLagging( i ) )
			continue;

		SLagInfo &lagInfo = lags[i];
		int nTimeLeft = lagInfo.nLagLeft - ( curTime - lagInfo.timeStartLag );
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
		 return false;
	 return slots[nPlayer].bPresent;
}

DWORD CMPManagerMode::GetPresentMask() const
{
	DWORD dwMask = 0;
	for ( int i = 0; i < slots.size(); ++i )
	{
		if ( slots[i].bPresent )
			dwMask |= ( 1UL << i );
	}
	return dwMask;
}

bool CMPManagerMode::IsPlayerLagging( int nPlayer )
{
	if ( !IsPlayerPresent( nPlayer ) )
		return false;
	return ( dwLaggers & ( 1UL << nPlayer ) );
}

bool CMPManagerMode::HasPlayerStoppedLagging( int nPlayer )
{
	if ( !IsPlayerPresent( nPlayer ) )
		return false;
	return ( dwLaggersOld & ~dwLaggers & ( 1UL << nPlayer ) );
}

bool CMPManagerMode::HasPlayerStartedLagging( int nPlayer )
{
	if ( !IsPlayerPresent( nPlayer ) )
		return false;
	return ( ~dwLaggersOld & dwLaggers & ( 1UL << nPlayer ) );
}

int CMPManagerMode::GetSlotByClientID( int nClientID ) const
{
	for ( int i = 0; i < slots.size(); ++i )
	{
		if ( slots[i].nClientID == nClientID )
			return i;
	}
	return -1;
}

bool CMPManagerMode::IsGameControlHost()
{
	const int nOwnClientID = GetOwnClientID();
	return nOwnClientID >= 0 && nOwnClientID == nHostClientID;
}

int CMPManagerMode::GetReplacementHostClientID( int nRemovedClientID ) const
{
	for ( int i = 0; i < slots.size(); ++i )
	{
		const SMPSlot &slot = slots[i];
		if ( slot.bPresent && slot.nClientID >= 0 && slot.nClientID != nRemovedClientID )
			return slot.nClientID;
	}
	return -1;
}

void CMPManagerMode::PromoteGameControlHostAfterRemoval( int nRemovedClientID )
{
	if ( nRemovedClientID != nHostClientID )
		return;

	const int nOldHostClientID = nHostClientID;
	nHostClientID = GetReplacementHostClientID( nRemovedClientID );
	NGameX::MatchPacketTrace_Log(
		IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
		"STATE",
		"PromoteGameControlHostAfterRemoval",
		GetOwnClientID(),
		StrFmt( "old_host=%d new_host=%d removed=%d", nOldHostClientID, nHostClientID, nRemovedClientID ) );
}

bool CMPManagerMode::IsAuthoritativeDropPacket( const class CB2DropPlayerAtSegmentPacket *pPacket )
{
	if ( !pPacket )
		return false;

	if ( pPacket->nClientID == nHostClientID )
		return true;

	const int nSlotToDrop = pPacket->nSlotToDrop;
	if ( nSlotToDrop < 0 || nSlotToDrop >= slots.size() )
		return false;

	// If the current game-control host is the player being dropped, accept the
	// packet from the deterministic replacement host and migrate authority.
	if ( slots[nSlotToDrop].nClientID == nHostClientID )
	{
		const int nReplacementHostClientID = GetReplacementHostClientID( nHostClientID );
		if ( nReplacementHostClientID >= 0 && pPacket->nClientID == nReplacementHostClientID )
		{
			const int nOldHostClientID = nHostClientID;
			nHostClientID = nReplacementHostClientID;
			NGameX::MatchPacketTrace_Log(
				IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
				"STATE",
				"PromoteGameControlHostFromDropPacket",
				GetOwnClientID(),
				StrFmt( "old_host=%d new_host=%d slot=%d target_seg=%d",
					nOldHostClientID, nHostClientID, nSlotToDrop, pPacket->nSegment ) );
			return true;
		}
	}

	return false;
}

void CMPManagerMode::BroadcastSynchronizedPlayerDrop( int nSlot, int nSegment, const char *szReason )
{
	if ( !IsGameControlHost() )
		return;

	NGameX::MatchPacketTrace_Log(
		nSegment,
		"TX",
		"CB2DropPlayerAtSegmentPacket",
		GetOwnClientID(),
		StrFmt( "slot=%d target_seg=%d reason=%s", nSlot, nSegment, szReason ? szReason : "" ) );
	pClient->SendGamePacket( new CB2DropPlayerAtSegmentPacket( 0, nSlot, nSegment ), true );
}

void CMPManagerMode::ScheduleSynchronizedPlayerDrop( int nSlot, int nSegment )
{
	if ( nSlot < 0 || nSlot >= slots.size() )
		return;

	// Remember and apply the earliest authoritative drop segment for this slot.
	if ( scheduledDropSegmentBySlot.size() != slots.size() )
		scheduledDropSegmentBySlot.assign( slots.size(), -1 );
	const int nCurrent = scheduledDropSegmentBySlot[nSlot];
	if ( nCurrent >= 0 && nCurrent <= nSegment )
		return;

	const DWORD dwLaggersBefore = dwLaggers;
	const DWORD dwLaggersOldBefore = dwLaggersOld;
	scheduledDropSegmentBySlot[nSlot] = nSegment;

	// Stop local lag accounting for this slot immediately to avoid repeated kick scheduling.
	dwLaggers &= ~( 1UL << nSlot );
	dwLaggersOld &= ~( 1UL << nSlot );
	lags[nSlot].dwHatedBy = 0;
	NGameX::MatchPacketTrace_RecordDropScheduled( nSlot, nSegment );
	NGameX::MatchPacketTrace_Log(
		IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
		"DECISION",
		"ScheduleSynchronizedPlayerDrop",
		GetOwnClientID(),
		StrFmt( "slot=%d target_seg=%d prev_target_seg=%d pre_laggers=%08X post_laggers=%08X pre_laggers_old=%08X post_laggers_old=%08X",
			nSlot, nSegment, nCurrent, dwLaggersBefore, dwLaggers, dwLaggersOldBefore, dwLaggersOld ) );

	// Defer both lockstep and manager-side removal to the same segment boundary.
	if ( IsValid( pTransceiver ) )
		pTransceiver->SchedulePlayerRemoval( nSlot, nSegment );
}

void CMPManagerMode::ApplyScheduledSlotDrops()
{
	if ( !IsGameRunning() || !IsValid( pTransceiver ) )
		return;
	if ( scheduledDropSegmentBySlot.size() != slots.size() )
		return;

	const int nCommonSegment = pTransceiver->GetCurrentCommonSegment();
	for ( int i = 0; i < slots.size(); ++i )
	{
		const int nDropSegment = scheduledDropSegmentBySlot[i];
		if ( nDropSegment < 0 || nDropSegment > nCommonSegment )
			continue;

		// Match the transceiver drop moment so game-control checks see consistent membership.
		scheduledDropSegmentBySlot[i] = -1;
		if ( IsPlayerPresent( i ) )
		{
			const DWORD dwPrePresentMask = GetPresentMask();
			slots[i].bPresent = false;
			const DWORD dwPostPresentMask = GetPresentMask();
			NGameX::MatchPacketTrace_Log(
				nCommonSegment,
				"STATE",
				"ApplyScheduledSlotDrops",
				GetOwnClientID(),
				StrFmt( "slot=%d target_seg=%d applied_seg=%d pre_present=%08X post_present=%08X",
					i, nDropSegment, nCommonSegment, dwPrePresentMask, dwPostPresentMask ) );
			NGameX::MatchPacketTrace_RecordDropApplied( i, nCommonSegment );
		}
	}
}

void CMPManagerMode::RemoveClient( int nClientID, bool bKicked )
{
	NGameX::MatchPacketTrace_Log(
		IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
		"DECISION",
		"RemoveClient",
		nClientID,
		StrFmt( "kicked=%d running=%d in_room=%d room_host=%d control_host=%d host_id=%d",
			bKicked ? 1 : 0, IsGameRunning() ? 1 : 0, IsInGameRoom() ? 1 : 0,
			IsGameHost() ? 1 : 0, IsGameControlHost() ? 1 : 0, nHostClientID ) );

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
			continue;

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
				if ( scheduledDropSegmentBySlot.size() != slots.size() )
					scheduledDropSegmentBySlot.assign( slots.size(), -1 );

				// Schedule by lockstep membership, not slot UI state.
				// Slot presence can already be false here while transceiver still waits for this player.
				if ( scheduledDropSegmentBySlot[i] < 0 && ( pTransceiver->GetPlayerMask() & ( 1UL << i ) ) != 0 )
				{
					const int nDropSegment = pTransceiver->GetCurrentCommonSegment();
					if ( bOwnsDropAuthority )
					{
						if ( bRemovingGameControlHost )
							PromoteGameControlHostAfterRemoval( nClientID );

						ScheduleSynchronizedPlayerDrop( i, nDropSegment );

						if ( IsGameControlHost() )
							BroadcastSynchronizedPlayerDrop( i, nDropSegment, bRemovingGameControlHost ? "host_removed" : "remove_client" );
						else
						{
							NGameX::MatchPacketTrace_Log(
								nDropSegment,
								"DECISION",
								"RemoveClientLocalDropFallback",
								GetOwnClientID(),
								StrFmt( "slot=%d target_seg=%d", i, nDropSegment ) );
						}
					}
					else
					{
						NGameX::MatchPacketTrace_Log(
							nDropSegment,
							"DECISION",
							"RemoveClientAwaitingDropAuthority",
							GetOwnClientID(),
							StrFmt( "slot=%d authority=%d", i, bRemovingGameControlHost ? nReplacementHostClientID : nHostClientID ) );
					}
				}
				else if ( bRemovingGameControlHost && GetOwnClientID() == nReplacementHostClientID )
					PromoteGameControlHostAfterRemoval( nClientID );
			}
			return;
		}

		slots[i].bPresent = false;
		if ( IsInGameRoom() )
		{
			SMPSlot emptySlot;
			if ( bKicked )
				emptySlot.bAccept = slots[i].bAccept;
			slots[i] = emptySlot;
			slots[i].nClientID = -1;
			--nSlotsUsed;

			SMPUIUpdateSlotMessage *pUpdateMsg = new SMPUIUpdateSlotMessage;
			pUpdateMsg->nSlot = i;
			pUpdateMsg->info = emptySlot;
			PushMessage( pUpdateMsg );
			if ( pLanTester )
				pLanTester->ClientRemoved( nClientID );

			OnGameRoomClientRemoved();
			UpdateMyConnectivityMask();
		}
		return;
	}
}

void CMPManagerMode::UpdateMyConnectivityMask()
{
	WORD &wMask = slots[nOwnSlot].wConnectedTo;
	WORD wNewMask = 0;
	for ( int i = 0; i < slots.size(); ++i )
	{
		if ( slots[i].bPresent )
			wNewMask |= ( 1 << i );
	}
	if ( wNewMask != wMask )
	{
		DebugTrace( "+++ Connectivity Mask change %d -> %d ", wMask, wNewMask );
		wMask = wNewMask;
		pClient->SendGamePacket( new CB2SlotInfoPacket( nHostClientID, nOwnSlot, slots[nOwnSlot] ), IsGameHost() );
	}
	CheckJoinGameConditions();
}


