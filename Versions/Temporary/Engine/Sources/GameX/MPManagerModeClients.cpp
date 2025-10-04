#include "stdafx.h"

#include "MPManagerMode.h"
#include "Server_Client_Common/GamePackets.h"
#include "Client/ServerClientInterface.h"
#include "MultiplayerNetPackets.h"
#include "MPTransceiver.h"
#include "ScenarioTracker.h"

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

void CMPManagerMode::RemoveClient( int nClientID, bool bKicked )
{
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
		if ( IsGameRunning() )
		{
			pTransceiver->PlayerRemoved( i );
			Singleton<IScenarioTracker>()->RemovePlayer( i );
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


