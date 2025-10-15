// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "uientries.h"

#include "Stats_B2_M1_export.h"

#include <cstdint>

namespace NDb
{



void SUIScreenEntry::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Screen", (uint8_t*)&pScreen - pThis, sizeof(pScreen), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Type", (uint8_t*)&szType - pThis, sizeof(szType), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "HelpHeaderFileRef", (uint8_t*)&szHelpHeaderFileRef - pThis, sizeof(szHelpHeaderFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "HelpDescFileRef", (uint8_t*)&szHelpDescFileRef - pThis, sizeof(szHelpDescFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "HelpNoMultiplayer", (uint8_t*)&bHelpNoMultiplayer - pThis, sizeof(bHelpNoMultiplayer), NTypeDef::TYPE_TYPE_BOOL );
}

int SUIScreenEntry::operator&( IXmlSaver &saver )
{
	saver.Add( "Screen", &pScreen );
	saver.Add( "Type", &szType );
	saver.Add( "HelpHeaderFileRef", &szHelpHeaderFileRef );
	saver.Add( "HelpDescFileRef", &szHelpDescFileRef );
	saver.Add( "HelpNoMultiplayer", &bHelpNoMultiplayer );

	return 0;
}

int SUIScreenEntry::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pScreen );
	saver.Add( 3, &szType );
	saver.Add( 4, &szHelpHeaderFileRef );
	saver.Add( 5, &szHelpDescFileRef );
	saver.Add( 6, &bHelpNoMultiplayer );

	return 0;
}

uint32_t SUIScreenEntry::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szType << bHelpNoMultiplayer;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SUITextEntry::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "TextFileRef", (uint8_t*)&szTextFileRef - pThis, sizeof(szTextFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "TextID", (uint8_t*)&szTextID - pThis, sizeof(szTextID), NTypeDef::TYPE_TYPE_STRING );
}

int SUITextEntry::operator&( IXmlSaver &saver )
{
	saver.Add( "TextFileRef", &szTextFileRef );
	saver.Add( "TextID", &szTextID );

	return 0;
}

int SUITextEntry::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szTextFileRef );
	saver.Add( 3, &szTextID );

	return 0;
}

uint32_t SUITextEntry::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szTextID;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SUITextureEntry::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "TextID", (uint8_t*)&szTextID - pThis, sizeof(szTextID), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "Texture", (uint8_t*)&pTexture - pThis, sizeof(pTexture), NTypeDef::TYPE_TYPE_REF );
}

int SUITextureEntry::operator&( IXmlSaver &saver )
{
	saver.Add( "TextID", &szTextID );
	saver.Add( "Texture", &pTexture );

	return 0;
}

int SUITextureEntry::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szTextID );
	saver.Add( 3, &pTexture );

	return 0;
}

uint32_t SUITextureEntry::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szTextID;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void STextEntry::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "TextEntry", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "Name", (uint8_t*)&szName - pThis, sizeof(szName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "TextFileRef", (uint8_t*)&szTextFileRef - pThis, sizeof(szTextFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::FinishMetaInfoReport();
}

int STextEntry::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Name", &szName );
	saver.Add( "TextFileRef", &szTextFileRef );

	return 0;
}

int STextEntry::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szName );
	saver.Add( 3, &szTextFileRef );

	return 0;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( STATS_B2_M1, 0x171AE380, STextEntry )

