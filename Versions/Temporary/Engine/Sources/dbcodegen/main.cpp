#include "stdafx.h"
#include "codegen.h"
#include "Config.h"
#include "Errors.h"
#include "SolutionAnalyzer.h"
#include "Misc/StrProc.h"
#include "System/FileUtils.h"
#include "System/FilePath.h"

#include "port/cdecl.h"

#include <fmt/format.h>

#include <boost/program_options.hpp>

#include <algorithm>
#include <iostream>

namespace po = boost::program_options;

//
using namespace NDb::NCodeGenTool;

namespace
{

void ThrowOutEqual( std::vector<std::string> *pArray )
{
	if ( pArray->empty() )
		return;

	std::sort( pArray->begin(), pArray->end() );

	int k = 0;
	for ( int i = 1; i < pArray->size(); ++i )
	{
		if ( (*pArray)[k] != (*pArray)[i] )
			(*pArray)[++k] = (*pArray)[i];
	}
	pArray->resize( k + 1 );
}

bool ReadConfigFile( SConfig *pConfig, const std::string &szConfigFile )
{
	CFileStream stream( szConfigFile, CFileStream::WIN_READ_ONLY );
	if ( stream.IsOk() )
	{
		if ( CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_READ) )
		{
			pSaver->AddTypedSuper( pConfig );
			return true;
		}
	}
	return false;
}
}


int PORT_CDECL main( int argc, char *argv[] )
{
	const std::string szCurrDir = NFile::GetNormalizedCurrDir();
	//
	std::string szConfigFileName = "dbconfig.xml";
	std::string szTypesPath = szCurrDir;
	std::string szSourcesPath = szCurrDir;

	// Boost::program_options replaced System/CmdLine.h, which the port deleted as
	// unused by the game. The options and their meaning are the original ones.
	po::options_description options( "Options" );
	options.add_options()
		( "show-version",   "show product version" )
		( "all",            "generate types.xml and sources" )
		( "nocopy",         "only generate new source files (and don't copy to version)" )
		( "types",          "only generate new types.xml" )
		( "config-file",    po::value<std::string>( &szConfigFileName ),
		                    fmt::format( "set name for config file (default: \"{}\")", szConfigFileName ).c_str() )
		( "types-path",     po::value<std::string>( &szTypesPath ),
		                    "set path to store types.xml (default: current dir)" )
		( "sources-path",   po::value<std::string>( &szSourcesPath ),
		                    "set path to get .cll sources from (default: current dir)" )
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
		// taken, -all, -nocopy and -types, keep working alongside --config-file
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
	const std::string szConfigFilePath = szBasePath + szConfigFileName;
	//
	SConfig config;
	{
		if ( ReadConfigFile(&config, szConfigFilePath) == false )
		{
			printf( "ERROR: Can't read config file \"%s\"\n", szConfigFilePath.c_str() );
			return 0xDEAD;
		}
	}
	// start work
	printf( "XML Database code generation utility\n" );
	printf( "Using config file \"%s\"\n", szConfigFilePath.c_str() );
	// pre-compile descriptors
	printf( "Pre-compile type descriptors\n" );

	try
	{
		std::vector<std::string> filesToCompile;
		for ( int i = 0; i < config.slns.size(); ++i )
			NSlnAnalyzer::GetTypesDescriptorsOfSln( config.slns[i], szBasePath, &filesToCompile );
		ThrowOutEqual( &filesToCompile );

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
	}
	catch ( CCodeGenException &exc )
	{
		printf( "ERROR: %s", exc.GetDesc().c_str() );
		return 0xDEAD;
	}

	//
	printf( "Done\n" );
	//
	return 0;
}
