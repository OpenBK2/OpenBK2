#include "../stdafx.h"

#include "../AIClassesID.h"
#include "../Building.h"
#include "../Diplomacy.h"
#include "../Guns.h"
#include "../Soldier.h"
#include "../Technics.h"
#include "../Turret.h"
#include "../UnitGuns.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <chrono>
#include <filesystem>
#include <functional>
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

class BuildingWeapons : public testing::Test
{
	static std::filesystem::path directory;
	std::vector<CObj<NDb::IObjMan>> records;
	std::vector<CObj<CObjectBase>> objects;
	std::vector<SBinSaverExternalObject> external;
	std::vector<CPtr<CUnitGuns>> slots;
	std::vector<CObj<CTurret>> turrets;
	NDb::SBuildingRPGStats *stats = nullptr;
	CPtr<CAIUnit> soldier;

protected:
	CPtr<CBuilding> building;
	std::vector<CPtr<CBasicGun>> mounted;
	std::vector<CPtr<SCommonGunInfo>> pools;

	static void SetUpTestSuite()
	{
		// Anchor the real AI and database factories on Windows and ELF.
		ASSERT_EQ( CastToObjectBaseImpl<CTank>( nullptr, static_cast<void *>( nullptr ) ), nullptr );
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() / ( "obk2-building-weapons-" + std::to_string( stamp ) );
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::filesystem::copy_file( BUILDING_WEAPONS_TYPES_FILE, directory / "types.xml" );
		const std::string root = directory.generic_string() + "/";
		NVFS::SetMainVFS( NVFS::CreateWinVFS( root ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( root ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
	}

	static void TearDownTestSuite()
	{
		NDb::CloseDatabase();
		NVFS::SetMainVFS( nullptr );
		NVFS::SetMainFileCreator( nullptr );
		std::error_code error;
		std::filesystem::remove_all( directory, error );
	}

	template <class T>
	T *Record( const char *type )
	{
		CObj<NDb::IObjMan> record = NDb::CreateNewObject( type );
		if ( !record )
		{
			ADD_FAILURE() << type;
			return nullptr;
		}
		static unsigned nextRecord = 0;
		const std::string name = std::string( type ) + std::to_string( nextRecord++ ) + ".xdb";
		EXPECT_TRUE( NDb::AddNewObject( name, CDBID( name ), record ) );
		records.push_back( record );
		return dynamic_cast<T *>( record->GetObject() );
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
			SFields fields{ visit };
			saver->Add( 1, &fields );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		saver->AddPolymorphicBase( 1, object );
	}

	void SetUp() override
	{
		theDipl.SetNPlayers( 2 );
		building = MakeObject<CBuilding>( 0x1108D450 );
		soldier = MakeObject<CAIUnit>( AI_INFANTRY );
		stats = Record<NDb::SBuildingRPGStats>( "BuildingRPGStats" );
		auto *infantry = Record<NDb::SInfantryRPGStats>( "InfantryRPGStats" );
		ASSERT_NE( stats, nullptr );
		ASSERT_NE( infantry, nullptr );
		stats->aiSlots.resize( 6 );
		stats->fMaxHP = 100;
		infantry->etype = NDb::RPG_TYPE_INFANTRY;
		CDBPtr<NDb::SInfantryRPGStats> infantryPointer = infantry;
		SFields fields{ [&]( IBinSaver &saver ) { saver.Add( 4, &infantryPointer ); } };
		LoadFields( soldier, [&]( IBinSaver &saver ) { saver.Add( 1, &fields ); } );
		slots.resize( stats->aiSlots.size() );
		turrets.resize( slots.size() );
		LoadBuilding( 100 );
	}

	void LoadBuilding( float hp )
	{
		CDBPtr<NDb::SBuildingRPGStats> statsPointer = stats;
		SFields existing{ [&]( IBinSaver &saver ) { saver.Add( 6, &hp ); } };
		SFields passability{ [&]( IBinSaver &saver ) { saver.Add( 1, &existing ); } };
		SFields fields{ [&]( IBinSaver &saver )
		                {
											saver.Add( 1, &passability );
											saver.Add( 14, &turrets );
											saver.Add( 15, &slots );
											saver.Add( 29, &statsPointer );
										} };
		LoadFields( building, [&]( IBinSaver &saver ) { saver.Add( 1, &fields ); } );
		// Set the same HP/alive pair as damage processing, without requiring a world map.
		static_cast<CExistingObject *>( building.GetPtr() )->CExistingObject::SetHitPoints( hp );
	}

	void AddWeapon( int slot, int ammo, int shells = 1, int shootState = 4 )
	{
		auto *weapon = Record<NDb::SWeaponRPGStats>( "WeaponRPGStats" );
		ASSERT_NE( weapon, nullptr );
		weapon->shells.resize( shells );
		for ( auto &shell : weapon->shells )
		{
			shell.nFireRate = 1000;
		}
		stats->aiSlots[slot].gun.pWeapon = weapon;
		stats->aiSlots[slot].gun.nAmmo = ammo;
		stats->aiSlots[slot].gun.nPriority = 1;
		CPtr<SCommonGunInfo> pool = MakeObject<SCommonGunInfo>( 0x1108D4C9 );
		pool->nAmmo = ammo;
		pool->nGun = slot;
		pool->bFiring = shootState != 4;
		pools.push_back( pool );
		std::vector<CObj<CBasicGun>> guns;
		for ( int shell = 0; shell < shells; ++shell )
		{
			CBasicGun *gun = MakeObject<CBasicGun>( 0x1108D4B6 );
			CDBPtr<NDb::SWeaponRPGStats> weaponPointer = weapon;
			SFields weaponFields{ [&]( IBinSaver &saver ) { saver.Add( 2, &weaponPointer ); } };
			int shotsLeft = 1;
			uint8_t shellIndex = shell;
			SFields basic{ [&]( IBinSaver &saver )
			               {
											 saver.Add( 1, &weaponFields );
											 saver.Add( 3, &shootState );
											 saver.Add( 6, &shotsLeft );
											 saver.Add( 12, &shellIndex );
											 saver.Add( 13, &soldier );
											 saver.Add( 15, &pool );
										 } };
			SFields base{ [&]( IBinSaver &saver ) { saver.Add( 1, &basic ); } };
			CDBPtr<NDb::SBuildingRPGStats> statsPointer = stats;
			SFields mount{ [&]( IBinSaver &saver )
			               {
											 saver.Add( 1, &statsPointer );
											 saver.Add( 2, &building );
											 saver.Add( 4, &slot );
										 } };
			LoadFields( gun,
			            [&]( IBinSaver &saver )
			            {
										saver.Add( 1, &base );
										saver.Add( 2, &mount );
									} );
			guns.push_back( gun );
			mounted.push_back( gun );
		}
		// Real mounted collections use the building slot as their common-gun index,
		// including empty entries before it. Preserve that legacy save representation.
		std::vector<CPtr<SCommonGunInfo>> common( slot + 1 );
		common[slot] = pool;
		std::vector<int> beginnings( slot + 2, 0 );
		beginnings.back() = shells;
		int count = slot + 1;
		SFields collection{ [&]( IBinSaver &saver )
		                    {
													saver.Add( 4, &common );
													saver.Add( 5, &guns );
													saver.Add( 6, &beginnings );
													saver.Add( 7, &count );
												} };
		slots[slot] = MakeObject<CUnitGuns>( 0x1108D4C7 );
		LoadFields( slots[slot], [&]( IBinSaver &saver ) { saver.Add( 1, &collection ); } );
		turrets[slot] = MakeObject<CTurret>( 0x1108D4B4 );
		LoadBuilding( 100 );
	}

	std::vector<SAINotifyRPGStats::SWeaponAmmo> ReportAmmo()
	{
		SAINotifyRPGStats report;
		building->GetRPGStats( &report );
		return report.ammo;
	}
};

std::filesystem::path BuildingWeapons::directory;

TEST_F( BuildingWeapons, EmptyBuildingHasNoMountedAmmo )
{
	EXPECT_TRUE( ReportAmmo().empty() );
}

TEST_F( BuildingWeapons, ThirdSlotReportsOnePoolAndOneRoundSpent )
{
	AddWeapon( 2, 100 );
	auto ammo = ReportAmmo();
	ASSERT_EQ( ammo.size(), 1 );
	EXPECT_EQ( ammo[0].nAmmo, 100 );
	mounted[0]->SetNAmmo( 99 );
	ammo = ReportAmmo();
	ASSERT_EQ( ammo.size(), 1 );
	EXPECT_EQ( ammo[0].nAmmo, 99 );
}

TEST_F( BuildingWeapons, EveryArmedSlotIsReportedOnceAcrossShellTypes )
{
	AddWeapon( 0, 40, 2 );
	AddWeapon( 2, 100, 3 );
	AddWeapon( 5, 70 );
	const auto ammo = ReportAmmo();
	ASSERT_EQ( ammo.size(), 3 );
	EXPECT_EQ( ammo[0].nAmmo, 40 );
	EXPECT_EQ( ammo[1].nAmmo, 100 );
	EXPECT_EQ( ammo[2].nAmmo, 70 );
}

TEST_F( BuildingWeapons, DestroyedBuildingStopsAnOccupantsPendingBurst )
{
	AddWeapon( 2, 100, 2, 3 );
	LoadBuilding( 0 );
	ASSERT_FALSE( building->IsAlive() );
	for ( CBasicGun *gun : mounted )
	{
		ASSERT_TRUE( gun->GetOwner()->IsAlive() );
		ASSERT_TRUE( gun->IsFiring() );
		gun->Segment();
		EXPECT_FALSE( gun->IsFiring() );
		EXPECT_FALSE( gun->IsAlive() );
		EXPECT_EQ( gun->GetNAmmo(), 100 );
	}
	EXPECT_FALSE( pools[0]->bFiring );
}

TEST_F( BuildingWeapons, IntactBuildingKeepsAnOccupantsPendingBurst )
{
	AddWeapon( 2, 100, 1, 3 );
	mounted[0]->Segment();
	EXPECT_TRUE( mounted[0]->IsFiring() );
	EXPECT_TRUE( mounted[0]->IsAlive() );
	EXPECT_EQ( mounted[0]->GetNAmmo(), 100 );
}

TEST_F( BuildingWeapons, DestructionCancelsEveryFiringPhaseBeforeRepair )
{
	// Turning, aiming, waiting for the animation's action point, and shooting.
	for ( int phase = 0; phase < 4; ++phase )
	{
		AddWeapon( phase, 100, 1, phase );
	}
	LoadBuilding( 0 );
	for ( CBasicGun *gun : mounted )
	{
		gun->Segment();
		EXPECT_FALSE( gun->IsFiring() );
		EXPECT_EQ( gun->GetNAmmo(), 100 );
	}
	LoadBuilding( 100 );
	for ( CBasicGun *gun : mounted )
	{
		gun->Segment();
		EXPECT_TRUE( gun->IsAlive() );
		EXPECT_FALSE( gun->IsFiring() );
		EXPECT_EQ( gun->GetNAmmo(), 100 );
	}
}
} // namespace
