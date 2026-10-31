// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "DBFlags.h"

#include "GameX_export.h"

#include <cstdint>

namespace NDb
{



void SFlagImage::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "FlagImage", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "Image", (uint8_t*)&szImage - pThis, sizeof(szImage), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::FinishMetaInfoReport();
}

int SFlagImage::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Image", &szImage );

	return 0;
}

int SFlagImage::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szImage );

	return 0;
}

uint32_t SFlagImage::CalcCheckSum() const
{
	CCheckSumScope scope( __dwCheckSum );
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SFlagAuthority::SPeriod::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "FromYear", (uint8_t*)&nFromYear - pThis, sizeof(nFromYear), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "ToYear", (uint8_t*)&nToYear - pThis, sizeof(nToYear), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "Image", (uint8_t*)&pImage - pThis, sizeof(pImage), NTypeDef::TYPE_TYPE_REF );
}

int SFlagAuthority::SPeriod::operator&( IXmlSaver &saver )
{
	saver.Add( "FromYear", &nFromYear );
	saver.Add( "ToYear", &nToYear );
	saver.Add( "Image", &pImage );

	return 0;
}

int SFlagAuthority::SPeriod::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nFromYear );
	saver.Add( 3, &nToYear );
	saver.Add( 4, &pImage );

	return 0;
}

uint32_t SFlagAuthority::SPeriod::CalcCheckSum() const
{
	CCheckSumScope scope( __dwCheckSum );
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nFromYear << nToYear << pImage;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SFlagAuthority::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "FlagAuthority", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "NameFileRef", (uint8_t*)&szNameFileRef - pThis, sizeof(szNameFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructArrayMetaInfo( "Periods", &periods, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SFlagAuthority::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "NameFileRef", &szNameFileRef );
	saver.Add( "Periods", &periods );

	return 0;
}

int SFlagAuthority::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szNameFileRef );
	saver.Add( 3, &periods );

	return 0;
}

uint32_t SFlagAuthority::CalcCheckSum() const
{
	CCheckSumScope scope( __dwCheckSum );
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << periods;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SFlagTerritory::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "FlagTerritory", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "NameFileRef", (uint8_t*)&szNameFileRef - pThis, sizeof(szNameFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportSimpleArrayMetaInfo( "Authorities", &authorities, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SFlagTerritory::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "NameFileRef", &szNameFileRef );
	saver.Add( "Authorities", &authorities );

	return 0;
}

int SFlagTerritory::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szNameFileRef );
	saver.Add( 3, &authorities );

	return 0;
}

uint32_t SFlagTerritory::CalcCheckSum() const
{
	CCheckSumScope scope( __dwCheckSum );
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << authorities;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SFlagMapping::SIcon::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "ImageHash", (uint8_t*)&szImageHash - pThis, sizeof(szImageHash), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "Territory", (uint8_t*)&pTerritory - pThis, sizeof(pTerritory), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Authority", (uint8_t*)&pAuthority - pThis, sizeof(pAuthority), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Image", (uint8_t*)&pImage - pThis, sizeof(pImage), NTypeDef::TYPE_TYPE_REF );
}

int SFlagMapping::SIcon::operator&( IXmlSaver &saver )
{
	saver.Add( "ImageHash", &szImageHash );
	saver.Add( "Territory", &pTerritory );
	saver.Add( "Authority", &pAuthority );
	saver.Add( "Image", &pImage );

	return 0;
}

int SFlagMapping::SIcon::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szImageHash );
	saver.Add( 3, &pTerritory );
	saver.Add( 4, &pAuthority );
	saver.Add( 5, &pImage );

	return 0;
}

uint32_t SFlagMapping::SIcon::CalcCheckSum() const
{
	CCheckSumScope scope( __dwCheckSum );
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szImageHash << pTerritory << pAuthority << pImage;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SFlagMapping::STechLevel::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Name", (uint8_t*)&szName - pThis, sizeof(szName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "FromYear", (uint8_t*)&nFromYear - pThis, sizeof(nFromYear), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "ToYear", (uint8_t*)&nToYear - pThis, sizeof(nToYear), NTypeDef::TYPE_TYPE_INT );
}

int SFlagMapping::STechLevel::operator&( IXmlSaver &saver )
{
	saver.Add( "Name", &szName );
	saver.Add( "FromYear", &nFromYear );
	saver.Add( "ToYear", &nToYear );

	return 0;
}

int SFlagMapping::STechLevel::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szName );
	saver.Add( 3, &nFromYear );
	saver.Add( 4, &nToYear );

	return 0;
}

uint32_t SFlagMapping::STechLevel::CalcCheckSum() const
{
	CCheckSumScope scope( __dwCheckSum );
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szName << nFromYear << nToYear;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SFlagMapping::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "FlagMapping", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "Icons", &icons, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "TechLevels", &techLevels, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SFlagMapping::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Icons", &icons );
	saver.Add( "TechLevels", &techLevels );

	return 0;
}

int SFlagMapping::operator&( IBinSaver &saver )
{
	saver.Add( 2, &icons );
	saver.Add( 3, &techLevels );

	return 0;
}

uint32_t SFlagMapping::CalcCheckSum() const
{
	CCheckSumScope scope( __dwCheckSum );
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << icons << techLevels;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( GAMEX, 0x1F1A6C10, SFlagImage )
REGISTER_DATABASE_CLASS( GAMEX, 0x1F1A6C11, SFlagAuthority )
REGISTER_DATABASE_CLASS( GAMEX, 0x1F1A6C12, SFlagTerritory )
REGISTER_DATABASE_CLASS( GAMEX, 0x1F1A6C13, SFlagMapping )
