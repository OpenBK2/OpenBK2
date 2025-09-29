#ifndef __UNIT_GUNS_H__
#define __UNIT_GUNS_H__
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma ONCE
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "Guns.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
interface IStaticPath;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*								  Все оружия юнита																*
//*******************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CUnitGuns : public CAIObjectBase
{
	struct SWeaponPathInfo
	{
		float fRadius;
		NTimer::STime time;
		CPtr<IStaticPath> pStaticPath;
	};

	ZDATA
	float fMaxFireRange;
	bool bCanShootToPlanes;

	vector< CPtr<SCommonGunInfo> > commonGunsInfo;
	vector< CObj<CBasicGun> > guns;
	vector<int> gunsBegins;
	int nCommonGuns;
	// с priority 0
	int nMainGun;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&fMaxFireRange); f.Add(3,&bCanShootToPlanes); f.Add(4,&commonGunsInfo); f.Add(5,&guns); f.Add(6,&gunsBegins); f.Add(7,&nCommonGuns); f.Add(8,&nMainGun); return 0; }

	//
	void FindTimeToTurn( class CAIUnit *pOwner, const WORD wWillPower, class CTurret *pTurret, class CAIUnit *pEnemy, const SVector &finishTile, const bool bIsEnemyInFireRange, NTimer::STime *pTimeToTurn ) const;

	bool FindTimeToStatObjGo( class CAIUnit *pUnit, class CStaticObject *pObj, const SWeaponRPGStats *pStats, CUnitGuns::SWeaponPathInfo &pInfo ) const;
public:
	CUnitGuns() : fMaxFireRange( -1 ), bCanShootToPlanes( false ), nCommonGuns( 0 ), nMainGun( 0 ) { }
	virtual void Init( class CCommonUnit *pCommonUnit ) = 0;

	bool AddGun( const interface IGunsFactory &gunsFactory, const int nPlatform, const int nGunInStats, const SWeaponRPGStats *pWeapon, int *nGuns, const int nAmmo );
	void SetOwner( class CAIUnit *pUnit );
	
	const BYTE GetNTotalGuns() const { return guns.size(); }
	void Segment();

	//
	virtual int GetNGuns() const { return guns.size(); }
	virtual class CBasicGun* GetGun( const int n ) const { return guns[n]; }
	// если есть пушки, которыми можно пристреливаться, то выдаёт первую из них, иначе 0
	virtual class CBasicGun* GetFirstArtilleryGun() const = 0;

	class CBasicGun* ChooseGunForStatObj( class CAIUnit *pOwner, class CStaticObject *pObj, NTimer::STime *pTime );
	
	const bool CanShootToPlanes() const { return bCanShootToPlanes; }
	float GetMaxFireRange( const class CAIUnit *pOwner ) const;
	
	const int GetNCommonGuns() const { return nCommonGuns; }
	const SBaseGunRPGStats& GetCommonGunStats( const int nCommonGun ) const;
	int GetNAmmo( const int nCommonGun ) const;
	// nAmmo со знаком
	void ChangeAmmo( const int nCommonGun, const int nAmmo );
	bool IsCommonGunFiring( const int nCommonGun ) const { return commonGunsInfo[nCommonGun]->bFiring; }

	// даёт reject reason самого приоритетного gun из тех, кто отказался стрелять
	const EUnitAckType GetRejectReason() const;
	bool DoesExistRejectReason( const EUnitAckType &ackType ) const;
	
	// gun с priority 0
	class CBasicGun* GetMainGun() const;

	virtual const int GetActiveShellType() const = 0;
	virtual bool SetActiveShellType( const NDb::SWeaponRPGStats::SShell::EShellDamageType eShellType ) = 0;
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CMechUnitGuns : public CUnitGuns
{
	OBJECT_BASIC_METHODS( CMechUnitGuns );	

	ZDATA_(CUnitGuns)
	int nFirstArtGun;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CUnitGuns*)this); f.Add(2,&nFirstArtGun); return 0; }
public:
	CMechUnitGuns() : nFirstArtGun( -1 ) { }
	virtual void Init( class CCommonUnit *pCommonUnit );

	virtual bool SetActiveShellType( const NDb::SWeaponRPGStats::SShell::EShellDamageType eShellType );
	virtual const int GetActiveShellType() const;

	virtual class CBasicGun* GetFirstArtilleryGun() const;
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CInfantryGuns : public CUnitGuns
{
	OBJECT_BASIC_METHODS( CInfantryGuns );	
	ZDATA_(CUnitGuns)
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CUnitGuns*)this); return 0; }
public:
	virtual void Init( class CCommonUnit *pCommonUnit );

	virtual class CBasicGun* GetFirstArtilleryGun() const { return 0; }

	virtual bool SetActiveShellType( const NDb::SWeaponRPGStats::SShell::EShellDamageType eShellType ) { return false; }
	virtual const int GetActiveShellType() const { return NDb::SWeaponRPGStats::SShell::DAMAGE_HEALTH; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // __UNIT_GUNS_H__
