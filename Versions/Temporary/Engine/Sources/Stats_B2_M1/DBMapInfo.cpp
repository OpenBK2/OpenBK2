// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "DBMapInfo.h"

#include "Stats_B2_M1_export.h"

#include <cstdint>

namespace NDb
{


std::string EnumToString( NDb::EMPGameType eValue )
{
	switch ( eValue )
	{
	case NDb::MP_GT_STANDARD:
		return "MP_GT_STANDARD";
	case NDb::MP_GT_COUNT:
		return "MP_GT_COUNT";
	default:
		return "MP_GT_STANDARD";
	}
}

NDb::EMPGameType NDb::StringToEnum_NDb_EMPGameType( const std::string &szValue )
{
	if ( szValue == "MP_GT_STANDARD" )
		return NDb::MP_GT_STANDARD;
	if ( szValue == "MP_GT_COUNT" )
		return NDb::MP_GT_COUNT;
	return NDb::MP_GT_STANDARD;
}


void SMPMapInfo::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "GameTypes", &gameTypes, pThis );
}

int SMPMapInfo::operator&( IXmlSaver &saver )
{
	saver.Add( "GameTypes", &gameTypes );

	return 0;
}

int SMPMapInfo::operator&( IBinSaver &saver )
{
	saver.Add( 2, &gameTypes );

	return 0;
}

uint32_t SMPMapInfo::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << gameTypes;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SCameraPlacement::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Anchor", &vAnchor, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "Yaw", (uint8_t*)&fYaw - pThis, sizeof(fYaw), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Pitch", (uint8_t*)&fPitch - pThis, sizeof(fPitch), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Dist", (uint8_t*)&fDist - pThis, sizeof(fDist), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "UseAnchorOnly", (uint8_t*)&bUseAnchorOnly - pThis, sizeof(bUseAnchorOnly), NTypeDef::TYPE_TYPE_BOOL );
}

int SCameraPlacement::operator&( IXmlSaver &saver )
{
	saver.Add( "Anchor", &vAnchor );
	saver.Add( "Yaw", &fYaw );
	saver.Add( "Pitch", &fPitch );
	saver.Add( "Dist", &fDist );
	saver.Add( "UseAnchorOnly", &bUseAnchorOnly );

	return 0;
}

int SCameraPlacement::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vAnchor );
	saver.Add( 3, &fYaw );
	saver.Add( 4, &fPitch );
	saver.Add( 5, &fDist );
	saver.Add( 6, &bUseAnchorOnly );

	return 0;
}

uint32_t SCameraPlacement::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << vAnchor << fYaw << fPitch << fDist << bUseAnchorOnly;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SScriptCameraPlacement::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Name", (uint8_t*)&szName - pThis, sizeof(szName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Position", &vPosition, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "Yaw", (uint8_t*)&fYaw - pThis, sizeof(fYaw), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Pitch", (uint8_t*)&fPitch - pThis, sizeof(fPitch), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "FOV", (uint8_t*)&fFOV - pThis, sizeof(fFOV), NTypeDef::TYPE_TYPE_FLOAT );
}

int SScriptCameraPlacement::operator&( IXmlSaver &saver )
{
	saver.Add( "Name", &szName );
	saver.Add( "Position", &vPosition );
	saver.Add( "Yaw", &fYaw );
	saver.Add( "Pitch", &fPitch );
	saver.Add( "FOV", &fFOV );

	return 0;
}

int SScriptCameraPlacement::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szName );
	saver.Add( 3, &vPosition );
	saver.Add( 4, &fYaw );
	saver.Add( 5, &fPitch );
	saver.Add( 6, &fFOV );

	return 0;
}

uint32_t SScriptCameraPlacement::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szName << vPosition << fYaw << fPitch << fFOV;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SScriptMovieKey::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "IsTangentIn", (uint8_t*)&bIsTangentIn - pThis, sizeof(bIsTangentIn), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "IsTangentOut", (uint8_t*)&bIsTangentOut - pThis, sizeof(bIsTangentOut), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "KeyParam", (uint8_t*)&szKeyParam - pThis, sizeof(szKeyParam), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "StartTime", (uint8_t*)&fStartTime - pThis, sizeof(fStartTime), NTypeDef::TYPE_TYPE_FLOAT );
}

int SScriptMovieKey::operator&( IXmlSaver &saver )
{
	saver.Add( "IsTangentIn", &bIsTangentIn );
	saver.Add( "IsTangentOut", &bIsTangentOut );
	saver.Add( "KeyParam", &szKeyParam );
	saver.Add( "StartTime", &fStartTime );

	return 0;
}

int SScriptMovieKey::operator&( IBinSaver &saver )
{
	saver.Add( 2, &bIsTangentIn );
	saver.Add( 3, &bIsTangentOut );
	saver.Add( 4, &szKeyParam );
	saver.Add( 5, &fStartTime );

	return 0;
}

uint32_t SScriptMovieKey::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << bIsTangentIn << bIsTangentOut << szKeyParam << fStartTime;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SScriptMovieKeyPos::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	SScriptMovieKey::ReportMetaInfo( szAddName, pThis );

	NMetaInfo::ReportMetaInfo( szAddName + "PositionIndex", (uint8_t*)&nPositionIndex - pThis, sizeof(nPositionIndex), NTypeDef::TYPE_TYPE_INT );
}

int SScriptMovieKeyPos::operator&( IXmlSaver &saver )
{
	saver.AddTypedSuper( (SScriptMovieKey*)(this) );
	saver.Add( "PositionIndex", &nPositionIndex );

	return 0;
}

int SScriptMovieKeyPos::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SScriptMovieKey*)this );
	saver.Add( 2, &nPositionIndex );

	return 0;
}

uint32_t SScriptMovieKeyPos::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SScriptMovieKey::CalcCheckSum() << nPositionIndex;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SScriptMovieKeyFollow::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	SScriptMovieKey::ReportMetaInfo( szAddName, pThis );

	NMetaInfo::ReportMetaInfo( szAddName + "ObjectScriptID", (uint8_t*)&nObjectScriptID - pThis, sizeof(nObjectScriptID), NTypeDef::TYPE_TYPE_INT );
}

int SScriptMovieKeyFollow::operator&( IXmlSaver &saver )
{
	saver.AddTypedSuper( (SScriptMovieKey*)(this) );
	saver.Add( "ObjectScriptID", &nObjectScriptID );

	return 0;
}

int SScriptMovieKeyFollow::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SScriptMovieKey*)this );
	saver.Add( 2, &nObjectScriptID );

	return 0;
}

uint32_t SScriptMovieKeyFollow::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SScriptMovieKey::CalcCheckSum() << nObjectScriptID;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SScriptMovieSequence::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "posKeys", &posKeys, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "followKeys", &followKeys, pThis );
}

int SScriptMovieSequence::operator&( IXmlSaver &saver )
{
	saver.Add( "posKeys", &posKeys );
	saver.Add( "followKeys", &followKeys );

	return 0;
}

int SScriptMovieSequence::operator&( IBinSaver &saver )
{
	saver.Add( 2, &posKeys );
	saver.Add( 3, &followKeys );

	return 0;
}

uint32_t SScriptMovieSequence::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << posKeys << followKeys;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SScriptMovies::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "ScriptCameraPlacements", &scriptCameraPlacements, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "ScriptMovieSequences", &scriptMovieSequences, pThis );
}

int SScriptMovies::operator&( IXmlSaver &saver )
{
	saver.Add( "ScriptCameraPlacements", &scriptCameraPlacements );
	saver.Add( "ScriptMovieSequences", &scriptMovieSequences );

	return 0;
}

int SScriptMovies::operator&( IBinSaver &saver )
{
	saver.Add( 2, &scriptCameraPlacements );
	saver.Add( 3, &scriptMovieSequences );

	return 0;
}

uint32_t SScriptMovies::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << scriptCameraPlacements << scriptMovieSequences;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SPartyDependentInfo::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "PartyDependentInfo", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "GeneralPartyName", (uint8_t*)&szGeneralPartyName - pThis, sizeof(szGeneralPartyName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "GunCrewSquad", (uint8_t*)&pGunCrewSquad - pThis, sizeof(pGunCrewSquad), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "HowitzerGunCrewSquad", (uint8_t*)&pHowitzerGunCrewSquad - pThis, sizeof(pHowitzerGunCrewSquad), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "HeavyMachinegunSquad", (uint8_t*)&pHeavyMachinegunSquad - pThis, sizeof(pHeavyMachinegunSquad), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "AAGunSquad", (uint8_t*)&pAAGunSquad - pThis, sizeof(pAAGunSquad), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "ResupplyEngineerSquad", (uint8_t*)&pResupplyEngineerSquad - pThis, sizeof(pResupplyEngineerSquad), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "LocalizedNameFileRef", (uint8_t*)&szLocalizedNameFileRef - pThis, sizeof(szLocalizedNameFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "MinimapKeyObjectIcon", (uint8_t*)&pMinimapKeyObjectIcon - pThis, sizeof(pMinimapKeyObjectIcon), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "MinimapKeyObjectIconSelected", (uint8_t*)&pMinimapKeyObjectIconSelected - pThis, sizeof(pMinimapKeyObjectIconSelected), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "StatisticsIcon", (uint8_t*)&pStatisticsIcon - pThis, sizeof(pStatisticsIcon), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "ParatrooperVisObj", (uint8_t*)&pParatrooperVisObj - pThis, sizeof(pParatrooperVisObj), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "ListItemIcon", (uint8_t*)&pListItemIcon - pThis, sizeof(pListItemIcon), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "KeyBuildingFlag", (uint8_t*)&pKeyBuildingFlag - pThis, sizeof(pKeyBuildingFlag), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SPartyDependentInfo::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "GeneralPartyName", &szGeneralPartyName );
	saver.Add( "GunCrewSquad", &pGunCrewSquad );
	saver.Add( "HowitzerGunCrewSquad", &pHowitzerGunCrewSquad );
	saver.Add( "HeavyMachinegunSquad", &pHeavyMachinegunSquad );
	saver.Add( "AAGunSquad", &pAAGunSquad );
	saver.Add( "ResupplyEngineerSquad", &pResupplyEngineerSquad );
	saver.Add( "LocalizedNameFileRef", &szLocalizedNameFileRef );
	saver.Add( "MinimapKeyObjectIcon", &pMinimapKeyObjectIcon );
	saver.Add( "MinimapKeyObjectIconSelected", &pMinimapKeyObjectIconSelected );
	saver.Add( "StatisticsIcon", &pStatisticsIcon );
	saver.Add( "ParatrooperVisObj", &pParatrooperVisObj );
	saver.Add( "ListItemIcon", &pListItemIcon );
	saver.Add( "KeyBuildingFlag", &pKeyBuildingFlag );

	return 0;
}

int SPartyDependentInfo::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szGeneralPartyName );
	saver.Add( 3, &pGunCrewSquad );
	saver.Add( 4, &pHowitzerGunCrewSquad );
	saver.Add( 5, &pHeavyMachinegunSquad );
	saver.Add( 6, &pAAGunSquad );
	saver.Add( 7, &pResupplyEngineerSquad );
	saver.Add( 8, &szLocalizedNameFileRef );
	saver.Add( 9, &pMinimapKeyObjectIcon );
	saver.Add( 10, &pMinimapKeyObjectIconSelected );
	saver.Add( 11, &pStatisticsIcon );
	saver.Add( 12, &pParatrooperVisObj );
	saver.Add( 13, &pListItemIcon );
	saver.Add( 14, &pKeyBuildingFlag );

	return 0;
}

uint32_t SPartyDependentInfo::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szGeneralPartyName << pGunCrewSquad << pHowitzerGunCrewSquad << pHeavyMachinegunSquad << pAAGunSquad << pResupplyEngineerSquad << pKeyBuildingFlag;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMissionObjective::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "MissionObjective", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "HeaderFileRef", (uint8_t*)&szHeaderFileRef - pThis, sizeof(szHeaderFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "BriefingFileRef", (uint8_t*)&szBriefingFileRef - pThis, sizeof(szBriefingFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "DescriptionFileRef", (uint8_t*)&szDescriptionFileRef - pThis, sizeof(szDescriptionFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "IsPrimary", (uint8_t*)&bIsPrimary - pThis, sizeof(bIsPrimary), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportStructArrayMetaInfo( "MapPositions", &mapPositions, pThis );
	NMetaInfo::ReportMetaInfo( "Experience", (uint8_t*)&nExperience - pThis, sizeof(nExperience), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::FinishMetaInfoReport();
}

int SMissionObjective::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "HeaderFileRef", &szHeaderFileRef );
	saver.Add( "BriefingFileRef", &szBriefingFileRef );
	saver.Add( "DescriptionFileRef", &szDescriptionFileRef );
	saver.Add( "IsPrimary", &bIsPrimary );
	saver.Add( "MapPositions", &mapPositions );
	saver.Add( "Experience", &nExperience );

	return 0;
}

int SMissionObjective::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szHeaderFileRef );
	saver.Add( 3, &szBriefingFileRef );
	saver.Add( 4, &szDescriptionFileRef );
	saver.Add( 5, &bIsPrimary );
	saver.Add( 6, &mapPositions );
	saver.Add( 7, &nExperience );

	return 0;
}



void SMapObjectInfo::SLinkInfo::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "LinkID", (uint8_t*)&nLinkID - pThis, sizeof(nLinkID), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "LinkWith", (uint8_t*)&nLinkWith - pThis, sizeof(nLinkWith), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "Intention", (uint8_t*)&bIntention - pThis, sizeof(bIntention), NTypeDef::TYPE_TYPE_BOOL );
}

int SMapObjectInfo::SLinkInfo::operator&( IXmlSaver &saver )
{
	saver.Add( "LinkID", &nLinkID );
	saver.Add( "LinkWith", &nLinkWith );
	saver.Add( "Intention", &bIntention );

	return 0;
}

int SMapObjectInfo::SLinkInfo::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nLinkID );
	saver.Add( 3, &nLinkWith );
	saver.Add( 4, &bIntention );

	return 0;
}

uint32_t SMapObjectInfo::SLinkInfo::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nLinkID << nLinkWith << bIntention;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMapObjectInfo::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Pos", &vPos, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "Dir", (uint8_t*)&nDir - pThis, sizeof(nDir), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "Player", (uint8_t*)&nPlayer - pThis, sizeof(nPlayer), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "ScriptID", (uint8_t*)&nScriptID - pThis, sizeof(nScriptID), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "HP", (uint8_t*)&fHP - pThis, sizeof(fHP), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "FrameIndex", (uint8_t*)&nFrameIndex - pThis, sizeof(nFrameIndex), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Link", &link, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "Object", (uint8_t*)&pObject - pThis, sizeof(pObject), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "ConstructorProfile", (uint8_t*)&pConstructorProfile - pThis, sizeof(pConstructorProfile), NTypeDef::TYPE_TYPE_REF );
}

int SMapObjectInfo::operator&( IXmlSaver &saver )
{
	saver.Add( "Pos", &vPos );
	saver.Add( "Dir", &nDir );
	saver.Add( "Player", &nPlayer );
	saver.Add( "ScriptID", &nScriptID );
	saver.Add( "HP", &fHP );
	saver.Add( "FrameIndex", &nFrameIndex );
	saver.Add( "Link", &link );
	saver.Add( "Object", &pObject );
	saver.Add( "ConstructorProfile", &pConstructorProfile );

	return 0;
}

int SMapObjectInfo::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vPos );
	saver.Add( 3, &nDir );
	saver.Add( 4, &nPlayer );
	saver.Add( 5, &nScriptID );
	saver.Add( 6, &fHP );
	saver.Add( 7, &nFrameIndex );
	saver.Add( 8, &link );
	saver.Add( 9, &pObject );
	saver.Add( 10, &pConstructorProfile );

	return 0;
}

uint32_t SMapObjectInfo::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << vPos << nDir << nPlayer << nScriptID << fHP << nFrameIndex << link << pObject << pConstructorProfile;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SEntrenchmentInfo::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "sections", &sections, pThis );
}

int SEntrenchmentInfo::operator&( IXmlSaver &saver )
{
	saver.Add( "sections", &sections );

	return 0;
}

int SEntrenchmentInfo::operator&( IBinSaver &saver )
{
	saver.Add( 2, &sections );

	return 0;
}

uint32_t SEntrenchmentInfo::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << sections;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}


std::string EnumToString( NDb::EScriptAreaTypes eValue )
{
	switch ( eValue )
	{
	case NDb::EAT_RECTANGLE:
		return "EAT_RECTANGLE";
	case NDb::EAT_CIRCLE:
		return "EAT_CIRCLE";
	default:
		return "EAT_RECTANGLE";
	}
}

NDb::EScriptAreaTypes NDb::StringToEnum_NDb_EScriptAreaTypes( const std::string &szValue )
{
	if ( szValue == "EAT_RECTANGLE" )
		return NDb::EAT_RECTANGLE;
	if ( szValue == "EAT_CIRCLE" )
		return NDb::EAT_CIRCLE;
	return NDb::EAT_RECTANGLE;
}


void SScriptArea::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Type", (uint8_t*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "Name", (uint8_t*)&szName - pThis, sizeof(szName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Center", &vCenter, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "AABBHalfSize", &vAABBHalfSize, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "R", (uint8_t*)&fR - pThis, sizeof(fR), NTypeDef::TYPE_TYPE_FLOAT );
}

int SScriptArea::operator&( IXmlSaver &saver )
{
	saver.Add( "Type", &eType );
	saver.Add( "Name", &szName );
	saver.Add( "Center", &vCenter );
	saver.Add( "AABBHalfSize", &vAABBHalfSize );
	saver.Add( "R", &fR );

	return 0;
}

int SScriptArea::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eType );
	saver.Add( 3, &szName );
	saver.Add( 4, &vCenter );
	saver.Add( 5, &vAABBHalfSize );
	saver.Add( 6, &fR );

	return 0;
}

uint32_t SScriptArea::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eType << szName << vCenter << vAABBHalfSize << fR;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SAIStartCommand::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "CmdType", (uint8_t*)&nCmdType - pThis, sizeof(nCmdType), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "unitLinkIDs", &unitLinkIDs, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "LinkID", (uint8_t*)&nLinkID - pThis, sizeof(nLinkID), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Pos", &vPos, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "FromExplosion", (uint8_t*)&bFromExplosion - pThis, sizeof(bFromExplosion), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "Number", (uint8_t*)&fNumber - pThis, sizeof(fNumber), NTypeDef::TYPE_TYPE_FLOAT );
}

int SAIStartCommand::operator&( IXmlSaver &saver )
{
	saver.Add( "CmdType", &nCmdType );
	saver.Add( "unitLinkIDs", &unitLinkIDs );
	saver.Add( "LinkID", &nLinkID );
	saver.Add( "Pos", &vPos );
	saver.Add( "FromExplosion", &bFromExplosion );
	saver.Add( "Number", &fNumber );

	return 0;
}

int SAIStartCommand::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nCmdType );
	saver.Add( 3, &unitLinkIDs );
	saver.Add( 4, &nLinkID );
	saver.Add( 5, &vPos );
	saver.Add( 6, &bFromExplosion );
	saver.Add( 7, &fNumber );

	return 0;
}

uint32_t SAIStartCommand::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nCmdType << unitLinkIDs << nLinkID << vPos << bFromExplosion << fNumber;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SBattlePosition::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "ArtilleryLinkID", (uint8_t*)&nArtilleryLinkID - pThis, sizeof(nArtilleryLinkID), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "TruckLinkID", (uint8_t*)&nTruckLinkID - pThis, sizeof(nTruckLinkID), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Pos", &vPos, pThis ); 
}

int SBattlePosition::operator&( IXmlSaver &saver )
{
	saver.Add( "ArtilleryLinkID", &nArtilleryLinkID );
	saver.Add( "TruckLinkID", &nTruckLinkID );
	saver.Add( "Pos", &vPos );

	return 0;
}

int SBattlePosition::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nArtilleryLinkID );
	saver.Add( 3, &nTruckLinkID );
	saver.Add( 4, &vPos );

	return 0;
}

uint32_t SBattlePosition::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nArtilleryLinkID << nTruckLinkID << vPos;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMapSoundInfo::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "sound", (uint8_t*)&psound - pThis, sizeof(psound), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Pos", &vPos, pThis ); 
}

int SMapSoundInfo::operator&( IXmlSaver &saver )
{
	saver.Add( "sound", &psound );
	saver.Add( "Pos", &vPos );

	return 0;
}

int SMapSoundInfo::operator&( IBinSaver &saver )
{
	saver.Add( 2, &psound );
	saver.Add( 3, &vPos );

	return 0;
}

uint32_t SMapSoundInfo::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << vPos;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SEditAreaInfo::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Name", (uint8_t*)&szName - pThis, sizeof(szName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "Points", &points, pThis );
}

int SEditAreaInfo::operator&( IXmlSaver &saver )
{
	saver.Add( "Name", &szName );
	saver.Add( "Points", &points );

	return 0;
}

int SEditAreaInfo::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szName );
	saver.Add( 3, &points );

	return 0;
}

uint32_t SEditAreaInfo::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szName << points;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}


std::string EnumToString( NDb::EParcelType eValue )
{
	switch ( eValue )
	{
	case NDb::EPATCH_UNKNOWN:
		return "EPATCH_UNKNOWN";
	case NDb::EPATCH_DEFENCE:
		return "EPATCH_DEFENCE";
	case NDb::EPATCH_REINFORCE:
		return "EPATCH_REINFORCE";
	default:
		return "EPATCH_UNKNOWN";
	}
}

NDb::EParcelType NDb::StringToEnum_NDb_EParcelType( const std::string &szValue )
{
	if ( szValue == "EPATCH_UNKNOWN" )
		return NDb::EPATCH_UNKNOWN;
	if ( szValue == "EPATCH_DEFENCE" )
		return NDb::EPATCH_DEFENCE;
	if ( szValue == "EPATCH_REINFORCE" )
		return NDb::EPATCH_REINFORCE;
	return NDb::EPATCH_UNKNOWN;
}


void SReinforcePoint::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Center", &vCenter, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "Direction", (uint8_t*)&fDirection - pThis, sizeof(fDirection), NTypeDef::TYPE_TYPE_FLOAT );
}

int SReinforcePoint::operator&( IXmlSaver &saver )
{
	saver.Add( "Center", &vCenter );
	saver.Add( "Direction", &fDirection );

	return 0;
}

int SReinforcePoint::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vCenter );
	saver.Add( 3, &fDirection );

	return 0;
}

uint32_t SReinforcePoint::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << vCenter << fDirection;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SAIGeneralParcel::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "reinforcePoints", &reinforcePoints, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "Type", (uint8_t*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Center", &vCenter, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "Radius", (uint8_t*)&fRadius - pThis, sizeof(fRadius), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Importance", (uint8_t*)&fImportance - pThis, sizeof(fImportance), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "DefenceDirection", (uint8_t*)&fDefenceDirection - pThis, sizeof(fDefenceDirection), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "MinUnitsToReinforce", (uint8_t*)&nMinUnitsToReinforce - pThis, sizeof(nMinUnitsToReinforce), NTypeDef::TYPE_TYPE_INT );
}

int SAIGeneralParcel::operator&( IXmlSaver &saver )
{
	saver.Add( "reinforcePoints", &reinforcePoints );
	saver.Add( "Type", &eType );
	saver.Add( "Center", &vCenter );
	saver.Add( "Radius", &fRadius );
	saver.Add( "Importance", &fImportance );
	saver.Add( "DefenceDirection", &fDefenceDirection );
	saver.Add( "MinUnitsToReinforce", &nMinUnitsToReinforce );

	return 0;
}

int SAIGeneralParcel::operator&( IBinSaver &saver )
{
	saver.Add( 2, &reinforcePoints );
	saver.Add( 3, &eType );
	saver.Add( 4, &vCenter );
	saver.Add( 5, &fRadius );
	saver.Add( 6, &fImportance );
	saver.Add( 7, &fDefenceDirection );
	saver.Add( 8, &nMinUnitsToReinforce );

	return 0;
}

uint32_t SAIGeneralParcel::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << reinforcePoints << eType << vCenter << fRadius << fImportance << fDefenceDirection << nMinUnitsToReinforce;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SAIGeneralSide::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "mobileScriptIDs", &mobileScriptIDs, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "parcels", &parcels, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "MaxMobileTanks", (uint8_t*)&nMaxMobileTanks - pThis, sizeof(nMaxMobileTanks), NTypeDef::TYPE_TYPE_INT );
}

int SAIGeneralSide::operator&( IXmlSaver &saver )
{
	saver.Add( "mobileScriptIDs", &mobileScriptIDs );
	saver.Add( "parcels", &parcels );
	saver.Add( "MaxMobileTanks", &nMaxMobileTanks );

	return 0;
}

int SAIGeneralSide::operator&( IBinSaver &saver )
{
	saver.Add( 2, &mobileScriptIDs );
	saver.Add( 3, &parcels );
	saver.Add( 4, &nMaxMobileTanks );

	return 0;
}

uint32_t SAIGeneralSide::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << mobileScriptIDs << parcels << nMaxMobileTanks;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SBonusInstance::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "LinkID", (uint8_t*)&nLinkID - pThis, sizeof(nLinkID), NTypeDef::TYPE_TYPE_INT );
}

int SBonusInstance::operator&( IXmlSaver &saver )
{
	saver.Add( "LinkID", &nLinkID );

	return 0;
}

int SBonusInstance::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nLinkID );

	return 0;
}

uint32_t SBonusInstance::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nLinkID;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SBuildingBonuses::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "PointID", (uint8_t*)&nPointID - pThis, sizeof(nPointID), NTypeDef::TYPE_TYPE_INT );
}

int SBuildingBonuses::operator&( IXmlSaver &saver )
{
	saver.Add( "PointID", &nPointID );

	return 0;
}

int SBuildingBonuses::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nPointID );

	return 0;
}

uint32_t SBuildingBonuses::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nPointID;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SPlayerBonusData::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "LinkID", (uint8_t*)&nLinkID - pThis, sizeof(nLinkID), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "PlayerBonuses", &playerBonuses, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "Storage", (uint8_t*)&bStorage - pThis, sizeof(bStorage), NTypeDef::TYPE_TYPE_BOOL );
}

int SPlayerBonusData::operator&( IXmlSaver &saver )
{
	saver.Add( "LinkID", &nLinkID );
	saver.Add( "PlayerBonuses", &playerBonuses );
	saver.Add( "Storage", &bStorage );

	return 0;
}

int SPlayerBonusData::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nLinkID );
	saver.Add( 3, &playerBonuses );
	saver.Add( 4, &bStorage );

	return 0;
}

uint32_t SPlayerBonusData::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nLinkID << playerBonuses << bStorage;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}


std::string EnumToString( NDb::ESuperWeaponType eValue )
{
	switch ( eValue )
	{
	case NDb::SUPER_WEAPON_BOMBER:
		return "SUPER_WEAPON_BOMBER";
	case NDb::SUPER_WEAPON_ROCKET:
		return "SUPER_WEAPON_ROCKET";
	case NDb::SUPER_WEAPON_ARTILLERY:
		return "SUPER_WEAPON_ARTILLERY";
	default:
		return "SUPER_WEAPON_BOMBER";
	}
}

NDb::ESuperWeaponType NDb::StringToEnum_NDb_ESuperWeaponType( const std::string &szValue )
{
	if ( szValue == "SUPER_WEAPON_BOMBER" )
		return NDb::SUPER_WEAPON_BOMBER;
	if ( szValue == "SUPER_WEAPON_ROCKET" )
		return NDb::SUPER_WEAPON_ROCKET;
	if ( szValue == "SUPER_WEAPON_ARTILLERY" )
		return NDb::SUPER_WEAPON_ARTILLERY;
	return NDb::SUPER_WEAPON_BOMBER;
}


void SMapPlayerInfo::SDeployPosition::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Position", &vPosition, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "Direction", (uint8_t*)&nDirection - pThis, sizeof(nDirection), NTypeDef::TYPE_TYPE_INT );
}

int SMapPlayerInfo::SDeployPosition::operator&( IXmlSaver &saver )
{
	saver.Add( "Position", &vPosition );
	saver.Add( "Direction", &nDirection );

	return 0;
}

int SMapPlayerInfo::SDeployPosition::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vPosition );
	saver.Add( 3, &nDirection );

	return 0;
}

uint32_t SMapPlayerInfo::SDeployPosition::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << vPosition << nDirection;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMapPlayerInfo::SSuperWeaponInfo::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "SuperWeaponType", (uint8_t*)&eSuperWeaponType - pThis, sizeof(eSuperWeaponType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "Count", (uint8_t*)&nCount - pThis, sizeof(nCount), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "RecycleTime", (uint8_t*)&fRecycleTime - pThis, sizeof(fRecycleTime), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "FlyTime", (uint8_t*)&fFlyTime - pThis, sizeof(fFlyTime), NTypeDef::TYPE_TYPE_FLOAT );
}

int SMapPlayerInfo::SSuperWeaponInfo::operator&( IXmlSaver &saver )
{
	saver.Add( "SuperWeaponType", &eSuperWeaponType );
	saver.Add( "Count", &nCount );
	saver.Add( "RecycleTime", &fRecycleTime );
	saver.Add( "FlyTime", &fFlyTime );

	return 0;
}

int SMapPlayerInfo::SSuperWeaponInfo::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eSuperWeaponType );
	saver.Add( 3, &nCount );
	saver.Add( 4, &fRecycleTime );
	saver.Add( 5, &fFlyTime );

	return 0;
}

uint32_t SMapPlayerInfo::SSuperWeaponInfo::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eSuperWeaponType << nCount << fRecycleTime << fFlyTime;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMapPlayerInfo::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Camera", &camera, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "general", &general, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "PartyInfo", (uint8_t*)&pPartyInfo - pThis, sizeof(pPartyInfo), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "ReinforcementPoints", &reinforcementPoints, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "ReinforcementTypes", &reinforcementTypes, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "DefaultRank", (uint8_t*)&pDefaultRank - pThis, sizeof(pDefaultRank), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "DiplomacySide", (uint8_t*)&nDiplomacySide - pThis, sizeof(nDiplomacySide), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "RecycleTimeCoefficient", (uint8_t*)&fRecycleTimeCoefficient - pThis, sizeof(fRecycleTimeCoefficient), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "ReinforcementCalls", (uint8_t*)&nReinforcementCalls - pThis, sizeof(nReinforcementCalls), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "LocalizedPlayerNameFileRef", (uint8_t*)&szLocalizedPlayerNameFileRef - pThis, sizeof(szLocalizedPlayerNameFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructMetaInfo( szAddName + "MPStartPos", &vMPStartPos, pThis ); 
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "ScriptReinforcements", &scriptReinforcements, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "ScriptReinforcementsTextID", &scriptReinforcementsTextID, pThis );
	NMetaInfo::ReportStructMetaInfo( szAddName + "SuperWeapon", &superWeapon, pThis ); 
}

int SMapPlayerInfo::operator&( IXmlSaver &saver )
{
	saver.Add( "Camera", &camera );
	saver.Add( "general", &general );
	saver.Add( "PartyInfo", &pPartyInfo );
	saver.Add( "ReinforcementPoints", &reinforcementPoints );
	saver.Add( "ReinforcementTypes", &reinforcementTypes );
	saver.Add( "DefaultRank", &pDefaultRank );
	saver.Add( "DiplomacySide", &nDiplomacySide );
	saver.Add( "RecycleTimeCoefficient", &fRecycleTimeCoefficient );
	saver.Add( "ReinforcementCalls", &nReinforcementCalls );
	saver.Add( "LocalizedPlayerNameFileRef", &szLocalizedPlayerNameFileRef );
	saver.Add( "MPStartPos", &vMPStartPos );
	saver.Add( "ScriptReinforcements", &scriptReinforcements );
	saver.Add( "ScriptReinforcementsTextID", &scriptReinforcementsTextID );
	saver.Add( "SuperWeapon", &superWeapon );

	return 0;
}

int SMapPlayerInfo::operator&( IBinSaver &saver )
{
	saver.Add( 2, &camera );
	saver.Add( 3, &general );
	saver.Add( 4, &pPartyInfo );
	saver.Add( 5, &reinforcementPoints );
	saver.Add( 6, &reinforcementTypes );
	saver.Add( 7, &pDefaultRank );
	saver.Add( 8, &nDiplomacySide );
	saver.Add( 9, &fRecycleTimeCoefficient );
	saver.Add( 10, &nReinforcementCalls );
	saver.Add( 11, &szLocalizedPlayerNameFileRef );
	saver.Add( 12, &vMPStartPos );
	saver.Add( 13, &scriptReinforcements );
	saver.Add( 14, &scriptReinforcementsTextID );
	saver.Add( 15, &superWeapon );

	return 0;
}

uint32_t SMapPlayerInfo::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << camera << general << pPartyInfo << reinforcementPoints << reinforcementTypes << pDefaultRank << nDiplomacySide << fRecycleTimeCoefficient << nReinforcementCalls << vMPStartPos << scriptReinforcements << scriptReinforcementsTextID << superWeapon;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMapInfo::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "MapInfo", typeID, sizeof(*this) );
	STerrain::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "MapDesignerFileRef", (uint8_t*)&szMapDesignerFileRef - pThis, sizeof(szMapDesignerFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructMetaInfo( "NorthPoint", &vNorthPoint, pThis ); 
	NMetaInfo::ReportMetaInfo( "NortType", (uint8_t*)&nNortType - pThis, sizeof(nNortType), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructArrayMetaInfo( "Players", &players, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "Objects", &objects, pThis );
	NMetaInfo::ReportMetaInfo( "Season", (uint8_t*)&eSeason - pThis, sizeof(eSeason), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "DayTime", (uint8_t*)&eDayTime - pThis, sizeof(eDayTime), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportSimpleArrayMetaInfo( "Diplomacies", &diplomacies, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "Entrenchments", &entrenchments, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "Bridges", &bridges, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "ScenarioObjects", &scenarioObjects, pThis );
	NMetaInfo::ReportStructMetaInfo( "Reinforcements", &reinforcements, pThis ); 
	NMetaInfo::ReportMetaInfo( "ScriptFileRef", (uint8_t*)&szScriptFileRef - pThis, sizeof(szScriptFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructArrayMetaInfo( "ScriptAreas", &scriptAreas, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "startCommandsList", &startCommandsList, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "reservePositionsList", &reservePositionsList, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "soundsList", &soundsList, pThis );
	NMetaInfo::ReportMetaInfo( "ForestCircleSound", (uint8_t*)&pForestCircleSound - pThis, sizeof(pForestCircleSound), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "ForestAmbientSounds", (uint8_t*)&pForestAmbientSounds - pThis, sizeof(pForestAmbientSounds), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "MapType", (uint8_t*)&nMapType - pThis, sizeof(nMapType), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "AttackingSide", (uint8_t*)&nAttackingSide - pThis, sizeof(nAttackingSide), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructArrayMetaInfo( "PlayerBonusObjects", &playerBonusObjects, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "Bonuses", &bonuses, pThis );
	NMetaInfo::ReportMetaInfo( "MiniMap", (uint8_t*)&pMiniMap - pThis, sizeof(pMiniMap), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "LocalizedNameFileRef", (uint8_t*)&szLocalizedNameFileRef - pThis, sizeof(szLocalizedNameFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "LocalizedDescriptionFileRef", (uint8_t*)&szLocalizedDescriptionFileRef - pThis, sizeof(szLocalizedDescriptionFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "LoadingDescriptionFileRef", (uint8_t*)&szLoadingDescriptionFileRef - pThis, sizeof(szLoadingDescriptionFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "LoadingPicture", (uint8_t*)&pLoadingPicture - pThis, sizeof(pLoadingPicture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructArrayMetaInfo( "CameraPositions", &cameraPositions, pThis );
	NMetaInfo::ReportStructMetaInfo( "ScriptMovies", &scriptMovies, pThis ); 
	NMetaInfo::ReportStructArrayMetaInfo( "FinalPositions", &finalPositions, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "Objectives", &objectives, pThis );
	NMetaInfo::ReportMetaInfo( "Music", (uint8_t*)&pMusic - pThis, sizeof(pMusic), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "MusicWin", (uint8_t*)&pMusicWin - pThis, sizeof(pMusicWin), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "MusicLost", (uint8_t*)&pMusicLost - pThis, sizeof(pMusicLost), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "MPInfo", &mPInfo, pThis ); 
	NMetaInfo::ReportMetaInfo( "BorderLockSize", (uint8_t*)&nBorderLockSize - pThis, sizeof(nBorderLockSize), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "BorderCameraSize", (uint8_t*)&nBorderCameraSize - pThis, sizeof(nBorderCameraSize), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportSimpleArrayMetaInfo( "ScriptEffects", &scriptEffects, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "CustomDifficultyLevels", &customDifficultyLevels, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SMapInfo::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (STerrain*)(this) );
	saver.Add( "MapDesignerFileRef", &szMapDesignerFileRef );
	saver.Add( "NorthPoint", &vNorthPoint );
	saver.Add( "NortType", &nNortType );
	saver.Add( "Players", &players );
	saver.Add( "Objects", &objects );
	saver.Add( "Season", &eSeason );
	saver.Add( "DayTime", &eDayTime );
	saver.Add( "Diplomacies", &diplomacies );
	saver.Add( "Entrenchments", &entrenchments );
	saver.Add( "Bridges", &bridges );
	saver.Add( "ScenarioObjects", &scenarioObjects );
	saver.Add( "Reinforcements", &reinforcements );
	saver.Add( "ScriptFileRef", &szScriptFileRef );
	saver.Add( "ScriptAreas", &scriptAreas );
	saver.Add( "startCommandsList", &startCommandsList );
	saver.Add( "reservePositionsList", &reservePositionsList );
	saver.Add( "soundsList", &soundsList );
	saver.Add( "ForestCircleSound", &pForestCircleSound );
	saver.Add( "ForestAmbientSounds", &pForestAmbientSounds );
	saver.Add( "MapType", &nMapType );
	saver.Add( "AttackingSide", &nAttackingSide );
	saver.Add( "PlayerBonusObjects", &playerBonusObjects );
	saver.Add( "Bonuses", &bonuses );
	saver.Add( "MiniMap", &pMiniMap );
	saver.Add( "LocalizedNameFileRef", &szLocalizedNameFileRef );
	saver.Add( "LocalizedDescriptionFileRef", &szLocalizedDescriptionFileRef );
	saver.Add( "LoadingDescriptionFileRef", &szLoadingDescriptionFileRef );
	saver.Add( "LoadingPicture", &pLoadingPicture );
	saver.Add( "CameraPositions", &cameraPositions );
	saver.Add( "ScriptMovies", &scriptMovies );
	saver.Add( "FinalPositions", &finalPositions );
	saver.Add( "Objectives", &objectives );
	saver.Add( "Music", &pMusic );
	saver.Add( "MusicWin", &pMusicWin );
	saver.Add( "MusicLost", &pMusicLost );
	saver.Add( "MPInfo", &mPInfo );
	saver.Add( "BorderLockSize", &nBorderLockSize );
	saver.Add( "BorderCameraSize", &nBorderCameraSize );
	saver.Add( "ScriptEffects", &scriptEffects );
	saver.Add( "CustomDifficultyLevels", &customDifficultyLevels );

	return 0;
}

int SMapInfo::operator&( IBinSaver &saver )
{
	saver.Add( 1, (STerrain*)this );
	saver.Add( 2, &szMapDesignerFileRef );
	saver.Add( 3, &vNorthPoint );
	saver.Add( 4, &nNortType );
	saver.Add( 5, &players );
	saver.Add( 6, &objects );
	saver.Add( 7, &eSeason );
	saver.Add( 8, &eDayTime );
	saver.Add( 9, &diplomacies );
	saver.Add( 10, &entrenchments );
	saver.Add( 11, &bridges );
	saver.Add( 12, &scenarioObjects );
	saver.Add( 13, &reinforcements );
	saver.Add( 14, &szScriptFileRef );
	saver.Add( 15, &scriptAreas );
	saver.Add( 16, &startCommandsList );
	saver.Add( 17, &reservePositionsList );
	saver.Add( 18, &soundsList );
	saver.Add( 19, &pForestCircleSound );
	saver.Add( 20, &pForestAmbientSounds );
	saver.Add( 21, &nMapType );
	saver.Add( 22, &nAttackingSide );
	saver.Add( 23, &playerBonusObjects );
	saver.Add( 24, &bonuses );
	saver.Add( 25, &pMiniMap );
	saver.Add( 26, &szLocalizedNameFileRef );
	saver.Add( 27, &szLocalizedDescriptionFileRef );
	saver.Add( 28, &szLoadingDescriptionFileRef );
	saver.Add( 29, &pLoadingPicture );
	saver.Add( 30, &cameraPositions );
	saver.Add( 31, &scriptMovies );
	saver.Add( 32, &finalPositions );
	saver.Add( 33, &objectives );
	saver.Add( 34, &pMusic );
	saver.Add( 35, &pMusicWin );
	saver.Add( 36, &pMusicLost );
	saver.Add( 37, &mPInfo );
	saver.Add( 38, &nBorderLockSize );
	saver.Add( 39, &nBorderCameraSize );
	saver.Add( 40, &scriptEffects );
	saver.Add( 41, &customDifficultyLevels );

	return 0;
}

uint32_t SMapInfo::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << STerrain::CalcCheckSum() << vNorthPoint << nNortType << players << objects << eSeason << eDayTime << diplomacies << entrenchments << bridges << scenarioObjects << reinforcements << scriptAreas << startCommandsList << reservePositionsList << soundsList << nMapType << nAttackingSide << playerBonusObjects << bonuses << cameraPositions << scriptMovies << finalPositions << objectives << mPInfo << nBorderLockSize << nBorderCameraSize << scriptEffects << customDifficultyLevels;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMultiplayerMap::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "MultiplayerMap", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "Map", (uint8_t*)&pMap - pThis, sizeof(pMap), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "MapNameFileRef", (uint8_t*)&szMapNameFileRef - pThis, sizeof(szMapNameFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "SizeX", (uint8_t*)&nSizeX - pThis, sizeof(nSizeX), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "SizeY", (uint8_t*)&nSizeY - pThis, sizeof(nSizeY), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "Players", (uint8_t*)&nPlayers - pThis, sizeof(nPlayers), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::FinishMetaInfoReport();
}

int SMultiplayerMap::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Map", &pMap );
	saver.Add( "MapNameFileRef", &szMapNameFileRef );
	saver.Add( "SizeX", &nSizeX );
	saver.Add( "SizeY", &nSizeY );
	saver.Add( "Players", &nPlayers );

	return 0;
}

int SMultiplayerMap::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pMap );
	saver.Add( 3, &szMapNameFileRef );
	saver.Add( 4, &nSizeX );
	saver.Add( 5, &nSizeY );
	saver.Add( 6, &nPlayers );

	return 0;
}

uint32_t SMultiplayerMap::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << pMap << nSizeX << nSizeY << nPlayers;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}


std::string EnumToString( NDb::EBonusType eValue )
{
	switch ( eValue )
	{
	case NDb::BT_REPLACE_REINFORCEMENT:
		return "BT_REPLACE_REINFORCEMENT";
	case NDb::BT_ENABLE_REINFORCEMENT:
		return "BT_ENABLE_REINFORCEMENT";
	default:
		return "BT_REPLACE_REINFORCEMENT";
	}
}

NDb::EBonusType NDb::StringToEnum_NDb_EBonusType( const std::string &szValue )
{
	if ( szValue == "BT_REPLACE_REINFORCEMENT" )
		return NDb::BT_REPLACE_REINFORCEMENT;
	if ( szValue == "BT_ENABLE_REINFORCEMENT" )
		return NDb::BT_ENABLE_REINFORCEMENT;
	return NDb::BT_REPLACE_REINFORCEMENT;
}


void SMissionBonus::ReportMetaInfo() const
{
	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "MapToApply", (uint8_t*)&pMapToApply - pThis, sizeof(pMapToApply), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "TextDescFileRef", (uint8_t*)&szTextDescFileRef - pThis, sizeof(szTextDescFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "Player", (uint8_t*)&nPlayer - pThis, sizeof(nPlayer), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "ReinforcementToChange", (uint8_t*)&eReinforcementToChange - pThis, sizeof(eReinforcementToChange), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "HumanPlayer", (uint8_t*)&bHumanPlayer - pThis, sizeof(bHumanPlayer), NTypeDef::TYPE_TYPE_BOOL );
}

int SMissionBonus::operator&( IXmlSaver &saver )
{
	saver.Add( "MapToApply", &pMapToApply );
	saver.Add( "TextDescFileRef", &szTextDescFileRef );
	saver.Add( "Player", &nPlayer );
	saver.Add( "ReinforcementToChange", &eReinforcementToChange );
	saver.Add( "HumanPlayer", &bHumanPlayer );

	return 0;
}

int SMissionBonus::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pMapToApply );
	saver.Add( 3, &szTextDescFileRef );
	saver.Add( 4, &nPlayer );
	saver.Add( 5, &eReinforcementToChange );
	saver.Add( 6, &bHumanPlayer );

	return 0;
}

uint32_t SMissionBonus::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << pMapToApply << nPlayer << eReinforcementToChange << bHumanPlayer;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SReinforcementChange::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "ReinforcementChange", typeID, sizeof(*this) );
	SMissionBonus::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "NewReinforcement", (uint8_t*)&pNewReinforcement - pThis, sizeof(pNewReinforcement), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SReinforcementChange::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SMissionBonus*)(this) );
	saver.Add( "NewReinforcement", &pNewReinforcement );

	return 0;
}

int SReinforcementChange::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SMissionBonus*)this );
	saver.Add( 2, &pNewReinforcement );

	return 0;
}

uint32_t SReinforcementChange::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SMissionBonus::CalcCheckSum() << pNewReinforcement;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SReinforcementEnable::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "ReinforcementEnable", typeID, sizeof(*this) );
	SMissionBonus::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::FinishMetaInfoReport();
}

int SReinforcementEnable::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SMissionBonus*)(this) );

	return 0;
}

int SReinforcementEnable::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SMissionBonus*)this );

	return 0;
}

uint32_t SReinforcementEnable::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SMissionBonus::CalcCheckSum();
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SReinforcementDisable::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "ReinforcementDisable", typeID, sizeof(*this) );
	SMissionBonus::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::FinishMetaInfoReport();
}

int SReinforcementDisable::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SMissionBonus*)(this) );

	return 0;
}

int SReinforcementDisable::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SMissionBonus*)this );

	return 0;
}

uint32_t SReinforcementDisable::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SMissionBonus::CalcCheckSum();
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SAddReinforcementCalls::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "AddReinforcementCalls", typeID, sizeof(*this) );
	SMissionBonus::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "Calls", (uint8_t*)&nCalls - pThis, sizeof(nCalls), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::FinishMetaInfoReport();
}

int SAddReinforcementCalls::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SMissionBonus*)(this) );
	saver.Add( "Calls", &nCalls );

	return 0;
}

int SAddReinforcementCalls::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SMissionBonus*)this );
	saver.Add( 2, &nCalls );

	return 0;
}

uint32_t SAddReinforcementCalls::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SMissionBonus::CalcCheckSum() << nCalls;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SStartUnisAvalabiltyEntry::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "StartReinforcmentType", (uint8_t*)&eStartReinforcmentType - pThis, sizeof(eStartReinforcmentType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "Number", (uint8_t*)&nNumber - pThis, sizeof(nNumber), NTypeDef::TYPE_TYPE_INT );
}

int SStartUnisAvalabiltyEntry::operator&( IXmlSaver &saver )
{
	saver.Add( "StartReinforcmentType", &eStartReinforcmentType );
	saver.Add( "Number", &nNumber );

	return 0;
}

int SStartUnisAvalabiltyEntry::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eStartReinforcmentType );
	saver.Add( 3, &nNumber );

	return 0;
}

uint32_t SStartUnisAvalabiltyEntry::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eStartReinforcmentType << nNumber;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SDifficultyLevel::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "DifficultyLevel", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "LocalizedNameFileRef", (uint8_t*)&szLocalizedNameFileRef - pThis, sizeof(szLocalizedNameFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "PlayerStatModifier", (uint8_t*)&pPlayerStatModifier - pThis, sizeof(pPlayerStatModifier), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "EnemyStatModifier", (uint8_t*)&pEnemyStatModifier - pThis, sizeof(pEnemyStatModifier), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "EnemyReinfCallsCoeff", (uint8_t*)&fEnemyReinfCallsCoeff - pThis, sizeof(fEnemyReinfCallsCoeff), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "EnemyReinfRecycleCoeff", (uint8_t*)&fEnemyReinfRecycleCoeff - pThis, sizeof(fEnemyReinfRecycleCoeff), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SDifficultyLevel::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "LocalizedNameFileRef", &szLocalizedNameFileRef );
	saver.Add( "PlayerStatModifier", &pPlayerStatModifier );
	saver.Add( "EnemyStatModifier", &pEnemyStatModifier );
	saver.Add( "EnemyReinfCallsCoeff", &fEnemyReinfCallsCoeff );
	saver.Add( "EnemyReinfRecycleCoeff", &fEnemyReinfRecycleCoeff );

	return 0;
}

int SDifficultyLevel::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szLocalizedNameFileRef );
	saver.Add( 3, &pPlayerStatModifier );
	saver.Add( 4, &pEnemyStatModifier );
	saver.Add( 5, &fEnemyReinfCallsCoeff );
	saver.Add( 6, &fEnemyReinfRecycleCoeff );

	return 0;
}

uint32_t SDifficultyLevel::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << pPlayerStatModifier << pEnemyStatModifier << fEnemyReinfCallsCoeff << fEnemyReinfRecycleCoeff;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( STATS_B2_M1, 0x11074C80, SPartyDependentInfo )
REGISTER_DATABASE_CLASS( STATS_B2_M1, 0x1711F2C0, SMissionObjective )
REGISTER_DATABASE_CLASS( STATS_B2_M1, 0x10071C00, SMapInfo )
REGISTER_DATABASE_CLASS( STATS_B2_M1, 0x19221C80, SMultiplayerMap )
BASIC_REGISTER_DATABASE_CLASS( STATS_B2_M1, SMissionBonus )
REGISTER_DATABASE_CLASS( STATS_B2_M1, 0x110BC4C1, SReinforcementChange )
REGISTER_DATABASE_CLASS( STATS_B2_M1, 0x110BC481, SReinforcementEnable )
REGISTER_DATABASE_CLASS( STATS_B2_M1, 0x110BC4C0, SReinforcementDisable )
REGISTER_DATABASE_CLASS( STATS_B2_M1, 0x11163C00, SAddReinforcementCalls )
REGISTER_DATABASE_CLASS( STATS_B2_M1, 0x1712D2C0, SDifficultyLevel )

