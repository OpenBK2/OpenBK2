// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "DBVSO.h"

#include "B2_M1_Terrain_export.h"

#include <cstdint>

namespace NDb
{



void STerrainAIProperties::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Passability", (uint8_t*)&fPassability - pThis, sizeof(fPassability), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "AIClass", (uint8_t*)&nAIClass - pThis, sizeof(nAIClass), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "AIPassabilityClass", (uint8_t*)&nAIPassabilityClass - pThis, sizeof(nAIPassabilityClass), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "CanEntrench", (uint8_t*)&bCanEntrench - pThis, sizeof(bCanEntrench), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "SoilType", (uint8_t*)&nSoilType - pThis, sizeof(nSoilType), NTypeDef::TYPE_TYPE_INT );
}

int STerrainAIProperties::operator&( IXmlSaver &saver )
{
	saver.Add( "Passability", &fPassability );
	saver.Add( "AIClass", &nAIClass );
	saver.Add( "AIPassabilityClass", &nAIPassabilityClass );
	saver.Add( "CanEntrench", &bCanEntrench );
	saver.Add( "SoilType", &nSoilType );

	return 0;
}

int STerrainAIProperties::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fPassability );
	saver.Add( 3, &nAIClass );
	saver.Add( 4, &nAIPassabilityClass );
	saver.Add( 5, &bCanEntrench );
	saver.Add( 6, &nSoilType );

	return 0;
}

uint32_t STerrainAIProperties::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fPassability << nAIClass << nAIPassabilityClass << bCanEntrench << nSoilType;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SVSOLayerBaseDesc::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "CenterOpacity", (uint8_t*)&fCenterOpacity - pThis, sizeof(fCenterOpacity), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "TilingStep", (uint8_t*)&fTilingStep - pThis, sizeof(fTilingStep), NTypeDef::TYPE_TYPE_FLOAT );
}

int SVSOLayerBaseDesc::operator&( IXmlSaver &saver )
{
	saver.Add( "CenterOpacity", &fCenterOpacity );
	saver.Add( "TilingStep", &fTilingStep );

	return 0;
}

int SVSOLayerBaseDesc::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fCenterOpacity );
	saver.Add( 3, &fTilingStep );

	return 0;
}

uint32_t SVSOLayerBaseDesc::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fCenterOpacity << fTilingStep;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SVSOLayerBorderDesc::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	SVSOLayerBaseDesc::ReportMetaInfo( szAddName, pThis );

	NMetaInfo::ReportMetaInfo( szAddName + "Material", (uint8_t*)&pMaterial - pThis, sizeof(pMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "UseFromPixel", (uint8_t*)&nUseFromPixel - pThis, sizeof(nUseFromPixel), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "UseToPixel", (uint8_t*)&nUseToPixel - pThis, sizeof(nUseToPixel), NTypeDef::TYPE_TYPE_INT );
}

int SVSOLayerBorderDesc::operator&( IXmlSaver &saver )
{
	saver.AddTypedSuper( (SVSOLayerBaseDesc*)(this) );
	saver.Add( "Material", &pMaterial );
	saver.Add( "UseFromPixel", &nUseFromPixel );
	saver.Add( "UseToPixel", &nUseToPixel );

	return 0;
}

int SVSOLayerBorderDesc::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SVSOLayerBaseDesc*)this );
	saver.Add( 2, &pMaterial );
	saver.Add( 3, &nUseFromPixel );
	saver.Add( 4, &nUseToPixel );

	return 0;
}

uint32_t SVSOLayerBorderDesc::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SVSOLayerBaseDesc::CalcCheckSum() << nUseFromPixel << nUseToPixel;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SVSOLayerCenterDesc::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	SVSOLayerBaseDesc::ReportMetaInfo( szAddName, pThis );

	NMetaInfo::ReportMetaInfo( szAddName + "Disturbance", (uint8_t*)&fDisturbance - pThis, sizeof(fDisturbance), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "NumCells", (uint8_t*)&nNumCells - pThis, sizeof(nNumCells), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "Materials", &materials, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "UseFromPixel", (uint8_t*)&nUseFromPixel - pThis, sizeof(nUseFromPixel), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "UseToPixel", (uint8_t*)&nUseToPixel - pThis, sizeof(nUseToPixel), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "StreamSpeed", (uint8_t*)&fStreamSpeed - pThis, sizeof(fStreamSpeed), NTypeDef::TYPE_TYPE_FLOAT );
}

int SVSOLayerCenterDesc::operator&( IXmlSaver &saver )
{
	saver.AddTypedSuper( (SVSOLayerBaseDesc*)(this) );
	saver.Add( "Disturbance", &fDisturbance );
	saver.Add( "NumCells", &nNumCells );
	saver.Add( "Materials", &materials );
	saver.Add( "UseFromPixel", &nUseFromPixel );
	saver.Add( "UseToPixel", &nUseToPixel );
	saver.Add( "StreamSpeed", &fStreamSpeed );

	return 0;
}

int SVSOLayerCenterDesc::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SVSOLayerBaseDesc*)this );
	saver.Add( 2, &fDisturbance );
	saver.Add( 3, &nNumCells );
	saver.Add( 4, &materials );
	saver.Add( 5, &nUseFromPixel );
	saver.Add( 6, &nUseToPixel );
	saver.Add( 7, &fStreamSpeed );

	return 0;
}

uint32_t SVSOLayerCenterDesc::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SVSOLayerBaseDesc::CalcCheckSum() << fDisturbance << nNumCells << materials << nUseFromPixel << nUseToPixel << fStreamSpeed;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SVSODesc::ReportMetaInfo() const
{
	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "Type", (uint8_t*)&nType - pThis, sizeof(nType), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "Priority", (uint8_t*)&nPriority - pThis, sizeof(nPriority), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructMetaInfo( "AIProperty", &aIProperty, pThis ); 
	NMetaInfo::ReportMetaInfo( "MiniMapCenterColor", (uint8_t*)&nMiniMapCenterColor - pThis, sizeof(nMiniMapCenterColor), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "MiniMapBorderColor", (uint8_t*)&nMiniMapBorderColor - pThis, sizeof(nMiniMapBorderColor), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "MiniMapCenterWidth", (uint8_t*)&nMiniMapCenterWidth - pThis, sizeof(nMiniMapCenterWidth), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "AmbientSound", (uint8_t*)&pAmbientSound - pThis, sizeof(pAmbientSound), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "CycledSound", (uint8_t*)&pCycledSound - pThis, sizeof(pCycledSound), NTypeDef::TYPE_TYPE_REF );
}

int SVSODesc::operator&( IXmlSaver &saver )
{
	saver.Add( "Type", &nType );
	saver.Add( "Priority", &nPriority );
	saver.Add( "AIProperty", &aIProperty );
	saver.Add( "MiniMapCenterColor", &nMiniMapCenterColor );
	saver.Add( "MiniMapBorderColor", &nMiniMapBorderColor );
	saver.Add( "MiniMapCenterWidth", &nMiniMapCenterWidth );
	saver.Add( "AmbientSound", &pAmbientSound );
	saver.Add( "CycledSound", &pCycledSound );

	return 0;
}

int SVSODesc::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nType );
	saver.Add( 3, &nPriority );
	saver.Add( 4, &aIProperty );
	saver.Add( 5, &nMiniMapCenterColor );
	saver.Add( 6, &nMiniMapBorderColor );
	saver.Add( 7, &nMiniMapCenterWidth );
	saver.Add( 8, &pAmbientSound );
	saver.Add( 9, &pCycledSound );

	return 0;
}

uint32_t SVSODesc::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nType << nPriority << aIProperty << nMiniMapCenterColor << nMiniMapBorderColor << nMiniMapCenterWidth;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SVSOPoint::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Pos", &vPos, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "Norm", &vNorm, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "Width", (uint8_t*)&fWidth - pThis, sizeof(fWidth), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Opacity", (uint8_t*)&fOpacity - pThis, sizeof(fOpacity), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "KeyPoint", (uint8_t*)&bKeyPoint - pThis, sizeof(bKeyPoint), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "Radius", (uint8_t*)&fRadius - pThis, sizeof(fRadius), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Reserved", (uint8_t*)&fReserved - pThis, sizeof(fReserved), NTypeDef::TYPE_TYPE_FLOAT );
}

int SVSOPoint::operator&( IXmlSaver &saver )
{
	saver.Add( "Pos", &vPos );
	saver.Add( "Norm", &vNorm );
	saver.Add( "Width", &fWidth );
	saver.Add( "Opacity", &fOpacity );
	saver.Add( "KeyPoint", &bKeyPoint );
	saver.Add( "Radius", &fRadius );
	saver.Add( "Reserved", &fReserved );

	return 0;
}

int SVSOPoint::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vPos );
	saver.Add( 3, &vNorm );
	saver.Add( 4, &fWidth );
	saver.Add( 5, &fOpacity );
	saver.Add( 6, &bKeyPoint );
	saver.Add( 7, &fRadius );
	saver.Add( 8, &fReserved );

	return 0;
}

uint32_t SVSOPoint::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << vPos << vNorm << fWidth << fOpacity << bKeyPoint << fRadius << fReserved;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SVSOInstance::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Descriptor", (uint8_t*)&pDescriptor - pThis, sizeof(pDescriptor), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "points", &points, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "ControlPoints", &controlPoints, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "VSOID", (uint8_t*)&nVSOID - pThis, sizeof(nVSOID), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "CMArrowType", (uint8_t*)&nCMArrowType - pThis, sizeof(nCMArrowType), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "CMArrowMission", (uint8_t*)&nCMArrowMission - pThis, sizeof(nCMArrowMission), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "CMArrowMission2", (uint8_t*)&nCMArrowMission2 - pThis, sizeof(nCMArrowMission2), NTypeDef::TYPE_TYPE_INT );
}

int SVSOInstance::operator&( IXmlSaver &saver )
{
	saver.Add( "Descriptor", &pDescriptor );
	saver.Add( "points", &points );
	saver.Add( "ControlPoints", &controlPoints );
	saver.Add( "VSOID", &nVSOID );
	saver.Add( "CMArrowType", &nCMArrowType );
	saver.Add( "CMArrowMission", &nCMArrowMission );
	saver.Add( "CMArrowMission2", &nCMArrowMission2 );

	return 0;
}

int SVSOInstance::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pDescriptor );
	saver.Add( 3, &points );
	saver.Add( 4, &controlPoints );
	saver.Add( 5, &nVSOID );
	saver.Add( 6, &nCMArrowType );
	saver.Add( 7, &nCMArrowMission );
	saver.Add( 8, &nCMArrowMission2 );

	return 0;
}

uint32_t SVSOInstance::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << pDescriptor << points << controlPoints << nVSOID << nCMArrowType << nCMArrowMission << nCMArrowMission2;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SRoadDesc::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "RoadDesc", typeID, sizeof(*this) );
	SVSODesc::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportStructMetaInfo( "LeftBorder", &leftBorder, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "RightBorder", &rightBorder, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "Center", &center, pThis ); 
	NMetaInfo::ReportMetaInfo( "DefaultOpacity", (uint8_t*)&fDefaultOpacity - pThis, sizeof(fDefaultOpacity), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SRoadDesc::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SVSODesc*)(this) );
	saver.Add( "LeftBorder", &leftBorder );
	saver.Add( "RightBorder", &rightBorder );
	saver.Add( "Center", &center );
	saver.Add( "DefaultOpacity", &fDefaultOpacity );

	return 0;
}

int SRoadDesc::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SVSODesc*)this );
	saver.Add( 2, &leftBorder );
	saver.Add( 3, &rightBorder );
	saver.Add( 4, &center );
	saver.Add( 5, &fDefaultOpacity );

	return 0;
}

uint32_t SRoadDesc::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SVSODesc::CalcCheckSum() << leftBorder << rightBorder << center << fDefaultOpacity;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SCragDesc::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "CragDesc", typeID, sizeof(*this) );
	SVSODesc::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "BorderRand", (uint8_t*)&fBorderRand - pThis, sizeof(fBorderRand), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Depth", (uint8_t*)&fDepth - pThis, sizeof(fDepth), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "DepthRand", (uint8_t*)&fDepthRand - pThis, sizeof(fDepthRand), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "RandX", (uint8_t*)&fRandX - pThis, sizeof(fRandX), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "RandY", (uint8_t*)&fRandY - pThis, sizeof(fRandY), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "HasPeak", (uint8_t*)&bHasPeak - pThis, sizeof(bHasPeak), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "RidgeMaterial", (uint8_t*)&pRidgeMaterial - pThis, sizeof(pRidgeMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "FootMaterial", (uint8_t*)&pFootMaterial - pThis, sizeof(pFootMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "LeftSided", (uint8_t*)&bLeftSided - pThis, sizeof(bLeftSided), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "RidgeTexGeomScale", (uint8_t*)&fRidgeTexGeomScale - pThis, sizeof(fRidgeTexGeomScale), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SCragDesc::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SVSODesc*)(this) );
	saver.Add( "BorderRand", &fBorderRand );
	saver.Add( "Depth", &fDepth );
	saver.Add( "DepthRand", &fDepthRand );
	saver.Add( "RandX", &fRandX );
	saver.Add( "RandY", &fRandY );
	saver.Add( "HasPeak", &bHasPeak );
	saver.Add( "RidgeMaterial", &pRidgeMaterial );
	saver.Add( "FootMaterial", &pFootMaterial );
	saver.Add( "LeftSided", &bLeftSided );
	saver.Add( "RidgeTexGeomScale", &fRidgeTexGeomScale );

	return 0;
}

int SCragDesc::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SVSODesc*)this );
	saver.Add( 2, &fBorderRand );
	saver.Add( 3, &fDepth );
	saver.Add( 4, &fDepthRand );
	saver.Add( 5, &fRandX );
	saver.Add( 6, &fRandY );
	saver.Add( 7, &bHasPeak );
	saver.Add( 8, &pRidgeMaterial );
	saver.Add( 9, &pFootMaterial );
	saver.Add( 10, &bLeftSided );
	saver.Add( 11, &fRidgeTexGeomScale );

	return 0;
}

uint32_t SCragDesc::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SVSODesc::CalcCheckSum() << fBorderRand << fDepth << fDepthRand << fRandX << fRandY << bHasPeak << bLeftSided << fRidgeTexGeomScale;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SRiverDesc::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "RiverDesc", typeID, sizeof(*this) );
	SVSODesc::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "BottomMaterial", (uint8_t*)&pBottomMaterial - pThis, sizeof(pBottomMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "PrecipiceMaterial", (uint8_t*)&pPrecipiceMaterial - pThis, sizeof(pPrecipiceMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "WaterMaterial", (uint8_t*)&pWaterMaterial - pThis, sizeof(pWaterMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "StreamSpeed", (uint8_t*)&fStreamSpeed - pThis, sizeof(fStreamSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "BorderRand", (uint8_t*)&fBorderRand - pThis, sizeof(fBorderRand), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Depth", (uint8_t*)&fDepth - pThis, sizeof(fDepth), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "DepthRand", (uint8_t*)&fDepthRand - pThis, sizeof(fDepthRand), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "RandX", (uint8_t*)&fRandX - pThis, sizeof(fRandX), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "RandY", (uint8_t*)&fRandY - pThis, sizeof(fRandY), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "RidgeTexGeomScale", (uint8_t*)&fRidgeTexGeomScale - pThis, sizeof(fRidgeTexGeomScale), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructArrayMetaInfo( "WaterLayers", &waterLayers, pThis );
	NMetaInfo::ReportMetaInfo( "HasPeak", (uint8_t*)&bHasPeak - pThis, sizeof(bHasPeak), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "DefaultWidth", (uint8_t*)&fDefaultWidth - pThis, sizeof(fDefaultWidth), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "DefaultOpacity", (uint8_t*)&fDefaultOpacity - pThis, sizeof(fDefaultOpacity), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SRiverDesc::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SVSODesc*)(this) );
	saver.Add( "BottomMaterial", &pBottomMaterial );
	saver.Add( "PrecipiceMaterial", &pPrecipiceMaterial );
	saver.Add( "WaterMaterial", &pWaterMaterial );
	saver.Add( "StreamSpeed", &fStreamSpeed );
	saver.Add( "BorderRand", &fBorderRand );
	saver.Add( "Depth", &fDepth );
	saver.Add( "DepthRand", &fDepthRand );
	saver.Add( "RandX", &fRandX );
	saver.Add( "RandY", &fRandY );
	saver.Add( "RidgeTexGeomScale", &fRidgeTexGeomScale );
	saver.Add( "WaterLayers", &waterLayers );
	saver.Add( "HasPeak", &bHasPeak );
	saver.Add( "DefaultWidth", &fDefaultWidth );
	saver.Add( "DefaultOpacity", &fDefaultOpacity );

	return 0;
}

int SRiverDesc::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SVSODesc*)this );
	saver.Add( 2, &pBottomMaterial );
	saver.Add( 3, &pPrecipiceMaterial );
	saver.Add( 4, &pWaterMaterial );
	saver.Add( 5, &fStreamSpeed );
	saver.Add( 6, &fBorderRand );
	saver.Add( 7, &fDepth );
	saver.Add( 8, &fDepthRand );
	saver.Add( 9, &fRandX );
	saver.Add( 10, &fRandY );
	saver.Add( 11, &fRidgeTexGeomScale );
	saver.Add( 12, &waterLayers );
	saver.Add( 13, &bHasPeak );
	saver.Add( 14, &fDefaultWidth );
	saver.Add( 15, &fDefaultOpacity );

	return 0;
}

uint32_t SRiverDesc::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SVSODesc::CalcCheckSum() << fStreamSpeed << fBorderRand << fDepth << fDepthRand << fRandX << fRandY << fRidgeTexGeomScale << waterLayers << bHasPeak << fDefaultWidth << fDefaultOpacity;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SCoastDesc::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "CoastDesc", typeID, sizeof(*this) );
	SVSODesc::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "Water", (uint8_t*)&pWater - pThis, sizeof(pWater), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "MiniMapGradientWidth", (uint8_t*)&nMiniMapGradientWidth - pThis, sizeof(nMiniMapGradientWidth), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::FinishMetaInfoReport();
}

int SCoastDesc::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SVSODesc*)(this) );
	saver.Add( "Water", &pWater );
	saver.Add( "MiniMapGradientWidth", &nMiniMapGradientWidth );

	return 0;
}

int SCoastDesc::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SVSODesc*)this );
	saver.Add( 2, &pWater );
	saver.Add( 3, &nMiniMapGradientWidth );

	return 0;
}

uint32_t SCoastDesc::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SVSODesc::CalcCheckSum() << nMiniMapGradientWidth;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SLakeDesc::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "LakeDesc", typeID, sizeof(*this) );
	SVSODesc::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "WaterParams", (uint8_t*)&pWaterParams - pThis, sizeof(pWaterParams), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "IsLake", (uint8_t*)&bIsLake - pThis, sizeof(bIsLake), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "MiniMapGradientWidth", (uint8_t*)&nMiniMapGradientWidth - pThis, sizeof(nMiniMapGradientWidth), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::FinishMetaInfoReport();
}

int SLakeDesc::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SVSODesc*)(this) );
	saver.Add( "WaterParams", &pWaterParams );
	saver.Add( "IsLake", &bIsLake );
	saver.Add( "MiniMapGradientWidth", &nMiniMapGradientWidth );

	return 0;
}

int SLakeDesc::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SVSODesc*)this );
	saver.Add( 2, &pWaterParams );
	saver.Add( 3, &bIsLake );
	saver.Add( 4, &nMiniMapGradientWidth );

	return 0;
}

uint32_t SLakeDesc::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SVSODesc::CalcCheckSum() << bIsLake << nMiniMapGradientWidth;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}

}
using namespace NDb;
BASIC_REGISTER_DATABASE_CLASS( B2_M1_TERRAIN, SVSODesc )
REGISTER_DATABASE_CLASS( B2_M1_TERRAIN, 0x1007C380, SRoadDesc )
REGISTER_DATABASE_CLASS( B2_M1_TERRAIN, 0x1308AC00, SCragDesc )
REGISTER_DATABASE_CLASS( B2_M1_TERRAIN, 0x10094B80, SRiverDesc )
REGISTER_DATABASE_CLASS( B2_M1_TERRAIN, 0x140C9400, SCoastDesc )
REGISTER_DATABASE_CLASS( B2_M1_TERRAIN, 0x100C8300, SLakeDesc )

