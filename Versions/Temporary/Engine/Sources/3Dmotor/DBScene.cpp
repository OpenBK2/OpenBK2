// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "DBScene.h"

#include "System/UuidChunk.h"

#include "3Dmotor_export.h"

#include <cstdint>

namespace NDb
{



void SModel::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Model", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportSimpleArrayMetaInfo( "Materials", &materials, pThis );
	NMetaInfo::ReportMetaInfo( "Geometry", (uint8_t*)&pGeometry - pThis, sizeof(pGeometry), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Skeleton", (uint8_t*)&pSkeleton - pThis, sizeof(pSkeleton), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportSimpleArrayMetaInfo( "Animations", &animations, pThis );
	NMetaInfo::ReportMetaInfo( "WindPower", (uint8_t*)&fWindPower - pThis, sizeof(fWindPower), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SModel::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Materials", &materials );
	saver.Add( "Geometry", &pGeometry );
	saver.Add( "Skeleton", &pSkeleton );
	saver.Add( "Animations", &animations );
	saver.Add( "WindPower", &fWindPower );

	return 0;
}

int SModel::operator&( IBinSaver &saver )
{
	saver.Add( 2, &materials );
	saver.Add( 3, &pGeometry );
	saver.Add( 4, &pSkeleton );
	saver.Add( 5, &animations );
	saver.Add( 6, &fWindPower );

	return 0;
}


std::string EnumToString( NDb::EConvertionType eValue )
{
	switch ( eValue )
	{
	case NDb::CONVERT_ORDINARY:
		return "CONVERT_ORDINARY";
	case NDb::CONVERT_BUMP:
		return "CONVERT_BUMP";
	case NDb::CONVERT_TRANSPARENT:
		return "CONVERT_TRANSPARENT";
	case NDb::CONVERT_TRANSPARENT_ADD:
		return "CONVERT_TRANSPARENT_ADD";
	case NDb::CONVERT_LINEAR_PICTURE:
		return "CONVERT_LINEAR_PICTURE";
	case NDb::CONVERT_ORDINARY_FASTMIP:
		return "CONVERT_ORDINARY_FASTMIP";
	default:
		return "CONVERT_ORDINARY";
	}
}

NDb::EConvertionType StringToEnum_NDb_EConvertionType( const std::string &szValue )
{
	if ( szValue == "CONVERT_ORDINARY" )
		return NDb::CONVERT_ORDINARY;
	if ( szValue == "CONVERT_BUMP" )
		return NDb::CONVERT_BUMP;
	if ( szValue == "CONVERT_TRANSPARENT" )
		return NDb::CONVERT_TRANSPARENT;
	if ( szValue == "CONVERT_TRANSPARENT_ADD" )
		return NDb::CONVERT_TRANSPARENT_ADD;
	if ( szValue == "CONVERT_LINEAR_PICTURE" )
		return NDb::CONVERT_LINEAR_PICTURE;
	if ( szValue == "CONVERT_ORDINARY_FASTMIP" )
		return NDb::CONVERT_ORDINARY_FASTMIP;
	return NDb::CONVERT_ORDINARY;
}

std::string EnumToString( NDb::STexture::EType eValue )
{
	switch ( eValue )
	{
	case NDb::STexture::REGULAR:
		return "REGULAR";
	case NDb::STexture::TEXTURE_2D:
		return "TEXTURE_2D";
	default:
		return "REGULAR";
	}
}

NDb::STexture::EType StringToEnum_NDb_STexture_EType( const std::string &szValue )
{
	if ( szValue == "REGULAR" )
		return NDb::STexture::REGULAR;
	if ( szValue == "TEXTURE_2D" )
		return NDb::STexture::TEXTURE_2D;
	return NDb::STexture::REGULAR;
}

std::string EnumToString( NDb::STexture::EAddrType eValue )
{
	switch ( eValue )
	{
	case NDb::STexture::CLAMP:
		return "CLAMP";
	case NDb::STexture::WRAP:
		return "WRAP";
	case NDb::STexture::WRAP_X:
		return "WRAP_X";
	case NDb::STexture::WRAP_Y:
		return "WRAP_Y";
	default:
		return "CLAMP";
	}
}

NDb::STexture::EAddrType StringToEnum_NDb_STexture_EAddrType( const std::string &szValue )
{
	if ( szValue == "CLAMP" )
		return NDb::STexture::CLAMP;
	if ( szValue == "WRAP" )
		return NDb::STexture::WRAP;
	if ( szValue == "WRAP_X" )
		return NDb::STexture::WRAP_X;
	if ( szValue == "WRAP_Y" )
		return NDb::STexture::WRAP_Y;
	return NDb::STexture::CLAMP;
}

std::string EnumToString( NDb::STexture::EFormat eValue )
{
	switch ( eValue )
	{
	case NDb::STexture::TF_DXT1:
		return "TF_DXT1";
	case NDb::STexture::TF_DXT3:
		return "TF_DXT3";
	case NDb::STexture::TF_8888:
		return "TF_8888";
	case NDb::STexture::TF_565:
		return "TF_565";
	default:
		return "TF_DXT1";
	}
}

NDb::STexture::EFormat StringToEnum_NDb_STexture_EFormat( const std::string &szValue )
{
	if ( szValue == "TF_DXT1" )
		return NDb::STexture::TF_DXT1;
	if ( szValue == "TF_DXT3" )
		return NDb::STexture::TF_DXT3;
	if ( szValue == "TF_8888" )
		return NDb::STexture::TF_8888;
	if ( szValue == "TF_565" )
		return NDb::STexture::TF_565;
	return NDb::STexture::TF_DXT1;
}


void STexture::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Texture", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "DestName", (uint8_t*)&szDestName - pThis, sizeof(szDestName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "Type", (uint8_t*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "ConversionType", (uint8_t*)&eConversionType - pThis, sizeof(eConversionType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "AddrType", (uint8_t*)&eAddrType - pThis, sizeof(eAddrType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "Format", (uint8_t*)&eFormat - pThis, sizeof(eFormat), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "Width", (uint8_t*)&nWidth - pThis, sizeof(nWidth), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "Height", (uint8_t*)&nHeight - pThis, sizeof(nHeight), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "MappingSize", (uint8_t*)&fMappingSize - pThis, sizeof(fMappingSize), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "NMips", (uint8_t*)&nNMips - pThis, sizeof(nNMips), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "Gain", (uint8_t*)&fGain - pThis, sizeof(fGain), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "AverageColor", (uint8_t*)&nAverageColor - pThis, sizeof(nAverageColor), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "InstantLoad", (uint8_t*)&bInstantLoad - pThis, sizeof(bInstantLoad), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "IsDXT", (uint8_t*)&bIsDXT - pThis, sizeof(bIsDXT), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "FlipY", (uint8_t*)&bFlipY - pThis, sizeof(bFlipY), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::FinishMetaInfoReport();
}

int STexture::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "DestName", &szDestName );
	saver.Add( "Type", &eType );
	saver.Add( "ConversionType", &eConversionType );
	saver.Add( "AddrType", &eAddrType );
	saver.Add( "Format", &eFormat );
	saver.Add( "Width", &nWidth );
	saver.Add( "Height", &nHeight );
	saver.Add( "MappingSize", &fMappingSize );
	saver.Add( "NMips", &nNMips );
	saver.Add( "Gain", &fGain );
	saver.Add( "AverageColor", &nAverageColor );
	saver.Add( "InstantLoad", &bInstantLoad );
	saver.Add( "IsDXT", &bIsDXT );
	saver.Add( "FlipY", &bFlipY );

	return 0;
}

int STexture::operator&( IBinSaver &saver )
{
	saver.Add( 3, &szDestName );
	saver.Add( 4, &eType );
	saver.Add( 5, &eConversionType );
	saver.Add( 6, &eAddrType );
	saver.Add( 7, &eFormat );
	saver.Add( 8, &nWidth );
	saver.Add( 9, &nHeight );
	saver.Add( 10, &fMappingSize );
	saver.Add( 11, &nNMips );
	saver.Add( 12, &fGain );
	saver.Add( 13, &nAverageColor );
	saver.Add( 14, &bInstantLoad );
	saver.Add( 15, &bIsDXT );
	saver.Add( 16, &bFlipY );

	return 0;
}



void SCubeTexture::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "CubeTexture", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "PositiveX", (uint8_t*)&pPositiveX - pThis, sizeof(pPositiveX), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "PositiveY", (uint8_t*)&pPositiveY - pThis, sizeof(pPositiveY), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "PositiveZ", (uint8_t*)&pPositiveZ - pThis, sizeof(pPositiveZ), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "NegativeX", (uint8_t*)&pNegativeX - pThis, sizeof(pNegativeX), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "NegativeY", (uint8_t*)&pNegativeY - pThis, sizeof(pNegativeY), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "NegativeZ", (uint8_t*)&pNegativeZ - pThis, sizeof(pNegativeZ), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SCubeTexture::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "PositiveX", &pPositiveX );
	saver.Add( "PositiveY", &pPositiveY );
	saver.Add( "PositiveZ", &pPositiveZ );
	saver.Add( "NegativeX", &pNegativeX );
	saver.Add( "NegativeY", &pNegativeY );
	saver.Add( "NegativeZ", &pNegativeZ );

	return 0;
}

int SCubeTexture::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pPositiveX );
	saver.Add( 3, &pPositiveY );
	saver.Add( 4, &pPositiveZ );
	saver.Add( 5, &pNegativeX );
	saver.Add( 6, &pNegativeY );
	saver.Add( 7, &pNegativeZ );

	return 0;
}



void SSunFlare::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Distance", (uint8_t*)&fDistance - pThis, sizeof(fDistance), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Texture", (uint8_t*)&pTexture - pThis, sizeof(pTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Fade", (uint8_t*)&bFade - pThis, sizeof(bFade), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "Scale", (uint8_t*)&fScale - pThis, sizeof(fScale), NTypeDef::TYPE_TYPE_FLOAT );
}

int SSunFlare::operator&( IXmlSaver &saver )
{
	saver.Add( "Distance", &fDistance );
	saver.Add( "Texture", &pTexture );
	saver.Add( "Fade", &bFade );
	saver.Add( "Scale", &fScale );

	return 0;
}

int SSunFlare::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fDistance );
	saver.Add( 3, &pTexture );
	saver.Add( 4, &bFade );
	saver.Add( 5, &fScale );

	return 0;
}

uint32_t SSunFlare::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fDistance << bFade << fScale;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SSunFlares::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "SunFlares", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "Flares", &flares, pThis );
	NMetaInfo::ReportMetaInfo( "OverBright", (uint8_t*)&pOverBright - pThis, sizeof(pOverBright), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SSunFlares::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Flares", &flares );
	saver.Add( "OverBright", &pOverBright );

	return 0;
}

int SSunFlares::operator&( IBinSaver &saver )
{
	saver.Add( 2, &flares );
	saver.Add( 3, &pOverBright );

	return 0;
}



void SAmbientLight::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "AmbientLight", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportStructMetaInfo( "LightColor", &vLightColor, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "AmbientColor", &vAmbientColor, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "ShadeColor", &vShadeColor, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "IncidentShadowColor", &vIncidentShadowColor, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "ParticlesColor", &vParticlesColor, pThis ); 
	NMetaInfo::ReportMetaInfo( "Whitening", (uint8_t*)&bWhitening - pThis, sizeof(bWhitening), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "Pitch", (uint8_t*)&fPitch - pThis, sizeof(fPitch), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Yaw", (uint8_t*)&fYaw - pThis, sizeof(fYaw), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ShadowPitch", (uint8_t*)&fShadowPitch - pThis, sizeof(fShadowPitch), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ShadowYaw", (uint8_t*)&fShadowYaw - pThis, sizeof(fShadowYaw), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Sky", (uint8_t*)&pSky - pThis, sizeof(pSky), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "GlossColor", &vGlossColor, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "FogColor", &vFogColor, pThis ); 
	NMetaInfo::ReportMetaInfo( "FogStartDistance", (uint8_t*)&fFogStartDistance - pThis, sizeof(fFogStartDistance), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "FogDistance", (uint8_t*)&fFogDistance - pThis, sizeof(fFogDistance), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "VapourHeight", (uint8_t*)&fVapourHeight - pThis, sizeof(fVapourHeight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "VapourDensity", (uint8_t*)&fVapourDensity - pThis, sizeof(fVapourDensity), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "VapourNoiseParam", (uint8_t*)&fVapourNoiseParam - pThis, sizeof(fVapourNoiseParam), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "VapourSpeed", (uint8_t*)&fVapourSpeed - pThis, sizeof(fVapourSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "VapourSwitchTime", (uint8_t*)&fVapourSwitchTime - pThis, sizeof(fVapourSwitchTime), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructMetaInfo( "VapourColor", &vVapourColor, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "ShadowColor", &vShadowColor, pThis ); 
	NMetaInfo::ReportMetaInfo( "InGameUse", (uint8_t*)&bInGameUse - pThis, sizeof(bInGameUse), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportStructMetaInfo( "BackColor", &vBackColor, pThis ); 
	NMetaInfo::ReportMetaInfo( "GForce2Light", (uint8_t*)&pGForce2Light - pThis, sizeof(pGForce2Light), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "VapourStartHeight", (uint8_t*)&fVapourStartHeight - pThis, sizeof(fVapourStartHeight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "BlurStrength", (uint8_t*)&fBlurStrength - pThis, sizeof(fBlurStrength), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructMetaInfo( "GroundAmbientColor", &vGroundAmbientColor, pThis ); 
	NMetaInfo::ReportMetaInfo( "MaxShadowHeight", (uint8_t*)&fMaxShadowHeight - pThis, sizeof(fMaxShadowHeight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "SunFlares", (uint8_t*)&pSunFlares - pThis, sizeof(pSunFlares), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Haze", (uint8_t*)&pHaze - pThis, sizeof(pHaze), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "CloudTex", (uint8_t*)&pCloudTex - pThis, sizeof(pCloudTex), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "CloudSize", &vCloudSize, pThis ); 
	NMetaInfo::ReportMetaInfo( "CloudDir", (uint8_t*)&fCloudDir - pThis, sizeof(fCloudDir), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CloudSpeed", (uint8_t*)&fCloudSpeed - pThis, sizeof(fCloudSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Rain", (uint8_t*)&pRain - pThis, sizeof(pRain), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "SunFlarePitch", (uint8_t*)&fSunFlarePitch - pThis, sizeof(fSunFlarePitch), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "SunFlareYaw", (uint8_t*)&fSunFlareYaw - pThis, sizeof(fSunFlareYaw), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ShadowsMaxDetailLength", (uint8_t*)&fShadowsMaxDetailLength - pThis, sizeof(fShadowsMaxDetailLength), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "HeightFog", (uint8_t*)&pHeightFog - pThis, sizeof(pHeightFog), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "DepthOfField", (uint8_t*)&pDepthOfField - pThis, sizeof(pDepthOfField), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "DistanceFog", (uint8_t*)&pDistanceFog - pThis, sizeof(pDistanceFog), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "SkyDome", (uint8_t*)&pSkyDome - pThis, sizeof(pSkyDome), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "DymanicLightsModifications", &vDymanicLightsModifications, pThis ); 
	NMetaInfo::FinishMetaInfoReport();
}

int SAmbientLight::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "LightColor", &vLightColor );
	saver.Add( "AmbientColor", &vAmbientColor );
	saver.Add( "ShadeColor", &vShadeColor );
	saver.Add( "IncidentShadowColor", &vIncidentShadowColor );
	saver.Add( "ParticlesColor", &vParticlesColor );
	saver.Add( "Whitening", &bWhitening );
	saver.Add( "Pitch", &fPitch );
	saver.Add( "Yaw", &fYaw );
	saver.Add( "ShadowPitch", &fShadowPitch );
	saver.Add( "ShadowYaw", &fShadowYaw );
	saver.Add( "Sky", &pSky );
	saver.Add( "GlossColor", &vGlossColor );
	saver.Add( "FogColor", &vFogColor );
	saver.Add( "FogStartDistance", &fFogStartDistance );
	saver.Add( "FogDistance", &fFogDistance );
	saver.Add( "VapourHeight", &fVapourHeight );
	saver.Add( "VapourDensity", &fVapourDensity );
	saver.Add( "VapourNoiseParam", &fVapourNoiseParam );
	saver.Add( "VapourSpeed", &fVapourSpeed );
	saver.Add( "VapourSwitchTime", &fVapourSwitchTime );
	saver.Add( "VapourColor", &vVapourColor );
	saver.Add( "ShadowColor", &vShadowColor );
	saver.Add( "InGameUse", &bInGameUse );
	saver.Add( "BackColor", &vBackColor );
	saver.Add( "GForce2Light", &pGForce2Light );
	saver.Add( "VapourStartHeight", &fVapourStartHeight );
	saver.Add( "BlurStrength", &fBlurStrength );
	saver.Add( "GroundAmbientColor", &vGroundAmbientColor );
	saver.Add( "MaxShadowHeight", &fMaxShadowHeight );
	saver.Add( "SunFlares", &pSunFlares );
	saver.Add( "Haze", &pHaze );
	saver.Add( "CloudTex", &pCloudTex );
	saver.Add( "CloudSize", &vCloudSize );
	saver.Add( "CloudDir", &fCloudDir );
	saver.Add( "CloudSpeed", &fCloudSpeed );
	saver.Add( "Rain", &pRain );
	saver.Add( "SunFlarePitch", &fSunFlarePitch );
	saver.Add( "SunFlareYaw", &fSunFlareYaw );
	saver.Add( "ShadowsMaxDetailLength", &fShadowsMaxDetailLength );
	saver.Add( "HeightFog", &pHeightFog );
	saver.Add( "DepthOfField", &pDepthOfField );
	saver.Add( "DistanceFog", &pDistanceFog );
	saver.Add( "SkyDome", &pSkyDome );
	saver.Add( "DymanicLightsModifications", &vDymanicLightsModifications );

	return 0;
}

int SAmbientLight::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vLightColor );
	saver.Add( 3, &vAmbientColor );
	saver.Add( 4, &vShadeColor );
	saver.Add( 5, &vIncidentShadowColor );
	saver.Add( 6, &vParticlesColor );
	saver.Add( 7, &bWhitening );
	saver.Add( 8, &fPitch );
	saver.Add( 9, &fYaw );
	saver.Add( 10, &fShadowPitch );
	saver.Add( 11, &fShadowYaw );
	saver.Add( 12, &pSky );
	saver.Add( 13, &vGlossColor );
	saver.Add( 14, &vFogColor );
	saver.Add( 15, &fFogStartDistance );
	saver.Add( 16, &fFogDistance );
	saver.Add( 17, &fVapourHeight );
	saver.Add( 18, &fVapourDensity );
	saver.Add( 19, &fVapourNoiseParam );
	saver.Add( 20, &fVapourSpeed );
	saver.Add( 21, &fVapourSwitchTime );
	saver.Add( 22, &vVapourColor );
	saver.Add( 23, &vShadowColor );
	saver.Add( 24, &bInGameUse );
	saver.Add( 25, &vBackColor );
	saver.Add( 26, &pGForce2Light );
	saver.Add( 27, &fVapourStartHeight );
	saver.Add( 28, &fBlurStrength );
	saver.Add( 29, &vGroundAmbientColor );
	saver.Add( 30, &fMaxShadowHeight );
	saver.Add( 31, &pSunFlares );
	saver.Add( 32, &pHaze );
	saver.Add( 33, &pCloudTex );
	saver.Add( 34, &vCloudSize );
	saver.Add( 35, &fCloudDir );
	saver.Add( 36, &fCloudSpeed );
	saver.Add( 37, &pRain );
	saver.Add( 38, &fSunFlarePitch );
	saver.Add( 39, &fSunFlareYaw );
	saver.Add( 40, &fShadowsMaxDetailLength );
	saver.Add( 41, &pHeightFog );
	saver.Add( 42, &pDepthOfField );
	saver.Add( 43, &pDistanceFog );
	saver.Add( 44, &pSkyDome );
	saver.Add( 45, &vDymanicLightsModifications );

	return 0;
}



void SHeightFog::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "HeightFog", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportStructMetaInfo( "FogColor", &vFogColor, pThis ); 
	NMetaInfo::ReportMetaInfo( "MinHeight", (uint8_t*)&fMinHeight - pThis, sizeof(fMinHeight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "MaxHeight", (uint8_t*)&fMaxHeight - pThis, sizeof(fMaxHeight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SHeightFog::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "FogColor", &vFogColor );
	saver.Add( "MinHeight", &fMinHeight );
	saver.Add( "MaxHeight", &fMaxHeight );

	return 0;
}

int SHeightFog::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vFogColor );
	saver.Add( 3, &fMinHeight );
	saver.Add( 4, &fMaxHeight );

	return 0;
}



void SDepthOfField::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "DepthOfField", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "FocalDist", (uint8_t*)&fFocalDist - pThis, sizeof(fFocalDist), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "FocusRange", (uint8_t*)&fFocusRange - pThis, sizeof(fFocusRange), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SDepthOfField::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "FocalDist", &fFocalDist );
	saver.Add( "FocusRange", &fFocusRange );

	return 0;
}

int SDepthOfField::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fFocalDist );
	saver.Add( 3, &fFocusRange );

	return 0;
}



void SDistanceFog::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "DistanceFog", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportStructMetaInfo( "Color", &vColor, pThis ); 
	NMetaInfo::ReportMetaInfo( "MinDist", (uint8_t*)&fMinDist - pThis, sizeof(fMinDist), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "MaxDist", (uint8_t*)&fMaxDist - pThis, sizeof(fMaxDist), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "MinZDis", (uint8_t*)&fMinZDis - pThis, sizeof(fMinZDis), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "MaxZDis", (uint8_t*)&fMaxZDis - pThis, sizeof(fMaxZDis), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ColorTexture", (uint8_t*)&pColorTexture - pThis, sizeof(pColorTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SDistanceFog::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Color", &vColor );
	saver.Add( "MinDist", &fMinDist );
	saver.Add( "MaxDist", &fMaxDist );
	saver.Add( "MinZDis", &fMinZDis );
	saver.Add( "MaxZDis", &fMaxZDis );
	saver.Add( "ColorTexture", &pColorTexture );

	return 0;
}

int SDistanceFog::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vColor );
	saver.Add( 3, &fMinDist );
	saver.Add( 4, &fMaxDist );
	saver.Add( 5, &fMinZDis );
	saver.Add( 6, &fMaxZDis );
	saver.Add( 7, &pColorTexture );

	return 0;
}



void SSkeleton::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Skeleton", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportSimpleArrayMetaInfo( "Animations", &animations, pThis );
	NMetaInfo::ReportMetaInfo( "uid", (uint8_t*)&uid - pThis, sizeof(uid), NTypeDef::TYPE_TYPE_GUID );
	NMetaInfo::FinishMetaInfoReport();
}

int SSkeleton::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Animations", &animations );
	saver.Add( "uid", &uid );

	return 0;
}

int SSkeleton::operator&( IBinSaver &saver )
{
	saver.Add( 3, &animations );
	AddUuidChunk( saver, 4, &uid );

	return 0;
}



void SAnimBase::ReportMetaInfo() const
{
	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "uid", (uint8_t*)&uid - pThis, sizeof(uid), NTypeDef::TYPE_TYPE_GUID );
}

int SAnimBase::operator&( IXmlSaver &saver )
{
	saver.Add( "uid", &uid );

	return 0;
}

int SAnimBase::operator&( IBinSaver &saver )
{
	AddUuidChunk( saver, 3, &uid );

	return 0;
}

uint32_t SAnimBase::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << uid;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SAnimLight::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "AnimLight", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "SelectNode", (uint8_t*)&szSelectNode - pThis, sizeof(szSelectNode), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "uid", (uint8_t*)&uid - pThis, sizeof(uid), NTypeDef::TYPE_TYPE_GUID );
	NMetaInfo::FinishMetaInfoReport();
}

int SAnimLight::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "SelectNode", &szSelectNode );
	saver.Add( "uid", &uid );

	return 0;
}

int SAnimLight::operator&( IBinSaver &saver )
{
	saver.Add( 3, &szSelectNode );
	AddUuidChunk( saver, 4, &uid );

	return 0;
}



void SParticle::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Particle", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportStructMetaInfo( "WrapSize", &vWrapSize, pThis ); 
	NMetaInfo::ReportMetaInfo( "Bound", (uint8_t*)&bound - pThis, sizeof(bound), NTypeDef::TYPE_TYPE_BINARY );
	NMetaInfo::ReportMetaInfo( "PerParticleFog", (uint8_t*)&bPerParticleFog - pThis, sizeof(bPerParticleFog), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "uid", (uint8_t*)&uid - pThis, sizeof(uid), NTypeDef::TYPE_TYPE_GUID );
	NMetaInfo::FinishMetaInfoReport();
}

int SParticle::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "WrapSize", &vWrapSize );
	saver.Add( "Bound", &bound );
	saver.Add( "PerParticleFog", &bPerParticleFog );
	saver.Add( "uid", &uid );

	return 0;
}

int SParticle::operator&( IBinSaver &saver )
{
	saver.Add( 3, &vWrapSize );
	saver.Add( 4, &bound );
	saver.Add( 5, &bPerParticleFog );
	AddUuidChunk( saver, 6, &uid );

	return 0;
}



void SLightInstance::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "LightInstance", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "Light", (uint8_t*)&pLight - pThis, sizeof(pLight), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "Position", &vPosition, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "Rotation", &qRotation, pThis ); 
	NMetaInfo::ReportMetaInfo( "Scale", (uint8_t*)&fScale - pThis, sizeof(fScale), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Speed", (uint8_t*)&fSpeed - pThis, sizeof(fSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Offset", (uint8_t*)&fOffset - pThis, sizeof(fOffset), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "EndCycle", (uint8_t*)&fEndCycle - pThis, sizeof(fEndCycle), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CycleCount", (uint8_t*)&nCycleCount - pThis, sizeof(nCycleCount), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "GlueToBone", (uint8_t*)&nGlueToBone - pThis, sizeof(nGlueToBone), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::FinishMetaInfoReport();
}

int SLightInstance::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Light", &pLight );
	saver.Add( "Position", &vPosition );
	saver.Add( "Rotation", &qRotation );
	saver.Add( "Scale", &fScale );
	saver.Add( "Speed", &fSpeed );
	saver.Add( "Offset", &fOffset );
	saver.Add( "EndCycle", &fEndCycle );
	saver.Add( "CycleCount", &nCycleCount );
	saver.Add( "GlueToBone", &nGlueToBone );

	return 0;
}

int SLightInstance::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pLight );
	saver.Add( 3, &vPosition );
	saver.Add( 4, &qRotation );
	saver.Add( 5, &fScale );
	saver.Add( 6, &fSpeed );
	saver.Add( 7, &fOffset );
	saver.Add( 8, &fEndCycle );
	saver.Add( 9, &nCycleCount );
	saver.Add( 10, &nGlueToBone );

	return 0;
}


std::string EnumToString( NDb::SParticleInstance::ELight eValue )
{
	switch ( eValue )
	{
	case NDb::SParticleInstance::L_NORMAL:
		return "L_NORMAL";
	case NDb::SParticleInstance::L_LIT:
		return "L_LIT";
	default:
		return "L_NORMAL";
	}
}

NDb::SParticleInstance::ELight StringToEnum_NDb_SParticleInstance_ELight( const std::string &szValue )
{
	if ( szValue == "L_NORMAL" )
		return NDb::SParticleInstance::L_NORMAL;
	if ( szValue == "L_LIT" )
		return NDb::SParticleInstance::L_LIT;
	return NDb::SParticleInstance::L_NORMAL;
}

std::string EnumToString( NDb::SParticleInstance::EStatic eValue )
{
	switch ( eValue )
	{
	case NDb::SParticleInstance::P_STATIC:
		return "P_STATIC";
	case NDb::SParticleInstance::P_DYNAMIC:
		return "P_DYNAMIC";
	default:
		return "P_STATIC";
	}
}

NDb::SParticleInstance::EStatic StringToEnum_NDb_SParticleInstance_EStatic( const std::string &szValue )
{
	if ( szValue == "P_STATIC" )
		return NDb::SParticleInstance::P_STATIC;
	if ( szValue == "P_DYNAMIC" )
		return NDb::SParticleInstance::P_DYNAMIC;
	return NDb::SParticleInstance::P_STATIC;
}


void SParticleInstance::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "ParticleInstance", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "Light", (uint8_t*)&eLight - pThis, sizeof(eLight), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "Particle", (uint8_t*)&pParticle - pThis, sizeof(pParticle), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "Position", &vPosition, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "Rotation", &qRotation, pThis ); 
	NMetaInfo::ReportMetaInfo( "Scale", (uint8_t*)&fScale - pThis, sizeof(fScale), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Speed", (uint8_t*)&fSpeed - pThis, sizeof(fSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Offset", (uint8_t*)&fOffset - pThis, sizeof(fOffset), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "EndCycle", (uint8_t*)&fEndCycle - pThis, sizeof(fEndCycle), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CycleCount", (uint8_t*)&nCycleCount - pThis, sizeof(nCycleCount), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructMetaInfo( "Pivot", &vPivot, pThis ); 
	NMetaInfo::ReportSimpleArrayMetaInfo( "Textures", &textures, pThis );
	NMetaInfo::ReportMetaInfo( "IsCrown", (uint8_t*)&bIsCrown - pThis, sizeof(bIsCrown), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "Static", (uint8_t*)&eStatic - pThis, sizeof(eStatic), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "DoesCastShadow", (uint8_t*)&bDoesCastShadow - pThis, sizeof(bDoesCastShadow), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "GlueToBone", (uint8_t*)&nGlueToBone - pThis, sizeof(nGlueToBone), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "LeaveParticlesWhereStarted", (uint8_t*)&bLeaveParticlesWhereStarted - pThis, sizeof(bLeaveParticlesWhereStarted), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "Priority", (uint8_t*)&nPriority - pThis, sizeof(nPriority), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::FinishMetaInfoReport();
}

int SParticleInstance::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Light", &eLight );
	saver.Add( "Particle", &pParticle );
	saver.Add( "Position", &vPosition );
	saver.Add( "Rotation", &qRotation );
	saver.Add( "Scale", &fScale );
	saver.Add( "Speed", &fSpeed );
	saver.Add( "Offset", &fOffset );
	saver.Add( "EndCycle", &fEndCycle );
	saver.Add( "CycleCount", &nCycleCount );
	saver.Add( "Pivot", &vPivot );
	saver.Add( "Textures", &textures );
	saver.Add( "IsCrown", &bIsCrown );
	saver.Add( "Static", &eStatic );
	saver.Add( "DoesCastShadow", &bDoesCastShadow );
	saver.Add( "GlueToBone", &nGlueToBone );
	saver.Add( "LeaveParticlesWhereStarted", &bLeaveParticlesWhereStarted );
	saver.Add( "Priority", &nPriority );

	return 0;
}

int SParticleInstance::operator&( IBinSaver &saver )
{
	saver.Add( 3, &eLight );
	saver.Add( 4, &pParticle );
	saver.Add( 5, &vPosition );
	saver.Add( 6, &qRotation );
	saver.Add( 7, &fScale );
	saver.Add( 8, &fSpeed );
	saver.Add( 9, &fOffset );
	saver.Add( 10, &fEndCycle );
	saver.Add( 11, &nCycleCount );
	saver.Add( 12, &vPivot );
	saver.Add( 13, &textures );
	saver.Add( 14, &bIsCrown );
	saver.Add( 15, &eStatic );
	saver.Add( 16, &bDoesCastShadow );
	saver.Add( 17, &nGlueToBone );
	saver.Add( 18, &bLeaveParticlesWhereStarted );
	saver.Add( 19, &nPriority );

	return 0;
}



void SModelInstance::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "ModelInstance", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "Model", (uint8_t*)&pModel - pThis, sizeof(pModel), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "SkelAnim", (uint8_t*)&pSkelAnim - pThis, sizeof(pSkelAnim), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "Position", &vPosition, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "Rotation", &qRotation, pThis ); 
	NMetaInfo::ReportMetaInfo( "Scale", (uint8_t*)&fScale - pThis, sizeof(fScale), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Offset", (uint8_t*)&fOffset - pThis, sizeof(fOffset), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CycleLength", (uint8_t*)&fCycleLength - pThis, sizeof(fCycleLength), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CycleCount", (uint8_t*)&nCycleCount - pThis, sizeof(nCycleCount), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "GlueToBone", (uint8_t*)&nGlueToBone - pThis, sizeof(nGlueToBone), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::FinishMetaInfoReport();
}

int SModelInstance::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Model", &pModel );
	saver.Add( "SkelAnim", &pSkelAnim );
	saver.Add( "Position", &vPosition );
	saver.Add( "Rotation", &qRotation );
	saver.Add( "Scale", &fScale );
	saver.Add( "Offset", &fOffset );
	saver.Add( "CycleLength", &fCycleLength );
	saver.Add( "CycleCount", &nCycleCount );
	saver.Add( "GlueToBone", &nGlueToBone );

	return 0;
}

int SModelInstance::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pModel );
	saver.Add( 3, &pSkelAnim );
	saver.Add( 4, &vPosition );
	saver.Add( 5, &qRotation );
	saver.Add( 6, &fScale );
	saver.Add( 7, &fOffset );
	saver.Add( 8, &fCycleLength );
	saver.Add( 9, &nCycleCount );
	saver.Add( 10, &nGlueToBone );

	return 0;
}



void SEffect::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Effect", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportSimpleArrayMetaInfo( "Instances", &instances, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "Lights", &lights, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "Models", &models, pThis );
	NMetaInfo::ReportMetaInfo( "WindAffected", (uint8_t*)&bWindAffected - pThis, sizeof(bWindAffected), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "WindPower", (uint8_t*)&fWindPower - pThis, sizeof(fWindPower), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Duration", (uint8_t*)&fDuration - pThis, sizeof(fDuration), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SEffect::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Instances", &instances );
	saver.Add( "Lights", &lights );
	saver.Add( "Models", &models );
	saver.Add( "WindAffected", &bWindAffected );
	saver.Add( "WindPower", &fWindPower );
	saver.Add( "Duration", &fDuration );

	return 0;
}

int SEffect::operator&( IBinSaver &saver )
{
	saver.Add( 2, &instances );
	saver.Add( 3, &lights );
	saver.Add( 4, &models );
	saver.Add( 5, &bWindAffected );
	saver.Add( 6, &fWindPower );
	saver.Add( 7, &fDuration );

	return 0;
}



void SDecal::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Decal", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "Material", (uint8_t*)&pMaterial - pThis, sizeof(pMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Radius", (uint8_t*)&fRadius - pThis, sizeof(fRadius), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "FadeInTime", (uint8_t*)&nFadeInTime - pThis, sizeof(nFadeInTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "NoFadingTime", (uint8_t*)&nNoFadingTime - pThis, sizeof(nNoFadingTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "FadeOutTime", (uint8_t*)&nFadeOutTime - pThis, sizeof(nFadeOutTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "ExplosionHeight", (uint8_t*)&fExplosionHeight - pThis, sizeof(fExplosionHeight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SDecal::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Material", &pMaterial );
	saver.Add( "Radius", &fRadius );
	saver.Add( "FadeInTime", &nFadeInTime );
	saver.Add( "NoFadingTime", &nNoFadingTime );
	saver.Add( "FadeOutTime", &nFadeOutTime );
	saver.Add( "ExplosionHeight", &fExplosionHeight );

	return 0;
}

int SDecal::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pMaterial );
	saver.Add( 3, &fRadius );
	saver.Add( 4, &nFadeInTime );
	saver.Add( 5, &nNoFadingTime );
	saver.Add( 6, &nFadeOutTime );
	saver.Add( 7, &fExplosionHeight );

	return 0;
}


std::string EnumToString( NDb::SFont::EPitch eValue )
{
	switch ( eValue )
	{
	case NDb::SFont::DEFAULT:
		return "DEFAULT";
	default:
		return "DEFAULT";
	}
}

NDb::SFont::EPitch StringToEnum_NDb_SFont_EPitch( const std::string &szValue )
{
	if ( szValue == "DEFAULT" )
		return NDb::SFont::DEFAULT;
	return NDb::SFont::DEFAULT;
}

std::string EnumToString( NDb::SFont::ECharset eValue )
{
	switch ( eValue )
	{
	case NDb::SFont::ANSI:
		return "ANSI";
	case NDb::SFont::BALTIC:
		return "BALTIC";
	case NDb::SFont::CHINESEBIG5:
		return "CHINESEBIG5";
	case NDb::SFont::DEF_CHARSET:
		return "DEF_CHARSET";
	case NDb::SFont::EASTEUROPE:
		return "EASTEUROPE";
	case NDb::SFont::GB2312:
		return "GB2312";
	case NDb::SFont::GREEK:
		return "GREEK";
	case NDb::SFont::HANGUL:
		return "HANGUL";
	case NDb::SFont::RUSSIAN:
		return "RUSSIAN";
	case NDb::SFont::SHIFTJIS:
		return "SHIFTJIS";
	case NDb::SFont::SYMBOL:
		return "SYMBOL";
	case NDb::SFont::TURKISH:
		return "TURKISH";
	case NDb::SFont::HEBREW:
		return "HEBREW";
	case NDb::SFont::ARABIC:
		return "ARABIC";
	case NDb::SFont::THAI:
		return "THAI";
	default:
		return "ANSI";
	}
}

NDb::SFont::ECharset StringToEnum_NDb_SFont_ECharset( const std::string &szValue )
{
	if ( szValue == "ANSI" )
		return NDb::SFont::ANSI;
	if ( szValue == "BALTIC" )
		return NDb::SFont::BALTIC;
	if ( szValue == "CHINESEBIG5" )
		return NDb::SFont::CHINESEBIG5;
	if ( szValue == "DEF_CHARSET" )
		return NDb::SFont::DEF_CHARSET;
	if ( szValue == "EASTEUROPE" )
		return NDb::SFont::EASTEUROPE;
	if ( szValue == "GB2312" )
		return NDb::SFont::GB2312;
	if ( szValue == "GREEK" )
		return NDb::SFont::GREEK;
	if ( szValue == "HANGUL" )
		return NDb::SFont::HANGUL;
	if ( szValue == "RUSSIAN" )
		return NDb::SFont::RUSSIAN;
	if ( szValue == "SHIFTJIS" )
		return NDb::SFont::SHIFTJIS;
	if ( szValue == "SYMBOL" )
		return NDb::SFont::SYMBOL;
	if ( szValue == "TURKISH" )
		return NDb::SFont::TURKISH;
	if ( szValue == "HEBREW" )
		return NDb::SFont::HEBREW;
	if ( szValue == "ARABIC" )
		return NDb::SFont::ARABIC;
	if ( szValue == "THAI" )
		return NDb::SFont::THAI;
	return NDb::SFont::ANSI;
}


void SFont::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Font", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "Texture", (uint8_t*)&pTexture - pThis, sizeof(pTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "uid", (uint8_t*)&uid - pThis, sizeof(uid), NTypeDef::TYPE_TYPE_GUID );
	NMetaInfo::ReportMetaInfo( "Height", (uint8_t*)&nHeight - pThis, sizeof(nHeight), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "Thickness", (uint8_t*)&nThickness - pThis, sizeof(nThickness), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "Italic", (uint8_t*)&bItalic - pThis, sizeof(bItalic), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "Antialiased", (uint8_t*)&bAntialiased - pThis, sizeof(bAntialiased), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "Pitch", (uint8_t*)&ePitch - pThis, sizeof(ePitch), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "Charset", (uint8_t*)&eCharset - pThis, sizeof(eCharset), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "FaceName", (uint8_t*)&szFaceName - pThis, sizeof(szFaceName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "Name", (uint8_t*)&szName - pThis, sizeof(szName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "CharactersFile", (uint8_t*)&szCharactersFile - pThis, sizeof(szCharactersFile), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::FinishMetaInfoReport();
}

int SFont::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Texture", &pTexture );
	saver.Add( "uid", &uid );
	saver.Add( "Height", &nHeight );
	saver.Add( "Thickness", &nThickness );
	saver.Add( "Italic", &bItalic );
	saver.Add( "Antialiased", &bAntialiased );
	saver.Add( "Pitch", &ePitch );
	saver.Add( "Charset", &eCharset );
	saver.Add( "FaceName", &szFaceName );
	saver.Add( "Name", &szName );
	saver.Add( "CharactersFile", &szCharactersFile );

	return 0;
}

int SFont::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pTexture );
	AddUuidChunk( saver, 3, &uid );
	saver.Add( 4, &nHeight );
	saver.Add( 5, &nThickness );
	saver.Add( 6, &bItalic );
	saver.Add( 7, &bAntialiased );
	saver.Add( 8, &ePitch );
	saver.Add( 9, &eCharset );
	saver.Add( 10, &szFaceName );
	saver.Add( 11, &szName );
	saver.Add( 12, &szCharactersFile );

	return 0;
}



void SAIGeometry::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "AIGeometry", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "Volume", (uint8_t*)&fVolume - pThis, sizeof(fVolume), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "SolidPart", (uint8_t*)&fSolidPart - pThis, sizeof(fSolidPart), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructMetaInfo( "AABBCenter", &vAABBCenter, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "AABBHalfSize", &vAABBHalfSize, pThis ); 
	NMetaInfo::ReportMetaInfo( "uid", (uint8_t*)&uid - pThis, sizeof(uid), NTypeDef::TYPE_TYPE_GUID );
	NMetaInfo::FinishMetaInfoReport();
}

int SAIGeometry::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Volume", &fVolume );
	saver.Add( "SolidPart", &fSolidPart );
	saver.Add( "AABBCenter", &vAABBCenter );
	saver.Add( "AABBHalfSize", &vAABBHalfSize );
	saver.Add( "uid", &uid );

	return 0;
}

int SAIGeometry::operator&( IBinSaver &saver )
{
	saver.Add( 3, &fVolume );
	saver.Add( 4, &fSolidPart );
	saver.Add( 5, &vAABBCenter );
	saver.Add( 6, &vAABBHalfSize );
	AddUuidChunk( saver, 7, &uid );

	return 0;
}



void SGeometry::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Geometry", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "uid", (uint8_t*)&uid - pThis, sizeof(uid), NTypeDef::TYPE_TYPE_GUID );
	NMetaInfo::ReportStructMetaInfo( "Size", &vSize, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "Center", &vCenter, pThis ); 
	NMetaInfo::ReportMetaInfo( "AIGeometry", (uint8_t*)&pAIGeometry - pThis, sizeof(pAIGeometry), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "NumMeshes", (uint8_t*)&nNumMeshes - pThis, sizeof(nNumMeshes), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportSimpleArrayMetaInfo( "MaterialQuantities", &materialQuantities, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "MeshNames", &meshNames, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "MeshAnimated", &meshAnimated, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "MeshWindAffected", &meshWindAffected, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SGeometry::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "uid", &uid );
	saver.Add( "Size", &vSize );
	saver.Add( "Center", &vCenter );
	saver.Add( "AIGeometry", &pAIGeometry );
	saver.Add( "NumMeshes", &nNumMeshes );
	saver.Add( "MaterialQuantities", &materialQuantities );
	saver.Add( "MeshNames", &meshNames );
	saver.Add( "MeshAnimated", &meshAnimated );
	saver.Add( "MeshWindAffected", &meshWindAffected );

	return 0;
}

int SGeometry::operator&( IBinSaver &saver )
{
	AddUuidChunk( saver, 3, &uid );
	saver.Add( 4, &vSize );
	saver.Add( 5, &vCenter );
	saver.Add( 6, &pAIGeometry );
	saver.Add( 7, &nNumMeshes );
	saver.Add( 8, &materialQuantities );
	saver.Add( 9, &meshNames );
	saver.Add( 10, &meshAnimated );
	saver.Add( 11, &meshWindAffected );

	return 0;
}


std::string EnumToString( NDb::EAddressMode eValue )
{
	switch ( eValue )
	{
	case NDb::AM_WRAP:
		return "AM_WRAP";
	case NDb::AM_CLAMP:
		return "AM_CLAMP";
	default:
		return "AM_WRAP";
	}
}

NDb::EAddressMode StringToEnum_NDb_EAddressMode( const std::string &szValue )
{
	if ( szValue == "AM_WRAP" )
		return NDb::AM_WRAP;
	if ( szValue == "AM_CLAMP" )
		return NDb::AM_CLAMP;
	return NDb::AM_WRAP;
}

std::string EnumToString( NDb::SMaterial::ELightingMode eValue )
{
	switch ( eValue )
	{
	case NDb::SMaterial::L_NORMAL:
		return "L_NORMAL";
	case NDb::SMaterial::L_SELFILLUM:
		return "L_SELFILLUM";
	default:
		return "L_NORMAL";
	}
}

NDb::SMaterial::ELightingMode StringToEnum_NDb_SMaterial_ELightingMode( const std::string &szValue )
{
	if ( szValue == "L_NORMAL" )
		return NDb::SMaterial::L_NORMAL;
	if ( szValue == "L_SELFILLUM" )
		return NDb::SMaterial::L_SELFILLUM;
	return NDb::SMaterial::L_NORMAL;
}

std::string EnumToString( NDb::SMaterial::EEffect eValue )
{
	switch ( eValue )
	{
	case NDb::SMaterial::M_GENERIC:
		return "M_GENERIC";
	case NDb::SMaterial::M_WATER:
		return "M_WATER";
	case NDb::SMaterial::M_TRACKS:
		return "M_TRACKS";
	case NDb::SMaterial::M_TERRAIN:
		return "M_TERRAIN";
	case NDb::SMaterial::M_CLOUDS_H5:
		return "M_CLOUDS_H5";
	case NDb::SMaterial::M_ANIM_WATER:
		return "M_ANIM_WATER";
	case NDb::SMaterial::M_SURF:
		return "M_SURF";
	case NDb::SMaterial::M_SIMPLE_SKY:
		return "M_SIMPLE_SKY";
	case NDb::SMaterial::M_REFLECT_WATER:
		return "M_REFLECT_WATER";
	default:
		return "M_GENERIC";
	}
}

NDb::SMaterial::EEffect StringToEnum_NDb_SMaterial_EEffect( const std::string &szValue )
{
	if ( szValue == "M_GENERIC" )
		return NDb::SMaterial::M_GENERIC;
	if ( szValue == "M_WATER" )
		return NDb::SMaterial::M_WATER;
	if ( szValue == "M_TRACKS" )
		return NDb::SMaterial::M_TRACKS;
	if ( szValue == "M_TERRAIN" )
		return NDb::SMaterial::M_TERRAIN;
	if ( szValue == "M_CLOUDS_H5" )
		return NDb::SMaterial::M_CLOUDS_H5;
	if ( szValue == "M_ANIM_WATER" )
		return NDb::SMaterial::M_ANIM_WATER;
	if ( szValue == "M_SURF" )
		return NDb::SMaterial::M_SURF;
	if ( szValue == "M_SIMPLE_SKY" )
		return NDb::SMaterial::M_SIMPLE_SKY;
	if ( szValue == "M_REFLECT_WATER" )
		return NDb::SMaterial::M_REFLECT_WATER;
	return NDb::SMaterial::M_GENERIC;
}

std::string EnumToString( NDb::SMaterial::EAlphaMode eValue )
{
	switch ( eValue )
	{
	case NDb::SMaterial::AM_OPAQUE:
		return "AM_OPAQUE";
	case NDb::SMaterial::AM_OVERLAY:
		return "AM_OVERLAY";
	case NDb::SMaterial::AM_OVERLAY_ZWRITE:
		return "AM_OVERLAY_ZWRITE";
	case NDb::SMaterial::AM_TRANSPARENT:
		return "AM_TRANSPARENT";
	case NDb::SMaterial::AM_ALPHA_TEST:
		return "AM_ALPHA_TEST";
	case NDb::SMaterial::AM_DECAL:
		return "AM_DECAL";
	default:
		return "AM_OPAQUE";
	}
}

NDb::SMaterial::EAlphaMode StringToEnum_NDb_SMaterial_EAlphaMode( const std::string &szValue )
{
	if ( szValue == "AM_OPAQUE" )
		return NDb::SMaterial::AM_OPAQUE;
	if ( szValue == "AM_OVERLAY" )
		return NDb::SMaterial::AM_OVERLAY;
	if ( szValue == "AM_OVERLAY_ZWRITE" )
		return NDb::SMaterial::AM_OVERLAY_ZWRITE;
	if ( szValue == "AM_TRANSPARENT" )
		return NDb::SMaterial::AM_TRANSPARENT;
	if ( szValue == "AM_ALPHA_TEST" )
		return NDb::SMaterial::AM_ALPHA_TEST;
	if ( szValue == "AM_DECAL" )
		return NDb::SMaterial::AM_DECAL;
	return NDb::SMaterial::AM_OPAQUE;
}

std::string EnumToString( NDb::SMaterial::EDynamicMode eValue )
{
	switch ( eValue )
	{
	case NDb::SMaterial::DM_DONT_CARE:
		return "DM_DONT_CARE";
	case NDb::SMaterial::DM_FORCE_STATIC:
		return "DM_FORCE_STATIC";
	case NDb::SMaterial::DM_FORCE_DYNAMIC:
		return "DM_FORCE_DYNAMIC";
	default:
		return "DM_DONT_CARE";
	}
}

NDb::SMaterial::EDynamicMode StringToEnum_NDb_SMaterial_EDynamicMode( const std::string &szValue )
{
	if ( szValue == "DM_DONT_CARE" )
		return NDb::SMaterial::DM_DONT_CARE;
	if ( szValue == "DM_FORCE_STATIC" )
		return NDb::SMaterial::DM_FORCE_STATIC;
	if ( szValue == "DM_FORCE_DYNAMIC" )
		return NDb::SMaterial::DM_FORCE_DYNAMIC;
	return NDb::SMaterial::DM_DONT_CARE;
}


void SMaterial::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Material", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "Texture", (uint8_t*)&pTexture - pThis, sizeof(pTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Bump", (uint8_t*)&pBump - pThis, sizeof(pBump), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "SpecFactor", (uint8_t*)&fSpecFactor - pThis, sizeof(fSpecFactor), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructMetaInfo( "SpecColor", &vSpecColor, pThis ); 
	NMetaInfo::ReportMetaInfo( "Gloss", (uint8_t*)&pGloss - pThis, sizeof(pGloss), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "MetalMirror", (uint8_t*)&fMetalMirror - pThis, sizeof(fMetalMirror), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "DielMirror", (uint8_t*)&fDielMirror - pThis, sizeof(fDielMirror), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Mirror", (uint8_t*)&pMirror - pThis, sizeof(pMirror), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "CastShadow", (uint8_t*)&bCastShadow - pThis, sizeof(bCastShadow), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "ReceiveShadow", (uint8_t*)&bReceiveShadow - pThis, sizeof(bReceiveShadow), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "Priority", (uint8_t*)&nPriority - pThis, sizeof(nPriority), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructMetaInfo( "TranslucentColor", &vTranslucentColor, pThis ); 
	NMetaInfo::ReportMetaInfo( "FloatParam", (uint8_t*)&fFloatParam - pThis, sizeof(fFloatParam), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "DetailTexture", (uint8_t*)&pDetailTexture - pThis, sizeof(pDetailTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "DetailScale", (uint8_t*)&fDetailScale - pThis, sizeof(fDetailScale), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ProjectOnTerrain", (uint8_t*)&bProjectOnTerrain - pThis, sizeof(bProjectOnTerrain), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "LightingMode", (uint8_t*)&eLightingMode - pThis, sizeof(eLightingMode), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "DynamicMode", (uint8_t*)&eDynamicMode - pThis, sizeof(eDynamicMode), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "Is2Sided", (uint8_t*)&bIs2Sided - pThis, sizeof(bIs2Sided), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "Effect", (uint8_t*)&eEffect - pThis, sizeof(eEffect), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "AlphaMode", (uint8_t*)&eAlphaMode - pThis, sizeof(eAlphaMode), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "AffectedByFog", (uint8_t*)&bAffectedByFog - pThis, sizeof(bAffectedByFog), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "AddPlaced", (uint8_t*)&bAddPlaced - pThis, sizeof(bAddPlaced), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "IgnoreZBuffer", (uint8_t*)&bIgnoreZBuffer - pThis, sizeof(bIgnoreZBuffer), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "BackFaceCastShadow", (uint8_t*)&bBackFaceCastShadow - pThis, sizeof(bBackFaceCastShadow), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::FinishMetaInfoReport();
}

int SMaterial::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Texture", &pTexture );
	saver.Add( "Bump", &pBump );
	saver.Add( "SpecFactor", &fSpecFactor );
	saver.Add( "SpecColor", &vSpecColor );
	saver.Add( "Gloss", &pGloss );
	saver.Add( "MetalMirror", &fMetalMirror );
	saver.Add( "DielMirror", &fDielMirror );
	saver.Add( "Mirror", &pMirror );
	saver.Add( "CastShadow", &bCastShadow );
	saver.Add( "ReceiveShadow", &bReceiveShadow );
	saver.Add( "Priority", &nPriority );
	saver.Add( "TranslucentColor", &vTranslucentColor );
	saver.Add( "FloatParam", &fFloatParam );
	saver.Add( "DetailTexture", &pDetailTexture );
	saver.Add( "DetailScale", &fDetailScale );
	saver.Add( "ProjectOnTerrain", &bProjectOnTerrain );
	saver.Add( "LightingMode", &eLightingMode );
	saver.Add( "DynamicMode", &eDynamicMode );
	saver.Add( "Is2Sided", &bIs2Sided );
	saver.Add( "Effect", &eEffect );
	saver.Add( "AlphaMode", &eAlphaMode );
	saver.Add( "AffectedByFog", &bAffectedByFog );
	saver.Add( "AddPlaced", &bAddPlaced );
	saver.Add( "IgnoreZBuffer", &bIgnoreZBuffer );
	saver.Add( "BackFaceCastShadow", &bBackFaceCastShadow );

	return 0;
}

int SMaterial::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pTexture );
	saver.Add( 3, &pBump );
	saver.Add( 4, &fSpecFactor );
	saver.Add( 5, &vSpecColor );
	saver.Add( 6, &pGloss );
	saver.Add( 7, &fMetalMirror );
	saver.Add( 8, &fDielMirror );
	saver.Add( 9, &pMirror );
	saver.Add( 10, &bCastShadow );
	saver.Add( 11, &bReceiveShadow );
	saver.Add( 12, &nPriority );
	saver.Add( 13, &vTranslucentColor );
	saver.Add( 14, &fFloatParam );
	saver.Add( 15, &pDetailTexture );
	saver.Add( 16, &fDetailScale );
	saver.Add( 17, &bProjectOnTerrain );
	saver.Add( 18, &eLightingMode );
	saver.Add( 19, &eDynamicMode );
	saver.Add( 20, &bIs2Sided );
	saver.Add( 21, &eEffect );
	saver.Add( 22, &eAlphaMode );
	saver.Add( 23, &bAffectedByFog );
	saver.Add( 24, &bAddPlaced );
	saver.Add( 25, &bIgnoreZBuffer );
	saver.Add( 26, &bBackFaceCastShadow );

	return 0;
}



void SSpot::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Spot", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "Material", (uint8_t*)&pMaterial - pThis, sizeof(pMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SSpot::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Material", &pMaterial );

	return 0;
}

int SSpot::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pMaterial );

	return 0;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( _3DMOTOR, 0x12069B88, SModel )
REGISTER_DATABASE_CLASS( _3DMOTOR, 0x12069B8E, STexture )
REGISTER_DATABASE_CLASS( _3DMOTOR, 0x12069B82, SCubeTexture )
REGISTER_DATABASE_CLASS( _3DMOTOR, 0xB4406170, SSunFlares )
REGISTER_DATABASE_CLASS( _3DMOTOR, 0x12069B80, SAmbientLight )
REGISTER_DATABASE_CLASS( _3DMOTOR, 0x1318BB40, SHeightFog )
REGISTER_DATABASE_CLASS( _3DMOTOR, 0x13192480, SDepthOfField )
REGISTER_DATABASE_CLASS( _3DMOTOR, 0x1319E340, SDistanceFog )
REGISTER_DATABASE_CLASS( _3DMOTOR, 0x12069B8A, SSkeleton )
BASIC_REGISTER_DATABASE_CLASS( _3DMOTOR, SAnimBase )
REGISTER_DATABASE_CLASS( _3DMOTOR, 0x1206A301, SAnimLight )
REGISTER_DATABASE_CLASS( _3DMOTOR, 0x12069B89, SParticle )
REGISTER_DATABASE_CLASS( _3DMOTOR, 0x1206A2C1, SLightInstance )
REGISTER_DATABASE_CLASS( _3DMOTOR, 0x1206A2C0, SParticleInstance )
REGISTER_DATABASE_CLASS( _3DMOTOR, 0x5014B340, SModelInstance )
REGISTER_DATABASE_CLASS( _3DMOTOR, 0x12069B83, SEffect )
REGISTER_DATABASE_CLASS( _3DMOTOR, 0x131A73C0, SDecal )
REGISTER_DATABASE_CLASS( _3DMOTOR, 0x12069B84, SFont )
REGISTER_DATABASE_CLASS( _3DMOTOR, 0x1007EC80, SAIGeometry )
REGISTER_DATABASE_CLASS( _3DMOTOR, 0x12069B85, SGeometry )
REGISTER_DATABASE_CLASS( _3DMOTOR, 0x12069B87, SMaterial )
REGISTER_DATABASE_CLASS( _3DMOTOR, 0x12069B8B, SSpot )

