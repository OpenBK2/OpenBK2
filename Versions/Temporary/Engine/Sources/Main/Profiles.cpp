#include "stdafx.h"

#include "port/unicode.h"
#include "Profiles.h"
#include "Misc/StrProc.h"
#include "MainLoop.h"
#include "System/FileUtils.h"
#include "System/FilePath.h"

#if BOOST_OS_WINDOWS
#include <objbase.h>
#endif

#include "Input/Bind.h"

#include <cstdint>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace NProfile
{
static std::string GetProfileRootDir()
{
	return NFile::JoinPath( NMainLoop::GetBaseDir(), NFile::DIR_PROFILES );
}

// directory name with default profile settings
// these settings are loaded in LoadProfile() if no data for specified in global.cfg profile is found 
// these settings are also loaded when ResetToDefault() is called
// in game installation in Profiles\\ directory there should be only this profile and correct localized 
// default player name should be specified in global.cfg (then during first run settings will be loaded 
// from szDefaultProfileName and then will be saved to normal profile dir)
static std::string szDefaultProfileName = "default_profile";
static bool IsDefaultProfileName( const std::string &szName )
{
	std::string szLowName;
	NStr::ToLower( &szLowName, szName );
	return szLowName == szDefaultProfileName;
}

static std::wstring GetProfileName( const std::string &szDir, const std::string &_szDirName )
{
	CFileStream stream( NFile::JoinPath( szDir, _szDirName, "name.txt" ), CFileStream::WIN_READ_ONLY );
	if  ( stream.IsOk() )
	{
		int nLength = stream.GetSize();
		if ( nLength > 0 )
		{
			// name.txt is UTF-16LE behind a 0xFEFF BOM, which is two bytes per
			// character and not sizeof( wchar_t ): the same thing on Windows, and
			// half of it off Windows, where the old arithmetic read a quarter of
			// the file and skipped two characters for a one character mark.
			std::vector<char> buffer( (std::min)( size_t( 2560 ), size_t( nLength ) ) );
			stream.Read( &buffer[0], buffer.size() );
			const std::wstring szName = UTF16LEToWide( &buffer[0], buffer.size() );
			// drop the byte order mark, which the writer below puts there
			return szName.empty() || szName[0] != L'\xFEFF' ? szName : szName.substr( 1 );
		}
	}
	return NStr::ToUnicode( _szDirName );
}

std::string GetDefaultProfileDir()
{
	std::string szDefaultDir = NFile::JoinPath( GetProfileRootDir(), szDefaultProfileName ) + NFile::PATH_SEPARATOR;
	return szDefaultDir;
}

// if such profile dir does not exist it will be created
static std::string GetProfileDir( const std::wstring &szName )
{
	std::string szRoot = GetProfileRootDir();
	std::string szResDir;
	std::string szNameAscii = NStr::ToMBCS( szName );
	if ( NFile::IsValidDirName( szNameAscii ) && !IsDefaultProfileName( szNameAscii ) )
	{
		szResDir = NFile::JoinPath( szRoot, szNameAscii ) + NFile::PATH_SEPARATOR;
		if ( NFile::DoesFileExist( szResDir + "user.cfg" ) )
			return szResDir;
		else
		{
			NFile::CreatePath( szResDir );
			// should not use file name.txt to specify profile name since directory name is already
			// used as profile name
		}
	}
	else
	{
		for ( NFile::CFileIterator it( NFile::JoinPath( szRoot, "*.*" ) ); !it.IsEnd(); ++it )
		{
			if ( !it.IsDirectory() || it.IsDots() )
				continue;
			if ( GetProfileName( szRoot, it.GetFileName() ) == szName )
				return it.GetFullName() + "/";
		}
		// create dir
		const auto guid = boost::uuids::random_generator()();
		szResDir = NFile::JoinPath( szRoot, boost::uuids::to_string(guid) ) + NFile::PATH_SEPARATOR;
		NFile::CreatePath( szResDir );
		// write name
		{
			CFileStream stream( szResDir + "name.txt", CFileStream::WIN_CREATE );
			uint16_t wUnicodeMagic = 0xFEFF;
			stream.Write( &wUnicodeMagic, 2 );
			// UTF-16LE to match the mark just written, rather than whatever width
			// wchar_t happens to be on this platform
			const std::string utf16 = WideToUTF16LE( szName );
			if ( !utf16.empty() )
			{
				stream.Write( utf16.data(), utf16.size() );
			}
		}
	}
	// copy configs from default
	NFile::CopyFile( GetDefaultProfileDir() + "user.cfg", szResDir + "user.cfg" );
	NFile::CopyFile( GetDefaultProfileDir() + "input.cfg", szResDir + "input.cfg" );
	return szResDir;
}

static void OnProfileChange()
{
	// if it happens to be project dependent we could separate this into special .cfg
	NGlobal::ProcessCommand( L"autodetect" ); 
}

static void LoadUserConfig( const std::string &szProfileDir )
{
	NInput::SetSection( "" );
	NGlobal::LoadConfig( szProfileDir + "user.cfg", STORAGE_USER );
	NGlobal::ProcessCommand( L"unbindall" );
	NGlobal::LoadConfig( szProfileDir + "input.cfg" );
	NGlobal::ProcessCommand( L"bind_update" );
}

void LoadProfile()
{
	NGlobal::LoadConfig( NFile::JoinPath( GetProfileRootDir(), "global.cfg" ), STORAGE_GLOBAL );
	OnProfileChange();
	std::string szProfileDir = GetProfileDir( GetCurrentProfileName() );
	if ( NFile::DoesFileExist( szProfileDir + "user.cfg" ) )
		LoadUserConfig( szProfileDir );
	else
		LoadUserConfig( GetDefaultProfileDir() );
}

void SaveProfile()
{
	std::string szGlobalCfg = NFile::JoinPath( GetProfileRootDir(), "global.cfg" );
	std::string szUserCfg = GetProfileDir( GetCurrentProfileName() ) + "user.cfg";
	NGlobal::SaveAllVars( szGlobalCfg, szUserCfg );
}

bool AddProfile( const std::wstring &szName )
{
	GetProfileDir( szName );
	return true;
}

void ChangeProfile( const std::wstring &szProfile )
{
	SaveProfile();
	NGlobal::SetVar( "profile_name", szProfile );
	NGlobal::ResetVarsToDefault( STORAGE_USER );
	OnProfileChange();
	LoadUserConfig( GetProfileDir( GetCurrentProfileName() ) );
}

bool RemoveProfile( const std::wstring &szProfile )
{
	std::string szDir = GetProfileDir( szProfile );
	NFile::DeleteDirectory( szDir );
	return true;
}

void ResetToDefault()
{
	NGlobal::ResetVarsToDefault( STORAGE_USER );
	OnProfileChange();
	LoadUserConfig( GetDefaultProfileDir() );
	SaveProfile();
}

void GetAllProfiles( std::vector<std::wstring> *pRes )
{
	std::string szRoot = GetProfileRootDir();
	pRes->resize( 0 );
	for ( NFile::CFileIterator it( NFile::JoinPath( szRoot, "*.*" ) ); !it.IsEnd(); ++it )
	{
		if ( !it.IsDirectory() || it.IsDots() )
			continue;
		if ( IsDefaultProfileName( it.GetFileName() ) )
			continue;
		pRes->push_back( GetProfileName( szRoot, it.GetFileName() ) );
	}
}

std::wstring GetCurrentProfileName()
{
	return NGlobal::GetVar( "profile_name", "default" );
}

std::string GetCurrentProfileDir()
{
	return GetProfileDir( GetCurrentProfileName() );
}

std::string GetSaveDir()
{
	return NFile::JoinPath( GetCurrentProfileDir(), "Saves" ) + NFile::PATH_SEPARATOR;
}

std::string GetReplayDir()
{
	return NFile::JoinPath( GetCurrentProfileDir(), "Replays" ) + NFile::PATH_SEPARATOR;
}

static void RemoveProfile( const std::string &szID, const std::vector<std::wstring> &szParams, void *pContext )
{
	if ( szParams.size() == 1 )
		RemoveProfile( szParams[0] );
	else
		csSystem << "Usage : remove_profile <user name>" << endl;
}

static void ChangeProfile( const std::string &szID, const std::vector<std::wstring> &szParams, void *pContext )
{
	if ( szParams.size() == 1 )
		ChangeProfile( szParams[0] );
	else
		csSystem << "Usage: change_profile <user name>" << endl;
}

START_REGISTER(Profiles)
REGISTER_CMD( "remove_profile", RemoveProfile );
REGISTER_CMD( "change_profile", ChangeProfile );
REGISTER_VAR( "profile_name", 0, "default", STORAGE_GLOBAL )
FINISH_REGISTER
}

