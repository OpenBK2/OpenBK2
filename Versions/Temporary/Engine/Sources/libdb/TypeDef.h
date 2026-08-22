#pragma once

#include "Nodes2TypeDefs.h"
#include "Type.h"
#include "Variant.h"
#include "Misc/StrProc.h"
#include "System/XmlSaver.h"

#include <boost/uuid/uuid.hpp>

//
// TODO: equivalence function and equality function!
//

// types to realize: binary (+size), simple type typedef + attributes, array
//
//
namespace NDb
{
namespace NTypeDef
{

//typedef variant_t CVariant;

struct SConstraints : public CXmlResource
{
	SConstraints() {}
	virtual bool CheckValueCorrect( const CVariant &value ) const = 0;
	//
	int operator&( IXmlSaver &saver ) { return 0; }
};

struct SAttributes : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( SAttributes );
public:
	std::unordered_map<std::string, CVariant> attributes;

	SAttributes() { }
	SAttributes( const std::unordered_map<std::string, CVariant> &_attributes ) : attributes( _attributes ) { }
	//
	bool HashAttribute( const std::string &szName ) const { return attributes.find( szName ) != attributes.end(); }
	//
	int operator&( IXmlSaver &saver )
	{
		saver.Add( "Attributes", &attributes );
		return 0;
	}
};

// ************************************************************************************************************************ //
// **
// ** basic type definition
// **
// **
// **
// ************************************************************************************************************************ //

struct STypeDef : public CXmlResource
{
	ETypeType eType;											// type's type :)
	//
	STypeDef( ETypeType _eType ): eType( _eType ) {}
	//
	virtual EEditorType GetDefaultEditorType() const { return EDITOR_TYPE_UNKNOWN; }
	virtual bool IsSimpleType() const = 0;
	virtual int GetTypeSize() const { return 0; }
	virtual const std::string GetTypeName() const = 0;
	//
	virtual void ToString( std::string *pRes, const CVariant &value ) const {  }
	std::string ToString( const CVariant &value ) const { std::string szRes; ToString(&szRes, value); return szRes; }
	virtual void FromString( CVariant *pRes, const std::string &szValue ) const {  }
	virtual SAttributes* GetAttributes() const { return 0; }
	//
	int operator&( IXmlSaver &saver )
	{
		saver.Add( "Type", &eType );
		return 0;
	}
};

// ************************************************************************************************************************ //
// **
// ** simple types: int, float, bool, string, wstring, GUID
// **
// **
// **
// ************************************************************************************************************************ //

struct STypeSimple : public STypeDef
{
	STypeSimple( ETypeType _eType ) : STypeDef( _eType ) {}
	//
	virtual CVariant GetDefaultValue() const = 0;
	int GetTypeSize() const { return 4; }
	bool IsSimpleType() const { return true; }

	//
	int operator&( IXmlSaver &saver )
	{
		saver.AddTypedSuper( static_cast<STypeDef*>(this) );
		return 0;
	}
};

struct STypeInt : public STypeSimple
{
	OBJECT_NOCOPY_METHODS( STypeInt );
public:
	STypeInt(): STypeSimple( TYPE_TYPE_INT ) {}
	//
	CVariant GetDefaultValue() const { return 0; }
	EEditorType GetDefaultEditorType() const { return EDITOR_TYPE_INT_INPUT; }
	const std::string GetTypeName() const { return "int"; }
	//
	void ToString( std::string *pRes, const CVariant &value ) const;
	void FromString( CVariant *pRes, const std::string &szValue ) const;
	//
	int operator&( IXmlSaver &saver )
	{
		saver.AddTypedSuper( static_cast<STypeSimple*>(this) );
		return 0;
	}
};

struct STypeFloat : public STypeSimple
{
	OBJECT_NOCOPY_METHODS( STypeFloat );
public:
	STypeFloat(): STypeSimple( TYPE_TYPE_FLOAT ) {}
	//
	CVariant GetDefaultValue() const { return 0; }
	EEditorType GetDefaultEditorType() const { return EDITOR_TYPE_FLOAT_INPUT; }
	const std::string GetTypeName() const { return "float"; }
	//
	void ToString( std::string *pRes, const CVariant &value ) const;
	void FromString( CVariant *pRes, const std::string &szValue ) const;
	//
	int operator&( IXmlSaver &saver )
	{
		saver.AddTypedSuper( static_cast<STypeSimple*>(this) );
		return 0;
	}
};

struct STypeBool : public STypeSimple
{
	OBJECT_NOCOPY_METHODS( STypeBool );
public:
	STypeBool(): STypeSimple( TYPE_TYPE_BOOL ) {}
	//
	CVariant GetDefaultValue() const { return false; }
	EEditorType GetDefaultEditorType() const { return EDITOR_TYPE_BOOL_COMBO; }
	int GetTypeSize() const { return 1; }
	const std::string GetTypeName() const { return "bool"; }
	//
	void ToString( std::string *pRes, const CVariant &value ) const;
	void FromString( CVariant *pRes, const std::string &szValue ) const;
	//
	int operator&( IXmlSaver &saver )
	{
		saver.AddTypedSuper( static_cast<STypeSimple*>(this) );
		return 0;
	}
};

struct STypeString : public STypeSimple
{
	OBJECT_NOCOPY_METHODS( STypeString );
public:
	STypeString(): STypeSimple( TYPE_TYPE_STRING ) {}
	//
	CVariant GetDefaultValue() const { return ""; }
	EEditorType GetDefaultEditorType() const { return EDITOR_TYPE_STRING_INPUT; }
	int GetTypeSize() const { return sizeof(std::string); }
	const std::string GetTypeName() const { return "string"; }
	//
	void ToString( std::string *pRes, const CVariant &value ) const;
	void FromString( CVariant *pRes, const std::string &szValue ) const;
	//
	int operator&( IXmlSaver &saver )
	{
		saver.AddTypedSuper( static_cast<STypeSimple*>(this) );
		return 0;
	}
};

struct STypeWString : public STypeSimple
{
	OBJECT_NOCOPY_METHODS( STypeWString );
public:
	STypeWString(): STypeSimple( TYPE_TYPE_WSTRING ) {}
	//
	CVariant GetDefaultValue() const { return L""; }
	EEditorType GetDefaultEditorType() const { return EDITOR_TYPE_STRING_INPUT; }
	int GetTypeSize() const { return sizeof(std::wstring); }
	const std::string GetTypeName() const { return "wstring"; }
	//
	void ToString( std::string *pRes, const CVariant &value ) const;
	void FromString( CVariant *pRes, const std::string &szValue ) const;
	//
	int operator&( IXmlSaver &saver )
	{
		saver.AddTypedSuper( static_cast<STypeSimple*>(this) );
		return 0;
	}
};

struct STypeGUID : public STypeSimple
{
	OBJECT_NOCOPY_METHODS( STypeGUID );
public:
	STypeGUID(): STypeSimple( TYPE_TYPE_GUID ) {}
	//
	CVariant GetDefaultValue() const;
	EEditorType GetDefaultEditorType() const { return EDITOR_TYPE_GUID; }
	int GetTypeSize() const { return sizeof(boost::uuids::uuid); }
	const std::string GetTypeName() const { return "GUID"; }
	//
	void ToString( std::string *pRes, const CVariant &value ) const;
	void FromString( CVariant *pRes, const std::string &szValue ) const;
	//
	int operator&( IXmlSaver &saver )
	{
		saver.AddTypedSuper( static_cast<STypeSimple*>(this) );
		return 0;
	}
};

// binary ...
struct STypeBinary : public STypeSimple
{
	OBJECT_NOCOPY_METHODS( STypeBinary );
public:
	std::string szTypeName;
	int nBinaryObjectSize;								// size of binary object
	CObj<SAttributes> pAttributes;
	//
	STypeBinary(): STypeSimple( TYPE_TYPE_BINARY ), nBinaryObjectSize( 0 ) {}
	STypeBinary( const std::string &_szTypeName ): STypeSimple( TYPE_TYPE_BINARY ), szTypeName( _szTypeName ), nBinaryObjectSize( 0 ) {}
	//
	CVariant GetDefaultValue() const;
	EEditorType GetDefaultEditorType() const { return EDITOR_TYPE_BIT_FIELD; }
	int GetTypeSize() const { return nBinaryObjectSize; }
	const std::string GetTypeName() const { return szTypeName; }
	virtual SAttributes* GetAttributes() const { return pAttributes; }
	//
	void ToString( std::string *pRes, const CVariant &value ) const;
	void FromString( CVariant *pRes, const std::string &szValue ) const;
	//
	int operator&( IXmlSaver &saver )
	{
		saver.AddTypedSuper( static_cast<STypeSimple*>(this) );
		saver.Add( "TypeName", &szTypeName );
		saver.Add( "BinaryObjectSize", &nBinaryObjectSize );
		saver.AddInPlace( "Attributes", &pAttributes );
		return 0;
	}
};

// enum contains additionally names and values for all enum entries
struct STypeEnum : public STypeSimple
{
	OBJECT_NOCOPY_METHODS( STypeEnum );
public:
	struct SEnumEntry
	{
		std::string szName;
		int nVal;
		//
		SEnumEntry(): nVal( -1 ) {}
		SEnumEntry( const std::string &_szName, int _nVal ): szName( _szName ), nVal( _nVal ) {}
		//
		int operator&( IXmlSaver &saver )
		{
			saver.Add( "Name", &szName );
			saver.Add( "Value", &nVal );
			return 0;
		}
	};
	std::string szTypeName;
	CObj<SAttributes> pAttributes;
	std::vector<SEnumEntry> entries;
	//
	STypeEnum(): STypeSimple( TYPE_TYPE_ENUM ) {}
	STypeEnum( const std::string &_szTypeName ): STypeSimple( TYPE_TYPE_ENUM ), szTypeName( _szTypeName ) {}
	//
	CVariant GetDefaultValue() const { return entries[0].szName; }
	EEditorType GetDefaultEditorType() const { return EDITOR_TYPE_STRING_COMBO; }
	int GetTypeSize() const { return 4; }
	const std::string GetTypeName() const { return szTypeName; }
	//
	void ToString( std::string *pRes, const CVariant &value ) const;
	void FromString( CVariant *pRes, const std::string &szValue ) const;
	virtual SAttributes* GetAttributes() const { return pAttributes; }
	//
	int operator&( IXmlSaver &saver )
	{
		saver.AddTypedSuper( static_cast<STypeSimple*>(this) );
		saver.Add( "TypeName", &szTypeName );
		saver.Add( "Entries", &entries );
		saver.AddInPlace( "Attributes", &pAttributes );
		return 0;
	}
};

// ************************************************************************************************************************ //
// **
// ** complex types: struct, class
// **
// **
// **
// ************************************************************************************************************************ //

// complex type, which contains own fields and nested types
struct STypeStructBase : public STypeDef
{
	struct SField
	{
		CPtr<STypeDef> pType;								// field type
		std::string szName;											// field name
		int nChunkID;												// chunkID (for binary serialization)
		std::wstring wszDesc;										// field description
		std::vector< CObj<SConstraints> > constraints;		// constraints
		CObj<SAttributes> pAttributes;			// attributes
		CVariant defaultValue;							// default value - for simple type fields only!

		CVariant complexDefaultValue;
		//
		SField(): nChunkID( -1 ) {}
		//
		bool CheckValueCorrect( const CVariant &value ) const;
		EEditorType GetEditorType() const;
		CVariant GetDefaultValue() const;
		bool HasAttribute( const std::string &szName ) const;
		const CVariant *GetAttribute( const std::string &szName ) const;
		//
		int operator&( IXmlSaver &saver )
		{
			saver.Add( "Type", &pType );
			saver.Add( "Name", &szName );
			saver.Add( "ChunkID", &nChunkID );
			saver.Add( "Description", &wszDesc );
			saver.Add( "Constraints", &constraints );
			saver.AddInPlace( "Attributes", &pAttributes );
			saver.Add( "DefaultValue", &defaultValue );
			saver.Add( "ComplexDefaultValue", &complexDefaultValue );

			return 0;
		}
	};
	//
	std::string szTypeName;										// complex type user-defined name
	CObj<SAttributes> pAttributes;				// complex type support attributes
	CObj<STypeStructBase> pBaseType;			// base type name
	typedef std::vector< CObj<STypeDef> > CNestedTypesList;
	CNestedTypesList nestedTypes;					// all nested types
	typedef std::vector<SField> CFieldsList;
	CFieldsList fields;										// complex type's fields
	//
	STypeStructBase( ETypeType _eType, const std::string &_szTypeName ): STypeDef( _eType ), szTypeName( _szTypeName ) {}
	//
	bool IsSimpleType() const { return false; }
	const std::string GetTypeName() const { return szTypeName; }
	//
	EEditorType GetDefaultEditorType() const { return EDITOR_TYPE_UNKNOWN; }
	void AddField( STypeDef *pType, 
		             const std::string &szName,
								 const int nChunkID, 
								 const std::wstring &wszDesc,
		             SConstraints *pConstraints, 
								 SAttributes *pAttributes,
								 const CVariant &_vtDefVal = CVariant() );
	virtual SAttributes* GetAttributes() const { return pAttributes; }
	//
	int operator&( IXmlSaver &saver )
	{
		saver.AddTypedSuper( static_cast<STypeDef*>(this) );
		saver.Add( "TypeName", &szTypeName );
		saver.AddInPlace( "Attributes", &pAttributes );
		saver.Add( "BaseType", &pBaseType );
		saver.Add( "NestedTypes", &nestedTypes );
		saver.Add( "Fields", &fields );
		return 0;
	}
};

struct STypeStruct : public STypeStructBase
{
	OBJECT_NOCOPY_METHODS( STypeStruct );
public:
	STypeStruct(): STypeStructBase( TYPE_TYPE_STRUCT, "" ) {}
	STypeStruct( const std::string &_szTypeName ): STypeStructBase( TYPE_TYPE_STRUCT, _szTypeName ) {}
	//
	int operator&( IXmlSaver &saver )
	{
		saver.AddTypedSuper( static_cast<STypeStructBase*>(this) );
		return 0;
	}
};

struct STypeClass : public STypeStructBase
{
	OBJECT_NOCOPY_METHODS( STypeClass );
public:
	int nClassTypeID;
	typedef std::vector< CPtr<STypeClass> > CDerivedTerminalTypesList;
	CDerivedTerminalTypesList derivedTerminalTypes;
	//
	STypeClass(): STypeStructBase( TYPE_TYPE_CLASS, "" ), nClassTypeID(-1) {}
	STypeClass( const std::string &_szTypeName ): STypeStructBase( TYPE_TYPE_CLASS, _szTypeName ), nClassTypeID(-1) {}
	//
	void RegisterTerminalType( STypeClass *pClass = 0 );
	//
	int operator&( IXmlSaver &saver )
	{
		saver.AddTypedSuper( static_cast<STypeStructBase*>(this) );
		saver.Add( "TypeID", &nClassTypeID );
		return 0;
	}
};

// ************************************************************************************************************************ //
// **
// ** conatiner classes: vector, ref
// **
// **
// **
// ************************************************************************************************************************ //

struct STypeArray : public STypeDef
{
	OBJECT_NOCOPY_METHODS( STypeArray );
public:
	STypeStructBase::SField field;
	//
	STypeArray(): STypeDef( TYPE_TYPE_ARRAY ) {}
	STypeArray( STypeDef *pType ): STypeDef( TYPE_TYPE_ARRAY ) { field.pType = pType; }
	//
	bool IsSimpleType() const { return false; }
	const std::string GetTypeName() const { return "array"; }
	void ToString( std::string *pRes, const CVariant &value ) const;
	void FromString( CVariant *pRes, const std::string &szValue ) const;
	//
	int operator&( IXmlSaver &saver )
	{
		saver.AddTypedSuper( static_cast<STypeDef*>(this) );
		saver.Add( "Field", &field );

		return 0;
	}
};

// ref to only one polymorphic type (multiref refers to non-terminal class)
struct STypeRef : public STypeSimple
{
	OBJECT_NOCOPY_METHODS( STypeRef );
public:
	CPtr<STypeDef> pRefType;
	//
	STypeRef(): STypeSimple( TYPE_TYPE_REF ) {}
	STypeRef( STypeDef *_pRefType ): STypeSimple( TYPE_TYPE_REF ), pRefType( _pRefType ) {}
	//
	EEditorType GetDefaultEditorType() const { return EDITOR_TYPE_STRING_NEW_MULTI_REF; }
	virtual CVariant GetDefaultValue() const { return CVariant(); }
	int GetTypeSize() const { return sizeof(CDBPtr<CResource>); }
	const std::string GetTypeName() const { return "ref"; }
	void ToString( std::string *pRes, const CVariant &value ) const;
	void FromString( CVariant *pRes, const std::string &szValue ) const;
	//
	void GetRefTypesList( std::vector<const STypeClass *> *pTypesList ) const;
	bool CheckRefType( const int nClassTypeID ) const;
	//
	int operator&( IXmlSaver &saver )
	{
		saver.AddTypedSuper( static_cast<STypeSimple*>(this) );
		saver.Add( "RefType", &pRefType );
		return 0;
	}
};

// ************************************************************************************************************************ //
// **
// ** Constraints - different limits and additional info for types
// **
// **
// **
// ************************************************************************************************************************ //

struct SConstraintsUnlimited : public SConstraints
{
	OBJECT_NOCOPY_METHODS( SConstraintsUnlimited );
public:
	bool CheckValueCorrect( const CVariant &value ) const { return true; }
	//
	int operator&( IXmlSaver &saver )
	{
		saver.AddTypedSuper( static_cast<SConstraints*>(this) );
		return 0;
	}
};
// min/max constraint
//
template <class TYPE>
struct SConstraintsMinMax : public SConstraints
{
	OBJECT_NOCOPY_METHODS( SConstraintsMinMax );
public:
	TYPE tMin, tMax;
	//
	SConstraintsMinMax(): tMin(0), tMax(0) {}
	SConstraintsMinMax( TYPE _tMin, TYPE _tMax ): tMin( _tMin ), tMax( _tMax ) {}
	//
	bool CheckValueCorrect( const CVariant &value ) const 
	{
		TYPE tValue = (TYPE)value;
		return tValue >= tMin && tValue <= tMax;
	}
	//
	int operator&( IXmlSaver &saver )
	{
		saver.AddTypedSuper( static_cast<SConstraints*>(this) );
		saver.Add( "Min", &tMin );
		saver.Add( "Max", &tMax );
		return 0;
	}
};
typedef SConstraintsMinMax<int> SConstraintsMinMaxInt;
typedef SConstraintsMinMax<float> SConstraintsMinMaxFloat;

// values list constraint
//
template <class TYPE>
struct SConstraintsValuesList : public SConstraints
{
	OBJECT_NOCOPY_METHODS( SConstraintsValuesList );
public:
	std::vector<TYPE> values;
	//
	SConstraintsValuesList() {}
	SConstraintsValuesList( const std::vector<TYPE> &_values ): values( _values ) {}
	//
	bool CheckValueCorrect( const CVariant &value ) const 
	{
		return find( values.begin(), values.end(), (TYPE)value ) != values.end();
	}
	//
	int operator&( IXmlSaver &saver )
	{
		saver.AddTypedSuper( static_cast<SConstraints*>(this) );
		saver.Add( "Values", &values );
		return 0;
	}
};
typedef SConstraintsValuesList<int> SConstraintsValuesListInt;
typedef SConstraintsValuesList<float> SConstraintsValuesListFloat;

// array min/max number of elements
//
struct SConstraintsArrayMinMax : public SConstraints
{
	OBJECT_NOCOPY_METHODS( SConstraintsArrayMinMax );
public:
	int nMinElements;
	int nMaxElements;
	//
	SConstraintsArrayMinMax(): nMinElements( -1 ), nMaxElements( -1 ) {}
	SConstraintsArrayMinMax( int _nMinElements, int _nMaxElements ): nMinElements( _nMinElements ), nMaxElements( _nMaxElements ) {}
	//
	bool CheckValueCorrect( const CVariant &value ) const 
	{
		int nNumElements = (int)value;
		if ( nMinElements >= 0 && nNumElements < nMinElements )
			return false;
		if ( nMaxElements >= 0 && nNumElements > nMaxElements )
			return false;
		return true;
	}
	//
	int operator&( IXmlSaver &saver )
	{
		saver.AddTypedSuper( static_cast<SConstraints*>(this) );
		saver.Add( "MinElements", &nMinElements );
		saver.Add( "MaxElements", &nMaxElements );
		return 0;
	}
};

// string filename constraint
//
struct SConstraintsString : public SConstraints
{
	OBJECT_NOCOPY_METHODS( SConstraintsString );
public:
	std::string szRegExp;
	//
	bool CheckValueCorrect( const CVariant &value ) const 
	{
//		string szValue = value.GetStr();
//		if ( szRegExp.empty() )
//			return IsValidPathName( szValue );
//		else
//			return true;
		return true;
	}
	//
	int operator&( IXmlSaver &saver )
	{
		saver.AddTypedSuper( static_cast<SConstraints*>(this) );
		saver.Add( "RegExp", &szRegExp );
		return 0;
	}
};

// type reference constraint
struct SConstraintsTypeRef : public SConstraints
{
	OBJECT_NOCOPY_METHODS( SConstraintsTypeRef );
public:
	CPtr<STypeDef> pType;
	//
	SConstraintsTypeRef() {}
	SConstraintsTypeRef( STypeDef *_pType ): pType( _pType ) {}
	//
	bool CheckValueCorrect( const CVariant &value ) const { return true; }
	//
	int operator&( IXmlSaver &saver )
	{
		saver.AddTypedSuper( static_cast<SConstraints*>(this) );
		saver.Add( "Type", &pType );
		return 0;
	}
};

}
}

