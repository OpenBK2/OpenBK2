#include "stdafx.h"

#include "libdb/EditorDb.h"
#include "libdb/TypeDef.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"
#include "System/FilePath.h"
#include "System/FileUtils.h"

#include "port/cdecl.h"

#include <fmt/printf.h>

#include <boost/program_options.hpp>

#include <iostream>

namespace po = boost::program_options;

namespace
{

class CDatabaseGuard
{
	bool bSuccessfullyOpened;
public:
	explicit CDatabaseGuard( const std::string &szCWD, NDb::EDatabaseMode eDBMode )
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

// The call sites keep their printf spelling, but fmt::sprintf formats it: the
// arguments are checked against the format and sized from their own types, so
// the %d that several of these pass a size_t through is no longer wrong on a
// 64-bit build.
//
// What this replaces had three separate problems. printf( charBuff ) passed
// formatted text back as a format string, so any percent sign that came out of
// a type or object name was interpreted a second time. _vsnprintf is the MSVC
// spelling and does not terminate the buffer when the text does not fit, which
// the fixed 1024 bytes here made reachable. OutputDebugString is Windows only.
//
// The debugger mirror goes rather than moving to DebugTrace, which would be the
// tree's spelling for it: DbgTrcRaw writes to stderr as well as the debugger
// pane, so a console would have shown every line of this twice. What it mirrors
// is this tool's entire output, not debug tracing, and stdout is where that
// belongs.
template < typename... TArgs >
void Log( const char *pszFormat, const TArgs &... args )
{
	fmt::print( "{}\n", fmt::sprintf( pszFormat, args... ) );
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
	std::vector<NDb::NTypeDef::STypeClass*> classes;
	if ( NDb::GetClassesList(&classes) && !classes.empty() )
	{
		int nCounter = 0;
		for ( std::vector<NDb::NTypeDef::STypeClass*>::const_iterator itClass = classes.begin(); itClass != classes.end(); ++itClass, ++nCounter )
		{
			std::vector<CDBID> objects;
			if ( NDb::GetObjectsList(&objects, (*itClass)->szTypeName) && !objects.empty() )
			{
				Log( "(%d of %d): Loading objects of type \"%s\" (total %d objects)", nCounter, classes.size(), (*itClass)->szTypeName.c_str(), objects.size() );
				for ( std::vector<CDBID>::const_iterator itDBID = objects.begin(); itDBID != objects.end(); ++itDBID )
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

int PORT_CDECL main( int argc, char *argv[] )
{
	const std::string szCWD = NFile::GetNormalizedCurrDir();
	//
	std::string szDataPath = szCWD;

	po::options_description options( "Options" );
	options.add_options()
		( "show-version",   "show current product version" )
		( "update-struct",  "update all database objects to new structure in accordance with types" )
		( "make-bin",       "convert .xdb files to packed binary" )
		( "data-path",      po::value<std::string>( &szDataPath ),
		                    "set data path to operate (default: current dir)" )
		( "help",           "show this message" );
	//
	NGlobal::SetVar( "code_version_number", REVISION_NUMBER_STR );
	NGlobal::SetVar( "code_build_date_time", BUILD_DATE_TIME_STR );
	//
	printf( "XML Database structure utility\n(C) Nival Interactive, 2005\n\n" );

	po::variables_map args;
	try
	{
		// allow_long_disguise so the single dash spellings this tool has always
		// taken, -update-struct and the rest, keep working. The double dash forms
		// parse too, which is what anyone would try first.
		po::store( po::command_line_parser( argc, argv )
		               .options( options )
		               .style( po::command_line_style::default_style
		                       | po::command_line_style::allow_long_disguise )
		               .run(),
		           args );
		po::notify( args );
	}
	catch ( const po::error &err )
	{
		// An unknown option or a missing value. Saying which, then the usage,
		// rather than the bare usage the old parser printed.
		printf( "ERROR: %s\n\n", err.what() );
		std::cout << options << std::endl;
		return 0xDEAD;
	}
	//
	const EDBStructMode eMode = args.count( "update-struct" ) ? MODE_UPDATE_STRUCT
	                          : args.count( "make-bin" )      ? MODE_MAKE_BIN
	                          : args.count( "show-version" )  ? MODE_SHOW_VERSION
	                          :                                 MODE_UNKNOWN;
	if ( eMode == MODE_UNKNOWN || args.count( "help" ) )
	{
		printf( "Usage: dbstruct [options]\n\n" );
		std::cout << options << std::endl;
		// Asking for help is not a failure; having picked no mode is.
		return args.count( "help" ) ? 0 : 0xDEAD;
	}
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


