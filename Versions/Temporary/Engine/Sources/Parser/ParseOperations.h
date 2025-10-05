#pragma once

#include "LangNode.h"

namespace NLang
{
	class CFileNode;

	int GetStep();
	void NextStep();
	void NullStep();

	const std::string GetBaseFileName();

	void AddDef( CLangNode *pNode );
	void OpenNewNamespace( CLangNode *pRawVisibleTypes );
	void CloseNamespace();

	void AddAttrToComplexTypeNode( CLangNode *pNode, CLangNode *pAttrList );
	void AddNamespaceToComplexTypeNode( CLangNode *pNode, CLangNode *pNamespace );

	CLangNode* CreateComplexTypeNode( const std::string &szTypeName, bool bClass );
	void AddParentsOfComplexType( CLangNode *pNode, CLangNode *pParentsList );

	CLangNode* CreateParentsList( const std::string &szFirstTypeName );
	void AddParentToParentsList( CLangNode *pComplexTypeList, const std::string &szTypeName );

	CLangNode* GetCurrentNamespace();
	void AddTypeToNamespace( CLangNode *pNamespace, CLangNode *pType );
	void AddVarListToNamespace( CLangNode *pNamespace, CLangNode *pVarList );
	void AddBadIncludeToNamespace( CLangNode *pNamespace, const std::string &szInclude );

	void SetTypeToVars( CLangNode *pVarListNode, const std::string &szTypeName );
	void SetRndTypeToVars( CLangNode *pVarListNode, const std::string &szTypeName );
	void SetAttrToVars( CLangNode *pVarListNode, CLangNode *pAttrListNode );

	CLangNode* CreateVarListNode( CLangNode *pVar );
	void AddVarToVarListNode( CLangNode *pVarListNode, CLangNode *pVar );
	CLangNode* CreateVar( const std::string &szVarName, const std::string &szMinAmount, const std::string &szMaxAmount );

	void SetEnumValueToVarNode( CLangNode *pVarNode, const std::string &szValue );
	void SetDefValueToVarNode( CLangNode *pVarNode, const std::string &szValue, bool bStringValue );
	void SetDefWStrValueToVarNode( CLangNode *pVarNode, const std::string &szValue );
	void SetComplexDefaultValueToVarNode( CLangNode *pVarNode, const std::string &szValue );
	void SetVarToPointer( CLangNode *pRawVar );

	CLangNode* CreateAttrListNode( CLangNode *pAttrNode );
	void MergeAttrList( CLangNode *pAttrListNode, CLangNode *pAttrListNode1 );
	void AddAttrEntry( CLangNode *pAttrListNode, CLangNode *pAttrNode );
	CLangNode* CreateAttrDef( const std::string &szAttrName, const std::string &szAttrValue, bool bStringValue );

	CLangNode* CreateTypeDefNode( CLangNode *pRawAttrListNode, const std::string &szReferencedTypeName, const std::string &szTypeName, bool bPointer );

	void SetNameToEnumNode( CLangNode *pEnumNode, const std::string &szName );
	CLangNode* CreateEnumNode( CLangNode *pEnumEntryNode );
	void AddEnumEntry( CLangNode *pEnumNode, CLangNode *pEnumEntryNode );
	void AddAttrToEnumNode( CLangNode *pEnumNode, CLangNode *pAttrList );

	CLangNode* CreateEnumEntryNode( const std::string &szEntryName, const std::string &szDefaultValue, bool bDefaultValueNumber );

	CLangNode* CreateBaseTypeNode( const std::string &szTypeName, bool bIsClass );

	CLangNode* CreateForwardEnumNode( const std::string &szEnumName );
	CLangNode* CreateForwardComplexType( const std::string &szTypeName, bool bIsClass );

	CLangNode* CreateAttributeDefNode( ESimpleType eType );
	void SetNameToAttrDef( CLangNode *pNode, const std::string &szName );

	bool IsEqualDefs( CLangNode *pNode1, CLangNode *pNode2 );
}


