#pragma once

#include "MapEditorLib/DefaultInputState.h"

class CTerrainEditor;
class CTerrainState : public CDefaultInputState
{
	//Данные специфичные для данного редактрора
	std::list<int> effectIDList;
	// Данные общего назначения 
	CTerrainEditor *pTerrainEditor;
public:
	CTerrainState( CTerrainEditor *_pCTerrainEditor );
	//IInputState
	void Enter();
	void Leave();
};



