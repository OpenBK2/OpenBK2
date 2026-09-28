#include "stdafx.h"
#include "codegen.h"
#include "Misc/StrProc.h"
#include "System/FileUtils.h"
#include "System/FilePath.h"

#include "port/cdecl.h"

#include <boost/program_options.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>

namespace po = boost::program_options;

//
using namespace NDb::NCodeGenTool;

namespace
{

std::string LowerCaseKey( std::string szPath )
{
	NStr::ToLowerASCII( &szPath );
	return szPath;
}

// Sorts the files and drops repeats, both ignoring case. The paths keep their
// spelling for opening, but they used to be lowercased here and sorted as such,
// and the order they reach the parser in is kept as it was.
void ThrowOutEqual( std::vector<std::string> *pArray )
{
	if ( pArray->empty() )
	{
		return;
	}

	std::sort( pArray->begin(), pArray->end(),
		[]( const std::string &a, const std::string &b ) { return LowerCaseKey( a ) < LowerCaseKey( b ); } );

	int k = 0;
	for ( int i = 1; i < pArray->size(); ++i )
	{
		if ( LowerCaseKey( (*pArray)[k] ) != LowerCaseKey( (*pArray)[i] ) )
		{
			(*pArray)[++k] = (*pArray)[i];
		}
	}
	pArray->resize( k + 1 );
}

// Reads the .cll files to compile, one path per line, as CMake writes them to
// type-descriptions.txt (see cmake/dbcodegen.cmake). The build owns this list:
// it replaced walking the projects of Game.sln and B2_MapEditor.sln, which had
// drifted from what CMake builds.
//
// Each path is normalized but keeps its case: the parser opens it as spelled,
// which is what lets this run where the filesystem minds case, and lowercases
// only the name it keys the file by.
bool ReadFileList( std::vector<std::string> *pFiles, const std::string &szFileList )
{
	std::ifstream stream( szFileList );
	if ( !stream )
	{
		return false;
	}
	std::string szLine;
	while ( std::getline( stream, szLine ) )
	{
		while ( !szLine.empty() && (szLine.back() == '\r' || szLine.back() == ' ') )
		{
			szLine.pop_back();
		}
		if ( szLine.empty() )
		{
			continue;
		}
		NFile::NormalizePath( &szLine );
		pFiles->push_back( szLine );
	}
	return true;
}
}


int PORT_CDECL main( int argc, char *argv[] )
{
	const std::string szCurrDir = NFile::GetNormalizedCurrDir();
	//
	std::string szFileList;
	std::string szTypesPath = szCurrDir;
	std::string szSourcesPath = szCurrDir;

	// Boost::program_options replaced System/CmdLine.h, which the port deleted as
	// unused by the game. The options are the original ones, except that
	// --file-list replaced --config-file.
	po::options_description options( "Options" );
	options.add_options()
		( "show-version",   "show product version" )
		( "all",            "generate types.xml and sources" )
		( "nocopy",         "only generate new source files (and don't copy to version)" )
		( "types",          "only generate new types.xml" )
		( "file-list",      po::value<std::string>( &szFileList ),
		                    "file listing the .cll files to compile, one per line; the build writes "
		                    "type-descriptions.txt beside dbcodegen" )
		( "types-path",     po::value<std::string>( &szTypesPath ),
		                    "set path to store types.xml (default: current dir)" )
		( "sources-path",   po::value<std::string>( &szSourcesPath ),
		                    "set the root of the sources, where base.cll and game.cll are and "
		                    "generated files go (default: current dir)" )
		( "help",           "show this message" );
	//
	NGlobal::SetVar( "code_version_number", REVISION_NUMBER_STR );
	NGlobal::SetVar( "code_build_date_time", BUILD_DATE_TIME_STR );
	//
	printf( "XML Database code generation utility\nWritten by [REDACTED]\n(C) [REDACTED], 2004\n\n" );

	po::variables_map args;
	try
	{
		// allow_long_disguise so the single dash spellings this tool has always
		// taken, -all, -nocopy and -types, keep working alongside --file-list
		// and the other double dash ones.
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
		printf( "ERROR: %s\n\n", err.what() );
		std::cout << options << std::endl;
		return 0xDEAD;
	}
	// The modes are exclusive; the old parser refused a second one as
	// ambiguous rather than picking either, so this does too.
	const int nModes = int( args.count( "all" ) + args.count( "nocopy" ) + args.count( "types" ) + args.count( "show-version" ) );
	if ( nModes > 1 )
	{
		printf( "ERROR: -all, -nocopy, -types and -show-version are exclusive\n\n" );
		std::cout << options << std::endl;
		return 0xDEAD;
	}
	const ECodeGenOpts eCodeGenOpts = args.count( "all" )          ? CODE_GEN_NORMAL
	                                : args.count( "nocopy" )       ? CODE_GEN_NOCOPY
	                                : args.count( "types" )        ? CODE_GEN_TYPES
	                                : args.count( "show-version" ) ? CODE_GEN_SHOW_VERSION
	                                :                                CODE_GEN_UNKNOWN;
	if ( eCodeGenOpts == CODE_GEN_UNKNOWN || args.count( "help" ) )
	{
		printf( "Usage: dbcodegen [options]\n\n" );
		std::cout << options << std::endl;
		// Asking for help is not a failure; having picked no mode is.
		return args.count( "help" ) ? 0 : 0xDEAD;
	}
	else if ( eCodeGenOpts == CODE_GEN_SHOW_VERSION )
	{
		printf( "Version: %s\n", REVISION_NUMBER_STR );
		printf( "Build date/time: %s\n", BUILD_DATE_TIME_STR );
		return 0;
	}
	//
	NFile::AppendSlash( &szTypesPath );
	NFile::AppendSlash( &szSourcesPath );
	//
	const std::string szBasePath = szSourcesPath;
	//
	if ( szFileList.empty() )
	{
		printf( "ERROR: --file-list is required\n" );
		return 0xDEAD;
	}
	std::vector<std::string> filesToCompile;
	if ( ReadFileList( &filesToCompile, szFileList ) == false )
	{
		printf( "ERROR: Can't read file list \"%s\"\n", szFileList.c_str() );
		return 0xDEAD;
	}
	// base.cll and game.cll sit at the root of the sources rather than in a
	// module, and every run needs them, so they are added here as the solution
	// walk added them rather than listed by the build.
	filesToCompile.push_back( szBasePath + "base.cll" );
	filesToCompile.push_back( szBasePath + "game.cll" );
	ThrowOutEqual( &filesToCompile );
	// start work
	printf( "XML Database code generation utility\n" );
	printf( "Using file list \"%s\"\n", szFileList.c_str() );
	// pre-compile descriptors
	printf( "Pre-compile type descriptors\n" );

	SCompiledTypesInfo compiledTypesInfo;
	if ( PrecompileTypes( &compiledTypesInfo, eCodeGenOpts != CODE_GEN_TYPES, filesToCompile, szBasePath ) == false )
	{
		printf( "ERROR: can't precompile types!\n" );
		return 0xDEAD;
	}
	// generate types
	if ( eCodeGenOpts != CODE_GEN_NOCOPY )
	{
		const std::string szTypeCollectionFile = szTypesPath + "types.xml";
		printf( "Generate types file (%s)\n", szTypeCollectionFile.c_str() );
		if ( GenerateTypes( szTypeCollectionFile, &compiledTypesInfo ) == false )
		{
			printf( "ERROR: can't generate types!\n" );
			return 0xDEAD;
		}
	}

	if ( eCodeGenOpts != CODE_GEN_TYPES )
	{
		std::string szSourceCodePath = NFile::GetTempPath() + "dbcode/";
		NFile::NormalizePath( &szSourceCodePath );
		const std::string szProjectSourcePath = szBasePath;

		printf( "Generate source files (in %s)\n", szSourceCodePath.c_str() );
		std::list<std::string> filenames;
		bool bRes = GenerateCode( &filenames, szSourceCodePath, &compiledTypesInfo );
		NI_VERIFY( bRes != false, "Failed to generate source code files", return 0xDEAD );
		if ( eCodeGenOpts != CODE_GEN_NOCOPY )
		{
			printf( "(Copy to %s)\n", szProjectSourcePath.c_str() );
			bRes = CopySourceCode( filenames, szSourceCodePath, szProjectSourcePath );
			NI_VERIFY( bRes != false, "Failed to copy source code files to project", return 0xDEAD );
		}
	}

	//
	printf( "Done\n" );
	//
	return 0;
}
