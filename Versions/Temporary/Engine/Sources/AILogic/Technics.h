#pragma once

#include "AIUnit.h"

#include <cstdint>

class CExistingObject;
class CTurret;
class CUnitGuns;
class CSoldier;
class CFormation;
class CArtillery;
class CEntrenchmentTankPit;
class CSupportAAGun;

// просто военная машинка, базовый класс
class CMilitaryCar : public CAIUnit
{
	typedef det_map<int /*unitID*/, int /*board number*/> CBoardOrder;
	typedef det_map<int /*turret UniqueID*/, CObj<CSupportAAGun>> CSupportAAGuns;

	ZDATA_( CAIUnit )
	CDBPtr<SMechUnitRPGStats> pStats;

	// орудийные стволы
	CPtr<CUnitGuns> pGuns;

	// вращающаяся пушка
	std::vector<CObj<CTurret>> turrets;

	// пассажиры
	std::list<CPtr<CSoldier>> pass;

	CPtr<CFormation> pLockingUnit;
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	float fDispersionBonus = {};
	NTimer::STime timeLastHeal = {}; // последнее время лечения

	// mech units, currently On The Way to board
	det_map<int, CPtr<CAIUnit>> boarding;
	std::vector<CPtr<CAIUnit>> onBoard;
	CBoardOrder boardOrder;
	bool bCanUnload = {}; // if entrance point is locked - disable unload

	CSupportAAGuns supportAAGuns;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CAIUnit *)this );
		f.Add( 2, &pStats );
		f.Add( 3, &pGuns );
		f.Add( 4, &turrets );
		f.Add( 5, &pass );
		f.Add( 6, &pLockingUnit );
		f.Add( 7, &fDispersionBonus );
		f.Add( 8, &timeLastHeal );
		f.Add( 9, &boarding );
		f.Add( 10, &onBoard );
		f.Add( 11, &boardOrder );
		f.Add( 12, &bCanUnload );
		f.Add( 13, &supportAAGuns );
		return 0;
	}

private:
	//
	// координаты пассажира n
	const CVec2 GetPassengerCoordinates( const int n );

protected:
	void InitGuns() override;

	const class CUnitGuns *GetGuns() const override
	{
		return pGuns;
	}

	class CUnitGuns *GetGuns() override
	{
		return pGuns;
	}

	void PrepareToDelete() override;
	// soldier doesn't have anti aircraft guns yet
	uint32_t InitSupportAntiAircraftGuns() override;

public:
	// эту функцию переопределяем в подклассах
	void Init( const CVec2 &center,
	           const int z,
	           const SUnitBaseRPGStats *pStats,
	           const float fHP,
	           const uint16_t dir,
	           const uint8_t player,
	           ICollisionsCollector *pCollisionsCollector ) override;

	virtual const CVec2 GetGunCenter( const int nGun ) const;
	void Lock( class CFormation *_pLockingUnit );

	bool IsLocked() const
	{
		return pLockingUnit != 0;
	}

	void Unlock()
	{
		pLockingUnit = 0;
	}

	const SUnitBaseRPGStats *GetStats() const override
	{
		return pStats;
	}

	IStatesFactory *GetStatesFactory() const override = 0;

	// расстояние от центра до точки, откуда можно напрямую бежать к entrance point
	virtual float GetDistanceToLandPoint() const;
	const bool IsIdle() const override;

	virtual uint8_t GetNAvailableSeats() const
	{
		return pStats->nPassangers - pass.size();
	}

	virtual uint8_t GetNPassengers() const
	{
		return pass.size();
	}

	virtual void AddPassenger( class CSoldier *pUnit );
	virtual class CSoldier *GetPassenger( const int n );

	const CVec2 GetEntrancePoint() const;

	// удалить всех пассажиров
	virtual void ClearAllPassengers();
	virtual void DelPassenger( const int n );
	virtual void DelPassenger( class CSoldier *pSoldier );

	void Segment() override;

	CTurret *GetTurret( const int nTurret ) const override
	{
		return turrets[nTurret];
	}

	const int GetNTurrets() const override
	{
		return turrets.size();
	}

	void GetShotInfo( struct SAINotifyMechShot *pShotInfo ) const override
	{
		pShotInfo->typeID = GetShootAction();
		pShotInfo->nObjUniqueID = GetUniqueId();
	}

	const EActionNotify GetShootAction() const override
	{
		return ACTION_NOTIFY_MECH_SHOOT;
	}

	const EActionNotify GetAimAction() const override
	{
		return ACTION_NOTIFY_AIM;
	}

	const EActionNotify GetDieAction() const override
	{
		return ACTION_NOTIFY_DIE;
	}

	const EActionNotify GetIdleAction() const override
	{
		return ACTION_NOTIFY_IDLE;
	}

	const EActionNotify GetMovingAction() const override
	{
		return ACTION_NOTIFY_MOVE;
	}

	const bool CanShootToPlanes() const override;

	//
	int GetNGuns() const override;
	class CBasicGun *GetGun( const int n ) const override;

	class CBasicGun *GetFirstArtilleryGun() const override;

	class CBasicGun *ChooseGunForStatObj( class CStaticObject *pObj, NTimer::STime *pTime ) override;
	float GetMaxFireRange() const override;

	bool IsMech() const override
	{
		return true;
	}

	void GetRangeArea( struct SShootAreas *pRangeArea ) const override;
	virtual class CArtillery *GetTowedArtillery() const = 0;

	const bool CanGoBackward() const override
	{
		return GetTowedArtillery() == 0 && GetMovementPlane() == PLANE_TERRAIN;
	}

	const CVec2 GetHookPoint() const;
	const CVec3 GetHookPoint3D() const;

	// killed: this unit + all units inside
	void SendNTotalKilledUnits( const int nPlayerOfShoot,
	                            NDb::EReinforcementType eKillerType,
	                            NDb::EReinforcementType eDeadType ) override;
	void LookForTarget( CAIUnit *pCurTarget,
	                    const bool bDamageUpdated,
	                    CAIUnit **pBestTarget,
	                    class CBasicGun **pGun ) override;

	virtual bool HasTowedArtilleryCrew() const
	{
		return false;
	}

	virtual void SetTowedArtilleryCrew( class CFormation *pFormation ) {}

	virtual CFormation *GetTowedArtilleryCrew()
	{
		return 0;
	}

	// return true if pUnit can board (this unit is at shore line and have free spots)
	const bool CanBoard( CAIUnit *pUnit ) const;
	void AddBoardingMechUnit( CAIUnit *pUnit ); // notify transport to wait for unit
	void RemoveBoardingMechUnit( CAIUnit *pUnit );
	void SetOnBoard( CAIUnit *pUnit, const bool bOnBoard );
	const uint16_t GetBoardedDirection( CAIUnit *pUnit, const NTimer::STime timeDiff ) const;
	const CVec3 GetBoardedPosition( CAIUnit *pUnit, const NTimer::STime timeDiff ) const;

	int GetNBoarded() const
	{
		return onBoard.size();
	}

	CAIUnit *GetBoarded( const int nIndex )
	{
		return onBoard[nIndex];
	}

	const int GetUnitPriority() const override
	{
		const int nType = SConsts::GetUnitPriority( pStats->eUnitType );
		//наверное в будущес стоит это раскоментить
		//NI_ASSERT( nType != -1, StrFmt( "Unknown unit type: %d", pStats->eUnitType ) );
		const int nArmor = pStats->armors[0].fMin;
		return ( ( nType & 0x0000FFFF ) << 16 ) | ( nArmor & 0x0000FFFF );
	}

	const bool CanUnitTrampled( const CBasePathUnit *pTramplerUnit ) const override;
	void GetPlacement( struct SAINotifyPlacement *pPlacement, const NTimer::STime timeDiff ) override;
};

// может сидеть в TankPit. при любой команде, которая может привести к движению этого юнита
// нужно сначала выполнить команду выхода из TankPit.
class CTank : public CMilitaryCar
{
	OBJECT_BASIC_METHODS( CTank );

	ZDATA_( CMilitaryCar )
	bool bTrackDamaged = {}; // true если у танка перебита гусеница

	SAIAngle wDangerousDir;
	bool bDangerousDirSet = {};
	bool bDangerousDirSetInertia = {};
	NTimer::STime nextTimeOfDangerousDirScan = {};
	NTimer::STime lastTimeOfDangerousDirChanged = {};

	SAIAngle wDangerousDirUnderFire;
	float fDangerousDamageUnderFire = {};

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CMilitaryCar *)this );
		f.Add( 2, &bTrackDamaged );
		f.Add( 3, &wDangerousDir );
		f.Add( 4, &bDangerousDirSet );
		f.Add( 5, &bDangerousDirSetInertia );
		f.Add( 6, &nextTimeOfDangerousDirScan );
		f.Add( 7, &lastTimeOfDangerousDirChanged );
		f.Add( 8, &wDangerousDirUnderFire );
		f.Add( 9, &fDangerousDamageUnderFire );
		return 0;
	}

	//
	void ScanForDangerousDir();

public:
	void Init( const CVec2 &center,
	           const int z,
	           const SUnitBaseRPGStats *pStats,
	           const float fHP,
	           const uint16_t dir,
	           const uint8_t player,
	           ICollisionsCollector *pCollisionsCollector ) override;
	IStatesFactory *GetStatesFactory() const override;

	bool IsTrackDamaged() const
	{
		return bTrackDamaged;
	}

	void RepairTrack(); // починили гусеницу

	void TakeDamage( const float fDamage,
	                 const SWeaponRPGStats::SShell *pShell,
	                 const int nPlayerOfShoot,
	                 CAIUnit *pShotUnit ) override;
	const bool CanMove() const override;
	const bool CanMoveCritical() const override;
	const bool CanRotate() const override;
	virtual bool CanTurnToFrontDir( const uint16_t wDir );

	const bool NeedDeinstall() const override
	{
		return bTrackDamaged;
	}

	class CArtillery *GetTowedArtillery() const override
	{
		return 0;
	}

	const NTimer::STime GetBehUpdateDuration() const override
	{
		if ( RPG_TYPE_SPG_AAGUN == GetStats()->etype )
		{
			return SConsts::AA_BEH_UPDATE_DURATION;
		}
		return SConsts::BEH_UPDATE_DURATION;
	}

	void Segment() override;

	const bool IsDangerousDirExist() const override
	{
		return bDangerousDirSetInertia;
	}

	const uint16_t GetDangerousDir() const override
	{
		return wDangerousDir;
	}

	void Grazed( CAIUnit *pUnit ) override;

	bool CanMoveAfterUserCommand() const override;
};

// транспорт, перевозит марериальные ресурсы,
// может цеплять пушки
class CAITransportUnit : public CMilitaryCar
{
	OBJECT_BASIC_METHODS( CAITransportUnit );
	typedef std::list<CPtr<CFormation>> CExternLoaders;

	ZDATA_( CMilitaryCar )
	float fResursUnits = {}; // количество RU, которые есть у грузовичка
	CPtr<CArtillery> pTowedArtillery;
	CPtr<CAIUnit> pMustTow;               // artillery, that this truck must tow (for general intendant)
	CExternLoaders externLoaders;         // дошоняющие гранспорт грузчики
	CPtr<CFormation> pTowedArtilleryCrew; // when artillery is attached the crew.

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CMilitaryCar *)this );
		f.Add( 2, &fResursUnits );
		f.Add( 3, &pTowedArtillery );
		f.Add( 4, &pMustTow );
		f.Add( 5, &externLoaders );
		f.Add( 6, &pTowedArtilleryCrew );
		return 0;
	}

	// для группового подцепления артиллерии,
	// выбирает юнит из нашей группы, ближайший к артиллерии и возвращает его nUniqueId
	const int GetNUnitToTakeArtillery( bool bPlaceInQueue, CAIUnit *pUnitToTake );

public:
	void Init( const CVec2 &center,
	           const int z,
	           const SUnitBaseRPGStats *pStats,
	           const float fHP,
	           const uint16_t dir,
	           const uint8_t player,
	           ICollisionsCollector *pCollisionsCollector ) override;

	// для процесса ремонта. грузчики расходуют RU в процессе починки и перезарядки
	float GetResursUnitsLeft() const
	{
		return fResursUnits;
	}

	void SetResursUnitsLeft( float _fResursUnits );
	void DecResursUnitsLeft( float dRU );

	// буксировка
	bool IsTowing() const override;

	class CArtillery *GetTowedArtillery() const override
	{
		return pTowedArtillery;
	}

	void SetTowedArtillery( class CArtillery *pTowedArtillery );

	IStatesFactory *GetStatesFactory() const override;
	bool CanCommandBeExecuted( class CAICommand *pCommand ) override;

	void GetRPGStats( struct SAINotifyRPGStats *pStats ) override;
	const int GetUnitState() const override;

	static void PrepareLoaders( CFormation *pLoaderSquad, CAITransportUnit *pTransport );
	static void FreeLoaders( CFormation *pLoaderSquad, CAITransportUnit *pTransport );
	void Segment() override;

	void AddExternLoaders( CFormation *pLoaders );
	void Die( const bool fromExplosion, const float fDamage ) override;
	// return true if all loaders are dead or in truck
	bool UpdateExternalLoaders();

	// towed artillery crew management
	bool HasTowedArtilleryCrew() const override;
	void SetTowedArtilleryCrew( class CFormation *pFormation ) override;
	CFormation *GetTowedArtilleryCrew() override;

	//
	void SetMustTow( class CAIUnit *_pUnit );
	bool IsMustTow() const;

	void UnitCommand( CAICommand *pCommand, bool bPlaceInQueue, bool bOnlyThisUnitCommand ) override;

	bool CanHookUnit( class CAIUnit *pUnitToHook ) const override;

	const bool CheckTurn( const float fRectCoeff,
	                      const CVec2 &vDir,
	                      const bool bWithUnits,
	                      const bool bCanGoBackward ) const override;
	bool CalculateUnitVisibility4Party( const uint8_t party ) override;
};
