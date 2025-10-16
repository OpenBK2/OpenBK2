#include "stdafx.h"

#include "ErrorsAndMessages.h"
#include "LangNode.h"
#include "StringNumbers.h"
#include "Misc/StrProc.h"

#include <fmt/format.h>

namespace NLang
{

static std::unordered_map<int, std::string> typeToName;

static struct SInitTypeNames
{
	SInitTypeNames()
	{
		typeToName[EST_UNKNOWN] = "unknown";
		typeToName[EST_NOTYPE] = "notype";
		typeToName[EST_STRING] = "string";
		typeToName[EST_HEXBINARY] = "hexbinary";
		typeToName[EST_BOOL] = "bool";
		typeToName[EST_INT] = "int";
		typeToName[EST_FLOAT] = "float";
		typeToName[EST_WORD] = "WORD";
		typeToName[EST_DWORD] = "DWORD";
		typeToName[EST_ENUM] = "enum";
		typeToName[EST_WSTRING] = "wstring";
	};
} initTypeNames;

const char* GetTypeName( ESimpleType eType )
{
	return ( typeToName.find( eType ) != typeToName.end() ) ? typeToName[eType].c_str() : "unknown";
}

ESimpleType GetType( const std::string &szTypeName )
{
	for ( std::unordered_map<int, std::string>::iterator iter = typeToName.begin(); iter != typeToName.end(); ++iter )
	{
		const std::string &szType = iter->second;
		if ( szType == szTypeName )
			return ESimpleType( iter->first );
	}

	return EST_UNKNOWN;
}

static bool IsNum( ESimpleType eType )
{
	return eType == EST_INT || eType == EST_WORD || eType == EST_DWORD || eType == EST_FLOAT || eType == EST_HEXBINARY;
}

bool IsTypesEqual( ESimpleType eType1, ESimpleType eType2 )
{
	return	eType1 == eType2 || IsNum( eType1 ) && IsNum( eType2 ) ||
					eType1 == EST_STRING && eType2 == EST_WSTRING ||
					eType1 == EST_WSTRING && eType2 == EST_STRING;
}



void CSimpleValue::SetValue( const std::string &szDefValue, bool bString )
{
	if ( bString )
	{
		eType = EST_STRING;
		szValue = szDefValue;
	}
	else if ( szDefValue == "" )
		eType = EST_NOTYPE;
	else if ( szDefValue == "true" )
	{
		eType = EST_BOOL;
		bValue = true;
	}
	else if ( szDefValue == "false" )
	{
		eType = EST_BOOL;
		bValue = false;
	}
	else if ( NStr::IsHexNumber( szDefValue ) )
	{
		eType = EST_HEXBINARY;
		dwHexValue = NStr::ToInt( szDefValue );
	}
	else if ( NStr::IsInt( szDefValue ) )
	{
		eType = EST_INT;
		fValue = NStr::ToInt( szDefValue );
	}
	else if ( NStr::IsFloat( szDefValue ) )
	{
		eType = EST_FLOAT;
		fValue = NStr::ToFloat( szDefValue );
	}
	else if ( NStr::IsHexNumber( szDefValue ) )
	{
		eType = EST_HEXBINARY;
		dwHexValue = NStr::ToInt( szDefValue );
	}
	else
		VERIFY( false, fmt::format( "unknown attr value {}", szDefValue ), return );
}

}


