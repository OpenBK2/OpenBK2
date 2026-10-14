#pragma once

#include "IPlane.h"
#include "Technics.h"

#include <cstdint>

class CUnitGuns;
class CTurret;
struct IManuver;
class CPlanesFormation;

class CAviation : public CMilitaryCar, public IPlane
{
	OBJECT_NOCOPY_METHODS( CAviation );

protected:
	ZDATA_( CMilitaryCar )
	CDBPtr<SMechUnitRPGStats> pStats;

	// для формации самолетов
	CObj<CPlanesFormation> pFormation;
	CVec3 vPlanesShift; // shift in formation
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	float fFuel = {};
	CVec3 vSpeed;
	CVec3 vPos;
	CVec3 vNormale;
	bool bBombsAutocast = {};

	CVec2 vInitialPoint; // plane will leave to appear point
	// Preserve the RNG scan schedule across saves; old saves read the new chunk as zero.
	NTimer::STime timeNextGroundScan = 0;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CMilitaryCar *)this );
		f.Add( 2, &pStats );
		f.Add( 3, &pFormation );
		f.Add( 4, &vPlanesShift );
		f.Add( 5, &fFuel );
		f.Add( 6, &vSpeed );
		f.Add( 7, &vPos );
		f.Add( 8, &vNormale );
		f.Add( 9, &bBombsAutocast );
		f.Add( 10, &vInitialPoint );
		f.Add( 11, &timeNextGroundScan );
		return 0;
	}

public:
	CAviation()
		: bBombsAutocast( false )
		, fFuel( 0.0f )
		, vPlanesShift( VNULL3 )
		, vSpeed( VNULL3 )
		, vPos( VNULL3 )
		, vNormale( VNULL3 )
		, vInitialPoint( VNULL2 )
	{
	}

	~CAviation() override;

	const CVec2 &GetAppearPoint() const
	{
		return vInitialPoint;
	}

	bool IsBombsAutocast() const
	{
		return bBombsAutocast;
	}

	void SetBombAutocast( const bool _bBombsAutocast )
	{
		bBombsAutocast = _bBombsAutocast;
	}

	void Init( const CVec2 &center,
	           const int z,
	           const SUnitBaseRPGStats *pStats,
	           const float fHP,
	           const uint16_t dir,
	           const uint8_t player,
	           ICollisionsCollector *pCollisionsCollector ) override;

	void GetSpeed3( CVec3 *pSpeed ) const override;

	void Segment() override;
	void SecondSegment( const NTimer::STime timeDiff ) override;

	const SUnitBaseRPGStats *GetStats() const override
	{
		return pStats;
	}

	IStatesFactory *GetStatesFactory() const override;

	// для стрельбы
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
	const bool IsVisible( const uint8_t party ) const override
	{
		return true;
	}

	NTimer::STime GetDisappearInterval() const override
	{
		return 0;
	}

	// для получения нормали у истребителей.
	float GetTerrainHeight( const float x, const float y, const NTimer::STime timeDiff ) const override
	{
		return 0;
	}

	void GetPlacement( struct SAINotifyPlacement *pPlacement, const NTimer::STime timeDiff ) override;

	bool IsMech() const override
	{
		return true;
	}

	// Strategic bombers use normal plane movement, but their attacks are forced through horizontal bombing.
	bool IsStrategicBomber() const
	{
		return GetStats()->eDBtype == NDb::DB_RPG_TYPE_AVIA_STRAT_BOMBER;
	}

	bool IsHelicopter() const override
	{
		return false;
	}

	const ECollidingType GetCollidingType( CBasePathUnit *pUnit ) const override
	{
		return ECT_NONE;
	}

	const bool CanGoBackward() const override
	{
		return false;
	}

	const float GetSightRadius() const override;
	void Die( const bool fromExplosion, const float fDamage ) override;

	void Stop() override {}

	void Disappear() override;

	// залокать unit ( если уже был залокана, то старый lock исчезает )
	void Lock( const CBasicGun *pGun ) override {}

	// unlock unit ( если залокан другим gun-ом, то ничего не делается )
	void Unlock( const CBasicGun *pGun ) override {}

	// залокан ли каким-либо gun-ом, не равным pGun
	bool IsLocked( const CBasicGun *pGun ) const override
	{
		return true;
	}

	// plane's formation, to force planes keep parade during flight.
	void SetPlanesFormation( class CPlanesFormation *pFormation, const CVec3 &vShift );
	CPlanesFormation *GetPlanesFormation();

	const CVec3 GetPlaneShift() const
	{
		return vPlanesShift;
	}

	const SRect &GetUnitRect() const override;
	virtual const uint16_t GetDir() const;
	virtual const CVec2 GetDirVector() const;
	virtual const float GetSpeed() const;

	const NTimer::STime GetNextSecondPathSegmTime() const override;

	// IPlane implementation
	CVec3 GetSpeedB2() const override;
	CVec3 GetPosB2() const override;
	CVec3 GetNormalB2() const override;
	const struct IManuver *GetManuver() const override;

	// next calculated position
	CVec3 GetPosNext() const override;
	CVec3 GetSpeedNext() const override;
	CVec3 GetNormalNext() const override;

	void SetB2( const CVec3 &_vPos, const CVec3 &_vSpeed, const CVec3 &_vNormal ) override
	{
		NI_ASSERT( false, "wrong call" );
	}

	bool IsBeingAttackedB2() override
	{
		return false;
	}

	void NotifyAttackedB2( IPlane &attacker, bool bAttack ) override {}
	const class CPlanePreferences &GetPreferencesB2() const override;

	// fuel control
	virtual void DecFuel( const bool bEconomyMode );
	float GetFuel() const;

	class CArtillery *GetTowedArtillery() const override
	{
		return 0;
	}

	const bool CanLockTiles() const override
	{
		return false;
	}

	const float GetZ() const override
	{
		return GetCenter().z;
	}

	const EMovementPlane GetMovementPlane() const override
	{
		return PLANE_AIR;
	}

	void GetRPGStats( struct SAINotifyRPGStats *pStats ) override;

	const bool IsAviation() const override
	{
		return true;
	}

	const float GetVisZ() const override
	{
		return GetCenter().z;
	}
};
