#include "../stdafx.h"

#include "../AICmdsAutoMagic.h"
#include "../Transceiver.h"
#include "AILogic/B2AI.h"
#include "Main/CommandsHistory.hpp"
#include "Main/GameTimer.h"

#include <gtest/gtest.h>

namespace
{
struct SClockObservations
{
	std::vector<int> commandSegments;
	std::vector<NTimer::STime> commandTimes;
	std::vector<NTimer::STime> simulationTimes;
	bool bSuspended = false;
	bool bSuspendAfterSegment = false;
};

SClockObservations observations;

class CClockCommand : public IAILogicCommandB2
{
	OBJECT_NOCOPY_METHODS( CClockCommand );

public:
	void Execute() override
	{
		observations.commandTimes.push_back( GameTimer()->GetSegmentTime() );
	}

	bool NeedToBeStored() const override
	{
		return true;
	}
};

class CClockHistory : public ICommandsHistory
{
	OBJECT_NOCOPY_METHODS( CClockHistory );

public:
	void StartNewGame( const NDb::SMapInfo * ) override {}

	const NDb::SMapInfo *GetMap() const override
	{
		return nullptr;
	}

	void AddCommand( const int, IAILogicCommandB2 * ) override {}

	void ExecuteSegmentCommands( const int nSegment, ITransceiver *pTransceiver ) override
	{
		observations.commandSegments.push_back( nSegment );
		CObj<IAILogicCommandB2> command = new CClockCommand;
		// Exercise the real SendCommand path as replay history does.
		pTransceiver->SendCommand( command );
	}

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
		return {};
	}
};

// The transceiver needs only suspension and Segment. Keep map loading, graphics
// and combat out of this fixture while observing calls from the real DLL loop.
class CClockAI : public IAILogic
{
	OBJECT_NOCOPY_METHODS( CClockAI );

public:
	void Suspend() override
	{
		observations.bSuspended = true;
	}

	void Resume() override
	{
		observations.bSuspended = false;
	}

	void Segment() override
	{
		observations.simulationTimes.push_back( GameTimer()->GetSegmentTime() );
		if ( observations.bSuspendAfterSegment )
		{
			Suspend();
		}
	}

	bool IsSuspended() const override
	{
		return observations.bSuspended;
	}

	void ClearAI() override {}

	bool IsMissionLoaded() const override
	{
		return false;
	}

	void SetProgressHook( IProgressHook * ) override {}

	void Init( ICheckSumLog *, const NDb::SMapInfo *, const NDb::SAIGameConsts *, IAIScenarioTracker * ) override {}

	void InitAfterMapLoad( const NDb::SMapInfo * ) override {}

	void ToggleWarFog( const bool ) override {}

	int GetMiniMapWarFogSizeX() const override
	{
		return 0;
	}

	int GetMiniMapWarFogSizeY() const override
	{
		return 0;
	}

	const bool GetMiniMapUnitsInfo( std::vector<SMiniMapUnitInfo> & ) override
	{
		return false;
	}

	bool GetMiniMapWarForInfo( CArray2D<uint8_t> **, bool ) override
	{
		return false;
	}

	bool UpdateAcknowledgment( SAIAcknowledgment & ) override
	{
		return false;
	}

	bool UpdateAcknowledgment( SAIBoredAcknowledgement & ) override
	{
		return false;
	}

	bool IsCombatSituation() override
	{
		return false;
	}

	void PrepareUpdates() override {}

	CObjectBase *GetUpdate() override
	{
		return nullptr;
	}

	float GetZ( const CVec2 & ) const override
	{
		return 0.0f;
	}

	const bool GetIntersectionWithTerrain( CVec3 *, const CVec3 &, const CVec3 & ) const override
	{
		return false;
	}

	const uint32_t GetNormal( const CVec2 & ) const override
	{
		return 0;
	}

	ITerraAIObserver *CreateTerraAIObserver( const int, const int ) override
	{
		return nullptr;
	}

	void RegisterGroup( const std::vector<int> &, const int, bool ) override {}

	void UnregisterGroup( const int ) override {}

	void GroupCommand( SAIUnitCmd *, const uint16_t, bool ) override {}

	void UnitCommand( SAIUnitCmd *, const uint16_t, const int ) override {}

	void TransferUnits( const std::vector<int> &, int, int ) override {}

	const int GenerateGroupNumber() override
	{
		return 0;
	}

	void RequestBuildPreview( const EActionCommand, const CVec2 &, const CVec2 &, bool ) override {}

	void ShowAreas( const std::vector<int> &, EActionNotify &, bool ) override {}

	void GetShootAreas( int, SShootAreas * ) override {}

	const int WAR_FOG_FULL_UPDATE() const override
	{
		return 0;
	}

	const int VIS_POWER() const override
	{
		return 0;
	}

	void PickedObj( const int ) override {}

	void PickEmpty() override {}

	void SetGlobeScriptHandler( IGlobeScriptHandler * ) override {}

	CStaticMapHeights *GetHeights() const override
	{
		return nullptr;
	}

	void NetGameStarted() override {}

	bool IsNetGameStarted() const override
	{
		return false;
	}

	void WriteDetailedChecksumInfo( const char * ) override {}

	void SetMyDiplomacyInfo( const int, const int ) override {}

	void SetNPlayers( const int ) override {}

	void SetNetGame( const bool ) override {}

	void NeutralizePlayer( const int ) override {}

	const int GetPlayerDiplomacy( const int ) const override
	{
		return 0;
	}

	const int GetUnitCount( const int ) const override
	{
		return 0;
	}

	const bool HasPlayerNoUnits( const int ) const override
	{
		return true;
	}

	void SetNeedNewGroupNumber() override {}

	const bool NeedNewGroupNumber() const override
	{
		return false;
	}

	void ResetNeedNewGroupNumber() override {}

	const NDb::SAIGameConsts *GetAIConsts() const override
	{
		return nullptr;
	}

	void DumpAfterAssinc() const override {}
};
} // namespace

// These two factory types exist only in this test executable.
#define REPLAYCLOCKTEST_EXPORT
REGISTER_SAVELOAD_CLASS( REPLAYCLOCKTEST, 0x7E57AC10, CClockHistory )
REGISTER_SAVELOAD_CLASS( REPLAYCLOCKTEST, 0x7E57AC11, CClockAI )

namespace
{
struct STimerState
{
	int nSegmentDuration = 200;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 7, &nSegmentDuration );
		return 0;
	}
};

struct STransceiverState
{
	CPtr<ICommandsHistory> pHistory = new CClockHistory;
	CPtr<IAILogic> pAI = new CClockAI;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 2, &pHistory );
		saver.Add( 5, &pAI );
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

class ReplayCommandTiming : public testing::Test
{
protected:
	static constexpr NTimer::STime TIME_ORIGIN = 1000;
	CObj<IGameTimer> timer;
	CObj<ITransceiver> transceiver;

	void SetUp() override
	{
		// Anchor GameX through an exported function so its factory registration is
		// loaded on Windows too. No production method needs a test-only export.
		CObj<IAICmdsAutoMagic> registry = CreateAICmdsAutoMagic();
		ASSERT_NE( registry, nullptr );
		timer = dynamic_cast<IGameTimer *>( NObjectFactory::MakeObject( 0x10075C05 ) );
		ASSERT_NE( timer, nullptr );
		STimerState timerState;
		RestoreState( timer, timerState );
		// Zero is the real timer's sentinel for an uninitialized wall-clock origin.
		timer->Reset( TIME_ORIGIN );
		timer->SetSpeed( 0 );
		NSingleton::RegisterSingleton( timer, IGameTimer::tidTypeID );
		transceiver = dynamic_cast<ITransceiver *>( NObjectFactory::MakeObject( 0x1007AB40 ) );
		ASSERT_NE( transceiver, nullptr );
		STransceiverState transceiverState;
		RestoreState( transceiver, transceiverState );
		observations = {};
		NGlobal::SetVar( "AI_SLOWDOWN", 0 );
		NGlobal::SetVar( "History.Playing", 1 );
		NGlobal::SetVar( "Replay.ExitWhenFinished", 0 );
	}

	void TearDown() override
	{
		transceiver = nullptr;
		NSingleton::UnRegisterSingleton( IGameTimer::tidTypeID );
		timer = nullptr;
		NGlobal::RemoveVar( "AI_SLOWDOWN" );
		NGlobal::RemoveVar( "History.Playing" );
		NGlobal::RemoveVar( "Replay.ExitWhenFinished" );
	}

	void AdvanceTo( const NTimer::STime time )
	{
		timer->Update( TIME_ORIGIN + time );
		transceiver->DoSegments();
	}
};

TEST_F( ReplayCommandTiming, DoesNotDispatchBeforeATickIsDue )
{
	transceiver->DoSegments();
	AdvanceTo( 199 );
	EXPECT_TRUE( observations.commandSegments.empty() );
	EXPECT_TRUE( observations.commandTimes.empty() );
	EXPECT_TRUE( observations.simulationTimes.empty() );
	EXPECT_EQ( timer->GetSegmentTime(), 0 );
}

TEST_F( ReplayCommandTiming, CommandsSeeCompletedTickBeforeEachCatchUpSimulation )
{
	AdvanceTo( 200 );
	AdvanceTo( 600 );
	EXPECT_EQ( observations.commandSegments, ( std::vector<int>{ 0, 1, 2 } ) );
	EXPECT_EQ( observations.commandTimes, ( std::vector<NTimer::STime>{ 0, 200, 400 } ) );
	EXPECT_EQ( observations.simulationTimes, ( std::vector<NTimer::STime>{ 200, 400, 600 } ) );
	EXPECT_EQ( timer->GetSegment(), 3 );
}

TEST_F( ReplayCommandTiming, SuspensionConsumesTimerTicksWithoutConsumingReplayCommands )
{
	observations.bSuspended = true;
	AdvanceTo( 400 );
	EXPECT_EQ( timer->GetSegmentTime(), 400 );
	EXPECT_TRUE( observations.commandSegments.empty() );
	EXPECT_TRUE( observations.simulationTimes.empty() );
	observations.bSuspended = false;
	AdvanceTo( 600 );
	EXPECT_EQ( observations.commandSegments, ( std::vector<int>{ 0 } ) );
	EXPECT_EQ( observations.commandTimes, ( std::vector<NTimer::STime>{ 400 } ) );
	EXPECT_EQ( observations.simulationTimes, ( std::vector<NTimer::STime>{ 600 } ) );
}

TEST_F( ReplayCommandTiming, SuspensionDuringCatchUpStopsCommandsUntilResume )
{
	observations.bSuspendAfterSegment = true;
	AdvanceTo( 400 );
	EXPECT_EQ( timer->GetSegmentTime(), 400 );
	EXPECT_EQ( observations.commandSegments, ( std::vector<int>{ 0 } ) );
	EXPECT_EQ( observations.simulationTimes, ( std::vector<NTimer::STime>{ 200 } ) );
	observations.bSuspended = false;
	observations.bSuspendAfterSegment = false;
	AdvanceTo( 600 );
	EXPECT_EQ( observations.commandSegments, ( std::vector<int>{ 0, 1 } ) );
	EXPECT_EQ( observations.commandTimes, ( std::vector<NTimer::STime>{ 0, 400 } ) );
	EXPECT_EQ( observations.simulationTimes, ( std::vector<NTimer::STime>{ 200, 600 } ) );
}
} // namespace
