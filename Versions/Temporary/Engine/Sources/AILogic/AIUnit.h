#pragma once

#include "CommonUnit.h"
#include "Stats_B2_M1/Actions.h"
#include "Stats_B2_M1/AnimationType.h"
#include "Stats_B2_M1/RPGStats.h"
#include "TransferredUnitExperience.h"

#include "AILogic_export.h"

#include <cstdint>

class CAntiArtillery;
class CAIUnitInfoForGeneral;
class CExistingObject;
class CExecutorUnitBonus;
class CBuilding;
struct ICollisionsCollector;

namespace NDb
{
enum EReinforcementType : int;
struct SUnitStatsModifier;
struct SAmphibianStats;
enum EUnitSpecialAbility : int;
struct SUnitBaseRPGStats;
}; // namespace NDb
enum EActionNotify : unsigned int;

class CAIUnit : public CCommonUnit
{
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	uint8_t player = {};
	bool bTrampled = {};

	// половина угла обзора
	SAIAngle wVisionAngle;

	//RPG stats
	float fHitPoints = {};

	CObj<CAntiArtillery> pAntiArtillery;
	CObj<struct IAnimUnit> pAnimUnit;

	//окоп, в котором этот юнит тусуется
	CObj<CExistingObject> pTankPit;
	bool bVirtualTankPit = {}; // unit cannot move like it is in tankpit, but don't have bonuses
	bool bIsInTankPit = {};
	NTimer::STime camouflateTime = {};

	SAIAngle wWisibility; // informatin about visibility for every player
	NTimer::STime lastAckTime = {};
	std::vector<int> visible4Party;
	std::vector<NTimer::STime> lastTimeOfVis;
	std::vector<int> bCountToDissapear;

	float fTakenDamagePower = {};
	int nGrenades = {};
	NTimer::STime targetScanRandom = {};

	bool bFreeEnemySearch = {};
	NTimer::STime creationTime = {};

	bool bAlwaysVisible = {};

	bool bRevealed = {};
	bool bQueredToReveal = {};
	NTimer::STime nextRevealCheck = {};
	CVec2 vPlaceOfReveal;
	NTimer::STime timeOfReveal = {};
	int nVisIndexInUnits = {};
	float fCamoflage = {};

	uint32_t dwForbiddenGuns = {}; // some guns will be excluded from auto attack

	bool bHoldingSector = {};
	bool bTargetingTrack = {};
	NDb::EReinforcementType eReinforcementType = {};
	CTransferredUnitExperience transferredExperience;
	bool bRestInside = {}; // mech unit inside mech unit
	SUnitProfile unitProfile;
	std::list<CPtr<CAIUnit>> targetsCache;
	NTimer::STime realScanDuration = {};
	NTimer::STime lastScanTime = {};
	bool bAmphibianWaterModifierApplied = {};

protected:
	CObjectBase *pObjInside = {};

private:
	CPtr<NDb::SUnitStatsModifier> pStatsModifiers;

	CPtr<CExecutorUnitBonus> pShootInMovementExecutor;
	bool bIgnoreAABBCoeff = {};

	int nMultipleShots = {}; // Number of shells used in one shot (default=1). Used by LinkedGrenades ability
	NTimer::STime timeLastAttackedAck = {};
	NTimer::STime timeLastAttacked = {};

	//int nAbilityLevel;
	//
	const SAINotifyHitInfo::EHitType ProcessExactHit( const SRect &combatRect,
	                                                  const CVec2 &explCoord,
	                                                  const int nRandPiercing,
	                                                  const int nRandArmor ) const;
	// sends to general request according to units's current state; quite slow.
	void UpdateUnitsRequestsForResupply();
	const NDb::SAmphibianStats *GetAmphibianStats() const;
	bool IsAmphibianWaterTile( const SVector &tile ) const;
	bool IsAmphibianWaterPoint( const CVec2 &point ) const;
	void UpdateAmphibianWaterModifier();
	void UpdateTankPitVisibility( const bool bVisibilityChanged, const bool bVisible );
	void
	InitializeShootArea( struct SShootArea *pArea, CBasicGun *pGun, const float fRangeMin, const float fRangeMax ) const;
	void CheckForReveal();
	void UpdateEnableAblitiy( const int nAbility );
	void InitAbility( const NDb::EUnitSpecialAbility nAbility );
	void UpdateUnitProfile();
	uint8_t AnalyzeTargetScanInternal( CAIUnit *pCurTarget,
	                                   const bool bDamageUpdated,
	                                   const bool bScanForObstacles,
	                                   CObjectBase *pCheckBuilding,
	                                   const bool bOnlyShootWithoutMoving );
	bool LookForTargetInRange( CAIUnit *pCurTarget,
	                           const bool bDamageUpdated,
	                           CAIUnit **pBestTarget,
	                           CBasicGun **pGun,
	                           const float fRange,
	                           const bool bIteratePlanes,
	                           const bool bIterateBuildings );
	void SetTargetScanRandom();
	const bool AttackTarget( CAIUnit *pTarget, CAIUnit *pCurTarget );

protected:
	NTimer::STime timeToDeath = {};

	CObj<CAIUnitInfoForGeneral> pUnitInfoForGeneral;

	// необходимые действия перед смертью/исчезновением с карты
	void DieTrain( const float fDamage );

	virtual void InitGuns() = 0;
	void Init( const CVec2 &center,
	           const int z,
	           const float fHP,
	           const uint16_t dir,
	           const uint8_t player,
	           ICollisionsCollector *pCollisionsCollector );

	const int GetRandArmorByDir( const int nArmorDir, const uint16_t wAttackDir, const SRect &unitRect );

	bool IsTimeToAnalyzeTargetScan() const;

	virtual bool CalculateUnitVisibility4Party( const uint8_t cParty );
	// for delayed units fade
	virtual bool UpdateUnitVisibilityForParty( const uint8_t party, const bool bVisibility );

	void UpdatePlacement( const CVec3 &vOldPosition, const uint16_t wOldDirection, const bool bNeedUpdate ) override;

	// return | of guns, that are used for anti-aviation fire and should be excluded from ordinary target scan
	virtual uint32_t InitSupportAntiAircraftGuns()
	{
		return 0;
	}

	CObjectBase *GetObjInside()
	{
		return pObjInside;
	}

public:
	CAIUnit()
		: pObjInside( 0 )
		, bTrampled( false )
		, nMultipleShots( 1 )
		, bTargetingTrack( false )
		, bIgnoreAABBCoeff( false )
		, timeLastAttackedAck( 0 )
		, timeLastAttacked( 0 )
		, bAmphibianWaterModifierApplied( false )
	{
	}

	virtual void Init( const CVec2 &center,
	                   const int z,
	                   const SUnitBaseRPGStats *pStats,
	                   const float fHP,
	                   const uint16_t dir,
	                   const uint8_t player,
	                   ICollisionsCollector *pCollisionsCollector ) = 0;
	void InitSpecialAbilities( int nFromLevel = 0 ); // Parameter is used on level-up
	static CAIUnit *GetUnitByUniqueID( const int nUniqueID );
	int operator&( IBinSaver &f ) override;

	virtual void PrepareToDelete();

	uint32_t GetForbiddenGuns() const
	{
		return dwForbiddenGuns;
	}

	// для updater-а
	void GetNewUnitInfo( struct SNewUnitInfo *pNewUnitInfo ) override;
	void GetRPGStats( struct SAINotifyRPGStats *pStats ) override;
	void GetPlacement( struct SAINotifyPlacement *pPlacement, const NTimer::STime timeDiff ) override;

	const NTimer::STime GetTimeOfDeath() const override
	{
		return timeToDeath;
	}

	virtual const CVec2 GetGunCenter( const int nGun, const int nPlatform ) const;
	// когда танк въехал в TankPit или его окопали вызывается эта функция
	void SetInTankPit( CExistingObject *pTankPit );
	void SetOffTankPit();

	void SetInVirtualTankPit()
	{
		bVirtualTankPit = true;
	}

	bool IsVirtualTankPit() const
	{
		return bVirtualTankPit;
	}

	class CExistingObject *GetTankPit() const
	{
		return pTankPit;
	}

	bool IsInTankPit() const
	{
		return bIsInTankPit;
	}

	void SetVisionAngle( const uint16_t wAngle )
	{
		wVisionAngle = wAngle;
	}

	uint16_t GetVisionAngle() const
	{
		return wVisionAngle;
	}

	//
	const SUnitBaseRPGStats *GetStats() const override = 0;

	// в месте, где его никто не видит и он никого не видит
	virtual bool IsInSolidPlace() const
	{
		return false;
	}

	virtual bool IsInFirePlace() const
	{
		return false;
	}

	bool IsFree() const override
	{
		return true;
	}

	// обработка команд
	void Segment() override;
	void FreezeSegment() override;

	bool IsPossibleChangeAction() const
	{
		return IsAlive();
	}

	void Die( const bool fromExplosion, const float fDamage ) override;
	void Disappear() override;

	// виден ли данной дипломатической стороной
	const bool IsVisible( const uint8_t party ) const override;

	void SetVisibility( uint8_t party, bool bVisible )
	{
		visible4Party[party] = bVisible;
	}

	// для отложенных updates
	void GetTilesForVisibility( CTilesSet *pTiles ) const override;
	bool ShouldSuspendAction( const EActionNotify &eAction ) const override;
	const uint32_t GetNormale( const CVec2 &vCenter ) const override;
	virtual const uint32_t GetNormale() const;

	// CBasePathUnit
	const float GetTurnSpeed() const override;

	const float GetMaxPossibleSpeed() const override
	{
		float speed = GetStatsModifier()->speed.Get( GetStats()->fSpeed );
		if ( !IsGoForward() && CanGoBackward() && IsMech() )
		{
			speed *= GetStats()->fReverseSpeedModifier;
		}
		return speed;

		//return GetStatsModifier()->speed.Get( GetStats()->fSpeed );
	}

	// Group forward move should use the "forward cap" and ignore temporary reverse-direction state.
	const float GetMaxPossibleForwardSpeed() const override
	{
		return GetStatsModifier()->speed.Get( GetStats()->fSpeed );
	}

	// Group reverse move should use the reverse cap even before the path switches direction.
	const float GetMaxPossibleBackwardSpeed() const override
	{
		const float speed = GetMaxPossibleForwardSpeed();
		return IsMech() && CanGoBackward() ? speed * GetStats()->fReverseSpeedModifier : speed;
	}

	const float GetPassability() const override;

	const int GetBoundTileRadius() const override
	{
		return GetStats()->nBoundTileRadius;
	}

	const bool CanMoveBetweenTiles( const SVector &fromTile, const SVector &toTile ) const override;

	//virtual struct IStaticPath* ( const CVec2 &vStartPoint, const CVec2 &vFinishPoint, struct IPointChecking *pPointChecking );

	const CVec2 &GetAABBHalfSize() const override
	{
		return GetStats()->vAABBHalfSize;
	}

	virtual const float GetVisZ() const;
	const uint16_t GetDirAtTheBeginning() const;

	// CPathUnit
	virtual bool IsInOneTrain( CBasePathUnit *pUnit ) const
	{
		return false;
	}

	const bool IsTrain() const override;

	//
	const float GetHitPoints() const
	{
		return fHitPoints;
	}

	void IncreaseHitPoints( const float fInc = 1 );
	virtual void TakeDamage( const float fDamage,
	                         const SWeaponRPGStats::SShell *pShell,
	                         const int nPlayerOfShoot,
	                         CAIUnit *pShotUnit );
	// true при попадании
	virtual bool ProcessCumulativeExpl( class CExplosion *pExpl, const int nArmorDir, const bool bFromExpl );
	// true при точном попадании
	virtual bool
	ProcessBurstExpl( class CExplosion *pExpl, const int nArmorDir, const float fRadius, const float fSmallRadius );
	// true при попадании
	virtual bool ProcessAreaDamage( const class CExplosion *pExpl,
	                                const int nArmorDir,
	                                const float fRadius,
	                                const float fSmallRadius );

	// вероятность, с которой нанесётся damage при попадании
	virtual const float GetCover() const;
	virtual bool IsSavedByCover() const;

	class CTurret *GetTurret( const int nTurret ) const override
	{
		NI_ASSERT( false, "Wrong call of get turret" );
		return 0;
	}

	// для стрельбы
	virtual void GetShotInfo( struct SAINotifyInfantryShot *pShotInfo ) const
	{
		NI_ASSERT( false, "Wrong call of GetShotInfo" );
	}

	virtual void GetShotInfo( struct SAINotifyMechShot *pShotInfo ) const
	{
		NI_ASSERT( false, "Wrong call of GetShotInfo" );
	}

	virtual const EActionNotify GetShootAction() const = 0;
	virtual const EActionNotify GetAimAction() const = 0;
	const EActionNotify GetDieAction() const override = 0;
	virtual const EActionNotify GetIdleAction() const = 0;
	// для Move есть типы
	virtual const EActionNotify GetMovingAction() const = 0;

	const bool CanMove() const override;
	const bool CanMoveCritical() const override;
	// может ли повернуться в принципе (может быть, понадобиться деинсталляция)
	const bool CanRotate() const override;

	void SetCamoulfage() override;
	const float GetCamouflage() const;
	void RemoveCamouflage( ECamouflageRemoveReason eReason ) override;

	virtual bool IsCamoulflated() const
	{
		return fCamoflage < 1.0f;
	}

	//Camouflage needs to be removed, probably. Is made obsolete by the following method:
	void ApplyStatsModifier( const NDb::SUnitStatsModifier *pModifier, const bool bForward );

	const NDb::SUnitStatsModifier *GetStatsModifier() const
	{
		return pStatsModifiers;
	}

	bool IsInWater() const;
	const float GetAmphibianWaterCoeff( const CVec2 &point ) const;

	bool CanCommandBeExecuted( class CAICommand *pCommand ) override;
	bool CanCommandBeExecutedByStats( class CAICommand *pCommand ) override;
	bool CanCommandBeExecutedByStats( int nCmd ) const override;

	const uint8_t GetPlayer() const override
	{
		return player;
	}

	// сменить дипломатию с корректным update в units
	void ChangePlayer( const uint8_t cPlayer ) override;
	void PrepareTransfer( int nTargetPlayer );
	void UpdateTransferredExperience();

	CCommonUnit *GetContainingUnit() const
	{
		// Unloaded soldiers can retain pObjInside until saving. Check their actual inside state,
		// or the separate mechanical-cargo flag, before following that potentially stale pointer.
		if ( !bRestInside && IsFree() )
		{
			return nullptr;
		}
		// Buildings and trenches are not transferable, but a passenger must stay with its carrier.
		return dynamic_cast<CCommonUnit *>( pObjInside );
	}

	bool HasTransferredExperience() const
	{
		return transferredExperience.IsTransferred();
	}

	// просто поставить другую дипломатию
	void SetPlayer( const uint8_t cPlayer )
	{
		player = cPlayer;
	}

	virtual bool InVisCone( const CVec2 &point ) const
	{
		return true;
	}

	const float GetSightRadius() const override;
	float GetRemissiveCoeff() const;

	const int GetNAIGroup() const override
	{
		return GetNGroup();
	}

	const NTimer::STime GetTimeToCamouflage() const override;
	virtual void AnalyzeCamouflage();
	virtual void StartCamouflating();

	void CreateAntiArtillery( const float fMaxRevealRadius );
	void Fired( const float fGunRadius, const int nGun ) override;
	virtual NTimer::STime GetDisappearInterval() const;

	// бонусы
	virtual const float GetDispersionBonus() const
	{
		return 1.0f;
	}

	virtual const void SetDispersionBonus( const float fBonus ) {}

	virtual const float GetRelaxTimeBonus() const
	{
		return 1.0f;
	}

	virtual const float GetFireRateBonus() const
	{
		return 1.0f;
	}

	virtual const float GetAimTimeBonus() const;

	virtual void SetAmbush();
	virtual void RemoveAmbush();

	virtual void GetFogInfo( struct SWarFogUnitInfo *pInfo ) const;
	bool HasSpecialSights() const;
	void GetShootAreas( struct SShootAreas *pShootAreas, int *pnAreas ) const override;
	void WarFogChanged() override;

	// update изображения shoot area или range area
	void UpdateArea( const EActionNotify eAction ) override;

	const EAIClasses GetAIPassabilityClass() const override
	{
		return (EAIClasses)GetStats()->nAIPassabilityClass;
	}

	// патроны
	const int GetNCommonGuns() const;
	int GetCommonGunIndex( const CBasicGun *pGun ) const;
	const SBaseGunRPGStats &GetCommonGunStats( const int nCommonGun ) const;
	virtual int GetNAmmo( const int nCommonGun ) const;
	// nAmmo со знаком
	virtual void ChangeAmmo( const int nCommonGun, const int nAmmo );
	virtual bool IsCommonGunFiring( const int nCommonGun ) const;

	const float GetSmoothTurnThreshold() const override
	{
		return 0.3f;
	}

	// заметен ли юнитом pUnit, радиус обнаружения fNoticeRadius
	virtual bool IsNoticableByUnit( class CCommonUnit *pUnit, const float fNoticeRadius );

	const int ChooseFatality( const float fDamage, NDb::EAnimationType *pDeathAnimType );

	void NullCollisions();

	void SendAcknowledgement( EUnitAckType ack, bool bForce = false ) override;
	// ack для команды pCommand
	void SendAcknowledgement( CAICommand *pCommand, EUnitAckType ack, bool bForce = false ) override;

	// коллизится ли с движущимися объектами
	const ECollidingType GetCollidingType( CBasePathUnit *pUnit ) const override;

	const int GetMinArmor() const override;
	const int GetMaxArmor() const override;
	const int GetMinPossibleArmor( const int nSide ) const override;
	const int GetMaxPossibleArmor( const int nSide ) const override;
	const int GetArmor( const int nSide ) const override;
	const int GetRandomArmor( const int nSide ) const override;

	//CRAP{ until SuspendedPoint unrealized in new Pathfinding
	virtual bool HasSuspendedPoint() const
	{
		return false;
	}

	//CRAP}

	//for bored condition
	void UnRegisterAsBored( const enum EUnitAckType eBoredType ) override;
	void RegisterAsBored( const enum EUnitAckType eBoredType ) override;

	virtual class CUnitGuns *GetGuns() = 0;
	virtual const class CUnitGuns *GetGuns() const = 0;
	EUnitAckType GetGunsRejectReason() const override;
	bool DoesExistRejectGunsReason( const EUnitAckType &ackType ) const;

	// целеразрешение
	// скорость убийства юнита с pStats из pGun
	const float GetKillSpeed( const SHPObjectRPGStats *pStats, const CVec2 &vCenter, CBasicGun *pGun ) const;
	const float GetKillSpeed( const SHPObjectRPGStats *pStats, const CVec2 &vCenter, const uint32_t dwGuns ) const;
	// скорость убийства юнита из наилучшего gun
	const float GetKillSpeed( class CAIUnit *pEnemy ) const override;
	// скорость убийства юнита из pGun
	virtual const float GetKillSpeed( class CAIUnit *pEnemy, class CBasicGun *pGun ) const;
	// скорость убийства юнита из набора Gun, номера задаются маской
	virtual const float GetKillSpeed( CAIUnit *pEnemy, const uint32_t dwGuns ) const;
	void UpdateTakenDamagePower( const float fUpdate );

	const float GetTakenDamagePower() const
	{
		return fTakenDamagePower;
	}

	// обнулить время для периодов сканирования
	void ResetTargetScan() override;
	// просканировать, если пора; если нашли цель, то атаковать
	// возвращает: в младшем бите - была ли найдена цель, во втором бите - было ли произведено сканирование
	uint8_t AnalyzeTargetScan( CAIUnit *pCurTarget,
	                           const bool bDamageUpdated,
	                           const bool bScanForObstacles,
	                           CObjectBase *pCheckBuilding = 0 ) override;
	uint8_t AnalyzeTargetScanWithoutMoving( CAIUnit *pCurTarget,
	                                        const bool bDamageUpdated,
	                                        const bool bScanForObstacles,
	                                        CObjectBase *pCheckBuilding = 0 );
	// поискать цель, текущая цель для атаки - pCurTarget
	void LookForTarget( CAIUnit *pCurTarget,
	                    const bool bDamageUpdated,
	                    CAIUnit **pBestTarget,
	                    class CBasicGun **pGun ) override;
	// поискать цель вдалеке для артиллерийского обстрела, текущая цель для атаки - pCurTarget
	virtual void
	LookForFarTarget( CAIUnit *pCurTarget, const bool bDamageUpdated, CAIUnit **pBestTarget, class CBasicGun **pGun );
	// для того, чтобы искать врагов не только в секторе атаки, а во всём круге
	void SetCircularAttack( const bool bCanAttack );
	// поискать препятствие.
	struct IObstacle *LookForObstacle() override;

	void UpdateNAttackingGrenages( const int nUpdate )
	{
		nGrenades += nUpdate;
		NI_ASSERT( nGrenades >= 0, "Wrong number of grenades" );
	}

	const int GetNAttackingGrenages() const
	{
		return nGrenades;
	}

	// установать в curTime время для периодов выбора gun
	virtual void ResetGunChoosing();
	// если пора перевыбрать gun, то перевыбрать
	CBasicGun *AnalyzeGunChoose( CAIUnit *pEnemy );

	void EnemyKilled( CAIUnit *pEnemy );

	bool CanMoveForGuard() const override
	{
		return CanMove() && !GetStats()->IsTrain();
	}

	// время, через которое general забудет о невидимом юните
	virtual const float GetTimeToForget() const;
	CAIUnitInfoForGeneral *GetUnitInfoForGeneral() const;
	void SetLastVisibleTime( const NTimer::STime time );

	// радиус, в котром сканируются цели
	const float GetTargetScanRadius() override;

	// юнит находиться в свободном поиске целей
	virtual bool IsFreeEnemySearch() const
	{
		return bFreeEnemySearch;
	}

	// количество сегментнов, прошедшее с прошлого вызова SecondSegment
	virtual const float GetPathSegmentsPeriod() const
	{
		return 1.0f;
	}

	float GetPriceMax() const override;
	// for Saving/Loading of static members
	friend class CStaticMembers;

	const NTimer::STime GetBehUpdateDuration() const override;

	// killed: this unit + all units inside
	virtual void SendNTotalKilledUnits( const int nPlayerOfShoot,
	                                    NDb::EReinforcementType eKillerType,
	                                    NDb::EReinforcementType eDeadType );

	//	const SAIExpLevel::SLevel& GetExpLevel() const;

	void UnitCommand( CAICommand *pCommand, bool bPlaceInQueue, bool bOnlyThisUnitCommand ) override;

	//для посылки генералу информации о видимых врагах, а также посылки update о видимости юнита
	void CalcVisibility( const bool bIgnoreTime );
	// клиент зависима!
	const bool IsVisibleByPlayer() override;
	bool CalculateUnitVisibility4PartyInner( const uint8_t party ) const;

	// залокать unit ( если уже был залокана, то старый lock исчезает )
	void Lock( const CBasicGun *pGun ) override;
	// unlock unit ( если залокан другим gun-ом, то ничего не делается )
	void Unlock( const CBasicGun *pGun ) override;
	// залокан ли каким-либо gun-ом, не равным pGun
	bool IsLocked( const CBasicGun *pGun ) const override;

	// смена текущего типа патронов.
	void SetActiveShellType( const NDb::SWeaponRPGStats::SShell::EShellDamageType eShellType );

	// for planes
	//void InitAviationPath();

	virtual void InstallAction( const EActionNotify eInstallAction, bool bAlreadyDone = false ) {}

	virtual bool IsUninstalled() const
	{
		return true;
	}

	virtual bool IsInstalled() const
	{
		return true;
	}

	void AnimationSet( int nAnimation ) override;
	void AnimationSegment() override;
	void Moved();
	void Stopped();
	void StopCurAnimation();

	virtual class CArtillery *GetArtilleryIfCrew() const
	{
		return 0;
	}

	virtual void TrackDamagedState( const bool bTrackDamaged ) {}

	void WantedToReveal( CAIUnit *pWhoRevealed );
	bool IsRevealed() const;

	const bool IsInfantry() const override;

	virtual bool IsHelicopter() const
	{
		return false;
	}

	// обстрелян юнитом pUnit
	virtual void Grazed( CAIUnit *pUnit ) {}

	void NullCreationTime()
	{
		creationTime = 0;
	}

	const int GetNVisIndexInUnits() const
	{
		return nVisIndexInUnits;
	}

	void SetNVisIndexInUnits( const int _nVisIndexInUnits )
	{
		nVisIndexInUnits = _nVisIndexInUnits;
	}

	bool CanMoveAfterUserCommand() const override;
	void NotifyAbilityRun( class CAICommand *pCommand ) override;

	bool IsHoldingSector() const
	{
		return bHoldingSector;
	}

	void SetHoldSector();
	void ResetHoldSector();

	void SetTrackTargeting( bool bOn );

	bool IsTargetingTrack() const
	{
		return bTargetingTrack;
	}

	void SetReinforcementType( const NDb::EReinforcementType eType );

	const NDb::EReinforcementType GetReinforcementType() const
	{
		return eReinforcementType;
	}

	void GetEntranceStateInfo( struct SAINotifyEntranceState *pInfo ) const override;
	bool IsRestInside() const;
	AILOGIC_EXPORT void SetRestInside( const bool bInside, CAIUnit *pTransport );

	const bool
	CanShootInMovement() override; // For aviation and ShootInMovement special ability (shoot from priority-0 guns)
	const int GetXPLevel();
	const int GetAbilityLevel(); // Introduced for leaders
	/*
	void SetAbilityLevel( const int _nLevel ) { nAbilityLevel = _nLevel; }
	void ResetAbilityLevel() { SetAbilityLevel( -1 ); }
	*/

	void SetIgnoreAABBCoeff( const bool bIgnore );

	bool IsIgnoreAABBCoeff() const
	{
		return bIgnoreAABBCoeff;
	}

	void SetMultiShot( const int nShells )
	{
		nMultipleShots = nShells;
	}

	const int GetMultiShot() const
	{
		return nMultipleShots;
	}

	const SUnitProfile &GetUnitProfile() const override;
	const SRect &GetUnitRect() const override;

	const float GetTurnRadius() const override
	{
		return GetStats()->GetTurnRadius();
	}

	// CBasePathUnit
	void SetDirection( const uint16_t wDirection );
	const CVec2 GetCenterShift() const override;
	const bool IsRound() const override;
	void UnitTrampled( const CBasePathUnit *pTramplerUnit ) override;
	const bool CanTurnRound() const override;

	bool IsTrampled() const
	{
		return bTrampled;
	}

	//
	void UpdateVisibilityForced();

	const bool IsStaticUnit() const override
	{
		return GetStats()->fSpeed == 0.0f &&
		       (std::max)( GetStats()->vAABBHalfSize.x, GetStats()->vAABBHalfSize.y ) > 128.0f;
	}

	// real implementation of the method
	const NDb::SUnitSpecialAblityDesc *GetUnitAbilityDesc( const NDb::EUnitSpecialAbility eType ) override;
	const bool CanLockTiles() const override;
	void UpdateCrewAndTruckVisibility( uint8_t party, bool bNewVisibility );
	void CheckAmmoStatus();
};
