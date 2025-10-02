#pragma once

#include "Mine.h"

namespace NDb
{
	struct SPartyDependentInfo;
	struct SAIGameConsts;
	struct SMapInfo;
	struct SMapPlayerInfo;
	enum EReinforcementType;
};
interface ICollisionsCollector;

// для унификации создания самолетов
interface IPlaneCreation 
{
	virtual const CVec2 &GetDestPoint() const = 0;
	virtual void CalcPositions( const int nMax,
		const CVec2 & vAABBbox,
		const CVec2 & vDirection,
		vector<CVec2> *positions,
		CVec2 * pvOffset, const bool bRandom = false ) = 0;

};

ICollisionsCollector;

//
class CPlaneCreation : public IPlaneCreation
{
	CVec2 vDestPoint;
public:
	CPlaneCreation( const CVec2 &vDestPoint )
		: vDestPoint( vDestPoint ) {  }
		virtual const CVec2 &GetDestPoint() const { return vDestPoint; }
};

// для создания маленьких самолетов
class CLightPlaneCreation : public CPlaneCreation
{
public:
	CLightPlaneCreation( const CVec2 &vDestPoint )
		: CPlaneCreation( vDestPoint) {  }
		virtual void CalcPositions( const int nMax,
			const CVec2 & vAABBbox,
			const CVec2 & vDirection,
			vector<CVec2> *positions,
			CVec2 * pvOffset, const bool bRandom = false );
};

// для создания тяжелых самолетов
class CHeavyPlaneCreation : public CPlaneCreation
{
	bool bNeedFormation;
public:
	CHeavyPlaneCreation( const CVec2 &vDestPoint, const bool _bNeedFormation = false )
		: CPlaneCreation( vDestPoint ), bNeedFormation( _bNeedFormation ) {  }
		virtual void CalcPositions( const int nMax,
			const CVec2 & vAABBbox,
			const CVec2 & vDirection,
			vector<CVec2> * positions,
			CVec2 * pvOffset, const bool bRandom = false );
};


//
struct SLocalInGameUnitCreationInfo
{
	ZDATA
	CDBPtr<SSquadRPGStats> pParatrooper;										// название парашютистов
	string szPartyName;												// название страны
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pParatrooper); f.Add(3,&szPartyName); return 0; }
	SLocalInGameUnitCreationInfo & operator=( const struct SMapPlayerInfo &rSUnitCreation );
	SLocalInGameUnitCreationInfo( const struct SMapPlayerInfo &rSUnitCreation );
	void Copy( const struct SMapPlayerInfo &rSUnitCreation );
	SLocalInGameUnitCreationInfo() {  }
};

class CUnitCreation
{
public: 
	struct SFeedBack
	{
		int eEnable, eDisable;		
		SFeedBack( const int eEnable, const int eDisable )
			: eEnable( eEnable ), eDisable( eDisable ) {  }
	};
	vector<SFeedBack> feedbacks;

private:
	ZDATA
	bool bInit;														// for delaying initialization untill segment
	CPtr<ICollisionsCollector> pCollisionsCollector;

	vector<SLocalInGameUnitCreationInfo> inGameUnits;
	// consts
	// читается из xml, сохранять не нужно
	vector<CDBPtr<NDb::SPartyDependentInfo> > partyDependentInfo;
	CDBPtr<NDb::SAIGameConsts> pConsts;
	CDBPtr<NDb::SMapInfo> pCurrentMap;

public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&bInit); f.Add(3,&pCollisionsCollector); f.Add(4,&inGameUnits); f.Add(5,&partyDependentInfo); f.Add(6,&pConsts); f.Add(7,&pCurrentMap); return 0; }
private:
	void InitConsts( const NDb::SAIGameConsts *pConsts );
	const SPartyDependentInfo* GetPartyDependentInfo( const int nDipl ) const;

public:
	CUnitCreation();
	// для редактора
	void Init();
	// для игры
	void SetConsts( const NDb::SAIGameConsts *pConsts );
	void Init( const NDb::SMapInfo *pMapInfo, ICollisionsCollector *pCollisionsCollector );
	void Clear();
	

	// returns number (in new units) of this unit
	int AddNewUnit( const int nUniqueID, const SUnitBaseRPGStats *pStats, 
									const float fHPFactor, 
									const int x, 
									const int y, 
									const int z, 
									const WORD _dir, 
									const BYTE player, 
									bool bInitialization,// = false, 
									bool bSendToWorld,// = true, 
									NDb::EReinforcementType eType) const;
	void GetCentersOfAllFormationUnits( const SSquadRPGStats *pStats, const CVec2 &vFormCenter, const WORD wFormDir, const int nFormation, const int nUnits, list<CVec2> *pCenters ) const;
	class CCommonUnit* AddNewFormation( const SSquadRPGStats *pStats, const int nFormation, const float fHP, const float x, const float y, const float z, const WORD wDir, const int nDiplomacy,
																			bool bInitialization,// = false,
																			bool bSendToWorld,// = true,
																			const int nUnits,// = -1 
																			NDb::EReinforcementType eType ) const;
	class CCommonUnit* CreateSingleUnitFormation( class CSoldier *pSoldier ) const;
	
	CMineStaticObject* CreateMine( const enum NDb::EMineType nType, const class CVec3 &vPoint, const int nPlayer );
	class CFormation* CreateParatroopers( const class CVec3 &where, class CAIUnit * pPlane, const int nScriptID ) const;
	class CFormation* CreateResupplyEngineers( class CAITransportUnit *pUnit ) const;
	CFormation * CreateCrew( class CArtillery *pUnit, const int nUnits,// = -1, 
													 const CVec3 &vPos,// = CVec3(-1,-1,-1), 
													 const int nPlayer,// = -1,
													 const bool bImmidiateAttach// = true 
													 ) const;
	class CAIUnit *CreateTorpedo( class CAIUnit *pOwner, const NDb::SWeaponRPGStats *pWeaponStats, const class CVec2 &vSource, const class CVec2 &vTarget );

	CExistingObject * CreatePlayerFlag( int nPlayer, CStaticObject *pSample );
	// послать Юре формацию, чтобы ее можно было селектить и вообще чтобы она на клиенте существовала
	void SendFormationToWorld( CFormation * pUnit, const bool bSelectable ) const;

	//void LockAppearPoint( const int nPlayer, const bool bLocked );
	CVec2 GetRandomAppearPoint( const int nPlayer, 
															const bool bLeave// = false 
															) const;
	
	const SFenceRPGStats* GetWireFence() const;
	const SObjectBaseRPGStats* GetRandomAntitankObject() const;
	const SEntrenchmentRPGStats* GetEntrenchment() const;
	
	const SMechUnitRPGStats *GetRandomTankPit( const class CVec2 &vSize, const bool bCanDig, float *pfResize ) const;
	const SMechUnitRPGStats * GetFoxHole( const bool bCanDig ) const;
	void Segment();

	bool IsAntiTank( const SHPObjectRPGStats *pStats ) const;
	bool IsAPFence( const SHPObjectRPGStats *pStats ) const;

	const NDb::SVisObj *GetParatrooperVisObj( int nPlayer ) const;
	const NDb::SVisObj *GetParachuteVisObj() const;
	int GetRemoveParachuteTime() const;
	const NDb::SUnitBaseRPGStats * GetMosinStats() const;
	const NDb::SUnitBaseRPGStats * Get152mmML20Stats() const;
	const NDb::SSquadRPGStats* GetSingleUnitFormation() const;
	const NDb::SStaticObjectRPGStats *GetShellBox() const;
	const NDb::SAIExpLevel * GetExpLevels( const enum EUnitRPGType eType );

	const NDb::SMapInfo *GetMap() const { return pCurrentMap; }

	void ApplyWeatherModifier( const bool bForward );
};

