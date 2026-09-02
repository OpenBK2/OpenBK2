// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "DBMinimap.h"

#include "ED_B2_M1_export.h"

#include <cstdint>

namespace NDb
{


std::string EnumToString( NDb::EMinimapLayerType eValue )
{
	switch ( eValue )
	{
	case NDb::LAYER_UNKNOWN:
		return "LAYER_UNKNOWN";
	case NDb::LAYER_BRIDGE:
		return "LAYER_BRIDGE";
	case NDb::LAYER_BUILDING:
		return "LAYER_BUILDING";
	case NDb::LAYER_RIVER:
		return "LAYER_RIVER";
	case NDb::LAYER_RAILOAD:
		return "LAYER_RAILOAD";
	case NDb::LAYER_ROAD:
		return "LAYER_ROAD";
	case NDb::LAYER_FLORA:
		return "LAYER_FLORA";
	case NDb::LAYER_GRAG:
		return "LAYER_GRAG";
	case NDb::LAYER_SWAMP:
		return "LAYER_SWAMP";
	case NDb::LAYER_LAKE:
		return "LAYER_LAKE";
	case NDb::LAYER_OCEAN:
		return "LAYER_OCEAN";
	case NDb::LAYER_TERRAIN:
		return "LAYER_TERRAIN";
	default:
		return "LAYER_UNKNOWN";
	}
}

NDb::EMinimapLayerType StringToEnum_NDb_EMinimapLayerType( const std::string &szValue )
{
	if ( szValue == "LAYER_UNKNOWN" )
		return NDb::LAYER_UNKNOWN;
	if ( szValue == "LAYER_BRIDGE" )
		return NDb::LAYER_BRIDGE;
	if ( szValue == "LAYER_BUILDING" )
		return NDb::LAYER_BUILDING;
	if ( szValue == "LAYER_RIVER" )
		return NDb::LAYER_RIVER;
	if ( szValue == "LAYER_RAILOAD" )
		return NDb::LAYER_RAILOAD;
	if ( szValue == "LAYER_ROAD" )
		return NDb::LAYER_ROAD;
	if ( szValue == "LAYER_FLORA" )
		return NDb::LAYER_FLORA;
	if ( szValue == "LAYER_GRAG" )
		return NDb::LAYER_GRAG;
	if ( szValue == "LAYER_SWAMP" )
		return NDb::LAYER_SWAMP;
	if ( szValue == "LAYER_LAKE" )
		return NDb::LAYER_LAKE;
	if ( szValue == "LAYER_OCEAN" )
		return NDb::LAYER_OCEAN;
	if ( szValue == "LAYER_TERRAIN" )
		return NDb::LAYER_TERRAIN;
	return NDb::LAYER_UNKNOWN;
}

std::string EnumToString( NDb::EImageScaleMethod eValue )
{
	switch ( eValue )
	{
	case NDb::IMAGE_SCALE_METHOD_DEFAULT:
		return "IMAGE_SCALE_METHOD_DEFAULT";
	case NDb::IMAGE_SCALE_METHOD_FILTER:
		return "IMAGE_SCALE_METHOD_FILTER";
	case NDb::IMAGE_SCALE_METHOD_BOX:
		return "IMAGE_SCALE_METHOD_BOX";
	case NDb::IMAGE_SCALE_METHOD_TRIANGLE:
		return "IMAGE_SCALE_METHOD_TRIANGLE";
	case NDb::IMAGE_SCALE_METHOD_BELL:
		return "IMAGE_SCALE_METHOD_BELL";
	case NDb::IMAGE_SCALE_METHOD_BSPLINE:
		return "IMAGE_SCALE_METHOD_BSPLINE";
	case NDb::IMAGE_SCALE_METHOD_LANCZOS3:
		return "IMAGE_SCALE_METHOD_LANCZOS3";
	case NDb::IMAGE_SCALE_METHOD_MITCHELL:
		return "IMAGE_SCALE_METHOD_MITCHELL";
	default:
		return "IMAGE_SCALE_METHOD_DEFAULT";
	}
}

NDb::EImageScaleMethod StringToEnum_NDb_EImageScaleMethod( const std::string &szValue )
{
	if ( szValue == "IMAGE_SCALE_METHOD_DEFAULT" )
		return NDb::IMAGE_SCALE_METHOD_DEFAULT;
	if ( szValue == "IMAGE_SCALE_METHOD_FILTER" )
		return NDb::IMAGE_SCALE_METHOD_FILTER;
	if ( szValue == "IMAGE_SCALE_METHOD_BOX" )
		return NDb::IMAGE_SCALE_METHOD_BOX;
	if ( szValue == "IMAGE_SCALE_METHOD_TRIANGLE" )
		return NDb::IMAGE_SCALE_METHOD_TRIANGLE;
	if ( szValue == "IMAGE_SCALE_METHOD_BELL" )
		return NDb::IMAGE_SCALE_METHOD_BELL;
	if ( szValue == "IMAGE_SCALE_METHOD_BSPLINE" )
		return NDb::IMAGE_SCALE_METHOD_BSPLINE;
	if ( szValue == "IMAGE_SCALE_METHOD_LANCZOS3" )
		return NDb::IMAGE_SCALE_METHOD_LANCZOS3;
	if ( szValue == "IMAGE_SCALE_METHOD_MITCHELL" )
		return NDb::IMAGE_SCALE_METHOD_MITCHELL;
	return NDb::IMAGE_SCALE_METHOD_DEFAULT;
}


void SShadowPoint::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "x", (uint8_t*)&nx - pThis, sizeof(nx), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "y", (uint8_t*)&ny - pThis, sizeof(ny), NTypeDef::TYPE_TYPE_INT );
}

int SShadowPoint::operator&( IXmlSaver &saver )
{
	saver.Add( "x", &nx );
	saver.Add( "y", &ny );

	return 0;
}

int SShadowPoint::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nx );
	saver.Add( 3, &ny );

	return 0;
}

uint32_t SShadowPoint::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nx << ny;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SEmbossPoint::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "x", (uint8_t*)&nx - pThis, sizeof(nx), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "y", (uint8_t*)&ny - pThis, sizeof(ny), NTypeDef::TYPE_TYPE_INT );
}

int SEmbossPoint::operator&( IXmlSaver &saver )
{
	saver.Add( "x", &nx );
	saver.Add( "y", &ny );

	return 0;
}

int SEmbossPoint::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nx );
	saver.Add( 3, &ny );

	return 0;
}

uint32_t SEmbossPoint::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nx << ny;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMinimapLayer::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Type", (uint8_t*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "EmbossFilterSize", (uint8_t*)&nEmbossFilterSize - pThis, sizeof(nEmbossFilterSize), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "ScaleNoise", (uint8_t*)&bScaleNoise - pThis, sizeof(bScaleNoise), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "Color", (uint8_t*)&nColor - pThis, sizeof(nColor), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "BorderColor", (uint8_t*)&nBorderColor - pThis, sizeof(nBorderColor), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "BorderWidth", (uint8_t*)&nBorderWidth - pThis, sizeof(nBorderWidth), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructMetaInfo( szAddName + "ShadowPoint", &shadowPoint, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "EmbossPoint", &embossPoint, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "NoiseImage", (uint8_t*)&szNoiseImage - pThis, sizeof(szNoiseImage), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "ScaleMethod", (uint8_t*)&eScaleMethod - pThis, sizeof(eScaleMethod), NTypeDef::TYPE_TYPE_ENUM );
}

int SMinimapLayer::operator&( IXmlSaver &saver )
{
	saver.Add( "Type", &eType );
	saver.Add( "EmbossFilterSize", &nEmbossFilterSize );
	saver.Add( "ScaleNoise", &bScaleNoise );
	saver.Add( "Color", &nColor );
	saver.Add( "BorderColor", &nBorderColor );
	saver.Add( "BorderWidth", &nBorderWidth );
	saver.Add( "ShadowPoint", &shadowPoint );
	saver.Add( "EmbossPoint", &embossPoint );
	saver.Add( "NoiseImage", &szNoiseImage );
	saver.Add( "ScaleMethod", &eScaleMethod );

	return 0;
}

int SMinimapLayer::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eType );
	saver.Add( 3, &nEmbossFilterSize );
	saver.Add( 4, &bScaleNoise );
	saver.Add( 5, &nColor );
	saver.Add( 6, &nBorderColor );
	saver.Add( 7, &nBorderWidth );
	saver.Add( 8, &shadowPoint );
	saver.Add( 9, &embossPoint );
	saver.Add( 10, &szNoiseImage );
	saver.Add( 11, &eScaleMethod );

	return 0;
}

uint32_t SMinimapLayer::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eType << nEmbossFilterSize << bScaleNoise << nColor << nBorderColor << nBorderWidth << shadowPoint << embossPoint << eScaleMethod;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMinimap::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Minimap", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "WoodRadius", (uint8_t*)&nWoodRadius - pThis, sizeof(nWoodRadius), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "TerrainShadeRatio", (uint8_t*)&nTerrainShadeRatio - pThis, sizeof(nTerrainShadeRatio), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "ShowAllBuildingsPassability", (uint8_t*)&bShowAllBuildingsPassability - pThis, sizeof(bShowAllBuildingsPassability), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "ShowTerrainShades", (uint8_t*)&bShowTerrainShades - pThis, sizeof(bShowTerrainShades), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "MinAlpha", (uint8_t*)&nMinAlpha - pThis, sizeof(nMinAlpha), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructArrayMetaInfo( "Layers", &layers, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SMinimap::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "WoodRadius", &nWoodRadius );
	saver.Add( "TerrainShadeRatio", &nTerrainShadeRatio );
	saver.Add( "ShowAllBuildingsPassability", &bShowAllBuildingsPassability );
	saver.Add( "ShowTerrainShades", &bShowTerrainShades );
	saver.Add( "MinAlpha", &nMinAlpha );
	saver.Add( "Layers", &layers );

	return 0;
}

int SMinimap::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nWoodRadius );
	saver.Add( 3, &nTerrainShadeRatio );
	saver.Add( 4, &bShowAllBuildingsPassability );
	saver.Add( 5, &bShowTerrainShades );
	saver.Add( 6, &nMinAlpha );
	saver.Add( 7, &layers );

	return 0;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( ED_B2_M1, 0x1414DB40, SMinimap ) 

