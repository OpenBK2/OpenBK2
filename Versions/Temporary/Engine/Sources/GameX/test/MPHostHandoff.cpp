#include "../stdafx.h"

#include "../AICmdsAutoMagic.h"
#include "../CommandsHistory.h"
#include "../DBMPConsts.h"
#include "../MPManagerModeNivalNet.hpp"
#include "../MPTransceiver.h"
#include "../ScenarioTracker.h"
#include "AILogic/B2AI.h"
#include "Client/ServerClientInterface.h"
#include "Main/GameTimer.h"
#include "Server_Client_Common/CommonPackets.h"
#include "Server_Client_Common/GamePackets.h"
#include "System/ConsoleBuffer.h"

#include <deque>

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

	static void
	ConfigureLagVoting( CMPManagerModeNivalNet *pManager, CMPTransceiver *pTransceiver, uint32_t dwParticipants = 0x1c )
	{
		pManager->nHostClientID = REPLACEMENT_HOST;
		pManager->bHost = pManager->nOwnSlot == 2;
		pManager->pMPConsts = dynamic_cast<NDb::SMultiplayerConsts *>( NObjectFactory::MakeObject( 0x191B2300 ) );
		pManager->timeNextGameHeartbeat = 0x7fffffff;
		pManager->gamesUpdate.bUpdating = false;
		pManager->updateChannels.bUpdating = false;
		pManager->lagsUpdate.bUpdating = false;
		pManager->bOutcomeKnown = true;
		pManager->dwLaggers = 1UL << 3;
		pManager->dwLaggersOld = pManager->dwLaggers;
		pManager->lags[3].nLagLeft = 0;
		pManager->lags[3].timeStartLag = GameTimer()->GetAbsTime();
		for ( int nSlot = 0; nSlot < pManager->slots.size(); ++nSlot )
		{
			pManager->slots[nSlot].bPresent = ( dwParticipants & ( 1UL << nSlot ) ) != 0;
		}
		pTransceiver->wMask = dwParticipants;
		pTransceiver->wWaitMask = dwParticipants;
	}

	static void SetObservedLaggers( CMPManagerMode *pManager, uint32_t dwPlayers )
	{
		pManager->dwLaggers = dwPlayers;
	}

	static void SetLagBudget( CMPManagerMode *pManager, int nSlot, int nMilliseconds )
	{
		pManager->lags[nSlot].nLagLeft = nMilliseconds;
	}

	static uint32_t GetVotes( const CMPManagerMode *pManager, int nSlot = 3 )
	{
		return pManager->lags[nSlot].dwHatedBy;
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

	void DispatchVote( int nSender )
	{
		CObj<CNetPacket> packet = new CB2SuggestKickPacket( nSender, 3 );
		CPacketProcessorBase *pProcessor = manager;
		ASSERT_TRUE( pProcessor->ProcessPacket( packet ) );
	}

	void StepManager()
	{
		CPacketProcessorBase *pProcessor = manager;
		ASSERT_TRUE( pProcessor->Segment() );
	}

	static int CountVotes( const CRecordingServerClient *pClient )
	{
		int nCount = 0;
		for ( const auto &packet : pClient->sent )
		{
			if ( dynamic_cast<const CB2SuggestKickPacket *>( packet.GetPtr() ) )
			{
				++nCount;
			}
		}
		return nCount;
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

TEST_F( MPHostHandoff, DepartedVoterDoesNotBlockAnotherPlayersTimeout )
{
	CMPPlayerDropTestAccess::ConfigureLagVoting( manager, transceiver, 0x3d );
	for ( int nVoter : { DEPARTING_HOST, REPLACEMENT_HOST, 266043, THIRD_PLAYER } )
	{
		DispatchVote( nVoter );
	}
	ASSERT_EQ( CMPPlayerDropTestAccess::GetVotes( manager ), 0x35u );
	DispatchDrop( REPLACEMENT_HOST, 7780, 0 );
	StepManager();
	EXPECT_EQ( CMPPlayerDropTestAccess::GetRemoval( transceiver, 3 ), 7782 );
}

TEST_F( MPHostHandoff, LaggingVotersExtraApprovalDoesNotVetoEligibleVoters )
{
	CMPPlayerDropTestAccess::ConfigureLagVoting( manager, transceiver, 0x3d );
	CMPPlayerDropTestAccess::SetObservedLaggers( manager, 0x09 );
	CMPPlayerDropTestAccess::SetLagBudget( manager, 0, 10000 );
	for ( int nVoter : { DEPARTING_HOST, REPLACEMENT_HOST, 266043, THIRD_PLAYER } )
	{
		DispatchVote( nVoter );
	}
	StepManager();
	EXPECT_EQ( CMPPlayerDropTestAccess::GetRemoval( transceiver, 0 ), -1 );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetRemoval( transceiver, 3 ), 7782 );
}

TEST_F( MPHostHandoff, ContinuousObserverRenewsVoteAfterHostsLocalRecovery )
{
	CMPPlayerDropTestAccess::ConfigureLagVoting( manager, transceiver );
	CObj<CRecordingServerClient> peerClient = new CRecordingServerClient;
	CObj<CMPManagerModeNivalNet> peerManager =
			dynamic_cast<CMPManagerModeNivalNet *>( NObjectFactory::MakeObject( 0x192444C0 ) );
	CObj<CMPTransceiver> peerTransceiver = dynamic_cast<CMPTransceiver *>( NObjectFactory::MakeObject( 0x1713C440 ) );
	CMPPlayerDropTestAccess::Initialize( peerManager, peerTransceiver, peerClient, history, ai, timer, 4 );
	CMPPlayerDropTestAccess::ConfigureLagVoting( peerManager, peerTransceiver );
	CPacketProcessorBase *pHost = manager;
	CPacketProcessorBase *pPeer = peerManager;

	ASSERT_TRUE( pPeer->Segment() );
	ASSERT_EQ( CountVotes( peerClient ), 1 );
	CPtr<CNetPacket> firstVote = peerClient->sent.back();
	firstVote->nClientID = 266043;
	ASSERT_TRUE( pHost->ProcessPacket( firstVote ) );
	ASSERT_EQ( CMPPlayerDropTestAccess::GetVotes( manager ), 0x10u );

	// Only the host briefly receives the target. The other observer remains
	// blocked and must renew its approval without seeing its own recovery.
	CMPPlayerDropTestAccess::SetObservedLaggers( manager, 0 );
	StepManager();
	ASSERT_EQ( CMPPlayerDropTestAccess::GetVotes( manager ), 0u );
	CMPPlayerDropTestAccess::SetObservedLaggers( manager, 0x08 );
	StepManager();
	StepManager();
	EXPECT_EQ( CMPPlayerDropTestAccess::GetRemoval( transceiver, 3 ), -1 );

	timer->Update( 1999 );
	ASSERT_EQ( timer->GetAbsTime(), 999u );
	for ( int nFrame = 0; nFrame < 20; ++nFrame )
	{
		ASSERT_TRUE( pPeer->Segment() );
	}
	EXPECT_EQ( CountVotes( peerClient ), 1 );
	timer->Update( 2000 );
	ASSERT_TRUE( pPeer->Segment() );
	ASSERT_EQ( CountVotes( peerClient ), 2 );
	CPtr<CNetPacket> renewedVote = peerClient->sent.back();
	ASSERT_NE( renewedVote.GetPtr(), firstVote.GetPtr() );
	renewedVote->nClientID = 266043;
	client->received.push_back( renewedVote );
	// The ordinary manager loop drains this newly generated packet before its
	// timeout decision. No manual re-delivery of the original vote is needed.
	StepManager();
	EXPECT_EQ( CMPPlayerDropTestAccess::GetRemoval( transceiver, 3 ), 7782 );
}

TEST_F( MPHostHandoff, ExpiredApprovalCannotCompleteTimeoutConsensus )
{
	CMPPlayerDropTestAccess::ConfigureLagVoting( manager, transceiver, 0x3c );
	DispatchVote( 266043 );
	StepManager();
	EXPECT_EQ( CMPPlayerDropTestAccess::GetRemoval( transceiver, 3 ), -1 );
	timer->Update( 4001 );
	ASSERT_EQ( timer->GetAbsTime(), 3001u );
	DispatchVote( THIRD_PLAYER );
	StepManager();
	EXPECT_EQ( CMPPlayerDropTestAccess::GetVotes( manager ), 0x24u );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetRemoval( transceiver, 3 ), -1 );
	DispatchVote( 266043 );
	StepManager();
	EXPECT_EQ( CMPPlayerDropTestAccess::GetRemoval( transceiver, 3 ), 7782 );
}

TEST_F( MPHostHandoff, RenewedApprovalRefreshesItsReceiptLifetime )
{
	CMPPlayerDropTestAccess::ConfigureLagVoting( manager, transceiver, 0x3c );
	DispatchVote( 266043 );
	StepManager();
	timer->Update( 3000 );
	DispatchVote( 266043 );
	timer->Update( 5000 );
	DispatchVote( THIRD_PLAYER );
	StepManager();
	EXPECT_EQ( CMPPlayerDropTestAccess::GetRemoval( transceiver, 3 ), 7782 );
}

TEST_F( MPHostHandoff, ZeroBudgetRecoveryWithdrawsTheSendersApproval )
{
	CMPPlayerDropTestAccess::ConfigureLagVoting( manager, transceiver, 0x3c );
	DispatchVote( 266043 );
	CObj<CNetPacket> recovery = new CB2LagTimeUpdatePacket( 266043, 3, 0 );
	CPacketProcessorBase *pProcessor = manager;
	ASSERT_TRUE( pProcessor->ProcessPacket( recovery ) );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetVotes( manager ), 0u );
	DispatchVote( THIRD_PLAYER );
	StepManager();
	EXPECT_EQ( CMPPlayerDropTestAccess::GetRemoval( transceiver, 3 ), -1 );
	DispatchVote( 266043 );
	StepManager();
	EXPECT_EQ( CMPPlayerDropTestAccess::GetRemoval( transceiver, 3 ), 7782 );
}

TEST_F( MPHostHandoff, ApprovalArrivingWhileTargetIsHealthyDoesNotSurviveIntoNextWait )
{
	CMPPlayerDropTestAccess::ConfigureLagVoting( manager, transceiver );
	CMPPlayerDropTestAccess::SetObservedLaggers( manager, 0 );
	StepManager();
	DispatchVote( 266043 );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetVotes( manager ), 0u );
	CMPPlayerDropTestAccess::SetObservedLaggers( manager, 0x08 );
	StepManager();
	StepManager();
	EXPECT_EQ( CMPPlayerDropTestAccess::GetRemoval( transceiver, 3 ), -1 );
}

TEST_F( MPHostHandoff, RecoveringObserverStopsRenewingItsApproval )
{
	CMPPlayerDropTestAccess::Initialize( manager, transceiver, client, history, ai, timer, 4 );
	CMPPlayerDropTestAccess::ConfigureLagVoting( manager, transceiver );
	StepManager();
	ASSERT_EQ( CountVotes( client ), 1 );
	timer->Update( 2000 );
	StepManager();
	ASSERT_EQ( CountVotes( client ), 2 );
	CMPPlayerDropTestAccess::SetObservedLaggers( manager, 0 );
	StepManager();
	ASSERT_NE( dynamic_cast<CB2LagTimeUpdatePacket *>( client->sent.back().GetPtr() ), nullptr );
	timer->Update( 6000 );
	StepManager();
	EXPECT_EQ( CountVotes( client ), 2 );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetVotes( manager ), 0u );
}

TEST_F( MPHostHandoff, ApprovalRetryAndExpiryHandleAbsoluteClockWrap )
{
	STimerState nearWrap;
	nearWrap.absolute.timeCurrent = 0xfffffe00u;
	RestoreState( timer, nearWrap );
	timer->Update( 1000 );
	ASSERT_EQ( timer->GetAbsTime(), 0xfffffe00u );
	CMPPlayerDropTestAccess::Initialize( manager, transceiver, client, history, ai, timer, 4 );
	CMPPlayerDropTestAccess::ConfigureLagVoting( manager, transceiver );
	DispatchVote( REPLACEMENT_HOST );
	StepManager();
	ASSERT_EQ( CountVotes( client ), 1 );
	timer->Update( 1999 );
	StepManager();
	EXPECT_EQ( CountVotes( client ), 1 );
	timer->Update( 2000 );
	StepManager();
	EXPECT_EQ( CountVotes( client ), 2 );
	timer->Update( 4001 );
	StepManager();
	EXPECT_EQ( CMPPlayerDropTestAccess::GetVotes( manager ), 0x10u );
}
} // namespace
