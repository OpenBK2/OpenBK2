#pragma once

#include "Interface_Controller.h"
#include "Interface_Widget.h"
#include "System/FilePath.h"

#define ALL_FILTER_ID 0
//
#define NORMAL_IMAGE_SIZE_X 64
#define NORMAL_IMAGE_SIZE_Y 64
#define SMALL_IMAGE_SIZE_X 16
#define SMALL_IMAGE_SIZE_Y 16  
//
#define NORMAL_IMAGE_SPACE_X 10
#define NORMAL_IMAGE_SPACE_Y 35
//

#define OCDE_NORMAL_BITMAP	0x01
#define OCDE_SMALL_BITMAP		0x02
#define OCDE_LABEL					0x04
#define OCDE_ALL						0xFFFFFFFF


struct IObjectCollectorCallback
{
	virtual void OnInsertObject( const std::string &szObjectTypeName, const std::string &szObjectName ) = 0;
	virtual void OnRemoveObject( const std::string &szObjectTypeName, const std::string &szObjectName ) = 0;
	//
	virtual void OnClearCollection() = 0;
};


struct IObjectDataExtractor : public CObjectBase
{
	// возвращает данные объекта, в качестве возвращаемого значение - битовая маска, что заполнено
	//
	// The icons come back as pixels rather than as bitmaps: everything behind
	// this call already works in CArray2D<uint32_t> and the conversion to a
	// front-end bitmap is the collector's job.
	virtual unsigned GetObjectData( CArray2D<uint32_t> *pNormalImage,
															CArray2D<uint32_t> *pSmallImage,
															std::string *pszLabel,
															const std::string &rszObjectTypeName,
															const std::string &rszObjectName,
															const std::string &rszDataExtractorType ) = 0;
};


struct IObjectFilter
{
	typedef std::unordered_map<NFile::CFilePath, int> CObjectNameCollection;
	typedef std::unordered_map<NFile::CFilePath, CObjectNameCollection> CObjectCollection;
	//
	virtual int GetObjectCollection( CObjectCollection *pObjectCollection ) const = 0;
	virtual bool Match( const std::string &szObjectTypeName, const std::string &szObjectName ) const = 0;
};


struct IObjectFilterCollector : public CObjectBase
{
	enum { tidTypeID = 0x14216B00 };
	//
	typedef std::vector<std::string> CFilterList;
	typedef std::unordered_map<std::string, CFilterList> CFilterListMap;
	//
	virtual bool Load( CDataStream *pStream ) = 0;
	virtual bool Save( CDataStream *pStream ) = 0;
	//
	virtual int GetFilterList( CFilterList* pFilterList, const std::string &rszFilterType ) const = 0;
	//
	virtual bool IsSeparator( const std::string &rszFilterType, const int nFilterIndex ) const = 0;
	virtual const IObjectFilter* Get( const std::string &rszFilterType, const int nFilterIndex ) const = 0;
	// IDOK or IDCANCEL
	virtual int ShowFilterSelectionDialog( IWidget* pParentWidget, std::string *pszFilterType, int *pnFilterIndex ) = 0;
	virtual int ShowFilterCreationDialog( IWidget* pParentWidget, std::string *pszFilterType, int *pnFilterIndex ) = 0;
};


// Коллекционирование объектов по меткам
// Создание ImageList на коллекцию
// Применение фильтров
struct IObjectCollector : public CObjectBase
{
	enum { tidTypeID = 0x14126380 };
	//
	struct SObjectParams
	{
		int nIconIndex;
		std::string szLabel;
	};
	typedef std::unordered_map<std::string, SObjectParams> CObjectNameCollection;
	typedef std::unordered_map<std::string, CObjectNameCollection> CObjectCollection;
	//
	virtual bool Load( CDataStream *pStream ) = 0;
	virtual bool Save( CDataStream *pStream ) = 0;
	//
	virtual void RegisterDataExtractor( IObjectDataExtractor *pDataExtractor ) = 0;
	virtual void RegisterDataExtractor( const std::string &rszDataExtractorType, IObjectDataExtractor *pDataExtractor ) = 0;
	//
	virtual void InsertCallback( IObjectCollectorCallback *pObjectCollectorCallback ) = 0;
	virtual void RemoveCallback( IObjectCollectorCallback *pObjectCollectorCallback ) = 0;
	virtual void ClearCallbackList() = 0;
	//
	// возвращает количество объектов
	virtual int ApplyFilter( CObjectCollection *pObjectCollection, const std::string &rszObjectTypeName ) = 0;
	virtual int ApplyFilter( CObjectCollection *pObjectCollection, const IObjectFilter *pObjectFilter ) = 0;
	virtual bool GetObjectParams( SObjectParams* pObjectParams, const std::string &rszObjectTypeName, const std::string &rszObjectName ) = 0;
	//
	virtual IImageList* GetImageList( int nImageListType ) = 0;
	//
	virtual void ClearCollection() = 0;
};



