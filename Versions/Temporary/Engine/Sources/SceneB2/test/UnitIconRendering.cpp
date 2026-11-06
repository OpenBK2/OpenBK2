#include "SceneB2/stdafx.h"

#include "3Dmotor/FrameTransition.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "SceneB2/UnitIconRendering.h"
#include "Stats_B2_M1/ActionCommand.h"
#include "Stats_B2_M1/ActionsRemap.h"
#include "Stats_B2_M1/DBVisObj.h"
#include "Stats_B2_M1/RPGStats.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>

#include <gtest/gtest.h>

using namespace NUnitIconRendering;

TEST( UnitIconRendering, ObjectTypesUseSummerModelsAndFencesUseIntactSegments )
{
	// Generated records are registered by their DLLs; their constructors are not exported.
	ASSERT_EQ( GetCommandByAction( NDb::USER_ACTION_MOVE ), ACTION_COMMAND_MOVE_TO );
	ASSERT_NE( nullptr, NGScene::GetFrameTransitionCapture2() );
	CObj<NDb::SModel> winter = MakeObject<NDb::SModel>( NDb::SModel::typeID );
	CObj<NDb::SModel> summer = MakeObject<NDb::SModel>( NDb::SModel::typeID );
	CObj<NDb::SVisObj> visual = MakeObject<NDb::SVisObj>( NDb::SVisObj::typeID );
	ASSERT_TRUE( winter );
	ASSERT_TRUE( summer );
	ASSERT_TRUE( visual );
	visual->models.resize( 2 );
	visual->models[0].eSeason = NDb::SEASON_WINTER;
	visual->models[0].pModel = winter;
	visual->models[1].eSeason = NDb::SEASON_SUMMER;
	visual->models[1].pModel = summer;
	std::vector<CObj<NDb::SHPObjectRPGStats>> objects;
	const int types[] = { NDb::SMechUnitRPGStats::typeID,
	                      NDb::SBuildingRPGStats::typeID,
	                      NDb::SBridgeRPGStats::typeID,
	                      NDb::SFenceRPGStats::typeID,
	                      NDb::SObjectRPGStats::typeID };
	for ( int type : types )
	{
		objects.emplace_back( MakeObject<NDb::SHPObjectRPGStats>( type ) );
	}
	for ( const auto &stats : objects )
	{
		ASSERT_TRUE( stats );
		EXPECT_EQ( nullptr, GetIconModel( stats ) );
		stats->pvisualObject = visual;
		EXPECT_EQ( summer.GetPtr(), GetIconModel( stats ) );
	}
	visual->models.resize( 1 );
	EXPECT_EQ( winter.GetPtr(), GetIconModel( objects[1] ) );
	CObj<NDb::SFenceRPGStats> fence = MakeObject<NDb::SFenceRPGStats>( NDb::SFenceRPGStats::typeID );
	ASSERT_TRUE( fence );
	fence->centerSegments.visObjes.push_back( nullptr );
	fence->centerSegments.visObjes.emplace_back( visual.GetPtr() );
	EXPECT_EQ( winter.GetPtr(), GetIconModel( fence ) );
	fence->centerSegments.visObjes.clear();
	fence->damagedSegments.visObjes.emplace_back( visual.GetPtr() );
	EXPECT_EQ( nullptr, GetIconModel( fence ) );
	EXPECT_EQ( nullptr, GetIconModel( nullptr ) );
}

TEST( UnitIconRendering, BundledCorrectionsUseTheOriginalRulesAndPortablePaths )
{
	const auto &overrides = GetDefaultOverrides();
	ASSERT_EQ( 11u, overrides.size() );
	const auto *frame =
			FindOverride( overrides, CDBID( "UNITS\\Technics\\German\\Aviation\\ar_234_blitz\\MechUnitRPGStats.xdb" ) );
	ASSERT_NE( nullptr, frame );
	EXPECT_TRUE( frame->bFrame );
	EXPECT_FLOAT_EQ( 0.93f, frame->fFill );
	const auto *angle =
			FindOverride( overrides, CDBID( "Units/Technics/Japan/Aviation/Nakadzima_Ki-43_Hayabusa/MechUnitRPGStats.xdb" ) );
	ASSERT_NE( nullptr, angle );
	EXPECT_TRUE( angle->bAngle );
	const auto *replacement =
			FindOverride( overrides, CDBID( "Units/Technics/USSR/Trains/tm_2_12_super/MechUnitRPGStats.xdb" ) );
	ASSERT_NE( nullptr, replacement );
	EXPECT_EQ( "Units/Technics/USSR/Trains/tm_2_12/MechUnitRPGStats.xdb", replacement->szLike );
	EXPECT_EQ( nullptr, FindOverride( overrides, CDBID( "Units/unlisted.xdb" ) ) );
}

TEST( UnitIconRendering, CustomCorrectionsRejectBadInputWithoutApplyingPartialRules )
{
	std::map<std::string, SOverride> overrides;
	std::string error;
	ASSERT_TRUE( ParseOverrides(
			"# Comment\nUnits/New Object.xdb frame fill=0.8 yaw=200 pitch=20 yaw+=5 pitch+=-2 distance*=1.5\n",
			&overrides,
			&error ) )
			<< error;
	ASSERT_EQ( 1u, overrides.size() );
	const auto *rule = FindOverride( overrides, CDBID( "Units/New Object.xdb" ) );
	ASSERT_NE( nullptr, rule );
	EXPECT_TRUE( rule->bFrame );
	EXPECT_TRUE( rule->bSetYaw );
	EXPECT_TRUE( rule->bSetPitch );
	EXPECT_FLOAT_EQ( 0.8f, rule->fFill );
	EXPECT_FLOAT_EQ( 200.0f, rule->fYaw );
	EXPECT_FLOAT_EQ( 20.0f, rule->fPitch );
	EXPECT_FLOAT_EQ( 5.0f, rule->fYawDelta );
	EXPECT_FLOAT_EQ( -2.0f, rule->fPitchDelta );
	EXPECT_FLOAT_EQ( 1.5f, rule->fDistanceScale );
	for ( const std::string &bad :
	      { "yaw", "yaw=nan", "pitch=20x", "fill=0", "fill=1.1", "distance*=-1", "frame=1", "like=", "unknown=5" } )
	{
		EXPECT_FALSE( ParseOverrides( "other.xdb frame\nbad.xdb " + bad, &overrides, &error ) ) << bad;
		EXPECT_NE( std::string::npos, error.find( "line 2:" ) );
		ASSERT_EQ( 1u, overrides.size() );
		EXPECT_NE( nullptr, FindOverride( overrides, CDBID( "Units/New Object.xdb" ) ) );
	}
}

TEST( UnitIconRendering, SupersamplingFitsBothDimensionsWithoutOverflow )
{
	EXPECT_EQ( 12, GetSupersample( 42, 42 ) );
	EXPECT_EQ( 3, GetSupersample( 168, 168, 3 ) );
	EXPECT_EQ( 2, GetSupersample( 170, 200 ) );
	EXPECT_EQ( 2, GetSupersample( 200, 170 ) );
	EXPECT_EQ( 1, GetSupersample( 512, 70 ) );
	EXPECT_EQ( 512, GetSupersample( 1, 1 ) );
	EXPECT_EQ( 0, GetSupersample( 42, 42, -1 ) );
	EXPECT_EQ( 0, GetSupersample( 42, 42, 13 ) );
	EXPECT_EQ( 0, GetSupersample( 0, 42 ) );
	EXPECT_EQ( 0, GetSupersample( 42, 513 ) );
	EXPECT_EQ( 0, GetSupersample( std::numeric_limits<int>::max(), 42 ) );
	EXPECT_EQ( 0, GetSupersample( 42, 42, std::numeric_limits<int>::max() ) );
}

TEST( UnitIconRendering, SSE4BoxFilterKeepsEdgeColorAndTransparentBackground )
{
	CArray2D<NGfx::SPixel8888> capture( 6, 3 );
	for ( int y = 0; y < 3; ++y )
	{
		for ( int x = 0; x < 6; ++x )
		{
			capture[y][x].dwColor = 0x00808080;
		}
	}
	// One covered sample in a 3x3 footprint: color is preserved, coverage is 1/9.
	capture[1][1].dwColor = 0xffc86432;
	CArray2D<NGfx::SPixel8888> image;
	ASSERT_TRUE( Downsample( &image, capture, 3 ) );
	ASSERT_EQ( 2, image.GetSizeX() );
	ASSERT_EQ( 1, image.GetSizeY() );
	EXPECT_EQ( 0x1cc86432u, image[0][0].dwColor );
	EXPECT_EQ( 0x00808080u, image[0][1].dwColor );
	// Fractional alpha participates in the color weights, not just the final alpha.
	capture[1][2].dwColor = 0x800000ff;
	ASSERT_TRUE( Downsample( &image, capture, 3 ) );
	EXPECT_EQ( 133, image[0][0].r );
	EXPECT_EQ( 67, image[0][0].g );
	EXPECT_EQ( 119, image[0][0].b );
	EXPECT_EQ( 43, image[0][0].a );
	EXPECT_FALSE( Downsample( &image, capture, 0 ) );
	EXPECT_FALSE( Downsample( &image, capture, 4 ) );
	EXPECT_FALSE( Downsample( &image, capture, 2 ) );
}

TEST( UnitIconRendering, SinglePixelExportDoesNotOverflowColorSums )
{
	CArray2D<NGfx::SPixel8888> capture( 512, 512 );
	for ( int y = 0; y < 512; ++y )
	{
		for ( int x = 0; x < 512; ++x )
		{
			capture[y][x].dwColor = 0xffffffff;
		}
	}
	CArray2D<NGfx::SPixel8888> image;
	ASSERT_TRUE( Downsample( &image, capture, 512 ) );
	ASSERT_EQ( 1, image.GetSizeX() );
	ASSERT_EQ( 1, image.GetSizeY() );
	EXPECT_EQ( 0xffffffffu, image[0][0].dwColor );
}

namespace
{
class UnitIconCamera : public testing::Test
{
protected:
	std::filesystem::path root;
	CObj<NVFS::IVFS> savedVFS;
	CObj<NVFS::IFileCreator> savedCreator;

	void SetUp() override
	{
		ASSERT_EQ( GetCommandByAction( NDb::USER_ACTION_MOVE ), ACTION_COMMAND_MOVE_TO );
		root = std::filesystem::temp_directory_path() /
		       ( "obk2-icon-camera-" + std::to_string( std::chrono::steady_clock::now().time_since_epoch().count() ) );
		std::filesystem::create_directories( root );
		std::filesystem::copy_file( ICON_CAMERA_SCHEMA, root / "types.xml" );
		std::ofstream( root / "unit.xdb" ) << "<MechUnitRPGStats><CameraPlacementIconShot>"
																			 << "<Anchor><x>44</x><y>45</y><z>1.5</z></Anchor>"
																			 << "<Distance>67.5</Distance><Pitch>15</Pitch><Yaw>205.2</Yaw><Fov>54</Fov>"
																			 << "</CameraPlacementIconShot></MechUnitRPGStats>";
		std::ofstream( root / "missing.xdb" ) << "<MechUnitRPGStats/>";
		savedVFS = NVFS::GetMainVFS();
		savedCreator = NVFS::GetMainFileCreator();
		const std::string folder = root.generic_u8string() + "/";
		NVFS::SetMainVFS( NVFS::CreateWinVFS( folder ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( folder ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
		ASSERT_TRUE( NDb::RegisterResourceFile( "unit.xdb" ) );
		ASSERT_TRUE( NDb::RegisterResourceFile( "missing.xdb" ) );
	}

	void TearDown() override
	{
		NDb::CloseDatabase();
		NVFS::SetMainVFS( savedVFS );
		NVFS::SetMainFileCreator( savedCreator );
		std::error_code error;
		std::filesystem::remove_all( root, error );
	}
};
} // namespace

TEST_F( UnitIconCamera, BothReadersAgreeButTheEditorRetainsUnsavedCameraChanges )
{
	SCameraShot xml;
	SCameraShot live;
	const CDBID dbid( "unit.xdb" );
	ASSERT_TRUE( ReadCameraShot( dbid, false, &xml ) );
	ASSERT_TRUE( ReadCameraShot( dbid, true, &live ) );
	EXPECT_EQ( xml.vAnchor, live.vAnchor );
	EXPECT_FLOAT_EQ( xml.fDistance, live.fDistance );
	EXPECT_FLOAT_EQ( xml.fPitch, live.fPitch );
	EXPECT_FLOAT_EQ( xml.fYaw, live.fYaw );
	EXPECT_FLOAT_EQ( xml.fFov, live.fFov );
	// Property tracking is opt-in and must be enabled before the edit.
	EXPECT_FALSE( NDb::IsPropertyModified( dbid, "CameraPlacementIconShot.Distance" ) );
	ASSERT_TRUE( NDb::GetManipulator( dbid )->SetValue( "CameraPlacementIconShot.Distance", 81.0f ) );
	ASSERT_TRUE( ReadCameraShot( dbid, true, &live ) );
	ASSERT_TRUE( ReadCameraShot( dbid, false, &xml ) );
	EXPECT_FLOAT_EQ( 81.0f, live.fDistance );
	EXPECT_FLOAT_EQ( 67.5f, xml.fDistance );
	EXPECT_TRUE( NDb::IsPropertyModified( dbid, "CameraPlacementIconShot.Distance" ) );
	EXPECT_FALSE( ReadCameraShot( CDBID( "missing.xdb" ), false, &xml ) );
	EXPECT_FALSE( ReadCameraShot( CDBID( "missing.xdb" ), true, &live ) );
	ASSERT_TRUE( NDb::GetManipulator( dbid )->SetValue( "CameraPlacementIconShot.Fov", 180.0f ) );
	EXPECT_FALSE( ReadCameraShot( dbid, true, &live ) );
	EXPECT_FALSE( live.bFromStats );
}
