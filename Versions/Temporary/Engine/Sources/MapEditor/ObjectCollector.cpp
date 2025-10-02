#include "StdAfx.h"
#include "ResourceDefines.h"

#include "ObjectCollector.h"
#include "libdb/ResourceManager.h"
#include "MapEditorLib/Interface_Logger.h"
#include "Misc/HPTimer.h"

int CObjectFilterCollector::SObjectFilter::SPart::operator&( IXmlSaver &saver )
{
	saver.Add( "Operation", &szOperation );
	saver.Add( "ObjectType", &szObjectType );
	saver.Add( "Names", &nameList );
	//
	return 0;
}

int CObjectFilterCollector::SObjectFilter::operator&( IXmlSaver &saver )
{
	saver.Add( "Name", &szName );
	saver.Add( "Parts", &partList );
	saver.Add( "Separator", &bSeparator );
	return 0;
}

/**
void CObjectCollector::LoadUnicodeText( CString *pstrText, const string &rszFileName )
{
	if ( pstrText )
	{
		pstrText->Empty();
		if ( CPtr<IDataStream> pFileStream = OpenStream( rszFileName ) )
		{
			vector<BYTE> fileBuffer;
			fileBuffer.resize( pFileStream->GetSize() );
			pFileStream->Read( &( fileBuffer[0] ), fileBuffer.size() );
			//
			if ( ( fileBuffer.size() > 3 ) && ( fileBuffer[0] == 0xFF ) && ( fileBuffer[1] == 0xFE ) )
			{
				pstrText->Empty();
				wstring wszText;
				wszText.resize( ( fileBuffer.size() - 2 ) / sizeof( wchar_t ) );
				memcpy( &( wszText[0] ), &( fileBuffer[0] ) + 2, wszText.size() * sizeof( wchar_t ) );
				//wszText.erase( remove_if( wszText.begin(), wszText.end(), bind2nd( std::equal_to<wchar_t>(), 0x0D ) ), wszText.end() );
				// отрезаем переносы строк с обратного конца
				int nLastIndex = 0;
				for ( nLastIndex = ( wszText.size() - 1 ); nLastIndex >= 0; --nLastIndex )
				{
					if ( ( wszText[nLastIndex] != 0x0A ) && ( wszText[nLastIndex] != 0x0D ) )
					{
						break;
					}
				}
				if ( nLastIndex < 0 )
				{
					wszText.clear();
				}
				else if ( nLastIndex < ( wszText.size() - 1 ) )
				{
					wszText = wszText.substr( 0, nLastIndex + 1 );
				}
				// переводим в СString
				const int nBufferLength = ::WideCharToMultiByte( ::GetACP(), 0, wszText.c_str(), wszText.length(), 0, 0, 0, 0 );
				LPTSTR lptStr = pstrText->GetBuffer( nBufferLength );
				::WideCharToMultiByte( ::GetACP(), 0, wszText.c_str(), wszText.length(), lptStr, nBufferLength, 0, 0 );
				pstrText->ReleaseBuffer();
			}
		}
	}
}
/**/


const string CObjectCollector::DEFAULT_DATA_EXTRACTOR_TYPE = "_DEFAULT_DATA_EXTRACTOR_TYPE_";


bool CObjectFilterCollector::SObjectFilter::InsertObjectToCollection( CObjectCollection *pObjectCollection, const string &rszObjectTypeName, const string &rszObjectName ) const
{
	if ( pObjectCollection != 0 )
	{
		CObjectCollection::iterator posObjectCollection = pObjectCollection->find( rszObjectTypeName );
		if ( posObjectCollection == pObjectCollection->end() )
		{
			( *pObjectCollection )[rszObjectTypeName] = CObjectNameCollection();
			posObjectCollection = pObjectCollection->find( rszObjectTypeName );
		}
		if ( posObjectCollection != pObjectCollection->end() )
		{
			CObjectNameCollection::iterator posObjectNameCollection = posObjectCollection->second.find( rszObjectName );
			if ( posObjectNameCollection == posObjectCollection->second.end() )
			{
				posObjectCollection->second[rszObjectName] = 0;
				return true;
			}
		}
	}
	return false;
}


int CObjectFilterCollector::SObjectFilter::GetObjectCollection( CObjectCollection *pObjectCollection, const string &rszObjectTypeName ) const
{
	int nObjectsFound = 0;
	if ( pObjectCollection != 0 )
	{
		if ( CPtr<IManipulator> pFolderManipulator = Singleton<IResourceManager>()->CreateFolderManipulator( rszObjectTypeName ) )
		{
			CPtr<IManipulatorIterator> pFolderManipulatorIterator = pFolderManipulator->Iterate( true, ECT_NO_CACHE );
			if ( pFolderManipulatorIterator != 0 )
			{
				while ( !pFolderManipulatorIterator->IsEnd() )
				{
					if ( !pFolderManipulatorIterator->IsFolder() )
					{
						string szObjectName;
						if ( pFolderManipulatorIterator->GetName( &szObjectName ) )
						{
							if ( InsertObjectToCollection( pObjectCollection, rszObjectTypeName, szObjectName ) )
							{
								++nObjectsFound;
							}
						}
					}
					pFolderManipulatorIterator->Next();
				}
			}
		}
	}
	return nObjectsFound;
}


void CObjectFilterCollector::SObjectFilter::ExtractObjectsForFilterPart( CObjectNameCollection *pObjectNameCollection, const SPart &rPart ) const
{
	if ( pObjectNameCollection != 0 )
	{
		if ( CPtr<IManipulator> pFolderManipulator = Singleton<IResourceManager>()->CreateFolderManipulator( rPart.szObjectType ) )
		{
			CPtr<IManipulatorIterator> pFolderManipulatorIterator = pFolderManipulator->Iterate( true, ECT_NO_CACHE );
			if ( pFolderManipulatorIterator != 0 )
			{
				string szFolderPrefix;
				for ( ; !pFolderManipulatorIterator->IsEnd(); pFolderManipulatorIterator->Next() ) 
				{
					string szObjectName;
					if ( pFolderManipulatorIterator->GetName( &szObjectName ) )
					{
						bool bMatch = false;
						for ( CNameList::const_iterator itName = rPart.nameList.begin(); itName != rPart.nameList.end(); ++itName )
						{
							if ( szObjectName.size() >= itName->size() )
							{
								const int nMinSize = Min( szObjectName.size(), itName->size() );
								if ( NFile::ComparePathEq(0, nMinSize, szObjectName, 0, nMinSize, *itName) == true )
								{
									bMatch = true;
									break;
								}
							}
						}
						if ( bMatch )
						{
							( *pObjectNameCollection )[szObjectName] = 0;
						}
					}
				}
			}
		}
	}
}


void CObjectFilterCollector::SObjectFilter::MergeSets( CObjectNameCollection *pDestination, const CObjectNameCollection &rSource, const string &szOperationType ) const
{
	if ( pDestination != 0 )
	{
		if ( szOperationType == "UNION" )									// class set
		{
			pDestination->insert( rSource.begin(), rSource.end() );
		}
		else if ( szOperationType == "MEET" )							// meet of set
		{
			for ( CObjectNameCollection::iterator itObjectNameCollection = pDestination->begin(); itObjectNameCollection != pDestination->end();  ) 
			{
				if ( rSource.find( itObjectNameCollection->first ) == rSource.end() )
				{
					pDestination->erase( itObjectNameCollection++ );
				}
				else
				{
					++itObjectNameCollection;
				}
			}
		}
		else if ( szOperationType == "SUB" )							// set substruction
		{
			for ( CObjectNameCollection::const_iterator itObjectNameCollection = rSource.begin(); itObjectNameCollection != rSource.end(); ++itObjectNameCollection ) 
			{
				CObjectNameCollection::iterator posObjectNameCollection = pDestination->find( itObjectNameCollection->first );
				if ( posObjectNameCollection != pDestination->end() )
				{
					pDestination->erase( posObjectNameCollection );
				}
			}
		}
	}
}


int CObjectFilterCollector::SObjectFilter::GetObjectCollection( CObjectCollection *pObjectCollection ) const
{
	if ( bSeparator || partList.empty() )
	{
		return 0;
	}
	if ( bCached )
	{
		if ( pObjectCollection != 0 )
		{
			( *pObjectCollection ) = objectCollection;
		}
		return nObjectCollectionCount;
	}
	//
	objectCollection.clear();
	nObjectCollectionCount = 0;
	// extract and merge sets
	for ( CPartList::const_iterator itPart = partList.begin(); itPart != partList.end(); ++itPart ) 
	{
		if ( itPart->nameList.empty() ) 
		{
			nObjectCollectionCount += GetObjectCollection( pObjectCollection, itPart->szObjectType );
		}
		else	
		{
			CObjectNameCollection &rObjectNameCollection = objectCollection[itPart->szObjectType];
			CObjectNameCollection objectNameCollection;
			ExtractObjectsForFilterPart( &objectNameCollection, *itPart );
			MergeSets( &rObjectNameCollection, objectNameCollection, itPart->szOperation );
		}
	}
	// add result sets
	for ( CObjectCollection::const_iterator itObjectCollection = objectCollection.begin(); itObjectCollection != objectCollection.end(); ++itObjectCollection ) 
	{
		for ( CObjectNameCollection::const_iterator itObjectNameCollection = itObjectCollection->second.begin(); itObjectNameCollection != itObjectCollection->second.end(); ++itObjectNameCollection ) 
		{
			if ( InsertObjectToCollection( pObjectCollection, itObjectCollection->first, itObjectNameCollection->first ) )
			{
				++nObjectCollectionCount;
			}
		}
	}
	//
	bCached = true;
	if ( pObjectCollection != 0 )
	{
		( *pObjectCollection ) = objectCollection;
	}
	return nObjectCollectionCount;
}


bool CObjectFilterCollector::SObjectFilter::Match( const string &szObjectTypeName, const string &szObjectName ) const
{
	return false;
}


const CObjectFilterCollector::SObjectFilter* CObjectFilterCollector::LocateObjectFilter( const string &rszFilterType, const int nFilterIndex ) const
{
	CObjectFilterListMap::const_iterator posObjectFilterList = objectFilterListMap.find( rszFilterType );
	if ( posObjectFilterList != objectFilterListMap.end() )
	{
		if ( ( nFilterIndex >= 0 ) && ( nFilterIndex < posObjectFilterList->second.size() ) )
		{
			return &( posObjectFilterList->second[nFilterIndex] );
		}
	}
	return 0;
}


bool CObjectFilterCollector::Load( CDataStream *pStream )
{
	try
	{
		CPtr<IXmlSaver> pSaver = CreateXmlSaver( pStream, SAVER_MODE_READ );
		pSaver->Add( "Filters", &objectFilterListMap );
	}
	catch ( ... ) 
	{
		NLog::GetLogger()->Log( LT_ERROR, "Can't load filters map\n" );
		return false;
	}
	return true;
}


bool CObjectFilterCollector::Save( CDataStream *pStream )
{
	try
	{
		CPtr<IXmlSaver> pSaver = CreateXmlSaver( pStream, SAVER_MODE_WRITE );
		pSaver->Add( "Filters", &objectFilterListMap );
	}
	catch ( ... ) 
	{
		NLog::GetLogger()->Log( LT_ERROR, "Can't save filters map\n" );
		return false;
	}
	return true;
}


int CObjectFilterCollector::GetFilterList( CFilterList* pFilterList, const string &rszFilterType ) const
{
	if ( pFilterList != 0 )
	{
		CObjectFilterListMap::const_iterator posObjectFilterList = objectFilterListMap.find( rszFilterType );
		if ( posObjectFilterList != objectFilterListMap.end() )
		{
			const int nFilterCount = posObjectFilterList->second.size();
			for ( int nFilterIndex = 0; nFilterIndex < nFilterCount; ++nFilterIndex )
			{
				pFilterList->push_back( posObjectFilterList->second[nFilterIndex].szName.c_str() );
			}
			return nFilterCount;
		}
	}
	return 0;
}


bool CObjectFilterCollector::IsSeparator( const string &rszFilterType, const int nFilterIndex ) const
{
	if ( const SObjectFilter* pObjectFilter = LocateObjectFilter( rszFilterType, nFilterIndex ) )
	{
		return pObjectFilter->bSeparator;
	}
	return false;
}


const IObjectFilter* CObjectFilterCollector::Get( const string &rszFilterType, const int nFilterIndex ) const
{
	return LocateObjectFilter( rszFilterType, nFilterIndex );
}


int CObjectFilterCollector::ShowFilterSelectionDialog( CWnd* pParentWindow, string *pszFilterType, int *pnFilterIndex )
{
	return IDCANCEL;
}


int CObjectFilterCollector::ShowFilterCreationDialog( CWnd* pParentWindow, string *pszFilterType, int *pnFilterIndex )
{
	return IDCANCEL;
}


void CObjectCollector::CreateImageLists()
{
	const COLORREF zeroColor = RGB( 0, 0, 0 );
	//
	CBitmap defaultNormalObjectBitmap;
	CBitmap defaultSmallObjectBitmap;
	defaultNormalObjectBitmap.LoadBitmap( IDB_DEFAULT_NORMAL_OBJECT_IMAGE );
	defaultSmallObjectBitmap.LoadBitmap( IDB_DEFAULT_SMALL_OBJECT_IMAGE );
	//
	normalImageList.Create( NORMAL_IMAGE_SIZE_X, NORMAL_IMAGE_SIZE_Y, ILC_COLOR24, 0, 10 );
	smallImageList.Create( SMALL_IMAGE_SIZE_X, SMALL_IMAGE_SIZE_Y, ILC_COLOR24, 0, 10 );
	//
	const int nDefaultNormalImageIndex = normalImageList.Add( &defaultNormalObjectBitmap, zeroColor );
	const int nDefaultSmallImageIndex = smallImageList.Add( &defaultSmallObjectBitmap, zeroColor );
	NI_ASSERT( nDefaultNormalImageIndex == nDefaultSmallImageIndex, StrFmt( "nDefaultNormalImageIndex != nDefaultSmallImageIndex" ) );
	nDefaultImageIndex = nDefaultNormalImageIndex;
}


const string& CObjectCollector::LocateExtractorType( const string &rszObjectTypeName ) const
{
	for ( CDataExtractorTypeMap::const_iterator itDataExtractorType = dataExtractorTypeMap.begin();
				itDataExtractorType != dataExtractorTypeMap.end();
				++itDataExtractorType )
	{
		CObjectTypeNameList::const_iterator posObjectTypeName = find( itDataExtractorType->second.begin(), itDataExtractorType->second.end(), rszObjectTypeName );
		if ( posObjectTypeName != itDataExtractorType->second.end() )
		{
			return itDataExtractorType->first;
		}
	}
	return DEFAULT_DATA_EXTRACTOR_TYPE;
}


const IObjectCollector::SObjectParams* CObjectCollector::LocateObjectParams( const string &rszObjectTypeName, const string &rszObjectName ) const
{
	CObjectCollection::const_iterator posObjectCollection = objectCollection.find( rszObjectTypeName );
	if ( posObjectCollection != objectCollection.end() )
	{
		CObjectNameCollection::const_iterator posObjectNameCollection = posObjectCollection->second.find( rszObjectName );
		if ( posObjectNameCollection != posObjectCollection->second.end() )
		{
			return &( posObjectNameCollection->second );
		}
	}
	return 0;
}


const IObjectCollector::SObjectParams* CObjectCollector::GetObjectParams( const string &rszObjectTypeName, const string &rszObjectName, const string &rszDataExtractorType )
{
	const SObjectParams *pLocatedObjectParams = LocateObjectParams( rszObjectTypeName, rszObjectName );
	if ( pLocatedObjectParams == 0 )
	{
		InsertObject( rszObjectTypeName, rszObjectName, rszDataExtractorType );
		pLocatedObjectParams = LocateObjectParams( rszObjectTypeName, rszObjectName );
	}
	return pLocatedObjectParams;
}


void CObjectCollector::FillObjectParams( SObjectParams *pObjectParams, const string &rszObjectTypeName, const string &rszObjectName, const string &rszDataExtractorType )
{
	if ( pObjectParams )
	{
		UINT nFlags = 0;
		CDataExtractorMap::iterator posDataExtractor = dataExtractorMap.find( rszDataExtractorType );
		if ( posDataExtractor == dataExtractorMap.end() )
		{
			posDataExtractor = dataExtractorMap.find( DEFAULT_DATA_EXTRACTOR_TYPE );
		}
		if ( posDataExtractor != dataExtractorMap.end() )
		{
			CBitmap normalBitmap;
			CBitmap smallBitmap;
			CString strLabel;
			nFlags = posDataExtractor->second->GetObjectData( &normalBitmap,
																												&smallBitmap,
																												&strLabel,
																												rszObjectTypeName,
																												rszObjectName,
																												rszDataExtractorType );
			if ( ( nFlags & OCDE_NORMAL_BITMAP ) && ( nFlags & OCDE_SMALL_BITMAP ) )
			{
				const COLORREF zeroColor = RGB( 0, 0, 0 );
				//
				const int nNormalImageIndex = normalImageList.Add( &normalBitmap, zeroColor );
				const int nSmallImageIndex = smallImageList.Add( &smallBitmap, zeroColor );
				NI_ASSERT( nNormalImageIndex == nSmallImageIndex, StrFmt( "nNormalImageIndex != nSmallImageIndex" ) );
				//
				pObjectParams->nIconIndex = nNormalImageIndex;
			}
			if ( nFlags & OCDE_LABEL )
			{
				pObjectParams->strLabel = strLabel;
			}
		}
		if ( ( ( nFlags & OCDE_NORMAL_BITMAP ) == 0 ) || ( ( nFlags & OCDE_SMALL_BITMAP ) == 0 ) )
		{
			pObjectParams->nIconIndex = nDefaultImageIndex;
		}
		if ( ( nFlags & OCDE_LABEL ) == 0 )
		{
			if ( rszObjectTypeName.empty() )
			{
				pObjectParams->strLabel = rszObjectName.c_str();
			}
			else
			{
				pObjectParams->strLabel = StrFmt( "%s%c%s", rszObjectTypeName.c_str(), TYPE_SEPARATOR_CHAR, rszObjectName.c_str() );
			}
		}
	}
}


bool CObjectCollector::InsertObjectToCollection( CObjectCollection *pObjectCollection, const string &rszObjectTypeName, const string &rszObjectName, const SObjectParams* pObjectParams ) const
{
	bool bResult = false;
	if ( pObjectCollection != 0 )
	{
		CObjectCollection::iterator posObjectCollection = pObjectCollection->find( rszObjectTypeName );
		if ( posObjectCollection == pObjectCollection->end() )
		{
			( *pObjectCollection )[rszObjectTypeName] = CObjectNameCollection();
			posObjectCollection = pObjectCollection->find( rszObjectTypeName );
		}
		if ( posObjectCollection != pObjectCollection->end() )
		{
			CObjectNameCollection::iterator posObjectNameCollection = posObjectCollection->second.find( rszObjectName );
			if ( posObjectNameCollection == posObjectCollection->second.end() )
			{
				bResult = true;
				posObjectCollection->second[rszObjectName] = ( *pObjectParams );
			}
			else
			{
				posObjectNameCollection->second = ( *pObjectParams );
			}
		}
	}
	return bResult;
}


bool CObjectCollector::InsertObjectToCollection( const string &rszObjectTypeName, const string &rszObjectName, const string &rszDataExtractorType )
{
	CObjectCollection::iterator posObjectCollection = objectCollection.find( rszObjectTypeName );
	if ( posObjectCollection == objectCollection.end() )
	{
		objectCollection[rszObjectTypeName] = CObjectNameCollection();
		posObjectCollection = objectCollection.find( rszObjectTypeName );
	}
	if ( posObjectCollection != objectCollection.end() )
	{
		CObjectNameCollection::iterator posObjectNameCollection = posObjectCollection->second.find( rszObjectName );
		if ( posObjectNameCollection == posObjectCollection->second.end() )
		{
			posObjectCollection->second[rszObjectName] = SObjectParams();
			posObjectNameCollection = posObjectCollection->second.find( rszObjectName );
			if ( posObjectNameCollection != posObjectCollection->second.end() )
			{
				FillObjectParams( &( posObjectNameCollection->second ), rszObjectTypeName, rszObjectName, rszDataExtractorType );
				return true;
			}
		}
	}
	return false;
}


bool CObjectCollector::RemoveObjectFromCollection( const string &rszObjectTypeName, const string &rszObjectName )
{
	CObjectCollection::iterator posObjectCollection = objectCollection.find( rszObjectTypeName );
	if ( posObjectCollection != objectCollection.end() )
	{
		CObjectNameCollection::iterator posObjectNameCollection = posObjectCollection->second.find( rszObjectName );
		bool bResult = false;
		if ( posObjectNameCollection != posObjectCollection->second.end() )
		{
			posObjectCollection->second.erase( posObjectNameCollection );
			bResult = true;
		}
		if ( posObjectCollection->second.empty() )
		{
			objectCollection.erase( posObjectCollection );
		}
		return bResult;
	}
	return false;
}


void CObjectCollector::InsertObject( const string &rszObjectTypeName, const string &rszObjectName, const string &rszDataExtractorType )
{
	if ( InsertObjectToCollection( rszObjectTypeName, rszObjectName, rszDataExtractorType ) )
	{
		for ( CObjectCollectorCallbackMap::iterator itObjectCollectorCallback = objectCollectorCallbackMap.begin();
					itObjectCollectorCallback != objectCollectorCallbackMap.end();
					++itObjectCollectorCallback )
		{
			if ( itObjectCollectorCallback->first != 0 )
			{
				itObjectCollectorCallback->first->OnInsertObject( rszObjectTypeName, rszObjectName );
			}
		}
	}
}


void CObjectCollector::RemoveObject( const string &rszObjectTypeName, const string &rszObjectName )
{
	if ( RemoveObjectFromCollection( rszObjectTypeName, rszObjectName ) )
	{
		for ( CObjectCollectorCallbackMap::iterator itObjectCollectorCallback = objectCollectorCallbackMap.begin();
					itObjectCollectorCallback != objectCollectorCallbackMap.end();
					++itObjectCollectorCallback )
		{
			if ( itObjectCollectorCallback->first != 0 )
			{
				itObjectCollectorCallback->first->OnRemoveObject( rszObjectTypeName, rszObjectName );
			}
		}
	}
}


bool CObjectCollector::Load( CDataStream *pStream )
{
	try
	{
		CPtr<IXmlSaver> pSaver = CreateXmlSaver( pStream, SAVER_MODE_READ );
		pSaver->Add( "Extractors", &dataExtractorTypeMap );
	}
	catch ( ... ) 
	{
		NLog::GetLogger()->Log( LT_ERROR, "Can't load extractors map\n" );
		return false;
	}
	return true;
}


bool CObjectCollector::Save( CDataStream *pStream )
{
	try
	{
		CPtr<IXmlSaver> pSaver = CreateXmlSaver( pStream, SAVER_MODE_WRITE );
		pSaver->Add( "Extractors", &dataExtractorTypeMap );
	}
	catch ( ... ) 
	{
		NLog::GetLogger()->Log( LT_ERROR, "Can't save extractors map\n" );
		return false;
	}
	return true;
}


void CObjectCollector::RegisterDataExtractor( IObjectDataExtractor *pDataExtractor )
{
	RegisterDataExtractor( DEFAULT_DATA_EXTRACTOR_TYPE, pDataExtractor );
}


void CObjectCollector::RegisterDataExtractor( const string &rszDataExtractorType, IObjectDataExtractor *pDataExtractor )
{
	if ( dataExtractorMap.find( rszDataExtractorType ) != dataExtractorMap.end() )
	{
		ILogger *pLogger = NLog::GetLogger();
		pLogger->Log( LT_ERROR, "Object data extractor already registered\n" );
		pLogger->Log( LT_ERROR, StrFmt("\tType: %s\n", rszDataExtractorType.c_str()) );
		return;
	}
	//
	dataExtractorMap[rszDataExtractorType] = pDataExtractor;
}


void CObjectCollector::InsertCallback( IObjectCollectorCallback *pObjectCollectorCallback )
{
	if ( pObjectCollectorCallback != 0 )
	{
		objectCollectorCallbackMap[pObjectCollectorCallback] = 0;
	}
}


void CObjectCollector::RemoveCallback( IObjectCollectorCallback *pObjectCollectorCallback )
{
	if ( pObjectCollectorCallback != 0 )
	{
		CObjectCollectorCallbackMap::iterator posObjectCollectorCallback = objectCollectorCallbackMap.find( pObjectCollectorCallback );
		if ( posObjectCollectorCallback != objectCollectorCallbackMap.end() )
		{
			objectCollectorCallbackMap.erase( posObjectCollectorCallback );
		}
	}
}


void CObjectCollector::ClearCallbackList()
{
	objectCollectorCallbackMap.clear();
}


int CObjectCollector::ApplyFilter( CObjectCollection *pObjectCollection, const string &rszObjectTypeName )
{
	int nObjectsFound = 0;
	if ( pObjectCollection != 0 )
	{
		if ( CPtr<IManipulator> pFolderManipulator = Singleton<IResourceManager>()->CreateFolderManipulator( rszObjectTypeName ) )
		{
			CPtr<IManipulatorIterator> pFolderManipulatorIterator = pFolderManipulator->Iterate( true, ECT_NO_CACHE );
			if ( pFolderManipulatorIterator != 0 )
			{
				const string &rszDataExtractorType = LocateExtractorType( rszObjectTypeName );
				while ( !pFolderManipulatorIterator->IsEnd() )
				{
					if ( !pFolderManipulatorIterator->IsFolder() )
					{
						string szObjectName;
						if ( pFolderManipulatorIterator->GetName( &szObjectName ) )
						{
							const SObjectParams *pObjectParams = GetObjectParams( rszObjectTypeName, szObjectName, rszDataExtractorType );
							if ( pObjectParams != 0 )
							{
								if ( InsertObjectToCollection( pObjectCollection, rszObjectTypeName, szObjectName, pObjectParams ) )
								{
									++nObjectsFound;
								}
							}
						}
					}
					pFolderManipulatorIterator->Next();
				}
			}
		}
	}
	return nObjectsFound;
}


int CObjectCollector::ApplyFilter( CObjectCollection *pObjectCollection, const IObjectFilter *pObjectFilter )
{
	int nObjectsFound = 0;
	if ( pObjectCollection != 0 )
	{
		IObjectFilter::CObjectCollection filteredObjectCollection;
		if ( pObjectFilter->GetObjectCollection( &filteredObjectCollection ) > 0 )
		{
			for ( IObjectFilter::CObjectCollection::const_iterator posFilteredObjectCollection = filteredObjectCollection.begin();
						posFilteredObjectCollection != filteredObjectCollection.end();
						++posFilteredObjectCollection )
			{
				const string &rszDataExtractorType = LocateExtractorType( posFilteredObjectCollection->first );
				for ( IObjectFilter::CObjectNameCollection::const_iterator posFilteredObjectNameCollection = posFilteredObjectCollection->second.begin();
							posFilteredObjectNameCollection != posFilteredObjectCollection->second.end();
							++posFilteredObjectNameCollection )
				{
					const SObjectParams *pObjectParams = GetObjectParams( posFilteredObjectCollection->first, posFilteredObjectNameCollection->first, rszDataExtractorType );
					if ( pObjectParams != 0 )
					{
						if ( InsertObjectToCollection( pObjectCollection, posFilteredObjectCollection->first, posFilteredObjectNameCollection->first, pObjectParams ) )
						{
							++nObjectsFound;
						}
					}
				}
			}
		}
	}
	return nObjectsFound;
}


bool CObjectCollector::GetObjectParams( SObjectParams* pObjectParams, const string &rszObjectTypeName, const string &rszObjectName )
{
	const SObjectParams *pLocatedObjectParams = GetObjectParams( rszObjectTypeName, rszObjectName, LocateExtractorType( rszObjectTypeName ) );
	if ( pLocatedObjectParams != 0 )
	{
		if ( pObjectParams != 0 )
		{
			( *pObjectParams ) = ( *pLocatedObjectParams );
		}
		return true;
	}
	return false;
}


CImageList* CObjectCollector::GetImageList( int nImageListType )
{
	return ( nImageListType == LVSIL_SMALL ) ? ( &smallImageList ) : ( &normalImageList );
}


void CObjectCollector::ClearCollection()
{
	objectCollection.clear();
	CreateImageLists();
	//
	for ( CObjectCollectorCallbackMap::iterator itObjectCollectorCallback = objectCollectorCallbackMap.begin();
				itObjectCollectorCallback != objectCollectorCallbackMap.end();
				++itObjectCollectorCallback )
	{
		if ( itObjectCollectorCallback->first != 0 )
		{
			itObjectCollectorCallback->first->OnClearCollection();
		}
	}
}

// basement storage 


