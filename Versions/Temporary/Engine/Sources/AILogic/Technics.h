#pragma once
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "AIUnit.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CExistingObject;
class CTurret;
class CUnitGuns;
class CSoldier;
class CFormation;
class CArtillery;
class CEntrenchmentTankPit;
class CSupportAAGun;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// просто военная машинка, базовый класс
class CMilitaryCar : public CAIUnit
{
	typedef hash_map<int/*unitID*/, int/*board number*/> CBoardOrder;
	typedef hash_map<int/*turret UniqueID*/, CObj<CSupportAAGun> > CSupportAAGuns;


	ZDATA_(CAIUnit)
	CDBPtr<SMechUnitRPGStats> pStats;

	// орудийные стволы
	CPtr<CUnitGuns> pGuns;

	// вращающаяся пушка
	vector< CObj<CTurret> > turrets;
	
	// пассажиры
	list<CPtr<CSoldier> > pass;
	
	CPtr<CFormation> pLockingUnit;
	float fDispersionBonus;
	NTimer::STime timeLastHeal;						// последнее время лечения
	
	// mech units, currently On The Way to board
	hash_map<int, CPtr<CAIUnit> > boarding;
	vector<CPtr<CAIUnit> > onBoard;
	CBoardOrder boardOrder;
	bool bCanUnload;								// if entrance point is locked - disable unload 

	CSupportAAGuns supportAAGuns;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CAIUnit*)this); f.Add(2,&pStats); f.Add(3,&pGuns); f.Add(4,&turrets); f.Add(5,&pass); f.Add(6,&pLockingUnit); f.Add(7,&fDispersionBonus); f.Add(8,&timeLastHeal); f.Add(9,&boarding); f.Add(10,&onBoard); f.Add(11,&boardOrder); f.Add(12,&bCanUnload); f.Add(13,&supportAAGuns); return 0; }
private:
	//
	// координаты пассажира n
	const CVec2 GetPassengerCoordinates( const int n );
protected:
	virtual void InitGuns();
	virtual const class CUnitGuns* GetGuns() const { return pGuns; }
	virtual class CUnitGuns* GetGuns() { return pGuns; }
	virtual void PrepareToDelete();
	// soldier doesn't have anti aircraft guns yet
	virtual DWORD InitSupportAntiAircraftGuns();

public:
	// эту функцию переопределяем в подклассах
	virtual void Init( const CVec2 &center, const int z, const SUnitBaseRPGStats *pStats, const float fHP, const WORD dir, const BYTE player, ICollisionsCollector *pCollisionsCollector );

	virtual const CVec2 GetGunCenter( const int nGun ) const;
	void Lock( class CFormation *_pLockingUnit );
	bool IsLocked() const { return pLockingUnit != 0; }
	void Unlock() { pLockingUnit = 0; }
	
	virtual const SUnitBaseRPGStats* GetStats() const { return pStats; }	
	virtual IStatesFactory* GetStatesFactory() const =0;

	// расстояние от центра до точки, откуда можно напрямую бежать к entrance point
	virtual float GetDistanceToLandPoint() const;
	const bool IsIdle() const;
	virtual BYTE GetNAvailableSeats() const { return pStats->nPassangers - pass.size(); }
	virtual BYTE GetNPassengers() const { return pass.size(); }
	virtual void AddPassenger( class CSoldier *pUnit );
	virtual class CSoldier* GetPassenger( const int n );

	const CVec2 GetEntrancePoint() const;

	// удалить всех пассажиров
	virtual void ClearAllPassengers();
	virtual void DelPassenger( const int n );
	virtual void DelPassenger( class CSoldier *pSoldier );

	virtual void Segment();

	virtual CTurret* GetTurret( const int nTurret ) const { return turrets[nTurret]; }
	virtual const int GetNTurrets() const { return turrets.size(); }

	virtual void GetShotInfo( struct SAINotifyMechShot *pShotInfo ) const
	{ 
		pShotInfo->typeID = GetShootAction(); 
		pShotInfo->nObjUniqueID = GetUniqueId();
	}

	virtual const EActionNotify GetShootAction() const { return ACTION_NOTIFY_MECH_SHOOT; }
	virtual const EActionNotify GetAimAction() const { return ACTION_NOTIFY_AIM; }
	virtual const EActionNotify GetDieAction() const { return ACTION_NOTIFY_DIE; }
	virtual const EActionNotify GetIdleAction() const { return ACTION_NOTIFY_IDLE; }
	virtual const EActionNotify GetMovingAction() const { return ACTION_NOTIFY_MOVE; }

	virtual const bool CanShootToPlanes() const;

	//
	virtual int GetNGuns() const;
	virtual class CBasicGun* GetGun( const int n ) const;

	virtual class CBasicGun* GetFirstArtilleryGun() const;

	virtual class CBasicGun* ChooseGunForStatObj( class CStaticObject *pObj, NTimer::STime *pTime );
	virtual float GetMaxFireRange() const;

	virtual bool IsMech() const { return true; }

	virtual void GetRangeArea( struct SShootAreas *pRangeArea ) const;
	virtual class CArtillery* GetTowedArtillery() const = 0;

	virtual const bool CanGoBackward() const { return GetTowedArtillery() == 0 && GetMovementPlane() == PLANE_TERRAIN; }
	
	const CVec2 GetHookPoint() const;
	const CVec3 GetHookPoint3D() const;
	
	// killed: this unit + all units inside
	virtual void SendNTotalKilledUnits( const int nPlayerOfShoot, NDb::EReinforcementType eKillerType, NDb::EReinforcementType eDeadType );
	virtual void LookForTarget( CAIUnit *pCurTarget, const bool bDamageUpdated, CAIUnit **pBestTarget, class CBasicGun **pGun );

	virtual bool HasTowedArtilleryCrew() const { return false; }
	virtual void SetTowedArtilleryCrew( class CFormation *pFormation ) {  }
	virtual CFormation * GetTowedArtilleryCrew()  { return 0; }

	// return true if pUnit can board (this unit is at shore line and have free spots)
	const bool CanBoard( CAIUnit *pUnit ) const;
	void AddBoardingMechUnit( CAIUnit *pUnit );	// notify transport to wait for unit
	void RemoveBoardingMechUnit( CAIUnit *pUnit );
	void SetOnBoard( CAIUnit *pUnit, const bool bOnBoard );
	const WORD GetBoardedDirection( CAIUnit *pUnit, const NTimer::STime timeDiff ) const;
	const CVec3 GetBoardedPosition( CAIUnit *pUnit, const NTimer::STime timeDiff ) const;
	int GetNBoarded() const { return onBoard.size(); }
	CAIUnit * GetBoarded( const int nIndex ) { return onBoard[nIndex]; }


	virtual const int GetUnitPriority() const
	{
		const int nType = SConsts::GetUnitPriority( pStats->eUnitType );
		//наверное в будущес стоит это раскоментить
		//NI_ASSERT( nType != -1, StrFmt( "Unknown unit type: %d", pStats->eUnitType ) );
		const int nArmor = pStats->armors[0].fMin;
		return ( ( nType & 0x0000FFFF ) << 16 ) | ( nArmor & 0x0000FFFF );
	}

	virtual const bool CanUnitTrampled( const CBasePathUnit *pTramplerUnit ) const;
	void GetPlacement( struct SAINotifyPlacement *pPlacement, const NTimer::STime timeDiff );
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// может сидеть в TankPit. при любой команде, которая может привести к движению этого юнита
// нужно сначала выполнить команду выхода из TankPit.
class CTank : public CMilitaryCar
{
	OBJECT_BASIC_METHODS( CTank );
	
	ZDATA_(CMilitaryCar)
	bool bTrackDamaged; // true если у танка перебита гусеница
	
	SAIAngle wDangerousDir;
	bool bDangerousDirSet;
	bool bDangerousDirSetInertia;
	NTimer::STime nextTimeOfDangerousDirScan;
	NTimer::STime lastTimeOfDangerousDirChanged;

	SAIAngle wDangerousDirUnderFire;
	float fDangerousDamageUnderFire;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CMilitaryCar*)this); f.Add(2,&bTrackDamaged); f.Add(3,&wDangerousDir); f.Add(4,&bDangerousDirSet); f.Add(5,&bDangerousDirSetInertia); f.Add(6,&nextTimeOfDangerousDirScan); f.Add(7,&lastTimeOfDangerousDirChanged); f.Add(8,&wDangerousDirUnderFire); f.Add(9,&fDangerousDamageUnderFire); return 0; }
	//
	void ScanForDangerousDir();
public:
	virtual void Init( const CVec2 &center, const int z, const SUnitBaseRPGStats *pStats, const float fHP, const WORD dir, const BYTE player, ICollisionsCollector *pCollisionsCollector );
	virtual IStatesFactory* GetStatesFactory() const;

	bool IsTrackDamaged() const { return bTrackDamaged; }
	void RepairTrack() ;// починили гусеницу

	virtual void TakeDamage( const float fDamage, const SWeaponRPGStats::SShell *pShell, const int nPlayerOfShoot, CAIUnit *pShotUnit );
	virtual const bool CanMove() const;
	virtual const bool CanMoveCritical() const;
	virtual const bool CanRotate() const;
	virtual bool CanTurnToFrontDir( const WORD wDir );

	virtual const bool NeedDeinstall() const { return bTrackDamaged; }

	virtual class CArtillery* GetTowedArtillery() const { return 0; }

	virtual const NTimer::STime GetBehUpdateDuration() const
	{
		if ( RPG_TYPE_SPG_AAGUN == GetStats()->etype )
			return SConsts::AA_BEH_UPDATE_DURATION;
		return SConsts::BEH_UPDATE_DURATION;
	}

	virtual void Segment();

	virtual const bool IsDangerousDirExist() const { return bDangerousDirSetInertia; }
	virtual const WORD GetDangerousDir() const { return wDangerousDir; }
	virtual void Grazed( CAIUnit *pUnit );

	virtual bool CanMoveAfterUserCommand() const;
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// транспорт, перевозит марериальные ресурсы, 
// может цеплять пушки
class CAITransportUnit : public CMilitaryCar
{
	OBJECT_BASIC_METHODS( CAITransportUnit );
	typedef list< CPtr<CFormation> > CExternLoaders;

	ZDATA_(CMilitaryCar)
	float fResursUnits; // количество RU, которые есть у грузовичка
	CPtr<CArtillery> pTowedArtillery;
	CPtr<CAIUnit> pMustTow;			// artillery, that this truck must tow (for general intendant)
	CExternLoaders externLoaders; // дошоняющие гранспорт грузчики
	CPtr<CFormation> pTowedArtilleryCrew;	// when artillery is attached the crew.
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CMilitaryCar*)this); f.Add(2,&fResursUnits); f.Add(3,&pTowedArtillery); f.Add(4,&pMustTow); f.Add(5,&externLoaders); f.Add(6,&pTowedArtilleryCrew); return 0; }
	// для группового подцепления артиллерии, 
	// выбирает юнит из нашей группы, ближайший к артиллерии и возвращает его nUniqueId
	const int GetNUnitToTakeArtillery( bool bPlaceInQueue, CAIUnit *pUnitToTake );
public:
	void Init( const CVec2 &center, const int z, const SUnitBaseRPGStats *pStats, const float fHP, const WORD dir, const BYTE player, ICollisionsCollector *pCollisionsCollector );

	// для процесса ремонта. грузчики расходуют RU в процессе починки и перезарядки
	float GetResursUnitsLeft() const { return fResursUnits; }
	void SetResursUnitsLeft( float _fResursUnits );
	void DecResursUnitsLeft( float dRU );

	// буксировка
	virtual bool IsTowing() const;
	virtual class CArtillery* GetTowedArtillery() const { return pTowedArtillery; }
	void SetTowedArtillery( class CArtillery *pTowedArtillery);

	virtual IStatesFactory* GetStatesFactory() const;
	bool CanCommandBeExecuted( class CAICommand *pCommand );
	
	virtual void GetRPGStats( struct SAINotifyRPGStats *pStats );
	virtual const int GetUnitState() const;
	
	static void PrepareLoaders( CFormation *pLoaderSquad, CAITransportUnit * pTransport );
	static void FreeLoaders( CFormation *pLoaderSquad, CAITransportUnit * pTransport );
	virtual void Segment();

	void AddExternLoaders( CFormation *pLoaders );
	void Die( const bool fromExplosion, const float fDamage );
	// return true if all loaders are dead or in truck
	bool UpdateExternalLoaders();

	// towed artillery crew management
	virtual bool HasTowedArtilleryCrew() const ;
	virtual void SetTowedArtilleryCrew( class CFormation *pFormation ) ;
	virtual CFormation * GetTowedArtilleryCrew() ;
	
	// 
	void SetMustTow( class CAIUnit *_pUnit );
	bool IsMustTow() const;
	
	virtual void UnitCommand( CAICommand *pCommand, bool bPlaceInQueue, bool bOnlyThisUnitCommand );

	virtual bool CanHookUnit( class CAIUnit *pUnitToHook ) const;

	virtual const bool CheckTurn( const float fRectCoeff, const CVec2 &vDir, const bool bWithUnits, const bool bCanGoBackward ) const;
	bool CalculateUnitVisibility4Party( const BYTE party );
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
