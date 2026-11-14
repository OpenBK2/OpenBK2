#include "../stdafx.h"

#include "../AICmdsAutoMagic.h"
#include "../CommandsHistory.h"
#include "../DBMPConsts.h"
#include "../MPManagerModeNivalNet.hpp"
#include "../MPTransceiver.h"
#include "../ScenarioTracker.h"
#include "AILogic/B2AI.h"
#include "Client/ServerClientInterface.h"
#include "Main/DBNetConsts.h"
#include "Main/GameTimer.h"
#include "Server_Client_Common/CommonPackets.h"
#include "Server_Client_Common/GamePackets.h"
#include "System/ConsoleBuffer.h"

#include <deque>

#include <gtest/gtest.h>

namespace
{
constexpr int HOST = 0;
constexpr int TARGET = 1;
constexpr int OBSERVER = 2;
constexpr int CLIENT_IDS[] = { 101, 102, 103 };
constexpr uint32_t ALL_PLAYERS = 7;
constexpr uint32_t TARGET_BIT = 1u << TARGET;
constexpr int TOTAL_BUDGET = 150000;

class CRecordingServerClient : public IServerClient
{
	OBJECT_NOCOPY_METHODS( CRecordingServerClient );

public:
	std::vector<CPtr<CNetPacket>> sent;
	std::deque<CPtr<CNetPacket>> received;
	CPtr<CNetPacket> currentPacket;

	CNetPacket *GetPacket() override
	{
		currentPacket = nullptr;
		if ( !received.empty() )
		{
			currentPacket = received.front();
			received.pop_front();
		}
		return currentPacket;
	}

	void SendPacket( CNetPacket *pPacket ) override
	{
		sent.push_back( pPacket );
	}

	void SendGamePacket( CNetPacket *pPacket, bool ) override
	{
		sent.push_back( pPacket );
	}

	void Segment() override {}
};

class CRecordingUI : public IMPToUIManager
{
	OBJECT_NOCOPY_METHODS( CRecordingUI );

public:
	int nLagReports = 0;

	void AddUIMessage( SMPUIMessage *pMessage ) override
	{
		CPtr<SMPUIMessage> retained = pMessage;
		if ( dynamic_cast<SMPUILagMessage *>( pMessage ) )
		{
			++nLagReports;
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
	std::vector<SPlayerPresence> players = std::vector<SPlayerPresence>( 3 );

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

// The production packet is private to MPTransceiver.cpp. Restore its real
// serialized fields, then dispatch its registered factory object virtually.
struct SReceivedSegment
{
	int nSegment = 7778;
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
} // namespace

// Restore only lobby, timer and receipt state. Pause transitions, accounting,
// voting and recovery run through the production objects' actual handlers.
class CMPPlayerDropTestAccess
{
public:
	static void Initialize( CMPManagerModeNivalNet *pManager,
	                        CMPTransceiver *pTransceiver,
	                        IServerClient *pClient,
	                        ICommandsHistory *pHistory,
	                        IAILogic *pAI,
	                        IGameTimer *pTimer,
	                        int nOwnSlot )
	{
		pManager->eState = CMPManagerModeNivalNet::EGS_GAME_STARTED;
		pManager->nMyClientID = CLIENT_IDS[nOwnSlot];
		pManager->bHost = nOwnSlot == HOST;
		pManager->pClient = pClient;
		pManager->pTransceiver = pTransceiver;
		pManager->nOwnSlot = nOwnSlot;
		pManager->nGameID = 968;
		pManager->nHostClientID = CLIENT_IDS[HOST];
		pManager->nSlotsUsed = 3;
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
		pManager->lagsUpdate.bUpdating = true;
		pManager->lagsUpdate.timeNextUpdate = 0;
		pManager->slots.resize( 3 );
		pManager->lags.resize( 3 );
		pManager->playerDrops.resize( 3 );
		pTransceiver->players.resize( 3 );
		pTransceiver->playerRemovals.resize( 3 );
		for ( int i = 0; i < 3; ++i )
		{
			pManager->slots[i].nClientID = CLIENT_IDS[i];
			pManager->slots[i].nTeam = i == TARGET ? 1 : 0;
			pManager->slots[i].szName = i == HOST ? "Host" : i == TARGET ? "Target" : "Observer";
			pManager->slots[i].bPresent = true;
			pManager->slots[i].bAccept = true;
			pManager->lags[i].nLagLeft = TOTAL_BUDGET;
			pTransceiver->players[i].nClientID = CLIENT_IDS[i];
			pTransceiver->players[i].nTeam = pManager->slots[i].nTeam;
		}
		CPtr<NDb::SMultiplayerConsts> mpConsts =
				dynamic_cast<NDb::SMultiplayerConsts *>( NObjectFactory::MakeObject( 0x191B2300 ) );
		mpConsts->nTimeUserMPPause = 120;
		mpConsts->nTimeUserMPLag = 30;
		pManager->pMPConsts = mpConsts;
		pTransceiver->pClient = pClient;
		pTransceiver->pCmdsHistory = pHistory;
		pTransceiver->pAI = pAI;
		pTransceiver->pTimer = pTimer;
		pTransceiver->nMyLogicID = nOwnSlot;
		pTransceiver->nLatency = 2;
		pTransceiver->nSegmentsPackSize = 1;
		pTransceiver->nCommonSegment = 7780;
		pTransceiver->nFinalSegment = -1;
		pTransceiver->bIsGameRunning = true;
		pTransceiver->bWaiting = false;
		pTransceiver->wMask = ALL_PLAYERS;
		pTransceiver->wWaitMask = ALL_PLAYERS;
		CPtr<NDb::SNetGameConsts> netConsts =
				dynamic_cast<NDb::SNetGameConsts *>( NObjectFactory::MakeObject( 0x300A7B40 ) );
		netConsts->nMaxLatency = 16;
		pTransceiver->pConsts = netConsts;
		pTransceiver->nSegment = pTransceiver->nCommonSegment % 16;
		pTransceiver->segmFinished.assign( 16, 0 );
		pTransceiver->segmFinished[2] = ALL_PLAYERS;
		pTransceiver->checkSums.SetSizes( 16, 3 );
		pTransceiver->checkSums.FillZero();
		pTransceiver->cmds.SetSizes( 3, 16 );
	}

	static void RequirePacket( CMPTransceiver *pTransceiver, int nPlayer )
	{
		pTransceiver->segmFinished[2] &= ~( 1u << nPlayer );
		pTransceiver->players[nPlayer].nLastReceivedSegment = -1;
	}

	static void SetBudget( CMPManagerMode *pManager, int nPlayer, int nMilliseconds )
	{
		pManager->lags[nPlayer].nLagLeft = nMilliseconds;
	}

	static int Budget( const CMPManagerMode *pManager, int nPlayer )
	{
		return pManager->lags[nPlayer].nLagLeft;
	}

	static NTimer::STime Start( const CMPManagerMode *pManager, int nPlayer )
	{
		return pManager->lags[nPlayer].timeStartLag;
	}

	static uint32_t Laggers( const CMPManagerMode *pManager )
	{
		return pManager->dwLaggers;
	}

	static uint32_t Paused( const CMPManagerMode *pManager )
	{
		return pManager->dwUserPausedPlayers;
	}

	static bool WindowShown( const CMPManagerMode *pManager )
	{
		return pManager->bWaitWindowShown;
	}

	static bool LocalWait( const CMPTransceiver *pTransceiver )
	{
		return pTransceiver->bWaiting;
	}

	static int Removal( const CMPTransceiver *pTransceiver, int nPlayer )
	{
		return pTransceiver->playerRemovals[nPlayer].GetRemovalSegment( pTransceiver->nLatency );
	}

	static int LastReceived( const CMPTransceiver *pTransceiver, int nPlayer )
	{
		return pTransceiver->players[nPlayer].nLastReceivedSegment;
	}

	static uint32_t Votes( const CMPManagerMode *pManager, int nPlayer )
	{
		return pManager->lags[nPlayer].dwHatedBy;
	}
};

namespace
{
struct SPeer
{
	int nOwnSlot = -1;
	CObj<CRecordingServerClient> client;
	CObj<CMPManagerModeNivalNet> manager;
	CObj<CMPTransceiver> transceiver;
};

class MPPause : public testing::Test
{
protected:
	SPeer host;
	SPeer observer;
	CObj<ICommandsHistory> history;
	CObj<IScenarioTracker> scenario;
	CObj<IAILogic> ai;
	CObj<IGameTimer> timer;
	CObj<IConsoleBuffer> console;
	CObj<CRecordingUI> ui;

	void InitializePeer( SPeer &peer, int nOwnSlot )
	{
		peer.nOwnSlot = nOwnSlot;
		peer.client = new CRecordingServerClient;
		peer.manager = dynamic_cast<CMPManagerModeNivalNet *>( NObjectFactory::MakeObject( 0x192444C0 ) );
		peer.transceiver = dynamic_cast<CMPTransceiver *>( NObjectFactory::MakeObject( 0x1713C440 ) );
		ASSERT_NE( peer.manager, nullptr );
		ASSERT_NE( peer.transceiver, nullptr );
		CMPPlayerDropTestAccess::Initialize( peer.manager, peer.transceiver, peer.client, history, ai, timer, nOwnSlot );
	}

	void SetUp() override
	{
		CObj<IAICmdsAutoMagic> registry = CreateAICmdsAutoMagic();
		ASSERT_NE( registry, nullptr );
		history = dynamic_cast<ICommandsHistory *>( NObjectFactory::MakeObject( 0x1007AB80 ) );
		scenario = dynamic_cast<IScenarioTracker *>( NObjectFactory::MakeObject( 0x1117BB00 ) );
		ai = dynamic_cast<IAILogic *>( NObjectFactory::MakeObject( 0x1108D441 ) );
		timer = dynamic_cast<IGameTimer *>( NObjectFactory::MakeObject( 0x10075C05 ) );
		console = dynamic_cast<IConsoleBuffer *>( NObjectFactory::MakeObject( 0x300C8D40 ) );
		ASSERT_NE( history, nullptr );
		ASSERT_NE( scenario, nullptr );
		ASSERT_NE( ai, nullptr );
		ASSERT_NE( timer, nullptr );
		ASSERT_NE( console, nullptr );
		ui = new CRecordingUI;
		SScenarioPlayers players;
		RestoreState( scenario, players );
		scenario->SetLocalPlayer( HOST );
		STimerState timerState;
		RestoreState( timer, timerState );
		timer->Reset( 1000 );
		NSingleton::RegisterSingleton( scenario, IScenarioTracker::tidTypeID );
		NSingleton::RegisterSingleton( ai, IAILogic::tidTypeID );
		NSingleton::RegisterSingleton( timer, IGameTimer::tidTypeID );
		NSingleton::RegisterSingleton( console, IConsoleBuffer::tidTypeID );
		NSingleton::RegisterSingleton( ui, IMPToUIManager::tidTypeID );
		InitializePeer( host, HOST );
		InitializePeer( observer, OBSERVER );
	}

	void TearDown() override
	{
		host.manager = nullptr;
		host.transceiver = nullptr;
		observer.manager = nullptr;
		observer.transceiver = nullptr;
		NSingleton::UnRegisterSingleton( IMPToUIManager::tidTypeID );
		NSingleton::UnRegisterSingleton( IConsoleBuffer::tidTypeID );
		NSingleton::UnRegisterSingleton( IGameTimer::tidTypeID );
		NSingleton::UnRegisterSingleton( IAILogic::tidTypeID );
		NSingleton::UnRegisterSingleton( IScenarioTracker::tidTypeID );
	}

	void At( NTimer::STime nElapsed )
	{
		timer->Update( 1000 + nElapsed );
		ASSERT_EQ( timer->GetAbsTime(), nElapsed );
	}

	static void Run( SPeer &peer )
	{
		CPacketProcessorBase *pProcessor = peer.manager;
		ASSERT_TRUE( pProcessor->Segment() );
	}

	static void Deliver( SPeer &peer, CNetPacket *pPacket )
	{
		CPacketProcessorBase *pProcessor = peer.manager;
		ASSERT_TRUE( pProcessor->ProcessPacket( pPacket ) );
	}

	static void PausePacket( SPeer &peer, int nSenderSlot, bool bPaused )
	{
		CObj<CNetPacket> packet = new CB2UserPausePacket( CLIENT_IDS[nSenderSlot], bPaused );
		Deliver( peer, packet );
	}

	static void TogglePause( SPeer &peer )
	{
		CObj<SMPUIMessage> message = new CPauseMessage;
		CMPUIMessageTranslator *pTranslator = peer.manager;
		ASSERT_TRUE( pTranslator->HandleMessage( message ) );
	}

	static void ReportLag( SPeer &peer, uint32_t dwPlayers )
	{
		CObj<SMPUIMessage> message = new SMPUILagMessage( dwPlayers, false );
		CMPUIMessageTranslator *pTranslator = peer.manager;
		ASSERT_TRUE( pTranslator->HandleMessage( message ) );
	}

	static void ReceiveCommands( SPeer &peer, int nSenderSlot )
	{
		CObj<CNetPacket> packet = dynamic_cast<CNetPacket *>( NObjectFactory::MakeObject( 111 ) );
		ASSERT_NE( packet, nullptr );
		SReceivedSegment segment;
		RestoreState( packet, segment );
		packet->nClientID = CLIENT_IDS[nSenderSlot];
		CPacketProcessorBase *pProcessor = peer.transceiver;
		ASSERT_TRUE( pProcessor->ProcessPacket( packet ) );
		ASSERT_EQ( CMPPlayerDropTestAccess::LastReceived( peer.transceiver, nSenderSlot ), 7778 );
	}

	static CPtr<SMPUILagInfoMessage> LastLagInfo( SPeer &peer )
	{
		CPtr<SMPUILagInfoMessage> result;
		CMPUIMessageProcessor *pProcessor = peer.manager;
		while ( CPtr<SMPUIMessage> message = pProcessor->GetMessage() )
		{
			if ( auto *pInfo = dynamic_cast<SMPUILagInfoMessage *>( message.GetPtr() ) )
			{
				result = pInfo;
			}
		}
		return result;
	}

	template <class TPacket>
	static CPtr<TPacket> LastPacket( const SPeer &peer )
	{
		CPtr<TPacket> result;
		for ( const auto &packet : peer.client->sent )
		{
			const auto *pRelay = dynamic_cast<CDirectPacketToClient *>( packet.GetPtr() );
			CNetPacket *pInner = pRelay ? pRelay->pPacket.GetPtr() : packet.GetPtr();
			if ( auto *pTyped = dynamic_cast<TPacket *>( pInner ) )
			{
				result = pTyped;
			}
		}
		return result;
	}

	static void DeliverSent( SPeer &from, SPeer &to )
	{
		for ( const auto &packet : from.client->sent )
		{
			if ( dynamic_cast<CB2UserPausePacket *>( packet.GetPtr() ) ||
			     dynamic_cast<CB2LagTimeUpdatePacket *>( packet.GetPtr() ) ||
			     dynamic_cast<CB2SuggestKickPacket *>( packet.GetPtr() ) )
			{
				packet->nClientID = CLIENT_IDS[from.nOwnSlot];
				to.client->received.push_back( packet );
			}
		}
		from.client->sent.clear();
		Run( to );
	}

	void StartBothPausedWithMissingTarget()
	{
		for ( SPeer *pPeer : { &host, &observer } )
		{
			CMPPlayerDropTestAccess::RequirePacket( pPeer->transceiver, TARGET );
			CMPPlayerDropTestAccess::SetBudget( pPeer->manager, TARGET, 10000 );
			ReportLag( *pPeer, TARGET_BIT );
			TogglePause( *pPeer );
		}
		PausePacket( host, OBSERVER, true );
		PausePacket( observer, HOST, true );
		ASSERT_EQ( host.transceiver->GetCurrentMissingPlayers(), TARGET_BIT );
		ASSERT_EQ( observer.transceiver->GetCurrentMissingPlayers(), TARGET_BIT );
	}
};

TEST_F( MPPause, ReceivedPauseImmediatelyShowsCountdownWithoutPausingLocalCommands )
{
	At( 1000 );
	PausePacket( host, TARGET, true );
	EXPECT_EQ( CMPPlayerDropTestAccess::Paused( host.manager ), TARGET_BIT );
	EXPECT_EQ( CMPPlayerDropTestAccess::Laggers( host.manager ), TARGET_BIT );
	EXPECT_EQ( CMPPlayerDropTestAccess::Start( host.manager, TARGET ), 1000u );
	EXPECT_TRUE( CMPPlayerDropTestAccess::WindowShown( host.manager ) );
	EXPECT_FALSE( CMPPlayerDropTestAccess::LocalWait( host.transceiver ) );
	EXPECT_FALSE( timer->HasPause( PAUSE_TYPE_USER_PAUSE ) );
	auto initial = LastLagInfo( host );
	ASSERT_NE( initial, nullptr );
	ASSERT_EQ( initial->lags.size(), 1u );
	EXPECT_EQ( initial->lags.front().szName, "Target" );
	EXPECT_EQ( initial->lags.front().nSecondsLeft, 120 );
	EXPECT_FALSE( initial->bOwnLag );
	At( 2000 );
	Run( host );
	auto later = LastLagInfo( host );
	ASSERT_NE( later, nullptr );
	ASSERT_EQ( later->lags.size(), 1u );
	EXPECT_EQ( later->lags.front().nSecondsLeft, initial->lags.front().nSecondsLeft - 1 );
}

TEST_F( MPPause, ShortPauseAndResumeAreAccountedBeforeAnyManagerTick )
{
	PausePacket( host, TARGET, true );
	At( 500 );
	PausePacket( host, TARGET, false );
	EXPECT_EQ( CMPPlayerDropTestAccess::Budget( host.manager, TARGET ), TOTAL_BUDGET - 500 );
	EXPECT_EQ( CMPPlayerDropTestAccess::Laggers( host.manager ), 0u );
	EXPECT_FALSE( CMPPlayerDropTestAccess::WindowShown( host.manager ) );
	EXPECT_FALSE( CMPPlayerDropTestAccess::LocalWait( host.transceiver ) );
	auto budget = LastPacket<CB2LagTimeUpdatePacket>( host );
	ASSERT_NE( budget, nullptr );
	EXPECT_EQ( budget->nPlayer, TARGET );
	EXPECT_EQ( budget->nTimeLeft, TOTAL_BUDGET - 500 );
}

TEST_F( MPPause, EmptyNetworkReportCannotClearExplicitRemotePause )
{
	PausePacket( host, TARGET, true );
	At( 1000 );
	ReportLag( host, 0 );
	Run( host );
	EXPECT_EQ( CMPPlayerDropTestAccess::Laggers( host.manager ), TARGET_BIT );
	EXPECT_EQ( CMPPlayerDropTestAccess::Paused( host.manager ), TARGET_BIT );
	EXPECT_EQ( CMPPlayerDropTestAccess::Start( host.manager, TARGET ), 0u );
	EXPECT_TRUE( CMPPlayerDropTestAccess::WindowShown( host.manager ) );
}

TEST_F( MPPause, ResumeRetainsNetworkWaitAndChargesOverlappingReasonsOnce )
{
	CMPPlayerDropTestAccess::RequirePacket( host.transceiver, TARGET );
	At( 1000 );
	ReportLag( host, TARGET_BIT );
	At( 5000 );
	PausePacket( host, TARGET, true );
	EXPECT_EQ( CMPPlayerDropTestAccess::Start( host.manager, TARGET ), 1000u );
	At( 10000 );
	PausePacket( host, TARGET, false );
	Run( host );
	EXPECT_EQ( CMPPlayerDropTestAccess::Laggers( host.manager ), TARGET_BIT );
	EXPECT_TRUE( CMPPlayerDropTestAccess::WindowShown( host.manager ) );
	At( 12000 );
	ReceiveCommands( host, TARGET );
	Run( host );
	EXPECT_EQ( CMPPlayerDropTestAccess::Laggers( host.manager ), 0u );
	EXPECT_EQ( CMPPlayerDropTestAccess::Budget( host.manager, TARGET ), TOTAL_BUDGET - 11000 );
	EXPECT_FALSE( CMPPlayerDropTestAccess::WindowShown( host.manager ) );
}

TEST_F( MPPause, OverlappingExplicitPausesKeepIndependentBudgetsAndWindow )
{
	TogglePause( host );
	At( 1000 );
	PausePacket( host, TARGET, true );
	At( 3000 );
	PausePacket( host, TARGET, false );
	EXPECT_EQ( CMPPlayerDropTestAccess::Budget( host.manager, TARGET ), TOTAL_BUDGET - 2000 );
	EXPECT_EQ( CMPPlayerDropTestAccess::Laggers( host.manager ), 1u << HOST );
	EXPECT_TRUE( CMPPlayerDropTestAccess::LocalWait( host.transceiver ) );
	EXPECT_TRUE( CMPPlayerDropTestAccess::WindowShown( host.manager ) );
	At( 5000 );
	TogglePause( host );
	EXPECT_EQ( CMPPlayerDropTestAccess::Budget( host.manager, HOST ), TOTAL_BUDGET - 5000 );
	EXPECT_FALSE( CMPPlayerDropTestAccess::WindowShown( host.manager ) );
	EXPECT_FALSE( CMPPlayerDropTestAccess::LocalWait( host.transceiver ) );
}

TEST_F( MPPause, OwnerResumePublishesBudgetAndConvergesAfterDelayedPauseReceipt )
{
	TogglePause( host );
	At( 2000 );
	DeliverSent( host, observer );
	At( 10000 );
	TogglePause( host );
	auto ownerBudget = LastPacket<CB2LagTimeUpdatePacket>( host );
	ASSERT_NE( ownerBudget, nullptr );
	EXPECT_EQ( ownerBudget->nTimeLeft, TOTAL_BUDGET - 10000 );
	DeliverSent( host, observer );
	DeliverSent( observer, host );
	EXPECT_EQ( CMPPlayerDropTestAccess::Budget( host.manager, HOST ), TOTAL_BUDGET - 10000 );
	EXPECT_EQ( CMPPlayerDropTestAccess::Budget( observer.manager, HOST ), TOTAL_BUDGET - 10000 );
	EXPECT_EQ( CMPPlayerDropTestAccess::Laggers( observer.manager ), 0u );
}

TEST_F( MPPause, BudgetArrivingBeforeResumeDoesNotChargeThePauseTwice )
{
	TogglePause( host );
	DeliverSent( host, observer );
	At( 10000 );
	TogglePause( host );
	auto budget = LastPacket<CB2LagTimeUpdatePacket>( host );
	auto resume = LastPacket<CB2UserPausePacket>( host );
	ASSERT_NE( budget, nullptr );
	ASSERT_NE( resume, nullptr );
	budget->nClientID = CLIENT_IDS[HOST];
	resume->nClientID = CLIENT_IDS[HOST];
	Deliver( observer, budget );
	Deliver( observer, budget );
	Deliver( observer, resume );
	Deliver( observer, budget );
	EXPECT_EQ( CMPPlayerDropTestAccess::Budget( observer.manager, HOST ), TOTAL_BUDGET - 10000 );
	EXPECT_EQ( CMPPlayerDropTestAccess::Laggers( observer.manager ), 0u );
}

TEST_F( MPPause, DuplicatePauseAndResumeDoNotRestartOrRechargeTheTimer )
{
	At( 1000 );
	PausePacket( host, TARGET, true );
	At( 3000 );
	PausePacket( host, TARGET, true );
	EXPECT_EQ( CMPPlayerDropTestAccess::Start( host.manager, TARGET ), 1000u );
	At( 6000 );
	PausePacket( host, TARGET, false );
	EXPECT_EQ( CMPPlayerDropTestAccess::Budget( host.manager, TARGET ), TOTAL_BUDGET - 5000 );
	At( 9000 );
	PausePacket( host, TARGET, false );
	EXPECT_EQ( CMPPlayerDropTestAccess::Budget( host.manager, TARGET ), TOTAL_BUDGET - 5000 );
}

TEST_F( MPPause, ExpiryDisplaysOnlyPauseAllowanceAndPreservesNetworkReserve )
{
	TogglePause( host );
	Run( host );
	auto initial = LastLagInfo( host );
	ASSERT_NE( initial, nullptr );
	EXPECT_TRUE( initial->bOwnLag );
	EXPECT_EQ( initial->nOwnTimeLeft, 120 );
	At( 119000 );
	Run( host );
	auto finalSecond = LastLagInfo( host );
	ASSERT_NE( finalSecond, nullptr );
	EXPECT_EQ( finalSecond->nOwnTimeLeft, 1 );
	At( 120000 );
	Run( host );
	EXPECT_EQ( CMPPlayerDropTestAccess::Budget( host.manager, HOST ), 30000 );
	EXPECT_EQ( CMPPlayerDropTestAccess::Paused( host.manager ), 0u );
	EXPECT_FALSE( CMPPlayerDropTestAccess::LocalWait( host.transceiver ) );
	EXPECT_FALSE( timer->HasPause( PAUSE_TYPE_USER_PAUSE ) );
	auto resume = LastPacket<CB2UserPausePacket>( host );
	ASSERT_NE( resume, nullptr );
	EXPECT_FALSE( resume->bPaused );
	TogglePause( host );
	EXPECT_EQ( CMPPlayerDropTestAccess::Paused( host.manager ), 0u );
}

TEST_F( MPPause, ActualRecoveredCommandsClearStaleVotesWhileBothObserversArePaused )
{
	StartBothPausedWithMissingTarget();
	At( 2000 );
	ReceiveCommands( host, TARGET );
	ReceiveCommands( observer, TARGET );
	ASSERT_EQ( host.transceiver->GetCurrentMissingPlayers(), 0u );
	ASSERT_EQ( observer.transceiver->GetCurrentMissingPlayers(), 0u );
	ITransceiver *pHostSimulation = host.transceiver;
	ITransceiver *pObserverSimulation = observer.transceiver;
	pHostSimulation->DoSegments();
	pObserverSimulation->DoSegments();
	// The manager must observe recovery even when the paused simulation emits
	// no new lag report. This also rules out the recording UI hiding a report.
	ASSERT_EQ( ui->nLagReports, 0 );
	At( 10000 );
	Run( observer );
	Run( host );
	EXPECT_EQ( LastPacket<CB2SuggestKickPacket>( observer ), nullptr );
	EXPECT_EQ( LastPacket<CB2SuggestKickPacket>( host ), nullptr );
	CPtr<CNetPacket> oldVote = new CB2SuggestKickPacket( CLIENT_IDS[OBSERVER], TARGET );
	host.client->received.push_back( oldVote );
	Run( host );
	EXPECT_EQ( CMPPlayerDropTestAccess::Votes( host.manager, TARGET ), 0u );
	EXPECT_EQ( CMPPlayerDropTestAccess::Removal( host.transceiver, TARGET ), -1 );
	EXPECT_FALSE( host.transceiver->IsRecoveringDeparture() );
	EXPECT_EQ( CMPPlayerDropTestAccess::Laggers( host.manager ) & TARGET_BIT, 0u );
	EXPECT_TRUE( CMPPlayerDropTestAccess::LocalWait( host.transceiver ) );
	EXPECT_TRUE( CMPPlayerDropTestAccess::LocalWait( observer.transceiver ) );
}

TEST_F( MPPause, GenuineMissingInputStartsRecoveryWhileHostIsPaused )
{
	for ( SPeer *pPeer : { &host, &observer } )
	{
		CMPPlayerDropTestAccess::RequirePacket( pPeer->transceiver, TARGET );
		CMPPlayerDropTestAccess::SetBudget( pPeer->manager, TARGET, 10000 );
		ReportLag( *pPeer, TARGET_BIT );
	}
	TogglePause( host );
	PausePacket( observer, HOST, true );
	At( 10000 );
	Run( observer );
	ASSERT_NE( LastPacket<CB2SuggestKickPacket>( observer ), nullptr );
	DeliverSent( observer, host );
	// Timeout consensus must enter recovery even while manually paused, but it
	// cannot guess the departed player's cutoff before the other survivor replies.
	EXPECT_EQ( CMPPlayerDropTestAccess::Removal( host.transceiver, TARGET ), -1 );
	EXPECT_TRUE( host.transceiver->IsRecoveringDeparture() );
	EXPECT_TRUE( timer->HasPause( PAUSE_TYPE_MP_DEPARTURE_RECOVERY ) );
	CPtr<CB2DepartureRecoveryPacket> report = LastPacket<CB2DepartureRecoveryPacket>( host );
	ASSERT_NE( report, nullptr );
	EXPECT_EQ( report->ePhase, CB2DepartureRecoveryPacket::SNAPSHOT );
	EXPECT_EQ( report->dwDeparted, TARGET_BIT );
	EXPECT_EQ( report->nCommonSegment, 7780 );
	ASSERT_EQ( report->cutoffs.size(), 3u );
	EXPECT_EQ( report->cutoffs[TARGET], -1 );
	EXPECT_TRUE( CMPPlayerDropTestAccess::LocalWait( host.transceiver ) );
	EXPECT_TRUE( CMPPlayerDropTestAccess::WindowShown( host.manager ) );
	// Resume releases only the user's pause; recovery still owns simulation.
	TogglePause( host );
	EXPECT_FALSE( CMPPlayerDropTestAccess::LocalWait( host.transceiver ) );
	EXPECT_TRUE( host.transceiver->IsRecoveringDeparture() );
	EXPECT_TRUE( timer->HasPause( PAUSE_TYPE_MP_DEPARTURE_RECOVERY ) );
}

TEST_F( MPPause, ReceiptInTheSamePacketBatchInvalidatesAnEarlierQueuedKickVote )
{
	CMPPlayerDropTestAccess::RequirePacket( host.transceiver, TARGET );
	CMPPlayerDropTestAccess::SetBudget( host.manager, TARGET, 10000 );
	ReportLag( host, TARGET_BIT );
	TogglePause( host );
	At( 10000 );
	CPtr<CNetPacket> vote = new CB2SuggestKickPacket( CLIENT_IDS[OBSERVER], TARGET );
	CPtr<CNetPacket> receipt = dynamic_cast<CNetPacket *>( NObjectFactory::MakeObject( 111 ) );
	ASSERT_NE( receipt, nullptr );
	SReceivedSegment segment;
	RestoreState( receipt, segment );
	receipt->nClientID = CLIENT_IDS[TARGET];
	// The vote is encountered first, while the manager still observes a lag.
	// The later receipt in this same frame must invalidate it before consensus.
	host.client->received.push_back( vote );
	host.client->received.push_back( receipt );
	Run( host );
	EXPECT_EQ( CMPPlayerDropTestAccess::LastReceived( host.transceiver, TARGET ), 7778 );
	EXPECT_EQ( host.transceiver->GetCurrentMissingPlayers(), 0u );
	EXPECT_EQ( CMPPlayerDropTestAccess::Laggers( host.manager ) & TARGET_BIT, 0u );
	EXPECT_EQ( CMPPlayerDropTestAccess::Votes( host.manager, TARGET ), 0u );
	EXPECT_EQ( LastPacket<CB2SuggestKickPacket>( host ), nullptr );
	EXPECT_EQ( CMPPlayerDropTestAccess::Removal( host.transceiver, TARGET ), -1 );
	EXPECT_FALSE( host.transceiver->IsRecoveringDeparture() );
}

TEST_F( MPPause, ScheduledDepartureCannotReintroducePauseOrConsumeBudget )
{
	CObj<CNetPacket> drop = new CB2DropPlayerAtSegmentPacket( CLIENT_IDS[HOST], TARGET, 7780 );
	Deliver( host, drop );
	ASSERT_EQ( CMPPlayerDropTestAccess::Removal( host.transceiver, TARGET ), 7782 );
	PausePacket( host, TARGET, true );
	ReportLag( host, TARGET_BIT );
	At( 1000 );
	Run( host );
	PausePacket( host, TARGET, false );
	EXPECT_EQ( CMPPlayerDropTestAccess::Paused( host.manager ), 0u );
	EXPECT_EQ( CMPPlayerDropTestAccess::Laggers( host.manager ), 0u );
	EXPECT_EQ( CMPPlayerDropTestAccess::Budget( host.manager, TARGET ), TOTAL_BUDGET );
	EXPECT_FALSE( CMPPlayerDropTestAccess::WindowShown( host.manager ) );
}

TEST_F( MPPause, PauseBudgetUsesUnsignedElapsedTimeAcrossClockWrap )
{
	STimerState state;
	state.absolute.timeCurrent = 0xfffffe00u;
	RestoreState( timer, state );
	timer->Update( 1000 );
	const NTimer::STime start = timer->GetAbsTime();
	PausePacket( host, TARGET, true );
	timer->Update( 2000 );
	ASSERT_EQ( timer->GetAbsTime() - start, 1000u );
	ASSERT_LT( timer->GetAbsTime(), start );
	PausePacket( host, TARGET, false );
	EXPECT_EQ( CMPPlayerDropTestAccess::Budget( host.manager, TARGET ), TOTAL_BUDGET - 1000 );
}
} // namespace
