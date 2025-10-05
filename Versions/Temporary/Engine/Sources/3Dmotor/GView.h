#pragma once

#include "3Dmotor_export.h"

#include "GSkeleton.h"
#include "System/time.hpp"
#include "GRenderModes.h"

struct SRandomSeed;
class CTransformStack;
class CMemObject;
class ILoadingCounter;
namespace NDb
{
	struct STexture;
	struct SModel;
	struct STemplVariant;
	struct SEffect;
	struct SAmbientLight;
	struct SComplexHead;
	//struct SAIGeometry;
	struct SSkeleton;
	struct SMaterial;
	struct SParticleInstance;
}
namespace NAnimation
{
	struct ISkeletonAnimator;
}
namespace NGfx
{
	struct SPixel8888;
	class CTexture;
}
class CAnimLight;

namespace NGScene
{
class CObjectInfo;
class CPolyline;
class CParticles;
class CParticleEffect;
struct SRenderStats;
struct IGScene;
class IPostProcess;
class CDecalTarget;
struct SDecalMappingInfo;
class IParticleFilter;
class IMaterial;
class CLightmapsHolder;
class CLightmapsLoader;
class CLightmapsTempHolder;
struct SMaterialCreateInfo;
class IFader;
struct SDepthOfField;
class CDecalFader;

struct SRoomInfo
{
	typedef unsigned short ushort;
	ZDATA
	ushort nLightFlags;
	short nFloor;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nLightFlags); f.Add(3,&nFloor); return 0; }
	short nLODFlags;
	//
	SRoomInfo(): nLightFlags(0), nFloor(0), nLODFlags(0) {}
	SRoomInfo( int _nLightFlags, int _nFloor, int _nLODFlags = 0 ): nLightFlags(_nLightFlags), nFloor(_nFloor), nLODFlags(_nLODFlags) {}
	bool operator==( const SRoomInfo &a ) const { return a.nLightFlags == nLightFlags && a.nFloor == nFloor; }
};

struct SFullRoomInfo
{
	ZDATA
	SRoomInfo room;
	CPtr<CObjectBase> pUser;
	int nUserID; // top 8 bits are used for material number
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&room); f.Add(3,&pUser); f.Add(4,&nUserID); return 0; }
	//
	SFullRoomInfo() : nUserID(0) {}
	SFullRoomInfo( int _nFloor, int _nLODFlags = 0 ) : room(0, _nFloor, _nLODFlags), nUserID(0) {}
	SFullRoomInfo( const SRoomInfo &_r, CObjectBase *_p, int _nUserID ) : room(_r), pUser(_p), nUserID(_nUserID) {}
};

class CCreateMeshTransform
{
	SFBTransform place;
	CPtr<CFuncBase<SFBTransform> > pTransformer;
public:
	CCreateMeshTransform( const SFBTransform &_place ) : place(_place), pTransformer(0) {}
	CCreateMeshTransform( CFuncBase<SFBTransform> *_pTransformer ) : pTransformer(_pTransformer) { Identity( &place.forward ); Identity( &place.backward ); }
	const SFBTransform &GetPlace() const { return place; }
	CFuncBase<SFBTransform> *GetTransformer() const { return pTransformer; }
};

class CCreateMeshBound
{
	ZDATA
	SBound bound;
	CPtr<CFuncBase<SBound> > pBounder;
	SBound hintBV;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&bound); f.Add(3,&pBounder); return 0; }
public:
	CCreateMeshBound() : pBounder(0) { Zero(bound); hintBV = MakeLargeHintBound(); }
	CCreateMeshBound( const SBound &_bound ) : bound(_bound), hintBV(_bound), pBounder(0) {}
	CCreateMeshBound( CFuncBase<SBound> *_pBounder ) : pBounder(_pBounder) { bound.BoxExInit( VNULL3, VNULL3 ); hintBV = MakeLargeHintBound(); }
	CCreateMeshBound( CFuncBase<NAnimation::SGrannySkeletonPose> *pAnimation, const NDb::SModel *pModel );
	const SBound &GetBound() const { return bound; }
	CFuncBase<SBound> *GetBounder() const { return pBounder; }
	const SBound GetHintBV() const { return hintBV; }

	void Create( CFuncBase<NAnimation::SGrannySkeletonPose> *pAnimation, const NDb::SModel *pModel );
	void Set( const SBound &_bound )
	{
		bound = _bound; 
		hintBV = _bound;
	}
	void Set( CFuncBase<SBound> *_pBounder )
	{
		pBounder = _pBounder;
		hintBV = MakeLargeHintBound();
	}
};

class CMeshAnimStuff
{
	const NDb::SModel *pModel;
	CFuncBase<NAnimation::SGrannySkeletonPose> *pAnimation;
	std::vector<CPtr<CFuncBase<float> > > *pTransparencyAnimations;
public:
	CMeshAnimStuff( const NDb::SModel *pM, CFuncBase<NAnimation::SGrannySkeletonPose> *pA, std::vector<CPtr<CFuncBase<float> > > *pTA = 0 ) :
		pModel(pM), pAnimation(pA), pTransparencyAnimations(pTA) {}
	CMeshAnimStuff( void * ) : pModel(0), pAnimation(0), pTransparencyAnimations(0) {}
		const NDb::SModel *GetModel() const { return pModel; }
	CFuncBase<NAnimation::SGrannySkeletonPose> *GetAnimation() const { return pAnimation; }
	std::vector<CPtr<CFuncBase<float> > > *GetTransparencyAnimations() const { return pTransparencyAnimations; }
};

class IGameView : public CObjectBase
{
public:
	struct SDrawInfo
	{
		CTransformStack *pTS; // pTS на весь экран
		NGfx::CTexture *pTarget; // render to texture if not 0
		CVec2 vOrigin, vSize; // in [0,1] diapason
		SRTClearParams rtClear;
		bool bUseDefaultClearColor, bShadows;

		SDrawInfo() : pTS(0), vOrigin(0,0), vSize(1,1), bUseDefaultClearColor(true), pTarget(0), bShadows(true) {}
	};
	struct SPartInfo
	{
		ZDATA
		CObj<CPtrFuncBase<CObjectInfo> > pGeometry;
		CObj<IMaterial> pMaterial;
		int nOrigMeshIndex;
		bool bAnimated;
		bool bWindAffected;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pGeometry); f.Add(3,&pMaterial); f.Add(4,&nOrigMeshIndex); f.Add(5,&bAnimated); f.Add(6,&bWindAffected); return 0; }
		SPartInfo() : nOrigMeshIndex(-1), bAnimated(true), bWindAffected(false) {}
		SPartInfo( CPtrFuncBase<CObjectInfo> *_pGeom, IMaterial *_pMat, int _nOrigMeshIndex = (-1) )
			: pGeometry(_pGeom), pMaterial(_pMat), nOrigMeshIndex(_nOrigMeshIndex), bAnimated(true), bWindAffected(false)
		{}
	};
	struct SMeshInfo
	{
		ZDATA
		std::vector<SPartInfo> parts;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&parts); return 0; }
	};
	virtual IMaterial *CreateMaterial( const NDb::SMaterial *pMaterial ) = 0;
	virtual IMaterial *CreateMaterial( const CVec4 &vColor, bool bDoesCastShadow = true ) = 0;
	virtual void CreateMaterialInfo( const NDb::SMaterial *pMaterial, SMaterialCreateInfo *pRes ) = 0;
	virtual void CreateMeshInfo( const NDb::SModel *pModel, SMeshInfo *pRes, bool bWholeAnimated, int nPlayer = 0, bool bIsLightMapped = false) = 0;
	virtual CObjectBase* CreateMesh( const SMeshInfo &meshInfo, const CCreateMeshTransform &meshTransform,
		const CCreateMeshBound &meshBound, const CMeshAnimStuff &animStuff, const SFullRoomInfo &_g = SFullRoomInfo(), IFader *pFader = 0 ) = 0;
	virtual CObjectBase* CreateDynamicMesh( const SMeshInfo &meshInfo, CFuncBase<SFBTransform> *pPlacement, CFuncBase<SBound> *pBound, const SBound &hintBV, const SFullRoomInfo &_g = SFullRoomInfo(), IFader *pFader = 0 ) = 0;
	virtual CObjectBase* CreateParticles( const NDb::SEffect *pEffect, STime stBeginTime, CFuncBase<STime> *pTime, CFuncBase<SFBTransform> *pPlacement, const SRoomInfo &_g = SRoomInfo(),
		IFader *pFader = 0, CFuncBase<NAnimation::SGrannySkeletonPose> *pScAnim = 0, NAnimation::SGrannySkeletonHandle *_pSkel = 0, IParticleFilter *pFilter = 0 ) = 0;
	virtual CObjectBase* CreateParticles( const NDb::SEffect *pEffect, STime stBeginTime, CFuncBase<STime> *pTime, const SFBTransform &place, const SRoomInfo &_g = SRoomInfo(),
		IFader *pFader = 0, IParticleFilter *pFilter = 0 ) = 0;
	virtual CObjectBase* CreateRain( const NDb::SParticleInstance *pInstance, CFuncBase<STime> *pTime, IParticleFilter *pFilter, const SRoomInfo &_g = SRoomInfo() ) = 0;
	virtual CPolyline* CreatePolyline( const std::vector<CVec3> &points, const std::vector<unsigned short> &indices, const CVec4 &color, bool bDepthTest ) = 0;
	virtual CObjectBase* Precache( const NDb::SModel *pModel ) = 0;
	virtual void Precache( const NDb::SEffect *pEffect ) = 0;
	virtual void LoadEverything() = 0;
	virtual void Draw( const SDrawInfo &drawInfo ) = 0;
	virtual CVec2 GetScreenRect() = 0;
	virtual int  GetCutFloor() = 0;
	virtual void SetCutFloor( int nFloor ) = 0;
	virtual CObjectBase* AddPointLight( const CVec3 &ptColor, const CVec3 &ptOrigin, float fR ) = 0;
	virtual CObjectBase* AddPointLight( CPtrFuncBase<CAnimLight> *pLight ) = 0;
	virtual CObjectBase* AddFlare( CFuncBase<CVec3> *pOrigin, CFuncBase<STime> *pTime, int nFloor, float fFlareRadius, const NDb::STexture *pFlareTexture, float fOnTime, float fOffTime ) = 0;
	virtual CObjectBase* AddDirFlare( CFuncBase<CVec3> *pPos, CFuncBase<CVec3> *pDir, CFuncBase<CVec2> *pSize, const CVec2 &vMaxSize, const NDb::STexture *pTexture, int nFloor ) = 0;
	virtual CObjectBase* AddDirFlare( CFuncBase<CVec3> *pPos, CFuncBase<CVec3> *pDir, const CVec2 &sSize, const NDb::STexture *pTexture, int nFloor ) = 0;
	virtual CObjectBase* AddPostFilter( const std::vector<CObjectBase*> &target, IPostProcess *pEffect ) = 0;
	virtual CObjectBase* AddSpotLight( const CVec3 &ptColor, const CVec3 &ptOrigin, const CVec3 &ptDir, float fFOV, float fRadius, const NDb::STexture *pMask, bool bLightmapOnly = false ) = 0;
	virtual CDecalTarget* CreateDecalTarget( const std::vector<CObjectBase*> &targets, const SDecalMappingInfo &_info ) = 0;
	virtual CObjectBase* AddDecal( NGScene::CDecalTarget *pTarget, const NDb::SMaterial *pMaterial ) = 0;
	virtual CDecalFader* AddDecal( NGScene::CDecalTarget *pTarget, const NDb::SMaterial *pMaterial, STime tFadeInStart, STime tFadeInEnd, STime tFadeOutStart, STime tFadeOutEnd, CFuncBase<STime> *pTime ) = 0;
	virtual void SetWarFogBlend( float fBlend ) = 0;
	virtual void SetWarFog( const CArray2D<unsigned char> &fog, float fScale ) = 0;
	virtual void SetAmbient( const NDb::SAmbientLight *pLight, CFuncBase<STime> *pTime = 0 ) = 0;
	virtual void SetAmbientEffect( const NDb::SEffect *pEffect, STime stBeginTime, CFuncBase<STime> *pTime ) = 0;
	virtual void SetDepthOfField( SDepthOfField *pDOFParams ) = 0;
	virtual SDepthOfField *GetDepthOfField() = 0;
	virtual ESceneRenderMode GetRenderMode() const = 0;
	virtual void SetRenderMode( ESceneRenderMode mode ) = 0;
	virtual void SetHSRMode( EHSRMode m ) = 0;
	virtual EHSRMode GetHSRMode() const = 0;
	virtual void SetTransparentMode( ETransparentMode m ) = 0;
	virtual ETransparentMode GetTransparentMode() const = 0;
	virtual bool TraceScene( const CRay &r, float *pfT, CVec3 *pNormal, EScenePartsSet ps = SPS_STATIC, SFullRoomInfo *pRoomInfo = 0, CObjectBase **ppPart = 0, bool bOpaqueOnly = true ) = 0;
	virtual void MakeHQShot( const SDrawInfo &drawInfo, CArray2D<NGfx::SPixel8888> *pRes ) = 0;
	virtual void SetLoadingCounter( ILoadingCounter *pWaitLoad, ILoadingCounter *pCalcTerrain, ILoadingCounter *pCounter ) = 0;
	virtual void WaitForLoad( bool bWait = true ) = 0;
	virtual void SetTwilight(bool _bIsTwilight ) = 0;
	virtual void SetFreezeMode( bool mode ) = 0;

	SMeshInfo MakeMeshInfo( CPtrFuncBase<CObjectInfo> *pGeom, const NDb::SMaterial *pMat )
	{
		SMeshInfo res;
		res.parts.push_back( SPartInfo( pGeom, CreateMaterial( pMat ), (-1) ) );
		return res;
	}
};
_3DMOTOR_EXPORT IGameView* CreateNewView();
IGameView* CreateNewFastInterfaceView();
// wrappers to Gfx to exclude interface on Gfx dependency, realisation in GScene.cpp
_3DMOTOR_EXPORT bool Is3DActive();
_3DMOTOR_EXPORT void SetWireframe( bool bWire );
bool GetWireframe();
_3DMOTOR_EXPORT void Flip();
_3DMOTOR_EXPORT void GetRenderStats( SRenderStats *pStats );
_3DMOTOR_EXPORT float GetFrameTime();
_3DMOTOR_EXPORT int CalcTouchedTextureSize();
_3DMOTOR_EXPORT void ClearScreen( const CVec3 &vColor );
CVec2 GetScreenRect();
// p - CreateMesh() result, fFade is in [0,1], 0 - invisible, 1 - normal
_3DMOTOR_EXPORT void SetFade( CObjectBase *p, float fFade );
void SetPriority( CObjectBase *_p, int _nPriority );
_3DMOTOR_EXPORT void StopParticlesGeneration( CObjectBase *_p, STime tStop );
_3DMOTOR_EXPORT void StopDynamicLighting( CObjectBase *_p );
// mode switch
void SetNextLightmapViewMode( IGameView *p );
void SetNextTranspRenderMode( IGameView *p );
void SetNextHSRMode( IGameView *p );
void ReloadTexture( const NDb::STexture *p );

CLightmapsHolder *CalcLightmaps( IGameView *pScene, CObjectBase *pUser, int nUserID, const SSphere &highResLM, ELightmapQuality quality, CLightmapsTempHolder *pTmpHolder);
void ApplyLightmaps( IGameView *pScene, CObjectBase *pUser, CLightmapsHolder *pLightmaps, CLightmapsLoader * pLD );
void CollectAllParts( std::vector<CObjectBase*> *pRes, IGameView *_pView );
CObjectBase *GetPartGeometry( CObjectBase *pB );

void CreateAnimatedTransparencyChannels( std::vector<CPtr<CFuncBase<float> > > *pResult,
		const IGameView::SMeshInfo &meshInfo, const NDb::SModel *pModel,
		NAnimation::ISkeletonAnimator *pAnimator
		);
}

