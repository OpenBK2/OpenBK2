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
#include "System/ConsoleBuffer.h"

#include <deque>

#include <gtest/gtest.h>

namespace
{
constexpr int HOST = 0;
constexpr int LEAVER = 4;
constexpr int CLIENT_IDS[] = { 271780, 271786, 271789, 271793, 271792, 271813 };
constexpr uint16_t ALL_PLAYERS = 0x3f;
constexpr int LAST_PACKET = 14667;
constexpr int DROP_CUTOFF = 14669;
constexpr int REMOVAL_SEGMENT = 14671;
std::vector<int> executedCommands;

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
		executedCommands.push_back( nValue );
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

public:
	void StartNewGame( const NDb::SMapInfo * ) override {}

	const NDb::SMapInfo *GetMap() const override
	{
		return nullptr;
	}

	void AddCommand( const int nSegment, IAILogicCommandB2 *pCommand ) override
	{
		recorded[nSegment].push_back( pCommand );
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

	CHistory GetCommandsHistory() const override
	{
		return recorded;
	}
};

class CRecordingClient : public IServerClient
{
	OBJECT_NOCOPY_METHODS( CRecordingClient );

public:
	std::vector<CPtr<CNetPacket>> direct;
	std::vector<CPtr<CNetPacket>> persistent;
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
		persistent.push_back( pPacket );
	}

	void SendGamePacket( CNetPacket *pPacket, bool ) override
	{
		direct.push_back( pPacket );
	}

	void Segment() override {}
};

class CRecordingUI : public IMPToUIManager
{
	OBJECT_NOCOPY_METHODS( CRecordingUI );

public:
	void AddUIMessage( SMPUIMessage *pMessage ) override
	{
		CPtr<SMPUIMessage> retained = pMessage;
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

class CLeaveMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS( CLeaveMessage );

public:
	CLeaveMessage()
		: SMPUIMessage( EMUI_LEAVE_GAME )
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
} // namespace

// Lobby and receipt fixtures do not manufacture departures or cache entries.
// Those are produced by actual DoSegments, UI surrender and packet dispatch.
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
		pManager->bHost = nOwnSlot == HOST;
		pManager->bLadderGame = false;
		pManager->pClient = pClient;
		pManager->pTransceiver = pTransceiver;
		pManager->nOwnSlot = nOwnSlot;
		pManager->nHostClientID = CLIENT_IDS[HOST];
		pManager->nGameID = 1219;
		pManager->nSlotsUsed = 6;
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
		pManager->slots.resize( 6 );
		pManager->lags.resize( 6 );
		pManager->playerDrops.resize( 6 );
		pTransceiver->players.resize( 6 );
		pTransceiver->playerRemovals.resize( 6 );
		for ( int i = 0; i < 6; ++i )
		{
			pManager->slots[i].nClientID = CLIENT_IDS[i];
			pManager->slots[i].nTeam = i == 0 || i == 1 || i == 4 ? 0 : 1;
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
		pTransceiver->nFinalSegment = -1;
		pTransceiver->bIsGameRunning = true;
		pTransceiver->bWaiting = false;
		pTransceiver->wMask = ALL_PLAYERS;
		pTransceiver->wWaitMask = ALL_PLAYERS;
		CPtr<NDb::SNetGameConsts> consts = dynamic_cast<NDb::SNetGameConsts *>( NObjectFactory::MakeObject( 0x300A7B40 ) );
		consts->nMaxLatency = 16;
		pTransceiver->pConsts = consts;
		pTransceiver->segmFinished.assign( 16, ALL_PLAYERS );
		pTransceiver->checkSums.SetSizes( 16, 6 );
		pTransceiver->checkSums.FillZero();
		pTransceiver->cmds.SetSizes( 6, 16 );
		SetBoundary( pTransceiver, 0 );
	}

	static void SetBoundary( CMPTransceiver *pTransceiver, int nSegment )
	{
		pTransceiver->nCommonSegment = nSegment;
		pTransceiver->nSegment = nSegment % 16;
	}

	static void SetReceiptMask( CMPTransceiver *pTransceiver, uint16_t wMask )
	{
		pTransceiver->segmFinished.assign( 16, wMask );
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
};

namespace
{
struct SPeer
{
	int nSlot = -1;
	CObj<CRecordingClient> client;
	CObj<CRecordingHistory> history;
	CObj<CMPManagerModeNivalNet> manager;
	CObj<CMPTransceiver> transceiver;
};

class MPGracefulLeave : public testing::Test
{
protected:
	SPeer host;
	SPeer leaver;
	CObj<CTailCommandPacker> packer;
	CObj<CMPHeadlessAI> ai;
	CObj<IScenarioTracker> scenario;
	CObj<IGameTimer> timer;
	CObj<IConsoleBuffer> console;
	CObj<CRecordingUI> ui;

	void InitializePeer( SPeer &peer, int nSlot )
	{
		peer.nSlot = nSlot;
		peer.client = new CRecordingClient;
		peer.history = new CRecordingHistory;
		peer.manager = dynamic_cast<CMPManagerModeNivalNet *>( NObjectFactory::MakeObject( 0x192444C0 ) );
		peer.transceiver = dynamic_cast<CMPTransceiver *>( NObjectFactory::MakeObject( 0x1713C440 ) );
		ASSERT_NE( peer.manager, nullptr );
		ASSERT_NE( peer.transceiver, nullptr );
		CMPPlayerDropTestAccess::Initialize(
				peer.manager, peer.transceiver, peer.client, peer.history, packer, ai, timer, nSlot );
	}

	void SetUp() override
	{
		CObj<IAICmdsAutoMagic> registration = CreateAICmdsAutoMagic();
		ASSERT_NE( registration, nullptr );
		packer = new CTailCommandPacker;
		ai = new CMPHeadlessAI;
		scenario = dynamic_cast<IScenarioTracker *>( NObjectFactory::MakeObject( 0x1117BB00 ) );
		timer = dynamic_cast<IGameTimer *>( NObjectFactory::MakeObject( 0x10075C05 ) );
		console = dynamic_cast<IConsoleBuffer *>( NObjectFactory::MakeObject( 0x300C8D40 ) );
		ASSERT_NE( scenario, nullptr );
		ASSERT_NE( timer, nullptr );
		ASSERT_NE( console, nullptr );
		ui = new CRecordingUI;
		SScenarioPlayers players;
		RestoreState( scenario, players );
		scenario->SetLocalPlayer( HOST );
		STimerState timerState;
		RestoreState( timer, timerState );
		timer->Reset( 1000 );
		timer->SetSpeed( 0 );
		NSingleton::RegisterSingleton( scenario, IScenarioTracker::tidTypeID );
		NSingleton::RegisterSingleton( ai, IAILogic::tidTypeID );
		NSingleton::RegisterSingleton( timer, IGameTimer::tidTypeID );
		NSingleton::RegisterSingleton( console, IConsoleBuffer::tidTypeID );
		NSingleton::RegisterSingleton( ui, IMPToUIManager::tidTypeID );
		InitializePeer( host, HOST );
		InitializePeer( leaver, LEAVER );
		executedCommands.clear();
	}

	void TearDown() override
	{
		host.manager = nullptr;
		host.transceiver = nullptr;
		leaver.manager = nullptr;
		leaver.transceiver = nullptr;
		NSingleton::UnRegisterSingleton( IMPToUIManager::tidTypeID );
		NSingleton::UnRegisterSingleton( IConsoleBuffer::tidTypeID );
		NSingleton::UnRegisterSingleton( IGameTimer::tidTypeID );
		NSingleton::UnRegisterSingleton( IAILogic::tidTypeID );
		NSingleton::UnRegisterSingleton( IScenarioTracker::tidTypeID );
	}

	void StepSimulation( SPeer &peer )
	{
		// Give exactly one tick to this simulated peer. No wall-clock sleeps or
		// map simulation are involved, but packet and removal dispatch are real.
		timer->Reset( 1000 );
		timer->Update( 1200 );
		ITransceiver *pSimulation = peer.transceiver;
		pSimulation->DoSegments();
	}

	void ProduceTail( SPeer &peer, int nFirst = LAST_PACKET - 27, bool bFinalCommand = true )
	{
		CMPPlayerDropTestAccess::SetBoundary( peer.transceiver, nFirst );
		for ( int nPacket = nFirst; nPacket <= LAST_PACKET; ++nPacket )
		{
			CMPPlayerDropTestAccess::SetReceiptMask( peer.transceiver, ALL_PLAYERS );
			if ( bFinalCommand && nPacket == LAST_PACKET )
			{
				CPtr<CTailCommand> command = new CTailCommand;
				command->nValue = 42;
				ITransceiver *pSimulation = peer.transceiver;
				pSimulation->SendCommand( command );
			}
			StepSimulation( peer );
			ASSERT_EQ( peer.transceiver->GetCurrentCommonSegment(), nPacket + 1 );
		}
	}

	static void Surrender( SPeer &peer )
	{
		CObj<SMPUIMessage> message = new CLeaveMessage;
		CMPUIMessageTranslator *pTranslator = peer.manager;
		ASSERT_TRUE( pTranslator->HandleMessage( message ) );
	}

	static void RunManager( SPeer &peer )
	{
		CPacketProcessorBase *pProcessor = peer.manager;
		ASSERT_TRUE( pProcessor->Segment() );
	}

	static void Deliver( SPeer &peer, CNetPacket *pPacket )
	{
		CPacketProcessorBase *pProcessor = peer.manager;
		ASSERT_TRUE( pProcessor->ProcessPacket( pPacket ) );
	}

	static void DeliverCommands( SPeer &peer, CNetPacket *pPacket, int nSenderSlot )
	{
		pPacket->nClientID = CLIENT_IDS[nSenderSlot];
		CPacketProcessorBase *pProcessor = peer.transceiver;
		ASSERT_TRUE( pProcessor->ProcessPacket( pPacket ) );
	}

	static std::vector<CPtr<CNetPacket>> ForwardedTo( const SPeer &sender, int nRecipient )
	{
		std::vector<CPtr<CNetPacket>> packets;
		for ( const auto &packet : sender.client->persistent )
		{
			const auto *pWrapper = dynamic_cast<const CDirectPacketToClient *>( packet.GetPtr() );
			if ( pWrapper && pWrapper->nClient == CLIENT_IDS[nRecipient] )
			{
				packets.push_back( pWrapper->pPacket );
			}
		}
		return packets;
	}

	static void QueueForwarded( const SPeer &sender, SPeer &receiver )
	{
		for ( const auto &packet : ForwardedTo( sender, receiver.nSlot ) )
		{
			// The online server attributes forwarded envelopes to their sender.
			packet->nClientID = CLIENT_IDS[sender.nSlot];
			receiver.client->received.push_back( packet );
		}
	}

	void PrepareHostAtCapturedBoundary()
	{
		CMPPlayerDropTestAccess::SetBoundary( host.transceiver, 14670 );
		CMPPlayerDropTestAccess::SetReceiptMask( host.transceiver, ALL_PLAYERS & ~( 1u << LEAVER ) );
	}

	void VerifyFinalCommandThenRemoval()
	{
		ASSERT_EQ( CMPPlayerDropTestAccess::Removal( host.transceiver, LEAVER ), REMOVAL_SEGMENT );
		StepSimulation( host );
		ASSERT_EQ( host.transceiver->GetCurrentCommonSegment(), REMOVAL_SEGMENT );
		EXPECT_EQ( executedCommands, ( std::vector<int>{ 42 } ) );
		EXPECT_EQ( CMPPlayerDropTestAccess::LastReceived( host.transceiver, LEAVER ), LAST_PACKET );
		StepSimulation( host );
		EXPECT_EQ( host.transceiver->GetCurrentCommonSegment(), REMOVAL_SEGMENT + 1 );
		EXPECT_FALSE( host.transceiver->HasPlayerDropError() );
		EXPECT_FALSE( scenario->IsPlayerPresent( LEAVER ) );
		EXPECT_EQ( ai->neutralized, ( std::vector<int>{ LEAVER } ) );
		const CHistory recorded = host.history->GetCommandsHistory();
		ASSERT_EQ( recorded.count( REMOVAL_SEGMENT - 1 ), 1u );
		ASSERT_EQ( recorded.at( REMOVAL_SEGMENT - 1 ).size(), 1u );
		EXPECT_EQ( recorded.at( REMOVAL_SEGMENT - 1 ).front()->GetDebugInfo(), "TailCommand 42" );
		ASSERT_EQ( recorded.count( REMOVAL_SEGMENT ), 1u );
		ASSERT_EQ( recorded.at( REMOVAL_SEGMENT ).size(), 1u );
		EXPECT_EQ( recorded.at( REMOVAL_SEGMENT ).front()->GetDebugInfo(), "DropPlayer 4" );
	}
};

TEST_F( MPGracefulLeave, ActualProducedTailIsBoundedAndDeterminesTheDepartureCutoff )
{
	ProduceTail( leaver );
	const auto &tail = leaver.transceiver->GetDeparturePackets();
	ASSERT_FALSE( tail.empty() );
	EXPECT_LE( tail.size(), 7u );
	EXPECT_GE( tail.size(), 5u );
	int nPrevious = ReadSegment( tail.front() ).nSegment - 1;
	for ( const auto &packet : tail )
	{
		ASSERT_TRUE( IsValid( packet ) );
		const SSegmentPacket decoded = ReadSegment( packet );
		EXPECT_EQ( decoded.nSegment, nPrevious + 1 );
		nPrevious = decoded.nSegment;
	}
	EXPECT_EQ( nPrevious, LAST_PACKET );
	EXPECT_EQ( ReadSegment( tail.back() ).commands.size(), 1u );
	EXPECT_EQ( leaver.transceiver->GetDepartureSegment(), DROP_CUTOFF );
	// A clock-position change cannot invent an unproduced command packet.
	CMPPlayerDropTestAccess::SetBoundary( leaver.transceiver, 14670 );
	EXPECT_EQ( leaver.transceiver->GetDepartureSegment(), DROP_CUTOFF );
}

TEST_F( MPGracefulLeave, NonHostSurrenderForwardsOwnedTailThenSelfDropBeforeBackendLeave )
{
	ProduceTail( leaver );
	const size_t nTailSize = leaver.transceiver->GetDeparturePackets().size();
	const int nFirstPacket = ReadSegment( leaver.transceiver->GetDeparturePackets().front() ).nSegment;
	Surrender( leaver );
	ASSERT_FALSE( leaver.client->persistent.empty() );
	EXPECT_NE( dynamic_cast<CLeaveGamePacket *>( leaver.client->persistent.back().GetPtr() ), nullptr );
	EXPECT_EQ( leaver.client->persistent.size(), 5 * ( nTailSize + 1 ) + 1 );
	for ( int nRecipient = 0; nRecipient < 6; ++nRecipient )
	{
		if ( nRecipient == LEAVER )
		{
			continue;
		}
		const auto packets = ForwardedTo( leaver, nRecipient );
		ASSERT_EQ( packets.size(), nTailSize + 1 );
		for ( size_t i = 0; i < nTailSize; ++i )
		{
			// EndGame releases the sender's transceiver; envelopes must retain
			// live packet objects rather than cleared CObj placeholders.
			ASSERT_TRUE( IsValid( packets[i] ) );
			EXPECT_EQ( ReadSegment( packets[i] ).nSegment, nFirstPacket + int( i ) );
		}
		const auto *pDrop = dynamic_cast<const CB2DropPlayerAtSegmentPacket *>( packets.back().GetPtr() );
		ASSERT_NE( pDrop, nullptr );
		EXPECT_EQ( pDrop->nSlotToDrop, LEAVER );
		EXPECT_EQ( pDrop->nSegment, DROP_CUTOFF );
	}
}

TEST_F( MPGracefulLeave, CapturedTwoTickSkewNeverWaitsForUnproducedPacket14668 )
{
	ProduceTail( leaver );
	PrepareHostAtCapturedBoundary();
	// This matches the reports: every outgoing packet, including the final
	// one, reached the host before the departing peer stopped.
	for ( const auto &packet : leaver.client->direct )
	{
		DeliverCommands( host, packet, LEAVER );
	}
	ASSERT_EQ( CMPPlayerDropTestAccess::LastReceived( host.transceiver, LEAVER ), LAST_PACKET );
	Surrender( leaver );
	QueueForwarded( leaver, host );
	RunManager( host );
	VerifyFinalCommandThenRemoval();
}

TEST_F( MPGracefulLeave, HostSurrenderPreservesFinalCommandsWhileMigratingAuthority )
{
	ProduceTail( host );
	CMPPlayerDropTestAccess::SetBoundary( leaver.transceiver, 14670 );
	CMPPlayerDropTestAccess::SetReceiptMask( leaver.transceiver, ALL_PLAYERS & ~( 1u << HOST ) );
	Surrender( host );
	QueueForwarded( host, leaver );
	RunManager( leaver );
	EXPECT_EQ( CMPPlayerDropTestAccess::HostClient( leaver.manager ), CLIENT_IDS[1] );
	ASSERT_EQ( CMPPlayerDropTestAccess::Removal( leaver.transceiver, HOST ), REMOVAL_SEGMENT );
	ASSERT_EQ( CMPPlayerDropTestAccess::LastReceived( leaver.transceiver, HOST ), LAST_PACKET );
	StepSimulation( leaver );
	ASSERT_EQ( leaver.transceiver->GetCurrentCommonSegment(), REMOVAL_SEGMENT );
	EXPECT_EQ( executedCommands, ( std::vector<int>{ 42 } ) );
	StepSimulation( leaver );
	EXPECT_EQ( leaver.transceiver->GetCurrentCommonSegment(), REMOVAL_SEGMENT + 1 );
	EXPECT_FALSE( leaver.transceiver->HasPlayerDropError() );
	EXPECT_FALSE( scenario->IsPlayerPresent( HOST ) );
	EXPECT_EQ( ai->neutralized, ( std::vector<int>{ HOST } ) );
	const CHistory recorded = leaver.history->GetCommandsHistory();
	ASSERT_EQ( recorded.count( REMOVAL_SEGMENT - 1 ), 1u );
	EXPECT_EQ( recorded.at( REMOVAL_SEGMENT - 1 ).front()->GetDebugInfo(), "TailCommand 42" );
	ASSERT_EQ( recorded.count( REMOVAL_SEGMENT ), 1u );
	EXPECT_EQ( recorded.at( REMOVAL_SEGMENT ).front()->GetDebugInfo(), "DropPlayer 0" );
}

TEST_F( MPGracefulLeave, DelayedLobbyDepartureArrivesAfterSurvivorReachesItsOrdinaryBarrier )
{
	ProduceTail( leaver );
	CMPPlayerDropTestAccess::SetBoundary( host.transceiver, 14668 );
	CMPPlayerDropTestAccess::SetReceiptMask( host.transceiver, ALL_PLAYERS & ~( 1u << LEAVER ) );
	for ( const auto &packet : leaver.transceiver->GetDeparturePackets() )
	{
		DeliverCommands( host, packet, LEAVER );
	}
	Surrender( leaver );
	// The final peer packets arrived, but the lobby departure is still delayed.
	// The survivor can consume two more ticks, then its ordinary barrier stops it.
	StepSimulation( host );
	ASSERT_EQ( host.transceiver->GetCurrentCommonSegment(), 14669 );
	StepSimulation( host );
	ASSERT_EQ( host.transceiver->GetCurrentCommonSegment(), 14670 );
	StepSimulation( host );
	ASSERT_EQ( host.transceiver->GetCurrentCommonSegment(), 14670 );
	ASSERT_EQ( host.transceiver->GetCurrentMissingPlayers(), 1u << LEAVER );
	ASSERT_TRUE( executedCommands.empty() );
	ASSERT_FALSE( host.transceiver->HasPlayerDropError() );
	QueueForwarded( leaver, host );
	RunManager( host );
	VerifyFinalCommandThenRemoval();
}

TEST_F( MPGracefulLeave, PersistentTailRecoversCommandsLostWhenDirectConnectionsClose )
{
	ProduceTail( leaver );
	PrepareHostAtCapturedBoundary();
	for ( const auto &packet : leaver.client->direct )
	{
		if ( ReadSegment( packet ).nSegment <= LAST_PACKET - 3 )
		{
			DeliverCommands( host, packet, LEAVER );
		}
	}
	ASSERT_EQ( CMPPlayerDropTestAccess::LastReceived( host.transceiver, LEAVER ), LAST_PACKET - 3 );
	Surrender( leaver );
	// Only the persistent server path remains. Its duplicate prefix and new
	// final commands use the same receiver as the original peer packets.
	QueueForwarded( leaver, host );
	RunManager( host );
	ASSERT_EQ( CMPPlayerDropTestAccess::LastReceived( host.transceiver, LEAVER ), LAST_PACKET );
	VerifyFinalCommandThenRemoval();
}

TEST_F( MPGracefulLeave, BackendNotificationDoesNotReplaceTheAcceptedSelfDeparture )
{
	ProduceTail( leaver );
	PrepareHostAtCapturedBoundary();
	Surrender( leaver );
	QueueForwarded( leaver, host );
	CPtr<CNetPacket> removed = new CGameClientRemoved( CLIENT_IDS[LEAVER] );
	host.client->received.push_back( removed );
	RunManager( host );
	EXPECT_EQ( CMPPlayerDropTestAccess::Removal( host.transceiver, LEAVER ), REMOVAL_SEGMENT );
	VerifyFinalCommandThenRemoval();
}

TEST_F( MPGracefulLeave, MissingRealFinalCommandStillBlocksUntilRecovered )
{
	ProduceTail( leaver );
	PrepareHostAtCapturedBoundary();
	for ( const auto &packet : leaver.client->direct )
	{
		if ( ReadSegment( packet ).nSegment < LAST_PACKET )
		{
			DeliverCommands( host, packet, LEAVER );
		}
	}
	CPtr<CNetPacket> finalPacket = leaver.client->direct.back();
	CPtr<CNetPacket> drop = new CB2DropPlayerAtSegmentPacket( CLIENT_IDS[LEAVER], LEAVER, DROP_CUTOFF );
	Deliver( host, drop );
	StepSimulation( host );
	EXPECT_EQ( host.transceiver->GetCurrentCommonSegment(), 14670 );
	EXPECT_TRUE( executedCommands.empty() );
	EXPECT_FALSE( host.transceiver->HasPlayerDropError() );
	DeliverCommands( host, finalPacket, LEAVER );
	VerifyFinalCommandThenRemoval();
}

TEST_F( MPGracefulLeave, SelfDepartureAuthorityDoesNotAuthorizeAnotherPlayersRemoval )
{
	PrepareHostAtCapturedBoundary();
	CPtr<CNetPacket> forged = new CB2DropPlayerAtSegmentPacket( CLIENT_IDS[LEAVER], 3, DROP_CUTOFF );
	Deliver( host, forged );
	EXPECT_EQ( CMPPlayerDropTestAccess::Removal( host.transceiver, 3 ), -1 );
	CPtr<CNetPacket> own = new CB2DropPlayerAtSegmentPacket( CLIENT_IDS[LEAVER], LEAVER, DROP_CUTOFF );
	Deliver( host, own );
	EXPECT_EQ( CMPPlayerDropTestAccess::Removal( host.transceiver, LEAVER ), REMOVAL_SEGMENT );
	EXPECT_FALSE( host.transceiver->HasPlayerDropError() );
}

TEST_F( MPGracefulLeave, SurrenderBeforeFirstPacketDoesNotInventARequiredCommandPrefix )
{
	EXPECT_TRUE( leaver.transceiver->GetDeparturePackets().empty() );
	EXPECT_EQ( leaver.transceiver->GetDepartureSegment(), 1 );
	Surrender( leaver );
	const auto packets = ForwardedTo( leaver, HOST );
	ASSERT_EQ( packets.size(), 1u );
	const auto *pDrop = dynamic_cast<const CB2DropPlayerAtSegmentPacket *>( packets.front().GetPtr() );
	ASSERT_NE( pDrop, nullptr );
	EXPECT_EQ( pDrop->nSegment, 1 );
	QueueForwarded( leaver, host );
	RunManager( host );
	EXPECT_EQ( CMPPlayerDropTestAccess::Removal( host.transceiver, LEAVER ), 3 );
	for ( int nSegment = 0; nSegment <= 3; ++nSegment )
	{
		CMPPlayerDropTestAccess::SetReceiptMask( host.transceiver, ALL_PLAYERS & ~( 1u << LEAVER ) );
		StepSimulation( host );
		ASSERT_EQ( host.transceiver->GetCurrentCommonSegment(), nSegment + 1 );
		ASSERT_FALSE( host.transceiver->HasPlayerDropError() );
	}
	EXPECT_EQ( CMPPlayerDropTestAccess::LastReceived( host.transceiver, LEAVER ), -1 );
	EXPECT_FALSE( scenario->IsPlayerPresent( LEAVER ) );
}
} // namespace
