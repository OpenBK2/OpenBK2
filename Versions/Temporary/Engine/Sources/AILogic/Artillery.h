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

	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	int nInitialPlayer = {};

	// орудийные стволы
	CPtr<CMechUnitGuns> pGuns;

	// вращающаяся пушка
	std::vector<CObj<CTurret>> turrets;

	EActionNotify eCurInstallAction = {}, eNextInstallAction = {};
	// в каком из install/uninstall мы сейчас находимся
	EActionNotify eCurrentStateOfInstall = {};

	bool bInstalled = {};
	NTimer::STime installActionTime = {};
	bool bInstallActionInstant = {}; // для того, чтобы артиллерию можно было создать в непроинсталлированном состоянии

	CPtr<IStaticPath> pStaticPathToSend;
	CVec2 vShift;
	CPtr<IPath> pIPathToSend;

	CPtr<CFormation> pCapturingUnit; // взвод, который бежит захватывать пушку.
	CPtr<CFormation> pCrew;          // взвод, который пушку обслуживает
	float fOperable = {};            // часть команды, которая обслуживает пушку

	CPtr<CAIUnit> pSlaveTransport;   // транспорт, который работает на эту пушку
	CPtr<CAIUnit> pHookingTransport; // transport, that is hooking this artillery.

	CObj<CArtilleryBulletStorage> pBulletStorage;
	bool bBulletStorageVisible = {};
	NTimer::STime lastCheckToInstall = {};

	NTimer::STime behUpdateDuration = {};
	// создаёт ammo box для AI, но не посылает его на визуализацию
	void CreateAmmoBox();

	// видет ли ammo box игроку
	void ShowAmmoBox();
	void HideAmmoBox();

	//
	bool IsInstallActionFinished();
	bool ShouldSendInstallAction( const EActionNotify &eAction ) const;

protected:
	void InitGuns() override;
	const CUnitGuns *GetGuns() const override;
	CUnitGuns *GetGuns() override;

public:
	CArtillery()
		: bBulletStorageVisible( false )
	{
	}

	void Init( const CVec2 &center,
	           const int z,
	           const SUnitBaseRPGStats *pStats,
	           const float fHP,
	           const uint16_t _dir,
	           const uint8_t player,
	           ICollisionsCollector *pCollisionsCollector ) override;
	int operator&( IBinSaver &f ) override;

	const SUnitBaseRPGStats *GetStats() const override
	{
		return pStats;
	}

	IStatesFactory *GetStatesFactory() const override;

	void Segment() override;
	void SetSelectable( bool bSelectable, bool bSendToWorld ) override;

	void SetCamoulfage() override;
	void RemoveCamouflage( ECamouflageRemoveReason eReason ) override;

	class CTurret *GetTurret( const int nTurret ) const override
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

	//
	int GetNGuns() const override;
	class CBasicGun *GetGun( const int n ) const override;

	class CBasicGun *ChooseGunForStatObj( class CStaticObject *pObj, NTimer::STime *pTime ) override;

	bool IsInstalled() const override
	{
		return bInstalled && eCurInstallAction == ACTION_NOTIFY_NONE && eNextInstallAction == ACTION_NOTIFY_NONE;
	}

	bool IsUninstalled() const override
	{
		return !bInstalled && eCurInstallAction == ACTION_NOTIFY_NONE && eNextInstallAction == ACTION_NOTIFY_NONE;
	}

	bool IsInInstallAction() const
	{
		return eCurInstallAction != ACTION_NOTIFY_NONE || eNextInstallAction != ACTION_NOTIFY_NONE;
	}

	void InstallBack( bool bAlreadyDone ); // инсталлировать артиллерию обратно, если она не деинсталлирована - то ошибка.
	void InstallAction( const EActionNotify eInstallAction, bool bAlreadyDone = false ) override;
	// проинсталлировать прямо сейчас
	void ForceInstallAction();

	const bool NeedDeinstall() const override;

	const bool CanShoot() const override
	{
		return IsInstalled();
	}
	class CBasicGun *GetFirstArtilleryGun() const override;
	void TakeDamage( const float fDamage,
	                 const SWeaponRPGStats::SShell *pShell,
	                 const int nPlayerOfShoot,
	                 CAIUnit *pShotUnit ) override;

	bool IsLightGun() const;
	const bool CanShootToPlanes() const override;

	const bool IsIdle() const override;
	const bool SendAlongPath( IStaticPath *pStaticPath, const CVec2 &vShift, const bool bSmoothTurn ) override;
	const bool SendAlongPath( IPath *pPath ) override;

	float GetMaxFireRange() const override;
	void GetRangeArea( struct SShootAreas *pRangeArea ) const override;

	bool IsMech() const override
	{
		return true;
	}

	virtual bool TurnToDir( const uint16_t &newDir, const bool bCanBackward = true, const bool bForward = true );
	const bool TurnToTarget( const CVec2 &vTarget ) override;

	virtual bool TurnToUnit( const CVec2 &targCenter );

	// обслуживание пушки артиллеристами
	void ChangePlayer( const uint8_t cPlayer ) override;
	virtual void SetCrew( class CFormation *_pCrew, const bool bCapture = true );
	virtual void DelCrew();
	virtual bool HasServeCrew() const;
	virtual bool MustHaveCrewToOperate() const;
	virtual class CFormation *GetCrew() const;

	bool IsOperable() const override
	{
		return fOperable != 0.0f;
	}

	virtual void SetOperable( float fOperable );

	void Disappear() override;

	//для буксировки
	virtual CVec2 GetTowPoint();

	//CRAP { к майлстоуну заплатка
	virtual void SetSlaveTransport( class CAIUnit *_pSlaveTransport )
	{
		pSlaveTransport = _pSlaveTransport;
	}

	virtual bool HasSlaveTransport();

	virtual class CAIUnit *GetSlaveTransport()
	{
		return pSlaveTransport;
	}

	//CRAP }

	const CVec3 GetAmmoBoxCoordinates();

	virtual const float GetMaxSpeedHere( const CVec2 &point, bool bAdjust = true ) const;
	virtual const float GetRotateSpeed() const;

	// разрещает / запрещает  всем Gun у данной пушки стрелять
	virtual void DoAllowShoot( bool allow );

	// у всех пушек очищает флаг на ожидание перезарядки
	virtual void ClearWaitForReload();

	const ECollidingType GetCollidingType( CBasePathUnit *pUnit ) const override;

	const bool CanGoBackward() const override
	{
		return GetCrew() == 0;
	}

	void Stop() override;
	const uint32_t GetNormale( const CVec2 &vCenter ) const override;
	const uint32_t GetNormale() const override;

	const CVec2 GetHookPoint() const;
	const CVec3 GetHookPoint3D() const;

	// The artillery's authored towing point determines the attachment position.
	bool IsForwardTowed() const
	{
		return pStats->pExtendedStats && pStats->pExtendedStats->bForwardTowed;
	}

	EActionNotify GetCurUninstallAction() const
	{
		return eCurrentStateOfInstall;
	}

	void LookForTarget( CAIUnit *pCurTarget,
	                    const bool bDamageUpdated,
	                    CAIUnit **pBestTarget,
	                    class CBasicGun **pGun ) override;

	void SendAcknowledgement( CAICommand *pCommand, EUnitAckType ack, bool bForce = false ) override;
	void SendAcknowledgement( EUnitAckType ack, bool bForce = false ) override;

	int GetInitialPlayer() const
	{
		return nInitialPlayer;
	}

	void SetInitialPlayer( const int nPlayer )
	{
		nInitialPlayer = nPlayer;
	}

	const NTimer::STime GetBehUpdateDuration() const override;
	//
	bool IsBeingCaptured() const;
	void SetCapturingUnit( CFormation *pFormation );

	CFormation *GetCapturedUnit()
	{
		return pCapturingUnit;
	}

	// to allow only one transport to hook artillery
	bool IsBeingHooked() const;
	void SetBeingHooked( class CAIUnit *pUnit );
	CAIUnit *GetHookingTransport();

	void UpdateAmmoBoxVisibility( const bool bVisibilityChanged, const bool bVisible );
	EActionNotify GetFinishInstallUpdate( EActionNotify eAction );
	const float GetMaxPossibleSpeed() const override;
	// Keep forward-speed queries consistent with hooked-artillery speed delegation.
	const float GetMaxPossibleForwardSpeed() const override;

	//
	const bool CanUnitTrampled( const CBasePathUnit *pTramplerUnit ) const override;
	const bool CanHook() const;
	//virtual const bool CanLockTiles() const { return false; }
};
