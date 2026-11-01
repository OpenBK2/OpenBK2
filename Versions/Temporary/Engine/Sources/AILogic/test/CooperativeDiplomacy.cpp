#include "../stdafx.h"

#include "../Diplomacy.h"
#include "Common_RTS_AI/Checksums.h"

#include <array>
#include <vector>

#include <gtest/gtest.h>

namespace
{
struct SSavedDiplomacy
{
	std::vector<int> parties{ 0, 0, 1, 0, 1, 2 };
	int nLocalPlayer = 0;
	bool bNetGame = true;
	std::vector<int> present{ 1, 1, 1, 1, 1, 1 };
	std::vector<int> bots{ 0, 0, 1, 0, 1, 1 };
	bool bHasRoles = true;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &parties );
		saver.Add( 2, &nLocalPlayer );
		saver.Add( 3, &bNetGame );
		saver.Add( 4, &present );
		if ( bHasRoles )
		{
			saver.Add( 5, &bots );
		}
		return 0;
	}
};

void RestoreDiplomacy( SSavedDiplomacy fields, CDiplomacy *pDiplomacy )
{
	// Exercise the real loader with both current and pre-COOP save layouts.
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->Add( 1, &fields );
	}
	stream.Seek( 0 );
	CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
	saver->Add( 1, pDiplomacy );
}

uint32_t GetRolesChecksum( CDiplomacy *pDiplomacy )
{
	uint32_t checksum = 1;
	{
		CPtr<IBinSaver> saver = CreateCheckSumSaver( &checksum, nullptr, 0 );
		saver->Add( 1, pDiplomacy );
	}
	return checksum;
}
} // namespace

TEST( CooperativeDiplomacy, BotRolesDoNotDependOnHumanOrReplayViewpoint )
{
	const std::array<bool, 6> expected{ false, false, true, false, true, false };
	for ( int nLocalPlayer = 0; nLocalPlayer < expected.size(); ++nLocalPlayer )
	{
		SSavedDiplomacy fields;
		fields.nLocalPlayer = nLocalPlayer;
		CDiplomacy diplomacy;
		RestoreDiplomacy( fields, &diplomacy );
		for ( int nPlayer = 0; nPlayer < expected.size(); ++nPlayer )
		{
			EXPECT_EQ( diplomacy.IsAIPlayer( nPlayer ), expected[nPlayer] )
					<< "viewpoint=" << nLocalPlayer << " player=" << nPlayer;
		}
		EXPECT_FALSE( diplomacy.IsAIPlayer( 255 ) );
	}
}

TEST( CooperativeDiplomacy, ClassicAndSinglePlayerKeepTheirExistingRoleRules )
{
	SSavedDiplomacy fields;
	fields.bots.clear();
	CDiplomacy diplomacy;
	RestoreDiplomacy( fields, &diplomacy );
	for ( int nPlayer = 0; nPlayer < fields.parties.size(); ++nPlayer )
	{
		EXPECT_FALSE( diplomacy.IsAIPlayer( nPlayer ) );
	}
	fields.bNetGame = false;
	fields.nLocalPlayer = 1;
	RestoreDiplomacy( fields, &diplomacy );
	EXPECT_TRUE( diplomacy.IsAIPlayer( 0 ) );
	EXPECT_FALSE( diplomacy.IsAIPlayer( 1 ) );
	EXPECT_TRUE( diplomacy.IsAIPlayer( 2 ) );
}

TEST( CooperativeDiplomacy, SaveLoadRetainsRolesAndOlderSavesClearThem )
{
	CDiplomacy original;
	RestoreDiplomacy( SSavedDiplomacy(), &original );
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->Add( 1, &original );
	}
	stream.Seek( 0 );
	CDiplomacy loaded;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
		saver->Add( 1, &loaded );
	}
	EXPECT_TRUE( loaded.IsAIPlayer( 2 ) );
	EXPECT_TRUE( loaded.IsAIPlayer( 4 ) );
	EXPECT_FALSE( loaded.IsAIPlayer( 5 ) );
	SSavedDiplomacy legacy;
	legacy.bHasRoles = false;
	RestoreDiplomacy( legacy, &loaded );
	EXPECT_FALSE( loaded.IsAIPlayer( 2 ) );
	EXPECT_FALSE( loaded.IsAIPlayer( 4 ) );
}

TEST( CooperativeDiplomacy, StartingAnotherMatchClearsPreviousBots )
{
	CDiplomacy diplomacy;
	RestoreDiplomacy( SSavedDiplomacy(), &diplomacy );
	diplomacy.SetNPlayers( 5 );
	EXPECT_FALSE( diplomacy.IsAIPlayer( 2 ) );
	EXPECT_FALSE( diplomacy.IsAIPlayer( 4 ) );
	RestoreDiplomacy( SSavedDiplomacy(), &diplomacy );
	diplomacy.Clear();
	EXPECT_FALSE( diplomacy.IsAIPlayer( 2 ) );
}

TEST( CooperativeDiplomacy, ChecksumsIncludeRolesButExcludeLocalViewpoint )
{
	SSavedDiplomacy fields;
	CDiplomacy first;
	RestoreDiplomacy( fields, &first );
	fields.nLocalPlayer = 1;
	CDiplomacy second;
	RestoreDiplomacy( fields, &second );
	EXPECT_EQ( GetRolesChecksum( &first ), GetRolesChecksum( &second ) );
	fields.bots[2] = 0;
	RestoreDiplomacy( fields, &second );
	EXPECT_NE( GetRolesChecksum( &first ), GetRolesChecksum( &second ) );
}
