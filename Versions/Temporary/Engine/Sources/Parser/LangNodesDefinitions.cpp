#include "stdafx.h"

#include "LangNodesDefinitions.h"
#include "ParseOperations.h"
#include "StringNumbers.h"
#include "ErrorsAndMessages.h"
#include "../Misc/StrProc.h"

void yyerror_no_line( char *s, ... );

namespace NLang
{
template <typename T>
struct SListOperations
{
	typedef list< CPtr<T> > TList;
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

CAttributeDefNode* CNamespace::FindInsideAttrDef( const string &szAttrDef )
{
	hash_map<string, CObj<CAttributeDefNode> >::iterator iter = insideAttrs.find( szAttrDef );
	return iter == insideAttrs.end() ? 0 : iter->second;
}

void CNamespace::AddInsideAttrDef( CAttributeDefNode *pAttrDefNode )
{
	insideAttrs[pAttrDefNode->GetName()] = pAttrDefNode;
	insideDefList.push_back( pAttrDefNode );
}

CTypeNode* CNamespace::FindForward( const string &szTypeName )
{
	hash_map<string, list< CObj<CTypeNode> > >::iterator iter = insideForwards.find( szTypeName );
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

CLangNode* CNamespace::FindInsideDef( const string &szDefName )
{
	hash_map<string, CObj<CLangNode> >::iterator iter = insideDefs.find( szDefName );
	if ( iter == insideDefs.end() )
	{
		if ( pVisibleTypes )
		{
			list< CPtr<CComplexTypeNode> > &visTypes = pVisibleTypes->GetNodes();
			for ( list< CPtr<CComplexTypeNode> >::iterator iter = visTypes.begin(); iter != visTypes.end(); ++iter )
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

void CNamespace::MergeFiles( CNamespace *pNM, const string &szFileName )
{
	if ( files.find( szFileName ) != files.end() )
		return;
	// merge insideDefs
	for ( hash_map<string, CObj<CLangNode> >::iterator iter = pNM->insideDefs.begin(); iter != pNM->insideDefs.end(); ++iter )
	{
		const string &szDefName = iter->first;
		CLangNode *pNode = iter->second;
		if ( files.find( pNode->GetFile() ) != files.end() )
			continue;

		VERIFY_NOLINE( insideDefs.find( szDefName ) == insideDefs.end(), 
			StrFmt( "%s(%d) error: type %s redifinition, see \n\t\t%s(%d):", 
			pNM->insideDefs[szDefName]->GetFile().c_str(), pNM->insideDefs[szDefName]->GetLine(), szDefName.c_str(),
			insideDefs[szDefName]->GetFile().c_str(), insideDefs[szDefName]->GetLine() ), return );

		CDynamicCast<CEnumNode> pEnum = pNode;
		if ( pEnum )
		{
			CEnumEntryNode *pWithEqualEntries = pEnum->FindAnyWithCrossedEntries( this );
			WARNING_NOLINE(	pWithEqualEntries == 0,
				StrFmt( "%s(%d) warning: enum entry \"%s\" redifinition, enum %s, see\n\t\t%s(%d):",
				pEnum->GetFile().c_str(), pEnum->GetLine(), pWithEqualEntries->GetName().c_str(), pEnum->GetName().c_str(),
				pWithEqualEntries->GetFile().c_str(), pWithEqualEntries->GetLine() ) );
		}

		insideDefs[iter->first] = iter->second;
	}

	// merge insideAttrs
	for ( hash_map<string, CObj<CAttributeDefNode> >::iterator iter = pNM->insideAttrs.begin(); iter != pNM->insideAttrs.end(); ++iter )
	{
		const string szAttrrDefName = iter->first;
		CAttributeDefNode *pAttr = iter->second;

		if ( files.find( pAttr->GetFile() ) != files.end() )
			continue;

		VERIFY_NOLINE( insideAttrs.find( szAttrrDefName ) == insideAttrs.end(), 
			StrFmt( "%s(%d): type %s redifinition, see\n\t\t %s(%d)", 
			pNM->insideAttrs[szAttrrDefName]->GetFile().c_str(), pNM->insideAttrs[szAttrrDefName]->GetLine(),
			szAttrrDefName.c_str(),
			insideAttrs[szAttrrDefName]->GetFile().c_str(), insideAttrs[szAttrrDefName]->GetLine() ),
			return );

		insideAttrs[iter->first] = iter->second;
	}

	// merge insideForwards
	for ( hash_map<string, list< CObj<CTypeNode> > >::iterator iter = pNM->insideForwards.begin(); iter != pNM->insideForwards.end(); ++iter )
	{
		const string szForwardName = iter->first;
		list< CObj<CTypeNode> > &forwList = iter->second;
		VERIFY( !forwList.empty(), "empty forwards list", return );

		if ( insideForwards.find( szForwardName ) != insideForwards.end() )
		{
			list< CObj<CTypeNode> > &ourForwList = insideForwards[szForwardName];
			VERIFY_NOLINE( !ourForwList.empty(), "empty forwards list", return );

			CTypeNode *pType = forwList.front();
			CTypeNode *pOurType = forwList.front();

			VERIFY_NOLINE( IsEqualDefs( pType, pOurType ),
											StrFmt( "%s(%d): type %s redifinition, see %s(%d)",
											pType->GetFile().c_str(), pType->GetLine(), pType->GetName().c_str(), 
											pOurType->GetFile().c_str(), pOurType->GetLine() ),
											return );
		}

		list< CObj<CTypeNode> > &ourForwList = insideForwards[szForwardName];
		list< CObj<CTypeNode> > forwToMerge;
		for ( list< CObj<CTypeNode> >::iterator forwardIter = forwList.begin(); forwardIter != forwList.end(); ++forwardIter )
		{
			CTypeNode *pNode = *forwardIter;
			if ( files.find( pNode->GetFile() ) == files.end() )
				forwToMerge.push_back( pNode );
		}
		ourForwList.insert( ourForwList.begin(), forwToMerge.begin(), forwToMerge.end() );
	}

	files.insert( szFileName );
	for ( hash_set<string>::iterator iter = pNM->files.begin(); iter != pNM->files.end(); ++iter )
		files.insert( *iter );
}

void CNamespace::ResolveForwards()
{
	for ( TForwards::iterator iter = insideForwards.begin(); iter != insideForwards.end(); ++iter )
	{
		const string &szName = iter->first;
		list< CObj<CTypeNode> > &forwards = iter->second;
		NI_VERIFY( !forwards.empty(), "empty forwards", return )

		CTypeNode *pForwardNode = forwards.front();
		CLangNode *pRawRealType = FindInsideDef( szName );

		VERIFY_NOLINE( pRawRealType != 0,
			StrFmt( "%s(%d) error: can't find corresponding type for forward %s",
			pForwardNode->GetFile().c_str(), pForwardNode->GetLine(), szName.c_str() ), return );

		CTypeNode *pForward = forwards.front();
		CDynamicCast<CTypeNode> pRealType = pRawRealType;
		VERIFY_NOLINE( pRealType != 0,
			StrFmt( "%s(%d) error: %s type redifinition, see %s(%d)", 
			pForwardNode->GetFile().c_str(), pForwardNode->GetLine(), szName.c_str(),
			pRealType->GetFile().c_str(), pRealType->GetLine() ),
			return );

		for ( list< CObj<CTypeNode> >::iterator iterList = forwards.begin(); iterList != forwards.end(); ++iterList )
		{
			CTypeNode *pForwardNode = *iterList;
			pForwardNode->SetRealType( pRealType );
		}
	}
}

CEnumEntryNode* CNamespace::FindEnumEntry( const string &szEnumEntry ) const
{
	for ( hash_map<string, CObj<CLangNode> >::const_iterator iter = insideDefs.begin(); iter != insideDefs.end(); ++iter )
	{
		const string &szDefName = iter->first;
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
		return pType ? pType->GetReferencedType( true ) : pReferencedType;
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

		VERIFY_NOLINE( !(bPointer && bRefPointer), StrFmt( "%s(%d): pointer to pointer", GetFile().c_str(), GetLine() ), return true );
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

CEnumEntryNode* CEnumNode::GetEnumEntry( const string &szEntryName ) const
{
	const list< CPtr<CEnumEntryNode> > &entries = pEntriesList->GetNodes();
	for ( list< CPtr<CEnumEntryNode> >::const_iterator iter = entries.begin(); iter != entries.end(); ++iter )
	{
		CEnumEntryNode *pEntry = *iter;
		if ( pEntry->GetName() == szEntryName )
			return pEntry;
	}

	return 0;
}

CEnumEntryNode* CEnumNode::FindAnyWithCrossedEntries( CNamespace *pNM ) const
{
	const list< CPtr<CEnumEntryNode> > &entries = pEntriesList->GetNodes();
	for ( list< CPtr<CEnumEntryNode> >::const_iterator iter = entries.begin(); iter != entries.end(); ++iter )
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
BASIC_REGISTER_CLASS( CTypeNode );
BASIC_REGISTER_CLASS( CAttributeDefNode );
BASIC_REGISTER_CLASS( CBaseTypeNode );
BASIC_REGISTER_CLASS( CEnumEntryNode );
BASIC_REGISTER_CLASS( CEnumNode );
BASIC_REGISTER_CLASS( CNamespace );
BASIC_REGISTER_CLASS( CAttributeNode );
BASIC_REGISTER_CLASS( CTypeDefNode );
BASIC_REGISTER_CLASS( CComplexTypeNode );
BASIC_REGISTER_CLASS( CVariableNode );
BASIC_REGISTER_CLASS( CVectorNode );
