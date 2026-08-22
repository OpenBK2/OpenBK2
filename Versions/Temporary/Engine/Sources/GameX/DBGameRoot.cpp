// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "DBGameRoot.h"

#include "GameX_export.h"

#include <cstdint>

namespace NDb
{



void SUISoundEntry::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Type", (uint8_t*)&szType - pThis, sizeof(szType), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "Sound", (uint8_t*)&pSound - pThis, sizeof(pSound), NTypeDef::TYPE_TYPE_REF );
}

int SUISoundEntry::operator&( IXmlSaver &saver )
{
	saver.Add( "Type", &szType );
	saver.Add( "Sound", &pSound );

	return 0;
}

int SUISoundEntry::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szType );
	saver.Add( 3, &pSound );

	return 0;
}

uint32_t SUISoundEntry::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szType;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMainMenuBackground::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Map", (uint8_t*)&pMap - pThis, sizeof(pMap), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Picture", (uint8_t*)&pPicture - pThis, sizeof(pPicture), NTypeDef::TYPE_TYPE_REF );
}

int SMainMenuBackground::operator&( IXmlSaver &saver )
{
	saver.Add( "Map", &pMap );
	saver.Add( "Picture", &pPicture );

	return 0;
}

int SMainMenuBackground::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pMap );
	saver.Add( 3, &pPicture );

	return 0;
}

uint32_t SMainMenuBackground::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << pMap;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SHallOfFameRecord::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "NameFileRef", (uint8_t*)&szNameFileRef - pThis, sizeof(szNameFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "Score", (uint8_t*)&nScore - pThis, sizeof(nScore), NTypeDef::TYPE_TYPE_INT );
}

int SHallOfFameRecord::operator&( IXmlSaver &saver )
{
	saver.Add( "NameFileRef", &szNameFileRef );
	saver.Add( "Score", &nScore );

	return 0;
}

int SHallOfFameRecord::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szNameFileRef );
	saver.Add( 3, &nScore );

	return 0;
}

uint32_t SHallOfFameRecord::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nScore;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SGameRoot::STutorialMap::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "MapInfo", (uint8_t*)&pMapInfo - pThis, sizeof(pMapInfo), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "DifficultyFileRef", (uint8_t*)&szDifficultyFileRef - pThis, sizeof(szDifficultyFileRef), NTypeDef::TYPE_TYPE_STRING );
}

int SGameRoot::STutorialMap::operator&( IXmlSaver &saver )
{
	saver.Add( "MapInfo", &pMapInfo );
	saver.Add( "DifficultyFileRef", &szDifficultyFileRef );

	return 0;
}

int SGameRoot::STutorialMap::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pMapInfo );
	saver.Add( 3, &szDifficultyFileRef );

	return 0;
}

uint32_t SGameRoot::STutorialMap::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << pMapInfo;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SGameRoot::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "GameRoot", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "Consts", (uint8_t*)&pConsts - pThis, sizeof(pConsts), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportSimpleArrayMetaInfo( "Campaigns", &campaigns, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "TutorialMaps", &tutorialMaps, pThis );
	NMetaInfo::ReportMetaInfo( "ScreenVideoPlayer", (uint8_t*)&pScreenVideoPlayer - pThis, sizeof(pScreenVideoPlayer), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructArrayMetaInfo( "Screens", &screens, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "TextEntries", &textEntries, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "Fonts", &fonts, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "Sounds", &sounds, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "Textures", &textures, pThis );
	NMetaInfo::ReportMetaInfo( "GameOptions", (uint8_t*)&pGameOptions - pThis, sizeof(pGameOptions), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "MainMenuBackground", &mainMenuBackground, pThis ); 
	NMetaInfo::ReportMetaInfo( "InterfacesBackground", (uint8_t*)&pInterfacesBackground - pThis, sizeof(pInterfacesBackground), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportSimpleArrayMetaInfo( "Notifications", &notifications, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "NotificationEvents", &notificationEvents, pThis );
	NMetaInfo::ReportMetaInfo( "IntroMovie", (uint8_t*)&szIntroMovie - pThis, sizeof(szIntroMovie), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportSimpleArrayMetaInfo( "EncyclopediaMechUnits", &encyclopediaMechUnits, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "CitationFileRefs", &citationFileRefs, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "MultiplayerMaps", &multiplayerMaps, pThis );
	NMetaInfo::ReportMetaInfo( "TestMap", (uint8_t*)&pTestMap - pThis, sizeof(pTestMap), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "MainMenuMusic", (uint8_t*)&pMainMenuMusic - pThis, sizeof(pMainMenuMusic), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructArrayMetaInfo( "HallOfFameDefaultRecords", &hallOfFameDefaultRecords, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SGameRoot::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Consts", &pConsts );
	saver.Add( "Campaigns", &campaigns );
	saver.Add( "TutorialMaps", &tutorialMaps );
	saver.Add( "ScreenVideoPlayer", &pScreenVideoPlayer );
	saver.Add( "Screens", &screens );
	saver.Add( "TextEntries", &textEntries );
	saver.Add( "Fonts", &fonts );
	saver.Add( "Sounds", &sounds );
	saver.Add( "Textures", &textures );
	saver.Add( "GameOptions", &pGameOptions );
	saver.Add( "MainMenuBackground", &mainMenuBackground );
	saver.Add( "InterfacesBackground", &pInterfacesBackground );
	saver.Add( "Notifications", &notifications );
	saver.Add( "NotificationEvents", &notificationEvents );
	saver.Add( "IntroMovie", &szIntroMovie );
	saver.Add( "EncyclopediaMechUnits", &encyclopediaMechUnits );
	saver.Add( "CitationFileRefs", &citationFileRefs );
	saver.Add( "MultiplayerMaps", &multiplayerMaps );
	saver.Add( "TestMap", &pTestMap );
	saver.Add( "MainMenuMusic", &pMainMenuMusic );
	saver.Add( "HallOfFameDefaultRecords", &hallOfFameDefaultRecords );

	return 0;
}

int SGameRoot::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pConsts );
	saver.Add( 3, &campaigns );
	saver.Add( 4, &tutorialMaps );
	saver.Add( 5, &pScreenVideoPlayer );
	saver.Add( 6, &screens );
	saver.Add( 7, &textEntries );
	saver.Add( 8, &fonts );
	saver.Add( 9, &sounds );
	saver.Add( 10, &textures );
	saver.Add( 11, &pGameOptions );
	saver.Add( 12, &mainMenuBackground );
	saver.Add( 13, &pInterfacesBackground );
	saver.Add( 14, &notifications );
	saver.Add( 15, &notificationEvents );
	saver.Add( 16, &szIntroMovie );
	saver.Add( 17, &encyclopediaMechUnits );
	saver.Add( 18, &citationFileRefs );
	saver.Add( 19, &multiplayerMaps );
	saver.Add( 20, &pTestMap );
	saver.Add( 21, &pMainMenuMusic );
	saver.Add( 22, &hallOfFameDefaultRecords );

	return 0;
}

uint32_t SGameRoot::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << pConsts << campaigns << tutorialMaps << screens << textEntries << fonts << sounds << textures << mainMenuBackground << notifications << notificationEvents << encyclopediaMechUnits << multiplayerMaps << pTestMap << hallOfFameDefaultRecords;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( GAMEX, 0x1007B4C1, SGameRoot )

