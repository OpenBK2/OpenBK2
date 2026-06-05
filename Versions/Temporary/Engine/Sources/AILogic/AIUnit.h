#pragma once

#include "CommonUnit.h"
#include "Stats_B2_M1/Actions.h"
#include "Stats_B2_M1/RPGStats.h"

class CAntiArtillery;
class CAIUnitInfoForGeneral;
class CExistingObject;
class CExecutorUnitBonus;
class CBuilding;
struct ICollisionsCollector;

namespace NDb
{
	enum EReinforcementType;
	struct SUnitStatsModifier;
	struct SAmphibianStats;
	enum EUnitSpecialAbility;
	struct SUnitBaseRPGStats;
};
enum EActionNotify;

class CAIUnit: public CCommonUnit
{
	BYTE player;
	bool bTrampled;

	// половина угла обзора
	SAIAngle wVisionAngle;
	
	//RPG stats
	float fHitPoints;

	CObj<CAntiArtillery> pAntiArtillery;
	CObj<struct IAnimUnit> pAnimUnit;

	//окоп, в котором этот юнит тусуется
	CObj<CExistingObject> pTankPit; 
	bool bVirtualTankPit;									// unit cannot move like it is in tankpit, but don't have bonuses
	bool bIsInTankPit;
	NTimer::STime camouflateTime;

	SAIAngle wWisibility;											// informatin about visibility for every player
	NTimer::STime lastAckTime;
	std::vector<int> visible4Party;
	std::vector<NTimer::STime> lastTimeOfVis;
	std::vector<int> bCountToDissapear;

	float fTakenDamagePower;
	int nGrenades;
	NTimer::STime targetScanRandom;

	bool bFreeEnemySearch;
	NTimer::STime creationTime;
	
	bool bAlwaysVisible;
	
	bool bRevealed;
	bool bQueredToReveal;
	NTimer::STime nextRevealCheck;
	CVec2 vPlaceOfReveal;
	NTimer::STime timeOfReveal;
	int nVisIndexInUnits;
	float fCamoflage;
	
	DWORD dwForbiddenGuns;									// some guns will be excluded from auto attack
	
	bool bHoldingSector;
	bool bTargetingTrack;
	NDb::EReinforcementType eReinforcementType;
	bool bRestInside; // mech unit inside mech unit
	SUnitProfile unitProfile;
	std::list< CPtr<CAIUnit> > targetsCache;
	NTimer::STime realScanDuration;
	NTimer::STime lastScanTime;
	bool bAmphibianWaterModifierApplied;
protected:
	CObjectBase *pObjInside;
private:

	CPtr<NDb::SUnitStatsModifier> pStatsModifiers;

	CPtr<CExecutorUnitBonus> pShootInMovementExecutor;
	bool bIgnoreAABBCoeff;

	int nMultipleShots;					// Number of shells used in one shot (default=1). Used by LinkedGrenades ability
	NTimer::STime timeLastAttackedAck;
	NTimer::STime timeLastAttacked;

	//int nAbilityLevel;
	//
	const SAINotifyHitInfo::EHitType ProcessExactHit( const SRect &combatRect, const CVec2 &explCoord, const int nRandPiercing, const int nRandArmor ) const;
	// sends to general request according to units's current state; quite slow.
	void UpdateUnitsRequestsForResupply();
	const NDb::SAmphibianStats* GetAmphibianStats() const;
	bool IsAmphibianWaterTile( const SVector &tile ) const;
	bool IsAmphibianWaterPoint( const CVec2 &point ) const;
	void UpdateAmphibianWaterModifier();
	void UpdateTankPitVisibility( const bool bVisibilityChanged, const bool bVisible  );
	void InitializeShootArea( struct SShootArea *pArea, CBasicGun *pGun, const float fRangeMin, const float fRangeMax ) const;
	void CheckForReveal();
	void UpdateEnableAblitiy( const int nAbility );
	void InitAbility( const NDb::EUnitSpecialAbility nAbility );
	void UpdateUnitProfile();
	BYTE AnalyzeTargetScanInternal( CAIUnit *pCurTarget, const bool bDamageUpdated, const bool bScanForObstacles, CObjectBase *pCheckBuilding, const bool bOnlyShootWithoutMoving );
	bool LookForTargetInRange(  CAIUnit *pCurTarget, const bool bDamageUpdated, CAIUnit **pBestTarget, CBasicGun **pGun, 
		const float fRange, const bool bIteratePlanes, const bool bIterateBuildings );
	void SetTargetScanRandom();
	const bool AttackTarget( CAIUnit *pTarget, CAIUnit *pCurTarget );
protected:
	NTimer::STime timeToDeath;

	CObj<CAIUnitInfoForGeneral> pUnitInfoForGeneral;

	// необходимые действия перед смертью/исчезновением с карты
	void DieTrain( const float fDamage );
	
	virtual void InitGuns() = 0;
	void Init( const CVec2 &center, const int z, const float fHP, const WORD dir, const BYTE player, ICollisionsCollector *pCollisionsCollector );

	const int GetRandArmorByDir( const int nArmorDir, const WORD wAttackDir, const SRect &unitRect );

	bool IsTimeToAnalyzeTargetScan() const;

	virtual bool CalculateUnitVisibility4Party( const BYTE cParty );
	// for delayed units fade
	virtual bool UpdateUnitVisibilityForParty( const BYTE party, const bool bVisibility );

	virtual void UpdatePlacement( const CVec3 &vOldPosition, const WORD wOldDirection, const bool bNeedUpdate );
	// return | of guns, that are used for anti-aviation fire and should be excluded from ordinary target scan
	virtual DWORD InitSupportAntiAircraftGuns() { return 0; }
	CObjectBase * GetObjInside() { return pObjInside; }
public:
	CAIUnit() : pObjInside( 0 ), bTrampled( false ), nMultipleShots(1), bTargetingTrack(false), bIgnoreAABBCoeff(false), timeLastAttackedAck( 0 ), timeLastAttacked( 0 ), bAmphibianWaterModifierApplied( false ) { }
	virtual void Init( const CVec2 &center, const int z, const SUnitBaseRPGStats *pStats, const float fHP, const WORD dir, const BYTE player, ICollisionsCollector *pCollisionsCollector ) = 0;
	void InitSpecialAbilities( int nFromLevel = 0 );			// Parameter is used on level-up
	static CAIUnit * GetUnitByUniqueID( const int nUniqueID );
	int operator&( IBinSaver &f );

	virtual void PrepareToDelete();
	DWORD GetForbiddenGuns() const { return dwForbiddenGuns; }
	// для updater-а
	virtual void GetNewUnitInfo( struct SNewUnitInfo *pNewUnitInfo );
	virtual void GetRPGStats( struct SAINotifyRPGStats *pStats );
	virtual void GetPlacement( struct SAINotifyPlacement *pPlacement, const NTimer::STime timeDiff );
	virtual const NTimer::STime GetTimeOfDeath() const { return timeToDeath; }

	virtual const CVec2 GetGunCenter( const int nGun, const int nPlatform ) const;
		// когда танк въехал в TankPit или его окопали вызывается эта функция
	void SetInTankPit( CExistingObject *pTankPit );
	void SetOffTankPit();
	void SetInVirtualTankPit() { bVirtualTankPit = true; }
	bool IsVirtualTankPit() const { return bVirtualTankPit; }
	class CExistingObject* GetTankPit() const { return pTankPit; }
	bool IsInTankPit() const { return bIsInTankPit; }

	void SetVisionAngle( const WORD wAngle ) { wVisionAngle = wAngle; }
	WORD GetVisionAngle() const { return wVisionAngle; }

	//
	virtual const SUnitBaseRPGStats* GetStats() const = 0;

	// в месте, где его никто не видит и он никого не видит
	virtual bool IsInSolidPlace() const { return false; }
	virtual bool IsInFirePlace() const { return false; }
	virtual bool IsFree() const { return true; }

	// обработка команд
	virtual void Segment();
	virtual void FreezeSegment();
	bool IsPossibleChangeAction() const	{ return IsAlive(); }
	virtual void Die( const bool fromExplosion, const float fDamage );
	virtual void Disappear();

	// виден ли данной дипломатической стороной
	virtual const bool IsVisible( const BYTE party ) const;
	void SetVisibility( BYTE party, bool bVisible ) { visible4Party[party] = bVisible; }
	// для отложенных updates
	virtual void GetTilesForVisibility( CTilesSet *pTiles ) const;
	virtual bool ShouldSuspendAction( const EActionNotify &eAction ) const;
	//virtual const DWORD GetNormale( const CVec2 &vCenter ) const;
	virtual const DWORD GetNormale() const;

	// CBasePathUnit
	virtual const float GetTurnSpeed() const;
	virtual const float GetMaxPossibleSpeed() const 
	{ 
		float speed = GetStatsModifier()->speed.Get(GetStats()->fSpeed);
		if (!IsGoForward() && CanGoBackward() && IsMech())
			speed *= GetStats()->fReverseSpeedModifier;
		return speed;

		//return GetStatsModifier()->speed.Get( GetStats()->fSpeed ); 
	}
	// Group forward move should use the "forward cap" and ignore temporary reverse-direction state.
	virtual const float GetMaxPossibleForwardSpeed() const { return GetStatsModifier()->speed.Get(GetStats()->fSpeed); }
	virtual const float GetPassability() const;
	virtual const int GetBoundTileRadius() const { return GetStats()->nBoundTileRadius; }
	virtual const bool CanMoveBetweenTiles( const SVector &fromTile, const SVector &toTile ) const;
	//virtual struct IStaticPath* ( const CVec2 &vStartPoint, const CVec2 &vFinishPoint, struct IPointChecking *pPointChecking );

	virtual const CVec2 &GetAABBHalfSize() const { return GetStats()->vAABBHalfSize; }
	virtual const float GetVisZ() const;
	const WORD GetDirAtTheBeginning() const;

	// CPathUnit
		virtual bool IsInOneTrain( CBasePathUnit *pUnit ) const { return false; }
	virtual const bool IsTrain() const;

	//
	const float GetHitPoints() const { return fHitPoints; }
	void IncreaseHitPoints( const float fInc = 1 );
	virtual void TakeDamage( const float fDamage, const SWeaponRPGStats::SShell *pShell, const int nPlayerOfShoot, CAIUnit *pShotUnit );
	// true при попадании
	virtual bool ProcessCumulativeExpl( class CExplosion *pExpl, const int nArmorDir, const bool bFromExpl );
	// true при точном попадании
	virtual bool ProcessBurstExpl( class CExplosion *pExpl, const int nArmorDir, const float fRadius, const float fSmallRadius );
	// true при попадании
	virtual bool ProcessAreaDamage( const class CExplosion *pExpl, const int nArmorDir, const float fRadius, const float fSmallRadius );

	// вероятность, с которой нанесётся damage при попадании
	virtual const float GetCover() const;
	virtual bool IsSavedByCover() const;

	virtual class CTurret* GetTurret( const int nTurret ) const { NI_ASSERT( false, "Wrong call of get turret" ); return 0; }
	
	// для стрельбы
	virtual void GetShotInfo( struct SAINotifyInfantryShot *pShotInfo ) const { NI_ASSERT( false, "Wrong call of GetShotInfo" ); }
	virtual void GetShotInfo( struct SAINotifyMechShot *pShotInfo ) const { NI_ASSERT( false, "Wrong call of GetShotInfo" ); }
	virtual const EActionNotify GetShootAction() const = 0;
	virtual const EActionNotify GetAimAction() const = 0;
	virtual const EActionNotify GetDieAction() const = 0;
	virtual const EActionNotify GetIdleAction() const = 0;
	// для Move есть типы
	virtual const EActionNotify GetMovingAction() const = 0;

	virtual const bool CanMove() const;
	virtual const bool CanMoveCritical() const;
	// может ли повернуться в принципе (может быть, понадобиться деинсталляция)
	virtual const bool CanRotate() const;
	
	virtual void SetCamoulfage();
	const float GetCamouflage() const;
	virtual void RemoveCamouflage( ECamouflageRemoveReason eReason );
	virtual bool IsCamoulflated() const { return fCamoflage < 1.0f; }

	//Camouflage needs to be removed, probably. Is made obsolete by the following method:
	void ApplyStatsModifier( const NDb::SUnitStatsModifier * pModifier, const bool bForward );
	const NDb::SUnitStatsModifier * GetStatsModifier() const { return pStatsModifiers; }
	bool IsInWater() const;
	const float GetAmphibianWaterCoeff( const CVec2 &point ) const;

	virtual bool CanCommandBeExecuted( class CAICommand *pCommand );
	virtual bool CanCommandBeExecutedByStats( class CAICommand *pCommand );
	virtual bool CanCommandBeExecutedByStats( int nCmd ) const;
	
	virtual const BYTE GetPlayer() const { return player; }
	// сменить дипломатию с корректным update в units
	virtual void ChangePlayer( const BYTE cPlayer );
	// просто поставить другую дипломатию
	void SetPlayer( const BYTE cPlayer ) { player = cPlayer; }

	virtual bool InVisCone( const CVec2 &point ) const { return true; }
	virtual const float GetSightRadius() const;
	float GetRemissiveCoeff() const;
	virtual const int GetNAIGroup() const { return GetNGroup(); }
	
	virtual const NTimer::STime GetTimeToCamouflage() const;
	virtual void AnalyzeCamouflage();
	virtual void StartCamouflating();


	void CreateAntiArtillery( const float fMaxRevealRadius );
	virtual void Fired( const float fGunRadius, const int nGun );
	virtual NTimer::STime GetDisappearInterval() const;

	// бонусы
	virtual const float GetDispersionBonus() const { return 1.0f; }
	virtual const void SetDispersionBonus( const float fBonus ) {}
	virtual const float GetRelaxTimeBonus() const { return 1.0f; }
	virtual const float GetFireRateBonus() const { return 1.0f; }
	virtual const float GetAimTimeBonus() const;
	
	virtual void SetAmbush();
	virtual void RemoveAmbush();
	
	virtual void GetFogInfo( struct SWarFogUnitInfo *pInfo ) const;
	virtual void GetShootAreas( struct SShootAreas *pShootAreas, int *pnAreas ) const;
	virtual void WarFogChanged();

	// update изображения shoot area или range area
	virtual void UpdateArea( const EActionNotify eAction );
	
	virtual const EAIClasses GetAIPassabilityClass() const { return (EAIClasses)GetStats()->nAIPassabilityClass; }
	
	// патроны
	const int GetNCommonGuns() const;
	const SBaseGunRPGStats& GetCommonGunStats( const int nCommonGun ) const;
	virtual int GetNAmmo( const int nCommonGun ) const;
	// nAmmo со знаком
	virtual void ChangeAmmo( const int nCommonGun, const int nAmmo );
	virtual bool IsCommonGunFiring( const int nCommonGun ) const;

	virtual const float GetSmoothTurnThreshold() const { return 0.3f; }
	
	// заметен ли юнитом pUnit, радиус обнаружения fNoticeRadius
	virtual bool IsNoticableByUnit( class CCommonUnit *pUnit, const float fNoticeRadius );
	
	const int ChooseFatality( const float fDamage );
	
	void NullCollisions();

	virtual void SendAcknowledgement( EUnitAckType ack, bool bForce = false );
	// ack для команды pCommand
	virtual void SendAcknowledgement( CAICommand *pCommand, EUnitAckType ack, bool bForce = false );
	
	// коллизится ли с движущимися объектами
	virtual const ECollidingType GetCollidingType( CBasePathUnit *pUnit ) const;
	
	virtual const int GetMinArmor() const;
	virtual const int GetMaxArmor() const;
	virtual const int GetMinPossibleArmor( const int nSide ) const;
	virtual const int GetMaxPossibleArmor( const int nSide ) const;
	virtual const int GetArmor( const int nSide ) const;
	virtual const int GetRandomArmor( const int nSide ) const;
	
	//CRAP{ until SuspendedPoint unrealized in new Pathfinding
	virtual bool HasSuspendedPoint() const { return false; }
	//CRAP}
  
	//for bored condition
	virtual void UnRegisterAsBored( const enum EUnitAckType eBoredType );
	virtual void RegisterAsBored( const enum EUnitAckType eBoredType );
	
	virtual class CUnitGuns* GetGuns() = 0;	
	virtual const class CUnitGuns* GetGuns() const = 0;
	virtual EUnitAckType GetGunsRejectReason() const;
	bool DoesExistRejectGunsReason( const EUnitAckType &ackType ) const;
	
	// целеразрешение
	// скорость убийства юнита с pStats из pGun
	const float GetKillSpeed( const SHPObjectRPGStats *pStats, const CVec2 &vCenter, CBasicGun *pGun ) const;
	const float GetKillSpeed( const SHPObjectRPGStats *pStats, const CVec2 &vCenter, const DWORD dwGuns ) const;
	// скорость убийства юнита из наилучшего gun
	virtual const float GetKillSpeed( class CAIUnit *pEnemy ) const;
	// скорость убийства юнита из pGun
	virtual const float GetKillSpeed( class CAIUnit *pEnemy, class CBasicGun *pGun ) const;
	// скорость убийства юнита из набора Gun, номера задаются маской
	virtual const float GetKillSpeed( CAIUnit *pEnemy, const DWORD dwGuns ) const;
	void UpdateTakenDamagePower( const float fUpdate );
	const float GetTakenDamagePower() const { return fTakenDamagePower; }
	
	// обнулить время для периодов сканирования
	virtual void ResetTargetScan();
	// просканировать, если пора; если нашли цель, то атаковать
	// возвращает: в младшем бите - была ли найдена цель, во втором бите - было ли произведено сканирование
	virtual BYTE AnalyzeTargetScan(	CAIUnit *pCurTarget, const bool bDamageUpdated, const bool bScanForObstacles, CObjectBase *pCheckBuilding = 0 );
	BYTE AnalyzeTargetScanWithoutMoving( CAIUnit *pCurTarget, const bool bDamageUpdated, const bool bScanForObstacles, CObjectBase *pCheckBuilding = 0 );
	// поискать цель, текущая цель для атаки - pCurTarget
	virtual void LookForTarget( CAIUnit *pCurTarget, const bool bDamageUpdated, CAIUnit **pBestTarget, class CBasicGun **pGun );
	// поискать цель вдалеке для артиллерийского обстрела, текущая цель для атаки - pCurTarget
	virtual void LookForFarTarget( CAIUnit *pCurTarget, const bool bDamageUpdated, CAIUnit **pBestTarget, class CBasicGun **pGun );
		// для того, чтобы искать врагов не только в секторе атаки, а во всём круге
	void SetCircularAttack( const bool bCanAttack );
	// поискать препятствие.
	virtual struct IObstacle *LookForObstacle();
	void UpdateNAttackingGrenages( const int nUpdate ) { nGrenades += nUpdate; NI_ASSERT( nGrenades >= 0, "Wrong number of grenades" ); }
	const int GetNAttackingGrenages() const { return nGrenades; }

	// установать в curTime время для периодов выбора gun
	virtual void ResetGunChoosing();
	// если пора перевыбрать gun, то перевыбрать
	CBasicGun* AnalyzeGunChoose( CAIUnit *pEnemy );

	void EnemyKilled( CAIUnit *pEnemy );

	virtual bool CanMoveForGuard() const { return CanMove() && !GetStats()->IsTrain(); }
	// время, через которое general забудет о невидимом юните
	virtual const float GetTimeToForget() const;
	CAIUnitInfoForGeneral* GetUnitInfoForGeneral() const;
	void SetLastVisibleTime( const NTimer::STime time );
	
	// радиус, в котром сканируются цели
	virtual const float GetTargetScanRadius();
	// юнит находиться в свободном поиске целей
	virtual bool IsFreeEnemySearch() const { return bFreeEnemySearch; }

	// количество сегментнов, прошедшее с прошлого вызова SecondSegment
	virtual const float GetPathSegmentsPeriod() const { return 1.0f; }
	
	virtual float GetPriceMax() const;
	// for Saving/Loading of static members
	friend class CStaticMembers;
	
	virtual const NTimer::STime GetBehUpdateDuration() const;
	
	// killed: this unit + all units inside
	virtual void SendNTotalKilledUnits( const int nPlayerOfShoot, NDb::EReinforcementType eKillerType, NDb::EReinforcementType eDeadType );

//	const SAIExpLevel::SLevel& GetExpLevel() const;
	
	virtual void UnitCommand( CAICommand *pCommand, bool bPlaceInQueue, bool bOnlyThisUnitCommand );

	//для посылки генералу информации о видимых врагах, а также посылки update о видимости юнита
	void CalcVisibility( const bool bIgnoreTime );
	// клиент зависима!
	virtual const bool IsVisibleByPlayer();
	bool CalculateUnitVisibility4PartyInner( const BYTE party ) const;
	
	// залокать unit ( если уже был залокана, то старый lock исчезает )
	virtual void Lock( const CBasicGun *pGun );
	// unlock unit ( если залокан другим gun-ом, то ничего не делается )
	virtual void Unlock( const CBasicGun *pGun );
	// залокан ли каким-либо gun-ом, не равным pGun
	virtual bool IsLocked( const CBasicGun *pGun ) const;
	
	// смена текущего типа патронов.
	void SetActiveShellType( const NDb::SWeaponRPGStats::SShell::EShellDamageType eShellType );

	// for planes
	//void InitAviationPath();
	
	virtual void InstallAction( const EActionNotify eInstallAction, bool bAlreadyDone = false ) { }
	virtual bool IsUninstalled() const { return true; }
	virtual bool IsInstalled() const { return true; }

	virtual void AnimationSet( int nAnimation );
	virtual void AnimationSegment();
	void Moved();
	void Stopped();
	void StopCurAnimation();

	virtual class CArtillery* GetArtilleryIfCrew() const { return 0; }
	virtual void TrackDamagedState( const bool bTrackDamaged ) {}

	void WantedToReveal( CAIUnit *pWhoRevealed );
	bool IsRevealed() const;
	
	virtual const bool IsInfantry() const;
	// обстрелян юнитом pUnit
	virtual void Grazed( CAIUnit *pUnit ) { }
	
	void NullCreationTime() { creationTime = 0; }

	const int GetNVisIndexInUnits() const { return nVisIndexInUnits; }
	void SetNVisIndexInUnits( const int _nVisIndexInUnits ) { nVisIndexInUnits = _nVisIndexInUnits; }

	virtual bool CanMoveAfterUserCommand() const;
	virtual void NotifyAbilityRun( class CAICommand * pCommand );
	
	bool IsHoldingSector() const { return bHoldingSector; }
	void SetHoldSector();
	void ResetHoldSector();
	
	void SetTrackTargeting( bool bOn );
	bool IsTargetingTrack() const { return bTargetingTrack; }

	void SetReinforcementType( const NDb::EReinforcementType eType );
	const NDb::EReinforcementType GetReinforcementType() const { return eReinforcementType; }

	void GetEntranceStateInfo( struct SAINotifyEntranceState *pInfo ) const;
	bool IsRestInside() const;
	void SetRestInside( const bool bInside, CAIUnit *pTransport );

	const bool CanShootInMovement();			// For aviation and ShootInMovement special ability (shoot from priority-0 guns)
	const int GetXPLevel();
	const int GetAbilityLevel();					// Introduced for leaders
	/*
	void SetAbilityLevel( const int _nLevel ) { nAbilityLevel = _nLevel; }  
	void ResetAbilityLevel() { SetAbilityLevel( -1 ); }
	*/

	void SetIgnoreAABBCoeff( const bool bIgnore );
	bool IsIgnoreAABBCoeff() const { return bIgnoreAABBCoeff; }

	void SetMultiShot( const int nShells ) { nMultipleShots = nShells; }
	const int GetMultiShot() const { return nMultipleShots;	}

	virtual const SUnitProfile &GetUnitProfile() const;
	const SRect& GetUnitRect() const;
	virtual const float GetTurnRadius() const { return GetStats()->GetTurnRadius(); }

	// CBasePathUnit
	void SetDirection( const WORD wDirection );
	virtual const CVec2 GetCenterShift() const;
	virtual const bool IsRound() const;
	virtual void UnitTrampled( const CBasePathUnit *pTramplerUnit );
	const bool CanTurnRound() const;
	bool IsTrampled() const { return bTrampled; }
	//
	void UpdateVisibilityForced();
	const bool IsStaticUnit() const { return GetStats()->fSpeed == 0.0f && (std::max)( GetStats()->vAABBHalfSize.x, GetStats()->vAABBHalfSize.y ) > 128.0f; }

	// real implementation of the method
	const NDb::SUnitSpecialAblityDesc *GetUnitAbilityDesc( const NDb::EUnitSpecialAbility eType );
	virtual const bool CanLockTiles() const;
	void UpdateCrewAndTruckVisibility( BYTE party, bool bNewVisibility );
	void CheckAmmoStatus();
};

 
