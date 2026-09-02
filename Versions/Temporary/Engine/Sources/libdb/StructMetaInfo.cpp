#include "stdafx.h"

#include "StructMetaInfo.h"
#include "Variant.h"
#include "BindArray.h"
#include "ReportMetaInfo.h"

#include <cstdint>

#include <fmt/format.h>

namespace NDb
{

namespace NMetaInfo
{

// ************************************************************************************************************************ //
// **
// ** data constructions/destruction with meta-info
// **
// **
// **
// ************************************************************************************************************************ //

// Construct a field in place, in memory the record owns but has not built yet.
//
// Every case here has to match DestructBinary below, which is the one that says
// what each type really is: an array field is a std::vector, a ref field is a
// CDBPtr. Zeroing a type that is not trivially constructible is not the same as
// constructing it, even where the zeroed bytes happen to read as the empty
// value.
void ConstructBinary( uint8_t *pData, NTypeDef::ETypeType eType, int nSize )
{
	switch ( eType )
	{
	case NTypeDef::TYPE_TYPE_STRING:
		new(pData) std::string();
		break;

	case NTypeDef::TYPE_TYPE_WSTRING:
		new(pData) std::wstring();
		break;

	// An array field was falling through to the memset. A zeroed std::vector
	// reads as an empty one under a release STL, which is why this has stood, but
	// it has no iterator proxy, so a debug STL fails the first dereference of
	// anything begin() hands out with "can't dereference invalidated vector
	// iterator". std::vector<uint8_t> is the same spelling BindArray resizes
	// these through and DestructBinary destroys them as.
	case NTypeDef::TYPE_TYPE_ARRAY:
		new(pData) std::vector<uint8_t>();
		break;

	// This one was constructed and then immediately memset over, for want of a
	// break. Harmless while a default CDBPtr is all zeroes, and not something to
	// leave standing.
	case NTypeDef::TYPE_TYPE_REF:
		new(pData) CDBPtr<CResource>();
		break;

	default:
		memset( pData, 0, nSize );
	}
}
void DestructBinary( uint8_t *pData, NTypeDef::ETypeType eType, int nSize )
{
	switch ( eType )
	{
	case NTypeDef::TYPE_TYPE_STRING:
		((std::string*)pData)->~basic_string();
		break;

	case NTypeDef::TYPE_TYPE_WSTRING:
		((std::wstring*)pData)->~basic_string();
		break;

	case NTypeDef::TYPE_TYPE_ARRAY:
		((std::vector<uint8_t>*)pData)->~vector<uint8_t>();
		break;

	case NTypeDef::TYPE_TYPE_REF:
		((CDBPtr<CResource>*)pData)->~CDBPtr<CResource>();
		break;

	default:
		memset( pData, 0xdddddddd, nSize );
	}
}

void SStructMetaInfo::SField::ConstructBinary( uint8_t *pThis, NBind::UValue *values, bool bOnlyOwn ) const
{
	if ( pTypeDef == 0 )
		return;
	const int nOwnValueIndex = GetOwnValueIndex();
	if ( nOwnValueIndex != -1 )
	{
		NBind::UValue &data = values[nOwnValueIndex];
		switch ( GetType() )
		{
		case NTypeDef::TYPE_TYPE_STRING:
			data.pString = new std::string();
			break;

		case NTypeDef::TYPE_TYPE_WSTRING:
			data.pWString = new std::wstring();
			break;

		case NTypeDef::TYPE_TYPE_GUID:
			data.pGUID = new boost::uuids::uuid();
			break;

		case NTypeDef::TYPE_TYPE_BINARY:
			data.pBLOB = new uint8_t[ int(main.size) ];
			break;

		case NTypeDef::TYPE_TYPE_ARRAY:
			data.pArray = new NBind::CBindArray();
			break;
		}
	}
	//
	if ( !bOnlyOwn && GetBinaryShift() != 0x0000ffff )
		NMetaInfo::ConstructBinary( pThis + GetBinaryShift(), GetType(), main.size );
}

void SStructMetaInfo::SField::DestructBinary( uint8_t *pThis, NBind::UValue *values, bool bOnlyOwn ) const
{
	if ( pTypeDef == 0 )
		return;
	const int nOwnValueIndex = GetOwnValueIndex();
	if ( nOwnValueIndex != -1 )
	{
		NBind::UValue &data = values[nOwnValueIndex];
		switch ( GetType() )
		{
		case NTypeDef::TYPE_TYPE_STRING:
			delete data.pString;
			break;

		case NTypeDef::TYPE_TYPE_WSTRING:
			delete data.pWString;
			break;

		case NTypeDef::TYPE_TYPE_GUID:
			delete data.pGUID;
			break;

		case NTypeDef::TYPE_TYPE_BINARY:
			delete []data.pBLOB;
			break;

		case NTypeDef::TYPE_TYPE_ARRAY:
			data.pArray->Remove( 0, -1, *this, pThis );
			delete data.pArray;
			break;
		}
		memset( &data, 0, sizeof(data) );
	}
	//
	if ( !bOnlyOwn && GetBinaryShift() != 0x0000ffff )
		NMetaInfo::DestructBinary( pThis + GetBinaryShift(), GetType(), int(main.size) );
}

void SStructMetaInfo::ConstructStruct( uint8_t *pThis, NBind::UValue *values, bool bOnlyOwn )
{
	if ( fields.empty() )
		singleField.ConstructBinary( pThis, values, bOnlyOwn );
	else
	{
		for ( CFieldsMap::iterator it = fields.begin(); it != fields.end();  )
		{
			it->second.ConstructBinary( pThis, values, bOnlyOwn );
			//
			if ( it->second.pTypeDef == 0 )
				fields.erase( it++ );
			else
				++it;
		}
	}
}
// meta-destructor
void SStructMetaInfo::DestructStruct( uint8_t *pThis, NBind::UValue *values, bool bOnlyOwn )
{
	if ( fields.empty() )
		singleField.DestructBinary( pThis, values, bOnlyOwn );
	else
	{
		for ( CFieldsMap::iterator it = fields.begin(); it != fields.end(); ++it )
			it->second.DestructBinary( pThis, values, bOnlyOwn );
	}
}

// ************************************************************************************************************************ //
// **
// ** fields adding
// **
// **
// **
// ************************************************************************************************************************ //

void SStructMetaInfo::AddField( const std::string &szName, int nShift, int nSize, NTypeDef::ETypeType eType )
{
	NI_ASSERT( fields.find(szName) == fields.end(), fmt::format("Field \"{}\" already exists!", szName ) );
	SField &field = fields[szName];
	field.nShift = nShift;
	field.main.type = eType;
	field.main.size = nSize;
	field.contained.type = NTypeDef::TYPE_TYPE_UNKNOWN;
	field.contained.size = 0;
#ifdef FAST_DEBUG
	field.szFieldName = szName;
#endif
}

void SStructMetaInfo::AddField( const std::string &szName, int nShift, int nSize, NTypeDef::ETypeType eType,
							                  int nContainedSize, NTypeDef::ETypeType eContainedType )
{
	NI_ASSERT( fields.find(szName) == fields.end(), fmt::format("Field \"{}\" already exists!", szName ) );
	SField &field = fields[szName];
	field.nShift = nShift;
	field.main.type = eType;
	field.main.size = nSize;
	field.contained.type = eContainedType;
	field.contained.size = nContainedSize;
	SStructMetaInfo *pContained = new SStructMetaInfo();
	field.pContained = pContained;
	pContained->nStructSize = nContainedSize;
#ifdef FAST_DEBUG
	field.szFieldName = szName;
#endif
	if ( eContainedType == NTypeDef::TYPE_TYPE_STRUCT )
		AddOnStack( pContained );
	else
	{
//		pContained->singleField.nShift = 0;
		pContained->singleField.main.type = eContainedType;
		pContained->singleField.main.size = nContainedSize;
		memset( &pContained->singleField.contained, 0, sizeof(pContained->singleField.contained) );
	}
}

// ************************************************************************************************************************ //
// **
// ** Deep copying
// **
// **
// **
// ************************************************************************************************************************ //

void SStructMetaInfo::SField::MakeDeepCopy( SStructMetaInfo::SField *pRes ) const
{
	pRes->nShift = nShift;
	memcpy( &pRes->main, &main, sizeof(main) );
	memcpy( &pRes->contained, &contained, sizeof(contained) );
	if ( pContained )
	{
		pRes->pContained = new SStructMetaInfo();
		pContained->MakeDeepCopy( pRes->pContained );
	}
	pRes->pTypeDef = pTypeDef;
#ifdef FAST_DEBUG
	pRes->szFieldName = szFieldName;
#endif
	//
	pRes->pfnSetValue = pfnSetValue;
	pRes->pfnGetValue = pfnGetValue;
}

void SStructMetaInfo::MakeDeepCopy( SStructMetaInfo *pRes ) const
{
	pRes->nStructSize = nStructSize;
	pRes->nNumOwnValues = nNumOwnValues;
	pRes->nNumCodeValues = nNumCodeValues;
	pRes->pStructTypeDef = pStructTypeDef;
	//
	singleField.MakeDeepCopy( &pRes->singleField );
	for ( CFieldsMap::const_iterator it = fields.begin(); it != fields.end(); ++it )
	{
		it->second.MakeDeepCopy( &(pRes->fields[it->first]) );
	}
}

// ************************************************************************************************************************ //
// **
// ** linking with typedef
// **
// **
// **
// ************************************************************************************************************************ //

void SetupSetGetOwnFuncs( SStructMetaInfo::SField *pField, NTypeDef::ETypeType eType )
{
	switch ( eType )
	{
	case NTypeDef::TYPE_TYPE_INT:
		pField->pfnSetValue = &SStructMetaInfo::SField::SetValueToOwnInt;
		pField->pfnGetValue = &SStructMetaInfo::SField::GetValueFromOwnInt;
		break;

	case NTypeDef::TYPE_TYPE_FLOAT:
		pField->pfnSetValue = &SStructMetaInfo::SField::SetValueToOwnFloat;
		pField->pfnGetValue = &SStructMetaInfo::SField::GetValueFromOwnFloat;
		break;

	case NTypeDef::TYPE_TYPE_BOOL:
		pField->pfnSetValue = &SStructMetaInfo::SField::SetValueToOwnBool;
		pField->pfnGetValue = &SStructMetaInfo::SField::GetValueFromOwnBool;
		break;

	case NTypeDef::TYPE_TYPE_STRING:
		pField->pfnSetValue = &SStructMetaInfo::SField::SetValueToOwnString;
		pField->pfnGetValue = &SStructMetaInfo::SField::GetValueFromOwnString;
		break;

	case NTypeDef::TYPE_TYPE_WSTRING:
		pField->pfnSetValue = &SStructMetaInfo::SField::SetValueToOwnWString;
		pField->pfnGetValue = &SStructMetaInfo::SField::GetValueFromOwnWString;
		break;

	case NTypeDef::TYPE_TYPE_GUID:
		pField->pfnSetValue = &SStructMetaInfo::SField::SetValueToOwnGUID;
		pField->pfnGetValue = &SStructMetaInfo::SField::GetValueFromOwnGUID;
		break;

	case NTypeDef::TYPE_TYPE_BINARY:
		pField->pfnSetValue = &SStructMetaInfo::SField::SetValueToOwnBinary;
		pField->pfnGetValue = &SStructMetaInfo::SField::GetValueFromOwnBinary;
		break;

	case NTypeDef::TYPE_TYPE_ENUM:
		pField->pfnSetValue = &SStructMetaInfo::SField::SetValueToOwnEnum;
		pField->pfnGetValue = &SStructMetaInfo::SField::GetValueFromOwnEnum;
		break;

	case NTypeDef::TYPE_TYPE_REF:
		pField->pfnSetValue = &SStructMetaInfo::SField::SetValueToOwnDBID;
		pField->pfnGetValue = &SStructMetaInfo::SField::GetValueFromOwnDBID;
		break;

	default:
		NI_ASSERT( false, fmt::format("Unknown terminal type {}", int( eType )) );
	}
}
void SetupSetGetStructFuncs( SStructMetaInfo::SField *pField, NTypeDef::ETypeType eType )
{
	switch ( eType )
	{
	case NTypeDef::TYPE_TYPE_INT:
		pField->pfnSetValue = &SStructMetaInfo::SField::SetValueToStructInt;
		pField->pfnGetValue = &SStructMetaInfo::SField::GetValueFromStructInt;
		break;

	case NTypeDef::TYPE_TYPE_FLOAT:
		pField->pfnSetValue = &SStructMetaInfo::SField::SetValueToStructFloat;
		pField->pfnGetValue = &SStructMetaInfo::SField::GetValueFromStructFloat;
		break;

	case NTypeDef::TYPE_TYPE_BOOL:
		pField->pfnSetValue = &SStructMetaInfo::SField::SetValueToStructBool;
		pField->pfnGetValue = &SStructMetaInfo::SField::GetValueFromStructBool;
		break;

	case NTypeDef::TYPE_TYPE_STRING:
		pField->pfnSetValue = &SStructMetaInfo::SField::SetValueToStructString;
		pField->pfnGetValue = &SStructMetaInfo::SField::GetValueFromStructString;
		break;

	case NTypeDef::TYPE_TYPE_WSTRING:
		pField->pfnSetValue = &SStructMetaInfo::SField::SetValueToStructWString;
		pField->pfnGetValue = &SStructMetaInfo::SField::GetValueFromStructWString;
		break;

	case NTypeDef::TYPE_TYPE_GUID:
		pField->pfnSetValue = &SStructMetaInfo::SField::SetValueToStructGUID;
		pField->pfnGetValue = &SStructMetaInfo::SField::GetValueFromStructGUIDorBinary;
		break;

	case NTypeDef::TYPE_TYPE_BINARY:
		pField->pfnSetValue = &SStructMetaInfo::SField::SetValueToStructBinary;
		pField->pfnGetValue = &SStructMetaInfo::SField::GetValueFromStructGUIDorBinary;
		break;

	case NTypeDef::TYPE_TYPE_ENUM:
		pField->pfnSetValue = &SStructMetaInfo::SField::SetValueToStructEnum;
		pField->pfnGetValue = &SStructMetaInfo::SField::GetValueFromStructEnum;
		break;

	case NTypeDef::TYPE_TYPE_REF:
		pField->pfnSetValue = &SStructMetaInfo::SField::SetValueToStructDBID;
		pField->pfnGetValue = &SStructMetaInfo::SField::GetValueFromStructDBID;
		break;

	default:
		NI_ASSERT( false, fmt::format("Unknown terminal type {}", int( eType )) );
	}
}

void SStructMetaInfo::LinkField( const std::string &szAddName, const std::string &szFieldName, NTypeDef::STypeDef *pType )
{
	if ( pType->IsSimpleType() )
	{
		const std::string szFullFieldName = szAddName.empty() ? szFieldName : szAddName + "." + szFieldName;
		CFieldsMap::iterator posField = fields.find( szFullFieldName );
		if ( posField == fields.end() )
		{
			// add nocode (own) value
			AddField( szFullFieldName, (uint32_t(nNumOwnValues) << 16) | 0x8000ffff, pType->GetTypeSize(), pType->eType );
			posField = fields.find( szFullFieldName );
			NI_ASSERT( posField != fields.end(), fmt::format("New field \"{}\" was not added", szFullFieldName) );
			SetupSetGetOwnFuncs( &posField->second, pType->eType );
			++nNumOwnValues;
//			DebugTrace( "\tNew nocode field \"%s\" of type %d added!", szFullFieldName.c_str(), pType->eType );
		}
		else
		{
			SetupSetGetStructFuncs( &posField->second, pType->eType );
			++nNumCodeValues;
		}
		posField->second.pTypeDef = pType;
		NI_ASSERT( pType->GetTypeSize() == int(posField->second.main.size), fmt::format("Size mismatch in struct meta info and type definition for \"{}\"", szFullFieldName) );
//		DebugTrace( "\tField \"%s\" of type %d linked!", szFullFieldName.c_str(), pType->eType );
		if ( posField->second.GetType() != pType->eType )
		{
			NI_ASSERT( posField->second.GetType() == pType->eType, fmt::format("Different types in structure and meta-info ({} != {}) field \"{}\". Removing field!", int( posField->second.GetType() ), int( pType->eType ), szFullFieldName) );
			fields.erase( posField );
		}
	}
	else if ( pType->eType == NTypeDef::TYPE_TYPE_STRUCT )
	{
		const std::string szNewAddName = szAddName.empty() ? szFieldName : szAddName + "." + szFieldName;
		LinkWithTypeDef( szNewAddName, checked_cast<NTypeDef::STypeStructBase*>(pType) );
	}
	else if ( pType->eType == NTypeDef::TYPE_TYPE_ARRAY )
	{
		NTypeDef::STypeArray *pTypeArray = checked_cast<NTypeDef::STypeArray*>( pType );
		NI_ASSERT( pTypeArray->field.pType->eType != NTypeDef::TYPE_TYPE_BOOL, "Don't use array of bool!!!" );
		const std::string szFullFieldName = szAddName.empty() ? szFieldName : szAddName + "." + szFieldName;
		CFieldsMap::iterator posField = fields.find( szFullFieldName );
		if ( posField == fields.end() )	// array of owns
		{
			// add nocode (own) array
			AddField( szFullFieldName, (uint32_t(nNumOwnValues) << 16) | 0x8000ffff, pType->GetTypeSize(), pType->eType,
				        pTypeArray->field.pType->GetTypeSize(), pTypeArray->field.pType->eType );
			if ( pTypeArray->field.pType->eType == NTypeDef::TYPE_TYPE_STRUCT )
				FinishMetaInfoReport();
			posField = fields.find( szFullFieldName );
			//
			if ( pTypeArray->field.pType->eType == NTypeDef::TYPE_TYPE_STRUCT )
			{
				posField->second.pContained->LinkWithTypeDef( "", 
					checked_cast_ptr<NTypeDef::STypeStruct*>(pTypeArray->field.pType) );
//				NI_ASSERT( false, "Still not realized!" );
			}
			else
			{
				SetupSetGetOwnFuncs( &posField->second.pContained->singleField, pTypeArray->field.pType->eType );
				posField->second.pContained->singleField.nShift = 0x8000ffff;
				posField->second.pContained->nNumOwnValues = 1;
				posField->second.pContained->singleField.pTypeDef = pTypeArray->field.pType;
		}
			//
			NI_ASSERT( posField != fields.end(), fmt::format("New field \"{}\" was not added", szFullFieldName) );
		}
		else
		{
			if ( posField->second.contained.type == NTypeDef::TYPE_TYPE_STRUCT )
			{
				// array of complex structs
				posField->second.pContained->LinkWithTypeDef( "", 
					checked_cast_ptr<NTypeDef::STypeStruct*>(pTypeArray->field.pType) );
			}
			else
			{
				// array of simple values
				SetupSetGetStructFuncs( &posField->second.pContained->singleField, pTypeArray->field.pType->eType );
				posField->second.pContained->singleField.nShift = 0;
				posField->second.pContained->nNumCodeValues = 1;
				posField->second.pContained->singleField.pTypeDef = pTypeArray->field.pType;
			}
			//
			posField->second.nShift = int( ( uint32_t(nNumOwnValues) << 16 ) | 0x80000000 | ( posField->second.nShift & 0x0000ffff ) );
			++nNumCodeValues;
		}
		//
		posField->second.pTypeDef = pTypeArray;
		posField->second.pfnSetValue = &SField::SetValueArraySize;
		posField->second.pfnGetValue = &SField::GetValueArraySize;
		// array has it's own value regardless to code
		++nNumOwnValues;
	}
	else
	{
		NI_ASSERT( 0, fmt::format("Unknown or unsupported type \"{}\"!", SKnownEnum<NDb::NTypeDef::ETypeType>::ToString(pType->eType)) );
	}
}

void SStructMetaInfo::LinkWithTypeDef( const std::string &szAddName, NTypeDef::STypeStructBase *pType )
{
	if ( pType->pBaseType )
		LinkWithTypeDef( szAddName, pType->pBaseType );
	//
	for ( NTypeDef::STypeStructBase::CFieldsList::iterator it = pType->fields.begin(); it != pType->fields.end(); ++it )
		LinkField( szAddName, it->szName, it->pType );
	//
	pStructTypeDef = pType;
}

}

};


