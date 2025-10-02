#include "stdafx.h"

#include "ErrorsAndMessages.h"
#include "FileNode.h"
#include "FileRead.h"
#include "LangNodesDefinitions.h"
#include "ParseOperations.h"
#include "StringNumbers.h"
#include "../System/FileUtils.h"
#include "../System/FilePath.h"
#include "../Misc/StrProc.h"

int yyparse( void );

extern int yydebug;			/*  nonzero means print parse trace	*/
static string szYYFileName;
extern int nyyLineNumber;

extern int nyyLineNumber;
bool byySuccess;
bool bInTestMode;
bool bNoTrace;

int yyReadData( char *pBuf, int nMaxSize )
{
	return NLang::ReadData( pBuf, nMaxSize );
}



namespace NLang
{

void AddFile( const NFile::CFileIterator &iter )
{
	if ( !iter.IsDirectory() )
	{
		string szFileName = iter.GetFullName();
		nyyLineNumber = 1;
		NStr::ReplaceAllChars( &szFileName, '\\', '/' );

		NLang::OpenFile( szFileName );
		yyparse();
	}
}

static string szBaseFileName;
const string GetBaseFileName()
{
	return szBaseFileName;
}

bool Parse( const string &_szDir, const string &szFileMask, bool _bInTestMode )
{
	bInTestMode = _bInTestMode;
	string szDir = _szDir;
	NLang::NullStep();

	if ( szDir[szDir.size() - 1] != '/' && szDir[szDir.size() - 1] != '\\' )
		szBaseFileName = szDir + "/base.cll";
	else
		szBaseFileName = szDir + "base.cll";
	NStr::ReplaceAllChars( &szBaseFileName, '\\', '/' );
	NStr::ToLower( &szBaseFileName );

	if ( !bInTestMode )
		printf( "lexer...\n" );
	
	NFile::ConvertSlashes( &szDir, '/', '\\' );
	if ( szDir[szDir.size() - 1] != '\\' )
		szDir += '\\';

	yydebug = 0;
	byySuccess = true;
	EnumerateFiles( szDir, szFileMask.c_str(), AddFile, true );

	if ( byySuccess )
	{
		NLang::NextStep();

		if ( !bInTestMode )
			printf( "\nparsing files...\n" );
		NLang::GetRootFile()->Parse();
		NLang::GetRootFile()->GetNamespace()->ResolveForwards();
	}

	if ( !bInTestMode )
	{
		if ( byySuccess )
			printf( "\nok\n");
		else
			printf( "\nparsing failed\n");
	}

	return byySuccess;
}

bool Parse( const vector<string> &files, const string &_szBaseFileName )
{
	NLang::NullStep();
	yydebug = 0;
	byySuccess = true;
	bNoTrace = false;
	szBaseFileName = _szBaseFileName;
	NStr::ReplaceAllChars( &szBaseFileName, '\\', '/' );
	NStr::ToLower( &szBaseFileName );
	for ( int i = 0; i < files.size() && byySuccess; ++i )
	{
		string szFile = files[i];

		nyyLineNumber = 1;
		NStr::ReplaceAllChars( &szFile, '\\', '/' );
		NStr::ToLower( &szFile );

		if ( !NLang::OpenFile( szFile ) )
			NErrors::ShowErrorNoLine( StrFmt( "can't open file %s", szFile.c_str() ) );
		else
			yyparse();
	}

	if ( byySuccess )
	{
		NLang::NextStep();
		NLang::GetRootFile()->Parse();
		NLang::GetRootFile()->GetNamespace()->ResolveForwards();
	}

	return byySuccess;
}

static int nStep = 0;
int GetStep()
{
	return nStep;
}

void NextStep()
{
	++nStep;
}

void NullStep()
{
	nStep = 0;
}

bool CheckEnumEntryNotExist( const string &szEnumEntryName )
{
	CFileNode *pFile = GetCurFileNode();

	CEnumEntryNode *pEnumEntry = pFile->FindEnumEntry( szEnumEntryName, true );
	VERIFY( pEnumEntry == 0,
					StrFmt( "%s redifinition,\n\t%s(%d) previous definition was a 'enumerator'",
						szEnumEntryName.c_str(), pEnumEntry->GetFile().c_str(), pEnumEntry->GetLine() ),
					return false );

	return true;
};

//*******************************************************************
//*												Variables                                 *
//*******************************************************************

void SetTypeToVars( CLangNode *pRawVarListNode, const string &szTypeName )
{
	CHECK_TYPE( CNodesList<CVariable>, pRawVarListNode, return );
	CDynamicCast< CNodesList<CVariable> > pVarListNode = pRawVarListNode;

	CLangNode *pRawType = 0;
	CFileNode *pFile = GetCurFileNode();

	pRawType = pFile->FindDef( szTypeName, false );
	if ( !pRawType )
		pRawType = pFile->FindForward( szTypeName, false );

	VERIFY( pRawType != 0, StrFmt( "can't find type %s", szTypeName.c_str() ), return );
	CDynamicCast<CTypeNode> pType = pRawType;
	VERIFY( pType != 0,
					StrFmt( "wrong type (%s) of variable, see %s(%d)",
						szTypeName.c_str(), pRawType->GetFile().c_str(), pRawType->GetLine() ),
					return );

	CDynamicCast<CTypeDefNode> pTypeDefNode = pRawType;
	CTypeNode *pRealType = pTypeDefNode ? pTypeDefNode->GetReferencedType( true ) : pType;

	list< CPtr<CVariable> > &varList = pVarListNode->GetNodes();
	for ( list< CPtr<CVariable> >::iterator iter = varList.begin(); iter != varList.end(); ++iter )
	{
		CVariable *pVar = *iter;

		if ( pTypeDefNode )
			VERIFY( !(pVar->IsPointer() && pTypeDefNode->IsPointer()), StrFmt( "pointer to pointer, see %s", pVar->GetName().c_str() ), return );

		bool bVarPointer = pVar->IsPointer() || pTypeDefNode && pTypeDefNode->IsPointer();

		if ( CDynamicCast<CEnumNode> pRealEnum = pRealType )
		{ VERIFY( !bVarPointer, StrFmt( "pointer to enum" ), return ); }
		else
			VERIFY( !pRealType->IsForward() || bVarPointer, StrFmt( "non-pointer to forward type" ), return );

		CDynamicCast<CComplexTypeNode> pComplexType = pRealType;
		CDynamicCast<CBaseTypeNode> pBaseType = pRealType;
		bool bClass = pComplexType && pComplexType->IsClass() || pBaseType && pBaseType->IsClass();
		if ( bClass )
			{ VERIFY( bVarPointer, StrFmt( "non-pointer to class %s", szTypeName.c_str() ), return ); }
		else
			VERIFY( !bVarPointer, StrFmt( "pointer to not class %s", szTypeName.c_str() ), return );

		CDynamicCast<CEnumNode> pEnum = pRealType;
		VERIFY( !pEnum || !bVarPointer, StrFmt( "pointer to enum %s", szTypeName.c_str() ), return );

		if ( pVar->HasDefault() )
		{
			if ( pVar->GetTypeOfDefault() == EST_ENUM )
			{
				CDynamicCast<CEnumNode> pEnum = pRealType;
				VERIFY( pEnum != 0, "default value not compliant with variable type", return );
				VERIFY( pEnum->GetEnumEntry( pVar->GetDefault().GetEnum() ) != 0,
																		 StrFmt( "entry %s doesn't exist in enum, see %s(%d)",
																			pVar->GetDefault().GetEnum().c_str(), pEnum->GetFile().c_str(), pEnum->GetLine() ),
								return );
			}
			else
			{
				ESimpleType eType = GetType( pRealType->GetName() );
				VERIFY( eType != EST_UNKNOWN, "default value not compliant with variable type", return );
				VERIFY( IsTypesEqual( eType, pVar->GetTypeOfDefault() ), "default value not compliant with variable type", return );
			}
		}

		pVar->SetType( pType );
	}
}

void ConstructRndType( const string &szRefType, const string &szFlagType, const string &szCodeTypeName )
{
	CPtr<CLangNode> pTypeNode = CreateComplexTypeNode( szCodeTypeName, false );
	NLang::OpenNewNamespace( 0 );
	{
		CPtr<CLangNode> pTypeNodeNamespace = NLang::GetCurrentNamespace();
		AddNamespaceToComplexTypeNode( pTypeNode, pTypeNodeNamespace );

		CPtr<CLangNode> pElementType = CreateComplexTypeNode( "SElement", false );
		NLang::OpenNewNamespace( 0 );
		{
			CPtr<CLangNode> pElementTypeNamespace = NLang::GetCurrentNamespace();
			AddNamespaceToComplexTypeNode( pElementType, pElementTypeNamespace );

			CPtr<CLangNode> pNoPrefixAttr = NLang::CreateAttrDef( "no_prefix", "", false );
			CPtr<CLangNode> pNoPrefix = NLang::CreateAttrListNode( pNoPrefixAttr );

			CPtr<CLangNode> pFlagVar = CreateVar( "eFlag", "", "" );
			CPtr<CLangNode> pFlagVarFull = CreateVarListNode( pFlagVar );
			SetAttrToVars( pFlagVarFull, pNoPrefix );
			SetTypeToVars( pFlagVarFull, szFlagType );
			AddVarListToNamespace( pElementTypeNamespace, pFlagVarFull );

			CPtr<CLangNode> pWeightVar = CreateVar( "fWeight", "", "" );
			CPtr<CLangNode> pWeightVarFull = CreateVarListNode( pWeightVar );
			SetAttrToVars( pFlagVarFull, pNoPrefix );
			SetTypeToVars( pWeightVarFull, "float" );
			AddVarListToNamespace( pElementTypeNamespace, pWeightVarFull );

			CPtr<CLangNode> pObjVar = CreateVar( "pObj", "", "" );
			SetVarToPointer( pObjVar );
			CPtr<CLangNode> pObjVarFull = CreateVarListNode( pObjVar );
			SetTypeToVars( pObjVarFull, szRefType );
			AddVarListToNamespace( pElementTypeNamespace, pObjVarFull );
		}
		NLang::CloseNamespace();

		AddTypeToNamespace( pTypeNodeNamespace, pElementType );

		const string szTypeDefs = StrFmt( "%s;TRefType;%s;EFlagType", szRefType.c_str(), szFlagType.c_str() );
		CPtr<CLangNode> pTypeDefs = NLang::CreateAttrDef( "type_defs", szTypeDefs, true );
		CPtr<CLangNode> pAttributes = NLang::CreateAttrListNode( pTypeDefs );
		NLang::AddAttrToComplexTypeNode( pTypeNode, pAttributes );

		CPtr<CLangNode> pElementsVar = CreateVar( "elements", "0", "unbounded" );
		CPtr<CLangNode> pElementsVarFull = CreateVarListNode( pElementsVar );
		SetTypeToVars( pElementsVarFull, "SElement" );
		AddVarListToNamespace( pTypeNodeNamespace, pElementsVarFull );
	}
	NLang::CloseNamespace();

	AddDef( pTypeNode );
}

void SetRndTypeToVars( CLangNode *pVarListNode, const string &szTypeName )
{
	const int nPos = szTypeName.find( "$" );
	const string szRefType = nPos == string::npos ? szTypeName : szTypeName.substr( 0, nPos );
	const string szFlagType = nPos == string::npos ? "int" : szTypeName.substr( nPos + 1, szTypeName.size() );
	const string szCodeTypeName = "SRnd_" + szRefType + "_" + szFlagType;

	CFileNode *pFile = GetCurFileNode();
	CLangNode *pRawType = pFile->FindDef( szCodeTypeName, false );
	if ( pRawType != 0 )
		SetTypeToVars( pVarListNode, szCodeTypeName );
	else
	{
		CLangNode *pRefType = pFile->FindDef( szRefType, false );
		if ( pRefType == 0 )
			pRefType = pFile->FindForward( szRefType, false );
		VERIFY( pRefType != 0, StrFmt( "can't find definition of type %s", szRefType.c_str() ), return );
		CDynamicCast<CComplexTypeNode> pComplexRefType = pRefType;
		VERIFY( pComplexRefType != 0 && pComplexRefType->IsClass(), StrFmt( "type %s isn't a class", szRefType.c_str() ), return );

		if ( szFlagType != "int" )
		{
			CLangNode *pFlagType = pFile->FindDef( szFlagType, false );
			VERIFY( pFlagType != 0, StrFmt( "can't find definition of enum %s", szFlagType.c_str() ), return );
			CDynamicCast<CEnumNode> pEnumFlagType = pFlagType;
			VERIFY( pEnumFlagType != 0, StrFmt( "type %s isn't enum,\n%s(%d) see definition", 
																					szFlagType.c_str(), pFlagType->GetFile().c_str(), pFlagType->GetLine() ), return );
		}
		
		ConstructRndType( szRefType, szFlagType, szCodeTypeName );
		SetTypeToVars( pVarListNode, szCodeTypeName );
	}
}

void SetAttrToVars( CLangNode *pRawVarListNode, CLangNode *pRawAttrListNode )
{
	if ( pRawAttrListNode == 0 )
		return;

	CHECK_TYPE( CNodesList<CVariable>, pRawVarListNode, return );
	CDynamicCast< CNodesList<CVariable> > pVarListNode = pRawVarListNode;

	CHECK_TYPE( CNodesList<CAttributeNode>, pRawAttrListNode, return );
	CDynamicCast< CNodesList<CAttributeNode> > pAttrListNode = pRawAttrListNode;

	list< CPtr<CVariable> > &varList = pVarListNode->GetNodes();
	for ( list< CPtr<CVariable> >::iterator iter = varList.begin(); iter != varList.end(); ++iter )
	{
		CVariable *pNode = *iter;
		pNode->SetAttrList( pAttrListNode );
	}
}

void AddVarToVarListNode( CLangNode *pRawVarListNode, CLangNode *pRawVar )
{
	if ( pRawVar == 0 )
		return;
	
	CHECK_TYPE( CNodesList<CVariable>, pRawVarListNode, return );
	CDynamicCast< CNodesList<CVariable> > pVarListNode = pRawVarListNode;

	CHECK_TYPE( CVariable, pRawVar, return );
	CDynamicCast<CVariable> pVar = pRawVar;

	pVarListNode->AddNode( pVar );
}

CLangNode* CreateVarListNode( CLangNode *pVar )
{
	CNodesList<CVariable> *pVarList = new CNodesList<CVariable>();
	AddVarToVarListNode( pVarList, pVar );

	return pVarList;
}

void SetVarToPointer( CLangNode *pRawVar )
{
	CHECK_TYPE( CVariable, pRawVar, return );
	CDynamicCast<CVariable> pVar = pRawVar;

	pVar->SetVarToPointer();
}

void SetDefValueToVarNode( CLangNode *pRawVarNode, const string &szValue, bool bStringValue )
{
	CHECK_TYPE( CVariable, pRawVarNode, return );
	CDynamicCast<CVariable> pVarNode = pRawVarNode;

	pVarNode->SetDefault( szValue, bStringValue );
}

void SetComplexDefaultValueToVarNode( CLangNode *pRawVarNode, const string &szValue )
{
	CHECK_TYPE( CVariable, pRawVarNode, return );
	CDynamicCast<CVariable> pVarNode = pRawVarNode;

	pVarNode->SetComplexDefault( szValue );
}

void SetDefWStrValueToVarNode( CLangNode *pRawVarNode, const string &szValue )
{
	CHECK_TYPE( CVariable, pRawVarNode, return );
	CDynamicCast<CVariable> pVarNode = pRawVarNode;

	pVarNode->SetDefaultWStr( szValue );
}

void SetEnumValueToVarNode( CLangNode *pRawVarNode, const string &szValue )
{
	CHECK_TYPE( CVariable, pRawVarNode, return );
	CDynamicCast<CVariable> pVarNode = pRawVarNode;

	pVarNode->SetDefaultEnum( szValue );
}

CLangNode* CreateVarNode( const string &szVarName )
{
	return new CVariableNode( szVarName, GetParsingFileName(), nyyLineNumber );
}

CLangNode* CreateVectorNode( const string &szVectorName, const string &szMinAmount, const string &szMaxAmount )
{
	int nMinAmount = NStr::ToInt( szMinAmount );
	VERIFY( nMinAmount >= 0, StrFmt( "wrong array lower bound %d", nMinAmount ), return 0 );

	int nMaxAmount = 0;
	if ( szMaxAmount == "unbounded" )
		nMaxAmount = -1;
	else
	{
		nMaxAmount = NStr::ToInt( szMaxAmount );
		VERIFY( nMaxAmount >= 0, StrFmt( "wrong array upper bound %d", nMaxAmount ), return 0 );
		VERIFY( nMaxAmount >= nMinAmount, StrFmt( "wrong array bounds [%d..%d]", nMinAmount, nMaxAmount ), return 0 );
	}

	return new CVectorNode( szVectorName, nMinAmount, nMaxAmount, GetParsingFileName(), nyyLineNumber );
}

CLangNode* CreateVar( const string &szVarName, const string &szMinAmount, const string &szMaxAmount )
{
	CheckEnumEntryNotExist( szVarName );
	return szMinAmount == "" ? CreateVarNode( szVarName ) : CreateVectorNode( szVarName, szMinAmount, szMaxAmount);
}

//*******************************************************************
//*                     Namespace                                   *
//*******************************************************************

void AddTypeToNamespace( CLangNode *pRawNamespace, CLangNode *pType )
{
	CHECK_TYPE( CNamespace, pRawNamespace, return );
	CDynamicCast<CNamespace> pNamespace = pRawNamespace;

	pNamespace->AddInsideDef( pType );
}

void AddVarListToNamespace( CLangNode *pRawNamespace, CLangNode *pRawVarList )
{
	CHECK_TYPE( CNamespace, pRawNamespace, return );
	CDynamicCast<CNamespace> pNamespace = pRawNamespace;

	CHECK_TYPE( CNodesList<CVariable>, pRawVarList, return );
	CDynamicCast< CNodesList<CVariable> > pVarList = pRawVarList;

	list< CPtr<CVariable> > &varList = pVarList->GetNodes();
	for ( list< CPtr<CVariable> >::iterator iter = varList.begin(); iter != varList.end(); ++iter )
	{
		CVariable *pNode = *iter;
		pNamespace->AddInsideDef( pNode );
	}
}

void AddBadIncludeToNamespace( CLangNode *pRawNamespace, const string &szInclude )
{
	CHECK_TYPE( CNamespace, pRawNamespace, return );
	CDynamicCast<CNamespace> pNamespace = pRawNamespace;
	pNamespace->AddBadInclude( szInclude );
}

CLangNode* GetCurrentNamespace()
{
	CFileNode *pFile = GetCurFileNode();
	return pFile->GetNamespace();
}

//*******************************************************************
//*                     Complex Type                                *
//*******************************************************************

void AddParentToParentsList( CLangNode *pRawComplexTypeList, const string &szTypeName )
{
	CHECK_TYPE( CNodesList<CComplexTypeNode>, pRawComplexTypeList, return );
	CDynamicCast< CNodesList<CComplexTypeNode> > pComplexTypeList = pRawComplexTypeList;

	CFileNode *pFileNode = GetCurFileNode();
	CLangNode *pNode = pFileNode->FindDef( szTypeName, false );
	VERIFY( pNode != 0, StrFmt( "can't find parent %s", szTypeName.c_str() ), return );

	CDynamicCast<CComplexTypeNode> pTypeNode = pNode;
	if ( !pTypeNode )
	{
		CDynamicCast<CTypeDefNode> pTypeDefNode = pNode;
		VERIFY( pTypeDefNode != 0, StrFmt( "wrong parent %s type, struct or class expected", szTypeName.c_str() ), return );
		pTypeNode = pTypeDefNode->GetReferencedType( true );
		VERIFY( pTypeNode != 0, StrFmt( "wrong parent %s type, struct or class expected", szTypeName.c_str() ), return );
	}

	for ( list< CPtr<CComplexTypeNode> >::const_iterator iter = pComplexTypeList->GetNodes().begin(); iter != pComplexTypeList->GetNodes().end(); ++iter )
	{
		CComplexTypeNode *pParent = *iter;
		VERIFY( pParent->GetName() != szTypeName,
						StrFmt( "%s is already direct base class of %s", szTypeName.c_str(), szTypeName.c_str() ),
						return );
	}

	pComplexTypeList->AddNode( pTypeNode.GetPtr() );
}

CLangNode* CreateParentsList( const string &szFirstTypeName )
{
	CNodesList<CComplexTypeNode> *pList = new CNodesList<CComplexTypeNode>();
	AddParentToParentsList( pList, szFirstTypeName );
	return pList;
}

void OpenNewNamespace( CLangNode *pRawVisibleTypes )
{
	CFileNode *pFileNode = GetCurFileNode();
	if ( pRawVisibleTypes == 0 )
		pFileNode->OpenNewNamespace( 0 );
	else
	{
		CHECK_TYPE( CNodesList<CComplexTypeNode>, pRawVisibleTypes, return );
		CDynamicCast< CNodesList<CComplexTypeNode> > pVisibleTypes = pRawVisibleTypes;
		pFileNode->OpenNewNamespace( pVisibleTypes );
	}
}

void CloseNamespace()
{
	CFileNode *pFileNode = GetCurFileNode();
	pFileNode->CloseNamespace( true );
}

void AddNamespaceToComplexTypeNode( CLangNode *pRawNode, CLangNode *pRawNamespace )
{
	if ( pRawNamespace == 0 || pRawNode == 0 )
		return;
	
	CHECK_TYPE( CComplexTypeNode, pRawNode, return );
	CDynamicCast<CComplexTypeNode> pNode = pRawNode;

	CHECK_TYPE( CNamespace, pRawNamespace, return );
	CDynamicCast<CNamespace> pNamespace = pRawNamespace;

	pNode->AddNamespace( pNamespace );
}

void AddAttrToComplexTypeNode( CLangNode *pRawNode, CLangNode *pRawAttrList )
{
	if ( pRawAttrList == 0 )
		return;
	
	CHECK_TYPE( CComplexTypeNode, pRawNode, return );
	CDynamicCast<CComplexTypeNode> pNode = pRawNode;

	CHECK_TYPE( CNodesList<CAttributeNode>, pRawAttrList, return );
	CDynamicCast< CNodesList<CAttributeNode> > pAttrList = pRawAttrList;

	pNode->AddAttributes( pAttrList );
}

void AddParentsOfComplexType( CLangNode *pRawNode, CLangNode *pRawParentsList )
{
	CHECK_TYPE( CComplexTypeNode, pRawNode, return );
	CDynamicCast<CComplexTypeNode> pNode = pRawNode;

	CHECK_TYPE( CNodesList<CComplexTypeNode>, pRawParentsList, return );
	CDynamicCast< CNodesList<CComplexTypeNode> > pParentsList = pRawParentsList;

	for ( list< CPtr<CComplexTypeNode> >::const_iterator iter = pParentsList->GetNodes().begin(); iter != pParentsList->GetNodes().end(); ++iter )
	{
		CComplexTypeNode *pParent = *iter;
		VERIFY( pParent->IsClass() == pNode->IsClass(), 
						StrFmt( "wrong parent %s of type %s (class <-> struct)", pParent->GetName().c_str(), pNode->GetName().c_str() ),
						return );
	}

	pNode->AddParents( pParentsList );
}

CLangNode* CreateComplexTypeNode( const string &szTypeName, bool bClass )
{
	CheckEnumEntryNotExist( szTypeName );

	CLangNode *pResult = new CComplexTypeNode( szTypeName, bClass, false, GetParsingFileName(), nyyLineNumber );
	
	CFileNode *pFile = GetCurFileNode();
	CLangNode *pTypeNode = pFile->FindDef( szTypeName, true );
	VERIFY( pTypeNode == 0, StrFmt( "type %s redifinition, see %s(%d)", szTypeName.c_str(), pTypeNode->GetFile().c_str(), pTypeNode->GetLine() ), return 0 );

	CTypeNode *pForwardTypeNode = pFile->FindForward( szTypeName, true );
	VERIFY( pForwardTypeNode == 0 || IsEqualDefs( pForwardTypeNode, pResult ), StrFmt( "type %s redifinition, see %s(%d)", szTypeName.c_str(), pForwardTypeNode->GetFile().c_str(), pForwardTypeNode->GetLine() ), return 0 );

	return pResult;
}

CLangNode* CreateForwardComplexType( const string &szTypeName, bool bIsClass )
{
	CFileNode *pFile = GetCurFileNode();
	CLangNode *pResult = new CComplexTypeNode( szTypeName, bIsClass, true, GetParsingFileName(), nyyLineNumber ); 

	CLangNode *pNode = pFile->FindDef( szTypeName, true );
	if ( pNode )
		VERIFY( IsEqualDefs( pNode, pResult ), StrFmt( "type %s redifinition, see %s(%d)", szTypeName.c_str(), pNode->GetFile().c_str(), pNode->GetLine() ), return 0 );

	CLangNode *pNodeForward = pFile->FindForward( szTypeName, true );
	if ( pNodeForward )
		VERIFY( IsEqualDefs( pNodeForward, pResult ), StrFmt( "type %s redifinition, see %s(%d)", szTypeName.c_str(), pNodeForward->GetFile().c_str(), pNodeForward->GetLine() ), return 0 );

	CheckEnumEntryNotExist( szTypeName );
	
	return pResult;
}

//*******************************************************************
//*													TypeDef																	*
//*******************************************************************

CLangNode* CreateTypeDefNode( CLangNode *pRawAttrListNode, const string &szReferencedTypeName, const string &szTypeName, bool bPointer )
{
	if ( pRawAttrListNode )
		CHECK_TYPE( CNodesList<CAttributeNode>, pRawAttrListNode, return 0 );
	CDynamicCast< CNodesList<CAttributeNode> > pAttrListNode = pRawAttrListNode ? pRawAttrListNode : 0;

	CheckEnumEntryNotExist( szTypeName );

	CFileNode *pCurFile = GetCurFileNode();

	{
		CLangNode *pNode = pCurFile->FindDef( szTypeName, true );
		VERIFY( pNode == 0, StrFmt( "type %s redifinition, see %s(%d)", szTypeName.c_str(), pNode->GetFile().c_str(), pNode->GetLine() ), return 0 );
	}

	CLangNode *pNode = pCurFile->FindDef( szReferencedTypeName, false );
	if ( pNode )
	{
		CDynamicCast<CTypeNode> pTypeNode = pNode;
		VERIFY( pTypeNode != 0, 
						StrFmt( "%s cannot be overloaded as typedef, \n	%s(%d): see declaration of %s",
						szReferencedTypeName.c_str(), pTypeNode->GetFile().c_str(), pTypeNode->GetLine(), szReferencedTypeName.c_str() ),
						return 0 );
		CDynamicCast<CEnumNode> pEnum = pNode;
		VERIFY( !bPointer || !pEnum, "pointer to enum", return 0 );

		CDynamicCast<CTypeDefNode> pTypeDef = pNode;
		if ( pTypeDef )
		{
			VERIFY( !bPointer || !pTypeDef->IsPointer(), "pointer to pointer", return 0 );
			CTypeNode *pRealTypeNode = pTypeDef->GetReferencedType( true );

			CDynamicCast<CEnumNode> pEnum = pRealTypeNode;
			VERIFY( !bPointer || !pEnum, "pointer to enum", return 0 );

			CDynamicCast<CComplexTypeNode> pComplexType = pRealTypeNode;
			if ( pComplexType )
				VERIFY( !bPointer || pComplexType->IsClass(), "pointer to struct",  return 0 );
		}

		CDynamicCast<CComplexTypeNode> pComplexNode = pNode;
		if ( pComplexNode )
			VERIFY( !bPointer || pComplexNode->IsClass(), "pointer to struct", return 0 );

		return new CTypeDefNode( szTypeName, pAttrListNode, pTypeNode.GetPtr(), bPointer, GetParsingFileName(), nyyLineNumber );
	}

	CTypeNode *pForwardNode = pCurFile->FindForward( szReferencedTypeName, false );
	if ( pForwardNode != 0 )
	{
		CDynamicCast<CEnumNode> pEnum = pForwardNode;
		VERIFY( !bPointer || !pEnum, "pointer to enum", return 0 );

		return new CTypeDefNode( szTypeName, pAttrListNode, pForwardNode, bPointer, GetParsingFileName(), nyyLineNumber );
	}

	VERIFY( false, StrFmt( "can't find type %s", szReferencedTypeName.c_str() ), return 0 );

	return 0;
}

//*******************************************************************
//*                     Attributes                                  *
//*******************************************************************

void MergeAttrList( CLangNode *pRawAttrListNode1, CLangNode *pRawAttrListNode2 )
{
	CPtr<CLangNode> pNode = pRawAttrListNode2;
	
	CHECK_TYPE( CNodesList<CAttributeNode>, pRawAttrListNode1, return );
	CDynamicCast< CNodesList<CAttributeNode> > pAttrListNode1 = pRawAttrListNode1;

	CHECK_TYPE( CNodesList<CAttributeNode>, pRawAttrListNode2, return );
	CDynamicCast< CNodesList<CAttributeNode> > pAttrListNode2 = pRawAttrListNode2;

	for ( list< CPtr<CAttributeNode> >::const_iterator iter = pAttrListNode2->GetNodes().begin(); iter != pAttrListNode2->GetNodes().end(); ++iter )
	{
		CAttributeNode *pNode = *iter;

		if ( find( pAttrListNode1->GetNodes().begin(), pAttrListNode1->GetNodes().end(), pNode ) == pAttrListNode1->GetNodes().end() )
			pAttrListNode1->AddNode( pNode );
	}
}

void AddAttrEntry( CLangNode *pRawAttrListNode, CLangNode *pRawAttrNode )
{
	if ( pRawAttrListNode == 0 || pRawAttrNode == 0 )
		return;

	CHECK_TYPE( CNodesList<CAttributeNode>, pRawAttrListNode, return );
	CDynamicCast< CNodesList<CAttributeNode> > pAttrListNode = pRawAttrListNode;

	CHECK_TYPE( CAttributeNode, pRawAttrNode, return );
	CDynamicCast<CAttributeNode> pAttrNode = pRawAttrNode;

	if ( find( pAttrListNode->GetNodes().begin(), pAttrListNode->GetNodes().end(), pAttrNode ) == pAttrListNode->GetNodes().end() )
		pAttrListNode->AddNode( pAttrNode );
}

CLangNode* CreateAttrListNode( CLangNode *pRawAttrNode )
{
	CNodesList<CAttributeNode> *pListNode = new CNodesList<CAttributeNode>();
	
	if ( pRawAttrNode )
	{
		CHECK_TYPE( CAttributeNode, pRawAttrNode, return pListNode );
		CDynamicCast<CAttributeNode> pAttrNode = pRawAttrNode;
		pListNode->AddNode( pAttrNode );
	}

	return pListNode;
}

CLangNode* CreateAttrDef( const string &szAttrName, const string &szRawAttrValue, bool bStringValue )
{
	CFileNode *pCurFileNode = GetCurFileNode();

	string szAttrValue( szRawAttrValue );
	if ( szAttrName == "comments" && !szAttrValue.empty() && szAttrValue[szAttrValue.size() - 1] == 13 )
		szAttrValue.pop_back();

	CAttributeDefNode *pAttrDef = 0;
	if ( szAttrName != "comments" )
	{
		pAttrDef = pCurFileNode->FindAttrDef( szAttrName );
		VERIFY( pAttrDef != 0, StrFmt( "unknown attribute %s", szAttrName.c_str() ), return 0 );
	}

	CAttributeNode *pNode = new CAttributeNode( szAttrName, szAttrValue, bStringValue, GetParsingFileName(), nyyLineNumber );
	if ( szAttrName != "comments" )
	{
		VERIFY( IsTypesEqual( pNode->GetType(), pAttrDef->GetType() ) ||
						pAttrDef->GetType() == EST_BOOL && pNode->GetType() == EST_NOTYPE,
							StrFmt( "wrong attribute \"%s\" of type \"%s\", \"%s\" expected", 
							szAttrName.c_str(), GetTypeName( pNode->GetType() ), GetTypeName( pAttrDef->GetType() ) ), return 0 );
	}

	return pNode;
}

//*******************************************************************
//*                     Enums                                       *
//*******************************************************************

CLangNode* CreateForwardEnumNode( const string &szEnumName )
{
	CFileNode *pFile = GetCurFileNode();
	CLangNode *pNode = pFile->FindDef( szEnumName, true );
	if ( pNode )
	{
		CDynamicCast<CEnumNode> pEnum = pNode;
		VERIFY( pEnum != 0, StrFmt( "type %s redifinition, see %s(%d)", szEnumName.c_str(), pNode->GetFile().c_str(), pNode->GetLine() ), return 0 );
	}

	CLangNode *pForwardNode = pFile->FindForward( szEnumName, true );
	if ( pForwardNode )
	{
		CDynamicCast<CEnumNode> pEnum = pForwardNode;
		VERIFY( pEnum != 0, StrFmt( "type %s redifinition, see %s(%d)", szEnumName.c_str(), pForwardNode->GetFile().c_str(), pForwardNode->GetLine() ), return 0 );
	}
	
	return new CEnumNode( szEnumName, true, GetParsingFileName(), nyyLineNumber );
}

void SetNameToEnumNode( CLangNode *pRawEnumNode, const string &szName )
{
	CHECK_TYPE( CEnumNode, pRawEnumNode, return );
	CDynamicCast<CEnumNode> pNode = pRawEnumNode;
	pNode->SetName( szName );
}

CLangNode* CreateEnumNode( CLangNode *pEnumEntryNode )
{
	CEnumNode *pNode = new CEnumNode( GetParsingFileName(), nyyLineNumber );

	if ( pEnumEntryNode != 0 )
		AddEnumEntry( pNode, pEnumEntryNode );
	
	return pNode;	
}

void AddEnumEntry( CLangNode *pRawEnumNode, CLangNode *pRawEnumEntryNode )
{
	CHECK_TYPE( CEnumNode, pRawEnumNode, return );
	CDynamicCast<CEnumNode> pEnumNode = pRawEnumNode;

	CHECK_TYPE( CEnumEntryNode, pRawEnumEntryNode, return );
	CDynamicCast<CEnumEntryNode> pEnumEntryNode = pRawEnumEntryNode;

	for ( CEnumNode::TEntriesIter iter = pEnumNode->EntriesBegin(); iter != pEnumNode->EntriesEnd(); ++iter )
	{
		CEnumEntryNode *pNode = *iter;
		VERIFY( pNode->GetName() != pEnumEntryNode->GetName(), StrFmt( "second enum entry (%s) found", pEnumEntryNode->GetName() ), return );
	}

	CEnumEntryNode *pLastEntry = 0;
	if ( pEnumNode->EntriesBegin() != pEnumNode->EntriesEnd() )
	{
		CEnumNode::TEntriesIter iter = pEnumNode->EntriesEnd();
		--iter;
		pLastEntry = *iter;
	}

	CheckEnumEntryNotExist( pEnumEntryNode->GetName() );
	pEnumNode->AddEnumEntry( pEnumEntryNode );

	if ( !pEnumEntryNode->IsValueDefined() )
	{
		if ( !pLastEntry )
			pEnumEntryNode->SetValue( 0 );
		else
		{
			NI_VERIFY( pLastEntry->IsValueDefined(), "non-defined enum entry", pLastEntry->SetValue( 0 ) );
			pEnumEntryNode->SetValue( pLastEntry->GetValue() + 1 );
		}
	}
}

void AddAttrToEnumNode( CLangNode *pRawEnumNode, CLangNode *pRawAttrList )
{
	if ( pRawAttrList == 0 )
		return;

	CHECK_TYPE( CEnumNode, pRawEnumNode, return );
	CDynamicCast<CEnumNode> pEnumNode = pRawEnumNode;

	CHECK_TYPE( CNodesList<CAttributeNode>, pRawAttrList, return );
	CDynamicCast< CNodesList<CAttributeNode> > pAttrList = pRawAttrList;

	pEnumNode->AddAttributes( pAttrList );
}

CLangNode* CreateEnumEntryNode( const string &szEntryName, const string &szDefaultValue, bool bDefaultValueNumber )
{
	if ( szDefaultValue == "" )
		return new CEnumEntryNode( szEntryName, GetParsingFileName(), nyyLineNumber );
	else
	{
		if ( bDefaultValueNumber )
		{
			VERIFY( NStr::IsInt( szDefaultValue ) || NStr::IsHexNumber( szDefaultValue ), 
							StrFmt( "value %s of enum entry is not a number", szDefaultValue ),
							return new CEnumEntryNode( szEntryName, GetParsingFileName(), nyyLineNumber ) );

			return new CEnumEntryNode( szEntryName, NStr::ToInt( szDefaultValue ), GetParsingFileName(), nyyLineNumber );
		}
		else
		{
			CFileNode *pFile = GetCurFileNode();
			CNamespace *pNM = pFile->GetNamespace();

			CEnumEntryNode *pNode = pFile->FindEnumEntry( szDefaultValue, true );
			VERIFY( pNode != 0, StrFmt( "can't find enumerator %s", szDefaultValue.c_str() ), return 0 );
			return	pNode->IsValueDefined() ?
							new CEnumEntryNode( szEntryName, pNode->GetValue(), GetParsingFileName(), nyyLineNumber ) :
							new CEnumEntryNode( szEntryName, GetParsingFileName(), nyyLineNumber );
		}
	}
}

//*******************************************************************
//*                     BaseType                                    *
//*******************************************************************

CLangNode* CreateBaseTypeNode( const string &szTypeName, bool bIsClass )
{ 
	return new CBaseTypeNode( szTypeName, bIsClass, GetParsingFileName(), nyyLineNumber );
}

//*******************************************************************
//*                     Attribute Def                               *
//*******************************************************************

CLangNode* CreateAttributeDefNode( ESimpleType eType )
{
	return new CAttributeDefNode( eType, GetParsingFileName(), nyyLineNumber );
}

void SetNameToAttrDef( CLangNode *pRawNode, const string &szName )
{
	CHECK_TYPE( CAttributeDefNode, pRawNode, return );

	CDynamicCast<CAttributeDefNode> pNode = pRawNode;
	pNode->SetName( szName );
}

//*******************************************************************
//*                     AddType                                     *
//*******************************************************************

bool IsEqualDefs( CLangNode *pNode1, CLangNode *pNode2 )
{
	if ( typeid( *pNode1 ) != typeid( *pNode2 ) )
		return false;

	CDynamicCast<CComplexTypeNode> pTypeNode1 = pNode1;
	CDynamicCast<CComplexTypeNode> pTypeNode2 = pNode2;
	if ( pTypeNode1 && pTypeNode1->IsClass() != pTypeNode2->IsClass() )
		return false;

	CDynamicCast<CBaseTypeNode> pBaseTypeNode1 = pNode1;
	CDynamicCast<CBaseTypeNode> pBaseTypeNode2 = pNode2;
	if ( pBaseTypeNode1 && pBaseTypeNode1->IsClass() != pBaseTypeNode2->IsClass() )
		return false;

	return true;
}

static void AddAttrDef( CAttributeDefNode *pAttrDefNode )
{
	CFileNode *pFile = GetCurFileNode();
	CAttributeDefNode* pAttrDefNode2 = pFile->FindAttrDef( pAttrDefNode->GetName() );
	if ( pAttrDefNode2 != 0 )
	{
		NErrors::ShowErrorNoLine( StrFmt( "second definition of attribute %s found, see %s(%d)", pAttrDefNode2->GetName().c_str(), pAttrDefNode2->GetFile().c_str(), pAttrDefNode2->GetLine() ) );
		return;
	}
	else
		pFile->AddAttrDef( pAttrDefNode );
}

void AddDef( CLangNode *pRawNode )
{
	if ( pRawNode == 0 )
		return;
	
	CFileNode *pFile = GetCurFileNode();
	
	if ( CDynamicCast<CAttributeDefNode> pNode = pRawNode )
		AddAttrDef( pNode );
	else
	{
		CDynamicCast<CTypeNode> pTypeNode = pRawNode;
		if ( pTypeNode )
		{
			if ( pTypeNode->IsForward() )
			{
				CLangNode *pNodeDef = pFile->FindDef( pRawNode->GetName(), true );
				if ( pNodeDef )
					VERIFY( IsEqualDefs( pRawNode, pNodeDef ) == true, StrFmt( "type %s redifinition, see %s(%d)", pRawNode->GetName().c_str(), pNodeDef->GetFile().c_str(), pNodeDef->GetLine() ), return );

				CLangNode *pForwardNodeDef = pFile->FindForward( pRawNode->GetName(), true );
				if ( pForwardNodeDef )
					VERIFY( IsEqualDefs( pRawNode, pForwardNodeDef ) == true, StrFmt( "%type s redifinition, see %s(%d)", pRawNode->GetName().c_str(), pForwardNodeDef->GetFile().c_str(), pForwardNodeDef->GetLine() ), return );
			}
			else
			{
				CLangNode *pForwardNode = pFile->FindForward( pRawNode->GetName(), true );
				if ( pForwardNode )
					VERIFY( IsEqualDefs( pRawNode, pForwardNode ) == true, StrFmt( "type %s redifinition, see %s(%d)", pRawNode->GetName().c_str(), pForwardNode->GetFile().c_str(), pForwardNode->GetLine() ), return );

				CLangNode *pNode1 = pFile->FindDef( pRawNode->GetName(), true );
				VERIFY( pNode1 == 0, StrFmt( "type %s redifinition, see %s(%d)", pRawNode->GetName().c_str(), pNode1->GetFile().c_str(), pNode1->GetLine() ), return );
			}
		}

		pFile->AddDef( pTypeNode );
	}
}

}


