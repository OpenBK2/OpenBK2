#include "stdafx.h"
#include "Profiles.h"
#include "Misc/StrProc.h"
#include "MainLoop.h"
#include "System/FileUtils.h"
#include "System/FilePath.h"
#include <objbase.h>
#include "Input/Bind.h"

namespace NProfile
{
static string GetProfileRootDir()
{
	return NMainLoop::GetBaseDir() + "Profiles\\";
}

// directory name with default profile settings
// these settings are loaded in LoadProfile() if no data for specified in global.cfg profile is found 
// these settings are also loaded when ResetToDefault() is called
// in game installation in Profiles\\ directory there should be only this profile and correct localized 
// default player name should be specified in global.cfg (then during first run settings will be loaded 
// from szDefaultProfileName and then will be saved to normal profile dir)
static string szDefaultProfileName = "default_profile";
static bool IsDefaultProfileName( const string &szName )
{
	string szLowName;
	NStr::ToLower( &szLowName, szName );
	return szLowName == szDefaultProfileName;
}

static wstring GetProfileName( const string &szDir, const string &_szDirName )
{
	CFileStream stream( szDir + "\\" + _szDirName + "\\name.txt", CFileStream::WIN_READ_ONLY );
	if  ( stream.IsOk() )
	{
		int nLength = stream.GetSize();
		if ( nLength > 0 )
		{
			wchar_t buffer[1280];
			int len = Min( ARRAY_SIZE(buffer)-1, nLength / sizeof( wchar_t ) );
			stream.Read( buffer, len * sizeof( wchar_t ) );
			buffer[len] = 0;
			wstring szName;
			szName.assign( &buffer[1], Max( 0, len - 1 ) );
			return szName;
		}
	}
	return NStr::ToUnicode( _szDirName );
}

string GetDefaultProfileDir()
{
	string szDefaultDir = GetProfileRootDir() + szDefaultProfileName + "\\";
	return szDefaultDir;
}

// if such profile dir does not exist it will be created
static string GetProfileDir( const wstring &szName )
{
	string szRoot = GetProfileRootDir();
	string szResDir;
	string szNameAscii = NStr::ToMBCS( szName );
	if ( NFile::IsValidDirName( szNameAscii ) && !IsDefaultProfileName( szNameAscii ) )
	{
		szResDir = szRoot + szNameAscii + "\\";
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
		for ( NFile::CFileIterator it( ( szRoot + "*.*" ) ); !it.IsEnd(); ++it )
		{
			if ( !it.IsDirectory() || it.IsDots() )
				continue;
			if ( GetProfileName( szRoot, it.GetFileName() ) == szName )
				return it.GetFullName() + "\\";
		}
		// create dir
		GUID guid;
		CoCreateGuid( &guid );
		NStr::GUID2String( &szResDir, guid );
		szResDir = szRoot + szResDir + '\\';
		NFile::CreatePath( szResDir );
		// write name
		{
			CFileStream stream( szResDir + "name.txt", CFileStream::WIN_CREATE );
			WORD wUnicodeMagic = 0xFEFF;
			stream.Write( &wUnicodeMagic, 2 );
			stream.Write( szName.data(), sizeof(wchar_t) * szName.size() );
		}
	}
	// copy configs from default
	::CopyFile( (GetDefaultProfileDir() + "user.cfg").c_str(), (szResDir + "user.cfg").c_str(), false );
	::CopyFile( (GetDefaultProfileDir() + "input.cfg").c_str(), (szResDir + "input.cfg").c_str(), false );
	return szResDir;
}

static void OnProfileChange()
{
	// if it happens to be project dependent we could separate this into special .cfg
	NGlobal::ProcessCommand( L"autodetect" ); 
}

static void LoadUserConfig( const string &szProfileDir )
{
	NInput::SetSection( "" );
	NGlobal::LoadConfig( szProfileDir + "user.cfg", STORAGE_USER );
	NGlobal::ProcessCommand( L"unbindall" );
	NGlobal::LoadConfig( szProfileDir + "input.cfg" );
	NGlobal::ProcessCommand( L"bind_update" );
}

void LoadProfile()
{
	NGlobal::LoadConfig( GetProfileRootDir() + "global.cfg", STORAGE_GLOBAL );
	OnProfileChange();
	string szProfileDir = GetProfileDir( GetCurrentProfileName() );	
	if ( NFile::DoesFileExist( szProfileDir + "user.cfg" ) )
		LoadUserConfig( szProfileDir );
	else
		LoadUserConfig( GetDefaultProfileDir() );
}

void SaveProfile()
{
	string szGlobalCfg = GetProfileRootDir() + "global.cfg";
	string szUserCfg = GetProfileDir( GetCurrentProfileName() ) + "user.cfg";
	NGlobal::SaveAllVars( szGlobalCfg, szUserCfg );
}

bool AddProfile( const wstring &szName )
{
	GetProfileDir( szName );
	return true;
}

void ChangeProfile( const wstring &szProfile )
{
	SaveProfile();
	NGlobal::SetVar( "profile_name", szProfile );
	NGlobal::ResetVarsToDefault( STORAGE_USER );
	OnProfileChange();
	LoadUserConfig( GetProfileDir( GetCurrentProfileName() ) );
}

bool RemoveProfile( const wstring &szProfile )
{
	string szDir = GetProfileDir( szProfile );
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

void GetAllProfiles( vector<wstring> *pRes )
{
	string szRoot = GetProfileRootDir();
	pRes->resize( 0 );
	for ( NFile::CFileIterator it( ( szRoot + "*.*" ) ); !it.IsEnd(); ++it )
	{
		if ( !it.IsDirectory() || it.IsDots() )
			continue;
		if ( IsDefaultProfileName( it.GetFileName() ) )
			continue;
		pRes->push_back( GetProfileName( szRoot, it.GetFileName() ) );
	}
}

wstring GetCurrentProfileName()
{
	return NGlobal::GetVar( "profile_name", "default" );
}

string GetCurrentProfileDir()
{
	return GetProfileDir( GetCurrentProfileName() );
}

static void RemoveProfile( const string &szID, const vector<wstring> &szParams, void *pContext )
{
	if ( szParams.size() == 1 )
		RemoveProfile( szParams[0] );
	else
		csSystem << "Usage : remove_profile <user name>" << endl;
}

static void ChangeProfile( const string &szID, const vector<wstring> &szParams, void *pContext )
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

