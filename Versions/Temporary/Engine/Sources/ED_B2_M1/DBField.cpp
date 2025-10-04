// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "DBField.h"

namespace NDb
{



void SFieldTileDesc::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Value", (BYTE*)&nValue - pThis, sizeof(nValue), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "Weight", (BYTE*)&nWeight - pThis, sizeof(nWeight), NTypeDef::TYPE_TYPE_INT );
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

DWORD SFieldTileDesc::CalcCheckSum() const
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



void SFieldObjectDesc::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Value", (BYTE*)&pValue - pThis, sizeof(pValue), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Weight", (BYTE*)&nWeight - pThis, sizeof(nWeight), NTypeDef::TYPE_TYPE_INT );
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

DWORD SFieldObjectDesc::CalcCheckSum() const
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



void SFieldPatternSize::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Min", (BYTE*)&nMin - pThis, sizeof(nMin), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "Max", (BYTE*)&nMax - pThis, sizeof(nMax), NTypeDef::TYPE_TYPE_INT );
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

DWORD SFieldPatternSize::CalcCheckSum() const
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



void SFieldTileShell::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "Tiles", &tiles, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "Width", (BYTE*)&fWidth - pThis, sizeof(fWidth), NTypeDef::TYPE_TYPE_FLOAT );
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

DWORD SFieldTileShell::CalcCheckSum() const
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



void SFieldObjectShell::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "Objects", &objects, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "Width", (BYTE*)&fWidth - pThis, sizeof(fWidth), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "BetweenDistance", (BYTE*)&nBetweenDistance - pThis, sizeof(nBetweenDistance), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "Ratio", (BYTE*)&fRatio - pThis, sizeof(fRatio), NTypeDef::TYPE_TYPE_FLOAT );
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

DWORD SFieldObjectShell::CalcCheckSum() const
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "TileShells", &tileShells, pThis );
	NMetaInfo::ReportMetaInfo( "TerraSet", (BYTE*)&pTerraSet - pThis, sizeof(pTerraSet), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructArrayMetaInfo( "ObjectShells", &objectShells, pThis );
	NMetaInfo::ReportMetaInfo( "ProfileFileName", (BYTE*)&szProfileFileName - pThis, sizeof(szProfileFileName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "Height", (BYTE*)&fHeight - pThis, sizeof(fHeight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructMetaInfo( "PatternSize", &patternSize, pThis ); 
	NMetaInfo::ReportMetaInfo( "PositiveRatio", (BYTE*)&fPositiveRatio - pThis, sizeof(fPositiveRatio), NTypeDef::TYPE_TYPE_FLOAT );
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

