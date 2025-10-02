#pragma once

#include "SuperWeapon.h"
#include "../UI/UI.h"
#include "../UISpecificB2/UISpecificB2.h"

namespace NDb
{
	enum ESeason;
}
class CWorldClient;

class CMissionSuperWeapon : public IMissionSuperWeapon
{
	OBJECT_NOCOPY_METHODS( CMissionSuperWeapon );

	ZDATA
	CPtr<IButton> pSuperWeaponBtn;
	CPtr<IWindowRoundProgressBar> pSuperWeaponProgress;
	CPtr<IWindow> pSuperWeaponMaskWnd;
	bool bExist; // super weapon presents in mission
	bool bEnabled; // super weapon can be used now (by logic info)
	float fProgress; // clock [0..1]
	bool bActive; // super weapon mode
	CObj<class CMissionUnitFullInfo> pUnitFullInfo;
	CPtr<CWorldClient> pWorld;
	bool bDisabledByInterface;
	CPtr<CMapObj> pMO;
	CDBPtr<NDb::SHPObjectRPGStats> pDBUnit;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pSuperWeaponBtn); f.Add(3,&pSuperWeaponProgress); f.Add(4,&pSuperWeaponMaskWnd); f.Add(5,&bExist); f.Add(6,&bEnabled); f.Add(7,&fProgress); f.Add(8,&bActive); f.Add(9,&pUnitFullInfo); f.Add(10,&pWorld); f.Add(11,&bDisabledByInterface); f.Add(12,&pMO); f.Add(13,&pDBUnit); return 0; }
private:	
	void UpdateVisual();
	bool IsEnabled() const;
public:
	CMissionSuperWeapon();
	
	void Init( IWindow *pScr, CWorldClient *pWorld, NDb::ESeason eSeason );
	
	//{ IMissionSuperWeapon
	void OnUpdateSuperWeaponControl( CMapObj *pMO, const NDb::SHPObjectRPGStats *pDBUnit, bool bExist );
	void OnUpdateSuperWeaponRecycle( float fProgress );
	//}

	void Call( const CVec2 &vPos );
	bool CanActivate() const;
	void Activate();
	void Deactivate();
	void UpdateObject( CMapObj *pMO );
};


