#include "StdAfx.h"
#include "GfxEffects.h"
#include "GRTShare.h"
#include "GLightmapCalc.h"
#include "GLightmap.h"
#include "GParts.h"
#include "GRenderExecute.h"
#include "GLMLightState.h"
#include "..\3dlib\GLMGeometry.h"
#include "GInit.h"
#include "GfxUtils.h"
#include "GRects.h"
#include "GObjectInfo.h"
#include "../Image/DDS.h"
#include "../Image/Targa.h"
#include "../Image/Image.h"
#include "../Image/ImageDDS.h"
#include "../Image/ImageTGA.h"
#include "../Image/ImagePSD.h"
#include "../3Dmotor/GPixelFormat.h"
#include "../3Dmotor/GView.h"
#include "../System/FilePath.h"
#include "../System/FileUtils.h"
#include "../System/VFSOperations.h"

const float F_MAX_SCENE_HEIGHT = 20; // CRAP need to store max height in single place
const int N_DEPTH_CHANNELS_PER_TEX = 3;
const float F_MINIMAL_SCENE_PART_SIZE = 20;

namespace NGScene
{
class CLMPart : public IPart
{
	OBJECT_NOCOPY_METHODS( CLMPart );
	CObj<ISomePart> pPart;
public:
	CLMPart() {}
	CLMPart( ISomePart *_pPart, CPtrFuncBase<CObjectInfo> *pData, CPerMaterialCombiner *_pCombiner )
		: IPart( pData, _pCombiner ), pPart(_pPart) {}
	virtual ETransformType GetTransformType() const { return pPart->GetTransformType(); }
	virtual const SFBTransform& GetSimplePos() { return pPart->GetSimplePos(); }
	virtual const vector<SHMatrix>& GetAnimation() { return pPart->GetAnimation(); }
	virtual const vector<NGfx::SCompactTransformer>& GetMMXAnimation() { return pPart->GetMMXAnimation(); }
	virtual bool Is2Sided() const { return pPart->Is2Sided(); }
	virtual int GetSortValue() const { return 0; }
};

class CLightmapsHolder : public CObjectBase
{
	OBJECT_NOCOPY_METHODS(CLightmapsHolder);
public:
	struct SLightmap
	{
		ZDATA
		int nLightmap;
		CTPoint<int> lmShift;
		float fLMResolution;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&nLightmap); f.Add(3,&lmShift); f.Add(4,&fLMResolution); return 0; }

		SLightmap() : nLightmap(-1), lmShift(0,0), fLMResolution(0.001f) {}
		SLightmap( int _nLightmap, const CTPoint<int> &_lmShift, float _fLMResolution ) 
			: nLightmap(_nLightmap), lmShift(_lmShift), fLMResolution(_fLMResolution) {}
	};
	ZDATA
	hash_map<int, SLightmap> lightmaps;
	vector<CArray2D<NGfx::SPixel8888> > textures;
	ZONSERIALIZE
	ZEND int operator&( IBinSaver &f ) { 
		f.Add(2,&lightmaps); 
		f.Add(3,&textures);  
		OnSerialize( f ); return 0; }
	
	void OnSerialize( IBinSaver &f )
	{
		/*
		if ( !f.IsReading() ) 
		{
			int r=rand();
			
			for( int i=0; i<textures.size(); ++i)
			{
				char buff[1024];
				sprintf( buff, "C:\\test\\lm%i_%i.ddx", r, i );
	
				CFileStream stream( buff, CFileStream::WIN_CREATE );
				NImage::ConvertAndSaveAsDDS( &stream, *(CArray2D<DWORD> *)&textures[i], NImage::IMAGE_TYPE_PICTURE, NGfx::CF_DXT1, 1, true, true, 1024.0f);
			}
		
		}*/
	}
};

struct SLMPartCalc
{
	CObj<ISomePart> pPart;
	CTPoint<int> lmShift, lmSize;
	CArray2D<NGfx::SPixel8888> lightmap;
	float fLMResolution;
	SBound geomBound;
	vector<SLMQuad> lmQuads;

	SLMPartCalc() {}
	SLMPartCalc( ISomePart *_p, const CTPoint<int> &_lmShift, const CTPoint<int> &_lmSize, float _fLMResolution,
		const SBound &_geomBound, const vector<SLMQuad> &_lmQuads ) 
		: pPart(_p), lmShift(_lmShift), lmSize(_lmSize), fLMResolution(_fLMResolution), geomBound(_geomBound), lmQuads(_lmQuads) {}
};
struct SLMGroup
{
	vector<SLMPartCalc> parts;
	CSingleTexAlloc lmAlloc;
	float fResolution;

	SLMGroup( float _fResolution = 0 ) : lmAlloc( N_LM_TEXTURE_SIZE ), fResolution(_fResolution) {}
};

class CLightmapsTempHolder : public CObjectBase
{
	OBJECT_NOCOPY_METHODS(CLightmapsTempHolder);
public:
	ZDATA
	vector<SLMGroup> lmGroups;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&lmGroups); return 0; }
};

static CPtrFuncBase<CObjectInfo> *GetRawGeometry( ISomePart *pPart )
{
	CPtrFuncBase<CObjectInfo> *pGeom = pPart->GetObjectInfoNode();
	CDynamicCast<CLMGeometryGen> pTest = pGeom;
	if ( pTest != 0 )
		pGeom = pTest->GetSrc();
	ASSERT( pGeom );
	ASSERT( dynamic_cast<CLMGeometryGen*>( pGeom ) == 0 );
	return pGeom;
}

static float CalcLMRes( const SSphere &highResLM, const SBound &partBV )
{
	float fDist = fabs( highResLM.ptCenter - partBV.s.ptCenter );
	float f = fDist + partBV.s.fRadius - highResLM.fRadius;
	f = Max( f, 0.0f );
	return 10 / ( 1 + sqr( f / highResLM.fRadius ) );
}

static void AddPart( const SSphere &highResLM, vector<SLMGroup> *pRes, ISomePart *pPart )
{
	//const CTPoint<int> &lmSize;
	CDGPtr<CPtrFuncBase<CObjectInfo> > pGeom = GetRawGeometry( pPart );
	// calc lm size
	pGeom.Refresh();
	CObjectInfo::SData data, lmData;
	MakeSData( &data, *pGeom->GetValue() );
	CTPoint<int> lmSize, lmPos;
	SBound geomBound;
	geomBound.BoxInit( pPart->vBVMin, pPart->vBVMax );
	float fLMResolution = min ( 3.0f, CalcLMRes( highResLM, geomBound ) );
	vector<SLMQuad> lmQuads;
	MakeLMCalcGeometry( &lmData, &lmSize, data, fLMResolution, N_LM_TEXTURE_SIZE, CTPoint<int>(0,0), &lmQuads );

	int nResolution = Float2Int( log( fLMResolution * 4 ) );
	// search for a suitable group
	for ( int k = 0; k < pRes->size(); ++k )
	{
		SLMGroup &g = (*pRes)[k];
		if ( g.fResolution != nResolution )
			continue;
		// check if group is suitable
		if ( g.lmAlloc.AllocRegion( lmSize, &lmPos ) )
		{
			g.parts.push_back( SLMPartCalc( pPart, lmPos, lmSize, fLMResolution, geomBound, lmQuads ) );
			return;
		}
	}
	SLMGroup &g = *pRes->insert( pRes->end(), SLMGroup( nResolution ) );
	if ( !g.lmAlloc.AllocRegion( lmSize, &lmPos ) )
	{
		ASSERT(0);
		return;
	}
	g.parts.push_back( SLMPartCalc( pPart, lmPos, lmSize, fLMResolution, geomBound, lmQuads ) );
}

static void RenderParallelDepth( IRender *pRender, NGfx::CRenderContext *pRC, 
																const SSphere &_bound, const CVec3 &vDir, const SGroupSelect &_groupSelect,
																SDirectionalDepthInfo *pDepthInfo )
{
	//	const float F_RANGE = 40;
	const CVec3 &vEnter = _bound.ptCenter;
	float fWidth = 2 * _bound.fRadius;//p.fWidth;
	float fRange = 2 * _bound.fRadius;
	// setup projection for depth map
	CTransformStack ts;
	ts.MakeParallel( fWidth, fWidth, -1000, 1000 );
	// fill depth info
	SHMatrix cameraPos;
	MakeMatrix( &cameraPos, vEnter, vDir );
	ts.SetCamera( cameraPos );
	//vLightDir = ptCenter;
	SHMatrix mTrans = ts.Get().forward;
	SDirectionalDepthInfo &depthInfo = *pDepthInfo;
	NGfx::GetTexMapFromProjection( &mTrans, N_DEFAULT_RT_RESOLUTION );
	depthInfo.vVecU = mTrans.x;
	depthInfo.vVecV = mTrans.y;
	float fRange1 = 1.0f / fRange;
	//depthInfo.vDepth = CVec4( -vDir * fRange1, 0.5f + fRange1 * ( vDir * vEnter ) );//0, 0, 1 / 20.0f, 0 ); //_fMaxHeight
	depthInfo.vDepth = CVec4( 0, 0, 1 / F_MAX_SCENE_HEIGHT, 0 );

	SLightInfo lightInfo;
	CSceneFragments geom;
	CRenderCmdList res;
	//pRender->FormDirOccludersList( &ts, vDir, &geom, _groupSelect );
	pRender->FormDepthList( &ts, vDir, IRender::DT_STATIC, &geom );
	MakeSingleOp( &res, geom, true, -1, 0, RO_LP_DEPTH, &depthInfo );
	Execute( pRender, pRC, ts, res, geom, lightInfo );
}

static void DrawBorder( NGfx::CRenderContext *pRC, int nXSize, int nYSize )
{
	CRectLayout borderLayout;
	borderLayout.AddRect( 0, 0, nXSize, 1 );
	borderLayout.AddRect( 0, 0, 1, nYSize );
	borderLayout.AddRect( 0, nYSize - 1, nXSize, 1 );
	borderLayout.AddRect( nXSize - 1, 0, 1, nYSize );
	NGfx::C2DQuadsRenderer qr( *pRC, CVec2( nXSize, nYSize ), NGfx::QRM_DEPTH_NONE|NGfx::QRM_NOCOLOR );
	RenderRectLayout( &qr, 0, borderLayout );
}

static NGfx::EColorWriteMask depthChannels[3] = 
{
	NGfx::COLORWRITE_RED, NGfx::COLORWRITE_GREEN, NGfx::COLORWRITE_BLUE
};
static void RecalcDepthChannels( IRender *pRender, NGfx::CTexture *pDepth, const SSphere &_bound,  
															 const vector<CVec3> &skyDirs, vector<SDirectionalDepthInfo> *pDepthInfos, 
															 int nBase, const SGroupSelect &groupSelect )
{
	vector<SDirectionalDepthInfo> &depthInfos = *pDepthInfos;
	NGfx::CRenderContext rcDepth;

	rcDepth.SetTextureRT( pDepth );
	rcDepth.ClearBuffers( 0 );

	bool bHasRendered = false;
	for ( int i = 0; i < N_DEPTH_CHANNELS_PER_TEX; ++i )
	{
		int nInfoIdx = nBase + i;
		const CVec3 &vDir = skyDirs[ nInfoIdx ];
		rcDepth.SetColorWrite( depthChannels[ i ] );
		if ( bHasRendered )
			rcDepth.ClearZBuffer();
		RenderParallelDepth( pRender, &rcDepth, _bound, vDir, groupSelect, &depthInfos[ nInfoIdx ] );
		bHasRendered = true;
	}
	rcDepth.SetColorWrite( NGfx::COLORWRITE_ALL );
	// depth map border
	DrawBorder( &rcDepth, N_DEFAULT_RT_RESOLUTION, N_DEFAULT_RT_RESOLUTION );
}

struct SLightmapTargetGeom
{
	NGfx::CRenderContext *pRC;
	CSceneFragments *pGeom;
	CTransformStack *pTS;
//	int nTargetRegister;

	SLightmapTargetGeom( CSceneFragments *_pGeom, NGfx::CRenderContext *_pRC, CTransformStack *_pTS ) 
		: pGeom(_pGeom), pRC(_pRC), pTS(_pTS) {}
};

static void RenderLight( 
												SLightmapTargetGeom *pTarget, const SLightInfo &lightInfo,
												ERenderOperation op, ERenderOperation opTrans, CRenderCmdList::UParameter param1, 
												int nStencilOp )
{
	NGfx::CRenderContext &rc = *pTarget->pRC;

	CRenderCmdList res;
	const vector<SRenderFragmentInfo*> &fragments = pTarget->pGeom->GetFragments();
	vector<CVec3> transColors( fragments.size() );
	for ( int i = 1; i < fragments.size(); ++i )
	{
		if ( pTarget->pGeom->IsFilteredFragment( i ) )
			continue;
		const SRenderFragmentInfo &frag = *fragments[i];
		COpGenContext fi( &res.ops, &frag );
		CVec3 vTranslucent(0,0,0);
		if ( frag.pMaterial )
			vTranslucent = frag.pMaterial->GetTranslucentColor();
		transColors[i] = vTranslucent;
		if ( vTranslucent == CVec3(0,0,0) )
			fi.AddOperation( op, 100, nStencilOp, 0, param1, &VNULL3 );
		else
			fi.AddOperation( opTrans, 100, nStencilOp, 0, param1, &transColors[ i ] );
	}
	Execute( 0, &rc, *pTarget->pTS, res, *pTarget->pGeom, lightInfo );
}

static void RenderSkyCheck( SLightmapTargetGeom *pTarget, 
													 const vector<CVec3> &skyDirs, const vector<SDirectionalDepthInfo> &depthInfos,
													 const CVec3 &vColor,
													 int nBase,
													 float fStrength, NGfx::CTexture *pDepth, NGfx::CTexture *pAdd )
{
	NGfx::CRenderContext &rc = *pTarget->pRC;
	rc.SetCulling( NGfx::CULL_NONE );
	SLightInfo lightInfo;
	lightInfo.bNeedSet = true;
	float f = fStrength;
	lightInfo.vLightColor = vColor * f;

	//rc.SetColorWrite( NGfx::COLORWRITE_ALPHA );
	if ( NGfx::GetHardwareLevel() >= NGfx::HL_GFORCE3 )
	{
		SSkyDepth3Info depthInfo;
		depthInfo.channels[0] = &depthInfos[ nBase + 0 ];
		depthInfo.channels[1] = &depthInfos[ nBase + 1 ];
		depthInfo.channels[2] = &depthInfos[ nBase + 2 ];
		depthInfo.vDirs[0] = -skyDirs[ nBase + 0 ];
		depthInfo.vDirs[1] = -skyDirs[ nBase + 1 ];
		depthInfo.vDirs[2] = -skyDirs[ nBase + 2 ];
		depthInfo.nResolution = N_LM_TEXTURE_SIZE;
		depthInfo.pAdd = pAdd;
		depthInfo.pDepth = pDepth;

		RenderLight( pTarget, lightInfo, RO_CL_SKY_3LIGHT, RO_CL_SKY_3LIGHT_TRANSLUCENT, &depthInfo, DPM_NONE|ABM_NONE );
	}
	else
		ASSERT(0);
}

static void InitLightInfo( SLightInfo *pRes, const CVec3 &_vCenter, float fRadius, const CVec3 &_vColor )
{
	SLightInfo &lightInfo = *pRes;
	lightInfo.bNeedSet = true;
	lightInfo.vLightColor = _vColor;
	lightInfo.vLightPos = CVec4( _vCenter, 0 );
	NGfx::InitRadius( &lightInfo.vRadius, fRadius );
}

static void RenderQuad( NGfx::CRenderContext *pRC, const CVec3 &_vCenter, const CVec3 &_vNormal, float _fRadius )
{
	NGfx::CRenderContext rc = *pRC;

	CVec3 v1( _vNormal ^ CVec3(1,0,0) );
	if ( fabs2( v1 ) <= 0.01f )
		v1 = _vNormal ^ CVec3(0,1,0);
	Normalize( &v1 );
	CVec3 v2( v1 ^ _vNormal );
	CVec3 vCenter( _vCenter - _vNormal * _fRadius / 1000 );
	CObj<NGfx::CGeometry> pGeom;
	vector<STriangle> tris(2);
	const float F_QUAD_SIZE = 100000;
	{
		NGfx::CBufferLock<NGfx::SGeomVecFull> points( &pGeom, 4 );
		points[0].pos = vCenter + v1 * F_QUAD_SIZE + v2 * F_QUAD_SIZE;
		points[1].pos = vCenter + v1 * F_QUAD_SIZE - v2 * F_QUAD_SIZE;
		points[2].pos = vCenter - v1 * F_QUAD_SIZE - v2 * F_QUAD_SIZE;
		points[3].pos = vCenter - v1 * F_QUAD_SIZE + v2 * F_QUAD_SIZE;
		tris[0] = STriangle(0,1,2);
		tris[1] = STriangle(0,2,3);
	}
	rc.SetCulling( NGfx::CULL_NONE );
	rc.SetDepth( NGfx::DEPTH_NONE );
	rc.SetAlphaCombine( NGfx::COMBINE_NONE );
	rc.SetStencil( NGfx::STENCIL_NONE );
	NGfx::SEffConstLight cl;
	cl.color = CVec4( 0, 0, 0, 0 );
	rc.SetEffect( &cl );
	rc.DrawPrimitive( pGeom, tris );
}

static void RenderCubeMapDepth(
	IRender *pRender,
	const CVec3 &_vCenter, float fRadius, const CVec3 &_vNormal, 
	NGfx::CCubeTexture *pDepth )
{
	ASSERT( IsValid( pDepth ) );
	SLightInfo lightInfo;
	InitLightInfo( &lightInfo, _vCenter, fRadius, CVec3(0,0,0) );

	CDynamicCast<NGfx::ICubeBuffer> pBuf( pDepth );
	int nResolution = pBuf->GetSize();
	// render occluders
	for ( int nDir = 0; nDir < 6; ++nDir )
	{
		NGfx::CRenderContext rc;
		NGfx::EFace face;
		CTransformStack ts;
		ts.MakeProjective( 1, 90, fRadius / 3000, fRadius * 2 );
		SHMatrix camera;
		CVec4 vX( 1, 0, 0, -_vCenter.x );
		CVec4 vY( 0, 1, 0, -_vCenter.y );
		CVec4 vZ( 0, 0, 1, -_vCenter.z );
		switch ( nDir )
		{
		case 0: face = NGfx::POSITIVE_X; camera.x = -vZ; camera.y = -vY; camera.z =  vX; break;
		case 1: face = NGfx::POSITIVE_Y; camera.x =  vX; camera.y =  vZ; camera.z =  vY; break;
		case 2: face = NGfx::POSITIVE_Z; camera.x =  vX; camera.y = -vY; camera.z =  vZ; break;
		case 3: face = NGfx::NEGATIVE_X; camera.x =  vZ; camera.y = -vY; camera.z = -vX; break;
		case 4: face = NGfx::NEGATIVE_Y; camera.x =  vX; camera.y = -vZ; camera.z = -vY; break;
		case 5: face = NGfx::NEGATIVE_Z; camera.x = -vX; camera.y = -vY; camera.z = -vZ; break;
		default: face = NGfx::POSITIVE_X; ASSERT(0); break;
		}
		// gather occluders
		CVec3 vDir = CVec3( camera.zx, camera.zy, camera.zz );
		CSceneFragments geom;
		CTransformStack tsFrustrum;
		tsFrustrum.MakeProjective( 1, 90, 0.01f, fRadius );
		SHMatrix mCubeCenter;
		MakeMatrix( &mCubeCenter, _vCenter, vDir );
		tsFrustrum.SetCamera( mCubeCenter );
		//pRender->FormDirOccludersList( &tsFrustrum, vDir, &geom, MakeSelectAll(), false );
		pRender->FormDepthList( &tsFrustrum, vDir, IRender::DT_STATIC, &geom );

		// form transform stack for render
		camera.y = -camera.y;
		camera.w = CVec4(0,0,0,1);
		ts.Push43( camera );
		camera = ts.Get().forward;
		camera.x = camera.x - (1.0f / nResolution ) * camera.w;
		camera.y = camera.y + (1.0f / nResolution ) * camera.w;
		ts.Init( camera );
		rc.SetCubeTextureRT( pDepth, face, 0 );
		rc.ClearBuffers( 0xffffffff );
		rc.SetCulling( NGfx::CULL_NONE );//CCW

		CRenderCmdList dp;
		MakeSingleOp( &dp, geom, true, -1, 0, RO_CL_CUBEMAP_DEPTH );
		Execute( 0, &rc, ts, dp, geom, lightInfo );
		
		if ( fabs2( _vNormal ) > 0 )
			RenderQuad( &rc, _vCenter, _vNormal, fRadius );
	}
}

static void RenderPointLightShadowed( 
	SLightmapTargetGeom *pTarget, 
	const CVec3 &_vCenter, float fRadius, const CVec3 &_vColor,
	NGfx::CCubeTexture *pDepth, NGfx::CTexture *pAdd )
{
	if ( fabs2(_vColor) == 0 || fRadius < 0.1f )
		return;
	pTarget->pRC->SetCulling( NGfx::CULL_NONE );
	SLightInfo lightInfo;
	InitLightInfo( &lightInfo, _vCenter, fRadius, _vColor );

	//pTarget->pRC->SetColorWrite( NGfx::COLORWRITE_COLOR );
	SPntLightShadowedInfo info;
	info.pDepth = pDepth;
	info.pAdd = pAdd;
	info.fResolution = N_LM_TEXTURE_SIZE;

	RenderLight( pTarget, lightInfo, RO_CL_PNT_LIGHT_SHADOWED, RO_CL_PNT_LIGHT_SHADOWED, &info, DPM_NONE|ABM_NONE );
}

static void MakeLMTS( CTransformStack *pTS, CVec4 &vZ, int nLMSize )
{
	CTransformStack &ts = *pTS;
	SHMatrix m;
	NGfx::MakeLMToScreenMatrix( &m, nLMSize, nLMSize );
	m.z = vZ;
	ts.Init( m );
}

static void CalcLight( IRender *pRender, CSceneFragments *pTargetGeom, const SSphere &_bound, 
	const SLMGroup &lmGroup,
	const CLightState &ls,
	const SGroupSelect &groupSelect, CArray2D<NGfx::SPixel8888> *pRes )
{
	static CRTPtr pTargetTex( "LMDest" ), pDepthTex( "ParticleLight" );
	static CRTPtr pTargetFPTex1( "LMFPDest1" ), pTargetFPTex2( "LMFPDest2" );
	static CCubeRTPtr pPointDepthTex( "LMPointDepth" );
	NGfx::CTexture *pDepth = pDepthTex.GetTexture();
	NGfx::CTexture *pTarget = pTargetTex.GetTexture();
	NGfx::CTexture *pTargetFP1 = pTargetFPTex1.GetTexture();
	NGfx::CTexture *pTargetFP2 = pTargetFPTex2.GetTexture();
	NGfx::CCubeTexture *pPointDepth = pPointDepthTex.GetTexture();
	// clear target
	NGfx::CRenderContext rcTarget;
	rcTarget.SetTextureRT( pTargetFP2 );
	rcTarget.ClearBuffers( 0 );

	// render sky checks
	const vector<CVec3> &skyDirs = ls.skyDirections;
	vector<SDirectionalDepthInfo> depthInfos;
	depthInfos.resize( skyDirs.size() );

	CTransformStack tsDirect;
	MakeLMTS( &tsDirect, CVec4( 0, 0, 0.001f, 0.5f ), N_LM_TEXTURE_SIZE );
	SLightmapTargetGeom lmTarget( pTargetGeom, &rcTarget, &tsDirect );

	int nTotalSkyDirs = skyDirs.size();
	for ( int k = 0; k < nTotalSkyDirs; k += N_DEPTH_CHANNELS_PER_TEX )
	{
		RecalcDepthChannels( pRender, pDepth, _bound, skyDirs, &depthInfos, k, groupSelect );
		rcTarget.SetTextureRT( pTargetFP1 );
		RenderSkyCheck( 
			&lmTarget, skyDirs, depthInfos, ls.vAmbientColor,//CVec3(1,1,1),//
			k, F_SKY_SINGLE_STRENGTH_MUL / nTotalSkyDirs, pDepth, pTargetFP2 );
		swap( pTargetFP1, pTargetFP2 );
	}

	for ( int k = 0; k < ls.points.size(); ++k )
	{
		const CLightState::SPointLight &p = ls.points[k];
		const CVec3 &vCenter = p.vCenter;
		const CVec3 &vColor = p.vColor;
		float fRadius = p.fRadius;
		RenderCubeMapDepth( pRender, vCenter, fRadius, CVec3(0,0,0), pPointDepth );
		rcTarget.SetTextureRT( pTargetFP1 );
		RenderPointLightShadowed( &lmTarget, vCenter, fRadius, vColor, pPointDepth, pTargetFP2 );
		swap( pTargetFP1, pTargetFP2 );
	}

	for ( int k = 0; k < ls.semiPoints.size(); ++k )
	{
		const CLightState::SSemiPointLight &s = ls.semiPoints[k];
		const CVec3 &vCenter = s.vCenter;
		float fRadius = s.fRadius;
		{
			CSelectGeometries selector( pTargetGeom, SSphereFilter( SSphere( vCenter, fRadius ) ) );
			if ( !pTargetGeom->HasSelectedFragments() )
				continue;
		}
		RenderCubeMapDepth( pRender, vCenter, fRadius, s.vNormal, pPointDepth );
		rcTarget.SetTextureRT( pTargetFP1 );
		RenderPointLightShadowed( &lmTarget, vCenter, fRadius, s.vColor, pPointDepth, pTargetFP2 );
		swap( pTargetFP1, pTargetFP2 );
	}

	// copy result, gamma correction?
	NGfx::CRenderContext rcTargetFixed;
	rcTargetFixed.SetTextureRT( pTarget );
	CTRect<float> rSize( 0, 0, N_LM_TEXTURE_SIZE, N_LM_TEXTURE_SIZE );
	CObj<NGfx::I2DEffect> pEffect = new NGfx::CLinearToGammaEffect;
	NGfx::CopyTexture( rcTargetFixed, CVec2( rSize.x2, rSize.y2 ), rSize, pTargetFP2, rSize, CVec4(1,1,1,1), pEffect );

	// get result
	GetRenderTargetData( pRes, pTarget );
}

// calc color for all out of triangle samples from inside triangle samples
static void FixBorders( CArray2D<NGfx::SPixel8888> *pRes, const vector<SLMQuad> &quads )
{
	for ( int k = 0; k < quads.size(); ++k )
	{
		const SLMQuad &q = quads[k];
		ASSERT( q.quad.x2 <= pRes->GetSizeX() );
		ASSERT( q.quad.y2 <= pRes->GetSizeY() );
		ASSERT( q.quad.x1 >= 0 );
		ASSERT( q.quad.y1 >= 0 );
		if ( !q.bFull )
		{
			for ( int x = q.quad.x1 + 1; x < q.quad.x2; ++x )
			{
				float fX = ( x - q.quad.x1 ) / float( q.quad.x2 - q.quad.x1 - 1 );
				int y = q.quad.y2 - 1 - ( q.quad.y2 - q.quad.y1 - 1 ) * fX;
				const NGfx::SPixel8888 &c00 = (*pRes)[y][x-1];
				const NGfx::SPixel8888 &c01 = (*pRes)[y][x];
				const NGfx::SPixel8888 &c10 = (*pRes)[y+1][x-1];
				NGfx::SPixel8888 &c11 = (*pRes)[y+1][x];
				c11 = NGfx::Get8888Color( NGfx::GetCVec4Color( c10.dwColor ) + NGfx::GetCVec4Color( c01.dwColor ) - NGfx::GetCVec4Color( c00.dwColor ) );
			}
		}
	}
}

static bool CalcLM( IRender *pRender, const CLightState &ls, SLMGroup *pRes )
{
	int nSize = pRes->parts.size();
	SBoundCalcer totalBound;
	CSceneFragments targetGeom;
	vector<SRenderGeometryInfo> geomInfos( nSize );

	for ( int nPart = 0; nPart < nSize; ++nPart )
	{
		SLMPartCalc &lmPart = pRes->parts[nPart];
		ISomePart *pPart = lmPart.pPart;
		//pRes->SetSizes( 1, 1 );
		CPtrFuncBase<CObjectInfo> *pGeom = GetRawGeometry( pPart );

		totalBound.Add( lmPart.geomBound );

		// create lm part
		CPerMaterialCombiner *pCombiner = new CPerMaterialCombiner;
		CLMGeometryGen *pLMCalc = new CLMGeometryGen( pGeom, lmPart.lmShift, lmPart.fLMResolution, true );
		CLMPart *pLMPart = new CLMPart( pPart, pLMCalc, pCombiner );

		// make fake scene
		SRenderGeometryInfo &gInfo = geomInfos[ nPart ];
		gInfo.pTriLists[0] = new CIBCombiner( pCombiner, 0, IBTT_VERTICES );
		for ( int k = 1; k < TLT_NUMBER; ++k )
			gInfo.pTriLists[k] = gInfo.pTriLists[0];
		gInfo.pVertices = new CVBCombiner( pCombiner, CT_STATIC, 0, pRender->GetLightState() );
		int nGeometry = targetGeom.AddGeometry( 0, &gInfo, lmPart.geomBound, false );
		CPartFlags parts;
		parts.Clear();
		parts.Set( 0 );
		//SMaterialCreateInfo materialInfo;
		targetGeom.AddElement( nGeometry, parts, pPart->GetMaterial(), SPerPartVariables() );
	}

	// calc some primitive stuff like hemisphere lighting
	SGroupSelect groupSelect( 0xffff, 0 );
	CArray2D<NGfx::SPixel8888> fullTex;
	SSphere sphereBV;
	totalBound.Make( &sphereBV );
	CalcLight( pRender, &targetGeom, sphereBV, *pRes, ls, groupSelect, &fullTex );

	for ( int nPart = 0; nPart < pRes->parts.size(); ++nPart )
	{
		SLMPartCalc &lmPart = pRes->parts[nPart];
		// copy result
		const CTPoint<int> &lmShift = lmPart.lmShift;
		const CTPoint<int> &lmSize = lmPart.lmSize;
		CArray2D<NGfx::SPixel8888> *pRes = &lmPart.lightmap;
		pRes->SetSizes( lmSize.x, lmSize.y );
		for ( int y = 0; y < lmSize.y; ++y )
		{
			for ( int x = 0; x < lmSize.x; ++x )
				(*pRes)[y][x] = fullTex[ y + lmShift.y ][ x + lmShift.x ];
		}
		FixBorders( pRes, lmPart.lmQuads );
	}
	return true;
}

static void GenerateLightState( IGScene *pScene, IRender *pRender, CLightState *pRes, ELightmapQuality quality )
{
	SLightStateCalcSeed lsSeed;
	CLightState &ls = *pRes;
	CDGPtr<CFuncBase<SPerVertexLightState> > pLightState = pRender->GetLightState();
	pLightState.Refresh();
	const SPerVertexLightState &pvls = pLightState->GetValue();

	SSphere bound( CVec3( 20, 20, 20 ), 40 );
	switch ( quality )
	{
	case LM_QUALITY_DRAFT:
		ls.CreateScattered( &lsSeed, bound, pvls, 0, N_DEPTH_CHANNELS_PER_TEX * 50 );
		break;
	case LM_QUALITY_RADIOSITY:
		ls.CreateScattered( &lsSeed, bound, pvls, pScene, N_DEPTH_CHANNELS_PER_TEX * 500 );
		break;
	default:
		ASSERT(0);
	}
}

static void FilterLightmappableParts( vector<ISomePart*> *pRes )
{
	int nDst = 0;
	for ( int k = 0; k < pRes->size(); ++k )
	{
		ISomePart *p = (*pRes)[k];
		IMaterial *pMat = p->GetMaterial();
		if ( !pMat->DoesSupportLightmaps() )
			continue;
		if ( p->GetGroupInfo().nLightFlags & LF_SKIP_LIGHTING )
			continue;
		p->RefreshObjectInfo();
		if ( !p->GetObjectInfo()->IsLightmappable() )
			continue;
		(*pRes)[ nDst++ ] = (*pRes)[k];
	}
	pRes->resize( nDst );
}

static void CollectParts( IRender *pRender, CObjectBase *pUser, int nUserID, bool bTakeNotLoaded, vector<ISomePart*> *pRes )
{
	CTransformStack ts;
	ts.MakeParallel( 1000, 1000, -1000, 1000 );
	list<SRenderPartSet> parts;
	SGroupSelect mask( 0xffff, 0 );
	pRender->FormPartList( &ts, &parts, IRender::DT_ALL, mask );

	for ( list<SRenderPartSet>::iterator i = parts.begin(); i != parts.end(); ++i )
	{
		const SRenderPartSet &r = *i;
		for ( int k = 0; k < r.pParts->size(); ++k )
		{
			if ( r.parts.IsSet( k ) )
			{
				CDynamicCast<ISomePart> pPart = (*r.pParts)[k];
				ASSERT( IsValid(pPart) );
				CDynamicCast<CSimplePart> pSPart = pPart.GetPtr();
				CDynamicCast<CNonePart> pNPart = pPart.GetPtr();
				if ( pSPart == 0 && pNPart == 0 )
					continue;

				CPtrFuncBase<CObjectInfo> *pGeom = GetRawGeometry( pPart );
				ASSERT( pGeom );

				if ( pPart->GetFullGroupInfo().pUser != pUser )
					continue;
				int nPartUserID = pPart->GetFullGroupInfo().nUserID;
				if ( nUserID == 0 || ( nPartUserID & N_USERID_MASK ) == nUserID )
					pRes->push_back( pPart );
			}
		}
	}
	vector<IPart*> notLoaded;
	pRender->GetNotLoaded( &notLoaded );
	for ( int k = 0; k < notLoaded.size(); ++k )
	{
		if ( CDynamicCast<ISomePart> p = notLoaded[k] )
		{
			ASSERT( find( pRes->begin(), pRes->end(), p ) == pRes->end() );
			ASSERT( IsValid( p.GetPtr() ) );
			if ( IsValid( p.GetPtr() ) )
				pRes->push_back( p );
		}
	}
	FilterLightmappableParts( pRes );
}

static CLightmapsHolder *CombineLightmaps( const vector<SLMGroup> &groups )
{
	CLightmapsHolder *pRes = new CLightmapsHolder;
	CLMAlloc lmAlloc;

	for ( int k = 0; k < groups.size(); ++k )
	{
		const SLMGroup &g = groups[k];
		for ( int nPart = 0; nPart < g.parts.size(); ++nPart )
		{
			const SLMPartCalc &lm = g.parts[ nPart ];
			const CArray2D<NGfx::SPixel8888> &tex = lm.lightmap;
			ISomePart *pPart = lm.pPart;
			
			const SFullGroupInfo &info = pPart->GetFullGroupInfo();
			int nUserID = info.nUserID;
			ASSERT( nUserID != 0 );
			//ASSERT( pRes->lightmaps.find( nUserID ) == pRes->lightmaps.end() );
			
			if(  pRes->lightmaps.find( nUserID ) != pRes->lightmaps.end() )continue;
			
			int nTexture;
			CTPoint<int> texPos;
			bool bAllocOk = lmAlloc.AllocRegion( tex, &texPos, &nTexture );
			ASSERT( bAllocOk );

		
			pRes->lightmaps[ nUserID ] = CLightmapsHolder::SLightmap( nTexture, texPos, lm.fLMResolution );
		}
	}

	int nTextures = lmAlloc.GetTexturesNum();
	pRes->textures.resize( nTextures );
	for ( int k = 0; k < nTextures; ++k )
		pRes->textures[k] = lmAlloc.GetTexture( k );
	
	// output amount of generated LMs
	csSystem << nTextures << " lightmaps generated" << endl;

	return pRes;
}

CLightmapsHolder *FinalMergeLightmaps( CLightmapsTempHolder *pTmpHolder )
{
	return CombineLightmaps( pTmpHolder->lmGroups );
}
CLightmapsTempHolder *CreateLightmapsTempHolder()
{
	return new  CLightmapsTempHolder();
}

CLightmapsHolder *CalcLightmaps( IGScene *pScene, IRender *pRender, CObjectBase *pUser, int nUserID, 
	const SSphere &highResLM, ELightmapQuality quality, CLightmapsTempHolder *pTmpHolder )
{
	if ( !CanCalcLM() )
	{
		// output error result
		csSystem << "Lightmap generation requires gfx_lm_calc = 1 and hw supporting fp32 textures" << endl;
		return 0;
	}

	CLightState ls;
	GenerateLightState( pScene, pRender, &ls, quality );

	vector<SLMGroup> lmGroups;

	vector<ISomePart*> parts;
	CollectParts( pRender, pUser, nUserID, false, &parts );
	for ( int k = 0; k < parts.size(); ++k )
		AddPart( highResLM, &lmGroups, parts[k] );

	// test overlap
	//for ( int k = 0; k < lmGroups.size(); ++k )
	//{
	//	CArray2D<int> f;
	//	f.SetSizes( N_LM_TEXTURE_SIZE, N_LM_TEXTURE_SIZE );
	//	f.FillZero();
	//	const SLMGroup &g = lmGroups[k];
	//	for ( int i = 0; i < g.parts.size(); ++i )
	//	{
	//		const SLMPartCalc &p = g.parts[i];
	//		for ( int y = 0; y < p.lmSize.y; ++y )
	//		{
	//			for ( int x = 0; x < p.lmSize.x; ++x )
	//				++f[y + p.lmShift.y][x + p.lmShift.x];
	//		}
	//	}
	//	for ( int y = 0; y < f.GetSizeY(); ++y )
	//	{
	//		for ( int x = 0; x < f.GetSizeX(); ++x )
	//			ASSERT( f[y][x] < 2 );
	//	}
	//}

	if ( pTmpHolder )
	{
		for ( int k = 0; k < lmGroups.size(); ++k )
		{
			pTmpHolder->lmGroups.push_back( lmGroups[k] );

			csSystem <<  pTmpHolder->lmGroups.size()  << endl;

			if ( !CalcLM( pRender, ls, &pTmpHolder->lmGroups.back() ) )
			{
				ASSERT(0);
				return 0;
			}

		}

		return 0;
	}
	for ( int k = 0; k < lmGroups.size(); ++k )
	{
		if ( !CalcLM( pRender, ls, &lmGroups[k] ) )
		{
			ASSERT(0);
			return 0;
		}
	}

	return CombineLightmaps( lmGroups );
}

void ApplyLightmaps( IGScene *pScene, IRender *pRender, CObjectBase *pUser, CLightmapsHolder *pLightmaps, CLightmapsLoader * pLD  )
{
	
	pLightmaps = pLD->GetHolder();
	if ( !IsValid( pLightmaps ) )
	{
		//ASSERT( 0 );
		return;
	}

	int nTextures = pLightmaps->textures.size();
	vector<CObj<CLightmapTexture> > textures( nTextures );
	for ( int k = 0; k < nTextures; ++k )
		textures[k] = new CLightmapTexture( pLD,  k );

	vector<ISomePart*> parts;
	CollectParts( pRender, pUser, 0, true, &parts );
	for ( int k = 0; k < parts.size(); ++k )
	{
		ISomePart *pPart = parts[k];

		int nUserID = pPart->GetFullGroupInfo().nUserID;
		hash_map<int, CLightmapsHolder::SLightmap>::iterator i = pLightmaps->lightmaps.find( nUserID );
		if ( i == pLightmaps->lightmaps.end() )
			continue;
		const CLightmapsHolder::SLightmap &lm = i->second;

		pPart->SetLM( textures[ lm.nLightmap ] );


		CPtrFuncBase<CObjectInfo> *pGeom = GetRawGeometry( pPart );
		CGrannyMeshLoader *pML=CDynamicCast<CGrannyMeshLoader>( pGeom );
		
		extern hash_set<string>  objects;


		if( GetDumpFlag() )
		{
			if ( objects.find( pML->GetString() ) == objects.end() )
			{
				objects.insert( pML->GetString() );
				pPart->SetObjectInfoNode( new CLMGeometryGen( pGeom, lm.lmShift, lm.fLMResolution, false, true ) );

				if( GetDumpFlag() )
					pPart->RefreshObjectInfo();

			}
		}
		else
		if( pML == 0 || !pML->IsLightMapped() )
		{
	    
			pPart->SetObjectInfoNode( new CLMGeometryGen( pGeom, lm.lmShift, lm.fLMResolution, false, false ) );

		}
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////	
const CArray2D< NGfx::SPixel8888>& CLightmapsLoader::GetTexture(int UID)
{
	if ( pLM == 0 )
	{
		CFileStream stream( NVFS::GetMainVFS(), szName );
		CObj<IBinSaver> pSerialize = CreateBinSaver( &stream, SAVER_MODE_READ );
		pSerialize->Add( 1, &pLM );
		nUseCount = 0;
	}

	return pLM.GetPtr()->textures[UID];
}

CLightmapsHolder* CLightmapsLoader::GetHolder()
{
	if (pLM == 0)
	{
		CFileStream stream( NVFS::GetMainVFS(), szName );
		CObj<IBinSaver> pSerialize = CreateBinSaver( &stream, SAVER_MODE_READ );
		pSerialize->Add( 1, &pLM );
		//pLM.GetPtr()->
		nUseCount = 0;
	}

	return pLM.GetPtr();

}

void CLightmapsLoader::ReleaseHint()
{
	nUseCount++;
	if(pLM == 0)
	{
		nUseCount = 0;
		return;
	}
	if(nUseCount == pLM.GetPtr()->textures.size())
	{
		pLM = 0;
		//csSystem << "data released!" << endl;
	}

}

};
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x2011EB40, CLightmapsHolder )
REGISTER_SAVELOAD_CLASS( 0x2024BE52, CLightmapsTempHolder )

