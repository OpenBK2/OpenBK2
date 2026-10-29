#include "../stdafx.h"

#include "../AIClassesID.h"
#include "../Artillery.h"
#include "../Commands.h"
#include "../Diplomacy.h"
#include "../GlobalWarFog.h"
#include "../StatesFactory.h"
#include "../Technics.h"
#include "../Turret.h"
#include "../UnitGuns.h"
#include "../UnitStates.h"
#include "Common_RTS_AI/AIMap.h"
#include "Common_RTS_AI/CommonPathFinder.h"
#include "Common_RTS_AI/Terrain.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <chrono>
#include <filesystem>
#include <functional>
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

class MechBinoculars : public testing::Test
{
	static std::filesystem::path directory;
	std::vector<CObj<NDb::IObjMan>> records;
	std::vector<CObj<CObjectBase>> objects;
	CObj<CAIMap> map;
	CObj<NDb::SUnitStatsModifier> modifier;
	CPtr<CUnitGuns> guns;

protected:
	std::vector<SBinSaverExternalObject> external;
	CPtr<CAIUnit> unit;
	CPtr<CTurret> turret;
	CObj<IUnitState> state;
	NDb::SMechUnitRPGStats *stats = nullptr;
	NDb::SExtendedStats *extended = nullptr;
	const CVec3 position = CVec3( 1024, 1024, 0 );

	static void SetUpTestSuite()
	{
		ASSERT_EQ( CastToObjectBaseImpl<CTank>( nullptr, static_cast<void *>( nullptr ) ), nullptr );
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() / ( "obk2-mech-binoculars-" + std::to_string( stamp ) );
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::filesystem::copy_file( MECH_BINOCULARS_TYPES_FILE, directory / "types.xml" );
		const std::string path = directory.generic_string() + "/";
		NVFS::SetMainVFS( NVFS::CreateWinVFS( path ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( path ) );
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

	void SetUp() override
	{
		NSingleton::RegisterSingleton( new CCommonPathFinder(), CCommonPathFinder::tidTypeID );
		map = new CAIMap( 128, 128, 32, 2, 16 );
		SetAIMap( map );
		theWarFog.Init( 128, 128, 32, 2.0f );
		theDipl.SetNPlayers( 2 );
		theDipl.SetParty( 1, 1 );
		modifier = new NDb::SUnitStatsModifier;
		external.emplace_back( 1000, modifier );
		guns = MakeObject<CUnitGuns>( 0x1108D4C7 );
		turret = MakeObject<CTurret>( 0x1108D4B3 );
		stats = MakeRecord<NDb::SMechUnitRPGStats>( "MechUnitRPGStats" );
		extended = MakeRecord<NDb::SExtendedStats>( "ExtendedStats" );
		ASSERT_NE( stats, nullptr );
		ASSERT_NE( extended, nullptr );
		stats->fSight = 4;
		stats->fSpeed = 10;
		stats->fRotateSpeed = 100;
		stats->fMaxHP = 100;
		stats->vAABBHalfSize = CVec2( 12, 24 );
		stats->nAIPassabilityClass = EAC_TRACK;
		stats->platforms.resize( 2 );
		stats->platforms[0].szRotatePoint = "hull";
		stats->platforms[1].szRotatePoint = "turret";
		stats->pExtendedStats = extended;
		MakeUnit( AI_TANK, NDb::RPG_TYPE_ARM_MEDIUM );
	}

	void TearDown() override
	{
		theWarFog.Clear();
		state = nullptr;
		objects.clear();
		SetAIMap( nullptr );
		NSingleton::UnRegisterSingleton( CCommonPathFinder::tidTypeID );
	}

	void MakeUnit( int factory, NDb::EUnitRPGType type )
	{
		unit = MakeObject<CAIUnit>( factory );
		stats->etype = type;
		CDBPtr<NDb::SMechUnitRPGStats> statsPointer = stats;
		CPtr<NDb::SUnitStatsModifier> modifierPointer = modifier.GetPtr();
		std::vector<CObj<CTurret>> turrets{ turret.GetPtr() };
		uint8_t player = 1;
		float hp = 100;
		SUnitProfile profile( CVec2( position.x, position.y ), 16 );
		SFields ai{ [&]( IBinSaver &saver )
		            {
									saver.Add( 6, &player );
									saver.Add( 15, &hp );
									saver.Add( 58, &modifierPointer );
									saver.Add( 69, &profile );
								} };
		SFields mechanical{ [&]( IBinSaver &saver )
		                    {
													saver.Add( 1, &ai );
													saver.Add( 2, &statsPointer );
													if ( factory == AI_ARTILLERY )
													{
														bool installed = true;
														float operable = 1;
														EActionNotify action = ACTION_NOTIFY_NONE;
														saver.Add( 4, &guns );
														saver.Add( 5, &turrets );
														saver.Add( 6, &action );
														saver.Add( 7, &action );
														saver.Add( 9, &installed );
														saver.Add( 17, &operable );
													}
													else
													{
														saver.Add( 3, &guns );
														saver.Add( 4, &turrets );
													}
												} };
		LoadFields( unit,
		            [&]( IBinSaver &saver )
		            {
									if ( factory == AI_ARTILLERY )
									{
										mechanical.visit( saver );
									}
									else
									{
										saver.Add( 1, &mechanical );
									}
								} );
		// Real paths and collisions let the ability stop and turn without a mock movement implementation.
		unit->CBasePathUnit::Init( position, 0, map, nullptr, nullptr );
		SetTurretLimit( 32768 );
	}

	void SetTurretLimit( uint16_t limit )
	{
		SAIAngle constraint( limit );
		bool canRotate = true;
		int platform = 1;
		LoadFields( turret,
		            [&]( IBinSaver &saver )
		            {
									saver.Add( 2, &unit );
									saver.Add( 3, &platform );
									saver.Add( 4, &constraint );
									saver.Add( 7, &canRotate );
								} );
		turret->CTurret::TurnHor( 0, true );
	}

	void Start( uint16_t direction = 0 )
	{
		CPtr<CAICommand> command = MakeObject<CAICommand>( 0x1108D44D );
		command->ToUnitCmd().nCmdType = ACTION_COMMAND_USE_SPYGLASS;
		command->ToUnitCmd().vPos = CVec2( position.x, position.y ) + GetVectorByDirection( direction ) * 500.0f;
		ASSERT_TRUE( unit->GetStatesFactory()->CanCommandBeExecuted( command ) );
		state = unit->GetStatesFactory()->ProduceState( unit, command );
		ASSERT_NE( state, nullptr );
		unit->SetCurState( state );
		theWarFog.AddUnit( unit->GetUniqueId(), Info(), 1 );
	}

	SWarFogUnitInfo Info() const
	{
		SWarFogUnitInfo info;
		unit->GetFogInfo( &info );
		return info;
	}

	void Step()
	{
		state->Segment();
		theWarFog.Segment();
	}

	bool Visible( uint16_t direction, int radius ) const
	{
		const CVec2 point = CVec2( position.x, position.y ) / SConsts::TILE_SIZE +
		                    GetVectorByDirection( direction ) * float( radius * AI_TILES_IN_VIS_TILE );
		return theWarFog.IsTileVisible( SVector( int( std::lround( point.x ) ), int( std::lround( point.y ) ) ), 1 );
	}

	void ExpectCone( uint16_t facing )
	{
		// Default infantry Binoculars use a 5000-unit half angle in the engine's 16-bit angle scale.
		constexpr uint16_t halfAngle = 5000;
		const auto info = Info();
		ASSERT_FALSE( info.specialSights.empty() );
		EXPECT_EQ( info.specialSights[0].sector.wStartAngle, uint16_t( facing - halfAngle ) );
		EXPECT_EQ( info.specialSights[0].sector.wEndAngle, uint16_t( facing + halfAngle ) );
	}
};

std::filesystem::path MechBinoculars::directory;

TEST_F( MechBinoculars, DefaultEmptyAndDataRoundTrip )
{
	EXPECT_TRUE( extended->szBinocularsPlatform.empty() );
	const auto checksum = extended->CalcCheckSum();
	extended->szBinocularsPlatform = "turret";
	EXPECT_NE( extended->CalcCheckSum(), checksum );
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->Add( 1, extended );
	}
	CObj<NDb::SExtendedStats> loaded = new NDb::SExtendedStats;
	stream.Seek( 0 );
	CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
	loader->Add( 1, loaded.GetPtr() );
	EXPECT_EQ( loaded->szBinocularsPlatform, "turret" );
}

TEST_F( MechBinoculars, EmptyUnknownAndHullNamesUseHull )
{
	for ( const char *name : { "", "missing", "hull" } )
	{
		SCOPED_TRACE( name );
		extended->szBinocularsPlatform = name;
		turret->CTurret::TurnHor( 16384, true );
		Start( 16384 );
		for ( int i = 0; i < 8; ++i )
		{
			Step();
		}
		EXPECT_EQ( unit->GetFrontDirection(), 16384 );
		ExpectCone( 16384 );
		EXPECT_EQ( unit->GetCenterPlain(), CVec2( position.x, position.y ) );
		state->TryInterruptState( nullptr );
		theWarFog.DeleteUnit( unit->GetUniqueId() );
	}
	stats->pExtendedStats = nullptr;
	Start();
	ExpectCone( unit->GetFrontDirection() );
}

TEST_F( MechBinoculars, TurretAimsWithoutHullAndSightAddsToCircleAndSpecialSights )
{
	extended->szBinocularsPlatform = "turret";
	NDb::SSpecialSight rear;
	rear.fAngleStartRad = float( PI ) - 0.3f;
	rear.fAngleEndRad = float( PI ) + 0.3f;
	rear.fSightRange = 24;
	extended->specialSightsArray.push_back( rear );
	Start( 16384 );
	Step();
	EXPECT_EQ( unit->GetFrontDirection(), 0 );
	EXPECT_EQ( turret->GetHorFinalAngle(), 16384 );
	ExpectCone( 16384 );
	EXPECT_TRUE( Visible( 16384, 10 ) );
	EXPECT_TRUE( Visible( 32768, 10 ) );
	EXPECT_TRUE( Visible( 49152, 2 ) );
	EXPECT_FALSE( Visible( 49152, 10 ) );
	EXPECT_EQ( state->TryInterruptState( nullptr ), TSIR_YES_IMMIDIATELY );
	theWarFog.Segment();
	EXPECT_FALSE( Visible( 16384, 10 ) );
	EXPECT_TRUE( Visible( 32768, 10 ) );
	EXPECT_TRUE( Visible( 49152, 2 ) );
}

TEST_F( MechBinoculars, LimitedTurretUsesHullWithoutMoving )
{
	extended->szBinocularsPlatform = "turret";
	SetTurretLimit( 4096 );
	Start( 16384 );
	for ( int i = 0; i < 8; ++i )
	{
		Step();
	}
	EXPECT_NE( unit->GetFrontDirection(), 0 );
	EXPECT_LE( DirsDifference( turret->GetHorFinalAngle(), 0 ), 4096 );
	ExpectCone( 16384 );
	EXPECT_EQ( unit->GetCenterPlain(), CVec2( position.x, position.y ) );
}

TEST_F( MechBinoculars, ImmobileHullKeepsTurretInsideBothLimits )
{
	extended->szBinocularsPlatform = "turret";
	stats->fRotateSpeed = 0;
	SetTurretLimit( 4096 );
	Start( 16384 );
	Step();
	EXPECT_EQ( unit->GetFrontDirection(), 0 );
	ExpectCone( 4096 );
	state->TryInterruptState( nullptr );
	theWarFog.DeleteUnit( unit->GetUniqueId() );
	Start( 49152 );
	Step();
	EXPECT_EQ( unit->GetFrontDirection(), 0 );
	ExpectCone( uint16_t( -4096 ) );
}

TEST_F( MechBinoculars, PassengerLockedTurretUsesItsRetainedAngleForHullAssistance )
{
	MakeUnit( AI_TRANSPORT_UNIT, NDb::RPG_TYPE_TRN_SUPPORT );
	extended->szBinocularsPlatform = "turret";
	turret->CTurret::TurnHor( 16384, true );
	turret->SetRotateTurretState( false );
	Start( 32768 );
	for ( int i = 0; i < 8; ++i )
	{
		Step();
	}
	EXPECT_EQ( unit->GetFrontDirection(), 16384 );
	EXPECT_EQ( turret->GetHorFinalAngle(), 16384 );
	ExpectCone( 32768 );
	EXPECT_EQ( unit->GetCenterPlain(), CVec2( position.x, position.y ) );
}

TEST_F( MechBinoculars, RangeCappedByFogMaximum )
{
	Start();
	ASSERT_EQ( Info().specialSights.size(), 1u );
	EXPECT_EQ( Info().specialSights[0].nRadius, theWarFog.GetMaxRadius() );
}

TEST_F( MechBinoculars, SaveLoadKeepsPlatformTargetAndCancellation )
{
	extended->szBinocularsPlatform = "turret";
	Start( 16384 );
	Step();
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
		saver->Add( 1, &state );
	}
	CObj<IUnitState> loaded;
	stream.Seek( 0 );
	{
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		loader->Add( 1, &loaded );
	}
	ASSERT_NE( loaded, nullptr );
	EXPECT_EQ( loaded->GetName(), EUSN_USE_SPYGLASS );
	EXPECT_EQ( loaded->GetPurposePoint(), state->GetPurposePoint() );
	state = loaded;
	unit->SetCurState( state );
	Step();
	ExpectCone( 16384 );
	EXPECT_TRUE( Visible( 16384, 10 ) );
	state->TryInterruptState( nullptr );
	theWarFog.Segment();
	EXPECT_TRUE( Info().specialSights.empty() );
	EXPECT_FALSE( Visible( 16384, 10 ) );
}

TEST_F( MechBinoculars, GroundMechFactoriesAcceptBinoculars )
{
	for ( const auto type : { NDb::RPG_TYPE_ARM_MEDIUM,
	                          NDb::RPG_TYPE_SPG_ASSAULT,
	                          NDb::RPG_TYPE_TRN_SUPPORT,
	                          NDb::RPG_TYPE_ART_GUN,
	                          NDb::RPG_TYPE_ART_ROCKET } )
	{
		SCOPED_TRACE( int( type ) );
		const int factory = type == NDb::RPG_TYPE_ART_GUN || type == NDb::RPG_TYPE_ART_ROCKET ? AI_ARTILLERY
		                    : type == NDb::RPG_TYPE_TRN_SUPPORT                               ? AI_TRANSPORT_UNIT
		                                                                                      : AI_TANK;
		MakeUnit( factory, type );
		Start();
		ASSERT_NE( state, nullptr );
		Step();
		EXPECT_EQ( state->GetName(), EUSN_USE_SPYGLASS );
		ExpectCone( 0 );
		theWarFog.DeleteUnit( unit->GetUniqueId() );
	}
}

TEST_F( MechBinoculars, TowedGunPacksBeforeHullAssistance )
{
	MakeUnit( AI_ARTILLERY, NDb::RPG_TYPE_ART_GUN );
	extended->szBinocularsPlatform = "turret";
	SetTurretLimit( 4096 );
	Start( 16384 );
	Step();
	EXPECT_EQ( unit->GetFrontDirection(), 0 );
	EXPECT_EQ( turret->GetHorFinalAngle(), 0 );
	EXPECT_TRUE( dynamic_cast<CArtillery *>( unit.GetPtr() )->IsInInstallAction() );
	EXPECT_EQ( unit->GetCenterPlain(), CVec2( position.x, position.y ) );
}
} // namespace
