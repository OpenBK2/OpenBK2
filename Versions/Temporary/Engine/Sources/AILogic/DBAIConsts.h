#pragma once

// automatically generated file, don't change manually!

#include "AILogic_export.h"

#include "Stats_B2_M1/dbreinforcements.h"
#include "Stats_B2_M1/RPGStats.h"

#include <cstdint>

struct IXmlSaver;

namespace NDb
{
	struct SFenceRPGStats;
	struct SEntrenchmentRPGStats;
	struct SSquadRPGStats;
	struct SUnitStatsModifier;
	enum EReinforcementType : int;
	enum EDesignUnitType : int;
	struct SObjectBaseRPGStats;
	enum EDBUnitRPGType : int;
	struct SMineRPGStats;
	struct SAIExpLevel;
	struct SInfantryRPGStats;
	struct SStaticObjectRPGStats;
	struct SMechUnitRPGStats;
	struct SManuverDescriptor;
	struct SVisObj;

	struct STankPitInfo
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		std::vector< CDBPtr< SMechUnitRPGStats > > sandBagTankPits;
		std::vector< CDBPtr< SMechUnitRPGStats > > digTankPits;

		STankPitInfo() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SCommonInfo
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		std::vector< CDBPtr< SObjectBaseRPGStats > > antitankObjects;
		CDBPtr< SFenceRPGStats > pAPFence;
		CDBPtr< SMineRPGStats > pMineUniversal;
		CDBPtr< SMineRPGStats > pMineAT;
		CDBPtr< SMineRPGStats > pMineAP;
		CDBPtr< SMineRPGStats > pMineCharge;
		CDBPtr< SMineRPGStats > pLandMine;
		CDBPtr< SEntrenchmentRPGStats > pEntrenchment;
		CDBPtr< SMechUnitRPGStats > pTorpedoStats;
		CDBPtr< SInfantryRPGStats > pMosinStats;
		CDBPtr< SMechUnitRPGStats > pG152mmML20Stats;
		CDBPtr< SSquadRPGStats > pSingleUnitFormation;
		CDBPtr< SStaticObjectRPGStats > pShellBox;
		std::vector< CDBPtr< SAIExpLevel > > expLevels;
		float fExpReinfDistributionCoeff;
		float fExpCommanderDistributionCoeff;
		float fExpCommanderUnitPenaltyCoeff;
		float fExpCommanderPenaltyCoeff;
		CDBPtr< SUnitStatsModifier > pNightStatModifier;
		CDBPtr< SUnitStatsModifier > pBadWeatherStatModifier;

		SCommonInfo() :
			__dwCheckSum( 0 ),
			fExpReinfDistributionCoeff( 0.0f ),
			fExpCommanderDistributionCoeff( 0.0f ),
			fExpCommanderUnitPenaltyCoeff( 0.0f ),
			fExpCommanderPenaltyCoeff( 0.0f )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SWarFogConsts
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		int nMaxRadius;
		bool bUseHeights;
		float fTanEpsilon;
		float fUnitHeight;

		SWarFogConsts() :
			__dwCheckSum( 0 ),
			nMaxRadius( 0 ),
			bUseHeights( false ),
			fTanEpsilon( 0.0f ),
			fUnitHeight( 0.0f )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SReinforcementRemap
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		EReinforcementType eReinfType;
		EDBUnitRPGType eUnitRPGType;

		SReinforcementRemap() :
			__dwCheckSum( 0 ),
			eReinfType( RT_MAIN_INFANTRY ),
			eUnitRPGType( DB_RPG_TYPE_SOLDIER )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SReinforcementExpediency
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		std::vector< float > expediency;

		SReinforcementExpediency() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SUnitTypePriority
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		EDesignUnitType eUnitType;
		int nPriority;

		SUnitTypePriority() :
			__dwCheckSum( 0 ),
			eUnitType( UNIT_TYPE_UNKNOWN ),
			nPriority( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SReinfRecycleTime
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		EReinforcementType eType;
		int nTime;

		SReinfRecycleTime() :
			__dwCheckSum( 0 ),
			eType( RT_MAIN_INFANTRY ),
			nTime( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SAIPropertyRevealInfo
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		std::string szUnitType;
		float fQuery;
		float fMovingOff;
		float fDistance;
		int nTime;

		SAIPropertyRevealInfo() :
			__dwCheckSum( 0 ),
			fQuery( 0.0000f ),
			fMovingOff( 0.0000f ),
			fDistance( 0.0000f ),
			nTime( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct AILOGIC_EXPORT SAIPropertyConsts : public CResource
	{
		OBJECT_BASIC_METHODS( SAIPropertyConsts )
	public:
		enum { typeID = 0x7F020031 };
	private:
		mutable uint32_t __dwCheckSum;
	public:
		float fPlaneMinHeight;
		float fPlaneMaxHeight;
		int nInfantryTimeOfLyingUnderFire;
		float fInfantryLyingSoldierCover;
		int nInfantryRadiusOfFormation;
		float fInfantryLyingSpeedFactor;
		float fInfantrySpyGlassRadius;
		int nEngineersMineVisRadius;
		int nEngineersMineClearRadius;
		int nEngineersEngineerMineCheckPeriod;
		int nEngineersTimeQuant;
		float fEngineersRuPerQuant;
		float fEngineersEngineerRuCarryWeight;
		float fAviationPlaneGuardStateRadius;
		float fAviationFuelDecreaseEconomy;
		float fAviationFuelDecrease;
		float fBuildingsCureSpeedInBuilding;
		int nBuildingsTimeOfBuildingAlarm;
		int nBuildingsCampingTime;
		float fBuildingsInsideObjWeaponFactor;
		int nBuildingsInsideObjCombatPeriod;
		int nArtilleryThresholdInstallTime;
		int nArtilleryShootsToRange;
		float fArtilleryRangedDispersionRadiusBonus;
		float fArtilleryRangedAreaRadius;
		float fArtilleryMaxAntiArtilleryRadius;
		float fArtilleryMinAntiArtilleryRadius;
		int nArtilleryShotsToMinimizeLocationRadius;
		int nArtilleryRevealCirclePeriod;
		float fArtilleryRadiusToStartAntiartilleryFire;
		float fArtilleryRelocationRadius;
		int nArtilleryAudibilityTime;
		float fArtilleryCheckFreePeriod;
		float fFormationCheckFreeArtPeriod;
		float fParatroopersParatrooperFallSpeed;
		int nParatroopersParadropSpred;
		float fParatroopersPlaneParadropInterval;
		float fParatroopersPlaneParadropIntervalPerpMin;
		float fParatroopersPlaneParadropIntervalPerpMax;
		int nParatroopersParatrooperGroundScanPeriod;
		int nTransportAndResupplyResupplyRadius;
		int nMoraleResupplyRadius;
		float fTransportAndResupplyTransportRuCapacity;
		float fTransportAndResupplyTransportLoadRuDistance;
		float fTransportAndResupplyLandDistance;
		int nCommonNumToScanInSegm;
		int nCommonBehUpdateDuration;
		int nCommonSoldierBehUpdateDuration;
		int nCommonAABehUpdateDuration;
		int nCommonDeadSeeTime;
		int nCommonTimeToReturnGun;
		int nCommonTimeBeforeCamouflage;
		int nCommonRadiusOfHitNotify;
		int nCommonTimeOfHitNotify;
		int nCommonCallForHelpRadius;
		int nCommonAICallForHelpRadius;
		int nCommonTimeToDisappear;
		float fCommonGuardStateRadius;
		float fCommonGoodAttackProbability;
		float fCommonAreaDamageCoeff;
		float fCommonTankTrackHitPoints;
		float fCommonTrajectoryLineRatio;
		float fCommonTrajectoryBombG;
		float fAviationHeavyFormationDistance;
		float fAviationLightFormationDistance;
		float fAviationStartRandom;
		float fInfantrySniperCamouflageDecreasePerShoot;
		float fInfantrySniperCamouflageIncrease;
		float fCommonAmbushBeginAttackCriteria;
		float fArtilleryArtilleryRevealCoefficient;
		float fArtilleryDispersionRatioLineMin;
		float fArtilleryDispersionRatioLineMax;
		float fArtilleryDispersionRatioHowitserMin;
		float fArtilleryDispersionRatioHowitserMax;
		float fArtilleryDispersionRatioBombMin;
		float fArtilleryDispersionRatioBombMax;
		float fArtilleryDispersionRatioCannonMin;
		float fArtilleryDispersionRatioCannonMax;
		float fArtilleryDispersionRatioRocketMin;
		float fArtilleryDispersionRatioRocketMax;
		float fArtilleryDispersionRatioGrenadeMin;
		float fArtilleryDispersionRatioGrenadeMax;
		float fArtilleryDispersionRatioTorpedoMin;
		float fArtilleryDispersionRatioTorpedoMax;
		float fArtilleryDispersionRatioAARocketMin;
		float fArtilleryDispersionRatioAARocketMax;
		float fArtilleryDispersionRatioFlameMin;
		float fArtilleryDispersionRatioFlameMax;
		float fArtilleryDispersionRatioATGMLineMin;
		float fArtilleryDispersionRatioATGMLineMax;
		float fArtilleryDispersionRatioATGMTopAttackMin;
		float fArtilleryDispersionRatioATGMTopAttackMax;
		float fArtilleryDispersionRatioSAMMin;
		float fArtilleryDispersionRatioSAMMax;
		float fCommonCoeffForRandomDelay;
		float fCommonHeightForVisRadiusInc;
		float fBuildingsBurningSpeed;
		float fFollowStopRadius;
		float fFollowEqualizeSpeedRadius;
		float fFollowGoRadius;
		float fFollowWaitRadius;
		float fCommonFatalityProbability;
		float fCommonDamageForMassiveDamageFatality;
		float fCommonMassiveDamageFatalityProbability;
		float fAviationPlanesBombHeight;
		float fInfantryStandLieRandomDelay;
		float fTransportAndResupplyResupplyOffset;
		float fTransportAndResupplyResupplyBalanceCoeff;
		int nInfantrySquadMemberLeaveInterval;
		float fTransportAndResupplySoldierRUPrice;
		float fCommonHpPercentageToWaitMedic;
		float fCombatSituationDamage;
		int nCombatSituationTimeDamage;
		int nCombatSituationMovingEnemyMechNumber;
		int nCombatSituationMovingEnemyInfantryNumber;
		float fTransportAndResupplyMainStorageHealingSpeed;
		int nTransportAndResupplyResupplyMaxPathLenght;
		float fTransportAndResupplyTakeStorageOwnershipRadius;
		float fCommonTankPitCover;
		float fCommonFenceSegmentRuPrice;
		float fCommonTrenchSegmentRuPrice;
		float fEngineersMineAPersRuPrice;
		float fEngineersMineATankRuPrice;
		float fEngineersAntitankRuPrice;
		int nEngineersComplexObstacleAntitankPeriod;
		int nTransportAndResupplyMedicalTruckHealRadius;
		float fTransportAndResupplyMedicalTruckHealPerUpdateDuration;
		int nCommonUnitEntrenchTime;
		float fFlagsRadius;
		float fFlagsPointsSpeed;
		float fFlagsPlayerPointsSpeed;
		float fFlagsPointsToReinforcement;
		float fFlagsTimeToCapture;
		int nCommonArmorForAreaDamage;
		float fBuildingsDefaultFireplaceCoverage;
		int nAviationDiveBeforeExplodeTime;
		int nAviationDiveAfterExplodeTime;
		int nWeatherTime;
		int nWeatherTimeRandom;
		int nWeatherPeriod;
		int nWeatherPeriodRandom;
		float fWeatherFireRangeCoefficient;
		int nWeatherTimeToFadeOff;
		int nAntiAviationArtilleryAimIterations;
		float fMoraleCoeffToLowMoraleWithoutOfficer;
		float fInfantryMaxDistanceToThrowGrenade;
		float fMoraleDispersionCoeff;
		float fMoraleRelaxCoeff;
		float fMoraleAimingCoeff;
		float fTargetResolutionGunplaneDistanceToCenter;
		float fTargetResolutionGunplaneAlphaAttack1;
		float fTargetResolutionGunplaneAlphaAttack2;
		float fTargetResolutionGunplaneAlphaGo;
		float fTargetResolutionGunplaneAlphaKill;
		float fTargetResolutionGunplaneAlphaPrice;
		float fTargetResolutionGunplaneLimitTime;
		float fCommonMaxFireRangeToShootByLine;
		float fBuildingsHPPercentToEscapeFromBuilding;
		float fInfantrySpyGlassAngle;
		float fCommonMinRotateAngle;
		int nAviationFighterPatrolTime;
		int nInfantryFullHealthTime;
		float fEngineersRepairCostAdjust;
		int nCommonTimeToAllowKillCrushedObj;
		int nFlagsStorageCaptureTime;
		bool bUseRoundUnits;
		std::vector< SAIPropertyRevealInfo > revealInfo;

		SAIPropertyConsts() :
			__dwCheckSum( 0 ),
			fPlaneMinHeight( 300.0000f ),
			fPlaneMaxHeight( 1500.0000f ),
			nInfantryTimeOfLyingUnderFire( 2000 ),
			fInfantryLyingSoldierCover( 0.7000f ),
			nInfantryRadiusOfFormation( 320 ),
			fInfantryLyingSpeedFactor( 0.5000f ),
			fInfantrySpyGlassRadius( 1920.0000f ),
			nEngineersMineVisRadius( 96 ),
			nEngineersMineClearRadius( 224 ),
			nEngineersEngineerMineCheckPeriod( 0 ),
			nEngineersTimeQuant( 160 ),
			fEngineersRuPerQuant( 10.0000f ),
			fEngineersEngineerRuCarryWeight( 3000.0000f ),
			fAviationPlaneGuardStateRadius( 3000.0000f ),
			fAviationFuelDecreaseEconomy( 0.5000f ),
			fAviationFuelDecrease( 2.0000f ),
			fBuildingsCureSpeedInBuilding( 0.0010f ),
			nBuildingsTimeOfBuildingAlarm( 8000 ),
			nBuildingsCampingTime( 2000 ),
			fBuildingsInsideObjWeaponFactor( 0.5000f ),
			nBuildingsInsideObjCombatPeriod( 800 ),
			nArtilleryThresholdInstallTime( 2000 ),
			nArtilleryShootsToRange( 4 ),
			fArtilleryRangedDispersionRadiusBonus( 0.5000f ),
			fArtilleryRangedAreaRadius( 160.0000f ),
			fArtilleryMaxAntiArtilleryRadius( 320.0000f ),
			fArtilleryMinAntiArtilleryRadius( 32.0000f ),
			nArtilleryShotsToMinimizeLocationRadius( 5 ),
			nArtilleryRevealCirclePeriod( 2000 ),
			fArtilleryRadiusToStartAntiartilleryFire( 320.0000f ),
			fArtilleryRelocationRadius( 160.0000f ),
			nArtilleryAudibilityTime( 20000 ),
			fArtilleryCheckFreePeriod( 20000.0000f ),
			fFormationCheckFreeArtPeriod( 30000.0000f ),
			fParatroopersParatrooperFallSpeed( 0.0500f ),
			nParatroopersParadropSpred( 4 ),
			fParatroopersPlaneParadropInterval( 50.0000f ),
			fParatroopersPlaneParadropIntervalPerpMin( 50.0000f ),
			fParatroopersPlaneParadropIntervalPerpMax( 50.0000f ),
			nParatroopersParatrooperGroundScanPeriod( 200 ),
			nTransportAndResupplyResupplyRadius( 1000 ),
			nMoraleResupplyRadius( 300 ),
			fTransportAndResupplyTransportRuCapacity( 10000.0000f ),
			fTransportAndResupplyTransportLoadRuDistance( 100.0000f ),
			fTransportAndResupplyLandDistance( 44.8000f ),
			nCommonNumToScanInSegm( 50 ),
			nCommonBehUpdateDuration( 2000 ),
			nCommonSoldierBehUpdateDuration( 3000 ),
			nCommonAABehUpdateDuration( 200 ),
			nCommonDeadSeeTime( 2000 ),
			nCommonTimeToReturnGun( 5000 ),
			nCommonTimeBeforeCamouflage( 2000 ),
			nCommonRadiusOfHitNotify( 160 ),
			nCommonTimeOfHitNotify( 1000 ),
			nCommonCallForHelpRadius( 640 ),
			nCommonAICallForHelpRadius( 640 ),
			nCommonTimeToDisappear( 0 ),
			fCommonGuardStateRadius( 320.0000f ),
			fCommonGoodAttackProbability( 0.6000f ),
			fCommonAreaDamageCoeff( 0.2000f ),
			fCommonTankTrackHitPoints( 0.0000f ),
			fCommonTrajectoryLineRatio( 0.6000f ),
			fCommonTrajectoryBombG( 0.0010f ),
			fAviationHeavyFormationDistance( 2.0000f ),
			fAviationLightFormationDistance( 1.2000f ),
			fAviationStartRandom( 0.0000f ),
			fInfantrySniperCamouflageDecreasePerShoot( 0.1000f ),
			fInfantrySniperCamouflageIncrease( 0.0004f ),
			fCommonAmbushBeginAttackCriteria( 0.5000f ),
			fArtilleryArtilleryRevealCoefficient( 200.0000f ),
			fArtilleryDispersionRatioLineMin( 1.0000f ),
			fArtilleryDispersionRatioLineMax( 1.0000f ),
			fArtilleryDispersionRatioHowitserMin( 1.0000f ),
			fArtilleryDispersionRatioHowitserMax( 2.0000f ),
			fArtilleryDispersionRatioBombMin( 1.0000f ),
			fArtilleryDispersionRatioBombMax( 3.0000f ),
			fArtilleryDispersionRatioCannonMin( 1.0000f ),
			fArtilleryDispersionRatioCannonMax( 4.0000f ),
			fArtilleryDispersionRatioRocketMin( 1.0000f ),
			fArtilleryDispersionRatioRocketMax( 4.0000f ),
			fArtilleryDispersionRatioGrenadeMin( 1.0000f ),
			fArtilleryDispersionRatioGrenadeMax( 1.0000f ),
			fArtilleryDispersionRatioTorpedoMin( 1.0000f ),
			fArtilleryDispersionRatioTorpedoMax( 1.0000f ),
			fArtilleryDispersionRatioAARocketMin( 1.0000f ),
			fArtilleryDispersionRatioAARocketMax( 1.0000f ),
			fArtilleryDispersionRatioFlameMin( 1.0000f ),
			fArtilleryDispersionRatioFlameMax( 1.0000f ),
			fArtilleryDispersionRatioATGMLineMin( 1.0000f ),
			fArtilleryDispersionRatioATGMLineMax( 1.0000f ),
			fArtilleryDispersionRatioATGMTopAttackMin( 1.0000f ),
			fArtilleryDispersionRatioATGMTopAttackMax( 1.0000f ),
			fArtilleryDispersionRatioSAMMin( 1.0000f ),
			fArtilleryDispersionRatioSAMMax( 1.0000f ),
			fCommonCoeffForRandomDelay( 1.2100f ),
			fCommonHeightForVisRadiusInc( 1.0000f ),
			fBuildingsBurningSpeed( 0.0020f ),
			fFollowStopRadius( 288.0000f ),
			fFollowEqualizeSpeedRadius( 384.0000f ),
			fFollowGoRadius( 352.0000f ),
			fFollowWaitRadius( 768.0000f ),
			fCommonFatalityProbability( 0.1000f ),
			fCommonDamageForMassiveDamageFatality( 0.7000f ),
			fCommonMassiveDamageFatalityProbability( 0.8000f ),
			fAviationPlanesBombHeight( 0.0000f ),
			fInfantryStandLieRandomDelay( 500.0000f ),
			fTransportAndResupplyResupplyOffset( 100.0000f ),
			fTransportAndResupplyResupplyBalanceCoeff( 500.0000f ),
			nInfantrySquadMemberLeaveInterval( 200 ),
			fTransportAndResupplySoldierRUPrice( 50.0000f ),
			fCommonHpPercentageToWaitMedic( 0.2000f ),
			fCombatSituationDamage( 100.0000f ),
			nCombatSituationTimeDamage( 1000 ),
			nCombatSituationMovingEnemyMechNumber( 3 ),
			nCombatSituationMovingEnemyInfantryNumber( 20 ),
			fTransportAndResupplyMainStorageHealingSpeed( 10.0000f ),
			nTransportAndResupplyResupplyMaxPathLenght( 30 ),
			fTransportAndResupplyTakeStorageOwnershipRadius( 100.0000f ),
			fCommonTankPitCover( 0.5000f ),
			fCommonFenceSegmentRuPrice( 100.0000f ),
			fCommonTrenchSegmentRuPrice( 100.0000f ),
			fEngineersMineAPersRuPrice( 1000.0000f ),
			fEngineersMineATankRuPrice( 2000.0000f ),
			fEngineersAntitankRuPrice( 100.0000f ),
			nEngineersComplexObstacleAntitankPeriod( 2 ),
			nTransportAndResupplyMedicalTruckHealRadius( 1000 ),
			fTransportAndResupplyMedicalTruckHealPerUpdateDuration( 0.5000f ),
			nCommonUnitEntrenchTime( 10000 ),
			fFlagsRadius( 320.0000f ),
			fFlagsPointsSpeed( 10.0000f ),
			fFlagsPlayerPointsSpeed( 10.0000f ),
			fFlagsPointsToReinforcement( 30.0000f ),
			fFlagsTimeToCapture( 5000.0000f ),
			nCommonArmorForAreaDamage( 10 ),
			fBuildingsDefaultFireplaceCoverage( 0.5000f ),
			nAviationDiveBeforeExplodeTime( 5000 ),
			nAviationDiveAfterExplodeTime( 150 ),
			nWeatherTime( 10000 ),
			nWeatherTimeRandom( 20000 ),
			nWeatherPeriod( 30000 ),
			nWeatherPeriodRandom( 30000 ),
			fWeatherFireRangeCoefficient( 0.5000f ),
			nWeatherTimeToFadeOff( 5 ),
			nAntiAviationArtilleryAimIterations( 3 ),
			fMoraleCoeffToLowMoraleWithoutOfficer( 2.0000f ),
			fInfantryMaxDistanceToThrowGrenade( 320.0000f ),
			fMoraleDispersionCoeff( 1.0000f ),
			fMoraleRelaxCoeff( 1.0000f ),
			fMoraleAimingCoeff( 1.0000f ),
			fTargetResolutionGunplaneDistanceToCenter( 10.0000f ),
			fTargetResolutionGunplaneAlphaAttack1( 1.0000f ),
			fTargetResolutionGunplaneAlphaAttack2( 0.3000f ),
			fTargetResolutionGunplaneAlphaGo( 0.0050f ),
			fTargetResolutionGunplaneAlphaKill( 1.0000f ),
			fTargetResolutionGunplaneAlphaPrice( 1.0000f ),
			fTargetResolutionGunplaneLimitTime( 1000.0000f ),
			fCommonMaxFireRangeToShootByLine( 1536.0000f ),
			fBuildingsHPPercentToEscapeFromBuilding( 0.1000f ),
			fInfantrySpyGlassAngle( 45.0000f ),
			fCommonMinRotateAngle( 0.0000f ),
			nAviationFighterPatrolTime( 0 ),
			nInfantryFullHealthTime( 2000 ),
			fEngineersRepairCostAdjust( 50.0000f ),
			nCommonTimeToAllowKillCrushedObj( 20000 ),
			nFlagsStorageCaptureTime( 5000 ),
			bUseRoundUnits( false )
		{ }
		//
		int GetTypeID() const override { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver ) override;
		int operator&( IXmlSaver &saver ) override;
		uint32_t CalcCheckSum() const override;
	};

	struct SAIGameConsts : public CResource
	{
		OBJECT_BASIC_METHODS( SAIGameConsts )
	public:
		enum { typeID = 0x11074CC0 };
	private:
		mutable uint32_t __dwCheckSum;
	public:

		struct SCommonScriptEntry
		{
		private:
			mutable uint32_t __dwCheckSum;
		public:
			NFile::CFilePath szScriptFileRef;

			SCommonScriptEntry() :
				__dwCheckSum( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			uint32_t CalcCheckSum() const;
		};
		STankPitInfo tankPits;
		std::vector< CDBPtr< SMechUnitRPGStats > > foxHoles;
		SCommonInfo common;
		std::vector< CDBPtr< SManuverDescriptor > > planeManuvers;
		SWarFogConsts warFog;
		std::vector< SCommonScriptEntry > commonScriptFileRefs;
		std::vector< SReinforcementRemap > reinforcementTypes;
		std::vector< SReinforcementExpediency > reinfExpediency;
		std::vector< SUnitTypePriority > unitTypesPriorities;
		CDBPtr< SVisObj > pParachute;
		int nRemoveParachuteTime;
		std::vector< SReinfRecycleTime > reinforcementRecycleTime;
		CDBPtr< SWeaponRPGStats > pAviationGroundCrashExplosion;
		CDBPtr< SWeaponRPGStats > pFlamethrowerDeathExplotion;
		int nGroundCrashPlaneSize;
		std::vector< STypedDeployTemplate > typedTemplates;
		bool bInfantryFormationModifiersApplyInTrenches;
		CDBPtr< SAIPropertyConsts > pAIPropertyConsts;
		bool bDisableEntrenchmentReloadSlowdown;
		float fFOWSoundDistance;
		float fFOWSoundMinBasePercentage;

		SAIGameConsts() :
			__dwCheckSum( 0 ),
			nRemoveParachuteTime( 0 ),
			nGroundCrashPlaneSize( 100 ),
			bInfantryFormationModifiersApplyInTrenches( true ),
			bDisableEntrenchmentReloadSlowdown( false ),
			fFOWSoundDistance( -1.0000f ),
			fFOWSoundMinBasePercentage( 0.1000f )
		{ }
		//
		int GetTypeID() const override { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver ) override;
		int operator&( IXmlSaver &saver ) override;
		uint32_t CalcCheckSum() const override;
	};
}
