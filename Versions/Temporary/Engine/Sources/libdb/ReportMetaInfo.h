#pragma once

#include "libdb_export.h"

#include "Type.h"

#include <cstdint>

namespace NFile
{
	class CFilePath;
}

namespace NDb
{
namespace NMetaInfo
{
struct SStructMetaInfo;

LIBDB_EXPORT void StartMetaInfoReport( const std::string &szTypeName, const int nTypeID, const int nStructSize );
LIBDB_EXPORT void FinishMetaInfoReport();
void AddOnStack( SStructMetaInfo *pInfo );
LIBDB_EXPORT void ReportMetaInfo( const std::string &szName, int nPtrShift, int nSizeof, NTypeDef::ETypeType eType );
LIBDB_EXPORT void ReportMetaInfo( const std::string &szName, int nPtrShift, int nSizeof, NTypeDef::ETypeType eType,
	                   int nContainedSize, NTypeDef::ETypeType eContainedType );
void DropMetaInfo();

typedef std::unordered_map<std::string, CObj<SStructMetaInfo> > CMetaInfoMap;
bool CreateFullMetaInfoCopy( CMetaInfoMap *pRes );

// ************************************************************************************************************************ //
// **
// ** simple types recognizing
// **
// **
// **
// ************************************************************************************************************************ //

inline NTypeDef::ETypeType GetSimpleTypeDef( int * ) { return NTypeDef::TYPE_TYPE_INT; }
inline NTypeDef::ETypeType GetSimpleTypeDef( float * ) { return NTypeDef::TYPE_TYPE_FLOAT; }
inline NTypeDef::ETypeType GetSimpleTypeDef( bool * ) { return NTypeDef::TYPE_TYPE_BOOL; }
inline NTypeDef::ETypeType GetSimpleTypeDef( std::string * ) { return NTypeDef::TYPE_TYPE_STRING; }
inline NTypeDef::ETypeType GetSimpleTypeDef( std::wstring * ) { return NTypeDef::TYPE_TYPE_WSTRING; }
inline NTypeDef::ETypeType GetSimpleTypeDef( GUID * ) { return NTypeDef::TYPE_TYPE_GUID; }
inline NTypeDef::ETypeType GetSimpleTypeDef( NFile::CFilePath * ) { return NTypeDef::TYPE_TYPE_STRING; }
//
template <class TYPE>
inline NTypeDef::ETypeType GetSimpleTypeDef( CDBPtr<TYPE> * ) { return NTypeDef::TYPE_TYPE_REF; }
//
inline NTypeDef::ETypeType GetSimpleTypeDefEnumOrBinary( SInt2Type<0> * ) { return NTypeDef::TYPE_TYPE_BINARY; }
inline NTypeDef::ETypeType GetSimpleTypeDefEnumOrBinary( SInt2Type<1> * ) { return NTypeDef::TYPE_TYPE_ENUM; }
template <class TYPE> 
inline NTypeDef::ETypeType GetSimpleTypeDef( TYPE * ) 
{ 
	const int N_KNOWN_ENUM = SKnownEnum<TYPE>::isKnown;
	SInt2Type<N_KNOWN_ENUM> separator;
	return GetSimpleTypeDefEnumOrBinary( &separator ); 
}

// ************************************************************************************************************************ //
// **
// ** structs report + pre-defined struct (Vec2-4, Quat)
// **
// **
// **
// ************************************************************************************************************************ //

template <class TYPE>
	inline void ReportStructMetaInfo( const std::string &_szName, const TYPE *pField, uint8_t *pThis )
{
	const std::string szName = _szName.empty() ? _szName : _szName + ".";
	pField->ReportMetaInfo( szName, pThis );
}

template <>
	inline void ReportStructMetaInfo<CVec2>( const std::string &_szName, const CVec2 *pField, uint8_t *pThis )
{
	const std::string szName = _szName.empty() ? _szName : _szName + ".";
	NMetaInfo::ReportMetaInfo( szName + "x", (uint8_t*)&pField->x - pThis, sizeof(pField->x), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szName + "y", (uint8_t*)&pField->y - pThis, sizeof(pField->y), NTypeDef::TYPE_TYPE_FLOAT );
}
template <>
	inline void ReportStructMetaInfo<CVec3>( const std::string &_szName, const CVec3 *pField, uint8_t *pThis )
{
	const std::string szName = _szName.empty() ? _szName : _szName + ".";
	NMetaInfo::ReportMetaInfo( szName + "x", (uint8_t*)&pField->x - pThis, sizeof(pField->x), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szName + "y", (uint8_t*)&pField->y - pThis, sizeof(pField->y), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szName + "z", (uint8_t*)&pField->z - pThis, sizeof(pField->z), NTypeDef::TYPE_TYPE_FLOAT );
}
template <>
	inline void ReportStructMetaInfo<CVec4>( const std::string &_szName, const CVec4 *pField, uint8_t *pThis )
{
	const std::string szName = _szName.empty() ? _szName : _szName + ".";
	NMetaInfo::ReportMetaInfo( szName + "x", (uint8_t*)&pField->x - pThis, sizeof(pField->x), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szName + "y", (uint8_t*)&pField->y - pThis, sizeof(pField->y), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szName + "z", (uint8_t*)&pField->z - pThis, sizeof(pField->z), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szName + "w", (uint8_t*)&pField->w - pThis, sizeof(pField->w), NTypeDef::TYPE_TYPE_FLOAT );
}
template <>
	inline void ReportStructMetaInfo<CQuat>( const std::string &szName, const CQuat *pField, uint8_t *pThis )
{
	ReportStructMetaInfo( szName, &(pField->GetInternalVector()), pThis );
}

// ************************************************************************************************************************ //
// **
// ** template structs (general pair, CTPoint<float>, CTRect<float>)
// **
// **
// **
// ************************************************************************************************************************ //

template <class TYPE>
inline void ReportFirstParamPairElement( const std::string &_szName, const TYPE *pField, uint8_t *pThis )
{
	NMetaInfo::ReportMetaInfo( _szName, (uint8_t*)pField - pThis, sizeof(*pField), GetSimpleTypeDef( (TYPE*)0 ) );
}
template <>
inline void ReportFirstParamPairElement<CVec2>( const std::string &_szName, const CVec2 *pField, uint8_t *pThis )
{
	NMetaInfo::ReportMetaInfo( _szName + ".x", (uint8_t*)&pField->x - pThis, sizeof(pField->x), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( _szName + ".y", (uint8_t*)&pField->y - pThis, sizeof(pField->y), NTypeDef::TYPE_TYPE_FLOAT );
}
template <>
inline void ReportFirstParamPairElement<CVec3>( const std::string &_szName, const CVec3 *pField, uint8_t *pThis )
{
	NMetaInfo::ReportMetaInfo( _szName + ".x", (uint8_t*)&pField->x - pThis, sizeof(pField->x), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( _szName + ".y", (uint8_t*)&pField->y - pThis, sizeof(pField->y), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( _szName + ".z", (uint8_t*)&pField->z - pThis, sizeof(pField->z), NTypeDef::TYPE_TYPE_FLOAT );
}
template <class TYPE, template <class T1> class TParam>
inline void ReportStructMetaInfo( const std::string &_szName, const TParam<TYPE> *pField, uint8_t *pThis )
{
	const std::string szName = _szName.empty() ? _szName : _szName + ".";
	ReportFirstParamPairElement( szName + "First", &pField->first, pThis );
	NMetaInfo::ReportMetaInfo( szName + "Second", (uint8_t*)&pField->second - pThis, sizeof(pField->second), NTypeDef::TYPE_TYPE_BOOL );
}
template <>
inline void ReportStructMetaInfo<float, CTPoint>( const std::string &_szName, const CTPoint<float> *pField, uint8_t *pThis )
{
	const std::string szName = _szName.empty() ? _szName : _szName + ".";
	NMetaInfo::ReportMetaInfo( szName + "x", (uint8_t*)&pField->x - pThis, sizeof(pField->x), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szName + "y", (uint8_t*)&pField->y - pThis, sizeof(pField->y), NTypeDef::TYPE_TYPE_FLOAT );
}
template <>
inline void ReportStructMetaInfo<float, CTRect>( const std::string &_szName, const CTRect<float> *pField, uint8_t *pThis )
{
	const std::string szName = _szName.empty() ? _szName : _szName + ".";
	NMetaInfo::ReportMetaInfo( szName + "x1", (uint8_t*)&pField->x1 - pThis, sizeof(pField->x1), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szName + "y1", (uint8_t*)&pField->y1 - pThis, sizeof(pField->y1), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szName + "x2", (uint8_t*)&pField->x2 - pThis, sizeof(pField->x2), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szName + "y2", (uint8_t*)&pField->y2 - pThis, sizeof(pField->y2), NTypeDef::TYPE_TYPE_FLOAT );
}

// ************************************************************************************************************************ //
// **
// ** CArray2D - just make it empty - can't process for now
// **
// **
// **
// ************************************************************************************************************************ //

//template <>
//inline void ReportStructMetaInfo<int, CArray2D>( const string &_szName, const CArray2D<int> *pField, uint8_t *pThis )
//{	
//}
//template <>
//inline void ReportStructMetaInfo<uint8_t, CArray2D>( const string &_szName, const CArray2D<uint8_t> *pField, uint8_t *pThis )
//{
//}

// ************************************************************************************************************************ //
// **
// ** array of complex structs report
// **
// **
// **
// ************************************************************************************************************************ //

template <class TYPE>
	inline void ReportStructArrayMetaInfo( const std::string &szName, const std::vector<TYPE> *pField, uint8_t *pThis )
{
	NMetaInfo::ReportMetaInfo( szName, (uint8_t*)pField - pThis, sizeof(*pField), NTypeDef::TYPE_TYPE_ARRAY,
		                         sizeof(TYPE), NTypeDef::TYPE_TYPE_STRUCT );
	TYPE temp;
	ReportStructMetaInfo( "", &temp, (uint8_t*)&temp );
	FinishMetaInfoReport();
}

template <class TYPE>
	inline void ReportSimpleArrayMetaInfo( const std::string &szName, const std::vector<TYPE> *pField, uint8_t *pThis )
{
	NMetaInfo::ReportMetaInfo( szName, (uint8_t*)pField - pThis, sizeof(*pField), NTypeDef::TYPE_TYPE_ARRAY,
		sizeof(TYPE), GetSimpleTypeDef( (TYPE*)0 ) );
}

struct LIBDB_EXPORT STerminalClassReporter
{
	IXmlSaver &saver;
	STerminalClassReporter( CResource *pRes, IXmlSaver &_saver );
	~STerminalClassReporter();
};

}
}

