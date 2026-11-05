#include "stdafx.h"

#include "3Dmotor/Gfx.h"
#include "3Dmotor/GTexture.h"
#include "Image/ImageTGA.h"
#include "libdb/Db.h"
#include "Main/MODs.h"
#include "Misc/2Darray.h"
#include "Misc/StrProc.h"
#include "Stats_B2_M1/RPGStats.h"
#include "System/FilePath.h"
#include "System/GeneralSettings.h"
#include "System/PlatformPaths.h"
#include "System/VFSOperations.h"
#include "UnitIconCache.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <unordered_map>

#include <fmt/format.h>

namespace NUnitIcons
{

namespace
{

	// The size the icons were made at, and that the UI lays them out at
	const int N_ICON_SIZE = 42;
	// The UI's virtual screen, which it scales to the real one (UI/UI.cpp)
	const float F_VIRTUAL_WIDTH = 1024.0f;
	const float F_VIRTUAL_HEIGHT = 768.0f;
	// The largest multiple a picture is made at
	const int N_MAX_SCALE = 8;

	// Part of the cache path, so pictures made by an older renderer are not
	// mistaken for current ones
	const char *const SZ_RENDER_VERSION = "v1";

	// FNV-1a, to give each installation and mod its own cache folder in a few
	// characters; it only has to be stable, not secret
	uint32_t HashString( const std::string &szText )
	{
		uint32_t dwHash = 2166136261u;
		for ( const char c : szText )
		{
			dwHash ^= uint8_t( c );
			dwHash *= 16777619u;
		}
		return dwHash;
	}

	std::string Lower( const std::string &szValue )
	{
		std::string szRes;
		NStr::ToLower( &szRes, szValue );
		return szRes;
	}

	class CUnitIconSource : public NGScene::ITextureOverride
	{
		struct SEntry
		{
			// the texture is a vehicle's icon; otherwise it is left alone
			bool bIcon = false;
			// the stats record whose icon it is, forward slashes
			std::string szStatsPath;
		};

		std::unordered_map<const NDb::STexture *, SEntry> entries;
		// per folder: the path of its MechUnitRPGStats record, or empty for none
		std::unordered_map<std::string, std::string> folderStats;

		const std::string &GetFolderStats( const std::string &szFolder );
		const SEntry &GetEntry( const NDb::STexture *pTexture );

	public:
		EState GetOverride( const NDb::STexture *pTexture, CArray2D<NGfx::SPixel8888> *pImage, int *pnScale ) override;
		void Reset();
	};

	// The MechUnitRPGStats record in a folder, which is where a vehicle's icon
	// texture sits beside it. Looked up once a folder.
	const std::string &CUnitIconSource::GetFolderStats( const std::string &szFolder )
	{
		auto it = folderStats.find( Lower( szFolder ) );
		if ( it != folderStats.end() )
		{
			return it->second;
		}
		std::string szStats = szFolder + "MechUnitRPGStats.xdb";
		if ( !NVFS::GetMainVFS()->DoesFileExist( szStats ) ||
		     NDb::GetClassTypeName( CDBID( szStats ) ) != "MechUnitRPGStats" )
		{
			szStats.clear();
		}
		return folderStats[Lower( szFolder )] = szStats;
	}

	const CUnitIconSource::SEntry &CUnitIconSource::GetEntry( const NDb::STexture *pTexture )
	{
		auto it = entries.find( pTexture );
		if ( it != entries.end() )
		{
			return it->second;
		}
		SEntry &entry = entries[pTexture];
		std::string szPath = pTexture->GetDBID().ToString();
		NStr::ReplaceAllChars( &szPath, '\\', '/' );
		const size_t nSlash = szPath.find_last_of( '/' );
		if ( nSlash == std::string::npos )
		{
			return entry;
		}
		const std::string &szStats = GetFolderStats( szPath.substr( 0, nSlash + 1 ) );
		if ( szStats.empty() )
		{
			return entry;
		}
		// Only the record's own icon: the folder also holds the model's textures,
		// which go through the same loader
		const NDb::SMechUnitRPGStats *pStats = NDb::Get<NDb::SMechUnitRPGStats>( CDBID( szStats ) );
		if ( pStats && pStats->pIconTexture && pStats->pIconTexture->GetDBID() == pTexture->GetDBID() )
		{
			entry.bIcon = true;
			entry.szStatsPath = szStats;
		}
		return entry;
	}

	NGScene::ITextureOverride::EState
	CUnitIconSource::GetOverride( const NDb::STexture *pTexture, CArray2D<NGfx::SPixel8888> *pImage, int *pnScale )
	{
		if ( !pTexture || NGlobal::GetVar( "unit_icons_hd", 1 ) == 0 )
		{
			return OVERRIDE_NONE;
		}
		const int nScale = GetScale();
		if ( nScale <= 1 )
		{
			return OVERRIDE_NONE;
		}
		const SEntry &entry = GetEntry( pTexture );
		if ( !entry.bIcon )
		{
			return OVERRIDE_NONE;
		}
		CFileStream file( GetCacheFile( entry.szStatsPath, nScale ), CFileStream::WIN_READ_ONLY );
		if ( !file.IsOk() )
		{
			return OVERRIDE_NONE;
		}
		CArray2D<uint32_t> image;
		if ( !NImage::LoadImageTGA( &image, &file ) || image.GetSizeX() != N_ICON_SIZE * nScale ||
		     image.GetSizeY() != N_ICON_SIZE * nScale )
		{
			return OVERRIDE_NONE;
		}
		pImage->SetSizes( image.GetSizeX(), image.GetSizeY() );
		for ( int y = 0; y < image.GetSizeY(); ++y )
		{
			for ( int x = 0; x < image.GetSizeX(); ++x )
			{
				( *pImage )[y][x].dwColor = image[y][x];
			}
		}
		*pnScale = nScale;
		return OVERRIDE_READY;
	}

	void CUnitIconSource::Reset()
	{
		entries.clear();
		folderStats.clear();
	}

	// The cache folder belongs to one installation and one mod, so pictures of a
	// mod's units never stand in for the base game's
	std::string GetCacheRoot()
	{
		std::string szInstall;
		if ( const NSettings::CGeneralSettings *pSettings = NSettings::GetGeneralSettings() )
		{
			szInstall = pSettings->GetInstallRoot();
		}
		std::string szMod = "base";
		NMOD::SMOD mod;
		if ( NMOD::GetAttachedMOD( &mod ) )
		{
			szMod = mod.szRelativePath;
		}
		std::string szRoot = fmt::format( "{}/Cache/UnitIcons/{}/{:08x}/",
		                                  NSettings::GetApplicationDataDirectory(),
		                                  SZ_RENDER_VERSION,
		                                  HashString( Lower( szInstall ) + "|" + Lower( szMod ) ) );
		NStr::ReplaceAllChars( &szRoot, '\\', '/' );
		return szRoot;
	}

	std::unique_ptr<CUnitIconSource> pSource;

} // namespace

// The virtual screen scales by width and height separately, and the larger of
// the two decides
int GetScale()
{
	const CVec2 vScreen = NGfx::GetScreenRect();
	const float fStretch = (std::max)( vScreen.x / F_VIRTUAL_WIDTH, vScreen.y / F_VIRTUAL_HEIGHT );
	// a hair under a whole multiple is close enough to it
	return Clamp( int( std::ceil( fStretch - 0.05f ) ), 1, N_MAX_SCALE );
}

// <cache root>/<scale>x/<stats path without .xdb>.tga, in lower case, since the
// VFS matches names without regard to case and a file system off Windows does not
std::string GetCacheFile( const std::string &szStatsPath, const int nScale )
{
	std::string szUnit = Lower( szStatsPath );
	NStr::ReplaceAllChars( &szUnit, '\\', '/' );
	if ( szUnit.size() > 4 && szUnit.compare( szUnit.size() - 4, 4, ".xdb" ) == 0 )
	{
		szUnit.resize( szUnit.size() - 4 );
	}
	return fmt::format( "{}{}x/{}.tga", GetCacheRoot(), nScale, szUnit );
}

void Initialize()
{
	if ( !pSource )
	{
		pSource = std::make_unique<CUnitIconSource>();
		NGScene::SetTextureOverride( pSource.get() );
	}
}

void Reset()
{
	if ( pSource )
	{
		pSource->Reset();
	}
}

} // namespace NUnitIcons
