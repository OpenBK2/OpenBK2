#pragma once

#include "VSOStateEx.h"

class CLakeState : public CVSOStateEx
{
	static const std::string VSO_NAME;
	static const std::string VSO_TYPE_NAME;

	friend class CMultiInputState;
	friend class CMapInfoState;
	friend class CVSOMultiState;

	//конструкторы и операторы присваивания
	CLakeState( class CVSOMultiState *_pParentState = 0 )
		: CVSOStateEx( _pParentState )
	{
		NI_ASSERT( _pParentState != 0, "CLakeState(): Invalid parameter: _pParentState == 0" );
	}

protected:
	// CVSOState
	bool IsDrawSceneDrawTool() override
	{
		return true;
	}

	bool EdgesMustBeZero() override
	{
		return false;
	}

	bool IsEdgesMustBeOut() override
	{
		return false;
	}

	bool IsClose() override
	{
		return true;
	}

	bool IsComplete() override
	{
		return false;
	}

	bool NoIntersection() override
	{
		return true;
	}

	bool NoSelfIntersection() override
	{
		return true;
	}

	bool CanUpdateVSO( int nVSOID ) override
	{
		return true;
	}

	bool CanRemoveVSO( int nVSOID ) override
	{
		return true;
	}

	void GetControlPointBounds( int *pnMinCount, int *pnMaxCount ) override
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

	float GetDefaultStep() override
	{
		return CVSOManager::DEFAULT_STEP * 8.0f;
	}

	float GetDefaultWidth() override
	{
		return CVSOManager::DEFAULT_WIDTH;
	}

	float GetDefaultHeight() override
	{
		return CVSOManager::DEFAULT_HEIGHT;
	}

	float GetDefaultOpacity() override
	{
		return CVSOManager::DEFAULT_OPACITY;
	}

	float GetDefaultStepOut() override
	{
		return CVSOManager::DEFAULT_STEP / 4.0f;
	}

	//
	bool CanEditPoints( CVSOManager::SVSOSelection::ESelectionType eSelectionType ) override;

	void PrepareInsertVSO() override {}

	//
	void PickVSO( const CVec3 &rvPos, CVSOIDList *pPickVSOIDList ) override;

	//CVSOStateEx
	//bool CanInsertVSO();
	bool IsRealtimeUpdate() const override
	{
		return false;
	}

	const std::string GetVSOName() const override
	{
		return VSO_NAME;
	}

	const std::string GetVSOTypeName() const override
	{
		return VSO_TYPE_NAME;
	}

	int GetVSOTypeID() const override
	{
		return NDb::SLakeDesc::typeID;
	}

	const CMapInfoController::SVSOUndoData::EVSOType GetVSOType() const override
	{
		return CMapInfoController::SVSOUndoData::VSO_LAKE;
	}

	NMapInfoEditor::CVSOInstanceList *GetVSOList() override;
	//
	void InsertVSOInTerrain( const NDb::SVSOInstance &rVSO ) override;
	void RemoveVSOFromTerrain( int nVSOID ) override;
	void UpdateVSOInTerrain( int nVSOID ) override;
};
