// Sampling a curve: the one thing between a skeleton in its bind pose and one
// that moves.
//
// Every curve in this game is DaK32fC32f, a degree and two float arrays, which
// Convert.cpp produces from the 2.5 old curve. Granny has eighteen formats and
// most of them are quantised; none of them appear here, so there is one
// evaluator and not eighteen.
//
// Three degrees occur, and each is a different function:
//
//   0  constant. One knot, one control, and the answer is that control whatever
//      t is. 638,669 of the corpus's 772,743 curves.
//   1  linear between neighbouring controls, in the span's own parameter.
//   2  a non-uniform quadratic B-spline. 133,685 curves.
//
// The degree-2 basis was measured rather than assumed, by setting one control to
// one and the rest to zero and reading the result back out of granny2.dll: that
// gives basis_i(t) directly and needs no hypothesis about which spline
// formulation RAD picked. It is the standard non-uniform quadratic B-spline. The
// later spans of a typical curve are evenly spaced and look uniform, which is
// exactly the trap: a uniform basis agrees with the measurement everywhere
// except the two ends and the one span in a hundred whose neighbour is a
// different length.
//
// In span i, with u = (t - k[i]) / (k[i+1] - k[i]) and h = k[i+1] - k[i]:
//
//   w-  =  h (1-u)^2 / ( k[i+1] - k[i-1] )      on control i-1
//   w+  =  h u^2     / ( k[i+2] - k[i]   )      on control i+1
//   w0  =  1 - w- - w+                          on control i
//
// The weights sum to one everywhere including outside the knot range, which is
// what makes extrapolation behave. Off the ends the sequence is clamped:
// k[-1] = k[0] and c[-1] = c[0], and k[n] = k[n-1]. Both were read off the
// measured weights rather than guessed, and the first guess for k[n], reflecting
// the last span, was wrong.
//
// Looping replaces the clamp with a wrap. The curve is periodic with period
// CurveDuration and its last control is the same keyframe as its first, so index
// n-1 is the duplicate and the cycle is over n-1 controls:
//
//   k[-1] = k[n-2] - CurveDuration,  c[-1] = c[n-2]     (BackwardsLoop)
//   k[n]  = k[1]   + CurveDuration,  c[n-1] -> c[0]     (ForwardsLoop)
//
// Normalize divides the result by its own length, which is what an orientation
// curve needs after blending three quaternions that are each unit but whose
// average is not. It reaches the interpolating paths only: a constant curve and
// an empty one come back unchanged however non-unit they are, which is the real
// DLL's behaviour and not an oversight of it.
//
// The identity vector is what an empty curve evaluates to, and 322,479 of the
// corpus's curves are empty.

#include <gr2/granny.h>

#include "Curve.h"
#include "Structures.h"
#include "Trace.h"

#include <cmath>
#include <cstring>

namespace NGr2
{

namespace
{

//! Where t falls, and how far into it.
//!
//! Spans are half-open, [k[i], k[i+1]), which is why a t exactly on a knot
//! belongs to the span that starts there and evaluates to weights (w-, w0, 0).
//!
//! Outside the knots the index is clamped and u runs past its span, so the
//! polynomial continues rather than the value being held. That matches the real
//! DLL just above the last knot and just below the first. Further below the
//! first knot the DLL returns an essentially arbitrary span, which this does not
//! reproduce; see the note in the header comment of Pose.cpp about the engine's
//! clocks never going there.
int32_t FindSpan( const float *pKnots, int32_t nKnotCount, float fT, float *pfU )
{
	// One span fewer than there are knots, and at least one.
	const int32_t nLast = nKnotCount - 2;

	int32_t nLow = 0;
	int32_t nHigh = nLast;
	while ( nLow < nHigh )
	{
		const int32_t nMid = nLow + ( nHigh - nLow + 1 ) / 2;
		if ( pKnots[nMid] <= fT )
		{
			nLow = nMid;
		}
		else
		{
			nHigh = nMid - 1;
		}
	}

	const float fSpan = pKnots[nLow + 1] - pKnots[nLow];
	// A zero-length span would divide by zero. Nothing in the corpus has one,
	// and if one appears the span's start is as good an answer as any.
	*pfU = fSpan > 0.0f ? ( fT - pKnots[nLow] ) / fSpan : 0.0f;
	return nLow;
}

//! One control, as a pointer into the controls array.
const float *Control( const SCurveDataDaK32fC32f *pCurve, int32_t nIndex, int32_t nDimension )
{
	return pCurve->pControls + static_cast<ptrdiff_t>( nIndex ) * nDimension;
}

void AddScaled( float *pResult, const float *pControl, float fWeight, int32_t nDimension )
{
	for ( int32_t i = 0; i < nDimension; ++i )
	{
		pResult[i] += pControl[i] * fWeight;
	}
}

//! Which three controls a degree-2 span reaches, and with what weights.
//!
//! Split out of the evaluation so that the quaternion path in Curve.h can use
//! the same spans and weights while correcting the controls' signs first. A
//! control index of -1 means the clamped case, where the weight belongs to the
//! middle control because the control before the first is the first.
struct SSpan
{
	int32_t nPrev;
	int32_t nMiddle;
	int32_t nNext;
	float fBelow;
	float fMiddle;
	float fAbove;
};

SSpan SpanAt( const SCurveDataDaK32fC32f *pCurve, float fT, bool bForwardsLoop,
              bool bBackwardsLoop, float fCurveDuration )
{
	const int32_t nKnots = pCurve->nKnotCount;
	const float *pKnots = pCurve->pKnots;

	float fU = 0.0f;
	const int32_t i = FindSpan( pKnots, nKnots, fT, &fU );

	// The neighbours of this span, which decide the two outer weights. Off
	// either end the sequence is either clamped or wrapped, and which one is
	// what the loop flags select.
	//
	// The wrap identifies control n-1 with control 0, so the cycle has n-1
	// entries and the knot one place before the first is k[n-2] - duration.
	float fPrevKnot;
	int32_t nPrevControl;
	if ( i > 0 )
	{
		fPrevKnot = pKnots[i - 1];
		nPrevControl = i - 1;
	}
	else if ( bBackwardsLoop && nKnots >= 2 )
	{
		fPrevKnot = pKnots[nKnots - 2] - fCurveDuration;
		nPrevControl = nKnots - 2;
	}
	else
	{
		// Clamped: the knot and the control both repeat, so the outer weight
		// lands on control i as well and the two add up.
		fPrevKnot = pKnots[0];
		nPrevControl = -1;
	}

	float fNextKnot;
	int32_t nNextControl;
	if ( i + 2 < nKnots )
	{
		fNextKnot = pKnots[i + 2];
		nNextControl = i + 1;
	}
	else if ( bForwardsLoop && nKnots >= 2 )
	{
		fNextKnot = pKnots[1] + fCurveDuration;
		nNextControl = i + 1 < nKnots - 1 ? i + 1 : 0;
	}
	else
	{
		// Clamped at the top: the knot repeats but control i+1 still exists,
		// since a span never runs past the last control.
		fNextKnot = pKnots[nKnots - 1];
		nNextControl = i + 1 < nKnots ? i + 1 : nKnots - 1;
	}

	const float fSpan = pKnots[i + 1] - pKnots[i];
	SSpan span;
	span.nPrev = nPrevControl;
	span.nMiddle = i;
	span.nNext = nNextControl;
	span.fBelow = fPrevKnot < pKnots[i + 1]
	                  ? fSpan * ( 1.0f - fU ) * ( 1.0f - fU ) / ( pKnots[i + 1] - fPrevKnot )
	                  : 0.0f;
	span.fAbove =
		fNextKnot > pKnots[i] ? fSpan * fU * fU / ( fNextKnot - pKnots[i] ) : 0.0f;
	span.fMiddle = 1.0f - span.fBelow - span.fAbove;
	return span;
}

void EvaluateDegree2( const SCurveDataDaK32fC32f *pCurve, int32_t nDimension, float fT,
                      bool bForwardsLoop, bool bBackwardsLoop, float fCurveDuration,
                      float *pResult )
{
	const SSpan span = SpanAt( pCurve, fT, bForwardsLoop, bBackwardsLoop, fCurveDuration );

	memset( pResult, 0, sizeof( float ) * static_cast<size_t>( nDimension ) );
	// Control i, plus the clamped neighbour where there is no control before the
	// first: the weight is real, it just lands on the same control.
	AddScaled( pResult, Control( pCurve, span.nMiddle, nDimension ),
	           span.nPrev < 0 ? span.fMiddle + span.fBelow : span.fMiddle, nDimension );
	if ( span.nPrev >= 0 )
	{
		AddScaled( pResult, Control( pCurve, span.nPrev, nDimension ), span.fBelow,
		           nDimension );
	}
	AddScaled( pResult, Control( pCurve, span.nNext, nDimension ), span.fAbove,
	           nDimension );
}

void EvaluateDegree1( const SCurveDataDaK32fC32f *pCurve, int32_t nDimension, float fT,
                      bool bForwardsLoop, float *pResult )
{
	float fU = 0.0f;
	const int32_t i = FindSpan( pCurve->pKnots, pCurve->nKnotCount, fT, &fU );

	// Looping identifies the last control with the first, the same way it does
	// at degree 2, and that is the whole of it here: a straight line between two
	// controls needs no knot outside its own span, so BackwardsLoop has nothing
	// to reach and was measured to change nothing.
	int32_t nHigh = i + 1;
	if ( bForwardsLoop && nHigh == pCurve->nKnotCount - 1 )
	{
		nHigh = 0;
	}

	const float *pLow = Control( pCurve, i, nDimension );
	const float *pHigh = Control( pCurve, nHigh, nDimension );
	for ( int32_t j = 0; j < nDimension; ++j )
	{
		pResult[j] = pLow[j] + ( pHigh[j] - pLow[j] ) * fU;
	}
}

}

void EvaluateQuaternion( const SCurve2 &curve, float fT, bool bForwardsLoop,
                         bool bBackwardsLoop, float fCurveDuration,
                         const float *pIdentity, float *pResult )
{
	const SCurveDataDaK32fC32f *pCurve =
		static_cast<const SCurveDataDaK32fC32f *>( curve.CurveData.pObject );

	// Everything but the interpolating degree-2 case is the public entry point's
	// job: an empty curve, a constant, and a straight line between two controls
	// have no third control to disagree with.
	if ( pCurve == nullptr || pCurve->nKnotCount <= 1 || pCurve->Header.nDegree != 2
	     || pCurve->pControls == nullptr || pCurve->pKnots == nullptr )
	{
		GrannyEvaluateCurveAtT( 4, true, bBackwardsLoop,
		                        reinterpret_cast<const granny_curve2 *>( &curve ),
		                        bForwardsLoop, fCurveDuration, fT, pResult, pIdentity );
		return;
	}

	const SSpan span = SpanAt( pCurve, fT, bForwardsLoop, bBackwardsLoop, fCurveDuration );

	// The span's middle control is the one the others join. Each of the outer two
	// goes in on its near side, which is what makes the wrap continuous: at the
	// wrap the control from the far end of the curve is the far-side one, and a
	// rotation that has turned most of the way round ends on the opposite side
	// from where it started.
	const float *pMiddle = Control( pCurve, span.nMiddle, 4 );
	float fWeightOfMiddle = span.fMiddle;
	if ( span.nPrev < 0 )
	{
		fWeightOfMiddle += span.fBelow;
	}
	for ( int32_t i = 0; i < 4; ++i )
	{
		pResult[i] = pMiddle[i] * fWeightOfMiddle;
	}

	const int32_t Outer[2] = { span.nPrev, span.nNext };
	const float Weights[2] = { span.fBelow, span.fAbove };
	for ( int32_t k = 0; k < 2; ++k )
	{
		if ( Outer[k] < 0 )
		{
			continue;
		}
		const float *pControl = Control( pCurve, Outer[k], 4 );
		float fDot = 0.0f;
		for ( int32_t i = 0; i < 4; ++i )
		{
			fDot += pMiddle[i] * pControl[i];
		}
		const float fWeight = fDot < 0.0f ? -Weights[k] : Weights[k];
		for ( int32_t i = 0; i < 4; ++i )
		{
			pResult[i] += pControl[i] * fWeight;
		}
	}

	float fLengthSquared = 0.0f;
	for ( int32_t i = 0; i < 4; ++i )
	{
		fLengthSquared += pResult[i] * pResult[i];
	}
	if ( fLengthSquared > 0.0f )
	{
		const float fScale = 1.0f / sqrtf( fLengthSquared );
		for ( int32_t i = 0; i < 4; ++i )
		{
			pResult[i] *= fScale;
		}
	}
}

}

extern "C"
{

GR2_API( void ) GrannyEvaluateCurveAtT( granny_int32x Dimension, bool Normalize,
                                        bool BackwardsLoop, granny_curve2 const *Curve,
                                        bool ForwardsLoop, granny_real32 CurveDuration,
                                        granny_real32 t, granny_real32 *Result,
                                        granny_real32 const *IdentityVector )
{
	using namespace NGr2;

	if ( Result == nullptr || Dimension <= 0 )
	{
		return;
	}

	const int32_t nDimension = static_cast<int32_t>( Dimension );
	const SVariant *pVariant = reinterpret_cast<const SVariant *>( Curve );
	const SCurveDataDaK32fC32f *pCurve =
		pVariant == nullptr
			? nullptr
			: static_cast<const SCurveDataDaK32fC32f *>( pVariant->pObject );

	// A curve with no keys is the identity, and the caller says what identity
	// means for this slot: a zero translation, a unit quaternion, an identity
	// 3x3. 228,061 scale-shear curves in the corpus take this path.
	//
	// The real DLL dereferences IdentityVector without checking it, so a null one
	// with an empty curve is an access violation there. Per the rule in
	// README.md this library does not reproduce that: the result is left alone,
	// which is what a caller that passed no identity can only have meant.
	if ( pCurve == nullptr || pCurve->nKnotCount <= 0 || pCurve->pControls == nullptr
	     || pCurve->pKnots == nullptr )
	{
		if ( IdentityVector != nullptr )
		{
			memcpy( Result, IdentityVector,
			        sizeof( float ) * static_cast<size_t>( nDimension ) );
		}
		return;
	}

	if ( pCurve->nKnotCount == 1 || pCurve->Header.nDegree == 0 )
	{
		// Constant, and the answer is the control itself. Degree 0 always has
		// exactly one knot in all 316,190 of the corpus's constant curves, and a
		// single knot leaves no span to interpolate across at any degree.
		//
		// Normalize does not reach here. The real DLL returns the control
		// unchanged even when it is asked to normalize and the control is not
		// unit length, which says the constant case is a separate path in it
		// rather than a degenerate interpolation, and the corpus caught the
		// difference on 2,884 files before this was written the other way.
		memcpy( Result, pCurve->pControls,
		        sizeof( float ) * static_cast<size_t>( nDimension ) );
		return;
	}

	if ( pCurve->Header.nDegree == 1 )
	{
		EvaluateDegree1( pCurve, nDimension, t, ForwardsLoop, Result );
	}
	else
	{
		EvaluateDegree2( pCurve, nDimension, t, ForwardsLoop, BackwardsLoop,
		                 CurveDuration, Result );
	}

	// A blend of three unit quaternions is not a unit quaternion, which is what
	// this is for.
	if ( Normalize )
	{
		float fLengthSquared = 0.0f;
		for ( int32_t i = 0; i < nDimension; ++i )
		{
			fLengthSquared += Result[i] * Result[i];
		}
		if ( fLengthSquared > 0.0f )
		{
			const float fScale = 1.0f / sqrtf( fLengthSquared );
			for ( int32_t i = 0; i < nDimension; ++i )
			{
				Result[i] *= fScale;
			}
		}
	}
}

}
