#if !defined(__EFFECT_STATE__)
#define __EFFECT_STATE__
#pragma once

#include "..\MapEditorLib\DefaultInputState.h"

class CEffectEditor;
class CEffectState : public CDefaultInputState
{
	//Данные специфичные для данного редактрора
	list<int> effectIDList;
	// Данные общего назначения 
	CEffectEditor *pEffectEditor;
public:
	CEffectState( CEffectEditor *_pCEffectEditor );
	//IInputState
	void Enter();
	void Leave();
};

#endif // !defined(__EFFECT_STATE__)

