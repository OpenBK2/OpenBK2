#pragma once

#include "3Dmotor_export.h"

namespace NGScene
{
struct SUserRTInfo;

_3DMOTOR_EXPORT bool SetModeFromConfig( bool bReinit, const SUserRTInfo &rtInfo );
//bool CanRenderShadows();
//bool CanCacheLighting();
//bool CanCalcAmbient();
//bool CanRenderPrecisePointShadows();
//int GetCLSkyTexturesNumber();
//int GetCLCubeResolution();
bool CanCalcLM();
int GetShadowsQuality();
bool IsUsing16bitShadows();
}

