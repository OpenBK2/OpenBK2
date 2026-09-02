#pragma once

#include "Misc/2Darray.h"
#include "CommandHandlerDefines.h"
#include "MapEditorLib/DefaultInputState.h"
#include "MapEditorLib/MaskManipulator.h"
#include "Tools_SceneDraw.h"
#include "DialogData.h"
#include "MarkerSet.h"
#include "BuildingEditor.h"

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
	CBuildingEditor* pBuildingEditor;
	CSceneDrawTool drawTool;

	// IInputState interface
	void Enter();
	void Leave();
	void PostDraw( CPaintDC *pPaintDC );
	void OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags );

	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uintptr_t dwData );
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );

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
	CPointsListState( unsigned nCHID, unsigned nInstanceID, CBuildingEditor* _pBuildingEditor );
	virtual ~CPointsListState();

	virtual void Draw( CPaintDC *pPaintDC ) {}
	virtual std::string GetPointsArrayFieldName() = 0;
	virtual std::string GetPositionFieldName() { return "Pos"; }
	virtual std::string GetDirectionFieldName() { return "Direction"; }
	virtual void GetMaskFields(std::vector<std::string> *pMaskFields ) = 0;
	virtual bool IsOriginInUse() { return false; }
	virtual EDirMeasure GetDirMeasure() { return DIR_IN_AIGRAD; }
	virtual void AddPointSpecificMarker( IManipulator *pManipulator, int nPointIndex ) {}
};

class CSmokePointsState  : public CPointsListState
{
public:
	CSmokePointsState()
	{
		NI_ASSERT( 0, "CSmokePointsState()" );
	}
	CSmokePointsState( CBuildingEditor* _pBuildingEditor )
		:	CPointsListState( CHID_BUILDING_SMOKE_POINTS_STATE, 0, _pBuildingEditor ) {}
	virtual ~CSmokePointsState() {}

	virtual std::string GetPointsArrayFieldName() { return "smokePoints"; }
	virtual void GetMaskFields(std::vector<std::string> *pMaskFields );
	virtual bool IsOriginInUse() { return true; }
};

class CSlotPointsState : public CPointsListState
{
public:
	CSlotPointsState ()
	{
		NI_ASSERT( 0, "CSlotPointsState()" );
	}
	CSlotPointsState( CBuildingEditor* _pBuildingEditor )
		:	CPointsListState( CHID_BUILDING_SLOT_POINTS_STATE, 1, _pBuildingEditor ) {}
	virtual ~CSlotPointsState() {}

	virtual std::string GetPointsArrayFieldName() { return "slots"; }
	virtual void GetMaskFields(std::vector<std::string> *pMaskFields );
	virtual EDirMeasure GetDirMeasure() { return DIR_IN_DEGREES; }
};

class CEntrancePointsState : public CPointsListState
{
public:
	CEntrancePointsState()
	{
		NI_ASSERT( 0, "CEntrancePointsState()" );
	}
	CEntrancePointsState( CBuildingEditor* _pBuildingEditor ) 
		:	CPointsListState( CHID_BUILDING_ENTRANCE_POINTS_STATE, 2, _pBuildingEditor ) {}
	virtual ~CEntrancePointsState() {}

	virtual std::string GetPointsArrayFieldName() { return "entrances"; }
	virtual void GetMaskFields( std::vector<std::string> *pMaskFields );
	virtual std::string GetDirectionFieldName() { return "Dir"; }
};

class CSurfacePointsState : public CPointsListState
{
	std::vector<NDb::SHPObjectRPGStats::SModelSurfacePoint> surfPoints;

public:
	CSurfacePointsState()
	{
		NI_ASSERT( 0, "CSurfacePointsState()" );
	}
	CSurfacePointsState::CSurfacePointsState( CBuildingEditor* _pBuildingEditor )
		:	CPointsListState( CHID_BUILDING_SURFACE_POINTS_STATE, 3, _pBuildingEditor )	{}
	//
	virtual ~CSurfacePointsState() {}

	virtual std::string GetPointsArrayFieldName() { return "SurfacePoints"; }
	virtual void GetMaskFields( std::vector<std::string> *pMaskFields );
	virtual std::string GetDirectionFieldName() { return ""; }
	virtual void Draw( CPaintDC *pPaintDC );
	virtual void AddPointSpecificMarker( IManipulator *pManipulator, int nPointIndex );
};

class CDamageLevelsState : public CPointsListState
{
public:
	CDamageLevelsState()
	{
		NI_ASSERT( 0, "CDamageLevelsState()" );
	}
	CDamageLevelsState::CDamageLevelsState( CBuildingEditor* _pBuildingEditor )
		:	CPointsListState( CHID_DAMAGE_LEVELS_STATE, 4, _pBuildingEditor )	{}
	//
	virtual ~CDamageLevelsState() {}

	std::string GetPointsArrayFieldName() { return "DamageLevels"; }
	void GetMaskFields( std::vector<std::string> *pMaskFields );
	std::string GetDirectionFieldName() { return ""; }
	void Draw( CPaintDC *pPaintDC ) {}
	void RefreshState();
	void SetPointMarkers() {}
	void Leave();
};


