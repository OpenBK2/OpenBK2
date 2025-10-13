#include "stdafx.h"
#include "StructMetaInfo.h"
#include "BindArray.h"
#include "Bind.h"
#include "EditorDb.h"

#include <cstdint>

namespace NDb
{
namespace NMetaInfo
{

bool VariantToEnum( int *pRes, const CVariant &value, NTypeDef::STypeDef *pType )
{
	if ( value.GetType() == CVariant::VT_STR )
	{
		CVariant var;
		pType->FromString( &var, value.GetStr() );
		if ( var.GetType() != CVariant::VT_INT )
			return false;
		*pRes = (int)var;
		return true;
	}
	else if ( value.GetType() == CVariant::VT_INT )
	{
		*pRes = (int)value;
		return true;
	}
	return false;
}

bool EnumToVariant( CVariant *pRes, int nValue, NTypeDef::STypeDef *pType )
{
	std::string szValue;
	pType->ToString( &szValue, CVariant(nValue) );
	*pRes = szValue;
	return true;
}

// CRAP{ legacy - remove it ASAP
void CutTypeNameFromRef( std::string *pRes, const std::string &szRefName )
{
	const int nPos = szRefName.find( ':' );
	if ( nPos == std::string::npos )
	{
		if ( pRes != &szRefName )
			*pRes = szRefName;
	}
	else
		pRes->assign( szRefName.begin() + nPos + 1, szRefName.end() );
}
// CRAP}

CDBID GetDBIDFromValue( const CVariant &value )
{
	std::string szNewRefName;
	switch ( value.GetType() )
	{
	case CVariant::VT_DBID:
		// CRAP{ legacy - remove it ASAP
		CutTypeNameFromRef( &szNewRefName, value.GetDBID().ToString() );
		return CDBID( szNewRefName );
		// CRAP}
		//		*((CDBPtr<CResource>*)pData) = NDb::GetObject( value.GetDBID() );

	case CVariant::VT_STR:
		// CRAP{ legacy - remove it ASAP
		CutTypeNameFromRef( &szNewRefName, value.GetStr() );
		return CDBID( szNewRefName );
		// CRAP}
		//		*((CDBPtr<CResource>*)pData) = NDb::GetObject( CDBID(value.GetStr()) );
		break;

	case CVariant::VT_NULL:
	case CVariant::VT_INT:
		return CDBID();

	default:
		NI_ASSERT( false, "Can't convert type to DBID" );
		return CDBID();
	}
}
// ************************************************************************************************************************ //
// **
// ** set-to-struct functions
// **
// **
// **
// ************************************************************************************************************************ //

bool SStructMetaInfo::SField::SetValueToStructInt( const CVariant &value, uint8_t *pThis, NBind::UValue *ownValues )
{
	void *pData = pThis + GetBinaryShift();
	*((int*)pData) = (int)value;
	return true;
}
bool SStructMetaInfo::SField::SetValueToStructFloat( const CVariant &value, uint8_t *pThis, NBind::UValue *ownValues )
{
	void *pData = pThis + GetBinaryShift();
	*((float*)pData) = (float)value;
	return true;
}
bool SStructMetaInfo::SField::SetValueToStructBool( const CVariant &value, uint8_t *pThis, NBind::UValue *ownValues )
{
	void *pData = pThis + GetBinaryShift();
	*((bool*)pData) = (bool)value;
	return true;
}
bool SStructMetaInfo::SField::SetValueToStructString( const CVariant &value, uint8_t *pThis, NBind::UValue *ownValues )
{
	void *pData = pThis + GetBinaryShift();
	*((std::string*)pData) = value.GetStr();
	return true;
}
bool SStructMetaInfo::SField::SetValueToStructWString( const CVariant &value, uint8_t *pThis, NBind::UValue *ownValues )
{
	void *pData = pThis + GetBinaryShift();
	*((std::wstring*)pData) = value.GetWStringRecode();
	return true;
}
bool SStructMetaInfo::SField::SetValueToStructGUID( const CVariant &value, uint8_t *pThis, NBind::UValue *ownValues )
{
	if ( value.GetBlobSize() != sizeof(GUID) )
		return false;
	void *pData = pThis + GetBinaryShift();
	memcpy( pData, value.GetPtr(), sizeof(GUID) );
	return true;
}
bool SStructMetaInfo::SField::SetValueToStructBinary( const CVariant &value, uint8_t *pThis, NBind::UValue *ownValues )
{
	void *pData = pThis + GetBinaryShift();
	if ( int(main.size) == 4 && value.GetType() == CVariant::VT_INT )
	{
		*((int*)pData) = (int)value;
		return true;
	}
	else if ( value.GetBlobSize() == int(main.size) )
	{
		memcpy( pData, value.GetPtr(), value.GetBlobSize() );
		return true;
	}
	else
	{
		NI_ASSERT( false, StrFmt("Can't convert type %d to binary", value.GetType()) );
		return false;
	}
}
bool SStructMetaInfo::SField::SetValueToStructEnum( const CVariant &value, uint8_t *pThis, NBind::UValue *ownValues )
{
	void *pData = pThis + GetBinaryShift();
	return VariantToEnum( (int*)pData, value, pTypeDef );
}
bool SStructMetaInfo::SField::SetValueToStructDBID( const CVariant &value, uint8_t *pThis, NBind::UValue *ownValues )
{
	void *pData = pThis + GetBinaryShift();
	const CDBID dbid = GetDBIDFromValue( value );
	CResource *pRes = NDb::GetObject( dbid );
	if ( !dbid.IsEmpty() && pRes == 0 )
		return false;

	if ( pRes && pTypeDef )
	{
		if ( checked_cast_ptr<const NTypeDef::STypeRef *>(pTypeDef)->CheckRefType( pRes->GetTypeID() ) == false )
			return false;
	}

	*((CDBPtr<CResource>*)pData) = pRes;
	return true;
}

bool SStructMetaInfo::SField::SetValueArraySize( const CVariant &value, uint8_t *pThis, NBind::UValue *ownValues )
{
	NBind::UValue &data = ownValues[ GetOwnValueIndex() ];
	const int nSize = data.pArray->GetSize( *this, pThis );
	const int nDesiredSize = (std::max)( 0, (int)value );
	//
	if ( nDesiredSize == 0 )
		return data.pArray->Remove( 0, -1, *this, pThis );
	if ( nDesiredSize > nSize )
		return data.pArray->Insert( nSize, nDesiredSize - nSize, *this, pThis, true );
	if ( nDesiredSize < nSize )
		return data.pArray->Remove( nSize - nDesiredSize, -1, *this, pThis );
	return true;
}

// ************************************************************************************************************************ //
// **
// ** get-from-struct functions
// **
// **
// **
// ************************************************************************************************************************ //

bool SStructMetaInfo::SField::GetValueFromStructInt( CVariant *pValue, uint8_t *pThis, const NBind::UValue *ownValues ) const
{
	void *pData = pThis + GetBinaryShift();
	*pValue = *((int*)pData);
	return true;
}
bool SStructMetaInfo::SField::GetValueFromStructFloat( CVariant *pValue, uint8_t *pThis, const NBind::UValue *ownValues ) const
{
	void *pData = pThis + GetBinaryShift();
	*pValue = *((float*)pData);
	return true;
}
bool SStructMetaInfo::SField::GetValueFromStructBool( CVariant *pValue, uint8_t *pThis, const NBind::UValue *ownValues ) const
{
	void *pData = pThis + GetBinaryShift();
	*pValue = *((bool*)pData);
	return true;
}
bool SStructMetaInfo::SField::GetValueFromStructString( CVariant *pValue, uint8_t *pThis, const NBind::UValue *ownValues ) const
{
	void *pData = pThis + GetBinaryShift();
	*pValue = *((std::string*)pData);
	return true;
}
bool SStructMetaInfo::SField::GetValueFromStructWString( CVariant *pValue, uint8_t *pThis, const NBind::UValue *ownValues ) const
{
	void *pData = pThis + GetBinaryShift();
	*pValue = *((std::wstring*)pData);
	return true;
}
bool SStructMetaInfo::SField::GetValueFromStructGUIDorBinary( CVariant *pValue, uint8_t *pThis, const NBind::UValue *ownValues ) const
{
	void *pData = pThis + GetBinaryShift();
	*pValue = CVariant( pData, int(main.size) );
	return true;
}
bool SStructMetaInfo::SField::GetValueFromStructEnum( CVariant *pValue, uint8_t *pThis, const NBind::UValue *ownValues ) const
{
	void *pData = pThis + GetBinaryShift();
	return EnumToVariant( pValue, *((int*)pData), pTypeDef );
}
bool SStructMetaInfo::SField::GetValueFromStructDBID( CVariant *pValue, uint8_t *pThis, const NBind::UValue *ownValues ) const
{
	void *pData = pThis + GetBinaryShift();
	if ( const CResource *pObj = ((CDBPtr<CResource>*)pData)->GetPtrNoLoad() )
		*pValue = pObj->GetDBID();
	else
		*pValue = CDBID();
	return true;
}
bool SStructMetaInfo::SField::GetValueArraySize( CVariant *pValue, uint8_t *pThis, const NBind::UValue *ownValues ) const
{
	const NBind::UValue &data = ownValues[ GetOwnValueIndex() ];
	*pValue = data.pArray->GetSize( *this, pThis );
	return true;
}

// ************************************************************************************************************************ //
// **
// ** set-to-own functions
// **
// **
// **
// ************************************************************************************************************************ //

bool SStructMetaInfo::SField::SetValueToOwnInt( const CVariant &value, uint8_t *pThis, NBind::UValue *ownValues )
{
	NBind::UValue &data = ownValues[ GetOwnValueIndex() ];
	data.nValue = (int)value;
	return true;
}
bool SStructMetaInfo::SField::SetValueToOwnFloat( const CVariant &value, uint8_t *pThis, NBind::UValue *ownValues )
{
	NBind::UValue &data = ownValues[ GetOwnValueIndex() ];
	data.fValue = (float)value;
	return true;
}
bool SStructMetaInfo::SField::SetValueToOwnBool( const CVariant &value, uint8_t *pThis, NBind::UValue *ownValues )
{
	NBind::UValue &data = ownValues[ GetOwnValueIndex() ];
	data.bValue = (bool)value;
	return true;
}
bool SStructMetaInfo::SField::SetValueToOwnString( const CVariant &value, uint8_t *pThis, NBind::UValue *ownValues )
{
	NBind::UValue &data = ownValues[ GetOwnValueIndex() ];
	*data.pString = value.GetStr();
	return true;
}
bool SStructMetaInfo::SField::SetValueToOwnWString( const CVariant &value, uint8_t *pThis, NBind::UValue *ownValues )
{
	NBind::UValue &data = ownValues[ GetOwnValueIndex() ];
	*data.pWString = value.GetWStringRecode();
	return true;
}
bool SStructMetaInfo::SField::SetValueToOwnGUID( const CVariant &value, uint8_t *pThis, NBind::UValue *ownValues )
{
	if ( value.GetBlobSize() != sizeof(GUID) )
		return false;
	NBind::UValue &data = ownValues[ GetOwnValueIndex() ];
	memcpy( data.pGUID, value.GetPtr(), sizeof(GUID) );
	return true;
}
bool SStructMetaInfo::SField::SetValueToOwnBinary( const CVariant &value, uint8_t *pThis, NBind::UValue *ownValues )
{
	const int nBinarySize = checked_cast_ptr<NTypeDef::STypeBinary*>(pTypeDef)->nBinaryObjectSize;
	NBind::UValue &data = ownValues[ GetOwnValueIndex() ];
	if ( nBinarySize == 4 && value.GetType() == CVariant::VT_INT )
	{
		const int nValue = (int)value;
		memcpy( data.pBLOB, &nValue, sizeof(nValue) );
		return true;
	}
	else if ( value.GetBlobSize() == nBinarySize )
	{
		memcpy( data.pBLOB, value.GetPtr(), value.GetBlobSize() );
		return true;
	}
	else
	{
		NI_ASSERT( false, StrFmt("Can't convert type %d to binary", value.GetType()) );
		return false;
	}
	return true;
}
bool SStructMetaInfo::SField::SetValueToOwnEnum( const CVariant &value, uint8_t *pThis, NBind::UValue *ownValues )
{
	NBind::UValue &data = ownValues[ GetOwnValueIndex() ];
	return VariantToEnum( &data.nValue, value, pTypeDef );
}
bool SStructMetaInfo::SField::SetValueToOwnDBID( const CVariant &value, uint8_t *pThis, NBind::UValue *ownValues )
{
	NBind::UValue &data = ownValues[ GetOwnValueIndex() ];
	const CDBID dbid = GetDBIDFromValue( value );
	CPtr<IObjMan> pObjMan = NDb::GetManipulator( dbid );
	if ( !dbid.IsEmpty() && pObjMan == 0 )
		return false;
	
	if ( pObjMan && pTypeDef )
	{
		if ( NBind::CBindStruct *pBind = dynamic_cast_ptr<NBind::CBindStruct *>( pObjMan ) )
		{
			const int nClassTypeID = checked_cast_ptr<const NTypeDef::STypeClass *>( pBind->GetMetaInfo()->pStructTypeDef )->nClassTypeID;
			if ( checked_cast_ptr<const NTypeDef::STypeRef *>(pTypeDef)->CheckRefType( nClassTypeID ) == false )
				return false;
		}
	}

	data.pObjMan = pObjMan;
	return true;
}

// ************************************************************************************************************************ //
// **
// ** get-from-own functions
// **
// **
// **
// ************************************************************************************************************************ //

bool SStructMetaInfo::SField::GetValueFromOwnInt( CVariant *pValue, uint8_t *pThis, const NBind::UValue *ownValues ) const
{
	const NBind::UValue &data = ownValues[ GetOwnValueIndex() ];
	*pValue = data.nValue;
	return true;
}
bool SStructMetaInfo::SField::GetValueFromOwnFloat( CVariant *pValue, uint8_t *pThis, const NBind::UValue *ownValues ) const
{
	const NBind::UValue &data = ownValues[ GetOwnValueIndex() ];
	*pValue = data.fValue;
	return true;
}
bool SStructMetaInfo::SField::GetValueFromOwnBool( CVariant *pValue, uint8_t *pThis, const NBind::UValue *ownValues ) const
{
	const NBind::UValue &data = ownValues[ GetOwnValueIndex() ];
	*pValue = data.bValue;
	return true;
}
bool SStructMetaInfo::SField::GetValueFromOwnString( CVariant *pValue, uint8_t *pThis, const NBind::UValue *ownValues ) const
{
	const NBind::UValue &data = ownValues[ GetOwnValueIndex() ];
	*pValue = *data.pString;
	return true;
}
bool SStructMetaInfo::SField::GetValueFromOwnWString( CVariant *pValue, uint8_t *pThis, const NBind::UValue *ownValues ) const
{
	const NBind::UValue &data = ownValues[ GetOwnValueIndex() ];
	*pValue = *data.pWString;
	return true;
}
bool SStructMetaInfo::SField::GetValueFromOwnGUID( CVariant *pValue, uint8_t *pThis, const NBind::UValue *ownValues ) const
{
	const NBind::UValue &data = ownValues[ GetOwnValueIndex() ];
	*pValue = CVariant( data.pGUID, sizeof(GUID) );
	return true;
}
bool SStructMetaInfo::SField::GetValueFromOwnBinary( CVariant *pValue, uint8_t *pThis, const NBind::UValue *ownValues ) const
{
	const NBind::UValue &data = ownValues[ GetOwnValueIndex() ];
	*pValue = CVariant( data.pBLOB, int(main.size) );
	return true;
}
bool SStructMetaInfo::SField::GetValueFromOwnEnum( CVariant *pValue, uint8_t *pThis, const NBind::UValue *ownValues ) const
{
	const NBind::UValue &data = ownValues[ GetOwnValueIndex() ];
	return EnumToVariant( pValue, data.nValue, pTypeDef );
}
bool SStructMetaInfo::SField::GetValueFromOwnDBID( CVariant *pValue, uint8_t *pThis, const NBind::UValue *ownValues ) const
{
	const NBind::UValue &data = ownValues[ GetOwnValueIndex() ];
	if ( data.pObjMan )
		*pValue = data.pObjMan->GetDBID();
	else
		*pValue = CDBID();
	return true;
}

}
}
