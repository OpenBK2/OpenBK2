#include "stdafx.h"

#include "ErrorsAndMessages.h"
#include "FileNode.h"
#include "FileRead.h"
#include "LangNodesDefinitions.h"
#include "ParseOperations.h"
#include "System/FilePath.h"
#include "Misc/StrProc.h"

#include <fmt/format.h>

int yyparse( void );
extern int nyyLineNumber;
extern bool bInTestMode;

namespace NLang
{

CPtr<CFileNode> pRootFile;

static struct SCreateRootFile
{
	SCreateRootFile() { pRootFile = new CFileNode( "", true ); }
} createRootFile;

CFileNode* GetRootFile()
{
	return pRootFile;
}

//*******************************************************************
//*                     CFileNode                                   *
//*******************************************************************

void CFileNode::SetFullName( const std::string &_szFullName )
{
	NFile::NormalizePath( &szFullFileName, _szFullName );
	NStr::ToLowerASCII( &szFullFileName );
}

void CFileNode::AddInclude( std::string szFileName )
{
	NStr::ToLowerASCII( &szFileName );
//	NFile::NormalizePath( &szFileName );
	if ( includes.find( szFileName ) == includes.end() )
	{
		includes[szFileName] = new CFileNode( szFileName, false );
		if ( !IsRootFile() )
			includes[szFileName]->SetIncludedInOtherFile();
	}
}

void CFileNode::AddInclude( CFileNode *pNode )
{
	VERIFY_NOLINE( pNode != 0, "trying to add empty include node", return );

	includes[pNode->GetName()] = pNode;
	if ( !IsRootFile() )
		pNode->SetIncludedInOtherFile();
}

void CFileNode::AddHExternal( const std::string &szIncludeName )
{
	std::string szResult( szIncludeName );
	NStr::ToLowerASCII( &szResult );
	NStr::ReplaceAllChars( &szResult, '\\', '/' );
	hExternalIncludes.push_back( szResult );
}

void CFileNode::AddCPPExternal( const std::string &szIncludeName )
{
	std::string szResult( szIncludeName );
	NStr::ToLowerASCII( &szResult );
	NStr::ReplaceAllChars( &szResult, '\\', '/' );	
	cppExternalIncludes.push_back( szResult );
}

CFileNode* CFileNode::GetInclude( const std::string &szFileName )
{
	return includes.find( szFileName ) == includes.end() ? 0 : includes[szFileName];
}

CNamespace* CFileNode::GetNamespace() const
{
	VERIFY( !namespaces.empty(), "No namespaces found", return 0 );
	return namespaces.front();
}

void CFileNode::OpenNewNamespace( CNodesList<CComplexTypeNode> *pVisibleTypes )
{
	namespaces.push_front( new CNamespace( pVisibleTypes, szFullFileName, nyyLineNumber ) );
}

void CFileNode::CloseNamespace( bool bResolveForwards )
{
	VERIFY( !namespaces.empty(), "can't close namespace", return );

	if ( bResolveForwards )
	{
		CNamespace *pNM = namespaces.front();
		pNM->ResolveForwards();
	}

	namespaces.pop_front();
}

void CFileNode::Parse()
{
	VERIFY_NOLINE( eParseState != EPS_INPARSING, fmt::format( "cyclic include of file {}", szFullFileName ), return );
	VERIFY_NOLINE( bFileExist = true, fmt::format( "file {} is included by doesn't exist", szFullFileName ), return );
	if ( eParseState == EPS_PARSED )
		return;

	eParseState = EPS_INPARSING;
	namespaces.push_front( new CNamespace() );

	const std::string szBaseFileName = NLang::GetBaseFileName();

	if ( GetName() != szBaseFileName && includes.find( szBaseFileName ) == includes.end() )
	{
		CFileNode *pBase = GetRootFile()->GetInclude( szBaseFileName );
		if ( pBase )
			AddInclude( pBase );
	}

	for ( std::unordered_map< std::string, CObj<CFileNode> >::iterator iter = includes.begin(); iter != includes.end(); ++iter )
	{
		CFileNode *pFileNode = iter->second;
		bool bParse = !IsRootFile() || !pFileNode->IsIncludedInOtherFile();

		if ( bParse )
		{
			pFileNode->Parse();
			GetNamespace()->MergeFiles( pFileNode->GetNamespace(), pFileNode->GetName() );
		}
	}

	if ( IsRootFile() )
	{
		for ( std::unordered_map< std::string, CObj<CFileNode> >::iterator iter = includes.begin(); iter != includes.end(); ++iter )
		{
			CFileNode *pNode = iter->second;
			VERIFY_NOLINE( pNode->IsParsed(), fmt::format( "cyclic include of file {}", iter->first ), return );
		}
	}
	else
	{
		OpenNewNamespace( 0 );
		VERIFY_NOLINE( NLang::OpenFile( GetName() ), fmt::format( "file {} not found", GetName() ), { eParseState = EPS_PARSED; return; } );
		
		nyyLineNumber = 1;
		yyparse();

		VERIFY_NOLINE( namespaces.size() == 2, "not all namespaces closed", { eParseState = EPS_PARSED; return; } );
		namespaces.back()->MergeFiles( namespaces.front(), GetName() );
		namespaces.back()->SetDefsListFrom( namespaces.front() );
		CloseNamespace( false );

		eParseState = EPS_PARSED;
	}
}

//*******************************************************************
//*													Parsing                                 *
//*******************************************************************

CFileNode* GetCurFileNode()
{
	const std::string &szCurFile = GetParsingFileName();
	return GetRootFile()->GetInclude( szCurFile );
}

void AddInclude( const std::string &szFileName )
{
	CFileNode *pNode = GetCurFileNode();
	std::string szCurFileName = pNode->GetName();
	szCurFileName = NFile::GetFilePath( szCurFileName );

	std::string szPartialName( NFile::GetFilePath( szFileName ) );
	NStr::ReplaceAllChars( &szCurFileName, '\\', '/' );
	NStr::ReplaceAllChars( &szPartialName, '\\', '/' );

	if ( szCurFileName[szCurFileName.size()-1] == '/' )
		szCurFileName.pop_back();
	if ( !szPartialName.empty() && szPartialName[szPartialName.size()-1] == '/' )
		szPartialName.pop_back();

	std::vector<std::string> szFullPath;
	NStr::SplitString( szCurFileName, &szFullPath, '/' );

	std::vector<std::string> szPartialPath;
	if ( !szPartialName.empty() )
		NStr::SplitString( szPartialName, &szPartialPath, '/' );
	szPartialPath.push_back( NFile::GetFileName( szFileName ) );
	VERIFY( !szPartialPath.back().empty(), fmt::format( "empty include (\"{}\")", szFileName ), return );

	int nCurFull = 0;
	for ( int i = 0; i < szPartialPath.size(); ++i )
	{
		if ( szPartialPath[i] == ".."  )
		{
			VERIFY( !szFullPath.empty(), fmt::format( "wrong include {}", szFileName ), return );
			szFullPath.pop_back();
		}
		else
			szFullPath.push_back( szPartialPath[i] );
	}

	std::string szResult = "";
	for ( int i = 0; i < szFullPath.size(); ++i )
		szResult += szFullPath[i] + '/';
	szResult.pop_back();

	NStr::ToLowerASCII( &szResult );

	GetRootFile()->AddInclude( szResult );
	pNode->AddInclude( GetRootFile()->GetInclude( szResult ) );
}

void AddHExternal( const std::string &szIncludeName )
{
	GetCurFileNode()->AddHExternal( szIncludeName );
}

void AddCPPExternal( const std::string &szIncludeName )
{
	GetCurFileNode()->AddCPPExternal( szIncludeName );
}


CAttributeDefNode* CFileNode::FindAttrDef( const std::string &szAttrDefName )
{
	for ( std::list< CObj<CNamespace> >::iterator iter = namespaces.begin(); iter != namespaces.end(); ++iter )
	{
		CNamespace *pNM = *iter;
		if ( CAttributeDefNode *pAttrDef = pNM->FindInsideAttrDef( szAttrDefName ) )
			return pAttrDef;
	}

	return 0;
}

void CFileNode::AddAttrDef( CAttributeDefNode *pAttrDefNode )
{
	namespaces.front()->AddInsideAttrDef( pAttrDefNode );
}

void CFileNode::AddDef( CLangNode *pNode )
{
	namespaces.front()->AddInsideDef( pNode );
}

CLangNode* CFileNode::FindDef( const std::string &szTypeName, bool bOnlyTopNamespace )
{
	if ( bOnlyTopNamespace )
		return namespaces.front()->FindInsideDef( szTypeName );
	else
	{
		for ( std::list< CObj<CNamespace> >::iterator iter = namespaces.begin(); iter != namespaces.end(); ++iter )
		{
			CNamespace *pNM = *iter;
			CLangNode *pDef = pNM->FindInsideDef( szTypeName );
			if ( pDef )
				return pDef;
		}
	}

	return 0;
}

CTypeNode* CFileNode::FindForward( const std::string &szTypeName, bool bOnlyTopNamespace )
{
	if ( bOnlyTopNamespace )
		return namespaces.front()->FindForward( szTypeName );
	else
	{
		for ( std::list< CObj<CNamespace> >::iterator iter = namespaces.begin(); iter != namespaces.end(); ++iter )
		{
			CNamespace *pNM = *iter;
			CTypeNode *pForward = pNM->FindForward( szTypeName );
			if ( pForward )
				return pForward;
		}
	}

	return 0;
}

CEnumEntryNode* CFileNode::FindEnumEntry( const std::string &szEntyrName, bool bOnlyTopNamespace )
{
	if ( bOnlyTopNamespace )
		return namespaces.front()->FindEnumEntry( szEntyrName );
	else
	{
		for ( std::list< CObj<CNamespace> >::iterator iter = namespaces.begin(); iter != namespaces.end(); ++iter )
		{
			CNamespace *pNM = *iter;
			CEnumEntryNode *pEntry = pNM->FindEnumEntry( szEntyrName );
			if ( pEntry )
				return pEntry;
		}
	}

	return 0;
}

}

using namespace NLang;
BASIC_REGISTER_CLASS( PARSER, CFileNode );

