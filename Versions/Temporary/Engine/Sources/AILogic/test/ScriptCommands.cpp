#include "../stdafx.h"

#include "../B2AI.h"
#include "../Commands.h"
#include "../Formation.h"
#include "../GroupLogic.h"
#include "../ScriptCommandQueue.h"
#include "../Scripts.h"
#include "../Soldier.h"
#include "../StaticMembers.h"
#include "Common_RTS_AI/Checksums.h"
#include "GameX/ScenarioTracker.h"
#include "Script/ScriptWrapper.h"
#include "System/BinSaver.h"
#include "System/ConsoleBuffer.h"
#include "System/RandomGen.h"
#include "System/Streams.h"

#include <functional>
#include <string>
#include <type_traits>
#include <vector>

#include <fmt/format.h>
#include <gtest/gtest.h>

namespace
{
SQueuedScriptCommand Order( int unitID, bool append = false )
{
	SQueuedScriptCommand command;
	command.unitIDs.push_back( unitID );
	command.command = SAIUnitCmd( ACTION_COMMAND_MOVE_TO, CVec2( unitID, 200.0f ) );
	command.bPlaceInQueue = append;
	return command;
}

void RoundTrip( CScriptCommandQueue *queue )
{
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->Add( 1, queue );
	}
	queue->Clear();
	stream.Seek( 0 );
	CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
	loader->Add( 1, queue );
}

const SRegFunction commandFunctions[] = { { "Cmd", CScripts::GiveCommand },
                                          { "QCmd", CScripts::GiveQCommand },
                                          { "UnitCmd", CScripts::GiveUnitCommand },
                                          { "UnitQCmd", CScripts::GiveUnitQCommand },
                                          { nullptr, nullptr } };

struct SScriptFields
{
	CPtr<IScriptWrapper> vm;
	CScriptGroups groups;
	CScriptCommandQueue queue;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 2, &vm );
		saver.Add( 3, &groups );
		saver.Add( 20, &queue );
		return 0;
	}
};

struct SFields
{
	std::function<void( IBinSaver & )> visit;

	int operator&( IBinSaver &saver )
	{
		visit( saver );
		return 0;
	}
};

class CooperativeScriptCommands : public testing::Test
{
protected:
	CScripts scripts;
	CObj<IAILogic> logic;
	CObj<IScenarioTracker> tracker;
	CObj<IScriptWrapper> vm;
	std::vector<CObj<CCommonUnit>> units;
	std::vector<SBinSaverExternalObject> external;
	SScriptFields fields;

	template <class TSource, class TTarget>
	void CopyFields( TSource *source, TTarget *target )
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			saver->Add( 1, source );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		if constexpr ( std::is_base_of_v<CObjectBase, TTarget> )
		{
			// Dispatch inside the object's DLL instead of instantiating its base serializer here.
			loader->AddPolymorphicBase( 1, target );
		}
		else
		{
			loader->Add( 1, target );
		}
	}

	void SetUp() override
	{
		tracker = CreateScenarioTrackerMultiplayer();
		tracker->SetGameType( IAIScenarioTracker::EGT_MULTI_COOP );
		NSingleton::RegisterSingleton( tracker, IAIScenarioTracker::tidTypeID );
		logic = dynamic_cast<IAILogic *>( NObjectFactory::MakeObject( 0x1108D441 ) );
		ASSERT_NE( logic, nullptr );
		NSingleton::RegisterSingleton( logic, IAILogic::tidTypeID );
		theGroupLogic.Init( nullptr );
		vm = CreateScriptWrapper();
		vm->Init();
		vm->AddRegFunctions( commandFunctions );
		external.emplace_back( 1000, vm );
		fields.vm = vm;
		CopyFields( &fields, &scripts );

		// These tests dispatch orders but do not advance unit states or pathfinding.
		CDecksSet<CObj<CAICommand>> queues( 128 );
		SFields queueFields{ [&]( IBinSaver &saver ) { saver.Add( 1, &queues ); } };
		CStaticMembers members;
		CopyFields( &queueFields, &members );
	}

	void TearDown() override
	{
		scripts.Clear();
		NSingleton::UnRegisterSingleton( IAILogic::tidTypeID );
		logic = nullptr;
		units.clear();
		NSingleton::UnRegisterSingleton( IAIScenarioTracker::tidTypeID );
	}

	CFormation *AddFormation( int id, int scriptGroup = 7 )
	{
		auto *unit = dynamic_cast<CFormation *>( NObjectFactory::MakeObject( 0x1108D498 ) );
		EXPECT_NE( unit, nullptr );
		if ( !unit )
		{
			return nullptr;
		}
		unit->nUniqueID = id;
		unit->Mem2UniqueIdObjs();
		units.push_back( unit );
		external.emplace_back( id, unit );
		CArray1Bit commands( 2000 );
		commands.SetData( ACTION_COMMAND_WAIT );
		commands.SetData( ACTION_COMMAND_STOP );
		commands.SetData( ACTION_COMMAND_ATTACK_UNIT );
		uint8_t owner = 2;
		SFields formationFields{ [&]( IBinSaver &saver )
		                         {
															 saver.Add( 4, &commands );
															 saver.Add( 5, &owner );
														 } };
		CopyFields( &formationFields, unit );
		fields.groups[scriptGroup].push_back( unit );
		CopyFields( &fields, &scripts );
		return unit;
	}

	void Run( const std::string &code )
	{
		ASSERT_EQ( vm->RunScript( code.c_str() ), 1 );
		vm->Segment();
	}

	CSoldier *AddSoldier( int id, CFormation *formation )
	{
		auto *soldier = dynamic_cast<CSoldier *>( NObjectFactory::MakeObject( 0x1108D442 ) );
		EXPECT_NE( soldier, nullptr );
		if ( soldier )
		{
			soldier->nUniqueID = id;
			soldier->Mem2UniqueIdObjs();
			units.push_back( soldier );
			external.emplace_back( id, soldier );
			JoinFormation( soldier, formation );
			fields.groups[7].push_back( soldier );
			CopyFields( &fields, &scripts );
		}
		return soldier;
	}

	void JoinFormation( CSoldier *soldier, CFormation *formation )
	{
		CPtr<CFormation> pointer = formation;
		SFields soldierFields{ [&]( IBinSaver &saver ) { saver.Add( 18, &pointer ); } };
		SFields infantryFields{ [&]( IBinSaver &saver ) { saver.Add( 1, &soldierFields ); } };
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			saver->Add( 1, &infantryFields );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		loader->AddPolymorphicBase( 1, soldier );
	}

	std::vector<SQueuedScriptCommand> Pending()
	{
		SScriptFields saved;
		CopyFields( &scripts, &saved );
		std::vector<SQueuedScriptCommand> result;
		saved.queue.Drain( [&]( SQueuedScriptCommand &order ) { result.push_back( order ); } );
		return result;
	}
};
} // namespace

TEST_F( CooperativeScriptCommands, AllAliasesQueueSnapshotsWithoutIssuingPlayerOrders )
{
	AddFormation( 12 );
	AddFormation( 4 );
	Run( fmt::format( "Cmd({0}, 7, 1); QCmd({0}, 7, 2); UnitCmd({0}, 12, 3); UnitQCmd({0}, 4, 4);",
	                  int( ACTION_COMMAND_WAIT ) ) );
	const auto pending = Pending();
	ASSERT_EQ( pending.size(), 4 );
	EXPECT_EQ( pending[0].unitIDs, ( std::vector<int>{ 4, 12 } ) );
	EXPECT_EQ( pending[1].unitIDs, pending[0].unitIDs );
	EXPECT_EQ( pending[2].unitIDs, ( std::vector<int>{ 12 } ) );
	EXPECT_EQ( pending[3].unitIDs, ( std::vector<int>{ 4 } ) );
	for ( int i = 0; i != 4; ++i )
	{
		EXPECT_EQ( pending[i].bPlaceInQueue, i % 2 != 0 );
		EXPECT_TRUE( pending[i].command.bFromAI );
		EXPECT_FLOAT_EQ( pending[i].command.fNumber, float( i + 1 ) );
	}
}

TEST_F( CooperativeScriptCommands, TargetIDsAreNotDispersionAndMembershipIsCaptured )
{
	AddFormation( 12 );
	AddFormation( 4 );
	AddFormation( 21, 8 );
	NRandom::SDebugState before, after;
	NRandom::GetDebugState( &before );
	Run( fmt::format( "Cmd({}, 7, 8);", int( ACTION_COMMAND_ATTACK_UNIT ) ) );
	NRandom::GetDebugState( &after );
	EXPECT_EQ( before.randomCalls, after.randomCalls );
	CopyFields( &scripts, &fields );
	fields.groups[7].clear();
	fields.groups[7].push_back( units.back().GetPtr() );
	CopyFields( &fields, &scripts );
	const auto pending = Pending();
	ASSERT_EQ( pending.size(), 1 );
	EXPECT_EQ( pending[0].unitIDs, ( std::vector<int>{ 4, 12 } ) );
	EXPECT_EQ( pending[0].command.nObjectID, 21 );
}

TEST_F( CooperativeScriptCommands, FormationMembersNormalizeToOneCommandRecipient )
{
	CFormation *formation = AddFormation( 12 );
	AddSoldier( 13, formation );
	AddSoldier( 14, formation );
	Run( fmt::format( "Cmd({}, 7, 2);", int( ACTION_COMMAND_WAIT ) ) );
	const auto pending = Pending();
	ASSERT_EQ( pending.size(), 1 );
	EXPECT_EQ( pending[0].unitIDs, ( std::vector<int>{ 12 } ) );
}

TEST_F( CooperativeScriptCommands, InvalidNumericArgumentsNeverReachIntegerConversion )
{
	AddFormation( 12 );
	Run( fmt::format( "Cmd(0/0, 7, 2); Cmd({0}, 1e100, 2); Cmd({0}, 7, 0/0); "
	                  "UnitCmd({1}, 12, 1e100); Cmd({2}, 7, 0, 1/0, 10); Cmd({2}, 7, 0, 10, -1e100);",
	                  int( ACTION_COMMAND_WAIT ),
	                  int( ACTION_COMMAND_ATTACK_UNIT ),
	                  int( ACTION_COMMAND_MOVE_TO ) ) );
	EXPECT_TRUE( Pending().empty() );
	EXPECT_EQ( units.front()->GetNextCommand(), nullptr );
}

TEST_F( CooperativeScriptCommands, NewlyJoinedFormationDoesNotAcquireAnOldIndividualOrder )
{
	CFormation *formation = AddFormation( 12, 8 );
	CSoldier *soldier = AddSoldier( 13, nullptr );
	Run( fmt::format( "UnitCmd({}, 13, 2);", int( ACTION_COMMAND_WAIT ) ) );
	JoinFormation( soldier, formation );
	scripts.Segment();
	EXPECT_EQ( formation->GetNextCommand(), nullptr );
	EXPECT_TRUE( Pending().empty() );
}

TEST_F( CooperativeScriptCommands, SegmentDispatchesToAIFormationAndRestoresItsPlayerGroup )
{
	CFormation *unit = AddFormation( 12 );
	ASSERT_NE( unit, nullptr );
	const int originalGroup = logic->GenerateGroupNumber();
	logic->RegisterGroup( std::vector<int>{ 12 }, originalGroup );
	Run( fmt::format( "Cmd({}, 7, 3);", int( ACTION_COMMAND_WAIT ) ) );
	EXPECT_EQ( unit->GetNextCommand(), nullptr );
	scripts.Segment();
	ASSERT_NE( unit->GetNextCommand(), nullptr );
	EXPECT_EQ( unit->GetNextCommand()->ToUnitCmd().nCmdType, ACTION_COMMAND_WAIT );
	EXPECT_FLOAT_EQ( unit->GetNextCommand()->ToUnitCmd().fNumber, 3.0f );
	EXPECT_EQ( unit->GetNGroup(), originalGroup );
	EXPECT_TRUE( Pending().empty() );
}

TEST_F( CooperativeScriptCommands, DeadTargetsCancelQueuedOrderAtDispatch )
{
	CFormation *unit = AddFormation( 12 );
	CFormation *target = AddFormation( 21, 8 );
	Run( fmt::format( "Cmd({}, 7, 8);", int( ACTION_COMMAND_ATTACK_UNIT ) ) );
	bool alive = false;
	bool aliveSet = true;
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->Add( 2, &alive );
		saver->Add( 3, &aliveSet );
	}
	stream.Seek( 0 );
	CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
	target->CUpdatableObj::operator&( *loader );
	scripts.Segment();
	EXPECT_EQ( unit->GetNextCommand(), nullptr );
	EXPECT_TRUE( Pending().empty() );
}

TEST_F( CooperativeScriptCommands, LocalPipeCannotScheduleLaterGameplayOrders )
{
	AddFormation( 12 );
	WriteToPipe( PIPE_SCRIPT_CMDS,
	             fmt::format( "function LocalOrder() Sleep(); Cmd({}, 7, 2); end; StartThread(LocalOrder);",
	                          int( ACTION_COMMAND_WAIT ) ) );
	scripts.Segment();
	scripts.Segment();
	EXPECT_TRUE( Pending().empty() );
	EXPECT_EQ( units.front()->GetNextCommand(), nullptr );
}

TEST_F( CooperativeScriptCommands, PendingOrdersAffectLightChecksumBeforeTheyExecute )
{
	AddFormation( 12 );
	const auto checksum = [this]()
	{
		uint32_t result = 1;
		{
			CPtr<IBinSaver> saver = CreateCheckSumSaver( &result, nullptr, 0 );
			scripts.SerializeCommandQueue( *saver );
		}
		return result;
	};
	const uint32_t empty = checksum();
	Run( fmt::format( "Cmd({}, 7, 3);", int( ACTION_COMMAND_WAIT ) ) );
	const uint32_t pending = checksum();
	EXPECT_NE( pending, empty );
	CopyFields( &scripts, &fields );
	CopyFields( &fields, &scripts );
	EXPECT_EQ( checksum(), pending );
	scripts.Segment();
	EXPECT_EQ( checksum(), empty );
}

TEST_F( CooperativeScriptCommands, LegacySaveWithoutQueueDiscardsPreviousPendingOrders )
{
	AddFormation( 12 );
	Run( fmt::format( "Cmd({}, 7, 3);", int( ACTION_COMMAND_WAIT ) ) );
	ASSERT_EQ( Pending().size(), 1 );
	SFields legacyFields{ [&]( IBinSaver &saver )
	                      {
													saver.Add( 2, &fields.vm );
													saver.Add( 3, &fields.groups );
												} };
	CopyFields( &legacyFields, &scripts );
	EXPECT_TRUE( Pending().empty() );
}

TEST( ScriptCommands, PreservesInitializationOrdersAndQueuedFlagAcrossSave )
{
	CScriptCommandQueue queue;
	SQueuedScriptCommand command = Order( 41 );
	queue.Add( command );
	// The issuing Lua stack and its temporary ID collection can be reused immediately.
	command.unitIDs[0] = 99;
	command.command.vPos.x = 99;
	queue.Add( Order( 42, true ) );
	RoundTrip( &queue );
	std::vector<int> executed;
	queue.Drain(
			[&]( SQueuedScriptCommand &order )
			{
				EXPECT_EQ( order.unitIDs.size(), 1 );
				executed.push_back( order.unitIDs[0] );
				EXPECT_FLOAT_EQ( order.command.vPos.x, float( order.unitIDs[0] ) );
				EXPECT_EQ( order.bPlaceInQueue, order.unitIDs[0] == 42 );
			} );
	EXPECT_EQ( executed, ( std::vector<int>{ 41, 42 } ) );
	queue.Drain( []( SQueuedScriptCommand & ) { ADD_FAILURE() << "A drained order ran twice"; } );
}

TEST( ScriptCommands, CallbackOrdersWaitForNextDrainAndSurviveSave )
{
	CScriptCommandQueue queue;
	queue.Add( Order( 1 ) );
	queue.Add( Order( 2 ) );
	std::vector<int> executed;
	queue.Drain(
			[&]( SQueuedScriptCommand &order )
			{
				executed.push_back( order.unitIDs[0] );
				queue.Add( Order( order.unitIDs[0] + 10, true ) );
			} );
	EXPECT_EQ( executed, ( std::vector<int>{ 1, 2 } ) );
	RoundTrip( &queue );
	queue.Drain( [&]( SQueuedScriptCommand &order ) { executed.push_back( order.unitIDs[0] ); } );
	EXPECT_EQ( executed, ( std::vector<int>{ 1, 2, 11, 12 } ) );
}

TEST( ScriptCommands, MapResetDiscardsPendingOrders )
{
	CScriptCommandQueue queue;
	queue.Add( Order( 1 ) );
	queue.Clear();
	queue.Drain( []( SQueuedScriptCommand & ) { ADD_FAILURE() << "An old mission's order survived reset"; } );
}

TEST( ScriptCommands, ScriptsSaveIncludesThePendingQueue )
{
	// Populate the real CScripts record, then save it again. This catches dropping
	// pending initialization/callback orders from the simulation's save representation.
	struct SFields
	{
		CScriptCommandQueue queue;

		int operator&( IBinSaver &saver )
		{
			saver.Add( 20, &queue );
			return 0;
		}
	} fields;

	fields.queue.Add( Order( 7, true ) );
	CScripts scripts;
	CMemoryStream input;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &input, SAVER_MODE_WRITE );
		saver->Add( 1, &fields );
	}
	input.Seek( 0 );
	{
		CPtr<IBinSaver> loader = CreateBinSaver( &input, SAVER_MODE_READ );
		loader->Add( 1, &scripts );
	}
	CMemoryStream output;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &output, SAVER_MODE_WRITE );
		saver->Add( 1, &scripts );
	}
	output.Seek( 0 );
	{
		CPtr<IBinSaver> loader = CreateBinSaver( &output, SAVER_MODE_READ );
		loader->Add( 1, &fields );
	}
	std::vector<int> executed;
	fields.queue.Drain( [&]( SQueuedScriptCommand &order ) { executed.push_back( order.unitIDs[0] ); } );
	EXPECT_EQ( executed, ( std::vector<int>{ 7 } ) );
	scripts.Clear();
}
