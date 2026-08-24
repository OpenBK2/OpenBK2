#include "stdafx.h"

#include "LangNodesDefinitions.h"
#include "ParseOperations.h"
#include "StringNumbers.h"
#include "ErrorsAndMessages.h"
#include "Misc/StrProc.h"

#include "Parser_export.h"

#include <fmt/format.h>

void yyerror_no_line( char *s, ... );

namespace NLang
{
template <typename T>
struct SListOperations
{
	typedef std::list< CPtr<T> > TList;
	static TList emptyList;

	typename TList::const_iterator	Begin( CNodesList<T> *pNodesList )
	{
		return pNodesList ? pNodesList->GetNodes().begin() : emptyList.end();
	}

	typename TList::const_iterator	End( CNodesList<T> *pNodesList )
	{
		return pNodesList ? pNodesList->GetNodes().end() : emptyList.end();
	}
};

SListOperations<CEnumEntryNode>::TList SListOperations<CEnumEntryNode>::emptyList;
SListOperations<CAttributeNode>::TList SListOperations<CAttributeNode>::emptyList;
SListOperations<CComplexTypeNode>::TList SListOperations<CComplexTypeNode>::emptyList;

//*******************************************************************
//*                     CNamespace                                  *
//*******************************************************************

CNamespace::TLangNodeIter CNamespace::DefsBegin() const
{
	return insideDefs.begin();
}

CNamespace::TLangNodeIter CNamespace::DefsEnd() const
{
	return insideDefs.end();
}

CAttributeDefNode* CNamespace::FindInsideAttrDef( const std::string &szAttrDef )
{
	std::unordered_map<std::string, CObj<CAttributeDefNode> >::iterator iter = insideAttrs.find( szAttrDef );
	return iter == insideAttrs.end() ? 0 : iter->second;
}

void CNamespace::AddInsideAttrDef( CAttributeDefNode *pAttrDefNode )
{
	insideAttrs[pAttrDefNode->GetName()] = pAttrDefNode;
	insideDefList.push_back( pAttrDefNode );
}

CTypeNode* CNamespace::FindForward( const std::string &szTypeName )
{
	std::unordered_map<std::string, std::list< CObj<CTypeNode> > >::iterator iter = insideForwards.find( szTypeName );
	return iter == insideForwards.end() ? 0 : iter->second.back();
}

void CNamespace::AddInsideDef( CLangNode *pNode )
{
	if ( pNode == 0 )
		return;

	if ( CDynamicCast<CTypeNode> pTypeNode = pNode )
	{
		if ( pTypeNode->IsForward() )
			insideForwards[pNode->GetName()].push_back( pTypeNode.GetPtr() );
		else
			insideDefs[pNode->GetName()] = pNode;
	}
	else
		insideDefs[pNode->GetName()] = pNode;

	insideDefList.push_back( pNode );
}

CLangNode* CNamespace::FindInsideDef( const std::string &szDefName )
{
	std::unordered_map<std::string, CObj<CLangNode> >::iterator iter = insideDefs.find( szDefName );
	if ( iter == insideDefs.end() )
	{
		if ( pVisibleTypes )
		{
			std::list< CPtr<CComplexTypeNode> > &visTypes = pVisibleTypes->GetNodes();
			for ( std::list< CPtr<CComplexTypeNode> >::iterator iter = visTypes.begin(); iter != visTypes.end(); ++iter )
			{
				CComplexTypeNode *pTypeNode = *iter;
				if ( pTypeNode->GetName() == szDefName )
					return pTypeNode;
				if ( CDynamicCast<CComplexTypeNode> pComplexTypeNode = pTypeNode )
				{
					CNamespace *pVisibleNM = pComplexTypeNode->GetNamespace();
					CLangNode *pNodeInVisibleNamespace = pVisibleNM->FindInsideDef( szDefName );
					if ( pNodeInVisibleNamespace != 0 )
						return pNodeInVisibleNamespace;
				}
			}
		}

		return 0;
	}
	else
    return iter->second;
}

void CNamespace::MergeFiles( CNamespace *pNM, const std::string &szFileName )
{
	if ( files.find( szFileName ) != files.end() )
		return;
	// merge insideDefs
	for ( std::unordered_map<std::string, CObj<CLangNode> >::iterator iter = pNM->insideDefs.begin(); iter != pNM->insideDefs.end(); ++iter )
	{
		const std::string &szDefName = iter->first;
		CLangNode *pNode = iter->second;
		if ( files.find( pNode->GetFile() ) != files.end() )
			continue;

		VERIFY_NOLINE( insideDefs.find( szDefName ) == insideDefs.end(), 
			fmt::format( "{}({}) error: type {} redifinition, see \n\t\t{}({}):",
			pNM->insideDefs[szDefName]->GetFile(), pNM->insideDefs[szDefName]->GetLine(), szDefName,
			insideDefs[szDefName]->GetFile(), insideDefs[szDefName]->GetLine() ), return );

		CDynamicCast<CEnumNode> pEnum = pNode;
		if ( pEnum )
		{
			CEnumEntryNode *pWithEqualEntries = pEnum->FindAnyWithCrossedEntries( this );
			WARNING_NOLINE(	pWithEqualEntries == 0,
				fmt::format( "{}({}) warning: enum entry \"{}\" redifinition, enum {}, see\n\t\t{}({}):",
				pEnum->GetFile(), pEnum->GetLine(), pWithEqualEntries->GetName(), pEnum->GetName(),
				pWithEqualEntries->GetFile(), pWithEqualEntries->GetLine() ) );
		}

		insideDefs[iter->first] = iter->second;
	}

	// merge insideAttrs
	for ( std::unordered_map<std::string, CObj<CAttributeDefNode> >::iterator iter = pNM->insideAttrs.begin(); iter != pNM->insideAttrs.end(); ++iter )
	{
		const std::string szAttrrDefName = iter->first;
		CAttributeDefNode *pAttr = iter->second;

		if ( files.find( pAttr->GetFile() ) != files.end() )
			continue;

		VERIFY_NOLINE( insideAttrs.find( szAttrrDefName ) == insideAttrs.end(), 
			fmt::format( "{}({}): type {} redifinition, see\n\t\t {}({})",
			pNM->insideAttrs[szAttrrDefName]->GetFile(), pNM->insideAttrs[szAttrrDefName]->GetLine(),
			szAttrrDefName,
			insideAttrs[szAttrrDefName]->GetFile(), insideAttrs[szAttrrDefName]->GetLine() ),
			return );

		insideAttrs[iter->first] = iter->second;
	}

	// merge insideForwards
	for ( std::unordered_map<std::string, std::list< CObj<CTypeNode> > >::iterator iter = pNM->insideForwards.begin(); iter != pNM->insideForwards.end(); ++iter )
	{
		const std::string szForwardName = iter->first;
		std::list< CObj<CTypeNode> > &forwList = iter->second;
		VERIFY( !forwList.empty(), "empty forwards list", return );

		if ( insideForwards.find( szForwardName ) != insideForwards.end() )
		{
			std::list< CObj<CTypeNode> > &ourForwList = insideForwards[szForwardName];
			VERIFY_NOLINE( !ourForwList.empty(), "empty forwards list", return );

			CTypeNode *pType = forwList.front();
			CTypeNode *pOurType = forwList.front();

			VERIFY_NOLINE( IsEqualDefs( pType, pOurType ),
											fmt::format( "{}({}): type {} redifinition, see {}({})",
											pType->GetFile(), pType->GetLine(), pType->GetName(),
											pOurType->GetFile(), pOurType->GetLine() ),
											return );
		}

		std::list< CObj<CTypeNode> > &ourForwList = insideForwards[szForwardName];
		std::list< CObj<CTypeNode> > forwToMerge;
		for ( std::list< CObj<CTypeNode> >::iterator forwardIter = forwList.begin(); forwardIter != forwList.end(); ++forwardIter )
		{
			CTypeNode *pNode = *forwardIter;
			if ( files.find( pNode->GetFile() ) == files.end() )
				forwToMerge.push_back( pNode );
		}
		ourForwList.insert( ourForwList.begin(), forwToMerge.begin(), forwToMerge.end() );
	}

	files.insert( szFileName );
	for ( std::unordered_set<std::string>::iterator iter = pNM->files.begin(); iter != pNM->files.end(); ++iter )
		files.insert( *iter );
}

void CNamespace::ResolveForwards()
{
	for ( TForwards::iterator iter = insideForwards.begin(); iter != insideForwards.end(); ++iter )
	{
		const std::string &szName = iter->first;
		std::list< CObj<CTypeNode> > &forwards = iter->second;
		NI_VERIFY( !forwards.empty(), "empty forwards", return )

		CTypeNode *pForwardNode = forwards.front();
		CLangNode *pRawRealType = FindInsideDef( szName );

		VERIFY_NOLINE( pRawRealType != 0,
			fmt::format( "{}({}) error: can't find corresponding type for forward {}",
			pForwardNode->GetFile(), pForwardNode->GetLine(), szName ), return );

		CTypeNode *pForward = forwards.front();
		CDynamicCast<CTypeNode> pRealType = pRawRealType;
		VERIFY_NOLINE( pRealType != 0,
			fmt::format( "{}({}) error: {} type redifinition, see {}({})",
			pForwardNode->GetFile(), pForwardNode->GetLine(), szName,
			pRealType->GetFile(), pRealType->GetLine() ),
			return );

		for ( std::list< CObj<CTypeNode> >::iterator iterList = forwards.begin(); iterList != forwards.end(); ++iterList )
		{
			CTypeNode *pForwardNode = *iterList;
			pForwardNode->SetRealType( pRealType );
		}
	}
}

CEnumEntryNode* CNamespace::FindEnumEntry( const std::string &szEnumEntry ) const
{
	for ( std::unordered_map<std::string, CObj<CLangNode> >::const_iterator iter = insideDefs.begin(); iter != insideDefs.end(); ++iter )
	{
		const std::string &szDefName = iter->first;
		CLangNode *pDef = iter->second;

		CDynamicCast<CEnumNode> pEnumNode = pDef;
		if ( pEnumNode )
		{
			CEnumEntryNode *pEntry = pEnumNode->GetEnumEntry( szEnumEntry );
			if ( pEntry )
				return pEntry;
		}
	}

	return 0;
}

//*******************************************************************
//*												CTypeNode																	*
//*******************************************************************

void CTypeNode::SetRealType( CTypeNode *pRealType )
{
	NI_VERIFY( bForward, "can't set real type, cause it isn't a forward", return );
	NI_VERIFY( pRealTypeIfForward == 0, "double set of real type", return );

	pRealTypeIfForward = pRealType;
}

//*******************************************************************
//*                     CTypeDefNode                                *
//*******************************************************************

CTypeDefNode::TAttrIter CTypeDefNode::AttrBegin() const
{
	return SListOperations<CAttributeNode>().Begin( pAttrList );
}

CTypeDefNode::TAttrIter CTypeDefNode::AttrEnd() const
{
	return SListOperations<CAttributeNode>().End( pAttrList );
}

CTypeNode* CTypeDefNode::GetReferencedType( bool bRecursive ) const
{ 
	if ( !bRecursive )
		return pReferencedType;
	else
	{
		NI_VERIFY( pReferencedType != 0, "null referenced type", return 0 );
		
		CDynamicCast<CTypeDefNode> pType = pReferencedType;
		return pType ? pType->GetReferencedType( true ) : pReferencedType.GetPtr();
	}
}

bool CTypeDefNode::IsPointer() const
{
	CDynamicCast<CTypeDefNode> pType = pReferencedType;
	if ( !pType )
		return bPointer;
	else
	{
		bool bRefPointer = pType->IsPointer();

		VERIFY_NOLINE( !(bPointer && bRefPointer), fmt::format( "{}({}): pointer to pointer", GetFile(), GetLine() ), return true );
		return bPointer || bRefPointer;
	}
}

//*******************************************************************
//*                    CComplexTypeNode                             *
//*******************************************************************

CComplexTypeNode::TAttrIter CComplexTypeNode::AttrBegin() const
{
	return SListOperations<CAttributeNode>().Begin( pAttrList );
}

CComplexTypeNode::TAttrIter CComplexTypeNode::AttrEnd() const
{
	return SListOperations<CAttributeNode>().End( pAttrList );
}

CComplexTypeNode::TParentsIter CComplexTypeNode::ParentsBegin() const
{
	return SListOperations<CComplexTypeNode>().Begin( pParentsList );
}

CComplexTypeNode::TParentsIter CComplexTypeNode::ParentsEnd() const
{
	return SListOperations<CComplexTypeNode>().End( pParentsList );
}

//*******************************************************************
//*                     CVariable                                   *
//*******************************************************************

CVariable::TAttrIter CVariable::AttrBegin() const
{
	return SListOperations<CAttributeNode>().Begin( pAttrList );
}

CVariable::TAttrIter CVariable::AttrEnd() const
{
	return SListOperations<CAttributeNode>().End( pAttrList );
}

//*******************************************************************
//*                     CEnumNode                                   *
//*******************************************************************

CEnumNode::TEntriesIter CEnumNode::EntriesBegin() const
{ 
	return SListOperations<CEnumEntryNode>().Begin( pEntriesList );
}

CEnumNode::TEntriesIter CEnumNode::EntriesEnd() const
{ 
	return SListOperations<CEnumEntryNode>().End( pEntriesList );
}

CEnumNode::TAttrIter CEnumNode::AttrBegin() const
{ 
	return SListOperations<CAttributeNode>().Begin( pAttrList );
}

CEnumNode::TAttrIter CEnumNode::AttrEnd() const
{
	return SListOperations<CAttributeNode>().End( pAttrList );
}

CEnumEntryNode* CEnumNode::GetEnumEntry( const std::string &szEntryName ) const
{
	const std::list< CPtr<CEnumEntryNode> > &entries = pEntriesList->GetNodes();
	for ( std::list< CPtr<CEnumEntryNode> >::const_iterator iter = entries.begin(); iter != entries.end(); ++iter )
	{
		CEnumEntryNode *pEntry = *iter;
		if ( pEntry->GetName() == szEntryName )
			return pEntry;
	}

	return 0;
}

CEnumEntryNode* CEnumNode::FindAnyWithCrossedEntries( CNamespace *pNM ) const
{
	const std::list< CPtr<CEnumEntryNode> > &entries = pEntriesList->GetNodes();
	for ( std::list< CPtr<CEnumEntryNode> >::const_iterator iter = entries.begin(); iter != entries.end(); ++iter )
	{
		CEnumEntryNode *pEntry = *iter;
		CEnumEntryNode *pEntryInNM = pNM->FindEnumEntry( pEntry->GetName() );
		if ( pEntryInNM )
			return pEntryInNM;
	}

	return 0;
}

}

using namespace NLang;
BASIC_REGISTER_CLASS( PARSER, CTypeNode );
BASIC_REGISTER_CLASS( PARSER, CAttributeDefNode );
BASIC_REGISTER_CLASS( PARSER, CBaseTypeNode );
BASIC_REGISTER_CLASS( PARSER, CEnumEntryNode );
BASIC_REGISTER_CLASS( PARSER, CEnumNode );
BASIC_REGISTER_CLASS( PARSER, CNamespace );
BASIC_REGISTER_CLASS( PARSER, CAttributeNode );
BASIC_REGISTER_CLASS( PARSER, CTypeDefNode );
BASIC_REGISTER_CLASS( PARSER, CComplexTypeNode );
BASIC_REGISTER_CLASS( PARSER, CVariableNode );
BASIC_REGISTER_CLASS( PARSER, CVectorNode );

