#include "stdafx.h"
#include "Gfx.h"
#include "GfxRender.h"
#include "GfxBuffers.h"
#include "GAutoDetect.h"
#include "Misc/HPTimer.h"
#include "3Dmotor/GfxBenchmark.h"

#include "pciids/detect.h"

#include <fmt/format.h>

namespace NGScene
{
struct SCfgValue { const char *pszName; float fValue; };

// Jerky

//static SCfgValue lightingQuality0[] = {
//	{ "gfx_cl_sky_textures", 0 },
//	{ "gfx_cl_use_precise_shadows", 0 },
//	{ "gfx_cl_use_bump_always", 1 },
//	{ 0, 0 } 
//};
//static SCfgValue lightingQuality1[] = {
//	{ "gfx_cl_sky_textures", 0 },
//	{ "gfx_cl_use_precise_shadows", 1 },
//	{ "gfx_cl_use_bump_always", 0 },
//	{ 0, 0 } 
//};
//static SCfgValue lightingQuality2[] = {
//	{ "gfx_cl_sky_textures", 0 },
//	{ "gfx_cl_use_precise_shadows", 1 },
//	{ "gfx_cl_use_bump_always", 1 },
//	{ 0, 0 } 
//};
//static SCfgValue lightingQuality3[] = {
//	{ "gfx_cl_sky_textures", 4 },
//	{ "gfx_cl_use_precise_shadows", 1 },
//	{ "gfx_cl_use_bump_always", 0 },
//	{ 0, 0 } 
//};
//static SCfgValue *lightingQualityConfig[4] = { lightingQuality0, lightingQuality1, lightingQuality2, lightingQuality3 };

// Speed

static SCfgValue speed0[] = {
	//{ "gfx_fog", 2 },
	//{ "gfx_specular", 1 },
	//{ "gfx_shadows", 1 },
	//{ "gfx_cl", 1 },
	//{ "gfx_blur_sun", 1 },
	//{ "gfx_fastest", 0 },
	//{ "gfx_cl_use_bump", 1 },
	{ "gfx_tnl_mode", 0 },
	{ "gfx_noshadows", 0 },
	//{ "gfx_point_specular", 1 },
	//{ "gfx_cl_blur", 1 },
	{ 0, 0 } 
};
static SCfgValue speed1[] = {
	//{ "gfx_fog", 1 },
	//{ "gfx_specular", 1 },
	//{ "gfx_shadows", 1 },
	//{ "gfx_cl", 1 },
	//{ "gfx_blur_sun", 0 },
	//{ "gfx_fastest", 0 },
	//{ "gfx_cl_use_bump", 0 },
	{ "gfx_tnl_mode", 0 },
	{ "gfx_noshadows", 0 },
	//{ "gfx_point_specular", 0 },
	//{ "gfx_cl_blur", 1 },
	{ 0, 0 } 
};
static SCfgValue speed2[] = {
	//{ "gfx_fog", 0 },
	//{ "gfx_specular", 0 },
	//{ "gfx_shadows", 1 },
	//{ "gfx_cl", 0 },
	//{ "gfx_blur_sun", 0 },
	//{ "gfx_fastest", 0 },
	//{ "gfx_cl_use_bump", 0 },
	{ "gfx_tnl_mode", 0 },
	{ "gfx_noshadows", 1 },
	//{ "gfx_point_specular", 0 },
	//{ "gfx_cl_blur", 0 },
	{ 0, 0 } 
};
static SCfgValue speed3[] = {
	//{ "gfx_fog", 0 },
	//{ "gfx_specular", 0 },
	//{ "gfx_shadows", 0 },
	//{ "gfx_cl", 0 },
	//{ "gfx_blur_sun", 0 },
	//{ "gfx_fastest", 1 },
	//{ "gfx_cl_use_bump", 0 },
	{ "gfx_tnl_mode", 1 },
	{ "gfx_noshadows", 1 },
	//{ "gfx_point_specular", 0 },
	//{ "gfx_cl_blur", 0 },
	{ 0, 0 } 
};
static SCfgValue *speedConfig[4] = { speed0, speed1, speed2, speed3 };

// Texture

static SCfgValue texture0[] = {
	//{ "gfx_cl_cube_resolution", 32 },
	//{ "gfx_texture_usedxt", 1 },
	{ "gfx_depth_tex_resolution", 1024 },
	{ "gfx_texture_mip", 2 },
	//{ "gfx_terrain_565", 1 },
	{ 0, 0 } 
};
static SCfgValue texture1[] = {
	//{ "gfx_cl_cube_resolution", 32 },
	//{ "gfx_texture_usedxt", 1 },
	{ "gfx_depth_tex_resolution", 1024 },
	{ "gfx_texture_mip", 1 },
	{ 0, 0 } 
};
static SCfgValue texture2[] = {
	//{ "gfx_cl_cube_resolution", 64 },
	//{ "gfx_texture_usedxt", 1 },
	{ "gfx_depth_tex_resolution", 1024 },
	//{ "gfx_terrain_565", 1 },
	{ 0, 0 } 
};
static SCfgValue texture3[] = {
	//{ "gfx_cl_cube_resolution", 128 },
	//{ "gfx_texture_usedxt", 0 },
	{ "gfx_depth_tex_resolution", 2048 },
	{ "gfx_texture_mip", 0 },
	//{ "gfx_terrain_565", 0 },
	{ 0, 0 } 
};
static SCfgValue *textureConfig[4] = { texture0, texture1, texture2, texture3 };

// FSAA

static SCfgValue fsaa0[] = {
	{ "gfx_fsaa", 0 },
	{ "gfx_register_resolution", 0.5f },
	{ 0, 0 } 
};
static SCfgValue fsaa1[] = {
	{ "gfx_fsaa", 0 },
	{ "gfx_register_resolution", 1 },
	{ 0, 0 } 
};
static SCfgValue fsaa2[] = {
	{ "gfx_fsaa", 2 },
	{ "gfx_register_resolution", 2 },
	{ 0, 0 } 
};
static SCfgValue fsaa3[] = {
	{ "gfx_fsaa", 4 },
	//{ "gfx_register_resolution", 4 },
	{ "gfx_register_resolution", 2 },
	{ 0, 0 } 
};
static SCfgValue *fsaaConfig[4] = { fsaa0, fsaa1, fsaa2, fsaa3 };

static void ApplyCfgValues( SCfgValue **pCfg, int nCfgEntries, int nValue )
{
	if ( nValue == CV_CUSTOM )
		return;

	nValue = Clamp( nValue, -1, nCfgEntries );
	if ( nValue >= 0 )
	{
		SCfgValue *pCfgEntry = pCfg[ nValue ];

		for ( ; pCfgEntry->pszName; ++pCfgEntry )
			NGlobal::SetVar( pCfgEntry->pszName, pCfgEntry->fValue );
	}
}

static EConfigValue FindCfgMode( SCfgValue **pCfg, int nCfgEntries )
{
	for ( int nTemp = 0; nTemp < nCfgEntries; nTemp++ )
	{
		SCfgValue *pCfgEntry = pCfg[ nTemp ];

		bool bSame = true;
		for ( ; pCfgEntry->pszName; ++pCfgEntry )
		{
			if ( NGlobal::GetVar( pCfgEntry->pszName, pCfgEntry->fValue ).GetFloat() == pCfgEntry->fValue )
				continue;

			bSame = false;
			break;
		}
		if ( bSame )
			return EConfigValue( nTemp );
	}

	return CV_CUSTOM;
}

static bool IsLowRAM()
{
	MEMORYSTATUS memoryStatus;
	GlobalMemoryStatus( &memoryStatus );
	return memoryStatus.dwTotalPhys <= 256 * 1024 * 1024;
}

// code currently happens to require DXT support
void AutoDetectVideoConfig()
{
	NGfx::EVideoCard card = NGfx::GetVideoCard();
	NGfx::EHardwareLevel hl = NGfx::GetHardwareLevel();
	if ( hl > NGfx::HL_TNL_DEVICE )
	{
		CVec2 vScreenRect = NGfx::GetScreenRect();
		bool bDoTest = true;
		if ( vScreenRect.x == 0 )
		{
			NGfx::SRenderTargetsInfo gfxRTInfo;
			int nBPP = NGfx::Is16BitDesktop() ? 16 : 32;
			bDoTest = NGfx::SetMode( NGfx::SVideoMode( 640, 480, nBPP, NGfx::WINDOWED ), gfxRTInfo );
		}
		if ( bDoTest )
			NGfx::PerformBenchmark();
		const NGfx::SPerformanceInfo &perf = NGfx::GetPerformanceInfo();
		csSystem << CC_GREEN << "PSRate = " << perf.fPSRate << ";  FillRate = " << perf.fFillRate << ";  TriRate = " << perf.fTriangleRate << ";  CPUclock = " << perf.fCPUclock << endl;
	}
	int nDefaultSpeed = -1, nDefaultTexture = -1, nDefaultFSAA = -1;
	int nDefault16bppMode = 1;
	for ( int nTemp = 0; nTemp < std::size( configAutoDetect ); nTemp++ )
	{
		const SAutoDetectCfg &sType = configAutoDetect[nTemp];
		if ( ( sType.nCard != NGfx::VC_DEFAULT ) && ( sType.nCard != card ) )
			continue;
		if ( ( sType.nLevel != HWLEVEL_ANY ) && ( sType.nLevel != hl ) )
			continue;

		nDefaultSpeed = sType.nSpeed;
		nDefaultTexture = sType.nTexture;
		nDefault16bppMode = sType.n16bppMode;
		csSystem << CC_GREEN << "AUTODETECTED: " << sType.pszName << endl;
		break;
	}

	const NGfx::SSystemInfo &systemInfo = NGfx::GetSystemInfo();
	const NGfx::SPerformanceInfo &perf = NGfx::GetPerformanceInfo();

	bool bLowRAM = IsLowRAM();
	NGlobal::SetVar( "gfx_low_ram", bLowRAM );
	if ( bLowRAM )
	{
		// CPU implied limitations
		nDefaultSpeed = CV_VHIGH;
		nDefaultTexture = CV_MED;
	}
	else
	{
		// select texture mode
		nDefaultTexture = (std::min)( nDefaultTexture, (int)CV_VHIGH );

		if ( perf.fFillRate != 0 )
		{
			if ( perf.fTriangleRate < 5 )
				nDefaultSpeed = CV_VHIGH;
			if ( perf.fFillRate < 400 )
			{
				nDefaultSpeed = CV_HIGH;
				nDefaultTexture = (std::min)( nDefaultTexture, (int)CV_HIGH );
			}
			if ( perf.fFillRate < 190 )
			{
				NGlobal::SetVar( "gfx_resolution", "1024x768" );
				if ( perf.fFillRate < perf.fPSRate * 0.7 )
					nDefault16bppMode = 1;
				nDefaultTexture = (std::min)( nDefaultTexture, (int)CV_MED );
			}
			if ( perf.fFillRate > 700 )
			{
				NGlobal::SetVar( "gfx_anisotropic_filter", 2 );
				nDefaultFSAA = CV_VHIGH;
			}
			else
				nDefaultTexture = (std::min)( nDefaultTexture, (int)CV_HIGH );
		}
	}

	int nDesktopWidth = systemInfo.nDesktopWidth;
	int nDesktopHeight = systemInfo.nDesktopHeight;
	NGlobal::SetVar( "gfx_resolution", fmt::format("{}x{}", nDesktopWidth, nDesktopHeight) );

	if ( !NGfx::CanStreamGeometry() || perf.fCPUclock < 1400 )
		nDefaultSpeed = CV_VHIGH;
	NGlobal::SetVar( "gfx_16bit_mode", nDefault16bppMode );

	csSystem << CC_GREEN << "AUTODETECTED: Speed: " << nDefaultSpeed << " Texture: " << nDefaultTexture << " FSAA: " << nDefaultFSAA << endl;
	SetSpeedMode( (EConfigValue)nDefaultSpeed );
	SetTextureMode( (EConfigValue)nDefaultTexture );
	SetFSAAMode( (EConfigValue)nDefaultFSAA );
}

//EConfigValue GetLightingQualityMode()
//{
//	return FindCfgMode( lightingQualityConfig, std::size( lightingQualityConfig ) );
//}

//void SetLightingQualityMode( EConfigValue eMode )
//{
//	ApplyCfgValues( lightingQualityConfig, std::size( lightingQualityConfig ), eMode );
//}

EConfigValue GetSpeedMode()
{
	return FindCfgMode( speedConfig, std::size( speedConfig ) );
}

void SetSpeedMode( EConfigValue eMode )
{
	ApplyCfgValues( speedConfig, std::size( speedConfig ), eMode );
	int nHSRMode = 0;
	if ( !IsLowRAM() )
	{
		if ( eMode != CV_VHIGH )
		{
			if ( NHPTimer::GetClockRate() > 1200000000 )
				nHSRMode = 2;
			else
				nHSRMode = 1;
		}
		else
			nHSRMode = 1;
	}
	NGlobal::SetVar( "gfx_hsr", nHSRMode );
}

EConfigValue GetTextureMode()
{
	return FindCfgMode( textureConfig, std::size( textureConfig ) );
}

void SetTextureMode( EConfigValue eMode )
{
	ApplyCfgValues( textureConfig, std::size( textureConfig ), eMode );
}

EConfigValue GetFSAAMode()
{
	return FindCfgMode( fsaaConfig, std::size( fsaaConfig ) );
}
void SetFSAAMode( EConfigValue eMode )
{
	ApplyCfgValues( fsaaConfig, std::size( fsaaConfig ), eMode );
}

static void CommandGfxAutodetect( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	AutoDetectVideoConfig();
}

START_REGISTER(GAutoDetect)
	REGISTER_CMD( "gfx_autodetect", CommandGfxAutodetect )
FINISH_REGISTER

} // namespace


