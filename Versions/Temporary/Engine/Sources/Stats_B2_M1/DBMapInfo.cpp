// automatically generated file, don't change manually!

#include "stdafx.h"
#include "../libdb/ReportMetaInfo.h"
#include "../libdb/Checksum.h"
#include "../System/XmlSaver.h"
#include "dbmapinfo.h"

namespace NDb
{


string EnumToString( NDb::EMPGameType eValue )
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

NDb::EMPGameType NDb::StringToEnum_NDb_EMPGameType( const string &szValue )
{
	if ( szValue == "MP_GT_STANDARD" )
		return NDb::MP_GT_STANDARD;
	if ( szValue == "MP_GT_COUNT" )
		return NDb::MP_GT_COUNT;
	return NDb::MP_GT_STANDARD;
}


void SMPMapInfo::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
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

DWORD SMPMapInfo::CalcCheckSum() const
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



void SCameraPlacement::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Anchor", &vAnchor, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "Yaw", (BYTE*)&fYaw - pThis, sizeof(fYaw), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Pitch", (BYTE*)&fPitch - pThis, sizeof(fPitch), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Dist", (BYTE*)&fDist - pThis, sizeof(fDist), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "UseAnchorOnly", (BYTE*)&bUseAnchorOnly - pThis, sizeof(bUseAnchorOnly), NTypeDef::TYPE_TYPE_BOOL );
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

DWORD SCameraPlacement::CalcCheckSum() const
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



void SScriptCameraPlacement::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Name", (BYTE*)&szName - pThis, sizeof(szName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Position", &vPosition, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "Yaw", (BYTE*)&fYaw - pThis, sizeof(fYaw), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Pitch", (BYTE*)&fPitch - pThis, sizeof(fPitch), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "FOV", (BYTE*)&fFOV - pThis, sizeof(fFOV), NTypeDef::TYPE_TYPE_FLOAT );
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

DWORD SScriptCameraPlacement::CalcCheckSum() const
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



void SScriptMovieKey::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "IsTangentIn", (BYTE*)&bIsTangentIn - pThis, sizeof(bIsTangentIn), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "IsTangentOut", (BYTE*)&bIsTangentOut - pThis, sizeof(bIsTangentOut), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "KeyParam", (BYTE*)&szKeyParam - pThis, sizeof(szKeyParam), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "StartTime", (BYTE*)&fStartTime - pThis, sizeof(fStartTime), NTypeDef::TYPE_TYPE_FLOAT );
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

DWORD SScriptMovieKey::CalcCheckSum() const
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



void SScriptMovieKeyPos::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	SScriptMovieKey::ReportMetaInfo( szAddName, pThis );

	NMetaInfo::ReportMetaInfo( szAddName + "PositionIndex", (BYTE*)&nPositionIndex - pThis, sizeof(nPositionIndex), NTypeDef::TYPE_TYPE_INT );
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

DWORD SScriptMovieKeyPos::CalcCheckSum() const
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



void SScriptMovieKeyFollow::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	SScriptMovieKey::ReportMetaInfo( szAddName, pThis );

	NMetaInfo::ReportMetaInfo( szAddName + "ObjectScriptID", (BYTE*)&nObjectScriptID - pThis, sizeof(nObjectScriptID), NTypeDef::TYPE_TYPE_INT );
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

DWORD SScriptMovieKeyFollow::CalcCheckSum() const
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



void SScriptMovieSequence::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
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

DWORD SScriptMovieSequence::CalcCheckSum() const
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



void SScriptMovies::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
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

DWORD SScriptMovies::CalcCheckSum() const
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "GeneralPartyName", (BYTE*)&szGeneralPartyName - pThis, sizeof(szGeneralPartyName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "GunCrewSquad", (BYTE*)&pGunCrewSquad - pThis, sizeof(pGunCrewSquad), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "HowitzerGunCrewSquad", (BYTE*)&pHowitzerGunCrewSquad - pThis, sizeof(pHowitzerGunCrewSquad), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "HeavyMachinegunSquad", (BYTE*)&pHeavyMachinegunSquad - pThis, sizeof(pHeavyMachinegunSquad), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "AAGunSquad", (BYTE*)&pAAGunSquad - pThis, sizeof(pAAGunSquad), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "ResupplyEngineerSquad", (BYTE*)&pResupplyEngineerSquad - pThis, sizeof(pResupplyEngineerSquad), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "LocalizedNameFileRef", (BYTE*)&szLocalizedNameFileRef - pThis, sizeof(szLocalizedNameFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "MinimapKeyObjectIcon", (BYTE*)&pMinimapKeyObjectIcon - pThis, sizeof(pMinimapKeyObjectIcon), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "MinimapKeyObjectIconSelected", (BYTE*)&pMinimapKeyObjectIconSelected - pThis, sizeof(pMinimapKeyObjectIconSelected), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "StatisticsIcon", (BYTE*)&pStatisticsIcon - pThis, sizeof(pStatisticsIcon), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "ParatrooperVisObj", (BYTE*)&pParatrooperVisObj - pThis, sizeof(pParatrooperVisObj), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "ListItemIcon", (BYTE*)&pListItemIcon - pThis, sizeof(pListItemIcon), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "KeyBuildingFlag", (BYTE*)&pKeyBuildingFlag - pThis, sizeof(pKeyBuildingFlag), NTypeDef::TYPE_TYPE_REF );
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

DWORD SPartyDependentInfo::CalcCheckSum() const
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "HeaderFileRef", (BYTE*)&szHeaderFileRef - pThis, sizeof(szHeaderFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "BriefingFileRef", (BYTE*)&szBriefingFileRef - pThis, sizeof(szBriefingFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "DescriptionFileRef", (BYTE*)&szDescriptionFileRef - pThis, sizeof(szDescriptionFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "IsPrimary", (BYTE*)&bIsPrimary - pThis, sizeof(bIsPrimary), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportStructArrayMetaInfo( "MapPositions", &mapPositions, pThis );
	NMetaInfo::ReportMetaInfo( "Experience", (BYTE*)&nExperience - pThis, sizeof(nExperience), NTypeDef::TYPE_TYPE_INT );
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



void SMapObjectInfo::SLinkInfo::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "LinkID", (BYTE*)&nLinkID - pThis, sizeof(nLinkID), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "LinkWith", (BYTE*)&nLinkWith - pThis, sizeof(nLinkWith), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "Intention", (BYTE*)&bIntention - pThis, sizeof(bIntention), NTypeDef::TYPE_TYPE_BOOL );
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

DWORD SMapObjectInfo::SLinkInfo::CalcCheckSum() const
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



void SMapObjectInfo::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Pos", &vPos, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "Dir", (BYTE*)&nDir - pThis, sizeof(nDir), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "Player", (BYTE*)&nPlayer - pThis, sizeof(nPlayer), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "ScriptID", (BYTE*)&nScriptID - pThis, sizeof(nScriptID), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "HP", (BYTE*)&fHP - pThis, sizeof(fHP), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "FrameIndex", (BYTE*)&nFrameIndex - pThis, sizeof(nFrameIndex), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Link", &link, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "Object", (BYTE*)&pObject - pThis, sizeof(pObject), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "ConstructorProfile", (BYTE*)&pConstructorProfile - pThis, sizeof(pConstructorProfile), NTypeDef::TYPE_TYPE_REF );
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

DWORD SMapObjectInfo::CalcCheckSum() const
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



void SEntrenchmentInfo::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
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

DWORD SEntrenchmentInfo::CalcCheckSum() const
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


string EnumToString( NDb::EScriptAreaTypes eValue )
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

NDb::EScriptAreaTypes NDb::StringToEnum_NDb_EScriptAreaTypes( const string &szValue )
{
	if ( szValue == "EAT_RECTANGLE" )
		return NDb::EAT_RECTANGLE;
	if ( szValue == "EAT_CIRCLE" )
		return NDb::EAT_CIRCLE;
	return NDb::EAT_RECTANGLE;
}


void SScriptArea::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Type", (BYTE*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "Name", (BYTE*)&szName - pThis, sizeof(szName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Center", &vCenter, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "AABBHalfSize", &vAABBHalfSize, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "R", (BYTE*)&fR - pThis, sizeof(fR), NTypeDef::TYPE_TYPE_FLOAT );
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

DWORD SScriptArea::CalcCheckSum() const
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



void SAIStartCommand::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "CmdType", (BYTE*)&nCmdType - pThis, sizeof(nCmdType), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "unitLinkIDs", &unitLinkIDs, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "LinkID", (BYTE*)&nLinkID - pThis, sizeof(nLinkID), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Pos", &vPos, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "FromExplosion", (BYTE*)&bFromExplosion - pThis, sizeof(bFromExplosion), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "Number", (BYTE*)&fNumber - pThis, sizeof(fNumber), NTypeDef::TYPE_TYPE_FLOAT );
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

DWORD SAIStartCommand::CalcCheckSum() const
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



void SBattlePosition::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "ArtilleryLinkID", (BYTE*)&nArtilleryLinkID - pThis, sizeof(nArtilleryLinkID), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "TruckLinkID", (BYTE*)&nTruckLinkID - pThis, sizeof(nTruckLinkID), NTypeDef::TYPE_TYPE_INT );
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

DWORD SBattlePosition::CalcCheckSum() const
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



void SMapSoundInfo::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "sound", (BYTE*)&psound - pThis, sizeof(psound), NTypeDef::TYPE_TYPE_REF );
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

DWORD SMapSoundInfo::CalcCheckSum() const
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



void SEditAreaInfo::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Name", (BYTE*)&szName - pThis, sizeof(szName), NTypeDef::TYPE_TYPE_STRING );
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

DWORD SEditAreaInfo::CalcCheckSum() const
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


string EnumToString( NDb::EParcelType eValue )
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

NDb::EParcelType NDb::StringToEnum_NDb_EParcelType( const string &szValue )
{
	if ( szValue == "EPATCH_UNKNOWN" )
		return NDb::EPATCH_UNKNOWN;
	if ( szValue == "EPATCH_DEFENCE" )
		return NDb::EPATCH_DEFENCE;
	if ( szValue == "EPATCH_REINFORCE" )
		return NDb::EPATCH_REINFORCE;
	return NDb::EPATCH_UNKNOWN;
}


void SReinforcePoint::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Center", &vCenter, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "Direction", (BYTE*)&fDirection - pThis, sizeof(fDirection), NTypeDef::TYPE_TYPE_FLOAT );
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

DWORD SReinforcePoint::CalcCheckSum() const
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



void SAIGeneralParcel::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "reinforcePoints", &reinforcePoints, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "Type", (BYTE*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Center", &vCenter, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "Radius", (BYTE*)&fRadius - pThis, sizeof(fRadius), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Importance", (BYTE*)&fImportance - pThis, sizeof(fImportance), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "DefenceDirection", (BYTE*)&fDefenceDirection - pThis, sizeof(fDefenceDirection), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "MinUnitsToReinforce", (BYTE*)&nMinUnitsToReinforce - pThis, sizeof(nMinUnitsToReinforce), NTypeDef::TYPE_TYPE_INT );
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

DWORD SAIGeneralParcel::CalcCheckSum() const
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



void SAIGeneralSide::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "mobileScriptIDs", &mobileScriptIDs, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "parcels", &parcels, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "MaxMobileTanks", (BYTE*)&nMaxMobileTanks - pThis, sizeof(nMaxMobileTanks), NTypeDef::TYPE_TYPE_INT );
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

DWORD SAIGeneralSide::CalcCheckSum() const
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



void SBonusInstance::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "LinkID", (BYTE*)&nLinkID - pThis, sizeof(nLinkID), NTypeDef::TYPE_TYPE_INT );
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

DWORD SBonusInstance::CalcCheckSum() const
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



void SBuildingBonuses::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "PointID", (BYTE*)&nPointID - pThis, sizeof(nPointID), NTypeDef::TYPE_TYPE_INT );
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

DWORD SBuildingBonuses::CalcCheckSum() const
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



void SPlayerBonusData::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "LinkID", (BYTE*)&nLinkID - pThis, sizeof(nLinkID), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "PlayerBonuses", &playerBonuses, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "Storage", (BYTE*)&bStorage - pThis, sizeof(bStorage), NTypeDef::TYPE_TYPE_BOOL );
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

DWORD SPlayerBonusData::CalcCheckSum() const
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


string EnumToString( NDb::ESuperWeaponType eValue )
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

NDb::ESuperWeaponType NDb::StringToEnum_NDb_ESuperWeaponType( const string &szValue )
{
	if ( szValue == "SUPER_WEAPON_BOMBER" )
		return NDb::SUPER_WEAPON_BOMBER;
	if ( szValue == "SUPER_WEAPON_ROCKET" )
		return NDb::SUPER_WEAPON_ROCKET;
	if ( szValue == "SUPER_WEAPON_ARTILLERY" )
		return NDb::SUPER_WEAPON_ARTILLERY;
	return NDb::SUPER_WEAPON_BOMBER;
}


void SMapPlayerInfo::SDeployPosition::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Position", &vPosition, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "Direction", (BYTE*)&nDirection - pThis, sizeof(nDirection), NTypeDef::TYPE_TYPE_INT );
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

DWORD SMapPlayerInfo::SDeployPosition::CalcCheckSum() const
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



void SMapPlayerInfo::SSuperWeaponInfo::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "SuperWeaponType", (BYTE*)&eSuperWeaponType - pThis, sizeof(eSuperWeaponType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "Count", (BYTE*)&nCount - pThis, sizeof(nCount), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "RecycleTime", (BYTE*)&fRecycleTime - pThis, sizeof(fRecycleTime), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "FlyTime", (BYTE*)&fFlyTime - pThis, sizeof(fFlyTime), NTypeDef::TYPE_TYPE_FLOAT );
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

DWORD SMapPlayerInfo::SSuperWeaponInfo::CalcCheckSum() const
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



void SMapPlayerInfo::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Camera", &camera, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "general", &general, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "PartyInfo", (BYTE*)&pPartyInfo - pThis, sizeof(pPartyInfo), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "ReinforcementPoints", &reinforcementPoints, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "ReinforcementTypes", &reinforcementTypes, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "DefaultRank", (BYTE*)&pDefaultRank - pThis, sizeof(pDefaultRank), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "DiplomacySide", (BYTE*)&nDiplomacySide - pThis, sizeof(nDiplomacySide), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "RecycleTimeCoefficient", (BYTE*)&fRecycleTimeCoefficient - pThis, sizeof(fRecycleTimeCoefficient), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "ReinforcementCalls", (BYTE*)&nReinforcementCalls - pThis, sizeof(nReinforcementCalls), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "LocalizedPlayerNameFileRef", (BYTE*)&szLocalizedPlayerNameFileRef - pThis, sizeof(szLocalizedPlayerNameFileRef), NTypeDef::TYPE_TYPE_STRING );
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

DWORD SMapPlayerInfo::CalcCheckSum() const
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "MapDesignerFileRef", (BYTE*)&szMapDesignerFileRef - pThis, sizeof(szMapDesignerFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructMetaInfo( "NorthPoint", &vNorthPoint, pThis ); 
	NMetaInfo::ReportMetaInfo( "NortType", (BYTE*)&nNortType - pThis, sizeof(nNortType), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructArrayMetaInfo( "Players", &players, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "Objects", &objects, pThis );
	NMetaInfo::ReportMetaInfo( "Season", (BYTE*)&eSeason - pThis, sizeof(eSeason), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "DayTime", (BYTE*)&eDayTime - pThis, sizeof(eDayTime), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportSimpleArrayMetaInfo( "Diplomacies", &diplomacies, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "Entrenchments", &entrenchments, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "Bridges", &bridges, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "ScenarioObjects", &scenarioObjects, pThis );
	NMetaInfo::ReportStructMetaInfo( "Reinforcements", &reinforcements, pThis ); 
	NMetaInfo::ReportMetaInfo( "ScriptFileRef", (BYTE*)&szScriptFileRef - pThis, sizeof(szScriptFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructArrayMetaInfo( "ScriptAreas", &scriptAreas, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "startCommandsList", &startCommandsList, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "reservePositionsList", &reservePositionsList, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "soundsList", &soundsList, pThis );
	NMetaInfo::ReportMetaInfo( "ForestCircleSound", (BYTE*)&pForestCircleSound - pThis, sizeof(pForestCircleSound), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "ForestAmbientSounds", (BYTE*)&pForestAmbientSounds - pThis, sizeof(pForestAmbientSounds), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "MapType", (BYTE*)&nMapType - pThis, sizeof(nMapType), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "AttackingSide", (BYTE*)&nAttackingSide - pThis, sizeof(nAttackingSide), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructArrayMetaInfo( "PlayerBonusObjects", &playerBonusObjects, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "Bonuses", &bonuses, pThis );
	NMetaInfo::ReportMetaInfo( "MiniMap", (BYTE*)&pMiniMap - pThis, sizeof(pMiniMap), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "LocalizedNameFileRef", (BYTE*)&szLocalizedNameFileRef - pThis, sizeof(szLocalizedNameFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "LocalizedDescriptionFileRef", (BYTE*)&szLocalizedDescriptionFileRef - pThis, sizeof(szLocalizedDescriptionFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "LoadingDescriptionFileRef", (BYTE*)&szLoadingDescriptionFileRef - pThis, sizeof(szLoadingDescriptionFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "LoadingPicture", (BYTE*)&pLoadingPicture - pThis, sizeof(pLoadingPicture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructArrayMetaInfo( "CameraPositions", &cameraPositions, pThis );
	NMetaInfo::ReportStructMetaInfo( "ScriptMovies", &scriptMovies, pThis ); 
	NMetaInfo::ReportStructArrayMetaInfo( "FinalPositions", &finalPositions, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "Objectives", &objectives, pThis );
	NMetaInfo::ReportMetaInfo( "Music", (BYTE*)&pMusic - pThis, sizeof(pMusic), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "MusicWin", (BYTE*)&pMusicWin - pThis, sizeof(pMusicWin), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "MusicLost", (BYTE*)&pMusicLost - pThis, sizeof(pMusicLost), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "MPInfo", &mPInfo, pThis ); 
	NMetaInfo::ReportMetaInfo( "BorderLockSize", (BYTE*)&nBorderLockSize - pThis, sizeof(nBorderLockSize), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "BorderCameraSize", (BYTE*)&nBorderCameraSize - pThis, sizeof(nBorderCameraSize), NTypeDef::TYPE_TYPE_INT );
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

DWORD SMapInfo::CalcCheckSum() const
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Map", (BYTE*)&pMap - pThis, sizeof(pMap), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "MapNameFileRef", (BYTE*)&szMapNameFileRef - pThis, sizeof(szMapNameFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "SizeX", (BYTE*)&nSizeX - pThis, sizeof(nSizeX), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "SizeY", (BYTE*)&nSizeY - pThis, sizeof(nSizeY), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "Players", (BYTE*)&nPlayers - pThis, sizeof(nPlayers), NTypeDef::TYPE_TYPE_INT );
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

DWORD SMultiplayerMap::CalcCheckSum() const
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


string EnumToString( NDb::EBonusType eValue )
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

NDb::EBonusType NDb::StringToEnum_NDb_EBonusType( const string &szValue )
{
	if ( szValue == "BT_REPLACE_REINFORCEMENT" )
		return NDb::BT_REPLACE_REINFORCEMENT;
	if ( szValue == "BT_ENABLE_REINFORCEMENT" )
		return NDb::BT_ENABLE_REINFORCEMENT;
	return NDb::BT_REPLACE_REINFORCEMENT;
}


void SMissionBonus::ReportMetaInfo() const
{
	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "MapToApply", (BYTE*)&pMapToApply - pThis, sizeof(pMapToApply), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "TextDescFileRef", (BYTE*)&szTextDescFileRef - pThis, sizeof(szTextDescFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "Player", (BYTE*)&nPlayer - pThis, sizeof(nPlayer), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "ReinforcementToChange", (BYTE*)&eReinforcementToChange - pThis, sizeof(eReinforcementToChange), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "HumanPlayer", (BYTE*)&bHumanPlayer - pThis, sizeof(bHumanPlayer), NTypeDef::TYPE_TYPE_BOOL );
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

DWORD SMissionBonus::CalcCheckSum() const
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "NewReinforcement", (BYTE*)&pNewReinforcement - pThis, sizeof(pNewReinforcement), NTypeDef::TYPE_TYPE_REF );
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

DWORD SReinforcementChange::CalcCheckSum() const
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

	BYTE *pThis = (BYTE*)this;
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

DWORD SReinforcementEnable::CalcCheckSum() const
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

	BYTE *pThis = (BYTE*)this;
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

DWORD SReinforcementDisable::CalcCheckSum() const
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Calls", (BYTE*)&nCalls - pThis, sizeof(nCalls), NTypeDef::TYPE_TYPE_INT );
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

DWORD SAddReinforcementCalls::CalcCheckSum() const
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



void SStartUnisAvalabiltyEntry::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "StartReinforcmentType", (BYTE*)&eStartReinforcmentType - pThis, sizeof(eStartReinforcmentType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "Number", (BYTE*)&nNumber - pThis, sizeof(nNumber), NTypeDef::TYPE_TYPE_INT );
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

DWORD SStartUnisAvalabiltyEntry::CalcCheckSum() const
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "LocalizedNameFileRef", (BYTE*)&szLocalizedNameFileRef - pThis, sizeof(szLocalizedNameFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "PlayerStatModifier", (BYTE*)&pPlayerStatModifier - pThis, sizeof(pPlayerStatModifier), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "EnemyStatModifier", (BYTE*)&pEnemyStatModifier - pThis, sizeof(pEnemyStatModifier), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "EnemyReinfCallsCoeff", (BYTE*)&fEnemyReinfCallsCoeff - pThis, sizeof(fEnemyReinfCallsCoeff), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "EnemyReinfRecycleCoeff", (BYTE*)&fEnemyReinfRecycleCoeff - pThis, sizeof(fEnemyReinfRecycleCoeff), NTypeDef::TYPE_TYPE_FLOAT );
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

DWORD SDifficultyLevel::CalcCheckSum() const
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
REGISTER_DATABASE_CLASS( 0x11074C80, SPartyDependentInfo ) 
REGISTER_DATABASE_CLASS( 0x1711F2C0, SMissionObjective ) 
REGISTER_DATABASE_CLASS( 0x10071C00, SMapInfo ) 
REGISTER_DATABASE_CLASS( 0x19221C80, SMultiplayerMap ) 
BASIC_REGISTER_DATABASE_CLASS( SMissionBonus )
REGISTER_DATABASE_CLASS( 0x110BC4C1, SReinforcementChange ) 
REGISTER_DATABASE_CLASS( 0x110BC481, SReinforcementEnable ) 
REGISTER_DATABASE_CLASS( 0x110BC4C0, SReinforcementDisable ) 
REGISTER_DATABASE_CLASS( 0x11163C00, SAddReinforcementCalls ) 
REGISTER_DATABASE_CLASS( 0x1712D2C0, SDifficultyLevel ) 
