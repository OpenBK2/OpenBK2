#include "../stdafx.h"

#include "../AIClassesID.h"
#include "../Aviation.h"
#include "../B2AI.h"
#include "../Commands.h"
#include "../DBAIConsts.h"
#include "../Diplomacy.h"
#include "../GlobalWarFog.h"
#include "../GroupLogic.h"
#include "../NewUpdater.h"
#include "../StatesFactory.h"
#include "../UnitCreation.h"
#include "../Units.h"
#include "../UnitStates.h"
#include "Common_RTS_AI/AIMap.h"
#include "Common_RTS_AI/CommonPathFinder.h"

#include <gtest/gtest.h>

namespace
{
class ReconFlight : public testing::Test
{
	CObj<CAIMap> map;
	CObj<IAILogic> logic;
	CObj<NDb::SAIGameConsts> constants;
	CObj<NDb::SMechUnitRPGStats> stats;
	CObj<NDb::SUnitActions> actions;
	CObj<NDb::SUnitSpecialAblityDesc> patrol;
	CMemoryStream savedUpdater;
	NTimer::STime oldTime = curTime;

protected:
	const CVec2 spawn = CVec2( 1200, 1600 );
	CObj<CAviation> plane;

	void SetUp() override
	{
		ASSERT_EQ( CastToObjectBaseImpl<CTank>( nullptr, static_cast<void *>( nullptr ) ), nullptr );
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &savedUpdater, SAVER_MODE_WRITE );
			saver->Add( 1, &updater );
		}
		logic = dynamic_cast<IAILogic *>( NObjectFactory::MakeObject( 0x1108D441 ) );
		NSingleton::RegisterSingleton( logic, IAILogic::tidTypeID );
		NSingleton::RegisterSingleton( new CCommonPathFinder(), CCommonPathFinder::tidTypeID );
		map = new CAIMap( 256, 256, 32, 2, 16 );
		SetAIMap( map );
		curTime = SConsts::AI_SEGMENT_DURATION;
		SConsts::Load();
		units.Init();
		theGroupLogic.Init( nullptr );
		theDipl.SetNPlayers( 2 );
		theWarFog.Init( 256, 256, 16, 2.0f );
		constants = dynamic_cast<NDb::SAIGameConsts *>( NObjectFactory::MakeObject( NDb::SAIGameConsts::typeID ) );
		theUnitCreation.SetConsts( constants );
		stats = dynamic_cast<NDb::SMechUnitRPGStats *>( NObjectFactory::MakeObject( NDb::SMechUnitRPGStats::typeID ) );
		actions = dynamic_cast<NDb::SUnitActions *>( NObjectFactory::MakeObject( NDb::SUnitActions::typeID ) );
		ASSERT_NE( stats, nullptr );
		ASSERT_NE( actions, nullptr );
		patrol = dynamic_cast<NDb::SUnitSpecialAblityDesc *>(
				NObjectFactory::MakeObject( NDb::SUnitSpecialAblityDesc::typeID ) );
		ASSERT_NE( patrol, nullptr );
		patrol->eName = NDb::ABILITY_PATROL;
		patrol->bStopCurrentAction = true;
		actions->specialAbilities.push_back( patrol.GetPtr() );
		// Stock scouts accept the reinforcement's Move order and ignore its Swarm order.
		actions->availCommands.SetData( ACTION_COMMAND_MOVE_TO );
		actions->availCommands.SetData( ACTION_COMMAND_PATROL );
		stats->pActions = actions;
		stats->fMaxHP = 100;
		// Po-2 flight data in simulation units (32 coordinates per metre, time in milliseconds).
		stats->fSpeed = 30.0f * 32.0f / 3600.0f;
		stats->fTurnRadius = 10.0f * 32.0f;
		stats->fMaxHeight = 50.0f * 32.0f;
		stats->fFuel = 3000;
	}

	void Spawn( NDb::EDBUnitRPGType type = NDb::DB_RPG_TYPE_AVIA_SCOUT )
	{
		stats->eDBtype = type;
		stats->etype = NDb::ReMapRPGType( type );
		plane = dynamic_cast<CAviation *>( NObjectFactory::MakeObject( AI_AVIATION ) );
		ASSERT_NE( plane, nullptr );
		plane->nUniqueID = 1;
		plane->Mem2UniqueIdObjs();
		units.AddUnitToUnits( plane, 0, stats->etype );
		plane->Init( spawn, 0, stats, 100, 0, 0, nullptr );
		units.AddUnitToMap( plane );
	}

	void Tick()
	{
		curTime += SConsts::AI_SEGMENT_DURATION;
		plane->Segment();
		plane->FirstSegment( SConsts::AI_SEGMENT_DURATION );
		plane->SecondSegment( SConsts::AI_SEGMENT_DURATION );
	}

	void TearDown() override
	{
		units.Clear();
		savedUpdater.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &savedUpdater, SAVER_MODE_READ );
		loader->Add( 1, &updater );
		theUnitCreation.SetConsts( nullptr );
		theGroupLogic.Clear();
		theWarFog.Clear();
		CLinkObject::Clear();
		plane = nullptr;
		SetAIMap( nullptr );
		NSingleton::UnRegisterSingleton( CCommonPathFinder::tidTypeID );
		NSingleton::UnRegisterSingleton( IAILogic::tidTypeID );
		curTime = oldTime;
	}

	void Order( EActionCommand action, const CVec2 &point )
	{
		CObj<CAICommand> command = dynamic_cast<CAICommand *>( NObjectFactory::MakeObject( 0x1108D44D ) );
		command->ToUnitCmd() = SAIUnitCmd( action, point );
		plane->UnitCommand( command, false, false );
	}

	void ExpectPatrol( const CVec2 &point )
	{
		// Once the plane reaches the area, check every tick for a return toward spawn.
		const float patrolRadius = 2.0f * stats->fTurnRadius;
		bool reached = false;
		int changes = 0;
		CVec2 lastWaypoint;
		for ( int i = 0; i < 2000; ++i )
		{
			Tick();
			const float distance = fabs( plane->GetCenterPlain() - point );
			reached = reached || distance <= patrolRadius;
			if ( !reached )
			{
				continue;
			}
			ASSERT_LT( distance, 2.2f * patrolRadius ) << "tick " << i;
			if ( !plane->GetState()->IsRestState() )
			{
				continue;
			}
			const CVec2 waypoint = plane->GetState()->GetPurposePoint();
			ASSERT_TRUE( map->IsPointInside( waypoint ) );
			if ( waypoint != lastWaypoint )
			{
				lastWaypoint = waypoint;
				++changes;
			}
		}
		EXPECT_TRUE( reached );
		EXPECT_GT( changes, 6 );
	}

	void ExpectMoveAtHandoff( bool bScout )
	{
		// A finished leg installs a straight continuation while the state still has its old waypoint.
		// Issue a Move at that boundary to exercise the flag from the previous maneuver.
		for ( int i = 0; i < 500; ++i )
		{
			const IManuver *pPrevious = plane->GetManuver();
			const CVec2 waypoint = plane->GetState()->GetPurposePoint();
			Tick();
			if ( pPrevious == plane->GetManuver() || waypoint != plane->GetState()->GetPurposePoint() )
			{
				continue;
			}
			const CVec2 requested( 6500, 5500 );
			const CVec2 groupShift( 100, -50 );
			plane->SetGroupShift( groupShift );
			CObj<CAICommand> move = dynamic_cast<CAICommand *>( NObjectFactory::MakeObject( 0x1108D44D ) );
			move->ToUnitCmd() = SAIUnitCmd( ACTION_COMMAND_MOVE_TO, requested );
			IStatesFactory *pFactory = plane->GetStatesFactory();
			plane->SetCurState( pFactory->ProduceState( plane, move ) );
			curTime += SConsts::AI_SEGMENT_DURATION;
			plane->GetState()->Segment();
			EXPECT_EQ( plane->IsCurCmdFinished(), !bScout );

			// Even an early handoff must retain a scout's order. Other aircraft keep their local idle pattern.
			CObj<IUnitState> rest = pFactory->ProduceRestState( plane );
			const CVec3 pos = plane->GetPosB2();
			const CVec2 expectedCenter = ( bScout ? requested : CVec2( pos.x, pos.y ) ) + groupShift;
			EXPECT_NEAR( fabs( rest->GetPurposePoint() - expectedCenter ), 2.0f * stats->fTurnRadius, 0.01f );
			return;
		}
		FAIL() << "The aircraft never completed a patrol leg";
	}
};

TEST_F( ReconFlight, ReinforcementStaysOverItsOnlyRequestedPoint )
{
	Spawn();
	const CVec2 point( 6500, 5500 );
	Order( ACTION_COMMAND_MOVE_TO, point );
	Order( ACTION_COMMAND_SWARM_TO, point );
	ExpectPatrol( point );
}

TEST_F( ReconFlight, PlacedScoutPatrolsItsPlacement )
{
	Spawn();
	ExpectPatrol( spawn );
}

TEST_F( ReconFlight, SinglePatrolPointNearMapEdge )
{
	Spawn();
	const CVec2 point( 20, 5000 );
	Order( ACTION_COMMAND_PATROL, point );
	ExpectPatrol( point );
}

TEST_F( ReconFlight, NearbyMoveStaysOverTheRequestedArea )
{
	Spawn();
	const CVec2 point = spawn - CVec2( 64, 128 );
	Order( ACTION_COMMAND_MOVE_TO, point );
	ExpectPatrol( point );
}

TEST_F( ReconFlight, ScoutMoveAtManeuverBoundaryKeepsDestination )
{
	Spawn();
	ExpectMoveAtHandoff( true );
}

TEST_F( ReconFlight, TransportRetainsItsOriginalHandoffAndLocalPatrol )
{
	Spawn( NDb::DB_RPG_TYPE_AVIA_LANDER );
	ExpectMoveAtHandoff( false );
}
} // namespace
