#include "../stdafx.h"

#include "../Artillery.h"
#include "../GroupMover.h"
#include "Common_RTS_AI/StaticPath.h"

#include <gtest/gtest.h>

namespace
{
CCommonUnit *MakeUnit( int id )
{
	auto *unit = dynamic_cast<CCommonUnit *>( NObjectFactory::MakeObject( 0x1108D4AB ) );
	EXPECT_NE( unit, nullptr );
	if ( unit )
	{
		unit->nUniqueID = id;
	}
	return unit;
}

TEST( GroupMoverLifetime, QueuedMoveDiscardsAnExpiredMember )
{
	const CVec2 destination( 300, 500 );
	CObj<CGroupMover> mover = new CGroupMover( destination );
	CObj<CCommonUnit> survivor = MakeUnit( 1 );
	CObj<CCommonUnit> casualty = MakeUnit( 2 );
	ASSERT_NE( survivor, nullptr );
	ASSERT_NE( casualty, nullptr );
	mover->AddUnit( survivor );
	mover->AddUnit( casualty );
	CPtr<CCommonUnit> weakCasualty = casualty.GetPtr();
	// Releasing the last owner uses the same invalidation/reconstruction as corpse removal.
	casualty = nullptr;
	ASSERT_FALSE( IsValid( weakCasualty ) );
	ASSERT_EQ( weakCasualty->GetSubGroup(), -1 );

	EXPECT_EQ( mover->GetMoveTarget( survivor ), destination );
	EXPECT_EQ( survivor->GetSubGroup(), 0 );
	EXPECT_EQ( mover->GetMoveTarget( survivor ), destination );
}

TEST( GroupMoverLifetime, AnEntirelyExpiredGroupKeepsTheFallbackDestination )
{
	const CVec2 destination( 300, 500 );
	CObj<CGroupMover> mover = new CGroupMover( destination );
	CObj<CCommonUnit> casualty = MakeUnit( 3 );
	ASSERT_NE( casualty, nullptr );
	mover->AddUnit( casualty );
	casualty = nullptr;

	EXPECT_EQ( mover->GetMoveTarget( nullptr ), destination );
}
} // namespace
