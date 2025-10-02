#include "StdAfx.h"
#include "GInit.h"
#include "Gfx.h"
#include "GfxRender.h"
#include "System/Commands.h"
#include "GRTShare.h"
#include "GRTInfo.h"
#include "GRenderModes.h"

namespace NGScene
{
static int nDepthTexResolution = 512;
static bool bUse16BitMode = false;
static SUserRTInfo lastUserRTInfo;
static bool bCanCalcLM;
static int nShadowsQuality = SQ_SIMPLE;
static bool bFrameTransition = true;
static bool b16bitShadows = false, bIsUsing16bitShadows = false;

int GetDepthTexResolution() { return nDepthTexResolution; }
int GetShadowsQuality() { return nShadowsQuality; }
bool IsUsing16bitShadows() { return bIsUsing16bitShadows; }
bool CanCalcLM()
{
	if ( !bCanCalcLM )
	{
		bCanCalcLM = true;
		SetModeFromConfig( true, lastUserRTInfo );
	}

	return bCanCalcLM;
}
//static void Add( const SUserRTInfo &r )
//{
//	for ( int k = 0; k < r.tex.size(); ++k )
//		AddTex( r.tex[k].nResolution, r.tex[k].szName );
//}

// DXT textures support is required
bool SetModeFromConfig( bool bReinit, const SUserRTInfo &_rtInfo )
{
	SUserRTInfo rtInfo( _rtInfo );
	lastUserRTInfo = _rtInfo;
	if ( bReinit )
	{
		NGfx::Done3D();
		if ( NGfx::Init3D( NGfx::GetHWND() ) == false )
			return false;
	}
	//if ( !NGfx::IsDXTSupported() )
	//	NGlobal::SetVar( "gfx_texture_usedxt", 0 );
	if ( !NGfx::Is8888FormatSupported() )
	{
		bUse16BitMode = true;
		NGlobal::SetVar( "gfx_16bit_textures", 1 );
	}

	//NGfx::RecreateDevice();
	NGlobal::CValue sValue;

	int nModeX = 1024, nModeY = -1;
	sValue = NGlobal::GetVar( "gfx_resolution", "1024x768" );
	swscanf( sValue.GetString().c_str(), L"%dx%d", &nModeX, &nModeY );
	if ( nModeY == -1 )
	{
		if ( nModeX == 320 ) { nModeY = 200; }
		else if ( nModeX == 400 ) { nModeY = 300; }
		else if ( nModeX == 640 ) { nModeY = 480; }
		else if ( nModeX == 800 ) { nModeY = 600; }
		else if ( nModeX == 1024 ) { nModeY = 768; }
		else if ( nModeX == 1152 ) { nModeY = 864; }
		else if ( nModeX == 1280 ) { nModeY = 960; } // 4:3 resolution
		else if ( nModeX == 1600 ) { nModeY = 1200; }
		else { nModeX = 1024; nModeY = 768; }
	}

	NGfx::EFS fullScreen = NGfx::WINDOWED;
	sValue = NGlobal::GetVar( "gfx_fullscreen", 1 );
	// should not happen in MapEditor
	if ( sValue.GetFloat() == 1 )
		fullScreen = NGfx::FULL_SCREEN;
	if ( fullScreen == NGfx::WINDOWED )
		bUse16BitMode = NGfx::Is16BitDesktop();

	NGfx::EHardwareLevel hl = NGfx::GetHardwareLevel();

	nDepthTexResolution = Float2Int( NGlobal::GetVar( "gfx_depth_tex_resolution", 512 ).GetFloat() );
	nDepthTexResolution = Clamp( nDepthTexResolution, 32, 8192 );
	nDepthTexResolution = GetNextPow2( nDepthTexResolution );

	// select feature set
	bool bCanRenderShadows;
	if ( hl == NGfx::HL_TNL_DEVICE )
	{
		bCanRenderShadows = false;
	}
	else
	{
		bCanRenderShadows = true;
	}
	// determine number and types of buffers
	if ( bCanRenderShadows )
	{
		bIsUsing16bitShadows = b16bitShadows;
		rtInfo.AddTex( 512, "ParticleLight" ); // for particles and ambient
		rtInfo.AddTex( nDepthTexResolution, "DepthMap" );

		if ( IsUsing16bitShadows() )
			rtInfo.AddTex( nDepthTexResolution, "DepthMap16", NGfx::SPixel565::ID );

		rtInfo.AddTex( 512, "WaterReflection"  );
		rtInfo.AddTex( nDepthTexResolution, "DepthCopy" );


	}
	if ( bCanCalcLM && hl >= NGfx::HL_R300 )
	{
		rtInfo.AddTex( 1024, "LMDest" );
		rtInfo.AddTex( 1024, "LMFPDest1", NGfx::SPixelFFFF::ID );
		rtInfo.AddTex( 1024, "LMFPDest2", NGfx::SPixelFFFF::ID );
		rtInfo.AddCubeTex( 512, "LMPointDepth" );
	}
	else
		bCanCalcLM = false;

	if ( NGlobal::GetVar( "gfx_frame_transition", 1 ) )
	{
		rtInfo.AddTex( 512, "FrameTransitionSrc1" );
		rtInfo.AddTex( 512, "FrameTransitionSrc2" );
	}

	//rtInfo.nFloatRegisters = 1;

	NGfx::SRenderTargetsInfo gfxRTInfo;
	InitRTShare( rtInfo, &gfxRTInfo );
	gfxRTInfo.nRegisters = 1;
	int nBPP = bUse16BitMode ? 16 : 32;
	bool bRes = NGfx::SetMode( NGfx::SVideoMode( nModeX, nModeY, nBPP, fullScreen ), gfxRTInfo );
	D3DASSERT( bRes ? S_OK : E_FAIL, "NGfx::SetMode failed with X: %d Y: %d BPP: %d FULLSCREEN: %d (REG %d, F-REG %d TGTS %d)", 
		nModeX, nModeY, nBPP, fullScreen, gfxRTInfo.nRegisters, gfxRTInfo.nFloatRegisters, gfxRTInfo.targets.size() );

	if ( !bRes )
	{
		// reduce quality till can run
		int nFSAA = NGlobal::GetVar( "gfx_fsaa", 0 ).GetFloat();
		if ( nFSAA > 0 )
		{
			nFSAA /= 2;
			if ( nFSAA == 1 )
				nFSAA = 0;
			NGlobal::SetVar( "gfx_fsaa", nFSAA / 2 );
			return SetModeFromConfig( true, _rtInfo );
		}
		float fRegResolution = NGlobal::GetVar( "gfx_register_resolution", 1 );
		if ( fRegResolution > 1 )
		{
			NGlobal::SetVar( "gfx_register_resolution", fRegResolution / 2 );
			return SetModeFromConfig( true, _rtInfo );
		}
		if ( nDepthTexResolution > 512 )
		{
			NGlobal::SetVar( "gfx_depth_tex_resolution", nDepthTexResolution / 2 );
			return SetModeFromConfig( true, _rtInfo );
		}
		if ( nModeX > 1024 )
		{
			NGlobal::SetVar( "gfx_resolution", "1024x768" );
			return SetModeFromConfig( true, _rtInfo );
		}
		// in case of failure try to create device limited to fastest mode
		if ( NGlobal::GetVar( "gfx_tnl_mode", 0 ) != 1 )
		{
			NGlobal::SetVar( "gfx_tnl_mode", 1 );
			return SetModeFromConfig( true, _rtInfo );
		}
		if ( nModeX > 800 )
		{
			NGlobal::SetVar( "gfx_resolution", "800x600" );
			return SetModeFromConfig( true, _rtInfo );
		}
		if ( NGlobal::GetVar( "gfx_swvertexprocess", 0 ) != 1 )
		{
			NGlobal::SetVar( "gfx_swvertexprocess", 1 );
			return SetModeFromConfig( true, _rtInfo );
		}
		if ( NGlobal::GetVar( "gfx_fullscreen", 1 ) != 1 )
		{
			NGlobal::SetVar( "gfx_fullscreen", 1 );
			return SetModeFromConfig( true, _rtInfo );
		}
		return false;
	}

	return true;
}

void CommandGfxUpdate( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	SetModeFromConfig( false, lastUserRTInfo );
}
void CommandGfxRecreate( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	SetModeFromConfig( true, lastUserRTInfo );
}

START_REGISTER(GInit)
	REGISTER_CMD( "gfx_update", CommandGfxUpdate )
	REGISTER_CMD( "gfx_recreate", CommandGfxRecreate )
	REGISTER_VAR( "gfx_resolution", 0, "1024x768", STORAGE_USER )
	REGISTER_VAR( "gfx_fullscreen", 0, 1, STORAGE_USER )
//	REGISTER_VAR( "gfx_refreshlimit", 0, 1000, true )
	REGISTER_VAR( "gfx_depth_tex_resolution", 0, 512, STORAGE_USER )
//	REGISTER_VAR( "gfx_cl_sky_textures", 0, 0, true )
//	REGISTER_VAR( "gfx_cl_cube_resolution", 0, 16, true )
//	REGISTER_VAR( "gfx_cl", 0, 1, true )
//	REGISTER_VAR( "gfx_fastest", 0, 0, true )
//	REGISTER_VAR( "gfx_cl_use_precise_shadows", 0, 1, true )
	REGISTER_VAR_EX( "gfx_16bit_mode", NGlobal::VarBoolHandler, &bUse16BitMode, 0, STORAGE_USER )
	REGISTER_VAR_EX( "gfx_lm_calc", NGlobal::VarBoolHandler, &bCanCalcLM, 0, STORAGE_USER )
	REGISTER_VAR_EX( "gfx_shadows_quality", NGlobal::VarIntHandler, &nShadowsQuality, SQ_SIMPLE, STORAGE_USER )
	REGISTER_VAR_EX( "gfx_frame_transition", NGlobal::VarBoolHandler, &bFrameTransition, 1, STORAGE_USER )
	REGISTER_VAR_EX( "gfx_16bit_shadows", NGlobal::VarBoolHandler, &b16bitShadows, 0, STORAGE_USER )
FINISH_REGISTER

}

