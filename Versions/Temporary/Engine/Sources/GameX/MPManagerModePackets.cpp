#include "stdafx.h"

#include "MPManagerMode.h"
#include "MultiplayerNetPackets.h"
#include "Server_Client_Common/GamePackets.h"
#include "Server_Client_Common/ChatPackets.h"
#include "Server_Client_Common/CommonPackets.h"
#include "Client/ServerClientInterface.h"
#include "Misc/StrProc.h"
#include "Misc/StringConversions.h"
#include "MPTransceiver.h"
#include "ScenarioTracker.h"
#include "DBMPConsts.h"
#include "InterfaceState.h"

#include "MPLANTest.h"

// CMPManagerMode - packet handling methods

bool CMPManagerMode::OnB2SlotInfoPacket( class CB2SlotInfoPacket *pPacket )
{
	DebugTrace( "+++ Slot Info packet from client %d for slot %d, name %s, conn %d", 
		pPacket->nClientID, pPacket->nSlot, pPacket->info.szName.c_str(), pPacket->info.wConnectedTo );
	// Store for myself
	NI_VERIFY( pPacket->nSlot >= 0 && pPacket->nSlot < slots.size(), "PRG: Got slot info that is outside bounds", return true );
	//NI_VERIFY( IsInGameRoom(), "PRG: Received slot info while not in game room", return true );
	if ( pPacket->info.bPresent && !IsPlayerPresent( pPacket->nSlot ) )
	{
		if ( nOwnSlot >= 0 )
			pClient->SendGamePacket( new CPingPacket( pPacket->nClientID, nOwnSlot, GameTimer()->GetAbsTime() ), false );
		// Only clients increase slot counter here, server increases it in NEW_CLIENT handler
		if ( !IsGameHost() )
			++nSlotsUsed;
	}
	else if ( !pPacket->info.bPresent && IsPlayerPresent( pPacket->nSlot ) )
	{
		--nSlotsUsed;
	}

	SMPSlot &slot = slots[pPacket->nSlot];
	int nStoredPing = slot.nPing;
	slot = pPacket->info;
	slot.nClientID = pPacket->nClientID;
	slot.nPing = nStoredPing;

	if ( nOwnSlot >= 0 )
		UpdateMyConnectivityMask();

	// Send message
	if ( bGameRoomInit && IsInGameRoom() )
	{
		SMPUIUpdateSlotMessage *pMsg = new SMPUIUpdateSlotMessage;
		pMsg->info = slot;
		pMsg->nSlot = pPacket->nSlot;
		PushMessage( pMsg );
		DebugTrace( "+++ Show Slot Info " );
	}
	else
		DebugTrace( "+++ DO NOT Show Slot Info " );
	if ( pLanTester )
		pLanTester->ClientInfoChanged( pPacket->nSlot, pPacket->info.bAccept );

	return true;
}

bool CMPManagerMode::OnGameClientRemoved( class CGameClientRemoved *pPacket )
{
	DebugTrace( "+++ Client Removed packet from client %d", pPacket->nClientID );
	RemoveClient( pPacket->nClientID, false );
	return true;
}

bool CMPManagerMode::OnGameKilled( class CGameKilled *pPacket )
{
	DebugTrace( "+++ GameKilled packet from client %d, game %d", pPacket->nClientID, pPacket->nGame );
	if ( IsInGameRoom() )
	{
		PushMessage( new SMPUIGameRoomInitMessage( SMPUIGameRoomInitMessage::ERR_GAME_KILLED ) );
		OnLeaveGame();
	}
	else if ( IsGameRunning() )
	{
		int nLostTeam = GetTeamWithLowestScore();
		int nOwnTeam = slots[nOwnSlot].nTeam;

		if ( nOwnTeam == nLostTeam )
		{
//			WriteToPipe( PIPE_CHAT, StrFmt( "Game time over, lost by score" ) );
			LoseGame();
			return true;
		}
		else
		{
//			WriteToPipe( PIPE_CHAT, StrFmt( "Game time over, won by score" ) );
			WinGame();
			return true;
		}
	}

	return true;
}

bool CMPManagerMode::OnB2GameRoomStartGamePacket( class CB2GameRoomStartGamePacket *pPacket )
{
	NI_VERIFY( IsInGameRoom() && !IsGameHost(), "PRG: StartGame received by not a client", return true );

	//DebugTrace( "+++ StartGame packet from client %d", pPacket->nClientID );
	if ( pPacket->slots.size() > slots.size() )
		slots.resize( nSlotsUsed );
	RehashSlots( pPacket->slotRehash );
	for ( int i = 0; i < slots.size(); ++i )
	{
		slots[i].nCountry = pPacket->slots[i].nCountry;
		slots[i].nTeam = pPacket->slots[i].nTeam;
	}
	StartGame();

	return true;
}

bool CMPManagerMode::OnB2GameSpecificInfoPacket( class CB2GameSpecificInfoPacket *pPacket )
{
	//DebugTrace( "+++ SpecificInfo packet from client %d, players %d", pPacket->nClientID, pPacket->info.nPlayers );

	if ( !IsInGameRoom() || IsGameHost() )
		return true;

	gameDesc = pPacket->info;
	OnGameSpecificInfo();

	return true;
}

bool CMPManagerMode::OnChatPacket( class CChatPacket *pPacket )
{
	if ( IsGameRunning() )
	{
		auto scenario = Singleton<IScenarioTracker>();

		int playerID = -1;
		const auto& players = scenario->GetMultiplayerInfo()->players;
		auto nick = NStr::ToUnicode(pPacket->szNick);
		for(int i = 0; i < players.size(); i++)
		{
			if (players[i].wszName == nick)
			{
				playerID = i;
				break;
			}
		}

		std::wstring colorStr = L"", colorEndStr = L"";
		if (playerID >= 0)
		{
			DWORD color = scenario->GetPlayerColor(playerID).dwColor;
			colorStr = L"<color = " + string_conversion::utf8_to_wstring(string_conversion::RGBA_to_hex(color)) + L">";
			colorEndStr = L"<color = FFFFFFFF>";
		}

		std::wstring txt = L"<font size = 16pt outlinesize = 1 outlinecolor = black forcefontsize = 1>" + colorStr + nick + colorEndStr + L": " + pPacket->wszMessage;

		InterfaceState()->AddMPChatMessage( txt );
		return true;
	}
	CPtr<SMPUIChatMessage> pMsg = new SMPUIChatMessage( pPacket->szNick, pPacket->wszMessage );
	pMsg->bPrivate = !pPacket->bIsBroadcast;
	PushMessage( pMsg );

	return true;
}

bool CMPManagerMode::OnSlotNumberPacket( class CSlotNumberPacket *pPacket )
{
	DebugTrace( "+++ SlotNumber packet from client %d, assigned slot %d", pPacket->nClientID, pPacket->nSlot );
	nHostClientID = pPacket->nClientID;
	nOwnSlot = pPacket->nSlot;
	++nSlotsUsed;
	slots[nOwnSlot].szName = szMPName;
	slots[nOwnSlot].nTeam = nOwnSlot % 2;
	slots[nOwnSlot].nColour = nOwnSlot % ( (pMPConsts) ? pMPConsts->playerColorInfos.size() : 4 );
	slots[nOwnSlot].bPresent = true;
	ulHostCheckSum = pPacket->ulCheckSum;

	for ( std::list<int>::iterator it = pendingClients.begin(); it != pendingClients.end(); ++it )
		ClientAssignNewClient( *it );
	pendingClients.clear();
	OnSetMySlotNumber();
	return true;
}

bool CMPManagerMode::OnConnectGameFailedPacket( class CConnectGameFailed *pPacket )
{
	PushMessage( new SMPUIGameRoomInitMessage( SMPUIGameRoomInitMessage::ERR_GAME_FULL ) );
	OnLeaveGame();

	return true;
}

bool CMPManagerMode::OnB2SuggestKickPacket( class CB2SuggestKickPacket *pPacket )
{
	for ( int i = 0; i < slots.size(); ++i )
	{
		if ( i == nOwnSlot || !IsPlayerPresent( i ) )
			continue;

		lags[pPacket->nSlotToKick].dwHatedBy &= ( 1UL << i );
		break;
	}

	return true;
}

bool CMPManagerMode::OnB2LagTimeUpdatePacket( class CB2LagTimeUpdatePacket *pPacket )
{
	if ( !IsGameRunning() || !IsPlayerPresent( pPacket->nPlayer ) )
		return true;

	SLagInfo &lag = lags[pPacket->nPlayer];
	//DebugTrace( "*** LAG UPDATE player %d, time %d --> %d", pPacket->nPlayer, lag.nLagLeft, pPacket->nTimeLeft );
	lag.nLagLeft = (std::min)( pPacket->nTimeLeft, int( lag.nLagLeft ) );

	return true;
}

bool CMPManagerMode::OnB2GameLostPacket( class CB2GameLostPacket *pPacket )
{
	if ( !IsGameRunning() || bOutcomeKnown || pPacket->nGameID != nGameID )
		return true;

	//DebugTrace( "+++ GameLost packet" );
	for ( int i = 0; i < slots.size(); ++i )
	{
		if ( i == nOwnSlot )
			continue;

		if ( slots[i].nClientID == pPacket->nClientID )
		{
			pTransceiver->ScheduleGameEnd( pPacket->nSegment );
			//DebugTrace( "+++ Game Lost for team %d (from slot %d) scheduled", slots[i].nTeam, i );
			bOutcomeKnown = true;
			bWinOnGameEnd = ( slots[nOwnSlot].nTeam != slots[i].nTeam );
			break;
		}
	}

	return true;
}

bool CMPManagerMode::OnPingPacket( class CPingPacket *pPacket )
{
	if ( pPacket->nFromID != nOwnSlot )		// Pong
	{
		pClient->SendGamePacket( new CPingPacket( pPacket->nClientID, pPacket->nFromID, pPacket->nSendTime ), false );
		return true;
	}

	if ( !IsInGameRoom() )
		return true;

	NTimer::STime curTime = GameTimer()->GetAbsTime();
	for ( int i = 0; i < slots.size(); ++i )
	{
		if ( i == nOwnSlot )
			continue;
		SMPSlot &slot = slots[i];
		if ( slot.nClientID == pPacket->nClientID )
		{
			const int nTimeDiff = curTime - pPacket->nSendTime;
			slot.nPing = (std::max)( 0, nTimeDiff );
			if ( bGameRoomInit )
			{
				SMPUIUpdateSlotMessage *pMsg = new SMPUIUpdateSlotMessage;
				pMsg->info = slot;
				pMsg->nSlot = i;
				PushMessage( pMsg );
			}
			break;
		}
	}

	return true;
}


