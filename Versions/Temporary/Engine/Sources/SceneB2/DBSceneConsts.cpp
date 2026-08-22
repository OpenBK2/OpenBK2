// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "DBSceneConsts.h"

#include <cstdint>

#include "SceneB2_export.h"

namespace NDb
{



void SLightEffectConsts::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "LightConeTexture", (uint8_t*)&pLightConeTexture - pThis, sizeof(pLightConeTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "FlareTexture", (uint8_t*)&pFlareTexture - pThis, sizeof(pFlareTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "FlareAppearTime", (uint8_t*)&fFlareAppearTime - pThis, sizeof(fFlareAppearTime), NTypeDef::TYPE_TYPE_FLOAT );
}

int SLightEffectConsts::operator&( IXmlSaver &saver )
{
	saver.Add( "LightConeTexture", &pLightConeTexture );
	saver.Add( "FlareTexture", &pFlareTexture );
	saver.Add( "FlareAppearTime", &fFlareAppearTime );

	return 0;
}

int SLightEffectConsts::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pLightConeTexture );
	saver.Add( 3, &pFlareTexture );
	saver.Add( 4, &fFlareAppearTime );

	return 0;
}

uint32_t SLightEffectConsts::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fFlareAppearTime;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SSceneConsts::SSelectionMaterials::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Air", (uint8_t*)&pAir - pThis, sizeof(pAir), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Water", (uint8_t*)&pWater - pThis, sizeof(pWater), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Ground", (uint8_t*)&pGround - pThis, sizeof(pGround), NTypeDef::TYPE_TYPE_REF );
}

int SSceneConsts::SSelectionMaterials::operator&( IXmlSaver &saver )
{
	saver.Add( "Air", &pAir );
	saver.Add( "Water", &pWater );
	saver.Add( "Ground", &pGround );

	return 0;
}

int SSceneConsts::SSelectionMaterials::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pAir );
	saver.Add( 3, &pWater );
	saver.Add( 4, &pGround );

	return 0;
}

uint32_t SSceneConsts::SSelectionMaterials::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SSceneConsts::STrackMaterials::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Track", (uint8_t*)&pTrack - pThis, sizeof(pTrack), NTypeDef::TYPE_TYPE_REF );
}

int SSceneConsts::STrackMaterials::operator&( IXmlSaver &saver )
{
	saver.Add( "Track", &pTrack );

	return 0;
}

int SSceneConsts::STrackMaterials::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pTrack );

	return 0;
}

uint32_t SSceneConsts::STrackMaterials::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SSceneConsts::SIconAIGeometry::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Icon", (uint8_t*)&pIcon - pThis, sizeof(pIcon), NTypeDef::TYPE_TYPE_REF );
}

int SSceneConsts::SIconAIGeometry::operator&( IXmlSaver &saver )
{
	saver.Add( "Icon", &pIcon );

	return 0;
}

int SSceneConsts::SIconAIGeometry::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pIcon );

	return 0;
}

uint32_t SSceneConsts::SIconAIGeometry::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SSceneConsts::STerraGenConsts::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "BorderSmoothNoise", (uint8_t*)&szBorderSmoothNoise - pThis, sizeof(szBorderSmoothNoise), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "TextureCombiningNoise", (uint8_t*)&szTextureCombiningNoise - pThis, sizeof(szTextureCombiningNoise), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "DebrisMaskNoise", (uint8_t*)&szDebrisMaskNoise - pThis, sizeof(szDebrisMaskNoise), NTypeDef::TYPE_TYPE_STRING );
}

int SSceneConsts::STerraGenConsts::operator&( IXmlSaver &saver )
{
	saver.Add( "BorderSmoothNoise", &szBorderSmoothNoise );
	saver.Add( "TextureCombiningNoise", &szTextureCombiningNoise );
	saver.Add( "DebrisMaskNoise", &szDebrisMaskNoise );

	return 0;
}

int SSceneConsts::STerraGenConsts::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szBorderSmoothNoise );
	saver.Add( 3, &szTextureCombiningNoise );
	saver.Add( 4, &szDebrisMaskNoise );

	return 0;
}

uint32_t SSceneConsts::STerraGenConsts::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SSceneConsts::SDebugMaterials::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "RedMaterial", (uint8_t*)&pRedMaterial - pThis, sizeof(pRedMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "GreenMaterial", (uint8_t*)&pGreenMaterial - pThis, sizeof(pGreenMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "BlueMaterial", (uint8_t*)&pBlueMaterial - pThis, sizeof(pBlueMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "BlackMaterial", (uint8_t*)&pBlackMaterial - pThis, sizeof(pBlackMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "WhiteMaterial", (uint8_t*)&pWhiteMaterial - pThis, sizeof(pWhiteMaterial), NTypeDef::TYPE_TYPE_REF );
}

int SSceneConsts::SDebugMaterials::operator&( IXmlSaver &saver )
{
	saver.Add( "RedMaterial", &pRedMaterial );
	saver.Add( "GreenMaterial", &pGreenMaterial );
	saver.Add( "BlueMaterial", &pBlueMaterial );
	saver.Add( "BlackMaterial", &pBlackMaterial );
	saver.Add( "WhiteMaterial", &pWhiteMaterial );

	return 0;
}

int SSceneConsts::SDebugMaterials::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pRedMaterial );
	saver.Add( 3, &pGreenMaterial );
	saver.Add( 4, &pBlueMaterial );
	saver.Add( 5, &pBlackMaterial );
	saver.Add( 6, &pWhiteMaterial );

	return 0;
}

uint32_t SSceneConsts::SDebugMaterials::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SSceneConsts::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "SceneConsts", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportStructMetaInfo( "SelectionMaterials", &selectionMaterials, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "TrackMaterials", &trackMaterials, pThis ); 
	NMetaInfo::ReportMetaInfo( "ShotTraceMaterial", (uint8_t*)&pShotTraceMaterial - pThis, sizeof(pShotTraceMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "TerraGenConsts", &terraGenConsts, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "IconAIGeometry", &iconAIGeometry, pThis ); 
	NMetaInfo::ReportMetaInfo( "VisObjIconsSet", (uint8_t*)&pVisObjIconsSet - pThis, sizeof(pVisObjIconsSet), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "LightFX", &lightFX, pThis ); 
	NMetaInfo::ReportMetaInfo( "InterfaceLight", (uint8_t*)&pInterfaceLight - pThis, sizeof(pInterfaceLight), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "DebugMaterials", &debugMaterials, pThis ); 
	NMetaInfo::FinishMetaInfoReport();
}

int SSceneConsts::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "SelectionMaterials", &selectionMaterials );
	saver.Add( "TrackMaterials", &trackMaterials );
	saver.Add( "ShotTraceMaterial", &pShotTraceMaterial );
	saver.Add( "TerraGenConsts", &terraGenConsts );
	saver.Add( "IconAIGeometry", &iconAIGeometry );
	saver.Add( "VisObjIconsSet", &pVisObjIconsSet );
	saver.Add( "LightFX", &lightFX );
	saver.Add( "InterfaceLight", &pInterfaceLight );
	saver.Add( "DebugMaterials", &debugMaterials );

	return 0;
}

int SSceneConsts::operator&( IBinSaver &saver )
{
	saver.Add( 2, &selectionMaterials );
	saver.Add( 3, &trackMaterials );
	saver.Add( 4, &pShotTraceMaterial );
	saver.Add( 5, &terraGenConsts );
	saver.Add( 6, &iconAIGeometry );
	saver.Add( 7, &pVisObjIconsSet );
	saver.Add( 8, &lightFX );
	saver.Add( 9, &pInterfaceLight );
	saver.Add( 10, &debugMaterials );

	return 0;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( SCENEB2, 0x100AC381, SSceneConsts )

