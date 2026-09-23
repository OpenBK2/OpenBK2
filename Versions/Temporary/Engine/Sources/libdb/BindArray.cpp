#include "stdafx.h"

#include "BindArray.h"
#include "BindProcessor.h"
#include "Bind.h"

#include <cstdint>

#include <fmt/format.h>

namespace NDb
{
namespace NBind
{

// ************************************************************************************************************************ //
// **
// ** array element manipulator
// **
// **
// **
// ************************************************************************************************************************ //

class CArrayElementManipulator : public IArrayElementManipulator, public ILoadableObjMan
{
	OBJECT_NOCOPY_METHODS( CArrayElementManipulator );
	//
	CPtr<NMetaInfo::SStructMetaInfo> pContained;	// contained type
	CPtr<NTypeDef::STypeArray> pTypeArray;				// array type def
	void *pRawVector;							// type-erased vector; metadata supplies its typed operations
	CBindArray *pBindArray;								// bind array of the parent array :)
	CObj<IObjMan> pParent;								// parent object
	std::string szAddName;											// additional name (for complete name restructuring)
	int nIndex;														// array element index
	//
	CArrayElementManipulator() {}
public:
	CArrayElementManipulator( const int _nIndex, const std::string &_szAddName, void *_pRawVector,
		NMetaInfo::SStructMetaInfo *_pContained, NTypeDef::STypeArray *_pTypeArray, IObjMan *_pParent, CBindArray *_pBindArray )
		: pContained( _pContained ), pTypeArray( _pTypeArray ), pRawVector( _pRawVector ), pBindArray( _pBindArray ), 
		  pParent( _pParent ), szAddName( _szAddName ), nIndex( _nIndex ) 
	{
		pBindArray->AddArrayElementManipulator( this );
	}
	~CArrayElementManipulator() { pBindArray->RemoveArrayElementManipulator( this ); }
	// position-in-array functions
	void SetIndex( const int _nIndex ) { nIndex = _nIndex; }
	int GetIndex() const { return nIndex; }
	int GetSize() const 
	{
		int nSize = 0;
		if ( pParent->GetValue(szAddName, &nSize) != false )
			return nSize;
		else
			return 0;
	}
	void ReportArrayInsert( int nPos, int nAmount ) { if ( nIndex >= nPos ) nIndex += nAmount; }
	void ReportArrayRemove( int nPos, int nAmount )
	{
		// A manipulator for an erased element must never refer to its successor.
		if ( nIndex >= nPos && nIndex < nPos + nAmount ) nIndex = -1;
		else if ( nIndex >= nPos + nAmount ) nIndex -= nAmount;
	}
	//
	IObjManIterator *CreateIterator( bool bShowHidden ) 
	{
		return new CArrayIterator( nIndex, "", pTypeArray, this, bShowHidden );
	}
	//
	void SetChanged() { pParent->SetChanged(); }
	//
	IObjMan *CreateManipulator( const std::string &szName )
	{
		if ( szName.empty() )
			return this;
		SBindProcessor bindProcessor;
		if ( pBindArray->InitBindProcessor( &bindProcessor, nIndex, pRawVector, pContained ) )
			return bindProcessor.CreateManipulator( szName, this );
		else
			return 0;
	}
	std::string GetFullName() const
	{
		std::string szFullName = pParent->GetFullName();
		if ( !szFullName.empty() )
		{
			if ( !szAddName.empty() )
				szFullName = szFullName + "." + szAddName;
		}
		else
			szFullName = szAddName;
		//
		return !szFullName.empty() ? szFullName + fmt::format( ".[{}]", nIndex ) : fmt::format( "[{}]", nIndex );
	}
	// main fields manipulation functions
	bool SetValue( const std::string &szName, const CVariant &value )
	{ 
		SetChanged();
		SBindProcessor bindProcessor;
		if ( pBindArray->InitBindProcessor( &bindProcessor, nIndex, pRawVector, pContained ) )
			return bindProcessor.SetValue( szName, value );
		else 
			return false;
	}
	bool GetValue( const std::string &szName, CVariant *pValue )
	{
		SBindProcessor bindProcessor;
		if ( pBindArray->InitBindProcessor( &bindProcessor, nIndex, pRawVector, pContained ) )
			return bindProcessor.GetValue( szName, pValue );
		else
			return false;
	}
	// array-specific functions
	bool Insert( const std::string &szName, const int nPos, const int nAmount = 1, bool bSetDefault = false )
	{
		SetChanged();
		if ( szName.empty() )
			return pParent->Insert( szAddName, nPos, nAmount, bSetDefault );
		else
		{
			SBindProcessor bindProcessor;
			if ( pBindArray->InitBindProcessor( &bindProcessor, nIndex, pRawVector, pContained ) )
				return bindProcessor.Insert( szName, nPos, nAmount, bSetDefault );
			else
				return false;
		}
	}
	bool Remove( const std::string &szName, const int nPos, const int nAmount = 1 )
	{
		SetChanged();
		if ( szName.empty() )
			return pParent->Remove( szAddName, nPos, nAmount );
		else
		{
			SBindProcessor bindProcessor;
			if ( pBindArray->InitBindProcessor( &bindProcessor, nIndex, pRawVector, pContained ) )
				return bindProcessor.Remove( szName, nPos, nAmount );
			else
				return false;
		}
	}
	// get property field descriptor by name
	const NTypeDef::STypeStructBase::SField *GetDesc( const std::string &szFullFieldName ) const
	{
		if ( szFullFieldName.empty() )
			return &( pTypeArray->field );
		else
			return FindField( szFullFieldName, 0, checked_cast_ptr<const NTypeDef::STypeStructBase *>(pTypeArray->field.pType) );
	}
	// direct access to embedded struct (if it is)
	CResource *GetObject() { return pParent->GetObject(); }
	const CDBID &GetDBID() const { return pParent->GetDBID(); }
	// additional custom attributes
	std::wstring GetAttribute( const std::string &szName ) const { return pParent->GetAttribute( szName ); }
	void SetAttribute( const std::string &szName, const std::wstring &szValue ) { pParent->SetAttribute( szName, szValue ); }
	//
	bool LoadXML( const std::string &szAddName, NTypeDef::STypeStructBase *pType, const NXml::CXmlNode *pNode )
	{
		SBindProcessor bindProcessor;
		if ( pBindArray->InitBindProcessor( &bindProcessor, nIndex, pRawVector, pContained ) )
			return bindProcessor.LoadXML( szAddName, pType, pNode, this );
		else
			return false;
	}
	bool SaveXML( const std::string &szAddName, NTypeDef::STypeStructBase *pType, NLXML::CXMLNode *pNode )
	{
		SBindProcessor bindProcessor;
		if ( pBindArray->InitBindProcessor( &bindProcessor, nIndex, pRawVector, pContained ) )
			return bindProcessor.SaveXML( szAddName, pType, pNode, this );
		else
			return false;
	}
	bool SetDefault( const std::string &szAddName, NTypeDef::STypeStructBase *pType )
	{
		SetChanged();
		SBindProcessor bindProcessor;
		if ( pBindArray->InitBindProcessor( &bindProcessor, nIndex, pRawVector, pContained ) )
			return bindProcessor.SetDefault( szAddName, pType );
		else
			return false;
	}
};

// ************************************************************************************************************************ //
// **
// ** bind array funcs
// **
// **
// **
// ************************************************************************************************************************ //

IObjMan *CBindArray::CreateManipulator( const int nIndex, const std::string &szAddName, void *pRawVector,
																			  NMetaInfo::SStructMetaInfo *pContained, NTypeDef::STypeArray *pTypeArray,
																			  IObjMan *pParent )
{
	return new CArrayElementManipulator( nIndex, szAddName, pRawVector, pContained, pTypeArray, pParent, this );
}

IObjManIterator *CBindArray::CreateIterator( const int nIndex, const std::string &szAddName,
																             NTypeDef::STypeArray *pTypeArray, IObjMan *pParent, bool bShowHidden )
{
	return new CArrayIterator( nIndex, szAddName, pTypeArray, pParent, bShowHidden );
}

bool CBindArray::InitBindProcessor( SBindProcessor *pBindProcessor, int nIndex, 
	void *pRawVector, NMetaInfo::SStructMetaInfo *pContained )
{
	if ( nIndex < 0 )
		return false;
	if ( pContained->nNumCodeValues == 0 )
		pBindProcessor->pThis = nullptr;
	else if ( pRawVector && pContained->pArrayOperations && nIndex < pContained->pArrayOperations->size( pRawVector ) )
		pBindProcessor->pThis = pContained->pArrayOperations->element( pRawVector, nIndex );
	else
		return false;
	//
	if ( pContained->nNumOwnValues == 0 )
		pBindProcessor->ownValues = 0;
	else if ( nIndex * pContained->nNumOwnValues < ownValues.size() )
		pBindProcessor->ownValues = &( ownValues[nIndex * pContained->nNumOwnValues] );
	else
		return false;
	//
	pBindProcessor->pMetaInfo = pContained;
	return true;
}

int CBindArray::GetSize( const NMetaInfo::SStructMetaInfo::SField &field, uint8_t *pThis ) const
{
	const int nBinaryShift = field.GetBinaryShift();
	if ( nBinaryShift != 0x0000ffff )
	{
		NI_VERIFY( field.pContained->pArrayOperations, "Missing typed array operations", return 0 );
		const int nSize = field.pContained->pArrayOperations->size( pThis + nBinaryShift );
#ifndef _FINALRELEASE
		if ( field.pContained->nNumOwnValues != 0 )
		{
			NI_ASSERT( (ownValues.size() / field.pContained->nNumOwnValues) == nSize, "array resized outside manipulator!" );
		}
#endif
		return nSize;
	}
	else
	{
		return ownValues.size() / field.pContained->nNumOwnValues;
	}
}

bool CBindArray::Insert( const int _nPos, const int nAmount, const NMetaInfo::SStructMetaInfo::SField &field, uint8_t *pThis, bool bSetDefault )
{
	if ( nAmount <= 0 || _nPos < -1 )
		return false;
	const int nSize = GetSize( field, pThis );
	const int nPos = _nPos == -1 || _nPos > nSize ? nSize : _nPos;
	const auto *operations = field.pContained->pArrayOperations;
	void *vector = field.GetBinaryShift() == 0x0000ffff ? nullptr : pThis + field.GetBinaryShift();
	if ( vector && !operations )
		return false;
	const int nOwn = field.pContained->nNumOwnValues;
	// Reserve the sidecar before changing the typed vector, so its allocation
	// cannot fail after vector insertion has already succeeded.
	const size_t required = (size_t(nSize) + nAmount) * nOwn;
	if ( required > ownValues.capacity() )
		ownValues.reserve( (std::max)( required, ownValues.capacity() * 2 ) );
	if ( vector )
		operations->insert( vector, nPos, nAmount );
	ownValues.insert( ownValues.begin() + nPos * nOwn, nAmount * nOwn, UValue() );
	for ( int i = 0; i < nAmount; ++i )
	{
		uint8_t *data = vector ? operations->element( vector, nPos + i ) : nullptr;
		UValue *values = nOwn ? &ownValues[(nPos + i) * nOwn] : nullptr;
		// C++ constructed the compiled fields; reflection owns only its extras.
		field.pContained->ConstructStruct( data, values, true );
		if ( bSetDefault )
		{
			SBindProcessor processor( data, values, field.pContained );
			if ( field.pContained->singleField.main.size != 0 )
				processor.SetValue( "", checked_cast_ptr<const NTypeDef::STypeSimple *>(field.pContained->singleField.pTypeDef)->GetDefaultValue() );
			else
				processor.SetDefault( "", field.pContained->pStructTypeDef );
		}
	}
	for ( auto *manipulator : arrayElementManipulators )
		manipulator->ReportArrayInsert( nPos, nAmount );
	return true;
}

bool CBindArray::Remove( const int _nPos, const int _nAmount, const NMetaInfo::SStructMetaInfo::SField &field, uint8_t *pThis )
{
	const int nSize = GetSize( field, pThis );
	if ( nSize == 0 )
		return true;
	const int nPos = _nPos == -1 ? (_nAmount == -1 ? 0 : nSize - 1) : _nPos;
	if ( nPos < 0 || nPos >= nSize || _nAmount < -1 || _nAmount == 0 )
		return false;
	// Honor the requested count (the old implementation always removed one).
	const int nAmount = _nAmount == -1 ? nSize - nPos : (std::min)( _nAmount, nSize - nPos );
	const auto *operations = field.pContained->pArrayOperations;
	void *vector = field.GetBinaryShift() == 0x0000ffff ? nullptr : pThis + field.GetBinaryShift();
	if ( vector && !operations )
		return false;
	const int nOwn = field.pContained->nNumOwnValues;
	for ( int i = 0; i < nAmount && nOwn; ++i )
		field.pContained->DestructStruct( vector ? operations->element( vector, nPos + i ) : nullptr,
			&ownValues[(nPos + i) * nOwn], true );
	if ( vector )
		operations->remove( vector, nPos, nAmount );
	ownValues.erase( ownValues.begin() + nPos * nOwn, ownValues.begin() + (nPos + nAmount) * nOwn );
	for ( auto *manipulator : arrayElementManipulators )
		manipulator->ReportArrayRemove( nPos, nAmount );
	return true;
}

void CBindArray::ClearOwnValues( const NMetaInfo::SStructMetaInfo::SField &field, uint8_t *pThis )
{
	const int nOwn = field.pContained->nNumOwnValues;
	const auto *operations = field.pContained->pArrayOperations;
	void *vector = field.GetBinaryShift() == 0x0000ffff ? nullptr : pThis + field.GetBinaryShift();
	// Do not erase compiled elements here: their enclosing C++ vector/resource
	// will destroy them, or may remain alive after its manipulator is released.
	if ( nOwn )
		for ( int i = 0; i < int(ownValues.size()) / nOwn; ++i )
			field.pContained->DestructStruct( vector && operations ? operations->element( vector, i ) : nullptr,
				&ownValues[i * nOwn], true );
	ownValues.clear();
}

bool CBindArray::SetValue( const std::string &szRestName, const int nIndex, const CVariant &value,
	void *pRawVector, NMetaInfo::SStructMetaInfo *pContained )
{
	SBindProcessor bindProcessor;
	if ( InitBindProcessor( &bindProcessor, nIndex, pRawVector, pContained ) )
		return bindProcessor.SetValue( szRestName, value );
	else
		return false;
}

bool CBindArray::GetValue( const std::string &szRestName, const int nIndex, CVariant *pValue,
	void *pRawVector, NMetaInfo::SStructMetaInfo *pContained )
{
	SBindProcessor bindProcessor;
	if ( InitBindProcessor( &bindProcessor, nIndex, pRawVector, pContained ) )
		return bindProcessor.GetValue( szRestName, pValue );
	else
		return false;
}

void CBindArray::RemoveArrayElementManipulator( IArrayElementManipulator *pArrElMan )
{
	for ( CArrayElementsList::iterator it = arrayElementManipulators.begin(); it != arrayElementManipulators.end(); ++it )
	{
		if ( (*it) == pArrElMan )
		{
			arrayElementManipulators.erase( it );
			return;
		}
	}
}

// ************************************************************************************************************************ //
// **
// ** array iterator
// **
// **
// **
// ************************************************************************************************************************ //

CArrayIterator::CArrayIterator( const int nIndex, const std::string &_szAddName, NTypeDef::STypeArray *_pTypeArray,
																IObjMan *_pParent, bool _bShowHidden )
: szAddName( _szAddName ), pTypeArray( _pTypeArray ), pParent( _pParent ), nCurrElementIndex( nIndex ), bArrayElementLocked( true )
{
	if ( nCurrElementIndex == -1 )
	{
		nCurrElementIndex = 0;
		bArrayElementLocked = false;
	}
	bShowHidden = _bShowHidden;
	nNumArrayElements = 0;
	if ( IArrayObjMan *pArrayObjMan = dynamic_cast_ptr<IArrayObjMan *>(pParent) )
	{
		if ( szAddName.empty() )
			nNumArrayElements = pArrayObjMan->GetSize();
		else
			pArrayObjMan->GetValue( szAddName, &nNumArrayElements );
	}
	else
	{
		NI_VERIFY( pParent->GetValue( szAddName, &nNumArrayElements ) != false, 
			fmt::format("Can't get number of elements for array iterator \"{}\"", szAddName), nNumArrayElements = 0 );
	}
	if ( pTypeArray->field.pType->IsSimpleType() == false && bArrayElementLocked )
	{
		if ( nNumArrayElements > 0 )
		{
			NTypeDef::STypeStructBase *pTypeStruct = checked_cast_ptr<NTypeDef::STypeStructBase *>( pTypeArray->field.pType );
			const std::string szFieldName = szAddName.empty() ? "" : szAddName + ".[0].";
			pIterator = new CStructIterator( szFieldName, pTypeStruct, pParent, bShowHidden );
		}
	}
}

bool CArrayIterator::Next()
{
	if ( pTypeArray->field.pType->IsSimpleType() )
		return ++nCurrElementIndex < nNumArrayElements;
	else
	{
		// check for iterator exists
		if ( pIterator == 0 )
		{
			if ( nCurrElementIndex < nNumArrayElements )
			{
				NTypeDef::STypeStructBase *pTypeStruct = checked_cast_ptr<NTypeDef::STypeStructBase *>( pTypeArray->field.pType );
				const std::string szFieldName = szAddName.empty() ? "" : szAddName + fmt::format(".[{}].", nCurrElementIndex);
				pIterator = new CStructIterator( szFieldName, pTypeStruct, pParent, bShowHidden );
				return !pIterator->IsEnd();
			}
			return false;
		}
		// can we obtain next field from iterator?
		if ( pIterator->Next() == true )
			return true;
		// go to next array element
		if ( bArrayElementLocked || ++nCurrElementIndex >= nNumArrayElements )
			return false;
		pIterator = 0;
		return true;
	}
}

bool CArrayIterator::IsEnd() const
{
	if ( pTypeArray->field.pType->IsSimpleType() )
		return nCurrElementIndex >= nNumArrayElements;
	else if ( pIterator )
	{
		return ( nCurrElementIndex >= nNumArrayElements ) || 
			     ( pIterator->IsEnd() && (nCurrElementIndex + 1 >= nNumArrayElements || bArrayElementLocked) );
	}
	else if ( pIterator == 0 )
	{
		return nCurrElementIndex >= nNumArrayElements || (nCurrElementIndex + 1 >= nNumArrayElements && bArrayElementLocked );
	}
	else
		return false;
}

std::string CArrayIterator::GetName() const
{
	if ( pTypeArray->field.pType->IsSimpleType() || pIterator == 0 )
		return szAddName.empty() ? "" : szAddName + fmt::format(".[{}]", nCurrElementIndex);
	else if ( pIterator )
		return pIterator->GetName();
	else
		return "";
}

const NTypeDef::STypeStructBase::SField *CArrayIterator::GetDesc() const
{
	if ( pTypeArray->field.pType->IsSimpleType() || (pIterator == 0 && !bArrayElementLocked) )
		return &( pTypeArray->field );
	else if ( pIterator )
		return pIterator->GetDesc();
	else
		return 0;
}

}
}
