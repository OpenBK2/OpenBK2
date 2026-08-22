// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "dbterrain.h"

#include "System/UuidChunk.h"

#include "B2_M1_Terrain_export.h"

#include <cstdint>

namespace NDb
{



void STGNoise::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "TGNoise", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "FileName", (uint8_t*)&szFileName - pThis, sizeof(szFileName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::FinishMetaInfoReport();
}

int STGNoise::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "FileName", &szFileName );

	return 0;
}

int STGNoise::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szFileName );

	return 0;
}



void STGTerraType::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "TGTerraType", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "Material", (uint8_t*)&pMaterial - pThis, sizeof(pMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "AIProperty", &aIProperty, pThis ); 
	NMetaInfo::ReportMetaInfo( "Color", (uint8_t*)&nColor - pThis, sizeof(nColor), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "PeakMaterial", (uint8_t*)&pPeakMaterial - pThis, sizeof(pPeakMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "ScaleCoeff", (uint8_t*)&fScaleCoeff - pThis, sizeof(fScaleCoeff), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Sound", (uint8_t*)&pSound - pThis, sizeof(pSound), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "CycledSound", (uint8_t*)&pCycledSound - pThis, sizeof(pCycledSound), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int STGTerraType::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Material", &pMaterial );
	saver.Add( "AIProperty", &aIProperty );
	saver.Add( "Color", &nColor );
	saver.Add( "PeakMaterial", &pPeakMaterial );
	saver.Add( "ScaleCoeff", &fScaleCoeff );
	saver.Add( "Sound", &pSound );
	saver.Add( "CycledSound", &pCycledSound );

	return 0;
}

int STGTerraType::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pMaterial );
	saver.Add( 3, &aIProperty );
	saver.Add( 4, &nColor );
	saver.Add( 5, &pPeakMaterial );
	saver.Add( 6, &fScaleCoeff );
	saver.Add( 7, &pSound );
	saver.Add( 8, &pCycledSound );

	return 0;
}

uint32_t STGTerraType::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << aIProperty << nColor << fScaleCoeff;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void STGTerraSet::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "TGTerraSet", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportSimpleArrayMetaInfo( "TerraTypes", &terraTypes, pThis );
	NMetaInfo::ReportMetaInfo( "WrapTexture", (uint8_t*)&bWrapTexture - pThis, sizeof(bWrapTexture), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::FinishMetaInfoReport();
}

int STGTerraSet::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "TerraTypes", &terraTypes );
	saver.Add( "WrapTexture", &bWrapTexture );

	return 0;
}

int STGTerraSet::operator&( IBinSaver &saver )
{
	saver.Add( 2, &terraTypes );
	saver.Add( 3, &bWrapTexture );

	return 0;
}

uint32_t STGTerraSet::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << terraTypes << bWrapTexture;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}


std::string EnumToString( NDb::EWeatherType eValue )
{
	switch ( eValue )
	{
	case NDb::WEATHER_RAIN:
		return "WEATHER_RAIN";
	case NDb::WEATHER_SNOW:
		return "WEATHER_SNOW";
	case NDb::WEATHER_SANDSTORM:
		return "WEATHER_SANDSTORM";
	default:
		return "WEATHER_RAIN";
	}
}

NDb::EWeatherType NDb::StringToEnum_NDb_EWeatherType( const std::string &szValue )
{
	if ( szValue == "WEATHER_RAIN" )
		return NDb::WEATHER_RAIN;
	if ( szValue == "WEATHER_SNOW" )
		return NDb::WEATHER_SNOW;
	if ( szValue == "WEATHER_SANDSTORM" )
		return NDb::WEATHER_SANDSTORM;
	return NDb::WEATHER_RAIN;
}


void SWeather::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "WindDirection", (uint8_t*)&nWindDirection - pThis, sizeof(nWindDirection), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "WindForce", (uint8_t*)&nWindForce - pThis, sizeof(nWindForce), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "WeatherPeriod", (uint8_t*)&fWeatherPeriod - pThis, sizeof(fWeatherPeriod), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "WeatherPeriodRandom", (uint8_t*)&fWeatherPeriodRandom - pThis, sizeof(fWeatherPeriodRandom), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Visuals", (uint8_t*)&pVisuals - pThis, sizeof(pVisuals), NTypeDef::TYPE_TYPE_REF );
}

int SWeather::operator&( IXmlSaver &saver )
{
	saver.Add( "WindDirection", &nWindDirection );
	saver.Add( "WindForce", &nWindForce );
	saver.Add( "WeatherPeriod", &fWeatherPeriod );
	saver.Add( "WeatherPeriodRandom", &fWeatherPeriodRandom );
	saver.Add( "Visuals", &pVisuals );

	return 0;
}

int SWeather::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nWindDirection );
	saver.Add( 3, &nWindForce );
	saver.Add( 4, &fWeatherPeriod );
	saver.Add( 5, &fWeatherPeriodRandom );
	saver.Add( 6, &pVisuals );

	return 0;
}

uint32_t SWeather::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nWindDirection << nWindForce << fWeatherPeriod << fWeatherPeriodRandom << pVisuals;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SWeatherDesc::SAmbientSoundDescr::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "AmbientSound", (uint8_t*)&pAmbientSound - pThis, sizeof(pAmbientSound), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "SoundLength", (uint8_t*)&fSoundLength - pThis, sizeof(fSoundLength), NTypeDef::TYPE_TYPE_FLOAT );
}

int SWeatherDesc::SAmbientSoundDescr::operator&( IXmlSaver &saver )
{
	saver.Add( "AmbientSound", &pAmbientSound );
	saver.Add( "SoundLength", &fSoundLength );

	return 0;
}

int SWeatherDesc::SAmbientSoundDescr::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pAmbientSound );
	saver.Add( 3, &fSoundLength );

	return 0;
}

uint32_t SWeatherDesc::SAmbientSoundDescr::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fSoundLength;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SWeatherDesc::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WeatherDesc", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "Type", (uint8_t*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "PartMaterial", (uint8_t*)&pPartMaterial - pThis, sizeof(pPartMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportSimpleArrayMetaInfo( "PartMaterials", &partMaterials, pThis );
	NMetaInfo::ReportMetaInfo( "PartSize", (uint8_t*)&fPartSize - pThis, sizeof(fPartSize), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "FallHeight", (uint8_t*)&fFallHeight - pThis, sizeof(fFallHeight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Speed", (uint8_t*)&fSpeed - pThis, sizeof(fSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "TrajectoryParameter", (uint8_t*)&fTrajectoryParameter - pThis, sizeof(fTrajectoryParameter), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Intensity", (uint8_t*)&nIntensity - pThis, sizeof(nIntensity), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "WindAffected", (uint8_t*)&bWindAffected - pThis, sizeof(bWindAffected), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "WeatherLight", (uint8_t*)&pWeatherLight - pThis, sizeof(pWeatherLight), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportSimpleArrayMetaInfo( "Lightnings", &lightnings, pThis );
	NMetaInfo::ReportMetaInfo( "LightningsPerMinute", (uint8_t*)&fLightningsPerMinute - pThis, sizeof(fLightningsPerMinute), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "LightningsRandom", (uint8_t*)&fLightningsRandom - pThis, sizeof(fLightningsRandom), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Water", (uint8_t*)&pWater - pThis, sizeof(pWater), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructArrayMetaInfo( "AmbientSound", &ambientSound, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "RandomSounds", &randomSounds, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SWeatherDesc::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Type", &eType );
	saver.Add( "PartMaterial", &pPartMaterial );
	saver.Add( "PartMaterials", &partMaterials );
	saver.Add( "PartSize", &fPartSize );
	saver.Add( "FallHeight", &fFallHeight );
	saver.Add( "Speed", &fSpeed );
	saver.Add( "TrajectoryParameter", &fTrajectoryParameter );
	saver.Add( "Intensity", &nIntensity );
	saver.Add( "WindAffected", &bWindAffected );
	saver.Add( "WeatherLight", &pWeatherLight );
	saver.Add( "Lightnings", &lightnings );
	saver.Add( "LightningsPerMinute", &fLightningsPerMinute );
	saver.Add( "LightningsRandom", &fLightningsRandom );
	saver.Add( "Water", &pWater );
	saver.Add( "AmbientSound", &ambientSound );
	saver.Add( "RandomSounds", &randomSounds );

	return 0;
}

int SWeatherDesc::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eType );
	saver.Add( 3, &pPartMaterial );
	saver.Add( 4, &partMaterials );
	saver.Add( 5, &fPartSize );
	saver.Add( 6, &fFallHeight );
	saver.Add( 7, &fSpeed );
	saver.Add( 8, &fTrajectoryParameter );
	saver.Add( 9, &nIntensity );
	saver.Add( 10, &bWindAffected );
	saver.Add( 11, &pWeatherLight );
	saver.Add( 12, &lightnings );
	saver.Add( 13, &fLightningsPerMinute );
	saver.Add( 14, &fLightningsRandom );
	saver.Add( 15, &pWater );
	saver.Add( 16, &ambientSound );
	saver.Add( 17, &randomSounds );

	return 0;
}

uint32_t SWeatherDesc::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eType << partMaterials << fPartSize << fFallHeight << fSpeed << fTrajectoryParameter << nIntensity << bWindAffected << lightnings << fLightningsPerMinute << fLightningsRandom << ambientSound << randomSounds;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void STerrain::ReportMetaInfo() const
{
	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "NumPatchesX", (uint8_t*)&nNumPatchesX - pThis, sizeof(nNumPatchesX), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "NumPatchesY", (uint8_t*)&nNumPatchesY - pThis, sizeof(nNumPatchesY), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "TerraSet", (uint8_t*)&pTerraSet - pThis, sizeof(pTerraSet), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "MapFilesPath", (uint8_t*)&szMapFilesPath - pThis, sizeof(szMapFilesPath), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "Light", (uint8_t*)&pLight - pThis, sizeof(pLight), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "PreLight", (uint8_t*)&pPreLight - pThis, sizeof(pPreLight), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "Weather", &weather, pThis ); 
	NMetaInfo::ReportMetaInfo( "OceanWater", (uint8_t*)&pOceanWater - pThis, sizeof(pOceanWater), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructArrayMetaInfo( "Roads", &roads, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "Rivers", &rivers, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "Crags", &crags, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "Spots", &spots, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "Lakes", &lakes, pThis );
	NMetaInfo::ReportMetaInfo( "HasCoast", (uint8_t*)&bHasCoast - pThis, sizeof(bHasCoast), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportStructMetaInfo( "Coast", &coast, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "CoastMidPoint", &vCoastMidPoint, pThis ); 
	NMetaInfo::ReportMetaInfo( "uid", (uint8_t*)&uid - pThis, sizeof(uid), NTypeDef::TYPE_TYPE_GUID );
}

int STerrain::operator&( IXmlSaver &saver )
{
	saver.Add( "NumPatchesX", &nNumPatchesX );
	saver.Add( "NumPatchesY", &nNumPatchesY );
	saver.Add( "TerraSet", &pTerraSet );
	saver.Add( "MapFilesPath", &szMapFilesPath );
	saver.Add( "Light", &pLight );
	saver.Add( "PreLight", &pPreLight );
	saver.Add( "Weather", &weather );
	saver.Add( "OceanWater", &pOceanWater );
	saver.Add( "Roads", &roads );
	saver.Add( "Rivers", &rivers );
	saver.Add( "Crags", &crags );
	saver.Add( "Spots", &spots );
	saver.Add( "Lakes", &lakes );
	saver.Add( "HasCoast", &bHasCoast );
	saver.Add( "Coast", &coast );
	saver.Add( "CoastMidPoint", &vCoastMidPoint );
	saver.Add( "uid", &uid );

	return 0;
}

int STerrain::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nNumPatchesX );
	saver.Add( 3, &nNumPatchesY );
	saver.Add( 4, &pTerraSet );
	saver.Add( 5, &szMapFilesPath );
	saver.Add( 6, &pLight );
	saver.Add( 7, &pPreLight );
	saver.Add( 8, &weather );
	saver.Add( 9, &pOceanWater );
	saver.Add( 10, &roads );
	saver.Add( 11, &rivers );
	saver.Add( 12, &crags );
	saver.Add( 13, &spots );
	saver.Add( 14, &lakes );
	saver.Add( 15, &bHasCoast );
	saver.Add( 16, &coast );
	saver.Add( 17, &vCoastMidPoint );
	AddUuidChunk( saver, 18, &uid );

	return 0;
}

uint32_t STerrain::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nNumPatchesX << nNumPatchesY << pTerraSet << szMapFilesPath << weather << roads << rivers << crags << spots << lakes << bHasCoast << coast << vCoastMidPoint << uid;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( B2_M1_TERRAIN, 0x10081500, STGNoise )
REGISTER_DATABASE_CLASS( B2_M1_TERRAIN, 0x13121B41, STGTerraType )
REGISTER_DATABASE_CLASS( B2_M1_TERRAIN, 0x13121B01, STGTerraSet )
REGISTER_DATABASE_CLASS( B2_M1_TERRAIN, 0x1918BBC0, SWeatherDesc )
BASIC_REGISTER_DATABASE_CLASS( B2_M1_TERRAIN, STerrain )

