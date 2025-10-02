#include "StdAfx.h"

#include "codegen.h"
#include "SolutionAnalyzer.h"
#include "../libdb/CodeStructure.h"
#include "../libdb/CompileCLike.h"
#include "../libdb/GenerateCode.h"
#include "../libdb/CodeGenFile.h"
#include "../libdb/GenerateCodeStructure.h"
#include "../libdb/TerminalTypesDesc.h"
#include "../Parser/FileNode.h"
#include "../Parser/LangNode.h"
#include "../System/FileUtils.h"
#include "../Misc/StrProc.h"

namespace NDb
{
namespace NCodeGenTool
{

struct STypesSort
{
	bool operator()( NDb::NTypeDef::STypeDef *pType1, NDb::NTypeDef::STypeDef *pType2 ) const 
	{
		return pType1->GetTypeName() < pType2->GetTypeName();
	}
};

bool PrecompileTypes( SCompiledTypesInfo *pRes, bool bGenerateCodeStructure, const vector<string> &files, const string &szDescriptorsPath )
{
	bool bParse = NLang::Parse( files, szDescriptorsPath + "base.cll" );
	NI_VERIFY( bParse != false, StrFmt("Can't parse type definitions from \"%s\"", szDescriptorsPath.c_str()), return false );

	if ( NLang::GetRootFile() && NLang::GetRootFile()->GetNamespace() )
	{
		CPtr<NDb::NTypeDef::CTerminalTypesDescriptor> pTermTypesDesc = new NDb::NTypeDef::CTerminalTypesDescriptor();
		const bool bCompiled = NCompileCLike::Compile( &pRes->types, pTermTypesDesc, &pRes->nodes2TypeDefs, NLang::GetRootFile()->GetNamespace() );
		NI_VERIFY( bCompiled != false, "can't compile types", return false );
		STypesSort typesSort;
		sort( pRes->types.begin(), pRes->types.end(), typesSort );
		if ( bGenerateCodeStructure )
			pRes->pCodeStructure = NCodeGen::GenerateCodeStructure( NLang::GetRootFile(), pRes->nodes2TypeDefs, szDescriptorsPath, pTermTypesDesc );
		return true;
	}
	return false;
}

bool GenerateTypes( const string &szTypesFilePath, SCompiledTypesInfo *pTypesInfo )
{
	CFileStream stream( szTypesFilePath, CFileStream::WIN_CREATE );
	if ( stream.IsOk() )
	{
		if ( CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_WRITE ) )
		{
			pSaver->Add( "Types", &pTypesInfo->types );
			return true;
		}
	}
	NI_ASSERT( false, StrFmt("Can't save compiled types to \"%s\"", szTypesFilePath.c_str()) );
	return false;
}

bool GenerateCode( list<string> *pFileTitles, const string &szSourceCodePath, SCompiledTypesInfo *pTypesInfo )
{
	if ( NCodeGen::CCodeStructure *pCodeStructure = dynamic_cast_ptr<NCodeGen::CCodeStructure *>( pTypesInfo->pCodeStructure ) )
	{
		NCodeGen::GenerateCode( pCodeStructure, szSourceCodePath );
		if ( pFileTitles )
		{
			const list< CObj<NCodeGen::CFile> > &files = pCodeStructure->GetFiles();
			for ( list< CObj<NCodeGen::CFile> >::const_iterator it = files.begin(); it != files.end(); ++it )
			{
				const string &szFileTitle = (*it)->GetName();
				pFileTitles->push_back( szFileTitle );
			}
		}
		return true;
	}
	else
	{
		NI_ASSERT( false, "Code structure was not generated during types compilation!" );
		return false;
	}
}

bool ReadFile( vector<BYTE> &data, const string &szFileName )
{
	CFileStream stream( szFileName, CFileStream::WIN_READ_ONLY );
	if ( !stream.IsOk() ) 
		return false;
	const int nSize = stream.GetSize();
	if ( nSize == 0 ) 
		return false;
	data.resize( nSize );
	stream.Read( &(data[0]), nSize );
	return true;
}

bool ProcessFile( const string &szSrcFileName, const string &szDstFileName )
{
	// check for changed
	{
		vector<BYTE> newFile;
		if ( ReadFile(newFile, szSrcFileName) == false )
			return false;
		vector<BYTE> oldFile;
		if ( ReadFile(oldFile, szDstFileName) != false )
		{
			if ( newFile == oldFile )
				return true;
		}
	}
	//
	printf( "Copying changed file: %s\n", szDstFileName.c_str() );
	DebugTrace( "Copying changed file: %s", szDstFileName.c_str() );
	//
	return NFile::CopyFile( szSrcFileName, szDstFileName );
}

bool CopySourceCode( const list<string> &filetitles, const string &szSrcPath, const string &szDstPath )
{
	for ( list<string>::const_iterator it = filetitles.begin(); it != filetitles.end(); ++it )
	{
		if ( (*it) == "game" || (*it) == "base" )
			continue;
		//
		string szFileName = (*it) + ".h";
		if ( ProcessFile( szSrcPath + szFileName, szDstPath + szFileName ) != false )
			::DeleteFile( (szSrcPath + szFileName).c_str() );
		//
		szFileName = (*it) + ".cpp";
		if ( ProcessFile( szSrcPath + szFileName, szDstPath + szFileName ) != false )
			::DeleteFile( (szSrcPath + szFileName).c_str() );
	}
	//
	return true;
}

}
}
