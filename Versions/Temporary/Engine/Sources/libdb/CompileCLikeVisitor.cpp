#include "stdafx.h"

#include "CompileCLikeVisitor.h"
#include "Hungarian.h"
#include "TypeDefType.h"
#include "CodeGenMisc.h"
#include "Parser/ErrorsAndMessages.h"
#include "Parser/LangNodesDefinitions.h"
#include "Parser/FileNode.h"

#include <cstdint>
#include <memory>

namespace NCompileCLike
{

struct SSimpleType
{
	ObjectFactoryNewFunc pfnFunc;

	SSimpleType() : pfnFunc( 0 ) { }
	SSimpleType( ObjectFactoryNewFunc _pfnFunc )
		: pfnFunc( _pfnFunc ) { }
};

CObjectBase* NewResource()
{
	NDb::NTypeDef::STypeClass *pResource = new NDb::NTypeDef::STypeClass( "Resource" );
	pResource->pAttributes = new NDb::NTypeDef::SAttributes();
	pResource->pAttributes->attributes["noHeader"] = CVariant();

	return pResource;
}

static std::unordered_map< std::string, SSimpleType> simpleTypes;
struct SSimpleTypesInit
{
	SSimpleTypesInit()
	{
		simpleTypes["int"]				= SSimpleType( NDb::NTypeDef::STypeInt::NewSTypeInt );
		simpleTypes["float"]			= SSimpleType( NDb::NTypeDef::STypeFloat::NewSTypeFloat );
		simpleTypes["uint32_t"]			= SSimpleType( NDb::NTypeDef::STypeInt::NewSTypeInt );
		simpleTypes["uint16_t"]				= SSimpleType( NDb::NTypeDef::STypeInt::NewSTypeInt );
		simpleTypes["string"]			= SSimpleType( NDb::NTypeDef::STypeString::NewSTypeString );
		simpleTypes["bool"]				= SSimpleType( NDb::NTypeDef::STypeBool::NewSTypeBool );
		simpleTypes["hexbinary"]	= SSimpleType( NDb::NTypeDef::STypeBinary::NewSTypeBinary );
		simpleTypes["void"]				= SSimpleType( NewResource );
		simpleTypes["wstring"]		= SSimpleType( NDb::NTypeDef::STypeWString::NewSTypeWString );
		simpleTypes["GUID"]				= SSimpleType( NDb::NTypeDef::STypeGUID::NewSTypeGUID );
	}
} simpleTypesInit;

template<class T>
void ClearStruct( T *pT )
{
	pT->~T();
	new (pT) T();
}

CVisitor::CVisitor( std::vector< CObj<NDb::NTypeDef::STypeDef> > *_pTypes, NDb::NTypeDef::CTerminalTypesDescriptor *_pTermTypesDesc )
: pTypes( _pTypes ), pTermTypesDesc( _pTermTypesDesc ), bFailed( false )
{
}

void CVisitor::Visit( NLang::CBaseTypeNode *pBaseTypeNode )
{
	pCreatedType = 0;

	if ( nodes2TypeDefs.find( pBaseTypeNode ) != nodes2TypeDefs.end() )
		pCreatedType = nodes2TypeDefs[pBaseTypeNode];
	else
	{
		std::unordered_map< std::string, SSimpleType>::iterator iter = simpleTypes.find( pBaseTypeNode->GetName() );
		NI_VERIFY( iter != simpleTypes.end(), StrFmt( "can't find corresponding type for \"%s\"", pBaseTypeNode->GetName().c_str() ), return );
		SSimpleType &simpleType = iter->second;
		pCreatedType = CastToUserObject( simpleType.pfnFunc(), (NDb::NTypeDef::STypeDef*)0 );
		nodes2TypeDefs[pBaseTypeNode] = pCreatedType;
	}
}

void CVisitor::ParseImportantStructBaseAttr( NDb::NTypeDef::STypeStructBase *pStruct )
{
	CDynamicCast<NDb::NTypeDef::STypeClass> pClass = pStruct;
	if ( pStruct->pAttributes )
	{
		std::unordered_map<std::string, CVariant> &attr = pStruct->pAttributes->attributes;
		if ( attr.find( "typeID" ) != attr.end() )
		{
			const int nTypeID = attr["typeID"];
			if ( pClass )
			{
				pClass->nClassTypeID = nTypeID;
			}
			else
				NErrors::ShowWarningNoLine( StrFmt( "warning: attribute \"typeID\" in struct %s found, skipping", 
																						pStruct->szTypeName.c_str() ) );

			attr.erase( "typeID" );
			attr.erase( "comments" );
		}

		attr.erase( "noKey" );

		if ( attr.empty() )
			pStruct->pAttributes = 0;
	}
}

void CVisitor::Visit( NLang::CComplexTypeNode *pComplexTypeNode )
{
	if ( pComplexTypeNode->IsForward() )
		pComplexTypeNode->GetRealType()->Visit( this );
	else if ( nodes2TypeDefs.find( pComplexTypeNode ) != nodes2TypeDefs.end() )
		pCreatedType = nodes2TypeDefs[pComplexTypeNode];
	else
	{
		CPtr<NDb::NTypeDef::STypeStructBase> pStruct;
		if ( pComplexTypeNode->IsClass() )
		{
			pStruct = new NDb::NTypeDef::STypeClass( pComplexTypeNode->GetName() );
			if ( pStruct->szTypeName[0] == 'S' )
				pStruct->szTypeName[0] = 'C';

			if ( pStruct->szTypeName[0] != 'C' )
				NErrors::ShowWarningNoLine( StrFmt( "%s(%d) warning: wrong class %s, prefix \"C\" expected", 
																							pComplexTypeNode->GetFile().c_str(), pComplexTypeNode->GetLine(), pStruct->szTypeName.c_str() ) );
			else if ( pStruct->szTypeName.size() == 1 )
				NErrors::ShowWarningNoLine( StrFmt( "%s(%d) warning: wrong class %s name, no symbols after standart prefix", 
																							pComplexTypeNode->GetFile().c_str(), pComplexTypeNode->GetLine(), pStruct->szTypeName.c_str() ) );
			else
				pStruct->szTypeName.erase( 0, 1 );
		}
		else
		{
			pStruct = new NDb::NTypeDef::STypeStruct( pComplexTypeNode->GetName() );

			if ( pStruct->szTypeName[0] == 'C' )
				pStruct->szTypeName[0] = 'S';

			if ( pStruct->szTypeName[0] != 'S' )
				NErrors::ShowWarningNoLine( StrFmt( "%s(%d) warning: wrong struct %s, prefix \"S\" expected", 
																						pComplexTypeNode->GetFile().c_str(), pComplexTypeNode->GetLine(), pStruct->szTypeName.c_str() ) );
			else if ( pStruct->szTypeName.size() == 1 )
				NErrors::ShowWarningNoLine( StrFmt( "%s(%d) warning: wrong struct %s name, no symbols after standart prefix",
																						pComplexTypeNode->GetFile().c_str(), pComplexTypeNode->GetLine(), pStruct->szTypeName.c_str() ) );
			else
				pStruct->szTypeName.erase( 0, 1 );
		}

		pCreatedType = pStruct;
		nodes2TypeDefs[pComplexTypeNode] = pCreatedType;

		if ( pComplexTypeNode->AttrBegin() != pComplexTypeNode->AttrEnd() )
		{
			pStruct->pAttributes = new NDb::NTypeDef::SAttributes();
			FillAttributes( pStruct->pAttributes, pComplexTypeNode->AttrBegin(), pComplexTypeNode->AttrEnd() );
		}
		ParseImportantStructBaseAttr( pStruct );

		if ( pComplexTypeNode->ParentsBegin() != pComplexTypeNode->ParentsEnd() )
		{
			// multiply inheritance isn't supported
			NLang::CComplexTypeNode *pParent = *(pComplexTypeNode->ParentsBegin());
			pParent->Visit( this );
			pStruct->pBaseType = CDynamicCast<NDb::NTypeDef::STypeStructBase>( pCreatedType );
			pTermTypesDesc->SetTypeToNonTerminal( pCreatedType );
		}

		if ( NLang::CNamespace *pNM = pComplexTypeNode->GetNamespace() )
		{
			namespaces.push_back( pStruct );
			pNM->Visit( this );
			namespaces.pop_back();
		}

		pCreatedType = pStruct;
	}
}

CVariant Val2Variant( const NLang::CSimpleValue &value )
{
	switch( value.GetType() )
	{
		case NLang::EST_UNKNOWN:		return CVariant( "" );
		case NLang::EST_NOTYPE:			return CVariant( "" );
		case NLang::EST_STRING:
			{
				const std::string szValue = value.GetString();

				std::wstring wszValue;
				NStr::ToUnicode( &wszValue, szValue );
				std::string szUTFValue;
				NStr::UnicodeToUTF8( &szUTFValue, wszValue );

				return CVariant( szUTFValue );
			}
		case NLang::EST_HEXBINARY:
					return CVariant( value.GetHexBinary() );
		case NLang::EST_BOOL:				return CVariant( value.GetBool() );
		case NLang::EST_INT:				return CVariant( value.GetInt() );
		case NLang::EST_FLOAT:			return CVariant( value.GetFloat() );
		case NLang::EST_WORD:				return CVariant( value.GetWORD() );
		case NLang::EST_DWORD:			return CVariant( value.GetDWORD() );
		case NLang::EST_ENUM:				return CVariant( value.GetEnum() );
		case NLang::EST_WSTRING:
			{
				const std::string szValue = value.GetString();
				std::wstring wszValue;
				NStr::ToUnicode( &wszValue, szValue );
				return CVariant( wszValue );
			}
		default:
			NI_VERIFY( false, StrFmt( "Unknown type %d", (int)value.GetType() ), return CVariant( "" ) );
	}

	return CVariant( "" );
}

void CVisitor::Visit( NLang::CAttributeNode *pAttributeNode )
{
	attr = Val2Variant( pAttributeNode->GetValue() );
}

void CVisitor::Visit( NLang::CTypeDefNode *pTypeDefNode )
{
	pCreatedType = 0;
	if ( nodes2TypeDefs.find( pTypeDefNode ) != nodes2TypeDefs.end() )
	{
		pCreatedType = nodes2TypeDefs[pTypeDefNode];
		return;
	}
	else
	{
		NLang::CTypeNode *pRefType = pTypeDefNode->GetReferencedType( false );
		pRefType->Visit( this );

		CPtr<NDb::NTypeDef::STypedefType> pTypedef = new NDb::NTypeDef::STypedefType( pTypeDefNode->GetName() );
		if ( CDynamicCast<NDb::NTypeDef::STypedefType> pCreatedTypeDef = pCreatedType )
		{
			NI_ASSERT( pCreatedTypeDef->pRealType != 0, "NULL type" );
			pTypedef->pRealType = pCreatedTypeDef->pRealType;
			if ( pCreatedTypeDef->pAttributes )
				pTypedef->pAttributes = new NDb::NTypeDef::SAttributes( pCreatedTypeDef->pAttributes->attributes );
		}
		else
		{
			NI_ASSERT( pCreatedType != 0, "NULL type" );
			pTypedef->pRealType = pCreatedType;
		}

		if ( pTypeDefNode->AttrBegin() != pTypeDefNode->AttrEnd() )
		{
			if ( pTypedef->pAttributes == 0 )
				pTypedef->pAttributes = new NDb::NTypeDef::SAttributes();
			FillAttributes( pTypedef->pAttributes, pTypeDefNode->AttrBegin(), pTypeDefNode->AttrEnd() );
		}

		if ( pTypedef->pRealType->eType == NDb::NTypeDef::TYPE_TYPE_BINARY )
		{
			CPtr<NDb::NTypeDef::STypeBinary> pBinaryType = new NDb::NTypeDef::STypeBinary( pTypedef->GetTypeName() );
			if ( pTypedef->pAttributes )
				pBinaryType->pAttributes = new NDb::NTypeDef::SAttributes( pTypedef->pAttributes->attributes );

			if ( pBinaryType->pAttributes != 0 )
			{
				std::unordered_map<std::string, CVariant> &attr = pTypedef->pAttributes->attributes;
				if ( attr.find( "numBytes" ) != attr.end() )
					pBinaryType->nBinaryObjectSize = attr["numBytes"];

				attr.erase( "numBytes" );
			}

			nodes2TypeDefs[pTypeDefNode] = pBinaryType;
			pCreatedType = pBinaryType;
		}
		else
		{
			pTermTypesDesc->SetTypeToNonTerminal( pTypedef );
			nodes2TypeDefs[pTypeDefNode] = pTypedef;
			pCreatedType = pTypedef;
		}
	}
}

void CVisitor::Visit( NLang::CEnumEntryNode *pEnumEntryNode )
{
	ClearStruct( &enumEntry );

	enumEntry.szName = pEnumEntryNode->GetName();
	enumEntry.nVal = pEnumEntryNode->GetValue();
}

void CVisitor::Visit( NLang::CEnumNode *pEnumNode )
{
	if ( pEnumNode->IsForward() )
		pEnumNode->GetRealType()->Visit( this );
	else
	{
		pCreatedType = 0;
		if ( nodes2TypeDefs.find( pEnumNode ) != nodes2TypeDefs.end() )
			pCreatedType = nodes2TypeDefs[pEnumNode];
		else
		{
			CPtr<NDb::NTypeDef::STypeEnum> pTypeEnum = new NDb::NTypeDef::STypeEnum( pEnumNode->GetName() );
			if ( pTypeEnum->szTypeName[0] != 'E' )
				NErrors::ShowWarningNoLine( StrFmt( "%s(%d) warning: wrong enum %s, prefix \"E\" expected", 
																						pEnumNode->GetFile().c_str(), pEnumNode->GetLine(), pTypeEnum->szTypeName.c_str() ) );
			else if ( pTypeEnum->szTypeName.size() == 1 )
				NErrors::ShowWarningNoLine( StrFmt( "%s(%d) warning: wrong enum %s name, no symbols after standart prefix",
																						pEnumNode->GetFile().c_str(), pEnumNode->GetLine(), pTypeEnum->szTypeName.c_str() ) );
			else
				pTypeEnum->szTypeName.erase( 0, 1 );

			pCreatedType = pTypeEnum;
			nodes2TypeDefs[pEnumNode] = pCreatedType;

			if ( pEnumNode->AttrBegin() != pEnumNode->AttrEnd() )
			{
				pTypeEnum->pAttributes = new NDb::NTypeDef::SAttributes();
				FillAttributes( pTypeEnum->pAttributes, pEnumNode->AttrBegin(), pEnumNode->AttrEnd() );
			}

			for ( NLang::CEnumNode::TEntriesIter	iter = pEnumNode->EntriesBegin(); iter != pEnumNode->EntriesEnd(); ++iter )
			{
				NLang::CEnumEntryNode *pEntry = *iter;
				pEntry->Visit( this );
				pTypeEnum->entries.push_back( enumEntry );
			}
		}
	}
}

static void ParseImportantFieldAttr( NDb::NTypeDef::STypeStructBase::SField *pField )
{
	if ( pField->pAttributes != 0 )
	{
		std::unordered_map<std::string, CVariant> &attr = pField->pAttributes->attributes;
		if ( attr.find( "comments" ) != attr.end() )
		{
			const std::string szDesc = attr["comments"].GetStr();
			NStr::UTF8ToUnicode( &(pField->wszDesc), szDesc );
			attr.erase( "comments" );
		}

		if ( attr.empty() )
			pField->pAttributes = 0;
	}
}

void CVisitor::VisitVariable( NLang::CVariable *pVariableNode, const bool bArray )
{
	NLang::CTypeNode *pType = pVariableNode->GetType()->GetRealType();
	pType->Visit( this );

	ClearStruct( &field );
	pFromFieldTypeDefAttr = 0;
	if ( CDynamicCast<NDb::NTypeDef::STypedefType> pTypeDefCreated = pCreatedType )
	{
		field.pType = pTypeDefCreated->pRealType;
		NI_ASSERT( field.pType != 0, "null type" );
		
		if ( pTypeDefCreated->pAttributes )
		{
			if ( bArray )
				pFromFieldTypeDefAttr = new NDb::NTypeDef::SAttributes( pTypeDefCreated->pAttributes->attributes );
			else
				field.pAttributes = new NDb::NTypeDef::SAttributes( pTypeDefCreated->pAttributes->attributes );
		}
	}
	else
	{
		field.pType = pCreatedType;
		NI_ASSERT( field.pType != 0, "null type" );
	}

	if ( field.pType->eType == NDb::NTypeDef::TYPE_TYPE_CLASS )
	{
		if ( classes2Refs.find( field.pType ) == classes2Refs.end() )
		{
			CPtr<NDb::NTypeDef::STypeRef> pRefType = new NDb::NTypeDef::STypeRef( field.pType );
			classes2Refs[field.pType] = pRefType;
			field.pType = pRefType;
		}
		else
			field.pType = classes2Refs[field.pType];
	}

	if ( pVariableNode->AttrBegin() != pVariableNode->AttrEnd() )
	{
		if ( !field.pAttributes )
			field.pAttributes = new NDb::NTypeDef::SAttributes();
		FillAttributes( field.pAttributes, pVariableNode->AttrBegin(), pVariableNode->AttrEnd() );
	}

	if ( pVariableNode->HasDefault() )
		field.defaultValue = Val2Variant( pVariableNode->GetDefault() );

	if ( pVariableNode->HasComplexDefault() )
		field.complexDefaultValue = CVariant( pVariableNode->GetComplexDefault() );

	ParseImportantFieldAttr( &field );
}

void CVisitor::Visit( NLang::CVariableNode *pVariableNode )
{
	ClearStruct( &field );
	VisitVariable( pVariableNode, false );

	const bool bCorrect = NHungarian::ConvertToShortName( &(field.szName), pVariableNode->GetName(), field.pType->eType, field.pType->GetAttributes() );
	if ( !bCorrect )
	{
//		NLang::CTypeNode *pType = pVariableNode->GetType();
//		NErrors::ShowWarningNoLine( StrFmt( "%s(%d) warning: wrong variable \"%s\" of type \"%s\" name", pType->GetFile().c_str(), pType->GetLine(), field.szName.c_str(), field.pType->GetTypeName() ) );
	}

	nodes2TypeDefs[pVariableNode] = field.pType;
}

void CVisitor::Visit( NLang::CVectorNode *pVectorNode )
{
	ClearStruct( &field );
	VisitVariable( pVectorNode, true );

	const bool bCorrect = NHungarian::ConvertToShortName( &(field.szName), pVectorNode->GetName(), NDb::NTypeDef::TYPE_TYPE_ARRAY, 0 );
	if ( !bCorrect )
	{
//		NLang::CTypeNode *pType = pVectorNode->GetType();
//		NErrors::ShowWarningNoLine( StrFmt( "%s(%d) warning: wrong variable \"%s\" of type \"%s\" name", pType->GetFile().c_str(), pType->GetLine(), field.szName.c_str(), field.pType->GetTypeName() ) );
	}

	CPtr<NDb::NTypeDef::STypeArray> pArrayType = new NDb::NTypeDef::STypeArray( field.pType );
	pArrayType->field.pAttributes = pFromFieldTypeDefAttr;
	field.pType = pArrayType;

	if ( pVectorNode->GetMinAmount() != 0 || pVectorNode->GetMaxAmount() != -1 )
		field.constraints.push_back( new NDb::NTypeDef::SConstraintsArrayMinMax( pVectorNode->GetMinAmount(), pVectorNode->GetMaxAmount() ) );

	nodes2TypeDefs[pVectorNode] = field.pType;
}

void CVisitor::Visit( NLang::CAttributeDefNode *pAttrDefNode )
{
	// do nothing
}

void CVisitor::NamespaceNodeVisited( NLang::CLangNode *pNode, NDb::NTypeDef::STypeStructBase *pUpperType )
{
	if ( CDynamicCast<NLang::CAttributeDefNode> pAttrDefNode = pNode )
	{
		// do nothing
	}
	else if ( CDynamicCast<NLang::CBaseTypeNode> pBaseTypeNode = pNode )
	{
		// do nothing
	}
	else if ( CDynamicCast<NLang::CTypeDefNode> pTypeDefNode = pNode )
	{
		// do nothing
	}
	else if ( CDynamicCast<NLang::CTypeNode> pComplexTypeNode = pNode )
	{
		pUpperType->nestedTypes.push_back( pCreatedType.GetPtr() );
	}
	else if ( CDynamicCast<NLang::CEnumNode> pEnumNode = pNode )
		pUpperType->nestedTypes.push_back( pCreatedType.GetPtr() );
	else if ( CDynamicCast<NLang::CVariable> pVariableNode = pNode )
	{
		// FIX ME! crapped chunkids generation
		if ( pUpperType->fields.empty() )
			field.nChunkID = 2;
		else
		{
			field.nChunkID = pUpperType->fields.back().nChunkID;
			if ( !NCodeGen::IsNoCode( field ) )
				++field.nChunkID;
		}

		pUpperType->fields.push_back( field );
	}
	else
	{
		NI_ASSERT( false, StrFmt( "can't recognize node %s", typeid( *pNode ).name() ) );
	}
}

void CVisitor::Visit( NLang::CNamespace *pNM )
{
	if ( namespaces.empty() )
		namespaces.push_back( new NDb::NTypeDef::STypeStruct() );
	NDb::NTypeDef::STypeStructBase *pUpperType = namespaces.back();

	if ( namespaces.size() == 1 )
	{
		for ( NLang::CNamespace::TLangNodeIter iter = pNM->DefsBegin(); iter != pNM->DefsEnd(); ++iter )
		{
			NLang::CLangNode *pNode = iter->second;
			pNode->Visit( this );
			NamespaceNodeVisited( pNode, pUpperType );
		}
	}
	else
	{
		for ( NLang::CNamespace::TDefsListIter iter = pNM->DefsListBegin(); iter != pNM->DefsListEnd(); ++iter )
		{
			NLang::CLangNode *pNode = *iter;
			pNode->Visit( this );
			NamespaceNodeVisited( pNode, pUpperType );
		}
	}

	if ( namespaces.size() == 1 )
	{
		NDb::NTypeDef::STypeStructBase *pRootNM = namespaces.back();

		pTypes->reserve( pRootNM->nestedTypes.size() );
		for ( NDb::NTypeDef::STypeStructBase::CNestedTypesList::iterator iter = pRootNM->nestedTypes.begin(); iter != pRootNM->nestedTypes.end(); ++iter )
		{
			NDb::NTypeDef::STypeDef *pType = *iter;
			pTypes->push_back( pType );
		}

		namespaces.pop_back();
	}
}

}


