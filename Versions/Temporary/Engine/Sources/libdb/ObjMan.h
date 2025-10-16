#pragma once

#include "Variant.h"
#include "TypeDef.h"

#include <fmt/format.h>

namespace NDb
{
struct IObjManIterator;
struct SObjectHeader;

#define MAN_REMOVE_LAST -1, 1
#define MAN_REMOVE_ALL 0, -1
#define MAN_APPEND -1, 1, true

struct IObjMan : public CObjectBase
{
	//
	virtual void SetChanged() = 0;
	// create mask manipulator for sub-struct, array, array element, etc.
	virtual IObjMan *CreateManipulator( const std::string &szBaseName ) = 0;
	// create iterator to iterate through all object's properties
	virtual IObjManIterator *CreateIterator( bool bShowHidden = false ) = 0;
	// get full 'add name' for this manipulator
	virtual std::string GetFullName() const = 0;
	// main fields manipulation functions
	virtual bool SetValue( const std::string &szName, const CVariant &value ) = 0;
	virtual bool GetValue( const std::string &szName, CVariant *pValue ) = 0;
	// array-specific functions
	virtual bool Insert( const std::string &szName, const int nPos, const int nAmount = 1, bool bSetDefault = false ) = 0;
	virtual bool Remove( const std::string &szName, const int nPos, const int nAmount = 1 ) = 0;
	// get property field descriptor by name
	virtual const NTypeDef::STypeStructBase::SField *GetDesc( const std::string &szFullFieldName ) const = 0;
	// direct access to embedded struct (if it is)
	virtual CResource *GetObject() = 0;
	//
	virtual const CDBID &GetDBID() const = 0;
	// additional custom attributes
	virtual std::wstring GetAttribute( const std::string &szName ) const = 0;
	virtual void SetAttribute( const std::string &szName, const std::wstring &szValue ) = 0;

	//
	// fields manipulation helper functions
	//
	// 'set' family
	template <class TYPE>
		bool SetValue( const std::string &szName, const TYPE &value )
	{
		return SetValue( szName, CVariant( value ) );
	}
	template <>
		bool SetValue<CVec2>( const std::string &szName, const CVec2 &value )
	{
		return SetValue( szName + ".x", CVariant( value.x ) ) &&
			SetValue( szName + ".y", CVariant( value.y ) );
	}
	template <>
		bool SetValue<CVec3>( const std::string &szName, const CVec3 &value )
	{
		return SetValue( szName + ".x", CVariant( value.x ) ) &&
			SetValue( szName + ".y", CVariant( value.y ) ) &&
			SetValue( szName + ".z", CVariant( value.z ) );
	}
	template <>
		bool SetValue<CVec4>( const std::string &szName, const CVec4 &value )
	{
		return SetValue( szName + ".x", CVariant( value.x ) ) &&
			SetValue( szName + ".y", CVariant( value.y ) ) &&
			SetValue( szName + ".z", CVariant( value.z ) ) &&
			SetValue( szName + ".w", CVariant( value.w ) );
	}
	template <>
		bool SetValue<CQuat>( const std::string &szName, const CQuat &value )
	{
		return SetValue( szName, value.GetInternalVector() );
	}
	//
	template <template <typename TYPE> class TContainer, typename TValue>
		bool SetValue( const std::string &szName, const TContainer<TValue> &container )
	{
		if ( SetValue( szName, int( container.size() ) ) == false )
			return false;
		int i = 0;
		for ( TContainer<TValue>::const_iterator it = container.begin(); it != container.end(); ++it, ++i )
		{
			if ( SetValue(szName + fmt::format(".[{}]", i), *it) == false )
				return false;
		}
		return true;
	}
	template <>
		bool SetValue<std::basic_string, char>( const std::string &szName, const std::basic_string<char> &value )
	{
		return SetValue( szName, CVariant( value ) );
	}
	template <>
		bool SetValue<std::basic_string, wchar_t>( const std::string &szName, const std::basic_string<wchar_t> &value )
	{
		return SetValue( szName, CVariant( value ) );
	}
	// 'get' family
	template <class TYPE>
		bool GetValue( const std::string &szName, TYPE *pValue )
	{
		CVariant value;
		if ( GetValue( szName, &value ) == false )
			return false;
		*pValue = (TYPE)value;
		return true;
	}
	template <>
		bool GetValue<std::string>( const std::string &szName, std::string *pValue )
	{
		CVariant value;
		if ( GetValue( szName, &value ) == false )
			return false;
		*pValue = value.GetStr();
		return true;
	}
	template <>
		bool GetValue<std::wstring>( const std::string &szName, std::wstring *pValue )
	{
		CVariant value;
		if ( GetValue( szName, &value ) == false )
			return false;
		*pValue = value.GetWStr();
		return true;
	}
	template <>
		bool GetValue<GUID>( const std::string &szName, GUID *pValue )
	{
		CVariant value;
		if ( GetValue( szName, &value ) == false )
			return false;
		NI_VERIFY( value.GetType() == CVariant::VT_POINTER && value.GetBlobSize() == sizeof(GUID), "Incorrect BLOB for GUID", return false );
		memcpy( pValue, value.GetPtr(), sizeof(GUID) );
		return true;
	}
	template <>
		bool GetValue<CDBID>( const std::string &szName, CDBID *pValue )
	{
		CVariant value;
		if ( GetValue( szName, &value ) == false )
			return false;
		*pValue = value.GetDBID();
		return true;
	}
	template <>
		bool GetValue<CVec2>( const std::string &szName, CVec2 *pValue )
	{
		CVariant value;
		if ( GetValue( szName + ".x", &value ) == false )
			return false;
		pValue->x = (float)value;
		if ( GetValue( szName + ".y", &value ) == false )
			return false;
		pValue->y = (float)value;
		return true;
	}
	template <>
		bool GetValue<CVec3>( const std::string &szName, CVec3 *pValue )
	{
		CVariant value;
		if ( GetValue( szName + ".x", &value ) == false )
			return false;
		pValue->x = (float)value;
		if ( GetValue( szName + ".y", &value ) == false )
			return false;
		pValue->y = (float)value;
		if ( GetValue( szName + ".z", &value ) == false )
			return false;
		pValue->z = (float)value;
		return true;
	}
	template <>
		bool GetValue<CVec4>( const std::string &szName, CVec4 *pValue )
	{
		CVariant value;
		if ( GetValue( szName + ".x", &value ) == false )
			return false;
		pValue->x = (float)value;
		if ( GetValue( szName + ".y", &value ) == false )
			return false;
		pValue->y = (float)value;
		if ( GetValue( szName + ".z", &value ) == false )
			return false;
		pValue->z = (float)value;
		if ( GetValue( szName + ".w", &value ) == false )
			return false;
		pValue->w = (float)value;
		return true;
	}
	template <>
		bool GetValue<CQuat>( const std::string &szName, CQuat *pValue )
	{
		CVec4 vRes;
		if ( GetValue(szName, &vRes) == false )
			return false;
		*pValue = CQuat( vRes );
		return true;
	}
	//
	template <template <typename TYPE> class TContainer, typename TValue>
		bool GetValue( const std::string &szName, TContainer<TValue> *pContainer )
	{
		int nSize = 0;
		if ( GetValue( szName, &nSize ) == false )
			return false;
		pContainer->resize( nSize );
		for ( int i = 0; i < nSize; ++i )
		{
			TValue value;
			if ( GetValue(szName + fmt::format(".[{}]", i), &((*pContainer)[i])) == false )
				return false;
		}
		return true;
	}
	template <>
		bool GetValue<std::basic_string, char>( const std::string &szName, std::basic_string<char> *pValue )
	{
		CVariant var;
		if ( GetValue( szName, &var ) == false )
			return false;
		*pValue = var.GetStr();
		return true;
	}
	template <>
		bool GetValue<std::basic_string, wchar_t>( const std::string &szName, std::basic_string<wchar_t> *pValue )
	{
		CVariant var;
		if ( GetValue( szName, &var ) == false )
			return false;
		*pValue = var.GetWStr();
		return true;
	}
};

struct IArrayObjMan : public IObjMan
{
	virtual void SetIndex( const int nIndex ) = 0;
	virtual int GetIndex() const = 0;
	virtual int GetSize() const = 0;
};

}
