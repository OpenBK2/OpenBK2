#include "stdafx.h"

#include "MPManagerMode.h"
#include "Server_Client_Common/ChatPackets.h"
#include "Client/ServerClientInterface.h"
#include "MultiplayerNetPackets.h"
#include "MPTransceiver.h"
#include "Main/MainLoop.h"
#include "Main/MainLoopCommands.h"
#include "GameXClassIDs.h"
#include "DBMPConsts.h"
#include "GetConsts.h"
#include "InterfaceState.h"
#include "Misc/StrProc.h"

// CMPManagerMode - UI->MP message handlers

bool CMPManagerMode::OnCreateGameMessage( SMPUICreateGameMessage *pMsg )
{
	// Store info
	gameDesc = pMsg->specificInfo;
	szSessionName = pMsg->info.szSessionName;
	if ( pMsg->info.bPwdReq )
		szPassword = pMsg->szPassword;
	else
		szPassword = "";
	gameDesc.eType = SB2GameSpecificData::EGT_FLAG_CONTROL_2;

	gameDesc.nPlayers = pMsg->info.nPlayersMax;

	// Arrange slots
	slots.clear();
	slots.resize( gameDesc.nPlayers );
	slots[0].szName = szMPName;			// Add self
	slots[0].bPresent = true;
	slots[0].nClientID = GetOwnClientID();
	nOwnSlot = 0;
	nSlotsUsed = 1;

	bGameRoomInit = false;

	ulGameCheckSum = GetCheckSum( gameDesc.pMPMap );
	ulHostCheckSum = ulGameCheckSum;

	TryToCreateGame();
	return true;
}

bool CMPManagerMode::OnJoinGameMessage( SMPUIJoinGameMessage *pMsg )
{
	bool bFound = false;
	for ( CNetGameList::iterator it = games.begin(); it != games.end(); ++it )
	{
		SNetGameInfo &game = *it;
		if ( game.nGameID == pMsg->nGameID )
		{
			if ( game.nMaxPlayers < 2 || game.nMaxPlayers - game.nPlayers < 1 )		// Do not join games with less than 2 players, or full games
				break;

			bFound = true;
			// Init all necessary data
			slots.clear();
			slots.resize( game.nMaxPlayers );
			nOwnSlot = -1;
			nSlotsUsed = 0;

			szSessionName = game.szSessionName;
			if ( game.bPasswordRequired )
				szPassword = pMsg->szPassword;
			else
				szPassword = "";

			ulGameCheckSum = 0;

			TryToJoinGame( game );

			//games.clear();
			gamesUpdate.bUpdating = false;
			break;
		}
	}

	if ( !bFound )
	{
		PushMessage( new SMPUIGameRoomInitMessage( SMPUIGameRoomInitMessage::ERR_CONNECT_FAILED ) );
		return true;
	}

	pendingClients.clear();
	bGameRoomInit = false;

	return true;
}

bool CMPManagerMode::OnStartGameMessage( SMPUIMessage *pMsg )
{
	// This happens only on the host side, so send StartGame packets to all
	NI_VERIFY( IsGameHost(), "PRG: Should be a host to start the game", return true );
	SendStartGamePacket();
	StartGame();
	return true;
}

bool CMPManagerMode::OnUpdateGameListMessage( SMPUIGameListMessage *pMsg )
{
	if ( gamesUpdate.bUpdating != pMsg->bSendUpdates )
	{
		// When turning on, do NOT do complete refresh, to minimize traffic
		//gamesUpdate.dwVersion = 0;
		//games.clear();
		gamesUpdate.timeNextUpdate = 0;

		if ( pMsg->bSendUpdates && games.size() > 0 )
		{
			SMPUIGameListMessage *pGamesMsg = new SMPUIGameListMessage;
			for ( CNetGameList::iterator it = games.begin(); it != games.end(); ++it )
			{
				SNetGameInfo &ownGame = *it;
				AddGameInfoForUI( &(pGamesMsg->gamesAddChange), ownGame );
			}
			PushMessage( pGamesMsg );
		}
	}
	gamesUpdate.bUpdating = pMsg->bSendUpdates;
	return true;
}

bool CMPManagerMode::OnUpdateSlotMessage( SMPUIUpdateSlotMessage *pMsg )
{
	NI_VERIFY( pMsg->nSlot >= 0 && pMsg->nSlot < slots.size(), "PRG: Player slot index out of bounds", return true );

	SMPSlot &oldSlot = slots[pMsg->nSlot];
	SMPSlot &newSlot = pMsg->info;

	if ( IsInGameRoom() )
	{	
		// Process slot open/close
		if ( IsGameHost() && newSlot.bPresent == false )
		{
			if ( oldSlot.bPresent )				// Kick player
			{
				KickPlayerFromSlot( pMsg->nSlot );
				RemoveClient( oldSlot.nClientID, true );
				--nSlotsUsed;
			}
		}
		WORD wOldConnectMask = oldSlot.wConnectedTo;
		oldSlot = newSlot;
		oldSlot.wConnectedTo = wOldConnectMask;
		pClient->SendGamePacket( new CB2SlotInfoPacket( 0, pMsg->nSlot, newSlot ), true );
	}
	else
	{
		NI_ASSERT( 0, "PRG: Wrong state to receive slot info message" );
	}

	return true;
}

bool CMPManagerMode::OnLagMessage( SMPUILagMessage *pMsg )
{
	if ( bInitialLoadInProgress && !pMsg->bInitialWait )
	{
		dwLaggersOld = 0;
		bInitialLoadInProgress = false;
	}
	else if ( dwLaggers == pMsg->dwLaggingPlayers )
		return true;

	dwLaggers = pMsg->dwLaggingPlayers;
	if ( pMsg->dwLaggingPlayers == 0 )								// Turning it off
		ShowWaitWindow( false );
	else if ( dwLaggersOld == 0 )											// Turning it on
		ShowWaitWindow( true );

	//DebugTrace( "*** LAG STATUS: %x ", pMsg->dwLaggingPlayers );

	return true;
}

bool CMPManagerMode::OnLeaveGameMessage( SMPUIMessage *pMsg )
{
	if ( IsGameRunning() )
	{
		OnSurrender();
		LoseGame();
	}
	return true;
}

bool CMPManagerMode::OnInterruptMessage( SMPUIMessage *pMsg )
{
	OnLeaveGame();
	return true;
}

bool CMPManagerMode::OnPauseMessage( SMPUIMessage *pMsg )
{
	if ( !IsGameRunning() )
		return true;

	bool bPausedNow = IsPlayerLagging( nOwnSlot );
	NTimer::STime curTime = GameTimer()->GetAbsTime();
	if ( !bPausedNow && lags[nOwnSlot].nLagLeft < NGameX::GetMPConsts()->nTimeUserMPLag * 1000 )
		return true;

	pTransceiver->CommandTimeOut( !bPausedNow );
	if ( bPausedNow )							// Pause off
	{
		dwLaggers &= ~( 1UL << nOwnSlot );
		int nTimeLeft = lags[nOwnSlot].nLagLeft - ( curTime - lags[nOwnSlot].timeStartLag );
		lags[nOwnSlot].nLagLeft = Max( nTimeLeft, 0 );
		lags[nOwnSlot].dwHatedBy = 0;
		lags[nOwnSlot].timeStartLag = 0;
	}
	else													// Pause on
	{
		dwLaggers |= ( 1UL << nOwnSlot );
		lags[nOwnSlot].timeStartLag = curTime;
	}

	ShowWaitWindow( !bPausedNow );
	return true;
}

bool CMPManagerMode::OnInGameChatMessage( SMPUIInGameChatMessage *pMsg )
{
	std::wstring wszFilteredText = InterfaceState()->FilterMPChatText( pMsg->wszText );
	for ( int i = 0; i < slots.size(); ++i )
	{
		SMPSlot &slot = slots[i];
		if ( !slot.bPresent )
			continue;
		if ( pMsg->bTeamOnly && slot.nTeam != slots[nOwnSlot].nTeam )
			continue;

		pClient->SendGamePacket( new CChatPacket( slot.nClientID, wszFilteredText, szMPName, slot.nClientID, !pMsg->bTeamOnly ), false );
	}
	InterfaceState()->AddMPChatMessage( NStr::ToUnicode( szMPName ) + L": " + wszFilteredText );
	return true;
}


