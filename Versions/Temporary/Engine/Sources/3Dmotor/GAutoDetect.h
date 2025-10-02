#pragma once

#include "3Dmotor_export.h"


namespace NGScene
{

enum EConfigValue
{
	CV_LOW,
	CV_MED,
	CV_HIGH,
	CV_VHIGH,
	CV_CUSTOM
};

_3DMOTOR_EXPORT void AutoDetectVideoConfig();

//EConfigValue GetLightingQualityMode();
//void SetLightingQualityMode( EConfigValue eMode );

_3DMOTOR_EXPORT EConfigValue GetSpeedMode();
void SetSpeedMode( EConfigValue eMode );

EConfigValue GetTextureMode();
void SetTextureMode( EConfigValue eMode );

EConfigValue GetFSAAMode();
void SetFSAAMode( EConfigValue eMode );

} // namespace


