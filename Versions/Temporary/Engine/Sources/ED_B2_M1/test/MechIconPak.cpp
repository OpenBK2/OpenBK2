#include "ED_B2_M1/stdafx.h"

#include "3Dmotor/2DSceneSW.h"
#include "3Dmotor/DBScene.h"
#include "3Dmotor/FrameTransition.h"
#include "3Dmotor/RectLayout.h"
#include "3Dmotor/SWTexture.h"
#include "3Dmotor/TextureCoordinates.h"
#include "3Dmotor/UITextureFilter.h"
#include "ED_B2_M1/MechIconPak.h"
#include "Image/Image.h"
#include "Image/ImageDDS.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "Stats_B2_M1/ActionCommand.h"
#include "Stats_B2_M1/ActionsRemap.h"
#include "Stats_B2_M1/RPGStats.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"
#include "UI/DBUserInterface.h"
#include "UI/test/MakeDescription.h"
#include "UI/UI.h"
#include "UI/UIVisitor.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>

#include <gtest/gtest.h>

namespace
{
namespace fs = std::filesystem;

struct IconVisitor : IUIVisitor
{
	CRectLayout layout;

	void ClipSet( const CTRect<float> & ) override {}

	void ClipRestore() override {}

	void VisitZClearRect( const CRectLayout &, const CTPoint<float> &, const CTRect<float> &, float ) override {}

	void VisitUIRect( const NDb::STexture *, int, const CRectLayout &value ) override
	{
		layout = value;
	}

	void
	VisitUIRect( const NDb::STexture *, int, const CVec2 *, const NGfx::SPixel8888 *, const CTRect<float> & ) override
	{
	}

	void VisitUITextureRect( CPtrFuncBase<NGfx::CTexture> *, int, const CRectLayout & ) override {}

	void VisitUIText( IML *, const CTPoint<float> &, const CTRect<float> & ) override {}
};

class MechIconPak : public testing::Test
{
protected:
	fs::path root;
	CObj<NVFS::IVFS> savedVFS;
	CObj<NVFS::IFileCreator> savedCreator;
	const std::string szUnit = u8"Units/St\u00fcrmm\u00f6rser/unit.xdb";
	const std::string szVariant = u8"Units/St\u00fcrmm\u00f6rser/variant.xdb";
	NIconRendering::SOptions options{ 3, 5 };
	CArray2D<NGfx::SPixel8888> capture;

	void SetUp() override
	{
		// Anchor the DLLs that register the real mech and texture resource types.
		ASSERT_EQ( GetCommandByAction( NDb::USER_ACTION_MOVE ), ACTION_COMMAND_MOVE_TO );
		ASSERT_NE( nullptr, NGScene::GetFrameTransitionCapture2() );
		NSingleton::RegisterSingleton( CreateUIInitialization(), IUIInitialization::tidTypeID );
		Singleton<IUIInitialization>()->GetVirtualScreenController()->SetResolution( 1024, 768 );
		NGlobal::SetVar( "resource_sync_load", 1 );
		root = fs::temp_directory_path() /
		       ( "obk2-icon-pak-" + std::to_string( std::chrono::steady_clock::now().time_since_epoch().count() ) );
		fs::create_directories( ( root / fs::u8path( szUnit ) ).parent_path() );
		fs::copy_file( ICON_PAK_SCHEMA, root / "types.xml" );
		std::ofstream( root / "shared.xdb" ) << "<Texture/>";
		for ( const auto &name : { szUnit, szVariant } )
		{
			std::ofstream( root / fs::u8path( name ) )
					<< "<MechUnitRPGStats ObjectRecordID=\"707\">"
					<< "<IconTexture href=\"/shared.xdb#xpointer(/Texture)\"/>"
					<< "<CameraPlacementIconShot><Distance>67.5</Distance></CameraPlacementIconShot>"
					<< "</MechUnitRPGStats>";
		}
		std::ofstream( root / "iconless.xdb" ) << "<MechUnitRPGStats/>";
		savedVFS = NVFS::GetMainVFS();
		savedCreator = NVFS::GetMainFileCreator();
		const std::string folder = root.generic_u8string() + "/";
		NVFS::SetMainVFS( NVFS::CreateWinVFS( folder ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( folder ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
		for ( const auto &name : { szUnit, szVariant, std::string( "shared.xdb" ), std::string( "iconless.xdb" ) } )
		{
			ASSERT_TRUE( NDb::RegisterResourceFile( name ) ) << name;
		}
		capture.SetSizes( 7, 9 );
		for ( int y = 0; y < capture.GetSizeY(); ++y )
		{
			for ( int x = 0; x < capture.GetSizeX(); ++x )
			{
				capture[y][x].dwColor = 0x80402000u + y * 7 + x;
			}
		}
	}

	void TearDown() override
	{
		NSingleton::UnRegisterSingleton( IUIInitialization::tidTypeID );
		NDb::CloseDatabase();
		NVFS::SetMainVFS( savedVFS );
		NVFS::SetMainFileCreator( savedCreator );
		std::error_code error;
		fs::remove_all( root, error );
	}

	std::string Snapshot( const std::string &name )
	{
		CMemoryStream stream;
		EXPECT_TRUE( NDb::SerializeObject( NDb::GetManipulator( CDBID( name ) ), &stream ) );
		return { reinterpret_cast<const char *>( stream.GetBuffer() ), static_cast<size_t>( stream.GetSize() ) };
	}
};
} // namespace

TEST_F( MechIconPak, ArchiveLoadsThroughGameVFSWithoutLooseIconsOrLiveDatabaseChanges )
{
	// Include a pending edit and a brand new unit, without saving either.
	EXPECT_FALSE( NDb::IsPropertyModified( CDBID( szUnit ), "CameraPlacementIconShot.Distance" ) );
	ASSERT_TRUE( NDb::GetManipulator( CDBID( szUnit ) )->SetValue( "CameraPlacementIconShot.Distance", 81.0f ) );
	CPtr<NDb::IObjMan> fresh = NDb::CreateNewObject( "MechUnitRPGStats" );
	ASSERT_TRUE( NDb::AddNewObject( "new.xdb", CDBID( "new.xdb" ), fresh ) );
	const std::string original = Snapshot( szUnit );
	EXPECT_TRUE( NDb::IsPropertyModified( CDBID( szUnit ), "CameraPlacementIconShot.Distance" ) );
	CMechIconPak pak;
	std::string error;
	for ( const auto &name : { szUnit, szVariant, std::string( "iconless.xdb" ), std::string( "new.xdb" ) } )
	{
		ASSERT_TRUE( pak.Add( CDBID( name ), capture, options, &error ) ) << error;
	}
	EXPECT_EQ( original, Snapshot( szUnit ) );
	EXPECT_TRUE( NDb::IsPropertyModified( CDBID( szUnit ), "CameraPlacementIconShot.Distance" ) );
	EXPECT_FALSE( fs::exists( root / "new.xdb" ) );
	EXPECT_FALSE( fs::exists( root / "MechUnitIcons" ) );
	EXPECT_FALSE( fs::exists( root / "index.bin" ) );
	ASSERT_TRUE( pak.Save( "Export/MechUnitIcons.pak", &error ) ) << error;
	EXPECT_EQ( 1, std::distance( fs::directory_iterator( root / "Export" ), fs::directory_iterator() ) );

	// Mount the package over the base data, which supplies types.xml. Loading a
	// unit must discover its new texture without a separately generated index.
	fresh = nullptr;
	NDb::CloseDatabase();
	CObj<NVFS::IVFS> packed = NVFS::CreateWinVFS( ( root / "Export/MechUnitIcons.pak" ).generic_u8string() );
	CObj<NVFS::ICombinerVFS> combined = NVFS::CreateCombinerVFS( NVFS::GetMainVFS() );
	combined->SetVFSList( { NVFS::GetMainVFS(), packed } );
	NVFS::SetMainVFS( combined );
	ASSERT_TRUE( NDb::OpenDatabase( combined, nullptr, NDb::DATABASE_MODE_GAME ) );
	std::vector<std::string> names;
	packed->GetAllFileNames( &names, "" );
	EXPECT_EQ( 12u, names.size() );
	CFileStream unitXML( packed, szUnit );
	ASSERT_TRUE( unitXML.IsOk() );
	const std::string serialized( reinterpret_cast<const char *>( unitXML.GetBuffer() ), unitXML.GetSize() );
	EXPECT_NE( std::string::npos, serialized.find( "ObjectRecordID=\"707\"" ) );
	EXPECT_NE( std::string::npos, serialized.find( "<Distance>81</Distance>" ) );
	for ( const auto &name : { szUnit, szVariant, std::string( "iconless.xdb" ), std::string( "new.xdb" ) } )
	{
		const auto *unit = NDb::Get<NDb::SMechUnitRPGStats>( CDBID( name ) );
		ASSERT_NE( nullptr, unit );
		ASSERT_NE( nullptr, unit->pIconTexture.GetPtr() );
		const auto *texture = unit->pIconTexture.GetPtr();
		EXPECT_EQ( 42, texture->nWidth );
		EXPECT_EQ( 42, texture->nHeight );
		std::unique_ptr<CDataStream> image( packed->OpenFile( texture->szDestName ) );
		ASSERT_NE( nullptr, image );
		CArray2D<uint32_t> pixels;
		ASSERT_TRUE( NImage::LoadImageDDS( &pixels, image.get() ) );
		ASSERT_EQ( 3, pixels.GetSizeX() );
		ASSERT_EQ( 5, pixels.GetSizeY() );
		for ( int y = 0; y < 5; ++y )
		{
			for ( int x = 0; x < 3; ++x )
			{
				EXPECT_EQ( capture[y + 2][x + 2].dwColor, pixels[y][x] );
			}
		}
	}
}

TEST_F( MechIconPak, HighResolutionIconsDrawTheModelInsteadOfTheTransparentCorner )
{
	CMechIconPak pak;
	std::string error;
	const NIconRendering::SOptions sizes[] = { { 512, 512 }, { 113, 67 }, { 42, 42 }, { 1, 67 } };
	const std::string units[] = { szUnit, szVariant, "iconless.xdb", "tiny.xdb" };
	CPtr<NDb::IObjMan> tiny = NDb::CreateNewObject( "MechUnitRPGStats" );
	ASSERT_TRUE( NDb::AddNewObject( "tiny.xdb", CDBID( "tiny.xdb" ), tiny ) );
	for ( int i = 0; i < 4; ++i )
	{
		capture.SetSizes( sizes[i].nWidth, sizes[i].nHeight );
		// Like a model capture, leave transparent corners and put the subject
		// in the middle. Reading the top-left 42 pixels must not find it.
		for ( int y = 0; y < capture.GetSizeY(); ++y )
		{
			for ( int x = 0; x < capture.GetSizeX(); ++x )
			{
				const bool center =
						( capture.GetSizeX() == 1 || ( x >= capture.GetSizeX() * 2 / 5 && x < capture.GetSizeX() * 3 / 5 ) ) &&
						y >= capture.GetSizeY() * 2 / 5 && y < capture.GetSizeY() * 3 / 5;
				capture[y][x].dwColor = center ? 0xffffffffu : 0;
			}
		}
		ASSERT_TRUE( pak.Add( CDBID( units[i] ), capture, sizes[i], &error ) ) << error;
	}
	ASSERT_TRUE( pak.Save( "Export/MechUnitIcons.pak", &error ) ) << error;
	tiny = nullptr;
	NDb::CloseDatabase();
	CObj<NVFS::IVFS> packed = NVFS::CreateWinVFS( ( root / "Export/MechUnitIcons.pak" ).generic_u8string() );
	CObj<NVFS::ICombinerVFS> combined = NVFS::CreateCombinerVFS( NVFS::GetMainVFS() );
	combined->SetVFSList( { NVFS::GetMainVFS(), packed } );
	NVFS::SetMainVFS( combined );
	ASSERT_TRUE( NDb::OpenDatabase( combined, nullptr, NDb::DATABASE_MODE_GAME ) );
	for ( int i = 0; i < 4; ++i )
	{
		SCOPED_TRACE( units[i] );
		const auto *unit = NDb::Get<NDb::SMechUnitRPGStats>( CDBID( units[i] ) );
		ASSERT_NE( nullptr, unit );
		const auto *texture = unit->pIconTexture.GetPtr();
		ASSERT_NE( nullptr, texture );
		for ( bool scaling : { false, true } )
		{
			CObj<IWindowPart> background = MakeObject<IWindowPart>( scaling ? 0x11075B40 : 0x11075B41 );
			ASSERT_TRUE( background );
			CObj<NDb::SUIDesc> description;
			if ( scaling )
			{
				CPtr<NDb::SBackgroundSimpleScallingTexture> desc =
						NUITest::MakeDescription<NDb::SBackgroundSimpleScallingTexture>();
				desc->pTexture = texture;
				desc->nColor = 0xffffffff;
				desc->vSize = CVec2( 42, 42 );
				description = desc;
				background->InitByDesc( desc );
			}
			else
			{
				CPtr<NDb::SBackgroundSimpleTexture> desc = NUITest::MakeDescription<NDb::SBackgroundSimpleTexture>();
				desc->pTexture = texture;
				desc->nColor = 0xffffffff;
				description = desc;
				background->InitByDesc( desc );
			}
			background->SetPos( CVec2( 0, 0 ), CVec2( 42, 42 ) );
			background->SetFadeValue( 1 );
			IconVisitor visitor;
			background->Visit( &visitor );
			ASSERT_EQ( 1u, visitor.layout.rects.size() );
			const auto &rect = visitor.layout.rects[0];
			EXPECT_FLOAT_EQ( 42, rect.fSizeX );
			EXPECT_FLOAT_EQ( 42, rect.fSizeY );
			EXPECT_FLOAT_EQ( 42, rect.sTex.rcTexRect.x2 );
			EXPECT_FLOAT_EQ( 42, rect.sTex.rcTexRect.y2 );

			// Exercise the actual DDS loader and software rectangle rasterizer
			// used by icon outlines. DrawFog allows reading its result without a GPU.
			CObj<NGScene::ISW2DScene> scene = NGScene::Make2DSWScene();
			CObj<CObjectBase> drawn = scene->CreateRects( NGScene::GetSWTex( texture ), visitor.layout );
			CArray2D<int> pixels( 42, 42 );
			pixels.FillEvery( 255 );
			scene->DrawFog( &pixels, CTPoint<int>( 42, 42 ) );
			EXPECT_EQ( 0, pixels[21][21] );
			EXPECT_EQ( 255, pixels[1][1] );
			CArray2D<NGfx::SPixel8888> colorPixels;
			scene->DrawImage( &colorPixels, CTPoint<int>( 42, 42 ) );
			EXPECT_EQ( 255, colorPixels[21][21].a );
			EXPECT_EQ( 0, colorPixels[1][1].a );

			// The hardware path uses the same per-axis mapping, including ratios
			// that are not whole multiples of the original 42 texels.
			CTRect<float> mapped = rect.sTex.rcTexRect;
			NGScene::ScaleTextureRect(
					&mapped,
					NGScene::GetTextureTexelScale( sizes[i].nWidth, sizes[i].nHeight, texture->nWidth, texture->nHeight ) );
			EXPECT_FLOAT_EQ( sizes[i].nWidth, mapped.x2 );
			EXPECT_FLOAT_EQ( sizes[i].nHeight, mapped.y2 );
		}
	}
}

TEST_F( MechIconPak, DownscalingAveragesFineDetailAndKeepsTransparentColorsOutOfTheModel )
{
	capture.SetSizes( 512, 512 );
	for ( int y = 0; y < 512; ++y )
	{
		for ( int x = 0; x < 512; ++x )
		{
			const bool even = ( ( x + y ) & 1 ) == 0;
			// Opaque checkerboard on the left; green details over transparent
			// red on the right. Point sampling produces noisy black/white dots.
			capture[y][x].dwColor = x < 256 ? ( even ? 0xffffffffu : 0xff000000u ) : ( even ? 0xff00ff00u : 0x00ff0000u );
		}
	}
	std::string error;
	CMechIconPak pak;
	ASSERT_TRUE( pak.Add( CDBID( szUnit ), capture, { 512, 512 }, &error ) ) << error;
	ASSERT_TRUE( pak.Save( "Export/MechUnitIcons.pak", &error ) ) << error;
	NDb::CloseDatabase();
	CObj<NVFS::IVFS> packed = NVFS::CreateWinVFS( ( root / "Export/MechUnitIcons.pak" ).generic_u8string() );
	CObj<NVFS::ICombinerVFS> combined = NVFS::CreateCombinerVFS( NVFS::GetMainVFS() );
	combined->SetVFSList( { NVFS::GetMainVFS(), packed } );
	NVFS::SetMainVFS( combined );
	ASSERT_TRUE( NDb::OpenDatabase( combined, nullptr, NDb::DATABASE_MODE_GAME ) );
	const auto *unit = NDb::Get<NDb::SMechUnitRPGStats>( CDBID( szUnit ) );
	ASSERT_NE( nullptr, unit );
	ASSERT_NE( nullptr, unit->pIconTexture.GetPtr() );
	for ( int size : { 42, 70, 113 } )
	{
		SCOPED_TRACE( size );
		CObj<NGScene::ISW2DScene> scene = NGScene::Make2DSWScene();
		CObj<CObjectBase> drawn =
				scene->CreateRects( NGScene::GetSWTex( unit->pIconTexture ), MakeLayout( 0, 0, size, size, 0, 0, 42, 42 ) );
		CArray2D<NGfx::SPixel8888> pixels;
		scene->DrawImage( &pixels, CTPoint<int>( size, size ) );
		for ( int y = 4; y < size - 4; ++y )
		{
			for ( int x = 4; x < size / 2 - 4; ++x )
			{
				EXPECT_NEAR( 128, pixels[y][x].r, 1 );
				EXPECT_EQ( 255, pixels[y][x].a );
			}
			const auto &green = pixels[y][size * 3 / 4];
			EXPECT_EQ( 0, green.r );
			EXPECT_EQ( 255, green.g );
			EXPECT_EQ( 0, green.b );
			EXPECT_NEAR( 128, green.a, 1 );
		}
	}
}

TEST( MechIconFilter, MipsRetainCoverageAndBilinearSamplingIgnoresTransparentRGB )
{
	CArray2D<NGfx::SPixel8888> pixels( 2, 2 );
	pixels.FillEvery( NGfx::SPixel8888( 255, 0, 0, 0 ) );
	pixels[0][0] = NGfx::SPixel8888( 0, 255, 0, 255 );
	std::vector<CArray2D<NGfx::SPixel8888>> mips;
	NGScene::BuildUITextureMips( &mips, pixels );
	ASSERT_EQ( 2u, mips.size() );
	EXPECT_EQ( 64, mips[1][0][0].a );
	EXPECT_EQ( 0, mips[1][0][0].r );
	EXPECT_EQ( 255, mips[1][0][0].g );
	const auto edge = NGScene::SampleUITexture( mips, 1, 0.5f, 0 );
	EXPECT_EQ( 128, edge.a );
	EXPECT_EQ( 0, edge.r );
	EXPECT_EQ( 255, edge.g );
}

TEST_F( MechIconPak, FailedUnitsAndFailedSavesPreserveExistingArchive )
{
	std::ofstream( root / "MechUnitIcons.pak" ) << "previous archive";
	std::string error;
	CMechIconPak empty;
	EXPECT_FALSE( empty.Add( CDBID( szUnit ), capture, { 512, 512 }, &error ) );
	EXPECT_FALSE( empty.Save( "MechUnitIcons.pak", &error ) );
	CFileStream original( ( root / "MechUnitIcons.pak" ).generic_u8string(), CFileStream::WIN_READ_ONLY );
	EXPECT_EQ( "previous archive",
	           std::string( reinterpret_cast<const char *>( original.GetBuffer() ), original.GetSize() ) );
	CMechIconPak pak;
	ASSERT_TRUE( pak.Add( CDBID( szUnit ), capture, options, &error ) ) << error;
	std::ofstream( root / "blocked" ) << "a file cannot be an output directory";
	EXPECT_FALSE( pak.Save( "blocked/MechUnitIcons.pak", &error ) );
	EXPECT_FALSE( error.empty() );
	EXPECT_FALSE( fs::exists( root / "MechUnitIcons" ) );
}

TEST_F( MechIconPak, ReplacesAnExistingMountedArchive )
{
	std::string error;
	CMechIconPak first;
	ASSERT_TRUE( first.Add( CDBID( szUnit ), capture, options, &error ) ) << error;
	ASSERT_TRUE( first.Save( "Export/MechUnitIcons.pak", &error ) ) << error;
	const std::string file = ( root / "Export/MechUnitIcons.pak" ).generic_u8string();
	CObj<NVFS::IVFS> previous = NVFS::CreateWinVFS( file );
	ASSERT_TRUE( previous->DoesFileExist( szUnit ) );
	CMechIconPak replacement;
	ASSERT_TRUE( replacement.Add( CDBID( szVariant ), capture, options, &error ) ) << error;
	ASSERT_TRUE( replacement.Save( "Export/MechUnitIcons.pak", &error ) ) << error;
	CFileStream previousUnit( previous, szUnit );
	EXPECT_TRUE( previousUnit.IsOk() );
	EXPECT_GT( previousUnit.GetSize(), 0 );
	CObj<NVFS::IVFS> current = NVFS::CreateWinVFS( file );
	EXPECT_TRUE( current->DoesFileExist( szVariant ) );
	EXPECT_FALSE( current->DoesFileExist( szUnit ) );
	EXPECT_EQ( 1, std::distance( fs::directory_iterator( root / "Export" ), fs::directory_iterator() ) );
}
