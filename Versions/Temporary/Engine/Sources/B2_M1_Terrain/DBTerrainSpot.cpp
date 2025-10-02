// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "dbterrainspot.h"

namespace NDb
{



void STerrainSpotDesc::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "TerrainSpotDesc", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Material", (BYTE*)&pMaterial - pThis, sizeof(pMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "UsedTexSizeX", (BYTE*)&fUsedTexSizeX - pThis, sizeof(fUsedTexSizeX), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "UsedTexSizeY", (BYTE*)&fUsedTexSizeY - pThis, sizeof(fUsedTexSizeY), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ScaleCoeff", (BYTE*)&fScaleCoeff - pThis, sizeof(fScaleCoeff), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int STerrainSpotDesc::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Material", &pMaterial );
	saver.Add( "UsedTexSizeX", &fUsedTexSizeX );
	saver.Add( "UsedTexSizeY", &fUsedTexSizeY );
	saver.Add( "ScaleCoeff", &fScaleCoeff );

	return 0;
}

int STerrainSpotDesc::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pMaterial );
	saver.Add( 3, &fUsedTexSizeX );
	saver.Add( 4, &fUsedTexSizeY );
	saver.Add( 5, &fScaleCoeff );

	return 0;
}



void STerrainSpotInstance::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Descriptor", (BYTE*)&pDescriptor - pThis, sizeof(pDescriptor), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "SpotID", (BYTE*)&nSpotID - pThis, sizeof(nSpotID), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "points", &points, pThis );
}

int STerrainSpotInstance::operator&( IXmlSaver &saver )
{
	saver.Add( "Descriptor", &pDescriptor );
	saver.Add( "SpotID", &nSpotID );
	saver.Add( "points", &points );

	return 0;
}

int STerrainSpotInstance::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pDescriptor );
	saver.Add( 3, &nSpotID );
	saver.Add( 4, &points );

	return 0;
}

DWORD STerrainSpotInstance::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nSpotID << points;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( 0x100AC382, STerrainSpotDesc ) 

