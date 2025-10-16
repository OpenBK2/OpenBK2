#include "stdafx.h"
#include "Type.h"

#include <fmt/format.h>

NDb::NTypeDef::ETypeType StringToEnum_NDb_NTypeDef_ETypeType( const std::string &szName )
{
	if ( szName == "TYPE_TYPE_UNKNOWN" )
		return NDb::NTypeDef::TYPE_TYPE_UNKNOWN;
	if ( szName == "TYPE_TYPE_INT" )
		return NDb::NTypeDef::TYPE_TYPE_INT;
	if ( szName == "TYPE_TYPE_FLOAT" )
		return NDb::NTypeDef::TYPE_TYPE_FLOAT;
	if ( szName == "TYPE_TYPE_BOOL" )
		return NDb::NTypeDef::TYPE_TYPE_BOOL;
	if ( szName == "TYPE_TYPE_GUID" )
		return NDb::NTypeDef::TYPE_TYPE_GUID;
	if ( szName == "TYPE_TYPE_STRING" )
		return NDb::NTypeDef::TYPE_TYPE_STRING;
	if ( szName == "TYPE_TYPE_WSTRING" )
		return NDb::NTypeDef::TYPE_TYPE_WSTRING;
	if ( szName == "TYPE_TYPE_BINARY" )
		return NDb::NTypeDef::TYPE_TYPE_BINARY;
	if ( szName == "TYPE_TYPE_ENUM" )
		return NDb::NTypeDef::TYPE_TYPE_ENUM;
	if ( szName == "TYPE_TYPE_STRUCT" )
		return NDb::NTypeDef::TYPE_TYPE_STRUCT;
	if ( szName == "TYPE_TYPE_CLASS" )
		return NDb::NTypeDef::TYPE_TYPE_CLASS;
	if ( szName == "TYPE_TYPE_REF" )
		return NDb::NTypeDef::TYPE_TYPE_REF;
	if ( szName == "TYPE_TYPE_ARRAY" )
		return NDb::NTypeDef::TYPE_TYPE_ARRAY;

	NI_ASSERT( false, fmt::format( "unknown enum {}", szName ) );
	return NDb::NTypeDef::TYPE_TYPE_UNKNOWN;
}

const char *EnumToString_NDb_NTypeDef_ETypeType( NDb::NTypeDef::ETypeType eType )
{
	switch ( eType )
	{
	case NDb::NTypeDef::TYPE_TYPE_UNKNOWN:
		return "TYPE_TYPE_UNKNOWN";
	case NDb::NTypeDef::TYPE_TYPE_INT:
		return "TYPE_TYPE_INT";
	case NDb::NTypeDef::TYPE_TYPE_FLOAT:
		return "TYPE_TYPE_FLOAT";
	case NDb::NTypeDef::TYPE_TYPE_BOOL:
		return "TYPE_TYPE_BOOL";
	case NDb::NTypeDef::TYPE_TYPE_GUID:
		return "TYPE_TYPE_GUID";
	case NDb::NTypeDef::TYPE_TYPE_STRING:
		return "TYPE_TYPE_STRING";
	case NDb::NTypeDef::TYPE_TYPE_WSTRING:
		return "TYPE_TYPE_WSTRING";
	case NDb::NTypeDef::TYPE_TYPE_BINARY:
		return "TYPE_TYPE_BINARY";
	case NDb::NTypeDef::TYPE_TYPE_ENUM:
		return "TYPE_TYPE_ENUM";
	case NDb::NTypeDef::TYPE_TYPE_STRUCT:
		return "TYPE_TYPE_STRUCT";
	case NDb::NTypeDef::TYPE_TYPE_CLASS:
		return "TYPE_TYPE_CLASS";
	case NDb::NTypeDef::TYPE_TYPE_REF:
		return "TYPE_TYPE_REF";
	case NDb::NTypeDef::TYPE_TYPE_ARRAY:
		return "TYPE_TYPE_ARRAY";
	default:
		NI_ASSERT( false, "unknown enum" );
		return "TYPE_TYPE_UNKNOWN";
	}
}


