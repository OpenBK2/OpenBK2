#pragma once

#include "VSOStateEx.h"

class CRoadState : public CVSOStateEx
{
	static const std::string VSO_NAME;
	static const std::string VSO_TYPE_NAME;
	
	friend class CMultiInputState;
	friend class CMapInfoState;
	friend class CVSOMultiState;
	
	//конструкторы и операторы присваивания
	CRoadState( class CVSOMultiState *_pParentState = 0 ) : CVSOStateEx( _pParentState )
	{
		NI_ASSERT( _pParentState != 0, "CRoadState(): Invalid parameter: _pParentState == 0" );
	}
	//
protected:
	// CVSOState
	bool IsDrawSceneDrawTool() { return true; }
	bool EdgesMustBeZero() { return false; }
	bool IsEdgesMustBeOut() { return false; }
	bool IsClose() { return false; }
	bool IsComplete() { return true; }
	bool NoIntersection() { return false; }
	bool NoSelfIntersection() { return false; }
	bool CanUpdateVSO( int nVSOID ) { return true; }
	bool CanRemoveVSO( int nVSOID ) { return true; }
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
	float GetDefaultStep() { return CVSOManager::DEFAULT_STEP * 2.0f; }
	float GetDefaultWidth() { return CVSOManager::DEFAULT_WIDTH; }
	float GetDefaultHeight() { return CVSOManager::DEFAULT_HEIGHT; }
	float GetDefaultOpacity() { return CVSOManager::DEFAULT_OPACITY; }
	float GetDefaultStepOut() { return CVSOManager::DEFAULT_STEP / 4.0f; }

	void UpdateVisualVSO( NDb::SVSOInstance *pVSO, bool bBothEdges );
	bool CanEditPoints( CVSOManager::SVSOSelection::ESelectionType eSelectionType );
	void PrepareInsertVSO() {}
	//
	//CVSOStateEx
	bool CanInsertVSO();
	bool IsRealtimeUpdate() const { return true; }
	const std::string GetVSOName() const { return VSO_NAME; }
	const std::string GetVSOTypeName() const { return VSO_TYPE_NAME; }
	int GetVSOTypeID() const { return NDb::SRoadDesc::typeID; }
	const CMapInfoController::SVSOUndoData::EVSOType GetVSOType() const { return CMapInfoController::SVSOUndoData::VSO_ROAD; }
	NMapInfoEditor::CVSOInstanceList* GetVSOList();
	//
	void InsertVSOInTerrain( const NDb::SVSOInstance &rVSO );
	void RemoveVSOFromTerrain( int nVSOID );
	void UpdateVSOInTerrain( int nVSOID );
};


