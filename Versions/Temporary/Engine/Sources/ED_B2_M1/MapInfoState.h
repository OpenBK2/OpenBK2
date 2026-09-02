#pragma once

#include "MapEditorLib/MultiInputState.h"
#include "MapEditorLib/MaskManipulator.h"
#include "MapEditorLib/Interface_CommandHandler.h"

#include <cstdint>

class CMapInfoEditor;
class CMapInfoState : public CMultiInputState, public ICommandHandler
{
	friend class CMapInfoEditor;

	// Данные общего назначения 
	CMapInfoEditor *pMapInfoEditor;
	CPtr<CMaskManipulator> pMaskManipulator;
	// Методы общего назначения
	bool IsMultiInputState( int nStateIndex );
	void LoadEnterConfig();
	void SaveEnterConfig();
	void SetShowStatistic( bool _bShowStatistic );
	void ShowStatistic();

	//конструкторы и операторы присваивания
	CMapInfoState( CMapInfoEditor *_pMapInfoEditor );
public:
	enum EInputStates
	{
		IS_TERRAIN										= 0,
		IS_OBJECT											= 1,
		IS_GAMEPLAY										= 2,
		IS_SCRIPT											= 3,
		//IS_ADVANCED										= 4,
		IS_COUNT											= 4,
	};
	enum ETerrainInputSubstates
	{
		//TERRAIN_ISS_TILE							= 0,
		//TERRAIN_ISS_HEIGHT						= 1,
		//TERRAIN_ISS_HEIGHT_V2					= 2,
		TERRAIN_ISS_HEIGHT_V3					= 0,
		TERRAIN_ISS_FIELD							= 1,
		TERRAIN_ISS_COUNT							= 2,
	};
	enum EObjectInputSubstates
	{
		OBJECT_ISS_MAP_OBJECT					= 0,
		OBJECT_ISS_VSO								= 1,
		OBJECT_ISS_COUNT							= 2,
	};
	enum EGameplayInputSubstates
	{
		GAMEPLAY_ISS_REINF_POINTS			= 0,
		GAMEPLAY_ISS_START_CAMERA			=	1,
		GAMEPLAY_ISS_AIGENERAL				= 2,
		GAMEPLAY_ISS_UNIT_START_CMD		= 3,
		GAMEPLAY_ISS_COUNT						= 4,
	};
	enum EScriptInputSubstates
	{
		SCRIPT_ISS_SCRIPT_AREAS				= 0,
		SCRIPT_ISS_SCRIPT_MOVIES			= 1,
		SCRIPT_ISS_COUNT							= 2,
	};
	//enum EMovEditorInputSubstates
	//{
	//	MOV_EDITOR_ISS_EDITOR					= 0,
	//	MOV_EDITOR_ISS_COUNT					= 1,
	//};
	/**
	enum EAdvancedInputSubstates
	{
		ADV_ISS_CLIPBOARD							= 0,
		ADV_ISS_COUNT									= 1,
	};
	/**/
	//
	static const unsigned INPUT_STATE_LABEL_ID[IS_COUNT];
	static const unsigned TERRAIN_INPUT_SUSBSTATE_LABEL_ID[TERRAIN_ISS_COUNT];
	static const unsigned OBJECT_INPUT_SUSBSTATE_LABEL_ID[OBJECT_ISS_COUNT];
	static const unsigned GAMEPLAY_INPUT_SUSBSTATE_LABEL_ID[GAMEPLAY_ISS_COUNT];
	static const unsigned SCRIPT_INPUT_SUSBSTATE_LABEL_ID[SCRIPT_ISS_COUNT];
	//static const unsigned MOV_EDITOR_INPUT_SUSBSTATE_LABEL_ID[MOV_EDITOR_ISS_COUNT];
	//static const unsigned ADV_INPUT_SUSBSTATE_LABEL_ID[ADV_ISS_COUNT];
	//
	static const unsigned DEFAULT_INPUT_STATE;
	static const unsigned INPUT_SUBSTATE_COUNT[IS_COUNT];
	static const unsigned DEFAULT_INPUT_SUBSTATE[IS_COUNT];

	//CMultiInputState
	void Enter();
	void Enter2();
	void Leave();
	void OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint );
	void OnRButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint );
	void OnRButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint );
	void OnMButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint );
	void OnMButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint );

	void CancelSelection();
	//ICommandHandler
	bool HandleCommand( unsigned nCommandID, uintptr_t dwData );
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );
};



