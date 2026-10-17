#include <cstdint>

// Derived fields must also be initialized before ToAIUnits runs.
uint16_t wDirection = 0;

void ToAIUnits( bool bInEditor )
{
	wDirection = fDirection;
	//wDirection = uint16_t( fDirection * 65536.0f / 360.0f );
}

// Virtual, because SInfantryGun and SMechUnitGun derive from this record and
// the unit code reads a gun through a const SBaseGunRPGStats reference. The
// destructor belongs in this hatch rather than in RPGStats.h, which dbcodegen
// regenerates.
virtual ~SBaseGunRPGStats() = default;

virtual const CVec3 GetShootPointPos() const
{
	return VNULL3;
}
