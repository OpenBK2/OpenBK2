// IconRender: re-renders the unit icons through the game's own renderer.
//
// The shipped icon of a vehicle is a 42x42 picture the model editor made
// (ED_B2_M1/IconsCreation.cpp, CModelEditor::CreateIcon): it drew the unit into
// the 512x512 FrameTransitionSrc2 target over a grey background with zero
// alpha and cut out the centre 42x42. The camera it used is kept in the hidden
// CameraPlacementIconShot field of every MechUnitRPGStats record, so the shot
// can be replayed for all units at once.
//
// For each unit this writes, under <out>/<unit folder>/:
//   orig.dds  the shipped icon, copied out of the VFS unchanged
//   x1.tga    the editor's shot reproduced exactly: same target, same FOV, same
//             42x42 crop. Comparing it with orig.dds says whether the scene
//             (light, model, camera) matches what the editor had
//   hq.tga    the same view at <scale> times the size. The FOV is narrowed so
//             the 42 pixel window fills <scale>*<supersample> pixels of the
//             target, and the result is box filtered down by <supersample>
// and one line per unit in <out>/manifest.tsv.
//
// scripts/iconrender/icon_report.py turns that into a comparison page.
//
// The data comes from the installation the executable is in (or --install),
// and the profiles are the user's own, as for the game.
//
//   IconRender [--out=DIR] [--filter=TEXT] [--scale=N] [--supersample=N]
//              [--light=DBID] [--limit=N] [--mod=NAME] [--overrides=FILE]
//              [--install=DIR] [--cache]
//
// The comparison files go to --out, by default IconRender/ in the
// installation. A folder that cannot be created stops the run before anything
// is rendered.
//
// --cache writes each unit's high resolution picture into the game's unit
// icon cache (GameX/UnitIconCache.h), and only that unless --out is given too,
// so the game shows it without rendering
// it first. Without --scale it uses the scale the game picks for the screen in
// the profile, and the most supersampling that still fits the target.
//
// --mod attaches a folder under MODs, as the game's own --mod does.
//
// --install takes the data from another installation than the one the
// executable is in, such as a Fall of the Reich install next to the original
// game. The profiles stay the user's own.
//
// The editor and CLI share SceneB2/UnitIconRendering and the bundled corrections
// from scripts/iconrender/icon_overrides.txt. --overrides=none disables them;
// --overrides=FILE replaces them with a custom text file, for the few units
// whose saved camera does not suit (or that never had one). One unit per line:
// the stats path, then any of
//   frame           place the camera so the unit fills the window the way the
//                   shipped icons do: its longer side about 93% of it, centred
//   fill=F          with frame, the share of the window the longer side fills
//   angle           search yaw and pitch for the view that best matches the
//                   silhouette of the shipped icon
//   yaw=A pitch=A   set the angle, in degrees as the stats store them
//   yaw+=D pitch+=D turn the saved angle by D degrees
//   distance*=K     move the camera K times further out
//   like=PATH       draw another unit's model with that unit's saved camera,
//                   for a record whose own visual object is not a vehicle
//                   (tm_2_12_super shows the railway gun's shell and borrows
//                   tm_2_12's icon); the other corrections then apply on top
// Everything after a # is a comment. Whatever order they are written in, like=
// picks the model and camera first, then the angle is set, turned and moved
// out, then searched (angle), and framing comes last.

#include "stdafx.h"

#include "3Dmotor/Gfx.h"
#include "3Dmotor/GView.h"
#include "AILogic/CreateAI.h"
#include "GameX/UnitIconCache.h"
#include "Image/Image.h"
#include "Image/ImageTGA.h"
#include "libdb/Db.h"
#include "Main/MainLoop.h"
#include "Main/MODs.h"
#include "Main/Profiles.h"
#include "Misc/2Darray.h"
#include "Misc/StrProc.h"
#include "port/messagebox.h"
#include "SceneB2/Camera.h"
#include "SceneB2/Scene.h"
#include "SceneB2/UnitIconRendering.h"
#include "Stats_B2_M1/RPGStats.h"
#include "Stats_B2_M1/SceneModes.h"
#include "System/FilePath.h"
#include "System/FileUtils.h"
#include "System/GeneralSettings.h"
#include "System/GResource.h"
#include "System/PlatformPaths.h"
#include "System/Text.h"
#include "System/VFSOperations.h"
#include "System/WinFrame.h"

#include "GameX_export.h"

#include <cstdint>
#include <map>
#include <memory>

#if BOOST_OS_WINDOWS
#include <shellapi.h> // CommandLineToArgvW
#endif

#include <fmt/format.h>

namespace NGameX
{
bool Initialize();
}

namespace
{

using NUnitIconRendering::SCameraShot;
using NUnitIconRendering::SOverride;
constexpr int N_TARGET_SIZE = NUnitIconRendering::TARGET_SIZE;
constexpr int N_ICON_SIZE = NUnitIconRendering::ICON_SIZE;

// The light the model editor falls back to when none is picked: the Misc.Light
// entry of Editor/EditorOptions.xdb.
const char *const SZ_DEFAULT_LIGHT = "Scene/Lights/All/Summer/Daylight_AmbientLight.xdb";

struct SOptions
{
	std::string szOutDir;
	std::string szFilter;
	std::string szLight = SZ_DEFAULT_LIGHT;
	int nScale = 4;
	int nSupersample = 3;
	int nLimit = 0;
	std::string szMod;
	std::string szOverrides;
	std::string szInstall;
	bool bCache = false;
	bool bScaleGiven = false;
	bool bSupersampleGiven = false;
};

std::string Lower( const std::string &szValue )
{
	std::string szRes;
	NStr::ToLower( &szRes, szValue );
	return szRes;
}

bool EndsWithNoCase( const std::string &szValue, const std::string &szTail )
{
	if ( szValue.size() < szTail.size() )
	{
		return false;
	}
	return Lower( szValue.substr( szValue.size() - szTail.size() ) ) == Lower( szTail );
}

bool ParseInt( const std::string &szValue, int *pnRes )
{
	if ( szValue.empty() || !NStr::IsDecNumber( szValue ) )
	{
		return false;
	}
	*pnRes = NStr::ToInt( szValue );
	return true;
}

bool ParseCommandLine( const std::vector<std::string> &arguments, SOptions *pOptions, std::string *pszError )
{
	for ( const std::string &szArg : arguments )
	{
		const size_t nEq = szArg.find( '=' );
		const std::string szName = szArg.substr( 0, nEq );
		const std::string szValue = nEq == std::string::npos ? std::string() : szArg.substr( nEq + 1 );
		bool bOk = true;
		if ( szName == "--out" )
		{
			pOptions->szOutDir = szValue;
		}
		else if ( szName == "--filter" )
		{
			pOptions->szFilter = szValue;
		}
		else if ( szName == "--light" )
		{
			pOptions->szLight = szValue;
		}
		else if ( szName == "--scale" )
		{
			bOk = ParseInt( szValue, &pOptions->nScale ) && pOptions->nScale >= 1;
			pOptions->bScaleGiven = true;
		}
		else if ( szName == "--supersample" )
		{
			bOk = ParseInt( szValue, &pOptions->nSupersample ) && pOptions->nSupersample >= 1;
			pOptions->bSupersampleGiven = true;
		}
		else if ( szName == "--mod" )
		{
			pOptions->szMod = szValue;
		}
		else if ( szName == "--overrides" )
		{
			pOptions->szOverrides = szValue;
		}
		else if ( szName == "--cache" )
		{
			pOptions->bCache = true;
		}
		else if ( szName == "--install" )
		{
			pOptions->szInstall = szValue;
		}
		else if ( szName == "--limit" )
		{
			bOk = ParseInt( szValue, &pOptions->nLimit ) && pOptions->nLimit >= 0;
		}
		else
		{
			bOk = false;
		}
		if ( !bOk )
		{
			*pszError = "bad argument: " + szArg;
			return false;
		}
	}
	// The high resolution window has to fit in the one target the scene can draw
	// into; divide before multiplying to reject oversized arguments safely.
	if ( pOptions->nScale > N_TARGET_SIZE / N_ICON_SIZE ||
	     pOptions->nSupersample > N_TARGET_SIZE / ( N_ICON_SIZE * pOptions->nScale ) )
	{
		*pszError = fmt::format( "--scale={} --supersample={} exceeds the {} pixel target",
		                         pOptions->nScale,
		                         pOptions->nSupersample,
		                         N_TARGET_SIZE );
		return false;
	}
	return true;
}

bool ReadVFSFile( const std::string &szPath, std::string *pRes )
{
	std::unique_ptr<CDataStream> pStream( NVFS::GetMainVFS()->OpenFile( szPath ) );
	if ( !pStream || !pStream->IsOk() )
	{
		return false;
	}
	pRes->resize( pStream->GetSize() );
	if ( !pRes->empty() )
	{
		pStream->Read( &( *pRes )[0], pRes->size() );
	}
	return true;
}

bool WriteTGA( const std::string &szFileName, const CArray2D<NGfx::SPixel8888> &image )
{
	CArray2D<uint32_t> data;
	data.SetSizes( image.GetSizeX(), image.GetSizeY() );
	for ( int y = 0; y < image.GetSizeY(); ++y )
	{
		for ( int x = 0; x < image.GetSizeX(); ++x )
		{
			data[y][x] = image[y][x].dwColor;
		}
	}
	CFileStream file( szFileName, CFileStream::WIN_CREATE );
	if ( !file.IsOk() )
	{
		return false;
	}
	return NImage::SaveImageAsTGA( &file, data );
}

bool WriteFile( const std::string &szFileName, const std::string &szData )
{
	CFileStream file( szFileName, CFileStream::WIN_CREATE );
	if ( !file.IsOk() )
	{
		return false;
	}
	file.Write( szData.data(), szData.size() );
	return true;
}

// The manifest is tab separated, so a name must not carry one.
std::string CleanField( std::string szValue )
{
	for ( char &c : szValue )
	{
		if ( c == '\t' || c == '\n' || c == '\r' )
		{
			c = ' ';
		}
	}
	return szValue;
}

std::string GetFolder( const std::string &szPath )
{
	const size_t nSlash = szPath.find_last_of( "/\\" );
	return nSlash == std::string::npos ? std::string() : szPath.substr( 0, nSlash + 1 );
}

std::vector<std::string> FindUnitStats( const std::string &szFilter )
{
	std::vector<std::string> fileNames;
	NVFS::GetMainVFS()->GetAllFileNames( &fileNames, "Units/" );
	std::vector<std::string> result;
	const std::string szLowFilter = Lower( szFilter );
	for ( const std::string &szFile : fileNames )
	{
		if ( !EndsWithNoCase( szFile, ".xdb" ) )
		{
			continue;
		}
		if ( !szLowFilter.empty() && Lower( szFile ).find( szLowFilter ) == std::string::npos )
		{
			continue;
		}
		// A zip entry named in the zip format's default CP437 rather than UTF-8
		// (MOD-18's "38_cm_Sturmm\x94rser_Sturmtiger") is not a valid UTF-8
		// path, and opening it throws. The VFS cannot reach such a file anyway,
		// so it is reported and skipped rather than ending the run.
		try
		{
			if ( NDb::GetClassTypeName( CDBID( szFile ) ) == "MechUnitRPGStats" )
			{
				result.push_back( szFile );
			}
		}
		catch ( const std::exception &error )
		{
			fmt::print( stderr, "skipped, cannot open: {} ({})\n", szFile, error.what() );
		}
	}
	return result;
}

bool InitGame( const SOptions &options, std::string *pszError )
{
	// The same start-up as Game/main.cpp, up to the point where the game would
	// enter its main loop, but with the editor's configs (MapEditorApp.cpp and
	// CCFCSceneB2::OnCreateChildFrameWnd). game.cfg is not one of them: it moves
	// the near plane to 40, which clips the small vehicles the editor shot from
	// closer than that, and it ends by opening the main menu.
	const bool bSettings =
			options.szInstall.empty()
					? NSettings::InitializeGeneralSettings( pszError )
					: NSettings::InitializeGeneralSettings(
								NSettings::CGeneralSettings( options.szInstall,
	                                           NFile::JoinPath( NSettings::GetApplicationDataDirectory(), "Profiles" ) ),
								pszError );
	if ( !bSettings )
	{
		return false;
	}
	NGlobal::LoadConfig( "Profiles/startup.cfg" );
	NGlobal::LoadConfig( "Profiles/editor.cfg" );
	NGScene::SFLB3_RunResourceLoadingThread();
	NGameX::Initialize();
	CreateAI();
	if ( !NWinFrame::SFLB1_InitApplication( " IconRender", "NIVAL_RTS_ENGINE", 0 ) )
	{
		*pszError = "cannot create the window";
		return false;
	}
	if ( !NGfx::Init3D( NWinFrame::GetWnd() ) )
	{
		*pszError = "failed to initialize Direct3D9";
		return false;
	}
	NMOD::InstantAttachMOD( "", NDb::DATABASE_MODE_GAME );
	NProfile::LoadProfile();
	// A mod, the way Game/main.cpp attaches the one --mod names
	if ( !options.szMod.empty() )
	{
		NMOD::InstantAttachMOD( NFile::JoinPath( NMainLoop::GetBaseDir(), NFile::DIR_MODS, options.szMod ),
		                        NDb::DATABASE_MODE_GAME );
	}
	NGlobal::LoadConfig( "Profiles/autoexec.cfg" );
	// What the editor does before it sets its mode (CCFCSceneB2::OnCreateChildFrameWnd)
	NGlobal::ProcessCommand( L"set_quality 1" );
	// Editor mode, as in the model editor: it draws a scene with no map in it.
	if ( !Scene()->SetupMode( SCENE_MODE_WINDOWED, true ) )
	{
		*pszError = "cannot set the video mode";
		return false;
	}
	// The model editor's camera handling (CModelEditor), which applies a
	// placement as given rather than within the game's limits.
	Camera()->SetHandleType( true );
	return true;
}

// Game/main.cpp's shutdown, so the singletons go in the order they expect.
void DoneGame()
{
	ClearHoldQueue();
	Scene()->Clear();
	NVFS::SetMainVFS( 0 );
	NVFS::SetMainFileCreator( 0 );
	NGfx::Done3D();
	NDb::CloseDatabase();
	NSingleton::DoneSingletons();
}

int RenderAll( const SOptions &_options );

int Run( const std::vector<std::string> &arguments )
{
	SOptions options;
	std::string szError;
	if ( !ParseCommandLine( arguments, &options, &szError ) )
	{
		fmt::print( stderr, "{}\n", szError );
		return 1;
	}
	if ( !InitGame( options, &szError ) )
	{
		fmt::print( stderr, "{}\n", szError );
		MessageBox( 0, szError.c_str(), "IconRender", MB_OK | MB_ICONERROR );
		return 1;
	}
	const int nRes = RenderAll( options );
	DoneGame();
	return nRes;
}

int RenderAll( const SOptions &_options )
{
	SOptions options = _options;
	// The comparison files: wanted unless only the cache is being filled
	const bool bWriteOut = !options.bCache || !options.szOutDir.empty();
	if ( bWriteOut )
	{
		if ( options.szOutDir.empty() )
		{
			options.szOutDir = NFile::JoinPath( NMainLoop::GetBaseDir(), "IconRender" );
		}
		NFile::AppendSlash( &options.szOutDir, '/' );
		// Checked once here rather than reported unit by unit: a shell that
		// passed --out="$env:TEMP\x" unexpanded gave a name with a colon in it,
		// and every unit then failed its writes after rendering
		NFile::CreatePath( options.szOutDir );
		if ( !NFile::DoesFolderExist( options.szOutDir ) )
		{
			fmt::print( stderr, "cannot create the output folder {}\n", options.szOutDir );
			return 1;
		}
	}

	const NDb::SAmbientLight *pLight = NDb::Get<NDb::SAmbientLight>( CDBID( options.szLight ) );
	if ( !pLight )
	{
		fmt::print( stderr, "no AmbientLight {}\n", options.szLight );
		return 1;
	}
	// Bundled corrections are shared with the editor; custom files replace them.
	std::map<std::string, SOverride> overrides = NUnitIconRendering::GetDefaultOverrides();
	if ( options.szOverrides == "none" )
	{
		overrides.clear();
	}
	else if ( !options.szOverrides.empty() )
	{
		std::string szError;
		if ( !NUnitIconRendering::LoadOverrides( options.szOverrides, &overrides, &szError ) )
		{
			fmt::print( stderr, "{}\n", szError );
			return 1;
		}
	}

	std::vector<std::string> units = FindUnitStats( options.szFilter );
	if ( options.nLimit > 0 && units.size() > options.nLimit )
	{
		units.resize( options.nLimit );
	}
	fmt::print( "{} units, light {}, output {}\n",
	            units.size(),
	            options.szLight,
	            bWriteOut ? options.szOutDir : std::string( "the cache only" ) );

	std::string szManifest =
			"folder\tstats\tname\tcamera\tanchor_x\tanchor_y\tanchor_z\tdistance\tpitch\tyaw\tfov\tlight\tscale\tstatus\tcorrections\n";
	if ( options.bCache && !options.bScaleGiven )
	{
		// what the game will ask for on this screen
		options.nScale = NUnitIcons::GetScale();
		if ( !options.bSupersampleGiven )
		{
			options.nSupersample = (std::max)( 1, N_TARGET_SIZE / ( N_ICON_SIZE * options.nScale ) );
		}
		fmt::print( "cache scale {}, supersample {}\n", options.nScale, options.nSupersample );
	}
	NUnitIconRendering::SOptions renderOptions;
	renderOptions.nWidth = N_ICON_SIZE * options.nScale;
	renderOptions.nHeight = renderOptions.nWidth;
	renderOptions.nSupersample = options.nSupersample;
	renderOptions.szLight = options.szLight;
	renderOptions.bLegacyImage = bWriteOut;
	if ( NUnitIconRendering::GetSupersample( renderOptions.nWidth, renderOptions.nHeight, renderOptions.nSupersample ) ==
	     0 )
	{
		fmt::print( stderr, "icon resolution and supersampling do not fit the {} pixel target\n", N_TARGET_SIZE );
		return 1;
	}
	int nDone = 0;
	for ( const std::string &szStatsPath : units )
	{
		NWinFrame::PumpMessages();

		// The VFS names files with backslashes; the output tree and the manifest
		// use forward ones, which every platform and the report script accept.
		std::string szPortablePath = szStatsPath;
		NStr::ReplaceAllChars( &szPortablePath, '\\', '/' );
		// One output folder per stats record: the unit's folder, and the record's
		// name below it when that is not the usual MechUnitRPGStats.xdb, since
		// some folders hold a second record (Fall of the Reich's "New Object.xdb").
		std::string szFolder = GetFolder( szPortablePath );
		const std::string szRecord = szPortablePath.substr( szFolder.size() );
		if ( Lower( szRecord ) != "mechunitrpgstats.xdb" )
		{
			szFolder += szRecord.substr( 0, szRecord.size() - 4 ) + "/";
		}
		std::string szStatus = "ok";
		std::string szName;
		NUnitIconRendering::SResult result;
		try
		{
			const CDBID dbid( szStatsPath );
			const NDb::SMechUnitRPGStats *pStats = NDb::Get<NDb::SMechUnitRPGStats>( dbid );
			if ( pStats && !pStats->szLocalizedNameFileRef.empty() )
			{
				szName = NStr::ToMBCS( NText::GetText( pStats->szLocalizedNameFileRef ) );
			}
			const std::string szUnitDir = options.szOutDir + szFolder;
			if ( bWriteOut )
			{
				NFile::CreatePath( szUnitDir );
				if ( pStats && pStats->pIconTexture )
				{
					std::string szDDS;
					if ( ReadVFSFile( pStats->pIconTexture->szDestName, &szDDS ) )
					{
						WriteFile( szUnitDir + "orig.dds", szDDS );
					}
				}
			}
			std::string szError;
			if ( !NUnitIconRendering::Render(
							 dbid, renderOptions, NUnitIconRendering::FindOverride( overrides, dbid ), &result, &szError ) )
			{
				szStatus = szError;
			}
			else
			{
				if ( bWriteOut && ( !WriteTGA( szUnitDir + "x1.tga", result.legacyImage ) ||
				                    !WriteTGA( szUnitDir + "hq.tga", result.image ) ) )
				{
					szStatus = "write failed";
				}
				if ( options.bCache )
				{
					const std::string szCacheFile = NUnitIcons::GetCacheFile( szPortablePath, options.nScale );
					NFile::CreatePath( GetFolder( szCacheFile ) );
					if ( !WriteTGA( szCacheFile, result.image ) )
					{
						szStatus = "cache write failed";
					}
				}
			}
		}
		catch ( const std::exception &error )
		{
			szStatus = error.what();
		}
		const SCameraShot &shot = result.shot;
		const std::string &szCorrections = result.szCorrections;

		szManifest += fmt::format( "{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\n",
		                           CleanField( szFolder ),
		                           CleanField( szPortablePath ),
		                           CleanField( szName ),
		                           shot.bFromStats ? "stats" : "fallback",
		                           shot.vAnchor.x,
		                           shot.vAnchor.y,
		                           shot.vAnchor.z,
		                           shot.fDistance,
		                           shot.fPitch,
		                           shot.fYaw,
		                           shot.fFov,
		                           options.szLight,
		                           options.nScale,
		                           szStatus,
		                           szCorrections );
		++nDone;
		fmt::print( "[{}/{}] {} {} {}\n", nDone, units.size(), szStatus, szStatsPath, szCorrections );
	}
	if ( bWriteOut && !WriteFile( options.szOutDir + "manifest.tsv", szManifest ) )
	{
		fmt::print( stderr, "cannot write {}manifest.tsv\n", options.szOutDir );
		return 1;
	}
	return 0;
}

} // namespace

#if BOOST_OS_WINDOWS

int main()
{
	// UTF-8 arguments, the same way Game/main.cpp reads them
	std::vector<std::string> arguments;
	int nArgs = 0;
	LPWSTR *pArgs = CommandLineToArgvW( GetCommandLineW(), &nArgs );
	if ( pArgs != nullptr )
	{
		for ( int i = 1; i < nArgs; ++i )
		{
			arguments.push_back( NStr::ToMBCS( std::wstring( pArgs[i] ) ) );
		}
		LocalFree( pArgs );
	}
	return Run( arguments );
}

#else

int main( int argc, char *argv[] )
{
	return Run( std::vector<std::string>( argv + 1, argv + argc ) );
}

#endif
