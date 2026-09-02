#pragma once

#include "MapEditorLib/DefaultInputState.h"

class CEffectEditor;
class CEffectState : public CDefaultInputState
{
	//Данные специфичные для данного редактрора
	std::list<int> effectIDList;
	// Данные общего назначения 
	CEffectEditor *pEffectEditor;
public:
	CEffectState( CEffectEditor *_pCEffectEditor );
	//IInputState
	void Enter();
	void Leave();
};



