#include "StdAfx.h"
#include "GMatShare.h"
#include "GMaterial.hpp"
#include "DBScene.h"
#include "System/BasicShare.h"
#include "GTexture.h"
#include "GSceneUtils.h"


namespace NGScene
{

// CSkyAdapter

bool CSkyAdapter::NeedUpdate() 
{ 
	if ( !pTex )
		return false;
	return pTex.Refresh(); 
}

void CSkyAdapter::Recalc()
{
	pValue = 0;
	if ( pTex )
		pValue = pTex->GetValue();
}

void CSkyAdapter::SetSource( CPtrFuncBase<NGfx::CCubeTexture> *_pTex ) 
{ 
	pTex = _pTex;
	pValue = 0;
	Updated();
}

// CMaterialShare

extern CBasicShare<STextureKey, CFileTexture, STextureKeyHash> shareTextures;
static CPtrFuncBase<NGfx::CTexture> *GetTexture( const NDb::STexture *pTex )
{
	if ( !pTex )
		return 0;
	CPtrFuncBase<NGfx::CTexture> *pRes;
	pRes = shareTextures.Get( GetKey( pTex ) );
	return pRes;
}

void CMaterialShare::FillCreateMaterialInfo( const NDb::SMaterial *_pMaterial, SMaterialCreateInfo *pRes )
{
	SMaterialCreateInfo &mc = *pRes;
	mc.pTexture = GetTexture( _pMaterial->pTexture );
	if ( _pMaterial->pTexture )
		mc.vAvrgTexColor = NGfx::GetCVec4Color( _pMaterial->pTexture->nAverageColor );
	//pBump = shareTextures.Get( 13 );
	mc.pBump = GetTexture( _pMaterial->pBump );
	mc.pMirrorTexture = GetTexture( _pMaterial->pMirror );
	mc.pSpecular = GetTexture( _pMaterial->pGloss );
	mc.fSpecPower = _pMaterial->fSpecFactor;
	mc.pDetail = GetTexture( _pMaterial->pDetailTexture );
	mc.fDetailScale = _pMaterial->fDetailScale;
	mc.bIgnoreZ = _pMaterial->bIgnoreZBuffer;
	mc.bReceiveShadow = _pMaterial->bReceiveShadow;
	
	switch ( _pMaterial->eDynamicMode )
	{
	case NDb::SMaterial::DM_DONT_CARE : mc.eDynamicType = DT_DONT_CARE; break;
	case NDb::SMaterial::DM_FORCE_STATIC : mc.eDynamicType = DT_FORCE_STATIC; break;
	case NDb::SMaterial::DM_FORCE_DYNAMIC : mc.eDynamicType = DT_FORCE_DYNAMIC; break;
	}
	int nFlags = 0;
	switch ( _pMaterial->eAlphaMode )
	{
	case NDb::SMaterial::AM_OPAQUE: nFlags |= MF_OPAQUE; break;
	case NDb::SMaterial::AM_OVERLAY: nFlags |= MF_OVERLAY; break;
	case NDb::SMaterial::AM_OVERLAY_ZWRITE: nFlags |= MF_OVERLAY_ZWRITE; break;
	case NDb::SMaterial::AM_TRANSPARENT: nFlags |= MF_TRANSPARENT; break;
	case NDb::SMaterial::AM_ALPHA_TEST: nFlags |= MF_ALPHA_TEST; break;
	case NDb::SMaterial::AM_DECAL: nFlags |= MF_DECAL; break;
	default: ASSERT(0); break;
	}
	switch ( _pMaterial->eEffect )
	{
	case NDb::SMaterial::M_GENERIC: nFlags |= MF_GENERIC; break;
	case NDb::SMaterial::M_WATER: nFlags |= MF_WATER; break;
	case NDb::SMaterial::M_TRACKS: nFlags |= MF_TRACKS; break;
	case NDb::SMaterial::M_TERRAIN: nFlags |= MF_TERRAIN; break;
	case NDb::SMaterial::M_CLOUDS_H5: nFlags |= MF_CLOUDS_H5; break;
	case NDb::SMaterial::M_ANIM_WATER: nFlags |= MF_ANIM_WATER; break;
	case NDb::SMaterial::M_SURF: nFlags |= MF_SURF; break;
	case NDb::SMaterial::M_SIMPLE_SKY: nFlags |= MF_SIMPLE_SKY; break;
	case NDb::SMaterial::M_REFLECT_WATER: nFlags |= MF_REFLECT_WATER; break;
	default: ASSERT(0); break;
	}
	switch ( _pMaterial->eLightingMode )
	{
	case NDb::SMaterial::L_NORMAL: break;
	case NDb::SMaterial::L_SELFILLUM: mc.bSelfIllum = true; break;
	default: ASSERT(0); break;
	}
	mc.alphaMode = nFlags;
	mc.b2Sided = _pMaterial->bIs2Sided;
	mc.fMetalMirror = _pMaterial->fMetalMirror;
	mc.fDielMirror = _pMaterial->fDielMirror;
	mc.pSky = pSky;
	mc.bDoesCastShadow = _pMaterial->bCastShadow;
	mc.nPriority = _pMaterial->nPriority;
	mc.vTranslucentColor = _pMaterial->vTranslucentColor;
	mc.fFloatParam = _pMaterial->fFloatParam;
	mc.bProjectOnTerrain = _pMaterial->bProjectOnTerrain;
	mc.bApplyFog = _pMaterial->bAffectedByFog;
	mc.bAddPlaced = _pMaterial->bAddPlaced;
	mc.bBackFaceCastShadow = _pMaterial->bBackFaceCastShadow;
}

IMaterial* CMaterialShare::CreateMaterial( const NDb::SMaterial *pMaterial )
{
	SMaterialCreateInfo mc;
	if ( pMaterial )
	{
		CMatHashmap::iterator i = materials.find( pMaterial );
		if ( i != materials.end() && IsValid( i->second ) )
			return i->second;
		FillCreateMaterialInfo( pMaterial, &mc );
		IMaterial *pRes = NGScene::CreateMaterial( mc );
		materials[ pMaterial ] = pRes;
		return pRes;
	}
	else
	{
		FillCreateMaterialInfo( pMaterial, &mc );
		return NGScene::CreateMaterial( mc );
	}
}

// CColorMaterialShare

IMaterial* CColorMaterialShare::CreateMaterial( const CVec3 &color )
{
	CMatHashmap::iterator i = materials.find( color );
	if ( i != materials.end() && IsValid( i->second ) )
		return i->second;
	SMaterialCreateInfo mc;
	mc.pColor = new CCVec4( CVec4(color,1) );
	IMaterial *pRes = NGScene::CreateMaterial( mc );

	materials[color] = pRes;
	return pRes;
}

// CTransparentMaterialShare

IMaterial* CTransparentMaterialShare::CreateMaterial( const CVec4 &cr, bool bDoesCastShadow )
{
	CVec4 color( cr.x * cr.w, cr.y * cr.w, cr.z * cr.w, cr.w );
	if ( bDoesCastShadow )
	{
		CMatHashmap::iterator i = materials.find( color );
		if ( i != materials.end() && IsValid( i->second ) )
			return i->second;
	}
	else
	{
		CMatHashmap::iterator i = noShadowMaterials.find( color );
		if ( i != noShadowMaterials.end() && IsValid( i->second ) )
			return i->second;
	}
	CObj<CPtrFuncBase<NGfx::CTexture> > pTexture = new CColorTexture( color );//shareTextures.Get( 1724 );
	SMaterialCreateInfo mc;
	mc.pTexture = pTexture;
	mc.vAvrgTexColor = color;
	mc.alphaMode = MF_GENERIC|MF_TRANSPARENT;
	mc.bDoesCastShadow = bDoesCastShadow;
	IMaterial *pRes = NGScene::CreateMaterial( mc );
	materials[color] = pRes;
	return pRes;
}

}
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x01671182, CMaterialShare )
REGISTER_SAVELOAD_CLASS( 0x01812140, CSkyAdapter )

