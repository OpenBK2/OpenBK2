#include "../stdafx.h"

#include "../AIClassesID.h"
#include "../CommonStates.h"
#include "../CommonUnit.h"
#include "../Technics.h"
#include "System/BinSaver.h"
#include "System/Streams.h"

#include <utility>

#include <gtest/gtest.h>

namespace
{
struct SCommonRestFields
{
	CVec2 guardPoint;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 3, &guardPoint );
		return 0;
	}
};

struct SMechanicalRestFields
{
	SCommonRestFields common;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &common );
		return 0;
	}
};

class RestStateMovement : public testing::Test
{
protected:
	static void SetUpTestSuite()
	{
		// Keep the real AILogic object registrations linked on Windows and ELF.
		// The explicit void overload is exported; the complete-type overload can be optimized away.
		ASSERT_EQ( CastToObjectBaseImpl<CTank>( nullptr, static_cast<void *>( nullptr ) ), nullptr );
	}
};

TEST_F( RestStateMovement, UnrelatedRestStatesDoNotImposeACommonGuardRadius )
{
	const std::pair<int, int> types[] = {
		{ AI_HELICOPTER, 0x1919A2C3 }, { AI_LOCOMOTIVE, 0x191BF340 }, { AI_AVIATION, 0x110993C0 } };
	for ( const auto &type : types )
	{
		SCOPED_TRACE( type.first );
		CObj<CCommonUnit> unit = dynamic_cast<CCommonUnit *>( NObjectFactory::MakeObject( type.first ) );
		CObj<IUnitState> state = dynamic_cast<IUnitState *>( NObjectFactory::MakeObject( type.second ) );
		ASSERT_NE( unit, nullptr );
		ASSERT_NE( state, nullptr );
		ASSERT_EQ( state->GetName(), EUSN_REST );
		ASSERT_EQ( dynamic_cast<CCommonRestState *>( state.GetPtr() ), nullptr );
		unit->SetCurState( state );
		// These are real small rest objects: no common-state fields exist beyond their allocation.
		EXPECT_TRUE( unit->CanGoToPoint( CVec2( 100000, 200000 ) ) );
		EXPECT_TRUE( unit->CanGoToPoint( CVec2( -100000, -200000 ) ) );
	}
}

TEST_F( RestStateMovement, MechanicalRestKeepsItsSavedGuardPointRestriction )
{
	CObj<CCommonUnit> unit = dynamic_cast<CCommonUnit *>( NObjectFactory::MakeObject( AI_TANK ) );
	CObj<IUnitState> state = dynamic_cast<IUnitState *>( NObjectFactory::MakeObject( 0x1508D494 ) );
	ASSERT_NE( unit, nullptr );
	ASSERT_NE( state, nullptr );
	const CVec2 guardPoint( 10000, 20000 );
	SMechanicalRestFields fields{ { guardPoint } };
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->Add( 1, &fields );
	}
	stream.Seek( 0 );
	{
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
		loader->AddPolymorphicBase( 1, state );
	}
	unit->SetCurState( state );
	EXPECT_TRUE( unit->CanGoToPoint( guardPoint ) );
	EXPECT_TRUE( unit->CanGoToPoint( guardPoint + CVec2( 1, 0 ) ) );
	EXPECT_FALSE( unit->CanGoToPoint( guardPoint + CVec2( 100000, 0 ) ) );
	EXPECT_FALSE( unit->CanGoToPoint( VNULL2 ) );
}
} // namespace
