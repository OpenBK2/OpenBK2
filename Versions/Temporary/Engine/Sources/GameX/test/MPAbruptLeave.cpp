#include "../stdafx.h"

#include "../AICmdsAutoMagic.h"
#include "../DBMPConsts.h"
#include "../MPManagerModeNivalNet.hpp"
#include "../MPTransceiver.h"
#include "../ScenarioTracker.h"
#include "Client/ServerClientInterface.h"
#include "Main/AICmdsAutoMagicInterface.h"
#include "Main/CommandsHistory.hpp"
#include "Main/DBNetConsts.h"
#include "Main/GameTimer.h"
#include "MPHeadlessAI.h"
#include "Server_Client_Common/CommonPackets.h"
#include "Server_Client_Common/GamePackets.h"
#include "Server_Client_Common/NetSaver.h"
#include "System/ConsoleBuffer.h"

#include <algorithm>
#include <array>
#include <deque>
#include <functional>
#include <limits>

#include <gtest/gtest.h>

namespace
{
constexpr int PLAYER_COUNT = 4;
constexpr int CLIENT_IDS[] = { 271780, 271786, 271789, 271792 };
constexpr uint16_t ALL_PLAYERS = ( 1u << PLAYER_COUNT ) - 1;
std::vector<int> *pExecutedCommands = nullptr;

class CTailCommand : public IAILogicCommandB2
{
	OBJECT_NOCOPY_METHODS( CTailCommand );

public:
	int nValue = 0;

	int operator&( IBinSaver &saver ) override
	{
		saver.Add( 2, &nValue );
		return 0;
	}

	void Execute() override
	{
		if ( pExecutedCommands )
		{
			pExecutedCommands->push_back( nValue );
		}
	}

	bool NeedToBeStored() const override
	{
		return true;
	}

	std::string GetDebugInfo() const override
	{
		return "TailCommand " + std::to_string( nValue );
	}
};

// Only the command payload is a fixture type. Serialization, packet production,
// retransmitted receipt and command dispatch all use the production transceiver.
class CTailCommandPacker : public IAICmdsAutoMagic
{
	OBJECT_NOCOPY_METHODS( CTailCommandPacker );

public:
	int GetCommandID( CObjectBase *pObject ) override
	{
		return dynamic_cast<CTailCommand *>( pObject ) ? 1 : -1;
	}

	CObjectBase *MakeCommand( int nID ) override
	{
		return nID == 1 ? new CTailCommand : nullptr;
	}

	int GetIDSize() const override
	{
		return 1;
	}
};

class CRecordingHistory : public ICommandsHistory
{
	OBJECT_NOCOPY_METHODS( CRecordingHistory );
	CHistory recorded;
	std::vector<std::pair<int, std::string>> ordered;

public:
	void StartNewGame( const NDb::SMapInfo * ) override {}

	const NDb::SMapInfo *GetMap() const override
	{
		return nullptr;
	}

	void AddCommand( const int nSegment, IAILogicCommandB2 *pCommand ) override
	{
		recorded[nSegment].push_back( pCommand );
		ordered.emplace_back( nSegment, pCommand->GetDebugInfo() );
	}

	void ExecuteSegmentCommands( const int, ITransceiver * ) override {}

	bool AddChecksumLog( const int, const uint32_t, const int ) override
	{
		return true;
	}

	const uint32_t GetLastChecksum() const override
	{
		return 0;
	}

	const std::vector<std::pair<int, std::string>> &GetOrderedCommands() const
	{
		return ordered;
	}

	CHistory GetCommandsHistory() const override
	{
		return recorded;
	}
};

struct SPlayerPresence
{
	bool bPresent = true;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 4, &bPresent );
		return 0;
	}
};

struct SScenarioPlayers
{
	std::vector<SPlayerPresence> players = std::vector<SPlayerPresence>( PLAYER_COUNT );

	int operator&( IBinSaver &saver )
	{
		saver.Add( 3, &players );
		return 0;
	}
};

struct SAbsoluteTimerState
{
	NTimer::STime timeCurrent = 0;
	float fScale = 1;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 2, &timeCurrent );
		saver.Add( 3, &fScale );
		return 0;
	}
};

struct STimerState
{
	SAbsoluteTimerState absolute;
	int nSegmentDuration = 200;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 2, &absolute );
		saver.Add( 7, &nSegmentDuration );
		return 0;
	}
};

struct SSegmentPacket
{
	int nSegment = -1;
	uint32_t checksum = 0;
	std::list<CMPTransceiver::SRawCommand> commands;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 2, &nSegment );
		saver.Add( 3, &checksum );
		saver.Add( 4, &commands );
		return 0;
	}
};

template <class TState>
void RestoreState( CObjectBase *pObject, TState &state )
{
	CMemoryStream stream;
	{
		CObj<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->Add( 1, &state );
	}
	stream.Seek( 0 );
	{
		CObj<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
		saver->AddPolymorphicBase( 1, pObject );
	}
}

SSegmentPacket ReadSegment( CNetPacket *pPacket )
{
	CMemoryStream stream;
	{
		CObj<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->AddPolymorphicBase( 1, pPacket );
	}
	stream.Seek( 0 );
	SSegmentPacket result;
	{
		CObj<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
		saver->Add( 1, &result );
	}
	return result;
}

CPtr<CNetPacket> ClonePacket( CNetPacket *pPacket )
{
	CMemoryStream stream;
	{
		CPtr<CNetPacket> source = pPacket;
		CPtr<IBinSaver> saver = CreateNetSaver( &stream, SAVER_MODE_WRITE );
		saver->Add( 1, &source );
	}
	stream.Seek( 0 );
	CPtr<CNetPacket> result;
	{
		CPtr<IBinSaver> saver = CreateNetSaver( &stream, SAVER_MODE_READ );
		saver->Add( 1, &result );
	}
	return result;
}

class CRoutedClient : public IServerClient
{
	OBJECT_NOCOPY_METHODS( CRoutedClient );

public:
	std::function<void( CNetPacket *, bool )> send;
	std::deque<CPtr<CNetPacket>> received;
	CPtr<CNetPacket> current;

	CNetPacket *GetPacket() override
	{
		current = nullptr;
		if ( !received.empty() )
		{
			current = received.front();
			received.pop_front();
		}
		return current;
	}

	void SendPacket( CNetPacket *pPacket ) override
	{
		CPtr<CNetPacket> retained = pPacket;
		send( pPacket, true );
	}

	void SendGamePacket( CNetPacket *pPacket, bool ) override
	{
		CPtr<CNetPacket> retained = pPacket;
		send( pPacket, false );
	}

	void Segment() override {}
};

class CDispatchingUI : public IMPToUIManager
{
	OBJECT_NOCOPY_METHODS( CDispatchingUI );

public:
	CMPUIMessageTranslator *pManager = nullptr;

	void AddUIMessage( SMPUIMessage *pMessage ) override
	{
		CPtr<SMPUIMessage> retained = pMessage;
		if ( pManager && dynamic_cast<SMPUILagMessage *>( pMessage ) )
		{
			// The real MP manager dispatches transceiver lag reports synchronously.
			EXPECT_TRUE( pManager->HandleMessage( pMessage ) );
		}
	}

	void AddUIMessage( EMPUIMessageType ) override {}

	SMPUIMessage *GetUIMessage() override
	{
		return nullptr;
	}

	SMPUIMessage *PeekUIMessage() override
	{
		return nullptr;
	}

	void MPUISegment() override {}

	bool SaveReplay( const std::string & ) override
	{
		return false;
	}
};

class CPauseMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS( CPauseMessage );

public:
	CPauseMessage()
		: SMPUIMessage( EMUI_MP_PAUSE )
	{
	}
};
} // namespace

// Restore lobby state only. Every receipt bit, outgoing command packet and
// removal thereafter comes from the actual protocol and simulation loop.
class CMPPlayerDropTestAccess
{
public:
	static void Initialize( CMPManagerModeNivalNet *pManager,
	                        CMPTransceiver *pTransceiver,
	                        IServerClient *pClient,
	                        ICommandsHistory *pHistory,
	                        IAICmdsAutoMagic *pPacker,
	                        IAILogic *pAI,
	                        IGameTimer *pTimer,
	                        int nOwnSlot )
	{
		pManager->eState = CMPManagerModeNivalNet::EGS_GAME_STARTED;
		pManager->nMyClientID = CLIENT_IDS[nOwnSlot];
		pManager->bHost = nOwnSlot == 0;
		pManager->bLadderGame = false;
		pManager->pClient = pClient;
		pManager->pTransceiver = pTransceiver;
		pManager->nOwnSlot = nOwnSlot;
		pManager->nHostClientID = CLIENT_IDS[0];
		pManager->nGameID = 1219;
		pManager->nSlotsUsed = PLAYER_COUNT;
		pManager->dwNetworkLaggers = 0;
		pManager->dwLaggers = 0;
		pManager->dwLaggersOld = 0;
		pManager->dwUserPausedPlayers = 0;
		pManager->dwInitialPlayers = ALL_PLAYERS;
		pManager->bInitialLoadInProgress = false;
		pManager->bWaitWindowShown = false;
		pManager->bOutcomeKnown = true;
		pManager->bWinOnGameEnd = false;
		pManager->bWinFromPlayerDrops = false;
		pManager->timeNextGameHeartbeat = 0x7fffffff;
		pManager->gamesUpdate.bUpdating = false;
		pManager->updateChannels.bUpdating = false;
		pManager->lagsUpdate.bUpdating = false;
		pManager->pMPConsts = dynamic_cast<NDb::SMultiplayerConsts *>( NObjectFactory::MakeObject( 0x191B2300 ) );
		pManager->slots.resize( PLAYER_COUNT );
		pManager->lags.resize( PLAYER_COUNT );
		pManager->playerDrops.resize( PLAYER_COUNT );
		pTransceiver->players.resize( PLAYER_COUNT );
		pTransceiver->playerRemovals.resize( PLAYER_COUNT );
		for ( int i = 0; i < PLAYER_COUNT; ++i )
		{
			pManager->slots[i].nClientID = CLIENT_IDS[i];
			pManager->slots[i].nTeam = i % 2;
			pManager->slots[i].bPresent = true;
			pManager->slots[i].bAccept = true;
			pManager->lags[i].nLagLeft = 180000;
			pTransceiver->players[i].nClientID = CLIENT_IDS[i];
			pTransceiver->players[i].nTeam = pManager->slots[i].nTeam;
		}
		pTransceiver->pClient = pClient;
		pTransceiver->pCmdsHistory = pHistory;
		pTransceiver->pCmdsSerializer = pPacker;
		pTransceiver->pAI = pAI;
		pTransceiver->pTimer = pTimer;
		pTransceiver->nMyLogicID = nOwnSlot;
		pTransceiver->nLatency = 2;
		pTransceiver->nSegmentsPackSize = 1;
		pTransceiver->nCommonSegment = 0;
		pTransceiver->nSegment = 0;
		pTransceiver->nFinalSegment = -1;
		pTransceiver->bIsGameRunning = true;
		pTransceiver->bWaiting = false;
		pTransceiver->wMask = ALL_PLAYERS;
		pTransceiver->wWaitMask = ALL_PLAYERS;
		CPtr<NDb::SNetGameConsts> consts = dynamic_cast<NDb::SNetGameConsts *>( NObjectFactory::MakeObject( 0x300A7B40 ) );
		consts->nMaxLatency = 16;
		pTransceiver->pConsts = consts;
		pTransceiver->segmFinished.assign( 16, 0 );
		pTransceiver->checkSums.SetSizes( 16, PLAYER_COUNT );
		pTransceiver->checkSums.FillZero();
		pTransceiver->cmds.SetSizes( PLAYER_COUNT, 16 );
	}

	static int LastReceived( const CMPTransceiver *pTransceiver, int nPlayer )
	{
		return pTransceiver->players[nPlayer].nLastReceivedSegment;
	}

	static int Removal( const CMPTransceiver *pTransceiver, int nPlayer )
	{
		return pTransceiver->playerRemovals[nPlayer].GetRemovalSegment( pTransceiver->nLatency );
	}

	static int HostClient( const CMPManagerMode *pManager )
	{
		return pManager->nHostClientID;
	}

	static bool LocalPause( const CMPTransceiver *pTransceiver )
	{
		return pTransceiver->bWaiting;
	}

	static int Budget( const CMPManagerMode *pManager, int nPlayer )
	{
		return pManager->lags[nPlayer].nLagLeft;
	}

	static size_t QueuedCommands( const CMPTransceiver *pTransceiver, int nPlayer )
	{
		size_t nCount = 0;
		for ( int nSegment = 0; nSegment < pTransceiver->cmds.GetSizeY(); ++nSegment )
		{
			nCount += pTransceiver->cmds[nSegment][nPlayer].size();
		}
		return nCount;
	}
};

namespace
{
struct SPeer
{
	bool bOnline = true;
	NTimer::STime wallTime = 1000;
	int nLastQueuedCommand = -1;
	std::vector<int> executed;
	CObj<CRoutedClient> client;
	CObj<CRecordingHistory> history;
	CObj<CMPManagerModeNivalNet> manager;
	CObj<CMPTransceiver> transceiver;
	CObj<IScenarioTracker> scenario;
	CObj<CMPHeadlessAI> ai;
	CObj<IGameTimer> timer;
};

struct SDelivery
{
	int nSender = -1;
	int nRecipient = -1;
	bool bPersistent = false;
	bool bReleased = false;
	int nSegment = -1;
	CPtr<CNetPacket> packet;
};

class MPAbruptLeave : public testing::Test
{
protected:
	std::array<SPeer, PLAYER_COUNT> peers;
	CObj<CTailCommandPacker> packer;
	CObj<IConsoleBuffer> console;
	CObj<CDispatchingUI> ui;
	std::deque<SDelivery> pending;
	std::deque<SDelivery> held;
	std::vector<SDelivery> delivered;
	std::function<bool( const SDelivery & )> shouldHold;
	int nSelectedPeer = -1;

	void SelectPeer( int nPlayer )
	{
		if ( nSelectedPeer != nPlayer )
		{
			if ( nSelectedPeer >= 0 )
			{
				NSingleton::UnRegisterSingleton( IGameTimer::tidTypeID );
				NSingleton::UnRegisterSingleton( IAILogic::tidTypeID );
				NSingleton::UnRegisterSingleton( IScenarioTracker::tidTypeID );
			}
			SPeer &peer = peers[nPlayer];
			NSingleton::RegisterSingleton( peer.timer, IGameTimer::tidTypeID );
			NSingleton::RegisterSingleton( peer.ai, IAILogic::tidTypeID );
			NSingleton::RegisterSingleton( peer.scenario, IScenarioTracker::tidTypeID );
			nSelectedPeer = nPlayer;
		}
		pExecutedCommands = &peers[nPlayer].executed;
		ui->pManager = peers[nPlayer].manager;
	}

	void Queue( int nSender, int nRecipient, CNetPacket *pPacket, bool bPersistent )
	{
		if ( nRecipient == nSender || !peers[nRecipient].bOnline )
		{
			return;
		}
		SDelivery delivery;
		delivery.nSender = nSender;
		delivery.nRecipient = nRecipient;
		delivery.bPersistent = bPersistent;
		delivery.packet = ClonePacket( pPacket );
		ASSERT_TRUE( IsValid( delivery.packet ) );
		delivery.packet->nClientID = CLIENT_IDS[nSender];
		if ( NObjectFactory::GetObjectTypeID( delivery.packet.GetPtr() ) == 111 )
		{
			delivery.nSegment = ReadSegment( delivery.packet ).nSegment;
		}
		pending.push_back( delivery );
	}

	void Route( int nSender, CNetPacket *pPacket, bool bPersistent )
	{
		if ( bPersistent )
		{
			CPtr<CNetPacket> decoded = ClonePacket( pPacket );
			ASSERT_TRUE( IsValid( decoded ) );
			if ( auto *pWrapper = dynamic_cast<CDirectPacketToClient *>( decoded.GetPtr() ) )
			{
				for ( int i = 0; i < PLAYER_COUNT; ++i )
				{
					if ( CLIENT_IDS[i] == pWrapper->nClient )
					{
						Queue( nSender, i, pWrapper->pPacket, true );
						return;
					}
				}
				ADD_FAILURE() << "Unknown persistent recipient " << pWrapper->nClient;
			}
			// Periodic lobby bookkeeping does not deliver simulation input.
			return;
		}
		for ( int i = 0; i < PLAYER_COUNT; ++i )
		{
			Queue( nSender, i, pPacket, false );
		}
	}

	void SetUp() override
	{
		CObj<IAICmdsAutoMagic> registration = CreateAICmdsAutoMagic();
		ASSERT_NE( registration, nullptr );
		packer = new CTailCommandPacker;
		console = dynamic_cast<IConsoleBuffer *>( NObjectFactory::MakeObject( 0x300C8D40 ) );
		ASSERT_NE( console, nullptr );
		ui = new CDispatchingUI;
		NSingleton::RegisterSingleton( console, IConsoleBuffer::tidTypeID );
		NSingleton::RegisterSingleton( ui, IMPToUIManager::tidTypeID );
		for ( int i = 0; i < PLAYER_COUNT; ++i )
		{
			SPeer &peer = peers[i];
			peer.client = new CRoutedClient;
			peer.client->send = [this, i]( CNetPacket *pPacket, bool bPersistent ) { Route( i, pPacket, bPersistent ); };
			peer.history = new CRecordingHistory;
			peer.ai = new CMPHeadlessAI;
			peer.manager = dynamic_cast<CMPManagerModeNivalNet *>( NObjectFactory::MakeObject( 0x192444C0 ) );
			peer.transceiver = dynamic_cast<CMPTransceiver *>( NObjectFactory::MakeObject( 0x1713C440 ) );
			peer.scenario = dynamic_cast<IScenarioTracker *>( NObjectFactory::MakeObject( 0x1117BB00 ) );
			peer.timer = dynamic_cast<IGameTimer *>( NObjectFactory::MakeObject( 0x10075C05 ) );
			ASSERT_NE( peer.manager, nullptr );
			ASSERT_NE( peer.transceiver, nullptr );
			ASSERT_NE( peer.scenario, nullptr );
			ASSERT_NE( peer.timer, nullptr );
			SScenarioPlayers players;
			RestoreState( peer.scenario, players );
			peer.scenario->SetLocalPlayer( i );
			STimerState timerState;
			RestoreState( peer.timer, timerState );
			peer.timer->Reset( peer.wallTime );
			peer.timer->SetSpeed( 0 );
			SelectPeer( i );
			CMPPlayerDropTestAccess::Initialize(
					peer.manager, peer.transceiver, peer.client, peer.history, packer, peer.ai, peer.timer, i );
		}
	}

	void TearDown() override
	{
		pExecutedCommands = nullptr;
		ui->pManager = nullptr;
		if ( nSelectedPeer >= 0 )
		{
			NSingleton::UnRegisterSingleton( IGameTimer::tidTypeID );
			NSingleton::UnRegisterSingleton( IAILogic::tidTypeID );
			NSingleton::UnRegisterSingleton( IScenarioTracker::tidTypeID );
		}
		NSingleton::UnRegisterSingleton( IMPToUIManager::tidTypeID );
		NSingleton::UnRegisterSingleton( IConsoleBuffer::tidTypeID );
		for ( SPeer &peer : peers )
		{
			peer.client->send = {};
			peer.manager = nullptr;
			peer.transceiver = nullptr;
		}
	}

	void RunManager( int nPlayer )
	{
		if ( peers[nPlayer].bOnline )
		{
			SelectPeer( nPlayer );
			CPacketProcessorBase *pProcessor = peers[nPlayer].manager;
			ASSERT_TRUE( pProcessor->Segment() );
		}
	}

	void DeliverRound()
	{
		while ( !pending.empty() )
		{
			SDelivery delivery = pending.front();
			pending.pop_front();
			if ( !peers[delivery.nRecipient].bOnline )
			{
				continue;
			}
			if ( !delivery.bReleased && shouldHold && shouldHold( delivery ) )
			{
				held.push_back( delivery );
				continue;
			}
			peers[delivery.nRecipient].client->received.push_back( delivery.packet );
			delivered.push_back( delivery );
		}
		for ( int i = 0; i < PLAYER_COUNT; ++i )
		{
			RunManager( i );
		}
	}

	void Pump()
	{
		for ( int nRound = 0; nRound < 128; ++nRound )
		{
			DeliverRound();
			if ( pending.empty() )
			{
				return;
			}
		}
		FAIL() << "Protocol did not quiesce after 128 delivery rounds";
	}

	void Frame()
	{
		for ( int i = 0; i < PLAYER_COUNT; ++i )
		{
			SPeer &peer = peers[i];
			if ( !peer.bOnline )
			{
				continue;
			}
			SelectPeer( i );
			peer.wallTime += 200;
			peer.timer->Update( peer.wallTime );
			const int nSegment = peer.transceiver->GetCurrentCommonSegment();
			if ( peer.nLastQueuedCommand != nSegment )
			{
				CPtr<CTailCommand> command = new CTailCommand;
				command->nValue = i * 100000 + nSegment;
				ITransceiver *pSimulation = peer.transceiver;
				pSimulation->SendCommand( command );
				peer.nLastQueuedCommand = nSegment;
			}
			ITransceiver *pSimulation = peer.transceiver;
			pSimulation->DoSegments();
		}
		Pump();
	}

	void Frames( int nCount )
	{
		for ( int i = 0; i < nCount; ++i )
		{
			Frame();
		}
	}

	void NotifyDeparture( int nRecipient, int nPlayer )
	{
		peers[nRecipient].client->received.push_back( new CGameClientRemoved( CLIENT_IDS[nPlayer] ) );
	}

	void Crash( int nPlayer )
	{
		peers[nPlayer].bOnline = false;
		for ( int i = 0; i < PLAYER_COUNT; ++i )
		{
			if ( peers[i].bOnline )
			{
				NotifyDeparture( i, nPlayer );
			}
		}
	}

	void CreateUnevenPrefixes( int nDeparting )
	{
		Frames( 20 );
		std::vector<int> survivors;
		for ( int i = 0; i < PLAYER_COUNT; ++i )
		{
			if ( i != nDeparting )
			{
				survivors.push_back( i );
			}
		}
		shouldHold = [nDeparting, survivors]( const SDelivery &delivery )
		{
			if ( delivery.bPersistent || delivery.nSender != nDeparting || delivery.nSegment < 0 )
			{
				return false;
			}
			const int limits[] = { 20, 22, 23 };
			for ( int i = 0; i < survivors.size(); ++i )
			{
				if ( delivery.nRecipient == survivors[i] )
				{
					return delivery.nSegment >= limits[i];
				}
			}
			return false;
		};
		Frames( 6 );
		ASSERT_EQ( CMPPlayerDropTestAccess::LastReceived( peers[survivors[0]].transceiver, nDeparting ), 19 );
		ASSERT_EQ( CMPPlayerDropTestAccess::LastReceived( peers[survivors[1]].transceiver, nDeparting ), 21 );
		ASSERT_EQ( CMPPlayerDropTestAccess::LastReceived( peers[survivors[2]].transceiver, nDeparting ), 22 );
		ASSERT_EQ( peers[survivors[0]].transceiver->GetCurrentCommonSegment(), 22 );
		ASSERT_EQ( peers[nDeparting].transceiver->GetDepartureSegment(), 25 );
	}

	void ReleaseHeld( const std::function<bool( const SDelivery & )> &matches )
	{
		for ( auto it = held.begin(); it != held.end(); )
		{
			if ( matches( *it ) )
			{
				it->bReleased = true;
				pending.push_back( *it );
				it = held.erase( it );
			}
			else
			{
				++it;
			}
		}
	}

	void TogglePause( int nPlayer )
	{
		SelectPeer( nPlayer );
		CObj<SMPUIMessage> message = new CPauseMessage;
		CMPUIMessageTranslator *pTranslator = peers[nPlayer].manager;
		ASSERT_TRUE( pTranslator->HandleMessage( message ) );
	}

	void AdvanceSurvivorsPast( int nSegment )
	{
		for ( int nFrame = 0; nFrame < 40; ++nFrame )
		{
			bool bReached = true;
			for ( const SPeer &peer : peers )
			{
				if ( peer.bOnline )
				{
					ASSERT_TRUE( IsValid( peer.transceiver ) );
					ASSERT_FALSE( peer.transceiver->HasPlayerDropError() );
					ASSERT_FALSE( peer.transceiver->IsGameEnded() );
					bReached = bReached && peer.transceiver->GetCurrentCommonSegment() > nSegment;
				}
			}
			if ( bReached )
			{
				return;
			}
			Frame();
		}
		FAIL() << "Survivors did not advance beyond " << nSegment;
	}

	void CheckEqualSurvivorHistories( uint32_t dwDeparted )
	{
		int nFirst = -1;
		int nCommonEnd = ( std::numeric_limits<int>::max )();
		for ( int i = 0; i < PLAYER_COUNT; ++i )
		{
			if ( peers[i].bOnline )
			{
				if ( nFirst < 0 )
				{
					nFirst = i;
				}
				nCommonEnd = (std::min)( nCommonEnd, int( peers[i].transceiver->GetCurrentCommonSegment() ) );
			}
		}
		ASSERT_GE( nFirst, 0 );
		std::vector<std::pair<int, std::string>> expected;
		for ( const auto &command : peers[nFirst].history->GetOrderedCommands() )
		{
			if ( command.first < nCommonEnd )
			{
				expected.push_back( command );
			}
		}
		ASSERT_FALSE( expected.empty() );
		for ( int i = 0; i < PLAYER_COUNT; ++i )
		{
			if ( !peers[i].bOnline )
			{
				continue;
			}
			std::vector<std::pair<int, std::string>> actual;
			for ( const auto &command : peers[i].history->GetOrderedCommands() )
			{
				if ( command.first < nCommonEnd )
				{
					actual.push_back( command );
				}
			}
			EXPECT_EQ( actual, expected ) << "survivor " << i;
			EXPECT_FALSE( peers[i].transceiver->HasPlayerDropError() );
			for ( int nPlayer = 0; nPlayer < PLAYER_COUNT; ++nPlayer )
			{
				if ( ( dwDeparted & ( 1u << nPlayer ) ) != 0 )
				{
					EXPECT_FALSE( peers[i].scenario->IsPlayerPresent( nPlayer ) );
					EXPECT_EQ( CMPPlayerDropTestAccess::Removal( peers[i].transceiver, nPlayer ),
					           CMPPlayerDropTestAccess::Removal( peers[nFirst].transceiver, nPlayer ) );
					const std::string removal = "DropPlayer " + std::to_string( nPlayer );
					EXPECT_EQ( std::count_if( actual.begin(),
					                          actual.end(),
					                          [&removal]( const auto &command ) { return command.second == removal; } ),
					           1 );
				}
			}
		}
	}

	static const CB2DepartureRecoveryPacket *Recovery( const SDelivery &delivery )
	{
		return dynamic_cast<const CB2DepartureRecoveryPacket *>( delivery.packet.GetPtr() );
	}
};

TEST_F( MPAbruptLeave, RoutedPeersProduceReceiveAndExecuteIdenticalCommandsFromStartup )
{
	Frames( 20 );
	for ( int i = 0; i < PLAYER_COUNT; ++i )
	{
		EXPECT_EQ( peers[i].transceiver->GetCurrentCommonSegment(), 20 );
		EXPECT_EQ( peers[i].timer->GetAbsTime(), 4000u );
		EXPECT_EQ( peers[i].ai->nSegments, 20 );
		EXPECT_FALSE( peers[i].transceiver->HasPlayerDropError() );
		EXPECT_EQ( peers[i].executed, peers[0].executed );
		EXPECT_EQ( peers[i].history->GetOrderedCommands(), peers[0].history->GetOrderedCommands() );
		for ( int nSender = 0; nSender < PLAYER_COUNT; ++nSender )
		{
			if ( nSender != i )
			{
				EXPECT_EQ( CMPPlayerDropTestAccess::LastReceived( peers[i].transceiver, nSender ), 19 );
			}
		}
	}
	EXPECT_EQ( peers[0].executed.size(), 17u * PLAYER_COUNT );
	EXPECT_TRUE( held.empty() );
}

TEST_F( MPAbruptLeave, HeldOrderedStreamsProduceUnevenPrefixesWithoutRestoringReceiptState )
{
	CreateUnevenPrefixes( 3 );
	EXPECT_EQ( peers[1].transceiver->GetCurrentCommonSegment(), 24 );
	EXPECT_EQ( peers[2].transceiver->GetCurrentCommonSegment(), 24 );
	EXPECT_EQ( peers[3].transceiver->GetCurrentCommonSegment(), 24 );
	EXPECT_FALSE( held.empty() );
	for ( const SDelivery &delivery : held )
	{
		EXPECT_EQ( delivery.nSender, 3 );
		EXPECT_FALSE( delivery.bPersistent );
		EXPECT_GE( delivery.nSegment, 20 );
	}
}

TEST_F( MPAbruptLeave, CrashBeforeAnyCommandPacketRequiresNoInventedStartupInput )
{
	Crash( 3 );
	Pump();
	for ( int i : { 0, 1, 2 } )
	{
		EXPECT_EQ( peers[i].transceiver->GetCurrentCommonSegment(), 0 );
		EXPECT_EQ( CMPPlayerDropTestAccess::LastReceived( peers[i].transceiver, 3 ), -1 );
		EXPECT_EQ( CMPPlayerDropTestAccess::Removal( peers[i].transceiver, 3 ), 3 );
		EXPECT_FALSE( peers[i].transceiver->IsRecoveringDeparture() );
	}
	AdvanceSurvivorsPast( 7 );
	CheckEqualSurvivorHistories( 1u << 3 );
	for ( int i : { 0, 1, 2 } )
	{
		EXPECT_EQ( CMPPlayerDropTestAccess::LastReceived( peers[i].transceiver, 3 ), -1 );
	}
}

TEST_F( MPAbruptLeave, UnevenSurvivorPrefixesRecoverAllAcceptedCommandsBeforeOneRemoval )
{
	CreateUnevenPrefixes( 3 );
	Crash( 3 );
	Pump();
	for ( int i : { 0, 1, 2 } )
	{
		EXPECT_FALSE( peers[i].transceiver->IsRecoveringDeparture() );
		EXPECT_EQ( CMPPlayerDropTestAccess::LastReceived( peers[i].transceiver, 3 ), 22 );
		EXPECT_EQ( CMPPlayerDropTestAccess::Removal( peers[i].transceiver, 3 ), 26 );
	}
	AdvanceSurvivorsPast( 29 );
	CheckEqualSurvivorHistories( 1u << 3 );
	for ( int i : { 0, 1, 2 } )
	{
		EXPECT_EQ( std::count( peers[i].executed.begin(), peers[i].executed.end(), 300022 ), 1 );
		EXPECT_EQ( std::count( peers[i].executed.begin(), peers[i].executed.end(), 300023 ), 0 );
	}
}

TEST_F( MPAbruptLeave, IncompleteRecoveryPrefixCannotAdvanceReceiptOrImportCommands )
{
	CreateUnevenPrefixes( 3 );
	std::vector<SMPRecoveryCommand> complete;
	peers[2].transceiver->GetRecoveryCommands( 1u << 3, &complete );
	std::vector<SMPRecoveryCommand> incomplete;
	for ( const SMPRecoveryCommand &command : complete )
	{
		if ( command.nSegment == 22 )
		{
			incomplete.push_back( command );
		}
	}
	ASSERT_EQ( incomplete.size(), 1u );
	std::vector<int> cutoffs( PLAYER_COUNT, -1 );
	cutoffs[3] = 24;
	SelectPeer( 0 );
	peers[0].transceiver->SetDepartureRecovery( true );
	const size_t nQueuedBefore = CMPPlayerDropTestAccess::QueuedCommands( peers[0].transceiver, 3 );
	const auto executedBefore = peers[0].executed;
	EXPECT_FALSE( peers[0].transceiver->RestoreRecoveryCommands( incomplete, 1u << 3, cutoffs ) );
	EXPECT_EQ( CMPPlayerDropTestAccess::LastReceived( peers[0].transceiver, 3 ), 19 );
	EXPECT_EQ( CMPPlayerDropTestAccess::QueuedCommands( peers[0].transceiver, 3 ), nQueuedBefore );
	EXPECT_EQ( peers[0].executed, executedBefore );
	EXPECT_TRUE( peers[0].transceiver->IsRecoveringDeparture() );
	// A rejected plan must not poison the subsequent complete repair either.
	EXPECT_TRUE( peers[0].transceiver->RestoreRecoveryCommands( complete, 1u << 3, cutoffs ) );
	EXPECT_EQ( CMPPlayerDropTestAccess::LastReceived( peers[0].transceiver, 3 ), 22 );
	EXPECT_EQ( CMPPlayerDropTestAccess::QueuedCommands( peers[0].transceiver, 3 ), nQueuedBefore + 3 );
	EXPECT_EQ( peers[0].executed, executedBefore );
}

TEST_F( MPAbruptLeave, FrozenSnapshotsFenceNativePacketsThatArriveAfterReporting )
{
	CreateUnevenPrefixes( 3 );
	const auto holdInput = shouldHold;
	shouldHold = [holdInput]( const SDelivery &delivery )
	{
		const auto *pRecovery = Recovery( delivery );
		return holdInput( delivery ) || ( pRecovery && pRecovery->ePhase == CB2DepartureRecoveryPacket::SNAPSHOT &&
		                                  delivery.nSender == 2 && delivery.nRecipient == 0 );
	};
	Crash( 3 );
	Pump();
	std::array<int, 3> frozenAt;
	for ( int i : { 0, 1, 2 } )
	{
		ASSERT_TRUE( peers[i].transceiver->IsRecoveringDeparture() );
		frozenAt[i] = peers[i].transceiver->GetCurrentCommonSegment();
	}
	Frames( 2 );
	for ( int i : { 0, 1, 2 } )
	{
		EXPECT_EQ( peers[i].transceiver->GetCurrentCommonSegment(), frozenAt[i] );
	}
	// Packet23 was genuinely produced before the crash, but no frozen report
	// included it. Receipt after the snapshot must not silently extend the plan.
	ReleaseHeld( []( const SDelivery &delivery )
	             { return delivery.nSender == 3 && delivery.nRecipient == 2 && delivery.nSegment == 23; } );
	Pump();
	ReleaseHeld(
			[]( const SDelivery &delivery )
			{
				const auto *pRecovery = Recovery( delivery );
				return pRecovery && pRecovery->ePhase == CB2DepartureRecoveryPacket::SNAPSHOT && delivery.nSender == 2 &&
		           delivery.nRecipient == 0;
			} );
	Pump();
	for ( int i : { 0, 1, 2 } )
	{
		EXPECT_EQ( CMPPlayerDropTestAccess::Removal( peers[i].transceiver, 3 ), 26 );
	}
	AdvanceSurvivorsPast( 29 );
	CheckEqualSurvivorHistories( 1u << 3 );
	for ( int i : { 0, 1, 2 } )
	{
		EXPECT_EQ( std::count( peers[i].executed.begin(), peers[i].executed.end(), 300023 ), 0 );
	}
}

TEST_F( MPAbruptLeave, AbruptHostDepartureRecoversThroughTheLowestSurvivingSlot )
{
	CreateUnevenPrefixes( 0 );
	Crash( 0 );
	Pump();
	for ( int i : { 1, 2, 3 } )
	{
		EXPECT_EQ( CMPPlayerDropTestAccess::HostClient( peers[i].manager ), CLIENT_IDS[1] );
		EXPECT_FALSE( peers[i].transceiver->IsRecoveringDeparture() );
		EXPECT_EQ( CMPPlayerDropTestAccess::LastReceived( peers[i].transceiver, 0 ), 22 );
		EXPECT_EQ( CMPPlayerDropTestAccess::Removal( peers[i].transceiver, 0 ), 26 );
	}
	AdvanceSurvivorsPast( 29 );
	CheckEqualSurvivorHistories( 1u << 0 );
}

TEST_F( MPAbruptLeave, CoordinatorJoiningAfterOtherPeersSnapshotsRequestsTheirOriginalReports )
{
	CreateUnevenPrefixes( 3 );
	peers[3].bOnline = false;
	NotifyDeparture( 1, 3 );
	NotifyDeparture( 2, 3 );
	Pump();
	// These peers already reported, but the coordinator could not accept their
	// departure claim before its own authoritative backend notification arrived.
	ASSERT_FALSE( peers[0].transceiver->IsRecoveringDeparture() );
	ASSERT_TRUE( peers[1].transceiver->IsRecoveringDeparture() );
	ASSERT_TRUE( peers[2].transceiver->IsRecoveringDeparture() );
	NotifyDeparture( 0, 3 );
	Pump();
	for ( int i : { 0, 1, 2 } )
	{
		EXPECT_FALSE( peers[i].transceiver->IsRecoveringDeparture() );
		EXPECT_EQ( CMPPlayerDropTestAccess::Removal( peers[i].transceiver, 3 ), 26 );
	}
	AdvanceSurvivorsPast( 29 );
	CheckEqualSurvivorHistories( 1u << 3 );
}

TEST_F( MPAbruptLeave, TwoAbruptDeparturesInOneReceiveBatchConverge )
{
	CreateUnevenPrefixes( 3 );
	Crash( 3 );
	Crash( 2 );
	Pump();
	for ( int i : { 0, 1 } )
	{
		EXPECT_FALSE( peers[i].transceiver->IsRecoveringDeparture() );
		EXPECT_GE( CMPPlayerDropTestAccess::Removal( peers[i].transceiver, 2 ), 0 );
		EXPECT_GE( CMPPlayerDropTestAccess::Removal( peers[i].transceiver, 3 ), 0 );
	}
	AdvanceSurvivorsPast( 31 );
	CheckEqualSurvivorHistories( ( 1u << 2 ) | ( 1u << 3 ) );
}

TEST_F( MPAbruptLeave, CoordinatorFailureAfterPartialCommitRetainsTheAcceptedPlan )
{
	CreateUnevenPrefixes( 3 );
	const auto holdInput = shouldHold;
	shouldHold = [holdInput]( const SDelivery &delivery )
	{
		const auto *pRecovery = Recovery( delivery );
		return holdInput( delivery ) || ( pRecovery && pRecovery->ePhase == CB2DepartureRecoveryPacket::COMMIT &&
		                                  pRecovery->dwDeparted == 8 && delivery.nSender == 0 && delivery.nRecipient == 2 );
	};
	Crash( 3 );
	Pump();
	ASSERT_EQ( CMPPlayerDropTestAccess::Removal( peers[1].transceiver, 3 ), 26 );
	ASSERT_FALSE( peers[1].transceiver->IsRecoveringDeparture() );
	ASSERT_TRUE( peers[2].transceiver->IsRecoveringDeparture() );
	Frame();
	Crash( 0 );
	Pump();
	for ( int i : { 1, 2 } )
	{
		EXPECT_EQ( CMPPlayerDropTestAccess::HostClient( peers[i].manager ), CLIENT_IDS[1] );
		EXPECT_FALSE( peers[i].transceiver->IsRecoveringDeparture() );
		EXPECT_EQ( CMPPlayerDropTestAccess::Removal( peers[i].transceiver, 3 ), 26 );
	}
	// The old coordinator's delayed smaller-mask commit cannot undo the new
	// round, even on the peer that missed that first commit entirely.
	ReleaseHeld(
			[]( const SDelivery &delivery )
			{
				const auto *pRecovery = Recovery( delivery );
				return pRecovery && pRecovery->ePhase == CB2DepartureRecoveryPacket::COMMIT && pRecovery->dwDeparted == 8;
			} );
	Pump();
	AdvanceSurvivorsPast( 33 );
	CheckEqualSurvivorHistories( ( 1u << 0 ) | ( 1u << 3 ) );
}

TEST_F( MPAbruptLeave, RecoveryDoesNotReleaseAnIndependentManualPause )
{
	CreateUnevenPrefixes( 3 );
	TogglePause( 1 );
	Pump();
	ASSERT_TRUE( CMPPlayerDropTestAccess::LocalPause( peers[1].transceiver ) );
	ASSERT_TRUE( peers[1].timer->HasPause( PAUSE_TYPE_USER_PAUSE ) );
	Crash( 3 );
	Pump();
	for ( int i : { 0, 1, 2 } )
	{
		EXPECT_FALSE( peers[i].transceiver->IsRecoveringDeparture() );
	}
	const int nPausedAt = peers[1].transceiver->GetCurrentCommonSegment();
	Frames( 2 );
	EXPECT_EQ( peers[1].transceiver->GetCurrentCommonSegment(), nPausedAt );
	EXPECT_TRUE( CMPPlayerDropTestAccess::LocalPause( peers[1].transceiver ) );
	EXPECT_TRUE( peers[1].timer->HasPause( PAUSE_TYPE_USER_PAUSE ) );
	TogglePause( 1 );
	Pump();
	AdvanceSurvivorsPast( 29 );
	CheckEqualSurvivorHistories( 1u << 3 );
}

TEST_F( MPAbruptLeave, DelayedLegacyDropCannotChangeAnActiveOrCompletedRecoveryPlan )
{
	CreateUnevenPrefixes( 3 );
	const auto holdInput = shouldHold;
	shouldHold = [holdInput]( const SDelivery &delivery )
	{
		const auto *pRecovery = Recovery( delivery );
		return holdInput( delivery ) || ( pRecovery && pRecovery->ePhase == CB2DepartureRecoveryPacket::SNAPSHOT &&
		                                  delivery.nSender == 2 && delivery.nRecipient == 0 );
	};
	Crash( 3 );
	Pump();
	CPtr<CNetPacket> earlierDrop = new CB2DropPlayerAtSegmentPacket( CLIENT_IDS[0], 3, 23 );
	for ( int i : { 1, 2 } )
	{
		ASSERT_TRUE( peers[i].transceiver->IsRecoveringDeparture() );
		Queue( 0, i, earlierDrop, true );
	}
	Pump();
	for ( int i : { 1, 2 } )
	{
		EXPECT_EQ( CMPPlayerDropTestAccess::Removal( peers[i].transceiver, 3 ), -1 );
	}
	ReleaseHeld(
			[]( const SDelivery &delivery )
			{
				const auto *pRecovery = Recovery( delivery );
				return pRecovery && pRecovery->ePhase == CB2DepartureRecoveryPacket::SNAPSHOT && delivery.nSender == 2 &&
		           delivery.nRecipient == 0;
			} );
	Pump();
	for ( int i : { 1, 2 } )
	{
		ASSERT_FALSE( peers[i].transceiver->IsRecoveringDeparture() );
		ASSERT_EQ( CMPPlayerDropTestAccess::Removal( peers[i].transceiver, 3 ), 26 );
		Queue( 0, i, earlierDrop, true );
	}
	Pump();
	for ( int i : { 1, 2 } )
	{
		EXPECT_EQ( CMPPlayerDropTestAccess::Removal( peers[i].transceiver, 3 ), 26 );
	}
	AdvanceSurvivorsPast( 29 );
	CheckEqualSurvivorHistories( 1u << 3 );
}

TEST_F( MPAbruptLeave, SlowSnapshotKeepsSimulationFrozenWithoutConsumingPauseAllowance )
{
	CreateUnevenPrefixes( 3 );
	TogglePause( 1 );
	Pump();
	const auto holdInput = shouldHold;
	shouldHold = [holdInput]( const SDelivery &delivery )
	{
		const auto *pRecovery = Recovery( delivery );
		return holdInput( delivery ) || ( pRecovery && pRecovery->ePhase == CB2DepartureRecoveryPacket::SNAPSHOT &&
		                                  delivery.nSender == 2 && delivery.nRecipient == 0 );
	};
	Crash( 3 );
	Pump();
	std::array<int, 3> frozenAt;
	for ( int i : { 0, 1, 2 } )
	{
		ASSERT_TRUE( peers[i].transceiver->IsRecoveringDeparture() );
		frozenAt[i] = peers[i].transceiver->GetCurrentCommonSegment();
		ASSERT_EQ( CMPPlayerDropTestAccess::Budget( peers[i].manager, 1 ), 180000 );
	}
	Frames( 35 );
	for ( int i : { 0, 1, 2 } )
	{
		EXPECT_EQ( peers[i].transceiver->GetCurrentCommonSegment(), frozenAt[i] );
		EXPECT_TRUE( peers[i].transceiver->IsRecoveringDeparture() );
		EXPECT_FALSE( peers[i].transceiver->HasPlayerDropError() );
		EXPECT_EQ( CMPPlayerDropTestAccess::Budget( peers[i].manager, 1 ), 180000 );
	}
	// Resume affects only the independent manual pause; it must not advance
	// simulation or charge the seven seconds spent waiting for recovery input.
	TogglePause( 1 );
	Pump();
	Frame();
	EXPECT_FALSE( CMPPlayerDropTestAccess::LocalPause( peers[1].transceiver ) );
	EXPECT_TRUE( peers[1].transceiver->IsRecoveringDeparture() );
	EXPECT_EQ( peers[1].transceiver->GetCurrentCommonSegment(), frozenAt[1] );
	EXPECT_EQ( CMPPlayerDropTestAccess::Budget( peers[1].manager, 1 ), 180000 );
	ReleaseHeld(
			[]( const SDelivery &delivery )
			{
				const auto *pRecovery = Recovery( delivery );
				return pRecovery && pRecovery->ePhase == CB2DepartureRecoveryPacket::SNAPSHOT && delivery.nSender == 2 &&
		           delivery.nRecipient == 0;
			} );
	Pump();
	for ( int i : { 0, 1, 2 } )
	{
		EXPECT_FALSE( peers[i].transceiver->IsRecoveringDeparture() );
		EXPECT_EQ( CMPPlayerDropTestAccess::Budget( peers[i].manager, 1 ), 180000 );
	}
	AdvanceSurvivorsPast( 29 );
	CheckEqualSurvivorHistories( 1u << 3 );
}
} // namespace
