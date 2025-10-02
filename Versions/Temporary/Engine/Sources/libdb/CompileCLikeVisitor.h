#pragma once

#include "Nodes2TypeDefs.h"
#include "TerminalTypesDesc.h"
#include "TypeDef.h"
#include "Variant.h"
#include "../Parser/Visitor.h"

namespace NLang
{
	class CFileNode;
	class CLangNode;
	class CVariable;
	class CAttributeNode;
	class CTerminalTypesDescriptor;
}

namespace NCompileCLike
{

class CVisitor: public NLang::IVisitor
{
	OBJECT_NOCOPY_METHODS( CVisitor );

	vector< CObj<NDb::NTypeDef::STypeDef> > *pTypes;

	CPtr<NDb::NTypeDef::STypeDef> pCreatedType;
	CVariant attr;
	NDb::NTypeDef::STypeStructBase::SField field;
	NDb::NTypeDef::STypeEnum::SEnumEntry enumEntry;
	CObj<NDb::NTypeDef::SAttributes> pFromFieldTypeDefAttr;
	list< CPtr<NDb::NTypeDef::STypeStructBase> > namespaces;

	CNodes2TypeDefs nodes2TypeDefs;
	CPtr<NDb::NTypeDef::CTerminalTypesDescriptor> pTermTypesDesc;
	CClasses2Refs classes2Refs;
	bool bFailed;

	template<typename TIter>
	void FillAttributes( NDb::NTypeDef::SAttributes *pAttr, TIter begin, TIter end )
	{
		for ( TIter current = begin; current != end; ++current )
		{
			NLang::CAttributeNode *pNode = *current;
			pNode->Visit( this );
			pAttr->attributes[pNode->GetName()] = attr;
		}
	}

	//
	void VisitVariable( NLang::CVariable *pVariable, const bool bArray );
	void NamespaceNodeVisited( NLang::CLangNode *pNode, NDb::NTypeDef::STypeStructBase *pUpperType );
	void ParseImportantStructBaseAttr( NDb::NTypeDef::STypeStructBase *pStruct );
public:
	CVisitor() : pTypes( 0 ), bFailed( false ) { }
	CVisitor( vector< CObj<NDb::NTypeDef::STypeDef> > *pTypes, NDb::NTypeDef::CTerminalTypesDescriptor *pTermTypesDesc );

	virtual void Visit( NLang::CBaseTypeNode *pBaseTypeNode );
	virtual void Visit( NLang::CComplexTypeNode *pComplexTypeNode );
	virtual void Visit( NLang::CAttributeNode *pAttributeNode );
	virtual void Visit( NLang::CTypeDefNode *pTypeDefNode );
	virtual void Visit( NLang::CEnumNode *pEnumNode );
	virtual void Visit( NLang::CEnumEntryNode *pEnumEntryNode );
	virtual void Visit( NLang::CVariableNode *pVariableNode );
	virtual void Visit( NLang::CVectorNode *pVectorNode );
	virtual void Visit( NLang::CAttributeDefNode *pAttrDefNode );
	virtual void Visit( NLang::CNamespace *pNM );

	const CNodes2TypeDefs& GetNodes2TypeDefs() const { return nodes2TypeDefs; }

	bool IsFailed() const { return bFailed; }
};

}


