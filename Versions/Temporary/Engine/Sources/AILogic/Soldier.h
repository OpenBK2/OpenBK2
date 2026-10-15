#pragma once

#include "AIUnit.h"
#include "Mine.h"
#include "StaticObjectSlotInfo.h"

#include <cstdint>

class CUnitGuns;
class CFormation;

class CSoldier : public CAIUnit
{
	enum EObjectInsideOf
	{
		EOIO_NONE,
		EOIO_BUILDING,
		EOIO_TRANSPORT,
		EOIO_ENTRENCHMENT,
		EOIO_UNKNOWN
	};

	ZDATA_( CAIUnit )
	ZSKIP
	ZONSERIALIZE
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	EObjectInsideOf eInsideType = {};

	CDBPtr<NDb::SInfantryRPGStats> pStats;
	// орудийные стволы
	CPtr<CUnitGuns> pGuns;

	SStaticObjectSlotInfo slotInfo;

	SAIAngle wMinAngle, wMaxAngle;
	float fOwnSightRadius = {};

	bool bInFirePlace = {}, bInSolidPlace = {};

	NTimer::STime lastHit = {}, lastCheck = {};
	NTimer::STime lastMineCheck = {}; // последняя проверка мин (для инженеров)
	NTimer::STime lastDirUpdate = {};
	bool bLying = {};

	CPtr<CFormation> pFormation;
	CPtr<CFormation> pMemorizedFormation;
	CPtr<CFormation> pVirtualFormation;
	//	uint8_t cFormSlot;
	bool bWait2Form = {};

	bool bAllowLieDown = {}; // может ли солдат ложиться под обстрелом (или стоит как оловянный one.)
	NTimer::STime nextSegmTime = {};
	NTimer::STime timeBWSegments = {};
	NTimer::STime nextPathSegmTime = {};
	NTimer::STime nextLogicSegmTime = {};

	std::list<CPtr<CMineStaticObject>> detonatableCharges;
	std::pair<int, int> numControlledCharges;
	std::pair<int, int> numBlastingCharges;
	std::pair<int, int> numLandMines;

	bool bBeingHealed = {};

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CAIUnit *)this );
		OnSerialize( f );
		f.Add( 3, &eInsideType );
		f.Add( 4, &pStats );
		f.Add( 6, &pGuns );
		f.Add( 7, &slotInfo );
		f.Add( 8, &wMinAngle );
		f.Add( 9, &wMaxAngle );
		f.Add( 10, &fOwnSightRadius );
		f.Add( 11, &bInFirePlace );
		f.Add( 12, &bInSolidPlace );
		f.Add( 13, &lastHit );
		f.Add( 14, &lastCheck );
		f.Add( 15, &lastMineCheck );
		f.Add( 16, &lastDirUpdate );
		f.Add( 17, &bLying );
		f.Add( 18, &pFormation );
		f.Add( 19, &pMemorizedFormation );
		f.Add( 20, &pVirtualFormation );
		f.Add( 21, &bWait2Form );
		f.Add( 22, &bAllowLieDown );
		f.Add( 23, &nextSegmTime );
		f.Add( 24, &timeBWSegments );
		f.Add( 25, &nextPathSegmTime );
		f.Add( 26, &nextLogicSegmTime );
		f.Add( 27, &detonatableCharges );
		f.Add( 28, &numControlledCharges );
		f.Add( 29, &numBlastingCharges );
		f.Add( 30, &numLandMines );
		f.Add( 31, &bBeingHealed );
		return 0;
	}

	void OnSerialize( IBinSaver &f );
	//
	void UpdateLyingPosition();
	const bool NeedReturnToFormation() const;

protected:
	void PrepareToDelete() override;

	void InitGuns() override;
	// показывает все мины, которые попали в радиус обнаружения этого инженера
	//
	virtual void RevealNearestMines( const bool bIncludingAP );

	bool CalculateUnitVisibility4Party( const uint8_t cParty ) override;

	// soldier doesn't have anti aircraft guns yet
	uint32_t InitSupportAntiAircraftGuns() override
	{
		return 0;
	}

	void AdjustSpeed() override;

public:
	CSoldier()
		: pFormation( 0 )
		, eInsideType( EOIO_NONE )
		, wMinAngle( 0 )
		, wMaxAngle( 0 )
		, bInFirePlace( false )
		, bInSolidPlace( false )
		, fOwnSightRadius( -1 )
		, numBlastingCharges( 0, 0 )
		, numControlledCharges( 0, 0 )
		, numLandMines( 0, 0 )
	{
	}

	~CSoldier() override;
	void Init( const CVec2 &center,
	           const int z,
	           const SUnitBaseRPGStats *pStats,
	           const float fHP,
	           const uint16_t dir,
	           const uint8_t player,
	           ICollisionsCollector *pCollisionsCollector ) override;

	const SUnitBaseRPGStats *GetStats() const override
	{
		return pStats;
	}

	IStatesFactory *GetStatesFactory() const override;

	virtual void SetDirection( const uint16_t newDir );

	virtual void AllowLieDown( bool _bAllowLieDown );

	bool IsAllowedToLieDown() const
	{
		return bAllowLieDown;
	}

	void MoveToEntrenchFireplace( const CVec3 &coord, const int _nSlot );

	class CBuilding *GetBuilding() const;
	class CEntrenchment *GetEntrenchment() const;
	class CMilitaryCar *GetTransportUnit() const;

	void SetInBuilding( class CBuilding *pBuilding );
	void SetInEntrenchment( class CEntrenchment *pEntrenchment );
	void SetInTransport( class CMilitaryCar *pUnit );
	void SetFree();
	void SetMasterOfStreetsBonus( EObjectInsideOf eNewInsideType );

	bool IsInSolidPlace() const override;
	bool IsInFirePlace() const override;
	void SetToFirePlace();
	void SetToSolidPlace();
	bool IsFree() const override;

	void SetNSlot( const int nSlot ); // { slotInfo.nSlot = nSlot; }

	const int GetSlot() const
	{
		return slotInfo.nSlot;
	}

	const SStaticObjectSlotInfo &GetSlotInfo() const
	{
		return slotInfo;
	}

	//SStaticObjectSlotInfo& GetSlotInfo() { return slotInfo; }
	void SetSlotInfo( const int nSlot, const int nType, const int nIndex )
	{
		SetNSlot( nSlot );
		slotInfo.nType = nType;
		slotInfo.nIndex = nIndex;
	}

	void SetSlotIndex( const int nIndex )
	{
		slotInfo.nIndex = nIndex;
	}

	const int GetSlotIndex() const
	{
		return slotInfo.nIndex;
	}

	void SetAngles( const uint16_t _wMinAngle, const uint16_t _wMaxAngle )
	{
		wMinAngle = _wMinAngle;
		wMaxAngle = _wMaxAngle;
	}

	uint16_t GetMinAngle() const
	{
		return wMinAngle;
	}

	uint16_t GetMaxAngle() const
	{
		return wMaxAngle;
	}

	bool IsAngleLimited() const;
	const uint32_t GetNormale() const override;
	const uint32_t GetNormale( const CVec2 &vCenter ) const override;

	bool IsInBuilding() const
	{
		return eInsideType == EOIO_BUILDING;
	}

	bool IsInEntrenchment() const
	{
		return eInsideType == EOIO_ENTRENCHMENT;
	}

	bool IsInTransport() const
	{
		return eInsideType == EOIO_TRANSPORT;
	}

	// для стрельбы  - заполняются поля typeID, pUnit и номер слота, если нужно
	void GetShotInfo( struct SAINotifyInfantryShot *pShotInfo ) const override;
	// для бросания гранат - заполняются поля typeID, pUnit и номер слота, если нужно
	void GetThrowInfo( struct SAINotifyInfantryShot *pThrowInfo ) const;
	void GetEntranceStateInfo( struct SAINotifyEntranceState *pInfo ) const override;

	const EActionNotify GetAimAction() const override;
	const EActionNotify GetShootAction() const override;
	virtual const EActionNotify GetThrowAction() const;
	const EActionNotify GetDieAction() const override;
	const EActionNotify GetIdleAction() const override;
	const EActionNotify GetMovingAction() const override;

	//
	const bool CanMove() const override
	{
		return !IsBeingHealed() && IsFree() && GetBehaviourMoving() != SBehaviour::EMHoldPos && !IsInTankPit();
	}

	const bool CanMoveCritical() const override
	{
		return IsFree() && !IsInTankPit();
	}

	bool InVisCone( const CVec2 &point ) const override;

	void SetOwnSightRadius( const float _fOwnSightRadius )
	{
		fOwnSightRadius = _fOwnSightRadius;
	}

	void RemoveOwnSightRadius()
	{
		fOwnSightRadius = -1;
	}

	const float GetSightRadius() const override
	{
		if ( bInSolidPlace )
		{
			return 0;
		}
		return ( fOwnSightRadius >= 0.0f ) ? fOwnSightRadius : CAIUnit::GetSightRadius();
	}

	// вероятность, с которой нанесётся damage при попадании
	const float GetCover() const override;

	bool IsLying() const
	{
		return bLying;
	}

	void LieDown();
	void LieDownForce(); // run statnding to lie ground animation
	void StandUp();

	void Segment() override;
	void FreezeSegment() override;

	// формация
	void SetFormation( class CFormation *pFormation );

	bool IsInFormation() const override
	{
		return pFormation != 0;
	}

	class CFormation *GetFormation() const override
	{
		return pFormation;
	}

	const CVec2 GetUnitPointInFormation() const override;
	//	virtual const int GetFormationSlot() const { return cFormSlot; }
	const bool CanShootToPlanes() const override;

	//
	int GetNGuns() const override;
	class CBasicGun *GetGun( const int n ) const override;
	class CTurret *GetTurret( const int nTurret ) const override;
	const int GetNTurrets() const override;
	int GetNAmmo( const int nCommonGun ) const override;
	void Fired( const float fGunRadius, const int nGun ) override;
	// nAmmo со знаком
	void ChangeAmmo( const int nCommonGun, const int nAmmo ) override;
	bool IsCommonGunFiring( const int nCommonGun ) const override;

	class CBasicGun *ChooseGunForStatObj( class CStaticObject *pObj, NTimer::STime *pTime ) override;

	void GetFogInfo( SWarFogUnitInfo *pInfo ) const override;
	void WarFogChanged() override;

	void GetShootAreas( struct SShootAreas *pShootAreas, int *pnAreas ) const override;
	float GetMaxFireRange() const override;

	bool IsMech() const override
	{
		return false;
	}

	bool IsNoticableByUnit( class CCommonUnit *pUnit, const float fNoticeRadius ) override;

	bool ProcessAreaDamage( const class CExplosion *pExpl,
	                        const int nArmorDir,
	                        const float fRadius,
	                        const float fSmallRadius ) override;

	virtual void MemCurFormation();

	class CFormation *GetMemFormation()
	{
		return pMemorizedFormation;
	}

	void SetVirtualFormation( class CFormation *pFormation );

	void SetWait2FormFlag( bool bNewValue )
	{
		bWait2Form = bNewValue;
	}

	bool IsInWait2Form() const
	{
		return bWait2Form;
	}

	void MemorizeFormation();

	const int GetMinArmor() const override;
	const int GetMaxArmor() const override;
	const int GetMinPossibleArmor( const int nSide ) const override;
	const int GetMaxPossibleArmor( const int nSide ) const override;
	const int GetArmor( const int nSide ) const override;
	const int GetRandomArmor( const int nSide ) const override;

	const bool CanGoBackward() const override
	{
		return false;
	}

	void GetTilesForVisibility( CTilesSet *pTiles ) const override;
	bool ShouldSuspendAction( const EActionNotify &eAction ) const override;
	virtual bool CanJoinToFormation() const;

	// поискать цель, текущая цель для атаки - pCurTarget
	void LookForTarget( CAIUnit *pCurTarget,
	                    const bool bDamageUpdated,
	                    CAIUnit **pBestTarget,
	                    class CBasicGun **pGun ) override;
	void TakeDamage( const float fDamage,
	                 const SWeaponRPGStats::SShell *pShell,
	                 const int nPlayerOfShoot,
	                 CAIUnit *pShotUnit ) override;

	const NTimer::STime GetNextSegmTime() const override
	{
		return nextSegmTime;
	}

	void NullSegmTime() override
	{
		timeBWSegments = 0;
	}

	void FirstSegment( const NTimer::STime timeDiff ) override;

	const NTimer::STime GetNextPathSegmTime() const override
	{
		return nextPathSegmTime;
	}

	// количество сегментнов, прошедшее с прошлого вызова SecondSegment
	const float GetPathSegmentsPeriod() const override;

	const NTimer::STime GetBehUpdateDuration() const override
	{
		return SConsts::SOLDIER_BEH_UPDATE_DURATION;
	}

	class CArtillery *GetArtilleryIfCrew() const override;

	const CUnitGuns *GetGuns() const override
	{
		return pGuns;
	}

	CUnitGuns *GetGuns() override
	{
		return pGuns;
	}

	virtual int HasGrenades() const;
	virtual void SetGrenadeAutocast( bool bOn );
	virtual void SetGrenadeFixed( bool bOn );

	void FreezeByState( const bool bFreeze ) override;

	void Die( const bool fromExplosion, const float fDamage ) override;

	void DetonateCharges();
	void UsedCharge( NDb::EUnitSpecialAbility eType, CMineStaticObject *pMine );
	bool HasChargesToDetonate() const;
	int GetChargesLeft( NDb::EUnitSpecialAbility eType ) const;

	void SetBeingHealed( const bool _bBeingHealed )
	{
		bBeingHealed = _bBeingHealed;
	}

	bool IsBeingHealed() const
	{
		return bBeingHealed;
	}

	const bool CanUnitTrampled( const CBasePathUnit *pTramplerUnit ) const override;

	const ECollidingType GetCollidingType( CBasePathUnit *pUnit ) const override;

	const bool ShouldScatter() const override
	{
		return true;
	}

	const bool CanTurnInstant() const override
	{
		return true;
	}

	virtual const bool CanChangeSpeedInstant() const
	{
		return true;
	}

	const bool IsRound() const override
	{
		return true;
	}
};

class CInfantry : public CSoldier
{
	OBJECT_BASIC_METHODS( CInfantry );
	ZDATA_( CSoldier )

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CSoldier *)this );
		return 0;
	}

public:
	CInfantry() {}
};

class CSniper : public CSoldier
{
	OBJECT_BASIC_METHODS( CSniper );

	ZDATA_( CSoldier )
	NTimer::STime lastVisibilityCheck = {};
	// виден ли для противоположной party
	bool bVisible = {};
	// находится ли в sneak mode
	bool bSneak = {};
	// вероятность снять камуфляж при выстреле, если находимся в sneak mode
	float fCamouflageRemoveWhenShootProbability = {};

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CSoldier *)this );
		f.Add( 2, &lastVisibilityCheck );
		f.Add( 3, &bVisible );
		f.Add( 4, &bSneak );
		f.Add( 5, &fCamouflageRemoveWhenShootProbability );
		return 0;
	}

protected:
	bool CalculateUnitVisibility4Party( const uint8_t cParty ) override;

public:
	CSniper()
		: lastVisibilityCheck( 0 )
		, bVisible( false )
		, bSneak( false )
	{
	}

	void Init( const CVec2 &center,
	           const int z,
	           const SUnitBaseRPGStats *pStats,
	           const float fHP,
	           const uint16_t dir,
	           const uint8_t player,
	           ICollisionsCollector *pCollisionsCollector ) override;

	void Segment() override;

	void SetVisible()
	{
		bVisible = true;
	}

	//virtual void RemoveCamouflage( ECamouflageRemoveReason eReason );
	//void SetSneak( const bool bSneakMode );

	void Fired( const float fGunRadius, const int nGun ) override;
};

/*
extern CGlobalWarFog theWarFog;
struct SSniperTrace
{
	const SVector centerTile;
	const bool bCamouflated;
	const float fCamouflage2;
	const int nParty;
	CSniper *pSniper;

	//
	SSniperTrace( CSniper *_pSniper )
		: centerTile( _pSniper->GetCenterTile() ),
			bCamouflated( _pSniper->IsCamoulflated() ), fCamouflage2( sqr( _pSniper->GetCamouflage() ) ),
			nParty( _pSniper->GetParty() ),
			pSniper( _pSniper )
	{ }

	bool CanTraceRay( const SVector &point ) const { return true; }
	bool VisitPoint( const SVector &point, const int vis, const float fLen2, const float fR2, const float fSightPower2 )
	{
		if ( point.x == centerTile.x && point.y == centerTile.y )
		{
			if ( !bCamouflated )
			{
				pSniper->SetVisible();
				return false;
			}
			else
			{
				const float fRatio = fSightPower2 * fLen2 / fR2;
				if ( theWarFog.IsTileVisible( point, nParty ) && fRatio <= fCamouflage2 )
				{
					pSniper->SetVisible();
					return false;
				}
			}
		}

		return true;
	}

};
*/
