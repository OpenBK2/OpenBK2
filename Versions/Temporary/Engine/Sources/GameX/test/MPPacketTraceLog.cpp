#include "GameX/MPPacketTraceLog.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>

namespace
{
namespace fs = std::filesystem;

class MPPacketTraceLog : public testing::Test
{
protected:
	fs::path originalDirectory;
	fs::path traceDirectory;

	void SetUp() override
	{
		originalDirectory = fs::current_path();
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		traceDirectory = fs::temp_directory_path() / ( "obk2-mp-trace-" + std::to_string( stamp ) );
		ASSERT_TRUE( fs::create_directory( traceDirectory ) );
		fs::current_path( traceDirectory );
		NGameX::MatchPacketTrace_Reset();
	}

	void TearDown() override
	{
		NGameX::MatchPacketTrace_Reset();
		std::error_code error;
		fs::current_path( originalDirectory, error );
		// Only the single trace and this test's empty directory are removed.
		fs::remove( traceDirectory / "last_match_packets.txt", error );
		fs::remove( traceDirectory, error );
	}

	void BeginMatch( int gameID = 12 )
	{
		NGameX::MatchPacketTrace_Reset();
		std::vector<NGameX::SMatchPacketTraceSlot> slots( 1 );
		slots[0].nSlot = 0;
		slots[0].nClientID = 42;
		slots[0].nTeam = 1;
		slots[0].bPresent = true;
		NGameX::MatchPacketTrace_SetHeader( gameID, "session", "map", 7, 42, 0, 42, 1, 1, 1, slots );
	}

	std::string ReadTrace()
	{
		std::ifstream file( traceDirectory / "last_match_packets.txt" );
		return std::string( std::istreambuf_iterator<char>( file ), std::istreambuf_iterator<char>() );
	}
};

TEST_F( MPPacketTraceLog, ControlEventsAreReadableBeforeMatchEnds )
{
	BeginMatch();
	const std::string header = ReadTrace();
	EXPECT_NE( header.find( "flush_reason=in_progress" ), std::string::npos );
	EXPECT_NE( header.find( "slot=0 client_id=42 team=1 present=1" ), std::string::npos );
	EXPECT_EQ( header.find( "final_present_mask" ), std::string::npos );

	NGameX::MatchPacketTrace_Log( 8, "TX", "CAISegmentFinishedPacket", 42, "segment=8" );
	NGameX::MatchPacketTrace_Log( 9, "DECISION", "ScheduleGameEnd", 42, "requested_segment=9" );
	// A separate reader sees the control decision and prior traffic without a
	// final flush or stream close, so terminating the process retains evidence.
	const std::string journal = ReadTrace();
	EXPECT_NE( journal.find( "#000001 seg=8 type=TX name=CAISegmentFinishedPacket" ), std::string::npos );
	EXPECT_NE( journal.find( "#000002 seg=9 type=DECISION name=ScheduleGameEnd" ), std::string::npos );
	EXPECT_EQ( journal.find( "-- event_type_counts --" ), std::string::npos );
}

TEST_F( MPPacketTraceLog, SegmentTrafficIsFlushedPeriodically )
{
	BeginMatch();
	NGameX::MatchPacketTrace_Log( 8, "TX", "CAISegmentFinishedPacket", 42, "segment=8" );
	// Use the real writer's steady clock once; no game time or simulation state
	// is involved, and normal control-event tests need no waiting.
	std::this_thread::sleep_for( std::chrono::milliseconds( 1100 ) );
	NGameX::MatchPacketTrace_Log( 9, "TX", "CAISegmentFinishedPacket", 42, "segment=9" );
	const std::string journal = ReadTrace();
	EXPECT_NE( journal.find( "#000001 seg=8" ), std::string::npos );
	EXPECT_NE( journal.find( "#000002 seg=9" ), std::string::npos );
}

TEST_F( MPPacketTraceLog, FinalFlushKeepsExistingFormatAndStopsLogging )
{
	BeginMatch();
	NGameX::MatchPacketTrace_Log( 8, "TX", "CAISegmentFinishedPacket", 42, "segment=8" );
	NGameX::MatchPacketTrace_Log( 9, "STATE", "EndGame", 42, "one\ntwo\tthree" );
	NGameX::MatchPacketTrace_RecordDropScheduled( 0, 8 );
	NGameX::MatchPacketTrace_RecordDropApplied( 0, 10 );
	NGameX::MatchPacketTrace_SetFinalState( 1, 0, 1 );
	NGameX::MatchPacketTrace_Flush( "match_end" );
	const std::string complete = ReadTrace();
	EXPECT_NE( complete.find( "flush_reason=match_end" ), std::string::npos );
	EXPECT_EQ( complete.find( "in_progress" ), std::string::npos );
	EXPECT_NE( complete.find( "final_present_mask=0x00000001 final_laggers_mask=0x00000000 final_transceiver_mask=0x00000001" ), std::string::npos );
	EXPECT_NE( complete.find( "-- events (2) --" ), std::string::npos );
	EXPECT_NE( complete.find( "one two three" ), std::string::npos );
	EXPECT_NE( complete.find( "-- event_type_counts --\nSTATE=1\nTX=1" ), std::string::npos );
	EXPECT_NE( complete.find( "slot=0 scheduled_seg=8 applied_seg=10 delta=2" ), std::string::npos );

	NGameX::MatchPacketTrace_Log( 11, "STATE", "IgnoredAfterEnd", 42, "" );
	NGameX::MatchPacketTrace_Flush( "second_flush" );
	EXPECT_EQ( ReadTrace(), complete );
}

TEST_F( MPPacketTraceLog, ResetClosesBufferedJournalAndNextMatchReplacesIt )
{
	BeginMatch();
	NGameX::MatchPacketTrace_Log( 8, "TX", "CAISegmentFinishedPacket", 42, "old_match=1" );
	NGameX::MatchPacketTrace_Reset();
	EXPECT_NE( ReadTrace().find( "old_match=1" ), std::string::npos );

	BeginMatch( 13 );
	NGameX::MatchPacketTrace_Log( 0, "STATE", "StartGame", 42, "" );
	const std::string journal = ReadTrace();
	EXPECT_NE( journal.find( "game_id=13" ), std::string::npos );
	EXPECT_NE( journal.find( "#000001 seg=0" ), std::string::npos );
	EXPECT_EQ( journal.find( "old_match=1" ), std::string::npos );
}

TEST_F( MPPacketTraceLog, FailedFinalOpenAllowsRetryWithAllEvents )
{
	// An existing directory makes fopen fail on both supported platforms.
	ASSERT_TRUE( fs::create_directory( traceDirectory / "last_match_packets.txt" ) );
	BeginMatch();
	NGameX::MatchPacketTrace_Log( 8, "STATE", "BeforeFailure", 42, "" );
	NGameX::MatchPacketTrace_Flush( "match_end" );
	NGameX::MatchPacketTrace_Log( 9, "STATE", "AfterFailure", 42, "" );
	ASSERT_TRUE( fs::remove( traceDirectory / "last_match_packets.txt" ) );
	NGameX::MatchPacketTrace_Flush( "retry" );
	const std::string complete = ReadTrace();
	EXPECT_NE( complete.find( "flush_reason=retry" ), std::string::npos );
	EXPECT_NE( complete.find( "-- events (2) --" ), std::string::npos );
	EXPECT_NE( complete.find( "name=BeforeFailure" ), std::string::npos );
	EXPECT_NE( complete.find( "name=AfterFailure" ), std::string::npos );
}
}
