#include "../stdafx.h"

#include "../AIClassesID.h"
#include "../AnimUnit.h"
#include "../Artillery.h"
#include "../Aviation.h"
#include "../B2AI.h"
#include "../Commands.h"
#include "../DBAIConsts.h"
#include "../Diplomacy.h"
#include "../GroupLogic.h"
#include "../Guns.h"
#include "../ShootEstimator.h"
#include "../StatesFactory.h"
#include "../StaticMembers.h"
#include "../Technics.h"
#include "../UnitGuns.h"
#include "../Units.h"
#include "../UnitStates.h"
#include "Common_RTS_AI/AIMap.h"
#include "Common_RTS_AI/CommonPathFinder.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <list>
#include <string>
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

class AggressiveAttacker : public testing::Test
{
	static std::filesystem::path directory;
	std::vector<CObj<NDb::IObjMan>> records;
	std::vector<CObj<CObjectBase>> objects;
	std::vector<SBinSaverExternalObject> external;
	CObj<CAIMap> map;
	CObj<NDb::SUnitStatsModifier> modifier;
	CPtr<CUnitGuns> guns;
	CPtr<CUnitGuns> emptyGuns;
	CPtr<IUnitState> rest;
	CDBPtr<NDb::SMechUnitRPGStats> stats;
	CDBPtr<NDb::SMechUnitRPGStats> aircraftStats;

protected:
	NDb::SExtendedStats *extended = nullptr;
	NDb::SWeaponRPGStats *weapon = nullptr;
	CPtr<CAIUnit> owner;
	CPtr<CAIUnit> enemy;
	CPtr<CBasicGun> gun;
	CPtr<SCommonGunInfo> ammo;
	CPtr<IShootEstimator> estimator;
	CPtr<IAnimUnit> ownerAnimation;
	std::list<CPtr<CAIUnit>> cachedTargets;
	NTimer::STime cacheDuration = 10000;

	virtual int GetOwnerType() const
	{
		return AI_TANK;
	}

	virtual int GetGunCeiling() const
	{
		return 0;
	}

	virtual bool UsesArtilleryGun() const
	{
		return false;
	}

	static void SetUpTestSuite()
	{
		ASSERT_EQ( CastToObjectBaseImpl<CTank>( nullptr, static_cast<void *>( nullptr ) ), nullptr );
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() / ( "obk2-aggressive-attacker-" + std::to_string( stamp ) );
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::filesystem::copy_file( AGGRESSIVE_ATTACKER_TYPES_FILE, directory / "types.xml" );
		std::ofstream( directory / "legacy-constants.xdb" ) << "<AIGameConsts/>";
		std::ofstream( directory / "entrenchment-constants.xdb" )
				<< "<AIGameConsts><DisableEntrenchmentReloadSlowdown>true</DisableEntrenchmentReloadSlowdown>"
					 "</AIGameConsts>";
		const std::string path = directory.generic_string() + "/";
		NVFS::SetMainVFS( NVFS::CreateWinVFS( path ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( path ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
		ASSERT_TRUE( NDb::RegisterResourceFile( "legacy-constants.xdb" ) );
		ASSERT_TRUE( NDb::RegisterResourceFile( "entrenchment-constants.xdb" ) );
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
	T *MakeRecord( const char *type )
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
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		loader->AddPolymorphicBase( 1, object );
	}

	void RoundTrip( CObjectBase *object )
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			saver->AddPolymorphicBase( 1, object );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		loader->AddPolymorphicBase( 1, object );
	}

	void InspectFields( CObjectBase *object, const std::function<void( IBinSaver & )> &visit )
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			saver->AddPolymorphicBase( 1, object );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		SFields fields{ visit };
		loader->Add( 1, &fields );
	}

	void PlaceUnit( CAIUnit *unit,
	                float distance = 0,
	                bool visible = true,
	                uint8_t player = 1,
	                CAICommand *command = nullptr,
	                bool entrenched = false )
	{
		// Load real units without starting movement, graphics or mission scripts.
		const bool aircraft = dynamic_cast<CAviation *>( unit ) != nullptr;
		CDBPtr<NDb::SMechUnitRPGStats> unitStats = aircraft ? aircraftStats : stats;
		CVec3 position( 80, 80 + distance, aircraft ? 40 : 0 );
		SVector tile( int( position.x / SConsts::TILE_SIZE ), int( position.y / SConsts::TILE_SIZE ) );
		SAIAngle direction( 0 );
		bool idle = true;
		float hp = 100;
		CPtr<CAICommand> current = command;
		CUnitStatsModifiers modifiers;
		modifiers.Set( "test.fixture", modifier );
		std::vector<int> visibility{ visible ? 1 : 0, 1, 0 };
		CPtr<CUnitGuns> unitGuns = unit == owner ? guns : emptyGuns;
		CPtr<IAnimUnit> animation = unit == owner ? ownerAnimation : nullptr;
		SFields path{ [&]( IBinSaver &saver )
		              {
										saver.Add( 3, &direction );
										saver.Add( 4, &direction );
										saver.Add( 9, &position );
										saver.Add( 10, &tile );
										saver.Add( 16, &idle );
									} };
		SFields queue{ [&]( IBinSaver &saver )
		               {
										 saver.Add( 2, &rest );
										 saver.Add( 4, &current );
									 } };
		SFields common{ [&]( IBinSaver &saver )
		                {
											saver.Add( 3, &queue );
											saver.Add( 16, &estimator );
											saver.Add( 20, &path );
										} };
		SFields ai{ [&]( IBinSaver &saver )
		            {
									saver.Add( 1, &common );
									saver.Add( 6, &player );
									saver.Add( 15, &hp );
									saver.Add( 41, &animation );
									saver.Add( 51, &visibility );
									saver.Add( 58, &modifiers );
									saver.Add( 71, &cachedTargets );
									saver.Add( 73, &cacheDuration );
									saver.Add( 74, &entrenched );
								} };
		SFields mechanical{ [&]( IBinSaver &saver )
		                    {
													saver.Add( 1, &ai );
													saver.Add( 2, &unitStats );
													saver.Add( 3, &unitGuns );
												} };
		if ( dynamic_cast<CArtillery *>( unit ) )
		{
			bool installed = true;
			float operable = 1;
			EActionNotify installAction = ACTION_NOTIFY_NONE;
			LoadFields( unit,
			            [&]( IBinSaver &saver )
			            {
										saver.Add( 1, &ai );
										saver.Add( 2, &unitStats );
										saver.Add( 4, &unitGuns );
										saver.Add( 6, &installAction );
										saver.Add( 7, &installAction );
										saver.Add( 8, &installAction );
										saver.Add( 9, &installed );
										saver.Add( 17, &operable );
									} );
		}
		else
		{
			LoadFields( unit,
			            [&]( IBinSaver &saver )
			            {
										saver.Add( 1, &mechanical );
										if ( aircraft )
										{
											saver.Add( 2, &unitStats );
											saver.Add( 7, &position );
										}
									} );
		}
		unit->SetBehaviourFire( SBehaviour::EFAtWill );
	}

	CAIUnit *MakeAircraft( float distance )
	{
		if ( !aircraftStats )
		{
			auto *planeStats = MakeRecord<NDb::SMechUnitRPGStats>( "MechUnitRPGStats" );
			planeStats->etype = NDb::RPG_TYPE_AVIA_FIGHTER;
			planeStats->fMaxHP = 100;
			planeStats->fPrice = 100;
			planeStats->vAABBHalfSize = CVec2( 4, 8 );
			aircraftStats = planeStats;
		}
		CAIUnit *plane = MakeObject<CAIUnit>( AI_AVIATION );
		PlaceUnit( plane, distance );
		return plane;
	}

	void SetUp() override
	{
		NSingleton::RegisterSingleton( MakeObject<IAILogic>( 0x1108D441 ), IAILogic::tidTypeID );
		NSingleton::RegisterSingleton( new CCommonPathFinder(), CCommonPathFinder::tidTypeID );
		map = new CAIMap( 256, 256, 32, 2, 16 );
		SetAIMap( map );
		units.Init();
		theDipl.SetNPlayers( 2 );
		theDipl.SetParty( 1, 1 );
		modifier = MakeRecord<NDb::SUnitStatsModifier>( "UnitStatsModifier" );
		external.emplace_back( 1000, modifier );
		owner = MakeObject<CAIUnit>( GetOwnerType() );
		enemy = MakeObject<CAIUnit>( AI_TANK );
		rest = MakeObject<IUnitState>( 0x1508D494 );
		guns = MakeObject<CUnitGuns>( 0x1108D4C7 );
		emptyGuns = MakeObject<CUnitGuns>( 0x1108D4C7 );
		gun = MakeObject<CBasicGun>( 0x1108D4B5 );
		ammo = MakeObject<SCommonGunInfo>( 0x1108D4C9 );
		estimator = MakeObject<IShootEstimator>( 0x1108D4DF );
		auto *unitStats = MakeRecord<NDb::SMechUnitRPGStats>( "MechUnitRPGStats" );
		extended = MakeRecord<NDb::SExtendedStats>( "ExtendedStats" );
		weapon = MakeRecord<NDb::SWeaponRPGStats>( "WeaponRPGStats" );
		ASSERT_NE( unitStats, nullptr );
		ASSERT_NE( extended, nullptr );
		ASSERT_NE( weapon, nullptr );
		unitStats->etype = NDb::RPG_TYPE_ARM_MEDIUM;
		unitStats->fSight = 1;
		unitStats->fSpeed = 10;
		unitStats->fRotateSpeed = 1;
		unitStats->fMaxHP = 100;
		unitStats->fPrice = 100;
		unitStats->vAABBHalfSize = CVec2( 4, 8 );
		auto *actions = MakeRecord<NDb::SUnitActions>( "UnitActions" );
		actions->availCommands.SetData( ACTION_COMMAND_ATTACK_UNIT );
		actions->availCommands.SetData( ACTION_COMMAND_GUARD );
		unitStats->pActions = actions;
		unitStats->pExtendedStats = extended;
		unitStats->platforms.resize( 1 );
		unitStats->platforms[0].guns.resize( 1 );
		unitStats->platforms[0].guns[0].pWeapon = weapon;
		stats = unitStats;
		weapon->fRangeMin = 10;
		weapon->fRangeMax = 160;
		weapon->nAmmoPerBurst = 1;
		weapon->nCeiling = GetGunCeiling();
		weapon->shells.resize( 1 );
		weapon->shells[0].etrajectory = NDb::SWeaponRPGStats::SShell::TRAJECTORY_HOWITZER;
		weapon->shells[0].fDamagePower = 30;
		weapon->shells[0].nPiercing = 100;
		weapon->shells[0].nPiercingRandom = 10;
		ammo->nAmmo = 10;
		CDBPtr<NDb::SWeaponRPGStats> weaponPointer = weapon;
		int gunType = IGunsFactory::VIS_CML_BALLIST_GUN;
		SFields gunStats{ [&]( IBinSaver &saver ) { saver.Add( 2, &weaponPointer ); } };
		SFields basicGun{ [&]( IBinSaver &saver )
		                  {
												saver.Add( 1, &gunStats );
												saver.Add( 7, &gunType );
												saver.Add( 13, &owner );
												saver.Add( 15, &ammo );
											} };
		LoadFields( gun, [&]( IBinSaver &saver ) { saver.Add( 1, &basicGun ); } );
		std::vector<CObj<CBasicGun>> gunArray{ gun.GetPtr() };
		std::vector<CPtr<SCommonGunInfo>> commonGuns{ ammo };
		std::vector<int> beginnings{ 0 };
		int count = 1;
		bool antiAircraft = weapon->nCeiling > 0;
		SFields gunCollection{ [&]( IBinSaver &saver )
		                       {
														 saver.Add( 2, &weapon->fRangeMax );
														 saver.Add( 3, &antiAircraft );
														 saver.Add( 4, &commonGuns );
														 saver.Add( 5, &gunArray );
														 saver.Add( 6, &beginnings );
														 saver.Add( 7, &count );
													 } };
		// This is a regular vehicle gun; artillery-only selection has separate range restrictions.
		int artilleryGun = UsesArtilleryGun() ? 0 : -1;
		LoadFields( guns,
		            [&]( IBinSaver &saver )
		            {
									saver.Add( 1, &gunCollection );
									saver.Add( 2, &artilleryGun );
								} );
		CDBPtr<NDb::SUnitBaseRPGStats> ratingStats = unitStats;
		LoadFields( estimator,
		            [&]( IBinSaver &saver )
		            {
									saver.Add( 2, &owner );
									saver.Add( 11, &ratingStats );
								} );
		PlaceUnit( owner, 0, true, 0 );
		PlaceUnit( enemy, 100 );
		extended->bAggressiveAttacker = true;
	}

	void TearDown() override
	{
		units.Clear();
		CLinkObject::Clear();
		NSingleton::UnRegisterSingleton( IAILogic::tidTypeID );
		objects.clear();
		SetAIMap( nullptr );
		NSingleton::UnRegisterSingleton( CCommonPathFinder::tidTypeID );
	}

	void ExpectTarget( CAIUnit *target )
	{
		SCOPED_TRACE( testing::Message() << "distance=" << fabs( enemy->GetCenterPlain() - owner->GetCenterPlain() )
		                                 << " height=" << enemy->GetZ() << " ceiling=" << weapon->nCeiling
		                                 << " ammo=" << ammo->nAmmo << " player=" << int( enemy->GetPlayer() )
		                                 << " visible=" << enemy->IsVisible( 0 ) );
		estimator->Reset( nullptr, false, 0 );
		estimator->AddUnit( enemy );
		EXPECT_EQ( estimator->GetBestUnit(), target );
		EXPECT_EQ( estimator->GetBestGun(), target ? gun.GetPtr() : nullptr );
	}

	IUnitState *StartAutomaticAttack( bool stationary = true, bool entrenched = false )
	{
		CPtr<CAICommand> command = MakeObject<CAICommand>( 0x1108D44D );
		command->ToUnitCmd().nCmdType = ACTION_COMMAND_ATTACK_UNIT;
		command->ToUnitCmd().nObjectID = 123;
		command->SetFromAI( true );
		if ( stationary )
		{
			command->SetStationaryAutoAttack();
		}
		enemy->nUniqueID = 123;
		enemy->Mem2UniqueIdObjs();
		PlaceUnit( owner, 0, true, 0, command, entrenched );
		IUnitState *attack = owner->GetStatesFactory()->ProduceState( owner, command );
		EXPECT_NE( attack, nullptr );
		if ( attack )
		{
			objects.push_back( attack );
			external.emplace_back( int( external.size() + 1 ), attack );
			owner->SetCurState( attack );
		}
		return attack;
	}

	void ExpectStationaryFiring( IUnitState *attack )
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			saver->AddPolymorphicBase( 1, attack );
		}
		int phase = -1;
		bool allowMove = true;
		bool stationary = false;
		CPtr<CBasicGun> selectedGun;
		SFields fields{ [&]( IBinSaver &saver )
		                {
											saver.Add( 3, &phase );
											saver.Add( 20, &selectedGun );
											saver.Add( 23, &allowMove );
											saver.Add( 24, &stationary );
										} };
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		loader->Add( 1, &fields );
		EXPECT_EQ( phase, 4 ); // ESAS_SIMPLE_FIRING, never a movement state.
		EXPECT_EQ( selectedGun.GetPtr(), gun.GetPtr() );
		EXPECT_FALSE( allowMove );
		EXPECT_TRUE( stationary );
	}

	void SetExplicitWait()
	{
		bool wait = true;
		NTimer::STime until = 10000;
		SFields base{ [&]( IBinSaver &saver )
		              {
										saver.Add( 8, &wait );
										saver.Add( 9, &until );
									} };
		LoadFields( rest, [&]( IBinSaver &saver ) { saver.Add( 1, &base ); } );
	}

	void PrepareGuard()
	{
		theGroupLogic.Init( nullptr );
		CPtr<CAICommand> command = MakeObject<CAICommand>( 0x1108D44D );
		command->ToUnitCmd().nCmdType = ACTION_COMMAND_GUARD;
		PlaceUnit( owner, 0, true, 0, command );
		owner->nUniqueID = 1;
		enemy->nUniqueID = 123;
		LoadFields( rest, [&]( IBinSaver &saver ) { saver.Add( 2, &owner ); } );
		CDecksSet<CObj<CAICommand>> queues( 128 );
		SFields fields{ [&]( IBinSaver &saver ) { saver.Add( 1, &queues ); } };
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			saver->Add( 1, &fields );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		CStaticMembers members;
		loader->Add( 1, &members );
		enemy->Mem2UniqueIdObjs();
	}
};

std::filesystem::path AggressiveAttacker::directory;

class EntrenchmentReload : public AggressiveAttacker
{
	NTimer::STime originalTime = 0;

protected:
	NDb::SAIGameConsts *constants = nullptr;

	void SetTime( NTimer::STime time )
	{
		// Restore the real simulation clock and settings without advancing the whole world.
		CDBPtr<NDb::SAIGameConsts> pointer = constants;
		CAIMap *aiMap = GetAIMap();
		LoadFields( Singleton<IAILogic>(),
		            [&]( IBinSaver &saver )
		            {
									saver.Add( 20, &pointer );
									saver.Add( 25, &time );
								} );
		SetAIMap( aiMap );
	}

	void SetUp() override
	{
		AggressiveAttacker::SetUp();
		// Serializing the bare AI also installs its map pointer; retain this fixture's map.
		CAIMap *aiMap = GetAIMap();
		InspectFields( Singleton<IAILogic>(), [&]( IBinSaver &saver ) { saver.Add( 25, &originalTime ); } );
		SetAIMap( aiMap );
		constants = MakeRecord<NDb::SAIGameConsts>( "AIGameConsts" );
		ASSERT_NE( constants, nullptr );
		EXPECT_FALSE( constants->bDisableEntrenchmentReloadSlowdown );
		SetTime( 10000 );
	}

	void TearDown() override
	{
		SetTime( originalTime );
		AggressiveAttacker::TearDown();
	}

	IUnitState *StartAttack( bool entrenched = true )
	{
		ownerAnimation = MakeObject<IAnimUnit>( AI_ANIM_UNIT_MECH );
		ownerAnimation->Init( owner );
		IUnitState *attack = StartAutomaticAttack( true, entrenched );
		// FireNow stops the real path. Aim along the fixture's gun direction and
		// leave time before the animation action point so no projectile world is needed.
		owner->CBasePathUnit::Init( owner->GetCenter(), 0, GetAIMap(), nullptr, nullptr );
		auto *unitStats =
				const_cast<NDb::SMechUnitRPGStats *>( dynamic_cast<const NDb::SMechUnitRPGStats *>( owner->GetStats() ) );
		unitStats->platforms[0].guns[0].wDirection =
				GetDirectionByVector( enemy->GetCenterPlain() - owner->GetCenterPlain() );
		unitStats->animdescs.resize( NDb::__ANIMATION_TYPE_COUNTER );
		for ( auto &animation : unitStats->animdescs )
		{
			animation.anims.resize( 1 );
			animation.anims[0].nAction = 1000;
			animation.anims[0].nLength = 2000;
		}
		weapon->nAimingTime = 2000;
		weapon->shells[0].nRelaxTime = 5000;
		// Initialize the gun's actual saved random factors to isolate the timing policy.
		float factor = 1;
		int rest = 4;
		int type = IGunsFactory::VIS_CML_BALLIST_GUN;
		CDBPtr<NDb::SWeaponRPGStats> weaponPointer = weapon;
		SFields stats{ [&]( IBinSaver &saver ) { saver.Add( 2, &weaponPointer ); } };
		SFields basic{ [&]( IBinSaver &saver )
		               {
										 saver.Add( 1, &stats );
										 saver.Add( 3, &rest );
										 saver.Add( 7, &type );
										 saver.Add( 10, &factor );
										 saver.Add( 11, &factor );
										 saver.Add( 13, &owner );
										 saver.Add( 15, &ammo );
									 } };
		LoadFields( gun, [&]( IBinSaver &saver ) { saver.Add( 1, &basic ); } );
		gun->CanShoot();
		attack->Segment();
		ExpectStationaryFiring( attack );
		attack->Segment();
		return attack;
	}

	void ExpectGun( int expectedPhase, bool expectedAim )
	{
		int phase = -1;
		bool aim = !expectedAim;
		SFields basic{ [&]( IBinSaver &saver )
		               {
										 saver.Add( 3, &phase );
										 saver.Add( 21, &aim );
									 } };
		InspectFields( gun, [&]( IBinSaver &saver ) { saver.Add( 1, &basic ); } );
		EXPECT_EQ( phase, expectedPhase );
		EXPECT_EQ( aim, expectedAim );
	}

	void ExpectRepeatTiming( bool fixed, bool entrenched = true, bool save = false )
	{
		IUnitState *attack = StartAttack( entrenched );
		ASSERT_NE( attack, nullptr );
		ExpectGun( 1, true ); // Every new attack still aims its first shot.
		SetTime( 11999 );
		gun->Segment();
		ExpectGun( 1, true );
		SetTime( 12000 );
		gun->Segment();
		ExpectGun( 2, true ); // The shot is ready, waiting for its animation action point.
		gun->StopFire();
		ammo->lastShoot = 12000;
		if ( save )
		{
			RoundTrip( attack );
			RoundTrip( gun );
		}
		attack->Segment();
		ExpectGun( 1, !fixed );
		SetTime( 16999 );
		gun->Segment();
		ExpectGun( 1, !fixed ); // The five-second reload is never skipped.
		SetTime( 17000 );
		gun->Segment();
		ExpectGun( fixed ? 2 : 1, !fixed );
		if ( !fixed )
		{
			SetTime( 19000 );
			gun->Segment();
			ExpectGun( 2, true ); // Legacy repeats pay the extra two-second aiming delay.
		}
	}
};

TEST_F( EntrenchmentReload, DefaultRetainsLegacyTiming )
{
	ExpectRepeatTiming( false );
}

TEST_F( EntrenchmentReload, DatabaseDefaultEnabledValueAndChecksumSurviveSaveLoad )
{
	const auto *legacy = NDb::Get<NDb::SAIGameConsts>( CDBID( "legacy-constants.xdb" ) );
	const auto *enabled = NDb::Get<NDb::SAIGameConsts>( CDBID( "entrenchment-constants.xdb" ) );
	ASSERT_NE( legacy, nullptr );
	ASSERT_NE( enabled, nullptr );
	EXPECT_FALSE( legacy->bDisableEntrenchmentReloadSlowdown );
	EXPECT_TRUE( enabled->bDisableEntrenchmentReloadSlowdown );
	// Old binary records omit chunk 20. Loading also invalidates the cached resource checksum.
	LoadFields( constants, []( IBinSaver & ) {} );
	EXPECT_FALSE( constants->bDisableEntrenchmentReloadSlowdown );
	const uint32_t legacyChecksum = constants->CalcCheckSum();
	bool disableSlowdown = true;
	LoadFields( constants, [&]( IBinSaver &saver ) { saver.Add( 20, &disableSlowdown ); } );
	EXPECT_NE( constants->CalcCheckSum(), legacyChecksum );
	RoundTrip( constants );
	EXPECT_TRUE( constants->bDisableEntrenchmentReloadSlowdown );
	LoadFields( constants, []( IBinSaver & ) {} );
	EXPECT_FALSE( constants->bDisableEntrenchmentReloadSlowdown );
	EXPECT_EQ( constants->CalcCheckSum(), legacyChecksum );
}

TEST_F( EntrenchmentReload, EnabledRemovesOnlyRepeatAimDelayAfterSaveLoad )
{
	constants->bDisableEntrenchmentReloadSlowdown = true;
	ExpectRepeatTiming( true, true, true );
}

TEST_F( EntrenchmentReload, EnabledDoesNotChangeUnentrenchedStationaryFire )
{
	constants->bDisableEntrenchmentReloadSlowdown = true;
	ExpectRepeatTiming( false, false );
}

TEST_F( EntrenchmentReload, MissingConstantsRetainLegacyTiming )
{
	constants = nullptr;
	SetTime( 10000 );
	ExpectRepeatTiming( false );
}

TEST_F( AggressiveAttacker, DefaultsAndSupportedClasses )
{
	CObj<NDb::SExtendedStats> defaults = new NDb::SExtendedStats;
	EXPECT_FALSE( defaults->bAggressiveAttacker );
	auto *mutableStats = const_cast<NDb::SUnitBaseRPGStats *>( owner->GetStats() );
	for ( const auto type : { NDb::RPG_TYPE_ARM_LIGHT,
	                          NDb::RPG_TYPE_ARM_MEDIUM,
	                          NDb::RPG_TYPE_ARM_HEAVY,
	                          NDb::RPG_TYPE_ARM_SUPER,
	                          NDb::RPG_TYPE_SPG_ASSAULT,
	                          NDb::RPG_TYPE_SPG_ANTITANK,
	                          NDb::RPG_TYPE_SPG_SUPER } )
	{
		mutableStats->etype = type;
		EXPECT_TRUE( owner->IsAggressiveAttacker() ) << type;
	}
	for ( const auto type :
	      { NDb::RPG_TYPE_SPG_AAGUN, NDb::RPG_TYPE_ART_HEAVY_GUN, NDb::RPG_TYPE_ART_GUN, NDb::RPG_TYPE_TRN_SUPPORT } )
	{
		mutableStats->etype = type;
		EXPECT_FALSE( owner->IsAggressiveAttacker() ) << type;
	}
	mutableStats->etype = NDb::RPG_TYPE_ARM_MEDIUM;
	extended->bAggressiveAttacker = false;
	EXPECT_FALSE( owner->IsAggressiveAttacker() );
}

TEST_F( AggressiveAttacker, UsesSharedVisibilityBeyondPersonalSight )
{
	ASSERT_GT( fabs( enemy->GetCenterPlain() - owner->GetCenterPlain() ), owner->GetSightRadius() );
	EXPECT_FLOAT_EQ( owner->GetTargetScanRadius(), 160 );
	ExpectTarget( enemy );
	PlaceUnit( enemy, 100, false );
	ExpectTarget( nullptr );
	PlaceUnit( enemy, 100, true, 0 );
	ExpectTarget( nullptr );
	PlaceUnit( enemy, 100, true, 2 );
	ExpectTarget( nullptr );
}

TEST_F( AggressiveAttacker, RejectsMinimumMaximumRangeAndEmptyAmmo )
{
	for ( const float distance : { 4.0f, 200.0f } )
	{
		PlaceUnit( enemy, distance );
		ExpectTarget( nullptr );
	}
	PlaceUnit( enemy, 100 );
	ExpectTarget( enemy );
	ammo->nAmmo = 0;
	ExpectTarget( nullptr );
}

TEST_F( AggressiveAttacker, ExplicitOrdersKeepTheirTargetingRules )
{
	EXPECT_TRUE( owner->UsesAggressiveTargeting() );
	CPtr<IUnitState> attack = MakeObject<IUnitState>( 0x1108D4DE );
	owner->SetCurState( attack );
	EXPECT_FALSE( owner->UsesAggressiveTargeting() );
	CPtr<CAICommand> command = MakeObject<CAICommand>( 0x1108D44D );
	command->SetStationaryAutoAttack();
	PlaceUnit( owner, 0, true, 0, command );
	owner->SetCurState( attack );
	EXPECT_TRUE( owner->UsesAggressiveTargeting() );
	RoundTrip( command );
	EXPECT_TRUE( command->IsStationaryAutoAttack() );
}

TEST_F( AggressiveAttacker, HoldFireSuppressesCachedTargets )
{
	ExpectTarget( enemy );
	owner->SetBehaviourFire( SBehaviour::EFNoFire );
	EXPECT_EQ( owner->AnalyzeTargetScan( nullptr, false, false ), 0 );
}

TEST_F( AggressiveAttacker, ExplicitWaitSuppressesCachedTargets )
{
	ExpectTarget( enemy );
	SetExplicitWait();
	EXPECT_FALSE( owner->UsesAggressiveTargeting() );
	EXPECT_EQ( owner->AnalyzeTargetScan( nullptr, false, false ), 0 );
}

TEST_F( AggressiveAttacker, UnreachableCachedTargetDoesNotMaskValidTarget )
{
	CPtr<CAIUnit> unreachable = MakeObject<CAIUnit>( AI_TANK );
	PlaceUnit( unreachable, 200 );
	cachedTargets = { unreachable, enemy };
	PlaceUnit( owner, 0, true, 0 );
	CAIUnit *target = nullptr;
	CBasicGun *selectedGun = nullptr;
	owner->LookForTarget( nullptr, false, &target, &selectedGun );
	EXPECT_EQ( target, enemy.GetPtr() );
	EXPECT_EQ( selectedGun, gun.GetPtr() );
}

TEST_F( AggressiveAttacker, FactoryCreatesStationaryAttackAndRangeLossEndsItAfterLoad )
{
	IUnitState *attack = StartAutomaticAttack();
	ASSERT_NE( attack, nullptr );
	attack->Segment();
	ASSERT_FALSE( owner->IsCurCmdFinished() );
	ExpectStationaryFiring( attack );
	RoundTrip( attack );
	ExpectStationaryFiring( attack );
	const CVec2 originalPosition = owner->GetCenterPlain();
	PlaceUnit( enemy, 200 );
	attack->Segment();
	EXPECT_TRUE( owner->IsCurCmdFinished() );
	EXPECT_EQ( owner->GetCenterPlain(), originalPosition );
}

TEST_F( AggressiveAttacker, HoldFireEndsActiveAutomaticAttack )
{
	IUnitState *attack = StartAutomaticAttack();
	ASSERT_NE( attack, nullptr );
	attack->Segment();
	owner->SetBehaviourFire( SBehaviour::EFNoFire );
	attack->Segment();
	EXPECT_TRUE( owner->IsCurCmdFinished() );
}

TEST_F( AggressiveAttacker, AcquisitionQueuesStationaryAttackWithoutExternalOrders )
{
	PrepareGuard();
	ExpectTarget( enemy );
	ASSERT_EQ( owner->AnalyzeTargetScan( nullptr, false, false ), 3 );
	CAICommand *command = owner->GetNextCommand();
	ASSERT_NE( command, nullptr );
	EXPECT_EQ( command->ToUnitCmd().nCmdType, ACTION_COMMAND_ATTACK_UNIT );
	EXPECT_EQ( command->ToUnitCmd().nObjectID, enemy->GetUniqueId() );
	EXPECT_TRUE( command->IsStationaryAutoAttack() );
	EXPECT_TRUE( command->IsFromAI() );
}

TEST_F( AggressiveAttacker, ExplicitAttackCanReplaceAutomaticAttackOnTheSameEnemy )
{
	IUnitState *attack = StartAutomaticAttack();
	ASSERT_NE( attack, nullptr );
	CPtr<CAICommand> command = MakeObject<CAICommand>( 0x1108D44D );
	command->ToUnitCmd().nCmdType = ACTION_COMMAND_ATTACK_UNIT;
	command->ToUnitCmd().nObjectID = enemy->GetUniqueId();
	EXPECT_EQ( attack->TryInterruptState( command ), TSIR_YES_IMMIDIATELY );
}

TEST_F( AggressiveAttacker, SavedAutomaticAttackFinishesWhenTargetBecomesHidden )
{
	CPtr<IUnitState> attack = MakeObject<IUnitState>( 0x1108D4DE );
	bool stationary = true;
	LoadFields( attack,
	            [&]( IBinSaver &saver )
	            {
								saver.Add( 14, &owner );
								saver.Add( 19, &enemy );
								saver.Add( 24, &stationary );
							} );
	RoundTrip( attack );
	PlaceUnit( enemy, 100, false );
	attack->Segment();
	EXPECT_TRUE( owner->IsCurCmdFinished() );
}

// Reuse the real weapon/state fixture to compare automatic aircraft fire with explicit orders.
class BlockLooseAirAttacks : public AggressiveAttacker
{
protected:
	int GetGunCeiling() const override
	{
		return 80;
	}

	void SetUp() override
	{
		AggressiveAttacker::SetUp();
		extended->bAggressiveAttacker = false;
		extended->bBlockLooseAirAttacks = true;
		weapon->nCeiling = 80;
		enemy = MakeAircraft( 100 );
	}

	void ExpectRangeLossEndsAttack()
	{
		IUnitState *attack = StartAutomaticAttack();
		ASSERT_NE( attack, nullptr );
		attack->Segment();
		ASSERT_FALSE( owner->IsCurCmdFinished() );
		ExpectStationaryFiring( attack );
		RoundTrip( owner->GetCurCmd() );
		RoundTrip( attack );
		ExpectStationaryFiring( attack );
		const CVec2 originalPosition = owner->GetCenterPlain();
		PlaceUnit( enemy, 200 );
		attack->Segment();
		EXPECT_TRUE( owner->IsCurCmdFinished() );
		EXPECT_EQ( owner->GetCenterPlain(), originalPosition );
	}
};

TEST_F( BlockLooseAirAttacks, DefaultsToFalseAndMissingExtendedStatsPreserveLegacyTargeting )
{
	CObj<NDb::SExtendedStats> defaults = new NDb::SExtendedStats;
	EXPECT_FALSE( defaults->bBlockLooseAirAttacks );
	PlaceUnit( enemy, 200 );
	owner->SetCurState( MakeObject<IUnitState>( 0x1108D4DE ) );
	ExpectTarget( nullptr );
	extended->bBlockLooseAirAttacks = false;
	EXPECT_FALSE( owner->BlocksLooseAirAttacks() );
	ExpectTarget( enemy );
	auto *stats =
			const_cast<NDb::SMechUnitRPGStats *>( dynamic_cast<const NDb::SMechUnitRPGStats *>( owner->GetStats() ) );
	stats->pExtendedStats = nullptr;
	EXPECT_FALSE( owner->BlocksLooseAirAttacks() );
	ExpectTarget( enemy );
}

TEST_F( BlockLooseAirAttacks, RequiresEnemyWithinRangeCeilingAndAmmo )
{
	ExpectTarget( enemy );
	PlaceUnit( enemy, 200 );
	ExpectTarget( nullptr );
	PlaceUnit( enemy, 100, true, 0 );
	ExpectTarget( nullptr );
	PlaceUnit( enemy, 100 );
	weapon->nCeiling = 20;
	ExpectTarget( nullptr );
	weapon->nCeiling = 80;
	ammo->nAmmo = 0;
	ExpectTarget( nullptr );
}

TEST_F( BlockLooseAirAttacks, GroundTargetsRetainLegacyRangeRules )
{
	enemy = MakeObject<CAIUnit>( AI_TANK );
	PlaceUnit( enemy, 200 );
	owner->SetCurState( MakeObject<IUnitState>( 0x1108D4DE ) );
	ExpectTarget( enemy );
}

TEST_F( BlockLooseAirAttacks, UnreachableCachedAircraftDoesNotMaskInRangeTarget )
{
	CPtr<CAIUnit> unreachable = MakeAircraft( 200 );
	cachedTargets = { unreachable, enemy };
	PlaceUnit( owner, 0, true, 0 );
	CAIUnit *target = nullptr;
	CBasicGun *selectedGun = nullptr;
	owner->LookForTarget( nullptr, false, &target, &selectedGun );
	EXPECT_EQ( target, enemy.GetPtr() );
	EXPECT_EQ( selectedGun, gun.GetPtr() );
}

TEST_F( BlockLooseAirAttacks, GlobalAircraftScanRejectsDistantPlanesAndQueuesInRangeFire )
{
	PrepareGuard();
	cacheDuration = 0;
	CPtr<CAIUnit> unreachable = MakeAircraft( 200 );
	unreachable->nUniqueID = 124;
	for ( CAIUnit *plane : { unreachable.GetPtr(), enemy.GetPtr() } )
	{
		units.AddUnitToUnits( plane, plane->GetPlayer(), plane->GetStats()->etype );
		units.AddUnitToMap( plane );
	}
	PlaceUnit( owner, 0, true, 0, owner->GetCurCmd() );
	CAIUnit *target = nullptr;
	CBasicGun *selectedGun = nullptr;
	owner->LookForTarget( nullptr, false, &target, &selectedGun );
	ASSERT_EQ( target, enemy.GetPtr() );
	EXPECT_EQ( selectedGun, gun.GetPtr() );
	ASSERT_EQ( owner->AnalyzeTargetScan( nullptr, false, false ), 3 );
	CAICommand *command = owner->GetNextCommand();
	ASSERT_NE( command, nullptr );
	EXPECT_TRUE( command->IsStationaryAutoAttack() );
	EXPECT_EQ( command->ToUnitCmd().nObjectID, enemy->GetUniqueId() );
}

TEST_F( BlockLooseAirAttacks, TankDropsAircraftAfterItLeavesRangeIncludingAfterLoad )
{
	ExpectRangeLossEndsAttack();
}

TEST_F( BlockLooseAirAttacks, HoldFireSuppressesCachedAndActiveAircraftAttacks )
{
	ExpectTarget( enemy );
	owner->SetBehaviourFire( SBehaviour::EFNoFire );
	EXPECT_EQ( owner->AnalyzeTargetScan( nullptr, false, false ), 0 );
	IUnitState *attack = StartAutomaticAttack();
	ASSERT_NE( attack, nullptr );
	attack->Segment();
	owner->SetBehaviourFire( SBehaviour::EFNoFire );
	attack->Segment();
	EXPECT_TRUE( owner->IsCurCmdFinished() );
}

TEST_F( BlockLooseAirAttacks, ExplicitAttackOverridesStationaryFireAndMayPursue )
{
	IUnitState *attack = StartAutomaticAttack();
	ASSERT_NE( attack, nullptr );
	CPtr<CAICommand> command = MakeObject<CAICommand>( 0x1108D44D );
	command->ToUnitCmd().nCmdType = ACTION_COMMAND_ATTACK_UNIT;
	command->ToUnitCmd().nObjectID = enemy->GetUniqueId();
	EXPECT_EQ( attack->TryInterruptState( command ), TSIR_YES_IMMIDIATELY );
	ASSERT_NE( StartAutomaticAttack( false ), nullptr );
	PlaceUnit( enemy, 200 );
	ASSERT_FALSE( owner->GetCurCmd()->IsStationaryAutoAttack() );
	ASSERT_TRUE( owner->CanMove() );
	ASSERT_TRUE( gun->CanShootToUnit( enemy ) );
	estimator->Reset( enemy, false, 0 );
	EXPECT_EQ( estimator->GetBestUnit(), enemy.GetPtr() );
	EXPECT_EQ( estimator->GetBestGun(), gun.GetPtr() );
}

class BlockLooseAirAttacksTransport : public BlockLooseAirAttacks
{
	int GetOwnerType() const override
	{
		return AI_TRANSPORT_UNIT;
	}
};

TEST_F( BlockLooseAirAttacksTransport, TransportFactoryPreservesStationaryAttack )
{
	ExpectRangeLossEndsAttack();
}

class BlockLooseAirAttacksArtillery : public BlockLooseAirAttacks
{
	int GetOwnerType() const override
	{
		return AI_ARTILLERY;
	}
};

TEST_F( BlockLooseAirAttacksArtillery, ZeroSetupArtilleryFactoryPreservesStationaryAttack )
{
	ExpectRangeLossEndsAttack();
}

class ZeroingInModifiers : public AggressiveAttacker
{
	int GetOwnerType() const override
	{
		return AI_ARTILLERY;
	}

	bool UsesArtilleryGun() const override
	{
		return true;
	}

protected:
	CPtr<IUnitState> StartZeroingIn( const CVec2 &point )
	{
		// Give Stop() a real path, while reusing the gun/range fixture above.
		owner->CBasePathUnit::Init( owner->GetCenter(), 0, GetAIMap(), nullptr, nullptr );
		auto *bonus = MakeRecord<NDb::SUnitStatsModifier>( "UnitStatsModifier" );
		bonus->weaponDamage.fMultBonus = 2;
		bonus->weaponRelaxTime.fMultBonus = 0;
		bonus->cover.fAddBonus = 0.25f;
		auto *ability = MakeRecord<NDb::SUnitSpecialAblityDesc>( "UnitSpecialAblityDesc" );
		ability->eName = NDb::ABILITY_ZEROING_IN;
		ability->pStatsBonus = bonus;
		ability->fParameter = 50;
		auto *actions = const_cast<NDb::SUnitActions *>( owner->GetStats()->GetActions() );
		actions->specialAbilities = { ability };
		CPtr<CAICommand> command = MakeObject<CAICommand>( 0x1108D44D );
		command->ToUnitCmd().nCmdType = ACTION_COMMAND_RANGE_AREA;
		command->ToUnitCmd().vPos = point;
		CPtr<IUnitState> state = owner->GetStatesFactory()->ProduceState( owner, command );
		owner->SetCurState( state );
		return state;
	}
};

TEST_F( ZeroingInModifiers, AppliesConfiguredBonusAndRemovesItOnInterruptionAfterSaveLoad )
{
	auto *other = MakeRecord<NDb::SUnitStatsModifier>( "UnitStatsModifier" );
	other->weaponDamage.fMultBonus = 3;
	owner->SetStatsModifier( "test.other", other );
	CPtr<IUnitState> state = StartZeroingIn( CVec2( 80, 180 ) );
	ASSERT_TRUE( state );
	EXPECT_EQ( owner->GetStatsModifier()->weaponDamage.Get( 10 ), 60 );
	EXPECT_EQ( owner->GetStatsModifier()->weaponRelaxTime.Get( 10 ), 0 );
	EXPECT_EQ( owner->GetStatsModifier()->cover.Get( 1 ), 1.25f );
	RoundTrip( state );
	RoundTrip( owner );
	// Loading the unit reconstructs its owned state; cancel the newly loaded instance.
	state = owner->GetState();
	ASSERT_TRUE( IsValid( state ) );
	EXPECT_EQ( owner->GetStatsModifier()->weaponDamage.Get( 10 ), 60 );
	EXPECT_EQ( state->TryInterruptState( nullptr ), TSIR_YES_IMMIDIATELY );
	EXPECT_EQ( owner->FindStatsModifier( NStatsModifierSource::Ability( NDb::ABILITY_ZEROING_IN ) ), nullptr );
	EXPECT_EQ( owner->GetStatsModifier()->weaponDamage.Get( 10 ), 30 );
	EXPECT_EQ( owner->GetStatsModifier()->weaponRelaxTime.Get( 10 ), 10 );
	EXPECT_EQ( owner->GetStatsModifier()->cover.Get( 1 ), 1 );
}

TEST_F( ZeroingInModifiers, RejectedOutOfRangeCommandDoesNotApplyBonus )
{
	CPtr<IUnitState> state = StartZeroingIn( CVec2( 80, 2000 ) );
	ASSERT_TRUE( state );
	EXPECT_EQ( owner->FindStatsModifier( NStatsModifierSource::Ability( NDb::ABILITY_ZEROING_IN ) ), nullptr );
	EXPECT_EQ( owner->GetStatsModifier()->weaponDamage.Get( 10 ), 10 );
	EXPECT_EQ( state->TryInterruptState( nullptr ), TSIR_YES_IMMIDIATELY );
}
} // namespace
