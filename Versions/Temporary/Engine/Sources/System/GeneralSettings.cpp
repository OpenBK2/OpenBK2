#include "stdafx.h"

#include "FilePath.h"
#include "FileUtils.h"
#include "GeneralSettings.h"
#include "PlatformPaths.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <system_error>
#include <vector>

#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace NSettings
{
namespace
{

	namespace fs = std::filesystem;
	std::unique_ptr<CGeneralSettings> pSettings;
	std::mutex migrationMutex;

	fs::path Path( std::string szPath )
	{
		std::replace( szPath.begin(), szPath.end(), '\\', '/' );
		return fs::u8path( szPath ).lexically_normal();
	}

	bool SameComponent( const fs::path &a, const fs::path &b )
	{
		return NFile::ComparePathEq( a.u8string(), b.u8string() );
	}

	// Compare components, not string prefixes: ProfilesBackup is a different tree.
	bool RelativeBelow( const fs::path &path, const fs::path &root, fs::path *pRelative )
	{
		auto part = path.begin();
		for ( const auto &rootPart : root )
		{
			if ( part == path.end() || !SameComponent( *part, rootPart ) )
			{
				return false;
			}
			++part;
		}
		pRelative->clear();
		for ( ; part != path.end(); ++part )
		{
			*pRelative /= *part;
		}
		return true;
	}

	bool Exists( const fs::path &path )
	{
		std::error_code error;
		const auto status = fs::symlink_status( path, error );
		if ( error && error != std::errc::no_such_file_or_directory )
		{
			throw fs::filesystem_error( "Cannot inspect directory", path, error );
		}
		return fs::exists( status );
	}

	void ValidateProfiles( const fs::path &path )
	{
		if ( !fs::is_directory( path ) )
		{
			throw std::runtime_error( "Profiles path is not a directory: " + path.u8string() );
		}
		for ( const char *pszRequired : { "startup.cfg", "default_profile/user.cfg", "default_profile/input.cfg" } )
		{
			std::ifstream file( path / pszRequired, std::ios::binary );
			if ( !file )
			{
				throw std::runtime_error( "Profiles is incomplete or unreadable: " + ( path / pszRequired ).u8string() );
			}
		}
	}

	struct CTemporaryDirectory
	{
		fs::path path;

		~CTemporaryDirectory()
		{
			// Only remove the unique staging directory this operation created.
			std::error_code ignored;
			if ( !path.empty() )
			{
				fs::remove_all( path, ignored );
			}
		}
	};

	std::string ResolveCase( const fs::path &path )
	{
		std::string result;
		const auto name = path.u8string();
		return NFile::ResolveDataPathCase( &result, "", name ) ? result : name;
	}

	fs::path LegacyRelative( const fs::path &path )
	{
		fs::path relative;
		bool bLeading = true;
		for ( const auto &part : path )
		{
			if ( bLeading && ( part == "." || part == ".." ) )
			{
				continue;
			}
			bLeading = false;
			relative /= part;
		}
		return relative;
	}

} // namespace

CGeneralSettings::CGeneralSettings( const std::string &szInstall, const std::string &szProfiles )
	: szInstallRoot( Path( szInstall ).u8string() )
	, szProfilesRoot( Path( szProfiles ).u8string() )
{
}

const std::string &CGeneralSettings::GetInstallRoot() const
{
	return szInstallRoot;
}

const std::string &CGeneralSettings::GetProfilesRoot() const
{
	return szProfilesRoot;
}

bool CGeneralSettings::PrepareProfiles( std::string *pszError ) const
{
	// POSIX file locks synchronize processes, so also serialize local callers.
	std::lock_guard<std::mutex> localGuard( migrationMutex );
	try
	{
		const auto source = Path( szInstallRoot ) / "Profiles";
		const auto destination = Path( szProfilesRoot );
		fs::path relative;
		if ( !source.is_absolute() || !destination.is_absolute() ||
		     RelativeBelow( fs::weakly_canonical( destination ), fs::weakly_canonical( source ), &relative ) ||
		     ( Exists( source ) && Exists( destination ) && fs::equivalent( source, destination ) ) )
		{
			throw std::runtime_error( "The user Profiles directory must be separate from the installation Profiles." );
		}
		if ( !Exists( destination ) )
		{
			if ( !Exists( source ) )
			{
				throw std::runtime_error(
						"Error: No Profiles folder found. Have you installed the game properly?\n\nSearched:\n" +
						destination.u8string() + "\n" + source.u8string() );
			}
			ValidateProfiles( source );
			fs::create_directories( destination.parent_path() );
			const auto lockPath = destination.parent_path() / ".profiles.lock";
			{
				std::ofstream lockFile( lockPath, std::ios::app );
				if ( !lockFile )
				{
					throw std::runtime_error( "Cannot create profile migration lock: " + lockPath.u8string() );
				}
			}
			boost::interprocess::file_lock lock( lockPath.u8string().c_str() );
			boost::interprocess::scoped_lock<boost::interprocess::file_lock> guard( lock );
			// A second process may have completed migration while we waited.
			if ( !Exists( destination ) )
			{
				CTemporaryDirectory temporary;
				const auto staging = destination.parent_path() /
				                     ( ".Profiles-import-" + boost::uuids::to_string( boost::uuids::random_generator()() ) );
				if ( !fs::create_directory( staging ) )
				{
					throw std::runtime_error( "Cannot create profile staging directory." );
				}
				temporary.path = staging;
				fs::permissions( staging, fs::perms::owner_all );
				for ( const auto &entry : fs::recursive_directory_iterator( source ) )
				{
					const auto target = staging / entry.path().lexically_relative( source );
					// Never retain a link into the read-only defaults or copy outside them.
					if ( entry.is_symlink() )
					{
						throw std::runtime_error( "Profiles template contains a symbolic link: " + entry.path().u8string() );
					}
					if ( entry.is_directory() )
					{
						fs::create_directory( target );
						fs::permissions( target, fs::perms::owner_all );
					}
					else if ( entry.is_regular_file() )
					{
						fs::copy_file( entry.path(), target );
						fs::permissions( target, fs::perms::owner_read | fs::perms::owner_write );
					}
					else
					{
						throw std::runtime_error( "Profiles template contains an unsupported file: " + entry.path().u8string() );
					}
				}
				ValidateProfiles( staging );
				fs::rename( staging, destination );
				temporary.path.clear();
			}
		}
		ValidateProfiles( destination );
		// A read-only existing directory is an error, never an installation fallback.
		const auto probe = destination / ( ".write-test-" + boost::uuids::to_string( boost::uuids::random_generator()() ) );
		{
			std::ofstream file( probe, std::ios::binary );
			if ( !file )
			{
				throw std::runtime_error( "Cannot write to user Profiles: " + destination.u8string() );
			}
		}
		fs::remove( probe );
		pszError->clear();
		return true;
	}
	catch ( const std::exception &error )
	{
		*pszError = error.what();
		return false;
	}
}

std::string CGeneralSettings::ResolveConfigPath( const std::string &szPath, const std::string &szIncludingFile ) const
{
	const auto path = Path( szPath );
	const auto install = Path( szInstallRoot );
	const auto profiles = Path( szProfilesRoot );
	fs::path relative;
	if ( path.is_absolute() )
	{
		if ( RelativeBelow( path, install / "Profiles", &relative ) )
		{
			return ResolveCase( profiles / relative );
		}
		return ResolveCase( path );
	}
	const auto legacy = LegacyRelative( path );
	if ( RelativeBelow( legacy, fs::path( "Profiles" ), &relative ) )
	{
		return ResolveCase( profiles / relative );
	}
	if ( !szIncludingFile.empty() )
	{
		const auto candidate = Path( szIncludingFile ).parent_path() / path;
		const auto resolved = ResolveCase( candidate.lexically_normal() );
		if ( NFile::DoesFileExist( resolved ) )
		{
			return resolved;
		}
	}
	const auto userPath = ResolveCase( profiles / path );
	if ( NFile::DoesFileExist( userPath ) )
	{
		return userPath;
	}
	return ResolveCase( install / legacy );
}

std::string CGeneralSettings::ResolveResourceID( const std::string &szPath ) const
{
	const auto path = Path( szPath );
	fs::path relative;
	if ( ( path.is_absolute() && RelativeBelow( path, Path( szInstallRoot ) / "Data", &relative ) ) ||
	     ( !path.is_absolute() && RelativeBelow( LegacyRelative( path ), fs::path( "Data" ), &relative ) ) )
	{
		// Keep this a DB-relative ID so the active mod can still override Data.
		return relative.generic_u8string();
	}
	return szPath;
}

bool InitializeGeneralSettings( const CGeneralSettings &settings, std::string *pszError )
{
	if ( !settings.PrepareProfiles( pszError ) )
	{
		return false;
	}
	pSettings = std::make_unique<CGeneralSettings>( settings );
	return true;
}

bool InitializeGeneralSettings( std::string *pszError )
{
	try
	{
		return InitializeGeneralSettings(
				CGeneralSettings( GetInstallationDirectory(), NFile::JoinPath( GetApplicationDataDirectory(), "Profiles" ) ),
				pszError );
	}
	catch ( const std::exception &error )
	{
		*pszError = error.what();
		return false;
	}
}

const CGeneralSettings *GetGeneralSettings()
{
	return pSettings.get();
}

} // namespace NSettings
