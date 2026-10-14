#if !defined( __COAST_STATE__ )
#define __COAST_STATE__
#pragma once

#include "ED_B2_M1/MapInfoController.h" // CMapInfoController
#include "Stats_B2_M1/DBMapInfo.h"      // NDb::SMapInfo
#include "VSOState.h"

class CCoastState : public CVSOState
{
	friend class CMultiInputState;
	friend class CMapInfoState;
	friend class CVSOMultiState;

	bool bVSOChanged;
	NDb::SVSOInstance oldVSOInstance;

	//конструкторы и операторы присваивания
	CCoastState( class CVSOMultiState *_pParentState = 0 )
		: CVSOState( _pParentState )
	{
		NI_ASSERT( _pParentState != 0, "CCoastState(): Invalid parameter: _pParentState == 0" );
	}

	//
	bool InsertVSOToBase( class CObjectBaseController *pObjectController, const NDb::SVSOInstance &rVSO );
	bool RemoveVSOFromBase( class CObjectBaseController *pObjectController );
	bool UpdateVSOInBase( class CObjectBaseController *pObjectController, const NDb::SVSOInstance &rVSO );

protected:
	// CVSOState
	bool CanEdit() override;

	bool IsDrawSceneDrawTool() override
	{
		return true;
	}

	bool CanEditPoints( CVSOManager::SVSOSelection::ESelectionType eSelectionType ) override;

	bool EdgesMustBeZero() override
	{
		return false;
	}

	bool IsEdgesMustBeOut() override
	{
		return true;
	}

	bool IsClose() override
	{
		return false;
	}

	bool IsComplete() override
	{
		return true;
	}

	bool NoIntersection() override
	{
		return true;
	}

	bool NoSelfIntersection() override
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
	void PickVSO( const CVec3 &rvPos, CVSOIDList *pPickVSOIDList ) override;
	bool CanInsertVSO() override;

	bool CanUpdateVSO( int nVSOID ) override
	{
		return false;
	}

	bool CanRemoveVSO( int nVSOID ) override
	{
		return false;
	}

	NDb::SVSOInstance *GetVSO( int nVSOID, int *pnVSOIndex ) override;
	void UpdateVSO( int nVSOID,
	                EUpdateType eEpdateType,
	                CVSOManager::SVSOSelection::ESelectionType eSelectionType,
	                unsigned nFlags ) override;

	void PrepareInsertVSO() override {}

	int InsertVSO( const std::vector<CVec3> &rControlPointList ) override;
	void RemoveVSO( int nVSOID ) override;

	//
	const CMapInfoController::SVSOUndoData::EVSOType GetVSOType() const
	{
		return CMapInfoController::SVSOUndoData::VSO_COAST;
	}

public:
	static void
	GetWaterPos( const NDb::SMapInfo *pMapInfo, const std::vector<NDb::SVSOPoint> &rPoints, CVec3 *pWaterPos );
};

#endif // !defined(__COAST_VSO_STATE__)
