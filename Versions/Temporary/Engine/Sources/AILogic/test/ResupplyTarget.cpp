#include "../stdafx.h"

#include "../AIClassesID.h"
#include "../Formation.h"
#include "../FormationStates.h"
#include "../Soldier.h"
#include "../Technics.h"
#include "System/BinSaver.h"
#include "System/Streams.h"

#include <functional>
#include <vector>

#include <gtest/gtest.h>

namespace
{
struct SFixtureFields
{
	std::function<void( IBinSaver & )> visit;

	int operator&( IBinSaver &saver )
	{
		visit( saver );
		return 0;
	}
};

class ResupplyTarget : public testing::Test
{
	std::vector<CObj<CObjectBase>> objects;
	std::vector<SBinSaverExternalObject> external;

protected:
	static constexpr int finding = 1;
	static constexpr int servicing = 6;
	CObj<IUnitState> state;
	CPtr<CFormation> crew;
	CPtr<CFormation> squad;
	CPtr<CAITransportUnit> home;
	std::vector<CPtr<CSoldier>> soldiers;

	static void SetUpTestSuite()
	{
		// Keep the real AILogic object registrations linked on Windows and ELF.
		ASSERT_EQ( CastToObjectBaseImpl<CTank>( nullptr, static_cast<void *>( nullptr ) ), nullptr );
	}

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

	void LoadFields( CObjectBase *object, const std::function<void( IBinSaver & )> &visit )
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			SFixtureFields fields{ visit };
			saver->Add( 1, &fields );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		loader->AddPolymorphicBase( 1, object );
	}

	void SetSoldierSquad( CSoldier *soldier, CPtr<CFormation> formation )
	{
		// The infantry factory returns CInfantry, whose chunk 1 contains the CSoldier fields.
		SFixtureFields base{ [&]( IBinSaver &saver ) { saver.Add( 18, &formation ); } };
		LoadFields( soldier, [&]( IBinSaver &saver ) { saver.Add( 1, &base ); } );
		EXPECT_EQ( soldier->GetFormation(), formation.GetPtr() );
	}

	void SetUp() override
	{
		state = MakeObject<IUnitState>( 0x1108D476 );
		crew = MakeObject<CFormation>( 0x1108D498 );
		squad = MakeObject<CFormation>( 0x1108D498 );
		home = MakeObject<CAITransportUnit>( AI_TRANSPORT_UNIT );
		ASSERT_NE( state, nullptr );
		ASSERT_NE( crew, nullptr );
		ASSERT_NE( squad, nullptr );
		ASSERT_NE( home, nullptr );
		for ( int i = 0; i < 3; ++i )
		{
			CSoldier *soldier = MakeObject<CSoldier>( AI_INFANTRY );
			ASSERT_NE( soldier, nullptr );
			SetSoldierSquad( soldier, squad );
			soldiers.push_back( soldier );
		}
		LoadFields( squad, [&]( IBinSaver &saver ) { saver.Add( 24, &soldiers ); } );
		ASSERT_EQ( squad->Size(), 3 );
		ASSERT_EQ( soldiers[0]->GetFormation(), squad.GetPtr() );
	}

	void PrepareService( CAIUnit *target, int member )
	{
		int phase = servicing;
		float resources = 100;
		CPtr<CAIUnit> current = target;
		CVec2 position = target->GetCenterPlain();
		NTimer::STime lastService = -1000;
		SFixtureFields service{ [&]( IBinSaver &saver )
		                        {
															saver.Add( 2, &phase );
															saver.Add( 3, &home );
															saver.Add( 5, &resources );
														} };
		// Restore a finished target and the squad cache left by an earlier service operation.
		// No guns means service is complete; no map, database or target-selection mocks are needed.
		LoadFields( state,
		            [&]( IBinSaver &saver )
		            {
									saver.Add( 1, &service );
									saver.Add( 2, &crew );
									saver.Add( 3, &current );
									saver.Add( 4, &position );
									saver.Add( 5, &lastService );
									saver.Add( 6, &squad );
									saver.Add( 7, &member );
								} );
	}

	void ExpectService( int expectedPhase, CAIUnit *expectedTarget, CFormation *expectedSquad, int expectedMember )
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			saver->AddPolymorphicBase( 1, state );
		}
		int phase = -1;
		int member = -1;
		CPtr<CAIUnit> current;
		CPtr<CFormation> cachedSquad;
		SFixtureFields service{ [&]( IBinSaver &saver ) { saver.Add( 2, &phase ); } };
		SFixtureFields fields{ [&]( IBinSaver &saver )
		                       {
														 saver.Add( 1, &service );
														 saver.Add( 3, &current );
														 saver.Add( 6, &cachedSquad );
														 saver.Add( 7, &member );
													 } };
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		loader->Add( 1, &fields );
		EXPECT_EQ( phase, expectedPhase );
		EXPECT_EQ( current.GetPtr(), expectedTarget );
		EXPECT_EQ( cachedSquad.GetPtr(), expectedSquad );
		EXPECT_EQ( member, expectedMember );
	}
};

TEST_F( ResupplyTarget, FinishingAVehicleDoesNotResumeThePreviousInfantrySquad )
{
	for ( int type : { AI_TANK, AI_ARTILLERY, AI_LOCOMOTIVE } )
	{
		SCOPED_TRACE( type );
		CAIUnit *target = MakeObject<CAIUnit>( type );
		ASSERT_NE( target, nullptr );
		PrepareService( target, 0 );
		state->Segment();
		ExpectService( finding, target, nullptr, 0 );
	}
}

TEST_F( ResupplyTarget, MatchingSavedSquadContinuesWithItsNextMember )
{
	PrepareService( soldiers[0], 0 );
	state->Segment();
	ExpectService( servicing, soldiers[1], squad, 1 );
}

TEST_F( ResupplyTarget, MismatchedSavedMemberDoesNotRedirectService )
{
	for ( int member : { -1, 0, 3 } )
	{
		SCOPED_TRACE( member );
		PrepareService( soldiers[1], member );
		state->Segment();
		ExpectService( finding, soldiers[1], nullptr, 0 );
	}
}

TEST_F( ResupplyTarget, SoldierFromAnotherSquadDoesNotContinueTheSavedSquad )
{
	CSoldier *target = MakeObject<CSoldier>( AI_INFANTRY );
	ASSERT_NE( target, nullptr );
	SetSoldierSquad( target, crew );
	PrepareService( target, 0 );
	state->Segment();
	ExpectService( finding, target, nullptr, 0 );
}
} // namespace
