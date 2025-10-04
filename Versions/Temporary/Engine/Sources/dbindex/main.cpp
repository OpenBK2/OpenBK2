#include "stdafx.h"

#include "revision.h"
#include "libdb/Db.h"
#include "Misc/HPTimer.h"
#include "Misc/StrProc.h"
#include "System/FileUtils.h"
#include "System/FilePath.h"
#include "System/VFS.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"


namespace NDb
{
	void SaveChanges();
	bool RegisterResourceFile( const string &szFileName );
}

static int s_nNumCollectedObjects = 0;
int PrintUsage()
{
	printf( "Database index build utility\n" );
	printf( "Written by [REDACTED]\n" );
	printf( "(C) [REDACTED], 2005\n" );
	printf( "\n" );
	printf( "Usage: dbindex\n" );
	printf( "  or   dbindex <data-root-directory>\n" );
	printf( "\n" );
	printf( "In first (implicit) form dbindex takes current working directory as data root.\n" );
	printf( "Second form allows explicit specification of data root.\n" );
	printf( "Result \"index.bin\" is always created in current working directory of dbindex.\n" );
	return 0xDEAD;
}

int __cdecl main( int argc, char *argv[] )
{
	string szCWD;
	{
		char buffer[1024];
		buffer[0] = 0;
		GetCurrentDirectory( 1024, buffer );
		szCWD = buffer;
		NFile::NormalizePath( &szCWD );
		NFile::AppendSlash( &szCWD, '/' );
	}
	//
	NGlobal::SetVar( "code_version_number", REVISION_NUMBER_STR );
	NGlobal::SetVar( "code_build_date_time", BUILD_DATE_TIME_STR );
	if ( argc >= 2 && string(argv[1]) == "-show-version"  )
	{
		printf( "Version: %s\n", REVISION_NUMBER_STR );
		printf( "Build date/time: %s\n", BUILD_DATE_TIME_STR );
		return 0;
	}

	string szDataDirectory;
	switch ( argc )
	{
	case 1:
		{
			szDataDirectory = szCWD;
		}
		break;
	case 2:
		{
			szDataDirectory = argv[1];
			NFile::AppendSlash( &szDataDirectory, '/' );
			if ( NFile::IsPathRelative(szDataDirectory) )
			{
				NFile::MakeFullPath( &szDataDirectory, szDataDirectory, szCWD );
			}
			NFile::NormalizePath( &szDataDirectory );
		}
		break;
	}
	if ( szDataDirectory.empty())
	{
		return PrintUsage();
	}
	if ( !NFile::DoesFolderExist(szDataDirectory) )
	{
		PrintUsage();
		printf( "\nERROR: Specified database root directory does not exist.\n\n");
		return 0xDEAD;
	}
	//
	string szIndexFile = (szCWD + "index.bin");
	string szIndexFileBackup = (szCWD + "index.bin.backup");
	printf( "Building index for database root \"%s\"\n", szDataDirectory.c_str() );
	printf( "Index will be stored as \"%s\"\n", szIndexFile.c_str() );
	//
	NHPTimer::STime hptime;
	NHPTimer::GetTime( &hptime );
	//
	if ( NFile::DoesFileExist(szIndexFile) )
		::MoveFile( szIndexFile.c_str(), szIndexFileBackup.c_str() );
	//
	CObj<NVFS::IVFS> pMainVFS = NVFS::CreateWinVFS( szDataDirectory );
	CObj<NVFS::IFileCreator> pMainFileCreator = NVFS::CreateWinFileCreator( szCWD );
	NVFS::SetMainVFS( pMainVFS );
	NVFS::SetMainFileCreator( pMainFileCreator );

	NDb::OpenDatabase( pMainVFS, pMainFileCreator, NDb::DATABASE_MODE_GAME );
	//
	printf( "Retrieving files list...\n" );
	vector<string> filenames;
	pMainVFS->GetAllFileNames( &filenames, string() );
	printf( "Processing files (%d files)...\n", filenames.size() );
	for ( vector<string>::const_iterator it = filenames.begin(); it != filenames.end(); ++it )
	{
		const int nSize = it->size();
		if ( it->size() < 4 )
			continue;
		//
		if ( (*it)[nSize - 4] == '.' && 
			   NStr::ASCII_tolower((*it)[nSize - 3]) == 'x' &&
				 NStr::ASCII_tolower((*it)[nSize - 2]) == 'd' &&
				 NStr::ASCII_tolower((*it)[nSize - 1]) == 'b' )
		{
			if ( NDb::RegisterResourceFile( *it ) == false )
				printf( "ERROR: can't register resource \"%s\"\n", it->c_str() );
			++s_nNumCollectedObjects;
		}
	}
	NDb::SaveChanges();
	//
	NDb::CloseDatabase();
	pMainVFS = 0;
	pMainFileCreator = 0;
	//
	if ( NFile::DoesFileExist(szIndexFile) == false )
	{
		printf( "Building database index failed!\n" );
		if ( NFile::DoesFileExist(szIndexFileBackup) )
		{
			printf( "Restoring previous index\n" );
			::MoveFile( szIndexFileBackup.c_str(), szIndexFile.c_str() );
		}
		return 0xDEAD;
	}
	//
	const float fSeconds = NHPTimer::GetTimePassed( &hptime );
	printf( "Done. %d objects collected at %g seconds\n", s_nNumCollectedObjects, fSeconds );
	//
	return 0;
}
