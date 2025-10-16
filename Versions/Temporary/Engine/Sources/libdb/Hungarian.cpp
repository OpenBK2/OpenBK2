#include "stdafx.h"

#include "Hungarian.h"
#include "Type.h"
#include "TypeDef.h"

#include <fmt/format.h>

namespace NHungarian
{

const std::string GetTypePrefix( const NDb::NTypeDef::ETypeType eType, NDb::NTypeDef::SAttributes *pTypeAttributes )
{
	if ( pTypeAttributes )
	{
		const std::unordered_map<std::string, CVariant>::const_iterator iter = pTypeAttributes->attributes.find( "typePrefix" );
		if ( iter != pTypeAttributes->attributes.end() )
		{
			const CVariant &res = iter->second;
			return res.GetStr();
		}
	}

	switch ( eType )
	{
	case NDb::NTypeDef::TYPE_TYPE_UNKNOWN: return "";
	case NDb::NTypeDef::TYPE_TYPE_INT: return "n";
	case NDb::NTypeDef::TYPE_TYPE_FLOAT: return "f";
	case NDb::NTypeDef::TYPE_TYPE_BOOL: return "b";
	case NDb::NTypeDef::TYPE_TYPE_GUID: return "";
	case NDb::NTypeDef::TYPE_TYPE_STRING: return "sz";
	case NDb::NTypeDef::TYPE_TYPE_WSTRING: return "wsz";
	case NDb::NTypeDef::TYPE_TYPE_BINARY: return "";
	case NDb::NTypeDef::TYPE_TYPE_ENUM: return "e";
	case NDb::NTypeDef::TYPE_TYPE_REF: return "p";
	case NDb::NTypeDef::TYPE_TYPE_ARRAY: return "";
	case NDb::NTypeDef::TYPE_TYPE_STRUCT: return "";
	case NDb::NTypeDef::TYPE_TYPE_CLASS: return "";
	default:
		NI_ASSERT( false, "Unknown type" );
		return "";
	}
}

bool ConvertToShortName( std::string *pszShortFieldName, const std::string &szFullFieldName, NDb::NTypeDef::ETypeType eType, NDb::NTypeDef::SAttributes *pTypeAttributes )
{
	const std::string szPrefix = GetTypePrefix( eType, pTypeAttributes );

	// FIX ME: for compatibility with xsd!
	if ( szFullFieldName[0] == '_' )
	{
		*pszShortFieldName = szFullFieldName.c_str() + 1;
		return true;
	}
	else
	{
		*pszShortFieldName = szFullFieldName;
		//
		if ( !szPrefix.empty() )
		{
			if ( pszShortFieldName->compare(0, szPrefix.size(), szPrefix) != 0 )
				return false;
			else if ( szPrefix.size() == pszShortFieldName->size() )
				return false;
			else
			{
				//			const char cNextPrefixLetter = (*pszShortFieldName)[szPrefix.size()];
				//			if ( NStr::ASCII_toupper( cNextPrefixLetter ) == cNextPrefixLetter )
				*pszShortFieldName = pszShortFieldName->substr( szPrefix.size() );
				return true;
			}
		}
		else
			return true;
	}


	return false;
}

static bool FindRenameInAttributes( std::string *pszResult, NDb::NTypeDef::SAttributes *pAttributes )
{
	if ( pAttributes )
	{
		std::unordered_map<std::string, CVariant> &attributes = pAttributes->attributes;
		if ( attributes.find( "typeRename" ) != attributes.end() )
		{
			std::string szTypeName = attributes["typeRename"].GetStr();
			NStr::ReplaceAllChars( &szTypeName, '{', '<' );
			NStr::ReplaceAllChars( &szTypeName, '}', '>' );

			*pszResult = szTypeName;
			return true;
		}
	}

	return false;
}

const std::string GetTypeNameInCode( NDb::NTypeDef::STypeDef *pRawType, const NDb::NTypeDef::STypeStructBase::SField *pField )
{
	NDb::NTypeDef::STypeDef *pType = pRawType;
	if ( CDynamicCast<NDb::NTypeDef::STypeRef> pRef = pType )
		pType = pRef->pRefType;

	{
		std::string szResult;
		if ( pField )
		{
			if ( FindRenameInAttributes( &szResult, pField->pAttributes ) )
				return szResult;
		}

		if ( FindRenameInAttributes( &szResult, pType->GetAttributes() ) )
			return szResult;

		if ( pField && pField->pType )
		{
			CDynamicCast<NDb::NTypeDef::STypeArray> pTypeArray = pField->pType;
			if ( pTypeArray != 0 )
			{
				if ( FindRenameInAttributes( &szResult, pTypeArray->field.pAttributes ) )
					return szResult;
			}
		}
	}

	if ( pType->eType == NDb::NTypeDef::TYPE_TYPE_CLASS || pType->eType == NDb::NTypeDef::TYPE_TYPE_STRUCT )
	{
		if ( pType->GetTypeName() == "Resource" )
			return fmt::format( "C{}", pType->GetTypeName() );
		else
			return fmt::format( "S{}", pType->GetTypeName() );
	}
	else if ( pType->eType == NDb::NTypeDef::TYPE_TYPE_ENUM )
		return fmt::format( "E{}", pType->GetTypeName() );

	return pType->GetTypeName();
}

const std::string GetFieldNameInCode( const NDb::NTypeDef::STypeClass::SField &field )
{
	if ( field.pAttributes && field.pAttributes->attributes.find( "no_prefix" ) != field.pAttributes->attributes.end() )
		return field.szName;
	
	NDb::NTypeDef::STypeDef *pFieldType = field.pType;
	const std::string szPrefix = NHungarian::GetTypePrefix( pFieldType->eType, pFieldType->GetAttributes() );
	std::string szResult( field.szName );
	if ( szPrefix.empty() )
	{
		szResult[0] = NStr::ASCII_tolower( szResult[0] );
		return szResult;
	}
	else
	{
//		szResult[0] = NStr::ASCII_toupper( szResult[0] );
		return szPrefix + szResult;
	}
}

}


