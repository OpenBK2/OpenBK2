#include "stdafx.h"

#include "CodeGenFile.h"
#include "CodeStructure.h"
#include "Misc/StrProc.h"
#include "Parser/FileNode.h"

#include "libdb_export.h"

namespace NCodeGen
{

CCodeStructure::CCodeStructure( NLang::CFileNode *pRootFile, const CNodes2TypeDefs &nodes2TypeDefs, const std::string &szRootDir, NDb::NTypeDef::CTerminalTypesDescriptor *pTermTypesDesc )
{
	for ( NLang::CFileNode::TIncludesIter iter = pRootFile->BeginIncludes(); iter != pRootFile->EndIncludes(); ++iter )
		files.push_back( new CFile( iter->second, nodes2TypeDefs, szRootDir, pTermTypesDesc ) );
}

void CCodeStructure::GenerateCode( const std::string &szRootDir )
{
	for ( std::list< CObj<CFile> >::iterator iter = files.begin(); iter != files.end(); ++iter )
	{
		CFile *pFile = *iter;
		pFile->GenerateCode( szRootDir );
	}
}



CXmlResource* GenerateCodeStructure( NLang::CFileNode *pRootFile, const CNodes2TypeDefs &nodes2TypeDefs, const std::string &szRawRootDir, NDb::NTypeDef::CTerminalTypesDescriptor *pTermTypesDesc )
{
	std::string szRootDir;
	NStr::ToLower( &szRootDir, szRawRootDir );
	NStr::ReplaceAllChars( &szRootDir, '\\', '/' );
	if ( szRootDir[szRootDir.size()-1] != '/' )
		szRootDir += "/";
	return new CCodeStructure( pRootFile, nodes2TypeDefs, szRootDir, pTermTypesDesc );
}

void GenerateCode( CCodeStructure *pCodeStructure, const std::string &szRawRootDir )
{
	std::string szRootDir;
	NStr::ToLower( &szRootDir, szRawRootDir );
	NStr::ReplaceAllChars( &szRootDir, '\\', '/' );
	if ( szRootDir[szRootDir.size()-1] != '/' )
		szRootDir += "/";

	pCodeStructure->GenerateCode( szRootDir );
}

}

using namespace NCodeGen;
REGISTER_SAVELOAD_CLASS( LIBDB, 0x301B6D00, CCodeStructure );

