#pragma once

#include "CommonUnit.h"
#include "../Stats_B2_M1/RPGStats.h"

class CArtillery;
class CCommonPathFinder;
class CBasePathUnit;
class CGroupSmoothPath;
class CSoldier;
class CMineStaticObject;
class CAITransportUnit;
enum EActionCommand;
struct ICollisionsCollector;
namespace NDb
{
	struct SUnitBaseRPGStats;
	struct SSquadRPGStats;
}

class CFormation : public CCommonUnit
{
	OBJECT_BASIC_METHODS( CFormation );

	// для хранения данных о переносимом миномете
	class CCarryedMortar
	{
		ZDATA
		bool bHasMortar;
		CDBPtr<SUnitBaseRPGStats> pStats;
		float fHP;
		vector<int> ammo;
	public:
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&bHasMortar); f.Add(3,&pStats); f.Add(4,&fHP); f.Add(5,&ammo); return 0; }
	public:
		CCarryedMortar() : bHasMortar( false ), fHP( 0.0f ) { }
		bool HasMortar() const { return bHasMortar; } 
		class CAIUnit* CreateMortar( class CFormation *pOwner );
		void Init( const class CAIUnit *pArt );
	};

	struct SGunInfo
	{
		int nUnit;
		int nUnitGun;

		SGunInfo() : nUnit( -1 ), nUnitGun( -1 ) { }
		SGunInfo( const int _nUnit, const int _nUnitGun ) : nUnit( _nUnit ), nUnitGun( _nUnitGun ) { }
	};

	enum EObjectInsideOf { EOIO_NONE, EOIO_BUILDING, EOIO_TRANSPORT, EOIO_ENTRENCHMENT, EOIO_UNKNOWN };

	struct SVirtualUnit
	{
		ZDATA
		CPtr<CSoldier> pSoldier;
		int nSlotInStats;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pSoldier); f.Add(3,&nSlotInStats); return 0; }
		SVirtualUnit() : pSoldier( 0 ) { }

	};

	ZDATA_( CCommonUnit )
	float fPass;

	NTimer::STime timeToCamouflage;

	CArray1Bit availCommands;
	//
	BYTE cPlayer;

	vector<SGunInfo> guns;

	bool bWaiting;
	CDBPtr<NDb::SSquadRPGStats> pStats;

	bool bDisabled;

	EObjectInsideOf eInsideType;
	CPtr<CObjectBase> pObjInside;

	float fMaxFireRange;

	vector<SVirtualUnit> virtualUnits;
	int nVirtualUnits;
	bool bCanBeResupplied;

	CCarryedMortar mortar;
	bool bBoredInMoveFormationSent;
	NTimer::STime lastBoredInMoveFormationCheck;

	bool bWithMoraleOfficer;

	bool bUsedCharge;
	CPtr<CMineStaticObject> pCharge;
	int nInUnitsID;

	CPtr<CGroupSmoothPath> pGroupSmoothPath;
	vector< CPtr<CSoldier> > soldiers;
	float fMaxSpeed;
	float fSpeedCoeff;
	CVec2 vAABBHalfSize;
	int nBoundTileRadius;

	NTimer::STime timeLastCatchArt;
	DWORD dwCatchArtFlag;
public: 
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CCommonUnit *)this); f.Add(2,&fPass); f.Add(3,&timeToCamouflage); f.Add(4,&availCommands); f.Add(5,&cPlayer); f.Add(6,&guns); f.Add(7,&bWaiting); f.Add(8,&pStats); f.Add(9,&bDisabled); f.Add(10,&eInsideType); f.Add(11,&pObjInside); f.Add(12,&fMaxFireRange); f.Add(13,&virtualUnits); f.Add(14,&nVirtualUnits); f.Add(15,&bCanBeResupplied); f.Add(16,&mortar); f.Add(17,&bBoredInMoveFormationSent); f.Add(18,&lastBoredInMoveFormationCheck); f.Add(19,&bWithMoraleOfficer); f.Add(20,&bUsedCharge); f.Add(21,&pCharge); f.Add(22,&nInUnitsID); f.Add(23,&pGroupSmoothPath); f.Add(24,&soldiers); f.Add(25,&fMaxSpeed); f.Add(26,&fSpeedCoeff); f.Add(27,&vAABBHalfSize); f.Add(28,&nBoundTileRadius); f.Add(29,&timeLastCatchArt); f.Add(30,&dwCatchArtFlag); return 0; }
	//
	void InitGeometries();
	void PrepareToDelete();

	bool IsMemberResting( class CSoldier *pSoldier ) const;

	void SetMaxSpeed( const float _fMaxSpeed ) { fMaxSpeed = _fMaxSpeed; }
	void SetBoundTileRadius( const int _nBoundTileRadius ) { nBoundTileRadius = _nBoundTileRadius; }
	void SetAABBHalfSize( const CVec2 &_vAABBHalfSize ) { vAABBHalfSize = _vAABBHalfSize; }
	void UpdateStats( class CSoldier *pUnit, const int nPos );
protected:
	virtual ISmoothPath *CreateSmoothPath();
	virtual void UpdatePlacement( const CVec3 &vOldPosition, const WORD wOldDirection, const bool bNeedUpdate ) {};

public:
	CFormation();
	void Init( const SSquadRPGStats *pStats, const CVec2 &center, const int z, const WORD dir, ICollisionsCollector *pCollisionsCollector );
	// передвигает центр формации в её центр масс и инициализирует geomInfo
	void ChangeGeometry( const int nGeometry );
	const int GetGeometriesCount() const;
	const int GetCurrentGeometry() const;

	const NDb::SSquadRPGStats *GetStats() const { return pStats; }

	// добавить новый юнит в формацию, порядковый номер его в статах - nSlot, местоположение юнита инициализируется
	void AddNewUnitToSlot( class CSoldier *pUnit, const bool bSendToWorld );

	// добавить новый юнит на позицию nPos в списке юнитов объекта formation, причём местоположение юнита не инициализируется
	void AddSoldier( class CSoldier *pUnit );
	void DeleteSoldier( class CSoldier *pUnit );

	// возвращает позицию в статах формации для юнита с порядковым номером cSlot в массиве units 
	const int GetUnitSlotInStats( const BYTE cSlot ) const;
	virtual const float GetPassability() const { return fPass; }

	virtual void Segment();

	virtual const bool IsIdle() const;
	virtual const bool IsTurning() const { return false; }
	// все ли юниты находятся в rest состоянии?
	bool IsEveryUnitResting() const;
	bool IsAnyUnitResting() const;
	bool IsEveryUnitInTransport() const;
	virtual void Stop();
	virtual void StopTurning();
	virtual void ForceGoByRightDir() {}

	virtual struct IStatesFactory* GetStatesFactory() const;

	const int Size() const { return soldiers.size(); }
	class CSoldier* operator[]( const int n ) const { NI_ASSERT( ( n >= 0 && n < Size() ), "Wrong unit number" ); return soldiers[n]; }

	virtual bool CanCommandBeExecuted( class CAICommand *pCommand );
	virtual bool CanCommandBeExecutedByStats( class CAICommand *pCommand );
	virtual bool CanCommandBeExecutedByStats( int nCmd ) const;

	//
	virtual const bool CanShootToPlanes() const;
	virtual int GetNGuns() const { return guns.size(); }
	virtual class CBasicGun* GetGun( const int n ) const;

	virtual class CBasicGun* ChooseGunForStatObj( class CStaticObject *pObj, NTimer::STime *pTime );

	virtual const BYTE GetPlayer() const { return cPlayer; }
	virtual void ChangePlayer( const BYTE cPlayer );

	virtual void SetSelectable( bool bSelectable, bool bSendToWorld );

	virtual const float GetSightRadius() const;

	//
	void SetToWaitingState() { bWaiting = true; }
	void UnsetFromWaitingState() { bWaiting = false; }
	const bool IsInWaitingState() const { return bWaiting; }

	virtual const bool IsVisible( const BYTE party ) const;

	//
	void WasHitNearUnit();

	virtual void Fired( const float fGunRadius, const int nGun  ) { }

	virtual const NTimer::STime GetTimeToCamouflage() const;
	virtual void SetCamoulfage();
	virtual void RemoveCamouflage( ECamouflageRemoveReason eReason );

	virtual void UpdateArea( const EActionNotify eAction );

	virtual class CTurret* GetTurret( const int nTurret ) const { return 0; }
	virtual const int GetNTurrets() const { return 0; }
	virtual bool IsMech() const { return false; }

	virtual void Disappear();
	virtual void Die( const bool fromExplosion, const float fDamage );

	const float GetSightMultiplier() const;

	bool IsAllowedLieDown() const;
	bool IsAllowedStandUp() const;

	void Disable();
	void Enable();
	bool IsDisabled() const { return bDisabled; }

	virtual void UnitCommand( CAICommand *pCommand, bool bPlaceInQueue, bool bOnlyThisUnitCommand );

	bool IsFree() const { return eInsideType == EOIO_NONE; }
	bool IsInBuilding() const { return eInsideType == EOIO_BUILDING; }
	bool IsInEntrenchment() const { return eInsideType == EOIO_ENTRENCHMENT; }
	bool IsInTransport() const { return eInsideType == EOIO_TRANSPORT; }

	void SetFree();
	void SetInBuilding( class CBuilding *pBuilding );
	void SetInTransport( class CMilitaryCar *pUnit );
	void SetInEntrenchment( class CEntrenchment *pEntrenchment );

	class CBuilding* GetBuilding() const;
	class CEntrenchment* GetEntrenchment() const;
	class CMilitaryCar* GetTransportUnit() const;

	virtual void GetNewUnitInfo( struct SNewUnitInfo *pNewUnitInfo );

	virtual bool IsFormation() const { return true; }
	virtual const bool IsInfantry() const { return true; }

	virtual void SendAcknowledgement( EUnitAckType ack, bool bForce = false );
	virtual void SendAcknowledgement( CAICommand *pCommand, EUnitAckType ack, bool bForce = false );
	// установить центр формации в центр тяжести юнитов
	void BalanceCenter();

	virtual const int GetMinArmor() const { return 0; }
	virtual const int GetMaxArmor() const { return 0; }
	virtual const int GetMinPossibleArmor( const int nSide ) const { return 0; }
	virtual const int GetMaxPossibleArmor( const int nSide ) const { return 0; }
	virtual const int GetArmor( const int nSide ) const { return 0; }
	virtual const int GetRandomArmor( const int nSide ) const { return 0; }

	virtual float GetMaxFireRange() const { return fMaxFireRange; }
	void AddAvailCmd( const EActionCommand &eCmd ) { availCommands.SetData( eCmd ); }

	virtual EUnitAckType GetGunsRejectReason() const;

	// используется только для отложенных updates
	virtual const bool IsVisible( const int nParty ) const { return true; }
	virtual void GetTilesForVisibility( CTilesSet *pTiles ) const { pTiles->clear(); }
	virtual bool ShouldSuspendAction( const EActionNotify &eAction ) const { return false; }

	const int VirtualUnitsSize() const { return nVirtualUnits; }
	const int GetVirtualUnitSlotInStats( const int nVirtualUnit ) const;
	void AddVirtualUnit( class CSoldier *pSoldier, const int nSlotInStats );
	void MakeVirtualUnitReal( class CSoldier *pSoldier );
	void DelVirtualUnit( class CSoldier *pSoldier );

	// for bored condition
	virtual void UnRegisterAsBored( const enum EUnitAckType eBoredType );
	virtual void RegisterAsBored( const enum EUnitAckType eBoredType );

	// устанавливает солдату бонусы от формации
	void SetGeometryPropertiesToSoldier( class CSoldier *pSoldier, const bool bChangeWarFog );

	// для переноса миномета
	void SetCarryedMortar( class CAIUnit *pMortar );
	bool HasMortar() const ;							// true when formation carryes a mortar
	CAIUnit* InstallCarryedMortar();

	virtual void ResetTargetScan();
	// просканировать, если пора; если нашли цель, то атаковать
	virtual BYTE AnalyzeTargetScan(	CAIUnit *pCurTarget, const bool bDamageUpdated, const bool bScanForObstacles, CObjectBase *pCheckBuilding );
	virtual void LookForTarget( CAIUnit *pCurTarget, const bool bDamageUpdated, CAIUnit **pBestTarget, class CBasicGun **pGun );

	virtual bool CanMoveForGuard() const { return CanMove(); }
	virtual float GetPriceMax() const;
	virtual const NTimer::STime GetBehUpdateDuration() const { return SConsts::BEH_UPDATE_DURATION; }
	//to protect from human resupply formation in some states
	virtual bool IsResupplyable() const { return bCanBeResupplied; }
	virtual void SetResupplyable( const bool _bCanBeResupplied ) { bCanBeResupplied = _bCanBeResupplied; }

	virtual const bool IsWithMoraleOfficer() const { return bWithMoraleOfficer; }

	virtual void FreezeByState( const bool bFreeze );

	bool HasCharge() { return !bUsedCharge; }
	void UsedCharge();
	void AttachCharge( CMineStaticObject *pMine );
	void DetonateCharge();

	// slow working
	virtual const float GetTargetScanRadius();

	virtual bool CanMoveAfterUserCommand() const { return CanMove(); }
	virtual void NotifyAbilityRun( class CAICommand * pCommand );

	bool ThrowGrenade();

	void SetInUnitsID( const int nID ) { nInUnitsID = nID; }
	const int GetInUnitsID() const { return nInUnitsID; }

	// доступ к положениям отдельных юнитов
	const CVec2 GetUnitCoord( const CBasePathUnit *pSoldier ) const;
	const CVec2 GetUnitDir( const CBasePathUnit *pSoldier ) const;

	const float GetMaxProjection() const;
	const float GetRadius() const;
	//
	void StopFormationCenter();

	void MoveGeometries2Center();

	virtual const SRect & GetUnitRect() const;
	// от нового CBasePathUnit
	virtual const EAIClasses GetAIPassabilityClass() const { return EAC_HUMAN; }
	virtual const float GetTurnRadius() const { return 0.0f; }
	virtual const bool IsRound() const { return true; }
	virtual const bool IsDangerousDirExist() const { return false; }
	virtual const WORD GetDangerousDir() const { return 0; }
	virtual const float GetMaxPossibleSpeed() const { return fMaxSpeed / fSpeedCoeff; }
	virtual const CVec2 &GetAABBHalfSize() const { return vAABBHalfSize; }
	virtual const CVec2 GetCenterShift() const { return VNULL2; }
	virtual const float GetTurnSpeed() const { return 0; }
	virtual const SUnitProfile &GetUnitProfile() const;
	virtual const bool CanGoBackward() const { return false; }
	virtual const bool CanTurnRound() const { return true; }
	virtual const bool CanRotate() const { return true; }
	virtual const float GetSmoothTurnThreshold() const { return 0.6f; }
	virtual const int GetBoundTileRadius() const { return nBoundTileRadius; }

	// возвращает - поехал или нет
	virtual const bool SendAlongPath( struct IStaticPath *pStaticPath, const CVec2 &vShift, const bool bSmoothTurn );
	virtual const bool SendAlongPath( IPath *pPath );

	// from previous B2 CFormation and CFormationCenter
	virtual const bool IsTrain() const { return false; }
	bool IsInTankPit() const;
	void WarFogChanged() {}

	virtual const bool CanUnitTrampled( const CBasePathUnit *pUnit ) const { return false; }

	virtual const NDb::SUnitSpecialAblityDesc * GetUnitAbilityDesc( const NDb::EUnitSpecialAbility eType );
	const bool IsStaticUnit() const { return false; }

	const bool CanCatchArtillery( const CArtillery *pArtillery ) const;
	void ResetCatchArtTimer();
	void SetCatchArtFlag( const DWORD _dwCatchArtFlag ) { dwCatchArtFlag = _dwCatchArtFlag; }
	void QuickLoadToMechUnit( CAITransportUnit *pTransport );
};


