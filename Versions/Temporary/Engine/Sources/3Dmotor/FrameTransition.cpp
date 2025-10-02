#include "StdAfx.h"
#include "Gfx.h"
#include "GfxUtils.h"
#include "GRTShare.h"
#include "FrameTransition.h"
#include "..\System\Commands.h"
#include "..\Misc\Win32Random.h"

namespace NGScene
{

static CRTPtr pScreen1( "FrameTransitionSrc1" );
static CRTPtr pScreen2( "FrameTransitionSrc2" );
static unsigned int nStartTime = 0;
static bool bStartTimeInvalid = true;
static bool bComplete = true;
static int nTargetResolution = 512;

static int N_EFFECT_DURATION = NDefFTVals::N_DEF_FT_EFFECT_DURATION;
static bool B_ZOOM = NDefFTVals::B_DEF_FT_ZOOM;

static int N_QUADS_GROUP1_NUM = NDefFTVals::N_DEF_FT_QUADS_GROUP1_NUM;
static float F_QUADS_GROUP1_MIN_Z = NDefFTVals::F_DEF_FT_QUADS_GROUP1_MIN_Z;
static float F_QUADS_GROUP1_MAX_Z = NDefFTVals::F_DEF_FT_QUADS_GROUP1_MAX_Z;
static float F_QUADS_GROUP1_MIN_ALPHA = NDefFTVals::F_DEF_FT_QUADS_GROUP1_MIN_ALPHA;
static float F_QUADS_GROUP1_MAX_ALPHA = NDefFTVals::F_DEF_FT_QUADS_GROUP1_MAX_ALPHA;

static int N_QUADS_GROUP2_NUM = NDefFTVals::N_DEF_FT_QUADS_GROUP2_NUM;
static float F_QUADS_GROUP2_MIN_Z = NDefFTVals::F_DEF_FT_QUADS_GROUP2_MIN_Z;
static float F_QUADS_GROUP2_MAX_Z = NDefFTVals::F_DEF_FT_QUADS_GROUP2_MAX_Z;
static float F_QUADS_GROUP2_MIN_ALPHA = NDefFTVals::F_DEF_FT_QUADS_GROUP2_MIN_ALPHA;
static float F_QUADS_GROUP2_MAX_ALPHA = NDefFTVals::F_DEF_FT_QUADS_GROUP2_MAX_ALPHA;

static bool B_RANDOM_DIR = NDefFTVals::B_DEF_FT_RANDOM_DIR;
static CVec2 V_TRANSITION_DIR( NDefFTVals::V_DEF_FT_TRANSITION_DIR );
static float F_TRANSITION_LENGTH = NDefFTVals::F_DEF_FT_TRANSITION_LENGTH;

static void GenerateZoomQuads1( NGfx::C2DQuadsRenderer &quadsRenderer, NGfx::CTexture *pTex, float fTime, const CVec2 &vScreenRectHalf )
{
	const int nAlpha = Clamp( Float2Int( ( F_QUADS_GROUP1_MIN_ALPHA + ( F_QUADS_GROUP1_MAX_ALPHA - F_QUADS_GROUP1_MIN_ALPHA ) * fTime ) * 255 ), 0, 255 );
	const CTRect<float> imgRect( 0, 0, nTargetResolution, nTargetResolution );
	CTRect<float> scrRect;
	NGfx::SPixel8888 color( 0xff, 0xff, 0xff, nAlpha );

	for ( int i = N_QUADS_GROUP1_NUM - 1; i >= 0; --i )
	{
		const float fQuadCoeff = (float)i / ( N_QUADS_GROUP1_NUM - 1 );
		const float fQuadPrjCoeff = fQuadCoeff * fTime;

		const float fCurZ = F_QUADS_GROUP1_MIN_Z + ( F_QUADS_GROUP1_MAX_Z - F_QUADS_GROUP1_MIN_Z ) * fQuadPrjCoeff;
		const float fDelta = 1.0f / fCurZ;

		const CVec2 vOffset( V_TRANSITION_DIR.x * vScreenRectHalf.x * F_TRANSITION_LENGTH * fQuadPrjCoeff * fDelta,
												 V_TRANSITION_DIR.y * vScreenRectHalf.y * F_TRANSITION_LENGTH * fQuadPrjCoeff * fDelta );

		const float fDeltaX = fDelta * vScreenRectHalf.x;
		const float fDeltaY = fDelta * vScreenRectHalf.y;
		scrRect.Set( vScreenRectHalf.x - fDeltaX + vOffset.x, vScreenRectHalf.y - fDeltaY + vOffset.y,
								 vScreenRectHalf.x + fDeltaX + vOffset.x, vScreenRectHalf.y + fDeltaY + vOffset.y );
		if ( i == ( N_QUADS_GROUP1_NUM - 1 ) )
		{
			color.a = 0xff;
			quadsRenderer.AddRect( scrRect, pTex, imgRect, color );
			color.a = nAlpha;
		}
		else
			quadsRenderer.AddRect( scrRect, pTex, imgRect, color );
	}
}

static void GenerateZoomQuads2( NGfx::C2DQuadsRenderer &quadsRenderer, NGfx::CTexture *pTex, float fTime, const CVec2 &vScreenRect )
{
	const int nAlpha = Clamp( Float2Int( ( F_QUADS_GROUP2_MIN_ALPHA + ( F_QUADS_GROUP2_MAX_ALPHA - F_QUADS_GROUP2_MIN_ALPHA ) * fTime ) * 255 ), 0, 255 );
	CTRect<float> imgRect;
	const CTRect<float> scrRect( 0, 0, vScreenRect.x, vScreenRect.y );
	NGfx::SPixel8888 color( 0xff, 0xff, 0xff, nAlpha );
	const float fTargetSizeHalf = (float)nTargetResolution * 0.5f;

	for ( int i = 0; i < N_QUADS_GROUP2_NUM; ++i )
	{
		const float fQuadCoeff = (float)i / ( N_QUADS_GROUP2_NUM - 1 );
		const float fQuadPrjCoeff = fQuadCoeff * ( 1.0f - fTime );

		const float fDelta = F_QUADS_GROUP2_MIN_Z + ( F_QUADS_GROUP2_MAX_Z - F_QUADS_GROUP2_MIN_Z ) * fQuadPrjCoeff;

		const CVec2 vOffset( V_TRANSITION_DIR.x * fTargetSizeHalf * F_TRANSITION_LENGTH * fQuadPrjCoeff * fDelta,
												 V_TRANSITION_DIR.y * fTargetSizeHalf * F_TRANSITION_LENGTH * fQuadPrjCoeff * fDelta );

		const float fDeltaX = fDelta * fTargetSizeHalf;
		const float fDeltaY = fDelta * fTargetSizeHalf;
		imgRect.Set( fTargetSizeHalf - fDeltaX + vOffset.x, fTargetSizeHalf - fDeltaY + vOffset.y,
								 fTargetSizeHalf + fDeltaX + vOffset.x, fTargetSizeHalf + fDeltaY + vOffset.y );
		quadsRenderer.AddRect( scrRect, pTex, imgRect, color );
	}
}

bool IsFrameTransitionComplete()
{
	return (bComplete || (NGlobal::GetVar( "gfx_frame_transition", 1 ) == 0));
}

void RenderFrameTransition()
{
	if ( IsFrameTransitionComplete() )
		return;

	if ( bStartTimeInvalid )
	{
		nStartTime = GetTickCount();
		bStartTimeInvalid = false;
	}

	unsigned int nTimeDelta = GetTickCount() - nStartTime;

	const CVec2 vScreenRect = NGfx::GetScreenRect();

	NGfx::CRenderContext rc;
	rc.ClearTarget();
	rc.SetScreenRT();
	rc.SetAlphaCombine( NGfx::COMBINE_ALPHA );
	NGfx::C2DQuadsRenderer quadsRender;
	quadsRender.SetTarget( rc, vScreenRect, NGfx::QRM_DEPTH_NONE );

	const CVec2 vScreenRectHalf = vScreenRect * 0.5f;

	if ( B_ZOOM )
	{
		const float fTime = 1.0f - Clamp( (float)(nTimeDelta) / N_EFFECT_DURATION, 0.0f, 1.0f );
		GenerateZoomQuads1( quadsRender, pScreen2.GetTexture(), fTime, vScreenRectHalf );
		GenerateZoomQuads2( quadsRender, pScreen1.GetTexture(), fTime, vScreenRect );
	}
	else
	{
		const float fTime = Clamp( (float)(nTimeDelta) / N_EFFECT_DURATION, 0.0f, 1.0f );
		GenerateZoomQuads1( quadsRender, pScreen1.GetTexture(), fTime, vScreenRectHalf );
		GenerateZoomQuads2( quadsRender, pScreen2.GetTexture(), fTime, vScreenRect );
	}

	if ( nTimeDelta > N_EFFECT_DURATION )
	{
		bComplete = true;
	}
}

inline void SetNewParams( const SFrameTransitionInfo &info )
{
	N_EFFECT_DURATION = info.nEffectDuration;
	B_ZOOM = info.bZoom;
	B_RANDOM_DIR = info.bRandomDir;
	V_TRANSITION_DIR = info.vTransitionDir;
	F_TRANSITION_LENGTH = info.fTransitionLength;
	//
	N_QUADS_GROUP1_NUM = info.nQuadsGroup1Num;
	F_QUADS_GROUP1_MIN_Z = info.fQuadsGroup1MinZ;
	F_QUADS_GROUP1_MAX_Z = info.fQuadsGroup1MaxZ;
	F_QUADS_GROUP1_MIN_ALPHA = info.fQuadsGroup1MinAlpha;
	F_QUADS_GROUP1_MAX_ALPHA = info.fQuadsGroup1MaxAlpha;
	//
	N_QUADS_GROUP2_NUM = info.nQuadsGroup2Num;
	F_QUADS_GROUP2_MIN_Z = info.fQuadsGroup2MinZ;
	F_QUADS_GROUP2_MAX_Z = info.fQuadsGroup2MaxZ;
	F_QUADS_GROUP2_MIN_ALPHA = info.fQuadsGroup2MinAlpha;
	F_QUADS_GROUP2_MAX_ALPHA = info.fQuadsGroup2MaxAlpha;
}

inline void InitFrameTransitionParams()
{
	bStartTimeInvalid = true;
	bComplete = false;
	const float fAngle = NWin32Random::Random( 0.0f, FP_PI * 2 );
	if ( B_RANDOM_DIR )
		V_TRANSITION_DIR.Set( cos( fAngle ), sin( fAngle ) );
}

CRTPtr *GetFrameTransitionCapture2()
{
	return &pScreen2;
}

CRTPtr *GetFrameTransitionCapture1( const SFrameTransitionInfo &info )
{
	SetNewParams( info );
	InitFrameTransitionParams();
	return &pScreen1;
}

void StartFrameTransition()
{
	InitFrameTransitionParams();
	NGfx::CopyScreenToTexture( pScreen1.GetTexture() );
}

void StartFrameTransition( const SFrameTransitionInfo &info )
{
	SetNewParams( info );
	StartFrameTransition();
}

NGfx::CTexture *GetTexture( CRTPtr * pTex)
{
	return pTex->GetTexture();
}


START_REGISTER(FrameTransition)
//
REGISTER_VAR_EX( "ft_effect_duration", NGlobal::VarIntHandler, &N_EFFECT_DURATION, NDefFTVals::N_DEF_FT_EFFECT_DURATION, STORAGE_NONE );
REGISTER_VAR_EX( "ft_zoom", NGlobal::VarIntHandler, &B_ZOOM, NDefFTVals::B_DEF_FT_ZOOM, STORAGE_NONE );
REGISTER_VAR_EX( "ft_transition_length", NGlobal::VarIntHandler, &F_TRANSITION_LENGTH, NDefFTVals::F_DEF_FT_TRANSITION_LENGTH, STORAGE_NONE );
//
REGISTER_VAR_EX( "ft_quads_group1_num", NGlobal::VarIntHandler, &N_QUADS_GROUP1_NUM, NDefFTVals::N_DEF_FT_QUADS_GROUP1_NUM, STORAGE_NONE );
REGISTER_VAR_EX( "ft_quads_group1_min_alpha", NGlobal::VarFloatHandler, &F_QUADS_GROUP1_MIN_ALPHA, NDefFTVals::F_DEF_FT_QUADS_GROUP1_MIN_ALPHA, STORAGE_NONE );
REGISTER_VAR_EX( "ft_quads_group1_max_alpha", NGlobal::VarFloatHandler, &F_QUADS_GROUP1_MAX_ALPHA, NDefFTVals::F_DEF_FT_QUADS_GROUP1_MAX_ALPHA, STORAGE_NONE );
REGISTER_VAR_EX( "ft_quads_group1_min_z", NGlobal::VarFloatHandler, &F_QUADS_GROUP1_MIN_Z, NDefFTVals::F_DEF_FT_QUADS_GROUP1_MIN_Z, STORAGE_NONE );
REGISTER_VAR_EX( "ft_quads_group1_max_z", NGlobal::VarFloatHandler, &F_QUADS_GROUP1_MAX_Z, NDefFTVals::F_DEF_FT_QUADS_GROUP1_MAX_Z, STORAGE_NONE );
//
REGISTER_VAR_EX( "ft_quads_group2_num", NGlobal::VarIntHandler, &N_QUADS_GROUP2_NUM, NDefFTVals::N_DEF_FT_QUADS_GROUP2_NUM, STORAGE_NONE );
REGISTER_VAR_EX( "ft_quads_group2_min_alpha", NGlobal::VarFloatHandler, &F_QUADS_GROUP2_MIN_ALPHA, NDefFTVals::F_DEF_FT_QUADS_GROUP2_MIN_ALPHA, STORAGE_NONE );
REGISTER_VAR_EX( "ft_quads_group2_max_alpha", NGlobal::VarFloatHandler, &F_QUADS_GROUP2_MAX_ALPHA, NDefFTVals::F_DEF_FT_QUADS_GROUP2_MAX_ALPHA, STORAGE_NONE );
REGISTER_VAR_EX( "ft_quads_group2_min_z", NGlobal::VarFloatHandler, &F_QUADS_GROUP2_MIN_Z, NDefFTVals::F_DEF_FT_QUADS_GROUP2_MIN_Z, STORAGE_NONE );
REGISTER_VAR_EX( "ft_quads_group2_max_z", NGlobal::VarFloatHandler, &F_QUADS_GROUP2_MAX_Z, NDefFTVals::F_DEF_FT_QUADS_GROUP2_MAX_Z, STORAGE_NONE );
//
FINISH_REGISTER

}


