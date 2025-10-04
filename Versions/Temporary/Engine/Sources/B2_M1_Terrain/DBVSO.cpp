// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "DBVSO.h"

namespace NDb
{



void STerrainAIProperties::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Passability", (BYTE*)&fPassability - pThis, sizeof(fPassability), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "AIClass", (BYTE*)&nAIClass - pThis, sizeof(nAIClass), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "AIPassabilityClass", (BYTE*)&nAIPassabilityClass - pThis, sizeof(nAIPassabilityClass), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "CanEntrench", (BYTE*)&bCanEntrench - pThis, sizeof(bCanEntrench), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "SoilType", (BYTE*)&nSoilType - pThis, sizeof(nSoilType), NTypeDef::TYPE_TYPE_INT );
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

DWORD STerrainAIProperties::CalcCheckSum() const
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



void SVSOLayerBaseDesc::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "CenterOpacity", (BYTE*)&fCenterOpacity - pThis, sizeof(fCenterOpacity), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "TilingStep", (BYTE*)&fTilingStep - pThis, sizeof(fTilingStep), NTypeDef::TYPE_TYPE_FLOAT );
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

DWORD SVSOLayerBaseDesc::CalcCheckSum() const
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



void SVSOLayerBorderDesc::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	SVSOLayerBaseDesc::ReportMetaInfo( szAddName, pThis );

	NMetaInfo::ReportMetaInfo( szAddName + "Material", (BYTE*)&pMaterial - pThis, sizeof(pMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "UseFromPixel", (BYTE*)&nUseFromPixel - pThis, sizeof(nUseFromPixel), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "UseToPixel", (BYTE*)&nUseToPixel - pThis, sizeof(nUseToPixel), NTypeDef::TYPE_TYPE_INT );
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

DWORD SVSOLayerBorderDesc::CalcCheckSum() const
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



void SVSOLayerCenterDesc::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	SVSOLayerBaseDesc::ReportMetaInfo( szAddName, pThis );

	NMetaInfo::ReportMetaInfo( szAddName + "Disturbance", (BYTE*)&fDisturbance - pThis, sizeof(fDisturbance), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "NumCells", (BYTE*)&nNumCells - pThis, sizeof(nNumCells), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "Materials", &materials, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "UseFromPixel", (BYTE*)&nUseFromPixel - pThis, sizeof(nUseFromPixel), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "UseToPixel", (BYTE*)&nUseToPixel - pThis, sizeof(nUseToPixel), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "StreamSpeed", (BYTE*)&fStreamSpeed - pThis, sizeof(fStreamSpeed), NTypeDef::TYPE_TYPE_FLOAT );
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

DWORD SVSOLayerCenterDesc::CalcCheckSum() const
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
	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Type", (BYTE*)&nType - pThis, sizeof(nType), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "Priority", (BYTE*)&nPriority - pThis, sizeof(nPriority), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructMetaInfo( "AIProperty", &aIProperty, pThis ); 
	NMetaInfo::ReportMetaInfo( "MiniMapCenterColor", (BYTE*)&nMiniMapCenterColor - pThis, sizeof(nMiniMapCenterColor), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "MiniMapBorderColor", (BYTE*)&nMiniMapBorderColor - pThis, sizeof(nMiniMapBorderColor), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "MiniMapCenterWidth", (BYTE*)&nMiniMapCenterWidth - pThis, sizeof(nMiniMapCenterWidth), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "AmbientSound", (BYTE*)&pAmbientSound - pThis, sizeof(pAmbientSound), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "CycledSound", (BYTE*)&pCycledSound - pThis, sizeof(pCycledSound), NTypeDef::TYPE_TYPE_REF );
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

DWORD SVSODesc::CalcCheckSum() const
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



void SVSOPoint::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Pos", &vPos, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "Norm", &vNorm, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "Width", (BYTE*)&fWidth - pThis, sizeof(fWidth), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Opacity", (BYTE*)&fOpacity - pThis, sizeof(fOpacity), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "KeyPoint", (BYTE*)&bKeyPoint - pThis, sizeof(bKeyPoint), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "Radius", (BYTE*)&fRadius - pThis, sizeof(fRadius), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Reserved", (BYTE*)&fReserved - pThis, sizeof(fReserved), NTypeDef::TYPE_TYPE_FLOAT );
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

DWORD SVSOPoint::CalcCheckSum() const
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



void SVSOInstance::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Descriptor", (BYTE*)&pDescriptor - pThis, sizeof(pDescriptor), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "points", &points, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "ControlPoints", &controlPoints, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "VSOID", (BYTE*)&nVSOID - pThis, sizeof(nVSOID), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "CMArrowType", (BYTE*)&nCMArrowType - pThis, sizeof(nCMArrowType), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "CMArrowMission", (BYTE*)&nCMArrowMission - pThis, sizeof(nCMArrowMission), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "CMArrowMission2", (BYTE*)&nCMArrowMission2 - pThis, sizeof(nCMArrowMission2), NTypeDef::TYPE_TYPE_INT );
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

DWORD SVSOInstance::CalcCheckSum() const
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructMetaInfo( "LeftBorder", &leftBorder, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "RightBorder", &rightBorder, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "Center", &center, pThis ); 
	NMetaInfo::ReportMetaInfo( "DefaultOpacity", (BYTE*)&fDefaultOpacity - pThis, sizeof(fDefaultOpacity), NTypeDef::TYPE_TYPE_FLOAT );
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

DWORD SRoadDesc::CalcCheckSum() const
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "BorderRand", (BYTE*)&fBorderRand - pThis, sizeof(fBorderRand), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Depth", (BYTE*)&fDepth - pThis, sizeof(fDepth), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "DepthRand", (BYTE*)&fDepthRand - pThis, sizeof(fDepthRand), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "RandX", (BYTE*)&fRandX - pThis, sizeof(fRandX), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "RandY", (BYTE*)&fRandY - pThis, sizeof(fRandY), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "HasPeak", (BYTE*)&bHasPeak - pThis, sizeof(bHasPeak), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "RidgeMaterial", (BYTE*)&pRidgeMaterial - pThis, sizeof(pRidgeMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "FootMaterial", (BYTE*)&pFootMaterial - pThis, sizeof(pFootMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "LeftSided", (BYTE*)&bLeftSided - pThis, sizeof(bLeftSided), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "RidgeTexGeomScale", (BYTE*)&fRidgeTexGeomScale - pThis, sizeof(fRidgeTexGeomScale), NTypeDef::TYPE_TYPE_FLOAT );
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

DWORD SCragDesc::CalcCheckSum() const
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "BottomMaterial", (BYTE*)&pBottomMaterial - pThis, sizeof(pBottomMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "PrecipiceMaterial", (BYTE*)&pPrecipiceMaterial - pThis, sizeof(pPrecipiceMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "WaterMaterial", (BYTE*)&pWaterMaterial - pThis, sizeof(pWaterMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "StreamSpeed", (BYTE*)&fStreamSpeed - pThis, sizeof(fStreamSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "BorderRand", (BYTE*)&fBorderRand - pThis, sizeof(fBorderRand), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Depth", (BYTE*)&fDepth - pThis, sizeof(fDepth), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "DepthRand", (BYTE*)&fDepthRand - pThis, sizeof(fDepthRand), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "RandX", (BYTE*)&fRandX - pThis, sizeof(fRandX), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "RandY", (BYTE*)&fRandY - pThis, sizeof(fRandY), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "RidgeTexGeomScale", (BYTE*)&fRidgeTexGeomScale - pThis, sizeof(fRidgeTexGeomScale), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructArrayMetaInfo( "WaterLayers", &waterLayers, pThis );
	NMetaInfo::ReportMetaInfo( "HasPeak", (BYTE*)&bHasPeak - pThis, sizeof(bHasPeak), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "DefaultWidth", (BYTE*)&fDefaultWidth - pThis, sizeof(fDefaultWidth), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "DefaultOpacity", (BYTE*)&fDefaultOpacity - pThis, sizeof(fDefaultOpacity), NTypeDef::TYPE_TYPE_FLOAT );
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

DWORD SRiverDesc::CalcCheckSum() const
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Water", (BYTE*)&pWater - pThis, sizeof(pWater), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "MiniMapGradientWidth", (BYTE*)&nMiniMapGradientWidth - pThis, sizeof(nMiniMapGradientWidth), NTypeDef::TYPE_TYPE_INT );
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

DWORD SCoastDesc::CalcCheckSum() const
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "WaterParams", (BYTE*)&pWaterParams - pThis, sizeof(pWaterParams), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "IsLake", (BYTE*)&bIsLake - pThis, sizeof(bIsLake), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "MiniMapGradientWidth", (BYTE*)&nMiniMapGradientWidth - pThis, sizeof(nMiniMapGradientWidth), NTypeDef::TYPE_TYPE_INT );
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

DWORD SLakeDesc::CalcCheckSum() const
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
BASIC_REGISTER_DATABASE_CLASS( SVSODesc )
REGISTER_DATABASE_CLASS( 0x1007C380, SRoadDesc ) 
REGISTER_DATABASE_CLASS( 0x1308AC00, SCragDesc ) 
REGISTER_DATABASE_CLASS( 0x10094B80, SRiverDesc ) 
REGISTER_DATABASE_CLASS( 0x140C9400, SCoastDesc ) 
REGISTER_DATABASE_CLASS( 0x100C8300, SLakeDesc ) 

