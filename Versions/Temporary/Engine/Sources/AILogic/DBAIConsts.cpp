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
	CCheckSumScope scope( __dwCheckSum );
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
	CCheckSumScope scope( __dwCheckSum );
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
	CCheckSumScope scope( __dwCheckSum );
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
	CCheckSumScope scope( __dwCheckSum );
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
	CCheckSumScope scope( __dwCheckSum );
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
	CCheckSumScope scope( __dwCheckSum );
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
	CCheckSumScope scope( __dwCheckSum );
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



void SAIPropertyRevealInfo::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "UnitType", (uint8_t*)&szUnitType - pThis, sizeof(szUnitType), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "Query", (uint8_t*)&fQuery - pThis, sizeof(fQuery), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "MovingOff", (uint8_t*)&fMovingOff - pThis, sizeof(fMovingOff), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Distance", (uint8_t*)&fDistance - pThis, sizeof(fDistance), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Time", (uint8_t*)&nTime - pThis, sizeof(nTime), NTypeDef::TYPE_TYPE_INT );
}

int SAIPropertyRevealInfo::operator&( IXmlSaver &saver )
{
	saver.Add( "UnitType", &szUnitType );
	saver.Add( "Query", &fQuery );
	saver.Add( "MovingOff", &fMovingOff );
	saver.Add( "Distance", &fDistance );
	saver.Add( "Time", &nTime );

	return 0;
}

int SAIPropertyRevealInfo::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szUnitType );
	saver.Add( 3, &fQuery );
	saver.Add( 4, &fMovingOff );
	saver.Add( 5, &fDistance );
	saver.Add( 6, &nTime );

	return 0;
}

uint32_t SAIPropertyRevealInfo::CalcCheckSum() const
{
	CCheckSumScope scope( __dwCheckSum );
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szUnitType << fQuery << fMovingOff << fDistance << nTime;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SAIPropertyConsts::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "AIPropertyConsts", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "PlaneMinHeight", (uint8_t*)&fPlaneMinHeight - pThis, sizeof(fPlaneMinHeight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "PlaneMaxHeight", (uint8_t*)&fPlaneMaxHeight - pThis, sizeof(fPlaneMaxHeight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "InfantryTimeOfLyingUnderFire", (uint8_t*)&nInfantryTimeOfLyingUnderFire - pThis, sizeof(nInfantryTimeOfLyingUnderFire), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "InfantryLyingSoldierCover", (uint8_t*)&fInfantryLyingSoldierCover - pThis, sizeof(fInfantryLyingSoldierCover), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "InfantryRadiusOfFormation", (uint8_t*)&nInfantryRadiusOfFormation - pThis, sizeof(nInfantryRadiusOfFormation), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "InfantryLyingSpeedFactor", (uint8_t*)&fInfantryLyingSpeedFactor - pThis, sizeof(fInfantryLyingSpeedFactor), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "InfantrySpyGlassRadius", (uint8_t*)&fInfantrySpyGlassRadius - pThis, sizeof(fInfantrySpyGlassRadius), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "EngineersMineVisRadius", (uint8_t*)&nEngineersMineVisRadius - pThis, sizeof(nEngineersMineVisRadius), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "EngineersMineClearRadius", (uint8_t*)&nEngineersMineClearRadius - pThis, sizeof(nEngineersMineClearRadius), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "EngineersEngineerMineCheckPeriod", (uint8_t*)&nEngineersEngineerMineCheckPeriod - pThis, sizeof(nEngineersEngineerMineCheckPeriod), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "EngineersTimeQuant", (uint8_t*)&nEngineersTimeQuant - pThis, sizeof(nEngineersTimeQuant), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "EngineersRuPerQuant", (uint8_t*)&fEngineersRuPerQuant - pThis, sizeof(fEngineersRuPerQuant), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "EngineersEngineerRuCarryWeight", (uint8_t*)&fEngineersEngineerRuCarryWeight - pThis, sizeof(fEngineersEngineerRuCarryWeight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "AviationPlaneGuardStateRadius", (uint8_t*)&fAviationPlaneGuardStateRadius - pThis, sizeof(fAviationPlaneGuardStateRadius), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "AviationFuelDecreaseEconomy", (uint8_t*)&fAviationFuelDecreaseEconomy - pThis, sizeof(fAviationFuelDecreaseEconomy), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "AviationFuelDecrease", (uint8_t*)&fAviationFuelDecrease - pThis, sizeof(fAviationFuelDecrease), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "BuildingsCureSpeedInBuilding", (uint8_t*)&fBuildingsCureSpeedInBuilding - pThis, sizeof(fBuildingsCureSpeedInBuilding), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "BuildingsTimeOfBuildingAlarm", (uint8_t*)&nBuildingsTimeOfBuildingAlarm - pThis, sizeof(nBuildingsTimeOfBuildingAlarm), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "BuildingsCampingTime", (uint8_t*)&nBuildingsCampingTime - pThis, sizeof(nBuildingsCampingTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "BuildingsInsideObjWeaponFactor", (uint8_t*)&fBuildingsInsideObjWeaponFactor - pThis, sizeof(fBuildingsInsideObjWeaponFactor), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "BuildingsInsideObjCombatPeriod", (uint8_t*)&nBuildingsInsideObjCombatPeriod - pThis, sizeof(nBuildingsInsideObjCombatPeriod), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "ArtilleryThresholdInstallTime", (uint8_t*)&nArtilleryThresholdInstallTime - pThis, sizeof(nArtilleryThresholdInstallTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "ArtilleryShootsToRange", (uint8_t*)&nArtilleryShootsToRange - pThis, sizeof(nArtilleryShootsToRange), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "ArtilleryRangedDispersionRadiusBonus", (uint8_t*)&fArtilleryRangedDispersionRadiusBonus - pThis, sizeof(fArtilleryRangedDispersionRadiusBonus), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryRangedAreaRadius", (uint8_t*)&fArtilleryRangedAreaRadius - pThis, sizeof(fArtilleryRangedAreaRadius), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryMaxAntiArtilleryRadius", (uint8_t*)&fArtilleryMaxAntiArtilleryRadius - pThis, sizeof(fArtilleryMaxAntiArtilleryRadius), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryMinAntiArtilleryRadius", (uint8_t*)&fArtilleryMinAntiArtilleryRadius - pThis, sizeof(fArtilleryMinAntiArtilleryRadius), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryShotsToMinimizeLocationRadius", (uint8_t*)&nArtilleryShotsToMinimizeLocationRadius - pThis, sizeof(nArtilleryShotsToMinimizeLocationRadius), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "ArtilleryRevealCirclePeriod", (uint8_t*)&nArtilleryRevealCirclePeriod - pThis, sizeof(nArtilleryRevealCirclePeriod), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "ArtilleryRadiusToStartAntiartilleryFire", (uint8_t*)&fArtilleryRadiusToStartAntiartilleryFire - pThis, sizeof(fArtilleryRadiusToStartAntiartilleryFire), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryRelocationRadius", (uint8_t*)&fArtilleryRelocationRadius - pThis, sizeof(fArtilleryRelocationRadius), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryAudibilityTime", (uint8_t*)&nArtilleryAudibilityTime - pThis, sizeof(nArtilleryAudibilityTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "ArtilleryCheckFreePeriod", (uint8_t*)&fArtilleryCheckFreePeriod - pThis, sizeof(fArtilleryCheckFreePeriod), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "FormationCheckFreeArtPeriod", (uint8_t*)&fFormationCheckFreeArtPeriod - pThis, sizeof(fFormationCheckFreeArtPeriod), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ParatroopersParatrooperFallSpeed", (uint8_t*)&fParatroopersParatrooperFallSpeed - pThis, sizeof(fParatroopersParatrooperFallSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ParatroopersParadropSpred", (uint8_t*)&nParatroopersParadropSpred - pThis, sizeof(nParatroopersParadropSpred), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "ParatroopersPlaneParadropInterval", (uint8_t*)&fParatroopersPlaneParadropInterval - pThis, sizeof(fParatroopersPlaneParadropInterval), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ParatroopersPlaneParadropIntervalPerpMin", (uint8_t*)&fParatroopersPlaneParadropIntervalPerpMin - pThis, sizeof(fParatroopersPlaneParadropIntervalPerpMin), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ParatroopersPlaneParadropIntervalPerpMax", (uint8_t*)&fParatroopersPlaneParadropIntervalPerpMax - pThis, sizeof(fParatroopersPlaneParadropIntervalPerpMax), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ParatroopersParatrooperGroundScanPeriod", (uint8_t*)&nParatroopersParatrooperGroundScanPeriod - pThis, sizeof(nParatroopersParatrooperGroundScanPeriod), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "TransportAndResupplyResupplyRadius", (uint8_t*)&nTransportAndResupplyResupplyRadius - pThis, sizeof(nTransportAndResupplyResupplyRadius), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "MoraleResupplyRadius", (uint8_t*)&nMoraleResupplyRadius - pThis, sizeof(nMoraleResupplyRadius), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "TransportAndResupplyTransportRuCapacity", (uint8_t*)&fTransportAndResupplyTransportRuCapacity - pThis, sizeof(fTransportAndResupplyTransportRuCapacity), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "TransportAndResupplyTransportLoadRuDistance", (uint8_t*)&fTransportAndResupplyTransportLoadRuDistance - pThis, sizeof(fTransportAndResupplyTransportLoadRuDistance), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "TransportAndResupplyLandDistance", (uint8_t*)&fTransportAndResupplyLandDistance - pThis, sizeof(fTransportAndResupplyLandDistance), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CommonNumToScanInSegm", (uint8_t*)&nCommonNumToScanInSegm - pThis, sizeof(nCommonNumToScanInSegm), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "CommonBehUpdateDuration", (uint8_t*)&nCommonBehUpdateDuration - pThis, sizeof(nCommonBehUpdateDuration), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "CommonSoldierBehUpdateDuration", (uint8_t*)&nCommonSoldierBehUpdateDuration - pThis, sizeof(nCommonSoldierBehUpdateDuration), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "CommonAABehUpdateDuration", (uint8_t*)&nCommonAABehUpdateDuration - pThis, sizeof(nCommonAABehUpdateDuration), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "CommonDeadSeeTime", (uint8_t*)&nCommonDeadSeeTime - pThis, sizeof(nCommonDeadSeeTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "CommonTimeToReturnGun", (uint8_t*)&nCommonTimeToReturnGun - pThis, sizeof(nCommonTimeToReturnGun), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "CommonTimeBeforeCamouflage", (uint8_t*)&nCommonTimeBeforeCamouflage - pThis, sizeof(nCommonTimeBeforeCamouflage), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "CommonRadiusOfHitNotify", (uint8_t*)&nCommonRadiusOfHitNotify - pThis, sizeof(nCommonRadiusOfHitNotify), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "CommonTimeOfHitNotify", (uint8_t*)&nCommonTimeOfHitNotify - pThis, sizeof(nCommonTimeOfHitNotify), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "CommonCallForHelpRadius", (uint8_t*)&nCommonCallForHelpRadius - pThis, sizeof(nCommonCallForHelpRadius), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "CommonAICallForHelpRadius", (uint8_t*)&nCommonAICallForHelpRadius - pThis, sizeof(nCommonAICallForHelpRadius), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "CommonTimeToDisappear", (uint8_t*)&nCommonTimeToDisappear - pThis, sizeof(nCommonTimeToDisappear), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "CommonGuardStateRadius", (uint8_t*)&fCommonGuardStateRadius - pThis, sizeof(fCommonGuardStateRadius), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CommonGoodAttackProbability", (uint8_t*)&fCommonGoodAttackProbability - pThis, sizeof(fCommonGoodAttackProbability), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CommonAreaDamageCoeff", (uint8_t*)&fCommonAreaDamageCoeff - pThis, sizeof(fCommonAreaDamageCoeff), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CommonTankTrackHitPoints", (uint8_t*)&fCommonTankTrackHitPoints - pThis, sizeof(fCommonTankTrackHitPoints), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CommonTrajectoryLineRatio", (uint8_t*)&fCommonTrajectoryLineRatio - pThis, sizeof(fCommonTrajectoryLineRatio), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CommonTrajectoryBombG", (uint8_t*)&fCommonTrajectoryBombG - pThis, sizeof(fCommonTrajectoryBombG), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "AviationHeavyFormationDistance", (uint8_t*)&fAviationHeavyFormationDistance - pThis, sizeof(fAviationHeavyFormationDistance), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "AviationLightFormationDistance", (uint8_t*)&fAviationLightFormationDistance - pThis, sizeof(fAviationLightFormationDistance), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "AviationStartRandom", (uint8_t*)&fAviationStartRandom - pThis, sizeof(fAviationStartRandom), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "InfantrySniperCamouflageDecreasePerShoot", (uint8_t*)&fInfantrySniperCamouflageDecreasePerShoot - pThis, sizeof(fInfantrySniperCamouflageDecreasePerShoot), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "InfantrySniperCamouflageIncrease", (uint8_t*)&fInfantrySniperCamouflageIncrease - pThis, sizeof(fInfantrySniperCamouflageIncrease), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CommonAmbushBeginAttackCriteria", (uint8_t*)&fCommonAmbushBeginAttackCriteria - pThis, sizeof(fCommonAmbushBeginAttackCriteria), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryArtilleryRevealCoefficient", (uint8_t*)&fArtilleryArtilleryRevealCoefficient - pThis, sizeof(fArtilleryArtilleryRevealCoefficient), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryDispersionRatioLineMin", (uint8_t*)&fArtilleryDispersionRatioLineMin - pThis, sizeof(fArtilleryDispersionRatioLineMin), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryDispersionRatioLineMax", (uint8_t*)&fArtilleryDispersionRatioLineMax - pThis, sizeof(fArtilleryDispersionRatioLineMax), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryDispersionRatioHowitserMin", (uint8_t*)&fArtilleryDispersionRatioHowitserMin - pThis, sizeof(fArtilleryDispersionRatioHowitserMin), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryDispersionRatioHowitserMax", (uint8_t*)&fArtilleryDispersionRatioHowitserMax - pThis, sizeof(fArtilleryDispersionRatioHowitserMax), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryDispersionRatioBombMin", (uint8_t*)&fArtilleryDispersionRatioBombMin - pThis, sizeof(fArtilleryDispersionRatioBombMin), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryDispersionRatioBombMax", (uint8_t*)&fArtilleryDispersionRatioBombMax - pThis, sizeof(fArtilleryDispersionRatioBombMax), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryDispersionRatioCannonMin", (uint8_t*)&fArtilleryDispersionRatioCannonMin - pThis, sizeof(fArtilleryDispersionRatioCannonMin), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryDispersionRatioCannonMax", (uint8_t*)&fArtilleryDispersionRatioCannonMax - pThis, sizeof(fArtilleryDispersionRatioCannonMax), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryDispersionRatioRocketMin", (uint8_t*)&fArtilleryDispersionRatioRocketMin - pThis, sizeof(fArtilleryDispersionRatioRocketMin), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryDispersionRatioRocketMax", (uint8_t*)&fArtilleryDispersionRatioRocketMax - pThis, sizeof(fArtilleryDispersionRatioRocketMax), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryDispersionRatioGrenadeMin", (uint8_t*)&fArtilleryDispersionRatioGrenadeMin - pThis, sizeof(fArtilleryDispersionRatioGrenadeMin), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryDispersionRatioGrenadeMax", (uint8_t*)&fArtilleryDispersionRatioGrenadeMax - pThis, sizeof(fArtilleryDispersionRatioGrenadeMax), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryDispersionRatioTorpedoMin", (uint8_t*)&fArtilleryDispersionRatioTorpedoMin - pThis, sizeof(fArtilleryDispersionRatioTorpedoMin), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryDispersionRatioTorpedoMax", (uint8_t*)&fArtilleryDispersionRatioTorpedoMax - pThis, sizeof(fArtilleryDispersionRatioTorpedoMax), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryDispersionRatioAARocketMin", (uint8_t*)&fArtilleryDispersionRatioAARocketMin - pThis, sizeof(fArtilleryDispersionRatioAARocketMin), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryDispersionRatioAARocketMax", (uint8_t*)&fArtilleryDispersionRatioAARocketMax - pThis, sizeof(fArtilleryDispersionRatioAARocketMax), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryDispersionRatioFlameMin", (uint8_t*)&fArtilleryDispersionRatioFlameMin - pThis, sizeof(fArtilleryDispersionRatioFlameMin), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryDispersionRatioFlameMax", (uint8_t*)&fArtilleryDispersionRatioFlameMax - pThis, sizeof(fArtilleryDispersionRatioFlameMax), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryDispersionRatioATGMLineMin", (uint8_t*)&fArtilleryDispersionRatioATGMLineMin - pThis, sizeof(fArtilleryDispersionRatioATGMLineMin), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryDispersionRatioATGMLineMax", (uint8_t*)&fArtilleryDispersionRatioATGMLineMax - pThis, sizeof(fArtilleryDispersionRatioATGMLineMax), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryDispersionRatioATGMTopAttackMin", (uint8_t*)&fArtilleryDispersionRatioATGMTopAttackMin - pThis, sizeof(fArtilleryDispersionRatioATGMTopAttackMin), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryDispersionRatioATGMTopAttackMax", (uint8_t*)&fArtilleryDispersionRatioATGMTopAttackMax - pThis, sizeof(fArtilleryDispersionRatioATGMTopAttackMax), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryDispersionRatioSAMMin", (uint8_t*)&fArtilleryDispersionRatioSAMMin - pThis, sizeof(fArtilleryDispersionRatioSAMMin), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArtilleryDispersionRatioSAMMax", (uint8_t*)&fArtilleryDispersionRatioSAMMax - pThis, sizeof(fArtilleryDispersionRatioSAMMax), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CommonCoeffForRandomDelay", (uint8_t*)&fCommonCoeffForRandomDelay - pThis, sizeof(fCommonCoeffForRandomDelay), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CommonHeightForVisRadiusInc", (uint8_t*)&fCommonHeightForVisRadiusInc - pThis, sizeof(fCommonHeightForVisRadiusInc), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "BuildingsBurningSpeed", (uint8_t*)&fBuildingsBurningSpeed - pThis, sizeof(fBuildingsBurningSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "FollowStopRadius", (uint8_t*)&fFollowStopRadius - pThis, sizeof(fFollowStopRadius), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "FollowEqualizeSpeedRadius", (uint8_t*)&fFollowEqualizeSpeedRadius - pThis, sizeof(fFollowEqualizeSpeedRadius), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "FollowGoRadius", (uint8_t*)&fFollowGoRadius - pThis, sizeof(fFollowGoRadius), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "FollowWaitRadius", (uint8_t*)&fFollowWaitRadius - pThis, sizeof(fFollowWaitRadius), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CommonFatalityProbability", (uint8_t*)&fCommonFatalityProbability - pThis, sizeof(fCommonFatalityProbability), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CommonDamageForMassiveDamageFatality", (uint8_t*)&fCommonDamageForMassiveDamageFatality - pThis, sizeof(fCommonDamageForMassiveDamageFatality), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CommonMassiveDamageFatalityProbability", (uint8_t*)&fCommonMassiveDamageFatalityProbability - pThis, sizeof(fCommonMassiveDamageFatalityProbability), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "AviationPlanesBombHeight", (uint8_t*)&fAviationPlanesBombHeight - pThis, sizeof(fAviationPlanesBombHeight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "InfantryStandLieRandomDelay", (uint8_t*)&fInfantryStandLieRandomDelay - pThis, sizeof(fInfantryStandLieRandomDelay), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "TransportAndResupplyResupplyOffset", (uint8_t*)&fTransportAndResupplyResupplyOffset - pThis, sizeof(fTransportAndResupplyResupplyOffset), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "TransportAndResupplyResupplyBalanceCoeff", (uint8_t*)&fTransportAndResupplyResupplyBalanceCoeff - pThis, sizeof(fTransportAndResupplyResupplyBalanceCoeff), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "InfantrySquadMemberLeaveInterval", (uint8_t*)&nInfantrySquadMemberLeaveInterval - pThis, sizeof(nInfantrySquadMemberLeaveInterval), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "TransportAndResupplySoldierRUPrice", (uint8_t*)&fTransportAndResupplySoldierRUPrice - pThis, sizeof(fTransportAndResupplySoldierRUPrice), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CommonHpPercentageToWaitMedic", (uint8_t*)&fCommonHpPercentageToWaitMedic - pThis, sizeof(fCommonHpPercentageToWaitMedic), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CombatSituationDamage", (uint8_t*)&fCombatSituationDamage - pThis, sizeof(fCombatSituationDamage), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CombatSituationTimeDamage", (uint8_t*)&nCombatSituationTimeDamage - pThis, sizeof(nCombatSituationTimeDamage), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "CombatSituationMovingEnemyMechNumber", (uint8_t*)&nCombatSituationMovingEnemyMechNumber - pThis, sizeof(nCombatSituationMovingEnemyMechNumber), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "CombatSituationMovingEnemyInfantryNumber", (uint8_t*)&nCombatSituationMovingEnemyInfantryNumber - pThis, sizeof(nCombatSituationMovingEnemyInfantryNumber), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "TransportAndResupplyMainStorageHealingSpeed", (uint8_t*)&fTransportAndResupplyMainStorageHealingSpeed - pThis, sizeof(fTransportAndResupplyMainStorageHealingSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "TransportAndResupplyResupplyMaxPathLenght", (uint8_t*)&nTransportAndResupplyResupplyMaxPathLenght - pThis, sizeof(nTransportAndResupplyResupplyMaxPathLenght), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "TransportAndResupplyTakeStorageOwnershipRadius", (uint8_t*)&fTransportAndResupplyTakeStorageOwnershipRadius - pThis, sizeof(fTransportAndResupplyTakeStorageOwnershipRadius), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CommonTankPitCover", (uint8_t*)&fCommonTankPitCover - pThis, sizeof(fCommonTankPitCover), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CommonFenceSegmentRuPrice", (uint8_t*)&fCommonFenceSegmentRuPrice - pThis, sizeof(fCommonFenceSegmentRuPrice), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CommonTrenchSegmentRuPrice", (uint8_t*)&fCommonTrenchSegmentRuPrice - pThis, sizeof(fCommonTrenchSegmentRuPrice), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "EngineersMineAPersRuPrice", (uint8_t*)&fEngineersMineAPersRuPrice - pThis, sizeof(fEngineersMineAPersRuPrice), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "EngineersMineATankRuPrice", (uint8_t*)&fEngineersMineATankRuPrice - pThis, sizeof(fEngineersMineATankRuPrice), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "EngineersAntitankRuPrice", (uint8_t*)&fEngineersAntitankRuPrice - pThis, sizeof(fEngineersAntitankRuPrice), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "EngineersComplexObstacleAntitankPeriod", (uint8_t*)&nEngineersComplexObstacleAntitankPeriod - pThis, sizeof(nEngineersComplexObstacleAntitankPeriod), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "TransportAndResupplyMedicalTruckHealRadius", (uint8_t*)&nTransportAndResupplyMedicalTruckHealRadius - pThis, sizeof(nTransportAndResupplyMedicalTruckHealRadius), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "TransportAndResupplyMedicalTruckHealPerUpdateDuration", (uint8_t*)&fTransportAndResupplyMedicalTruckHealPerUpdateDuration - pThis, sizeof(fTransportAndResupplyMedicalTruckHealPerUpdateDuration), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CommonUnitEntrenchTime", (uint8_t*)&nCommonUnitEntrenchTime - pThis, sizeof(nCommonUnitEntrenchTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "FlagsRadius", (uint8_t*)&fFlagsRadius - pThis, sizeof(fFlagsRadius), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "FlagsPointsSpeed", (uint8_t*)&fFlagsPointsSpeed - pThis, sizeof(fFlagsPointsSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "FlagsPlayerPointsSpeed", (uint8_t*)&fFlagsPlayerPointsSpeed - pThis, sizeof(fFlagsPlayerPointsSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "FlagsPointsToReinforcement", (uint8_t*)&fFlagsPointsToReinforcement - pThis, sizeof(fFlagsPointsToReinforcement), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "FlagsTimeToCapture", (uint8_t*)&fFlagsTimeToCapture - pThis, sizeof(fFlagsTimeToCapture), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CommonArmorForAreaDamage", (uint8_t*)&nCommonArmorForAreaDamage - pThis, sizeof(nCommonArmorForAreaDamage), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "BuildingsDefaultFireplaceCoverage", (uint8_t*)&fBuildingsDefaultFireplaceCoverage - pThis, sizeof(fBuildingsDefaultFireplaceCoverage), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "AviationDiveBeforeExplodeTime", (uint8_t*)&nAviationDiveBeforeExplodeTime - pThis, sizeof(nAviationDiveBeforeExplodeTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "AviationDiveAfterExplodeTime", (uint8_t*)&nAviationDiveAfterExplodeTime - pThis, sizeof(nAviationDiveAfterExplodeTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "WeatherTime", (uint8_t*)&nWeatherTime - pThis, sizeof(nWeatherTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "WeatherTimeRandom", (uint8_t*)&nWeatherTimeRandom - pThis, sizeof(nWeatherTimeRandom), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "WeatherPeriod", (uint8_t*)&nWeatherPeriod - pThis, sizeof(nWeatherPeriod), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "WeatherPeriodRandom", (uint8_t*)&nWeatherPeriodRandom - pThis, sizeof(nWeatherPeriodRandom), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "WeatherFireRangeCoefficient", (uint8_t*)&fWeatherFireRangeCoefficient - pThis, sizeof(fWeatherFireRangeCoefficient), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "WeatherTimeToFadeOff", (uint8_t*)&nWeatherTimeToFadeOff - pThis, sizeof(nWeatherTimeToFadeOff), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "AntiAviationArtilleryAimIterations", (uint8_t*)&nAntiAviationArtilleryAimIterations - pThis, sizeof(nAntiAviationArtilleryAimIterations), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "MoraleCoeffToLowMoraleWithoutOfficer", (uint8_t*)&fMoraleCoeffToLowMoraleWithoutOfficer - pThis, sizeof(fMoraleCoeffToLowMoraleWithoutOfficer), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "InfantryMaxDistanceToThrowGrenade", (uint8_t*)&fInfantryMaxDistanceToThrowGrenade - pThis, sizeof(fInfantryMaxDistanceToThrowGrenade), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "MoraleDispersionCoeff", (uint8_t*)&fMoraleDispersionCoeff - pThis, sizeof(fMoraleDispersionCoeff), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "MoraleRelaxCoeff", (uint8_t*)&fMoraleRelaxCoeff - pThis, sizeof(fMoraleRelaxCoeff), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "MoraleAimingCoeff", (uint8_t*)&fMoraleAimingCoeff - pThis, sizeof(fMoraleAimingCoeff), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "TargetResolutionGunplaneDistanceToCenter", (uint8_t*)&fTargetResolutionGunplaneDistanceToCenter - pThis, sizeof(fTargetResolutionGunplaneDistanceToCenter), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "TargetResolutionGunplaneAlphaAttack1", (uint8_t*)&fTargetResolutionGunplaneAlphaAttack1 - pThis, sizeof(fTargetResolutionGunplaneAlphaAttack1), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "TargetResolutionGunplaneAlphaAttack2", (uint8_t*)&fTargetResolutionGunplaneAlphaAttack2 - pThis, sizeof(fTargetResolutionGunplaneAlphaAttack2), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "TargetResolutionGunplaneAlphaGo", (uint8_t*)&fTargetResolutionGunplaneAlphaGo - pThis, sizeof(fTargetResolutionGunplaneAlphaGo), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "TargetResolutionGunplaneAlphaKill", (uint8_t*)&fTargetResolutionGunplaneAlphaKill - pThis, sizeof(fTargetResolutionGunplaneAlphaKill), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "TargetResolutionGunplaneAlphaPrice", (uint8_t*)&fTargetResolutionGunplaneAlphaPrice - pThis, sizeof(fTargetResolutionGunplaneAlphaPrice), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "TargetResolutionGunplaneLimitTime", (uint8_t*)&fTargetResolutionGunplaneLimitTime - pThis, sizeof(fTargetResolutionGunplaneLimitTime), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CommonMaxFireRangeToShootByLine", (uint8_t*)&fCommonMaxFireRangeToShootByLine - pThis, sizeof(fCommonMaxFireRangeToShootByLine), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "BuildingsHPPercentToEscapeFromBuilding", (uint8_t*)&fBuildingsHPPercentToEscapeFromBuilding - pThis, sizeof(fBuildingsHPPercentToEscapeFromBuilding), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "InfantrySpyGlassAngle", (uint8_t*)&fInfantrySpyGlassAngle - pThis, sizeof(fInfantrySpyGlassAngle), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CommonMinRotateAngle", (uint8_t*)&fCommonMinRotateAngle - pThis, sizeof(fCommonMinRotateAngle), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "AviationFighterPatrolTime", (uint8_t*)&nAviationFighterPatrolTime - pThis, sizeof(nAviationFighterPatrolTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "InfantryFullHealthTime", (uint8_t*)&nInfantryFullHealthTime - pThis, sizeof(nInfantryFullHealthTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "EngineersRepairCostAdjust", (uint8_t*)&fEngineersRepairCostAdjust - pThis, sizeof(fEngineersRepairCostAdjust), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CommonTimeToAllowKillCrushedObj", (uint8_t*)&nCommonTimeToAllowKillCrushedObj - pThis, sizeof(nCommonTimeToAllowKillCrushedObj), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "FlagsStorageCaptureTime", (uint8_t*)&nFlagsStorageCaptureTime - pThis, sizeof(nFlagsStorageCaptureTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "UseRoundUnits", (uint8_t*)&bUseRoundUnits - pThis, sizeof(bUseRoundUnits), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportStructArrayMetaInfo( "RevealInfo", &revealInfo, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SAIPropertyConsts::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "PlaneMinHeight", &fPlaneMinHeight );
	saver.Add( "PlaneMaxHeight", &fPlaneMaxHeight );
	saver.Add( "InfantryTimeOfLyingUnderFire", &nInfantryTimeOfLyingUnderFire );
	saver.Add( "InfantryLyingSoldierCover", &fInfantryLyingSoldierCover );
	saver.Add( "InfantryRadiusOfFormation", &nInfantryRadiusOfFormation );
	saver.Add( "InfantryLyingSpeedFactor", &fInfantryLyingSpeedFactor );
	saver.Add( "InfantrySpyGlassRadius", &fInfantrySpyGlassRadius );
	saver.Add( "EngineersMineVisRadius", &nEngineersMineVisRadius );
	saver.Add( "EngineersMineClearRadius", &nEngineersMineClearRadius );
	saver.Add( "EngineersEngineerMineCheckPeriod", &nEngineersEngineerMineCheckPeriod );
	saver.Add( "EngineersTimeQuant", &nEngineersTimeQuant );
	saver.Add( "EngineersRuPerQuant", &fEngineersRuPerQuant );
	saver.Add( "EngineersEngineerRuCarryWeight", &fEngineersEngineerRuCarryWeight );
	saver.Add( "AviationPlaneGuardStateRadius", &fAviationPlaneGuardStateRadius );
	saver.Add( "AviationFuelDecreaseEconomy", &fAviationFuelDecreaseEconomy );
	saver.Add( "AviationFuelDecrease", &fAviationFuelDecrease );
	saver.Add( "BuildingsCureSpeedInBuilding", &fBuildingsCureSpeedInBuilding );
	saver.Add( "BuildingsTimeOfBuildingAlarm", &nBuildingsTimeOfBuildingAlarm );
	saver.Add( "BuildingsCampingTime", &nBuildingsCampingTime );
	saver.Add( "BuildingsInsideObjWeaponFactor", &fBuildingsInsideObjWeaponFactor );
	saver.Add( "BuildingsInsideObjCombatPeriod", &nBuildingsInsideObjCombatPeriod );
	saver.Add( "ArtilleryThresholdInstallTime", &nArtilleryThresholdInstallTime );
	saver.Add( "ArtilleryShootsToRange", &nArtilleryShootsToRange );
	saver.Add( "ArtilleryRangedDispersionRadiusBonus", &fArtilleryRangedDispersionRadiusBonus );
	saver.Add( "ArtilleryRangedAreaRadius", &fArtilleryRangedAreaRadius );
	saver.Add( "ArtilleryMaxAntiArtilleryRadius", &fArtilleryMaxAntiArtilleryRadius );
	saver.Add( "ArtilleryMinAntiArtilleryRadius", &fArtilleryMinAntiArtilleryRadius );
	saver.Add( "ArtilleryShotsToMinimizeLocationRadius", &nArtilleryShotsToMinimizeLocationRadius );
	saver.Add( "ArtilleryRevealCirclePeriod", &nArtilleryRevealCirclePeriod );
	saver.Add( "ArtilleryRadiusToStartAntiartilleryFire", &fArtilleryRadiusToStartAntiartilleryFire );
	saver.Add( "ArtilleryRelocationRadius", &fArtilleryRelocationRadius );
	saver.Add( "ArtilleryAudibilityTime", &nArtilleryAudibilityTime );
	saver.Add( "ArtilleryCheckFreePeriod", &fArtilleryCheckFreePeriod );
	saver.Add( "FormationCheckFreeArtPeriod", &fFormationCheckFreeArtPeriod );
	saver.Add( "ParatroopersParatrooperFallSpeed", &fParatroopersParatrooperFallSpeed );
	saver.Add( "ParatroopersParadropSpred", &nParatroopersParadropSpred );
	saver.Add( "ParatroopersPlaneParadropInterval", &fParatroopersPlaneParadropInterval );
	saver.Add( "ParatroopersPlaneParadropIntervalPerpMin", &fParatroopersPlaneParadropIntervalPerpMin );
	saver.Add( "ParatroopersPlaneParadropIntervalPerpMax", &fParatroopersPlaneParadropIntervalPerpMax );
	saver.Add( "ParatroopersParatrooperGroundScanPeriod", &nParatroopersParatrooperGroundScanPeriod );
	saver.Add( "TransportAndResupplyResupplyRadius", &nTransportAndResupplyResupplyRadius );
	saver.Add( "MoraleResupplyRadius", &nMoraleResupplyRadius );
	saver.Add( "TransportAndResupplyTransportRuCapacity", &fTransportAndResupplyTransportRuCapacity );
	saver.Add( "TransportAndResupplyTransportLoadRuDistance", &fTransportAndResupplyTransportLoadRuDistance );
	saver.Add( "TransportAndResupplyLandDistance", &fTransportAndResupplyLandDistance );
	saver.Add( "CommonNumToScanInSegm", &nCommonNumToScanInSegm );
	saver.Add( "CommonBehUpdateDuration", &nCommonBehUpdateDuration );
	saver.Add( "CommonSoldierBehUpdateDuration", &nCommonSoldierBehUpdateDuration );
	saver.Add( "CommonAABehUpdateDuration", &nCommonAABehUpdateDuration );
	saver.Add( "CommonDeadSeeTime", &nCommonDeadSeeTime );
	saver.Add( "CommonTimeToReturnGun", &nCommonTimeToReturnGun );
	saver.Add( "CommonTimeBeforeCamouflage", &nCommonTimeBeforeCamouflage );
	saver.Add( "CommonRadiusOfHitNotify", &nCommonRadiusOfHitNotify );
	saver.Add( "CommonTimeOfHitNotify", &nCommonTimeOfHitNotify );
	saver.Add( "CommonCallForHelpRadius", &nCommonCallForHelpRadius );
	saver.Add( "CommonAICallForHelpRadius", &nCommonAICallForHelpRadius );
	saver.Add( "CommonTimeToDisappear", &nCommonTimeToDisappear );
	saver.Add( "CommonGuardStateRadius", &fCommonGuardStateRadius );
	saver.Add( "CommonGoodAttackProbability", &fCommonGoodAttackProbability );
	saver.Add( "CommonAreaDamageCoeff", &fCommonAreaDamageCoeff );
	saver.Add( "CommonTankTrackHitPoints", &fCommonTankTrackHitPoints );
	saver.Add( "CommonTrajectoryLineRatio", &fCommonTrajectoryLineRatio );
	saver.Add( "CommonTrajectoryBombG", &fCommonTrajectoryBombG );
	saver.Add( "AviationHeavyFormationDistance", &fAviationHeavyFormationDistance );
	saver.Add( "AviationLightFormationDistance", &fAviationLightFormationDistance );
	saver.Add( "AviationStartRandom", &fAviationStartRandom );
	saver.Add( "InfantrySniperCamouflageDecreasePerShoot", &fInfantrySniperCamouflageDecreasePerShoot );
	saver.Add( "InfantrySniperCamouflageIncrease", &fInfantrySniperCamouflageIncrease );
	saver.Add( "CommonAmbushBeginAttackCriteria", &fCommonAmbushBeginAttackCriteria );
	saver.Add( "ArtilleryArtilleryRevealCoefficient", &fArtilleryArtilleryRevealCoefficient );
	saver.Add( "ArtilleryDispersionRatioLineMin", &fArtilleryDispersionRatioLineMin );
	saver.Add( "ArtilleryDispersionRatioLineMax", &fArtilleryDispersionRatioLineMax );
	saver.Add( "ArtilleryDispersionRatioHowitserMin", &fArtilleryDispersionRatioHowitserMin );
	saver.Add( "ArtilleryDispersionRatioHowitserMax", &fArtilleryDispersionRatioHowitserMax );
	saver.Add( "ArtilleryDispersionRatioBombMin", &fArtilleryDispersionRatioBombMin );
	saver.Add( "ArtilleryDispersionRatioBombMax", &fArtilleryDispersionRatioBombMax );
	saver.Add( "ArtilleryDispersionRatioCannonMin", &fArtilleryDispersionRatioCannonMin );
	saver.Add( "ArtilleryDispersionRatioCannonMax", &fArtilleryDispersionRatioCannonMax );
	saver.Add( "ArtilleryDispersionRatioRocketMin", &fArtilleryDispersionRatioRocketMin );
	saver.Add( "ArtilleryDispersionRatioRocketMax", &fArtilleryDispersionRatioRocketMax );
	saver.Add( "ArtilleryDispersionRatioGrenadeMin", &fArtilleryDispersionRatioGrenadeMin );
	saver.Add( "ArtilleryDispersionRatioGrenadeMax", &fArtilleryDispersionRatioGrenadeMax );
	saver.Add( "ArtilleryDispersionRatioTorpedoMin", &fArtilleryDispersionRatioTorpedoMin );
	saver.Add( "ArtilleryDispersionRatioTorpedoMax", &fArtilleryDispersionRatioTorpedoMax );
	saver.Add( "ArtilleryDispersionRatioAARocketMin", &fArtilleryDispersionRatioAARocketMin );
	saver.Add( "ArtilleryDispersionRatioAARocketMax", &fArtilleryDispersionRatioAARocketMax );
	saver.Add( "ArtilleryDispersionRatioFlameMin", &fArtilleryDispersionRatioFlameMin );
	saver.Add( "ArtilleryDispersionRatioFlameMax", &fArtilleryDispersionRatioFlameMax );
	saver.Add( "ArtilleryDispersionRatioATGMLineMin", &fArtilleryDispersionRatioATGMLineMin );
	saver.Add( "ArtilleryDispersionRatioATGMLineMax", &fArtilleryDispersionRatioATGMLineMax );
	saver.Add( "ArtilleryDispersionRatioATGMTopAttackMin", &fArtilleryDispersionRatioATGMTopAttackMin );
	saver.Add( "ArtilleryDispersionRatioATGMTopAttackMax", &fArtilleryDispersionRatioATGMTopAttackMax );
	saver.Add( "ArtilleryDispersionRatioSAMMin", &fArtilleryDispersionRatioSAMMin );
	saver.Add( "ArtilleryDispersionRatioSAMMax", &fArtilleryDispersionRatioSAMMax );
	saver.Add( "CommonCoeffForRandomDelay", &fCommonCoeffForRandomDelay );
	saver.Add( "CommonHeightForVisRadiusInc", &fCommonHeightForVisRadiusInc );
	saver.Add( "BuildingsBurningSpeed", &fBuildingsBurningSpeed );
	saver.Add( "FollowStopRadius", &fFollowStopRadius );
	saver.Add( "FollowEqualizeSpeedRadius", &fFollowEqualizeSpeedRadius );
	saver.Add( "FollowGoRadius", &fFollowGoRadius );
	saver.Add( "FollowWaitRadius", &fFollowWaitRadius );
	saver.Add( "CommonFatalityProbability", &fCommonFatalityProbability );
	saver.Add( "CommonDamageForMassiveDamageFatality", &fCommonDamageForMassiveDamageFatality );
	saver.Add( "CommonMassiveDamageFatalityProbability", &fCommonMassiveDamageFatalityProbability );
	saver.Add( "AviationPlanesBombHeight", &fAviationPlanesBombHeight );
	saver.Add( "InfantryStandLieRandomDelay", &fInfantryStandLieRandomDelay );
	saver.Add( "TransportAndResupplyResupplyOffset", &fTransportAndResupplyResupplyOffset );
	saver.Add( "TransportAndResupplyResupplyBalanceCoeff", &fTransportAndResupplyResupplyBalanceCoeff );
	saver.Add( "InfantrySquadMemberLeaveInterval", &nInfantrySquadMemberLeaveInterval );
	saver.Add( "TransportAndResupplySoldierRUPrice", &fTransportAndResupplySoldierRUPrice );
	saver.Add( "CommonHpPercentageToWaitMedic", &fCommonHpPercentageToWaitMedic );
	saver.Add( "CombatSituationDamage", &fCombatSituationDamage );
	saver.Add( "CombatSituationTimeDamage", &nCombatSituationTimeDamage );
	saver.Add( "CombatSituationMovingEnemyMechNumber", &nCombatSituationMovingEnemyMechNumber );
	saver.Add( "CombatSituationMovingEnemyInfantryNumber", &nCombatSituationMovingEnemyInfantryNumber );
	saver.Add( "TransportAndResupplyMainStorageHealingSpeed", &fTransportAndResupplyMainStorageHealingSpeed );
	saver.Add( "TransportAndResupplyResupplyMaxPathLenght", &nTransportAndResupplyResupplyMaxPathLenght );
	saver.Add( "TransportAndResupplyTakeStorageOwnershipRadius", &fTransportAndResupplyTakeStorageOwnershipRadius );
	saver.Add( "CommonTankPitCover", &fCommonTankPitCover );
	saver.Add( "CommonFenceSegmentRuPrice", &fCommonFenceSegmentRuPrice );
	saver.Add( "CommonTrenchSegmentRuPrice", &fCommonTrenchSegmentRuPrice );
	saver.Add( "EngineersMineAPersRuPrice", &fEngineersMineAPersRuPrice );
	saver.Add( "EngineersMineATankRuPrice", &fEngineersMineATankRuPrice );
	saver.Add( "EngineersAntitankRuPrice", &fEngineersAntitankRuPrice );
	saver.Add( "EngineersComplexObstacleAntitankPeriod", &nEngineersComplexObstacleAntitankPeriod );
	saver.Add( "TransportAndResupplyMedicalTruckHealRadius", &nTransportAndResupplyMedicalTruckHealRadius );
	saver.Add( "TransportAndResupplyMedicalTruckHealPerUpdateDuration", &fTransportAndResupplyMedicalTruckHealPerUpdateDuration );
	saver.Add( "CommonUnitEntrenchTime", &nCommonUnitEntrenchTime );
	saver.Add( "FlagsRadius", &fFlagsRadius );
	saver.Add( "FlagsPointsSpeed", &fFlagsPointsSpeed );
	saver.Add( "FlagsPlayerPointsSpeed", &fFlagsPlayerPointsSpeed );
	saver.Add( "FlagsPointsToReinforcement", &fFlagsPointsToReinforcement );
	saver.Add( "FlagsTimeToCapture", &fFlagsTimeToCapture );
	saver.Add( "CommonArmorForAreaDamage", &nCommonArmorForAreaDamage );
	saver.Add( "BuildingsDefaultFireplaceCoverage", &fBuildingsDefaultFireplaceCoverage );
	saver.Add( "AviationDiveBeforeExplodeTime", &nAviationDiveBeforeExplodeTime );
	saver.Add( "AviationDiveAfterExplodeTime", &nAviationDiveAfterExplodeTime );
	saver.Add( "WeatherTime", &nWeatherTime );
	saver.Add( "WeatherTimeRandom", &nWeatherTimeRandom );
	saver.Add( "WeatherPeriod", &nWeatherPeriod );
	saver.Add( "WeatherPeriodRandom", &nWeatherPeriodRandom );
	saver.Add( "WeatherFireRangeCoefficient", &fWeatherFireRangeCoefficient );
	saver.Add( "WeatherTimeToFadeOff", &nWeatherTimeToFadeOff );
	saver.Add( "AntiAviationArtilleryAimIterations", &nAntiAviationArtilleryAimIterations );
	saver.Add( "MoraleCoeffToLowMoraleWithoutOfficer", &fMoraleCoeffToLowMoraleWithoutOfficer );
	saver.Add( "InfantryMaxDistanceToThrowGrenade", &fInfantryMaxDistanceToThrowGrenade );
	saver.Add( "MoraleDispersionCoeff", &fMoraleDispersionCoeff );
	saver.Add( "MoraleRelaxCoeff", &fMoraleRelaxCoeff );
	saver.Add( "MoraleAimingCoeff", &fMoraleAimingCoeff );
	saver.Add( "TargetResolutionGunplaneDistanceToCenter", &fTargetResolutionGunplaneDistanceToCenter );
	saver.Add( "TargetResolutionGunplaneAlphaAttack1", &fTargetResolutionGunplaneAlphaAttack1 );
	saver.Add( "TargetResolutionGunplaneAlphaAttack2", &fTargetResolutionGunplaneAlphaAttack2 );
	saver.Add( "TargetResolutionGunplaneAlphaGo", &fTargetResolutionGunplaneAlphaGo );
	saver.Add( "TargetResolutionGunplaneAlphaKill", &fTargetResolutionGunplaneAlphaKill );
	saver.Add( "TargetResolutionGunplaneAlphaPrice", &fTargetResolutionGunplaneAlphaPrice );
	saver.Add( "TargetResolutionGunplaneLimitTime", &fTargetResolutionGunplaneLimitTime );
	saver.Add( "CommonMaxFireRangeToShootByLine", &fCommonMaxFireRangeToShootByLine );
	saver.Add( "BuildingsHPPercentToEscapeFromBuilding", &fBuildingsHPPercentToEscapeFromBuilding );
	saver.Add( "InfantrySpyGlassAngle", &fInfantrySpyGlassAngle );
	saver.Add( "CommonMinRotateAngle", &fCommonMinRotateAngle );
	saver.Add( "AviationFighterPatrolTime", &nAviationFighterPatrolTime );
	saver.Add( "InfantryFullHealthTime", &nInfantryFullHealthTime );
	saver.Add( "EngineersRepairCostAdjust", &fEngineersRepairCostAdjust );
	saver.Add( "CommonTimeToAllowKillCrushedObj", &nCommonTimeToAllowKillCrushedObj );
	saver.Add( "FlagsStorageCaptureTime", &nFlagsStorageCaptureTime );
	saver.Add( "UseRoundUnits", &bUseRoundUnits );
	saver.Add( "RevealInfo", &revealInfo );

	return 0;
}

int SAIPropertyConsts::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fPlaneMinHeight );
	saver.Add( 3, &fPlaneMaxHeight );
	saver.Add( 4, &nInfantryTimeOfLyingUnderFire );
	saver.Add( 5, &fInfantryLyingSoldierCover );
	saver.Add( 6, &nInfantryRadiusOfFormation );
	saver.Add( 7, &fInfantryLyingSpeedFactor );
	saver.Add( 8, &fInfantrySpyGlassRadius );
	saver.Add( 9, &nEngineersMineVisRadius );
	saver.Add( 10, &nEngineersMineClearRadius );
	saver.Add( 11, &nEngineersEngineerMineCheckPeriod );
	saver.Add( 12, &nEngineersTimeQuant );
	saver.Add( 13, &fEngineersRuPerQuant );
	saver.Add( 14, &fEngineersEngineerRuCarryWeight );
	saver.Add( 15, &fAviationPlaneGuardStateRadius );
	saver.Add( 16, &fAviationFuelDecreaseEconomy );
	saver.Add( 17, &fAviationFuelDecrease );
	saver.Add( 18, &fBuildingsCureSpeedInBuilding );
	saver.Add( 19, &nBuildingsTimeOfBuildingAlarm );
	saver.Add( 20, &nBuildingsCampingTime );
	saver.Add( 21, &fBuildingsInsideObjWeaponFactor );
	saver.Add( 22, &nBuildingsInsideObjCombatPeriod );
	saver.Add( 23, &nArtilleryThresholdInstallTime );
	saver.Add( 24, &nArtilleryShootsToRange );
	saver.Add( 25, &fArtilleryRangedDispersionRadiusBonus );
	saver.Add( 26, &fArtilleryRangedAreaRadius );
	saver.Add( 27, &fArtilleryMaxAntiArtilleryRadius );
	saver.Add( 28, &fArtilleryMinAntiArtilleryRadius );
	saver.Add( 29, &nArtilleryShotsToMinimizeLocationRadius );
	saver.Add( 30, &nArtilleryRevealCirclePeriod );
	saver.Add( 31, &fArtilleryRadiusToStartAntiartilleryFire );
	saver.Add( 32, &fArtilleryRelocationRadius );
	saver.Add( 33, &nArtilleryAudibilityTime );
	saver.Add( 34, &fArtilleryCheckFreePeriod );
	saver.Add( 35, &fFormationCheckFreeArtPeriod );
	saver.Add( 36, &fParatroopersParatrooperFallSpeed );
	saver.Add( 37, &nParatroopersParadropSpred );
	saver.Add( 38, &fParatroopersPlaneParadropInterval );
	saver.Add( 39, &fParatroopersPlaneParadropIntervalPerpMin );
	saver.Add( 40, &fParatroopersPlaneParadropIntervalPerpMax );
	saver.Add( 41, &nParatroopersParatrooperGroundScanPeriod );
	saver.Add( 42, &nTransportAndResupplyResupplyRadius );
	saver.Add( 43, &nMoraleResupplyRadius );
	saver.Add( 44, &fTransportAndResupplyTransportRuCapacity );
	saver.Add( 45, &fTransportAndResupplyTransportLoadRuDistance );
	saver.Add( 46, &fTransportAndResupplyLandDistance );
	saver.Add( 47, &nCommonNumToScanInSegm );
	saver.Add( 48, &nCommonBehUpdateDuration );
	saver.Add( 49, &nCommonSoldierBehUpdateDuration );
	saver.Add( 50, &nCommonAABehUpdateDuration );
	saver.Add( 51, &nCommonDeadSeeTime );
	saver.Add( 52, &nCommonTimeToReturnGun );
	saver.Add( 53, &nCommonTimeBeforeCamouflage );
	saver.Add( 54, &nCommonRadiusOfHitNotify );
	saver.Add( 55, &nCommonTimeOfHitNotify );
	saver.Add( 56, &nCommonCallForHelpRadius );
	saver.Add( 57, &nCommonAICallForHelpRadius );
	saver.Add( 58, &nCommonTimeToDisappear );
	saver.Add( 59, &fCommonGuardStateRadius );
	saver.Add( 60, &fCommonGoodAttackProbability );
	saver.Add( 61, &fCommonAreaDamageCoeff );
	saver.Add( 62, &fCommonTankTrackHitPoints );
	saver.Add( 63, &fCommonTrajectoryLineRatio );
	saver.Add( 64, &fCommonTrajectoryBombG );
	saver.Add( 65, &fAviationHeavyFormationDistance );
	saver.Add( 66, &fAviationLightFormationDistance );
	saver.Add( 67, &fAviationStartRandom );
	saver.Add( 68, &fInfantrySniperCamouflageDecreasePerShoot );
	saver.Add( 69, &fInfantrySniperCamouflageIncrease );
	saver.Add( 70, &fCommonAmbushBeginAttackCriteria );
	saver.Add( 71, &fArtilleryArtilleryRevealCoefficient );
	saver.Add( 72, &fArtilleryDispersionRatioLineMin );
	saver.Add( 73, &fArtilleryDispersionRatioLineMax );
	saver.Add( 74, &fArtilleryDispersionRatioHowitserMin );
	saver.Add( 75, &fArtilleryDispersionRatioHowitserMax );
	saver.Add( 76, &fArtilleryDispersionRatioBombMin );
	saver.Add( 77, &fArtilleryDispersionRatioBombMax );
	saver.Add( 78, &fArtilleryDispersionRatioCannonMin );
	saver.Add( 79, &fArtilleryDispersionRatioCannonMax );
	saver.Add( 80, &fArtilleryDispersionRatioRocketMin );
	saver.Add( 81, &fArtilleryDispersionRatioRocketMax );
	saver.Add( 82, &fArtilleryDispersionRatioGrenadeMin );
	saver.Add( 83, &fArtilleryDispersionRatioGrenadeMax );
	saver.Add( 84, &fArtilleryDispersionRatioTorpedoMin );
	saver.Add( 85, &fArtilleryDispersionRatioTorpedoMax );
	saver.Add( 86, &fArtilleryDispersionRatioAARocketMin );
	saver.Add( 87, &fArtilleryDispersionRatioAARocketMax );
	saver.Add( 88, &fArtilleryDispersionRatioFlameMin );
	saver.Add( 89, &fArtilleryDispersionRatioFlameMax );
	saver.Add( 90, &fArtilleryDispersionRatioATGMLineMin );
	saver.Add( 91, &fArtilleryDispersionRatioATGMLineMax );
	saver.Add( 92, &fArtilleryDispersionRatioATGMTopAttackMin );
	saver.Add( 93, &fArtilleryDispersionRatioATGMTopAttackMax );
	saver.Add( 94, &fArtilleryDispersionRatioSAMMin );
	saver.Add( 95, &fArtilleryDispersionRatioSAMMax );
	saver.Add( 96, &fCommonCoeffForRandomDelay );
	saver.Add( 97, &fCommonHeightForVisRadiusInc );
	saver.Add( 98, &fBuildingsBurningSpeed );
	saver.Add( 99, &fFollowStopRadius );
	saver.Add( 100, &fFollowEqualizeSpeedRadius );
	saver.Add( 101, &fFollowGoRadius );
	saver.Add( 102, &fFollowWaitRadius );
	saver.Add( 103, &fCommonFatalityProbability );
	saver.Add( 104, &fCommonDamageForMassiveDamageFatality );
	saver.Add( 105, &fCommonMassiveDamageFatalityProbability );
	saver.Add( 106, &fAviationPlanesBombHeight );
	saver.Add( 107, &fInfantryStandLieRandomDelay );
	saver.Add( 108, &fTransportAndResupplyResupplyOffset );
	saver.Add( 109, &fTransportAndResupplyResupplyBalanceCoeff );
	saver.Add( 110, &nInfantrySquadMemberLeaveInterval );
	saver.Add( 111, &fTransportAndResupplySoldierRUPrice );
	saver.Add( 112, &fCommonHpPercentageToWaitMedic );
	saver.Add( 113, &fCombatSituationDamage );
	saver.Add( 114, &nCombatSituationTimeDamage );
	saver.Add( 115, &nCombatSituationMovingEnemyMechNumber );
	saver.Add( 116, &nCombatSituationMovingEnemyInfantryNumber );
	saver.Add( 117, &fTransportAndResupplyMainStorageHealingSpeed );
	saver.Add( 118, &nTransportAndResupplyResupplyMaxPathLenght );
	saver.Add( 119, &fTransportAndResupplyTakeStorageOwnershipRadius );
	saver.Add( 120, &fCommonTankPitCover );
	saver.Add( 121, &fCommonFenceSegmentRuPrice );
	saver.Add( 122, &fCommonTrenchSegmentRuPrice );
	saver.Add( 123, &fEngineersMineAPersRuPrice );
	saver.Add( 124, &fEngineersMineATankRuPrice );
	saver.Add( 125, &fEngineersAntitankRuPrice );
	saver.Add( 126, &nEngineersComplexObstacleAntitankPeriod );
	saver.Add( 127, &nTransportAndResupplyMedicalTruckHealRadius );
	saver.Add( 128, &fTransportAndResupplyMedicalTruckHealPerUpdateDuration );
	saver.Add( 129, &nCommonUnitEntrenchTime );
	saver.Add( 130, &fFlagsRadius );
	saver.Add( 131, &fFlagsPointsSpeed );
	saver.Add( 132, &fFlagsPlayerPointsSpeed );
	saver.Add( 133, &fFlagsPointsToReinforcement );
	saver.Add( 134, &fFlagsTimeToCapture );
	saver.Add( 135, &nCommonArmorForAreaDamage );
	saver.Add( 136, &fBuildingsDefaultFireplaceCoverage );
	saver.Add( 137, &nAviationDiveBeforeExplodeTime );
	saver.Add( 138, &nAviationDiveAfterExplodeTime );
	saver.Add( 139, &nWeatherTime );
	saver.Add( 140, &nWeatherTimeRandom );
	saver.Add( 141, &nWeatherPeriod );
	saver.Add( 142, &nWeatherPeriodRandom );
	saver.Add( 143, &fWeatherFireRangeCoefficient );
	saver.Add( 144, &nWeatherTimeToFadeOff );
	saver.Add( 145, &nAntiAviationArtilleryAimIterations );
	saver.Add( 146, &fMoraleCoeffToLowMoraleWithoutOfficer );
	saver.Add( 147, &fInfantryMaxDistanceToThrowGrenade );
	saver.Add( 148, &fMoraleDispersionCoeff );
	saver.Add( 149, &fMoraleRelaxCoeff );
	saver.Add( 150, &fMoraleAimingCoeff );
	saver.Add( 151, &fTargetResolutionGunplaneDistanceToCenter );
	saver.Add( 152, &fTargetResolutionGunplaneAlphaAttack1 );
	saver.Add( 153, &fTargetResolutionGunplaneAlphaAttack2 );
	saver.Add( 154, &fTargetResolutionGunplaneAlphaGo );
	saver.Add( 155, &fTargetResolutionGunplaneAlphaKill );
	saver.Add( 156, &fTargetResolutionGunplaneAlphaPrice );
	saver.Add( 157, &fTargetResolutionGunplaneLimitTime );
	saver.Add( 158, &fCommonMaxFireRangeToShootByLine );
	saver.Add( 159, &fBuildingsHPPercentToEscapeFromBuilding );
	saver.Add( 160, &fInfantrySpyGlassAngle );
	saver.Add( 161, &fCommonMinRotateAngle );
	saver.Add( 162, &nAviationFighterPatrolTime );
	saver.Add( 163, &nInfantryFullHealthTime );
	saver.Add( 164, &fEngineersRepairCostAdjust );
	saver.Add( 165, &nCommonTimeToAllowKillCrushedObj );
	saver.Add( 166, &nFlagsStorageCaptureTime );
	saver.Add( 167, &bUseRoundUnits );
	saver.Add( 168, &revealInfo );

	return 0;
}

uint32_t SAIPropertyConsts::CalcCheckSum() const
{
	CCheckSumScope scope( __dwCheckSum );
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fPlaneMinHeight << fPlaneMaxHeight << nInfantryTimeOfLyingUnderFire << fInfantryLyingSoldierCover << nInfantryRadiusOfFormation << fInfantryLyingSpeedFactor << fInfantrySpyGlassRadius << nEngineersMineVisRadius << nEngineersMineClearRadius << nEngineersEngineerMineCheckPeriod << nEngineersTimeQuant << fEngineersRuPerQuant << fEngineersEngineerRuCarryWeight << fAviationPlaneGuardStateRadius << fAviationFuelDecreaseEconomy << fAviationFuelDecrease << fBuildingsCureSpeedInBuilding << nBuildingsTimeOfBuildingAlarm << nBuildingsCampingTime << fBuildingsInsideObjWeaponFactor << nBuildingsInsideObjCombatPeriod << nArtilleryThresholdInstallTime << nArtilleryShootsToRange << fArtilleryRangedDispersionRadiusBonus << fArtilleryRangedAreaRadius << fArtilleryMaxAntiArtilleryRadius << fArtilleryMinAntiArtilleryRadius << nArtilleryShotsToMinimizeLocationRadius << nArtilleryRevealCirclePeriod << fArtilleryRadiusToStartAntiartilleryFire << fArtilleryRelocationRadius << nArtilleryAudibilityTime << fArtilleryCheckFreePeriod << fFormationCheckFreeArtPeriod << fParatroopersParatrooperFallSpeed << nParatroopersParadropSpred << fParatroopersPlaneParadropInterval << fParatroopersPlaneParadropIntervalPerpMin << fParatroopersPlaneParadropIntervalPerpMax << nParatroopersParatrooperGroundScanPeriod << nTransportAndResupplyResupplyRadius << nMoraleResupplyRadius << fTransportAndResupplyTransportRuCapacity << fTransportAndResupplyTransportLoadRuDistance << fTransportAndResupplyLandDistance << nCommonNumToScanInSegm << nCommonBehUpdateDuration << nCommonSoldierBehUpdateDuration << nCommonAABehUpdateDuration << nCommonDeadSeeTime << nCommonTimeToReturnGun << nCommonTimeBeforeCamouflage << nCommonRadiusOfHitNotify << nCommonTimeOfHitNotify << nCommonCallForHelpRadius << nCommonAICallForHelpRadius << nCommonTimeToDisappear << fCommonGuardStateRadius << fCommonGoodAttackProbability << fCommonAreaDamageCoeff << fCommonTankTrackHitPoints << fCommonTrajectoryLineRatio << fCommonTrajectoryBombG << fAviationHeavyFormationDistance << fAviationLightFormationDistance << fAviationStartRandom << fInfantrySniperCamouflageDecreasePerShoot << fInfantrySniperCamouflageIncrease << fCommonAmbushBeginAttackCriteria << fArtilleryArtilleryRevealCoefficient << fArtilleryDispersionRatioLineMin << fArtilleryDispersionRatioLineMax << fArtilleryDispersionRatioHowitserMin << fArtilleryDispersionRatioHowitserMax << fArtilleryDispersionRatioBombMin << fArtilleryDispersionRatioBombMax << fArtilleryDispersionRatioCannonMin << fArtilleryDispersionRatioCannonMax << fArtilleryDispersionRatioRocketMin << fArtilleryDispersionRatioRocketMax << fArtilleryDispersionRatioGrenadeMin << fArtilleryDispersionRatioGrenadeMax << fArtilleryDispersionRatioTorpedoMin << fArtilleryDispersionRatioTorpedoMax << fArtilleryDispersionRatioAARocketMin << fArtilleryDispersionRatioAARocketMax << fArtilleryDispersionRatioFlameMin << fArtilleryDispersionRatioFlameMax << fArtilleryDispersionRatioATGMLineMin << fArtilleryDispersionRatioATGMLineMax << fArtilleryDispersionRatioATGMTopAttackMin << fArtilleryDispersionRatioATGMTopAttackMax << fArtilleryDispersionRatioSAMMin << fArtilleryDispersionRatioSAMMax << fCommonCoeffForRandomDelay << fCommonHeightForVisRadiusInc << fBuildingsBurningSpeed << fFollowStopRadius << fFollowEqualizeSpeedRadius << fFollowGoRadius << fFollowWaitRadius << fCommonFatalityProbability << fCommonDamageForMassiveDamageFatality << fCommonMassiveDamageFatalityProbability << fAviationPlanesBombHeight << fInfantryStandLieRandomDelay << fTransportAndResupplyResupplyOffset << fTransportAndResupplyResupplyBalanceCoeff << nInfantrySquadMemberLeaveInterval << fTransportAndResupplySoldierRUPrice << fCommonHpPercentageToWaitMedic << fCombatSituationDamage << nCombatSituationTimeDamage << nCombatSituationMovingEnemyMechNumber << nCombatSituationMovingEnemyInfantryNumber << fTransportAndResupplyMainStorageHealingSpeed << nTransportAndResupplyResupplyMaxPathLenght << fTransportAndResupplyTakeStorageOwnershipRadius << fCommonTankPitCover << fCommonFenceSegmentRuPrice << fCommonTrenchSegmentRuPrice << fEngineersMineAPersRuPrice << fEngineersMineATankRuPrice << fEngineersAntitankRuPrice << nEngineersComplexObstacleAntitankPeriod << nTransportAndResupplyMedicalTruckHealRadius << fTransportAndResupplyMedicalTruckHealPerUpdateDuration << nCommonUnitEntrenchTime << fFlagsRadius << fFlagsPointsSpeed << fFlagsPlayerPointsSpeed << fFlagsPointsToReinforcement << fFlagsTimeToCapture << nCommonArmorForAreaDamage << fBuildingsDefaultFireplaceCoverage << nAviationDiveBeforeExplodeTime << nAviationDiveAfterExplodeTime << nWeatherTime << nWeatherTimeRandom << nWeatherPeriod << nWeatherPeriodRandom << fWeatherFireRangeCoefficient << nWeatherTimeToFadeOff << nAntiAviationArtilleryAimIterations << fMoraleCoeffToLowMoraleWithoutOfficer << fInfantryMaxDistanceToThrowGrenade << fMoraleDispersionCoeff << fMoraleRelaxCoeff << fMoraleAimingCoeff << fTargetResolutionGunplaneDistanceToCenter << fTargetResolutionGunplaneAlphaAttack1 << fTargetResolutionGunplaneAlphaAttack2 << fTargetResolutionGunplaneAlphaGo << fTargetResolutionGunplaneAlphaKill << fTargetResolutionGunplaneAlphaPrice << fTargetResolutionGunplaneLimitTime << fCommonMaxFireRangeToShootByLine << fBuildingsHPPercentToEscapeFromBuilding << fInfantrySpyGlassAngle << fCommonMinRotateAngle << nAviationFighterPatrolTime << nInfantryFullHealthTime << fEngineersRepairCostAdjust << nCommonTimeToAllowKillCrushedObj << nFlagsStorageCaptureTime << bUseRoundUnits << revealInfo;
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
	CCheckSumScope scope( __dwCheckSum );
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
	NMetaInfo::ReportMetaInfo( "InfantryFormationModifiersApplyInTrenches", (uint8_t*)&bInfantryFormationModifiersApplyInTrenches - pThis, sizeof(bInfantryFormationModifiersApplyInTrenches), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "AIPropertyConsts", (uint8_t*)&pAIPropertyConsts - pThis, sizeof(pAIPropertyConsts), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "DisableEntrenchmentReloadSlowdown", (uint8_t*)&bDisableEntrenchmentReloadSlowdown - pThis, sizeof(bDisableEntrenchmentReloadSlowdown), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "FOWSoundDistance", (uint8_t*)&fFOWSoundDistance - pThis, sizeof(fFOWSoundDistance), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "FOWSoundMinBasePercentage", (uint8_t*)&fFOWSoundMinBasePercentage - pThis, sizeof(fFOWSoundMinBasePercentage), NTypeDef::TYPE_TYPE_FLOAT );
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
	saver.Add( "InfantryFormationModifiersApplyInTrenches", &bInfantryFormationModifiersApplyInTrenches );
	saver.Add( "AIPropertyConsts", &pAIPropertyConsts );
	saver.Add( "DisableEntrenchmentReloadSlowdown", &bDisableEntrenchmentReloadSlowdown );
	saver.Add( "FOWSoundDistance", &fFOWSoundDistance );
	saver.Add( "FOWSoundMinBasePercentage", &fFOWSoundMinBasePercentage );

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
	if ( !saver.IsReading() || saver.HasChunk( 18 ) )
	{
		saver.Add( 18, &bInfantryFormationModifiersApplyInTrenches );
	}
	saver.Add( 19, &pAIPropertyConsts );
	saver.Add( 20, &bDisableEntrenchmentReloadSlowdown );
	if ( !saver.IsReading() || saver.HasChunk( 21 ) )
	{
		saver.Add( 21, &fFOWSoundDistance );
	}
	if ( !saver.IsReading() || saver.HasChunk( 22 ) )
	{
		saver.Add( 22, &fFOWSoundMinBasePercentage );
	}

	return 0;
}

uint32_t SAIGameConsts::CalcCheckSum() const
{
	CCheckSumScope scope( __dwCheckSum );
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << tankPits << foxHoles << common << planeManuvers << warFog << commonScriptFileRefs << reinforcementTypes << reinfExpediency << unitTypesPriorities << nRemoveParachuteTime << reinforcementRecycleTime << pAviationGroundCrashExplosion << pFlamethrowerDeathExplotion << nGroundCrashPlaneSize << typedTemplates << bInfantryFormationModifiersApplyInTrenches << pAIPropertyConsts << bDisableEntrenchmentReloadSlowdown << fFOWSoundDistance << fFOWSoundMinBasePercentage;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( AILOGIC, 0x7F020031, SAIPropertyConsts )
REGISTER_DATABASE_CLASS( AILOGIC, 0x11074CC0, SAIGameConsts )
