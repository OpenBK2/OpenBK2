// automatically generated file, don't change manually!

#include "stdafx.h"
#include "../libdb/ReportMetaInfo.h"
#include "../libdb/Checksum.h"
#include "../System/XmlSaver.h"
#include "dbmpconsts.h"

namespace NDb
{


string EnumToString( NDb::EHistoricalSide eValue )
{
	switch ( eValue )
	{
	case NDb::HS_ALLIES:
		return "HS_ALLIES";
	case NDb::HS_AXIS:
		return "HS_AXIS";
	default:
		return "HS_ALLIES";
	}
}

NDb::EHistoricalSide NDb::StringToEnum_NDb_EHistoricalSide( const string &szValue )
{
	if ( szValue == "HS_ALLIES" )
		return NDb::HS_ALLIES;
	if ( szValue == "HS_AXIS" )
		return NDb::HS_AXIS;
	return NDb::HS_ALLIES;
}


void SMultiplayerTechLevel::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "NameFileRef", (BYTE*)&szNameFileRef - pThis, sizeof(szNameFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "DescriptionFileRef", (BYTE*)&szDescriptionFileRef - pThis, sizeof(szDescriptionFileRef), NTypeDef::TYPE_TYPE_STRING );
}

int SMultiplayerTechLevel::operator&( IXmlSaver &saver )
{
	saver.Add( "NameFileRef", &szNameFileRef );
	saver.Add( "DescriptionFileRef", &szDescriptionFileRef );

	return 0;
}

int SMultiplayerTechLevel::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szNameFileRef );
	saver.Add( 3, &szDescriptionFileRef );

	return 0;
}

DWORD SMultiplayerTechLevel::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void STechLevelReinfSet::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "Reinforcements", &reinforcements, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "StartingUnits", (BYTE*)&pStartingUnits - pThis, sizeof(pStartingUnits), NTypeDef::TYPE_TYPE_REF );
}

int STechLevelReinfSet::operator&( IXmlSaver &saver )
{
	saver.Add( "Reinforcements", &reinforcements );
	saver.Add( "StartingUnits", &pStartingUnits );

	return 0;
}

int STechLevelReinfSet::operator&( IBinSaver &saver )
{
	saver.Add( 2, &reinforcements );
	saver.Add( 3, &pStartingUnits );

	return 0;
}

DWORD STechLevelReinfSet::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << reinforcements << pStartingUnits;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SLadderRank::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Level", (BYTE*)&nLevel - pThis, sizeof(nLevel), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "NameFileRef", (BYTE*)&szNameFileRef - pThis, sizeof(szNameFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "Texture", (BYTE*)&pTexture - pThis, sizeof(pTexture), NTypeDef::TYPE_TYPE_REF );
}

int SLadderRank::operator&( IXmlSaver &saver )
{
	saver.Add( "Level", &nLevel );
	saver.Add( "NameFileRef", &szNameFileRef );
	saver.Add( "Texture", &pTexture );

	return 0;
}

int SLadderRank::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nLevel );
	saver.Add( 3, &szNameFileRef );
	saver.Add( 4, &pTexture );

	return 0;
}

DWORD SLadderRank::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nLevel;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMultiplayerSide::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "NameFileRef", (BYTE*)&szNameFileRef - pThis, sizeof(szNameFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "PartyInfo", (BYTE*)&pPartyInfo - pThis, sizeof(pPartyInfo), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "ListItemIcon", (BYTE*)&pListItemIcon - pThis, sizeof(pListItemIcon), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "HistoricalSide", (BYTE*)&eHistoricalSide - pThis, sizeof(eHistoricalSide), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "TechLevels", &techLevels, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "LadderRanks", &ladderRanks, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "Medals", &medals, pThis );
}

int SMultiplayerSide::operator&( IXmlSaver &saver )
{
	saver.Add( "NameFileRef", &szNameFileRef );
	saver.Add( "PartyInfo", &pPartyInfo );
	saver.Add( "ListItemIcon", &pListItemIcon );
	saver.Add( "HistoricalSide", &eHistoricalSide );
	saver.Add( "TechLevels", &techLevels );
	saver.Add( "LadderRanks", &ladderRanks );
	saver.Add( "Medals", &medals );

	return 0;
}

int SMultiplayerSide::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szNameFileRef );
	saver.Add( 3, &pPartyInfo );
	saver.Add( 4, &pListItemIcon );
	saver.Add( 5, &eHistoricalSide );
	saver.Add( 6, &techLevels );
	saver.Add( 7, &ladderRanks );
	saver.Add( 8, &medals );

	return 0;
}

DWORD SMultiplayerSide::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << pPartyInfo << eHistoricalSide << techLevels << ladderRanks << medals;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMultiplayerConsts::SPlayerColor::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Color", (BYTE*)&nColor - pThis, sizeof(nColor), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "UnitFullInfo", (BYTE*)&pUnitFullInfo - pThis, sizeof(pUnitFullInfo), NTypeDef::TYPE_TYPE_REF );
}

int SMultiplayerConsts::SPlayerColor::operator&( IXmlSaver &saver )
{
	saver.Add( "Color", &nColor );
	saver.Add( "UnitFullInfo", &pUnitFullInfo );

	return 0;
}

int SMultiplayerConsts::SPlayerColor::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nColor );
	saver.Add( 3, &pUnitFullInfo );

	return 0;
}

DWORD SMultiplayerConsts::SPlayerColor::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nColor << pUnitFullInfo;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMultiplayerConsts::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "MultiplayerConsts", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "TechLevels", &techLevels, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "Sides", &sides, pThis );
	NMetaInfo::ReportMetaInfo( "RandomCountryIcon", (BYTE*)&pRandomCountryIcon - pThis, sizeof(pRandomCountryIcon), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportSimpleArrayMetaInfo( "DiplomacyInfo", &diplomacyInfo, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "expLevels", &expLevels, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "PlayerColorInfos", &playerColorInfos, pThis );
	NMetaInfo::ReportStructMetaInfo( "ReinfCounterRecycle", &vReinfCounterRecycle, pThis ); 
	NMetaInfo::ReportMetaInfo( "TimeUserMPPause", (BYTE*)&nTimeUserMPPause - pThis, sizeof(nTimeUserMPPause), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "TimeUserMPLag", (BYTE*)&nTimeUserMPLag - pThis, sizeof(nTimeUserMPLag), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::FinishMetaInfoReport();
}

int SMultiplayerConsts::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "TechLevels", &techLevels );
	saver.Add( "Sides", &sides );
	saver.Add( "RandomCountryIcon", &pRandomCountryIcon );
	saver.Add( "DiplomacyInfo", &diplomacyInfo );
	saver.Add( "expLevels", &expLevels );
	saver.Add( "PlayerColorInfos", &playerColorInfos );
	saver.Add( "ReinfCounterRecycle", &vReinfCounterRecycle );
	saver.Add( "TimeUserMPPause", &nTimeUserMPPause );
	saver.Add( "TimeUserMPLag", &nTimeUserMPLag );

	return 0;
}

int SMultiplayerConsts::operator&( IBinSaver &saver )
{
	saver.Add( 2, &techLevels );
	saver.Add( 3, &sides );
	saver.Add( 4, &pRandomCountryIcon );
	saver.Add( 5, &diplomacyInfo );
	saver.Add( 6, &expLevels );
	saver.Add( 7, &playerColorInfos );
	saver.Add( 8, &vReinfCounterRecycle );
	saver.Add( 9, &nTimeUserMPPause );
	saver.Add( 10, &nTimeUserMPLag );

	return 0;
}

DWORD SMultiplayerConsts::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << techLevels << sides << diplomacyInfo << expLevels << playerColorInfos << vReinfCounterRecycle << nTimeUserMPPause << nTimeUserMPLag;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( 0x191B2300, SMultiplayerConsts ) 
