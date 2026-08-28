// GrannyEvaluateCurveAtT: the three degrees, the two ends, and the flags.
//
// The fixtures here are authored curves with knots and controls chosen so that
// the answer is a number somebody can check by hand, which the corpus cannot
// give: a shipped curve's control points are motion capture and its expected
// value is whatever the other implementation says.
//
// The knot spacings are deliberately unequal. A degree-2 curve with evenly
// spaced knots evaluates identically under the uniform B-spline basis and the
// non-uniform one, so a fixture built on even spacing confirms a wrong
// implementation. Every degree-2 case below has at least one span a different
// length from its neighbour.
//
// Agreement with granny2.dll over the corpus lives in scripts/port/gr2diff.py,
// which samples every curve at nine values of t drawn from its own knots. The
// commit that added this file records what that run reported.

#include "Structures.h"

#include <gr2/granny.h>

#include <cmath>
#include <vector>

#include <gtest/gtest.h>

namespace
{

//! A curve2 and its data, held together so a test can hand over one pointer.
//!
//! Built by hand rather than through the file reader: this entry point takes a
//! granny_curve2 and knows nothing about where it came from, and a fixture that
//! had to author a GR2 to test a spline would be testing the wrong thing.
class CCurve
{
public:
	CCurve( uint8_t nDegree, std::vector<float> knots, std::vector<float> controls )
		: m_Knots( std::move( knots ) )
		, m_Controls( std::move( controls ) )
	{
		m_Data.Header.nFormat = NGr2::CURVE_DA_K32F_C32F;
		m_Data.Header.nDegree = nDegree;
		m_Data.nPadding = 0;
		m_Data.nKnotCount = static_cast<int32_t>( m_Knots.size() );
		m_Data.pKnots = m_Knots.empty() ? nullptr : m_Knots.data();
		m_Data.nControlCount = static_cast<int32_t>( m_Controls.size() );
		m_Data.pControls = m_Controls.empty() ? nullptr : m_Controls.data();
		m_Curve.CurveData.pType = nullptr;
		m_Curve.CurveData.pObject = &m_Data;
	}

	CCurve( const CCurve & ) = delete;
	CCurve &operator=( const CCurve & ) = delete;

	const granny_curve2 *Get() const
	{
		return reinterpret_cast<const granny_curve2 *>( &m_Curve );
	}

private:
	std::vector<float> m_Knots;
	std::vector<float> m_Controls;
	NGr2::SCurveDataDaK32fC32f m_Data{};
	NGr2::SCurve2 m_Curve{};
};

//! Evaluate, and hand back the result as a vector so it can be compared.
std::vector<float> At( const CCurve &curve, int32_t nDimension, float fT,
                       bool bNormalize = false, bool bForwards = false,
                       bool bBackwards = false, float fDuration = 0.0f )
{
	std::vector<float> result( static_cast<size_t>( nDimension ), -12345.0f );
	GrannyEvaluateCurveAtT( nDimension, bNormalize, bBackwards, curve.Get(), bForwards,
	                        fDuration, fT, result.data(), nullptr );
	return result;
}

//! The weights the non-uniform quadratic basis puts on controls i-1, i and i+1.
//!
//! Written out independently of the implementation, from the formula in
//! Curve.cpp's header, so that a test says what the answer should be rather than
//! what the code produces.
void Weights( const std::vector<float> &knots, int32_t i, float fU, float fPrev,
              float fNext, float *pOut )
{
	const float h = knots[static_cast<size_t>( i ) + 1] - knots[static_cast<size_t>( i )];
	pOut[0] = h * ( 1.0f - fU ) * ( 1.0f - fU )
	          / ( knots[static_cast<size_t>( i ) + 1] - fPrev );
	pOut[2] = h * fU * fU / ( fNext - knots[static_cast<size_t>( i )] );
	pOut[1] = 1.0f - pOut[0] - pOut[2];
}

}

TEST( Curve, ConstantIsTheControlWhateverTIs )
{
	// Degree 0, and every one of the corpus's 316,190 constant curves has
	// exactly one knot.
	const CCurve curve( 0, { 0.0f }, { 1.5f, -2.5f, 7.0f } );
	for ( float t : { -100.0f, 0.0f, 0.5f, 1000.0f } )
	{
		const std::vector<float> got = At( curve, 3, t );
		EXPECT_FLOAT_EQ( 1.5f, got[0] ) << "at t=" << t;
		EXPECT_FLOAT_EQ( -2.5f, got[1] ) << "at t=" << t;
		EXPECT_FLOAT_EQ( 7.0f, got[2] ) << "at t=" << t;
	}
}

TEST( Curve, ConstantIgnoresNormalizeAndTheLoopFlags )
{
	// Measured: granny2.dll hands back a control of length 5 unchanged when
	// asked to normalize it. The constant case is a separate path in it, and an
	// implementation that normalizes here disagrees with the DLL on 2,884 files.
	const CCurve curve( 0, { 0.0f }, { 3.0f, 4.0f, 0.0f } );
	const std::vector<float> normalized = At( curve, 3, 0.0f, true );
	EXPECT_FLOAT_EQ( 3.0f, normalized[0] );
	EXPECT_FLOAT_EQ( 4.0f, normalized[1] );

	const std::vector<float> looped = At( curve, 3, 0.0f, false, true, true, 1.0f );
	EXPECT_FLOAT_EQ( 3.0f, looped[0] );
	EXPECT_FLOAT_EQ( 4.0f, looped[1] );
}

TEST( Curve, EmptyIsTheIdentityVector )
{
	// 322,479 curves in the corpus have no keys at all, most of them scale-shear.
	const CCurve curve( 0, {}, {} );
	const float identity[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float result[4] = { -1.0f, -1.0f, -1.0f, -1.0f };
	GrannyEvaluateCurveAtT( 4, false, false, curve.Get(), false, 0.0f, 0.25f, result,
	                        identity );
	EXPECT_FLOAT_EQ( 0.0f, result[0] );
	EXPECT_FLOAT_EQ( 1.0f, result[3] );

	// Normalize does not reach this path either: a non-unit identity comes back
	// as it went in.
	const float longIdentity[4] = { 3.0f, 4.0f, 0.0f, 0.0f };
	GrannyEvaluateCurveAtT( 4, true, false, curve.Get(), false, 0.0f, 0.25f, result,
	                        longIdentity );
	EXPECT_FLOAT_EQ( 3.0f, result[0] );
	EXPECT_FLOAT_EQ( 4.0f, result[1] );
}

TEST( Curve, EmptyWithNoIdentityLeavesTheResultAlone )
{
	// The real DLL dereferences the identity vector without checking it, so this
	// is an access violation there. README.md says not to reproduce that.
	const CCurve curve( 0, {}, {} );
	float result[3] = { 8.0f, 9.0f, 10.0f };
	GrannyEvaluateCurveAtT( 3, false, false, curve.Get(), false, 0.0f, 0.0f, result,
	                        nullptr );
	EXPECT_FLOAT_EQ( 8.0f, result[0] );
	EXPECT_FLOAT_EQ( 9.0f, result[1] );
	EXPECT_FLOAT_EQ( 10.0f, result[2] );

	// And a null curve, and a null result, are returns rather than crashes.
	GrannyEvaluateCurveAtT( 3, false, false, nullptr, false, 0.0f, 0.0f, result, nullptr );
	EXPECT_FLOAT_EQ( 8.0f, result[0] );
	GrannyEvaluateCurveAtT( 3, false, false, curve.Get(), false, 0.0f, 0.0f, nullptr,
	                        nullptr );
}

TEST( Curve, LinearInterpolatesBetweenNeighbours )
{
	// Unequal spans, so a span length that leaked into the arithmetic would show.
	const CCurve curve( 1, { 0.0f, 1.0f, 5.0f }, { 0.0f, 10.0f, 30.0f } );
	EXPECT_FLOAT_EQ( 0.0f, At( curve, 1, 0.0f )[0] );
	EXPECT_FLOAT_EQ( 2.5f, At( curve, 1, 0.25f )[0] );
	EXPECT_FLOAT_EQ( 10.0f, At( curve, 1, 1.0f )[0] );
	EXPECT_FLOAT_EQ( 15.0f, At( curve, 1, 2.0f )[0] );
	EXPECT_FLOAT_EQ( 30.0f, At( curve, 1, 5.0f )[0] );

	// Past the last knot the last span's line continues rather than being held,
	// which is what the DLL does and what keeps the weights summing to one.
	EXPECT_FLOAT_EQ( 35.0f, At( curve, 1, 6.0f )[0] );
}

TEST( Curve, LinearLoopsForwardsByReplacingTheLastControl )
{
	// The last key of a looping clip is the same keyframe as the first, so
	// forwards looping runs the last span into control 0 instead of control n-1.
	// Backwards looping was measured to change nothing at this degree: a
	// straight line needs no control outside its own span.
	const CCurve curve( 1, { 0.0f, 1.0f, 2.0f }, { 5.0f, 10.0f, 99.0f } );
	EXPECT_FLOAT_EQ( 99.0f, At( curve, 1, 2.0f )[0] );
	EXPECT_FLOAT_EQ( 5.0f, At( curve, 1, 2.0f, false, true, false, 2.0f )[0] );
	EXPECT_FLOAT_EQ( 7.5f, At( curve, 1, 1.5f, false, true, false, 2.0f )[0] );
	EXPECT_FLOAT_EQ( 54.5f, At( curve, 1, 1.5f )[0] );

	EXPECT_FLOAT_EQ( 5.0f, At( curve, 1, 0.0f, false, false, true, 2.0f )[0] )
		<< "backwards looping does nothing at degree 1";
}

//! Four controls, each the unit vector along its own axis, in four dimensions.
//!
//! The result then reads back the weights directly, one per component, which is
//! the same one-hot trick the basis was measured out of granny2.dll with. It has
//! to be four dimensions and not three: with three, control 3 has nowhere to put
//! a one and its weight is invisible, which is how the first version of the
//! clamping test below passed while asserting the wrong thing.
std::vector<float> OneHotControls()
{
	return { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
	         0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
}

TEST( Curve, QuadraticIsTheNonUniformBSplineBasis )
{
	// Spans of 1, 4 and 2, so the uniform basis and the non-uniform one give
	// different answers everywhere but the very first sample.
	const std::vector<float> knots = { 0.0f, 1.0f, 5.0f, 7.0f };
	const CCurve curve( 2, knots, OneHotControls() );

	// Span 1, which has a neighbour on each side, so nothing is clamped.
	const float fU = 0.25f;
	const float t = knots[1] + fU * ( knots[2] - knots[1] );
	float expected[3] = {};
	Weights( knots, 1, fU, knots[0], knots[3], expected );

	const std::vector<float> got = At( curve, 4, t );
	EXPECT_NEAR( expected[0], got[0], 1e-6f );
	EXPECT_NEAR( expected[1], got[1], 1e-6f );
	EXPECT_NEAR( expected[2], got[2], 1e-6f );
	EXPECT_NEAR( 0.0f, got[3], 1e-6f ) << "span 1 does not reach control 3";

	// And the uniform basis, which is what a reasonable first guess produces,
	// really does differ here. Without this the test above would pass against a
	// uniform implementation.
	const float uniform[3] = { ( 1.0f - fU ) * ( 1.0f - fU ) * 0.5f,
	                           ( 1.0f + 2.0f * fU - 2.0f * fU * fU ) * 0.5f,
	                           fU * fU * 0.5f };
	EXPECT_GT( std::fabs( uniform[0] - expected[0] ), 1e-3f )
		<< "this fixture no longer distinguishes the two bases";
}

TEST( Curve, QuadraticClampsBothEnds )
{
	// There is no control before the first, so its weight lands on control 0 and
	// the curve passes exactly through it. At the far end the knot repeats, which
	// was measured: the first guess, reflecting the last span, was wrong and gave
	// the wrong weight on every last span in the corpus.
	const std::vector<float> knots = { 0.0f, 1.0f, 5.0f, 7.0f };
	const CCurve curve( 2, knots, OneHotControls() );

	const std::vector<float> atStart = At( curve, 4, 0.0f );
	EXPECT_NEAR( 1.0f, atStart[0], 1e-6f ) << "the whole weight falls on control 0";
	EXPECT_NEAR( 0.0f, atStart[1], 1e-6f );

	// The last span, at three quarters. k[i+2] is the last knot repeated, so the
	// weight on control 3 is h u^2 / (k[3] - k[2]) = 2 * 0.5625 / 2 = 0.5625.
	// Reflecting the span instead, which was the first guess, would make the
	// divisor 4 and the weight 0.28125.
	const float fU = 0.75f;
	const float t = knots[2] + fU * ( knots[3] - knots[2] );
	float expected[3] = {};
	Weights( knots, 2, fU, knots[1], knots[3], expected );
	EXPECT_NEAR( 0.5625f, expected[2], 1e-6f ) << "the repeated last knot";

	const std::vector<float> got = At( curve, 4, t );
	EXPECT_NEAR( expected[0], got[1], 1e-6f ) << "weight on control 1";
	EXPECT_NEAR( expected[1], got[2], 1e-6f ) << "weight on control 2";
	EXPECT_NEAR( expected[2], got[3], 1e-6f ) << "weight on control 3";

	// Exactly on the last knot the curve reaches the last control.
	const std::vector<float> atEnd = At( curve, 4, knots[3] );
	EXPECT_NEAR( 0.0f, atEnd[1], 1e-6f );
	EXPECT_NEAR( 0.0f, atEnd[2], 1e-6f );
	EXPECT_NEAR( 1.0f, atEnd[3], 1e-6f );
}

TEST( Curve, QuadraticLoopsAtBothEnds )
{
	// Looping replaces the clamp with a wrap over a period of CurveDuration,
	// where control n-1 is the same keyframe as control 0. So the knot before
	// the first is k[n-2] - duration and the one after the last is k[1] +
	// duration, both measured out of the DLL.
	const std::vector<float> knots = { 0.0f, 1.0f, 5.0f, 7.0f };
	const float fDuration = 7.0f;
	const CCurve curve( 2, knots, OneHotControls() );

	// Backwards, at the very start: the previous knot is k[2] - duration = -2 and
	// the previous control is control 2, which without looping would not be
	// reachable from here at all.
	float expected[3] = {};
	Weights( knots, 0, 0.0f, knots[2] - fDuration, knots[2], expected );
	const std::vector<float> back = At( curve, 4, 0.0f, false, false, true, fDuration );
	EXPECT_NEAR( expected[0], back[2], 1e-6f ) << "control 2 reached backwards";
	EXPECT_NEAR( expected[1], back[0], 1e-6f );
	EXPECT_GT( back[2], 1e-3f ) << "without the wrap this weight would be zero";
	EXPECT_NEAR( 1.0f, At( curve, 4, 0.0f )[0], 1e-6f )
		<< "and without the flag the same t is control 0 alone";

	// Forwards, at the very end: control 3 becomes control 0, and the knot after
	// the last is k[1] + duration = 8.
	Weights( knots, 2, 1.0f, knots[1], knots[1] + fDuration, expected );
	const std::vector<float> fwd = At( curve, 4, knots[3], false, true, false, fDuration );
	EXPECT_NEAR( expected[2], fwd[0], 1e-6f ) << "control 3 became control 0";
	EXPECT_NEAR( expected[1], fwd[2], 1e-6f );
	EXPECT_NEAR( 0.0f, fwd[3], 1e-6f ) << "so control 3 itself is no longer reached";
	EXPECT_GT( fwd[0], 1e-3f ) << "without the wrap control 0 would have no weight";
}

TEST( Curve, NormalizeDividesByTheLength )
{
	// A blend of unit quaternions is not a unit quaternion, which is the only
	// reason this flag exists. It reaches the interpolating paths only.
	const CCurve curve( 1, { 0.0f, 1.0f }, { 3.0f, 4.0f, 0.0f, 0.0f, 3.0f, 4.0f,
	                                         0.0f, 0.0f } );
	const std::vector<float> plain = At( curve, 4, 0.5f );
	EXPECT_FLOAT_EQ( 3.0f, plain[0] );
	EXPECT_FLOAT_EQ( 4.0f, plain[1] );

	const std::vector<float> unit = At( curve, 4, 0.5f, true );
	EXPECT_FLOAT_EQ( 0.6f, unit[0] );
	EXPECT_FLOAT_EQ( 0.8f, unit[1] );
	EXPECT_FLOAT_EQ( 0.0f, unit[2] );
}

TEST( Curve, ADuplicatedKnotDoesNotDivideByZero )
{
	// 38 curves in the corpus have two knots at the same time, which is a span of
	// zero length. Nothing here may produce an infinity or a NaN out of it.
	const CCurve curve( 2, { 0.0f, 1.0f, 1.0f, 3.0f },
	                    { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
	                      0.0f, 0.0f, 0.0f } );
	for ( float t : { 0.0f, 0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f } )
	{
		const std::vector<float> got = At( curve, 3, t );
		for ( size_t i = 0; i < got.size(); ++i )
		{
			EXPECT_TRUE( std::isfinite( got[i] ) )
				<< "component " << i << " at t=" << t << " is " << got[i];
		}
	}

	const CCurve linear( 1, { 0.0f, 2.0f, 2.0f }, { 0.0f, 5.0f, 9.0f } );
	for ( float t : { 0.0f, 1.0f, 2.0f, 3.0f } )
	{
		EXPECT_TRUE( std::isfinite( At( linear, 1, t )[0] ) ) << "at t=" << t;
	}
}

TEST( Curve, TheDimensionIsTheCallersAndSoIsTheStride )
{
	// A curve carries a knot count and a control count and no dimension, so the
	// caller's Dimension is what strides the control array. The engine reads a
	// scalar channel out of a track by asking for one component, and it gets the
	// first float of each control, not the first component of each key of a
	// wider curve.
	//
	// Measured: a three-wide curve with controls forced to (3, 4, 0) per key
	// evaluates to 3.125 at Dimension 1 and to a vector of length 4.711505 at
	// Dimension 2, both of which only follow if the stride is the caller's.
	const CCurve curve( 1, { 0.0f, 1.0f }, { 1.0f, 2.0f, 3.0f, 5.0f, 6.0f, 7.0f } );

	const std::vector<float> one = At( curve, 1, 0.5f );
	ASSERT_EQ( 1u, one.size() );
	EXPECT_FLOAT_EQ( 1.5f, one[0] ) << "halfway between controls 1.0 and 2.0";

	const std::vector<float> three = At( curve, 3, 0.5f );
	EXPECT_FLOAT_EQ( 3.0f, three[0] );
	EXPECT_FLOAT_EQ( 4.0f, three[1] );
	EXPECT_FLOAT_EQ( 5.0f, three[2] );
}
