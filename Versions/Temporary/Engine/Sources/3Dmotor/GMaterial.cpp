#include "StdAfx.h"
#include "GMaterial.hpp"
#include "GMaterial.h"
#include "GfxRender.h"
#include "GRenderFactor.h"
#include "GfxShaders.h"
#include "GfxBuffers.h"
#include "GRenderExecute.h"
#include "GRenderModes.h"
#include "GInit.h"
#include "..\System\Commands.h"



//bool bSimpleLightmaps = false;
static bool s_bShadowOnTrees = true;
static float s_fTestHorse = 0;

namespace NGScene
{
static bool bFPB = true;

bool bNewShadows = false;

extern bool bNoDepthRender;

inline bool IsUseG5()
{
	int nShadowsQuality = GetShadowsQuality();
	return ( nShadowsQuality >= SQ_PRE_BEST ) && ( NGfx::GetHardwareLevel() == NGfx::HL_R300 );
}

inline int GetAlphaMode( bool bAdd, bool bFogEnabled, int *pnFog )
{
	if ( pnFog )
		*pnFog = bFogEnabled ? FOG_NORMAL : FOG_NONE;
	if ( bAdd )
	{
		if ( bFogEnabled )
			return ABM_ALPHA_ADD | FOG_BLACK;
		return ABM_ALPHA_ADD | FOG_NONE;
	}
	if ( bFogEnabled )
		return ABM_ALPHA_BLEND | FOG_NORMAL;
	return ABM_ALPHA_BLEND | FOG_NONE;
}

template<class T>
inline void AssignTex( T **pTex, CDGPtr<CPtrFuncBase<T> > &p )
{
	*pTex = 0;
	if ( p )
	{
		p.Refresh();
		*pTex = p->GetValue();
	}
}
inline void AssignTexBlackDefault( NGfx::CTexture **pTex, CDGPtr<CPtrFuncBase<NGfx::CTexture> > &p )
{
	AssignTex( pTex, p );
	if ( *pTex == 0 )
		*pTex = GetBlackTexture();
}
inline void AssignTexWhiteDefault( NGfx::CTexture **pTex, CDGPtr<CPtrFuncBase<NGfx::CTexture> > &p )
{
	AssignTex( pTex, p );
	if ( *pTex == 0 )
		*pTex = GetWhiteTexture();
}
template<class T>
inline void Refresh( CDGPtr<T> *p ) { if ( *p ) { p->Refresh(); (*p)->GetValue(); } }

static void SetupPShadows( NGfx::CRenderContext *pRC, const SFastGf3RenderInfo *pFastInfo, const SLightInfo &lightInfo )
{
	SetupNLShadowsProjection( pRC, *pFastInfo->pPersp );
	pRC->SetVSConst( 29, pFastInfo->vColor );
	pRC->SetVSConst( 30, CVec4( lightInfo.fWarFogBlend, 1-lightInfo.fWarFogBlend, 0,0) );
	pRC->SetTexture( 0, pFastInfo->pTexture, NGfx::FILTER_BEST );
	pRC->SetTexture( 1, pFastInfo->pDepth, NGfx::FILTER_LINEAR );
	pRC->SetAlphaRef( pFastInfo->nAlphaTest );
	if ( pFastInfo->bTranslucent )
		pRC->SetVSConst( 35, CVec4(0,0,0,1e-5f) );
	else
		pRC->SetVSConst( 35, lightInfo.vLightPos );
}

inline void AddCommonCustomOperation( CDGPtr<CPtrFuncBase<NGfx::CTexture> > &pMatTex, COpGenContext *pOp,
																		 const ERenderOperation &op, unsigned char nPass, int nSBM, char nDestRegister, CRenderCmdList::UParameter p )
{
	NGfx::CTexture *pTex = 0;
	AssignTex( &pTex, pMatTex );
	if ( !pTex )
		pTex = GetWhiteTexture();

	pOp->AddOperation( op, 0, nSBM, nDestRegister, pTex, p );
}

const float fShadSamp = 0.001f;
const float fShadSampDiag = fShadSamp * FP_SQRT_2 * 0.5f;

static void SetG5Props( NGfx::CRenderContext *pRC )
{
	pRC->SetPSConst( 7, CVec4(4, 2, 1, 5/256.0f) );
	pRC->SetPSConst( 8, CVec4(1.0f/9, 0, 0, 0) );
	pRC->SetPSConst( 10, CVec4( fShadSamp, 0, 0, 0) );
	pRC->SetPSConst( 11, CVec4(-fShadSamp, 0, 0, 0) );
	pRC->SetPSConst( 12, CVec4(0, fShadSamp, 0, 0) );
	pRC->SetPSConst( 13, CVec4(0, -fShadSamp, 0, 0) );
	pRC->SetPSConst( 14, CVec4(fShadSampDiag,fShadSampDiag,0,0) );
	pRC->SetPSConst( 15, CVec4(-fShadSampDiag,fShadSampDiag,0,0) );
	pRC->SetPSConst( 16, CVec4(fShadSampDiag,-fShadSampDiag,0,0) );
	pRC->SetPSConst( 17, CVec4(-fShadSampDiag,-fShadSampDiag,0,0) );
}

// CGenericMaterial

IMaterial* CGenericMaterial::GetExactDecal()
{
	if ( IsValid( pExactDecal ) )
		return pExactDecal;
	if ( mt == EXACT_DECAL )
		return this;
	if ( mt != DECAL )
		return 0;
	pExactDecal = Duplicate();
	pExactDecal->SetMT( EXACT_DECAL );
	return pExactDecal;
}

// CGenericMaterial

IMaterial* CGenericMaterial::GetWindAffected()
{
	if ( IsValid( pWindAffected ) )
		return pWindAffected;
	if ( bWindAffected )
		return this;
	pWindAffected = Duplicate();
	pWindAffected->bWindAffected = true;
	return pWindAffected;
}

// CGenericMaterial

IMaterial* CGenericMaterial::GetNoReceiveShadows()
{
	if ( IsValid( pNoReceiveShadows ) )
		return pNoReceiveShadows;
	if ( !bReceiveShadows )
		return this;
	pNoReceiveShadows = Duplicate();
	pNoReceiveShadows->bReceiveShadows = false;
	return pNoReceiveShadows;
}

void CGenericMaterial::SetReflectionInfo( CPtrFuncBase<NGfx::CCubeTexture> *_pSky, CPtrFuncBase<NGfx::CTexture> *_pMirrorTex,
	float _fDielMirror, float _fMetalMirror )
{
	if ( _pSky && ( _fDielMirror > 0 || _fMetalMirror > 0 ) )
	{
		pSkyTex = _pSky;
		pMirrorTex = _pMirrorTex;
	}
	else
	{
		pSkyTex = 0;
		pMirrorTex = 0;
	}
	vMirrorParam = CVec4( 0, 0, _fDielMirror, _fMetalMirror );
}

NGfx::CTexture *CGenericMaterial::GetAlphaTestTex()
{
	if ( !pDiffuseTex )
		return 0;

	if( bNewShadows )
	if( mt == MT_TRANSPARENT)
	{
		pDiffuseTex.Refresh();
		return pDiffuseTex->GetValue();
	}

	if ( !bAlphaTest )return 0;
	pDiffuseTex.Refresh();
	return pDiffuseTex->GetValue();
}

void CGenericMaterial::AddATOperations( COpGenContext *p, const SPerspDirectionalDepthInfo *pDepthInfo )
{
	if ( bAlphaTest )
	{
		ASSERT( pDiffuseTex );
		pDiffuseTex.Refresh();
		if ( pDepthInfo )
			p->AddOperation( RO_TEXTURE_AT_NLP, 0, 0, 0, pDiffuseTex->GetValue(), p->GetCurFragment()->vars.fFade, pDepthInfo );
		else
			p->AddOperation( RO_TEXTURE_AT, 0, 0, 0, pDiffuseTex->GetValue(), p->GetCurFragment()->vars.fFade );
	}
}

void CGenericMaterial::Precache()
{
	Refresh( &pDiffuseColor );
	Refresh( &pDiffuseTex );
	Refresh( &pDiffuseTex2 );
	Refresh( &pDetailTex );
	Refresh( &pSkyTex );
	Refresh( &pMirrorTex );
}


bool CGenericMaterial::SetRenderMode( NGfx::CRenderContext *pRC, const SLightInfo &lightInfo, int nROP, CRenderCmdList::UParameter p1, CRenderCmdList::UParameter p2 )
{
	//ASSERT( bWindAffected );


	float fMul =  1.0f / NGfx::N_VEC_FULL_TEX_SIZE;
	float Time = GetTickCount() * 0.001f * fAngVel; 

	float si=sin(Time);
	float co=cos(Time);


	pRC->SetVSConst( 38, CVec4( fMul * co, fMul * si, -fMul * si, fMul * co ) );
	pRC->SetVSConst( 39, CVec4( ( -co + si ) * 0.5f + 0.5f, ( -co - si ) * 0.5f + 0.5f, 0.0f, 0.0f ) );


	const bool bUseG5 = IsUseG5();

	//ASSERT ( bReceiveShadows );

	switch ( nROP )
	{
	case RO_G3_DIFFUSE_TEX:
		if ( p1.pFastInfo->pTexture2 )
		{
			if ( bUseG5 )
			{
				pRC->SetPixelShader( psG5DiffuseTex2 );
				SetG5Props( pRC );
			}
			else
				pRC->SetPixelShader( psG3DiffuseTex2 );
			pRC->SetVertexShader( vsG3DiffuseTex2 );
			pRC->SetVSConst( 31, CVec4( 1.0f / NGfx::N_VEC_FULL_TEX_SIZE, 0, 0, 0 ) );
		}
		else
		{
			if ( bUseG5 )
			{
				pRC->SetPixelShader( psG5DiffuseTex );
				SetG5Props( pRC );
			}
			else if( !bNewShadows )
			{
				if ( s_bShadowOnTrees )
					pRC->SetPixelShader( psG3DiffuseTex );
				else
					pRC->SetPixelShader( bAlphaTest ? psG3DiffuseTexNoShadows : psG3DiffuseTex );
			}
			else
			{
				pRC->SetPixelShader( bNoDepthRender || bAlphaTest || !bReceiveShadows ||( eType == DT_FORCE_DYNAMIC ) 
				                    ? psG3DiffuseTexNoShadows : psG3DiffuseTex );
			}

			//pRC->SetPixelShader( psDiffuse );

			if ( bWindAffected )
			{
				//FIXME!
				pRC->SetVertexShader( vsG3DiffuseTexEXT );
				float WindTime = 0.0f;
				WindTime = GetTickCount() * 0.003f; 

				float _co = 2.0f * cos( WindTime ) / NGfx::N_VEC_FULL_TEX_SIZE;
				float _si = 2.0f * sin( WindTime ) / NGfx::N_VEC_FULL_TEX_SIZE;

				pRC->SetVSConst( 31, CVec4( +_co, +_si, -_si * 0.2f, +_co * 0.2f ) );			
			}
			else
			{
				pRC->SetVertexShader( bNewShadows ? vsG3DiffuseTexLP : vsG3DiffuseTex );
			}
		}
		SetupPShadows( pRC, p1.pFastInfo, lightInfo );
		pRC->SetPSConst( 4, CVec4( 0,0,0, 124/256.0f ) );
		if ( p1.pFastInfo->pTexture2 )
			pRC->SetTexture( 3, p1.pFastInfo->pTexture2, NGfx::FILTER_BEST );
			//pRC->SetTexture( 2, p1.pFastInfo->pTexture2, NGfx::FILTER_BEST );
		break;
	case RO_G3_TRASPARENT:
		if ( bUseG5 )
		{
			pRC->SetPixelShader( psG5TransparentGeom );
			SetG5Props( pRC );
		}
		else
			pRC->SetPixelShader( bNewShadows ? psG3TransparentGeomNoLM : psG3TransparentGeom );

		if(bWindAffected)
		{
			//FIXME!
			pRC->SetVertexShader( vsG3TransparentGeomEXT );
			float WindTime = 0.0f;
			WindTime = GetTickCount() * 0.003f; 

			float _co = 2.0f * cos( WindTime ) / NGfx::N_VEC_FULL_TEX_SIZE;
			float _si = 2.0f * sin( WindTime ) / NGfx::N_VEC_FULL_TEX_SIZE;

			pRC->SetVSConst( 31, CVec4( +_co, +_si, -_si * 0.2f, +_co * 0.2f ) );			
		}
		else
		{
			pRC->SetVertexShader( vsG3TransparentGeom);
		}

		//pRC->SetVertexShader( vsG3TransparentGeom );

		SetupPShadows( pRC, p1.pFastInfo, lightInfo );
		pRC->SetPSConst( 4, CVec4( 0,0,0, 124/256.0f ) );
		break;
	case RO_G3_DIFFUSE_TEX_MIRROR:
		if ( p1.pFastInfo->pTexture2 )
		{
			if ( bUseG5 )
			{
				pRC->SetPixelShader( psG5DiffuseTex2 );
				SetG5Props( pRC );
			}
			else
				pRC->SetPixelShader( psG3DiffuseTex2 );
			pRC->SetVertexShader( vsG3DiffuseTex2 );
			pRC->SetVSConst( 31, CVec4( 1.0f / NGfx::N_VEC_FULL_TEX_SIZE, 0, 0, 0 ) );
		}
		else
		{
			if ( bUseG5 )
			{
				pRC->SetPixelShader( psG5DiffuseTex );
				SetG5Props( pRC );
			}
			else
        pRC->SetPixelShader( psG3DiffuseTex );
			//pRC->SetPixelShader( psDiffuse );
			pRC->SetVertexShader( vsG3DiffuseTex );
		}
		SetupPShadows( pRC, p1.pFastInfo, lightInfo );
		pRC->SetPSConst( 4, CVec4( 0,0,0, 124/256.0f ) );
		if ( p1.pFastInfo->pTexture2 )
			pRC->SetTexture( 3, p1.pFastInfo->pTexture2, NGfx::FILTER_BEST );
			//pRC->SetTexture( 2, p1.pFastInfo->pTexture2, NGfx::FILTER_BEST );
		break;
	case RO_G3_DIFFUSE_SPEC:
	case RO_G3_DIFFUSE_SPEC_LM:
		if ( nROP == RO_G3_DIFFUSE_SPEC || NGfx::GetHardwareLevel() < NGfx::HL_RADEON2 )
		{
			if ( bUseG5 )
			{
				pRC->SetPixelShader( psG5DiffuseTexSpec );
				SetG5Props( pRC );
			}
			else
			{
				pRC->SetPixelShader( psG3DiffuseTexSpec );
				pRC->SetPSConst( 6, CVec4( 0,0,0, 124/256.0f ) );
			}
			pRC->SetVertexShader( vsG3DiffuseTexSpec );
		}
		else
		{
			if ( bUseG5 )
			{
				pRC->SetPixelShader( psG5DiffuseTexSpecLM );
				SetG5Props( pRC );
			}
			else
			{
				pRC->SetPixelShader( psG3DiffuseTexSpecLM );
				pRC->SetPSConst( 6, CVec4( 0,0,0, 124/256.0f ) );
			}
			pRC->SetVertexShader( vsG3DiffuseTexSpecLM );
			pRC->SetTexture( 5, p1.pFastInfo->pLM, NGfx::FILTER_BEST );
			//pRC->SetTexture( 4, p1.pFastInfo->pLM, NGfx::FILTER_BEST );
		}
		//pRC->SetPixelShader( psDiffuse );
		SetupPShadows( pRC, p1.pFastInfo, lightInfo );
		pRC->SetVSConst( 31, CVec4( 1, -1, 0, 0 ) );
		pRC->SetPSConst( 5, CVec4( lightInfo.vGlossColor.x, lightInfo.vGlossColor.y, lightInfo.vGlossColor.z, 1 ) ); // specular color
		{
			float fPower = p1.pFastInfo->fSpecPower;
			float f8 = 0, f16 = 0, f32 = 0;
			if ( fPower > 8 )
			{
				if ( fPower > 16 )
				{
					if ( fPower > 32 )
						f32 = 1;
					else
					{
						f32 = ( fPower - 16 ) / 16;
						f16 = 1 - f32;
					}
				}
				else
				{
					f16 = ( fPower - 8 ) / 8;
					f8 = 1 - f16;
				}
			}
			else
				f8 = 1;
			pRC->SetPSConst( 0, f8 * CVec4(1,1,1,1) ); // specular power 8
			pRC->SetPSConst( 1, f16 * CVec4(1,1,1,1) ); // specular power 16
			pRC->SetPSConst( 2, f32 * CVec4(1,1,1,1) ); // specular power 32
			pRC->SetPSConst( 3, p1.pFastInfo->vColor );
			pRC->SetPSConst( 4, -lightInfo.vAmbientColor );
		}
		//pRC->SetTexture( 2, p1.pFastInfo->pSpecular, NGfx::FILTER_BEST );
		//pRC->SetTexture( 3, GetNormalizeTexture() );
		pRC->SetTexture( 3, p1.pFastInfo->pSpecular, NGfx::FILTER_BEST );
		pRC->SetTexture( 4, GetNormalizeTexture() );
		break;
	case RO_G3_DIFFUSE_TEX_LM:
		if ( bUseG5 )
		{
			pRC->SetPixelShader( psG5DiffuseTexLM );
			SetG5Props( pRC );
		}
		else
		{
			pRC->SetPixelShader( bNoDepthRender ? psG3DiffuseTexLMNoShadows : psG3DiffuseTexLM );
			pRC->SetPSConst( 4, CVec4( 0,0,0, 124/256.0f ) );
		}
		pRC->SetVertexShader( bNoDepthRender ? vsG3DiffuseTex2NoShadows : vsG3DiffuseTex2 );
		SetupPShadows( pRC, p1.pFastInfo, lightInfo );
		pRC->SetVSConst( 31, CVec4( 1.0f/65536, 0.5f, 0, 0 ) );
		pRC->SetPSConst( 1, -lightInfo.vAmbientColor );
		pRC->SetPSConst( 2, p1.pFastInfo->vColor );
		pRC->SetTexture( bNoDepthRender ? 1 : 3, p1.pFastInfo->pLM, NGfx::FILTER_BEST );
		//pRC->SetTexture( 2, p1.pFastInfo->pLM, NGfx::FILTER_BEST );
		break;
	case RO_G3_DIFFUSE_TEX_DETAIL:
		if ( bUseG5 )
		{
			pRC->SetPixelShader( psG5DiffuseTexDetail );
			SetG5Props( pRC );
		}
		else
		{
			pRC->SetPixelShader( psG3DiffuseTexDetail );
			pRC->SetPSConst( 4, CVec4( 0,0,0, 124/256.0f ) );
		}
		pRC->SetVertexShader( vsG3DiffuseTexDetail );
		pRC->SetVSConst( 32, CVec4( p1.pFastInfo->fSecondUVMult / NGfx::N_VEC_FULL_TEX_SIZE, 0, 0, 0 ) );
		SetupPShadows( pRC, p1.pFastInfo, lightInfo );
		pRC->SetTexture( 3, p1.pFastInfo->pTexture2, NGfx::FILTER_BEST );
		//pRC->SetTexture( 2, p1.pFastInfo->pTexture2, NGfx::FILTER_BEST );
		break;
	case RO_G3_DIFFUSE_TEX_LM_DETAIL:
		if ( bUseG5 )
		{
			pRC->SetPixelShader( psG5DiffuseTexDetailLM );
			SetG5Props( pRC );
		}
		else
		{
			pRC->SetPixelShader( psG3DiffuseTexDetailLM );
			pRC->SetPSConst( 4, CVec4( 0,0,0, 124/256.0f ) );
		}
		pRC->SetVertexShader( vsG3DiffuseTexDetailLM );
		pRC->SetVSConst( 32, CVec4( p1.pFastInfo->fSecondUVMult / NGfx::N_VEC_FULL_TEX_SIZE, 0, 0, 0 ) );
		pRC->SetTexture( 4, p1.pFastInfo->pTexture2, NGfx::FILTER_BEST );
		//pRC->SetTexture( 3, p1.pFastInfo->pTexture2, NGfx::FILTER_BEST );
		SetupPShadows( pRC, p1.pFastInfo, lightInfo );
		pRC->SetVSConst( 31, CVec4( 1.0f/65536, 0.5f, 0, 0 ) );
		pRC->SetPSConst( 1, -lightInfo.vAmbientColor );
		pRC->SetPSConst( 2, p1.pFastInfo->vColor );
		pRC->SetTexture( 3, p1.pFastInfo->pLM, NGfx::FILTER_BEST );
		//pRC->SetTexture( 2, p1.pFastInfo->pLM, NGfx::FILTER_BEST );
		break;
	case RO_GLOSSED_MIRROR:
		pRC->SetPixelShader( psGlossedMirror );
		pRC->SetVertexShader( vsGlossedMirror );
		pRC->SetVSConst( 30, CVec4( lightInfo.fWarFogBlend, 1-lightInfo.fWarFogBlend, 0,0) );
		pRC->SetVSConst( 34, vMirrorParam );
		pRC->SetTexture( 0, p1.pCubeTex );
		pRC->SetTexture( 1, p2.pTex, NGfx::FILTER_BEST );
		break;
	default:
		ASSERT(0);
		return false;
		break;
	}
	return true;
}

void CGenericMaterial::SetTransparentRenderMode( NGfx::CRenderContext *pRC, const SPerPartVariables &vars, 
	const SLightInfo &lightInfo, SRenderPathContext *pRPC ) 
{
	if ( bAddPlaced )
	{
		pRC->SetAlphaCombine( NGfx::COMBINE_ALPHA_ADD );
		pRC->SetFog( bApplyFog ? NGfx::FOG_BLACK : NGfx::FOG_NONE );
	}
	else
	{
		pRC->SetAlphaCombine( NGfx::COMBINE_ALPHA );
		pRC->SetFog( bApplyFog ? NGfx::FOG_NORMAL : NGfx::FOG_NONE );
	}

	NGfx::CTexture *pTex = 0, *pMirror = 0;
	NGfx::CCubeTexture *pSky = 0;
	CVec4 vDiffuseColor(1,1,1,1);
	AssignTex( &pTex, pDiffuseTex );
	AssignTex( &pSky, pSkyTex );
	AssignTexWhiteDefault( &pMirror, pMirrorTex );
	if ( !pTex )
	{
		pTex = GetWhiteTexture();
		vDiffuseColor = vAvrgTexColor;
	}
	if ( pDiffuseColor )
	{
		pDiffuseColor.Refresh();
		vDiffuseColor = MulPerComp( vDiffuseColor, pDiffuseColor->GetValue() );
	}
	vDiffuseColor *= vars.fFade;

	// to do - account fFade
	if ( pRPC->bTnL )
	{
		pRC->SetPixelShader( psTnLLitTexture );
		//pRC->SetVertexShader( NGfx::TNLVS_VERTEX_COLOR_AND_ALPHA );
		pRC->SetVertexShader( NGfx::TNLVS_VERTEX_COLOR );
		pRC->SetTnlVertexColor( vDiffuseColor );
		pRC->SetTexture( 0, pTex, NGfx::FILTER_BEST );
		pRC->SetAlphaRef( 0 );
	}
	else
	{
		SFastGf3RenderInfo *pInfo = pRPC->fastGF3infos.Alloc();
		pInfo->pPersp = &pRPC->depthInfo;
		pInfo->pDepth = pRPC->pCurrentDepthTexture;
		pInfo->nAlphaTest = 0;
		pInfo->pTexture = pTex;
		pInfo->pTexture2 = 0;
		pInfo->vColor = vDiffuseColor;
		pInfo->pSpecular = 0;
		pInfo->fSpecPower = 0;
		pInfo->bTranslucent = fabs2( vTranslucentColor ) > 0;
		pInfo->pLM = 0;

		float fDielMirror = vMirrorParam.z;
		float fMetalMirror = vMirrorParam.w;
		if ( pSky && ( fDielMirror > 0 || fMetalMirror > 0 ) )
		{
			// per vertex transparency is ignored
			if ( IsUseG5() )
			{
				pRC->SetPixelShader( psG5DiffuseTexMirror );
				SetG5Props( pRC );
			}
			else
			{
				pRC->SetPixelShader( psG3DiffuseTexMirror );
				pRC->SetPSConst( 4, CVec4( 0,0,0, 124/256.0f ) );
			}

			pRC->SetVertexShader( vsG3DiffuseTexMirror );
			SetupPShadows( pRC, pInfo, lightInfo );
			pRC->SetVSConst( 34, CVec4( 0, 0, fDielMirror, fMetalMirror ) );
			pRC->SetPSConst( 0, vDiffuseColor );
			pRC->SetTexture( 3, pSky );
			pRC->SetTexture( 4, pMirror, NGfx::FILTER_BEST );
			//pRC->SetTexture( 2, pSky );
			//pRC->SetTexture( 3, pMirror, NGfx::FILTER_BEST );
		}
		else
		{
			int nROP = RO_G3_TRASPARENT;
			SetRenderMode( pRC, lightInfo, nROP, pInfo, 0.0f );
		}
	}
}

void CGenericMaterial::AddOperations( COpGenContext *pOp, SRenderPathContext *pRPC )
{
	//return;

	CVec4 vDiffuseColor(1,1,1,1);
	NGfx::CTexture *pTex = 0, *pTex2 = 0, *pDetail = 0, *pMirror = 0, *pSpecular = 0;
	NGfx::CCubeTexture *pSky = 0;
	AssignTex( &pTex, pDiffuseTex );
	if ( !pTex )
	{
		pTex = GetWhiteTexture();
		vDiffuseColor = vAvrgTexColor;
	}
	AssignTex( &pDetail, pDetailTex );
	AssignTex( &pTex2, pDiffuseTex2 );
	// account color
	if ( pDiffuseColor )
	{
		pDiffuseColor.Refresh();
		vDiffuseColor = MulPerComp( vDiffuseColor, pDiffuseColor->GetValue() );
	}
	// fill reflection info
	AssignTex( &pSky, pSkyTex );
	if ( pSky )
		AssignTexWhiteDefault( &pMirror, pMirrorTex );
	AssignTex( &pSpecular, pSpecularTex );

	// add rops
	const SRenderFragmentInfo &frag = *pOp->GetCurFragment();
	SFastGf3RenderInfo *pInfo = pRPC->fastGF3infos.Alloc();

	int nFogMode;
	const int nAlphaMode = GetAlphaMode( bAddPlaced, bApplyFog, &nFogMode );
	//const int nUseZBufferMask = GetZBufferMask();

	if ( pRPC->bTnL )
	{
		pInfo->pPersp = 0;
		pInfo->pDepth = 0;
		pInfo->nAlphaTest = bAlphaTest ? 120 : 0;
		pInfo->pTexture = pTex;
		pInfo->pTexture2 = pTex2;
		pInfo->vColor = vDiffuseColor;
		pInfo->bPreferPerVertexTransp = false;

		float fFade = frag.vars.fFade;
		if ( fFade == 1 )
		{
			switch ( mt )
			{
			case NORMAL:
				pOp->AddOperation( RO_TNL_LIT_TEXTURE, 10, nFogMode, 0, pInfo );
				break;
			case DECAL:
			case EXACT_DECAL:
			case DECAL_ZWRITE:
				pInfo->bPreferPerVertexTransp = true;
				pOp->AddOperation( RO_TNL_LIT_TEXTURE, 31 + nPriority, nAlphaMode | GetDecalDepthTest(), 0, pInfo );
				break;
			default:
				break;
			}
		}
		else if ( fFade != 0 )
		{
			switch ( mt )
			{
			case NORMAL:
				pInfo->vColor *= fFade;
				if ( !bAlphaTest )
				{
					pOp->AddOperation( RO_TNL_SOLID_COLOR, 99, ABM_ZERO, 0, &VNULL4 );
					pOp->AddOperation( RO_TNL_LIT_TEXTURE, 100, DPM_EQUAL | nAlphaMode, 0, pInfo );
				}
				else
				{
					pInfo->nAlphaTest = Max( 1, Float2Int( pInfo->nAlphaTest * fFade ) );
					pOp->AddOperation( RO_TNL_LIT_TEXTURE, 100, nAlphaMode, 0, pInfo );
				}
				break;
			case DECAL:
			case EXACT_DECAL:
			case DECAL_ZWRITE:
				pInfo->bPreferPerVertexTransp = true;
				pOp->AddOperation( RO_TNL_LIT_TEXTURE, ( bFPB ? 101 : 31 ) + nPriority, nAlphaMode | GetDecalDepthTest(), 0, pInfo );
				break;
			default:
				break;
			}
		}
		return;
	}

	pInfo->pPersp = &pRPC->depthInfo;
	pInfo->pDepth = pRPC->pCurrentDepthTexture;
	pInfo->nAlphaTest = bAlphaTest ? 120 : 0;
	pInfo->pTexture = pTex;
	pInfo->pTexture2 = pTex2;
	pInfo->vColor = vDiffuseColor;
	pInfo->pSpecular = pSpecular;//GetWhiteTexture();//
	pInfo->fSpecPower = fSpecPower;
	pInfo->bTranslucent = fabs2( vTranslucentColor ) > 0;
	pInfo->pLM = 0;

	float fFade = frag.vars.fFade;
	int rop;
	if ( frag.vars.pLM )
	{
		rop = pInfo->pSpecular ? RO_G3_DIFFUSE_SPEC_LM : RO_G3_DIFFUSE_TEX_LM;
		if ( rop == RO_G3_DIFFUSE_TEX_LM && pDetail )
		{
			rop = RO_G3_DIFFUSE_TEX_LM_DETAIL;
			pInfo->pTexture2 = pDetail;
			pInfo->fSecondUVMult = fDetailScale;
		}
		ASSERT( pInfo->pLM == 0 );
		CDGPtr<CPtrFuncBase<NGfx::CTexture> > pTex = frag.vars.pLM;
		pTex.Refresh();
		pInfo->pLM = pTex->GetValue();
	}
	else
	{
		rop = pInfo->pSpecular ? RO_G3_DIFFUSE_SPEC : RO_G3_DIFFUSE_TEX;
		if ( rop == RO_G3_DIFFUSE_TEX && pDetail )
		{
			rop = RO_G3_DIFFUSE_TEX_DETAIL;
			pInfo->pTexture2 = pDetail;
			pInfo->fSecondUVMult = fDetailScale;
		}
	}
	if ( fFade == 1 )
	{
		switch ( mt )
		{
		case NORMAL:
			pOp->AddOperation( rop, 10, nFogMode, 0, this, pInfo );
			break;
		case DECAL:
		case EXACT_DECAL:
		case DECAL_ZWRITE:
			pOp->AddOperation( rop, 31 + nPriority, nAlphaMode | GetDecalDepthTest(), 0, this, pInfo );
			break;
		default:
			break;
		}
		if ( pSky )
		{
			pOp->AddOperation( RO_GLOSSED_MIRROR, 51, nAlphaMode | DPM_EQUAL, 0, 
				this, pSky, pMirror );
		}
	}
	else if ( fFade != 0 )
	{
		pInfo->vColor *= fFade;
		switch ( mt )
		{
		case NORMAL:
			if ( !bAlphaTest )
			{
				pOp->AddOperation( RO_SOLID_COLOR, 99, ABM_ZERO, 0, &VNULL4 );
				pOp->AddOperation( rop, 100, DPM_EQUAL | nAlphaMode, 0, this, pInfo );
			}
			else
			{
				pInfo->nAlphaTest = Max( 1, Float2Int( pInfo->nAlphaTest * fFade ) );
				pOp->AddOperation( rop, 100, nAlphaMode, 0, this, pInfo );
			}
			break;
		case DECAL:
		case EXACT_DECAL:
		case DECAL_ZWRITE:
			pOp->AddOperation( rop, ( bFPB ?  101 : 31 ) + nPriority, nAlphaMode | GetDecalDepthTest(), 0, this, pInfo );
			break;
		default:
			break;
		}
	}
}

void CGenericMaterial::AddCustomOperation( COpGenContext *pOp, const ERenderOperation &op, unsigned char nPass,
																					int nSBM, char nDestRegister, CRenderCmdList::UParameter p )
{
	AddCommonCustomOperation( pDiffuseTex, pOp, op, nPass, nSBM, nDestRegister, p );
}

// CWaterMaterial

void CWaterMaterial::Precache()
{
	Refresh( &pTex );
	Refresh( &pSecondTex );
}

bool CWaterMaterial::SetRenderMode( NGfx::CRenderContext *pRC, const SLightInfo &lightInfo, int nROP, CRenderCmdList::UParameter p1, CRenderCmdList::UParameter p2 )
{
	const bool bUseG5 = IsUseG5();
	switch ( nROP )
	{
	case RO_G3_DIFFUSE_TEX:
		if ( bUseG5 )
		{
			pRC->SetPixelShader( psG5DiffuseTex );
			SetG5Props( pRC );
		}
		else
			pRC->SetPixelShader( psG3DiffuseTex );
		pRC->SetVertexShader( vsG3DiffuseTex );
		SetupPShadows( pRC, p1.pFastInfo, lightInfo );
		pRC->SetPSConst( 4, CVec4( 0,0,0, 124/256.0f ) );
		break;
	case RO_BLEND2TEX_COLOR:
		pRC->SetPixelShader( psBlend2TexColor );
		pRC->SetVertexShader( vsBlend2TexColor );
		pRC->SetVSConst( 30, CVec4( lightInfo.fWarFogBlend, 1-lightInfo.fWarFogBlend, 0,0) );
		pRC->SetTexture( 0, p1.pFastInfo->pTexture, NGfx::FILTER_BEST );
		pRC->SetTexture( 1, p1.pFastInfo->pTexture2, NGfx::FILTER_BEST );

		break;
	}
	return true;
}

void CWaterMaterial::AddOperations( COpGenContext *pOp, SRenderPathContext *pRPC )
{
	NGfx::CTexture *pTex = 0, *pSecondTex = 0;
	AssignTex( &pTex, this->pTex );
	AssignTex( &pSecondTex, this->pSecondTex );

	SFastGf3RenderInfo *pInfo = pRPC->fastGF3infos.Alloc();

	const int nAlphaMode = GetAlphaMode( bAddPlaced, bApplyFog, 0 );

	if ( pRPC->bTnL )
	{
		pInfo->pPersp = 0;
		pInfo->pDepth = 0;
		pInfo->nAlphaTest = 0;
		pInfo->pTexture = pTex;
		pInfo->pTexture2 = 0;
		pInfo->pSpecular = 0;
		pInfo->fSpecPower = 0;
		pInfo->bTranslucent = false;
		pInfo->pLM = 0;
		pInfo->vColor = CVec4(1,1,1,1);
		pInfo->bPreferPerVertexTransp = true;
		pOp->AddOperation( RO_TNL_LIT_TEXTURE, 31 + nPriority, DPM_TESTONLY | nAlphaMode, 0, pInfo );
		return;
	}

	pInfo->pPersp = &pRPC->depthInfo;
	pInfo->pDepth = pRPC->pCurrentDepthTexture;
	pInfo->nAlphaTest = 0;
	pInfo->pTexture = pTex;
	pInfo->pTexture2 = pSecondTex;
	pInfo->vColor = CVec4(1,1,1,1);
	pInfo->pSpecular = 0;
	pInfo->fSpecPower = 0;
	pInfo->bTranslucent = false;
	pInfo->pLM = 0;
	if ( pSecondTex )
		pOp->AddOperation( RO_BLEND2TEX_COLOR, 31 + nPriority, DPM_TESTONLY | nAlphaMode, 0, this, pInfo );
	else
	{
		pOp->AddOperation( RO_G3_DIFFUSE_TEX, 31 + nPriority, DPM_TESTONLY | nAlphaMode, 0, this, pInfo );
	}
}

void CWaterMaterial::AddCustomOperation( COpGenContext *pOp, const ERenderOperation &op, unsigned char nPass,
																					int nSBM, char nDestRegister, CRenderCmdList::UParameter p )
{
	AddCommonCustomOperation( pTex, pOp, op, nPass, nSBM, nDestRegister, p );
}

// CAnimWaterMaterial

CAnimWaterMaterial::CAnimWaterMaterial( CPtrFuncBase<NGfx::CTexture> *_pTex, CPtrFuncBase<NGfx::CTexture> *_pSecondTex, int _nPriority,
		 CFuncBase<STime> *_pTime, bool _bProjectOnTerrain, int _nNumFramesX, int _nNumFramesY, bool _bApplyFog, bool _bAddPlaced, bool _bDrawHorses ) :
	pTex(_pTex), pSecondTex(_pSecondTex), nPriority(_nPriority), pTime(_pTime), bProjectOnTerrain(_bProjectOnTerrain), nNumFramesX(_nNumFramesX), nNumFramesY(_nNumFramesY), bApplyFog(_bApplyFog), bAddPlaced(_bAddPlaced), bDrawHorses(_bDrawHorses)
{
	fTexScaleX = 1.0f / nNumFramesX;
	fTexScaleY = 1.0f / nNumFramesY;
}

void CAnimWaterMaterial::Precache()
{
	Refresh( &pTex );
	Refresh( &pSecondTex );
}

bool CAnimWaterMaterial::SetRenderMode( NGfx::CRenderContext *pRC, const SLightInfo &lightInfo, int nROP, CRenderCmdList::UParameter p1, CRenderCmdList::UParameter p2 )
{	
	const float F_RADIUS = 0.1f;
	pRC->SetAlphaRef( p1.pFastInfo->nAlphaTest );
	const int nFrame = ( p1.pFastInfo->tTime / 75 ) % ( nNumFramesX * nNumFramesY ) ;
	const float fOffsX = fTexScaleX * ( nFrame % nNumFramesX );
	const float fOffsY = fTexScaleY * ( nFrame / nNumFramesY );

	float fRot = 0.0f;
	fRot = p1.pFastInfo->tTime / 4000.0f;

	float fHorseOffset = p1.pFastInfo->tTime * 0.00015f*0.25f;
	fHorseOffset -= floor( fHorseOffset );

	pRC->SetVSConst( 29, p1.pFastInfo->vColor );
	pRC->SetVSConst( 30, CVec4( lightInfo.fWarFogBlend, 1-lightInfo.fWarFogBlend, 0,0) );
	pRC->SetVSConst( 31, CVec4( fOffsX, fOffsY, fOffsX, fOffsY+fHorseOffset ) );
	pRC->SetVSConst( 32, CVec4( 1, 1, 0.025f, 0.025f ) );
	pRC->SetVSConst( 33, CVec4( fRot, fRot, fRot, fRot ) );
	pRC->SetVSConst( 34, CVec4( 2.0f, -1.0f, 0.5f, F_RADIUS * 2 ) );
	pRC->SetVSConst( 35, lightInfo.vLightPos );
	pRC->SetVSConst( 36, CVec4( 1.0f / NGfx::N_VEC_FULL_TEX_SIZE, 1.0f / NGfx::N_VEC_FULL_TEX_SIZE, 0.01f, 1.000f ) );

	const float fVertOffset = ( pTime == 0 ) ? ( F_RADIUS * 2 ) : 0.0f;
	pRC->SetVSConst( 37, CVec4( 4.0f, -1.0f, fVertOffset, 0.0f ) );

	// Main texture
	pRC->SetTexture( 0, p1.pFastInfo->pTexture, NGfx::FILTER_BEST );

	if ( bDrawHorses && p1.pFastInfo->pTexture2 )
	{
		pRC->SetPixelShader( psG3AnimWater );
		pRC->SetVertexShader( vsG3AnimWater );

		// Building horses		
		const float fCurTime = p1.pFastInfo->tTime * 0.00003f;

		const float fInvPeriod = 1.0f/0.3f;
		float fPhaseOffset = fCurTime*fInvPeriod;
		fPhaseOffset = fPhaseOffset - (int)fPhaseOffset;

		const float fWaveAmplitude = 1.4f;

		const float fMinHeight = 0.3f;
		const float fMaxHeight = 1.0f;
		const float fHeightCoeff = 1.0f / ( fMaxHeight - fMinHeight );
		pRC->SetVSConst( 38, CVec4( fPhaseOffset, 24*2*fWaveAmplitude, fHeightCoeff, -fMinHeight*fHeightCoeff ) );
		pRC->SetVSConst( 39, CVec4( 1.0f, 0, 0, 0 ) );

		pRC->SetVSConst( 40, CVec4( 0.7f, -0.3f*0.7f, 1.5, 0 ) );

		// Horses texture
		pRC->SetTexture( 1, p1.pFastInfo->pTexture2, NGfx::FILTER_BEST );
	}
	else
	{
		pRC->SetPixelShader( psG3AnimWaterWithoutHorse );
		pRC->SetVertexShader( vsG3AnimWaterWithoutHorse );
	}

	return true;
}

void CAnimWaterMaterial::AddOperations( COpGenContext *pOp, SRenderPathContext *pRPC )
{
	NGfx::CTexture *pTex = 0, *pSecondTex = 0;
	AssignTex( &pTex, this->pTex );
	AssignTex( &pSecondTex, this->pSecondTex );
	STime tTime = 0;
	if ( pTime )
	{
		pTime.Refresh();
		tTime = pTime->GetValue();
	}

	SFastGf3RenderInfo *pInfo = pRPC->fastGF3infos.Alloc();
	const int nAlphaMode = GetAlphaMode( bAddPlaced, bApplyFog, 0 );

	if ( pRPC->bTnL )
	{
		pInfo->pPersp = 0;
		pInfo->pDepth = 0;
		pInfo->nAlphaTest = 0;
		pInfo->pTexture = pTex;
		pInfo->pTexture2 = 0;
		pInfo->pSpecular = 0;
		pInfo->fSpecPower = 0;
		pInfo->bTranslucent = false;
		pInfo->pLM = 0;
		pInfo->vColor = CVec4(1,1,1,1);
		pInfo->tTime = tTime;
		pInfo->bPreferPerVertexTransp = true;

		const int nFrame = ( tTime / 75 ) % ( nNumFramesX * nNumFramesY ) ;
		const float fOffsX = fTexScaleX * ( nFrame % nNumFramesX );
		const float fOffsY = fTexScaleY * ( nFrame / nNumFramesY );

		CVec3 &vShift = *pRPC->vec3Pool.Alloc();
		vShift = CVec3( fOffsX, fOffsY, 0 );

		pOp->AddOperation( RO_TNL_SLIDING_TEXTURE, 31 + nPriority, DPM_TESTONLY | nAlphaMode, 0, pInfo, &vShift );
		return;
	}

	pInfo->pPersp = &pRPC->depthInfo;
	pInfo->pDepth = pRPC->pCurrentDepthTexture;
	pInfo->nAlphaTest = 0;
	pInfo->pTexture = pTex;
	pInfo->pTexture2 = pSecondTex;
	pInfo->pSpecular = 0;
	pInfo->fSpecPower = 0;
	pInfo->bTranslucent = false;
	pInfo->pLM = 0;
	pInfo->vColor = CVec4(1,1,1,1);
	pInfo->tTime = tTime;
	pOp->AddOperation( RO_USER, 31 + nPriority, DPM_TESTONLY | nAlphaMode, 0, this, pInfo );
/*
	NGfx::CTexture *pTex = 0, *pSecondTex = 0;
	AssignTex( &pTex, this->pTex );
	AssignTex( &pSecondTex, this->pSecondTex );
	STime tTime = 0;
	if ( pTime )
	{
		pTime.Refresh();
		tTime = pTime->GetValue();
	}

	SFastGf3RenderInfo *pInfo = pRPC->fastGF3infos.Alloc();
	const int nAlphaMode = GetAlphaMode( bAddPlaced, bApplyFog, 0 );

	if ( pRPC->bTnL )
	{
		pInfo->pPersp = 0;
		pInfo->pDepth = 0;
		pInfo->nAlphaTest = 0;
		pInfo->pTexture = pSecondTex;
		pInfo->pTexture2 = 0;
		pInfo->pSpecular = 0;
		pInfo->fSpecPower = 0;
		pInfo->bTranslucent = false;
		pInfo->pLM = 0;
		pInfo->vColor = CVec4(1,1,1,1);
		pInfo->tTime = tTime;
		pInfo->bPreferPerVertexTransp = true;
		pOp->AddOperation( RO_TNL_LIT_TEXTURE, 31 + nPriority, DPM_TESTONLY | nAlphaMode, 0, pInfo );
		return;
	}

	pInfo->pPersp = &pRPC->depthInfo;
	pInfo->pDepth = pRPC->pCurrentDepthTexture;
	pInfo->nAlphaTest = 0;
	pInfo->pTexture = pTex;
	pInfo->pTexture2 = pSecondTex;
	pInfo->pSpecular = 0;
	pInfo->fSpecPower = 0;
	pInfo->bTranslucent = false;
	pInfo->pLM = 0;
	pInfo->vColor = CVec4( 0.196f , 0.75f, 0.75f, 1);
	pInfo->vColor = CVec4(1,1,1,1);
	pInfo->tTime = tTime;
	pOp->AddOperation( RO_USER, 31 + nPriority, DPM_TESTONLY | nAlphaMode, 0, this, pInfo );
*/
}

void CAnimWaterMaterial::AddCustomOperation( COpGenContext *pOp, const ERenderOperation &op, unsigned char nPass,
																					int nSBM, char nDestRegister, CRenderCmdList::UParameter p )
{
	AddCommonCustomOperation( pTex, pOp, op, nPass, nSBM, nDestRegister, p );
}

// CSurfMaterial

void CSurfMaterial::Precache()
{
	Refresh( &pTex );
	Refresh( &pSecondTex );
}

bool CSurfMaterial::SetRenderMode( NGfx::CRenderContext *pRC, const SLightInfo &lightInfo, int nROP, CRenderCmdList::UParameter p1, CRenderCmdList::UParameter p2 )
{
	/*
	if ( NGfx::GetHardwareLevel() == NGfx::HL_R300 )
	{
		pRC->SetPixelShader( psG5DiffuseTex );
		SetG5Props( pRC );
	}
	else*/
	pRC->SetPixelShader( psG3DiffuseTex );
	pRC->SetVertexShader( vsG3Surf );
	SetupPShadows( pRC, p1.pFastInfo, lightInfo );
	//const float fTrans = (float)GetTickCount() / 8000.0f;
	float fTrans = 0.0f;
	fTrans = p1.pFastInfo->tTime / 6000.0f;
	//pRC->SetVSConst( 31, CVec4( 1.0f/65536, 0.5f, 0, 0 ) );
	pRC->SetVSConst( 30, CVec4( lightInfo.fWarFogBlend, 1-lightInfo.fWarFogBlend, 0,0) );
	pRC->SetVSConst( 31, CVec4( 1.0f / NGfx::N_VEC_FULL_TEX_SIZE, 0, 0, 0 ) );
	pRC->SetVSConst( 32, CVec4( fTrans, fTrans, fTrans, 0.5f ) );
	pRC->SetVSConst( 33, CVec4( 2, 1.0f, 0.5f, 1.5f ) );
	pRC->SetPSConst( 4, CVec4( 0,0,0, 124/256.0f ) );
	pRC->SetPSConst( 3, CVec4( 0,0,0, 0.5 ) );
	//pRC->SetTexture( 2, p1.pFastInfo->pTexture, NGfx::FILTER_BEST );
	//pRC->SetTexture( 3, p1.pFastInfo->pTexture2, NGfx::FILTER_BEST );
	pRC->SetAlphaRef( p1.pFastInfo->nAlphaTest );
	return true;
}

void CSurfMaterial::AddOperations( COpGenContext *pOp, SRenderPathContext *pRPC )
{
	if ( pRPC->bTnL )
	{
		// do nothing - no surfs on GF2
		return;
	}
	NGfx::CTexture *pTex = 0, *pSecondTex = 0;
	AssignTex( &pTex, this->pTex );
	AssignTex( &pSecondTex, this->pSecondTex );
	STime tTime = 0;
	if ( pTime )
	{
		pTime.Refresh();
		tTime = pTime->GetValue();
	}

	SFastGf3RenderInfo *pInfo = pRPC->fastGF3infos.Alloc();
	const int nAlphaMode = GetAlphaMode( bAddPlaced, bApplyFog, 0 );

	pInfo->pPersp = &pRPC->depthInfo;
	pInfo->pDepth = pRPC->pCurrentDepthTexture;
	pInfo->nAlphaTest = 0;
	pInfo->pTexture = pTex;
	pInfo->pTexture2 = pSecondTex;
	pInfo->pSpecular = 0;
	pInfo->fSpecPower = 0;
	pInfo->bTranslucent = false;
	pInfo->pLM = 0;
	pInfo->vColor = CVec4(1,1,1,1);
	pInfo->tTime = tTime;
	pOp->AddOperation( RO_USER, 31 + nPriority, DPM_TESTONLY | nAlphaMode, 0, this, pInfo );
}

void CSurfMaterial::AddCustomOperation( COpGenContext *pOp, const ERenderOperation &op, unsigned char nPass,
																					int nSBM, char nDestRegister, CRenderCmdList::UParameter p )
{
	AddCommonCustomOperation( pTex, pOp, op, nPass, nSBM, nDestRegister, p );
}

// CReflectWaterMaterial

void CReflectWaterMaterial::Precache()
{
	Refresh( &pTex );
}

// IT WILL BE DELETED SOON - just for fast calibration parameters
extern float s_fWaterAmplitude;
extern float s_fWaterWaveLength;
extern float s_fWaterWaveFrequence;


bool CReflectWaterMaterial::SetRenderMode( NGfx::CRenderContext *pRC, const SLightInfo &lightInfo, int nROP, CRenderCmdList::UParameter p1, CRenderCmdList::UParameter p2 )
{
	const bool bUseG5 = IsUseG5();
	switch ( nROP )
	{
	case RO_WITHOUT_REFLECTION:
		pRC->SetPixelShader( psG3DiffuseTex );
		pRC->SetVertexShader( vsG3DiffuseTex );
		SetupPShadows( pRC, p1.pFastInfo, lightInfo );
		pRC->SetPSConst( 4, CVec4( 0,0,0, 124/256.0f ) );
		break;

	case RO_WITH_REFLECTION:
		

		//pRC->SetVSConst( 30, CVec4( lightInfo.fWarFogBlend, 1-lightInfo.fWarFogBlend, 0,0) );

		NGfx::CTexture *pTexB = 0;
		NGfx::CTexture *pTexC = 0;
		NGfx::CCubeTexture *pSkyC = 0;


		AssignTex( &pTexB, pBump );
		AssignTex( &pTexC, pTex );
		AssignTex( &pSkyC, pSky );


		if (  true ) // fMetalMirror < 0.01f )
		{
			pRC->SetPixelShader( psReflectionWater );
			pRC->SetVertexShader( vsReflectionWater );
			pRC->SetTexture( 0, pTexB, NGfx::FILTER_BEST );
			pRC->SetTexture( 2, p1.pFastInfo->pTexture2, NGfx::FILTER_BEST );
			pRC->SetTexture( 3, pTexC, NGfx::FILTER_BEST );

			pRC->SetVSConst( 35, CVec4( 0.1f, 0, 1, 1 ) );
			pRC->SetVSConst( 36, CVec4( 0, 0.1f, 1, 1 ) );

		}
		else
		{
			pRC->SetPixelShader( psReflectionWaterCube );
			pRC->SetVertexShader( vsReflectionWaterCube );
			pRC->SetTexture( 0, pTexB, NGfx::FILTER_BEST );
			pRC->SetTexture( 3, pSkyC );

			pRC->SetVSConst( 35, CVec4( 1, 0, 0, 0 ) );
			pRC->SetVSConst( 36, CVec4( 0, 1, 0, 0 ) );
			pRC->SetVSConst( 37, CVec4( 0, 0, 1, 0 ) );
			pRC->SetVSConst( 38, CVec4( 0, 0, 0, 0.1f ) );

		}


		float fOffset = GetTickCount() / s_fWaterWaveFrequence;

		const float fAmplitude = s_fWaterAmplitude;
		const float fWaveLength = s_fWaterWaveLength;

		pRC->SetVSConst( 30, CVec4( fWaveLength, fWaveLength, fOffset, fOffset) );
		//pRC->SetVSConst( 31, CVec4( fAmplitude, fAmplitude, 0.1f, 0.1f) );
		//pRC->SetVSConst( 32, CVec4( 1.0f/FP_SQRT_2, 0.03f, 0.1f, 0.1f) );

		const int nNumFramesX = 4;
		const int nNumFramesY = 4;

		const float fTexScaleX = 1.0f / 4.0f;
		const float fTexScaleY = 1.0f / 4.0f;


		const int nFrame = ( (int) GetTickCount() / 75 ) % ( nNumFramesX * nNumFramesY ) ;
		const float fOffsX = fTexScaleX * ( nFrame % nNumFramesX );
		const float fOffsY = fTexScaleY * ( nFrame / nNumFramesY );


		pRC->SetVSConst( 31, CVec4( fOffsX, fOffsY, fAmplitude, fAmplitude ) );
		pRC->SetVSConst( 32, CVec4( fTexScaleX, fTexScaleY, 0.025f, 0.025f ) );

		pRC->SetVSConst( 34, CVec4( 0, 0, fDielMirror, fMetalMirror ) );

	


		pRC->SetPSConst( 0, CVec4( 0.0f, 0.0f, 0.0f, 0.5f) );


		break;
	}
	return true;
}

void CReflectWaterMaterial::AddOperations( COpGenContext *pOp, SRenderPathContext *pRPC )
{
	NGfx::CTexture *pTex = 0, *pSecondTex = 0;
	AssignTex( &pTex, this->pTex );
	pSecondTex = pRPC->pCurrentWaterReflectionTexture;

	SFastGf3RenderInfo *pInfo = pRPC->fastGF3infos.Alloc();

	const int nAlphaMode = GetAlphaMode( bAddPlaced, bApplyFog, 0 );
	//nAlphaMode = ABM_ALPHA_BLEND;

	if ( pRPC->bTnL )
	{
		pInfo->pPersp = 0;
		pInfo->pDepth = 0;
		pInfo->nAlphaTest = 0;
		pInfo->pTexture = pTex;
		pInfo->pTexture2 = 0;
		pInfo->pSpecular = 0;
		pInfo->fSpecPower = 0;
		pInfo->bTranslucent = false;
		pInfo->pLM = 0;
		pInfo->vColor = CVec4(1,1,1,1);
		pInfo->bPreferPerVertexTransp = true;
		pOp->AddOperation( RO_TNL_LIT_TEXTURE, 31 + nPriority, DPM_TESTONLY | nAlphaMode, 0, pInfo );
		return;
	}

	pInfo->pPersp = &pRPC->depthInfo;
	pInfo->pDepth = pRPC->pCurrentDepthTexture;
	pInfo->nAlphaTest = 0;
	pInfo->pTexture = pTex;
	pInfo->pTexture2 = pSecondTex;
	pInfo->vColor = CVec4(1,1,1,1);
	pInfo->pSpecular = 0;
	pInfo->fSpecPower = 0;
	pInfo->bTranslucent = false;
	pInfo->pLM = 0;
	if ( pSecondTex )
		pOp->AddOperation( RO_WITH_REFLECTION, 31 + nPriority, DPM_TESTONLY | nAlphaMode, 0, this, pInfo );
	else
	{
		pOp->AddOperation( RO_WITHOUT_REFLECTION, 31 + nPriority, DPM_TESTONLY | nAlphaMode, 0, this, pInfo );
	}
}

void CReflectWaterMaterial::AddCustomOperation( COpGenContext *pOp, const ERenderOperation &op, unsigned char nPass,
																				int nSBM, char nDestRegister, CRenderCmdList::UParameter p )
{
	AddCommonCustomOperation( pTex, pOp, op, nPass, nSBM, nDestRegister, p );
}

// CTracksMaterial

void CTracksMaterial::Precache()
{
	Refresh( &pTex );
}

bool CTracksMaterial::SetRenderMode( NGfx::CRenderContext *pRC, const SLightInfo &lightInfo, int nROP, CRenderCmdList::UParameter p1, CRenderCmdList::UParameter p2 )
{
	if ( NGfx::IsTnLDevice() )
	{
		pRC->SetPixelShader( psTexturePlusDiffuse );
		pRC->SetVertexShader( NGfx::TNLVS_NONE );
	}
	else
	{
		pRC->SetPixelShader( psTexturePlusDiffuse );
		pRC->SetVertexShader( vsTracks );
	}
	pRC->SetTexture( 0, p1.pTex, NGfx::FILTER_BEST );
	return true;
}

void CTracksMaterial::AddOperations( COpGenContext *pOp, SRenderPathContext *pRPC )
{
	NGfx::CTexture *pTex = 0;
	AssignTexBlackDefault( &pTex, this->pTex );
	const int nDisableFog = bApplyFog ? FOG_NORMAL : FOG_NONE;
	// works for both TnL & normal
	pOp->AddOperation( RO_USER, 31 + nPriority, DPM_TESTONLY | ABM_MUL | nDisableFog, 0, this, pTex );
}

void CTracksMaterial::AddCustomOperation( COpGenContext *pOp, const ERenderOperation &op, unsigned char nPass,
																					int nSBM, char nDestRegister, CRenderCmdList::UParameter p )
{
	AddCommonCustomOperation( pTex, pOp, op, nPass, nSBM, nDestRegister, p );
}

// CTerrainMaterial

void CTerrainMaterial::Precache()
{
	Refresh( &pTex );
	Refresh( &pMask );
}

bool CTerrainMaterial::SetRenderMode( NGfx::CRenderContext *pRC, const SLightInfo &lightInfo, int nROP, CRenderCmdList::UParameter p1, CRenderCmdList::UParameter p2 )
{
	const bool bUseG5 = IsUseG5();

	switch ( nROP )
	{
	case RO_TERRAIN:
		if ( p1.pFastInfo->pTexture2 )
		{
			if ( bUseG5 )
			{
				pRC->SetPixelShader( psG5DiffuseTex2Terrain );
				SetG5Props( pRC );
			}
			else
        pRC->SetPixelShader( psG3DiffuseTex2Terrain );
			pRC->SetVertexShader( vsG3DiffuseTex2Terrain );
		}
		else
		{
			if ( bUseG5 )
			{
				pRC->SetPixelShader( psG5DiffuseTexTerrain );
				SetG5Props( pRC );
			}
			else
				pRC->SetPixelShader( psG3DiffuseTexTerrain );
			//pRC->SetPixelShader( psDiffuseAlpha );
			pRC->SetVertexShader( vsG3DiffuseTexTerrain );
		}
		SetupPShadows( pRC, p1.pFastInfo, lightInfo );
		pRC->SetPSConst( 4, CVec4( 0,0,0, 124/256.0f ) );
		pRC->SetPSConst( 3, CVec4( 0,0,0, -0.375 ) );
		if ( p1.pFastInfo->pTexture2 )
			pRC->SetTexture( 3, p1.pFastInfo->pTexture2, NGfx::FILTER_BEST );
			//pRC->SetTexture( 2, p1.pFastInfo->pTexture2, NGfx::FILTER_BEST );
		break;
	case RO_TERRAIN_GF2:
		if ( p1.pFastInfo->pTexture2 )
			pRC->SetPixelShader( psTnLLitTexture2Terrain );
		else
			pRC->SetPixelShader( psTnLLitTextureTerrain );
		pRC->SetVertexShader( NGfx::TNLVS_VERTEX_COLOR_AND_ALPHA );
		pRC->SetTnlVertexColor( p1.pFastInfo->vColor );
		pRC->SetTexture( 0, p1.pFastInfo->pTexture, NGfx::FILTER_BEST );
		if ( p1.pFastInfo->pTexture2 )
			pRC->SetTexture( 1, p1.pFastInfo->pTexture2, NGfx::FILTER_BEST );
		pRC->SetAlphaRef( p1.pFastInfo->nAlphaTest );
		break;
	}
	return true;
}

void CTerrainMaterial::AddOperations( COpGenContext *pOp, SRenderPathContext *pRPC )
{
	NGfx::CTexture *pTex = 0, *pMask = 0;
	AssignTexBlackDefault( &pTex, this->pTex );
	AssignTex( &pMask, this->pMask );

	SFastGf3RenderInfo *pInfo = pRPC->fastGF3infos.Alloc();
	int nFogMode;
	const int nAlphaMode = GetAlphaMode( bAddPlaced, bApplyFog, &nFogMode );

	if ( pRPC->bTnL )
	{
		pInfo->pPersp = 0;
		pInfo->pDepth = 0;
		pInfo->nAlphaTest = 140;
		pInfo->pTexture = pTex;
		pInfo->pTexture2 = pMask;
		pInfo->vColor = CVec4(1,1,1,1);
		pInfo->bPreferPerVertexTransp = true;
		pOp->AddOperation( RO_TERRAIN_GF2, 11 + nPriority, DPM_NORMAL | nFogMode, 0, this, pInfo );
		return;
	}
	
	pInfo->pPersp = &pRPC->depthInfo;
	pInfo->pDepth = pRPC->pCurrentDepthTexture;
	pInfo->nAlphaTest = 1;//40;
	pInfo->pTexture = pTex;
	pInfo->pTexture2 = pMask;
	pInfo->vColor = CVec4(1,1,1,1);
	pOp->AddOperation( RO_TERRAIN, 11 + nPriority, DPM_NORMAL | nAlphaMode, 0, this, pInfo );
}

void CTerrainMaterial::AddCustomOperation( COpGenContext *pOp, const ERenderOperation &op, unsigned char nPass,
																					int nSBM, char nDestRegister, CRenderCmdList::UParameter p )
{
	AddCommonCustomOperation( pTex, pOp, op, nPass, nSBM, nDestRegister, p );
}

// CCloudsH5Material

void CCloudsH5Material::Precache()
{
	Refresh( &pClouds );
}

CVec3 CCloudsH5Material::CalcShift()
{
	float fShift = GetTickCount() / ( 1024.0f ) * fWrapsPerSecond;
	fShift = fShift - floor( fShift );
	return CVec3( 0, fShift, 0 );
}

bool CCloudsH5Material::SetRenderMode( NGfx::CRenderContext *pRC, const SLightInfo &lightInfo, int nROP, CRenderCmdList::UParameter p1, CRenderCmdList::UParameter p2 )
{
	CVec3 vShift = CalcShift();
	//pRC->SetPixelShader( psTextureTFactor );
	pRC->SetPixelShader( psClouds );
	pRC->SetVertexShader( vsClouds );
	pRC->SetVSConst( 25, CVec3( vShift.x, vShift.y, 1.0f / NGfx::N_VEC_FULL_TEX_SIZE ) );
	pRC->SetPSConst( 0, p2.f * CVec4(1,1,1,1) );
	pRC->SetTexture( 0, p1.pTex, NGfx::FILTER_BEST );
	return true;
}

void CCloudsH5Material::AddOperations( COpGenContext *pOp, SRenderPathContext *pRPC )
{
	float fFade = pOp->GetCurFragment()->vars.fFade;
	NGfx::CTexture *pClouds = 0;
	AssignTexBlackDefault( &pClouds, this->pClouds );


	int nFogMode;
	const int nAlphaMode = GetAlphaMode( bAddPlaced, bApplyFog, &nFogMode );

	if ( pRPC->bTnL )
	{
		CVec3 &vShift = *pRPC->vec3Pool.Alloc();
		vShift = CalcShift();
		// RO_TNL_SLIDING_TEXTURE - has changed its interface. For resurection of this material you need to pass FastInfo
		//pOp->AddOperation( RO_TNL_SLIDING_TEXTURE, 31 + nPriority, nAlphaMode | DPM_TESTONLY, 0, pClouds, &vShift, fFade );
		return;
	}
	pOp->AddOperation( RO_USER, 31 + nPriority, nAlphaMode | DPM_TESTONLY, 0, this, pClouds, fFade );
}

void CCloudsH5Material::AddCustomOperation( COpGenContext *pOp, const ERenderOperation &op, unsigned char nPass,
																					int nSBM, char nDestRegister, CRenderCmdList::UParameter p )
{
	AddCommonCustomOperation( pClouds, pOp, op, nPass, nSBM, nDestRegister, p );
}

// CSimpleSkyMaterial

void CSimpleSkyMaterial::Precache()
{
	Refresh( &pTex );
}

bool CSimpleSkyMaterial::SetRenderMode( NGfx::CRenderContext *pRC, const SLightInfo &lightInfo, int nROP, CRenderCmdList::UParameter p1, CRenderCmdList::UParameter p2 )
{
	const float _fFOVZoom = ( fFOVZoom > EPS_VALUE ) ? fFOVZoom : 1.0f;
	pRC->SetVSConst( 30, CVec4( _fFOVZoom, _fFOVZoom, _fFOVZoom, _fFOVZoom ) );
	pRC->SetVSConst( 31, CVec4( _fFOVZoom, _fFOVZoom, _fFOVZoom, _fFOVZoom ) );
	pRC->SetPixelShader( psTextureCopyAlpha );
	//pRC->SetVertexShader( vsTexture );
	pRC->SetVertexShader( vsSimpleSky );
	pRC->SetTexture( 0, p1.pTex, NGfx::FILTER_BEST );
	return true;
}

void CSimpleSkyMaterial::AddOperations( COpGenContext *pOp, SRenderPathContext *pRPC )
{
	NGfx::CTexture *pClouds = 0;
	AssignTexBlackDefault( &pClouds, this->pTex );
	const int nAlphaMode = GetAlphaMode( bAddPlaced, bApplyFog, 0 );
	const int nZBufferMode = DPM_NONE;

	if ( pRPC->bTnL )
	{
		pOp->AddOperation( RO_TNL_TEXTURE, 0, nAlphaMode|nZBufferMode, 0, pClouds );
		return;
	}
	pOp->AddOperation( RO_USER, 0, nAlphaMode|nZBufferMode, 0, this, pClouds );
}

void CSimpleSkyMaterial::AddCustomOperation( COpGenContext *pOp, const ERenderOperation &op, unsigned char nPass,
																					 int nSBM, char nDestRegister, CRenderCmdList::UParameter p )
{
	AddCommonCustomOperation( pTex, pOp, op, nPass, nSBM, nDestRegister, p );
}

IMaterial* GetExactDecal( IMaterial *_p )
{
	if ( CDynamicCast<CGenericMaterial> p = _p )
		return p->GetExactDecal();
	return 0;
}

IMaterial* CreateMaterial( const SMaterialCreateInfo &m )
{
	CObj<CObjectBase> pHold1(m.pBump), pHold3(m.pMirrorTexture);
	int nType = m.alphaMode & MF_EFFECT_MASK;
	switch ( nType )
	{
	case MF_GENERIC:
		{
			CGenericMaterial *pRes = new CGenericMaterial( m.bDoesCastShadow, m.bBackFaceCastShadow );
			if ( m.pColor )
				pRes->SetDiffuseColor( m.pColor );
			//const_cast<CVec3&>(vGlossVolor) = CVec3(1,1,1);
			//fSpecPower = 1;
			if ( m.pTexture )
				pRes->SetDiffuseTex( m.pTexture, m.vAvrgTexColor );
			pRes->SetSelfIllum( m.bSelfIllum );
			pRes->Set2Sided( m.b2Sided );
			pRes->SetFogApplying( m.bApplyFog );
			pRes->SetAddPlacing( m.bAddPlaced );
			pRes->SetUsageZBuffer( m.bIgnoreZ );
			pRes->SetReceiveShadows ( m.bReceiveShadow );
			pRes->SetAngVel( m.fFloatParam );
			pRes->SetDynamicType( m.eDynamicType );
		
			switch ( m.alphaMode & MF_ALPHA_MASK )
			{
			case MF_OPAQUE:         break;
			case MF_DECAL:          pRes->SetExactDecal(); break;
			case MF_OVERLAY:        pRes->SetDecal(); break;
			case MF_OVERLAY_ZWRITE: pRes->SetDecalZWrite(); break;
			case MF_TRANSPARENT:    
				if ( ( m.fMetalMirror != 0 || m.fDielMirror != 0 ) && NGfx::GetHardwareLevel() < NGfx::HL_R300 )
					pRes->SetDecalZWrite();
				else
					pRes->SetTransparent(); 
				break;
			case MF_ALPHA_TEST:     pRes->SetAlphaTest( true ); break;
			}
			if ( m.pBump )
				pRes->SetDiffuseTex2( m.pBump );
			if ( m.pDetail )
				pRes->SetDetail( m.pDetail, m.fDetailScale );
			pRes->SetPriority( m.nPriority );
			if ( m.pSpecular )
				pRes->SetSpecular( m.pSpecular, m.fSpecPower );

			// fill reflection info
			//fDielMirror = 1;
			//fMetalMirror = 1;
			//ASSERT ( m.pSky == 0 )
	

			pRes->SetReflectionInfo( m.pSky, m.pMirrorTexture, m.fDielMirror, m.fMetalMirror );
			pRes->SetTranslucentColor( m.vTranslucentColor );
			pRes->SetProjectOnTerrain( m.bProjectOnTerrain );
			pRes->Check();
			return pRes;
		}
	case MF_WATER:
		return new CWaterMaterial( m.pTexture, m.pBump, m.nPriority, m.bApplyFog, m.bAddPlaced );
	case MF_TRACKS:
		return new CTracksMaterial( m.pTexture, m.nPriority, m.bApplyFog, m.bAddPlaced );
	case MF_TERRAIN:
		return new CTerrainMaterial( m.pTexture, m.pBump, m.nPriority, m.bApplyFog, m.bAddPlaced, m.bDoesCastShadow, m.bBackFaceCastShadow );
	case MF_CLOUDS_H5:
		return new CCloudsH5Material( m.pTexture, m.nPriority, m.fFloatParam, m.bApplyFog, m.bAddPlaced, m.bDoesCastShadow, m.bProjectOnTerrain );
	case MF_ANIM_WATER:
		return new CAnimWaterMaterial( m.pTexture, m.pBump, m.nPriority, 0, m.bProjectOnTerrain, 4, 4, m.bApplyFog, m.bAddPlaced, false );
	case MF_SURF:
		return new CSurfMaterial( m.pTexture, m.pBump, m.nPriority, 0, m.bApplyFog, m.bAddPlaced );
	case MF_SIMPLE_SKY:
		return new CSimpleSkyMaterial( m.pTexture, m.nPriority, m.bApplyFog, m.fFloatParam, m.bAddPlaced );
	case MF_REFLECT_WATER:
		return new CReflectWaterMaterial( m.pTexture, m.pBump, m.nPriority, m.bApplyFog, m.bAddPlaced, m.fDielMirror, m.fMetalMirror, m.pSky, m.fFloatParam );
	default:
		ASSERT(0);
		break;
	}
	return 0;
}

IMaterial *AttachColor( IMaterial *pSrc, CFuncBase<CVec4> *pColor )
{
	CDynamicCast<CGenericMaterial> pMat( pSrc );
	if ( !pMat )
		return 0;
	CGenericMaterial *pRes = pMat->Duplicate();
	pRes->SetDiffuseColor( pColor );
	return pRes;
}

}

START_REGISTER(GfxMaterials)
REGISTER_VAR_EX( "gfx_fade_priority_boost", NGlobal::VarBoolHandler, &NGScene::bFPB, 0, STORAGE_NONE )
REGISTER_VAR_EX( "gfx_static_shadows", NGlobal::VarBoolHandler, &NGScene::bNewShadows, 0, STORAGE_NONE )
REGISTER_VAR_EX( "shadow_on_trees", NGlobal::VarBoolHandler, &s_bShadowOnTrees, true, STORAGE_USER );
REGISTER_VAR_EX( "test_horse", NGlobal::VarFloatHandler, &s_fTestHorse, true, STORAGE_USER );
FINISH_REGISTER

using namespace NGScene;
BASIC_REGISTER_CLASS( IMaterial )
REGISTER_SAVELOAD_CLASS( 0x02191161, CGenericMaterial )
REGISTER_SAVELOAD_CLASS( 0x020A2170, CWaterMaterial )
REGISTER_SAVELOAD_CLASS( 0x1314D3C0, CAnimWaterMaterial )
REGISTER_SAVELOAD_CLASS( 0x1314E480, CSurfMaterial )
REGISTER_SAVELOAD_CLASS( 0x004c2180, CTracksMaterial )
REGISTER_SAVELOAD_CLASS( 0x200BE340, CTerrainMaterial )
REGISTER_SAVELOAD_CLASS( 0x2014BAC1, CCloudsH5Material )
REGISTER_SAVELOAD_CLASS( 0x131A9B00, CSimpleSkyMaterial )
REGISTER_SAVELOAD_CLASS( 0x341C6B80, CReflectWaterMaterial )
