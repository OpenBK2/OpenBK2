#include "stdafx.h"

#include "CodeGenMisc.h"
#include "CodeGenNamespace.h"
#include "CodeGenType.h"
#include "Hungarian.h"
#include "StrStream.h"
#include "TerminalTypesDesc.h"
#include "TypeDef.h"
#include "Parser/LangNodesDefinitions.h"

namespace NCodeGen
{

CTypeDefinition::CTypeDefinition( NLang::CComplexTypeNode *pComplexTypeNode, const CNodes2TypeDefs &nodes2TypeDefs, NDb::NTypeDef::CTerminalTypesDescriptor *pTermTypesDesc )
{
	CNodes2TypeDefs::const_iterator iter = nodes2TypeDefs.find( pComplexTypeNode );
	NI_VERIFY( iter != nodes2TypeDefs.end(), StrFmt( "can't find typedef for node %s", pComplexTypeNode->GetName().c_str() ), return );
	pType = GetRealType( iter->second );

	NI_ASSERT( !pType->GetTypeName().empty(), "type with empty name" );

	bTerminal = pTermTypesDesc->IsTerminalType( pType );
	pNamespace = new CNamespace( pComplexTypeNode->GetNamespace(), nodes2TypeDefs, pTermTypesDesc );
}

CTypeDefinition::CTypeDefinition( NLang::CEnumNode *pEnumNode, const CNodes2TypeDefs &nodes2TypeDefs )
: bTerminal( false )
{
	CNodes2TypeDefs::const_iterator iter = nodes2TypeDefs.find( pEnumNode );
	NI_VERIFY( iter != nodes2TypeDefs.end(), StrFmt( "can't find typedef for node %s", pEnumNode->GetName().c_str() ), return );
	pType = GetRealType( iter->second );
	NI_ASSERT( !pType->GetTypeName().empty(), "type with empty name" );
}

static void GenerateEnum( ICode::SCodeStreams *pCode, NDb::NTypeDef::STypeEnum *pEnum, const std::string &szTabs, const std::string &szQualifiedName )
{
	pCode->h << endl;

	pCode->h << szTabs << "enum " << NHungarian::GetTypeNameInCode( pEnum, 0 ) << endl;
	pCode->h << szTabs << "{" << endl;
	for ( int i = 0; i < pEnum->entries.size(); ++i )
	{
		const NDb::NTypeDef::STypeEnum::SEnumEntry &entry = pEnum->entries[i];
		pCode->h << szTabs << tab << entry.szName;
		if ( entry.nVal != -1 )
			pCode->h << " = " << entry.nVal;
		pCode->h << "," << endl;
	}
	pCode->h << szTabs + "};" << endl;

	const std::string szFullQualifiedName = szQualifiedName + "::" + NHungarian::GetTypeNameInCode( pEnum, 0 );

	std::string szUnderlinedName( szFullQualifiedName );
	int nCnt = 0;
	int i = 0;
	while ( i < szUnderlinedName.size() )
	{
		if ( szUnderlinedName[i] != ':' )
			szUnderlinedName[nCnt++] = szUnderlinedName[i];
		else
		{
			szUnderlinedName[nCnt++] = '_';
			++i;
		}

		++i;
	}
	szUnderlinedName.resize( nCnt );
	szUnderlinedName = "NDb::StringToEnum_" + szUnderlinedName;

	const std::string szQualifiedNameWithoutNDB = szFullQualifiedName.substr( 5, szFullQualifiedName.size() );
	const std::string szUnderlinedNameWithoutNDB = szUnderlinedName.substr( 5, szUnderlinedName.size() );
	pCode->hEOF << separator;
	pCode->hEOF << "namespace NDb" << endl;
	pCode->hEOF << "{" << endl;
	pCode->hEOF << tab << "string EnumToString( " << szFullQualifiedName << " eValue );" << endl;
	pCode->hEOF << tab << szQualifiedNameWithoutNDB << " " << szUnderlinedNameWithoutNDB << "( const string &szValue );" << endl;
	pCode->hEOF << "}" << endl;

	pCode->hEOF << separator;
	pCode->hEOF << "template <>" << endl;
	pCode->hEOF << "struct SKnownEnum<" << szFullQualifiedName << ">" << endl;
	pCode->hEOF << "{" << endl;
	pCode->hEOF << tab << "enum { isKnown = 1 };" << endl;
	pCode->hEOF << tab << "static string ToString( " << szFullQualifiedName << " eValue ) { return NDb::EnumToString( eValue ); }" << endl;
	pCode->hEOF << tab << "static " << szFullQualifiedName << " ToEnum( const string &szValue ) { return " << szUnderlinedName << "( szValue ); }" << endl;
	pCode->hEOF << "};" << endl;

	pCode->cpp << separator;
	pCode->cpp << "string EnumToString( " << szFullQualifiedName << " eValue )" << endl;
	pCode->cpp << "{" << endl;
	pCode->cpp << tab << "switch ( eValue )" << endl;
	pCode->cpp << tab << "{" << endl;
	for ( int i = 0; i < pEnum->entries.size(); ++i )
	{
		const NDb::NTypeDef::STypeEnum::SEnumEntry &entry = pEnum->entries[i];
		pCode->cpp << tab << "case " << szQualifiedName << "::" << entry.szName << ":" << endl;
		pCode->cpp << tab << tab << "return " << qcomma << entry.szName << qcomma << ";" << endl;
	}
	pCode->cpp << tab << "default:" << endl;
	pCode->cpp << tab << tab << "return " << qcomma << pEnum->entries[0].szName << qcomma << ";" << endl;
	pCode->cpp << tab << "}" << endl;
	pCode->cpp << "}" << endl;

	pCode->cpp << separator;
	pCode->cpp << szFullQualifiedName << " " << szUnderlinedName << "( const string &szValue )" << endl;
	pCode->cpp << "{" << endl;
	for ( int i = 0; i < pEnum->entries.size(); ++i )
	{
		const NDb::NTypeDef::STypeEnum::SEnumEntry &entry = pEnum->entries[i];
		pCode->cpp << tab << "if ( szValue == " << qcomma << entry.szName << qcomma << " )" << endl;
		pCode->cpp << tab << tab << "return " <<  szQualifiedName << "::" << entry.szName << ";" << endl;
	}
	pCode->cpp << tab << "return " << szQualifiedName << "::" << pEnum->entries[0].szName << ";" << endl;
	pCode->cpp << "}" << endl;
}

enum EStructType
{
	EST_ERROR,
	EST_STRUCT,
	EST_CLASS_NOT_TERMINAL,
	EST_CLASS_TERMINAL,
};

static void GenerateMetaInfoFunc( ICode::SCodeStreams *pCode, NDb::NTypeDef::STypeStructBase *pStructBase, EStructType eType, const std::string &szFullQualifiedName )
{
	const std::string szQualifiedName = szFullQualifiedName.substr( 5, szFullQualifiedName.size() );
	if ( eType != EST_STRUCT )
		pCode->cpp << "void " << szQualifiedName << "::ReportMetaInfo() const" << endl;
	else
		pCode->cpp << "void " << szQualifiedName << "::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const" << endl;

	pCode->cpp << "{" << endl;
	bool bPrintEndL = false;
	if ( eType == EST_CLASS_TERMINAL )
	{
		pCode->cpp << tab << "NMetaInfo::StartMetaInfoReport( " << qcomma << pStructBase->GetTypeName() << qcomma << ", typeID, sizeof(*this) );" << endl;
		bPrintEndL = true;
	}

	if ( pStructBase->pBaseType != 0 )
	{
		if ( eType != EST_STRUCT )
			pCode->cpp << tab << NHungarian::GetTypeNameInCode( pStructBase->pBaseType, 0 ) << "::ReportMetaInfo();" << endl;
		else
			pCode->cpp << tab << NHungarian::GetTypeNameInCode( pStructBase->pBaseType, 0 ) << "::ReportMetaInfo( szAddName, pThis );" << endl;

		bPrintEndL = true;
	}

	if ( bPrintEndL )
		pCode->cpp << endl;

	if ( eType != EST_STRUCT )
		pCode->cpp << tab << "BYTE *pThis = (BYTE*)this;" << endl;

	for ( NDb::NTypeDef::STypeClass::CFieldsList::iterator iter = pStructBase->fields.begin(); iter != pStructBase->fields.end(); ++iter )
	{
		NDb::NTypeDef::STypeClass::SField &field = *iter;

		if ( IsNoCode( field ) )
			continue;

		NDb::NTypeDef::STypeDef *pFieldType = field.pType;
		const std::string szCodeFieldName = NHungarian::GetFieldNameInCode( field );
		const std::string szParamsPrefix = eType == EST_STRUCT ? "szAddName + " : "";
		switch ( pFieldType->eType )
		{
		case NDb::NTypeDef::TYPE_TYPE_ARRAY:
			{
				CDynamicCast<NDb::NTypeDef::STypeArray> pArray = pFieldType;
				NDb::NTypeDef::STypeDef *pArrayType = pArray->field.pType;
				if ( pArrayType->eType == NDb::NTypeDef::TYPE_TYPE_CLASS || pArrayType->eType == NDb::NTypeDef::TYPE_TYPE_STRUCT )
					pCode->cpp << tab << "NMetaInfo::ReportStructArrayMetaInfo( " << szParamsPrefix << qcomma << field.szName << qcomma << ", &" << szCodeFieldName << ", pThis );" << endl;
				else
					pCode->cpp << tab << "NMetaInfo::ReportSimpleArrayMetaInfo( " << szParamsPrefix << qcomma << field.szName << qcomma << ", &" << szCodeFieldName << ", pThis );" << endl;
			}
			break;

		case NDb::NTypeDef::TYPE_TYPE_STRUCT:
			pCode->cpp << tab << "NMetaInfo::ReportStructMetaInfo( " << szParamsPrefix << qcomma << field.szName << qcomma << ", &" << szCodeFieldName << ", pThis ); " << endl;
			break;
		default:
			const std::string szEnumTypeName =
				pFieldType->eType == NDb::NTypeDef::TYPE_TYPE_CLASS ?
				SKnownEnum<NDb::NTypeDef::ETypeType>::ToString( NDb::NTypeDef::TYPE_TYPE_REF ) :
			SKnownEnum<NDb::NTypeDef::ETypeType>::ToString( pFieldType->eType );

			pCode->cpp << tab << "NMetaInfo::ReportMetaInfo( " << szParamsPrefix << qcomma << field.szName << qcomma << ", (BYTE*)&" << szCodeFieldName << " - pThis, sizeof(" << szCodeFieldName << "), NTypeDef::" << szEnumTypeName << " );" << endl;
		}
	}

	if ( eType == EST_CLASS_TERMINAL )
		pCode->cpp << tab << "NMetaInfo::FinishMetaInfoReport();" << endl;

	pCode->cpp << "}" << endl;
}

static void GenerateXMLSaveFunc( ICode::SCodeStreams *pCode, NDb::NTypeDef::STypeStructBase *pStructBase, EStructType eType, const std::string &szFullQualifiedName )
{
	const std::string szQualifiedName = szFullQualifiedName.substr( 5, szFullQualifiedName.size() );
	pCode->cpp << "int " << szQualifiedName << "::operator&( IXmlSaver &saver )" << endl;
	pCode->cpp << "{" << endl;

	if ( eType == EST_CLASS_TERMINAL )
		pCode->cpp << tab << "NMetaInfo::STerminalClassReporter reporter( this, saver );" << endl;

	if ( pStructBase->pBaseType != 0 )
		pCode->cpp << tab << "saver.AddTypedSuper( (" << NHungarian::GetTypeNameInCode( pStructBase->pBaseType, 0 ) << "*)(this) );" << endl;

	for ( NDb::NTypeDef::STypeClass::CFieldsList::iterator iter = pStructBase->fields.begin(); iter != pStructBase->fields.end(); ++iter )
	{
		NDb::NTypeDef::STypeClass::SField &field = *iter;
		if ( IsNoCode( field ) )
			continue;

		const std::string szCodeFieldName = NHungarian::GetFieldNameInCode( field );
		pCode->cpp << tab << "saver.Add( " << qcomma << field.szName << qcomma << ", &" << szCodeFieldName << " );" << endl;
	}
	pCode->cpp << endl;
	pCode->cpp << tab << "return 0;" << endl;

	pCode->cpp << "}" << endl;
}

struct SFieldSort
{
	bool operator()( const NDb::NTypeDef::STypeClass::SField &field1, NDb::NTypeDef::STypeClass::SField &field2 ) const
	{
		return field1.nChunkID < field2.nChunkID;
	}
};

static void GenerateBinSaveFunc( ICode::SCodeStreams *pCode, NDb::NTypeDef::STypeStructBase *pStructBase, const std::string &szFullQualifiedName )
{
	const std::string szQualifiedName = szFullQualifiedName.substr( 5, szFullQualifiedName.size() );
	pCode->cpp << "int " << szQualifiedName << "::operator&( IBinSaver &saver )" << endl;
	pCode->cpp << "{" << endl;

	if ( pStructBase->pBaseType != 0 )
		pCode->cpp << tab << "saver.Add( 1, (" << NHungarian::GetTypeNameInCode( pStructBase->pBaseType, 0 ) << "*)this );" << endl;

	NDb::NTypeDef::STypeClass::CFieldsList sortedFields( pStructBase->fields );
	SFieldSort fieldSort;
	sort( sortedFields.begin(), sortedFields.end(), fieldSort );
	for ( NDb::NTypeDef::STypeClass::CFieldsList::iterator iter = sortedFields.begin(); iter != sortedFields.end(); ++iter )
	{
		NDb::NTypeDef::STypeClass::SField &field = *iter;
		if ( IsNoCode( field ) )
			continue;

		const std::string szCodeFieldName = NHungarian::GetFieldNameInCode( field );
		pCode->cpp << tab << "saver.Add( " << field.nChunkID << ", &" << szCodeFieldName << " );" << endl;
	}

	pCode->cpp << endl;
	pCode->cpp << tab << "return 0;" << endl;

	pCode->cpp << "}" << endl;
}

static bool IsNoCheckSum( NDb::NTypeDef::SAttributes *pAttr )
{
	return pAttr && pAttr->attributes.find( "no_checksum" ) != pAttr->attributes.end();
}

static bool IsNoCheckSum( NDb::NTypeDef::STypeDef *pType )
{
	if ( !pType )
		return true;

	if ( IsNoCheckSum( pType->GetAttributes() ) )
		return true;

	if ( pType->eType == NDb::NTypeDef::TYPE_TYPE_REF )
	{
		CDynamicCast<NDb::NTypeDef::STypeRef> pRefType = pType;
		if ( pRefType && pRefType->pRefType && IsNoCheckSum( pRefType->pRefType->GetAttributes() ) )
			return true;
	}

	if ( pType->eType == NDb::NTypeDef::TYPE_TYPE_ARRAY )
	{
		CDynamicCast<NDb::NTypeDef::STypeArray> pArrayType = pType;
		if ( pArrayType && IsNoCheckSum( pArrayType->field.pAttributes ) )
			return true;
	}

	return false;
}

static void GenerateCheckSumFunc( ICode::SCodeStreams *pCode, NDb::NTypeDef::STypeStructBase *pStructBase, const std::string &szFullQualifiedName )
{
	const std::string szQualifiedName = szFullQualifiedName.substr( 5, szFullQualifiedName.size() );
	pCode->cpp << "DWORD " << szQualifiedName <<"::CalcCheckSum() const" << endl;
	pCode->cpp << "{" << endl;

	pCode->cpp << tab << "if ( __dwCheckSum != 0 )" << endl;
	pCode->cpp << tab << tab << "return __dwCheckSum;" << endl;
	pCode->cpp << tab << "__dwCheckSum = 1;" << endl << endl;

	bool bStartedWriteCheckSum = false;

	if ( !IsNoCheckSum( pStructBase->pBaseType ) )
	{
		if ( !bStartedWriteCheckSum )
		{
			pCode->cpp << tab << "CCheckSum checkSum;" << endl;
			pCode->cpp << tab << "checkSum";
			bStartedWriteCheckSum = true;
		}

		pCode->cpp << " << " << NHungarian::GetTypeNameInCode( pStructBase->pBaseType, 0 ) << "::CalcCheckSum()";
	}

	for ( int i = 0; i < pStructBase->fields.size(); ++i )
	{
		const NDb::NTypeDef::STypeStructBase::SField &field = pStructBase->fields[i];
		if ( IsNoCode( field ) )
			continue;

		if ( IsNoCheckSum( field.pAttributes ) )
			continue;
		if ( IsNoCheckSum( field.pType->GetAttributes() ) )
			continue;

		if ( IsNoCheckSum( field.pType ) )
			continue;

		if ( !bStartedWriteCheckSum )
		{
			pCode->cpp << tab << "CCheckSum checkSum;" << endl;
			pCode->cpp << tab << "checkSum";
		}
		bStartedWriteCheckSum = true;

		const std::string szCodeFieldName = NHungarian::GetFieldNameInCode( field );
		pCode->cpp << " << " << szCodeFieldName;
	}

	if ( bStartedWriteCheckSum )
	{
		pCode->cpp << ";" << endl;
		pCode->cpp << tab << "__dwCheckSum = checkSum.GetCheckSum();" << endl;
	}

	pCode->cpp << tab << "if ( __dwCheckSum == 0 )" << endl;
	pCode->cpp << tab << tab << "__dwCheckSum = 1;" << endl << endl;
	pCode->cpp << tab << "return __dwCheckSum;" << endl;

	pCode->cpp << "}" << endl;
}

static void GenerateBaseStructCPPFile( ICode::SCodeStreams *pCode, NDb::NTypeDef::STypeStructBase *pStructBase, EStructType eType, const std::string &szFullQualifiedName )
{
	pCode->cpp << endl;

	pCode->cpp << separator;
	GenerateMetaInfoFunc( pCode, pStructBase, eType, szFullQualifiedName );

	pCode->cpp << separator;
	GenerateXMLSaveFunc( pCode, pStructBase, eType , szFullQualifiedName );

	pCode->cpp << separator;
	GenerateBinSaveFunc( pCode, pStructBase, szFullQualifiedName );

	if ( !IsNoCheckSum( pStructBase ) )
	{
		pCode->cpp << separator;
		GenerateCheckSumFunc( pCode, pStructBase, szFullQualifiedName );
	}

	pCode->cpp << separator;
}

static std::unordered_map<std::string, std::string> defaultValues;
struct SSetDefaultValues
{
	SSetDefaultValues()
	{
		defaultValues["int"] = "0";
		defaultValues["float"] = "0.0f";
		defaultValues["bool"] = "false";
		defaultValues["Vec2"] = "VNULL2";
		defaultValues["Vec3"] = "VNULL3";
		defaultValues["Vec4"] = "VNULL4";
		defaultValues["Quat"] = "QNULL";
	}

} setDefaultValues;

static void GenerateStructBaseConstructor( ICode::SCodeStreams *pCode, NDb::NTypeDef::STypeStructBase *pStructBase, const std::string &szTabs )
{
	pCode->h << szTabs << tab << NHungarian::GetTypeNameInCode( pStructBase, 0 ) << "() ";
	if ( pStructBase->fields.empty() )
		pCode->h << "{ }" << endl;
	else
	{
		bool bFirst = true;
		if ( !IsNoCheckSum( pStructBase ) )
		{
			pCode->h << ":" << endl << szTabs << tab << tab << "__dwCheckSum( 0 )";
			bFirst = false;
		}
		for ( NDb::NTypeDef::STypeStructBase::CFieldsList::iterator iter = pStructBase->fields.begin(); iter != pStructBase->fields.end(); ++iter )
		{
			const NDb::NTypeDef::STypeStructBase::SField &field = *iter;
			if ( IsNoCode( field ) )
				continue;

			if ( field.defaultValue.IsNull() && field.complexDefaultValue.IsNull() )
			{
				const std::string szFieldTypeName = field.pType->GetTypeName();
				bool bHasDefaultValue =
					defaultValues.find( szFieldTypeName ) != defaultValues.end() ||
					field.pType->eType == NDb::NTypeDef::TYPE_TYPE_ENUM;
				if ( bHasDefaultValue )
				{
					const std::string szCodeFieldName = NHungarian::GetFieldNameInCode( field );
					if ( bFirst )
					{
						bFirst = false;
						pCode->h << ":" << endl;
					}
					else
						pCode->h << "," << endl;

					std::string szValue;
					if ( field.pType->eType == NDb::NTypeDef::TYPE_TYPE_ENUM )
					{
						CDynamicCast<NDb::NTypeDef::STypeEnum> pEnum = field.pType;
						szValue = pEnum->entries[0].szName;
					}
					else
						szValue = defaultValues[szFieldTypeName];

					pCode->h << szTabs << tab << tab << szCodeFieldName << "( " << szValue << " )";
				}
			}
			else
			{
				const CVariant &value = field.defaultValue.IsNull() ? field.complexDefaultValue : field.defaultValue;
				std::string szValue;
				if ( value.GetType() == CVariant::VT_STR )
				{
					if ( field.pType->eType == NDb::NTypeDef::TYPE_TYPE_ENUM || field.defaultValue.IsNull() )
						szValue = value.GetStr();
					else
						szValue = std::string("\"") + value.GetStr() + "\"";
				}
				else if ( value.GetType() == CVariant::VT_WSTR )
				{
					NStr::ToMBCS( &szValue, value.GetWStr() );
					szValue = "L\"" + szValue + "\"";
				}
				else
					value.ToText( &szValue );

				if ( field.pType->eType == NDb::NTypeDef::TYPE_TYPE_REF )
					szValue = "NDb::Get(CDBID(" + szValue + "))";

				const std::string szCodeFieldName = NHungarian::GetFieldNameInCode( field );
				if ( bFirst )
				{
					bFirst = false;
					pCode->h << ":" << endl;
				}
				else
					pCode->h << "," << endl;

				pCode->h << szTabs << tab << tab << szCodeFieldName << "( " << szValue << " )";
			}
		}

		if ( bFirst )
			pCode->h << "{ }" << endl;
		else if ( !bFirst )
		{
			pCode->h << endl;
			pCode->h << szTabs << tab << "{ }" << endl;
		}
	}
}

static void GenereateTypeDefs( ICode::SCodeStreams *pCode, NDb::NTypeDef::SAttributes *pAttr, const std::string &szTabs )
{
	if ( pAttr == 0 )
		return;

	std::unordered_map<std::string, CVariant> &attr = pAttr->attributes;
	std::unordered_map<std::string, CVariant>::iterator iter = attr.find( "type_defs" );
	if ( iter == attr.end() )
		return;

	const std::string szTypeDefs = iter->second.GetStr();
	std::vector<std::string> typedefs;
	NStr::SplitString( szTypeDefs, &typedefs, ';' );
	for ( int i = 0; i < typedefs.size() - 1; i += 2 )
		pCode->h << szTabs << "typedef " << typedefs[i] << " " << typedefs[i+1] << ";" << endl;
}

static void GenerateStructHFileAndNestedTypes( ICode::SCodeStreams *pCode, NDb::NTypeDef::STypeStruct *pStruct, const std::string &szTabs, ICode *pNamespace, const std::string &szQualifiedName )
{
	pCode->h << endl;

	pCode->h << szTabs << "struct " << NHungarian::GetTypeNameInCode( pStruct, 0 );
	if ( pStruct->pBaseType != 0 )
		pCode->h << " : public " << NHungarian::GetTypeNameInCode( pStruct->pBaseType, 0 );
	pCode->h << endl;

	pCode->h << szTabs << "{" << endl;

	GenereateTypeDefs( pCode, pStruct->pAttributes, szTabs + '\t' );

	const bool bNoCheckSum = IsNoCheckSum( pStruct );
	if ( !bNoCheckSum )
	{
		pCode->h << szTabs << "private:" << endl;
		pCode->h << szTabs << tab << "mutable DWORD __dwCheckSum;" << endl;
		pCode->h << szTabs << "public:" << endl;
	}
	pNamespace->GenerateCode( pCode, szTabs, pStruct, szQualifiedName );

	pCode->h << endl;
	GenerateStructBaseConstructor( pCode, pStruct, szTabs );
	pCode->h << szTabs << tab << "//" << endl;
	pCode->h << szTabs << tab << "void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;" << endl;
	pCode->h << szTabs << tab << "//" << endl;
	pCode->h << szTabs << tab << "int operator&( IBinSaver &saver );" << endl;
	pCode->h << szTabs << tab << "int operator&( IXmlSaver &saver );" << endl;
	if ( !bNoCheckSum )
		pCode->h << szTabs << tab << "DWORD CalcCheckSum() const;" << endl;
	else
		pCode->h << szTabs << tab << "DWORD CalcCheckSum() const { return 0; }" << endl;

	pCode->h << szTabs << "};" << endl;
}

static void GenerateStruct( ICode::SCodeStreams *pCode, NDb::NTypeDef::STypeStruct *pStruct, const std::string &szTabs, ICode *pNamespace, const std::string &szQualifiedName )
{
	const std::string szNewQualifiedName( szQualifiedName + "::" + NHungarian::GetTypeNameInCode( pStruct, 0 ) );
	GenerateStructHFileAndNestedTypes( pCode, pStruct, szTabs, pNamespace, szNewQualifiedName );
	GenerateBaseStructCPPFile( pCode, pStruct, EST_STRUCT, szNewQualifiedName );
}

static void GenerateClassHFileAndNestedTypes( ICode::SCodeStreams *pCode, NDb::NTypeDef::STypeClass *pClass, const std::string &szTabs, ICode *pNamespace, const bool bTerminal, const std::string &szQualifiedName )
{
	pCode->h << endl;

	pCode->h << szTabs << "struct " << NHungarian::GetTypeNameInCode( pClass, 0 );
	if ( pClass->pBaseType != 0 )
		pCode->h << " : public " << NHungarian::GetTypeNameInCode( pClass->pBaseType, 0 );
	else
		pCode->h << " : public CResource";
	pCode->h << endl;

	pCode->h << szTabs << "{" << endl;

	GenereateTypeDefs( pCode, pClass->pAttributes, szTabs + "\t" );

	if ( bTerminal )
		pCode->h << szTabs << tab << "OBJECT_BASIC_METHODS( " << NHungarian::GetTypeNameInCode( pClass, 0 ) << " )" << endl;
	pCode->h << szTabs << "public:" << endl;
	if ( bTerminal )
		pCode->h << szTabs << tab << "enum { typeID = " << StrFmt( "0x%X", pClass->nClassTypeID ) << " };" << endl;

	const bool bNoCheckSum = IsNoCheckSum( pClass );
	if ( !bNoCheckSum )
	{
		pCode->h << szTabs << "private:" << endl;
		pCode->h << szTabs << tab << "mutable DWORD __dwCheckSum;" << endl;
		pCode->h << szTabs << "public:" << endl;
	}
	pNamespace->GenerateCode( pCode, szTabs, pClass, szQualifiedName );

	pCode->h << endl;
	GenerateStructBaseConstructor( pCode, pClass, szTabs );

	if ( bTerminal )
	{
		pCode->h << szTabs << tab << "//" << endl;
		pCode->h << szTabs << tab << "int GetTypeID() const { return typeID; }" << endl;
	}

	pCode->h << szTabs << tab << "//" << endl;
	pCode->h << szTabs << tab << "void ReportMetaInfo() const;" << endl;
	pCode->h << szTabs << tab << "//" << endl;
	pCode->h << szTabs << tab << "int operator&( IBinSaver &saver );" << endl;
	pCode->h << szTabs << tab << "int operator&( IXmlSaver &saver );" << endl;
	if ( !bNoCheckSum )
		pCode->h << szTabs << tab << "DWORD CalcCheckSum() const;" << endl;
	else
		pCode->h << szTabs << tab << "DWORD CalcCheckSum() const { return 0; }" << endl;

	pCode->h << szTabs << "};" << endl;
}

static void GenerateClass( ICode::SCodeStreams *pCode, NDb::NTypeDef::STypeClass *pClass, const std::string &szTabs, ICode *pNamespace, const bool bTerminal, const std::string &szQualifiedName )
{
	const std::string szNewQualifiedName( szQualifiedName + "::" + NHungarian::GetTypeNameInCode( pClass, 0 ) );
	GenerateClassHFileAndNestedTypes( pCode, pClass, szTabs, pNamespace, bTerminal, szNewQualifiedName );
	GenerateBaseStructCPPFile( pCode, pClass, bTerminal ? EST_CLASS_TERMINAL : EST_CLASS_NOT_TERMINAL, szNewQualifiedName );

	if ( bTerminal )
		pCode->cppEOF << "REGISTER_DATABASE_CLASS( " << StrFmt( "0x%X", pClass->nClassTypeID ) << ", " << NHungarian::GetTypeNameInCode( pClass, 0 ) << " ) " << endl;
	else
		pCode->cppEOF << "BASIC_REGISTER_DATABASE_CLASS( " << NHungarian::GetTypeNameInCode( pClass, 0 ) << " )" << endl;
}

void CTypeDefinition::GenerateCode( SCodeStreams *pCode, const std::string &szTabs, NDb::NTypeDef::STypeDef *pParentType, const std::string &szQualifiedName )
{
	if ( IsNoCode( pType->GetAttributes() ) )
		return;

	if ( NDb::NTypeDef::SAttributes *pAttr = pType->GetAttributes() )
	{
		if ( pAttr->attributes.find( "noHeader" ) != pAttr->attributes.end() )
			return;
	}

	if ( CDynamicCast<NDb::NTypeDef::STypeEnum> pTypeEnum = pType )
		GenerateEnum( pCode, pTypeEnum, szTabs, szQualifiedName );
	else if ( CDynamicCast<NDb::NTypeDef::STypeStruct> pTypeStruct = pType )
		GenerateStruct( pCode, pTypeStruct, szTabs, pNamespace, szQualifiedName );
	else if ( CDynamicCast<NDb::NTypeDef::STypeClass> pTypeClass = pType )
		GenerateClass( pCode, pTypeClass, szTabs, pNamespace, bTerminal, szQualifiedName );
	else
		NI_ASSERT( false, StrFmt( "can't generate type definition of type %s", typeid( *pType ).name() ) );
}

}

using namespace NCodeGen;
REGISTER_SAVELOAD_CLASS( 0x301B6D02, CTypeDefinition );

