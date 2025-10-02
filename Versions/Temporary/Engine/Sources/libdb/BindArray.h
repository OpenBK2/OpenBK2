#pragma once

#include "StructMetaInfo.h"
#include "OwnValue.h"
#include "Variant.h"
#include "ObjMan.h"
#include "BindProcessor.h"
#include "ObjManIterator.h"

namespace NDb
{
namespace NBind
{

interface IArrayElementManipulator : public IArrayObjMan
{
	virtual void ReportArrayInsert( int nPos, int nAmount ) = 0;
	virtual void ReportArrayRemove( int nPos, int nAmount ) = 0;
};

class CBindArray
{
	vector<UValue> ownValues; // This is an actually 2D array - it contains N elements for each of K array entries. Total size = N*K
	typedef list<IArrayElementManipulator*> CArrayElementsList;
	CArrayElementsList arrayElementManipulators;
public:
	IObjMan *CreateManipulator( const int nIndex, const string &szAddName, vector<BYTE> *pRawVector, 
		NMetaInfo::SStructMetaInfo *pContained, NTypeDef::STypeArray *pTypeArray, IObjMan *pParent );
	IObjManIterator *CreateIterator( const int _nIndex, const string &_szAddName, 
		NTypeDef::STypeArray *_pTypeArray, IObjMan *pParent, bool bShowHidden );
	//
	int GetSize( const NMetaInfo::SStructMetaInfo::SField &field, BYTE *pThis ) const;
	bool Insert( const int nPos, const int nAmount, const NMetaInfo::SStructMetaInfo::SField &field, BYTE *pThis, bool bSetDefault );
	bool Remove( const int nPos, const int nAmount, const NMetaInfo::SStructMetaInfo::SField &field, BYTE *pThis );
	bool SetValue( const string &szRestName, const int nIndex, const CVariant &value, 
  		           vector<BYTE> *pRawVector, NMetaInfo::SStructMetaInfo *pContained );
	bool GetValue( const string &szRestName, const int nIndex, CVariant *pValue, 
		             vector<BYTE> *pRawVector, NMetaInfo::SStructMetaInfo *pContained );
	//
	bool InitBindProcessor( SBindProcessor *pBindProcessor, int nIndex, 
		                      vector<BYTE> *pRawVector, NMetaInfo::SStructMetaInfo *pContained );
	//
	void AddArrayElementManipulator( IArrayElementManipulator *pArrElMan ) { arrayElementManipulators.push_back( pArrElMan ); }
	void RemoveArrayElementManipulator( IArrayElementManipulator *pArrElMan );
};

class CArrayIterator : public IObjManIterator
{
	OBJECT_NOCOPY_METHODS( CArrayIterator );
	//
	string szAddName;
	CPtr<NTypeDef::STypeArray> pTypeArray;
	CPtr<IObjMan> pParent;
	bool bArrayElementLocked;
	int nCurrElementIndex;
	int nNumArrayElements;
	bool bShowHidden;
	CObj<IObjManIterator> pIterator;
	//
	CArrayIterator(): bArrayElementLocked( false ), nCurrElementIndex( -1 ), nNumArrayElements( -1 ), bShowHidden( false ) {}
public:
	// use index -1 for unlocked array
	CArrayIterator( const int nIndex, const string &_szAddName, NTypeDef::STypeArray *_pTypeArray, 
		              IObjMan *_pParent, bool _bShowHidden );
	//
	string GetBaseName() const { return pParent->GetFullName(); }
	// goto next field
	bool Next();
	// have we reached end?
	bool IsEnd() const;
	// get current field's (full) name
	string GetName() const;
	// get current field descriptor
	const NTypeDef::STypeStructBase::SField *GetDesc() const;
};

}
}

