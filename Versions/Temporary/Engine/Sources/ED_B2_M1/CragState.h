#pragma once

#include "VSOStateEx.h"

class CCragState : public CVSOStateEx
{
	static const std::string VSO_NAME;
	static const std::string VSO_TYPE_NAME;

	friend class CMultiInputState;
	friend class CMapInfoState;
	friend class CVSOMultiState;
	
	const NDb::SCragDesc* pSelectedCragDesc;
	//конструкторы и операторы присваивания
	CCragState( class CVSOMultiState *_pParentState = 0 ) : CVSOStateEx( _pParentState ), pSelectedCragDesc( 0 )
	{
		NI_ASSERT( _pParentState != 0, "CCragState(): Invalid parameter: _pParentState == 0" );
	}
	//
protected:
	// CVSOState
	bool IsDrawSceneDrawTool() { return true; }
	bool EdgesMustBeZero() { return true; }
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
	float GetDefaultStep() { return CVSOManager::DEFAULT_STEP; }
	float GetDefaultWidth() { return CVSOManager::DEFAULT_WIDTH; }
	float GetDefaultHeight() { return CVSOManager::DEFAULT_HEIGHT; }
	float GetDefaultOpacity() { return CVSOManager::DEFAULT_OPACITY; }
	float GetDefaultStepOut() { return CVSOManager::DEFAULT_STEP / 4.0f; }
	//
	bool CanEditPoints( CVSOManager::SVSOSelection::ESelectionType eSelectionType );
	void PrepareInsertVSO();
	void PickVSO( const CVec3 &rvPos, CVSOIDList *pPickVSOIDList );

	//CVSOStateEx
	//bool CanInsertVSO();
	bool IsRealtimeUpdate() const { return false; }
	const std::string GetVSOName() const { return VSO_NAME; }
	const std::string GetVSOTypeName() const { return VSO_TYPE_NAME; }
	int GetVSOTypeID() const { return NDb::SCragDesc::typeID; }
	const CMapInfoController::SVSOUndoData::EVSOType GetVSOType() const { return CMapInfoController::SVSOUndoData::VSO_CRAG; }
	NMapInfoEditor::CVSOInstanceList* GetVSOList();
	//
	void InsertVSOInTerrain( const NDb::SVSOInstance &rVSO );
	void RemoveVSOFromTerrain( int nVSOID );
	void UpdateVSOInTerrain( int nVSOID );
};

/**
class CCragState : public CVSOState
{
	friend class CMultiInputState;
	friend class CMapInfoState;
	friend class CVSOMultiState;
	
	bool bVSOChanged;
	const NDb::SCragDesc* pSelectedCragDesc;
	//конструкторы и операторы присваивания
	CCragState( class CVSOMultiState *_pParentState = 0 ) : CVSOState( _pParentState ), bVSOChanged( false ), pSelectedCragDesc( 0 )
	{
		NI_ASSERT( _pParentState != 0, "CCragState(): Invalid parameter: _pParentState == 0" );
	}
	//
	bool InsertVSOToBase( int nVSOIndex, const NDb::SVSOInstance &rVSO );
	bool RemoveVSOFromBase( int nVSOIndex );
	inline bool UpdateVSOInBase( int nVSOIndex, const NDb::SVSOInstance &rVSO )
	{
		if ( RemoveVSOFromBase( nVSOIndex ) )
		{
			return InsertVSOToBase( nVSOIndex, rVSO );
		}
		return false;
	}
	
protected:
	// CVSOState
	bool CanEdit();
	bool IsDrawSceneDrawTool() { return true; }
	bool CanEditPoints( CVSOManager::SVSOSelection::ESelectionType eSelectionType );
	//
	bool EdgesMustBeZero() { return true; }
	bool IsEdgesMustBeOut() { return false; }
	bool IsClose() { return false; }
	bool IsComplete() { return true; }
	bool NoIntersection() { return false; }
	bool NoSelfIntersection() { return false; }
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
	float GetDefaultStep() { return CVSOManager::DEFAULT_STEP; }
	float GetDefaultWidth() { return CVSOManager::DEFAULT_WIDTH; }
	float GetDefaultHeight() { return CVSOManager::DEFAULT_HEIGHT; }
	float GetDefaultOpacity() { return CVSOManager::DEFAULT_OPACITY; }
	float GetDefaultStepOut() { return CVSOManager::DEFAULT_STEP / 4.0f; }
	//
	void PickVSO( const CVec3 &rvPos, CVSOIDList *pPickVSOIDList );
	bool CanInsertVSO();
	bool CanUpdateVSO( int nVSOID ) { return true; }
	bool CanRemoveVSO( int nVSOID ) { return true; }
	NDb::SVSOInstance* GetVSO( int nVSOID );
	void UpdateVSO( int nVSOID, EUpdateType eEpdateType, CVSOManager::SVSOSelection::ESelectionType eSelectionType, unsigned nFlags );
	void PrepareInsertVSO();
	int InsertVSO( const std::vector<CVec3> &rControlPointList );
	void RemoveVSO( int nVSOID );
};
/**/

