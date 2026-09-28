#include "stdafx.h"

#include "codegen.h"
#include "libdb/CodeStructure.h"
#include "libdb/CompileCLike.h"
#include "libdb/GenerateCode.h"
#include "libdb/CodeGenFile.h"
#include "libdb/GenerateCodeStructure.h"
#include "libdb/TerminalTypesDesc.h"
#include "Parser/FileNode.h"
#include "Parser/LangNode.h"
#include "System/FileUtils.h"
#include "Misc/StrProc.h"

#include <cstdint>

#include <fmt/format.h>

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

bool PrecompileTypes( SCompiledTypesInfo *pRes, bool bGenerateCodeStructure, const std::vector<std::string> &files, const std::string &szDescriptorsPath )
{
	bool bParse = NLang::Parse( files, szDescriptorsPath + "base.cll" );
	NI_VERIFY( bParse != false, fmt::format("Can't parse type definitions from \"{}\"", szDescriptorsPath), return false );

	if ( NLang::GetRootFile() && NLang::GetRootFile()->GetNamespace() )
	{
		CPtr<NDb::NTypeDef::CTerminalTypesDescriptor> pTermTypesDesc = new NDb::NTypeDef::CTerminalTypesDescriptor();
		const bool bCompiled = NCompileCLike::Compile( &pRes->types, pTermTypesDesc, &pRes->nodes2TypeDefs, NLang::GetRootFile()->GetNamespace() );
		NI_VERIFY( bCompiled != false, "can't compile types", return false );
		STypesSort typesSort;
		std::sort( pRes->types.begin(), pRes->types.end(), typesSort );
		if ( bGenerateCodeStructure )
			pRes->pCodeStructure = NCodeGen::GenerateCodeStructure( NLang::GetRootFile(), pRes->nodes2TypeDefs, szDescriptorsPath, pTermTypesDesc );
		return true;
	}
	return false;
}

bool ReadFile( std::vector<uint8_t> &data, const std::string &szFileName );

// Saved to memory first and written only when the content differs from the
// file already there, as CopySourceCode does for the sources. A run that
// changes nothing then leaves types.xml's timestamp alone.
bool GenerateTypes( const std::string &szTypesFilePath, SCompiledTypesInfo *pTypesInfo )
{
	CMemoryStream memStream;
	{
		// the saver writes its document into the stream when it is released
		CPtr<IXmlSaver> pSaver = CreateXmlSaver( &memStream, SAVER_MODE_WRITE );
		if ( !pSaver )
		{
			NI_ASSERT( false, fmt::format("Can't save compiled types to \"{}\"", szTypesFilePath) );
			return false;
		}
		pSaver->Add( "Types", &pTypesInfo->types );
	}
	const unsigned char *pNew = memStream.GetBuffer();
	const std::vector<uint8_t> newFile( pNew, pNew + memStream.GetSize() );
	std::vector<uint8_t> oldFile;
	if ( ReadFile( oldFile, szTypesFilePath ) && oldFile == newFile )
	{
		return true;
	}
	printf( "Writing changed file: %s\n", szTypesFilePath.c_str() );
	CFileStream stream( szTypesFilePath, CFileStream::WIN_CREATE );
	if ( !stream.IsOk() )
	{
		NI_ASSERT( false, fmt::format("Can't save compiled types to \"{}\"", szTypesFilePath) );
		return false;
	}
	stream.Write( newFile.data(), static_cast<int>( newFile.size() ) );
	return true;
}

bool GenerateCode( std::list<std::string> *pFileTitles, const std::string &szSourceCodePath, SCompiledTypesInfo *pTypesInfo )
{
	if ( NCodeGen::CCodeStructure *pCodeStructure = dynamic_cast_ptr<NCodeGen::CCodeStructure *>( pTypesInfo->pCodeStructure ) )
	{
		NCodeGen::GenerateCode( pCodeStructure, szSourceCodePath );
		if ( pFileTitles )
		{
			const std::list< CObj<NCodeGen::CFile> > &files = pCodeStructure->GetFiles();
			for ( std::list< CObj<NCodeGen::CFile> >::const_iterator it = files.begin(); it != files.end(); ++it )
			{
				const std::string &szFileTitle = (*it)->GetName();
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

bool ReadFile( std::vector<uint8_t> &data, const std::string &szFileName )
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

bool ProcessFile( const std::string &szSrcFileName, const std::string &szDstFileName )
{
	// check for changed
	{
		std::vector<uint8_t> newFile;
		if ( ReadFile(newFile, szSrcFileName) == false )
			return false;
		std::vector<uint8_t> oldFile;
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

bool CopySourceCode( const std::list<std::string> &filetitles, const std::string &szSrcPath, const std::string &szDstPath )
{
	for ( std::list<std::string>::const_iterator it = filetitles.begin(); it != filetitles.end(); ++it )
	{
		if ( (*it) == "game" || (*it) == "base" )
			continue;
		//
		// Once a generated file is in the tree, or found unchanged there, its temp
		// copy goes. RemoveFile is std::filesystem::remove behind System's
		// wrapper, in place of Win32's DeleteFile; as before, a failure to remove
		// the temp copy is not an error.
		std::string szFileName = (*it) + ".h";
		if ( ProcessFile( szSrcPath + szFileName, szDstPath + szFileName ) != false )
		{
			NFile::RemoveFile( szSrcPath + szFileName );
		}
		//
		szFileName = (*it) + ".cpp";
		if ( ProcessFile( szSrcPath + szFileName, szDstPath + szFileName ) != false )
		{
			NFile::RemoveFile( szSrcPath + szFileName );
		}
	}
	//
	return true;
}

}
}

