#pragma once

#include "3Dmotor_export.h"

#include "..\System\DG.h"
#include "..\3Dlib\GGeometry.h"
#include "GRenderModes.h"
#include "..\System\Time.hpp"
////////////////////////////////////////////////////////////////////////////////////////////////////
class CTransformStack;
class CRectLayout;
class CFontFormatInfo;
class ILoadingCounter;
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NGfx
{
	class CTexture;
	class CGeometry;
	class CRenderContext;
	class CCubeTexture;
}
namespace NGScene
{
////////////////////////////////////////////////////////////////////////////////////////////////////
class CRenderPart;
class CPolyline;
class IMaterial;
class CParticles;
class CParticleEffect;
struct SRenderStats;
class ITextureLoader;
struct SDepthOfField;
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SGroupSelect
{
	unsigned short nMaskAny, nMaskEvery;

	SGroupSelect( unsigned short _nMaskAny, unsigned short _nMaskEvery ): nMaskAny(_nMaskAny), nMaskEvery(_nMaskEvery) {}
	SGroupSelect() {}
};
inline bool operator!=( const SGroupSelect &a, const SGroupSelect &b ) { return a.nMaskAny != b.nMaskAny || a.nMaskEvery != b.nMaskEvery; }

const int N_MASK_TREECROWN = 0x1000;
const int N_MASK_CAST_SHADOW = 0x2000;

const int N_MASK_OPAQUE = 0x100;
const int N_MASK_IGNOREZ = 0x200;

const int N_MASK_LOD_LOW = 0x400;
const int N_MASK_LOD_HIGH = 0x800;
const int N_MASK_LOD = 0xc00;

const int N_MASK_FLOORS = 0xfff;


inline SGroupSelect MakeSelectAll() { return SGroupSelect( N_MASK_FLOORS, 0 ); }
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SGroupInfo
{
	unsigned short nLightFlags, nObjectGroup;
	//
	SGroupInfo(): nLightFlags(0), nObjectGroup(0xffff) {}
	SGroupInfo( int _nLG, int _nOG ): nLightFlags(_nLG), nObjectGroup(_nOG) {}
	bool operator==( const SGroupInfo &a ) const { return a.nLightFlags == nLightFlags && a.nObjectGroup == nObjectGroup; }
	bool IsMaskMatch( const SGroupSelect &m ) const { return ( nObjectGroup & m.nMaskAny ) != 0 && ( nObjectGroup & m.nMaskEvery ) == m.nMaskEvery; }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
const int N_USERID_MASK = 0xffffff; // these bits do not depend on part number
struct SFullGroupInfo
{
	ZDATA
	SGroupInfo groupInfo;
	CPtr<CObjectBase> pUser;
	int nUserID;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&groupInfo); f.Add(3,&pUser); f.Add(4,&nUserID); return 0; }
	SFullGroupInfo() : nUserID(0) {}
	SFullGroupInfo( const SGroupInfo &_g, CObjectBase *_p, int _nUserID ) : groupInfo(_g), pUser(_p), nUserID(_nUserID) {}
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class CAnimLight: public CObjectBase
{
	OBJECT_BASIC_METHODS(CAnimLight);
public:
	CVec3 position;
	CVec3 color;
	float fRadius;
	bool bActive;
	bool bEnd;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class CSunFlares: public CObjectBase
{
	OBJECT_BASIC_METHODS(CSunFlares)
public:
	struct SFlare
	{
		ZDATA
		bool bFade;
		float fScale;
		float fDistance;
		CDGPtr<CPtrFuncBase<NGfx::CTexture> > pFlare;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&bFade); f.Add(3,&fScale); f.Add(4,&fDistance); f.Add(5,&pFlare); return 0; }
	};
	////
	ZDATA
	vector<SFlare> flares;
	CDGPtr<CPtrFuncBase<NGfx::CTexture> > pOverbright;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&flares); f.Add(3,&pOverbright); return 0; }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
enum EParticleFlags
{
//	PF_NOSHADOWS = 0,
//	PF_CAST_SHADOW = 1,
	PF_SELF_ILLUM = 0,
	PF_LIT = 2,
	PF_STATIC = 0,
	PF_DYNAMIC = 4
};
enum ELightingOptions
{
	LO_NOSHADOWS = 1
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SRenderGeometryInfo;
struct SPostProcessData;
class IPostProcess : public CObjectBase
{
public:
	struct SObject
	{
		SRenderGeometryInfo *pInfo;
		int nIdx;

		SObject( SRenderGeometryInfo *_pInfo, int _nIdx ) : pInfo(_pInfo), nIdx(_nIdx) {}
	};
	virtual void Render( SPostProcessData *pDst, const vector<SObject> &render ) = 0;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
// hintBV specifies area where object will be moving, if area is not limited just specify very large BV
struct SDecalMappingInfo;
class CDecalTarget;
interface IGScene : virtual public CObjectBase
{
public:
	virtual CObjectBase* CreateGeometry( CPtrFuncBase<CObjectInfo> *pInfo, IMaterial *pMat, 
		const SFullGroupInfo &_ginfo ) = 0;
	virtual CObjectBase* CreateGeometry( CPtrFuncBase<CObjectInfo> *pInfo, IMaterial *pMat,
		const SFBTransform &trans, const SFullGroupInfo &_ginfo ) = 0;
	virtual CObjectBase* CreateGeometry( CPtrFuncBase<CObjectInfo> *pInfo, IMaterial *pMat, 
		CFuncBase<SFBTransform> *pPlacement, const SBound &hintBV, const SFullGroupInfo &_ginfo ) = 0;
	virtual CObjectBase* CreateGeometry( CPtrFuncBase<CObjectInfo> *pInfo, IMaterial *pMat, 
		CFuncBase<SFBTransform> *pPlacement, CFuncBase<SBound> *_pBound, const SBound &hintBV, const SFullGroupInfo &_ginfo ) = 0;
	virtual CObjectBase* CreateGeometry( CPtrFuncBase<CObjectInfo> *pInfo, IMaterial *pMat, 
		CFuncBase<vector<SHMatrix> > *pPlacement, CFuncBase<vector<NGfx::SCompactTransformer> > *_pMMXAnim, 
		CFuncBase<SBound> *_pBound, const SBound &hintBV, const SFullGroupInfo &_ginfo ) = 0;
	virtual CObjectBase* CreateGeometry( CPtrFuncBase<CObjectInfo> *pInfo, IMaterial *pMat, 
		CFuncBase<vector<SHMatrix> > *pPlacement, CFuncBase<vector<NGfx::SCompactTransformer> > *_pMMXAnim, 
		const SBound &_bv, const SFullGroupInfo &_ginfo ) = 0;
	virtual CObjectBase* CreateDynamicGeometry( CPtrFuncBase<CObjectInfo> *pInfo, CFuncBase<SFBTransform> *pPlacement, IMaterial *pMat, 
		CFuncBase<SBound> *pBound, const SBound &hintBV, const SFullGroupInfo &_ginfo ) = 0;
	virtual CObjectBase* CreateParticles( CPtrFuncBase<CParticleEffect> *pInfo,
		CFuncBase<SFBTransform> *pPlacement, const SBound &bound, const SBound &hintBV, const SGroupInfo &_ginfo, int nPFlags ) = 0;
	virtual CPolyline* CreatePolyline( CPtrFuncBase<NGfx::CGeometry> *pGeometry, const vector<unsigned short> &indices, 
		const CVec4 &color, bool bDepthTest ) = 0;
	virtual CObjectBase* CreatePostProcessor( CObjectBase *pRenderNode, IPostProcess *pProcessor ) = 0;
	virtual void SetAmbientAnimation( CPtrFuncBase<CAnimLight> *pLight ) = 0;
	virtual void SetSunFlares( const CVec3 &dir, CSunFlares *pSunFlares, CFuncBase<STime>* pSunFlaresTime ) = 0;
	virtual void SetWarFogBlend( float fBlend ) = 0;
	virtual void SetDirectionalLight( CFuncBase<CVec3> *pColor, CFuncBase<CVec3> *pGlossColor, 
		const CVec3 &_vLightDir, const CVec3 &_vShadowsLightDir,
		float fMaxHeight, float fShadowsMaxDetailLength, float fBlurShift, 
		CFuncBase<CVec3> *_pAmbient, CFuncBase<CVec3> *_pShadeColor, CFuncBase<CVec3> *_pIncidentShadeColor, const CVec3 &vParticlesColor,
		CPtrFuncBase<NGfx::CTexture> *_pClouds, CFuncBase<SHMatrix> *_pCloudsProjection, const CVec3 &vDymanicLightsModifications ) = 0;
	virtual CObjectBase* AddPointLight( const CVec3 &_vColor, const CVec3 &ptOrigin, float fR ) = 0;
	virtual CObjectBase* AddPointLight( CPtrFuncBase<CAnimLight> *pLight ) = 0;
	virtual CObjectBase* AddSpotLight( CFuncBase<CVec3> *pColor, const CVec3 &ptOrigin, const CVec3 &ptDir, float fFOV, 
		float fRadius, CPtrFuncBase<NGfx::CTexture> *pMask, bool bLightmapOnly ) = 0;
	virtual CDecalTarget* CreateDecalTarget( const vector<CObjectBase*> &targets, const SDecalMappingInfo &_info ) = 0;
	virtual CObjectBase* AddDecal( NGScene::CDecalTarget *pTarget, IMaterial *pMaterial ) = 0;
	virtual void SetWarFog( const CArray2D<unsigned char> &fog, float fScale ) = 0;
	virtual void LoadEverything() = 0;
	// nLightOptions - LO_NOSHADOWS
	virtual void Draw( CTransformStack *pTS, CTransformStack *pClipTS, NGfx::CRenderContext *pRC, const SGroupSelect &mask,
		ERenderPath rp, const SRTClearParams &rtClear, EHSRMode hsrMode,
		ETransparentMode trMode, NGfx::CCubeTexture *pSky, SDepthOfField *pDOF, int nLightOptions ) = 0;
	virtual CFuncBase<CVec4>* GetCamera() = 0;
	virtual CVec2 GetScreenRect() = 0;
	virtual bool TraceScene( const SGroupSelect &mask, const CRay &r, float *pfT, CVec3 *pNormal, EScenePartsSet ps = SPS_STATIC, SFullGroupInfo *pGroupInfo = 0, CObjectBase **ppPart = 0 ) = 0;
	virtual SGroupSelect GetLastMask() = 0;
	virtual int PrecacheMaterials( ILoadingCounter *pCounter ) = 0;
	virtual void WaitForLoad( bool bWait = true ) = 0;
	virtual void SetTwilight( bool _bIsTwilight ) = 0;
	virtual CFuncBase<CVec3> *GetParticlesLightColor() = 0;
	virtual void CollectAllParts( vector<CObjectBase*> *pRes ) = 0;

	virtual void SetLoadingCounter( ILoadingCounter *pCounter ) = 0;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
IGScene* CreateScene();
// wrappers to Gfx to exclude interface on Gfx dependency
_3DMOTOR_EXPORT bool Is3DActive();
_3DMOTOR_EXPORT void SetWireframe( bool bWire );
bool GetWireframe();
_3DMOTOR_EXPORT void ClearScreen( const CVec3 &vColor );
enum ERegisterCopyMode
{
	RCM_COPY,
	RCM_TRANSPARENT,
	RCM_SHOWALPHA
};
void CopyRegisterOnScreen( const CTRect<float> &rScreenRect, ERegisterCopyMode mode, int nRegister );
_3DMOTOR_EXPORT void GetRenderStats( SRenderStats *pStats );
_3DMOTOR_EXPORT float GetFrameTime();
_3DMOTOR_EXPORT void Flip();
_3DMOTOR_EXPORT int CalcTouchedTextureSize();
bool GetGeometryObjectInfo( CObjectBase *p, 
	CPtrFuncBase<CObjectInfo> **pGeometry, SFBTransform *pPos, SFullGroupInfo *pGroupInfo );
CFuncBase<vector<NGfx::SCompactTransformer> >* MakeMMXAnimation( CFuncBase<vector<SHMatrix> > *pAnim );
void SetPartFade( CObjectBase *p, float fFade );
void SetPartPriority( CObjectBase *_p, int _nPriority );
void SetPartLM( CObjectBase *p, CPtrFuncBase<NGfx::CTexture> *pLM );//CFuncBase<CArray2D<NGfx::SPixel8888> > *pLM )
CPtrFuncBase<CParticleEffect> *GetParticleAnimator( CObjectBase *p );
class CLightmapsHolder;
class CLightmapsLoader;
class CLightmapsTempHolder; 
enum ELightmapQuality;
CLightmapsTempHolder *CreateLightmapsTempHolder();
CLightmapsHolder *FinalMergeLightmaps( CLightmapsTempHolder *pTmpHolder );
CLightmapsHolder *CalcLightmaps( IGScene *pScene, CObjectBase *pUser, int nUserID, const SSphere &highResLM, ELightmapQuality quality, CLightmapsTempHolder *pTmpHolder );
void ApplyLightmaps( IGScene *pScene, CObjectBase *pUser, CLightmapsHolder *pLightmaps,  CLightmapsLoader * pLD  );
////////////////////////////////////////////////////////////////////////////////////////////////////
}
