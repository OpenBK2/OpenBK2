#pragma once

#include "Misc/2DArray.h"
#include "MapEditorLib/ObjectBaseController.h"
#include "B2_M1_Terrain/DBVSO.h"

class CMapInfoController : public CObjectBaseController
{
	OBJECT_NOCOPY_METHODS( CMapInfoController );

public:
	static const char TEMPORARY_LABEL[];
	//
	struct STileUndoData
	{
		CArray2D<BYTE> brush;
		CVec3 vBrushPos;
	};
	//
	struct SHeightUndoData
	{
		CArray2D<float> brush;
		CTPoint<int> brushPos;
	};
	//
	// TYPE_INSERT:
	// (int)newValue	- ID элемента куда вставлять
	//
	// TYPE_REMOVE:
	// (int)newValue	- ID элемента откуда удалять
	//
	// TYPE_CHANGE
	// oldValue				- старое (предыдущее) значение
	// newValue				- новое (установленное) значение
	struct SVSOUndoData
	{
		enum EVSOType
		{
			VSO_ROAD			= 0,
			VSO_RIVER			= 1,
			VSO_CRAG			= 2,
			VSO_LAKE			= 3,
			VSO_COAST			= 4,
		};
		enum EType
		{
			TYPE_INSERT		= 0,
			TYPE_REMOVE		= 1,
			TYPE_CHANGE		= 2,
		};
		//
		EVSOType eVSOType;
		EType eType;
		NDb::SVSOInstance newValue;
		NDb::SVSOInstance oldValue;
	};
	//
	typedef list<STileUndoData> CTileUndoDataList;
	typedef list<SHeightUndoData> CHeightUndoDataList;
	typedef list<SVSOUndoData> CVSOUndoDataList;

	// список данных подвергнутых изменениям
	CTileUndoDataList tileUndoDataList;
	CHeightUndoDataList heightUndoDataList;
	CVSOUndoDataList vsoUndoDataList;
	// IManipulator
	virtual bool IsEmpty() const
	{ return ( CObjectBaseController::IsEmpty() &&
						 tileUndoDataList.empty() &&
						 heightUndoDataList.empty() &&
						 vsoUndoDataList.empty() ); }

	// Helpers
	inline bool AddChangeTileOperation( const CArray2D<BYTE> &rBrush, const CVec3 &rvBrushPos )
	{
		CTileUndoDataList::iterator posTileUndoData = tileUndoDataList.insert( tileUndoDataList.begin(), STileUndoData() );
		posTileUndoData->brush = rBrush;
		posTileUndoData->vBrushPos = rvBrushPos;
		return true;
	}
	inline bool AddChangeHeightOperation( const CArray2D<float> &rBrush, const CTPoint<int> &rBrushPos )
	{
		CHeightUndoDataList::iterator posHeightUndoData = heightUndoDataList.insert( heightUndoDataList.begin(), SHeightUndoData() );
		posHeightUndoData->brush = rBrush;
		posHeightUndoData->brushPos = rBrushPos;
		return true;
	}
	inline bool AddInsertVSOOperation( SVSOUndoData::EVSOType _eVSOType, const NDb::SVSOInstance &rVSOInstance )
	{
		CVSOUndoDataList::iterator posVSOUndoData = vsoUndoDataList.insert( vsoUndoDataList.begin(), SVSOUndoData() );
		posVSOUndoData->eVSOType = _eVSOType;
		posVSOUndoData->eType = SVSOUndoData::TYPE_INSERT;
		posVSOUndoData->newValue = rVSOInstance;
		return true;
	}
	inline bool AddRemoveVSOOperation( SVSOUndoData::EVSOType _eVSOType, const NDb::SVSOInstance &rVSOInstance )
	{
		CVSOUndoDataList::iterator posVSOUndoData = vsoUndoDataList.insert( vsoUndoDataList.begin(), SVSOUndoData() );
		posVSOUndoData->eVSOType = _eVSOType;
		posVSOUndoData->eType = SVSOUndoData::TYPE_REMOVE;
		posVSOUndoData->newValue = rVSOInstance;
		return true;
	}
	inline bool AddChangeVSOOperation( SVSOUndoData::EVSOType _eVSOType, const NDb::SVSOInstance &rOldVSOInstance, const NDb::SVSOInstance &rNewVSOInstance )
	{
		CVSOUndoDataList::iterator posVSOUndoData = vsoUndoDataList.insert( vsoUndoDataList.begin(), SVSOUndoData() );
		posVSOUndoData->eVSOType = _eVSOType;
		posVSOUndoData->eType = SVSOUndoData::TYPE_CHANGE;
		posVSOUndoData->newValue = rNewVSOInstance;
		posVSOUndoData->oldValue = rOldVSOInstance;
		return true;
	}
};



