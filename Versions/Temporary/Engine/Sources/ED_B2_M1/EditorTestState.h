#pragma once

#include "../MapEditorLib/DefaultInputState.h"

class CEditorTestEditor;
class CEditorTestState : public CDefaultInputState
{
	// Данные общего назначения 
	CEditorTestEditor *pEditorTestEditor;
public:
	CEditorTestState( CEditorTestEditor *_pCEditorTestEditor );
	//IInputState
	void Enter();
	void Leave();
};



