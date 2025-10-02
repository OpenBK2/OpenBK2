#pragma once

#include "StructMetaInfo.h"
#include "OwnValue.h"
#include "Variant.h"
#include "ObjMan.h"

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

interface ILoadableObjMan
{
	virtual bool LoadXML( const string &szAddName, NTypeDef::STypeStructBase *pType, const NXml::CXmlNode *pNode ) = 0;
	virtual bool SaveXML( const string &szAddName, NTypeDef::STypeStructBase *pType, NLXML::CXMLNode *pNode ) = 0;
	virtual bool SetDefault( const string &szAddName, NTypeDef::STypeStructBase *pType ) = 0;
};

struct SBindProcessor
{
	struct SArrayRequisites
	{
		vector<BYTE> *pRawVector;
		NMetaInfo::SStructMetaInfo *pContained;
		NTypeDef::STypeArray *pTypeArray;
		UValue *pUValue;
		SArrayRequisites(): pRawVector(0), pContained(0), pTypeArray(0), pUValue(0) {}
	};
	struct SArrayCallParams : public SArrayRequisites
	{
		string szRestName;
		int nArrayIndex;
		SArrayCallParams(): nArrayIndex(0) {}
	};

	BYTE *pThis;
	UValue *ownValues;
	NMetaInfo::SStructMetaInfo *pMetaInfo;
	//
	SBindProcessor(): pThis(0), ownValues(0), pMetaInfo(0) {}
	SBindProcessor( BYTE *_pThis, UValue *_ownValues, NMetaInfo::SStructMetaInfo *_pMetaInfo )
		: pThis( _pThis ), ownValues( _ownValues ), pMetaInfo( _pMetaInfo ) {}
	//
	IObjMan *CreateManipulator( const string &szName, IObjMan *pParent );
	IObjManIterator *CreateIterator( const string &_szAddName, NTypeDef::STypeStructBase *_pType, 
		                               IObjMan *pParent, bool bShowHidden );
	//
	bool SetValue( const string &szName, const CVariant &value );
	bool GetValue( const string &szName, CVariant *pValue );
	//
	bool Insert( const string &szName, const int nPos, const int nAmount = 1, bool bSetDefault = false );
	bool Remove( const string &szName, const int nPos, const int nAmount = 1 );
	//
	CBindArray *GetBindArray( const string &szName );
	//
	bool ExtractArrayCallParams( const string &szName, int nArrayIndexStartPos, SArrayCallParams *pCallParams );
	bool GetArrayRequisites( const string &szName, SArrayRequisites *pReqs );
	bool InitArrayElementBindProcessor( SBindProcessor *pProc, string *pszRestName, const string &szName );
	//
	bool LoadXML( const string &szAddName, NTypeDef::STypeStructBase *pType, const NXml::CXmlNode *pNode, IObjMan *pParent );
	bool SaveXML( const string &szAddName, NTypeDef::STypeStructBase *pType, NLXML::CXMLNode *pNode, IObjMan *pParent );
	bool SetDefault( const string &szAddName, NTypeDef::STypeStructBase *pType );
};

}

const NTypeDef::STypeStructBase::SField *FindField( const string &szFullFieldName, const int nCurrPos, const NTypeDef::STypeStructBase *pStruct );

}