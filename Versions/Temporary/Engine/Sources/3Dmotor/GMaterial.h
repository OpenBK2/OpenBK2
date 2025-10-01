#pragma once

#include "3Dmotor_export.h"

#include "GRenderCore.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NGScene
{
////////////////////////////////////////////////////////////////////////////////////////////////////
class CGenericMaterial : public IMaterial
{
	OBJECT_BASIC_METHODS( CGenericMaterial );
	enum EMaterialType
	{
		NORMAL,
		DECAL,
		EXACT_DECAL,
		DECAL_ZWRITE,
		MT_TRANSPARENT
	};
	enum ERenderOps
	{
		RO_G3_DIFFUSE_SPEC_LM = RO_USER,
		RO_G3_DIFFUSE_SPEC,
		RO_G3_DIFFUSE_TEX_LM,
		RO_G3_DIFFUSE_TEX_DETAIL,
		RO_G3_DIFFUSE_TEX_LM_DETAIL,
		RO_G3_DIFFUSE_TEX,
		RO_G3_DIFFUSE_TEX_MIRROR,
		RO_GLOSSED_MIRROR,
		RO_G3_TRASPARENT
	};
	ZDATA
	bool bDoesCastShadow;
	CVec4 vAvrgTexColor;
	CDGPtr<CFuncBase<CVec4> > pDiffuseColor;
	CDGPtr<CPtrFuncBase<NGfx::CTexture> > pDiffuseTex;
	CDGPtr<CPtrFuncBase<NGfx::CTexture> > pDiffuseTex2;
	CDGPtr<CPtrFuncBase<NGfx::CTexture> > pDetailTex;
	// reflection props
	CDGPtr<CPtrFuncBase<NGfx::CCubeTexture> > pSkyTex;
	CDGPtr<CPtrFuncBase<NGfx::CTexture> > pMirrorTex;
	CObj<CGenericMaterial> pExactDecal;
	bool bSelfIllum;
	CDGPtr<CPtrFuncBase<NGfx::CTexture> > pSpecularTex;
	bool bProjectOnTerrain;
	EMaterialType mt;
	char nDecalDepthTest, nPriority;
	CVec4 vMirrorParam;
	bool bAlphaTest;
	float fSpecPower;
	CVec3 vTranslucentColor;
	float fDetailScale;
	bool b2Sided;
	bool bApplyFog;
	bool bAddPlaced;
	bool bIgnoreZ;
	bool bBackFaceCastShadow;
	CObj<CGenericMaterial> pWindAffected;
	bool bWindAffected;
	CObj<CGenericMaterial> pNoReceiveShadows;
	bool bReceiveShadows;
	float fAngVel;
	EDynamicType eType;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&bDoesCastShadow); f.Add(3,&vAvrgTexColor); f.Add(4,&pDiffuseColor); f.Add(5,&pDiffuseTex); f.Add(6,&pDiffuseTex2); f.Add(7,&pDetailTex); f.Add(8,&pSkyTex); f.Add(9,&pMirrorTex); f.Add(10,&pExactDecal); f.Add(11,&bSelfIllum); f.Add(12,&pSpecularTex); f.Add(13,&bProjectOnTerrain); f.Add(14,&mt); f.Add(15,&nDecalDepthTest); f.Add(16,&nPriority); f.Add(17,&vMirrorParam); f.Add(18,&bAlphaTest); f.Add(19,&fSpecPower); f.Add(20,&vTranslucentColor); f.Add(21,&fDetailScale); f.Add(22,&b2Sided); f.Add(23,&bApplyFog); f.Add(24,&bAddPlaced); f.Add(25,&bIgnoreZ); f.Add(26,&bBackFaceCastShadow); f.Add(27,&pWindAffected); f.Add(28,&bWindAffected); f.Add(29,&pNoReceiveShadows); f.Add(30,&bReceiveShadows); f.Add(31,&fAngVel); f.Add(32,&eType); return 0; }

	void SetMT( EMaterialType _mt )
	{
		mt = _mt;
		nDecalDepthTest = 0;
		if ( mt == EXACT_DECAL )
			nDecalDepthTest = DPM_EQUAL;
		else if ( mt == DECAL_ZWRITE )
			nDecalDepthTest = DPM_NORMAL;
		else
			nDecalDepthTest = DPM_TESTONLY;
	}
	char GetDecalDepthTest() const { return nDecalDepthTest; }
	bool SetRenderMode( NGfx::CRenderContext *pRC, const SLightInfo &lightInfo, int nROP, CRenderCmdList::UParameter p1, CRenderCmdList::UParameter p2 );
public:
	CGenericMaterial( bool _bDoesCastShadow = true, bool _bBackFaceCastShadow = false )
		: bDoesCastShadow(_bDoesCastShadow), bSelfIllum(false), bProjectOnTerrain(false), vAvrgTexColor(1,1,1,1),
		mt(NORMAL), nPriority(0), nDecalDepthTest(0), vMirrorParam(0,0,0,0), bAlphaTest(false), fSpecPower(0),
		vTranslucentColor(0,0,0),fDetailScale(1), b2Sided(false), bApplyFog(true), bAddPlaced(false), bIgnoreZ(false), bBackFaceCastShadow(_bBackFaceCastShadow), bWindAffected(false), bReceiveShadows(true), fAngVel(0)  {}
	virtual bool DoesCastShadow() const { return bDoesCastShadow; }

	virtual bool IsOpaque() const { return !IsAlphaTest() && mt == NORMAL; }
	virtual bool IsAlphaTest() const { return bAlphaTest; }
	virtual NGfx::CTexture *GetAlphaTestTex();
	virtual bool IsTransparent() const { return mt == MT_TRANSPARENT; }
	virtual void AddATOperations( COpGenContext *p, const SPerspDirectionalDepthInfo *pDepthInfo );
	virtual void AddOperations( COpGenContext *pOp, SRenderPathContext *pRPC );
	virtual void AddCustomOperation( COpGenContext *pOp, const ERenderOperation &op, unsigned char nPass,
		int nSBM, char nDestRegister, CRenderCmdList::UParameter p = CRenderCmdList::UParameter() );
	virtual void SetTransparentRenderMode( NGfx::CRenderContext *pRC, const SPerPartVariables &vars, const SLightInfo &lightInfo, SRenderPathContext *pRPC );
	virtual bool Is2Sided() const { return b2Sided; }
	virtual bool IsSelfIllum() const { return bSelfIllum; }
	virtual void Precache();
	virtual CVec3 GetTranslucentColor() const { return vTranslucentColor; }
	virtual bool IsProjectOnTerrain() const { return bProjectOnTerrain; }
	virtual bool DoesSupportLightmaps() const { return true; }
	virtual bool DoesIgnoreZ() const { return bIgnoreZ; }
	virtual bool DoesBackFaceCastShadow() const { return bBackFaceCastShadow; }
	EDynamicType GetDynamicType(){ return eType; };
	void SetDynamicType( EDynamicType _eType ){ eType = _eType; };
	void SetAlphaTest( bool _b ) { bAlphaTest = _b; }
	void SetDiffuseColor( CFuncBase<CVec4> *_p ) { pDiffuseColor = _p; }
	void SetDiffuseTex( CPtrFuncBase<NGfx::CTexture> *_p, const CVec4 &_vAvrgTexColor ) { vAvrgTexColor = _vAvrgTexColor; pDiffuseTex = _p; }
	void SetExactDecal() { SetMT( EXACT_DECAL ); bDoesCastShadow = false; }
	void SetDecal() { SetMT( DECAL ); bDoesCastShadow = false; }
	void SetDecalZWrite() { SetMT( DECAL_ZWRITE ); }
	void SetTransparent() { SetMT( MT_TRANSPARENT ); }
	void SetReflectionInfo( CPtrFuncBase<NGfx::CCubeTexture> *_pSky, CPtrFuncBase<NGfx::CTexture> *_pMirror,
		float _fDielMirror, float _fMetalMirror );
	void SetSelfIllum( bool _b ) { bSelfIllum = _b; }
	void SetDiffuseTex2( CPtrFuncBase<NGfx::CTexture> *_p ) { pDiffuseTex2 = _p; }
	void SetDetail( CPtrFuncBase<NGfx::CTexture> *_p, float _fDetailScale ) { pDetailTex = _p; fDetailScale = _fDetailScale; }
	void SetPriority( int _nPriority ) { nPriority = _nPriority; }
	void SetSpecular( CPtrFuncBase<NGfx::CTexture> *_pSpec, float _fPower ) { pSpecularTex = _pSpec; fSpecPower = _fPower; }
	void SetTranslucentColor( const CVec3 &vColor ) { vTranslucentColor = vColor; }
	void SetProjectOnTerrain( bool _bProjectOnTerrain ) { bProjectOnTerrain = _bProjectOnTerrain; }
	void SetAngVel( float _fVel ) { fAngVel = _fVel; }
	void Set2Sided( bool _b ) { b2Sided = _b; }
	void SetFogApplying( bool _bApplyFog ) { bApplyFog = _bApplyFog; }
	void SetAddPlacing( bool _bAddPlaced ) { bAddPlaced = _bAddPlaced; }
	void SetUsageZBuffer( bool _bIgnoreZ ) { bIgnoreZ = _bIgnoreZ ; }
	void SetReceiveShadows( bool _bReceiveShadows ) { bReceiveShadows = _bReceiveShadows; }
	void Check()
	{
		ASSERT( !bAlphaTest || pDiffuseTex );
	}
	IMaterial* GetExactDecal();
	virtual IMaterial* GetWindAffected();
	virtual IMaterial* GetNoReceiveShadows();
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class CWaterMaterial : public IMaterial
{
	OBJECT_BASIC_METHODS(CWaterMaterial);
	ZDATA
	CDGPtr<CPtrFuncBase<NGfx::CTexture> > pTex, pSecondTex;
	int nPriority;
	bool bApplyFog;
	bool bAddPlaced;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTex); f.Add(3,&pSecondTex); f.Add(4,&nPriority); f.Add(5,&bApplyFog); f.Add(6,&bAddPlaced); return 0; }
	enum ERenderOps
	{
		RO_BLEND2TEX_COLOR = RO_USER,
		RO_G3_DIFFUSE_TEX,
	};
	bool SetRenderMode( NGfx::CRenderContext *pRC, const SLightInfo &lightInfo, int nROP, CRenderCmdList::UParameter p1, CRenderCmdList::UParameter p2 );
public:
	CWaterMaterial() {}
	CWaterMaterial( CPtrFuncBase<NGfx::CTexture> *_pTex, CPtrFuncBase<NGfx::CTexture> *_pSecondTex, int _nPriority, bool _bApplyFog, bool _bAddPlaced )
		: pTex(_pTex), pSecondTex(_pSecondTex), nPriority(_nPriority), bApplyFog(_bApplyFog), bAddPlaced(_bAddPlaced) {}
	virtual bool DoesCastShadow() const { return false; }
	virtual bool IsOpaque() const { return false; }
	virtual bool IsAlphaTest() const { return false; }
	virtual bool IsTransparent() const { return false; }
	virtual void AddATOperations( COpGenContext *p, const SPerspDirectionalDepthInfo *pDepthInfo ) {}
	virtual void AddOperations( COpGenContext *pOp, SRenderPathContext *pRPC );
	virtual void AddCustomOperation( COpGenContext *pOp, const ERenderOperation &op, unsigned char nPass,
		int nSBM, char nDestRegister, CRenderCmdList::UParameter p = CRenderCmdList::UParameter() );
	virtual bool Is2Sided() const { return false; }
	virtual bool IsSelfIllum() const { return false; }
	virtual void Precache();
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class _3DMOTOR_EXPORT CAnimWaterMaterial : public IMaterial
{
	OBJECT_BASIC_METHODS(CAnimWaterMaterial);
	ZDATA
	CDGPtr<CPtrFuncBase<NGfx::CTexture> > pTex, pSecondTex;
	int nPriority;
	CDGPtr<CFuncBase<STime> > pTime;
	bool bProjectOnTerrain;
	int nNumFramesX, nNumFramesY;
	float fTexScaleX, fTexScaleY;
	bool bApplyFog;
	bool bAddPlaced;
	bool bDrawHorses;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTex); f.Add(3,&pSecondTex); f.Add(4,&nPriority); f.Add(5,&pTime); f.Add(6,&bProjectOnTerrain); f.Add(7,&nNumFramesX); f.Add(8,&nNumFramesY); f.Add(9,&fTexScaleX); f.Add(10,&fTexScaleY); f.Add(11,&bApplyFog); f.Add(12,&bAddPlaced); f.Add(13,&bDrawHorses); return 0; }
	bool SetRenderMode( NGfx::CRenderContext *pRC, const SLightInfo &lightInfo, int nROP, CRenderCmdList::UParameter p1, CRenderCmdList::UParameter p2 );
public:
	CAnimWaterMaterial() {}
	CAnimWaterMaterial( CPtrFuncBase<NGfx::CTexture> *_pTex, CPtrFuncBase<NGfx::CTexture> *_pSecondTex, int _nPriority,
		CFuncBase<STime> *_pTime, bool _bProjectOnTerrain, int _nNumFramesX, int _nNumFramesY, bool _bApplyFog, bool _bAddPlaced, bool _bDrawHorses );
	virtual bool DoesCastShadow() const { return false; }
	virtual bool IsOpaque() const { return false; }
	virtual bool IsAlphaTest() const { return false; }
	virtual bool IsTransparent() const { return false; }
	virtual void AddATOperations( COpGenContext *p, const SPerspDirectionalDepthInfo *pDepthInfo ) {}
	virtual void AddOperations( COpGenContext *pOp, SRenderPathContext *pRPC );
	virtual void AddCustomOperation( COpGenContext *pOp, const ERenderOperation &op, unsigned char nPass,
		int nSBM, char nDestRegister, CRenderCmdList::UParameter p = CRenderCmdList::UParameter() );
	virtual bool Is2Sided() const { return false; }
	virtual bool IsSelfIllum() const { return false; }
	virtual void Precache();
	virtual bool IsProjectOnTerrain() const { return bProjectOnTerrain; }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class CSurfMaterial : public IMaterial
{
	OBJECT_BASIC_METHODS(CSurfMaterial);
	ZDATA
	CDGPtr<CPtrFuncBase<NGfx::CTexture> > pTex, pSecondTex;
	int nPriority;
	CDGPtr<CFuncBase<STime> > pTime;
	bool bApplyFog;
	bool bAddPlaced;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTex); f.Add(3,&pSecondTex); f.Add(4,&nPriority); f.Add(5,&pTime); f.Add(6,&bApplyFog); f.Add(7,&bAddPlaced); return 0; }
	bool SetRenderMode( NGfx::CRenderContext *pRC, const SLightInfo &lightInfo, int nROP, CRenderCmdList::UParameter p1, CRenderCmdList::UParameter p2 );
public:
	CSurfMaterial() {}
	CSurfMaterial( CPtrFuncBase<NGfx::CTexture> *_pTex, CPtrFuncBase<NGfx::CTexture> *_pSecondTex, int _nPriority,
		CFuncBase<STime> *_pTime, bool _bApplyFog, bool _bAddPlaced )
		: pTex(_pTex), pSecondTex(_pSecondTex), nPriority(_nPriority), pTime(_pTime), bApplyFog(_bApplyFog), bAddPlaced(_bAddPlaced) {}
	virtual bool DoesCastShadow() const { return false; }
	virtual bool IsOpaque() const { return false; }
	virtual bool IsAlphaTest() const { return false; }
	virtual bool IsTransparent() const { return false; }
	virtual void AddATOperations( COpGenContext *p, const SPerspDirectionalDepthInfo *pDepthInfo ) {}
	virtual void AddOperations( COpGenContext *pOp, SRenderPathContext *pRPC );
	virtual void AddCustomOperation( COpGenContext *pOp, const ERenderOperation &op, unsigned char nPass,
		int nSBM, char nDestRegister, CRenderCmdList::UParameter p = CRenderCmdList::UParameter() );
	virtual bool Is2Sided() const { return false; }
	virtual bool IsSelfIllum() const { return false; }
	virtual void Precache();
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class CReflectWaterMaterial : public IMaterial
{
	enum ERenderOps
	{
		RO_WITHOUT_REFLECTION= RO_USER,
		RO_WITH_REFLECTION,
	};

	OBJECT_BASIC_METHODS(CReflectWaterMaterial);
	ZDATA
	CDGPtr<CPtrFuncBase<NGfx::CTexture> > pTex;
	int nPriority;
	bool bApplyFog;
	bool bAddPlaced;
	float fDielMirror;
	float fMetalMirror;
	CDGPtr<CPtrFuncBase<NGfx::CTexture> > pBump;
	CDGPtr<CPtrFuncBase<NGfx::CCubeTexture> > pSky;
	float fReflectionZ;

	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTex); f.Add(3,&nPriority); f.Add(4,&bApplyFog); f.Add(5,&bAddPlaced); f.Add(6,&fDielMirror); f.Add(7,&fMetalMirror); f.Add(8,&pBump); f.Add(9,&pSky);f.Add(10,&fReflectionZ);  return 0; }
	
	bool SetRenderMode( NGfx::CRenderContext *pRC, const SLightInfo &lightInfo, int nROP, CRenderCmdList::UParameter p1, CRenderCmdList::UParameter p2 );
public:
	CReflectWaterMaterial() {}
	CReflectWaterMaterial( CPtrFuncBase<NGfx::CTexture> *_pTex, CPtrFuncBase<NGfx::CTexture> *_pBump, int _nPriority, bool _bApplyFog, bool _bAddPlaced, float _fDielMirror,
		float _fMetalMirror, CPtrFuncBase<NGfx::CCubeTexture> *_pSky, float _fReflectionZ) : pTex(_pTex), pBump(_pBump), nPriority(_nPriority), bApplyFog(_bApplyFog), bAddPlaced(_bAddPlaced), fDielMirror(_fDielMirror), fMetalMirror(_fMetalMirror), pSky(_pSky), fReflectionZ(_fReflectionZ) {}
		virtual bool DoesCastShadow() const { return false; }
		virtual bool IsOpaque() const { return false; }
		virtual bool IsAlphaTest() const { return false; }
		virtual bool IsTransparent() const { return false; }
		virtual void AddATOperations( COpGenContext *p, const SPerspDirectionalDepthInfo *pDepthInfo ) {}
		virtual void AddOperations( COpGenContext *pOp, SRenderPathContext *pRPC );
		virtual void AddCustomOperation( COpGenContext *pOp, const ERenderOperation &op, unsigned char nPass,
			int nSBM, char nDestRegister, CRenderCmdList::UParameter p = CRenderCmdList::UParameter() );
		virtual bool Is2Sided() const { return false; }
		virtual bool IsSelfIllum() const { return false; }
		virtual void Precache();
		virtual float GetReflectionZ()const { return fReflectionZ; }

		virtual bool IsUsingWaterReflection() const { return true; }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class CTracksMaterial : public IMaterial
{
	OBJECT_BASIC_METHODS(CTracksMaterial);
	ZDATA
	CDGPtr<CPtrFuncBase<NGfx::CTexture> > pTex;
  int nPriority;
	bool bApplyFog;
	bool bAddPlaced;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTex); f.Add(3,&nPriority); f.Add(4,&bApplyFog); f.Add(5,&bAddPlaced); return 0; }
	bool SetRenderMode( NGfx::CRenderContext *pRC, const SLightInfo &lightInfo, int nROP, CRenderCmdList::UParameter p1, CRenderCmdList::UParameter p2 );
public:
	CTracksMaterial() {}
	CTracksMaterial( CPtrFuncBase<NGfx::CTexture> *_pTex, int _nPriority, bool _bApplyFog, bool _bAddPlaced ) : pTex(_pTex), bApplyFog(_bApplyFog), bAddPlaced(_bAddPlaced) {}
	virtual bool DoesCastShadow() const { return false; }
	virtual bool IsOpaque() const { return false; }
	virtual bool IsAlphaTest() const { return false; }
	virtual bool IsTransparent() const { return false; }
	virtual void AddATOperations( COpGenContext *p, const SPerspDirectionalDepthInfo *pDepthInfo ) {}
	virtual void AddOperations( COpGenContext *pOp, SRenderPathContext *pRPC );
	virtual void AddCustomOperation( COpGenContext *pOp, const ERenderOperation &op, unsigned char nPass,
		int nSBM, char nDestRegister, CRenderCmdList::UParameter p = CRenderCmdList::UParameter() );
	virtual bool Is2Sided() const { return false; }
	virtual bool IsSelfIllum() const { return false; }
	virtual void Precache();
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class CTerrainMaterial : public IMaterial
{
	OBJECT_BASIC_METHODS(CTerrainMaterial);
	ZDATA
	CDGPtr<CPtrFuncBase<NGfx::CTexture> > pTex, pMask;
	int nPriority;
	bool bApplyFog;
	bool bAddPlaced;
	bool bCastShadow;
	bool bBackFaceCastShadow;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTex); f.Add(3,&pMask); f.Add(4,&nPriority); f.Add(5,&bApplyFog); f.Add(6,&bAddPlaced); f.Add(7,&bCastShadow); f.Add(8,&bBackFaceCastShadow); return 0; }
	enum ERenderOps
	{
		RO_TERRAIN = RO_USER,
		RO_TERRAIN_GF2,
	};
	bool SetRenderMode( NGfx::CRenderContext *pRC, const SLightInfo &lightInfo, int nROP, CRenderCmdList::UParameter p1, CRenderCmdList::UParameter p2 );
public:
	CTerrainMaterial() {}
	CTerrainMaterial( CPtrFuncBase<NGfx::CTexture> *_pTex, CPtrFuncBase<NGfx::CTexture> *_pMask, int _nPriority, bool _bApplyFog, bool _bAddPlaced, bool _bCastShadow, bool _bBackFaceCastShadow )
		: pTex(_pTex), pMask(_pMask), nPriority(_nPriority), bApplyFog(_bApplyFog), bAddPlaced(_bAddPlaced), bCastShadow(_bCastShadow), bBackFaceCastShadow(_bBackFaceCastShadow) {}
	virtual bool DoesCastShadow() const { return bCastShadow; }
	virtual bool IsOpaque() const { return true; }
	virtual bool IsAlphaTest() const { return true; }
	virtual bool IsTransparent() const { return false; }
	virtual void AddATOperations( COpGenContext *p, const SPerspDirectionalDepthInfo *pDepthInfo ) {}
	virtual void AddOperations( COpGenContext *pOp, SRenderPathContext *pRPC );
	virtual void AddCustomOperation( COpGenContext *pOp, const ERenderOperation &op, unsigned char nPass,
		int nSBM, char nDestRegister, CRenderCmdList::UParameter p = CRenderCmdList::UParameter() );
	virtual bool Is2Sided() const { return false; }
	virtual bool IsSelfIllum() const { return false; }
	virtual void Precache();
	virtual bool DoesBackFaceCastShadow() const { return bBackFaceCastShadow; }
};
class CCloudsH5Material : public IMaterial
{
	OBJECT_BASIC_METHODS(CCloudsH5Material);
public:
	ZDATA
		CDGPtr<CPtrFuncBase<NGfx::CTexture> > pClouds;
	int nPriority;
	float fWrapsPerSecond;
	bool bApplyFog;
	bool bAddPlaced;
	bool bDoesCastShadow;
	bool bProjectOnTerrain;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pClouds); f.Add(3,&nPriority); f.Add(4,&fWrapsPerSecond); f.Add(5,&bApplyFog); f.Add(6,&bAddPlaced); f.Add(7,&bDoesCastShadow); f.Add(8,&bProjectOnTerrain);  return 0; }
	bool SetRenderMode( NGfx::CRenderContext *pRC, const SLightInfo &lightInfo, int nROP, CRenderCmdList::UParameter p1, CRenderCmdList::UParameter p2 );
	CVec3 CalcShift();
public:
	CCloudsH5Material() {}
	CCloudsH5Material( CPtrFuncBase<NGfx::CTexture> *_pTex, int _nPriority, float _fWrapsPerSecond, bool _bApplyFog, bool _bAddPlaced, bool _bDoesCastShadow, bool _bProjectOnTerrain )
		: pClouds(_pTex), nPriority(_nPriority), fWrapsPerSecond(_fWrapsPerSecond), bApplyFog(_bApplyFog), bAddPlaced(_bAddPlaced), bDoesCastShadow(_bDoesCastShadow), bProjectOnTerrain(_bProjectOnTerrain) {}
		virtual bool DoesCastShadow() const { return bDoesCastShadow; }
		virtual bool IsProjectOnTerrain() const { return bProjectOnTerrain; }
		virtual bool IsOpaque() const { return false; }
		virtual bool IsAlphaTest() const { return false; }
		virtual bool IsTransparent() const { return false; }
		virtual void AddATOperations( COpGenContext *p, const SPerspDirectionalDepthInfo *pDepthInfo ) {}
		virtual void AddOperations( COpGenContext *pOp, SRenderPathContext *pRPC );
		virtual void AddCustomOperation( COpGenContext *pOp, const ERenderOperation &op, unsigned char nPass,
			int nSBM, char nDestRegister, CRenderCmdList::UParameter p = CRenderCmdList::UParameter() );
		virtual bool Is2Sided() const { return false; }
		virtual bool IsSelfIllum() const { return false; }
		virtual void Precache();
};
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
class CSimpleSkyMaterial : public IMaterial
{
	OBJECT_BASIC_METHODS(CSimpleSkyMaterial);
public:
	ZDATA
	CDGPtr<CPtrFuncBase<NGfx::CTexture> > pTex;
	int nPriority;
	bool bApplyFog;
	float fFOVZoom;
	bool bAddPlaced;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTex); f.Add(3,&nPriority); f.Add(4,&bApplyFog); f.Add(5,&fFOVZoom); f.Add(6,&bAddPlaced); return 0; }
	bool SetRenderMode( NGfx::CRenderContext *pRC, const SLightInfo &lightInfo, int nROP, CRenderCmdList::UParameter p1, CRenderCmdList::UParameter p2 );
public:
	CSimpleSkyMaterial() {}
	CSimpleSkyMaterial( CPtrFuncBase<NGfx::CTexture> *_pTex, int _nPriority, bool _bApplyFog, float _fFOVZoom, bool _bAddPlaced )
		: pTex(_pTex), nPriority(_nPriority), bApplyFog(_bApplyFog), fFOVZoom(_fFOVZoom), bAddPlaced(_bAddPlaced) {}
	virtual bool DoesCastShadow() const { return false; }
	virtual bool IsOpaque() const { return true; }
	virtual bool IsAlphaTest() const { return false; }
	virtual bool IsTransparent() const { return false; }
	virtual void AddATOperations( COpGenContext *p, const SPerspDirectionalDepthInfo *pDepthInfo ) {}
	virtual void AddOperations( COpGenContext *pOp, SRenderPathContext *pRPC );
	virtual void AddCustomOperation( COpGenContext *pOp, const ERenderOperation &op, unsigned char nPass,
		int nSBM, char nDestRegister, CRenderCmdList::UParameter p = CRenderCmdList::UParameter() );
	virtual bool Is2Sided() const { return false; }
	virtual bool IsSelfIllum() const { return false; }
	virtual bool DoesIgnoreZ() const { return true; }
	virtual void Precache();
};
////////////////////////////////////////////////////////////////////////////////////////////////////
}
