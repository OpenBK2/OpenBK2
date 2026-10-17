#pragma once

#include "CommonUnit.h"
#include "Stats_B2_M1/RPGStats.h"

#include <cstdint>

class CArtillery;
class CCommonPathFinder;
class CBasePathUnit;
class CGroupSmoothPath;
class CSoldier;
class CMineStaticObject;
class CAITransportUnit;
enum EActionCommand : int;
struct ICollisionsCollector;

namespace NDb
{
struct SUnitBaseRPGStats;
struct SSquadRPGStats;
} // namespace NDb

class CFormation : public CCommonUnit
{
	OBJECT_BASIC_METHODS( CFormation );

	// для хранения данных о переносимом миномете
	class CCarryedMortar
	{
		ZDATA
		// Defaults cover construction before Init/load; explicit constructor values still take precedence.
		bool bHasMortar = {};
		CDBPtr<SUnitBaseRPGStats> pStats;
		float fHP = {};
		std::vector<int> ammo;

	public:
		ZEND int operator&( IBinSaver &f )
		{
			f.Add( 2, &bHasMortar );
			f.Add( 3, &pStats );
			f.Add( 4, &fHP );
			f.Add( 5, &ammo );
			return 0;
		}

	public:
		CCarryedMortar()
			: bHasMortar( false )
			, fHP( 0.0f )
		{
		}

		bool HasMortar() const
		{
			return bHasMortar;
		}
		class CAIUnit *CreateMortar( class CFormation *pOwner );
		void Init( const class CAIUnit *pArt );
	};

	struct SGunInfo
	{
		int nUnit = {};
		int nUnitGun = {};

		SGunInfo()
			: nUnit( -1 )
			, nUnitGun( -1 )
		{
		}

		SGunInfo( const int _nUnit, const int _nUnitGun )
			: nUnit( _nUnit )
			, nUnitGun( _nUnitGun )
		{
		}
	};

	enum EObjectInsideOf
	{
		EOIO_NONE,
		EOIO_BUILDING,
		EOIO_TRANSPORT,
		EOIO_ENTRENCHMENT,
		EOIO_UNKNOWN
	};

	struct SVirtualUnit
	{
		ZDATA
		CPtr<CSoldier> pSoldier;
		int nSlotInStats = {};

		ZEND int operator&( IBinSaver &f )
		{
			f.Add( 2, &pSoldier );
			f.Add( 3, &nSlotInStats );
			return 0;
		}

		SVirtualUnit()
			: pSoldier( 0 )
		{
		}
	};

	ZDATA_( CCommonUnit )
	float fPass = {};

	NTimer::STime timeToCamouflage = {};

	CArray1Bit availCommands;
	//
	uint8_t cPlayer = {};

	std::vector<SGunInfo> guns;

	bool bWaiting = {};
	CDBPtr<NDb::SSquadRPGStats> pStats;

	bool bDisabled = {};

	EObjectInsideOf eInsideType = {};
	CPtr<CObjectBase> pObjInside;

	float fMaxFireRange = {};

	std::vector<SVirtualUnit> virtualUnits;
	int nVirtualUnits = {};
	bool bCanBeResupplied = {};

	CCarryedMortar mortar;
	bool bBoredInMoveFormationSent = {};
	NTimer::STime lastBoredInMoveFormationCheck = {};

	bool bWithMoraleOfficer = {};

	bool bUsedCharge = {};
	CPtr<CMineStaticObject> pCharge;
	int nInUnitsID = {};

	CPtr<CGroupSmoothPath> pGroupSmoothPath;
	std::vector<CPtr<CSoldier>> soldiers;
	float fMaxSpeed = {};
	float fSpeedCoeff = {};
	CVec2 vAABBHalfSize;
	int nBoundTileRadius = {};

	NTimer::STime timeLastCatchArt = {};
	uint32_t dwCatchArtFlag = {};

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CCommonUnit *)this );
		f.Add( 2, &fPass );
		f.Add( 3, &timeToCamouflage );
		f.Add( 4, &availCommands );
		f.Add( 5, &cPlayer );
		f.Add( 6, &guns );
		f.Add( 7, &bWaiting );
		f.Add( 8, &pStats );
		f.Add( 9, &bDisabled );
		f.Add( 10, &eInsideType );
		f.Add( 11, &pObjInside );
		f.Add( 12, &fMaxFireRange );
		f.Add( 13, &virtualUnits );
		f.Add( 14, &nVirtualUnits );
		f.Add( 15, &bCanBeResupplied );
		f.Add( 16, &mortar );
		f.Add( 17, &bBoredInMoveFormationSent );
		f.Add( 18, &lastBoredInMoveFormationCheck );
		f.Add( 19, &bWithMoraleOfficer );
		f.Add( 20, &bUsedCharge );
		f.Add( 21, &pCharge );
		f.Add( 22, &nInUnitsID );
		f.Add( 23, &pGroupSmoothPath );
		f.Add( 24, &soldiers );
		f.Add( 25, &fMaxSpeed );
		f.Add( 26, &fSpeedCoeff );
		f.Add( 27, &vAABBHalfSize );
		f.Add( 28, &nBoundTileRadius );
		f.Add( 29, &timeLastCatchArt );
		f.Add( 30, &dwCatchArtFlag );
		return 0;
	}

	//
	void InitGeometries();
	void PrepareToDelete();

	bool IsMemberResting( class CSoldier *pSoldier ) const;

	void SetMaxSpeed( const float _fMaxSpeed )
	{
		fMaxSpeed = _fMaxSpeed;
	}

	void SetBoundTileRadius( const int _nBoundTileRadius )
	{
		nBoundTileRadius = _nBoundTileRadius;
	}

	void SetAABBHalfSize( const CVec2 &_vAABBHalfSize )
	{
		vAABBHalfSize = _vAABBHalfSize;
	}

	void UpdateStats( class CSoldier *pUnit, const int nPos );

protected:
	ISmoothPath *CreateSmoothPath() override;

	void UpdatePlacement( const CVec3 &vOldPosition, const uint16_t wOldDirection, const bool bNeedUpdate ) override {}

public:
	CFormation();
	void Init( const SSquadRPGStats *pStats,
	           const CVec2 &center,
	           const int z,
	           const uint16_t dir,
	           ICollisionsCollector *pCollisionsCollector );
	// передвигает центр формации в её центр масс и инициализирует geomInfo
	void ChangeGeometry( const int nGeometry );
	const int GetGeometriesCount() const;
	const int GetCurrentGeometry() const;

	const NDb::SSquadRPGStats *GetStats() const override
	{
		return pStats;
	}

	// добавить новый юнит в формацию, порядковый номер его в статах - nSlot, местоположение юнита инициализируется
	void AddNewUnitToSlot( class CSoldier *pUnit, const bool bSendToWorld );

	// добавить новый юнит на позицию nPos в списке юнитов объекта formation, причём местоположение юнита не инициализируется
	void AddSoldier( class CSoldier *pUnit );
	void DeleteSoldier( class CSoldier *pUnit );

	// возвращает позицию в статах формации для юнита с порядковым номером cSlot в массиве units
	const int GetUnitSlotInStats( const uint8_t cSlot ) const;

	const float GetPassability() const override
	{
		return fPass;
	}

	void Segment() override;

	const bool IsIdle() const override;

	const bool IsTurning() const override
	{
		return false;
	}

	// все ли юниты находятся в rest состоянии?
	bool IsEveryUnitResting() const;
	bool IsAnyUnitResting() const;
	bool IsEveryUnitInTransport() const;
	void Stop() override;
	void StopTurning() override;

	virtual void ForceGoByRightDir() {}

	struct IStatesFactory *GetStatesFactory() const override;

	const int Size() const
	{
		return soldiers.size();
	}

	class CSoldier *operator[]( const int n ) const
	{
		NI_ASSERT( ( n >= 0 && n < Size() ), "Wrong unit number" );
		return soldiers[n];
	}

	bool CanCommandBeExecuted( class CAICommand *pCommand ) override;
	bool CanCommandBeExecutedByStats( class CAICommand *pCommand ) override;
	bool CanCommandBeExecutedByStats( int nCmd ) const override;

	//
	const bool CanShootToPlanes() const override;

	int GetNGuns() const override
	{
		return guns.size();
	}
	class CBasicGun *GetGun( const int n ) const override;

	class CBasicGun *ChooseGunForStatObj( class CStaticObject *pObj, NTimer::STime *pTime ) override;

	const uint8_t GetPlayer() const override
	{
		return cPlayer;
	}

	void ChangePlayer( const uint8_t cPlayer ) override;

	void SetSelectable( bool bSelectable, bool bSendToWorld ) override;

	const float GetSightRadius() const override;

	//
	void SetToWaitingState()
	{
		bWaiting = true;
	}

	void UnsetFromWaitingState()
	{
		bWaiting = false;
	}

	const bool IsInWaitingState() const
	{
		return bWaiting;
	}

	const bool IsVisible( const uint8_t party ) const override;

	//
	void WasHitNearUnit();

	void Fired( const float fGunRadius, const int nGun ) override {}

	const NTimer::STime GetTimeToCamouflage() const override;
	void SetCamoulfage() override;
	void RemoveCamouflage( ECamouflageRemoveReason eReason ) override;

	void UpdateArea( const EActionNotify eAction ) override;

	class CTurret *GetTurret( const int nTurret ) const override
	{
		return 0;
	}

	const int GetNTurrets() const override
	{
		return 0;
	}

	bool IsMech() const override
	{
		return false;
	}

	void Disappear() override;
	void Die( const bool fromExplosion, const float fDamage ) override;

	const float GetSightMultiplier() const;

	bool IsAllowedLieDown() const;
	bool IsAllowedStandUp() const;

	void Disable();
	void Enable();

	bool IsDisabled() const
	{
		return bDisabled;
	}

	void UnitCommand( CAICommand *pCommand, bool bPlaceInQueue, bool bOnlyThisUnitCommand ) override;

	bool IsFree() const override
	{
		return eInsideType == EOIO_NONE;
	}

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

	void SetFree();
	void SetInBuilding( class CBuilding *pBuilding );
	void SetInTransport( class CMilitaryCar *pUnit );
	void SetInEntrenchment( class CEntrenchment *pEntrenchment );

	class CBuilding *GetBuilding() const;
	class CEntrenchment *GetEntrenchment() const;
	class CMilitaryCar *GetTransportUnit() const;

	void GetNewUnitInfo( struct SNewUnitInfo *pNewUnitInfo ) override;

	bool IsFormation() const override
	{
		return true;
	}

	const bool IsInfantry() const override
	{
		return true;
	}

	void SendAcknowledgement( EUnitAckType ack, bool bForce = false ) override;
	void SendAcknowledgement( CAICommand *pCommand, EUnitAckType ack, bool bForce = false ) override;
	// установить центр формации в центр тяжести юнитов
	void BalanceCenter();

	const int GetMinArmor() const override
	{
		return 0;
	}

	const int GetMaxArmor() const override
	{
		return 0;
	}

	const int GetMinPossibleArmor( const int nSide ) const override
	{
		return 0;
	}

	const int GetMaxPossibleArmor( const int nSide ) const override
	{
		return 0;
	}

	const int GetArmor( const int nSide ) const override
	{
		return 0;
	}

	const int GetRandomArmor( const int nSide ) const override
	{
		return 0;
	}

	float GetMaxFireRange() const override
	{
		return fMaxFireRange;
	}

	void AddAvailCmd( const EActionCommand &eCmd )
	{
		availCommands.SetData( eCmd );
	}

	EUnitAckType GetGunsRejectReason() const override;

	// используется только для отложенных updates
	virtual const bool IsVisible( const int nParty ) const
	{
		return true;
	}

	void GetTilesForVisibility( CTilesSet *pTiles ) const override
	{
		pTiles->clear();
	}

	bool ShouldSuspendAction( const EActionNotify &eAction ) const override
	{
		return false;
	}

	const int VirtualUnitsSize() const
	{
		return nVirtualUnits;
	}

	const int GetVirtualUnitSlotInStats( const int nVirtualUnit ) const;

	class CSoldier *GetVirtualUnit( int nIndex ) const
	{
		return virtualUnits[nIndex].pSoldier;
	}

	void AddVirtualUnit( class CSoldier *pSoldier, const int nSlotInStats );
	void MakeVirtualUnitReal( class CSoldier *pSoldier );
	void DelVirtualUnit( class CSoldier *pSoldier );

	// for bored condition
	void UnRegisterAsBored( const enum EUnitAckType eBoredType ) override;
	void RegisterAsBored( const enum EUnitAckType eBoredType ) override;

	// устанавливает солдату бонусы от формации
	void SetGeometryPropertiesToSoldier( class CSoldier *pSoldier, const bool bChangeWarFog );

	// для переноса миномета
	void SetCarryedMortar( class CAIUnit *pMortar );
	bool HasMortar() const; // true when formation carryes a mortar
	CAIUnit *InstallCarryedMortar();

	void ResetTargetScan() override;
	// просканировать, если пора; если нашли цель, то атаковать
	uint8_t AnalyzeTargetScan( CAIUnit *pCurTarget,
	                           const bool bDamageUpdated,
	                           const bool bScanForObstacles,
	                           CObjectBase *pCheckBuilding ) override;
	void LookForTarget( CAIUnit *pCurTarget,
	                    const bool bDamageUpdated,
	                    CAIUnit **pBestTarget,
	                    class CBasicGun **pGun ) override;

	bool CanMoveForGuard() const override
	{
		return CanMove();
	}

	float GetPriceMax() const override;

	const NTimer::STime GetBehUpdateDuration() const override
	{
		return SConsts::BEH_UPDATE_DURATION;
	}

	//to protect from human resupply formation in some states
	virtual bool IsResupplyable() const
	{
		return bCanBeResupplied;
	}

	virtual void SetResupplyable( const bool _bCanBeResupplied )
	{
		bCanBeResupplied = _bCanBeResupplied;
	}

	virtual const bool IsWithMoraleOfficer() const
	{
		return bWithMoraleOfficer;
	}

	void FreezeByState( const bool bFreeze ) override;

	bool HasCharge()
	{
		return !bUsedCharge;
	}

	void UsedCharge();
	void AttachCharge( CMineStaticObject *pMine );
	void DetonateCharge();

	// slow working
	const float GetTargetScanRadius() override;

	bool CanMoveAfterUserCommand() const override
	{
		return CanMove();
	}

	void NotifyAbilityRun( class CAICommand *pCommand ) override;

	bool ThrowGrenade();

	void SetInUnitsID( const int nID )
	{
		nInUnitsID = nID;
	}

	const int GetInUnitsID() const
	{
		return nInUnitsID;
	}

	// доступ к положениям отдельных юнитов
	const CVec2 GetUnitCoord( const CBasePathUnit *pSoldier ) const;
	const CVec2 GetUnitDir( const CBasePathUnit *pSoldier ) const;

	const float GetMaxProjection() const;
	const float GetRadius() const;
	//
	void StopFormationCenter();

	void MoveGeometries2Center();

	const SRect &GetUnitRect() const override;

	// от нового CBasePathUnit
	const EAIClasses GetAIPassabilityClass() const override
	{
		return EAC_HUMAN;
	}

	const float GetTurnRadius() const override
	{
		return 0.0f;
	}

	const bool IsRound() const override
	{
		return true;
	}

	const bool IsDangerousDirExist() const override
	{
		return false;
	}

	const uint16_t GetDangerousDir() const override
	{
		return 0;
	}

	const float GetMaxPossibleSpeed() const override
	{
		return fMaxSpeed / fSpeedCoeff;
	}

	const CVec2 &GetAABBHalfSize() const override
	{
		return vAABBHalfSize;
	}

	const CVec2 GetCenterShift() const override
	{
		return VNULL2;
	}

	const float GetTurnSpeed() const override
	{
		return 0;
	}

	const SUnitProfile &GetUnitProfile() const override;

	const bool CanGoBackward() const override
	{
		return false;
	}

	const bool CanTurnRound() const override
	{
		return true;
	}

	const bool CanRotate() const override
	{
		return true;
	}

	const float GetSmoothTurnThreshold() const override
	{
		return 0.6f;
	}

	const int GetBoundTileRadius() const override
	{
		return nBoundTileRadius;
	}

	// возвращает - поехал или нет
	const bool SendAlongPath( struct IStaticPath *pStaticPath, const CVec2 &vShift, const bool bSmoothTurn ) override;
	const bool SendAlongPath( IPath *pPath ) override;

	// from previous B2 CFormation and CFormationCenter
	const bool IsTrain() const override
	{
		return false;
	}

	bool IsInTankPit() const;

	void WarFogChanged() override {}

	const bool CanUnitTrampled( const CBasePathUnit *pUnit ) const override
	{
		return false;
	}

	const NDb::SUnitSpecialAblityDesc *GetUnitAbilityDesc( const NDb::EUnitSpecialAbility eType ) override;

	const bool IsStaticUnit() const override
	{
		return false;
	}

	const bool CanCatchArtillery( const CArtillery *pArtillery ) const;
	void ResetCatchArtTimer();

	void SetCatchArtFlag( const uint32_t _dwCatchArtFlag )
	{
		dwCatchArtFlag = _dwCatchArtFlag;
	}

	void QuickLoadToMechUnit( CAITransportUnit *pTransport );
};
