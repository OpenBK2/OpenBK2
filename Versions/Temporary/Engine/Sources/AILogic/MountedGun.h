#pragma once
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "GunsInternal.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CMountedTurret;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CCommonMountedGun
{
	public: int operator&( IBinSaver &saver ); private:
	
	CDBPtr<SBuildingRPGStats> pBuildingStats;
	class CBuilding *pBuilding;
	CPtr<CMountedTurret> pTurret;
	int nSlot;
public:
	CCommonMountedGun() { }
	CCommonMountedGun( class CBuilding *pObject, class CMountedTurret *pTurret, const int nSlot );

	class CBuilding *GetBuilding() const { return pBuilding; };

	virtual const SBaseGunRPGStats& GetGun() const;
	virtual const SWeaponRPGStats* GetWeapon() const;
	virtual bool IsOnTurret() const { return true; }

	virtual class CTurret* GetTurret() const;

	virtual void GetInfantryShotInfo( SAINotifyInfantryShot *pInfantryShotInfo, const NTimer::STime &time ) const;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// new saveload & CObjectBase system doesn't support templated ptr
//template< class T >
//class CMountedGun : public T, public CCommonMountedGun
class CMountedToBaseGun : public CBaseGun, public CCommonMountedGun
{
	OBJECT_NOCOPY_METHODS(CMountedToBaseGun);
	ZDATA_(CBaseGun)
	ZPARENT(CCommonMountedGun)
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CBaseGun*)this); f.Add(2,(CCommonMountedGun*)this); return 0; }
public:
	CMountedToBaseGun() { }

	CMountedToBaseGun( class CBuilding *pObject, class CMountedTurret *pTurret, const int nSlot, const BYTE nShellType, SCommonGunInfo *pCommonGunInfo, IGunsFactory::EGunTypes eType ) 
	: CCommonMountedGun( pObject, pTurret, nSlot ), CBaseGun( 0, nShellType, pCommonGunInfo, eType ) { }

	virtual const SBaseGunRPGStats& GetGun() const { return CCommonMountedGun::GetGun(); }
	virtual const SWeaponRPGStats* GetWeapon() const { return CCommonMountedGun::GetWeapon(); }
	virtual bool IsOnTurret() const { return CCommonMountedGun::IsOnTurret(); }

	virtual class CTurret* GetTurret() const { return CCommonMountedGun::GetTurret(); }
	virtual class CBuilding *GetMountBuilding() const { return CCommonMountedGun::GetBuilding(); }

	virtual void GetInfantryShotInfo( SAINotifyInfantryShot *pInfantryShotInfo, const NTimer::STime &time ) const;
};
/*
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
void CMountedToBaseGun<T>::GetInfantryShotInfo( SAINotifyInfantryShot *pInfantryShotInfo, const NTimer::STime &time ) const
void CMountedToBaseGun::GetInfantryShotInfo( SAINotifyInfantryShot *pInfantryShotInfo, const NTimer::STime &time ) const
{
	CCommonMountedGun::GetInfantryShotInfo( pInfantryShotInfo, time );
	//pInfantryShotInfo->cShell = T::nShellType;
	pInfantryShotInfo->cShell = CBaseGun::nShellType;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
int CMountedToBaseGun<T>::operator&( IBinSaver &saver )
int CMountedToBaseGun::operator&( IBinSaver &saver )
{
	//saver.AddTypedSuper( 1, static_cast<T*>(this) );
	saver.AddTypedSuper( 1, static_cast<CBaseGun*>(this) );
	saver.AddTypedSuper( 2, static_cast<CCommonMountedGun*>(this) );
	return 0;
}*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CMountedGunsFactory : public IGunsFactory
{
	class CBuilding *pBuilding;
	class CMountedTurret *pMountedTurret;
	const int nCommonGun;
public:
	CMountedGunsFactory( class CBuilding *_pBuilding, class CMountedTurret *_pMountedTurret, const int _nCommonGun )
		: pBuilding( _pBuilding ), pMountedTurret( _pMountedTurret ), nCommonGun( _nCommonGun ) { }
	
	virtual int GetNCommonGun() const { return nCommonGun; }

	virtual class CBasicGun* CreateGun( const EGunTypes eType, const int nShell, SCommonGunInfo *pCommonGunInfo ) const;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

