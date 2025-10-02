// automatically generated file, don't change manually!

#include "stdafx.h"
#include "../libdb/ReportMetaInfo.h"
#include "../libdb/Checksum.h"
#include "../System/XmlSaver.h"
#include "uientries.h"

namespace NDb
{



void SUIScreenEntry::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Screen", (BYTE*)&pScreen - pThis, sizeof(pScreen), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Type", (BYTE*)&szType - pThis, sizeof(szType), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "HelpHeaderFileRef", (BYTE*)&szHelpHeaderFileRef - pThis, sizeof(szHelpHeaderFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "HelpDescFileRef", (BYTE*)&szHelpDescFileRef - pThis, sizeof(szHelpDescFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "HelpNoMultiplayer", (BYTE*)&bHelpNoMultiplayer - pThis, sizeof(bHelpNoMultiplayer), NTypeDef::TYPE_TYPE_BOOL );
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

DWORD SUIScreenEntry::CalcCheckSum() const
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



void SUITextEntry::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "TextFileRef", (BYTE*)&szTextFileRef - pThis, sizeof(szTextFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "TextID", (BYTE*)&szTextID - pThis, sizeof(szTextID), NTypeDef::TYPE_TYPE_STRING );
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

DWORD SUITextEntry::CalcCheckSum() const
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



void SUITextureEntry::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "TextID", (BYTE*)&szTextID - pThis, sizeof(szTextID), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "Texture", (BYTE*)&pTexture - pThis, sizeof(pTexture), NTypeDef::TYPE_TYPE_REF );
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

DWORD SUITextureEntry::CalcCheckSum() const
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Name", (BYTE*)&szName - pThis, sizeof(szName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "TextFileRef", (BYTE*)&szTextFileRef - pThis, sizeof(szTextFileRef), NTypeDef::TYPE_TYPE_STRING );
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
REGISTER_DATABASE_CLASS( 0x171AE380, STextEntry ) 
