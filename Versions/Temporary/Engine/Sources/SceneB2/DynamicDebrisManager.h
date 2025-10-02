#pragma once

#include "B2_M1_Terrain/PatchHolder.h"
#include "3DMotor/GView.h"

class CVisDynamicDebrisPatch : public CPtrFuncBase<NGScene::CObjectInfo>
{
	NMeshData::SMeshDataTex2 data;
	void Recalc();
public:
	CVisDynamicDebrisPatch() {}
	CVisDynamicDebrisPatch( const NMeshData::SMeshDataTex2 &_data ) : data(_data) {}
};
typedef NMeshData::SPatchHolder<CVisDynamicDebrisPatch> CVisDynamicDebrisPatchHolder;

class CDynamicDebrisManager
{
	CObj<NGScene::IGameView> pGScene;
	vector<CVisDynamicDebrisPatchHolder> debrisPatches;
	//
public:
	//
	CDynamicDebrisManager() {}
	void AttachGameView( NGScene::IGameView *_pGScene ) { pGScene = _pGScene; }
	void AddDynamicDebris( const NMeshData::SMeshDataTex2 &debrisData, const NDb::SMaterial *pMaterial );
};


