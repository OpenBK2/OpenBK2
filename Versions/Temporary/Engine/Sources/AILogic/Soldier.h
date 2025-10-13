#pragma once

#include "AIUnit.h"
#include "StaticObjectSlotInfo.h"
#include "Mine.h"

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

	ZDATA_(CAIUnit)
	ZSKIP
	ZONSERIALIZE
	EObjectInsideOf eInsideType;

	CDBPtr<NDb::SInfantryRPGStats> pStats;
	CDBPtr<NDb::SInfantryRPGStats> pRememberedStats; // for paratroopers
		// орудийные стволы
	CPtr<CUnitGuns> pGuns;
	

	SStaticObjectSlotInfo slotInfo;

	SAIAngle wMinAngle, wMaxAngle;
	float fOwnSightRadius;

	bool bInFirePlace, bInSolidPlace;

	NTimer::STime lastHit, lastCheck;
	NTimer::STime lastMineCheck; // последняя проверка мин (для инженеров)
	NTimer::STime lastDirUpdate;
	bool bLying;

	CPtr<CFormation> pFormation;
	CPtr<CFormation> pMemorizedFormation;
	CPtr<CFormation> pVirtualFormation;
//	uint8_t cFormSlot;
	bool bWait2Form;

	bool bAllowLieDown; // может ли солдат ложиться под обстрелом (или стоит как оловянный one.)
	NTimer::STime nextSegmTime;
	NTimer::STime timeBWSegments;
	NTimer::STime nextPathSegmTime;
	NTimer::STime nextLogicSegmTime;
	
	std::list< CPtr<CMineStaticObject> > detonatableCharges;
	std::pair<int,int> numControlledCharges;
	std::pair<int,int> numBlastingCharges;
	std::pair<int,int> numLandMines;

	bool bBeingHealed;
	public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CAIUnit*)this); OnSerialize( f ); f.Add(3,&eInsideType); f.Add(4,&pStats); f.Add(5,&pRememberedStats); f.Add(6,&pGuns); f.Add(7,&slotInfo); f.Add(8,&wMinAngle); f.Add(9,&wMaxAngle); f.Add(10,&fOwnSightRadius); f.Add(11,&bInFirePlace); f.Add(12,&bInSolidPlace); f.Add(13,&lastHit); f.Add(14,&lastCheck); f.Add(15,&lastMineCheck); f.Add(16,&lastDirUpdate); f.Add(17,&bLying); f.Add(18,&pFormation); f.Add(19,&pMemorizedFormation); f.Add(20,&pVirtualFormation); f.Add(21,&bWait2Form); f.Add(22,&bAllowLieDown); f.Add(23,&nextSegmTime); f.Add(24,&timeBWSegments); f.Add(25,&nextPathSegmTime); f.Add(26,&nextLogicSegmTime); f.Add(27,&detonatableCharges); f.Add(28,&numControlledCharges); f.Add(29,&numBlastingCharges); f.Add(30,&numLandMines); f.Add(31,&bBeingHealed); return 0; }
	void OnSerialize( IBinSaver &f );
	//
	void UpdateLyingPosition();
	const bool NeedReturnToFormation() const;  
protected:
	virtual void PrepareToDelete();
	
	virtual void InitGuns();
	// показывает все мины, которые попали в радиус обнаружения этого инженера
	// 
	virtual void RevealNearestMines( const bool bIncludingAP );

	virtual bool CalculateUnitVisibility4Party( const uint8_t cParty ) ;
	// soldier doesn't have anti aircraft guns yet
	virtual uint32_t InitSupportAntiAircraftGuns() { return 0; }
	virtual void AdjustSpeed();

public:
	CSoldier() : pFormation( 0 ), eInsideType( EOIO_NONE ),
							 wMinAngle( 0 ), wMaxAngle( 0 ), bInFirePlace( false ), bInSolidPlace( false ),
							 fOwnSightRadius( -1 ), numBlastingCharges( 0, 0 ), numControlledCharges( 0, 0 ), numLandMines( 0,0 ) { }

	virtual ~CSoldier();
	virtual void Init( const CVec2 &center, const int z, const SUnitBaseRPGStats *pStats, const float fHP, const uint16_t dir, const uint8_t player, ICollisionsCollector *pCollisionsCollector );
	
	void SetRememberedStats( const NDb::SInfantryRPGStats *_pRememberedStats ) { pRememberedStats = pRememberedStats; }
	virtual const SUnitBaseRPGStats *GetStats() const { return pStats; }
	virtual IStatesFactory* GetStatesFactory() const;
	
	virtual void SetDirection( const uint16_t newDir );

	virtual void AllowLieDown( bool _bAllowLieDown );
	bool IsAllowedToLieDown() const { return bAllowLieDown; }
	void MoveToEntrenchFireplace( const CVec3 &coord, const int _nSlot  );
	
	class CBuilding* GetBuilding() const;
	class CEntrenchment* GetEntrenchment() const;
	class CMilitaryCar* GetTransportUnit() const;

	void SetInBuilding( class CBuilding *pBuilding );
	void SetInEntrenchment( class CEntrenchment *pEntrenchment );
	void SetInTransport( class CMilitaryCar *pUnit );
	void SetFree();
	void SetMasterOfStreetsBonus( EObjectInsideOf eNewInsideType );
	
	virtual bool IsInSolidPlace() const;
	virtual bool IsInFirePlace() const;
	void SetToFirePlace();
	void SetToSolidPlace();
	virtual bool IsFree() const ;

	void SetNSlot( const int nSlot );// { slotInfo.nSlot = nSlot; }
	const int GetSlot() const { return slotInfo.nSlot; }
	const SStaticObjectSlotInfo& GetSlotInfo() const { return slotInfo; }
	//SStaticObjectSlotInfo& GetSlotInfo() { return slotInfo; }
	void SetSlotInfo( const int nSlot, const int nType, const int nIndex ) { SetNSlot( nSlot ); slotInfo.nType = nType; slotInfo.nIndex = nIndex; }
	void SetSlotIndex( const int nIndex ) { slotInfo.nIndex = nIndex; }
	const int GetSlotIndex() const { return slotInfo.nIndex; }

	void SetAngles( const uint16_t _wMinAngle, const uint16_t _wMaxAngle )
	{ 
		wMinAngle = _wMinAngle; 
		wMaxAngle = _wMaxAngle; 
	}
	uint16_t GetMinAngle() const { return wMinAngle; }
	uint16_t GetMaxAngle() const { return wMaxAngle; }
	bool IsAngleLimited() const;
	virtual const uint32_t GetNormale() const;
	virtual const uint32_t GetNormale( const CVec2 &vCenter ) const;
	
	bool IsInBuilding() const { return eInsideType == EOIO_BUILDING; }
	bool IsInEntrenchment() const { return eInsideType == EOIO_ENTRENCHMENT; }
	bool IsInTransport() const { return eInsideType == EOIO_TRANSPORT; }

	// для стрельбы  - заполняются поля typeID, pUnit и номер слота, если нужно
	virtual void  GetShotInfo( struct SAINotifyInfantryShot *pShotInfo ) const;
	// для бросания гранат - заполняются поля typeID, pUnit и номер слота, если нужно
	void GetThrowInfo( struct SAINotifyInfantryShot *pThrowInfo ) const;
	void GetEntranceStateInfo( struct SAINotifyEntranceState *pInfo ) const;

	virtual const EActionNotify GetAimAction() const;
	virtual const EActionNotify GetShootAction() const;
	virtual const EActionNotify GetThrowAction() const;
	virtual const EActionNotify GetDieAction() const;
	virtual const EActionNotify GetIdleAction() const;
	virtual const EActionNotify GetMovingAction() const;

	//
	virtual const bool CanMove() const 
	{ 
		return !IsBeingHealed() && IsFree() && GetBehaviourMoving() != SBehaviour::EMHoldPos && !IsInTankPit();
	}
	virtual const bool CanMoveCritical() const 
	{ 
		return IsFree() && !IsInTankPit(); 
	}

	virtual bool InVisCone( const CVec2 &point ) const;
	void SetOwnSightRadius( const float _fOwnSightRadius ) { fOwnSightRadius = _fOwnSightRadius; }
	void RemoveOwnSightRadius() { fOwnSightRadius = -1; }
	virtual const float GetSightRadius() const
	{
		if ( bInSolidPlace )
			return 0;
		return ( fOwnSightRadius >= 0.0f ) ? fOwnSightRadius : CAIUnit::GetSightRadius();
	}

	// вероятность, с которой нанесётся damage при попадании
	virtual const float GetCover() const;
	bool IsLying() const { return bLying; }
	void LieDown();
	void LieDownForce(); // run statnding to lie ground animation
	void StandUp();
	
	virtual void Segment();
	virtual void FreezeSegment();
	
	// формация
	void SetFormation( class CFormation* pFormation );
	virtual bool IsInFormation() const { return pFormation != 0; }
	virtual class CFormation* GetFormation() const { return pFormation; }
	virtual const CVec2 GetUnitPointInFormation() const;
//	virtual const int GetFormationSlot() const { return cFormSlot; }
	virtual const bool CanShootToPlanes() const;

	//
	virtual int GetNGuns() const;
	virtual class CBasicGun* GetGun( const int n ) const;
	virtual class CTurret* GetTurret( const int nTurret ) const;
	virtual const int GetNTurrets() const;
	virtual int GetNAmmo( const int nCommonGun ) const;
	virtual void Fired( const float fGunRadius, const int nGun );
	// nAmmo со знаком
	virtual void ChangeAmmo( const int nCommonGun, const int nAmmo );
	virtual bool IsCommonGunFiring( const int nCommonGun ) const;

	virtual class CBasicGun* ChooseGunForStatObj( class CStaticObject *pObj, NTimer::STime *pTime );
	
	virtual void GetFogInfo( SWarFogUnitInfo *pInfo ) const;
	virtual void WarFogChanged();
	
	virtual void GetShootAreas( struct SShootAreas *pShootAreas, int *pnAreas ) const;
	virtual float GetMaxFireRange() const;
	
	virtual bool IsMech() const { return false; }

	virtual bool IsNoticableByUnit( class CCommonUnit *pUnit, const float fNoticeRadius );
	
	virtual bool ProcessAreaDamage( const class CExplosion *pExpl, const int nArmorDir, const float fRadius, const float fSmallRadius );
	
	virtual void MemCurFormation();
	class CFormation* GetMemFormation() { return pMemorizedFormation; }
	void SetVirtualFormation( class CFormation *pFormation );

	void SetWait2FormFlag( bool bNewValue ) { bWait2Form = bNewValue; }
	bool IsInWait2Form() const { return bWait2Form; }
	
	void MemorizeFormation();
	
	virtual const int GetMinArmor() const;
	virtual const int GetMaxArmor() const;
	virtual const int GetMinPossibleArmor( const int nSide ) const;
	virtual const int GetMaxPossibleArmor( const int nSide ) const;
	virtual const int GetArmor( const int nSide ) const;
	virtual const int GetRandomArmor( const int nSide ) const;
	
	virtual const bool CanGoBackward() const { return false; }
	
	virtual void GetTilesForVisibility( CTilesSet *pTiles ) const;
	virtual bool ShouldSuspendAction( const EActionNotify &eAction ) const;
	virtual bool CanJoinToFormation() const;

	// поискать цель, текущая цель для атаки - pCurTarget
	virtual void LookForTarget( CAIUnit *pCurTarget, const bool bDamageUpdated, CAIUnit **pBestTarget, class CBasicGun **pGun );
	virtual void TakeDamage( const float fDamage, const SWeaponRPGStats::SShell *pShell, const int nPlayerOfShoot, CAIUnit *pShotUnit );
	
	virtual const NTimer::STime GetNextSegmTime() const { return nextSegmTime; }
	virtual void NullSegmTime() { timeBWSegments = 0; }

	virtual void FirstSegment( const NTimer::STime timeDiff );
	virtual const NTimer::STime GetNextPathSegmTime() const { return nextPathSegmTime; }

	// количество сегментнов, прошедшее с прошлого вызова SecondSegment
	virtual const float GetPathSegmentsPeriod() const;
	
	virtual const NTimer::STime GetBehUpdateDuration() const { return SConsts::SOLDIER_BEH_UPDATE_DURATION; }

	virtual class CArtillery* GetArtilleryIfCrew() const;

	virtual const CUnitGuns* GetGuns() const { return pGuns; }
	virtual CUnitGuns* GetGuns() { return pGuns; }
	virtual int HasGrenades() const;
	virtual void SetGrenadeAutocast( bool bOn );
	virtual void SetGrenadeFixed( bool bOn );

	virtual void FreezeByState( const bool bFreeze );
	
	void Die( const bool fromExplosion, const float fDamage );

	void DetonateCharges();
	void UsedCharge( NDb::EUnitSpecialAbility eType, CMineStaticObject *pMine );
	bool HasChargesToDetonate() const;
	int GetChargesLeft( NDb::EUnitSpecialAbility eType ) const;
	void SetBeingHealed( const bool _bBeingHealed ) { bBeingHealed = _bBeingHealed; }
	bool IsBeingHealed() const { return bBeingHealed; }

	virtual const bool CanUnitTrampled( const CBasePathUnit *pTramplerUnit ) const;

	virtual const ECollidingType GetCollidingType( CBasePathUnit *pUnit ) const;
	virtual const bool ShouldScatter() const { return true; }
	virtual const bool CanTurnInstant() const { return true; }
	virtual const bool CanChangeSpeedInstant() const { return true; }
	virtual const bool IsRound() const { return true; }
};

class CInfantry : public CSoldier
{
	OBJECT_BASIC_METHODS( CInfantry );
	ZDATA_(CSoldier)
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CSoldier*)this); return 0; }
public:
	CInfantry() { }
};

class CSniper : public CSoldier
{
	OBJECT_BASIC_METHODS( CSniper );

	ZDATA_(CSoldier)
	NTimer::STime lastVisibilityCheck;
	// виден ли для противоположной party
	bool bVisible;
	// находится ли в sneak mode
	bool bSneak;
	// вероятность снять камуфляж при выстреле, если находимся в sneak mode
	float fCamouflageRemoveWhenShootProbability;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CSoldier*)this); f.Add(2,&lastVisibilityCheck); f.Add(3,&bVisible); f.Add(4,&bSneak); f.Add(5,&fCamouflageRemoveWhenShootProbability); return 0; }
protected:
	virtual bool CalculateUnitVisibility4Party( const uint8_t cParty ) ;
public:
	CSniper() : lastVisibilityCheck( 0 ), bVisible( false ), bSneak( false ) { }

	virtual void Init( const CVec2 &center, const int z, const SUnitBaseRPGStats *pStats, const float fHP, const uint16_t dir, const uint8_t player, ICollisionsCollector *pCollisionsCollector );

	virtual void Segment();

	void SetVisible() { bVisible = true; }

	//virtual void RemoveCamouflage( ECamouflageRemoveReason eReason );
	//void SetSneak( const bool bSneakMode );

	virtual void Fired( const float fGunRadius, const int nGun  );
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


