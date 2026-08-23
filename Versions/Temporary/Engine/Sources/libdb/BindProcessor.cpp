#include "stdafx.h"
#include "BindProcessor.h"
#include "BindArray.h"
#include "Bind.h"

#include <cstdint>

namespace NDb
{
namespace NBind
{

// mask manipulator
IObjMan *SBindProcessor::CreateManipulator( const std::string &szName, IObjMan *pParent )
{
	if ( szName.empty() )
		return pParent;
	// array element mask manipulator
	const int nArrayIndexStartPos = szName.find( '[' );
	if ( nArrayIndexStartPos != std::string::npos )
	{
		SArrayCallParams callParams;
		if ( ExtractArrayCallParams( szName, nArrayIndexStartPos, &callParams ) == false )
			return 0;

		IObjMan *pMan = callParams.pUValue->pArray->CreateManipulator( callParams.nArrayIndex, szName.substr(0, nArrayIndexStartPos - 1), 
			callParams.pRawVector, callParams.pContained, callParams.pTypeArray, pParent );
		return callParams.szRestName.empty() ? pMan : pMan->CreateManipulator( callParams.szRestName );
	}
	else
	{
		NMetaInfo::SStructMetaInfo::CFieldsMap::iterator pos = pMetaInfo->fields.find( szName );
		if ( pos == pMetaInfo->fields.end() )
		{
			// TODO: process ref type here
		}
		else if ( pos->second.main.type == NTypeDef::TYPE_TYPE_ARRAY )
		{
			// array mask manipulator
			SArrayRequisites reqs;
			GetArrayRequisites( szName, &reqs );
			return reqs.pUValue->pArray->CreateManipulator( 0, szName, reqs.pRawVector, reqs.pContained, reqs.pTypeArray, pParent );
		}
		else
		{
			// TODO: mask manipulator for sub-structure
			NI_ASSERT( false, "still not realized" );
		}
		return 0;
	}
}

IObjManIterator *SBindProcessor::CreateIterator( const std::string &_szAddName, NTypeDef::STypeStructBase *_pType,
																								 IObjMan *pParent, bool bShowHidden )
{
	return new CStructIterator( _szAddName, _pType, pParent, bShowHidden );
}

bool SBindProcessor::ExtractArrayCallParams( const std::string &szName, int nArrayIndexStartPos, SArrayCallParams *pCallParams )
{
	NMetaInfo::SStructMetaInfo::CFieldsMap::iterator pos = pMetaInfo->fields.find( szName.substr(0, nArrayIndexStartPos - 1) );
	NI_VERIFY( pos != pMetaInfo->fields.end(), "Can't find array field", return false );
	const int nArrayIndexFinishPos = szName.find( ']', nArrayIndexStartPos );
	NI_VERIFY( nArrayIndexFinishPos != std::string::npos, "Can't find array index finish position tag", return false );
	int nArrayIndex = 0;
	for ( int i = nArrayIndexStartPos + 1; i < nArrayIndexFinishPos; ++i )
		nArrayIndex = nArrayIndex * 10 + szName[i] - '0';
	//
	if ( nArrayIndexFinishPos == szName.size() )
		pCallParams->szRestName.clear();
	else if ( nArrayIndexFinishPos + 2 < szName.size() && szName[nArrayIndexFinishPos + 1] == '.' )
		pCallParams->szRestName = szName.substr( nArrayIndexFinishPos + 2 );
	else
		pCallParams->szRestName = szName.substr( nArrayIndexFinishPos + 1 );
	pCallParams->nArrayIndex = nArrayIndex;
	//
	const int nBinaryShift = pos->second.GetBinaryShift();
	if ( nBinaryShift != 0x0000ffff )
		pCallParams->pRawVector = reinterpret_cast<std::vector<uint8_t>*>( pThis + nBinaryShift );
	//
	pCallParams->pUValue = &( ownValues[ pos->second.GetOwnValueIndex() ] );
	pCallParams->pContained = pos->second.pContained;
	pCallParams->pTypeArray = checked_cast_ptr<NTypeDef::STypeArray *>( pos->second.pTypeDef );
	return true;
}

bool SBindProcessor::GetArrayRequisites( const std::string &szName, SArrayRequisites *pReqs )
{
	NMetaInfo::SStructMetaInfo::CFieldsMap::iterator pos = pMetaInfo->fields.find( szName );
	const int nBinaryShift = pos->second.GetBinaryShift();
	if ( nBinaryShift != 0x0000ffff )
		pReqs->pRawVector = reinterpret_cast<std::vector<uint8_t>*>( pThis + nBinaryShift );
	//
	pReqs->pUValue = &( ownValues[ pos->second.GetOwnValueIndex() ] );
	pReqs->pContained = pos->second.pContained;
	pReqs->pTypeArray = checked_cast_ptr<NTypeDef::STypeArray *>( pos->second.pTypeDef );
	return true;
}

bool SBindProcessor::InitArrayElementBindProcessor( SBindProcessor *pProc, std::string *pszRestName, const std::string &szName )
{
	const int nArrayIndexStartPos = szName.find( '[' );
	if ( nArrayIndexStartPos != std::string::npos )
	{
		SArrayCallParams callParams;
		if ( ExtractArrayCallParams( szName, nArrayIndexStartPos, &callParams ) == false )
			return false;
		*pszRestName = callParams.szRestName;
		SBindProcessor bindProcessor;
		return callParams.pUValue->pArray->InitBindProcessor( pProc, callParams.nArrayIndex, callParams.pRawVector, callParams.pContained );
	}
	else
		return false;
}

bool SBindProcessor::SetValue( const std::string &szName, const CVariant &value )
{
	if ( szName.empty() )
	{
		NI_VERIFY( pMetaInfo->singleField.pfnSetValue != 0, "Wrong SetValue func!", return false );
		return (pMetaInfo->singleField.*pMetaInfo->singleField.pfnSetValue)( value, pThis, ownValues );
	}
	//
	NMetaInfo::SStructMetaInfo::CFieldsMap::iterator pos = pMetaInfo->fields.find( szName );
	if ( pos == pMetaInfo->fields.end() )
	{
		SBindProcessor bindProcessor;
		std::string szRestName;
		if ( InitArrayElementBindProcessor( &bindProcessor, &szRestName, szName ) != false )
			return bindProcessor.SetValue( szRestName, value );
		else
		{
			// TODO: add ptr processing here
			return false;
		}
	}
	else
		return (pos->second.*pos->second.pfnSetValue)( value, pThis, ownValues );
}

bool SBindProcessor::GetValue( const std::string &szName, CVariant *pValue )
{
	if ( szName.empty() )
	{
		NI_VERIFY( pMetaInfo->singleField.pfnGetValue != 0, "Wrong GetValue func!", return false );
		return (pMetaInfo->singleField.*pMetaInfo->singleField.pfnGetValue)( pValue, pThis, ownValues );
	}
	//
	NMetaInfo::SStructMetaInfo::CFieldsMap::iterator pos = pMetaInfo->fields.find( szName );
	if ( pos == pMetaInfo->fields.end() )
	{
		SBindProcessor bindProcessor;
		std::string szRestName;
		if ( InitArrayElementBindProcessor( &bindProcessor, &szRestName, szName ) != false )
			return bindProcessor.GetValue( szRestName, pValue );
		else
		{
			// TODO: add ptr processing here
			return false;
		}
	}
	else
		return (pos->second.*pos->second.pfnGetValue)( pValue, pThis, ownValues );
}

bool SBindProcessor::Insert( const std::string &szName, const int nPos, const int nAmount, bool bSetDefault )
{
	NMetaInfo::SStructMetaInfo::CFieldsMap::iterator pos = pMetaInfo->fields.find( szName );
	if ( pos == pMetaInfo->fields.end() )
	{
		SBindProcessor bindProcessor;
		std::string szRestName;
		if ( InitArrayElementBindProcessor( &bindProcessor, &szRestName, szName ) != false )
			return bindProcessor.Insert( szRestName, nPos, nAmount, bSetDefault );
		else
		{
			// TODO: add ptr processing here
			return false;
		}
	}
	else
	{
		if ( pos->second.GetType() != NTypeDef::TYPE_TYPE_ARRAY )
			return false;
		UValue &value = ownValues[ pos->second.GetOwnValueIndex() ];
		return value.pArray->Insert( nPos, nAmount, pos->second, pThis, bSetDefault );
	}
}

bool SBindProcessor::Remove( const std::string &szName, const int nPos, const int nAmount )
{
	NMetaInfo::SStructMetaInfo::CFieldsMap::iterator pos = pMetaInfo->fields.find( szName );
	if ( pos == pMetaInfo->fields.end() )
	{
		SBindProcessor bindProcessor;
		std::string szRestName;
		if ( InitArrayElementBindProcessor( &bindProcessor, &szRestName, szName ) != false )
			return bindProcessor.Remove( szRestName, nPos, nAmount );
		else
		{
			// TODO: add ptr processing here
			return false;
		}
	}
	else
	{
		if ( pos->second.GetType() != NTypeDef::TYPE_TYPE_ARRAY )
			return false;
		UValue &value = ownValues[ pos->second.GetOwnValueIndex() ];
		return value.pArray->Remove( nPos, nAmount, pos->second, pThis );
	}
}

CBindArray *SBindProcessor::GetBindArray( const std::string &szName )
{
	NMetaInfo::SStructMetaInfo::CFieldsMap::iterator pos = pMetaInfo->fields.find( szName );
	// TODO{ add here ptr processing
	if ( pos == pMetaInfo->fields.end() )
		return nullptr;
	// TODO}
	if ( pos->second.GetType() != NTypeDef::TYPE_TYPE_ARRAY )
		return nullptr;
	UValue &value = ownValues[ pos->second.GetOwnValueIndex() ];
	return value.pArray;
}

}

const NTypeDef::STypeStructBase::SField *FindField( const std::string &szFullFieldName, const int nCurrPos, const NTypeDef::STypeStructBase *pStruct )
{
	if ( pStruct->pBaseType )
	{
		if ( const NTypeDef::STypeStructBase::SField *pField = FindField( szFullFieldName, nCurrPos, pStruct->pBaseType ) )
			return pField;
	}
	//
	const int nPos = szFullFieldName.find( '.', nCurrPos );
	if ( nPos == std::string::npos )
	{
		for ( NTypeDef::STypeStructBase::CFieldsList::const_iterator it = pStruct->fields.begin(); it != pStruct->fields.end(); ++it )
		{
			if ( szFullFieldName.compare(nCurrPos, szFullFieldName.size() - nCurrPos, it->szName) == 0 )
				return &( *it );
		}
	}
	else
	{
		for ( NTypeDef::STypeStructBase::CFieldsList::const_iterator it = pStruct->fields.begin(); it != pStruct->fields.end(); ++it )
		{
			if ( szFullFieldName.compare(nCurrPos, nPos - nCurrPos, it->szName) == 0 )
			{
				if ( it->pType->eType == NTypeDef::TYPE_TYPE_ARRAY )
				{
					if ( szFullFieldName[nPos + 1] != '[' )
						return 0;
					const NTypeDef::STypeArray *pTypeArray = checked_cast_ptr<const NTypeDef::STypeArray *>( it->pType );
					const int nNewPos = szFullFieldName.find( '.', nPos + 1 );
					return nNewPos == -1 ? &( pTypeArray->field ) :
										FindField( szFullFieldName, nNewPos + 1, checked_cast_ptr<const NTypeDef::STypeStructBase *>( pTypeArray->field.pType ) );
				}
				else
					return FindField( szFullFieldName, nPos + 1, dynamic_cast_ptr<const NTypeDef::STypeStructBase *>(it->pType) );
			}
		}
	}
	return 0;
}

}
