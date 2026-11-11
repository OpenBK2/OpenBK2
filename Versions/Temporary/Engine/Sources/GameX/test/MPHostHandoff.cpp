#include "../stdafx.h"

#include "../AICmdsAutoMagic.h"
#include "../CommandsHistory.h"
#include "../MPManagerModeNivalNet.hpp"
#include "../MPTransceiver.h"
#include "../ScenarioTracker.h"
#include "AILogic/B2AI.h"
#include "Client/ServerClientInterface.h"
#include "Main/GameTimer.h"
#include "Server_Client_Common/CommonPackets.h"
#include "Server_Client_Common/GamePackets.h"
#include "System/ConsoleBuffer.h"

#include <gtest/gtest.h>

namespace
{
constexpr int DEPARTING_HOST = 266084;
constexpr int REPLACEMENT_HOST = 266025;
constexpr int NEXT_HOST = 266012;
constexpr int THIRD_PLAYER = 266060;

class CRecordingServerClient : public IServerClient
{
	OBJECT_NOCOPY_METHODS( CRecordingServerClient );

public:
	std::vector<CPtr<CNetPacket>> sent;

	CNetPacket *GetPacket() override
	{
		return nullptr;
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
	std::vector<CPtr<SMPUIMessage>> messages;

public:
	void AddUIMessage( SMPUIMessage *pMessage ) override
	{
		messages.push_back( pMessage );
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
	std::vector<SPlayerPresence> players = std::vector<SPlayerPresence>( 6 );

	int operator&( IBinSaver &saver )
	{
		saver.Add( 3, &players );
		return 0;
	}
};

struct STimerState
{
	int nSegmentDuration = 200;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 7, &nSegmentDuration );
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

// Only fixture state access belongs here. Packets and simulation removals run
// through the actual DLL objects' virtual entry points, without test-only exports.
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
		const int clientIDs[] = { DEPARTING_HOST, 266028, REPLACEMENT_HOST, NEXT_HOST, 266043, THIRD_PLAYER };
		pManager->eState = CMPManagerModeNivalNet::EGS_GAME_STARTED;
		pManager->nMyClientID = clientIDs[nOwnSlot];
		pManager->bHost = false;
		pManager->pClient = pClient;
		pManager->pTransceiver = pTransceiver;
		pManager->nOwnSlot = nOwnSlot;
		pManager->nHostClientID = DEPARTING_HOST;
		pManager->nSlotsUsed = 6;
		pManager->dwLaggers = 0;
		pManager->dwLaggersOld = 0;
		pManager->dwUserPausedPlayers = 0;
		pManager->dwInitialPlayers = 0x3f;
		pManager->bInitialLoadInProgress = false;
		pManager->bWaitWindowShown = false;
		pManager->bOutcomeKnown = false;
		pManager->bWinOnGameEnd = false;
		pManager->bWinFromPlayerDrops = false;
		pManager->slots.resize( 6 );
		pManager->lags.resize( 6 );
		pManager->playerDrops.resize( 6 );
		pTransceiver->players.resize( 6 );
		pTransceiver->playerRemovals.resize( 6 );
		for ( int i = 0; i < 6; ++i )
		{
			pManager->slots[i].nClientID = clientIDs[i];
			pManager->slots[i].nTeam = ( i == 0 || i == 1 || i == 5 ) ? 0 : 1;
			pManager->slots[i].bPresent = i != 1;
			pManager->slots[i].bAccept = true;
			pTransceiver->players[i].nClientID = clientIDs[i];
			pTransceiver->players[i].nTeam = pManager->slots[i].nTeam;
		}
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
		pTransceiver->bWaiting = true;
		pTransceiver->wMask = 0x3d;
		pTransceiver->wWaitMask = 0x3d;
	}

	static void SetBoundary( CMPTransceiver *pTransceiver, int nSegment, bool bFinish = false )
	{
		pTransceiver->nCommonSegment = nSegment;
		if ( bFinish )
		{
			pTransceiver->nFinalSegment = nSegment;
		}
	}

	static int GetHost( const CMPManagerMode *pManager )
	{
		return pManager->nHostClientID;
	}

	static int GetRemoval( const CMPTransceiver *pTransceiver, int nSlot = 0 )
	{
		return pTransceiver->playerRemovals[nSlot].GetRemovalSegment( pTransceiver->nLatency );
	}

	static void SetPendingVictory( CMPManagerMode *pManager, CMPTransceiver *pTransceiver, bool bFromDrops )
	{
		// Restore a pending outcome, then let the real packet handler reconcile it.
		pManager->dwInitialPlayers = 0x05;
		pManager->bOutcomeKnown = true;
		pManager->bWinOnGameEnd = true;
		pManager->bWinFromPlayerDrops = bFromDrops;
		pTransceiver->nFinalSegment = 7782;
	}

	static int GetFinalSegment( const CMPTransceiver *pTransceiver )
	{
		return pTransceiver->nFinalSegment;
	}
};

namespace
{
class MPHostHandoff : public testing::Test
{
protected:
	CObj<CRecordingServerClient> client;
	CObj<CMPManagerModeNivalNet> manager;
	CObj<CMPTransceiver> transceiver;
	CObj<ICommandsHistory> history;
	CObj<IScenarioTracker> scenario;
	CObj<IAILogic> ai;
	CObj<IGameTimer> timer;
	CObj<IConsoleBuffer> console;
	CObj<CRecordingUI> ui;

	void SetUp() override
	{
		CObj<IAICmdsAutoMagic> registry = CreateAICmdsAutoMagic();
		ASSERT_NE( registry, nullptr );
		manager = dynamic_cast<CMPManagerModeNivalNet *>( NObjectFactory::MakeObject( 0x192444C0 ) );
		transceiver = dynamic_cast<CMPTransceiver *>( NObjectFactory::MakeObject( 0x1713C440 ) );
		history = dynamic_cast<ICommandsHistory *>( NObjectFactory::MakeObject( 0x1007AB80 ) );
		scenario = dynamic_cast<IScenarioTracker *>( NObjectFactory::MakeObject( 0x1117BB00 ) );
		ai = dynamic_cast<IAILogic *>( NObjectFactory::MakeObject( 0x1108D441 ) );
		timer = dynamic_cast<IGameTimer *>( NObjectFactory::MakeObject( 0x10075C05 ) );
		console = dynamic_cast<IConsoleBuffer *>( NObjectFactory::MakeObject( 0x300C8D40 ) );
		ASSERT_NE( manager, nullptr );
		ASSERT_NE( transceiver, nullptr );
		ASSERT_NE( history, nullptr );
		ASSERT_NE( scenario, nullptr );
		ASSERT_NE( ai, nullptr );
		ASSERT_NE( timer, nullptr );
		ASSERT_NE( console, nullptr );
		client = new CRecordingServerClient;
		ui = new CRecordingUI;
		SScenarioPlayers players;
		players.players[1].bPresent = false;
		RestoreState( scenario, players );
		scenario->SetLocalPlayer( 2 );
		STimerState timerState;
		RestoreState( timer, timerState );
		timer->Reset( 1000 );
		NSingleton::RegisterSingleton( scenario, IScenarioTracker::tidTypeID );
		NSingleton::RegisterSingleton( ai, IAILogic::tidTypeID );
		NSingleton::RegisterSingleton( timer, IGameTimer::tidTypeID );
		NSingleton::RegisterSingleton( console, IConsoleBuffer::tidTypeID );
		NSingleton::RegisterSingleton( ui, IMPToUIManager::tidTypeID );
		CMPPlayerDropTestAccess::Initialize( manager, transceiver, client, history, ai, timer, 2 );
	}

	void TearDown() override
	{
		manager = nullptr;
		transceiver = nullptr;
		NSingleton::UnRegisterSingleton( IMPToUIManager::tidTypeID );
		NSingleton::UnRegisterSingleton( IConsoleBuffer::tidTypeID );
		NSingleton::UnRegisterSingleton( IGameTimer::tidTypeID );
		NSingleton::UnRegisterSingleton( IAILogic::tidTypeID );
		NSingleton::UnRegisterSingleton( IScenarioTracker::tidTypeID );
	}

	void DispatchDrop( int nSender, int nSegment, int nSlot = 0 )
	{
		CObj<CNetPacket> packet = new CB2DropPlayerAtSegmentPacket( nSender, nSlot, nSegment );
		CPacketProcessorBase *pProcessor = manager;
		ASSERT_TRUE( pProcessor->ProcessPacket( packet ) );
	}

	void DispatchBackendRemoval()
	{
		CObj<CNetPacket> packet = new CGameClientRemoved( DEPARTING_HOST );
		CPacketProcessorBase *pProcessor = manager;
		ASSERT_TRUE( pProcessor->ProcessPacket( packet ) );
	}

	int CountRelays( int nSegment ) const
	{
		int nCount = 0;
		for ( const auto &packet : client->sent )
		{
			const auto *pRelay = dynamic_cast<const CDirectPacketToClient *>( packet.GetPtr() );
			const auto *pDrop =
					pRelay ? dynamic_cast<const CB2DropPlayerAtSegmentPacket *>( pRelay->pPacket.GetPtr() ) : nullptr;
			if ( pDrop && pDrop->nSegment == nSegment && pDrop->nSlotToDrop == 0 )
			{
				++nCount;
			}
		}
		return nCount;
	}

	void FinishAt( int nSegment )
	{
		// The real loop commits final-boundary removals even with no timer tick due.
		// This exercises removal/history/UI effects without running an unloaded map.
		CMPPlayerDropTestAccess::SetBoundary( transceiver, nSegment, true );
		ITransceiver *pSimulation = transceiver;
		pSimulation->DoSegments();
	}

	void CheckSingleRecordedRemoval()
	{
		const CHistory recorded = history->GetCommandsHistory();
		ASSERT_EQ( recorded.size(), 1 );
		ASSERT_EQ( recorded.count( 7781 ), 1 );
		ASSERT_EQ( recorded.at( 7781 ).size(), 1 );
		ASSERT_TRUE( IsValid( recorded.at( 7781 ).front() ) );
		EXPECT_EQ( recorded.at( 7781 ).front()->GetDebugInfo(), "DropPlayer 0" );
		EXPECT_FALSE( scenario->IsPlayerPresent( 0 ) );
		EXPECT_FALSE( transceiver->HasPlayerDropError() );
	}
};

TEST_F( MPHostHandoff, BackendFirstAcceptsAndRelaysDepartingHostsEarlierBoundary )
{
	DispatchBackendRemoval();
	EXPECT_EQ( CMPPlayerDropTestAccess::GetHost( manager ), REPLACEMENT_HOST );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetRemoval( transceiver ), 7782 );
	EXPECT_EQ( CountRelays( 7780 ), 4 );
	CMPPlayerDropTestAccess::SetBoundary( transceiver, 7781 );
	DispatchDrop( DEPARTING_HOST, 7779 );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetRemoval( transceiver ), 7781 );
	EXPECT_EQ( CountRelays( 7779 ), 4 );
	EXPECT_TRUE( history->GetCommandsHistory().empty() );
	// Neither duplicates nor the successor's superseded cutoff can relay again.
	DispatchDrop( DEPARTING_HOST, 7779 );
	DispatchDrop( REPLACEMENT_HOST, 7780 );
	EXPECT_EQ( CountRelays( 7779 ), 4 );
	EXPECT_EQ( CountRelays( 7780 ), 4 );
	FinishAt( 7781 );
	CheckSingleRecordedRemoval();
}

TEST_F( MPHostHandoff, AnnouncementFirstRetainsTheSameBoundaryAfterBackendRemoval )
{
	DispatchDrop( DEPARTING_HOST, 7779 );
	DispatchBackendRemoval();
	EXPECT_EQ( CMPPlayerDropTestAccess::GetHost( manager ), REPLACEMENT_HOST );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetRemoval( transceiver ), 7781 );
	EXPECT_EQ( CountRelays( 7779 ), 4 );
	EXPECT_EQ( CountRelays( 7780 ), 0 );
	FinishAt( 7781 );
	CheckSingleRecordedRemoval();
}

TEST_F( MPHostHandoff, ThirdPeerAcceptsSuccessorsCorrectionWithoutTheOriginalAnnouncement )
{
	CMPPlayerDropTestAccess::Initialize( manager, transceiver, client, history, ai, timer, 5 );
	DispatchDrop( REPLACEMENT_HOST, 7780 );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetHost( manager ), REPLACEMENT_HOST );
	CMPPlayerDropTestAccess::SetBoundary( transceiver, 7781 );
	DispatchDrop( REPLACEMENT_HOST, 7779 );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetRemoval( transceiver ), 7781 );
	EXPECT_TRUE( client->sent.empty() );
	FinishAt( 7781 );
	CheckSingleRecordedRemoval();
}

TEST_F( MPHostHandoff, RetiredSuccessorsCorrectionStillReachesTheNextHost )
{
	CMPPlayerDropTestAccess::Initialize( manager, transceiver, client, history, ai, timer, 3 );
	DispatchDrop( REPLACEMENT_HOST, 7780 );
	ASSERT_EQ( CMPPlayerDropTestAccess::GetHost( manager ), REPLACEMENT_HOST );
	ASSERT_EQ( CMPPlayerDropTestAccess::GetRemoval( transceiver ), 7782 );
	CMPPlayerDropTestAccess::SetBoundary( transceiver, 7781 );
	// The successor's own departure overtakes its correction of the first host.
	// The next host must still accept and relay that already-known departure.
	DispatchDrop( REPLACEMENT_HOST, 7784, 2 );
	ASSERT_EQ( CMPPlayerDropTestAccess::GetHost( manager ), NEXT_HOST );
	DispatchDrop( REPLACEMENT_HOST, 7779 );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetRemoval( transceiver ), 7781 );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetRemoval( transceiver, 2 ), 7786 );
	EXPECT_EQ( CountRelays( 7779 ), 4 );
	FinishAt( 7781 );
	CheckSingleRecordedRemoval();
}

TEST_F( MPHostHandoff, SuccessorsCorrectionBeforeItsDepartureRecordsTheSameBoundary )
{
	CMPPlayerDropTestAccess::Initialize( manager, transceiver, client, history, ai, timer, 3 );
	DispatchDrop( REPLACEMENT_HOST, 7780 );
	ASSERT_EQ( CMPPlayerDropTestAccess::GetHost( manager ), REPLACEMENT_HOST );
	CMPPlayerDropTestAccess::SetBoundary( transceiver, 7781 );
	DispatchDrop( REPLACEMENT_HOST, 7779 );
	DispatchDrop( REPLACEMENT_HOST, 7784, 2 );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetHost( manager ), NEXT_HOST );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetRemoval( transceiver ), 7781 );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetRemoval( transceiver, 2 ), 7786 );
	FinishAt( 7781 );
	CheckSingleRecordedRemoval();
}

TEST_F( MPHostHandoff, EarlierLastOpponentDropCorrectsThePendingVictoryBoundary )
{
	DispatchBackendRemoval();
	CMPPlayerDropTestAccess::SetPendingVictory( manager, transceiver, true );
	CMPPlayerDropTestAccess::SetBoundary( transceiver, 7781 );
	DispatchDrop( DEPARTING_HOST, 7779 );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetFinalSegment( transceiver ), 7781 );
	ITransceiver *pSimulation = transceiver;
	pSimulation->DoSegments();
	CheckSingleRecordedRemoval();
}

TEST_F( MPHostHandoff, DropCorrectionDoesNotRescheduleAnUnrelatedVictory )
{
	DispatchBackendRemoval();
	CMPPlayerDropTestAccess::SetPendingVictory( manager, transceiver, false );
	CMPPlayerDropTestAccess::SetBoundary( transceiver, 7781 );
	DispatchDrop( DEPARTING_HOST, 7779 );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetFinalSegment( transceiver ), 7782 );
}

TEST_F( MPHostHandoff, SimultaneousDeparturesAreRecordedInSlotOrder )
{
	// Arrival order is intentionally reversed; neither packet may mutate the
	// simulation before the loop sees all departures for this boundary.
	CMPPlayerDropTestAccess::SetBoundary( transceiver, 7781 );
	DispatchDrop( DEPARTING_HOST, 7779, 5 );
	DispatchDrop( DEPARTING_HOST, 7779, 0 );
	EXPECT_TRUE( history->GetCommandsHistory().empty() );
	EXPECT_TRUE( scenario->IsPlayerPresent( 0 ) );
	EXPECT_TRUE( scenario->IsPlayerPresent( 5 ) );
	FinishAt( 7781 );
	const CHistory recorded = history->GetCommandsHistory();
	ASSERT_EQ( recorded.size(), 1 );
	ASSERT_EQ( recorded.count( 7781 ), 1 );
	ASSERT_EQ( recorded.at( 7781 ).size(), 2 );
	auto command = recorded.at( 7781 ).begin();
	ASSERT_TRUE( IsValid( *command ) );
	EXPECT_EQ( ( *command )->GetDebugInfo(), "DropPlayer 0" );
	++command;
	ASSERT_TRUE( IsValid( *command ) );
	EXPECT_EQ( ( *command )->GetDebugInfo(), "DropPlayer 5" );
	EXPECT_FALSE( scenario->IsPlayerPresent( 0 ) );
	EXPECT_FALSE( scenario->IsPlayerPresent( 5 ) );
}
} // namespace
