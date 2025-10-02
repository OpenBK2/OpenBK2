#ifndef __GRenderLight_H_
#define __GRenderLight_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "..\3Dlib\Transform.h"
#include "GRenderExecute.h"
#include "GLightPerVertex.h"
namespace NGScene
{

class ILight: public CVersioningBase
{
public:
	//virtual int GetPriority() const = 0;
	virtual bool CheckCulling( CTransformStack *pTS ) { return true; }
	virtual void AddToState( 	SPerVertexLightState *pRes ) = 0;
};

class CDirectionalLight: public ILight
{
	OBJECT_BASIC_METHODS(CDirectionalLight);
	virtual bool NeedUpdate();
	virtual void Recalc() {}
	ZDATA
	CDGPtr<CFuncBase<CVec3> > pColor;
	CDGPtr<CFuncBase<CVec3> > pGlossColor;
	CDGPtr<CFuncBase<CVec3> > pAmbient;	
	CVec4 vDepth;
	CVec3 vLightDir;
	float fMaxHeight;
	CDGPtr<CFuncBase<CVec3> > pShadeColor, pIncidentShadeColor;
	ZSKIP
	float fSmoothedSceneHeight;
	CVec3 vShadowsLightDir;
	CDGPtr<CPtrFuncBase<NGfx::CTexture> > pClouds;
	CDGPtr<CFuncBase<SHMatrix> > pCloudsProjection;
	SShadowMatrixAlign shadowAlign;
	float fShadowsMaxDetailLength;
	ZSKIP
	CVec3 vDymanicLightsModification;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pColor); f.Add(3,&pGlossColor); f.Add(4,&pAmbient); f.Add(5,&vDepth); f.Add(6,&vLightDir); f.Add(7,&fMaxHeight); f.Add(8,&pShadeColor); f.Add(9,&pIncidentShadeColor); f.Add(11,&fSmoothedSceneHeight); f.Add(12,&vShadowsLightDir); f.Add(13,&pClouds); f.Add(14,&pCloudsProjection); f.Add(15,&shadowAlign); f.Add(16,&fShadowsMaxDetailLength); f.Add(18,&vDymanicLightsModification); return 0; }
	int nVersionID;
public:
	CDirectionalLight() : nVersionID(0) {}
	CDirectionalLight( CFuncBase<CVec3> *pColor, CFuncBase<CVec3> *pGlossColor, 
		const CVec3 &_vLightDir, const CVec3 &_vShadowsLightDir,
		float fMaxHeight, 
		CFuncBase<CVec3> *pAmbient,
		CFuncBase<CVec3> *_pShadeColor, CFuncBase<CVec3> *_pIncidentShadeColor,
		CPtrFuncBase<NGfx::CTexture> *_pClouds, CFuncBase<SHMatrix> *_pCloudsProjection,
		float _fShadowsMaxDetailLength, const CVec3 &_vDymanicLightsModification );
	void AddToState( SPerVertexLightState *pRes );
	void PrepareLightInfo( SLightInfo *pLightInfo );
	const CVec3& GetLightDir() const { return vLightDir; }
	const CVec3& GetShadowsLightDir() const { return vShadowsLightDir; }
	const CVec4& GetDepth() const { return vDepth; }
	float GetMaxHeight() const { return fMaxHeight; }
	float GetSmoothedSceneHeight( float f );
	SShadowMatrixAlign &GetShadowMatrixAlign() { return shadowAlign; }
	NGfx::CTexture *GetCloudsTexture();
	void CalcCloudProjection( SHMatrix *pRes );
	float GetShadowsMDLength() const { return fShadowsMaxDetailLength; }
}; 

class CPointLight: public ILight
{
	OBJECT_BASIC_METHODS(CPointLight);
	ZDATA
	SBound sBound;
	CVec3 vCenter;
	float fRadius;
	CTransformStack sTransform;
	CVec3 vColor;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&sBound); f.Add(3,&vCenter); f.Add(4,&fRadius); f.Add(5,&sTransform); f.Add(6,&vColor); return 0; }
	int nThisPointLightID;
public:
	CPointLight();
	CPointLight( const CVec3 &_vColor, const CVec3 &ptCenter, float fRadius );
	virtual bool CheckCulling( CTransformStack *pTS );
	void AddToState( SPerVertexLightState *pRes );
};

class CAnimLight;
class CDynamicPointLight : public ILight
{
	OBJECT_BASIC_METHODS(CDynamicPointLight);
	ZDATA
	CDGPtr<CPtrFuncBase<CAnimLight> > pLight;
	bool bIsOn;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pLight); f.Add(3,&bIsOn); return 0; }

public:
	CDynamicPointLight() : bIsOn( true ) {}
	CDynamicPointLight( CPtrFuncBase<CAnimLight> *_p ) : pLight(_p), bIsOn( true ) {}
	virtual bool CheckCulling( CTransformStack *pTS );
	void AddToState( SPerVertexLightState *pRes );
	void SwitchLight( bool bMode ) { bIsOn = bMode; }
};

class CLightStateNode : public CFuncBase<SPerVertexLightState>
{
	OBJECT_BASIC_METHODS(CLightStateNode);
	ZDATA
	CDGPtr<CDirectionalLight> pDirectionalLight;
	vector< CPtr<ILight> > lights, dynamicLights;
	bool bForceRecalc;
	bool bSmthHasChanged;
	ZONSERIALIZE
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pDirectionalLight); f.Add(3,&lights); f.Add(4,&dynamicLights); f.Add(5,&bForceRecalc); f.Add(6,&bSmthHasChanged); OnSerialize( f ); return 0; }
	CTransformStack tsClip;

	void OnSerialize( IBinSaver &f ) { f.Add( 200, &value ); }
protected:
	virtual bool NeedUpdate();
	virtual void Recalc();
public:
	CLightStateNode(): bForceRecalc(false) {}
	//
	void SetWarFogBlend( float fBlend );
	void SetWarFog( const CArray2D<unsigned char> &fog, float fScale );
	void SetDirectional( CDirectionalLight *p );
	void AddPointLight( CPointLight *p );
	void AddPointLight( CDynamicPointLight *p );
	CDirectionalLight* GetDirectional() const { return pDirectionalLight; }
	void SetClipTS( const CTransformStack &_ts );
};

}
#endif

