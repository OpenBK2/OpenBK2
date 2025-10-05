#pragma once

#include "System/Dg.h"
#include "B2_M1_Terrain/PatchHolder.h"

namespace NGScene
{
	class IGameView;
	class CObjectInfo;
};

namespace NDb
{
	struct SMaterial;
}

class CExplosionObjInfo : public CPtrFuncBase<NGScene::CObjectInfo>
{
	NMeshData::SMeshData data;
	void Recalc();
public:
	CExplosionObjInfo() {}
	CExplosionObjInfo( const NMeshData::SMeshData &_data ) : data(_data) {}
};

typedef NMeshData::SPatchHolder<CExplosionObjInfo> CExplosionObjHolder;

struct SExplosionObj
{
	int nID;
	CExplosionObjHolder explosionHolder;
	//
	bool operator == ( const SExplosionObj &v ) const { return v.nID == nID; }
};

class CExplosionsManager
{
	std::list<SExplosionObj> explosions;
	int nLastID;
	CObj<NGScene::IGameView> pGScene;
	//
public:
	CExplosionsManager() : nLastID(0) {}
	int AddExplosion( const NMeshData::SMeshData &_data, const NDb::SMaterial *pMaterial );
	void RemoveExplosion( const int nID );
	void AttachGameView( NGScene::IGameView *_pGScene ) { pGScene = _pGScene; }
};


