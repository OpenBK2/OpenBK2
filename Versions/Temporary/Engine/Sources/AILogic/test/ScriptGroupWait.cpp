#include "../stdafx.h"

#include "../Formation.h"
#include "../Scripts.h"
#include "Script/lstate.h"
#include "Script/Script.h"
#include "Script/ScriptWrapper.h"
#include "System/BinSaver.h"
#include "System/RandomGen.h"
#include "System/Streams.h"

#include <functional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{
bool ready = false;
int attempts = 0;
int completions = 0;
std::vector<int> events;
std::vector<std::string> errors;

int WaitGate( lua_State *state )
{
	Script script( state );
	++attempts;
	EXPECT_EQ( script.GetTop(), 2 );
	EXPECT_EQ( script.GetObject( 1 ).GetInteger(), 17 );
	EXPECT_STREQ( script.GetObject( 2 ).GetString(), "retained argument" );
	if ( !ready )
	{
		return LUA_RETRY_NEXT_SEGMENT;
	}
	++completions;
	script.PushNumber( 23 );
	return 1;
}

int Capture( lua_State *state )
{
	Script script( state );
	events.push_back( script.GetObject( 1 ).GetInteger() );
	return 0;
}

int CaptureError( lua_State *state )
{
	Script script( state );
	const char *message = script.GetObject( 1 ).GetString();
	errors.emplace_back( message ? message : "missing error message" );
	return 0;
}

int EnterSynchronous( lua_State *state )
{
	// Model the guard luaD_call installs around synchronous native/Lua callbacks.
	state->nNoWait = 1;
	return 0;
}

int LeaveSynchronous( lua_State *state )
{
	state->nNoWait = 0;
	return 0;
}

const SRegFunction functions[] = { { "WaitGate", WaitGate },
                                   { "Capture", Capture },
                                   { "_ERRORMESSAGE", CaptureError },
                                   { "EnterSynchronous", EnterSynchronous },
                                   { "LeaveSynchronous", LeaveSynchronous },
                                   { "WaitForGroupInArea", CScripts::WaitForGroupInArea },
                                   { nullptr, nullptr } };

struct SFixtureFields
{
	std::function<void( IBinSaver & )> visit;

	int operator&( IBinSaver &saver )
	{
		visit( saver );
		return 0;
	}
};

class ScriptGroupWait : public testing::Test
{
protected:
	CScripts scripts;
	CObj<IScriptWrapper> vm;
	std::vector<CObj<CUpdatableObj>> objects;
	std::vector<SBinSaverExternalObject> external;
	CScriptGroups groups;
	det_map<std::string, NDb::SScriptArea> areas;

	void SetUp() override
	{
		ready = false;
		attempts = 0;
		completions = 0;
		events.clear();
		errors.clear();
		vm = CreateScriptWrapper();
		ASSERT_NE( vm.GetPtr(), nullptr );
		vm->Init();
		vm->AddRegFunctions( functions );
		NScript::RegisterCommonFunctionsToSaveLoad();
		NScript::AddScriptFunctionsToSaveLoad( functions );
		LoadGroups();
	}

	void TearDown() override
	{
		scripts.Clear();
	}

	void Run( const char *code )
	{
		ASSERT_EQ( vm->RunScript( code ), 1 );
		vm->Segment();
	}

	void LoadGroups()
	{
		// Populate the real script registry using its save representation, without loading a map.
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			SFixtureFields fields{ [&]( IBinSaver &s )
			                       {
															 s.Add( 3, &groups );
															 s.Add( 10, &areas );
														 } };
			saver->Add( 1, &fields );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		loader->Add( 1, &scripts );
	}

	CFormation *AddFormation( int group = 7 )
	{
		CFormation *unit = dynamic_cast<CFormation *>( NObjectFactory::MakeObject( 0x1108D498 ) );
		EXPECT_NE( unit, nullptr );
		if ( unit )
		{
			objects.push_back( unit );
			external.emplace_back( int( external.size() + 1 ), unit );
			groups[group].push_back( unit );
		}
		return unit;
	}

	void SetAlive( CUpdatableObj *object, bool alive )
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
			SFixtureFields fields{ [&]( IBinSaver &s )
			                       {
															 bool aliveSet = true;
															 s.Add( 2, &alive );
															 s.Add( 3, &aliveSet );
														 } };
			saver->Add( 1, &fields );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
		loader->Add( 1, object );
	}

	void SaveAndReload()
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
			saver->Add( 1, &vm );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
		loader->Add( 1, &vm );
	}
};

TEST_F( ScriptGroupWait, NativeWaitRetainsArgumentsAndRunsContinuationExactlyOnce )
{
	Run( "Capture(1); result = WaitGate(17, 'retained ' .. 'argument'); Capture(result); Capture(2);" );
	EXPECT_EQ( events, std::vector<int>( { 1 } ) );
	EXPECT_EQ( attempts, 1 );
	vm->Segment();
	EXPECT_EQ( attempts, 2 );
	EXPECT_EQ( completions, 0 );
	SaveAndReload();
	vm->Segment();
	EXPECT_EQ( attempts, 3 );
	ready = true;
	vm->Segment();
	EXPECT_EQ( events, std::vector<int>( { 1, 23, 2 } ) );
	EXPECT_EQ( completions, 1 );
	vm->Segment();
	EXPECT_EQ( completions, 1 );
	EXPECT_TRUE( errors.empty() );
}

TEST_F( ScriptGroupWait, TailCallWaitSurvivesSaveAndReturnsItsResult )
{
	Run( "function WaitTail() return WaitGate(17, 'retained argument'); end; Capture(WaitTail()); Capture(2);" );
	EXPECT_TRUE( events.empty() );
	SaveAndReload();
	vm->Segment();
	EXPECT_EQ( completions, 0 );
	ready = true;
	vm->Segment();
	EXPECT_EQ( events, std::vector<int>( { 23, 2 } ) );
	EXPECT_EQ( completions, 1 );
	EXPECT_TRUE( errors.empty() );
}

TEST_F( ScriptGroupWait, DirectNativeThreadWaitsWhileOtherThreadsAdvance )
{
	Run( "StartThread(WaitGate, 17, 'retained argument'); Capture(1);"
	     "function OtherThread() Sleep(4); Capture(2); end; StartThread(OtherThread);" );
	EXPECT_EQ( completions, 0 );
	SaveAndReload();
	vm->Segment();
	EXPECT_EQ( events, std::vector<int>( { 1, 2 } ) );
	EXPECT_EQ( completions, 0 );
	ready = true;
	vm->Segment();
	EXPECT_EQ( completions, 1 );
	vm->Segment();
	EXPECT_EQ( completions, 1 );
	EXPECT_TRUE( errors.empty() );
}

TEST_F( ScriptGroupWait, DirectNativeCoroutineCanWaitWhenItsCreatorIsSynchronous )
{
	Run( "EnterSynchronous(); StartThread(WaitGate, 17, 'retained argument');"
	     "LeaveSynchronous(); Capture(1);" );
	EXPECT_EQ( events, std::vector<int>( { 1 } ) );
	EXPECT_EQ( completions, 0 );
	SaveAndReload();
	ready = true;
	vm->Segment();
	EXPECT_EQ( completions, 1 );
	EXPECT_TRUE( errors.empty() );
}

TEST_F( ScriptGroupWait, SynchronousWaitReportsErrorInsteadOfContinuingPastCondition )
{
	Run( "EnterSynchronous(); WaitGate(17, 'retained argument'); Capture(1);" );
	EXPECT_TRUE( events.empty() );
	EXPECT_EQ( completions, 0 );
	ASSERT_EQ( errors.size(), 1 );
	EXPECT_NE( errors[0].find( "synchronous call" ), std::string::npos );
}

TEST_F( ScriptGroupWait, CircleWaitKeepsCallerSuspendedWithoutConsumingRandomness )
{
	ASSERT_NE( AddFormation(), nullptr );
	NDb::SScriptArea &area = areas["zone"];
	area.eType = NDb::EAT_CIRCLE;
	area.vCenter = CVec2( 100, 100 );
	area.fR = 5;
	LoadGroups();
	const uint64_t randomBefore = NRandom::GetRandomCallsCounter();
	Run( "WaitForGroupInArea(0, 7, 'zone'); Capture(1);" );
	EXPECT_TRUE( events.empty() );
	vm->Segment();
	EXPECT_TRUE( events.empty() );
	area.vCenter = VNULL2;
	LoadGroups();
	vm->Segment();
	EXPECT_EQ( events, std::vector<int>( { 1 } ) );
	EXPECT_EQ( NRandom::GetRandomCallsCounter(), randomBefore );
	EXPECT_TRUE( errors.empty() );
}

TEST_F( ScriptGroupWait, RectangleWaitUsesRectangleInsteadOfDefaultOriginCircle )
{
	ASSERT_NE( AddFormation(), nullptr );
	NDb::SScriptArea &area = areas["zone"];
	area.eType = NDb::EAT_RECTANGLE;
	area.vCenter = CVec2( 100, 100 );
	area.vAABBHalfSize = CVec2( 10, 20 );
	LoadGroups();
	Run( "WaitForGroupInArea(0, 7, 'zone'); Capture(1);" );
	EXPECT_TRUE( events.empty() );
	area.vCenter = VNULL2;
	LoadGroups();
	vm->Segment();
	EXPECT_EQ( events, std::vector<int>( { 1 } ) );
	EXPECT_TRUE( errors.empty() );
}

TEST_F( ScriptGroupWait, AbsentDeadAndNonUnitMembersDoNotBlockOrCrash )
{
	CFormation *dead = AddFormation();
	ASSERT_NE( dead, nullptr );
	SetAlive( dead, false );
	CUpdatableObj *box = dynamic_cast<CUpdatableObj *>( NObjectFactory::MakeObject( 0x1108D4A0 ) );
	ASSERT_NE( box, nullptr );
	objects.push_back( box );
	external.emplace_back( int( external.size() + 1 ), box );
	groups[7].push_back( box );
	groups[7].push_back( nullptr );
	LoadGroups();
	Run( "WaitForGroupInArea(0, 7, 100, 100, 5); Capture(1);"
	     "WaitForGroupInArea(0, 999, 100, 100, 5); Capture(2);" );
	EXPECT_EQ( events, std::vector<int>( { 1, 2 } ) );
	EXPECT_TRUE( errors.empty() );
}

TEST_F( ScriptGroupWait, FiltersOtherPlayersAndCompletesAfterWaitingGroupIsDestroyed )
{
	CFormation *unit = AddFormation();
	ASSERT_NE( unit, nullptr );
	LoadGroups();
	Run( "WaitForGroupInArea(1, 7, 100, 100, 5); Capture(1);"
	     "WaitForGroupInArea(0, 7, 100, 100, 5); Capture(2);" );
	EXPECT_EQ( events, std::vector<int>( { 1 } ) );
	SetAlive( unit, false );
	vm->Segment();
	EXPECT_EQ( events, std::vector<int>( { 1, 2 } ) );
	EXPECT_TRUE( errors.empty() );
}

TEST_F( ScriptGroupWait, InvalidAreaArgumentsReturnSafely )
{
	ASSERT_NE( AddFormation(), nullptr );
	LoadGroups();
	Run( "WaitForGroupInArea(0, 7, 'missing'); Capture(1);"
	     "WaitForGroupInArea(0, 7, nil); Capture(2);"
	     "WaitForGroupInArea(0, 7, 0); Capture(3);" );
	EXPECT_EQ( events, std::vector<int>( { 1, 2, 3 } ) );
}
} // namespace
