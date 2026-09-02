#pragma once

#include "Misc/HashFuncs.h"
#include "Misc/StrProc.h"
#include "libdb/Manipulator.h"
#include "MapEditorLib/Interface_ObjectCollector.h"


class CObjectFilterCollector : public IObjectFilterCollector
{
	OBJECT_NOCOPY_METHODS( CObjectFilterCollector );
	//
	struct SObjectFilter : public IObjectFilter
	{
		typedef std::vector<std::string> CNameList;
		struct SPart
		{
			std::string szOperation;
			std::string szObjectType;
			CNameList nameList;
			//
			int operator&( IXmlSaver &saver );
		};
		typedef std::vector<SPart> CPartList;

		//
		std::string szName;
		CPartList partList;
		bool bSeparator;
		mutable bool bCached;
		mutable CObjectCollection objectCollection;
		mutable int nObjectCollectionCount;
		//
		SObjectFilter()
			: bSeparator( false ),
				bCached( false ),
				nObjectCollectionCount( 0 ) {}
		SObjectFilter( const SObjectFilter &rObjectFilter )
			: szName( rObjectFilter.szName ),
				partList( rObjectFilter.partList ),
				bSeparator( rObjectFilter.bSeparator ),
				bCached( rObjectFilter.bCached ),
				objectCollection( rObjectFilter.objectCollection ),
				nObjectCollectionCount( rObjectFilter.nObjectCollectionCount ) {}
		SObjectFilter& operator=( const SObjectFilter &rObjectFilter )
		{
			if( &rObjectFilter != this )
			{
				szName = rObjectFilter.szName;
				partList = rObjectFilter.partList;
				bSeparator = rObjectFilter.bSeparator;
				bCached = rObjectFilter.bCached;
				objectCollection = rObjectFilter.objectCollection;
				nObjectCollectionCount = rObjectFilter.nObjectCollectionCount;
			}
			return *this;
		}
		//
		int operator&( IXmlSaver &saver );

		bool InsertObjectToCollection( CObjectCollection *pObjectCollection, const std::string &rszObjectTypeName, const std::string &rszObjectName ) const;
		int GetObjectCollection( CObjectCollection *pObjectCollection, const std::string &rszObjectTypeName ) const;
		void ExtractObjectsForFilterPart( CObjectNameCollection *pObjectNameCollection, const SPart &rPart ) const;
		void MergeSets( CObjectNameCollection *pDestination, const CObjectNameCollection &rSource, const std::string &szOperationType ) const;

		//IObjectFilter
		int GetObjectCollection( CObjectCollection *pObjectCollection ) const;
		bool Match( const std::string &szObjectTypeName, const std::string &szObjectName ) const;
	};
	typedef std::vector<SObjectFilter> CObjectFilterList;
	typedef std::unordered_map<std::string, CObjectFilterList> CObjectFilterListMap;
	//
	CObjectFilterListMap objectFilterListMap;

	const SObjectFilter* LocateObjectFilter( const std::string &rszFilterType, const int nFilterIndex ) const;

protected:
	// IObjectFilterCollector
	bool Load( CDataStream *pStream );
	bool Save( CDataStream *pStream );
	//
	int GetFilterList( CFilterList* pFilterList, const std::string &rszFilterType ) const;
	//
	bool IsSeparator( const std::string &rszFilterType, const int nFilterIndex ) const;
	const IObjectFilter* Get( const std::string &rszFilterType, const int nFilterIndex ) const;

	int ShowFilterSelectionDialog( CWnd* pParentWindow, std::string *pszFilterType, int *pnFilterIndex );
	int ShowFilterCreationDialog( CWnd* pParentWindow, std::string *pszFilterType, int *pnFilterIndex );
};


class CObjectCollector : public IObjectCollector
{
	OBJECT_NOCOPY_METHODS( CObjectCollector );

	static const std::string DEFAULT_DATA_EXTRACTOR_TYPE;

	typedef std::vector<std::string> CObjectTypeNameList;
	typedef std::unordered_map<std::string, CObjectTypeNameList> CDataExtractorTypeMap;
	//
	typedef std::unordered_map<IObjectCollectorCallback*, int> CObjectCollectorCallbackMap;
	typedef std::unordered_map<std::string, CObj<IObjectDataExtractor> > CDataExtractorMap;
	//
	CObjectCollection objectCollection;
	CObjectCollectorCallbackMap objectCollectorCallbackMap;
	//
	int nDefaultImageIndex;
	CImageList normalImageList;
	CImageList smallImageList;

	CDataExtractorTypeMap dataExtractorTypeMap;
	CDataExtractorMap dataExtractorMap;

	void CreateImageLists();
	const std::string& LocateExtractorType( const std::string &rszObjectTypeName ) const;
	const SObjectParams* LocateObjectParams( const std::string &rszObjectTypeName, const std::string &rszObjectName ) const;
	const SObjectParams* GetObjectParams( const std::string &rszObjectTypeName, const std::string &rszObjectName, const std::string &rszDataExtractorType );
	void FillObjectParams( SObjectParams *pObjectParams, const std::string &rszObjectTypeName, const std::string &rszObjectName, const std::string &rszDataExtractorType );
	bool InsertObjectToCollection( CObjectCollection *pObjectCollection, const std::string &rszObjectTypeName, const std::string &rszObjectName, const SObjectParams* pObjectParams ) const;
	//
	bool InsertObjectToCollection( const std::string &rszObjectTypeName, const std::string &rszObjectName, const std::string &rszDataExtractorType );
	bool RemoveObjectFromCollection( const std::string &rszObjectTypeName, const std::string &rszObjectName );
	//
	void InsertObject( const std::string &rszObjectTypeName, const std::string &rszObjectName, const std::string &rszDataExtractorType );
	void RemoveObject( const std::string &rszObjectTypeName, const std::string &rszObjectName );

protected:
	// IObjectCollector
	bool Load( CDataStream *pStream );
	bool Save( CDataStream *pStream );

	void RegisterDataExtractor( IObjectDataExtractor *pDataExtractor );
	void RegisterDataExtractor( const std::string &rszDataExtractorType, IObjectDataExtractor *pDataExtractor );
	//
	void InsertCallback( IObjectCollectorCallback *pObjectCollectorCallback );
	void RemoveCallback( IObjectCollectorCallback *pObjectCollectorCallback );
	void ClearCallbackList();
	//
	// возвращает общее количество объектов
	int ApplyFilter( CObjectCollection *pObjectCollection, const std::string &rszObjectTypeName );
	int ApplyFilter( CObjectCollection *pObjectCollection, const IObjectFilter *pObjectFilter );
	bool GetObjectParams( SObjectParams* pObjectParams, const std::string &rszObjectTypeName, const std::string &rszObjectName );
	//
	CImageList* GetImageList( int nImageListType );
	//
	void ClearCollection();

public:
	CObjectCollector() : nDefaultImageIndex( INVALID_NODE_ID )
	{
		CreateImageLists();
	}
};



