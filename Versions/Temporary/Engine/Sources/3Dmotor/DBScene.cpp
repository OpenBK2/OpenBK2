// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "dbscene.h"

namespace NDb
{



void SModel::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Model", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportSimpleArrayMetaInfo( "Materials", &materials, pThis );
	NMetaInfo::ReportMetaInfo( "Geometry", (BYTE*)&pGeometry - pThis, sizeof(pGeometry), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Skeleton", (BYTE*)&pSkeleton - pThis, sizeof(pSkeleton), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportSimpleArrayMetaInfo( "Animations", &animations, pThis );
	NMetaInfo::ReportMetaInfo( "WindPower", (BYTE*)&fWindPower - pThis, sizeof(fWindPower), NTypeDef::TYPE_TYPE_FLOAT );
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


string EnumToString( NDb::EConvertionType eValue )
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

NDb::EConvertionType NDb::StringToEnum_NDb_EConvertionType( const string &szValue )
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

string EnumToString( NDb::STexture::EType eValue )
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

NDb::STexture::EType NDb::StringToEnum_NDb_STexture_EType( const string &szValue )
{
	if ( szValue == "REGULAR" )
		return NDb::STexture::REGULAR;
	if ( szValue == "TEXTURE_2D" )
		return NDb::STexture::TEXTURE_2D;
	return NDb::STexture::REGULAR;
}

string EnumToString( NDb::STexture::EAddrType eValue )
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

NDb::STexture::EAddrType NDb::StringToEnum_NDb_STexture_EAddrType( const string &szValue )
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

string EnumToString( NDb::STexture::EFormat eValue )
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

NDb::STexture::EFormat NDb::StringToEnum_NDb_STexture_EFormat( const string &szValue )
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "DestName", (BYTE*)&szDestName - pThis, sizeof(szDestName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "Type", (BYTE*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "ConversionType", (BYTE*)&eConversionType - pThis, sizeof(eConversionType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "AddrType", (BYTE*)&eAddrType - pThis, sizeof(eAddrType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "Format", (BYTE*)&eFormat - pThis, sizeof(eFormat), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "Width", (BYTE*)&nWidth - pThis, sizeof(nWidth), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "Height", (BYTE*)&nHeight - pThis, sizeof(nHeight), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "MappingSize", (BYTE*)&fMappingSize - pThis, sizeof(fMappingSize), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "NMips", (BYTE*)&nNMips - pThis, sizeof(nNMips), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "Gain", (BYTE*)&fGain - pThis, sizeof(fGain), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "AverageColor", (BYTE*)&nAverageColor - pThis, sizeof(nAverageColor), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "InstantLoad", (BYTE*)&bInstantLoad - pThis, sizeof(bInstantLoad), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "IsDXT", (BYTE*)&bIsDXT - pThis, sizeof(bIsDXT), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "FlipY", (BYTE*)&bFlipY - pThis, sizeof(bFlipY), NTypeDef::TYPE_TYPE_BOOL );
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "PositiveX", (BYTE*)&pPositiveX - pThis, sizeof(pPositiveX), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "PositiveY", (BYTE*)&pPositiveY - pThis, sizeof(pPositiveY), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "PositiveZ", (BYTE*)&pPositiveZ - pThis, sizeof(pPositiveZ), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "NegativeX", (BYTE*)&pNegativeX - pThis, sizeof(pNegativeX), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "NegativeY", (BYTE*)&pNegativeY - pThis, sizeof(pNegativeY), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "NegativeZ", (BYTE*)&pNegativeZ - pThis, sizeof(pNegativeZ), NTypeDef::TYPE_TYPE_REF );
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



void SSunFlare::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Distance", (BYTE*)&fDistance - pThis, sizeof(fDistance), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Texture", (BYTE*)&pTexture - pThis, sizeof(pTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Fade", (BYTE*)&bFade - pThis, sizeof(bFade), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "Scale", (BYTE*)&fScale - pThis, sizeof(fScale), NTypeDef::TYPE_TYPE_FLOAT );
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

DWORD SSunFlare::CalcCheckSum() const
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "Flares", &flares, pThis );
	NMetaInfo::ReportMetaInfo( "OverBright", (BYTE*)&pOverBright - pThis, sizeof(pOverBright), NTypeDef::TYPE_TYPE_REF );
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructMetaInfo( "LightColor", &vLightColor, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "AmbientColor", &vAmbientColor, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "ShadeColor", &vShadeColor, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "IncidentShadowColor", &vIncidentShadowColor, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "ParticlesColor", &vParticlesColor, pThis ); 
	NMetaInfo::ReportMetaInfo( "Whitening", (BYTE*)&bWhitening - pThis, sizeof(bWhitening), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "Pitch", (BYTE*)&fPitch - pThis, sizeof(fPitch), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Yaw", (BYTE*)&fYaw - pThis, sizeof(fYaw), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ShadowPitch", (BYTE*)&fShadowPitch - pThis, sizeof(fShadowPitch), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ShadowYaw", (BYTE*)&fShadowYaw - pThis, sizeof(fShadowYaw), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Sky", (BYTE*)&pSky - pThis, sizeof(pSky), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "GlossColor", &vGlossColor, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "FogColor", &vFogColor, pThis ); 
	NMetaInfo::ReportMetaInfo( "FogStartDistance", (BYTE*)&fFogStartDistance - pThis, sizeof(fFogStartDistance), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "FogDistance", (BYTE*)&fFogDistance - pThis, sizeof(fFogDistance), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "VapourHeight", (BYTE*)&fVapourHeight - pThis, sizeof(fVapourHeight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "VapourDensity", (BYTE*)&fVapourDensity - pThis, sizeof(fVapourDensity), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "VapourNoiseParam", (BYTE*)&fVapourNoiseParam - pThis, sizeof(fVapourNoiseParam), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "VapourSpeed", (BYTE*)&fVapourSpeed - pThis, sizeof(fVapourSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "VapourSwitchTime", (BYTE*)&fVapourSwitchTime - pThis, sizeof(fVapourSwitchTime), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructMetaInfo( "VapourColor", &vVapourColor, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "ShadowColor", &vShadowColor, pThis ); 
	NMetaInfo::ReportMetaInfo( "InGameUse", (BYTE*)&bInGameUse - pThis, sizeof(bInGameUse), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportStructMetaInfo( "BackColor", &vBackColor, pThis ); 
	NMetaInfo::ReportMetaInfo( "GForce2Light", (BYTE*)&pGForce2Light - pThis, sizeof(pGForce2Light), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "VapourStartHeight", (BYTE*)&fVapourStartHeight - pThis, sizeof(fVapourStartHeight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "BlurStrength", (BYTE*)&fBlurStrength - pThis, sizeof(fBlurStrength), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructMetaInfo( "GroundAmbientColor", &vGroundAmbientColor, pThis ); 
	NMetaInfo::ReportMetaInfo( "MaxShadowHeight", (BYTE*)&fMaxShadowHeight - pThis, sizeof(fMaxShadowHeight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "SunFlares", (BYTE*)&pSunFlares - pThis, sizeof(pSunFlares), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Haze", (BYTE*)&pHaze - pThis, sizeof(pHaze), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "CloudTex", (BYTE*)&pCloudTex - pThis, sizeof(pCloudTex), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "CloudSize", &vCloudSize, pThis ); 
	NMetaInfo::ReportMetaInfo( "CloudDir", (BYTE*)&fCloudDir - pThis, sizeof(fCloudDir), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CloudSpeed", (BYTE*)&fCloudSpeed - pThis, sizeof(fCloudSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Rain", (BYTE*)&pRain - pThis, sizeof(pRain), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "SunFlarePitch", (BYTE*)&fSunFlarePitch - pThis, sizeof(fSunFlarePitch), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "SunFlareYaw", (BYTE*)&fSunFlareYaw - pThis, sizeof(fSunFlareYaw), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ShadowsMaxDetailLength", (BYTE*)&fShadowsMaxDetailLength - pThis, sizeof(fShadowsMaxDetailLength), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "HeightFog", (BYTE*)&pHeightFog - pThis, sizeof(pHeightFog), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "DepthOfField", (BYTE*)&pDepthOfField - pThis, sizeof(pDepthOfField), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "DistanceFog", (BYTE*)&pDistanceFog - pThis, sizeof(pDistanceFog), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "SkyDome", (BYTE*)&pSkyDome - pThis, sizeof(pSkyDome), NTypeDef::TYPE_TYPE_REF );
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructMetaInfo( "FogColor", &vFogColor, pThis ); 
	NMetaInfo::ReportMetaInfo( "MinHeight", (BYTE*)&fMinHeight - pThis, sizeof(fMinHeight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "MaxHeight", (BYTE*)&fMaxHeight - pThis, sizeof(fMaxHeight), NTypeDef::TYPE_TYPE_FLOAT );
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "FocalDist", (BYTE*)&fFocalDist - pThis, sizeof(fFocalDist), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "FocusRange", (BYTE*)&fFocusRange - pThis, sizeof(fFocusRange), NTypeDef::TYPE_TYPE_FLOAT );
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructMetaInfo( "Color", &vColor, pThis ); 
	NMetaInfo::ReportMetaInfo( "MinDist", (BYTE*)&fMinDist - pThis, sizeof(fMinDist), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "MaxDist", (BYTE*)&fMaxDist - pThis, sizeof(fMaxDist), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "MinZDis", (BYTE*)&fMinZDis - pThis, sizeof(fMinZDis), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "MaxZDis", (BYTE*)&fMaxZDis - pThis, sizeof(fMaxZDis), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ColorTexture", (BYTE*)&pColorTexture - pThis, sizeof(pColorTexture), NTypeDef::TYPE_TYPE_REF );
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportSimpleArrayMetaInfo( "Animations", &animations, pThis );
	NMetaInfo::ReportMetaInfo( "uid", (BYTE*)&uid - pThis, sizeof(uid), NTypeDef::TYPE_TYPE_GUID );
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
	saver.Add( 4, &uid );

	return 0;
}



void SAnimBase::ReportMetaInfo() const
{
	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "uid", (BYTE*)&uid - pThis, sizeof(uid), NTypeDef::TYPE_TYPE_GUID );
}

int SAnimBase::operator&( IXmlSaver &saver )
{
	saver.Add( "uid", &uid );

	return 0;
}

int SAnimBase::operator&( IBinSaver &saver )
{
	saver.Add( 3, &uid );

	return 0;
}

DWORD SAnimBase::CalcCheckSum() const
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "SelectNode", (BYTE*)&szSelectNode - pThis, sizeof(szSelectNode), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "uid", (BYTE*)&uid - pThis, sizeof(uid), NTypeDef::TYPE_TYPE_GUID );
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
	saver.Add( 4, &uid );

	return 0;
}



void SParticle::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Particle", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructMetaInfo( "WrapSize", &vWrapSize, pThis ); 
	NMetaInfo::ReportMetaInfo( "Bound", (BYTE*)&bound - pThis, sizeof(bound), NTypeDef::TYPE_TYPE_BINARY );
	NMetaInfo::ReportMetaInfo( "PerParticleFog", (BYTE*)&bPerParticleFog - pThis, sizeof(bPerParticleFog), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "uid", (BYTE*)&uid - pThis, sizeof(uid), NTypeDef::TYPE_TYPE_GUID );
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
	saver.Add( 6, &uid );

	return 0;
}



void SLightInstance::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "LightInstance", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Light", (BYTE*)&pLight - pThis, sizeof(pLight), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "Position", &vPosition, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "Rotation", &qRotation, pThis ); 
	NMetaInfo::ReportMetaInfo( "Scale", (BYTE*)&fScale - pThis, sizeof(fScale), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Speed", (BYTE*)&fSpeed - pThis, sizeof(fSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Offset", (BYTE*)&fOffset - pThis, sizeof(fOffset), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "EndCycle", (BYTE*)&fEndCycle - pThis, sizeof(fEndCycle), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CycleCount", (BYTE*)&nCycleCount - pThis, sizeof(nCycleCount), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "GlueToBone", (BYTE*)&nGlueToBone - pThis, sizeof(nGlueToBone), NTypeDef::TYPE_TYPE_INT );
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


string EnumToString( NDb::SParticleInstance::ELight eValue )
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

NDb::SParticleInstance::ELight NDb::StringToEnum_NDb_SParticleInstance_ELight( const string &szValue )
{
	if ( szValue == "L_NORMAL" )
		return NDb::SParticleInstance::L_NORMAL;
	if ( szValue == "L_LIT" )
		return NDb::SParticleInstance::L_LIT;
	return NDb::SParticleInstance::L_NORMAL;
}

string EnumToString( NDb::SParticleInstance::EStatic eValue )
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

NDb::SParticleInstance::EStatic NDb::StringToEnum_NDb_SParticleInstance_EStatic( const string &szValue )
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Light", (BYTE*)&eLight - pThis, sizeof(eLight), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "Particle", (BYTE*)&pParticle - pThis, sizeof(pParticle), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "Position", &vPosition, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "Rotation", &qRotation, pThis ); 
	NMetaInfo::ReportMetaInfo( "Scale", (BYTE*)&fScale - pThis, sizeof(fScale), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Speed", (BYTE*)&fSpeed - pThis, sizeof(fSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Offset", (BYTE*)&fOffset - pThis, sizeof(fOffset), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "EndCycle", (BYTE*)&fEndCycle - pThis, sizeof(fEndCycle), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CycleCount", (BYTE*)&nCycleCount - pThis, sizeof(nCycleCount), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructMetaInfo( "Pivot", &vPivot, pThis ); 
	NMetaInfo::ReportSimpleArrayMetaInfo( "Textures", &textures, pThis );
	NMetaInfo::ReportMetaInfo( "IsCrown", (BYTE*)&bIsCrown - pThis, sizeof(bIsCrown), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "Static", (BYTE*)&eStatic - pThis, sizeof(eStatic), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "DoesCastShadow", (BYTE*)&bDoesCastShadow - pThis, sizeof(bDoesCastShadow), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "GlueToBone", (BYTE*)&nGlueToBone - pThis, sizeof(nGlueToBone), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "LeaveParticlesWhereStarted", (BYTE*)&bLeaveParticlesWhereStarted - pThis, sizeof(bLeaveParticlesWhereStarted), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "Priority", (BYTE*)&nPriority - pThis, sizeof(nPriority), NTypeDef::TYPE_TYPE_INT );
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Model", (BYTE*)&pModel - pThis, sizeof(pModel), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "SkelAnim", (BYTE*)&pSkelAnim - pThis, sizeof(pSkelAnim), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "Position", &vPosition, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "Rotation", &qRotation, pThis ); 
	NMetaInfo::ReportMetaInfo( "Scale", (BYTE*)&fScale - pThis, sizeof(fScale), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Offset", (BYTE*)&fOffset - pThis, sizeof(fOffset), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CycleLength", (BYTE*)&fCycleLength - pThis, sizeof(fCycleLength), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CycleCount", (BYTE*)&nCycleCount - pThis, sizeof(nCycleCount), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "GlueToBone", (BYTE*)&nGlueToBone - pThis, sizeof(nGlueToBone), NTypeDef::TYPE_TYPE_INT );
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportSimpleArrayMetaInfo( "Instances", &instances, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "Lights", &lights, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "Models", &models, pThis );
	NMetaInfo::ReportMetaInfo( "WindAffected", (BYTE*)&bWindAffected - pThis, sizeof(bWindAffected), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "WindPower", (BYTE*)&fWindPower - pThis, sizeof(fWindPower), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Duration", (BYTE*)&fDuration - pThis, sizeof(fDuration), NTypeDef::TYPE_TYPE_FLOAT );
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Material", (BYTE*)&pMaterial - pThis, sizeof(pMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Radius", (BYTE*)&fRadius - pThis, sizeof(fRadius), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "FadeInTime", (BYTE*)&nFadeInTime - pThis, sizeof(nFadeInTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "NoFadingTime", (BYTE*)&nNoFadingTime - pThis, sizeof(nNoFadingTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "FadeOutTime", (BYTE*)&nFadeOutTime - pThis, sizeof(nFadeOutTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "ExplosionHeight", (BYTE*)&fExplosionHeight - pThis, sizeof(fExplosionHeight), NTypeDef::TYPE_TYPE_FLOAT );
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


string EnumToString( NDb::SFont::EPitch eValue )
{
	switch ( eValue )
	{
	case NDb::SFont::DEFAULT:
		return "DEFAULT";
	default:
		return "DEFAULT";
	}
}

NDb::SFont::EPitch NDb::StringToEnum_NDb_SFont_EPitch( const string &szValue )
{
	if ( szValue == "DEFAULT" )
		return NDb::SFont::DEFAULT;
	return NDb::SFont::DEFAULT;
}

string EnumToString( NDb::SFont::ECharset eValue )
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

NDb::SFont::ECharset NDb::StringToEnum_NDb_SFont_ECharset( const string &szValue )
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Texture", (BYTE*)&pTexture - pThis, sizeof(pTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "uid", (BYTE*)&uid - pThis, sizeof(uid), NTypeDef::TYPE_TYPE_GUID );
	NMetaInfo::ReportMetaInfo( "Height", (BYTE*)&nHeight - pThis, sizeof(nHeight), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "Thickness", (BYTE*)&nThickness - pThis, sizeof(nThickness), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "Italic", (BYTE*)&bItalic - pThis, sizeof(bItalic), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "Antialiased", (BYTE*)&bAntialiased - pThis, sizeof(bAntialiased), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "Pitch", (BYTE*)&ePitch - pThis, sizeof(ePitch), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "Charset", (BYTE*)&eCharset - pThis, sizeof(eCharset), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "FaceName", (BYTE*)&szFaceName - pThis, sizeof(szFaceName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "Name", (BYTE*)&szName - pThis, sizeof(szName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "CharactersFile", (BYTE*)&szCharactersFile - pThis, sizeof(szCharactersFile), NTypeDef::TYPE_TYPE_STRING );
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
	saver.Add( 3, &uid );
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Volume", (BYTE*)&fVolume - pThis, sizeof(fVolume), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "SolidPart", (BYTE*)&fSolidPart - pThis, sizeof(fSolidPart), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructMetaInfo( "AABBCenter", &vAABBCenter, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "AABBHalfSize", &vAABBHalfSize, pThis ); 
	NMetaInfo::ReportMetaInfo( "uid", (BYTE*)&uid - pThis, sizeof(uid), NTypeDef::TYPE_TYPE_GUID );
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
	saver.Add( 7, &uid );

	return 0;
}



void SGeometry::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Geometry", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "uid", (BYTE*)&uid - pThis, sizeof(uid), NTypeDef::TYPE_TYPE_GUID );
	NMetaInfo::ReportStructMetaInfo( "Size", &vSize, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "Center", &vCenter, pThis ); 
	NMetaInfo::ReportMetaInfo( "AIGeometry", (BYTE*)&pAIGeometry - pThis, sizeof(pAIGeometry), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "NumMeshes", (BYTE*)&nNumMeshes - pThis, sizeof(nNumMeshes), NTypeDef::TYPE_TYPE_INT );
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
	saver.Add( 3, &uid );
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


string EnumToString( NDb::EAddressMode eValue )
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

NDb::EAddressMode NDb::StringToEnum_NDb_EAddressMode( const string &szValue )
{
	if ( szValue == "AM_WRAP" )
		return NDb::AM_WRAP;
	if ( szValue == "AM_CLAMP" )
		return NDb::AM_CLAMP;
	return NDb::AM_WRAP;
}

string EnumToString( NDb::SMaterial::ELightingMode eValue )
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

NDb::SMaterial::ELightingMode NDb::StringToEnum_NDb_SMaterial_ELightingMode( const string &szValue )
{
	if ( szValue == "L_NORMAL" )
		return NDb::SMaterial::L_NORMAL;
	if ( szValue == "L_SELFILLUM" )
		return NDb::SMaterial::L_SELFILLUM;
	return NDb::SMaterial::L_NORMAL;
}

string EnumToString( NDb::SMaterial::EEffect eValue )
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

NDb::SMaterial::EEffect NDb::StringToEnum_NDb_SMaterial_EEffect( const string &szValue )
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

string EnumToString( NDb::SMaterial::EAlphaMode eValue )
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

NDb::SMaterial::EAlphaMode NDb::StringToEnum_NDb_SMaterial_EAlphaMode( const string &szValue )
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

string EnumToString( NDb::SMaterial::EDynamicMode eValue )
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

NDb::SMaterial::EDynamicMode NDb::StringToEnum_NDb_SMaterial_EDynamicMode( const string &szValue )
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Texture", (BYTE*)&pTexture - pThis, sizeof(pTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Bump", (BYTE*)&pBump - pThis, sizeof(pBump), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "SpecFactor", (BYTE*)&fSpecFactor - pThis, sizeof(fSpecFactor), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructMetaInfo( "SpecColor", &vSpecColor, pThis ); 
	NMetaInfo::ReportMetaInfo( "Gloss", (BYTE*)&pGloss - pThis, sizeof(pGloss), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "MetalMirror", (BYTE*)&fMetalMirror - pThis, sizeof(fMetalMirror), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "DielMirror", (BYTE*)&fDielMirror - pThis, sizeof(fDielMirror), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Mirror", (BYTE*)&pMirror - pThis, sizeof(pMirror), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "CastShadow", (BYTE*)&bCastShadow - pThis, sizeof(bCastShadow), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "ReceiveShadow", (BYTE*)&bReceiveShadow - pThis, sizeof(bReceiveShadow), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "Priority", (BYTE*)&nPriority - pThis, sizeof(nPriority), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructMetaInfo( "TranslucentColor", &vTranslucentColor, pThis ); 
	NMetaInfo::ReportMetaInfo( "FloatParam", (BYTE*)&fFloatParam - pThis, sizeof(fFloatParam), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "DetailTexture", (BYTE*)&pDetailTexture - pThis, sizeof(pDetailTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "DetailScale", (BYTE*)&fDetailScale - pThis, sizeof(fDetailScale), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ProjectOnTerrain", (BYTE*)&bProjectOnTerrain - pThis, sizeof(bProjectOnTerrain), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "LightingMode", (BYTE*)&eLightingMode - pThis, sizeof(eLightingMode), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "DynamicMode", (BYTE*)&eDynamicMode - pThis, sizeof(eDynamicMode), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "Is2Sided", (BYTE*)&bIs2Sided - pThis, sizeof(bIs2Sided), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "Effect", (BYTE*)&eEffect - pThis, sizeof(eEffect), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "AlphaMode", (BYTE*)&eAlphaMode - pThis, sizeof(eAlphaMode), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "AffectedByFog", (BYTE*)&bAffectedByFog - pThis, sizeof(bAffectedByFog), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "AddPlaced", (BYTE*)&bAddPlaced - pThis, sizeof(bAddPlaced), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "IgnoreZBuffer", (BYTE*)&bIgnoreZBuffer - pThis, sizeof(bIgnoreZBuffer), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "BackFaceCastShadow", (BYTE*)&bBackFaceCastShadow - pThis, sizeof(bBackFaceCastShadow), NTypeDef::TYPE_TYPE_BOOL );
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Material", (BYTE*)&pMaterial - pThis, sizeof(pMaterial), NTypeDef::TYPE_TYPE_REF );
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
REGISTER_DATABASE_CLASS( 0x12069B88, SModel ) 
REGISTER_DATABASE_CLASS( 0x12069B8E, STexture ) 
REGISTER_DATABASE_CLASS( 0x12069B82, SCubeTexture ) 
REGISTER_DATABASE_CLASS( 0xB4406170, SSunFlares ) 
REGISTER_DATABASE_CLASS( 0x12069B80, SAmbientLight ) 
REGISTER_DATABASE_CLASS( 0x1318BB40, SHeightFog ) 
REGISTER_DATABASE_CLASS( 0x13192480, SDepthOfField ) 
REGISTER_DATABASE_CLASS( 0x1319E340, SDistanceFog ) 
REGISTER_DATABASE_CLASS( 0x12069B8A, SSkeleton ) 
BASIC_REGISTER_DATABASE_CLASS( SAnimBase )
REGISTER_DATABASE_CLASS( 0x1206A301, SAnimLight ) 
REGISTER_DATABASE_CLASS( 0x12069B89, SParticle ) 
REGISTER_DATABASE_CLASS( 0x1206A2C1, SLightInstance ) 
REGISTER_DATABASE_CLASS( 0x1206A2C0, SParticleInstance ) 
REGISTER_DATABASE_CLASS( 0x5014B340, SModelInstance ) 
REGISTER_DATABASE_CLASS( 0x12069B83, SEffect ) 
REGISTER_DATABASE_CLASS( 0x131A73C0, SDecal ) 
REGISTER_DATABASE_CLASS( 0x12069B84, SFont ) 
REGISTER_DATABASE_CLASS( 0x1007EC80, SAIGeometry ) 
REGISTER_DATABASE_CLASS( 0x12069B85, SGeometry ) 
REGISTER_DATABASE_CLASS( 0x12069B87, SMaterial ) 
REGISTER_DATABASE_CLASS( 0x12069B8B, SSpot ) 

