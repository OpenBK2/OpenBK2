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
	//
	// возвращает количество объектов
	virtual int ApplyFilter( CObjectCollection *pObjectCollection, const std::string &rszObjectTypeName ) = 0;
	virtual int ApplyFilter( CObjectCollection *pObjectCollection, const IObjectFilter *pObjectFilter ) = 0;
	virtual bool GetObjectParams( SObjectParams* pObjectParams, const std::string &rszObjectTypeName, const std::string &rszObjectName ) = 0;
	// Which of an object's two icon sizes is wanted. The numbers are
	// wxIMAGE_LIST_NORMAL and wxIMAGE_LIST_SMALL, which are also the
	// LVSIL_NORMAL and LVSIL_SMALL the list controls were asked with before the
	// lists were wx's; named here so the interface layer keeps naming no
	// toolkit.
	enum EImageListSize
	{
		IMAGE_LIST_NORMAL = 0,
		IMAGE_LIST_SMALL = 1,
	};
	virtual IImageList* GetImageList( int nImageListType ) = 0;
};



