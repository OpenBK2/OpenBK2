#pragma once

// Internal curve sampling, beyond what the 54 entry points expose.
//
// GrannyEvaluateCurveAtT blends a curve's controls as they are, which is right:
// it is what the real DLL does, checked against every curve in the corpus at
// nine values of t each. But an orientation curve's controls are quaternions,
// and q and -q are the same rotation, so a blend of controls that sit on
// opposite sides of that ambiguity is meaningless.
//
// It matters in exactly one place. A looping clip evaluated at the wrap brings
// in a control from the far end of the curve, and the two ends of a rotation
// that has turned a long way are on opposite sides: one bone in this corpus has
// keys whose w runs from +1.0007 at the first to -1.0035 at the last. Blended
// raw, the wrap frame produces a rotation unrelated to either neighbour, which
// is a visible pop once per loop. Blended after putting each control on the near
// side of the one the span is centred on, it is continuous.
//
// So the sampler needs a quaternion-aware evaluation that the public entry point
// does not provide, and this is it.

#include "Structures.h"

namespace NGr2
{

//! Evaluate an orientation curve, with its controls neighbourhooded first.
//!
//! Same spans, same weights and same wrap rules as GrannyEvaluateCurveAtT, and
//! for a curve whose controls are all on one side it produces the same answer.
//! \param pIdentity what an empty curve evaluates to, as for the public entry
//!                  point: for a bone that is its rest orientation.
void EvaluateQuaternion( const SCurve2 &curve, float fT, bool bForwardsLoop,
                         bool bBackwardsLoop, float fCurveDuration,
                         const float *pIdentity, float *pResult );

}
