#include "stdafx.h"
#include "TypeDef.h"
#include "Misc/StrProc.h"

#include "libdb_export.h"

#include <cstdint>

#include <fmt/format.h>

int CRAPTooSmartCompiler_DBTools_TypeDef()
{
	return 0;
}

namespace NDb
{
namespace NTypeDef
{

// ************************************************************************************************************************ //
// **
// ** string <=> variant convertors
// **
// **
// **
// ************************************************************************************************************************ //

void STypeInt::ToString( std::string *pRes, const CVariant &value ) const
{
	*pRes = std::to_string(  (int)value );
}
void STypeInt::FromString( CVariant *pRes, const std::string &szValue ) const
{
	*pRes = NStr::ToInt( szValue );
}

void STypeFloat::ToString( std::string *pRes, const CVariant &value ) const
{
	*pRes = fmt::format( "{:g}", (float)value );
}
void STypeFloat::FromString( CVariant *pRes, const std::string &szValue ) const
{
	*pRes = NStr::ToFloat( szValue );
}

void STypeBool::ToString( std::string *pRes, const CVariant &value ) const
{
	*pRes = (bool)value == false ? "false" : "true";
}
void STypeBool::FromString( CVariant *pRes, const std::string &szValue ) const
{
	if ( szValue.empty() || szValue[0] == '0' || szValue == "false" )
		*pRes = false;
	else if ( szValue[0] == '1' || szValue == "true" )
		*pRes = true;
	else
	{
		NI_ASSERT( false, fmt::format("Unknown string value \"{}\" for 'bool' type (can be: 0, 1, true, false)", szValue) );
		*pRes = false;
	}
}

void STypeString::ToString( std::string *pRes, const CVariant &value ) const
{
	*pRes = value.GetStr();
}
void STypeString::FromString( CVariant *pRes, const std::string &szValue ) const
{
	*pRes = szValue;
}

void STypeWString::ToString( std::string *pRes, const CVariant &value ) const
{
	NStr::UnicodeToUTF8( pRes, value.GetWStr() );
}
void STypeWString::FromString( CVariant *pRes, const std::string &szValue ) const
{
	std::wstring wszRes;
	NStr::UTF8ToUnicode( &wszRes, szValue );
	*pRes = wszRes;
}

void STypeGUID::ToString( std::string *pRes, const CVariant &value ) const
{
	NStr::GUID2String( pRes, *((GUID*)value.GetPtr()) );
}
void STypeGUID::FromString( CVariant *pRes, const std::string &szValue ) const
{
	GUID guid;
	NStr::String2GUID( szValue, &guid );
	*pRes = CVariant( &guid, sizeof(guid) );
}

void STypeBinary::ToString( std::string *pRes, const CVariant &value ) const
{
	NI_ASSERT( value.GetBlobSize() == nBinaryObjectSize, "Wrong binary object size" );
	const int nSize = (std::min)( (int)value.GetBlobSize(), nBinaryObjectSize );
	NI_VERIFY( nSize > 0, "Binary object size can't have size < 0", return );
	pRes->resize( nSize * 2 );
	NStr::BinToString( value.GetPtr(), nSize, (char*)pRes->data() );
}
void STypeBinary::FromString( CVariant *pRes, const std::string &szValue ) const
{
	const int nSize = szValue.size() / 2;
	uint8_t *pData = new uint8_t[nSize];
	NStr::StringToBin( szValue.c_str(), pData, 0 );
	*pRes = CVariant( pData, nSize );
	delete []pData;
}

void STypeEnum::ToString( std::string *pRes, const CVariant &value ) const
{
	if ( value.GetType() == CVariant::VT_STR )
	{
		*pRes = value.GetStr();
		return;
	}
	else if ( value.GetType() == CVariant::VT_INT )
	{
		const int nValue = value;
		for ( std::vector<SEnumEntry>::const_iterator it = entries.begin(); it != entries.end(); ++it )
		{
			if ( it->nVal == nValue )
			{
				*pRes = it->szName;
				return;
			}
		}
	}
	*pRes = entries.empty() ? "" : entries[0].szName;
}
void STypeEnum::FromString( CVariant *pRes, const std::string &szValue ) const
{
	for ( std::vector<SEnumEntry>::const_iterator it = entries.begin(); it != entries.end(); ++it )
	{
		if ( it->szName == szValue )
		{
			*pRes = it->nVal;
			return;
		}
	}
	*pRes = entries.empty() ? CVariant() : entries[0].nVal;
}

void STypeArray::ToString( std::string *pRes, const CVariant &value ) const
{
	*pRes = std::to_string(  (int)value );
}
void STypeArray::FromString( CVariant *pRes, const std::string &szValue ) const
{
	*pRes = NStr::ToInt( szValue );
}

void STypeRef::ToString( std::string *pRes, const CVariant &value ) const
{
	NI_VERIFY( value.GetType() == CVariant::VT_DBID || value.GetType() == CVariant::VT_NULL, fmt::format("Can't convert type {} to DBID", int( value.GetType() )), return );
	if ( value.GetType() == CVariant::VT_NULL )
		pRes->clear();
	else
		*pRes = value.GetDBID().ToString();
}
void STypeRef::FromString( CVariant *pRes, const std::string &szValue ) const
{
	*pRes = CDBID( szValue );
}
void STypeRef::GetRefTypesList( std::vector<const STypeClass *> *pTypesList ) const
{
	if ( const STypeClass *pTypeClass = dynamic_cast_ptr<const STypeClass *>(pRefType) )
	{
		pTypesList->resize( 0 );
		if ( pTypeClass->derivedTerminalTypes.empty() )
			pTypesList->push_back( pTypeClass );
		else
		{
			pTypesList->reserve( pTypeClass->derivedTerminalTypes.size() );
			pTypesList->insert( pTypesList->end(), pTypeClass->derivedTerminalTypes.begin(), pTypeClass->derivedTerminalTypes.end() );
		}
	}
}

bool STypeRef::CheckRefType( const int nClassTypeID ) const
{
	if ( const STypeClass *pTypeClass = dynamic_cast_ptr<const STypeClass *>(pRefType) )
	{
		if ( pTypeClass->derivedTerminalTypes.empty() )
			return pTypeClass->nClassTypeID == nClassTypeID;
		else
		{
			for ( STypeClass::CDerivedTerminalTypesList::const_iterator it = pTypeClass->derivedTerminalTypes.begin(); 
				    it != pTypeClass->derivedTerminalTypes.end(); ++it )
			{
				if ( (*it)->nClassTypeID == nClassTypeID )
					return true;
			}
		}
	}
	return false;
}

// ************************************************************************************************************************ //
// **
// ** 
// **
// **
// **
// ************************************************************************************************************************ //

CVariant STypeGUID::GetDefaultValue() const 
{ 
	GUID guid;
	CoCreateGuid( &guid );
	return guid;
}

CVariant STypeBinary::GetDefaultValue() const 
{ 
	std::vector<uint8_t> buffer( nBinaryObjectSize );
	fill( buffer.begin(), buffer.end(), 0 );
	CVariant var( &(buffer[0]), nBinaryObjectSize );
	return var; 
}

// ************************************************************************************************************************ //
// **
// ** 
// **
// **
// **
// ************************************************************************************************************************ //

void STypeStructBase::AddField( STypeDef *pType, 
																const std::string &szName,
																const int nChunkID, 
																const std::wstring &wszDesc,
							                  SConstraints *pConstraints, 
																SAttributes *pAttributes,
																const CVariant &_vtDefVal )
{
	CFieldsList::iterator posField = fields.insert( fields.end(), SField() );
	posField->pType = pType;
	posField->szName = szName;
	posField->nChunkID = nChunkID;
	posField->wszDesc = wszDesc;
	posField->constraints.push_back( pConstraints );
	posField->pAttributes = pAttributes;
	posField->defaultValue = _vtDefVal;
}

bool STypeStructBase::SField::CheckValueCorrect( const CVariant &value ) const
{ 
	for ( int i = 0; i < constraints.size(); ++i )
	{
		if ( !constraints[i]->CheckValueCorrect( value ) )
			return false;
	}
	return true;
}

EEditorType STypeStructBase::SField::GetEditorType() const
{
	return pType->GetDefaultEditorType();
	/*
	if ( pAttributes != 0 && pAttributes->attributes.find( "editorControl" ) != pAttributes->attributes.end() )
		return GetEditorType( pAttributes->attributes["editorControl"] );
	else
		return pType->GetDefaultEditorType();
	*/
}

CVariant STypeStructBase::SField::GetDefaultValue() const
{
	if ( defaultValue.GetType() != CVariant::VT_NULL || !pType->IsSimpleType() )
		return defaultValue;
	else
		return checked_cast_ptr<STypeSimple *>(pType)->GetDefaultValue();
}

bool STypeStructBase::SField::HasAttribute( const std::string &szName ) const
{
	// check field's attribs
	if ( pAttributes && pAttributes->attributes.find( szName ) != pAttributes->attributes.end() )
		return true;
	// check type's attribs
	if ( pType->eType == TYPE_TYPE_BINARY )
	{
		const STypeBinary *pTypeBinary = checked_cast_ptr<const STypeBinary *>( pType );
		if ( pTypeBinary->pAttributes && pTypeBinary->pAttributes->attributes.find(szName) != pTypeBinary->pAttributes->attributes.end() )
			return true;
	}
	else if ( pType->eType == TYPE_TYPE_STRUCT )
	{
		const STypeStructBase *pTypeStruct = checked_cast_ptr<const STypeStructBase *>( pType );
		if ( pTypeStruct->pAttributes && pTypeStruct->pAttributes->attributes.find(szName) != pTypeStruct->pAttributes->attributes.end() )
			return true;
	}
	return false;
}

const CVariant *STypeStructBase::SField::GetAttribute( const std::string &szName ) const
{
	// check field's attribs
	if ( pAttributes )
	{
		std::unordered_map<std::string, CVariant>::const_iterator pos = pAttributes->attributes.find( szName );
		if ( pos != pAttributes->attributes.end() )
			return &( pos->second );
	}
	// check type's attribs
	if ( pType->eType == TYPE_TYPE_BINARY )
	{
		const STypeBinary *pTypeBinary = checked_cast_ptr<const STypeBinary *>( pType );
		if ( pTypeBinary->pAttributes )
		{
			std::unordered_map<std::string, CVariant>::const_iterator pos = pTypeBinary->pAttributes->attributes.find( szName );
			if ( pos != pTypeBinary->pAttributes->attributes.end() )
				return &( pos->second );
		}
	}
	else if ( pType->eType == TYPE_TYPE_STRUCT )
	{
		const STypeStructBase *pTypeStruct = checked_cast_ptr<const STypeStructBase *>( pType );
		if ( pTypeStruct->pAttributes )
		{
			std::unordered_map<std::string, CVariant>::const_iterator pos = pTypeStruct->pAttributes->attributes.find( szName );
			if ( pos != pTypeStruct->pAttributes->attributes.end() )
				return &( pos->second );
		}
	}
	return 0;
}

void STypeClass::RegisterTerminalType( STypeClass *pClass )
{
	if ( nClassTypeID == -1 )
	{
		if ( pClass != 0 )
		{
			derivedTerminalTypes.push_back( pClass );
			if ( STypeClass *pBaseClass = checked_cast_ptr<STypeClass*>( pBaseType ) )
				pBaseClass->RegisterTerminalType( pClass );
		}
	}
	else
	{
		NI_ASSERT( derivedTerminalTypes.empty(), fmt::format("Terminal class {} has derived classes!!!", szTypeName) );
		if ( STypeClass *pBaseClass = checked_cast_ptr<STypeClass*>( pBaseType ) )
			pBaseClass->RegisterTerminalType( this );
	}
}

}
}

using namespace NDb::NTypeDef;
REGISTER_SAVELOAD_CLASS( LIBDB, 0x1018DAC7, SAttributes );

BASIC_REGISTER_CLASS( LIBDB, STypeDef );
REGISTER_SAVELOAD_CLASS( LIBDB, 0x10192300, STypeInt );
REGISTER_SAVELOAD_CLASS( LIBDB, 0x10192301, STypeFloat );
REGISTER_SAVELOAD_CLASS( LIBDB, 0x10192302, STypeBool );
REGISTER_SAVELOAD_CLASS( LIBDB, 0x10192303, STypeString );
REGISTER_SAVELOAD_CLASS( LIBDB, 0x10192304, STypeWString );
REGISTER_SAVELOAD_CLASS( LIBDB, 0x10192305, STypeGUID );
REGISTER_SAVELOAD_CLASS( LIBDB, 0x10192307, STypeBinary );
REGISTER_SAVELOAD_CLASS( LIBDB, 0x10192308, STypeEnum );
REGISTER_SAVELOAD_CLASS( LIBDB, 0x10192309, STypeStruct );
REGISTER_SAVELOAD_CLASS( LIBDB, 0x1019230A, STypeClass );
REGISTER_SAVELOAD_CLASS( LIBDB, 0x1019230B, STypeArray );
REGISTER_SAVELOAD_CLASS( LIBDB, 0x1019230C, STypeRef );

REGISTER_SAVELOAD_CLASS( LIBDB, 0x101A7B40, SConstraintsUnlimited );
REGISTER_SAVELOAD_TEMPL_CLASS( LIBDB, 0x1018D400, SConstraintsMinMaxInt, SConstraintsMinMax );
REGISTER_SAVELOAD_TEMPL_CLASS( LIBDB, 0x1018D401, SConstraintsMinMaxFloat, SConstraintsMinMax );
REGISTER_SAVELOAD_TEMPL_CLASS( LIBDB, 0x1018D402, SConstraintsValuesListInt, SConstraintsValuesList );
REGISTER_SAVELOAD_TEMPL_CLASS( LIBDB, 0x1018D403, SConstraintsValuesListFloat, SConstraintsValuesList );
REGISTER_SAVELOAD_CLASS( LIBDB, 0x1018D404, SConstraintsArrayMinMax );
REGISTER_SAVELOAD_CLASS( LIBDB, 0x1018D405, SConstraintsString );
REGISTER_SAVELOAD_CLASS( LIBDB, 0x10191A80, SConstraintsTypeRef );


