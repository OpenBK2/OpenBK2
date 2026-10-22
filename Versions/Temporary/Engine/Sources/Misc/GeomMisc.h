#pragma once

#include "port/wordpack.h"

#include <cstdint>

#pragma pack( push, 4 )

// Eight floating-point bit-twiddling helpers stood here, every one of them
// unused across the engine and the editor and every one an example of a defect
// class: seven macros reading a float through *(int *)&, which is the strict
// aliasing violation that silently miscompiled the DXT decoder, and a
// GetFloatBits whose comment claimed to be "C/C++ standard conforming" while
// reading an inactive union member, one byte at a time, to reassemble the value
// little-endian. Removed rather than fixed, since nothing calls them and
// leaving them invited the pattern to be copied.
//
// Misc/Tools.h's bit_cast, over boost::core::bit_cast and so
// __builtin_bit_cast, is the defined way to say any of this and costs nothing.
// FP_BITS lived there and was the separate job this note deferred; its nine
// users are converted and the macro is gone, so no hand-rolled float pun is
// left in the engine.

inline uint32_t PackDWORD( const uint16_t high, const uint16_t low )
{
	return ( uint32_t( high ) << 16 ) | uint32_t( low );
}

inline uint32_t PackDWORD( const uint8_t b3, const uint8_t b2, const uint8_t b1, const uint8_t b0 )
{
	return ( uint32_t( b3 ) << 24 ) | ( uint32_t( b2 ) << 16 ) | ( uint32_t( b1 ) << 8 ) | uint32_t( b0 );
}

inline uint16_t UnpackHighWORD( const uint32_t value )
{
	return ( value >> 16 ) & 0x0000ffff;
}

inline uint16_t UnpackLowWORD( const uint32_t value )
{
	return value & 0x0000ffff;
}

inline uint8_t UnpackBYTE0( const uint32_t value )
{
	return value & 0xff;
}

inline uint8_t UnpackBYTE1( const uint32_t value )
{
	return ( value >> 8 ) & 0xff;
}

inline uint8_t UnpackBYTE2( const uint32_t value )
{
	return ( value >> 16 ) & 0xff;
}

inline uint8_t UnpackBYTE3( const uint32_t value )
{
	return ( value >> 24 ) & 0xff;
}

// geometry
inline float fabsxy2( const CVec3 &a )
{
	return fabs2( a.x, a.y );
}

inline float fabsxy( const CVec3 &a )
{
	return fabs( a.x, a.y );
}

inline float fabsxyz2( const CVec4 &a )
{
	return fabs2( a.x, a.y, a.z );
}

inline float fabsxyz( const CVec4 &a )
{
	return fabs( a.x, a.y, a.z );
}

inline float fabsxy2( const CVec4 &a )
{
	return fabs2( a.x, a.y );
}

inline float fabsxy( const CVec4 &a )
{
	return fabs( a.x, a.y );
}

#pragma pack( pop )

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
	int d = 3 - 2 * y;
	//
	do
	{
		if ( d < 0 )
		{
			d += 4 * x + 6;
		}
		else
		{
			d += 4 * ( x - y ) + 10;
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
	} while ( x <= y );
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
	int d = 3 - 2 * y;
	//
	do
	{
		if ( d < 0 )
		{
			d += 4 * x + 6;
		}
		else
		{
			d += 4 * ( x - y ) + 10;
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
	} while ( x <= y );

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
	int d = 3 - 2 * y;
	//
	do
	{
		if ( d < 0 )
		{
			d += 4 * x + 6;
		}
		else
		{
			d += 4 * ( x - y ) + 10;
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
	} while ( x <= y );
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
	pRes->_11 = vX.x;
	pRes->_12 = vX.y;
	pRes->_13 = vX.z;
	pRes->_14 = -( vX * vO );
	pRes->_21 = vY.x;
	pRes->_22 = vY.y;
	pRes->_23 = vY.z;
	pRes->_24 = -( vY * vO );
	pRes->_31 = vZ.x;
	pRes->_32 = vZ.y;
	pRes->_33 = vZ.z;
	pRes->_34 = -( vZ * vO );
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
	rot.RotateVector( &vX, CVec3( 1, 0, 0 ) );
	rot.RotateVector( &vY, CVec3( 0, 1, 0 ) );
	rot.RotateVector( &vZ, CVec3( 0, 0, 1 ) );
	// create matrix
	CreateViewMatrix( pRes, vX, -vY, -vZ, pos );
}

inline void CreateViewMatrixLH( SHMatrix *pRes, const CVec3 &pos, const SHMatrix &rot )
{
	// create view axises
	CVec3 vX, vY, vZ;
	rot.RotateVector( &vX, CVec3( 1, 0, 0 ) );
	rot.RotateVector( &vY, CVec3( 0, 1, 0 ) );
	rot.RotateVector( &vZ, CVec3( 0, 0, 1 ) );
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
	pRes->_14 = -( 1.0f + 1.0f / fWidth );
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
	const float c = static_cast<float>( cos( fov * 0.5 ) );
	const float s = static_cast<float>( sin( fov * 0.5 ) );
	const float Q = s * fFar / ( fFar - fNear );
	//
	Zero( *pRes );
	pRes->_11 = c / s / fAspect;
	pRes->_22 = c / s;
	pRes->_33 = Q / s;
	pRes->_34 = -Q * fNear / s;
	pRes->_43 = s / s;
	// NOTE: divide by 's' to be compatible with 'w-buffer' technique and 'w-fog'
}

inline void CreatePerspectiveProjectionMatrixRH( SHMatrix *pRes, float fov, float fAspect, float fNear, float fFar )
{
	const float c = static_cast<float>( cos( fov * 0.5 ) );
	const float s = static_cast<float>( sin( fov * 0.5 ) );
	const float Q = s * fFar / ( fFar - fNear );
	//
	Zero( *pRes );
	pRes->_11 = c / s / fAspect;
	pRes->_22 = c / s;
	pRes->_33 = -Q / s;
	pRes->_34 = -Q * fNear / s;
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
