#ifndef __SHOOT_ESTIMATOR_H__
#define __SHOOT_ESTIMATOR_H__

#pragma ONCE
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
interface IShootEstimator : public CAIObjectBase
{
	virtual void Reset( class CAIUnit *pCurEnemy, const bool bDamageUpdated, const DWORD dwForbidden ) = 0;
	virtual void AddUnit( class CAIUnit *pUnit ) = 0;
	virtual class CAIUnit* GetBestUnit() const = 0;
	virtual class CBasicGun* GetBestGun() const = 0;
	virtual const int GetNumberOfBestGun() const = 0;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif __SHOOT_ESTIMATOR_H__

