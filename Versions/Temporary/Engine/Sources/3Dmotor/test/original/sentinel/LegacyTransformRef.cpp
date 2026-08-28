// Exposes the legacy inline-__asm MMXTransformVector2/3 under distinct names.
// Compiled on its own because it and the .asm-backed header both define these in
// namespace original; keeping each in its own translation unit avoids any renaming
// and means the sentinel compares against the literal original rather than a copy.

#include "3Dmotor/stdafx.h"
#include "SentinelRef.h"

#include "../../original.h"

void LegacyTransform2( NGfx::SCompactVector &res, const NGfx::SCompactVector &src,
    const SHMatrix &m1, uint8_t w1, const SHMatrix &m2, uint8_t w2 )
{
    original::MMXTransformVector2( res, src, m1, w1, m2, w2 );
}

void LegacyTransform3( NGfx::SCompactVector &res, const NGfx::SCompactVector &src,
    const SHMatrix &m1, uint8_t w1, const SHMatrix &m2, uint8_t w2,
    const SHMatrix &m3, uint8_t w3 )
{
    original::MMXTransformVector3( res, src, m1, w1, m2, w2, m3, w3 );
}
