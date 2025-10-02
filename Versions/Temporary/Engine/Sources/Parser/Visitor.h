#pragma once

namespace NLang
{
class CBaseTypeNode;
class CComplexTypeNode;
class CAttributeNode;
class CTypeDefNode;
class CEnumNode;
class CVariableNode;
class CVectorNode;
class CAttributeDefNode;
class CNamespace;
class CEnumEntryNode;

struct IVisitor : public CObjectBase
{
	virtual void Visit( CBaseTypeNode *pBaseTypeNode ) = 0;
	virtual void Visit( CComplexTypeNode *pComplexTypeNode ) = 0;
	virtual void Visit( CAttributeNode *pAttributeNode ) = 0;
	virtual void Visit( CTypeDefNode *pTypeDefNode ) = 0;
	virtual void Visit( CEnumNode *pEnumNode ) = 0;
	virtual void Visit( CEnumEntryNode *pEnumEntryNode ) = 0;
	virtual void Visit( CVariableNode *pVariableNode ) = 0;
	virtual void Visit( CVectorNode *pVectorNode ) = 0;
	virtual void Visit( CAttributeDefNode *pAttrDefNode ) = 0;
	virtual void Visit( CNamespace *pNM ) = 0;
};
}


