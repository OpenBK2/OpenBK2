#include "../stdafx.h"

#include "../CommonCommands.h"
#include "../ScenarioTracker.h"
#include "AILogic/B2AI.h"
#include "Main/CommandsHistory.hpp"

#include <gtest/gtest.h>

namespace
{
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
	std::vector<SPlayerPresence> players = std::vector<SPlayerPresence>( 4 );

	int operator&( IBinSaver &saver )
	{
		saver.Add( 3, &players );
		return 0;
	}
};

void RestorePresentPlayers( IScenarioTracker *pTracker )
{
	// Restore four present players without loading a map. The real AI has no units.
	SScenarioPlayers scenario;
	CMemoryStream state;
	{
		CObj<IBinSaver> saver = CreateBinSaver( &state, SAVER_MODE_WRITE );
		saver->Add( 1, &scenario );
	}
	state.Seek( 0 );
	{
		CObj<IBinSaver> saver = CreateBinSaver( &state, SAVER_MODE_READ );
		saver->AddPolymorphicBase( 1, pTracker );
	}
}

class CReplayDropSingletons
{
public:
	CReplayDropSingletons( IScenarioTracker *pTracker, IAILogic *pAI )
	{
		NSingleton::RegisterSingleton( pTracker, IScenarioTracker::tidTypeID );
		NSingleton::RegisterSingleton( pAI, IAILogic::tidTypeID );
	}

	~CReplayDropSingletons()
	{
		NSingleton::UnRegisterSingleton( IAILogic::tidTypeID );
		NSingleton::UnRegisterSingleton( IScenarioTracker::tidTypeID );
	}
};
} // namespace

TEST( ReplayPlayerDrop, RecordedRemovalSurvivesExecutionScopeAndUpdatesEveryViewer )
{
	// Use multiplayer's actual creation and execution scope. Directly adding a new
	// command to history missed the temporary CObj invalidating it after execution.
	CObj<ICommandsHistory> history = dynamic_cast<ICommandsHistory *>( NObjectFactory::MakeObject( 0x1007AB80 ) );
	ASSERT_NE( history, nullptr );
	{
		CObj<IScenarioTracker> tracker = dynamic_cast<IScenarioTracker *>( NObjectFactory::MakeObject( 0x1117BB00 ) );
		CObj<IAILogic> ai = dynamic_cast<IAILogic *>( NObjectFactory::MakeObject( 0x1108D441 ) );
		ASSERT_NE( tracker, nullptr );
		ASSERT_NE( ai, nullptr );
		RestorePresentPlayers( tracker );
		tracker->SetLocalPlayer( 0 );
		CReplayDropSingletons singletons( tracker, ai );
		ASSERT_TRUE( tracker->IsPlayerPresent( 2 ) );
		CDropPlayerCommand::RecordAndExecute( history, 19470, 2 );
		EXPECT_FALSE( tracker->IsPlayerPresent( 2 ) );
	}
	CHistory recorded = history->GetCommandsHistory();
	ASSERT_EQ( recorded.size(), 1 );
	ASSERT_EQ( recorded.count( 19470 ), 1 );
	ASSERT_EQ( recorded.at( 19470 ).size(), 1 );
	ASSERT_TRUE( IsValid( recorded.at( 19470 ).front() ) );
	EXPECT_EQ( recorded.at( 19470 ).front()->GetDebugInfo(), "DropPlayer 2" );
	// Round-trip the retained command after the live removal has returned.
	CMemoryStream replay;
	{
		CObj<IBinSaver> saver = CreateBinSaver( &replay, SAVER_MODE_WRITE );
		saver->Add( 3, &recorded );
	}
	replay.Seek( 0 );
	CHistory loaded;
	{
		CObj<IBinSaver> saver = CreateBinSaver( &replay, SAVER_MODE_READ );
		saver->Add( 3, &loaded );
	}
	ASSERT_EQ( loaded.size(), 1 );
	ASSERT_EQ( loaded.count( 19470 ), 1 );
	ASSERT_EQ( loaded.at( 19470 ).size(), 1 );
	CPtr<IAILogicCommandB2> command = loaded.at( 19470 ).front();
	ASSERT_TRUE( IsValid( command ) );
	EXPECT_TRUE( command->NeedToBeStored() );
	EXPECT_EQ( command->GetDebugInfo(), "DropPlayer 2" );

	for ( int nViewer : { 0, 1, 2, 3 } )
	{
		CObj<IScenarioTracker> tracker = dynamic_cast<IScenarioTracker *>( NObjectFactory::MakeObject( 0x1117BB00 ) );
		CObj<IAILogic> ai = dynamic_cast<IAILogic *>( NObjectFactory::MakeObject( 0x1108D441 ) );
		ASSERT_NE( tracker, nullptr );
		ASSERT_NE( ai, nullptr );
		// Replay must update scenario presence as well as diplomacy for every viewer.
		RestorePresentPlayers( tracker );
		tracker->SetLocalPlayer( nViewer );
		CReplayDropSingletons singletons( tracker, ai );
		ASSERT_TRUE( tracker->IsPlayerPresent( 2 ) );
		command->Execute();
		EXPECT_FALSE( tracker->IsPlayerPresent( 2 ) );
		// Reapplying a removal must leave the other players present.
		command->Execute();
		for ( int nPlayer : { 0, 1, 3 } )
		{
			EXPECT_TRUE( tracker->IsPlayerPresent( nPlayer ) );
		}
	}
}
