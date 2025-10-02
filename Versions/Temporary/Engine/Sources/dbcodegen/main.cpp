#include "StdAfx.h"
#include "revision.h"
#include "codegen.h"
#include "config.h"
#include "Errors.h"
#include "SolutionAnalyzer.h"
#include "../System/CmdLine.h"
#include "../Misc/StrProc.h"
#include "../System/FileUtils.h"
#include "../System/FilePath.h"
//
using namespace NDb::NCodeGenTool;

namespace
{

void ThrowOutEqual( vector<string> *pArray )
{
	if ( pArray->empty() )
		return;

	sort( pArray->begin(), pArray->end() );

	int k = 0;
	for ( int i = 1; i < pArray->size(); ++i )
	{
		if ( (*pArray)[k] != (*pArray)[i] )
			(*pArray)[++k] = (*pArray)[i];
	}
	pArray->resize( k + 1 );
}

bool ReadConfigFile( SConfig *pConfig, const string &szConfigFile )
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


int __cdecl main( int argc, char *argv[] )
{
	const string szCurrDir = NFile::GetNormalizedCurrDir();
	//
	ECodeGenOpts eCodeGenOpts = CODE_GEN_UNKNOWN;
	string szConfigFileName = "dbconfig.xml";
	string szTypesPath = szCurrDir;
	string szSourcesPath = szCurrDir;

	NCmdLine::CCmdLine cmdLine( "XML Database code generation utility\nWritten by [REDACTED]\n(C) [REDACTED], 2004\n" );
	cmdLine.AddOption( "-show-version", &eCodeGenOpts, CODE_GEN_SHOW_VERSION, "show product version" );
	cmdLine.AddOption( "-all", &eCodeGenOpts, CODE_GEN_NORMAL, "generate types.xml and sources" );
	cmdLine.AddOption( "-nocopy", &eCodeGenOpts, CODE_GEN_NOCOPY, "only generate new source files (and don't copy to version)" );
	cmdLine.AddOption( "-types", &eCodeGenOpts, CODE_GEN_TYPES, "only generate new types.xml" );
	cmdLine.AddOption( "--config-file", &szConfigFileName, StrFmt("set name for config file (default: \"%s\")", szConfigFileName.c_str()) );
	cmdLine.AddOption( "--types-path", &szTypesPath, "set path to store types.xml (default: current dir)" );
	cmdLine.AddOption( "--sources-path", &szSourcesPath, "set path to get .cll sources from (default: current dir)" );
	//
	NGlobal::SetVar( "code_version_number", REVISION_NUMBER_STR );
	NGlobal::SetVar( "code_build_date_time", BUILD_DATE_TIME_STR );
	//
	cmdLine.PrintHeader();
	if ( cmdLine.Process( argc, argv ) != NCmdLine::CCmdLine::PROC_RESULT_OK )
		return 0xDEAD;
	//
	if ( eCodeGenOpts == CODE_GEN_UNKNOWN )
		return cmdLine.PrintUsage( "Usage: dbcodegen.exe [options] [other_config_name]" );
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
	const string szBasePath = szSourcesPath;
	const string szConfigFilePath = szBasePath + szConfigFileName;
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
		vector<string> filesToCompile;
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
			const string szTypeCollectionFile = szTypesPath + "types.xml";
			printf( "Generate types file (%s)\n", szTypeCollectionFile.c_str() );
			if ( GenerateTypes( szTypeCollectionFile, &compiledTypesInfo ) == false )
			{
				printf( "ERROR: can't generate types!\n" );
				return 0xDEAD;
			}
		}

		if ( eCodeGenOpts != CODE_GEN_TYPES )
		{
			string szSourceCodePath = NFile::GetTempPath() + "dbcode/";
			NFile::NormalizePath( &szSourceCodePath );
			const string szProjectSourcePath = szBasePath;

			printf( "Generate source files (in %s)\n", szSourceCodePath.c_str() );
			list<string> filenames;
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

