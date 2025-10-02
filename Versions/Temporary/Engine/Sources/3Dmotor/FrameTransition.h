#pragma once

#include "3Dmotor_export.h"


namespace NGfx
{
	class CTexture;
}

namespace NGScene
{
	class CRTPtr;

namespace NDefFTVals
{
	const int N_DEF_FT_EFFECT_DURATION = 1000;
	const bool B_DEF_FT_ZOOM = true;
	//
	const int N_DEF_FT_QUADS_GROUP1_NUM = 10;
	const float F_DEF_FT_QUADS_GROUP1_MIN_Z = 1.0f;
	const float F_DEF_FT_QUADS_GROUP1_MAX_Z = 4.0f;
	const float F_DEF_FT_QUADS_GROUP1_MIN_ALPHA = 0.264f;
	const float F_DEF_FT_QUADS_GROUP1_MAX_ALPHA = 0.264f;
	//
	const int N_DEF_FT_QUADS_GROUP2_NUM = 10;
	const float F_DEF_FT_QUADS_GROUP2_MIN_Z = 1.0f;
	const float F_DEF_FT_QUADS_GROUP2_MAX_Z = /*0.7f*/0.5f;
	const float F_DEF_FT_QUADS_GROUP2_MIN_ALPHA = 0.0f;
	const float F_DEF_FT_QUADS_GROUP2_MAX_ALPHA = 0.15f;
	//
	const bool B_DEF_FT_RANDOM_DIR = true;
	const CVec2 V_DEF_FT_TRANSITION_DIR( 0, 0 );
	const float F_DEF_FT_TRANSITION_LENGTH = 3.0f;
}

struct SFrameTransitionInfo
{
	int nEffectDuration;
	bool bZoom;
	//
	bool bRandomDir;
	CVec2 vTransitionDir;
	float fTransitionLength;
	//
	int nQuadsGroup1Num;
	float fQuadsGroup1MinZ;
	float fQuadsGroup1MaxZ;
	float fQuadsGroup1MinAlpha;
	float fQuadsGroup1MaxAlpha;
	//
	int nQuadsGroup2Num;
	float fQuadsGroup2MinZ;
	float fQuadsGroup2MaxZ;
	float fQuadsGroup2MinAlpha;
	float fQuadsGroup2MaxAlpha;
	//
	SFrameTransitionInfo() : nEffectDuration( NDefFTVals::N_DEF_FT_EFFECT_DURATION ), bZoom( NDefFTVals::B_DEF_FT_ZOOM ),
		bRandomDir( NDefFTVals::B_DEF_FT_RANDOM_DIR ), vTransitionDir( NDefFTVals::V_DEF_FT_TRANSITION_DIR ), fTransitionLength( NDefFTVals::F_DEF_FT_TRANSITION_LENGTH ),
		nQuadsGroup1Num( NDefFTVals::N_DEF_FT_QUADS_GROUP1_NUM ), fQuadsGroup1MinZ( NDefFTVals::F_DEF_FT_QUADS_GROUP1_MIN_Z ), fQuadsGroup1MaxZ( NDefFTVals::F_DEF_FT_QUADS_GROUP1_MAX_Z ),
		fQuadsGroup1MinAlpha( NDefFTVals::F_DEF_FT_QUADS_GROUP1_MIN_ALPHA ), fQuadsGroup1MaxAlpha( NDefFTVals::F_DEF_FT_QUADS_GROUP1_MAX_ALPHA ),
		nQuadsGroup2Num( NDefFTVals::N_DEF_FT_QUADS_GROUP2_NUM ), fQuadsGroup2MinZ( NDefFTVals::F_DEF_FT_QUADS_GROUP2_MIN_Z ), fQuadsGroup2MaxZ( NDefFTVals::F_DEF_FT_QUADS_GROUP2_MAX_Z ),
		fQuadsGroup2MinAlpha( NDefFTVals::F_DEF_FT_QUADS_GROUP2_MIN_ALPHA ), fQuadsGroup2MaxAlpha( NDefFTVals::F_DEF_FT_QUADS_GROUP2_MAX_ALPHA ) {}
};

void StartFrameTransition();
void StartFrameTransition( const SFrameTransitionInfo &info );
_3DMOTOR_EXPORT bool IsFrameTransitionComplete();
_3DMOTOR_EXPORT void RenderFrameTransition();
_3DMOTOR_EXPORT CRTPtr *GetFrameTransitionCapture2();
_3DMOTOR_EXPORT CRTPtr *GetFrameTransitionCapture1( const SFrameTransitionInfo &info ); // need for B2 interfaces using (flip is not placed in Interface::Draw() necessary)

NGfx::CTexture *GetTexture( CRTPtr * pTex);

}


