#if !defined(__COAST_STATE__)
#define __COAST_STATE__
#pragma once

#include "VSOState.h"

class CCoastState : public CVSOState
{
	friend class CMultiInputState;
	friend class CMapInfoState;
	friend class CVSOMultiState;

	bool bVSOChanged;
	NDb::SVSOInstance oldVSOInstance;
	//конструкторы и операторы присваивания
	CCoastState( class CVSOMultiState *_pParentState = 0 ) : CVSOState( _pParentState )
	{
		NI_ASSERT( _pParentState != 0, "CCoastState(): Invalid parameter: _pParentState == 0" );
	}
	//
	bool InsertVSOToBase( class CObjectBaseController *pObjectController, const NDb::SVSOInstance &rVSO );
	bool RemoveVSOFromBase( class CObjectBaseController *pObjectController );
	bool UpdateVSOInBase( class CObjectBaseController *pObjectController, const NDb::SVSOInstance &rVSO );

protected:
	// CVSOState
	bool CanEdit();
	bool IsDrawSceneDrawTool() { return true; }
	bool CanEditPoints( CVSOManager::SVSOSelection::ESelectionType eSelectionType );
	bool EdgesMustBeZero() { return false; }
	bool IsEdgesMustBeOut() { return true; }
	bool IsClose() { return false; }
	bool IsComplete() { return true; }
	bool NoIntersection() { return true; }
	bool NoSelfIntersection() { return true; }
	void GetControlPointBounds( int *pnMinCount, int *pnMaxCount )
	{
		if ( pnMinCount )
		{
			( *pnMinCount ) = 2;
		}
		if ( pnMaxCount )
		{
			( *pnMaxCount ) = INVALID_NODE_ID;
		}
	}
	float GetDefaultStep() { return CVSOManager::DEFAULT_STEP * 8.0f; }
	float GetDefaultWidth() { return CVSOManager::DEFAULT_WIDTH; }
	float GetDefaultHeight() { return CVSOManager::DEFAULT_HEIGHT; }
	float GetDefaultOpacity() { return CVSOManager::DEFAULT_OPACITY; }
	float GetDefaultStepOut() { return CVSOManager::DEFAULT_STEP / 4.0f; }
	//
	void PickVSO( const CVec3 &rvPos, CVSOIDList *pPickVSOIDList );
	bool CanInsertVSO();
	bool CanUpdateVSO( int nVSOID ) { return false; }
	bool CanRemoveVSO( int nVSOID ) { return false; }
	NDb::SVSOInstance* GetVSO( int nVSOID, int *pnVSOIndex );
	void UpdateVSO( int nVSOID, EUpdateType eEpdateType, CVSOManager::SVSOSelection::ESelectionType eSelectionType, unsigned nFlags );
	void PrepareInsertVSO() {}
	int InsertVSO( const vector<CVec3> &rControlPointList );
	void RemoveVSO( int nVSOID );
	//
	const CMapInfoController::SVSOUndoData::EVSOType GetVSOType() const { return CMapInfoController::SVSOUndoData::VSO_COAST; }

public:
	static void GetWaterPos( const NDb::SMapInfo *pMapInfo, const vector<NDb::SVSOPoint> &rPoints, CVec3 *pWaterPos );
};

#endif // !defined(__COAST_VSO_STATE__)

