// Exposes the .asm-backed MMXTransformVector2/3 under distinct names, for the same
// reason as LegacyTransformRef.cpp.

#include "3Dmotor/stdafx.h"
#include "SentinelRef.h"

#include "../MMXTransformVector.h"

void AsmTransform2( NGfx::SCompactVector &res, const NGfx::SCompactVector &src,
    const SHMatrix &m1, uint8_t w1, const SHMatrix &m2, uint8_t w2 )
{
    original::MMXTransformVector2( res, src, m1, w1, m2, w2 );
}

void AsmTransform3( NGfx::SCompactVector &res, const NGfx::SCompactVector &src,
    const SHMatrix &m1, uint8_t w1, const SHMatrix &m2, uint8_t w2,
    const SHMatrix &m3, uint8_t w3 )
{
    original::MMXTransformVector3( res, src, m1, w1, m2, w2, m3, w3 );
}
