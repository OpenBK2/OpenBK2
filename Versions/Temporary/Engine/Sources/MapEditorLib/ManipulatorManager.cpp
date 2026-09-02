#include "stdafx.h"

#include "libdb/ResourceManager.h"
#include "ManipulatorManager.h"
#include "StringManager.h"
#include "PCIEMnemonics.h"
#include "System/FilePath.h"
#include "MapEditorLib/Interface_Controller.h"
#include "MapEditorLib/MultiManipulator.h"

#include <cstdint>

IManipulator *CManipulatorManager::CreateObectSetManipulator( const SObjectSet &rObjectSet )
{
	if ( !rObjectSet.objectNameSet.empty() )
	{
		IResourceManager *pResourceManager = Singleton<IResourceManager>();
		if ( rObjectSet.objectNameSet.size() == 1 )
		{
			return pResourceManager->CreateObjectManipulator( rObjectSet.szObjectTypeName, rObjectSet.objectNameSet.begin()->first );
		}
		else
		{
			if ( CMultiManipulator *pMultiManipulator = new CMultiManipulator() )
			{
				for ( CObjectNameSet::const_iterator itObjectName = rObjectSet.objectNameSet.begin(); itObjectName != rObjectSet.objectNameSet.end(); ++itObjectName )
				{
					CPtr<IManipulator> pObjectManipulator = pResourceManager->CreateObjectManipulator( rObjectSet.szObjectTypeName, itObjectName->first );
					if ( pObjectManipulator != 0 )
					{
						pMultiManipulator->InsertManipulator( itObjectName->first, pObjectManipulator, false, false );
					}
				}
				return pMultiManipulator;
			}
		}
	}
	return 0;
}


bool CManipulatorManager::CloneDBManipulator( IManipulator *pDestinationManipulator,
																							IManipulator *pSourceManipulator,
																							bool bEqual )
{
	NI_ASSERT( pDestinationManipulator != 0, "CManipulatorManager::CloneDBManipulator(): pDestinationManipulator == 0" );
	NI_ASSERT( pSourceManipulator != 0, "CManipulatorManager::CloneDBManipulator(): pSourceManipulator == 0" );
	if ( ( pDestinationManipulator == 0 ) || ( pSourceManipulator == 0 ) )
	{
		return false;
	}
	std::string szName;
	//собираем данные по массивам ( сколько нужно каких массивов )
	std::list<std::string> sourceArrayList;
	std::list<int> sourceArraySizeList;
	{
		CPtr<IManipulatorIterator> pSourceManipulatorIterator = pSourceManipulator->Iterate( true, ECT_CACHE_LOCAL );
		while ( !pSourceManipulatorIterator->IsEnd() )
		{
			pSourceManipulatorIterator->GetName( &szName );
			if ( const SPropertyDesc *pDesc = dynamic_cast<const SPropertyDesc*>( pSourceManipulatorIterator->GetDesc() ) )
			{
				if ( pDesc->bArray && ( szName[szName.size() - 1] != ARRAY_NODE_END_CHAR ) )
				{
					int nCount = 0;
					if ( GetValue( &nCount, pSourceManipulator, szName ) )
					{
						sourceArrayList.push_back( szName );
						sourceArraySizeList.push_back( nCount );
					}
				}
			}
			pSourceManipulatorIterator->Next();
		}
	}
	
	//создаем все необходимые массивы
	std::list<std::string>::const_iterator itSourceArray = sourceArrayList.begin();
	std::list<int>::const_iterator itSourceArraySize = sourceArraySizeList.begin();
	while ( ( itSourceArray != sourceArrayList.end() ) && ( itSourceArraySize != sourceArraySizeList.end() ) )
	{
		//DebugTrace( "CloneDBManipulator(): Add array <%s>: %d", itSourceArray->c_str(), ( *itSourceArraySize ) );
		pDestinationManipulator->RemoveNode( ( *itSourceArray ), NODE_REMOVEALL_INDEX );
		for ( int nNodeIndex = 0; nNodeIndex < ( *itSourceArraySize ); ++nNodeIndex )
		{
			pDestinationManipulator->InsertNode( ( *itSourceArray ), NODE_ADD_INDEX );
		}
		++itSourceArray;
		++itSourceArraySize;
	}

	//создаем список полей
	std::unordered_map<std::string, uint32_t> destinationFields;
	if ( !bEqual )
	{
		CPtr<IManipulatorIterator> pDestinationManipulatorIterator = pDestinationManipulator->Iterate( true, ECT_CACHE_LOCAL );
		while ( !pDestinationManipulatorIterator->IsEnd() )
		{
			pDestinationManipulatorIterator->GetName( &szName );
			if ( szName != "uid" )
			{
				if ( const SPropertyDesc *pDesc = dynamic_cast<const SPropertyDesc*>( pDestinationManipulatorIterator->GetDesc() ) )
				{
					if ( ( !pDesc->bArray || ( szName[szName.size() - 1] == ARRAY_NODE_END_CHAR ) ) && ( !pDesc->bStruct ) )
					{
						// добавление 
						destinationFields[szName] = 0;
					}
				}
			}
			pDestinationManipulatorIterator->Next();
		}
	}

	//копируем поля
	{
		CPtr<IManipulatorIterator> pSourceManipulatorIterator = pSourceManipulator->Iterate( true, ECT_CACHE_LOCAL );
		std::list<std::string> arrays; // стек по массивам
		while ( !pSourceManipulatorIterator->IsEnd() )
		{
			pSourceManipulatorIterator->GetName( &szName );
			if ( szName != "uid" )
			{
				if ( const SPropertyDesc *pDesc = dynamic_cast<const SPropertyDesc*>( pSourceManipulatorIterator->GetDesc() ) )
				{
					if ( ( !pDesc->bArray || ( szName[szName.size() - 1] == ARRAY_NODE_END_CHAR ) ) && ( !pDesc->bStruct ) )
					{
						if ( bEqual || ( destinationFields.find( szName ) != destinationFields.end() ) )
						{
							//DebugTrace( "CloneDBManipulator(): CopyValue <%s>", szName.c_str() );

							CVariant value;
							pSourceManipulator->GetValue( szName, &value );
							pDestinationManipulator->SetValue( szName, value );
							
						}
					}
				}
			}
			pSourceManipulatorIterator->Next();
		}
	}
	return true;
}


bool CManipulatorManager::GetParamsFromReference( const std::string &rszRefValueName,
																									const IManipulator *pSourceManipulator,
																									std::string *pszRefObjectTypeName,
																									std::string *pszRefObjectName,
																									const SPropertyDesc **ppRefDesc )
{
	NI_ASSERT( pSourceManipulator != 0, "CManipulatorManager::GetParamsFromReference(): GetManipulatorFromRef == 0" );
	if ( pSourceManipulator == 0 )
	{
		return false;
	}
	if ( const SPropertyDesc *pRefDesc = dynamic_cast<const SPropertyDesc*>( pSourceManipulator->GetDesc( rszRefValueName ) ) )
	{
		EPCIEType nType = typePCIEMnemonics.Get( pRefDesc, rszRefValueName );
		if ( typePCIEMnemonics.IsRef( nType ) )
		{
			CVariant refValue;
			if ( pSourceManipulator->GetValue( rszRefValueName, &refValue ) )
			{
				if ( !IsDBIDEmpty(refValue) ) 
				{
					std::string szRefObjectTypeName;
					std::string szRefObjectName;
					CStringManager::GetTypeAndNameFromRefValue( &szRefObjectTypeName,
																											&szRefObjectName,
																											refValue.GetStr(),
																											TYPE_SEPARATOR_CHAR,
																											pRefDesc->refTypes.begin()->first );
					if ( pszRefObjectTypeName != 0 )
					{
						( *pszRefObjectTypeName ) = szRefObjectTypeName;
					}
					if ( pszRefObjectName != 0 )
					{
						( *pszRefObjectName ) = szRefObjectName;
					}
					if ( ppRefDesc != 0 )
					{
						( *ppRefDesc ) = pRefDesc;
					}
					return true;
				}
			}
		}
	}
	return false;
}


IManipulator* CManipulatorManager::CreateManipulatorFromReference( const std::string &rszRefValueName,
																																	 const IManipulator *pSourceManipulator,
																																	 std::string *pszRefObjectTypeName,
																																	 std::string *pszRefObjectName,
																																	 const SPropertyDesc **ppRefDesc )
{
	NI_ASSERT( pSourceManipulator != 0, "CManipulatorManager::CreateManipulatorFromReference(): GetManipulatorFromRef == 0" );
	if ( pSourceManipulator == 0 )
	{
		return nullptr;
	}
	std::string szRefObjectTypeName;
	std::string szRefObjectName;
	if ( GetParamsFromReference( rszRefValueName, pSourceManipulator, &szRefObjectTypeName, &szRefObjectName, ppRefDesc ) )
	{
		if ( pszRefObjectTypeName != 0 )
		{
			( *pszRefObjectTypeName ) = szRefObjectTypeName;
		}
		if ( pszRefObjectName != 0 )
		{
			( *pszRefObjectName ) = szRefObjectName;
		}
		return Singleton<IResourceManager>()->CreateObjectManipulator( szRefObjectTypeName, szRefObjectName );
	}
	return 0;
}


bool CManipulatorManager::ForceCreateManipulatorForReference( CPtr<IManipulator> *pResultManipulator,
																															IManipulator *pManipulator,
																															const std::string &szTableName,
																															const std::string &szFieldName,
																															const std::string &szReferenceName,
																															std::string *pszResultName )
{
	std::string sManipulatorName, sManipulatorCurrentName;
	std::string sExt;

	if ( pManipulator->GetName( -1, &sManipulatorName ) )
	{
		sExt = NFile::GetFileExt( sManipulatorName );
		sManipulatorName = NFile::CutFileExt( sManipulatorName, 0 );
		sManipulatorName += "-" + szReferenceName + sExt;
	}

	if ( *pResultManipulator = CManipulatorManager::CreateManipulatorFromReference( szFieldName, pManipulator, 0, pszResultName, 0 ) )
	{
		if((*pResultManipulator)->GetName(-1, &sManipulatorCurrentName) && (sManipulatorCurrentName == sManipulatorName))
  			return true;
	}

	if ( CPtr< IManipulator > pFolderManipulator = Singleton<IResourceManager>()->CreateFolderManipulator( szTableName ) )
	{
		if ( !pFolderManipulator->InsertNode( sManipulatorName, NODE_ADD_INDEX ) ) return false;

		if ( CManipulatorManager::SetValue( sManipulatorName, pManipulator, szFieldName ) )
		{
			*pResultManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( szTableName, sManipulatorName );
			if ( pszResultName ) *pszResultName = sManipulatorName;
			return true;
		}
	}
	return false;
}


bool CManipulatorManager::EnumReferences( CReferenceInfoList *pReferenceInfoList,
																					const IManipulator* pSourceManipulator,
																					const unsigned nFlags,
																					const bool bEnumHidden,
																					const ECacheType eCacheType )
{
  NI_ASSERT( pReferenceInfoList != 0, "EnumReferences() pReferenceInfoList == 0" );
  NI_ASSERT( pSourceManipulator != 0, "EnumReferences() pSourceManipulator == 0" );
	//
	std::string szRefValue;
	std::unordered_map<std::string, unsigned> propertyMap;
	if ( nFlags & REFINFO_MAKE_UNIQUE_LIST )
	{
		for ( CReferenceInfoList::const_iterator itReferenceInfo = pReferenceInfoList->begin(); itReferenceInfo != pReferenceInfoList->end(); ++itReferenceInfo )
		{
			CStringManager::GetRefValueFromTypeAndName( &szRefValue, itReferenceInfo->szObjectTypeName, itReferenceInfo->szObjectName, TYPE_SEPARATOR_CHAR );
			propertyMap[szRefValue] = 0;
		}
	}
	//
	if ( const CPtr<IManipulatorIterator> pManipulatorIterator = const_cast<IManipulator*>( pSourceManipulator )->Iterate( bEnumHidden, eCacheType ) )
	{
		std::string szName;
		while ( !pManipulatorIterator->IsEnd() )
		{
			szName.clear();
			pManipulatorIterator->GetName( &szName );
			//
			std::string szObjectTypeName;
			std::string szObjectName;
			const SPropertyDesc *pRefDesc = 0;
			//
			if ( GetParamsFromReference( szName,
																	 pSourceManipulator,
																	 ( nFlags & ( REFINFO_MAKE_UNIQUE_LIST | REFINFO_OBJECT_TYPE_NAME | REFINFO_CHECK_EMPTY | REFINFO_CHECK_VALID ) ) ? &szObjectTypeName : 0,
																	 ( nFlags & ( REFINFO_MAKE_UNIQUE_LIST | REFINFO_OBJECT_NAME | REFINFO_CHECK_EMPTY | REFINFO_CHECK_VALID ) ) ? &szObjectName : 0,
																	 ( nFlags & REFINFO_CHECK_VALID ) ? &pRefDesc : 0 ) )
			{
				bool bInsert = true;
				if ( nFlags & REFINFO_MAKE_UNIQUE_LIST )
				{
					CStringManager::GetRefValueFromTypeAndName( &szRefValue, szObjectTypeName, szObjectName, TYPE_SEPARATOR_CHAR );
					bInsert = ( propertyMap.find( szRefValue ) != propertyMap.end() );
					if ( bInsert )
					{
						propertyMap[szRefValue] = 0;
					}
				}
				if ( bInsert )
				{
					CReferenceInfoList::iterator posReferenceInfo = pReferenceInfoList->insert( pReferenceInfoList->end(), SReferenceInfo() );
					posReferenceInfo->nFlags = nFlags;
					if ( posReferenceInfo->nFlags & REFINFO_PROPERTY_NAME ) 
					{
						posReferenceInfo->szName = szName;
					}
					if ( posReferenceInfo->nFlags & REFINFO_OBJECT_TYPE_NAME ) 
					{
						posReferenceInfo->szObjectTypeName = szObjectTypeName;
					}
					if ( posReferenceInfo->nFlags & REFINFO_OBJECT_NAME ) 
					{
						posReferenceInfo->szObjectName = szObjectName;
					}
					if ( posReferenceInfo->nFlags & REFINFO_CHECK_EMPTY )
					{
						posReferenceInfo->isEmpty = ( ( !szObjectTypeName.empty() ) && ( szObjectName.empty() ) );
					}
					if ( posReferenceInfo->nFlags & REFINFO_CHECK_VALID ) 
					{
						if ( szObjectTypeName.empty() || szObjectName.empty() )
						{
							if ( pRefDesc && ( !pRefDesc->bUnsafe ) )
							{
								posReferenceInfo->isValid = false;
							}
						}
					}
				}
			}
			pManipulatorIterator->Next();
		}
	}
	return true;
}


bool CManipulatorManager::EnsureArraySize( const int nSize, IManipulator *pManipulator, const std::string &rszArrayName )
{
	int nExistingCount = 0;
	bool bResult = GetValue( &nExistingCount, pManipulator, rszArrayName );
	if ( bResult )
	{
		while ( nExistingCount < nSize )
		{
			bResult = bResult && pManipulator->InsertNode( rszArrayName, NODE_ADD_INDEX );
			if ( !bResult )
			{
				break;
			}
			++nExistingCount;
		}
		if ( bResult )
		{
			while ( nExistingCount > nSize )
			{
				bResult = bResult && pManipulator->RemoveNode( rszArrayName, nExistingCount - 1 );
				if ( !bResult )
				{
					break;
				}
				--nExistingCount;
			}
		}
	}
	return bResult;
}


bool CManipulatorManager::Remove2DArray( struct IManipulator *pManipulator, const std::string &rszName )
{
	NI_ASSERT( pManipulator != 0, "CManipulatorManager::Remove2DArray(): pManipulator == 0" );
	if ( pManipulator == 0 )
	{
		return false;
	}
	const std::string sz2DArrayName = StrFmt( "%s%cdata", rszName.c_str(), LEVEL_SEPARATOR_CHAR );
	int nExistingXCount = 0;
	bool bResult = GetValue( &nExistingXCount, pManipulator, sz2DArrayName );
	if ( bResult )
	{
		for ( int nXIndex = 0; nXIndex < nExistingXCount; ++nXIndex )
		{
			const std::string szArrayName = StrFmt( "%s%c%c%d%c%cdata", sz2DArrayName.c_str(), LEVEL_SEPARATOR_CHAR, ARRAY_NODE_START_CHAR, nXIndex, ARRAY_NODE_END_CHAR, LEVEL_SEPARATOR_CHAR );
			bResult = pManipulator->RemoveNode( szArrayName );
			if ( !bResult )
			{
				break;
			}
		}
		bResult = bResult && pManipulator->RemoveNode( sz2DArrayName );
	}
	return bResult;
}


void CManipulatorManager::Trace( const std::string &rszPrefix, IManipulator* pManipulator )
{
	NI_ASSERT( pManipulator != 0, "CManipulatorManager::Trace(): pManipulator == 0" );
	if ( pManipulator == 0 )
	{
		return;
	}
	if ( CPtr<IManipulatorIterator> pManipulatorIterator = pManipulator->Iterate( true, ECT_NO_CACHE ) )
	{
		bool bFound = false;
		const int nPrefixSize = rszPrefix.size();
		while ( !pManipulatorIterator->IsEnd() )
 		{
			std::string szName;
			pManipulatorIterator->GetName( &szName );
			if ( !rszPrefix.empty() )
			{
				if ( szName.compare( 0, nPrefixSize, rszPrefix ) != 0 )
				{
					if ( bFound )
					{
						break;
					}
				}
				else
				{
					bFound = true;
					DebugTrace( "%s", szName.c_str() );
				}
			}
			pManipulatorIterator->Next();
		}
	}
}

// basement storage  


