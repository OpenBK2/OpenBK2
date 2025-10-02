#pragma once

#include "../3DMotor/GView.h"
#include "../B2_M1_Terrain/PatchHolder.h"

namespace NDb
{
	enum ESelectionType;
};

class CTerrainManager;
class CCSFBTransform;
class CCSBound;

struct SSelectionInfo
{
	CVec3 vMin, vMax;
	//
	SSelectionInfo() {}
	SSelectionInfo( const CVec3 &_vCenter, const CVec3 &_vSize ) { vMin = _vCenter - _vSize; vMax = _vCenter + _vSize; }
	void MakeBound( SBound *pRes ) { pRes->BoxInit( vMin, vMax ); }
};

inline bool operator==( const SSelectionInfo &a, const SSelectionInfo &b ) { return a.vMin == b.vMin && a.vMax == b.vMax; }

class CVisObjSelectionInfo : public CPtrFuncBase<NGScene::CObjectInfo>
{
	OBJECT_BASIC_METHODS( CVisObjSelectionInfo );

	CPtr<CTerrainManager> pTerraManager;
	SSelectionInfo info;
	bool bUpdate;
	NDb::ESelectionType eSelType;
protected:
	void Recalc();
	bool NeedUpdate() { return bUpdate; }
public:
	NMeshData::SMeshData data;
	//
	CVisObjSelectionInfo() {}
	CVisObjSelectionInfo( CTerrainManager *_pTerraManager ): pTerraManager( _pTerraManager ), bUpdate( true ) {}
	//
	void SetInfo( const SSelectionInfo &_info );
	void SetSelectionType( const NDb::ESelectionType _eSelType ) { eSelType = _eSelType; }
	void ForceUpdate() { bUpdate = true; }
	//
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, static_cast<CPtrFuncBase<NGScene::CObjectInfo>*>(this) );
		saver.Add( 2, &pTerraManager );
		saver.Add( 3, &info );
		saver.Add( 4, &bUpdate );
		saver.Add( 5, &eSelType );
		saver.Add( 6, &data );
		return 0;
	}
};

typedef NMeshData::SPatchHolder<CVisObjSelectionInfo> CVisObjSelectionHolder;

struct SVisObjSelection
{
	CObj<CCSFBTransform> pTransform;
	CObj<CCSBound> pBound;
	float fSelScale;
	float fSelSize;
	NDb::ESelectionType eSelType;
	CVisObjSelectionHolder selHolder;
	CVec3 vSelCenter;
	//
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &pTransform );
		saver.Add( 2, &pBound );
		saver.Add( 3, &fSelScale );
		saver.Add( 4, &fSelSize );
		saver.Add( 5, &eSelType );
		//saver.Add( 6, &selHolder );
		saver.Add( 7, &vSelCenter );
		return 0;
	}
	
	void Recreate() { SVisObjSelection::~SVisObjSelection(); new(this) SVisObjSelection(); }
};

struct SVisObjSelectionHandler : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( SVisObjSelectionHandler )
public:
	enum EState
	{
		ESS_NONE,
		ESS_FADE_IN,
		ESS_FADE_OUT,
	};

	ZDATA
	int nID;
	EState eState;
	SVisObjSelection visObjSelection;
	CVec3 vPos;
	float fScale;
	NDb::ESelectionType eSelType;
	NTimer::STime timeStart;
	int nFadeIn; // msec
	int nFadeOut; // msec
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nID); f.Add(3,&eState); f.Add(4,&visObjSelection); f.Add(5,&vPos); f.Add(6,&fScale); f.Add(7,&eSelType); f.Add(8,&timeStart); f.Add(9,&nFadeIn); f.Add(10,&nFadeOut); return 0; }
};

