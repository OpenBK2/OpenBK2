#pragma once

#include "System/Dg.h"
#include "B2_M1_Terrain/PatchHolder.h"
#include "WaterStuff.h"

class CCSBound;

namespace NDb
{
	struct SWater;
}

namespace NGScene
{
	class IGameView;
}

class CVisSurfPatch : public CPtrFuncBase<NGScene::CObjectInfo>
{
	CVec2 vMaxMapCoords;
	//
	bool bUpdate;
	CDGPtr<CFuncBase<STime> > pTimer;
protected:
	void UpdateGeomData();
	void Recalc();
	bool NeedUpdate() { return pTimer == 0 ? bUpdate : pTimer.Refresh(); }
public:
	NMeshData::SMeshData data;
	std::vector<float> texYOffsets;
	//
	CVisSurfPatch( CFuncBase<STime> *_pTimer, const CVec2 &vBounds )
		: bUpdate( true ), pTimer( _pTimer ), vMaxMapCoords( vBounds ) {}
};
typedef NMeshData::SPatchHolder<CVisSurfPatch> CVisSurfPatchHolder;

class CSurfController
{
	CObj<NGScene::IGameView> pGScene;
	std::vector<CVisSurfPatchHolder> surfPatches;
	std::vector<CObj<CCSBound> > surfPatchesBounds;
	//
	CPtr<CFuncBase<STime> > pTimer;
public:
	CSurfController() {}
	void Init( const float fAngle, const CVec2i &vMapSize, const std::vector<NWaterStuff::SSurfBorder> &borders, NGScene::IGameView *_pGScene );
	void AttachTimer( CFuncBase<STime> *_pTimer ) { pTimer = _pTimer; }
};


