#include "../stdafx.h"

#include "../AckManager.h"
#include "../AIUnit.h"
#include "../Technics.h"
#include "Stats_B2_M1/AckTypes.h"

#include <gtest/gtest.h>

namespace
{
CObj<CAIUnit> MakeUnit( int nID )
{
	// Load the real AILogic factory on Windows as well as ELF.
	CastToUserObject( static_cast<CObjectBase *>( nullptr ), static_cast<CTank *>( nullptr ) );
	CObj<CAIUnit> unit = dynamic_cast<CAIUnit *>( NObjectFactory::MakeObject( 0x1108D442 ) );
	if ( unit )
	{
		unit->nUniqueID = nID;
	}
	return unit;
}
} // namespace

TEST( BoredAcknowledgments, DeliversSingletonRegistrationAndUnregistration )
{
	CObj<CAIUnit> unit = MakeUnit( 101 );
	ASSERT_NE( unit.GetPtr(), nullptr );
	CAckManager manager;
	SAIBoredAcknowledgement ack;
	EXPECT_FALSE( manager.UpdateAcknowledgment( ack ) );

	manager.RegisterAsBored( NDb::ACK_BORED_IDLE, unit );
	ASSERT_TRUE( manager.UpdateAcknowledgment( ack ) );
	EXPECT_EQ( ack.nObjUniqueID, 101 );
	EXPECT_EQ( ack.nAck, NDb::ACK_BORED_IDLE );
	EXPECT_TRUE( ack.bPresent );
	EXPECT_FALSE( manager.UpdateAcknowledgment( ack ) );

	manager.UnRegisterAsBored( NDb::ACK_BORED_IDLE, unit );
	ASSERT_TRUE( manager.UpdateAcknowledgment( ack ) );
	EXPECT_EQ( ack.nObjUniqueID, 101 );
	EXPECT_FALSE( ack.bPresent );
	EXPECT_FALSE( manager.UpdateAcknowledgment( ack ) );
}

TEST( BoredAcknowledgments, DrainsEveryEventAcrossCategories )
{
	CObj<CAIUnit> first = MakeUnit( 101 );
	CObj<CAIUnit> second = MakeUnit( 102 );
	ASSERT_NE( first.GetPtr(), nullptr );
	ASSERT_NE( second.GetPtr(), nullptr );
	CAckManager manager;
	manager.RegisterAsBored( NDb::ACK_BORED_ATTACK, first );
	manager.RegisterAsBored( NDb::ACK_BORED_ATTACK, second );
	manager.UnRegisterAsBored( NDb::ACK_BORED_IDLE, first );

	SAIBoredAcknowledgement ack;
	int nEvents = 0;
	while ( manager.UpdateAcknowledgment( ack ) )
	{
		++nEvents;
		ASSERT_LE( nEvents, 3 );
	}
	EXPECT_EQ( nEvents, 3 );
}

TEST( BoredAcknowledgments, SkipsCategoriesEmptiedByDeaths )
{
	CObj<CAIUnit> dead = MakeUnit( 101 );
	CObj<CAIUnit> survivor = MakeUnit( 102 );
	ASSERT_NE( dead.GetPtr(), nullptr );
	ASSERT_NE( survivor.GetPtr(), nullptr );
	CAckManager manager;
	manager.RegisterAsBored( NDb::ACK_BORED_ATTACK, dead );
	manager.RegisterAsBored( NDb::ACK_BORED_IDLE, survivor );
	manager.UnitDead( dead );

	SAIBoredAcknowledgement ack;
	ASSERT_TRUE( manager.UpdateAcknowledgment( ack ) );
	EXPECT_EQ( ack.nObjUniqueID, 102 );
	EXPECT_EQ( ack.nAck, NDb::ACK_BORED_IDLE );
	EXPECT_FALSE( manager.UpdateAcknowledgment( ack ) );
}
