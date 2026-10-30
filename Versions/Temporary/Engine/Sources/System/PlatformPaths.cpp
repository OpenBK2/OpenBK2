#include "stdafx.h"

#include "FileUtils.h"
#include "PlatformPaths.h"
#include "port/unicode.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <system_error>
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

	bool IsValidApplicationName( const std::string &szName )
	{
		// Apply the same portable filename rules on every OS. In particular, a
		// Linux-valid slash-free name can still be a device or stream on Windows.
		if ( szName.empty() || szName.size() > 255 || szName.back() == '.' ||
		     szName.find_first_of( "<>:\"/\\|?*" ) != std::string::npos || WideToUTF8( UTF8ToWide( szName ) ) != szName )
		{
			return false;
		}
		for ( const unsigned char c : szName )
		{
			if ( c < 32 || c == 127 )
			{
				return false;
			}
		}
		// Device names remain reserved when followed by an extension.
		std::string szStem = szName.substr( 0, szName.find( '.' ) );
		while ( !szStem.empty() && szStem.back() == ' ' )
		{
			szStem.pop_back();
		}
		for ( char &c : szStem )
		{
			if ( c >= 'a' && c <= 'z' )
			{
				c -= 'a' - 'A';
			}
		}
		if ( szStem == "CON" || szStem == "PRN" || szStem == "AUX" || szStem == "NUL" || szStem == "CONIN$" ||
		     szStem == "CONOUT$" )
		{
			return false;
		}
		if ( szStem.compare( 0, 3, "COM" ) == 0 || szStem.compare( 0, 3, "LPT" ) == 0 )
		{
			const auto szNumber = szStem.substr( 3 );
			if ( ( szNumber.size() == 1 && szNumber[0] >= '1' && szNumber[0] <= '9' ) || szNumber == u8"\u00b9" ||
			     szNumber == u8"\u00b2" || szNumber == u8"\u00b3" )
			{
				return false;
			}
		}
		return true;
	}

	std::string ReadApplicationName( const std::filesystem::path &install )
	{
		const auto filePath = install / "app_name.txt";
		std::error_code error;
		// Do not try to read a directory or a pipe supplied in place of the file.
		if ( !std::filesystem::is_regular_file( filePath, error ) || error )
		{
			return {};
		}
		std::ifstream file( filePath, std::ios::binary );
		if ( !file )
		{
			return {};
		}
		// A tiny config file should not turn into an unbounded read at startup.
		std::array<char, 1024> buffer{};
		file.read( buffer.data(), buffer.size() );
		if ( file.bad() || !file.eof() )
		{
			return {};
		}
		std::string szName( buffer.data(), static_cast<size_t>( file.gcount() ) );
		if ( szName.compare( 0, 3, "\xef\xbb\xbf" ) == 0 )
		{
			szName.erase( 0, 3 );
		}
		const auto first = szName.find_first_not_of( " \t\r\n" );
		if ( first == std::string::npos )
		{
			return {};
		}
		szName = szName.substr( first, szName.find_last_not_of( " \t\r\n" ) - first + 1 );
		return IsValidApplicationName( szName ) ? szName : std::string();
	}

} // namespace

#if BOOST_OS_WINDOWS
std::string GetWindowsApplicationDataDirectory();
#elif BOOST_OS_MACOS
// Foundation supplies the user-domain directory, including an app's container.
std::string GetMacApplicationSupportDirectory();
#endif

namespace
{
	std::string GetApplicationDataRoot()
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
} // namespace

std::string GetApplicationDataDirectory( const std::string &szInstallRoot, const std::string &szDataRoot )
{
	const auto szName = ReadApplicationName( std::filesystem::u8path( szInstallRoot ) );
	return szName.empty() ? szDataRoot
	                      : ( std::filesystem::u8path( szDataRoot ) / std::filesystem::u8path( szName ) ).u8string();
}

std::string GetApplicationDataDirectory()
{
	// Use the installation root, not the working directory or the executable's bin folder.
	return GetApplicationDataDirectory( GetInstallationDirectory(), GetApplicationDataRoot() );
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
