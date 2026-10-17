#include "../stdafx.h"

#include "../PlayerCommandGroup.h"
#include "../TransferredUnitExperience.h"
#include "../UnitTransfer.h"

#include <algorithm>
#include <map>

#include <gtest/gtest.h>

namespace
{
struct STransferMatch
{
	IAIScenarioTracker::EGameType eGameType = IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL;
	bool bActive = true;
	std::vector<int> sides{ 0, 1, 0, 1, 2 };
	std::vector<bool> present{ true, true, true, true, true };

	IAIScenarioTracker::EGameType GetGameType() const
	{
		return eGameType;
	}

	bool IsMissionActive() const
	{
		return bActive;
	}

	int GetNPlayers() const
	{
		return sides.size();
	}

	bool IsPlayerPresent( int nPlayer ) const
	{
		return present.at( nPlayer );
	}

	int GetPlayerSide( int nPlayer ) const
	{
		return sides.at( nPlayer );
	}
};
} // namespace

TEST( UnitTransfer, OnlyOtherPresentPlayersOnTheSamePlayableSideCanReceiveUnits )
{
	STransferMatch match;
	const std::set<std::pair<int, int>> teammates{ { 0, 2 }, { 2, 0 }, { 1, 3 }, { 3, 1 } };
	for ( int nSource = -1; nSource <= match.GetNPlayers(); ++nSource )
	{
		for ( int nTarget = -1; nTarget <= match.GetNPlayers(); ++nTarget )
		{
			EXPECT_EQ( CanTransferUnitsToPlayer( &match, nSource, nTarget ), teammates.count( { nSource, nTarget } ) != 0 )
					<< "source=" << nSource << " target=" << nTarget;
		}
	}
	match.present[2] = false;
	EXPECT_FALSE( CanTransferUnitsToPlayer( &match, 0, 2 ) );
	EXPECT_FALSE( CanTransferUnitsToPlayer( &match, 2, 0 ) );
	match.sides[1] = -1;
	EXPECT_FALSE( CanTransferUnitsToPlayer( &match, 1, 3 ) );
	EXPECT_FALSE( CanTransferUnitsToPlayer( &match, 3, 1 ) );
}

TEST( UnitTransfer, DonationsRequireAnActiveMultiplayerMatch )
{
	STransferMatch match;
	match.eGameType = IAIScenarioTracker::EGT_SINGLE;
	EXPECT_FALSE( CanTransferUnitsToPlayer( &match, 0, 2 ) );
	match.eGameType = IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL;
	match.bActive = false;
	EXPECT_FALSE( CanTransferUnitsToPlayer( &match, 0, 2 ) );
	EXPECT_FALSE( CanTransferUnitsToPlayer<STransferMatch>( nullptr, 0, 2 ) );
}

TEST( UnitTransfer, SelectionOrderDuplicatesAndAttachmentCyclesHaveOneStableResult )
{
	// Transport, passenger, formation, gun and crew form a cyclic ownership group.
	const std::map<int, std::vector<int>> graph{
		{ 1, { 2, 4 } }, { 2, { 1, 3 } }, { 3, { 2 } }, { 4, { 1, 5 } }, { 5, { 4 } }, { 9, {} } };
	const auto inspect = [&graph]( int nID, std::vector<int> *pRelated )
	{
		const auto it = graph.find( nID );
		if ( it == graph.end() )
		{
			return false;
		}
		*pRelated = it->second;
		return true;
	};
	std::vector<int> selection{ 1, 2, 5, 9 };
	do
	{
		EXPECT_EQ( CollectUnitTransfer( selection, inspect ), ( std::vector<int>{ 1, 2, 3, 4, 5, 9 } ) );
	} while ( std::next_permutation( selection.begin(), selection.end() ) );
	EXPECT_EQ( CollectUnitTransfer( { 5, 5, 1, 2, -1, 100, 9 }, inspect ), ( std::vector<int>{ 1, 2, 3, 4, 5, 9 } ) );
}

TEST( UnitTransfer, InvalidOrForeignCargoRejectsItsWholeGroupWithoutBlockingIndependentUnits )
{
	const auto inspect = []( int nID, std::vector<int> *pRelated )
	{
		if ( nID == 1 )
		{
			*pRelated = { 2, 3 };
		}
		if ( nID == 2 )
		{
			*pRelated = { 1 };
		}
		return nID == 1 || nID == 2 || nID == 9;
	};
	EXPECT_EQ( CollectUnitTransfer( { 1, 2, 3, 9 }, inspect ), ( std::vector<int>{ 9 } ) );
	EXPECT_TRUE( CollectUnitTransfer( {}, inspect ).empty() );
}

TEST( UnitTransfer, QueuedDonorSelectionCannotReclaimPlanesAfterOwnershipChanges )
{
	struct SUnit
	{
		int nPlayer;
		bool bAlive = true;
		SUnit *pFormation = nullptr;

		bool IsAlive() const
		{
			return bAlive;
		}

		int GetPlayer() const
		{
			return nPlayer;
		}

		bool IsInFormation() const
		{
			return pFormation != nullptr;
		}

		SUnit *GetFormation() const
		{
			return pFormation;
		}
	};

	// Both clients execute the same queued registrations after transferring planes 10 and 11.
	// No client-local selectable flag participates in command ownership checks.
	for ( int nClient = 0; nClient < 2; ++nClient )
	{
		std::map<int, SUnit> units{ { 10, { 0 } }, { 11, { 0 } }, { 20, { 0 } }, { 30, { 2 } } };
		const auto resolve = [&units]( int nID ) -> SUnit *
		{
			const auto it = units.find( nID );
			return it == units.end() ? nullptr : &it->second;
		};
		const uint16_t wDonorGroup = ( ( 3 << 4 ) | 0 ) << 1;
		const uint16_t wRecipientGroup = ( ( 5 << 4 ) | 2 ) << 1;
		EXPECT_EQ( ResolvePlayerCommandGroup<SUnit>( { 10, 11, 20 }, wDonorGroup, resolve ),
		           ( std::vector<SUnit *>{ &units.at( 10 ), &units.at( 11 ), &units.at( 20 ) } ) );
		units.at( 10 ).nPlayer = 2;
		units.at( 11 ).nPlayer = 2;
		EXPECT_EQ( ResolvePlayerCommandGroup<SUnit>( { 10, 11, 20 }, wDonorGroup, resolve ),
		           ( std::vector<SUnit *>{ &units.at( 20 ) } ) );
		EXPECT_EQ( ResolvePlayerCommandGroup<SUnit>( { 10, 11, 30 }, wRecipientGroup, resolve ),
		           ( std::vector<SUnit *>{ &units.at( 10 ), &units.at( 11 ), &units.at( 30 ) } ) );
		units.at( 11 ).bAlive = false;
		units.at( 30 ).pFormation = &units.at( 20 );
		EXPECT_EQ( ResolvePlayerCommandGroup<SUnit>( { -1, 10, 11, 30 }, wRecipientGroup, resolve ),
		           ( std::vector<SUnit *>{ &units.at( 10 ) } ) );
	}
}

TEST( TransferredExperience, VeteranWaitsUntilRecipientSurpassesItsRetainedLevel )
{
	CTransferredUnitExperience experience;
	// The game's displayed levels 1..4 are stored as 0..3.
	experience.Transfer( 2, 0 );
	EXPECT_FALSE( experience.UpdateOwnerLevel( 1 ) );
	EXPECT_EQ( experience.GetLevel( 1 ), 2 );
	EXPECT_FALSE( experience.UpdateOwnerLevel( 2 ) );
	EXPECT_TRUE( experience.UpdateOwnerLevel( 3 ) );
	EXPECT_EQ( experience.GetLevel( 3 ), 3 );
	EXPECT_FALSE( experience.UpdateOwnerLevel( 3 ) );
}

TEST( TransferredExperience, LowerLevelDonationOnlyCatchesUpOnFutureRecipientLevelUp )
{
	CTransferredUnitExperience experience;
	experience.Transfer( 1, 2 );
	EXPECT_FALSE( experience.UpdateOwnerLevel( 2 ) );
	EXPECT_EQ( experience.GetLevel( 2 ), 1 );
	EXPECT_TRUE( experience.UpdateOwnerLevel( 3 ) );
	EXPECT_EQ( experience.GetLevel( 3 ), 3 );

	experience.Transfer( 1, 3 );
	for ( int i = 0; i < 10; ++i )
	{
		EXPECT_FALSE( experience.UpdateOwnerLevel( 3 ) );
		EXPECT_EQ( experience.GetLevel( 3 ), 1 );
	}
}

TEST( TransferredExperience, RedonationPreservesLevelAndResetsOnlyRecipientBaseline )
{
	CTransferredUnitExperience experience;
	EXPECT_FALSE( experience.IsTransferred() );
	EXPECT_EQ( experience.GetLevel( 3 ), 3 );
	experience.Transfer( 2, 0 );
	experience.Transfer( experience.GetLevel( 0 ), 3 );
	EXPECT_EQ( experience.GetLevel( 3 ), 2 );
	EXPECT_FALSE( experience.UpdateOwnerLevel( 3 ) );
	experience.Transfer( experience.GetLevel( 3 ), 1 );
	EXPECT_FALSE( experience.UpdateOwnerLevel( 2 ) );
	EXPECT_TRUE( experience.UpdateOwnerLevel( 3 ) );
}

TEST( TransferredExperience, SaveLoadPreservesRetainedLevelAndFutureUpgradeBaseline )
{
	CTransferredUnitExperience original;
	original.Transfer( 1, 3 );
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->Add( 1, &original );
	}
	stream.Seek( 0 );
	CTransferredUnitExperience loaded;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
		saver->Add( 1, &loaded );
	}
	EXPECT_TRUE( loaded.IsTransferred() );
	EXPECT_EQ( loaded.GetLevel( 3 ), 1 );
	EXPECT_FALSE( loaded.UpdateOwnerLevel( 3 ) );
}
