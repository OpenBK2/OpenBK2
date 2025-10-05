#include "stdafx.h"

#include "CodeGenField.h"
#include "CodeGenMisc.h"
#include "Hungarian.h"
#include "StrStream.h"
#include "Parser/LangNodesDefinitions.h"

namespace NCodeGen
{

CFieldDefinition::CFieldDefinition( NLang::CVariable *pVarNode, const CNodes2TypeDefs &nodes2TypeDefs )
{
	CNodes2TypeDefs::const_iterator iterNodes = nodes2TypeDefs.find( pVarNode );
	NI_VERIFY( iterNodes != nodes2TypeDefs.end(), StrFmt( "can't find typedef for node %s", pVarNode->GetName().c_str() ), return );
	pType = GetRealType( iterNodes->second );
	NI_ASSERT( pType != 0, "null type" );

	NHungarian::ConvertToShortName( &szFieldName, pVarNode->GetName(), pType->eType, pType->GetAttributes() );
}

void CFieldDefinition::GenerateCode( SCodeStreams *pCode, const std::string &szTabs, NDb::NTypeDef::STypeDef *pParentType, const std::string &szQualifiedName )
{
	NI_VERIFY( pParentType != 0, "Wrong field definition", return );

	CDynamicCast<NDb::NTypeDef::STypeStructBase> pStruct = pParentType;
	NI_VERIFY( pStruct != 0, "Wrong field definition", return );

	int nField = 0;
	while ( nField < pStruct->fields.size() )
	{
		const NDb::NTypeDef::STypeStructBase::SField &field = pStruct->fields[nField];
		if ( field.szName == szFieldName && field.pType == pType )
			break;

		++nField;
	}
	NI_VERIFY( nField < pStruct->fields.size(), StrFmt( "can't find field %s of struct %s", szFieldName.c_str(), pStruct->GetTypeName() ), return );

	const NDb::NTypeDef::STypeStructBase::SField &field = pStruct->fields[nField];
	NDb::NTypeDef::STypeDef *pVarType = field.pType;

	if ( IsNoCode( field ) )
		return;

	pCode->h << szTabs;
	int nBrackets = 0;
	if ( pType->eType == NDb::NTypeDef::TYPE_TYPE_ARRAY )
	{
		pCode->h << "vector< ";
		++nBrackets;
		pVarType = dynamic_cast<NDb::NTypeDef::STypeArray*>( pVarType )->field.pType;
	}

	NI_ASSERT( pVarType->eType != NDb::NTypeDef::TYPE_TYPE_CLASS,
						 StrFmt( "variable %s is not-reference ty type %s", field.szName.c_str(), pVarType->GetTypeName() ) );

	if ( pVarType->eType == NDb::NTypeDef::TYPE_TYPE_REF )
	{
		pCode->h << "CDBPtr< ";
		++nBrackets;
	}

	const std::string szTypeName = NHungarian::GetTypeNameInCode( pVarType, &field );
	NI_ASSERT( szTypeName != "", "empty type name" );

	pCode->h << szTypeName;
	while ( nBrackets != 0 )
	{
		pCode->h << " " << ">";
		--nBrackets;
	}

	const std::string szFieldNameInCode = NHungarian::GetFieldNameInCode( field );
	pCode->h << " " << szFieldNameInCode << ";" << endl;
}

}

using namespace NCodeGen;
REGISTER_SAVELOAD_CLASS( 0x301B6D03, CFieldDefinition );

