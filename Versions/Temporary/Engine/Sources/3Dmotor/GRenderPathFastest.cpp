#include "stdafx.h"
#include "GRenderPathFastest.h"
#include "GfxUtils.h"
#include "GRenderFactor.h"
#include "GRenderLight.h"
#include "GScene.h"
#include "GRTShare.h"
#include "GRenderUtils.h"
#include "System/Commands.h"
#include "GTransparent.h"
#include "GRenderClouds.h"
#include "GInit.h"
#include "GfxUtils.h"

#include "Image/ImageTGA.h"

namespace NGfx
{
extern bool bSimpleParticles;
}

namespace NGScene
{
extern bool bWaterReflection;
bool bFreeze = false; 
static CObj<NGfx::CTexture> pCurrentDepthTexture;
static CObj<NGfx::CTexture> pCurrentWaterReflectionTexture;

extern bool bNewShadows;

static CVec3 vParticleLMShadowTestLight[2]; //[0] - Full, [1] - Shadowed
bool bNoDepthRender;
static void RenderParticleShadowTest( COpGenContext &op, const SPerspDirectionalDepthInfo &depthInfo, NGfx::CTexture *pDepthTex )
{
	op.AddOperation( RO_DIR_PARTICLE_LM_SOFT_SHADOW_TEST, 10, ABM_ALPHA_BLEND|DPM_NONE, 0,
		&depthInfo, pDepthTex, vParticleLMShadowTestLight );
}

static void MakeDepthRenderInfo( SNLProjectionInfo *pRes, int nResolution )
{
	//pRes->vInverseScale.x *= 0.5f;
	//pRes->vInverseScale.y *= 0.5f;
	//pRes->vInverseScale.z *= -0.5f;
	//pRes->vInverseScale.w *= -0.5f;
	pRes->vShift.x *= 2;
	pRes->vShift.y *= -2;
	float f = 1 + 1.0f / nResolution;
	pRes->vShift.z = pRes->vShift.z * 2 - f;
	pRes->vShift.w = -(pRes->vShift.w * 2 - f);

	pRes->lvShift = CVec4( 2.0f, -2.0f, -1.0f, 1.0f );


}

static void MakeDepthRenderInfo( SPerspDirectionalDepthInfo *pRes, const SPerspDirectionalDepthInfo &src, int nResolution )
{
	*pRes = src;
	MakeDepthRenderInfo( &pRes->nlp, nResolution );
}

static void MakeShadowDepthCmdList( CRenderCmdList *pRes, CSceneFragments &src, float fMinFade, 
	SPerspDirectionalDepthInfo *pDepthInfo, ERenderOperation normalOp, ERenderOperation atOp )
{
	const std::vector<SRenderFragmentInfo*> &fragments = src.GetFragments();
	for ( int k = 0; k < fragments.size(); ++k )
	{
		if ( src.IsFilteredFragment( k ) )
			continue;
		const SRenderFragmentInfo &f = *fragments[k];
		if ( f.vars.fFade < fMinFade )
			continue;

		COpGenContext fi( &pRes->ops, &f );
		NGfx::CTexture *pATexture = 0;
		
		if ( f.pMaterial == 0 )
			continue;
		
		pATexture = f.pMaterial->GetAlphaTestTex();
		bool reject = !f.pMaterial->DoesCastShadow();

		if ( !pATexture && !reject )
			fi.AddOperation( normalOp, 100, DPM_NORMAL_NOTEQ, 0, pDepthInfo );
	}
}

static void MakeShadowColorCmdList( CRenderCmdList *pRes, CSceneFragments &src, float fMinFade, 
								   SPerspDirectionalDepthInfo *pDepthInfo, ERenderOperation normalOp, ERenderOperation atOp )
{
	const std::vector<SRenderFragmentInfo*> &fragments = src.GetFragments();
	for ( int k = 0; k < fragments.size(); ++k )
	{
		if ( src.IsFilteredFragment( k ) )
			continue;
		const SRenderFragmentInfo &f = *fragments[k];
		if ( f.vars.fFade < fMinFade )
			continue;

		COpGenContext fi( &pRes->ops, &f );
		NGfx::CTexture *pATexture = 0;

		bool reject = false;
		if ( f.pMaterial )
		{
			pATexture = f.pMaterial->GetAlphaTestTex();
			reject =  !f.pMaterial->DoesCastShadow();
		}
		
		if ( pATexture == 0 )
			pATexture = GetWhiteTexture();
		
		if ( !reject )
			fi.AddOperation( normalOp, 100, DPM_NORMAL_NOTEQ, 0, pDepthInfo, pATexture );
	}
}


static CRTPtr pDepthShadow16( "DepthMap16" );
static CRTPtr pDepthShadowCopy( "DepthCopy" );

static void RenderDynamicDepthTexture( NGfx::CTexture *pDepthTex, CSceneFragments &geom, CDirectionalLight *pLight,
							   const SLightInfo &lightInfo, const CTransformStack &sts, int nDepthTexResolution, const SParticleLMRenderTargetInfo &particleLM,
							   SPerspDirectionalDepthInfo *pDepthInfo, IRender *pRender, CSceneFragments &sceneGeom )
{
	NGfx::CRenderContext rc;

	SPerspDirectionalDepthInfo renderDepthInfo;
	MakeDepthRenderInfo( &renderDepthInfo, *pDepthInfo, nDepthTexResolution );
	//MakeSingleOp( &res, geom, true, 0.01f, &renderDepthInfo, RO_DIR_DEPTH, &renderDepthInfo );

	rc.SetTextureRT( pDepthTex );

	//rc.ClearBuffers( 0 );

	rc.SetAlphaCombine( NGfx::COMBINE_NONE );
	rc.SetStencil( NGfx::STENCIL_NONE );
	rc.SetColorWrite( NGfx::COLORWRITE_COLOR );
	rc.SetCulling( NGfx::CULL_NONE );
	rc.SetFog( NGfx::FOG_NONE );

	CRenderCmdList res;
	MakeShadowColorCmdList( &res, geom, 0.01f, &renderDepthInfo, RO_DIR_COLOR , RO_DIR_COLOR );
	Execute( pRender, &rc, sts, res, geom, lightInfo );
}

static void RenderDepthTexture( NGfx::CTexture *pDepthTex, CSceneFragments &geom, CDirectionalLight *pLight,
	const SLightInfo &lightInfo, const CTransformStack &sts, int nDepthTexResolution, const SParticleLMRenderTargetInfo &particleLM,
	SPerspDirectionalDepthInfo *pDepthInfo, IRender *pRender, CSceneFragments &sceneGeom )
{
	NGfx::CRenderContext rc;

	SPerspDirectionalDepthInfo renderDepthInfo;
	MakeDepthRenderInfo( &renderDepthInfo, *pDepthInfo, nDepthTexResolution );
	


	
	if ( !IsUsing16bitShadows() || bNewShadows )
	{
		rc.SetTextureRT( pDepthTex );

		rc.ClearBuffers( 0xFFFFFF );

		CRenderCmdList res;
		MakeShadowDepthCmdList( &res, geom, 0.01f, &renderDepthInfo, RO_DIR_DEPTH, RO_DIR_DEPTH_AT_NLP );
		if( !bNewShadows )
		MakeSingleOp( &res, geom, true, 0.01f, &renderDepthInfo, RO_DIR_DEPTH, &renderDepthInfo );

		Execute( pRender, &rc, sts, res, geom, lightInfo );
	}
	else
	{
		NGfx::CTexture *pDepthTex16 = pDepthShadow16.GetTexture();
		rc.SetTextureRT( pDepthTex16 );

		rc.ClearBuffers( 0xFFFFFF );

		CRenderCmdList res;
	
		SPerspDirectionalDepthInfo renderDepthInfo16 = renderDepthInfo;
		//renderDepthInfo16.vDepth.w -= 4.0 / 256;

		NGfx::SetDithering( NGfx::DITHER_OFF );
		
		MakeShadowDepthCmdList( &res, geom, 0.01f, &renderDepthInfo16, RO_DIR_DEPTH_16, RO_DIR_DEPTH_16_AT_NLP );
		Execute( pRender, &rc, sts, res, geom, lightInfo );

		rc.SetTextureRT( pDepthTex );
		CTRect<float> depthTexSize( 0, 0, nDepthTexResolution, nDepthTexResolution );

		CObj<NGfx::I2DEffect> pEffect = new NGfx::CShadow16toAlphaEffect;
		rc.SetAlphaCombine( NGfx::COMBINE_NONE );
		rc.SetStencil( NGfx::STENCIL_NONE );
		NGfx::CopyTexture( rc, CVec2(nDepthTexResolution,nDepthTexResolution), depthTexSize, pDepthTex16, depthTexSize, CVec4(1,1,1,1), pEffect );
		
		NGfx::SetDithering( NGfx::DITHER_ON );
	}
	

	if ( bNewShadows )
	{
		CRenderCmdList res;
		
		rc.SetAlphaCombine( NGfx::COMBINE_NONE );
		rc.SetStencil( NGfx::STENCIL_NONE );
		rc.SetColorWrite( NGfx::COLORWRITE_COLOR );
		rc.SetCulling( NGfx::CULL_NONE );
		rc.SetFog( NGfx::FOG_NONE );

		MakeShadowColorCmdList( &res, geom, 0.01f, &renderDepthInfo, RO_DIR_COLOR , RO_DIR_COLOR );
		Execute( pRender, &rc, sts, res, geom, lightInfo );
	}
	//DrawBorder( &rc, nDepthTexResolution, nDepthTexResolution );
	else if ( pLight->GetCloudsTexture() )
	{
		SHMatrix cloudProjection;
		pLight->CalcCloudProjection( &cloudProjection );
		RenderClouds( &rc, renderDepthInfo, *pDepthInfo, pLight->GetCloudsTexture(), cloudProjection );
	}

	// particles
	if ( particleLM.pParticleLMs && !NGfx::bSimpleParticles )
	{
		CVec3 vFullAmbient = lightInfo.vAmbientColor + lightInfo.vShadeColor;//pAmbient->GetValue() + MulPerComp( pColor->GetValue(), vShadowColor );
		CVec3 vFullLight = lightInfo.vAmbientColor + lightInfo.vLightColor;
		vParticleLMShadowTestLight[0] = vFullLight;
		vParticleLMShadowTestLight[1] = vFullAmbient;

		NGfx::CRenderContext particleRC;
		particleRC.SetTextureRT( particleLM.pParticleLMs );
		// in soft shadows stencil buffer is not used so there is no need to clear one
		particleRC.ClearTarget( NGfx::GetDWORDColor( CVec4( vFullAmbient, 1 ) ) );

		// calc shadows on particles
		CRenderCmdList cmds;
		{
			COpGenContext op( &cmds.ops, &sceneGeom.GetLitParticles() );
			RenderParticleShadowTest( op, *pDepthInfo, pDepthTex );
		}
		CTransformStack ts;
		ts.Init( particleLM.rootTransform );
		Execute( pRender, &particleRC, ts, cmds, sceneGeom, lightInfo );

		particleRC.SetAlphaCombine( NGfx::COMBINE_NONE );
		{
			CTRect<float> rDest( 0, 0, particleLM.vParticleLMSize.x, particleLM.vParticleLMSize.y );
			// render kernel
			NGfx::C2DQuadsRenderer qr( particleRC, CVec2( rDest.x2, rDest.y2 ), NGfx::QRM_DEPTH_NONE|NGfx::QRM_SOLID );
			CTRect<float> rKernel( 0, 0, particleLM.vKernelSize.x, particleLM.vKernelSize.y );
			qr.AddRect( rKernel, 0, rKernel, NGfx::Get8888Color( CVec4( 0.25f, 0.25f, 0.25f, 1 ) ) );
		}
	}
}

static void FixParticles( const SParticleLMRenderTargetInfo &particleLM, const SLightInfo &lightInfo )
{	
	if ( particleLM.pParticleLMs && !NGfx::bSimpleParticles )
	{	
		NGfx::CRenderContext particleRC;
		particleRC.SetTextureRT( particleLM.pParticleLMs );		
		particleRC.SetAlphaCombine( NGfx::COMBINE_NONE );

		CTRect<float> rDest( 0, 0, particleLM.vParticleLMSize.x, particleLM.vParticleLMSize.y );

		NGfx::C2DQuadsRenderer qr( particleRC, CVec2( rDest.x2, rDest.y2 ), NGfx::QRM_DEPTH_NONE|NGfx::QRM_SOLID );

		qr.AddRect( rDest, 0, rDest, NGfx::Get8888Color( CVec4( 0.25, 0.25, 0.25, 1 ) ) );
	}
}

static void UpdateDepthTexture( CTransformStack *pTS, CTransformStack *pClipTS, IRender *pRender,	const SLightInfo &lightInfo,
	SPerspDirectionalDepthInfo *pDepthInfo, const SBound &sceneBound, int nLightingOptions, CDirectionalLight *pLight,
	const SParticleLMRenderTargetInfo &particleLM, float fMinimalSize, NGfx::CTexture *pDepthTexture, CSceneFragments &sceneGeom )
{
	*pDepthInfo = SPerspDirectionalDepthInfo();

	if ( nLightingOptions & LO_NOSHADOWS )
		return;
	float fSmoothHeight = pLight->GetSmoothedSceneHeight( sceneBound.s.ptCenter.z + sceneBound.ptHalfBox.z );
	CVec3 vShadowsLightDir = pLight->GetShadowsLightDir();

	SShadowMatrixAlign &align = pLight->GetShadowMatrixAlign();
	CTransformStack sts;

	MakeShadowMatrix( &pDepthInfo->nlp, &sts, fMinimalSize, *pClipTS, vShadowsLightDir, pLight->GetMaxHeight(),
		sceneBound, fSmoothHeight, &align );

	pDepthInfo->vDepth = pLight->GetDepth();
	int nDepthTexResolution = GetDepthTexResolution();
	CSceneFragments depthGeom;
	pRender->FormDepthList( &sts, vShadowsLightDir, 
		bNewShadows ? IRender::DT_STATIC : IRender::DT_ALL, &depthGeom );
	
	//MakeSingleOp( &res, geom, true, -1, 0, RO_LP_DEPTH, &depthInfo );

	NGfx::CTexture *pDepthTexCopy = pDepthShadowCopy.GetTexture();
	
	if( pDepthInfo->nlp.bNeedUpdate || !bNewShadows )
		RenderDepthTexture( bNewShadows ? pDepthTexCopy : pDepthTexture, depthGeom, pLight, lightInfo, sts, nDepthTexResolution, particleLM, pDepthInfo, pRender, sceneGeom );


	
	if( bNewShadows )
	{
		NGfx::CRenderContext rc;	
		rc.SetTextureRT( pDepthTexture );
		//rc.ClearBuffers( 0 );
		rc.ClearZBuffer();

		rc.SetAlphaCombine( NGfx::COMBINE_NONE );
		rc.SetStencil( NGfx::STENCIL_NONE );
		rc.SetColorWrite( NGfx::COLORWRITE_ALL );
		rc.SetCulling( NGfx::CULL_NONE );
		rc.SetFog( NGfx::FOG_NONE );

		float c1 = pDepthInfo->nlp.fMaxX - pDepthInfo->nlp.fMinX;
		float c2 = pDepthInfo->nlp.fMinX ;

		float b1 = pDepthInfo->nlp.fMaxY - pDepthInfo->nlp.fMinY;
		float b2 = pDepthInfo->nlp.fMinY ;

		SHMatrix cloudProjection;
		pLight->CalcCloudProjection( &cloudProjection );

		b2 += GetTickCount() * 0.002f;
		c2 += GetTickCount() * 0.001f;

		c1 *= 0.02f;
		b1 *= 0.02f;

		c2 *= 0.02f;
		b2 *= 0.02f;

		CObj<NGfx::I2DEffect> pEffect = new NGfx::CCopyShadowsAndCloudsEffect 
			( pLight->GetCloudsTexture(), CVec4( b1, c1, b2, c2 ) );
		
		CTRect<float> depthTexSize( 0, 0, nDepthTexResolution, nDepthTexResolution );
		NGfx::CopyTexture( rc, 
			CVec2( nDepthTexResolution, nDepthTexResolution ), 
			depthTexSize, 
			pDepthTexCopy, 
			depthTexSize, 
			CVec4(1,1,1,1), 
			pEffect );

		CSceneFragments depthDynamicGeom;
		pRender->FormDepthList( &sts, vShadowsLightDir, IRender::DT_DYNAMIC, &depthDynamicGeom );

		RenderDynamicDepthTexture( 
			pDepthTexture, 
			depthDynamicGeom, 
			pLight, 
			lightInfo, 
			sts, 
			nDepthTexResolution, 
			particleLM, 
			pDepthInfo, 
			pRender, 
			sceneGeom );

	}

	//checker fill depth map
	/*
	static bool bFilled = false;
    if ( !bFilled )
	{
		bFilled = true;
    	CArray2D<NGfx::SPixel8888> tmp;
		NGfx::GetRenderTargetData( &tmp, pDepthTexture );

		NGfx::CTextureLock<NGfx::SPixel8888> t( pDepthTexture, 0, NGfx::WRITEONLY );
		
		//csSystem << t.GetSizeY() << endl;
		for ( int y = 0; y < t.GetSizeY(); ++y )
		{
			for ( int x = 0; x < t.GetSizeX(); ++x )
			{
				char c = 255;( ( (x & 16) + (y  & 16 ) ) & 16 ) ? 255 : 0;
				//char c = tex[y][x].a;
				//t[y][x] = NGfx::SPixel8888( c, c, c, c );
				t[y][x] = NGfx::SPixel8888( c, c, c, 0 );
			}
		}
	}*/
}


static void Render( CTransformStack *pTS, NGfx::CRenderContext *pRC, IRender *pRender, const CSceneFragments &scene,
	const SLightInfo &lightInfo, SRenderPathContext *pRPC )
{
	CRenderCmdList lightOps;
	const std::vector<SRenderFragmentInfo*> &fragments = scene.GetFragments();
	for ( int i = 1; i < fragments.size(); ++i )
	{
		if ( scene.IsFilteredFragment( i ) )
			continue;
		const SRenderFragmentInfo &frag = *fragments[i];
		COpGenContext op( &lightOps.ops, &frag );
		frag.pMaterial->AddOperations( &op, pRPC );
	}
	Execute( pRender, pRC, *pTS, lightOps, scene, lightInfo );
}

static bool GetIntersectBound( SBound *pRes, const SBound &b1, const SBound &b2 )
{
	const CVec3 vMin1 = b1.s.ptCenter - b1.ptHalfBox;
	const CVec3 vMax1 = b1.s.ptCenter + b1.ptHalfBox;
	const CVec3 vMin2 = b2.s.ptCenter - b2.ptHalfBox;
	const CVec3 vMax2 = b2.s.ptCenter + b2.ptHalfBox;

	CVec3 vMin, vMax;
	vMin.x = (std::max)( vMin1.x, vMin2.x ); vMin.y = (std::max)( vMin1.y, vMin2.y ); vMin.z = (std::max)( vMin1.z, vMin2.z );
	vMax.x = (std::min)( vMax1.x, vMax2.x ); vMax.y = (std::min)( vMax1.y, vMax2.y ); vMax.z = (std::min)( vMax1.z, vMax2.z );
	if ( vMax.x < vMin.x || vMax.y < vMin.y || vMax.z < vMin.z )
		return false;
	pRes->BoxInit( vMin, vMax );
	return true;
}

static void ScaleClipVertex( CVec4 *pRes, const CVec4 &vClip, const SHMatrix &m )
{
	CVec4 leng;
	m.RotateHVector( &leng, vClip );
	leng.w = 0;
	*pRes = vClip * ( 1 / fabs( leng ) );
}

static float Dot4( const CVec4 &a, const CVec4 &b ) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

void UpdateWaterReflection( CTransformStack *pTS, IRender *pRender, const NGfx::CRenderContext *pRC, const CSceneFragments &scene,
													 const SLightInfo &lightInfo )
{	
	CVec3 vBoxMins( FP_MAX_VALUE, FP_MAX_VALUE, FP_MAX_VALUE );
	CVec3 vBoxMaxs( -FP_MAX_VALUE, -FP_MAX_VALUE, -FP_MAX_VALUE );

	float fZ=0.0f, fDiv = 0.00001f;

	// Calculating bounding box for materials which are need the reflection texture
	const std::vector<SRenderFragmentInfo *> &fragments = scene.GetFragments();
	for ( std::vector<SRenderFragmentInfo *>::const_iterator iFragment = fragments.begin();
			iFragment != fragments.end();
			++iFragment )
	{
		const SRenderFragmentInfo &fragment = *( *iFragment );

		if ( !fragment.pMaterial || !fragment.pMaterial->IsUsingWaterReflection() )
			continue;
	
		fZ += fragment.pMaterial->GetReflectionZ();
		fDiv += 1.0f;

		for ( int iElement = 0; iElement < fragment.elements.size(); ++iElement )
		{
			const SRenderFragmentInfo::SElement &element = fragment.elements[iElement];
			
			SRenderGeometryInfo *pGeometryInfo = scene.GetGeometryInfo( element.nGeometry );
			pGeometryInfo->pVertices.Refresh();

			const std::vector<SSphere> &bounds = pGeometryInfo->pVertices->GetBounds();
			int nFlags = element.nFlags;

			for ( int k = 0; k < 32; ++k )
			{
				if ( !(nFlags & (1<< k)) )
					continue;

				vBoxMins.Minimize( bounds[k].ptCenter );
				vBoxMaxs.Maximize( bounds[k].ptCenter );
			}
		}
	}

	if ( vBoxMins.x > vBoxMaxs.x )
		return;

	const float fWaterHeight = fZ / fDiv; //(vBoxMins.z + vBoxMaxs.z)/2.0f;

	//csSystem << vBoxMins.z << " " << vBoxMaxs.z << endl;


	// Calculating transformation for mirrored camera
	SHMatrix matMirrorZ( 1.0f, 0.0f, 0.0f, 0.0f, 
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f, 2.0f*fWaterHeight, 
		0.0f, 0.0f, 0.0f, 1.0f);

	SHMatrix matMirrorY( 1.0f, 0.0f, 0.0f, 0.0f, 
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);

	SHMatrix matTransform = matMirrorY * pTS->Get().forward * matMirrorZ;

	SHMatrix matTranspose;
	Transpose( &matTranspose, matTransform );

	// Change projection matrix to set near clip plane as a water plane
	SHMatrix matTransposeInverse;
	InvertMatrix( &matTransposeInverse, matTranspose );

	CVec4 vProjectWaterPlane;
	matTransposeInverse.RotateHVector( &vProjectWaterPlane, CVec4( 0, 0, 1.0f, -fWaterHeight + 0.2f ) );

	SHMatrix projClipMatrix;
	if ( fabs(vProjectWaterPlane.z) < FP_EPSILON )
	{
		// plane is perpendicular to the near plane
		projClipMatrix = pTS->GetProjection().forward;
	}
	else
	{
		if ( vProjectWaterPlane.z < 0 )
		{
			// flip plane to point away from eye
			matTransposeInverse.RotateHVector( &vProjectWaterPlane, CVec4( 0, 0, -1.0f, fWaterHeight ) );
		}

		// "Normalize" plane ( if (-1<x<1 && -1<y<1 && 0<z<1 && w<farPlane) => Dot( xyzw, plane ) < 1 )
		const float fFarPlane = 10000.0f;
		float fMaxDot = fabs( vProjectWaterPlane.x ) + fabs( vProjectWaterPlane.y ) + fabs( vProjectWaterPlane.z )
			+ fabs( vProjectWaterPlane.w ) * fFarPlane;

		vProjectWaterPlane /= fFarPlane;

		// put projection space clip plane in Z column
		SHMatrix matClipProj( 1.0f, 0.0f, 0.0f, 0.0f, 
													0.0f, 1.0f, 0.0f, 0.0f,
													vProjectWaterPlane.x, vProjectWaterPlane.y, vProjectWaterPlane.z,  vProjectWaterPlane.w,
													0.0f, 0.0f, 0.0f, 1.0f);

		projClipMatrix = matClipProj*pTS->GetProjection().forward;
	}

	CTransformStack tsMirrored;
	tsMirrored.Make( projClipMatrix );
	tsMirrored.Push44( pTS->GetProjection().backward * matTransform );

	// Creating new clip planes to reduce renderlist
	CVec4 points[4];
	points[0] = CVec4( vBoxMins.x, vBoxMins.y, fWaterHeight, 1.0f );
	points[1] = CVec4( vBoxMins.x, vBoxMaxs.y, fWaterHeight, 1.0f );
	points[2] = CVec4( vBoxMaxs.x, vBoxMaxs.y, fWaterHeight, 1.0f );
	points[3] = CVec4( vBoxMaxs.x, vBoxMins.y, fWaterHeight, 1.0f );	

	for ( int iPoint = 0; iPoint < 4; ++iPoint)
	{
		matTransform.RotateHVector( &points[iPoint], CVec4(points[iPoint]) );
		points[iPoint].x /= points[iPoint].w;
		points[iPoint].y /= points[iPoint].w;
		points[iPoint].z = 0.0f;
		points[iPoint].w = 1.0f;
	}

	/* FIX ME
	for ( int iPlane = 0; iPlane < 4; ++iPlane)
	{
		const CVec4 &point0 = points[iPlane];
		const CVec4 &point1 = points[(iPlane+1)%4];

		CVec4 plane( point0.y - point1.y, point1.x - point0.x, 0.0f, fabs( point1.y*point0.x - point1.x*point0.y ) );

		CVec4 planeTransposed;
		ScaleClipVertex( &planeTransposed, plane, matTranspose );
		tsMirrored.AddClipPlane( planeTransposed );
	}	
	*/

	// Render
	NGfx::CRenderContext rc;
	rc.SetTextureRT( pCurrentWaterReflectionTexture );

	NGfx::SFogParams fogParams( pRC->GetFogParams() );
	rc.SetFogParams( fogParams );
	rc.ClearBuffers( NGfx::GetDWORDColor( CVec4( fogParams.vColor, 1 ) ) ); // FIX ME - detect sky or fog else it will be a hom effect

	// We needn't lit particles for reflection
	CObj<CTransparentRenderer> pTransp = pRender->CreateTransparentRenderer( &tsMirrored, false );

	// Creating scene list
	CSceneFragments mirrorScene;
	pRender->FormRenderList( &tsMirrored, &mirrorScene, pTransp );
	
	SRenderPathContext rpc( false, pCurrentDepthTexture, 0 );
	Render( &tsMirrored, &rc, pRender, mirrorScene, lightInfo, &rpc );

	pRender->RenderPostProcess( &tsMirrored, &rc );

	STransparentRenderContext trc( rpc.bTnL, &rc, 0, 0, RP_GF3_FAST, &rpc, lightInfo );
	pTransp->Render( trc );
	
	/*
	CPtr<IDataStream> pStream = CreateStream( "C:/Temp/Test.tga", STREAM_PATH_ABSOLUTE );

	CArray2D<NGfx::SPixel8888> texture;
	NGfx::GetRenderTargetData( &texture, pCurrentWaterReflectionTexture );	

	CArray2D<DWORD> image;
	image.SetSizes( texture.GetSizeX(), texture.GetSizeY() );

	for ( int iX = 0; iX < texture.GetSizeX(); ++iX )
		for ( int iY = 0; iY < texture.GetSizeY(); ++iY )
		{
			image[iX][iY] = 0xff000000 | ( texture[iX][iY].r << 16 ) | ( texture[iX][iY].g << 8 ) | texture[iX][iY].b;
			if ( image[iX][iY] > 0xff000000 )
				int aaa = 0;
		}

	NImage::SaveImageAsTGA( pStream, image);
	*/
}


static CRTPtr pDepthShadow( "DepthMap" );
static CRTPtr pWaterReflection( "WaterReflection" );

void RenderGf3Fast( CTransformStack *pTS, CTransformStack *pClipTS, NGfx::CRenderContext *pRC, 
	IRender *pRender, CSceneFragments &scene, const SParticleLMRenderTargetInfo &particleLM,
	const SRTClearParams &rtClear, CDirectionalLight *pLight, float _fWarFogBlend, int nLightingOptions,
	CTransparentRenderer *pTransp, ETransparentMode trMode,
	NGfx::CTexture *pParticleLight, NGfx::CCubeTexture *_pSky )
{
	if ( bNoDepthRender )
		nLightingOptions |= LO_NOSHADOWS;
	if ( nLightingOptions & LO_NOSHADOWS )
		pCurrentDepthTexture = GetWhiteTexture();
	else
		pCurrentDepthTexture = pDepthShadow.GetTexture();	

	SLightInfo lightInfo;
	pLight->PrepareLightInfo( &lightInfo );
	lightInfo.fWarFogBlend = _fWarFogBlend;
	ASSERT( NGfx::GetHardwareLevel() >= NGfx::HL_GFORCE3 );

	SBound sceneBound;
	scene.GetBound( &sceneBound );

	
	bool bDrawWaterReflection = NGScene::bWaterReflection;
		
	if ( bDrawWaterReflection )
		pCurrentWaterReflectionTexture = pWaterReflection.GetTexture();

	SRenderPathContext rpc( false, pCurrentDepthTexture, pCurrentWaterReflectionTexture );

	if ( ( nLightingOptions & LO_NOSHADOWS ) == 0 )
	{
		float fShadowsMDLength = pLight->GetShadowsMDLength();
		//static float fDp = 1;
		UpdateDepthTexture( pTS, pClipTS, pRender, lightInfo, &rpc.depthInfo, sceneBound, nLightingOptions, pLight, particleLM,
			fShadowsMDLength, pCurrentDepthTexture, scene );
	}
	else
	{
		FixParticles( particleLM, lightInfo );
	}

	if ( bDrawWaterReflection ) 
	{
		UpdateWaterReflection( pTS, pRender, pRC, scene, lightInfo );
	}

	ClearRT( pRC, rtClear );

	if ( trMode != TRM_ONLY )
		Render( pTS, pRC, pRender, scene, lightInfo, &rpc );

	pRender->RenderPostProcess( pTS, pRC );
	// draw transparent stuff
	if ( trMode != TRM_NONE )
	{
		STransparentRenderContext trc( rpc.bTnL, pRC, pParticleLight, _pSky, RP_GF3_FAST, &rpc, lightInfo );
		pTransp->Render( trc );
	}

	pCurrentDepthTexture = 0;
	pCurrentWaterReflectionTexture = 0;
}

START_REGISTER(GRenderPathFastest)
	REGISTER_VAR_EX( "gfx_noshadows", NGlobal::VarBoolHandler, &bNoDepthRender, 0, STORAGE_USER )
	REGISTER_VAR_EX( "gfx_freeze_shadows", NGlobal::VarBoolHandler, &bFreeze, 0, STORAGE_USER )

FINISH_REGISTER
}

