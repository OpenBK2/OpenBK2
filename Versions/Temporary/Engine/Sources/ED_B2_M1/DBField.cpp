// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "DBField.h"

#include <cstdint>

namespace NDb
{



void SFieldTileDesc::ReportMetaInfo( const string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Value", (uint8_t*)&nValue - pThis, sizeof(nValue), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "Weight", (uint8_t*)&nWeight - pThis, sizeof(nWeight), NTypeDef::TYPE_TYPE_INT );
}

int SFieldTileDesc::operator&( IXmlSaver &saver )
{
	saver.Add( "Value", &nValue );
	saver.Add( "Weight", &nWeight );

	return 0;
}

int SFieldTileDesc::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nValue );
	saver.Add( 3, &nWeight );

	return 0;
}

uint32_t SFieldTileDesc::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nValue << nWeight;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SFieldObjectDesc::ReportMetaInfo( const string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Value", (uint8_t*)&pValue - pThis, sizeof(pValue), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Weight", (uint8_t*)&nWeight - pThis, sizeof(nWeight), NTypeDef::TYPE_TYPE_INT );
}

int SFieldObjectDesc::operator&( IXmlSaver &saver )
{
	saver.Add( "Value", &pValue );
	saver.Add( "Weight", &nWeight );

	return 0;
}

int SFieldObjectDesc::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pValue );
	saver.Add( 3, &nWeight );

	return 0;
}

uint32_t SFieldObjectDesc::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << pValue << nWeight;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SFieldPatternSize::ReportMetaInfo( const string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Min", (uint8_t*)&nMin - pThis, sizeof(nMin), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "Max", (uint8_t*)&nMax - pThis, sizeof(nMax), NTypeDef::TYPE_TYPE_INT );
}

int SFieldPatternSize::operator&( IXmlSaver &saver )
{
	saver.Add( "Min", &nMin );
	saver.Add( "Max", &nMax );

	return 0;
}

int SFieldPatternSize::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nMin );
	saver.Add( 3, &nMax );

	return 0;
}

uint32_t SFieldPatternSize::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nMin << nMax;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SFieldTileShell::ReportMetaInfo( const string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "Tiles", &tiles, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "Width", (uint8_t*)&fWidth - pThis, sizeof(fWidth), NTypeDef::TYPE_TYPE_FLOAT );
}

int SFieldTileShell::operator&( IXmlSaver &saver )
{
	saver.Add( "Tiles", &tiles );
	saver.Add( "Width", &fWidth );

	return 0;
}

int SFieldTileShell::operator&( IBinSaver &saver )
{
	saver.Add( 2, &tiles );
	saver.Add( 3, &fWidth );

	return 0;
}

uint32_t SFieldTileShell::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << tiles << fWidth;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SFieldObjectShell::ReportMetaInfo( const string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "Objects", &objects, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "Width", (uint8_t*)&fWidth - pThis, sizeof(fWidth), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "BetweenDistance", (uint8_t*)&nBetweenDistance - pThis, sizeof(nBetweenDistance), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "Ratio", (uint8_t*)&fRatio - pThis, sizeof(fRatio), NTypeDef::TYPE_TYPE_FLOAT );
}

int SFieldObjectShell::operator&( IXmlSaver &saver )
{
	saver.Add( "Objects", &objects );
	saver.Add( "Width", &fWidth );
	saver.Add( "BetweenDistance", &nBetweenDistance );
	saver.Add( "Ratio", &fRatio );

	return 0;
}

int SFieldObjectShell::operator&( IBinSaver &saver )
{
	saver.Add( 2, &objects );
	saver.Add( 3, &fWidth );
	saver.Add( 4, &nBetweenDistance );
	saver.Add( 5, &fRatio );

	return 0;
}

uint32_t SFieldObjectShell::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << objects << fWidth << nBetweenDistance << fRatio;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SField::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Field", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "TileShells", &tileShells, pThis );
	NMetaInfo::ReportMetaInfo( "TerraSet", (uint8_t*)&pTerraSet - pThis, sizeof(pTerraSet), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructArrayMetaInfo( "ObjectShells", &objectShells, pThis );
	NMetaInfo::ReportMetaInfo( "ProfileFileName", (uint8_t*)&szProfileFileName - pThis, sizeof(szProfileFileName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "Height", (uint8_t*)&fHeight - pThis, sizeof(fHeight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructMetaInfo( "PatternSize", &patternSize, pThis ); 
	NMetaInfo::ReportMetaInfo( "PositiveRatio", (uint8_t*)&fPositiveRatio - pThis, sizeof(fPositiveRatio), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SField::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "TileShells", &tileShells );
	saver.Add( "TerraSet", &pTerraSet );
	saver.Add( "ObjectShells", &objectShells );
	saver.Add( "ProfileFileName", &szProfileFileName );
	saver.Add( "Height", &fHeight );
	saver.Add( "PatternSize", &patternSize );
	saver.Add( "PositiveRatio", &fPositiveRatio );

	return 0;
}

int SField::operator&( IBinSaver &saver )
{
	saver.Add( 2, &tileShells );
	saver.Add( 3, &pTerraSet );
	saver.Add( 4, &objectShells );
	saver.Add( 5, &szProfileFileName );
	saver.Add( 6, &fHeight );
	saver.Add( 7, &patternSize );
	saver.Add( 8, &fPositiveRatio );

	return 0;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( 0x14130C40, SField ) 

