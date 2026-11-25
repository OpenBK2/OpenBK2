#include "stdafx.h"

#include "Client/ServerClientInterface.h"
#include "CommandsHistory.h"
#include "DBMPConsts.h"
#include "GameXClassIDs.h"
#include "GetConsts.h"
#include "InterfaceMisc.h"
#include "Main/MainLoopCommands.h"
#include "Misc/StrProc.h"
#include "MPCoopRules.h"
#include "MPManagerMode.h"
#include "MPTransceiver.h"
#include "MultiplayerTestNet.h"
#include "Server_Client_Common/ChatPackets.h"
#include "Server_Client_Common/CommonPackets.h"
#include "Server_Client_Common/GamePackets.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "System/Commands.h"

#include "GameX_export.h"

static std::wstring s_wszMultiplayerName;

REGISTER_SAVELOAD_CLASS( GAMEX, 0x19245AC0, CSlotNumberPacket );
REGISTER_SAVELOAD_CLASS( GAMEX, 0x19260C00, CB2GameLostPacket );
START_REGISTER( MultiplayerVars )
// This GVar is here to force it to be saved in user.cfg
// Do not remove, please
REGISTER_VAR_EX( "Multiplayer.PlayerName", NGlobal::VarWStrHandler, &s_wszMultiplayerName, L"Player", STORAGE_NONE );
FINISH_REGISTER

#define PING_PERIOD 5000

// CMPManagerMode - general methods

CMPManagerMode::CMPManagerMode()
	: nGameID( -1 )
	, nHostClientID( -1 )
	, dwUserPausedPlayers( 0 )
{
	// Messages
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_CREATE_GAME, SMPUICreateGameMessage, &CMPManagerMode::OnCreateGameMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_JOIN_GAME, SMPUIJoinGameMessage, &CMPManagerMode::OnJoinGameMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_START_GAME, SMPUIMessage, &CMPManagerMode::OnStartGameMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_LEAVE_GAME, SMPUIMessage, &CMPManagerMode::OnLeaveGameMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_INTERRUPT, SMPUIMessage, &CMPManagerMode::OnInterruptMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_UPDATE_SLOT, SMPUIUpdateSlotMessage, &CMPManagerMode::OnUpdateSlotMessage );
	REGISTER_MPUI_MESSAGE_HANDLER(
			EMUI_MANUAL_PLACEMENT, SMPUIManualPlacementMessage, &CMPManagerMode::OnManualPlacementMessage );
	REGISTER_MPUI_MESSAGE_HANDLER(
			EMUI_UPDATE_GAME_LIST, SMPUIGameListMessage, &CMPManagerMode::OnUpdateGameListMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_WAITING_FOR_PLAYERS, SMPUILagMessage, &CMPManagerMode::OnLagMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_MP_PAUSE, SMPUIMessage, &CMPManagerMode::OnPauseMessage );
	REGISTER_MPUI_MESSAGE_HANDLER(
			EMUI_IN_GAME_CHAT_MESSAGE, SMPUIInGameChatMessage, &CMPManagerMode::OnInGameChatMessage );
	// Packets
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnGameClientRemoved );
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnB2SlotInfoPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnB2ManualPlacementPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnGameKilled );
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnB2GameRoomStartGamePacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnB2GameSpecificInfoPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnChatPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnSlotNumberPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnB2SuggestKickPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnB2LagTimeUpdatePacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnB2UserPausePacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnB2DropPlayerAtSegmentPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnB2GameLostPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnPingPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnDepartureRecoveryPacket );

	pClient = new CEmptyServerClient;
	szMPName = NStr::ToMBCS( NGlobal::GetVar( "profile_name", "Player" ) );
	NGlobal::SetVar( "Multiplayer.PlayerName", szMPName );
	pingUpdate.timeUpdatePeriod = PING_PERIOD;
}

bool CMPManagerMode::SAutoUpdate::CheckNeedUpdate()
{
	if ( !bUpdating )
	{
		return false;
	}

	NTimer::STime currTime = Singleton<IGameTimer>()->GetAbsTime();
	if ( currTime < timeNextUpdate )
	{
		return false;
	}
	timeNextUpdate = currTime + timeUpdatePeriod;
	return true;
}

CMPManagerMode::~CMPManagerMode() {}

void CMPManagerMode::BroadcastGameControlPacket( CNetPacket *pPacket )
{
	// LAN uses its peer channel; online play overrides this for announcements
	// that must survive the immediate teardown of the match's peer connections.
	pClient->SendGamePacket( pPacket, true );
}

bool CMPManagerMode::IsPacketFromDisqualifiedPlayer( const CNetPacket *pPacket ) const
{
	const uint32_t dwBlocked = dwDisqualifiedPlayers | pTransceiver->GetReinforcementOutliers();
	if ( dwBlocked == 0 || dynamic_cast<const CGameClientRemoved *>( pPacket ) ||
	     dynamic_cast<const CNetRemoveClient *>( pPacket ) )
	{
		// These transport notices name the disconnected client in nClientID.
		// Applying a real disconnect remains idempotent after its scheduled drop.
		return false;
	}
	// The lobby stamps forwarded chat with the author's connection ID. Older
	// servers supply its account name. A peer cannot assert either identity.
	const auto *pServerChat =
			GetMode() == ENM_NIVAL_NET && pPacket->nClientID == 0 ? dynamic_cast<const CChatPacket *>( pPacket ) : nullptr;
	const int nSender = pServerChat ? pServerChat->nID : pPacket->nClientID;
	for ( int i = 0; i < slots.size() && i < 32; ++i )
	{
		if ( ( dwBlocked & ( uint32_t( 1 ) << i ) ) != 0 &&
		     ( nSender == slots[i].nClientID ||
		       ( pServerChat && nSender == 0 && pServerChat->szNick == slots[i].szName ) ) )
		{
			return true;
		}
	}
	return false;
}

bool CMPManagerMode::ProcessPacket( CNetPacket *pPacket )
{
	if ( IsGameRunning() )
	{
		NI_VERIFY( pTransceiver, "PRG: Game in progress, but Transceiver not present", return true );
		// The verdict revokes input, control, chat and former-host authority at
		// once. Slot visibility can legitimately lag until the simulation boundary.
		if ( IsPacketFromDisqualifiedPlayer( pPacket ) || pTransceiver->ProcessPacket( pPacket ) )
		{
			return true;
		}
	}
	else if ( CMPTransceiver::IsGamePacket( pPacket ) )
	{
		return true;
	}
	return CPacketProcessorBase::ProcessPacket( pPacket );
}

bool CMPManagerMode::Segment()
{
	if ( !pMPConsts )
	{
		pMPConsts = NGameX::GetMPConsts();
	}

	pClient->Segment();

	// Update games list if necessary
	if ( gamesUpdate.CheckNeedUpdate() )
	{
		UpdateGameList();
	}

	if ( IsInGameRoom() && pingUpdate.CheckNeedUpdate() )
	{
		pClient->SendGamePacket( new CPingPacket( 0, nOwnSlot, GameTimer()->GetAbsTime() ), true );
	}

	// The checkpoint already committed locally before this update. Revoke its
	// reporting peer before queued chat, outcomes or old-host corrections can run.
	if ( IsGameRunning() && IsValid( pTransceiver ) && pTransceiver->GetReinforcementMismatchSegment() >= 0 &&
	     !IsDepartureRecoveryActive() && HandleReinforcementMismatch() )
	{
		return true;
	}
	// Process queued pause, removal and outcome packets before making timeout or
	// end-game decisions; those packets may invalidate the previous frame's state.
	while ( CPtr<CNetPacket> pPacket = pClient->GetPacket() )
	{
		ProcessPacket( pPacket );
	}

	if ( IsGameRunning() )
	{
		UpdateDepartureRecovery();
		if ( IsDepartureRecoveryActive() )
		{
			// Network packets and server heartbeats keep flowing while simulation,
			// victory decisions and ordinary lag voting wait for the repair commit.
			ShowWaitWindow( true );
			return true;
		}
		// Consecutive hosts can leave before their announcements reach this peer.
		// Resolve deferred self-announcements in authority order before simulation.
		ResolvePendingHostDrops();
		if ( pTransceiver->HasPlayerDropError() )
		{
			OnCriticalNetworkError( L"A player departure could not be synchronized.<br>"
			                        L"The match was stopped to prevent different simulation results." );
			return true;
		}
		// Keep manager-side slot visibility in sync with transceiver drop segment processing.
		ApplyScheduledSlotDrops();
		AnalyzeLaggers();
		// A start-game packet can queue mission loading during this same call.
		// Do not inspect simulation victory conditions until lockstep has started.
		if ( pTransceiver->IsGameRunning() )
		{
			CheckEndGameConditions();
		}
	}

	return true;
}

const bool CMPManagerMode::SaveReplay( const std::string &szFileName )
{
	if ( IsValid( pCommandsHistory ) )
	{
		for ( int i = 0; i < slots.size(); ++i )
		{
			SMPSlot &slot = slots[i];
			if ( !slot.bPresent && slot.bAccept && ( dwInitialPlayers & ( 1UL << i ) ) )
			{
				slot.bPresent = true;
			}
		}
		return pCommandsHistory->SaveReplay( szFileName, gameDesc, slots, nWinningSide );
	}
	else
	{
		return false;
	}
}

/*
void CMPManagerMode::UpdateInfoForReplay( SMultiplayerReplayInfo *pReplayInfo )
{
	if ( !gameDesc.pMPMap )
		return;
	if ( !pReplayInfo )
		return;

	pReplayInfo->bUnitExperience = gameDesc.bUnitExp;
	pReplayInfo->nCaptureTime = gameDesc.nCaptureTime;;
	pReplayInfo->nTechLevel = gameDesc.nTechLevel;
	pReplayInfo->bUnitExperience = gameDesc.bUnitExp;
	pReplayInfo->nWinningSide = nWinningSide;
	pReplayInfo->slots.resize( slots.size() );
	for ( int i = 0; i < slots.size(); ++i )
		pReplayInfo->slots[i] = slots[i];
}
*/

void CMPManagerMode::SendGameRoomInitMessages()
{
	// Send CREATE OK and slot info
	SMPUIGameRoomInitMessage *pCreateMsg = new SMPUIGameRoomInitMessage( SMPUIGameRoomInitMessage::ERR_SUCCESS );
	pCreateMsg->bHost = IsGameHost();
	pCreateMsg->szSessionName = szSessionName;
	pCreateMsg->specificInfo = gameDesc;
	pCreateMsg->nOwnSlot = nOwnSlot;
	PushMessage( pCreateMsg );

	for ( int i = 0; i < slots.size(); ++i )
	{
		SMPUIUpdateSlotMessage *pSlotMsg = new SMPUIUpdateSlotMessage();
		pSlotMsg->nSlot = i;
		pSlotMsg->info = slots[i];

		PushMessage( pSlotMsg );
	}
	bGameRoomInit = true;
	PushMessage( new SMPUIManualPlacementMessage( manualPlacement ) );
	pingUpdate.bUpdating = true;
	pingUpdate.timeNextUpdate = 0;

	pClient->SendGamePacket( new CB2SlotInfoPacket( 0, nOwnSlot, slots[nOwnSlot] ), true );
}

bool CMPManagerMode::SetManualPlacement( const std::vector<uint8_t> &order )
{
	if ( manualPlacement == order || ( !order.empty() && !NGameX::IsValidManualPlacement( gameDesc, order ) ) )
	{
		return false;
	}
	manualPlacement = order;
	if ( bGameRoomInit )
	{
		PushMessage( new SMPUIManualPlacementMessage( manualPlacement ) );
	}
	return true;
}

EPeerConnectionRoute CMPManagerMode::GetPeerRoute( int nSlot ) const
{
	return pClient && nSlot >= 0 && nSlot < slots.size() && slots[nSlot].bPresent
	           ? pClient->GetPeerRoute( slots[nSlot].nClientID )
	           : EPeerConnectionRoute::Unknown;
}

void CMPManagerMode::ShowWaitWindow( bool bShow )
{
	if ( bShow == bWaitWindowShown )
	{
		return;
	}
	if ( bShow )
	{
		NMainLoop::Command( ML_COMMAND_MP_WAIT_PLAYERS, "" );
	}
	else
	{
		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	}

	lagsUpdate.bUpdating = bShow;
	if ( bShow )
	{
		lagsUpdate.timeNextUpdate = 0;
	}
	bWaitWindowShown = bShow;
}

void CMPManagerMode::OnCriticalNetworkError( const std::wstring &wszMessage )
{
	// The commands below only close the mission on a later frame, and until then
	// it would keep stepping a match whose client is replaced at the end of this
	// function. End the match now instead.
	AbortRunningGame();

	NMainLoop::Command( ML_COMMAND_CLEAR_INTERFACES, "" );
	NMainLoop::Command( ML_COMMAND_MISSION_BACKGROUND, "" );
	NMainLoop::Command( ML_COMMAND_MULTIPLAYER_MENU, "" );
	NMainLoop::Command( ML_COMMAND_MESSAGE_BOX,
	                    CICMessageBox::MakeConfigString( "MessageBoxWindowOk", wszMessage ).c_str() );
	pClient = 0;
	pClient = new CEmptyServerClient;
}

void CMPManagerMode::SendStartGamePacket()
{
	if ( !IsGameHost() )
	{
		return;
	}

	CPtr<CB2GameRoomStartGamePacket> pStartPkt = new CB2GameRoomStartGamePacket( 0 );
	// Only the host resolves this profile setting, once, for every player in the match.
	bBaseReinforcementBonuses = NGlobal::GetVar( "multiplayer_flag_calls_bonus", 0 ) == 1;
	pStartPkt->bBaseReinforcementBonuses = bBaseReinforcementBonuses;
	pStartPkt->bManualPlacement = !manualPlacement.empty();
	CreateRehash( &( pStartPkt->slotRehash ) );
	RehashSlots( pStartPkt->slotRehash );
	for ( const SMPSlot &slot : slots )
	{
		// Copy construction includes client IDs; SMPSlot::operator= deliberately omits them.
		pStartPkt->slots.push_back( slot );
	}
	pClient->SendGamePacket( pStartPkt, true );
}

void CMPManagerMode::AddGameInfoForUI( std::list<SUIGameInfo> *pList, const SNetGameInfo &src )
{
	SUIGameInfo newGame;
	newGame.nGameID = src.nGameID;
	newGame.szSessionName = src.szSessionName;
	newGame.szMapName = src.szMapName;
	newGame.nPlayers = src.nPlayers;
	newGame.nPlayersMax = src.nMaxPlayers;
	newGame.bPwdReq = src.bPasswordRequired;
	newGame.nSizeX = src.nSizeX;
	newGame.nSizeY = src.nSizeY;
	newGame.nGameType = src.nGameType;
	newGame.nTechLevel = src.nTechLevel;
	pList->push_back( newGame );
}

uint32_t CMPManagerMode::GetCheckSum( const NDb::SMultiplayerMap *pMPMap )
{
	uint32_t ulCheckSum = 0;
	if ( pMPMap )
	{
		ulCheckSum = pMPMap->CalcCheckSum();
	}

	return ulCheckSum;
}
