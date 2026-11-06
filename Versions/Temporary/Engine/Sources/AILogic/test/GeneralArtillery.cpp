#include "../stdafx.h"

#include "../Artillery.h"
#include "../Diplomacy.h"
#include "../GeneralArtillery.h"
#include "../GeneralInternal.h"
#include "../Technics.h"
#include "../UnitStates.h"

#include <cmath>
#include <functional>
#include <limits>
#include <list>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

namespace
{
struct SFields
{
	std::function<void( IBinSaver & )> visit;

	int operator&( IBinSaver &saver )
	{
		visit( saver );
		return 0;
	}
};

// Read the real task's saved state without exposing its internal container.
struct SBombardmentUnit
{
	CVec2 reserve;
	CVec2 attack;
	CPtr<CAIUnit> unit;
	CPtr<CGeneralArtilleryGoToPosition> returning;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 2, &reserve );
		saver.Add( 3, &attack );
		saver.Add( 4, &unit );
		saver.Add( 5, &returning );
		return 0;
	}
};

struct STaskState
{
	CPtr<CGeneralArtillery> owner;
	bool finished = false;
	NTimer::STime finishTime = ( std::numeric_limits<NTimer::STime>::max )();
	NTimer::STime acknowledgementTime = 0;
	CVec2 acknowledgementCenter;
	int state = 3; // EBS_FIRING: keep valid guns assigned without issuing another order.
	std::list<SBombardmentUnit> units;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 2, &owner );
		saver.Add( 3, &finished );
		saver.Add( 6, &finishTime );
		saver.Add( 7, &acknowledgementTime );
		saver.Add( 8, &acknowledgementCenter );
		saver.Add( 11, &state );
		saver.Add( 12, &units );
		return 0;
	}
};

class GeneralArtillery : public testing::Test
{
protected:
	std::vector<CObj<CObjectBase>> objects;
	std::vector<SBinSaverExternalObject> external;
	CGeneralArtillery *artillery = nullptr;

	template <class T>
	T *MakeObject( int type )
	{
		T *object = dynamic_cast<T *>( NObjectFactory::MakeObject( type ) );
		EXPECT_NE( object, nullptr );
		if ( object )
		{
			objects.push_back( object );
			external.emplace_back( int( external.size() + 1 ), object );
		}
		return object;
	}

	template <class T>
	void Visit( IBinSaver &saver, T *object )
	{
		if constexpr ( std::is_base_of_v<CObjectBase, T> )
		{
			saver.AddPolymorphicBase( 1, object );
		}
		else
		{
			saver.Add( 1, object );
		}
	}

	template <class TSource, class TTarget>
	void Copy( TSource *source, TTarget *target )
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			Visit( *saver, source );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		Visit( *loader, target );
	}

	void SetMode( bool net, int local )
	{
		std::vector<int> parties{ 0, 0, 0, 1, 0, 1, 2 };
		std::vector<int> present( 7, 1 );
		std::vector<int> bots{ 0, 0, 0, 1, 0, 1, 1 };
		SFields fields{ [&]( IBinSaver &saver )
		                {
											saver.Add( 1, &parties );
											saver.Add( 2, &local );
											saver.Add( 3, &net );
											saver.Add( 4, &present );
											saver.Add( 5, &bots );
										} };
		Copy( &fields, &theDipl );
	}

	void SetUp() override
	{
		SetMode( true, 0 );
		CPtr<CGeneral> general = MakeObject<CGeneral>( 0x1508D4B3 );
		ASSERT_NE( general, nullptr );
		int party = 1;
		int player = 3;
		int cursor = -1;
		SFields fields{ [&]( IBinSaver &saver )
		                {
											saver.Add( 3, &party );
											saver.Add( 24, &player );
											saver.Add( 25, &cursor );
										} };
		Copy( &fields, general.GetPtr() );
		artillery = MakeObject<CGeneralArtillery>( 0x1508D4AF );
		ASSERT_NE( artillery, nullptr );
		SFields owner{ [&]( IBinSaver &saver ) { saver.Add( 2, &general ); } };
		Copy( &owner, artillery );
	}

	void TearDown() override
	{
		external.clear();
		objects.clear();
		theDipl = CDiplomacy();
	}

	CAIUnit *MakeUnit( uint8_t player, CAIUnit *truck = nullptr, bool towed = false, bool transport = false )
	{
		CAIUnit *unit = MakeObject<CAIUnit>( transport ? 0x1108D445 : 0x1108D4AB );
		CPtr<CAIUnit> towingTruck = truck;
		CObj<IUnitState> state = towed ? MakeObject<IUnitState>( 0x1108D4A5 ) : nullptr;
		bool alive = true;
		int orientationVersion = 666;
		float hitPoints = 100;
		SFields updatable{ [&]( IBinSaver &saver )
		                   {
												 saver.Add( 2, &alive );
												 saver.Add( 3, &alive );
											 } };
		SFields link{ [&]( IBinSaver &saver ) { saver.Add( 1, &updatable ); } };
		SFields queue{ [&]( IBinSaver &saver ) { saver.Add( 2, &state ); } };
		SFields common{ [&]( IBinSaver &saver )
		                {
											saver.Add( 1, &link );
											saver.Add( 3, &queue );
											saver.Add( 17, &towingTruck );
											saver.Add( 30, &orientationVersion );
										} };
		SFields ai{ [&]( IBinSaver &saver )
		            {
									saver.Add( 1, &common );
									saver.Add( 6, &player );
									saver.Add( 15, &hitPoints );
								} };
		SFields mechanical{ [&]( IBinSaver &saver ) { saver.Add( 1, &ai ); } };
		SFields fields{ [&]( IBinSaver &saver ) { saver.Add( 1, transport ? &mechanical : &ai ); } };
		Copy( &fields, unit );
		EXPECT_TRUE( unit->IsAlive() );
		return unit;
	}

	STaskState RunTask( const std::vector<CAIUnit *> &units )
	{
		STaskState state;
		state.owner = artillery;
		for ( auto *unit : units )
		{
			SBombardmentUnit entry;
			entry.unit = unit;
			state.units.push_back( entry );
		}
		CGeneralArtilleryTask task;
		Copy( &state, &task );
		task.Segment();
		// Exercise the same decision again after restoring the actual task.
		Copy( &task, &task );
		task.Segment();
		Copy( &task, &state );
		return state;
	}
};

TEST_F( GeneralArtillery, CoopTaskUsesAssignedPlayerRegardlessOfLocalViewpoint )
{
	auto *owned = MakeUnit( 3 );
	auto *otherBot = MakeUnit( 5 );
	auto *human = MakeUnit( 0 );
	for ( int local : { 0, 2, 3, 5 } )
	{
		SCOPED_TRACE( local );
		SetMode( true, local );
		EXPECT_TRUE( artillery->CanControlUnit( owned ) );
		EXPECT_FALSE( artillery->CanControlUnit( otherBot ) );
		EXPECT_FALSE( artillery->CanControlUnit( human ) );
		const auto result = RunTask( { owned, otherBot, human } );
		ASSERT_EQ( result.units.size(), 1u );
		EXPECT_EQ( result.units.front().unit.GetPtr(), owned );
		EXPECT_FALSE( result.finished );
	}
}

TEST_F( GeneralArtillery, CoopTowedGunUsesTruckOwnerAndRejectsForeignTrucks )
{
	auto *ownTruck = MakeUnit( 3, nullptr, false, true );
	auto *foreignTruck = MakeUnit( 5, nullptr, false, true );
	auto *ownTowed = MakeUnit( 6, ownTruck, true );
	auto *foreignTowed = MakeUnit( 6, foreignTruck, true );
	auto *unattached = MakeUnit( 6, nullptr, true );
	for ( int local : { 0, 2, 3, 5 } )
	{
		SCOPED_TRACE( local );
		SetMode( true, local );
		const auto result = RunTask( { ownTowed, foreignTowed, unattached } );
		ASSERT_EQ( result.units.size(), 1u );
		EXPECT_EQ( result.units.front().unit.GetPtr(), ownTowed );
		EXPECT_FALSE( result.finished );
		const auto rejected = RunTask( { foreignTowed, unattached } );
		EXPECT_TRUE( rejected.units.empty() );
		EXPECT_TRUE( rejected.finished );
	}
}

TEST_F( GeneralArtillery, SinglePlayerRetainsSamePartyArtillery )
{
	SetMode( false, 0 );
	auto *owned = MakeUnit( 3 );
	auto *sameParty = MakeUnit( 5 );
	auto *enemy = MakeUnit( 0 );
	const auto result = RunTask( { owned, sameParty, enemy } );
	ASSERT_EQ( result.units.size(), 2u );
	auto entry = result.units.begin();
	EXPECT_EQ( entry++->unit.GetPtr(), owned );
	EXPECT_EQ( entry->unit.GetPtr(), sameParty );
}

TEST_F( GeneralArtillery, NoFiringGunsClearAcknowledgementWithoutInvalidCoordinates )
{
	for ( bool returning : { false, true } )
	{
		SCOPED_TRACE( returning );
		STaskState state;
		state.owner = artillery;
		state.acknowledgementTime = 123;
		state.acknowledgementCenter = CVec2( 10, 20 );
		if ( returning )
		{
			SBombardmentUnit entry;
			entry.unit = MakeUnit( 3 );
			entry.returning = MakeObject<CGeneralArtilleryGoToPosition>( 0x1508D4AB );
			state.units.push_back( entry );
		}
		CGeneralArtilleryTask task;
		Copy( &state, &task );
		task.CalculateTimeToSendAntiArtilleryAck();
		Copy( &task, &state );
		EXPECT_EQ( state.acknowledgementTime, 0u );
		EXPECT_TRUE( std::isfinite( state.acknowledgementCenter.x ) );
		EXPECT_TRUE( std::isfinite( state.acknowledgementCenter.y ) );
		EXPECT_EQ( state.acknowledgementCenter.x, 0 );
		EXPECT_EQ( state.acknowledgementCenter.y, 0 );
	}
}
} // namespace
