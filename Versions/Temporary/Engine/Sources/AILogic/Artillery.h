#pragma once

#include "AIUnit.h"

#include <cstdint>

class CUnitGuns;
class CTurret;
struct IPath;
class CFormation;
class CAIUnit;
class CArtilleryBulletStorage;
class CMechUnitGuns;

class CArtillery : public CAIUnit
{
	OBJECT_NOCOPY_METHODS( CArtillery );
	
	CDBPtr<SMechUnitRPGStats> pStats;

	int nInitialPlayer;

	// орудийные стволы
	CPtr<CMechUnitGuns> pGuns;

	// вращающаяся пушка
	std::vector< CObj<CTurret> > turrets;

	EActionNotify eCurInstallAction, eNextInstallAction;
	// в каком из install/uninstall мы сейчас находимся
	EActionNotify eCurrentStateOfInstall;

	bool bInstalled;
	NTimer::STime installActionTime;
	bool bInstallActionInstant;						// для того, чтобы артиллерию можно было создать в непроинсталлированном состоянии

	CPtr<IStaticPath> pStaticPathToSend;
	CVec2 vShift;
	CPtr<IPath> pIPathToSend;

	CPtr<CFormation> pCapturingUnit;			// взвод, который бежит захватывать пушку.
	CPtr<CFormation> pCrew;								// взвод, который пушку обслуживает
	float fOperable; // часть команды, которая обслуживает пушку

	CPtr<CAIUnit> pSlaveTransport; // транспорт, который работает на эту пушку
	CPtr<CAIUnit> pHookingTransport;	// transport, that is hooking this artillery.
	
	CObj<CArtilleryBulletStorage> pBulletStorage;
	bool bBulletStorageVisible;
	NTimer::STime lastCheckToInstall;

	NTimer::STime behUpdateDuration;
	// создаёт ammo box для AI, но не посылает его на визуализацию
	void CreateAmmoBox();

	// видет ли ammo box игроку
	void ShowAmmoBox();
	void HideAmmoBox();

	//
	bool IsInstallActionFinished();
	bool ShouldSendInstallAction( const EActionNotify &eAction ) const;
protected:
	virtual void InitGuns();
	virtual const CUnitGuns* GetGuns() const;
	virtual CUnitGuns* GetGuns();
public:
	CArtillery() : bBulletStorageVisible( false ) { }
	virtual void Init( const CVec2 &center, const int z, const SUnitBaseRPGStats *pStats, const float fHP, const uint16_t _dir, const uint8_t player, ICollisionsCollector *pCollisionsCollector );
	int operator&( IBinSaver &f );

	virtual const SUnitBaseRPGStats* GetStats() const { return pStats; }
	virtual IStatesFactory* GetStatesFactory() const;

	virtual void Segment();
	virtual void SetSelectable( bool bSelectable, bool bSendToWorld );

	virtual void SetCamoulfage();
	virtual void RemoveCamouflage( ECamouflageRemoveReason eReason );

	virtual class CTurret* GetTurret( const int nTurret ) const { return turrets[nTurret]; }
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

	//
	virtual int GetNGuns() const;
	virtual class CBasicGun* GetGun( const int n ) const;

	virtual class CBasicGun* ChooseGunForStatObj( class CStaticObject *pObj, NTimer::STime *pTime );

	virtual bool IsInstalled() const { return bInstalled && eCurInstallAction == ACTION_NOTIFY_NONE && eNextInstallAction == ACTION_NOTIFY_NONE; }
	virtual bool IsUninstalled() const { return !bInstalled && eCurInstallAction == ACTION_NOTIFY_NONE && eNextInstallAction == ACTION_NOTIFY_NONE; }
	bool IsInInstallAction() const { return eCurInstallAction != ACTION_NOTIFY_NONE || eNextInstallAction != ACTION_NOTIFY_NONE; }

	void InstallBack( bool bAlreadyDone ); // инсталлировать артиллерию обратно, если она не деинсталлирована - то ошибка.
	virtual void InstallAction( const EActionNotify eInstallAction, bool bAlreadyDone = false );
	// проинсталлировать прямо сейчас
	void ForceInstallAction();

	virtual const bool NeedDeinstall() const;
	virtual const bool CanShoot() const { return IsInstalled(); }
	virtual class CBasicGun* GetFirstArtilleryGun() const;
	virtual void TakeDamage( const float fDamage, const SWeaponRPGStats::SShell *pShell, const int nPlayerOfShoot, CAIUnit *pShotUnit );

	bool IsLightGun() const;
	virtual const bool CanShootToPlanes() const;

	virtual const bool IsIdle() const;
	virtual const bool SendAlongPath( IStaticPath *pStaticPath, const CVec2 &vShift, const bool bSmoothTurn );
	virtual const bool SendAlongPath( IPath *pPath );
	
	virtual float GetMaxFireRange() const;
	virtual void GetRangeArea( struct SShootAreas *pRangeArea ) const;
		
	virtual bool IsMech() const { return true; }
	
	virtual bool TurnToDir( const uint16_t &newDir, const bool bCanBackward = true, const bool bForward = true );
	const bool TurnToTarget( const CVec2 &vTarget );

	virtual bool TurnToUnit( const CVec2 &targCenter );

	// обслуживание пушки артиллеристами
	virtual void ChangePlayer( const uint8_t cPlayer );
	virtual void SetCrew( class CFormation * _pCrew, const bool bCapture = true );
	virtual void DelCrew();
	virtual bool HasServeCrew() const;
	virtual bool MustHaveCrewToOperate() const;
	virtual class CFormation* GetCrew() const;
	virtual bool IsOperable() const { return fOperable != 0.0f; }
	virtual void SetOperable( float fOperable );

	virtual void Disappear();

	//для буксировки
	virtual CVec2 GetTowPoint();
	
	//CRAP { к майлстоуну заплатка
	virtual void SetSlaveTransport( class CAIUnit* _pSlaveTransport ){ pSlaveTransport = _pSlaveTransport; }
	virtual bool HasSlaveTransport();
	virtual class CAIUnit* GetSlaveTransport() { return pSlaveTransport; }
	//CRAP } 

	const CVec3 GetAmmoBoxCoordinates();

	virtual const float GetMaxSpeedHere( const CVec2 &point, bool bAdjust = true ) const;
	virtual const float GetRotateSpeed() const;

	// разрещает / запрещает  всем Gun у данной пушки стрелять
	virtual void DoAllowShoot( bool allow );

	// у всех пушек очищает флаг на ожидание перезарядки
	virtual void ClearWaitForReload();
	
	virtual const ECollidingType GetCollidingType( CBasePathUnit *pUnit ) const;
	
	virtual const bool CanGoBackward() const { return GetCrew() == 0; }

	virtual void Stop();
	virtual const uint32_t GetNormale( const CVec2 &vCenter ) const;
	virtual const uint32_t GetNormale() const;

	const CVec2 GetHookPoint() const;
	const CVec3 GetHookPoint3D() const;
	
	EActionNotify GetCurUninstallAction() const { return eCurrentStateOfInstall; }

	virtual void LookForTarget( CAIUnit *pCurTarget, const bool bDamageUpdated, CAIUnit **pBestTarget, class CBasicGun **pGun );
	
	virtual void SendAcknowledgement( CAICommand *pCommand, EUnitAckType ack, bool bForce = false );
	virtual void SendAcknowledgement( EUnitAckType ack, bool bForce = false );

	int GetInitialPlayer() const { return nInitialPlayer; }
	void SetInitialPlayer( const int nPlayer ) { nInitialPlayer = nPlayer; }
	virtual const NTimer::STime GetBehUpdateDuration() const ;
	//
	bool IsBeingCaptured() const ;
	void SetCapturingUnit( CFormation * pFormation ) ;
	CFormation * GetCapturedUnit() { return pCapturingUnit; }
	
	// to allow only one transport to hook artillery
	bool IsBeingHooked() const;
	void SetBeingHooked( class CAIUnit *pUnit );
	CAIUnit *GetHookingTransport();
	
	void UpdateAmmoBoxVisibility( const bool bVisibilityChanged, const bool bVisible );
	EActionNotify GetFinishInstallUpdate( EActionNotify eAction );
	virtual const float GetMaxPossibleSpeed() const;
	// Keep forward-speed queries consistent with hooked-artillery speed delegation.
	virtual const float GetMaxPossibleForwardSpeed() const;

	//
	virtual const bool CanUnitTrampled( const CBasePathUnit *pTramplerUnit ) const;
	const bool CanHook() const;
	//virtual const bool CanLockTiles() const { return false; }
};


