#pragma once

#include "MapEditorLib/MultiInputState.h"

#include "BuildingEditor.h"

#include <cstdint>

//
//
//					BUILDING STATE
//
//

class CBuildingEditor;
class CBuildingState : public CMultiInputState, public ICommandHandler
{
	friend class CBuildingEditor;

	// Данные общего назначения 
	CBuildingEditor *pBuildingEditor;
	bool bNeedLoadEnterConfig;

	// Методы общего назначения
	bool IsMultiInputState( int nStateIndex )
	{
		return (nStateIndex == IS_POINTS);
	}

	void LoadEnterConfig();

public:
	//конструкторы и операторы присваивания
	CBuildingState( CBuildingEditor *_pBuildingEditor );
			
	enum EInputStates
	{
		IS_POINTS = 0,
		//
		//
		IS_COUNT
	};
	enum EPointsInputSubstates
	{
		POINTS_ISS_SMOKE_POINTS = 0,
		POINTS_ISS_SLOTS = 1,
		POINTS_ISS_ENTRANCE_POINTS = 2,
		POINTS_ISS_SURFACE_POINTS = 3,
		POINTS_ISS_DAMAGE_LEVELS = 4,
		//
		//
		POINTS_ISS_COUNT
	};

	static const unsigned INPUT_STATE_LABEL_ID[IS_COUNT];
	static const unsigned POINTS_INPUT_SUSBSTATE_LABEL_ID[POINTS_ISS_COUNT];

	//CMultiInputState
	void Enter();
	void Leave();

	//ICommandHandler
	bool HandleCommand( unsigned nCommandID, uint32_t dwData );
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );
};


