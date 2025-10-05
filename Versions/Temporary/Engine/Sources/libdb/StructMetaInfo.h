#pragma once
#include "TypeDef.h"
#include "OwnValue.h"

class CVariant;
namespace NDb
{
namespace NMetaInfo
{

void ConstructBinary( BYTE *pData, NTypeDef::ETypeType eType, int nSize );
void DestructBinary( BYTE *pData, NTypeDef::ETypeType eType, int nSize );

//! struct meta info, obtained from code! NOTE: Don't forget to add any new field to MakeDeepCopy functions here!
struct SStructMetaInfo : public CObjectBase
{
	OBJECT_BASIC_METHODS( SStructMetaInfo );
public:
	struct SField
	{
		struct STypeSize
		{
			DWORD type : 8;
			DWORD size : 24;
			//
			STypeSize(): type(0), size(0) {}
		};
		//
		int nShift;													// binary field shift
		STypeSize main;											// main data type & size
		STypeSize contained;								// contained data type & size
		CObj<SStructMetaInfo> pContained;		// in the case of array-of-structs, this field will contain struct descriptor
		CPtr<NTypeDef::STypeDef> pTypeDef;	// link to type descriptor. this field will be filled during linking struct meta info with type definition
#ifdef FAST_DEBUG
		std::string szFieldName;
#endif
		//
		typedef bool (SField::*SetValue)( const CVariant &value, BYTE *pThis, NBind::UValue *ownValues );
		typedef bool (SField::*GetValue)( CVariant *pValue, BYTE *pThis, const NBind::UValue *ownValues ) const;
		SetValue pfnSetValue;
		GetValue pfnGetValue;
		//
		SField(): nShift(0x0000ffff), pfnSetValue(0), pfnGetValue(0) {}
		//
		NTypeDef::ETypeType GetType() const { return NTypeDef::ETypeType( main.type ); }
		int GetOwnValueIndex() const { return (nShift & 0x80000000) != 0 ? DWORD(nShift & 0x7fff0000) >> 16 : -1; }
		int GetBinaryShift() const { return nShift & 0x0000ffff; }
		//
		void ConstructBinary( BYTE *pThis, NBind::UValue *values, bool bOnlyOwn ) const;
		void DestructBinary( BYTE *pThis, NBind::UValue *values, bool bOnlyOwn ) const;
		//
		bool SetValueToStructInt( const CVariant &value, BYTE *pThis, NBind::UValue *ownValues );
		bool SetValueToStructFloat( const CVariant &value, BYTE *pThis, NBind::UValue *ownValues );
		bool SetValueToStructBool( const CVariant &value, BYTE *pThis, NBind::UValue *ownValues );
		bool SetValueToStructString( const CVariant &value, BYTE *pThis, NBind::UValue *ownValues );
		bool SetValueToStructWString( const CVariant &value, BYTE *pThis, NBind::UValue *ownValues );
		bool SetValueToStructGUID( const CVariant &value, BYTE *pThis, NBind::UValue *ownValues );
		bool SetValueToStructBinary( const CVariant &value, BYTE *pThis, NBind::UValue *ownValues );
		bool SetValueToStructEnum( const CVariant &value, BYTE *pThis, NBind::UValue *ownValues );
		bool SetValueToStructDBID( const CVariant &value, BYTE *pThis, NBind::UValue *ownValues );
		//
		bool GetValueFromStructInt( CVariant *pValue, BYTE *pThis, const NBind::UValue *ownValues ) const;
		bool GetValueFromStructFloat( CVariant *pValue, BYTE *pThis, const NBind::UValue *ownValues ) const;
		bool GetValueFromStructBool( CVariant *pValue, BYTE *pThis, const NBind::UValue *ownValues ) const;
		bool GetValueFromStructString( CVariant *pValue, BYTE *pThis, const NBind::UValue *ownValues ) const;
		bool GetValueFromStructWString( CVariant *pValue, BYTE *pThis, const NBind::UValue *ownValues ) const;
		bool GetValueFromStructGUIDorBinary( CVariant *pValue, BYTE *pThis, const NBind::UValue *ownValues ) const;
		bool GetValueFromStructEnum( CVariant *pValue, BYTE *pThis, const NBind::UValue *ownValues ) const;
		bool GetValueFromStructDBID( CVariant *pValue, BYTE *pThis, const NBind::UValue *ownValues ) const;
		//
		bool SetValueToOwnInt( const CVariant &value, BYTE *pThis, NBind::UValue *ownValues );
		bool SetValueToOwnFloat( const CVariant &value, BYTE *pThis, NBind::UValue *ownValues );
		bool SetValueToOwnBool( const CVariant &value, BYTE *pThis, NBind::UValue *ownValues );
		bool SetValueToOwnString( const CVariant &value, BYTE *pThis, NBind::UValue *ownValues );
		bool SetValueToOwnWString( const CVariant &value, BYTE *pThis, NBind::UValue *ownValues );
		bool SetValueToOwnGUID( const CVariant &value, BYTE *pThis, NBind::UValue *ownValues );
		bool SetValueToOwnBinary( const CVariant &value, BYTE *pThis, NBind::UValue *ownValues );
		bool SetValueToOwnEnum( const CVariant &value, BYTE *pThis, NBind::UValue *ownValues );
		bool SetValueToOwnDBID( const CVariant &value, BYTE *pThis, NBind::UValue *ownValues );
		//
		bool GetValueFromOwnInt( CVariant *pValue, BYTE *pThis, const NBind::UValue *ownValues ) const;
		bool GetValueFromOwnFloat( CVariant *pValue, BYTE *pThis, const NBind::UValue *ownValues ) const;
		bool GetValueFromOwnBool( CVariant *pValue, BYTE *pThis, const NBind::UValue *ownValues ) const;
		bool GetValueFromOwnString( CVariant *pValue, BYTE *pThis, const NBind::UValue *ownValues ) const;
		bool GetValueFromOwnWString( CVariant *pValue, BYTE *pThis, const NBind::UValue *ownValues ) const;
		bool GetValueFromOwnGUID( CVariant *pValue, BYTE *pThis, const NBind::UValue *ownValues ) const;
		bool GetValueFromOwnBinary( CVariant *pValue, BYTE *pThis, const NBind::UValue *ownValues ) const;
		bool GetValueFromOwnEnum( CVariant *pValue, BYTE *pThis, const NBind::UValue *ownValues ) const;
		bool GetValueFromOwnDBID( CVariant *pValue, BYTE *pThis, const NBind::UValue *ownValues ) const;
		//
		bool SetValueArraySize( const CVariant &value, BYTE *pThis, NBind::UValue *ownValues );
		bool GetValueArraySize( CVariant *pValue, BYTE *pThis, const NBind::UValue *ownValues ) const;
		//
		void MakeDeepCopy( SField *pRes ) const;
	};
	//
	typedef std::unordered_map<std::string, SField> CFieldsMap;
	CFieldsMap fields;
	SField singleField;	// for simple arrays
	int nStructSize;
	int nNumOwnValues;
	int nNumCodeValues;
	CPtr<NTypeDef::STypeStructBase> pStructTypeDef;
	//
	SStructMetaInfo(): nStructSize(0), nNumOwnValues(0), nNumCodeValues(0) {}
	//
	void AddField( const std::string &szName, int nShift, int nSize, NTypeDef::ETypeType eType );
	void AddField( const std::string &szName, int nShift, int nSize, NTypeDef::ETypeType eType,
		             int nContainedSize, NTypeDef::ETypeType eContainedType );
	// meta constructor/destructor
	void ConstructStruct( BYTE *pThis, NBind::UValue *values, bool bOnlyOwn );
	void DestructStruct( BYTE *pThis, NBind::UValue *values, bool bOnlyOwn );
	//
	void LinkWithTypeDef( const std::string &szAddName, NTypeDef::STypeStructBase *pType );
	void LinkField( const std::string &szAddName, const std::string &szFieldName, NTypeDef::STypeDef *pType );
	//
	void MakeDeepCopy( SStructMetaInfo *pRes ) const;
};

}
}

