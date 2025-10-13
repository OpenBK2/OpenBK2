#pragma once

#include "StructMetaInfo.h"
#include "OwnValue.h"
#include "Variant.h"
#include "ObjMan.h"

#include <cstdint>

namespace NLXML
{
	class CXMLNode;
}
namespace NXml
{
	class CXmlNode;
}
namespace NDb
{
namespace NBind
{

struct ILoadableObjMan
{
	virtual bool LoadXML( const std::string &szAddName, NTypeDef::STypeStructBase *pType, const NXml::CXmlNode *pNode ) = 0;
	virtual bool SaveXML( const std::string &szAddName, NTypeDef::STypeStructBase *pType, NLXML::CXMLNode *pNode ) = 0;
	virtual bool SetDefault( const std::string &szAddName, NTypeDef::STypeStructBase *pType ) = 0;
};

struct SBindProcessor
{
	struct SArrayRequisites
	{
		std::vector<uint8_t> *pRawVector;
		NMetaInfo::SStructMetaInfo *pContained;
		NTypeDef::STypeArray *pTypeArray;
		UValue *pUValue;
		SArrayRequisites(): pRawVector(0), pContained(0), pTypeArray(0), pUValue(0) {}
	};
	struct SArrayCallParams : public SArrayRequisites
	{
		std::string szRestName;
		int nArrayIndex;
		SArrayCallParams(): nArrayIndex(0) {}
	};

	uint8_t *pThis;
	UValue *ownValues;
	NMetaInfo::SStructMetaInfo *pMetaInfo;
	//
	SBindProcessor(): pThis(0), ownValues(0), pMetaInfo(0) {}
	SBindProcessor( uint8_t *_pThis, UValue *_ownValues, NMetaInfo::SStructMetaInfo *_pMetaInfo )
		: pThis( _pThis ), ownValues( _ownValues ), pMetaInfo( _pMetaInfo ) {}
	//
	IObjMan *CreateManipulator( const std::string &szName, IObjMan *pParent );
	IObjManIterator *CreateIterator( const std::string &_szAddName, NTypeDef::STypeStructBase *_pType,
		                               IObjMan *pParent, bool bShowHidden );
	//
	bool SetValue( const std::string &szName, const CVariant &value );
	bool GetValue( const std::string &szName, CVariant *pValue );
	//
	bool Insert( const std::string &szName, const int nPos, const int nAmount = 1, bool bSetDefault = false );
	bool Remove( const std::string &szName, const int nPos, const int nAmount = 1 );
	//
	CBindArray *GetBindArray( const std::string &szName );
	//
	bool ExtractArrayCallParams( const std::string &szName, int nArrayIndexStartPos, SArrayCallParams *pCallParams );
	bool GetArrayRequisites( const std::string &szName, SArrayRequisites *pReqs );
	bool InitArrayElementBindProcessor( SBindProcessor *pProc, std::string *pszRestName, const std::string &szName );
	//
	bool LoadXML( const std::string &szAddName, NTypeDef::STypeStructBase *pType, const NXml::CXmlNode *pNode, IObjMan *pParent );
	bool SaveXML( const std::string &szAddName, NTypeDef::STypeStructBase *pType, NLXML::CXMLNode *pNode, IObjMan *pParent );
	bool SetDefault( const std::string &szAddName, NTypeDef::STypeStructBase *pType );
};

}

const NTypeDef::STypeStructBase::SField *FindField( const std::string &szFullFieldName, const int nCurrPos, const NTypeDef::STypeStructBase *pStruct );

}
