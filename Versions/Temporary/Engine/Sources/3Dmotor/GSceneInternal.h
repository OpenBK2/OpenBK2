#pragma once
#include "GDecal.h"
#include "GParts.h"
namespace NDb
{
	struct SMaterial;
}

namespace NGScene
{

struct SDepthOfField;

//! bind post process effect to target geometry
class CPostProcessBinder : public CObjectBase
{
	OBJECT_BASIC_METHODS( CPostProcessBinder );
private:
	ZDATA
	CPtr<ISomePart> pTarget;
	CPtr<IPostProcess> pPostProcess;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTarget); f.Add(3,&pPostProcess); return 0; }
public:
	CPostProcessBinder() {}
	bool Initialize( CObjectBase *p, IPostProcess *pPost );
	void Store( vector<IPostProcess::SObject> *pRes, CTransformStack *pTS, const SGroupSelect &mask );
	bool Update( IGScene *pScene ) { return IsValid( pTarget ); }
	IPostProcess* GetPostProcessor() const { return pPostProcess; }
};

class CPolyline: public CObjectBase
{
	OBJECT_BASIC_METHODS(CPolyline);
	ZDATA
	CDGPtr< CPtrFuncBase<NGfx::CGeometry> > pGeometry;
	vector<unsigned short> indices;
	CVec4 color;
	bool bCheckDepth;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pGeometry); f.Add(3,&indices); f.Add(4,&color); f.Add(5,&bCheckDepth); return 0; }
public:
	CPolyline() {}
	CPolyline( CPtrFuncBase<NGfx::CGeometry> *_pGeometry, const vector<unsigned short> &_indices,
		const CVec4 &_color, bool _bCheckDepth );
	//
	bool GetCheckDepth() const { return bCheckDepth; }
	const CVec4& GetColor() const { return color; } 
	void Render( NGfx::CRenderContext *pRC );
};

struct SRayInfo
{
	CVec3 vOrigin, vDir, vDirOrt;
	float fLength;

	SRayInfo( const CRay &r ) 
		: vOrigin( r.ptOrigin ), vDir( r.ptDir ), vDirOrt( r.ptDir ), fLength( fabs( r.ptDir ) )
	{
		Normalize( &vDirOrt );
	}
};

class CFakeParticleLMTexture : public CPtrFuncBase<NGfx::CTexture>
{
	OBJECT_NOCOPY_METHODS(CFakeParticleLMTexture);
	ZDATA
	CDGPtr<CFuncBase<CVec3> > pAmbient;
	CDGPtr<CFuncBase<CVec3> > pColor;
	DWORD dwNormalColor, dwParticleColor;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pAmbient); f.Add(3,&pColor); f.Add(4,&dwNormalColor); f.Add(5,&dwParticleColor); return 0; }
	bool NeedUpdate() 
	{ 
		bool b1 = pColor.Refresh();
		bool b2 = pAmbient.Refresh();
		return b1 || b2;
	}
	void Recalc();
public:
	void SetAmbient( CFuncBase<CVec3> *_pAmbient ) { pAmbient = _pAmbient; Updated(); }
	void SetColor( CFuncBase<CVec3> *_pColor ) { pColor = _pColor; }
	DWORD GetNormalColor() const { return dwNormalColor; }
	DWORD GetParticleColor() const { return dwParticleColor; }
};

class CDecalsManager;
class CAmbientMean;
class CAmbientAnimator;
class ILight;
class CDirectionalLight;
class CCVec4;
class CLightStateNode;
class CTransparentRenderer;
class CGScene: public IGScene, public IDecalQuery
{
	OBJECT_BASIC_METHODS(CGScene);
	enum ERLRequest
	{
		RN_STATIC = 1,
		RN_DYNAMIC = 2,
		RN_ALL = 3,
		RN_LIGHTMAPS = 4,
		RN_DEPTH = 8,
		RN_LIT_PARTICLES = 16,
	};

	ZDATA
	SFullStaticTrackers trackers;
	CObj<CVolumeNode> pVolume;
	list< CPtr<CPolyline> > lines;
	ESceneRenderMode renderMode;
	CObj<CCVec4> pCamera;
	SHMatrix mHoldTransform;
	int nCurrentIgnoreMark;
	int nIgnoreListWasCalced;
	SGroupSelect holdMask;
	CObj<CFuncBase<CVec3> > pAmbient;
	CObj<IMaterial> pTransparentMaterial;
	CDGPtr<CFakeParticleLMTexture> pFakeParticleLM;
	int nFrameCounter;
	list< CPtr<CPostProcessBinder> > postprocessors;
	list< CPtr<ISomePart> > toBeLoaded;
	CObj<CDecalsManager> pDecalsManager;
	CObj<CAmbientAnimator> pAmbientAnimator;
	//CObj<CDirectionalLight> pDirectionalLight;
	CObj<CLightStateNode> pLightState;
	list< CPtr<CStaticAnimatedPart> > toBeLoadedAnimated;
	CObj<CSunFlares> pSunFlares;
	CDGPtr<CFuncBase<STime> > pSunFlaresTime;
	STime sSunFlareTime;
	float fSunFlareCoeff;
	CVec3 sunFlareDir;
	bool bIsTwilight;
	CObj<CCVec3> pParticlesLightColor;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&trackers); f.Add(3,&pVolume); f.Add(4,&lines); f.Add(5,&renderMode); f.Add(6,&pCamera); f.Add(7,&mHoldTransform); f.Add(8,&nCurrentIgnoreMark); f.Add(9,&nIgnoreListWasCalced); f.Add(10,&holdMask); f.Add(11,&pAmbient); f.Add(12,&pTransparentMaterial); f.Add(13,&pFakeParticleLM); f.Add(14,&nFrameCounter); f.Add(15,&postprocessors); f.Add(16,&toBeLoaded); f.Add(17,&pDecalsManager); f.Add(18,&pAmbientAnimator); f.Add(19,&pLightState); f.Add(20,&toBeLoadedAnimated); f.Add(21,&pSunFlares); f.Add(22,&pSunFlaresTime); f.Add(23,&sSunFlareTime); f.Add(24,&fSunFlareCoeff); f.Add(25,&sunFlareDir); f.Add(26,&bIsTwilight); f.Add(27,&pParticlesLightColor); return 0; }
	int nSlowVolumeWalk;
	SParticleLMRenderTargetInfo particleLM;
	SGroupSelect lastMask;
	CObj<IHZBuffer> pHZBuffer;
	bool bWaitForLoad;
	//! if true lightmap catch up will recalc all depth textures
	//bool bFirstLMCatch;
	int nReuseIgnoreList;
	int nGfxDeviceCreationID;
	CObj<ILoadingCounter> pLoadingCounter;

	struct SSceneFragmentGroupInfo
	{
		SGroupSelect mask;
		CTransparentRenderer *pTransp;
		int nLMTextureUsed;
		IHZBuffer *pHZBuffer;
		CSceneFragments *pList;
	public:
		SSceneFragmentGroupInfo( const SGroupSelect &_mask, CSceneFragments *_pList, CTransparentRenderer *_pTransp );
		SSceneFragmentGroupInfo( const SGroupSelect &_mask, CSceneFragments *_pList, CTransparentRenderer *_pTransp,	IHZBuffer *_pHZBuffer );
	private:
		void FilterParts( vector<CPartFlags> *pRes, CTransformStack *pTS, CCombinedPart *p, ERLRequest req, int _nIgnoreMark );
		void AddTranspElement( CCombinedPart *p, const vector<CPartFlags> &flags );
		void AddDynamicLMElement( CCombinedPart *p, CTransformStack *pTS, ERLRequest req, int _nIgnoreMark );
		void AddStaticLMElement( CCombinedPart *p, CTransformStack *pTS, ERLRequest req, int _nIgnoreMark );
		void AddElement( CSceneFragments *pRes, CTransformStack *pTS, CCombinedPart *p, ERLRequest req, int _nIgnoreMark );
	public:
		void AddMaterialHolder( CTransformStack *pTS, const CVolumeNode::SPerMaterialHolder &h, ERLRequest req, int _nIgnoreMark );
	};

	void SelectNodes( CTransformStack *pTS, CVolumeNode *pNode, vector<CVolumeNode*> *pRes );
	void MakePartList( CTransformStack *pTS, list<SRenderPartSet> *pRes, ERLRequest req, const SGroupSelect &mask );
	void MakeRenderList( CTransformStack *pTS, SSceneFragmentGroupInfo *pFragmentsInfo, ERLRequest req, int nIgnoreMark );
	void MakePolycountRenderList( CTransformStack *pTS, CSceneFragments *pList );
	void RecalcRenderStats( int nSceneTris, int nParticles, int nLitParticles );
	void WalkOctree();
	//void UpdateIgnoreMark( IRender *pRender, CTransformStack *pTS, const SGroupSelect &mask, EHSRMode hsrMode );
	//bool NeedUseHWHSR() const;
	bool TraceParts( ERLRequest req, const SGroupSelect &mask, CVolumeNode *pNode, const SRayInfo &r, float *pfT, CVec3 *pNormal, SFullGroupInfo *pGroupInfo, CObjectBase **ppPart );
	void DrawPostProcess( CTransformStack *pTS, NGfx::CRenderContext *pRC, const SGroupSelect &mask );
	void DrawSunFlares( CTransformStack *pTS, NGfx::CRenderContext *pRC );
	void DrawLines( NGfx::CRenderContext *pRC );
	void RefreshParticleLMTarget();
	void WalkNotLoadedObjects();
	int PrecacheMaterials( CVolumeNode *pNode, ILoadingCounter *pCounter );
	// IDecalQuery
	CObjectBase* CreateDecal( ISomePart *pTarget, const vector<CVec3> &srcPositions, const SDecalMappingInfo &_info, IMaterial *pMaterial );
	CObjectBase* CreateStaticDecal( ISomePart *pTarget, CPtrFuncBase<CObjectInfo> *pDecal, IMaterial *pMaterial, const SFullGroupInfo &fg );
	CObjectBase* CreateDynamicDecal( ISomePart *pTarget, CPtrFuncBase<CObjectInfo> *pDecal, IMaterial *pMaterial, const SFullGroupInfo &fg );
	void GetPartsList( const SDecalMappingInfo &_info, const CObjectBaseSet &targets, vector<CPtr<ISomePart> > *pRes );
public:
	CGScene();
	CGScene( int );
	// outer space integration
	virtual CObjectBase* CreateGeometry( CPtrFuncBase<CObjectInfo> *pInfo, IMaterial *pMat, 
		const SFullGroupInfo &_ginfo );
	virtual CObjectBase* CreateGeometry( CPtrFuncBase<CObjectInfo> *pInfo, IMaterial *pMat,
		const SFBTransform &trans, const SFullGroupInfo &_ginfo );
	virtual CObjectBase* CreateGeometry( CPtrFuncBase<CObjectInfo> *pInfo, IMaterial *pMat, 
		CFuncBase<SFBTransform> *pPlacement, const SBound &hintBV, const SFullGroupInfo &_ginfo );
	virtual CObjectBase* CreateGeometry( CPtrFuncBase<CObjectInfo> *pInfo, IMaterial *pMat, 
		CFuncBase<SFBTransform> *pPlacement, CFuncBase<SBound> *_pBound, const SBound &hintBV, const SFullGroupInfo &_ginfo );
	virtual CObjectBase* CreateGeometry( CPtrFuncBase<CObjectInfo> *pInfo, IMaterial *pMat, 
		CFuncBase<vector<SHMatrix> > *pPlacement, CFuncBase<vector<NGfx::SCompactTransformer> > *_pMMXAnim, 
		CFuncBase<SBound> *_pBound, const SBound &hintBV, const SFullGroupInfo &_ginfo );
	virtual CObjectBase* CreateGeometry( CPtrFuncBase<CObjectInfo> *pInfo, IMaterial *pMat, 
		CFuncBase<vector<SHMatrix> > *pPlacement, CFuncBase<vector<NGfx::SCompactTransformer> > *_pMMXAnim, 
		const SBound &_bv, const SFullGroupInfo &_ginfo );
	virtual CObjectBase* CreateDynamicGeometry( CPtrFuncBase<CObjectInfo> *pInfo, CFuncBase<SFBTransform> *pPlacement, IMaterial *pMat, 
		CFuncBase<SBound> *pBound, const SBound &hintBV, const SFullGroupInfo &_ginfo );
	//
	virtual CObjectBase* CreateParticles( CPtrFuncBase<CParticleEffect> *pInfo,
		CFuncBase<SFBTransform> *pPlacement, const SBound &bound, const SBound &hintBV, const SGroupInfo &_ginfo, int nPFlags );
	virtual CPolyline* CreatePolyline( CPtrFuncBase<NGfx::CGeometry> *pGeometry, const vector<unsigned short> &indices, 
		const CVec4 &color, bool bDepthTest );
	virtual CObjectBase* CreatePostProcessor( CObjectBase *pRenderNode, IPostProcess *pProcessor );
	virtual void SetAmbientAnimation( CPtrFuncBase<CAnimLight> *pLight );
	virtual void SetSunFlares( const CVec3 &_dir, CSunFlares *_pSunFlares, CFuncBase<STime>* _pSunFlaresTime ) { sunFlareDir = _dir; pSunFlares = _pSunFlares; pSunFlaresTime = _pSunFlaresTime; }
	virtual void SetDirectionalLight( CFuncBase<CVec3> *pColor, CFuncBase<CVec3> *pGlossColor, 
		const CVec3 &_vLightDir, const CVec3 &_vShadowsLightDir, float fMaxHeight, float fShadowsMaxDetailLength, float fBlurShift, 
		CFuncBase<CVec3> *_pAmbient, CFuncBase<CVec3> *_pShadeColor, CFuncBase<CVec3> *_pIncidentShadeColor, const CVec3 &vParticlesColor,
		CPtrFuncBase<NGfx::CTexture> *_pClouds, CFuncBase<SHMatrix> *_pCloudsProjection, const CVec3 &vDymanicLightsModifications );
	virtual CObjectBase* AddPointLight( const CVec3 &_vColor, const CVec3 &ptOrigin, float fR );
	virtual CObjectBase* AddPointLight( CPtrFuncBase<CAnimLight> *pLight );
	virtual CObjectBase* AddSpotLight( CFuncBase<CVec3> *pColor, const CVec3 &ptOrigin, const CVec3 &ptDir, float fFOV, float fRadius, CPtrFuncBase<NGfx::CTexture> *pMask, bool bLightmapOnly );
	virtual CDecalTarget* CreateDecalTarget( const vector<CObjectBase*> &targets, const SDecalMappingInfo &_info );
	virtual CObjectBase* AddDecal( NGScene::CDecalTarget *pTarget, IMaterial *pMaterial );
	virtual void SetWarFogBlend( float fBlend );
	virtual void SetWarFog( const CArray2D<unsigned char> &fog, float fScale );
	void RenderPostProcess( CTransformStack *pTS, NGfx::CRenderContext *pRC );
	virtual void LoadEverything();
	virtual void Draw( CTransformStack *pTS, CTransformStack *pClipTS, NGfx::CRenderContext *pRC, const SGroupSelect &mask,
		ERenderPath rp, const SRTClearParams &rtClear, EHSRMode hsrMode,
		ETransparentMode trMode, NGfx::CCubeTexture *pSky, SDepthOfField *pDOF, int nLightOptions );
	virtual CVec2 GetScreenRect();
	virtual CFuncBase<CVec4>* GetCamera();
	virtual bool TraceScene( const SGroupSelect &mask, const CRay &r, float *pfT, CVec3 *pNormal, EScenePartsSet ps, SFullGroupInfo *pGroupInfo, CObjectBase **ppPart );
	virtual SGroupSelect GetLastMask() { return lastMask; }
	virtual int PrecacheMaterials( ILoadingCounter *pCounter );
	virtual void SetLoadingCounter( ILoadingCounter *pCounter ) { pLoadingCounter = pCounter; }
	virtual void WaitForLoad( bool bWait = true ) { bWaitForLoad = bWait; }
	CFuncBase<SPerVertexLightState> *GetLightState() const;
	void GetNotLoaded( vector<IPart*> *pRes );
	virtual void SetTwilight( bool _bIsTwilight ) { bIsTwilight = _bIsTwilight; }
	virtual CFuncBase<CVec3> *GetParticlesLightColor() { return pParticlesLightColor; }
	virtual void CollectAllParts( vector<CObjectBase*> *pRes );
	CTransparentRenderer *CreateTransparentRenderer( CTransformStack *pTS, bool bLitParticles );

	friend class CRenderWrapper;
};

class CRenderWrapper: public IRender
{
	CGScene *pScene;
public:
	CRenderWrapper( CGScene *_pScene ): pScene(_pScene) {}

	virtual CFuncBase<SPerVertexLightState> *GetLightState() const { return pScene->GetLightState(); }
	virtual void FormPartList( CTransformStack *pTS, list<SRenderPartSet> *pRes, EDepthType dt, const SGroupSelect &mask );
	virtual void FormDepthList( CTransformStack *pTS, const CVec3 &vDir, EDepthType dt, CSceneFragments *pRes );
	virtual void FormRenderList( CTransformStack *pTS, CSceneFragments *pRes, CTransparentRenderer *pTransparentRender );
	virtual void GetNotLoaded( vector<IPart*> *pRes ) { pScene->GetNotLoaded( pRes ); }
	virtual void RenderPostProcess( CTransformStack *pTS, NGfx::CRenderContext *pRC ) { pScene->RenderPostProcess( pTS, pRC ); }
	virtual CTransparentRenderer *CreateTransparentRenderer( CTransformStack *pTS, bool bLitParticles );
};

} // namespace


