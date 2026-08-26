#pragma once

#include "port/wordpack.h"

#include <cstdint>

#pragma pack( push, 4 )
// clear MSb
#define FP_ABS_BITS( fp ) ( FP_BITS( fp ) & 0x7FFFFFFF )
#define FP_ABS_BITS_CONST( fp ) ( FP_BITS_CONST( fp ) & 0x7FFFFFFF )
// get MSb
#define FP_SIGN_BIT( fp ) ( FP_BITS( fp ) & 0x80000000 )
#define FP_SIGN_BIT_CONST( fp ) ( FP_BITS_CONST( fp ) & 0x80000000 )

#define FP_NORM_TO_BYTE(i,p)            \
{                                       \
	float _n = (p) + 1.0f;                \
	i = *(int *)&_n;                      \
	if (i >= 0x40000000)     i = 0xFF;    \
	else if (i <=0x3F800000) i = 0;       \
	else i = ((i) >> 15) & 0xFF;          \
}
// e = exp( p )
#define FP_EXP(e,p)                     \
{                                       \
	int _i;                               \
	e = -1.44269504f * (float)0x00800000 * (p); \
	_i = (int)e + 0x3F800000;             \
	e = *(float *)&_i;                    \
}
// r = 1/p
#define FP_INV(r,p)                     \
{                                       \
	int _i = 2 * FP_ONE_BITS - *(int *)&(p); \
	r = *(float *)&_i;                    \
	r = r * (2.0f - (p) * r);             \
}
//#define MAKELONG(x,y) (int32_t((uint16_t(x)&0xffff)))|(int32_t((uint16_t(y)&0xffff)<<16))
// C/C++ standard conforming fp-bits retrieving
inline uint32_t GetFloatBits( float fVal )
{
	union 
	{
		float f;
		unsigned char c[4];
	} u;
	u.f = fVal;
	return ( uint32_t(u.c[3]) << 24L ) | ( uint32_t(u.c[2]) << 16L ) | ( uint32_t(u.c[1]) << 8L ) | uint32_t( u.c[0] );
}

inline uint32_t PackDWORD( const uint16_t high, const uint16_t low )
{ 
	return ( uint32_t(high) << 16 ) | uint32_t(low);
}
inline uint32_t PackDWORD( const uint8_t b3, const uint8_t b2, const uint8_t b1, const uint8_t b0 )
{ 
	return ( uint32_t(b3) << 24 ) | ( uint32_t(b2) << 16 ) | ( uint32_t(b1) << 8 ) | uint32_t(b0);
}
inline uint16_t UnpackHighWORD( const uint32_t value ) { return (value >> 16) & 0x0000ffff; }
inline uint16_t UnpackLowWORD( const uint32_t value ) { return value & 0x0000ffff; }
inline uint8_t UnpackBYTE0( const uint32_t value ) { return value & 0xff; }
inline uint8_t UnpackBYTE1( const uint32_t value ) { return (value >> 8) & 0xff; }
inline uint8_t UnpackBYTE2( const uint32_t value ) { return (value >> 16) & 0xff; }
inline uint8_t UnpackBYTE3( const uint32_t value ) { return (value >> 24) & 0xff; }

// geometry
inline float fabsxy2( const CVec3 &a ) { return fabs2( a.x, a.y ); }
inline float fabsxy( const CVec3 &a ) { return fabs( a.x, a.y ); }
inline float fabsxyz2( const CVec4 &a ) { return fabs2( a.x, a.y, a.z ); }
inline float fabsxyz( const CVec4 &a ) { return fabs( a.x, a.y, a.z ); }
inline float fabsxy2( const CVec4 &a ) { return fabs2( a.x, a.y ); }
inline float fabsxy( const CVec4 &a ) { return fabs( a.x, a.y ); }

#pragma pack(pop)

// ************************************************************************************************************************ //
// **
// ** Bresenham circle function
// **
// **
// **
// ************************************************************************************************************************ //

template <class TFunctional>
void BresenhamCircle( int nCenterX, int nCenterY, int nRadius, TFunctional &func )
{
	int x = 0, y = nRadius;
	int d = 3 - 2*y;
	//
	do
	{
		if ( d < 0 )
			d += 4*x + 6;
		else
		{
			d += 4*(x - y) + 10;
			--y;
		}
		++x;
		//
		func( nCenterX - x, nCenterY + y );
		func( nCenterX + x, nCenterY + y );
		func( nCenterX - x, nCenterY - y );
		func( nCenterX + x, nCenterY - y );
		//
		func( nCenterX - y, nCenterY + x );
		func( nCenterX + y, nCenterY + x );
		func( nCenterX - y, nCenterY - x );
		func( nCenterX + y, nCenterY - x );
		//
	}	while ( x <= y );
	// last 4 points
	func( nCenterX - nRadius, nCenterY );
	func( nCenterX + nRadius, nCenterY );
	func( nCenterX, nCenterY - nRadius );
	func( nCenterX, nCenterY + nRadius );
}


template <class TFunctional>
void BresenhamFilledCircle( int nCenterX, int nCenterY, int nRadius, TFunctional &func )
{
	int x = 0, y = nRadius;
	int d = 3 - 2*y;
	//
	do 
	{
		if ( d < 0 )
			d += 4*x + 6;
		else
		{
			d += 4*( x - y ) + 10;
			--y;
		}
		++x;
		//
		for ( int index = ( nCenterX - x ); index <= ( nCenterX + x ); ++index )
		{
			func( index, nCenterY + y );
		}
		for ( int index = ( nCenterX - x ); index <= ( nCenterX + x ); ++index )
		{
			func( index, nCenterY - y );
		}
		for ( int index = ( nCenterX - y ); index <= ( nCenterX + y ); ++index )
		{
			func( index, nCenterY + x );
		}
		for ( int index = ( nCenterX - y ); index <= ( nCenterX + y ); ++index )
		{
			func( index, nCenterY - x );
		}
	}	while ( x <= y );

	// last 4 points
	for ( int index = ( nCenterX - nRadius ); index <= ( nCenterX + nRadius ); ++index )
	{
		func( index, nCenterY );
	}
	func( nCenterX, nCenterY - nRadius );
	func( nCenterX, nCenterY + nRadius );
}


// ************************************************************************************************************************ //
// **
// ** Bresenham ellipse function
// **
// **
// **
// ************************************************************************************************************************ //

template <class TFunctional>
void BresenhamEllipse( int nCenterX, int nCenterY, int nXRadius, float fY2XRatio, TFunctional &func )
{
	int x = 0, y = nXRadius;
	int d = 3 - 2*y;
	//
	do
	{
		if ( d < 0 )
			d += 4*x + 6;
		else
		{
			d += 4*(x - y) + 10;
			--y;
		}
		++x;
		//
		func( nCenterX - x, nCenterY + y * fY2XRatio );
		func( nCenterX + x, nCenterY + y * fY2XRatio );
		func( nCenterX - x, nCenterY - y * fY2XRatio );
		func( nCenterX + x, nCenterY - y * fY2XRatio );
		//
		func( nCenterX - y, nCenterY + x * fY2XRatio );
		func( nCenterX + y, nCenterY + x * fY2XRatio );
		func( nCenterX - y, nCenterY - x * fY2XRatio );
		func( nCenterX + y, nCenterY - x * fY2XRatio );
		//
	}	while ( x <= y );
	// last 4 points
	func( nCenterX - nXRadius, nCenterY );
	func( nCenterX + nXRadius, nCenterY );
	func( nCenterX, nCenterY - nXRadius * fY2XRatio );
	func( nCenterX, nCenterY + nXRadius * fY2XRatio );
}


// ************************************************************************************************************************ //
// **
// ** viewing matrix creation. RH => right-handed coordinate system, LH => left-handed coordinate system
// **
// **
// **
// ************************************************************************************************************************ //

// low-level creation
inline void CreateViewMatrix( SHMatrix *pRes, const CVec3 &vX, const CVec3 &vY, const CVec3 &vZ, const CVec3 &vO )
{
	pRes->_11 = vX.x;  pRes->_12 = vX.y;  pRes->_13 = vX.z;  pRes->_14 = -( vX * vO );
	pRes->_21 = vY.x;  pRes->_22 = vY.y;  pRes->_23 = vY.z;  pRes->_24 = -( vY * vO );
	pRes->_31 = vZ.x;  pRes->_32 = vZ.y;  pRes->_33 = vZ.z;  pRes->_34 = -( vZ * vO );
	pRes->_41 = pRes->_42 = pRes->_43 = 0.0f;
	pRes->_44 = 1.0f;
}
// from position and rotation as quaternion
inline void CreateViewMatrixRH( SHMatrix *pRes, const CVec3 &pos, const CQuat &rot )
{
	// create view axises
	CVec3 vX, vY, vZ;
	rot.GetXAxis( &vX );
	rot.GetYAxis( &vY );
	rot.GetZAxis( &vZ );
	// create matrix
	CreateViewMatrix( pRes, vX, -vY, -vZ, pos );
}
inline void CreateViewMatrixLH( SHMatrix *pRes, const CVec3 &pos, const CQuat &rot )
{
	// create view axises
	CVec3 vX, vY, vZ;
	rot.GetXAxis( &vX );
	rot.GetYAxis( &vY );
	rot.GetZAxis( &vZ );
	// create matrix
	CreateViewMatrix( pRes, vX, vY, vZ, pos );
}
// from position and rotation as matrix
inline void CreateViewMatrixRH( SHMatrix *pRes, const CVec3 &pos, const SHMatrix &rot )
{
	// create view axises
	CVec3 vX, vY, vZ;
	rot.RotateVector( &vX, CVec3(1, 0, 0) );
	rot.RotateVector( &vY, CVec3(0, 1, 0) );
	rot.RotateVector( &vZ, CVec3(0, 0, 1) );
	// create matrix
	CreateViewMatrix( pRes, vX, -vY, -vZ, pos );
}
inline void CreateViewMatrixLH( SHMatrix *pRes, const CVec3 &pos, const SHMatrix &rot )
{
	// create view axises
	CVec3 vX, vY, vZ;
	rot.RotateVector( &vX, CVec3(1, 0, 0) );
	rot.RotateVector( &vY, CVec3(0, 1, 0) );
	rot.RotateVector( &vZ, CVec3(0, 0, 1) );
	// create matrix
	CreateViewMatrix( pRes, vX, vY, vZ, pos );
}
// from 'look-at'
inline void CreateViewMatrixRH( SHMatrix *pRes, const CVec3 &vFrom, const CVec3 &vTo, const CVec3 &vUp )
{
	CVec3 vX, vY = vUp, vZ = vTo - vFrom;
	// norm 'z' vector
	Normalize( &vZ );
	// calc and norm 'x' vector
	vX = vY ^ vZ;
	Normalize( &vX );
	// calc and norm 'y' vector (to ensure, that we have orthonormal basis)
	vY = vZ ^ vX;
	Normalize( &vY );
	//
	CreateViewMatrix( pRes, vX, -vY, -vZ, vFrom );
}
inline void CreateViewMatrixLH( SHMatrix *pRes, const CVec3 &vFrom, const CVec3 &vTo, const CVec3 &vUp )
{
	CVec3 vX, vY = vUp, vZ = vTo - vFrom;
	// norm 'z' vector
	Normalize( &vZ );
	// calc and norm 'x' vector
	vX = vY ^ vZ;
	Normalize( &vX );
	// calc and norm 'y' vector (to ensure, that we have orthonormal basis)
	vY = vZ ^ vX;
	Normalize( &vY );
	//
	CreateViewMatrix( pRes, vX, vY, vZ, vFrom );
}

// ************************************************************************************************************************ //
// **
// ** direct transform (texel-to-pixel) matrix creation
// **
// **
// **
// ************************************************************************************************************************ //

inline void CreateDirectTransformMatrix( SHMatrix *pRes, const float fWidth, const float fHeight )
{
	Zero( *pRes );
	pRes->_11 = 2.0f / fWidth;
	pRes->_14 = -( 1.0f + 1.0f/fWidth );
	pRes->_22 = -2.0f / fHeight;
	pRes->_24 = 1.0 + 1.0f / fHeight;
	pRes->_33 = pRes->_44 = 1.0f;
}

//const SHMatrix MNULL = SHMatrix( 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );
//const SHMatrix MONE  = SHMatrix( 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 );

// ************************************************************************************************************************ //
// **
// ** projection matrix creation. RH => right-handed coordinate system, LH => left-handed coordinate system
// **
// **
// **
// ************************************************************************************************************************ //

// perspective projection matrix
inline void CreatePerspectiveProjectionMatrixLH( SHMatrix *pRes, float fov, float fAspect, float fNear, float fFar )
{
	const float c = static_cast<float>( cos( fov*0.5 ) );
	const float s = static_cast<float>( sin( fov*0.5 ) );
	const float Q = s * fFar / ( fFar - fNear );
	//
	Zero( *pRes );
	pRes->_11 = c / s / fAspect;
	pRes->_22 = c / s;
	pRes->_33 = Q / s;
	pRes->_34 = -Q*fNear / s;
	pRes->_43 = s / s;
	// NOTE: divide by 's' to be compatible with 'w-buffer' technique and 'w-fog'
}
inline void CreatePerspectiveProjectionMatrixRH( SHMatrix *pRes, float fov, float fAspect, float fNear, float fFar )
{
	const float c = static_cast<float>( cos( fov*0.5 ) );
	const float s = static_cast<float>( sin( fov*0.5 ) );
	const float Q = s * fFar / ( fFar - fNear );
	//
	Zero( *pRes );
	pRes->_11 = c / s / fAspect;
	pRes->_22 = c / s;
	pRes->_33 = -Q / s;
	pRes->_34 = -Q*fNear / s;
	pRes->_43 = -s / s;
	// NOTE: divide by 's' to be compatible with 'w-buffer' technique and 'w-fog'
}
// orthographics projection
inline void CreateOrthographicProjectionMatrixLH( SHMatrix *pRes, float fWidth, float fHeight, float fNear, float fFar )
{
	Zero( *pRes );
	pRes->_11 = 2.0f / fWidth;
	pRes->_14 = 0;
	pRes->_22 = 2.0f / fHeight;
	pRes->_24 = 0;
	pRes->_33 = 1.0f / ( fFar - fNear );
	pRes->_34 = -fNear / ( fFar - fNear );
	pRes->_44 = 1.0f;
}
inline void CreateOrthographicProjectionMatrixRH( SHMatrix *pRes, float fWidth, float fHeight, float fNear, float fFar )
{
	Zero( *pRes );
	pRes->_11 = 2.0f / fWidth;
	pRes->_14 = 0;
	pRes->_22 = 2.0f / fHeight;
	pRes->_24 = 0;
	pRes->_33 = -1.0f / ( fFar - fNear );
	pRes->_34 = -fNear / ( fFar - fNear );
	pRes->_44 = 1.0f;
}

