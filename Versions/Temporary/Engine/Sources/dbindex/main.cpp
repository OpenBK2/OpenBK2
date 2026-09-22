#include "stdafx.h"

#include "libdb/Db.h"
#include "Misc/HPTimer.h"
#include "Misc/StrProc.h"
#include "System/FileUtils.h"
#include "System/FilePath.h"
#include "System/VFS.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include "port/cdecl.h"

// SaveChanges and RegisterResourceFile used to be declared here by hand, which
// dropped the LIBDB_EXPORT off both and left the linker to find them by name.
// They are declared in EditorDb.h, despite being the pair this game-mode tool
// needs: CGameDatabase implements them for real, and only the editor-specific
// calls around them assert in game mode.
#include "libdb/EditorDb.h"

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

int PORT_CDECL main( int argc, char *argv[] )
{
	// NFile::GetNormalizedCurrDir is what the rest of the tree uses in place of
	// GetCurrentDirectory. It already returns forward slashes with one on the
	// end, so the NormalizePath and AppendSlash that followed are gone with it,
	// and it fixes the bug the 1024 byte buffer carried: GetCurrentDirectory
	// wrote nothing and returned the size it wanted when the path did not fit,
	// leaving uninitialised stack behind.
	const std::string szCWD = NFile::GetNormalizedCurrDir();
	//
	NGlobal::SetVar( "code_version_number", REVISION_NUMBER_STR );
	NGlobal::SetVar( "code_build_date_time", BUILD_DATE_TIME_STR );
	if ( argc >= 2 && std::string(argv[1]) == "-show-version"  )
	{
		printf( "Version: %s\n", REVISION_NUMBER_STR );
		printf( "Build date/time: %s\n", BUILD_DATE_TIME_STR );
		return 0;
	}

	std::string szDataDirectory;
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
	std::string szIndexFile = (szCWD + "index.bin");
	std::string szIndexFileBackup = (szCWD + "index.bin.backup");
	printf( "Building index for database root \"%s\"\n", szDataDirectory.c_str() );
	printf( "Index will be stored as \"%s\"\n", szIndexFile.c_str() );
	//
	NHPTimer::STime hptime;
	NHPTimer::GetTime( &hptime );
	//
	if ( NFile::DoesFileExist(szIndexFile) )
		// NFile::RenameFile, not MoveFile: windows.h rewrites that name, and this
		// is the tree's portable spelling of the same move.
		NFile::RenameFile( szIndexFile, szIndexFileBackup );
	//
	CObj<NVFS::IVFS> pMainVFS = NVFS::CreateWinVFS( szDataDirectory );
	CObj<NVFS::IFileCreator> pMainFileCreator = NVFS::CreateWinFileCreator( szCWD );
	NVFS::SetMainVFS( pMainVFS );
	NVFS::SetMainFileCreator( pMainFileCreator );

	NDb::OpenDatabase( pMainVFS, pMainFileCreator, NDb::DATABASE_MODE_GAME );
	//
	printf( "Retrieving files list...\n" );
	std::vector<std::string> filenames;
	pMainVFS->GetAllFileNames( &filenames, std::string() );
	// size() is size_t, so %d was wrong on x64 and read the wrong half of it
	printf( "Processing files (%zu files)...\n", filenames.size() );
	for ( std::vector<std::string>::const_iterator it = filenames.begin(); it != filenames.end(); ++it )
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
			NFile::RenameFile( szIndexFileBackup, szIndexFile );
		}
		return 0xDEAD;
	}
	//
	const float fSeconds = NHPTimer::GetTimePassed( &hptime );
	printf( "Done. %d objects collected at %g seconds\n", s_nNumCollectedObjects, fSeconds );
	//
	return 0;
}
