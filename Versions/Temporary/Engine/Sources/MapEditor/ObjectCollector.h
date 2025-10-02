#if !defined(__OBJECT_COLLECTOR__)
#define __OBJECT_COLLECTOR__
#pragma once

#include "..\misc\HashFuncs.h"
#include "..\misc\StrProc.h"
#include "../libdb/Manipulator.h"
#include "..\MapEditorLib\Interface_ObjectCollector.h"


class CObjectFilterCollector : public IObjectFilterCollector
{
	OBJECT_NOCOPY_METHODS( CObjectFilterCollector );
	//
	struct SObjectFilter : public IObjectFilter
	{
		typedef vector<string> CNameList;
		struct SPart
		{
			string szOperation;
			string szObjectType;
			CNameList nameList;
			//
			int operator&( IXmlSaver &saver );
		};
		typedef vector<SPart> CPartList;

		//
		string szName;
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

		bool InsertObjectToCollection( CObjectCollection *pObjectCollection, const string &rszObjectTypeName, const string &rszObjectName ) const;
		int GetObjectCollection( CObjectCollection *pObjectCollection, const string &rszObjectTypeName ) const;
		void ExtractObjectsForFilterPart( CObjectNameCollection *pObjectNameCollection, const SPart &rPart ) const;
		void MergeSets( CObjectNameCollection *pDestination, const CObjectNameCollection &rSource, const string &szOperationType ) const;

		//IObjectFilter
		int GetObjectCollection( CObjectCollection *pObjectCollection ) const;
		bool Match( const string &szObjectTypeName, const string &szObjectName ) const;
	};
	typedef vector<SObjectFilter> CObjectFilterList;
	typedef hash_map<string, CObjectFilterList> CObjectFilterListMap;
	//
	CObjectFilterListMap objectFilterListMap;

	const SObjectFilter* LocateObjectFilter( const string &rszFilterType, const int nFilterIndex ) const;

protected:
	// IObjectFilterCollector
	bool Load( CDataStream *pStream );
	bool Save( CDataStream *pStream );
	//
	int GetFilterList( CFilterList* pFilterList, const string &rszFilterType ) const;
	//
	bool IsSeparator( const string &rszFilterType, const int nFilterIndex ) const;
	const IObjectFilter* Get( const string &rszFilterType, const int nFilterIndex ) const;

	int ShowFilterSelectionDialog( CWnd* pParentWindow, string *pszFilterType, int *pnFilterIndex );
	int ShowFilterCreationDialog( CWnd* pParentWindow, string *pszFilterType, int *pnFilterIndex );
};


class CObjectCollector : public IObjectCollector
{
	OBJECT_NOCOPY_METHODS( CObjectCollector );

	static const string DEFAULT_DATA_EXTRACTOR_TYPE;

	typedef vector<string> CObjectTypeNameList;
	typedef hash_map<string, CObjectTypeNameList> CDataExtractorTypeMap;
	//
	typedef hash_map<IObjectCollectorCallback*, int, SDefaultPtrHash> CObjectCollectorCallbackMap;
	typedef hash_map<string, CObj<IObjectDataExtractor> > CDataExtractorMap;
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
	const string& LocateExtractorType( const string &rszObjectTypeName ) const;
	const SObjectParams* LocateObjectParams( const string &rszObjectTypeName, const string &rszObjectName ) const;
	const SObjectParams* GetObjectParams( const string &rszObjectTypeName, const string &rszObjectName, const string &rszDataExtractorType );
	void FillObjectParams( SObjectParams *pObjectParams, const string &rszObjectTypeName, const string &rszObjectName, const string &rszDataExtractorType );
	bool InsertObjectToCollection( CObjectCollection *pObjectCollection, const string &rszObjectTypeName, const string &rszObjectName, const SObjectParams* pObjectParams ) const;
	//
	bool InsertObjectToCollection( const string &rszObjectTypeName, const string &rszObjectName, const string &rszDataExtractorType );
	bool RemoveObjectFromCollection( const string &rszObjectTypeName, const string &rszObjectName );
	//
	void InsertObject( const string &rszObjectTypeName, const string &rszObjectName, const string &rszDataExtractorType );
	void RemoveObject( const string &rszObjectTypeName, const string &rszObjectName );

protected:
	// IObjectCollector
	bool Load( CDataStream *pStream );
	bool Save( CDataStream *pStream );

	void RegisterDataExtractor( IObjectDataExtractor *pDataExtractor );
	void RegisterDataExtractor( const string &rszDataExtractorType, IObjectDataExtractor *pDataExtractor );
	//
	void InsertCallback( IObjectCollectorCallback *pObjectCollectorCallback );
	void RemoveCallback( IObjectCollectorCallback *pObjectCollectorCallback );
	void ClearCallbackList();
	//
	// возвращает общее количество объектов
	int ApplyFilter( CObjectCollection *pObjectCollection, const string &rszObjectTypeName );
	int ApplyFilter( CObjectCollection *pObjectCollection, const IObjectFilter *pObjectFilter );
	bool GetObjectParams( SObjectParams* pObjectParams, const string &rszObjectTypeName, const string &rszObjectName );
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

#endif // !defined(__OBJECT_COLLECTOR__)

