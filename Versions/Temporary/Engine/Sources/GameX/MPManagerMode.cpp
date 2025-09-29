#include "StdAfx.h"

#include "MPManagerMode.h"
#include "MPTransceiver.h"
#include "../Misc/StrProc.h"
#include "CommandsHistory.h"
#include "../Stats_B2_M1/DBMapInfo.h"
#include "../System/Commands.h"
#include "../Main/MainLoopCommands.h"
#include "MultiplayerTestNet.h"
#include "GameXClassIDs.h"
#include "InterfaceMisc.h"
#include "DBMPConsts.h"
#include "GetConsts.h"
#include "../Server_Client_Common/GamePackets.h"

static wstring s_wszMultiplayerName;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
REGISTER_SAVELOAD_CLASS( 0x19245AC0, CSlotNumberPacket );
REGISTER_SAVELOAD_CLASS( 0x19260C00, CB2GameLostPacket );
START_REGISTER(MultiplayerVars)
// This GVar is here to force it to be saved in user.cfg
// Do not remove, please
REGISTER_VAR_EX( "Multiplayer.PlayerName", NGlobal::VarWStrHandler, &s_wszMultiplayerName, L"Player", STORAGE_NONE );
FINISH_REGISTER
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define PING_PERIOD 5000
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CMPManagerMode - general methods
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CMPManagerMode::CMPManagerMode()
: nGameID( -1 )
{
	// Messages
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_CREATE_GAME, SMPUICreateGameMessage, &CMPManagerMode::OnCreateGameMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_JOIN_GAME, SMPUIJoinGameMessage, &CMPManagerMode::OnJoinGameMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_START_GAME, SMPUIMessage, &CMPManagerMode::OnStartGameMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_LEAVE_GAME, SMPUIMessage, &CMPManagerMode::OnLeaveGameMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_INTERRUPT, SMPUIMessage, &CMPManagerMode::OnInterruptMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_UPDATE_SLOT, SMPUIUpdateSlotMessage, &CMPManagerMode::OnUpdateSlotMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_UPDATE_GAME_LIST, SMPUIGameListMessage, &CMPManagerMode::OnUpdateGameListMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_WAITING_FOR_PLAYERS, SMPUILagMessage, &CMPManagerMode::OnLagMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_MP_PAUSE, SMPUIMessage, &CMPManagerMode::OnPauseMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_IN_GAME_CHAT_MESSAGE, SMPUIInGameChatMessage, &CMPManagerMode::OnInGameChatMessage );
	// Packets
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnGameClientRemoved );
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnB2SlotInfoPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnGameKilled );
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnB2GameRoomStartGamePacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnB2GameSpecificInfoPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnChatPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnSlotNumberPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnB2SuggestKickPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnB2LagTimeUpdatePacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnB2GameLostPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerMode::OnPingPacket );

	pClient = new CEmptyServerClient;
	szMPName = NStr::ToMBCS( NGlobal::GetVar( "profile_name", "Player" ) );
	NGlobal::SetVar( "Multiplayer.PlayerName", szMPName );
	pingUpdate.timeUpdatePeriod = PING_PERIOD;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CMPManagerMode::SAutoUpdate::CheckNeedUpdate()
{
	if ( !bUpdating )
		return false;

	NTimer::STime currTime = Singleton<IGameTimer>()->GetAbsTime();
	if ( currTime < timeNextUpdate )
		return false;
	timeNextUpdate = currTime + timeUpdatePeriod;
	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CMPManagerMode::~CMPManagerMode() 
{
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CMPManagerMode::Segment()
{
	if ( !pMPConsts )
		pMPConsts = NGameX::GetMPConsts();

	pClient->Segment();

	// Update games list if necessary
	if ( gamesUpdate.CheckNeedUpdate() )
		UpdateGameList();

	if ( IsInGameRoom() && pingUpdate.CheckNeedUpdate() )
	{
		pClient->SendGamePacket( new CPingPacket( 0, nOwnSlot, GameTimer()->GetAbsTime() ), true );
	}

	if ( IsGameRunning() )
	{
		AnalyzeLaggers();
		CheckEndGameConditions();
	}

	// Process packets
	while ( CPtr<CNetPacket> pPacket = pClient->GetPacket() )
	{
		if ( IsGameRunning() )
		{
			NI_VERIFY( pTransceiver, "PRG: Game in progress, but Transceiver not present", continue );
			bool bResult = pTransceiver->ProcessPacket( pPacket );
			//NI_VERIFY( bResult, "PRG: Game packet unhandled by transceiver", continue );
			if ( bResult )
				continue;
		}
		else if ( CMPTransceiver::IsGamePacket( pPacket ) )		// Game packet, but no game, ignore
			continue;

		bool bResult = ProcessPacket( pPacket );
		//NI_ASSERT( bResult, "PRG: Unhandled general packet" );
	}

	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const bool CMPManagerMode::SaveReplay( const string &szFileName )
{
	if ( IsValid( pCommandsHistory ) )
	{
		for ( int i = 0; i < slots.size(); ++i )
		{
			SMPSlot &slot = slots[i];
			if ( !slot.bPresent && slot.bAccept && ( dwInitialPlayers & ( 1UL << i ) ) )
				slot.bPresent = true;
		}
		return pCommandsHistory->SaveReplay( szFileName, gameDesc, slots, nWinningSide );
	}
	else
		return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
	pingUpdate.bUpdating = true;
	pingUpdate.timeNextUpdate = 0;

	pClient->SendGamePacket( new CB2SlotInfoPacket( 0, nOwnSlot, slots[nOwnSlot] ), true );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMPManagerMode::ShowWaitWindow( bool bShow )
{
	if ( bShow == bWaitWindowShown )
		return;
	if ( bShow )
		NMainLoop::Command( ML_COMMAND_MP_WAIT_PLAYERS, "" );
	else
		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );

	lagsUpdate.bUpdating = bShow;
	if ( bShow )
		lagsUpdate.timeNextUpdate = 0;
	bWaitWindowShown = bShow;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMPManagerMode::OnCriticalNetworkError( const wstring &wszMessage )
{
	NMainLoop::Command( ML_COMMAND_CLEAR_INTERFACES, "" );
	NMainLoop::Command( ML_COMMAND_MISSION_BACKGROUND, "" );
	NMainLoop::Command( ML_COMMAND_MULTIPLAYER_MENU, "" );
	NMainLoop::Command( ML_COMMAND_MESSAGE_BOX, CICMessageBox::MakeConfigString( "MessageBoxWindowOk", wszMessage ).c_str() );
	pClient = 0;
	pClient = new CEmptyServerClient;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMPManagerMode::SendStartGamePacket()
{
	if ( !IsGameHost() )
		return;

	CPtr<CB2GameRoomStartGamePacket> pStartPkt = new CB2GameRoomStartGamePacket( 0 );
	pStartPkt->slots.resize( slots.size() );
	CreateRehash( &( pStartPkt->slotRehash ) );
	RehashSlots( pStartPkt->slotRehash );
	for ( int i = 0; i < slots.size(); ++i )
	{
		pStartPkt->slots[i].nCountry = slots[i].nCountry;
		pStartPkt->slots[i].nTeam = slots[i].nTeam;
	}
	pClient->SendGamePacket( pStartPkt, true );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMPManagerMode::AddGameInfoForUI( list<SUIGameInfo> *pList, const SNetGameInfo &src )
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
	newGame.nGameType = 0;
	newGame.nTechLevel = src.nTechLevel;
	pList->push_back( newGame );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned long CMPManagerMode::GetCheckSum( const NDb::SMultiplayerMap *pMPMap )
{
	unsigned long ulCheckSum = 0;
	if ( pMPMap )
		ulCheckSum = pMPMap->CalcCheckSum();

	return ulCheckSum;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
