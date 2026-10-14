#pragma once

#include "Nodes2TypeDefs.h"
// FillAttributes below calls through a CAttributeNode, so this header needs the
// definition and not just Visitor.h's forward declaration.
#include "Parser/LangNodesDefinitions.h"
#include "Parser/Visitor.h"
#include "TerminalTypesDesc.h"
#include "TypeDef.h"
#include "Variant.h"

namespace NLang
{
class CFileNode;
class CLangNode;
class CVariable;
class CTerminalTypesDescriptor;
} // namespace NLang

namespace NCompileCLike
{

class CVisitor : public NLang::IVisitor
{
	OBJECT_NOCOPY_METHODS( CVisitor );

	std::vector<CObj<NDb::NTypeDef::STypeDef>> *pTypes;

	CPtr<NDb::NTypeDef::STypeDef> pCreatedType;
	CVariant attr;
	NDb::NTypeDef::STypeStructBase::SField field;
	NDb::NTypeDef::STypeEnum::SEnumEntry enumEntry;
	CObj<NDb::NTypeDef::SAttributes> pFromFieldTypeDefAttr;
	std::list<CPtr<NDb::NTypeDef::STypeStructBase>> namespaces;

	CNodes2TypeDefs nodes2TypeDefs;
	CPtr<NDb::NTypeDef::CTerminalTypesDescriptor> pTermTypesDesc;
	CClasses2Refs classes2Refs;
	bool bFailed;

	template <typename TIter>
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
	CVisitor()
		: pTypes( 0 )
		, bFailed( false )
	{
	}

	CVisitor( std::vector<CObj<NDb::NTypeDef::STypeDef>> *pTypes,
	          NDb::NTypeDef::CTerminalTypesDescriptor *pTermTypesDesc );

	void Visit( NLang::CBaseTypeNode *pBaseTypeNode ) override;
	void Visit( NLang::CComplexTypeNode *pComplexTypeNode ) override;
	void Visit( NLang::CAttributeNode *pAttributeNode ) override;
	void Visit( NLang::CTypeDefNode *pTypeDefNode ) override;
	void Visit( NLang::CEnumNode *pEnumNode ) override;
	void Visit( NLang::CEnumEntryNode *pEnumEntryNode ) override;
	void Visit( NLang::CVariableNode *pVariableNode ) override;
	void Visit( NLang::CVectorNode *pVectorNode ) override;
	void Visit( NLang::CAttributeDefNode *pAttrDefNode ) override;
	void Visit( NLang::CNamespace *pNM ) override;

	const CNodes2TypeDefs &GetNodes2TypeDefs() const
	{
		return nodes2TypeDefs;
	}

	bool IsFailed() const
	{
		return bFailed;
	}
};

} // namespace NCompileCLike
