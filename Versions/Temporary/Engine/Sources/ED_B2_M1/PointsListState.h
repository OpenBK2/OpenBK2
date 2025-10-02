#if !defined(__POINTS_LIST_STATE__)
#define __POINTS_LIST_STATE__
#pragma once

#include "../misc/2darray.h"
#include "../zlib/zconf.h"
#include "commandhandlerdefines.h"
#include "../MapEditorLib/DefaultInputState.h"
#include "../MapEditorLib/MaskManipulator.h"
#include "Tools_SceneDraw.h"
#include "DialogData.h"
#include "MarkerSet.h"
#include "BuildingEditor.h"


class CPointsListState : public CDefaultInputState, public ICommandHandler
{
	CPtr<CMaskManipulator> pMaskManipulator;
	UINT nCHID;
	UINT nInstanceID;
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
	void OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );

	// ICommandHandler
	bool HandleCommand( UINT nCommandID, DWORD dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );

	// CPointsListState 
	bool GetPointListDialogData( SPointListDialogData *pData );
	bool SetPointListDialogData( SPointListDialogData *pData );

	int GetSelectedPointIndex();
	virtual void RefreshState();
	int GetPointsNum( const string &rszField );
	void SetPointMarkers();

public:
	CPointsListState()
	{
		NI_ASSERT( 0, "CPointsListState()" );
	}
	CPointsListState( UINT nCHID, UINT nInstanceID, CBuildingEditor* _pBuildingEditor );
	virtual ~CPointsListState();

	virtual void Draw( CPaintDC *pPaintDC ) {}
	virtual string GetPointsArrayFieldName() = 0;
	virtual string GetPositionFieldName() { return "Pos"; }
	virtual string GetDirectionFieldName() { return "Direction"; }
	virtual void GetMaskFields(vector<string> *pMaskFields ) = 0;
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

	virtual string GetPointsArrayFieldName() { return "smokePoints"; }
	virtual void GetMaskFields(vector<string> *pMaskFields );
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

	virtual string GetPointsArrayFieldName() { return "slots"; }
	virtual void GetMaskFields(vector<string> *pMaskFields );
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

	virtual string GetPointsArrayFieldName() { return "entrances"; }
	virtual void GetMaskFields( vector<string> *pMaskFields );
	virtual string GetDirectionFieldName() { return "Dir"; }
};

class CSurfacePointsState : public CPointsListState
{
	vector<NDb::SHPObjectRPGStats::SModelSurfacePoint> surfPoints;

public:
	CSurfacePointsState()
	{
		NI_ASSERT( 0, "CSurfacePointsState()" );
	}
	CSurfacePointsState::CSurfacePointsState( CBuildingEditor* _pBuildingEditor )
		:	CPointsListState( CHID_BUILDING_SURFACE_POINTS_STATE, 3, _pBuildingEditor )	{}
	//
	virtual ~CSurfacePointsState() {}

	virtual string GetPointsArrayFieldName() { return "SurfacePoints"; }
	virtual void GetMaskFields( vector<string> *pMaskFields );
	virtual string GetDirectionFieldName() { return ""; }
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

	string GetPointsArrayFieldName() { return "DamageLevels"; }
	void GetMaskFields( vector<string> *pMaskFields );
	string GetDirectionFieldName() { return ""; }
	void Draw( CPaintDC *pPaintDC ) {}
	void RefreshState();
	void SetPointMarkers() {}
	void Leave();
};

#endif//#if !defined(__POINTS_LIST_STATE__)

