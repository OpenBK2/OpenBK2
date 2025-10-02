#pragma once

#include "LangNode.h"

namespace NLang
{
	class CFileNode;

	int GetStep();
	void NextStep();
	void NullStep();

	const string GetBaseFileName();

	void AddDef( CLangNode *pNode );
	void OpenNewNamespace( CLangNode *pRawVisibleTypes );
	void CloseNamespace();

	void AddAttrToComplexTypeNode( CLangNode *pNode, CLangNode *pAttrList );
	void AddNamespaceToComplexTypeNode( CLangNode *pNode, CLangNode *pNamespace );

	CLangNode* CreateComplexTypeNode( const string &szTypeName, bool bClass );
	void AddParentsOfComplexType( CLangNode *pNode, CLangNode *pParentsList );

	CLangNode* CreateParentsList( const string &szFirstTypeName );
	void AddParentToParentsList( CLangNode *pComplexTypeList, const string &szTypeName );

	CLangNode* GetCurrentNamespace();
	void AddTypeToNamespace( CLangNode *pNamespace, CLangNode *pType );
	void AddVarListToNamespace( CLangNode *pNamespace, CLangNode *pVarList );
	void AddBadIncludeToNamespace( CLangNode *pNamespace, const string &szInclude );

	void SetTypeToVars( CLangNode *pVarListNode, const string &szTypeName );
	void SetRndTypeToVars( CLangNode *pVarListNode, const string &szTypeName );
	void SetAttrToVars( CLangNode *pVarListNode, CLangNode *pAttrListNode );

	CLangNode* CreateVarListNode( CLangNode *pVar );
	void AddVarToVarListNode( CLangNode *pVarListNode, CLangNode *pVar );
	CLangNode* CreateVar( const string &szVarName, const string &szMinAmount, const string &szMaxAmount );

	void SetEnumValueToVarNode( CLangNode *pVarNode, const string &szValue );
	void SetDefValueToVarNode( CLangNode *pVarNode, const string &szValue, bool bStringValue );
	void SetDefWStrValueToVarNode( CLangNode *pVarNode, const string &szValue );
	void SetComplexDefaultValueToVarNode( CLangNode *pVarNode, const string &szValue );
	void SetVarToPointer( CLangNode *pRawVar );

	CLangNode* CreateAttrListNode( CLangNode *pAttrNode );
	void MergeAttrList( CLangNode *pAttrListNode, CLangNode *pAttrListNode1 );
	void AddAttrEntry( CLangNode *pAttrListNode, CLangNode *pAttrNode );
	CLangNode* CreateAttrDef( const string &szAttrName, const string &szAttrValue, bool bStringValue );

	CLangNode* CreateTypeDefNode( CLangNode *pRawAttrListNode, const string &szReferencedTypeName, const string &szTypeName, bool bPointer );

	void SetNameToEnumNode( CLangNode *pEnumNode, const string &szName );
	CLangNode* CreateEnumNode( CLangNode *pEnumEntryNode );
	void AddEnumEntry( CLangNode *pEnumNode, CLangNode *pEnumEntryNode );
	void AddAttrToEnumNode( CLangNode *pEnumNode, CLangNode *pAttrList );

	CLangNode* CreateEnumEntryNode( const string &szEntryName, const string &szDefaultValue, bool bDefaultValueNumber );

	CLangNode* CreateBaseTypeNode( const string &szTypeName, bool bIsClass );

	CLangNode* CreateForwardEnumNode( const string &szEnumName );
	CLangNode* CreateForwardComplexType( const string &szTypeName, bool bIsClass );

	CLangNode* CreateAttributeDefNode( ESimpleType eType );
	void SetNameToAttrDef( CLangNode *pNode, const string &szName );

	bool IsEqualDefs( CLangNode *pNode1, CLangNode *pNode2 );
}

