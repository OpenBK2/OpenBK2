#pragma once

#include "Variant.h"
#include "TypeDef.h"

namespace NDb
{
interface IObjManIterator;
struct SObjectHeader;

#define MAN_REMOVE_LAST -1, 1
#define MAN_REMOVE_ALL 0, -1
#define MAN_APPEND -1, 1, true

interface IObjMan : public CObjectBase
{
	//
	virtual void SetChanged() = 0;
	// create mask manipulator for sub-struct, array, array element, etc.
	virtual IObjMan *CreateManipulator( const string &szBaseName ) = 0;
	// create iterator to iterate through all object's properties
	virtual IObjManIterator *CreateIterator( bool bShowHidden = false ) = 0;
	// get full 'add name' for this manipulator
	virtual string GetFullName() const = 0;
	// main fields manipulation functions
	virtual bool SetValue( const string &szName, const CVariant &value ) = 0;
	virtual bool GetValue( const string &szName, CVariant *pValue ) = 0;
	// array-specific functions
	virtual bool Insert( const string &szName, const int nPos, const int nAmount = 1, bool bSetDefault = false ) = 0;
	virtual bool Remove( const string &szName, const int nPos, const int nAmount = 1 ) = 0;
	// get property field descriptor by name
	virtual const NTypeDef::STypeStructBase::SField *GetDesc( const string &szFullFieldName ) const = 0;
	// direct access to embedded struct (if it is)
	virtual CResource *GetObject() = 0;
	//
	virtual const CDBID &GetDBID() const = 0;
	// additional custom attributes
	virtual wstring GetAttribute( const string &szName ) const = 0;
	virtual void SetAttribute( const string &szName, const wstring &szValue ) = 0;

	//
	// fields manipulation helper functions
	//
	// 'set' family
	template <class TYPE>
		bool SetValue( const string &szName, const TYPE &value )
	{
		return SetValue( szName, CVariant( value ) );
	}
	template <>
		bool SetValue<CVec2>( const string &szName, const CVec2 &value )
	{
		return SetValue( szName + ".x", CVariant( value.x ) ) &&
			SetValue( szName + ".y", CVariant( value.y ) );
	}
	template <>
		bool SetValue<CVec3>( const string &szName, const CVec3 &value )
	{
		return SetValue( szName + ".x", CVariant( value.x ) ) &&
			SetValue( szName + ".y", CVariant( value.y ) ) &&
			SetValue( szName + ".z", CVariant( value.z ) );
	}
	template <>
		bool SetValue<CVec4>( const string &szName, const CVec4 &value )
	{
		return SetValue( szName + ".x", CVariant( value.x ) ) &&
			SetValue( szName + ".y", CVariant( value.y ) ) &&
			SetValue( szName + ".z", CVariant( value.z ) ) &&
			SetValue( szName + ".w", CVariant( value.w ) );
	}
	template <>
		bool SetValue<CQuat>( const string &szName, const CQuat &value )
	{
		return SetValue( szName, value.GetInternalVector() );
	}
	//
	template <template <typename TYPE> class TContainer, typename TValue>
		bool SetValue( const string &szName, const TContainer<TValue> &container )
	{
		if ( SetValue( szName, int( container.size() ) ) == false )
			return false;
		int i = 0;
		for ( TContainer<TValue>::const_iterator it = container.begin(); it != container.end(); ++it, ++i )
		{
			if ( SetValue(szName + StrFmt(".[%d]", i), *it) == false )
				return false;
		}
		return true;
	}
	template <>
		bool SetValue<basic_string, char>( const string &szName, const basic_string<char> &value )
	{
		return SetValue( szName, CVariant( value ) );
	}
	template <>
		bool SetValue<basic_string, wchar_t>( const string &szName, const basic_string<wchar_t> &value )
	{
		return SetValue( szName, CVariant( value ) );
	}
	// 'get' family
	template <class TYPE>
		bool GetValue( const string &szName, TYPE *pValue )
	{
		CVariant value;
		if ( GetValue( szName, &value ) == false )
			return false;
		*pValue = (TYPE)value;
		return true;
	}
	template <>
		bool GetValue<string>( const string &szName, string *pValue )
	{
		CVariant value;
		if ( GetValue( szName, &value ) == false )
			return false;
		*pValue = value.GetStr();
		return true;
	}
	template <>
		bool GetValue<wstring>( const string &szName, wstring *pValue )
	{
		CVariant value;
		if ( GetValue( szName, &value ) == false )
			return false;
		*pValue = value.GetWStr();
		return true;
	}
	template <>
		bool GetValue<GUID>( const string &szName, GUID *pValue )
	{
		CVariant value;
		if ( GetValue( szName, &value ) == false )
			return false;
		NI_VERIFY( value.GetType() == CVariant::VT_POINTER && value.GetBlobSize() == sizeof(GUID), "Incorrect BLOB for GUID", return false );
		memcpy( pValue, value.GetPtr(), sizeof(GUID) );
		return true;
	}
	template <>
		bool GetValue<CDBID>( const string &szName, CDBID *pValue )
	{
		CVariant value;
		if ( GetValue( szName, &value ) == false )
			return false;
		*pValue = value.GetDBID();
		return true;
	}
	template <>
		bool GetValue<CVec2>( const string &szName, CVec2 *pValue )
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
		bool GetValue<CVec3>( const string &szName, CVec3 *pValue )
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
		bool GetValue<CVec4>( const string &szName, CVec4 *pValue )
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
		bool GetValue<CQuat>( const string &szName, CQuat *pValue )
	{
		CVec4 vRes;
		if ( GetValue(szName, &vRes) == false )
			return false;
		*pValue = CQuat( vRes );
		return true;
	}
	//
	template <template <typename TYPE> class TContainer, typename TValue>
		bool GetValue( const string &szName, TContainer<TValue> *pContainer )
	{
		int nSize = 0;
		if ( GetValue( szName, &nSize ) == false )
			return false;
		pContainer->resize( nSize );
		for ( int i = 0; i < nSize; ++i )
		{
			TValue value;
			if ( GetValue(szName + StrFmt(".[%d]", i), &((*pContainer)[i])) == false )
				return false;
		}
		return true;
	}
	template <>
		bool GetValue<basic_string, char>( const string &szName, basic_string<char> *pValue )
	{
		CVariant var;
		if ( GetValue( szName, &var ) == false )
			return false;
		*pValue = var.GetStr();
		return true;
	}
	template <>
		bool GetValue<basic_string, wchar_t>( const string &szName, basic_string<wchar_t> *pValue )
	{
		CVariant var;
		if ( GetValue( szName, &var ) == false )
			return false;
		*pValue = var.GetWStr();
		return true;
	}
};

interface IArrayObjMan : public IObjMan
{
	virtual void SetIndex( const int nIndex ) = 0;
	virtual int GetIndex() const = 0;
	virtual int GetSize() const = 0;
};

}