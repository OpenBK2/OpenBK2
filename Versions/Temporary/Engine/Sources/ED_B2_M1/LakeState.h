#if !defined(__LAKE_STATE__)
#define __LAKE_STATE__
#pragma once

#include "VSOStateEx.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CLakeState : public CVSOStateEx
{
	static const string VSO_NAME;
	static const string VSO_TYPE_NAME;

	friend class CMultiInputState;
	friend class CMapInfoState;
	friend class CVSOMultiState;
	
	//конструкторы и операторы присваивания
	CLakeState( class CVSOMultiState *_pParentState = 0 ) : CVSOStateEx( _pParentState )
	{
		NI_ASSERT( _pParentState != 0, "CLakeState(): Invalid parameter: _pParentState == 0" );
	}

protected:
	// CVSOState
	bool IsDrawSceneDrawTool() { return true; }
	bool EdgesMustBeZero() { return false; }
	bool IsEdgesMustBeOut() { return false; }
	bool IsClose() { return true; }
	bool IsComplete() { return false; }
	bool NoIntersection() { return true; }
	bool NoSelfIntersection() { return true; }
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
	float GetDefaultStep() { return CVSOManager::DEFAULT_STEP * 8.0f; }
	float GetDefaultWidth() { return CVSOManager::DEFAULT_WIDTH; }
	float GetDefaultHeight() { return CVSOManager::DEFAULT_HEIGHT; }
	float GetDefaultOpacity() { return CVSOManager::DEFAULT_OPACITY; }
	float GetDefaultStepOut() { return CVSOManager::DEFAULT_STEP / 4.0f; }
	//
	bool CanEditPoints( CVSOManager::SVSOSelection::ESelectionType eSelectionType );
	void PrepareInsertVSO() {}
	//
	virtual void PickVSO( const CVec3 &rvPos, CVSOIDList *pPickVSOIDList );

	//CVSOStateEx
	//bool CanInsertVSO();
	bool IsRealtimeUpdate() const { return false; }
	const string GetVSOName() const { return VSO_NAME; }
	const string GetVSOTypeName() const { return VSO_TYPE_NAME; }
	int GetVSOTypeID() const { return NDb::SLakeDesc::typeID; }
	const CMapInfoController::SVSOUndoData::EVSOType GetVSOType() const { return CMapInfoController::SVSOUndoData::VSO_LAKE; }
	NMapInfoEditor::CVSOInstanceList* GetVSOList();
	//
	void InsertVSOInTerrain( const NDb::SVSOInstance &rVSO );
	void RemoveVSOFromTerrain( int nVSOID );
	void UpdateVSOInTerrain( int nVSOID );
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // !defined(__LAKE_STATE__)

