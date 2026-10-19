#include "../stdafx.h"

#include "../AIUnit.h"
#include "../CommonUnit.h"
#include "../UnitStates.h"
#include "../UnitTransfer.h"

#include <utility>

#include <gtest/gtest.h>

TEST( UnitTransferSelection, AttachedCrewStaysUnselectableAfterChangingOwner )
{
	// Use the real formation, crew state and ownership update without needing a running map.
	CObj<CCommonUnit> crew = dynamic_cast<CCommonUnit *>( NObjectFactory::MakeObject( 0x1108D498 ) );
	CObj<IUnitState> state = dynamic_cast<IUnitState *>( NObjectFactory::MakeObject( 0x1108D4BF ) );
	ASSERT_NE( crew.GetPtr(), nullptr );
	ASSERT_NE( state.GetPtr(), nullptr );
	ASSERT_EQ( state->GetName(), EUSN_GUN_CREW_STATE );
	crew->SetCurState( state );
	crew->SetSelectable( false, false );
	// Player 0 is the local player in the default diplomacy state.
	for ( int nRecipient : { 1, 0, 1, 0 } )
	{
		crew->ChangePlayer( nRecipient );
		ASSERT_EQ( crew->IsSelectable(), nRecipient == 0 );
		RestoreTransferredUnitSelection( crew );
		EXPECT_FALSE( crew->IsSelectable() );
		EXPECT_EQ( crew->GetPlayer(), nRecipient );
		EXPECT_EQ( crew->GetState(), state.GetPtr() );
	}
}

TEST( UnitTransferSelection, TowedGunsAndDescendingParatroopersKeepTheirSelectionRestrictions )
{
	const std::pair<int, int> types[] = {
		{ 0x1108D4AB, 0x1108D4A5 }, // Artillery and its being-towed state.
		{ 0x1108D498, 0x1108D4BD }  // Formation and its parachute-drop state.
	};
	for ( const auto &type : types )
	{
		CObj<CCommonUnit> unit = dynamic_cast<CCommonUnit *>( NObjectFactory::MakeObject( type.first ) );
		CObj<IUnitState> state = dynamic_cast<IUnitState *>( NObjectFactory::MakeObject( type.second ) );
		ASSERT_NE( unit.GetPtr(), nullptr );
		ASSERT_NE( state.GetPtr(), nullptr );
		unit->SetCurState( state );
		for ( bool bLocallyOwned : { false, true } )
		{
			unit->SetSelectable( bLocallyOwned, false );
			RestoreTransferredUnitSelection( unit );
			EXPECT_FALSE( unit->IsSelectable() );
			EXPECT_EQ( unit->GetState(), state.GetPtr() );
		}
		// Unhooking or landing can make the unit selectable again through the normal state transition.
		unit->SetCurState( nullptr );
		unit->SetSelectable( true, false );
		RestoreTransferredUnitSelection( unit );
		EXPECT_TRUE( unit->IsSelectable() );
	}
}

TEST( UnitTransferSelection, SelectionRefreshPreservesOrdinaryUnitsLocalOwnershipFlags )
{
	CObj<CCommonUnit> unit = dynamic_cast<CCommonUnit *>( NObjectFactory::MakeObject( 0x1108D498 ) );
	ASSERT_NE( unit.GetPtr(), nullptr );
	for ( int nRecipient : { 1, 0, 1, 0 } )
	{
		unit->ChangePlayer( nRecipient );
		RestoreTransferredUnitSelection( unit );
		EXPECT_EQ( unit->IsSelectable(), nRecipient == 0 );
	}
}

TEST( UnitTransferSelection, UnloadedInfantryDoesNotFollowItsFormerTransport )
{
	CObj<CAIUnit> transport = dynamic_cast<CAIUnit *>( NObjectFactory::MakeObject( 0x1108D445 ) );
	CObj<CAIUnit> soldier = dynamic_cast<CAIUnit *>( NObjectFactory::MakeObject( 0x1108D442 ) );
	ASSERT_NE( transport.GetPtr(), nullptr );
	ASSERT_NE( soldier.GetPtr(), nullptr );
	ASSERT_TRUE( soldier->IsFree() );
	// Reproduce SetFree's retained carrier pointer, then the cleared pointer left by saving.
	// Transfer collection and selection must agree in both cases, on every client.
	for ( CAIUnit *pFormerTransport : { transport.GetPtr(), static_cast<CAIUnit *>( nullptr ) } )
	{
		soldier->SetRestInside( false, pFormerTransport );
		EXPECT_EQ( soldier->GetContainingUnit(), nullptr );
		for ( bool bLocallyOwned : { false, true } )
		{
			soldier->SetSelectable( bLocallyOwned, false );
			RestoreTransferredUnitSelection( soldier );
			EXPECT_EQ( soldier->IsSelectable(), bLocallyOwned );
		}
	}
}

TEST( UnitTransferSelection, LoadedMechanicalCargoStaysWithItsCarrierUntilUnloaded )
{
	CObj<CAIUnit> transport = dynamic_cast<CAIUnit *>( NObjectFactory::MakeObject( 0x1108D445 ) );
	CObj<CAIUnit> cargo = dynamic_cast<CAIUnit *>( NObjectFactory::MakeObject( 0x1108D4AB ) );
	ASSERT_NE( transport.GetPtr(), nullptr );
	ASSERT_NE( cargo.GetPtr(), nullptr );
	cargo->SetRestInside( true, transport );
	// Mechanical cargo uses bRestInside; its base IsFree() remains true even while aboard.
	ASSERT_TRUE( cargo->IsFree() );
	EXPECT_EQ( cargo->GetContainingUnit(), transport.GetPtr() );
	cargo->SetSelectable( true, false );
	RestoreTransferredUnitSelection( cargo );
	EXPECT_FALSE( cargo->IsSelectable() );
	cargo->SetRestInside( false, nullptr );
	EXPECT_EQ( cargo->GetContainingUnit(), nullptr );
	cargo->SetSelectable( true, false );
	RestoreTransferredUnitSelection( cargo );
	EXPECT_TRUE( cargo->IsSelectable() );
}
