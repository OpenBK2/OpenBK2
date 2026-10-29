#include "stdafx.h"

#include "FileUtils.h"
#include "PlatformPaths.h"

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <vector>

#include <boost/predef.h>

#if BOOST_OS_WINDOWS
#include <windows.h>
#else
#include <pwd.h>
#include <unistd.h>
#endif

namespace NSettings
{
namespace
{

	std::filesystem::path AbsoluteEnvironmentPath( const char *pszName )
	{
#if BOOST_OS_WINDOWS
		// getenv's narrow encoding depends on the caller's CRT locale.
		std::wstring wszName;
		for ( const char *p = pszName; *p; ++p )
		{
			wszName += *p;
		}
		const DWORD nSize = GetEnvironmentVariableW( wszName.c_str(), nullptr, 0 );
		if ( nSize == 0 )
		{
			return {};
		}
		std::vector<wchar_t> buffer( nSize );
		const DWORD nLength = GetEnvironmentVariableW( wszName.c_str(), buffer.data(), nSize );
		if ( nLength == 0 || nLength >= nSize )
		{
			return {};
		}
		const std::filesystem::path path( buffer.data() );
#else
		const char *pszValue = std::getenv( pszName );
		const std::filesystem::path path = pszValue ? std::filesystem::u8path( pszValue ) : std::filesystem::path();
#endif
		return path.is_absolute() ? path.lexically_normal() : std::filesystem::path();
	}

} // namespace

#if BOOST_OS_WINDOWS
std::string GetWindowsApplicationDataDirectory();
#elif BOOST_OS_MACOS
// Foundation supplies the user-domain directory, including an app's container.
std::string GetMacApplicationSupportDirectory();
#endif

std::string GetApplicationDataDirectory()
{
	const auto overridePath = AbsoluteEnvironmentPath( "OPENBK2_USER_DATA_DIR" );
	if ( !overridePath.empty() )
	{
		return overridePath.u8string();
	}
#if BOOST_OS_WINDOWS
	return GetWindowsApplicationDataDirectory();
#elif BOOST_OS_MACOS
	return GetMacApplicationSupportDirectory();
#else
	auto path = AbsoluteEnvironmentPath( "XDG_DATA_HOME" );
	if ( path.empty() )
	{
		path = AbsoluteEnvironmentPath( "HOME" );
		if ( path.empty() )
		{
			// HOME is not guaranteed in service or minimal desktop sessions.
			std::vector<char> buffer( 16384 );
			passwd entry{};
			passwd *pEntry = nullptr;
			if ( getpwuid_r( getuid(), &entry, buffer.data(), buffer.size(), &pEntry ) == 0 && pEntry )
			{
				path = std::filesystem::u8path( entry.pw_dir );
			}
		}
		if ( !path.is_absolute() )
		{
			throw std::runtime_error( "Cannot locate the user's home directory." );
		}
		path /= ".local/share";
	}
	return ( path / "OpenBK2" ).u8string();
#endif
}

std::string GetInstallationDirectory()
{
	const auto overridePath = AbsoluteEnvironmentPath( "OPENBK2_DATA_DIR" );
	if ( !overridePath.empty() )
	{
		return overridePath.u8string();
	}
	const auto executable = std::filesystem::u8path( NFile::GetExecutablePath() );
	if ( !executable.is_absolute() )
	{
		throw std::runtime_error( "Cannot locate the game executable." );
	}
	const auto directory = executable.parent_path();
#if BOOST_OS_MACOS
	if ( directory.filename() == "MacOS" && directory.parent_path().filename() == "Contents" )
	{
		return ( directory.parent_path() / "Resources" ).u8string();
	}
#endif
	// The installed layout is <game>/bin/<executable>, independent of cwd.
	return directory.parent_path().u8string();
}

} // namespace NSettings
