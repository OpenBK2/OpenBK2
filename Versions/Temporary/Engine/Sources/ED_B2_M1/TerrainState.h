#if !defined(__TERRAIN_STATE__)
#define __TERRAIN_STATE__
#pragma once

#include "../MapEditorLib/DefaultInputState.h"

class CTerrainEditor;
class CTerrainState : public CDefaultInputState
{
	//Данные специфичные для данного редактрора
	list<int> effectIDList;
	// Данные общего назначения 
	CTerrainEditor *pTerrainEditor;
public:
	CTerrainState( CTerrainEditor *_pCTerrainEditor );
	//IInputState
	void Enter();
	void Leave();
};

#endif // !defined(__TERRAIN_STATE__)


