
#pragma once

#include "Technics.h"
#include "IPlane.h"

class CUnitGuns;
class CTurret;
interface IManuver;
class CPlanesFormation;

class CAviation : public CMilitaryCar, public IPlane
{
	OBJECT_NOCOPY_METHODS( CAviation );
	
		ZDATA_(CMilitaryCar)
	CDBPtr<SMechUnitRPGStats> pStats;

	// для формации самолетов
	CObj<CPlanesFormation> pFormation;
	CVec3 vPlanesShift;										// shift in formation
	float fFuel;
	CVec3 vSpeed;
	CVec3 vPos;
	CVec3 vNormale;
	bool bBombsAutocast;

	CVec2 vInitialPoint;										// plane will leave to appear point
	NTimer::STime timeNextGroundScan;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CMilitaryCar*)this); f.Add(2,&pStats); f.Add(3,&pFormation); f.Add(4,&vPlanesShift); f.Add(5,&fFuel); f.Add(6,&vSpeed); f.Add(7,&vPos); f.Add(8,&vNormale); f.Add(9,&bBombsAutocast); f.Add(10,&vInitialPoint); return 0; }
public:
	CAviation() : bBombsAutocast( false ), fFuel( 0.0f ), vPlanesShift( VNULL3 ), vSpeed( VNULL3 ), vPos( VNULL3 ),
		vNormale( VNULL3 ), vInitialPoint( VNULL2 ) {  }
	virtual ~CAviation();

	const CVec2 & GetAppearPoint() const { return vInitialPoint; }
	bool IsBombsAutocast() const { return bBombsAutocast; }
	void SetBombAutocast( const bool _bBombsAutocast ) { bBombsAutocast = _bBombsAutocast; }

	virtual void Init( const CVec2 &center, const int z, const SUnitBaseRPGStats *pStats, const float fHP, const WORD dir, const BYTE player, ICollisionsCollector *pCollisionsCollector );

	virtual void GetSpeed3( CVec3 *pSpeed ) const ;

	virtual void Segment();
	virtual void SecondSegment( const NTimer::STime timeDiff );
	
	virtual const SUnitBaseRPGStats* GetStats() const { return pStats; }
	virtual IStatesFactory* GetStatesFactory() const;
	// для стрельбы
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
	virtual const bool IsVisible( const BYTE party ) const { return true; }
	
	virtual NTimer::STime GetDisappearInterval() const { return 0; }

	// для получения нормали у истребителей.
	virtual float GetTerrainHeight( const float x, const float y, const NTimer::STime timeDiff ) const { return 0; }

	virtual void GetPlacement( struct SAINotifyPlacement *pPlacement, const NTimer::STime timeDiff );
	
	virtual bool IsMech() const { return true; }
	
	virtual const ECollidingType GetCollidingType( CBasePathUnit *pUnit ) const { return ECT_NONE; }
	virtual const bool CanGoBackward() const { return false; }
	virtual const float GetSightRadius() const;
	virtual void Die( const bool fromExplosion, const float fDamage );

	virtual void Stop() { }
	virtual void Disappear();
	
	// залокать unit ( если уже был залокана, то старый lock исчезает )
	virtual void Lock( const CBasicGun *pGun ) { }
	// unlock unit ( если залокан другим gun-ом, то ничего не делается )
	virtual void Unlock( const CBasicGun *pGun ) { }
	// залокан ли каким-либо gun-ом, не равным pGun
	virtual bool IsLocked( const CBasicGun *pGun ) const { return true; }
	
	// plane's formation, to force planes keep parade during flight.
	void SetPlanesFormation( class CPlanesFormation *pFormation, const CVec3 &vShift );
	CPlanesFormation * GetPlanesFormation();
	const CVec3 GetPlaneShift() const { return vPlanesShift; }

	const SRect & GetUnitRect() const;
	virtual const WORD GetDir() const;
	virtual const CVec2 GetDirVector() const;
	virtual const float GetSpeed() const;

	const NTimer::STime GetNextSecondPathSegmTime() const;

	// IPlane implementation
	virtual CVec3 GetSpeedB2() const;
	virtual CVec3 GetPosB2() const;
	virtual CVec3 GetNormalB2() const;
	virtual const interface IManuver * GetManuver() const;

	// next calculated position
	virtual CVec3 GetPosNext() const;
	virtual CVec3 GetSpeedNext() const;
	virtual CVec3 GetNormalNext() const;

	virtual void SetB2( const CVec3 &_vPos, const CVec3 &_vSpeed, const CVec3 &_vNormal )
	{ 
		NI_ASSERT( false, "wrong call" );
	}
	virtual bool IsBeingAttackedB2() { return false; }
	virtual void NotifyAttackedB2(IPlane & attacker, bool bAttack) {}
	virtual const class CPlanePreferences & GetPreferencesB2() const;
	
	// fuel control
	void DecFuel( const bool bEconomyMode );
	float GetFuel() const;

	virtual class CArtillery* GetTowedArtillery() const { return 0; }
	virtual const bool CanLockTiles() const { return false; }

	virtual const float GetZ() const { return GetCenter().z; }
	virtual const EMovementPlane GetMovementPlane() const { return PLANE_AIR; }
	virtual void GetRPGStats( struct SAINotifyRPGStats *pStats );
	virtual const bool IsAviation() const { return true; }
	virtual const float GetVisZ() const { return GetCenter().z; }
};


