// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "DBAIConsts.h"

#include "AILogic_export.h"

#include <cstdint>

namespace NDb
{



void STankPitInfo::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "sandBagTankPits", &sandBagTankPits, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "digTankPits", &digTankPits, pThis );
}

int STankPitInfo::operator&( IXmlSaver &saver )
{
	saver.Add( "sandBagTankPits", &sandBagTankPits );
	saver.Add( "digTankPits", &digTankPits );

	return 0;
}

int STankPitInfo::operator&( IBinSaver &saver )
{
	saver.Add( 2, &sandBagTankPits );
	saver.Add( 3, &digTankPits );

	return 0;
}

uint32_t STankPitInfo::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << sandBagTankPits << digTankPits;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SCommonInfo::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "antitankObjects", &antitankObjects, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "APFence", (uint8_t*)&pAPFence - pThis, sizeof(pAPFence), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "MineUniversal", (uint8_t*)&pMineUniversal - pThis, sizeof(pMineUniversal), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "MineAT", (uint8_t*)&pMineAT - pThis, sizeof(pMineAT), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "MineAP", (uint8_t*)&pMineAP - pThis, sizeof(pMineAP), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "MineCharge", (uint8_t*)&pMineCharge - pThis, sizeof(pMineCharge), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "LandMine", (uint8_t*)&pLandMine - pThis, sizeof(pLandMine), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Entrenchment", (uint8_t*)&pEntrenchment - pThis, sizeof(pEntrenchment), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "TorpedoStats", (uint8_t*)&pTorpedoStats - pThis, sizeof(pTorpedoStats), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "MosinStats", (uint8_t*)&pMosinStats - pThis, sizeof(pMosinStats), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "G152mmML20Stats", (uint8_t*)&pG152mmML20Stats - pThis, sizeof(pG152mmML20Stats), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "SingleUnitFormation", (uint8_t*)&pSingleUnitFormation - pThis, sizeof(pSingleUnitFormation), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "ShellBox", (uint8_t*)&pShellBox - pThis, sizeof(pShellBox), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "expLevels", &expLevels, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "ExpReinfDistributionCoeff", (uint8_t*)&fExpReinfDistributionCoeff - pThis, sizeof(fExpReinfDistributionCoeff), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "ExpCommanderDistributionCoeff", (uint8_t*)&fExpCommanderDistributionCoeff - pThis, sizeof(fExpCommanderDistributionCoeff), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "ExpCommanderUnitPenaltyCoeff", (uint8_t*)&fExpCommanderUnitPenaltyCoeff - pThis, sizeof(fExpCommanderUnitPenaltyCoeff), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "ExpCommanderPenaltyCoeff", (uint8_t*)&fExpCommanderPenaltyCoeff - pThis, sizeof(fExpCommanderPenaltyCoeff), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "NightStatModifier", (uint8_t*)&pNightStatModifier - pThis, sizeof(pNightStatModifier), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "BadWeatherStatModifier", (uint8_t*)&pBadWeatherStatModifier - pThis, sizeof(pBadWeatherStatModifier), NTypeDef::TYPE_TYPE_REF );
}

int SCommonInfo::operator&( IXmlSaver &saver )
{
	saver.Add( "antitankObjects", &antitankObjects );
	saver.Add( "APFence", &pAPFence );
	saver.Add( "MineUniversal", &pMineUniversal );
	saver.Add( "MineAT", &pMineAT );
	saver.Add( "MineAP", &pMineAP );
	saver.Add( "MineCharge", &pMineCharge );
	saver.Add( "LandMine", &pLandMine );
	saver.Add( "Entrenchment", &pEntrenchment );
	saver.Add( "TorpedoStats", &pTorpedoStats );
	saver.Add( "MosinStats", &pMosinStats );
	saver.Add( "G152mmML20Stats", &pG152mmML20Stats );
	saver.Add( "SingleUnitFormation", &pSingleUnitFormation );
	saver.Add( "ShellBox", &pShellBox );
	saver.Add( "expLevels", &expLevels );
	saver.Add( "ExpReinfDistributionCoeff", &fExpReinfDistributionCoeff );
	saver.Add( "ExpCommanderDistributionCoeff", &fExpCommanderDistributionCoeff );
	saver.Add( "ExpCommanderUnitPenaltyCoeff", &fExpCommanderUnitPenaltyCoeff );
	saver.Add( "ExpCommanderPenaltyCoeff", &fExpCommanderPenaltyCoeff );
	saver.Add( "NightStatModifier", &pNightStatModifier );
	saver.Add( "BadWeatherStatModifier", &pBadWeatherStatModifier );

	return 0;
}

int SCommonInfo::operator&( IBinSaver &saver )
{
	saver.Add( 2, &antitankObjects );
	saver.Add( 3, &pAPFence );
	saver.Add( 4, &pMineUniversal );
	saver.Add( 5, &pMineAT );
	saver.Add( 6, &pMineAP );
	saver.Add( 7, &pMineCharge );
	saver.Add( 8, &pLandMine );
	saver.Add( 9, &pEntrenchment );
	saver.Add( 10, &pTorpedoStats );
	saver.Add( 11, &pMosinStats );
	saver.Add( 12, &pG152mmML20Stats );
	saver.Add( 13, &pSingleUnitFormation );
	saver.Add( 14, &pShellBox );
	saver.Add( 15, &expLevels );
	saver.Add( 16, &fExpReinfDistributionCoeff );
	saver.Add( 17, &fExpCommanderDistributionCoeff );
	saver.Add( 18, &fExpCommanderUnitPenaltyCoeff );
	saver.Add( 19, &fExpCommanderPenaltyCoeff );
	saver.Add( 20, &pNightStatModifier );
	saver.Add( 21, &pBadWeatherStatModifier );

	return 0;
}

uint32_t SCommonInfo::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << antitankObjects << pAPFence << pMineUniversal << pMineAT << pMineAP << pMineCharge << pLandMine << pEntrenchment << pTorpedoStats << pMosinStats << pG152mmML20Stats << pSingleUnitFormation << pShellBox << expLevels << fExpReinfDistributionCoeff << fExpCommanderDistributionCoeff << fExpCommanderUnitPenaltyCoeff << fExpCommanderPenaltyCoeff << pNightStatModifier << pBadWeatherStatModifier;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SWarFogConsts::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "MaxRadius", (uint8_t*)&nMaxRadius - pThis, sizeof(nMaxRadius), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "UseHeights", (uint8_t*)&bUseHeights - pThis, sizeof(bUseHeights), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "TanEpsilon", (uint8_t*)&fTanEpsilon - pThis, sizeof(fTanEpsilon), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "UnitHeight", (uint8_t*)&fUnitHeight - pThis, sizeof(fUnitHeight), NTypeDef::TYPE_TYPE_FLOAT );
}

int SWarFogConsts::operator&( IXmlSaver &saver )
{
	saver.Add( "MaxRadius", &nMaxRadius );
	saver.Add( "UseHeights", &bUseHeights );
	saver.Add( "TanEpsilon", &fTanEpsilon );
	saver.Add( "UnitHeight", &fUnitHeight );

	return 0;
}

int SWarFogConsts::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nMaxRadius );
	saver.Add( 3, &bUseHeights );
	saver.Add( 4, &fTanEpsilon );
	saver.Add( 5, &fUnitHeight );

	return 0;
}

uint32_t SWarFogConsts::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nMaxRadius << bUseHeights << fTanEpsilon << fUnitHeight;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SReinforcementRemap::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "ReinfType", (uint8_t*)&eReinfType - pThis, sizeof(eReinfType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "UnitRPGType", (uint8_t*)&eUnitRPGType - pThis, sizeof(eUnitRPGType), NTypeDef::TYPE_TYPE_ENUM );
}

int SReinforcementRemap::operator&( IXmlSaver &saver )
{
	saver.Add( "ReinfType", &eReinfType );
	saver.Add( "UnitRPGType", &eUnitRPGType );

	return 0;
}

int SReinforcementRemap::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eReinfType );
	saver.Add( 3, &eUnitRPGType );

	return 0;
}

uint32_t SReinforcementRemap::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eReinfType << eUnitRPGType;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SReinforcementExpediency::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "Expediency", &expediency, pThis );
}

int SReinforcementExpediency::operator&( IXmlSaver &saver )
{
	saver.Add( "Expediency", &expediency );

	return 0;
}

int SReinforcementExpediency::operator&( IBinSaver &saver )
{
	saver.Add( 2, &expediency );

	return 0;
}

uint32_t SReinforcementExpediency::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << expediency;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SUnitTypePriority::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "UnitType", (uint8_t*)&eUnitType - pThis, sizeof(eUnitType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "Priority", (uint8_t*)&nPriority - pThis, sizeof(nPriority), NTypeDef::TYPE_TYPE_INT );
}

int SUnitTypePriority::operator&( IXmlSaver &saver )
{
	saver.Add( "UnitType", &eUnitType );
	saver.Add( "Priority", &nPriority );

	return 0;
}

int SUnitTypePriority::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eUnitType );
	saver.Add( 3, &nPriority );

	return 0;
}

uint32_t SUnitTypePriority::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eUnitType << nPriority;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SReinfRecycleTime::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Type", (uint8_t*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "Time", (uint8_t*)&nTime - pThis, sizeof(nTime), NTypeDef::TYPE_TYPE_INT );
}

int SReinfRecycleTime::operator&( IXmlSaver &saver )
{
	saver.Add( "Type", &eType );
	saver.Add( "Time", &nTime );

	return 0;
}

int SReinfRecycleTime::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eType );
	saver.Add( 3, &nTime );

	return 0;
}

uint32_t SReinfRecycleTime::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eType << nTime;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SAIGameConsts::SCommonScriptEntry::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "ScriptFileRef", (uint8_t*)&szScriptFileRef - pThis, sizeof(szScriptFileRef), NTypeDef::TYPE_TYPE_STRING );
}

int SAIGameConsts::SCommonScriptEntry::operator&( IXmlSaver &saver )
{
	saver.Add( "ScriptFileRef", &szScriptFileRef );

	return 0;
}

int SAIGameConsts::SCommonScriptEntry::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szScriptFileRef );

	return 0;
}

uint32_t SAIGameConsts::SCommonScriptEntry::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SAIGameConsts::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "AIGameConsts", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportStructMetaInfo( "TankPits", &tankPits, pThis ); 
	NMetaInfo::ReportSimpleArrayMetaInfo( "FoxHoles", &foxHoles, pThis );
	NMetaInfo::ReportStructMetaInfo( "Common", &common, pThis ); 
	NMetaInfo::ReportSimpleArrayMetaInfo( "PlaneManuvers", &planeManuvers, pThis );
	NMetaInfo::ReportStructMetaInfo( "WarFog", &warFog, pThis ); 
	NMetaInfo::ReportStructArrayMetaInfo( "CommonScriptFileRefs", &commonScriptFileRefs, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "ReinforcementTypes", &reinforcementTypes, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "ReinfExpediency", &reinfExpediency, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "UnitTypesPriorities", &unitTypesPriorities, pThis );
	NMetaInfo::ReportMetaInfo( "Parachute", (uint8_t*)&pParachute - pThis, sizeof(pParachute), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "RemoveParachuteTime", (uint8_t*)&nRemoveParachuteTime - pThis, sizeof(nRemoveParachuteTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructArrayMetaInfo( "ReinforcementRecycleTime", &reinforcementRecycleTime, pThis );
	NMetaInfo::ReportMetaInfo( "AviationGroundCrashExplosion", (uint8_t*)&pAviationGroundCrashExplosion - pThis, sizeof(pAviationGroundCrashExplosion), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "FlamethrowerDeathExplotion", (uint8_t*)&pFlamethrowerDeathExplotion - pThis, sizeof(pFlamethrowerDeathExplotion), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "GroundCrashPlaneSize", (uint8_t*)&nGroundCrashPlaneSize - pThis, sizeof(nGroundCrashPlaneSize), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructArrayMetaInfo( "TypedTemplates", &typedTemplates, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SAIGameConsts::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "TankPits", &tankPits );
	saver.Add( "FoxHoles", &foxHoles );
	saver.Add( "Common", &common );
	saver.Add( "PlaneManuvers", &planeManuvers );
	saver.Add( "WarFog", &warFog );
	saver.Add( "CommonScriptFileRefs", &commonScriptFileRefs );
	saver.Add( "ReinforcementTypes", &reinforcementTypes );
	saver.Add( "ReinfExpediency", &reinfExpediency );
	saver.Add( "UnitTypesPriorities", &unitTypesPriorities );
	saver.Add( "Parachute", &pParachute );
	saver.Add( "RemoveParachuteTime", &nRemoveParachuteTime );
	saver.Add( "ReinforcementRecycleTime", &reinforcementRecycleTime );
	saver.Add( "AviationGroundCrashExplosion", &pAviationGroundCrashExplosion );
	saver.Add( "FlamethrowerDeathExplotion", &pFlamethrowerDeathExplotion );
	saver.Add( "GroundCrashPlaneSize", &nGroundCrashPlaneSize );
	saver.Add( "TypedTemplates", &typedTemplates );

	return 0;
}

int SAIGameConsts::operator&( IBinSaver &saver )
{
	saver.Add( 2, &tankPits );
	saver.Add( 3, &foxHoles );
	saver.Add( 4, &common );
	saver.Add( 5, &planeManuvers );
	saver.Add( 6, &warFog );
	saver.Add( 7, &commonScriptFileRefs );
	saver.Add( 8, &reinforcementTypes );
	saver.Add( 9, &reinfExpediency );
	saver.Add( 10, &unitTypesPriorities );
	saver.Add( 11, &pParachute );
	saver.Add( 12, &nRemoveParachuteTime );
	saver.Add( 13, &reinforcementRecycleTime );
	saver.Add( 14, &pAviationGroundCrashExplosion );
	saver.Add( 15, &pFlamethrowerDeathExplotion );
	saver.Add( 16, &nGroundCrashPlaneSize );
	saver.Add( 17, &typedTemplates );

	return 0;
}

uint32_t SAIGameConsts::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << tankPits << foxHoles << common << planeManuvers << warFog << commonScriptFileRefs << reinforcementTypes << reinfExpediency << unitTypesPriorities << nRemoveParachuteTime << reinforcementRecycleTime << pAviationGroundCrashExplosion << pFlamethrowerDeathExplotion << nGroundCrashPlaneSize << typedTemplates;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( AILOGIC, 0x11074CC0, SAIGameConsts )

