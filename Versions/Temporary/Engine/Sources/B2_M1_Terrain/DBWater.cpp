// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "DBWater.h"

#include <cstdint>

namespace NDb
{



void SAnimatedTexture::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Material", (uint8_t*)&pMaterial - pThis, sizeof(pMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "NumFramesX", (uint8_t*)&nNumFramesX - pThis, sizeof(nNumFramesX), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "NumFramesY", (uint8_t*)&nNumFramesY - pThis, sizeof(nNumFramesY), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "UseFrames", (uint8_t*)&nUseFrames - pThis, sizeof(nUseFrames), NTypeDef::TYPE_TYPE_INT );
}

int SAnimatedTexture::operator&( IXmlSaver &saver )
{
	saver.Add( "Material", &pMaterial );
	saver.Add( "NumFramesX", &nNumFramesX );
	saver.Add( "NumFramesY", &nNumFramesY );
	saver.Add( "UseFrames", &nUseFrames );

	return 0;
}

int SAnimatedTexture::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pMaterial );
	saver.Add( 3, &nNumFramesX );
	saver.Add( 4, &nNumFramesY );
	saver.Add( 5, &nUseFrames );

	return 0;
}

uint32_t SAnimatedTexture::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nNumFramesX << nNumFramesY << nUseFrames;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SWaterSet::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WaterSet", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportStructMetaInfo( "Water", &water, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "WhiteHorses", &whiteHorses, pThis ); 
	NMetaInfo::ReportMetaInfo( "Surf", (uint8_t*)&pSurf - pThis, sizeof(pSurf), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SWaterSet::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Water", &water );
	saver.Add( "WhiteHorses", &whiteHorses );
	saver.Add( "Surf", &pSurf );

	return 0;
}

int SWaterSet::operator&( IBinSaver &saver )
{
	saver.Add( 2, &water );
	saver.Add( 3, &whiteHorses );
	saver.Add( 4, &pSurf );

	return 0;
}


std::string EnumToString( NDb::SWater::EWaterType eValue )
{
	switch ( eValue )
	{
	case NDb::SWater::WT_OCEAN:
		return "WT_OCEAN";
	case NDb::SWater::WT_LAKE:
		return "WT_LAKE";
	case NDb::SWater::WT_RIVER:
		return "WT_RIVER";
	case NDb::SWater::WT_SWAMP:
		return "WT_SWAMP";
	case NDb::SWater::WT_OTHER:
		return "WT_OTHER";
	default:
		return "WT_OCEAN";
	}
}

NDb::SWater::EWaterType NDb::StringToEnum_NDb_SWater_EWaterType( const std::string &szValue )
{
	if ( szValue == "WT_OCEAN" )
		return NDb::SWater::WT_OCEAN;
	if ( szValue == "WT_LAKE" )
		return NDb::SWater::WT_LAKE;
	if ( szValue == "WT_RIVER" )
		return NDb::SWater::WT_RIVER;
	if ( szValue == "WT_SWAMP" )
		return NDb::SWater::WT_SWAMP;
	if ( szValue == "WT_OTHER" )
		return NDb::SWater::WT_OTHER;
	return NDb::SWater::WT_OCEAN;
}


void SWater::SWaterWaveType::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Amplitude", (uint8_t*)&fAmplitude - pThis, sizeof(fAmplitude), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Period", (uint8_t*)&fPeriod - pThis, sizeof(fPeriod), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "PeriodVariation", (uint8_t*)&fPeriodVariation - pThis, sizeof(fPeriodVariation), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "InvPeriod", (uint8_t*)&fInvPeriod - pThis, sizeof(fInvPeriod), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "DeepWaveNumber", (uint8_t*)&fDeepWaveNumber - pThis, sizeof(fDeepWaveNumber), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "PhaseOffset", (uint8_t*)&fPhaseOffset - pThis, sizeof(fPhaseOffset), NTypeDef::TYPE_TYPE_FLOAT );
}

int SWater::SWaterWaveType::operator&( IXmlSaver &saver )
{
	saver.Add( "Amplitude", &fAmplitude );
	saver.Add( "Period", &fPeriod );
	saver.Add( "PeriodVariation", &fPeriodVariation );
	saver.Add( "InvPeriod", &fInvPeriod );
	saver.Add( "DeepWaveNumber", &fDeepWaveNumber );
	saver.Add( "PhaseOffset", &fPhaseOffset );

	return 0;
}

int SWater::SWaterWaveType::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fAmplitude );
	saver.Add( 3, &fPeriod );
	saver.Add( 4, &fPeriodVariation );
	saver.Add( 5, &fInvPeriod );
	saver.Add( 6, &fDeepWaveNumber );
	saver.Add( 7, &fPhaseOffset );

	return 0;
}

uint32_t SWater::SWaterWaveType::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fAmplitude << fPeriod << fPeriodVariation << fInvPeriod << fDeepWaveNumber << fPhaseOffset;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SWater::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Water", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "Waves", &waves, pThis );
	NMetaInfo::ReportMetaInfo( "WaterSet", (uint8_t*)&pWaterSet - pThis, sizeof(pWaterSet), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Light", (uint8_t*)&pLight - pThis, sizeof(pLight), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "DepthNoise", (uint8_t*)&pDepthNoise - pThis, sizeof(pDepthNoise), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "DepthNoiseCoeff", (uint8_t*)&fDepthNoiseCoeff - pThis, sizeof(fDepthNoiseCoeff), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "TilesNumPerWaterTexture", (uint8_t*)&nTilesNumPerWaterTexture - pThis, sizeof(nTilesNumPerWaterTexture), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "UseWaves", (uint8_t*)&bUseWaves - pThis, sizeof(bUseWaves), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "HorDeformMinRadius", (uint8_t*)&fHorDeformMinRadius - pThis, sizeof(fHorDeformMinRadius), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "HorDeformMaxRadius", (uint8_t*)&fHorDeformMaxRadius - pThis, sizeof(fHorDeformMaxRadius), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "HorDeformRadiusSpeed", (uint8_t*)&fHorDeformRadiusSpeed - pThis, sizeof(fHorDeformRadiusSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "HorDeformRotationSpeedMin", (uint8_t*)&fHorDeformRotationSpeedMin - pThis, sizeof(fHorDeformRotationSpeedMin), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "HorDeformRotationSpeedVariation", (uint8_t*)&fHorDeformRotationSpeedVariation - pThis, sizeof(fHorDeformRotationSpeedVariation), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "WaterType", (uint8_t*)&eWaterType - pThis, sizeof(eWaterType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::FinishMetaInfoReport();
}

int SWater::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Waves", &waves );
	saver.Add( "WaterSet", &pWaterSet );
	saver.Add( "Light", &pLight );
	saver.Add( "DepthNoise", &pDepthNoise );
	saver.Add( "DepthNoiseCoeff", &fDepthNoiseCoeff );
	saver.Add( "TilesNumPerWaterTexture", &nTilesNumPerWaterTexture );
	saver.Add( "UseWaves", &bUseWaves );
	saver.Add( "HorDeformMinRadius", &fHorDeformMinRadius );
	saver.Add( "HorDeformMaxRadius", &fHorDeformMaxRadius );
	saver.Add( "HorDeformRadiusSpeed", &fHorDeformRadiusSpeed );
	saver.Add( "HorDeformRotationSpeedMin", &fHorDeformRotationSpeedMin );
	saver.Add( "HorDeformRotationSpeedVariation", &fHorDeformRotationSpeedVariation );
	saver.Add( "WaterType", &eWaterType );

	return 0;
}

int SWater::operator&( IBinSaver &saver )
{
	saver.Add( 2, &waves );
	saver.Add( 3, &pWaterSet );
	saver.Add( 4, &pLight );
	saver.Add( 5, &pDepthNoise );
	saver.Add( 6, &fDepthNoiseCoeff );
	saver.Add( 7, &nTilesNumPerWaterTexture );
	saver.Add( 8, &bUseWaves );
	saver.Add( 9, &fHorDeformMinRadius );
	saver.Add( 10, &fHorDeformMaxRadius );
	saver.Add( 11, &fHorDeformRadiusSpeed );
	saver.Add( 12, &fHorDeformRotationSpeedMin );
	saver.Add( 13, &fHorDeformRotationSpeedVariation );
	saver.Add( 14, &eWaterType );

	return 0;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( 0x10084340, SWaterSet ) 
REGISTER_DATABASE_CLASS( 0x10084341, SWater ) 

