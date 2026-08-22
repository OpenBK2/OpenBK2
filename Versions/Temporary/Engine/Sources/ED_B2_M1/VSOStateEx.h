#pragma once

#include "VSOState.h"
#include "MapInfoController.h"
#include "MapInfoEditorData.h"

class CVSOStateEx : public CVSOState
{
	const NDb::SCragDesc* pSelectedCragDesc;
	NDb::SVSOInstance oldVSOInstance;
	void ClearOldVSOInstance()
	{
		oldVSOInstance.nVSOID = INVALID_NODE_ID;
		oldVSOInstance.pDescriptor = 0;
		oldVSOInstance.controlPoints.clear();	
		oldVSOInstance.points.clear();
	}
	bool bVSOChanged;

protected:
	// CVSOState
	virtual bool CanEdit();
	//
	virtual void PickVSO( const CVec3 &rvPos, CVSOIDList *pPickVSOIDList );
	virtual bool CanInsertVSO();
	virtual NDb::SVSOInstance* GetVSO( int nVSOID, int *pnVSOIndex );
	virtual int GetFreeVSOID();
	virtual void UpdateVSO( int nVSOID, EUpdateType eEpdateType, CVSOManager::SVSOSelection::ESelectionType eSelectionType, unsigned nFlags );
	virtual int InsertVSO( const vector<CVec3> &rControlPointList );
	virtual void RemoveVSO( int nVSOID );

	virtual bool InsertVSOToBase( class CObjectBaseController *pObjectController, int nVSOIndex, const NDb::SVSOInstance &rVSO );
	virtual bool RemoveVSOFromBase( class CObjectBaseController *pObjectController, int nVSOIndex );
	virtual bool UpdateVSOInBase( class CObjectBaseController *pObjectController, int nVSOIndex, const NDb::SVSOInstance &rVSO );

public:
	//конструкторы и операторы присваивания
	CVSOStateEx( class CVSOMultiState *_pParentState = 0 ) : CVSOState( _pParentState ), bVSOChanged( false )
	{
		NI_ASSERT( _pParentState != 0, "CVSOStateEx(): Invalid parameter: _pParentState == 0" );
		ClearOldVSOInstance();
	}
	//
	//CVSOStateEx
	// real time updare in terrain while editing
	virtual bool IsRealtimeUpdate() const = 0;
	// array name in MapInfo structure (like "roads" )
	virtual const string GetVSOName() const = 0;
	// VSO DB type name ( like "RoadDesc" )
	virtual const string GetVSOTypeName() const = 0;
	// VSO DB type ID ( like NDb::SRoadDesc::nID )
	virtual int GetVSOTypeID() const = 0;
	// VSO type in CMapInfoController::SVSOUndoData structure (like CMapInfoController::SVSOUndoData::VSO_ROAD )
	virtual const CMapInfoController::SVSOUndoData::EVSOType GetVSOType() const = 0;
	// VSO list in pMapInfo structure ( like pMapInfo->roads )
	virtual NMapInfoEditor::CVSOInstanceList* GetVSOList() = 0;
	//
	// Terrain only
	virtual void InsertVSOInTerrain( const NDb::SVSOInstance &rVSO ) = 0;
	virtual void RemoveVSOFromTerrain( int nVSOID ) = 0;
	virtual void UpdateVSOInTerrain( int nVSOID ) = 0;
};


