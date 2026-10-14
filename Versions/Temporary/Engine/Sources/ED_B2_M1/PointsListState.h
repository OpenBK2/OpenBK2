#pragma once

#include "BuildingEditor.h"
#include "CommandHandlerDefines.h"
#include "DialogData.h"
#include "MapEditorLib/DefaultInputState.h"
#include "MapEditorLib/MaskManipulator.h"
#include "MarkerSet.h"
#include "Misc/2Darray.h"
#include "Tools_SceneDraw.h"

#include <cstdint>

#include <zconf.h>

class CPointsListState : public CDefaultInputState, public ICommandHandler
{
	CPtr<CMaskManipulator> pMaskManipulator;
	unsigned nCHID;
	unsigned nInstanceID;
	SPointListDialogData currDialogData;
	CObj<SMarkerSet> pMarkers;
	//
	void SetMaskManipulator();
	void ClearMaskManipulator();

protected:
	CPtr<IClientUpdatableProcess> pProcess;
	CBuildingEditor *pBuildingEditor;
	CSceneDrawTool drawTool;

	// IInputState interface
	void Enter() override;
	void Leave() override;
	void PostDraw( IPaintContext *pPaintDC ) override;
	void OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) override;

	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uintptr_t dwData ) override;
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck ) override;

	// CPointsListState
	bool GetPointListDialogData( SPointListDialogData *pData );
	bool SetPointListDialogData( SPointListDialogData *pData );

	int GetSelectedPointIndex();
	virtual void RefreshState();
	int GetPointsNum( const std::string &rszField );
	void SetPointMarkers();

public:
	CPointsListState()
	{
		NI_ASSERT( 0, "CPointsListState()" );
	}

	CPointsListState( unsigned nCHID, unsigned nInstanceID, CBuildingEditor *_pBuildingEditor );
	~CPointsListState() override;

	void Draw( IPaintContext *pPaintDC ) override {}

	virtual std::string GetPointsArrayFieldName() = 0;

	virtual std::string GetPositionFieldName()
	{
		return "Pos";
	}

	virtual std::string GetDirectionFieldName()
	{
		return "Direction";
	}

	virtual void GetMaskFields( std::vector<std::string> *pMaskFields ) = 0;

	virtual bool IsOriginInUse()
	{
		return false;
	}

	virtual EDirMeasure GetDirMeasure()
	{
		return DIR_IN_AIGRAD;
	}

	virtual void AddPointSpecificMarker( IManipulator *pManipulator, int nPointIndex ) {}
};

class CSmokePointsState : public CPointsListState
{
public:
	CSmokePointsState()
	{
		NI_ASSERT( 0, "CSmokePointsState()" );
	}

	CSmokePointsState( CBuildingEditor *_pBuildingEditor )
		: CPointsListState( CHID_BUILDING_SMOKE_POINTS_STATE, 0, _pBuildingEditor )
	{
	}

	~CSmokePointsState() override {}

	std::string GetPointsArrayFieldName() override
	{
		return "smokePoints";
	}

	void GetMaskFields( std::vector<std::string> *pMaskFields ) override;

	bool IsOriginInUse() override
	{
		return true;
	}
};

class CSlotPointsState : public CPointsListState
{
public:
	CSlotPointsState()
	{
		NI_ASSERT( 0, "CSlotPointsState()" );
	}

	CSlotPointsState( CBuildingEditor *_pBuildingEditor )
		: CPointsListState( CHID_BUILDING_SLOT_POINTS_STATE, 1, _pBuildingEditor )
	{
	}

	~CSlotPointsState() override {}

	std::string GetPointsArrayFieldName() override
	{
		return "slots";
	}

	void GetMaskFields( std::vector<std::string> *pMaskFields ) override;

	EDirMeasure GetDirMeasure() override
	{
		return DIR_IN_DEGREES;
	}
};

class CEntrancePointsState : public CPointsListState
{
public:
	CEntrancePointsState()
	{
		NI_ASSERT( 0, "CEntrancePointsState()" );
	}

	CEntrancePointsState( CBuildingEditor *_pBuildingEditor )
		: CPointsListState( CHID_BUILDING_ENTRANCE_POINTS_STATE, 2, _pBuildingEditor )
	{
	}

	~CEntrancePointsState() override {}

	std::string GetPointsArrayFieldName() override
	{
		return "entrances";
	}

	void GetMaskFields( std::vector<std::string> *pMaskFields ) override;

	std::string GetDirectionFieldName() override
	{
		return "Dir";
	}
};

class CSurfacePointsState : public CPointsListState
{
	std::vector<NDb::SHPObjectRPGStats::SModelSurfacePoint> surfPoints;

public:
	CSurfacePointsState()
	{
		NI_ASSERT( 0, "CSurfacePointsState()" );
	}

	CSurfacePointsState( CBuildingEditor *_pBuildingEditor )
		: CPointsListState( CHID_BUILDING_SURFACE_POINTS_STATE, 3, _pBuildingEditor )
	{
	}

	//
	~CSurfacePointsState() override {}

	std::string GetPointsArrayFieldName() override
	{
		return "SurfacePoints";
	}

	void GetMaskFields( std::vector<std::string> *pMaskFields ) override;

	std::string GetDirectionFieldName() override
	{
		return "";
	}

	void Draw( IPaintContext *pPaintDC ) override;
	void AddPointSpecificMarker( IManipulator *pManipulator, int nPointIndex ) override;
};

class CDamageLevelsState : public CPointsListState
{
public:
	CDamageLevelsState()
	{
		NI_ASSERT( 0, "CDamageLevelsState()" );
	}

	CDamageLevelsState( CBuildingEditor *_pBuildingEditor )
		: CPointsListState( CHID_DAMAGE_LEVELS_STATE, 4, _pBuildingEditor )
	{
	}

	//
	~CDamageLevelsState() override {}

	std::string GetPointsArrayFieldName() override
	{
		return "DamageLevels";
	}

	void GetMaskFields( std::vector<std::string> *pMaskFields ) override;

	std::string GetDirectionFieldName() override
	{
		return "";
	}

	void Draw( IPaintContext *pPaintDC ) override {}

	void RefreshState() override;

	void SetPointMarkers() {}

	void Leave() override;
};
