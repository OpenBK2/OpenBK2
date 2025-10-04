#include "stdafx.h"

#include "revision.h"
#include "libdb/EditorDb.h"
#include "libdb/TypeDef.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"
#include "System/FilePath.h"
#include "System/FileUtils.h"
#include "System/CmdLine.h"

namespace
{

class CDatabaseGuard
{
	bool bSuccessfullyOpened;
public:
	explicit CDatabaseGuard( const string &szCWD, NDb::EDatabaseMode eDBMode )
	{
		NVFS::SetMainVFS( NVFS::CreateWinVFS(szCWD) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator(szCWD) );
		bSuccessfullyOpened = NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), eDBMode );
	}
	~CDatabaseGuard()
	{
		NDb::CloseDatabase();
		NVFS::SetMainFileCreator( 0 );
		NVFS::SetMainVFS( 0 );
	}
	bool IsOk() const { return bSuccessfullyOpened; }
};

void __stdcall Log( const char *pszFormat, ... )
{
	static const int BUF_SIZE = 1024;
	static char charBuff[BUF_SIZE];
	//
	va_list va;
	va_start( va, pszFormat );
	_vsnprintf( charBuff, BUF_SIZE - 1, pszFormat, va );
	va_end( va );
	OutputDebugString( charBuff );
	OutputDebugString( "\n" );
	printf( charBuff );
	printf( "\n" );
}


enum EDBStructMode
{
	MODE_UNKNOWN,
	MODE_UPDATE_STRUCT,
	MODE_MAKE_BIN,
	MODE_SHOW_VERSION,
};

struct ILoadObjectCallback
{
	virtual void ObjectLoaded( NDb::IObjMan *pObjMan, const CDBID &dbid ) = 0;
};

int LoadAllObjects( ILoadObjectCallback *pCallback )
{
	vector<NDb::NTypeDef::STypeClass*> classes;
	if ( NDb::GetClassesList(&classes) && !classes.empty() )
	{
		int nCounter = 0;
		for ( vector<NDb::NTypeDef::STypeClass*>::const_iterator itClass = classes.begin(); itClass != classes.end(); ++itClass, ++nCounter )
		{
			vector<CDBID> objects;
			if ( NDb::GetObjectsList(&objects, (*itClass)->szTypeName) && !objects.empty() )
			{
				Log( "(%d of %d): Loading objects of type \"%s\" (total %d objects)", nCounter, classes.size(), (*itClass)->szTypeName.c_str(), objects.size() );
				for ( vector<CDBID>::const_iterator itDBID = objects.begin(); itDBID != objects.end(); ++itDBID )
				{
					NDb::IObjMan *pObjMan = NDb::GetManipulator( *itDBID );
					if ( pCallback != 0 )
						pCallback->ObjectLoaded( pObjMan, *itDBID );
				}
			}
			else
			{
				Log( "WARNING: no objects of type \"%s\"", (*itClass)->szTypeName.c_str() );
			}
		}
	}
	else
	{
		Log( "ERROR: Can't find any type!" );
		return 0xDEAD;
	}
	return 0;
}

// ************************************************************************************************************************ //
// **
// ** 
// **
// **
// **
// ************************************************************************************************************************ //

class CMarkChangedCallback : public ILoadObjectCallback
{
	void ObjectLoaded( NDb::IObjMan *pObjMan, const CDBID &dbid )
	{
		NDb::MarkChanged( dbid );
	}
};

int UpdateStruct()
{
	Log( "Loading all objects and modifying structure" );
	//
	CMarkChangedCallback callbackMarkChanged;
	int nRetCode = LoadAllObjects( &callbackMarkChanged );
	if ( nRetCode != 0 )
		return nRetCode;
	//
	Log( "All objects loaded and structure modified. Saving..." );
	NDb::SaveChanges();
	Log( "Done." );
	//
	return 0;
}

// ************************************************************************************************************************ //
// **
// ** 
// **
// **
// **
// ************************************************************************************************************************ //

class CMakeBinCallback : public ILoadObjectCallback
{
	void ObjectLoaded( NDb::IObjMan *pObjMan, const CDBID &dbid )
	{
	}
};

int MakeBin()
{
	printf( "make bin functionality not realized\n" );
	return 0xDEAD;
//	CMakeBinCallback callbackMakeBin;
//	int nRetCode = LoadAllObjects( &callbackMakeBin );
//	if ( nRetCode != 0 )
//		return nRetCode;
}

}

int __cdecl main( int argc, char *argv[] )
{
	const string szCWD = NFile::GetNormalizedCurrDir();
	//
	EDBStructMode eMode = MODE_UNKNOWN;
	string szDataPath = szCWD;
	NCmdLine::CCmdLine cmdLine( "XML Database structure utility\nWritten by Yuri Blazhevich\n(C) Nival Interactive, 2005\n" );
	cmdLine.AddOption( "-show-version", &eMode, MODE_SHOW_VERSION, "show current product version" );
	cmdLine.AddOption( "-update-struct", &eMode, MODE_UPDATE_STRUCT, "update all database objects to new structure in accordance with types" );
	cmdLine.AddOption( "-make-bin", &eMode, MODE_MAKE_BIN, "convert .xdb files to packed binary" );
	cmdLine.AddOption( "--data-path", &szDataPath, "set data path to operate (default: current dir)" );
	//
	NGlobal::SetVar( "code_version_number", REVISION_NUMBER_STR );
	NGlobal::SetVar( "code_build_date_time", BUILD_DATE_TIME_STR );
	//
	cmdLine.PrintHeader();
	if ( cmdLine.Process( argc, argv ) != NCmdLine::CCmdLine::PROC_RESULT_OK )
		return 0xDEAD;
	//
	if ( eMode == MODE_UNKNOWN )
		return cmdLine.PrintUsage( "Usage: dbstruct.exe [options]" );
	else if ( eMode == MODE_SHOW_VERSION )
	{
		printf( "Version: %s\n", REVISION_NUMBER_STR );
		printf( "Build date/time: %s\n", BUILD_DATE_TIME_STR );
		return 0;
	}
	//
	NFile::AppendSlash( &szDataPath );
	//
	Log( "Operating on \"%s\"", szDataPath.c_str() );
	CDatabaseGuard dbGuard( szDataPath, NDb::DATABASE_MODE_EDITOR );
	if ( !dbGuard.IsOk() )
		return 0xDEAD;
	//
	if ( eMode == MODE_UPDATE_STRUCT )
		return UpdateStruct();
	else if ( eMode == MODE_MAKE_BIN )
		return MakeBin();
	//
	return 0;
}


