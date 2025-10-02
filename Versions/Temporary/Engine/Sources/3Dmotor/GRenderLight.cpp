#include "StdAfx.h"
#include "GScene.h"
#include "GRenderLight.h"

#include "../System/Commands.h"

static int nDirectionalLightID, nPointLightID;
namespace NGScene
{

// CDirectionalLight

CDirectionalLight::CDirectionalLight( CFuncBase<CVec3> *_pColor, CFuncBase<CVec3> *_pGlossColor, 
	const CVec3 &_vLightDir, const CVec3 &_vShadowsLightDir,
	float _fMaxHeight, 
	CFuncBase<CVec3> *_pAmbient,
	CFuncBase<CVec3> *_pShadeColor, CFuncBase<CVec3> *_pIncidentShadeColor,
	CPtrFuncBase<NGfx::CTexture> *_pClouds, CFuncBase<SHMatrix> *_pCloudsProjection,
	float _fShadowsMaxDetailLength, const CVec3 &_vDymanicLightsModification )
: pColor(_pColor), pGlossColor(_pGlossColor), pAmbient(_pAmbient), vDepth( 0, 0, 1 / _fMaxHeight, 0 ),
	fMaxHeight(_fMaxHeight), pShadeColor(_pShadeColor), pIncidentShadeColor(_pIncidentShadeColor), nVersionID(0), fSmoothedSceneHeight(0),
	pClouds(_pClouds), pCloudsProjection(_pCloudsProjection), fShadowsMaxDetailLength(_fShadowsMaxDetailLength), vDymanicLightsModification(_vDymanicLightsModification)	
{
	vLightDir = _vLightDir;
	vShadowsLightDir = _vShadowsLightDir;
}

bool CDirectionalLight::NeedUpdate()
{
	bool bHasChanged = false;
	bHasChanged |= pColor.Refresh();
	bHasChanged |= pAmbient.Refresh();
	bHasChanged |= pShadeColor.Refresh();
	bHasChanged |= pIncidentShadeColor.Refresh();
	bHasChanged |= pGlossColor.Refresh();
	if ( bHasChanged )
	{
		++nDirectionalLightID;
		nVersionID = nDirectionalLightID;
	}
	return bHasChanged;
}

void CDirectionalLight::AddToState( SPerVertexLightState *pRes )
{
//	float fTestBrightness = 0.25f;//
//	pRes->SetDirectional(
//		CVec3(0,0,0), CVec3(0,fTestBrightness,0), CVec3(0,0,fTestBrightness), CVec3(fTestBrightness,0,0),
//		-vLightDir, nDirectionalLightID );
	pRes->SetDirectional(
		pAmbient->GetValue(), pColor->GetValue(), pShadeColor->GetValue(), pIncidentShadeColor->GetValue(),
		-vLightDir, nDirectionalLightID, vDymanicLightsModification );
}

void CDirectionalLight::PrepareLightInfo( SLightInfo *pLightInfo )
{
	SLightInfo &lightInfo = *pLightInfo;

	lightInfo.bNeedSet = true;
	lightInfo.vGlossColor = pGlossColor->GetValue();
	lightInfo.vLightColor = pColor->GetValue();
	lightInfo.vAmbientColor = pAmbient->GetValue();
	lightInfo.vShadeColor = pShadeColor->GetValue();
	lightInfo.vIncidentShadeColor = pIncidentShadeColor->GetValue();
	lightInfo.vLightPos = CVec4(-vLightDir, 0);
}

float CDirectionalLight::GetSmoothedSceneHeight( float f )
{
	if ( f < fSmoothedSceneHeight * 0.7f )
		fSmoothedSceneHeight = f;
	if ( f > fSmoothedSceneHeight )
		fSmoothedSceneHeight = Float2Int( f + 0.5f );
	return fSmoothedSceneHeight;
}

NGfx::CTexture *CDirectionalLight::GetCloudsTexture() 
{ 
	if ( !pClouds )
		return 0;
	pClouds.Refresh();
	return pClouds->GetValue();
}

void CDirectionalLight::CalcCloudProjection( SHMatrix *pRes )
{
	if ( !pClouds )
		return;
	Identity( pRes );
	if ( !pCloudsProjection )
		return; 
	pCloudsProjection.Refresh();
	*pRes = pCloudsProjection->GetValue();
}

// CPointLight

CPointLight::CPointLight() : nThisPointLightID( ++nPointLightID ) 
{
}

CPointLight::CPointLight( const CVec3 &_vColor, const CVec3 &_ptCenter, float _fRadius )
	: vColor(_vColor), vCenter( _ptCenter), fRadius(_fRadius), nThisPointLightID( ++nPointLightID )
{
	sTransform.MakeParallel( 2 * _fRadius, 2 * _fRadius, -_fRadius, _fRadius );
	SHMatrix cameraPos;
	MakeMatrix( &cameraPos, _ptCenter, CVec3( 0, 0, 1 ) );
	sTransform.SetCamera( cameraPos );

	sBound.SphereInit( _ptCenter, _fRadius );
}

bool CPointLight::CheckCulling( CTransformStack *pTS )
{
	return pTS->IsIn( sBound );
}

void CPointLight::AddToState( SPerVertexLightState *pRes )
{
	pRes->AddPointLight( vCenter, fRadius, vColor, nThisPointLightID );
}

// CDynamicPointLight

bool CDynamicPointLight::CheckCulling( CTransformStack *pTS )
{
	if ( !bIsOn )
		return false;

	pLight.Refresh();
	const CAnimLight &l = *pLight->GetValue();
	if ( !l.bActive || l.bEnd )
		return false;
	return pTS->IsIn( SSphere( l.position, l.fRadius ) );
}

void CDynamicPointLight::AddToState( SPerVertexLightState *pRes )
{
	pLight.Refresh();
	const CAnimLight &l = *pLight->GetValue();
	if ( !l.bActive || l.bEnd )
		return;
	pRes->AddPointLight( l.position, l.fRadius, l.color );
}

// CLightStateNode

static bool s_bDynamicLights = true;
bool CLightStateNode::NeedUpdate()
{
	bool bRes = false;
	if ( IsValid(pDirectionalLight) )
		bRes |= pDirectionalLight.Refresh();
	bRes |= EraseInvalidRefs( &lights );
	bRes |= !value.dynamicPointLights.empty();
	bRes |= bSmthHasChanged;
	value.ResetDynamicLights();

	if ( !s_bDynamicLights )
		return bRes;

	int nIndex = 0;
	for ( int k = 0; k < dynamicLights.size(); ++k )
	{
		ILight *pLight = dynamicLights[k];
		if ( !IsValid( pLight ) )
			continue;

		if ( pLight->CheckCulling( &tsClip ) )
			pLight->AddToState( &value );

		dynamicLights[nIndex++] = pLight;
	}
	dynamicLights.resize( nIndex );
	
	bRes |= !value.dynamicPointLights.empty();
	return bRes;
}

void CLightStateNode::Recalc()
{
	bSmthHasChanged = false;
	value.ResetStaticLights();
	if ( IsValid(pDirectionalLight) )
		pDirectionalLight->AddToState( &value );
	for ( vector< CPtr<ILight> >::iterator k = lights.begin(); k != lights.end(); ++k )
	{
		ILight *pLight = *k;
//		if ( pLight->CheckCulling( pClipTS ) )
		pLight->AddToState( &value );
	}
	value.SortPointLights();
}

void CLightStateNode::SetWarFogBlend( float fBlend )
{
	value.SetWarFogBlend( fBlend );
}

void CLightStateNode::SetWarFog( const CArray2D<unsigned char> &_fog, float _fScale )
{
	if ( value.SetWarFog( _fog, _fScale ) )
		bSmthHasChanged = true;
}

void CLightStateNode::SetDirectional( CDirectionalLight *p )
{
	pDirectionalLight = p;
	bSmthHasChanged = true;
}

void CLightStateNode::AddPointLight( CPointLight *p )
{
	lights.push_back( p );
	bSmthHasChanged = true;
}

void CLightStateNode::AddPointLight( CDynamicPointLight *p )
{
	dynamicLights.push_back( p );
}

void CLightStateNode::SetClipTS( const CTransformStack &_ts ) 
{ 
	tsClip = _ts; 
}

START_REGISTER(GRenderLight)
REGISTER_VAR_EX( "gfx_dynamic_lights", NGlobal::VarBoolHandler, &s_bDynamicLights, true, STORAGE_USER )
FINISH_REGISTER

}
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x02511009, CDirectionalLight )
REGISTER_SAVELOAD_CLASS( 0x01961170, CPointLight )
REGISTER_SAVELOAD_CLASS( 0x02682130, CDynamicPointLight )
REGISTER_SAVELOAD_CLASS( 0x01693170, CLightStateNode )

