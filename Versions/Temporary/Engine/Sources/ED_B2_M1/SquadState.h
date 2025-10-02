#if !defined(__SQUAD_STATE__)
#define __SQUAD_STATE__
#pragma once

#include "../MapEditorLib/MultiInputState.h"
#include "../MapEditorLib/Interface_CommandHandler.h"

//
//			
//					SQUAD STATE					
//
//

class CSquadEditor;
class CSquadState : public CMultiInputState, public ICommandHandler
{
	friend class CSquadEditor;

	// Данные общего назначения 
	CSquadEditor *pSquadEditor;
	bool bNeedLoadEnterConfig;

	// Методы общего назначения
	bool IsMultiInputState( int nStateIndex );
	void LoadEnterConfig();

	//конструкторы и операторы присваивания
	CSquadState( CSquadEditor *_pSquadEditor );
			
public:
	enum EInputStates
	{
		IS_FORMATION = 0,
		//
		//
		IS_COUNT
	};
	enum EFormationInputSubstates
	{
		FORMATION_ISS_FORMATION = 0,
		//
		//
		FORMATION_ISS_COUNT
	};

	static const UINT INPUT_STATE_LABEL_ID[IS_COUNT];
	static const UINT FORMATION_INPUT_SUSBSTATE_LABEL_ID[FORMATION_ISS_COUNT];

	//CMultiInputState
	void Enter();
	void Leave();

	//ICommandHandler
	bool HandleCommand( UINT nCommandID, DWORD dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );
};

#endif // !defined(__SQUAD_STATE__)

