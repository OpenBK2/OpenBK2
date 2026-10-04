#include <cstdint>

// Derived fields must also be initialized before ToAIUnits runs.
uint16_t wDirection = 0;
void ToAIUnits( bool bInEditor )
{
	wDirection = fDirection;
	//wDirection = uint16_t( fDirection * 65536.0f / 360.0f );
}

virtual const CVec3 GetShootPointPos() const { return VNULL3; }

