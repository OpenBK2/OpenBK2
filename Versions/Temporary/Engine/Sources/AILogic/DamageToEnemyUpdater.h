#ifndef __DAMAGE_TO_ENEMY_UPDATER__
#define __DAMAGE_TO_ENEMY_UPDATER__
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma ONCE
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CAIUnit;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDamageToEnemyUpdater
{
	ZDATA
	int nTakenDamageUpdated;
	float fTakenDamagePower;
	
	CPtr<CAIUnit> pCurEnemy;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&nTakenDamageUpdated); f.Add(3,&fTakenDamagePower); f.Add(4,&pCurEnemy); return 0; }
public:
	CDamageToEnemyUpdater() : nTakenDamageUpdated( 0 ), fTakenDamagePower( 0.0f ) { }

	void SetDamageToEnemy( CAIUnit *pOwner, CAIUnit *pEnemy, const DWORD dwGuns );
	void SetDamageToEnemy( class CAIUnit *pOwner, class CAIUnit *pEnemy, class CBasicGun *pGun );
	void UnsetDamageFromEnemy( class CAIUnit *pEnemy );

	const bool IsDamageUpdated() { return nTakenDamageUpdated != 0; }
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif //__DAMAGE_TO_ENEMY_UPDATER__

