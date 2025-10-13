#pragma once

#include <cstdint>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct IShootEstimator : public CAIObjectBase
{
	virtual void Reset( class CAIUnit *pCurEnemy, const bool bDamageUpdated, const uint32_t dwForbidden ) = 0;
	virtual void AddUnit( class CAIUnit *pUnit ) = 0;
	virtual class CAIUnit* GetBestUnit() const = 0;
	virtual class CBasicGun* GetBestGun() const = 0;
	virtual const int GetNumberOfBestGun() const = 0;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

