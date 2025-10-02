#pragma once

class CMapObj;
namespace NDb
{
	struct SHPObjectRPGStats;
}

interface IMissionSuperWeapon : public CObjectBase
{
	virtual void OnUpdateSuperWeaponControl( CMapObj *pMO, const NDb::SHPObjectRPGStats *pDBUnit, bool bExist ) = 0;
	virtual void OnUpdateSuperWeaponRecycle( float fProgress ) = 0;
};


