// A bone's placement: position, orientation and scale-shear, and composing two.
//
// granny_transform keeps its three parts separate rather than as one matrix, and
// the Flags say which of them are not the identity. That is not merely a space
// saving: a pose is interpolated per part, and a quaternion cannot be recovered
// from a matrix that has been through a lerp.
//
// The composition below was measured out of granny2.dll rather than derived,
// because every ordering in it had a plausible alternative and picking wrong
// gives a model that renders and is subtly wrong. Each probe used inputs chosen
// so the candidates disagree:
//
//   Orientation  Hamilton product A then B. Rotating 90 about Z then 90 about X
//                gives (0.5, 0.5, 0.5, 0.5), which is A*B; B*A would have given
//                (0.5, -0.5, 0.5, 0.5).
//   Position     A's, plus B's carried into A's frame: scaled first, then
//                rotated. With A a 90 degree turn about Z and a diag(2,3,1)
//                scale, and B a unit step along X, the answer is (0, 2, 0);
//                rotating before scaling would have given (0, 3, 0). The scale
//                acts on the column, (SS * v)[i] = sum SS[i][j] * v[j]: with A a
//                shear whose first row is (1,1,0) the answer is (1, 0, 0), and
//                the row convention would have given (1, 1, 0).
//   ScaleShear   A's scale conjugated by B's rotation, then B's scale:
//                Rb^T * Sa * Rb * Sb. Not the plain product, which is what the
//                first version of this did and what the corpus sweep caught
//                within a minute of being taught to call it. With A a diag(2,3,1)
//                and no rotation, and B a 90 degree turn about Z, the answer is
//                diag(3,2,1) where the plain product gives diag(2,3,1); a 30
//                degree turn then separates the two conjugation directions, and
//                Rb^T Sa Rb is the one.
//
//                It is forced rather than arbitrary. Composing (Sa, Ra) then
//                (Sb, Rb) as affine maps gives a linear part Ra Sa Rb Sb, and
//                storing the rotation as Ra Rb leaves S = Rb^T Sa Rb Sb. The
//                same derivation produces the position rule above, which is how
//                the two came to be checked against each other.
//   Flags        A plain OR of the two.

#include <gr2/granny.h>

#include "Structures.h"
#include "Trace.h"

#include <cstring>

using namespace NGr2;

namespace
{

//! Rotate a vector by a quaternion stored (x, y, z, w).
void Rotate( const float *pQuaternion, const float *pVector, float *pResult )
{
	const float x = pQuaternion[0];
	const float y = pQuaternion[1];
	const float z = pQuaternion[2];
	const float w = pQuaternion[3];

	// v + 2 * cross( q.xyz, cross( q.xyz, v ) + w * v ), which is the form that
	// needs no normalisation and no matrix.
	const float tx = 2.0f * ( y * pVector[2] - z * pVector[1] );
	const float ty = 2.0f * ( z * pVector[0] - x * pVector[2] );
	const float tz = 2.0f * ( x * pVector[1] - y * pVector[0] );

	pResult[0] = pVector[0] + w * tx + ( y * tz - z * ty );
	pResult[1] = pVector[1] + w * ty + ( z * tx - x * tz );
	pResult[2] = pVector[2] + w * tz + ( x * ty - y * tx );
}

//! A quaternion as a rotation matrix, in the convention the rest of this uses:
//! stored by rows, applied to a column, so ( M * v )[i] = sum M[i][j] * v[j].
void ToMatrix( float x, float y, float z, float w, float pResult[3][3] )
{
	pResult[0][0] = 1.0f - 2.0f * ( y * y + z * z );
	pResult[0][1] = 2.0f * ( x * y - w * z );
	pResult[0][2] = 2.0f * ( x * z + w * y );
	pResult[1][0] = 2.0f * ( x * y + w * z );
	pResult[1][1] = 1.0f - 2.0f * ( x * x + z * z );
	pResult[1][2] = 2.0f * ( y * z - w * x );
	pResult[2][0] = 2.0f * ( x * z - w * y );
	pResult[2][1] = 2.0f * ( y * z + w * x );
	pResult[2][2] = 1.0f - 2.0f * ( x * x + y * y );
}

void Transpose( const float pSource[3][3], float pResult[3][3] )
{
	for ( int r = 0; r < 3; ++r )
	{
		for ( int c = 0; c < 3; ++c )
		{
			pResult[r][c] = pSource[c][r];
		}
	}
}

//! pResult = pLeft * pRight, safe when pResult aliases either input.
void Multiply( const float pLeft[3][3], const float pRight[3][3], float pResult[3][3] )
{
	float product[3][3];
	for ( int r = 0; r < 3; ++r )
	{
		for ( int c = 0; c < 3; ++c )
		{
			product[r][c] = pLeft[r][0] * pRight[0][c] + pLeft[r][1] * pRight[1][c]
			                + pLeft[r][2] * pRight[2][c];
		}
	}
	memcpy( pResult, product, sizeof( product ) );
}

}

extern "C"
{

GR2_API( void ) GrannyMakeIdentity( granny_transform *Result )
{
	GR2_TRACE( "Result={}", Result );

	if ( Result == 0 )
	{
		return;
	}

	// Flags 0, a zero position, a unit quaternion and an identity scale-shear,
	// which is what the real DLL writes.
	STransform *pResult = reinterpret_cast<STransform *>( Result );
	memset( pResult, 0, sizeof( *pResult ) );
	pResult->Orientation[3] = 1.0f;
	pResult->ScaleShear[0][0] = 1.0f;
	pResult->ScaleShear[1][1] = 1.0f;
	pResult->ScaleShear[2][2] = 1.0f;
}

GR2_API( void ) GrannyPostMultiplyBy( granny_transform *Transform,
                                      granny_transform const *PostMult )
{
	GR2_TRACE( "Transform={} PostMult={}", Transform, PostMult );

	if ( Transform == 0 || PostMult == 0 )
	{
		return;
	}

	STransform *pA = reinterpret_cast<STransform *>( Transform );
	const STransform *pB = reinterpret_cast<const STransform *>( PostMult );

	// Position first, while A's orientation and scale are still A's own.
	float scaled[3];
	for ( int i = 0; i < 3; ++i )
	{
		scaled[i] = pA->ScaleShear[i][0] * pB->Position[0]
		            + pA->ScaleShear[i][1] * pB->Position[1]
		            + pA->ScaleShear[i][2] * pB->Position[2];
	}
	float rotated[3];
	Rotate( pA->Orientation, scaled, rotated );
	pA->Position[0] += rotated[0];
	pA->Position[1] += rotated[1];
	pA->Position[2] += rotated[2];

	const float ax = pA->Orientation[0];
	const float ay = pA->Orientation[1];
	const float az = pA->Orientation[2];
	const float aw = pA->Orientation[3];
	const float bx = pB->Orientation[0];
	const float by = pB->Orientation[1];
	const float bz = pB->Orientation[2];
	const float bw = pB->Orientation[3];

	pA->Orientation[0] = aw * bx + ax * bw + ay * bz - az * by;
	pA->Orientation[1] = aw * by - ax * bz + ay * bw + az * bx;
	pA->Orientation[2] = aw * bz + ax * by - ay * bx + az * bw;
	pA->Orientation[3] = aw * bw - ax * bx - ay * by - az * bz;

	// Rb^T * Sa * Rb * Sb, using B's orientation, which the lines above did not
	// touch: they overwrote A's.
	float rotation[3][3];
	ToMatrix( bx, by, bz, bw, rotation );
	float transposed[3][3];
	Transpose( rotation, transposed );

	float conjugated[3][3];
	Multiply( pA->ScaleShear, rotation, conjugated );
	Multiply( transposed, conjugated, conjugated );
	Multiply( conjugated, pB->ScaleShear, pA->ScaleShear );

	pA->nFlags |= pB->nFlags;
}

}
