#pragma once

#include "B2_M1_Terrain_export.h"


#include <math.h>
#include "DBVSO.h"

#define DEF_EPS 0.0001f
#define DEF_EPS2 (DEF_EPS * DEF_EPS)
#define DEF_FLOAT_EPS 0.01f
#define DEF_FLOAT_EPS2 (DEF_FLOAT_EPS * DEF_FLOAT_EPS)
const float invCoeffs[] = { 0.0f, 1.0f, 0.5f, 0.3333333f, 0.25f, 0.2f };

class CVec2i
{
public:
	int x, y;
	//
	CVec2i() {}
	CVec2i( const int _x, const int _y ) : x( _x ), y( _y ) {}
	CVec2i( const CVec2i &v ) : x( v.x ), y( v.y ) {}
	//
	void Set( const int _x, const int _y ) { x = _x; y = _y; }
	//
	bool operator == ( const CVec2i &v ) const { return ( x == v.x ) && ( y == v.y ); }
	//
	int operator &( IBinSaver &saver )
	{
		saver.Add( 1, &x );
		saver.Add( 2, &y );
		return 0;
	}
};

inline bool IsBBIntersect( const CVec2i &vMin1, const CVec2i &vMax1, const CVec2i &vMin2, const CVec2i &vMax2 )
{
	return ( vMin1.x <= vMax2.x ) && ( vMin1.y <= vMax2.y ) && ( vMax1.x >= vMin2.x ) && ( vMax1.y >= vMin2.y );
}

inline bool IsBBIntersect( const CVec2 &vMin1, const CVec2 &vMax1, const CVec2 &vMin2, const CVec2 &vMax2 )
{
	return ( vMin1.x <= vMax2.x ) && ( vMin1.y <= vMax2.y ) && ( vMax1.x >= vMin2.x ) && ( vMax1.y >= vMin2.y );
}

inline bool IsInsideBB( const CVec2 &v, const CVec2 &vMin, const CVec2 &vMax )
{
	return ( v.x >= vMin.x ) && ( v.y >= vMin.y ) && ( v.x <= vMax.x ) && ( v.y <= vMax.y );
}

template<class TReal> class CVec3Ex // CRAP {not properly connected yet} CRAP
{
public:
	TReal x, y, z;
	BYTE flag;
	//
	CVec3Ex() {}
	CVec3Ex( const TReal _x, const TReal _y, const TReal _z, const BYTE _flag ) : x( _x) , y( _y ), z( _z ), flag( _flag ) {}
	CVec3Ex( const CVec3 &v, const BYTE _flag ) : x( v.x ), y( v.y ), z( v.z ), flag( _flag ) {}
	CVec3Ex( const CVec3Ex &v ) : x( v.x ), y( v.y ), z( v.z ), flag( v.flag ) {}
	//
	void Set( const TReal _x, const TReal _y, const TReal _z, const BYTE _flag ) { x = _x; y = _y; z = _z; flag = _flag; }
	void Set( const CVec3 &v, const BYTE _flag ) { x = v.x; y = v.y; z = v.z; flag = _flag; }
	//
	bool operator == ( const CVec3Ex &v ) const
	{
		return ( (fabs(x - v.x) < DEF_EPS) && (fabs(y - v.y) < DEF_EPS) && (fabs(z - v.z) < DEF_EPS) && (flag == v.flag) );
	}
	bool operator != ( const CVec3Ex &v ) const
	{
		return ( (fabs(x - v.x) >= DEF_EPS) || (fabs(y - v.y) >= DEF_EPS) || (fabs(z - v.z) >= DEF_EPS) || (flag != v.flag) );
	}
	//
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &x );
		saver.Add( 2, &y );
		saver.Add( 3, &z );
		saver.Add( 4, &flag );
		return 0;
	}
};

class CVec3dEx;
class CVec3fEx
{
public:
	float x, y, z;
	BYTE flag;
	//
	CVec3fEx() {}
	//
	CVec3fEx( const float _x, const float _y, const float _z, const BYTE _flag ) : x( _x ), y( _y ), z( _z ), flag( _flag ) {}
	CVec3fEx( const CVec3 &v, const BYTE _flag = 0 ) : x( v.x ), y( v.y ), z( v.z ), flag( _flag ) {}
	CVec3fEx( const CVec3fEx &v ) : x( v.x ), y( v.y ), z( v.z ), flag( v.flag ) {}
	//
	void Set( const float _x, const float _y, const float _z, const BYTE _flag ) { x = _x; y = _y; z = _z; flag = _flag; }
	void Set( const CVec3 &v, const BYTE _flag ) { x = v.x; y = v.y; z = v.z; flag = _flag; }
	//
	bool operator == ( const CVec3fEx &v ) const
	{
		return ( (fabs(x - v.x) < DEF_EPS) && (fabs(y - v.y) < DEF_EPS) && (fabs(z - v.z) < DEF_EPS) && (flag == v.flag) );
	}
	bool operator != ( const CVec3fEx &v ) const
	{
		return ( (fabs(x - v.x) >= DEF_EPS) || (fabs(y - v.y) >= DEF_EPS) || (fabs(z - v.z) >= DEF_EPS) || (flag != v.flag) );
	}
	//
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &x );
		saver.Add( 2, &y );
		saver.Add( 3, &z );
		saver.Add( 4, &flag );
		return 0;
	}
	//
	CVec3 GetVec3() const { return CVec3( x, y, z ); }
	CVec2 GetVec2() const { return CVec2( x, y ); }
	CVec3dEx GetVec3dEx() const;
};

class CVec3dEx
{
public:
	double x, y, z;
	BYTE flag;
	//
	CVec3dEx() {}
	//
	CVec3dEx( const double _x, const double _y, const double _z, const BYTE _flag ) : x( _x ), y( _y ), z( _z ), flag( _flag ) {}
	CVec3dEx( const CVec3 &v, const BYTE _flag = 0 ) : x( v.x ), y( v.y ), z( v.z ), flag( _flag ) {}
	CVec3dEx( const CVec3fEx &v ) : x( v.x ), y( v.y ), z( v.z ), flag( v.flag ) {}
	//
	void Set( const double _x, const double _y, const double _z, const BYTE _flag ) { x = _x; y = _y; z = _z; flag = _flag; }
	void Set( const CVec3 &v, const BYTE _flag ) { x = v.x; y = v.y; z = v.z; flag = _flag; }
	//
	bool operator == ( const CVec3dEx &v ) const
	{
		return ( (fabs(x - v.x) < DEF_EPS) && (fabs(y - v.y) < DEF_EPS) && (fabs(z - v.z) < DEF_EPS) && (flag == v.flag) );
	}
	bool operator != ( const CVec3dEx &v ) const
	{
		return ( (fabs(x - v.x) >= DEF_EPS) || (fabs(y - v.y) >= DEF_EPS) || (fabs(z - v.z) >= DEF_EPS) || (flag != v.flag) );
	}
	//
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &x );
		saver.Add( 2, &y );
		saver.Add( 3, &z );
		saver.Add( 4, &flag );
		return 0;
	}
	//
	CVec3 GetVec3() const { return CVec3( x, y, z ); }
	CVec2 GetVec2() const { return CVec2( x, y ); }
	CVec3fEx GetVec3fEx() const;
};

struct SIntersectPoint
{
	CVec3 vPoint;
	float fDist;
	//
	SIntersectPoint() {}
	SIntersectPoint( const SIntersectPoint &p ) : vPoint(p.vPoint), fDist(p.fDist) {}
	SIntersectPoint( const CVec3 &_vPoint, const float _fDist ) : vPoint(_vPoint), fDist(_fDist) {}
	//
	void Set( const CVec3 &_vPoint, const float _fDist ) { vPoint = _vPoint; fDist = _fDist; }
	//
	bool operator < ( const SIntersectPoint &v ) const { return fDist < v.fDist; }
	bool operator == ( const SIntersectPoint &v	) const { return fabs( fDist - v.fDist ) < DEF_EPS; }
};

inline CVec2 GetVec2( const CVec3 &v )
{
	return CVec2( v.x, v.y );
}

template <class TYPE>
inline bool PushBackUnique( std::vector<TYPE> *arr, const TYPE &elem )
{
	for ( std::vector<TYPE>::const_iterator it = arr->begin(); it != arr->end(); ++it )
	{
		if ( *it == elem )
			return false;
	}
	arr->push_back( elem );
	return true;
}

template <>
inline bool PushBackUnique<CVec3>( std::vector<CVec3> *arr, const CVec3 &elem )
{
	for ( std::vector<CVec3>::const_iterator it = arr->begin(); it != arr->end(); ++it )
	{
		if ( ( fabs( it->x - elem.x ) < DEF_EPS ) && ( fabs( it->y - elem.y ) < DEF_EPS ) )
			return false;
	}
	arr->push_back( elem );
	return true;
}

template <>
inline bool PushBackUnique<CVec3fEx>( std::vector<CVec3fEx> *arr, const CVec3fEx &elem )
{
	for ( std::vector<CVec3fEx>::const_iterator it = arr->begin(); it != arr->end(); ++it )
	{
		if ( ( fabs( it->x - elem.x ) < DEF_EPS ) && ( fabs( it->y - elem.y ) < DEF_EPS ) )
			return false;
	}
	arr->push_back( elem );
	return true;
}

template <>
inline bool PushBackUnique<CVec3dEx>( std::vector<CVec3dEx> *arr, const CVec3dEx &elem )
{
	for ( std::vector<CVec3dEx>::const_iterator it = arr->begin(); it != arr->end(); ++it )
	{
		if ( ( fabs( it->x - elem.x ) < DEF_EPS ) && ( fabs( it->y - elem.y ) < DEF_EPS ) )
			return false;
	}
	arr->push_back( elem );
	return true;
}

template <class TYPE>
inline int AddUnique( std::vector<TYPE> *arr, const TYPE &elem )
{
	for ( int i = 0; i < arr->size(); ++i )
	{
		if ( (*arr)[i] == elem )
			return i;
	}
	arr->push_back( elem );
	return ( arr->size() - 1 );
}

template <class TYPE>
inline void AddUnique( std::list<TYPE> *arr, const TYPE &elem )
{
	for ( std::list<TYPE>::const_iterator it = arr->begin(); it != arr->end(); ++it )
	{
		if ( *it == elem )
			return;
	}
	arr->push_back( elem );
}

template <>
inline int AddUnique<CVec3>( std::vector<CVec3> *arr, const CVec3 &elem )
{
	for ( int i = 0; i < arr->size(); ++i )
	{
		if ( (fabs((*arr)[i].x - elem.x) < DEF_EPS) && (fabs((*arr)[i].y - elem.y) < DEF_EPS) )
			return i;
	}
	arr->push_back( elem );
	return ( arr->size() - 1 );
}

template <>
inline int AddUnique<CVec3fEx>( std::vector<CVec3fEx> *arr, const CVec3fEx &elem )
{
	for ( int i = 0; i < arr->size(); ++i )
	{
		if ( (fabs((*arr)[i].x - elem.x) < DEF_EPS) && (fabs((*arr)[i].y - elem.y) < DEF_EPS) )
			return i;
	}
	arr->push_back( elem );
	return ( arr->size() - 1 );
}

template <>
inline int AddUnique<CVec3dEx>( std::vector<CVec3dEx> *arr, const CVec3dEx &elem )
{
	for ( int i = 0; i < arr->size(); ++i )
	{
		if ( (fabs((*arr)[i].x - elem.x) < DEF_EPS) && (fabs((*arr)[i].y - elem.y) < DEF_EPS) )
			return i;
	}
	arr->push_back( elem );
	return ( arr->size() - 1 );
}

inline bool IsCCW( const CVec3dEx &p1, const CVec3dEx &p2, const CVec3dEx &p3 )
{
	return ( ((p2.x - p1.x) * (p3.y - p1.y) - (p2.y - p1.y) * (p3.x - p1.x)) >= 0.0f );
}

inline bool IsCCW( const CVec3 &p1, const CVec3 &p2, const CVec3 &p3 )
{
	return ( ((p2.x - p1.x) * (p3.y - p1.y) - (p2.y - p1.y) * (p3.x - p1.x)) >= 0.0f );
}

inline bool IsCCW( const CVec3 &p1, const CVec3 &p2, const CVec3 &p3, const CVec3 &vNorm )
{
	CVec3 vDot = ( p2 - p1 ) ^ ( p3 - p1 );
	Normalize( &vDot );
	const float fAng = vDot * vNorm;
	return fAng >= 0.0f;
}

inline bool IsTrgSingular( const CVec3dEx &p1, const CVec3dEx &p2, const CVec3dEx &p3 )
{
	return ( abs((p1.x - p2.x) * (p3.y - p2.y) - (p1.y - p2.y) * (p3.x - p2.x)) < DEF_EPS );
}

inline bool IsTrgSingular( const CVec3 &p1, const CVec3 &p2, const CVec3 &p3 )
{
	return ( fabs((p1.x - p2.x) * (p3.y - p2.y) - (p1.y - p2.y) * (p3.x - p2.x)) < DEF_EPS2 );
}

inline void GetTrueNormal( CVec3 *pNorm, const CVec3 &v1, const CVec3 &v2, const CVec3 &v3 )
{
	const CVec3 dv1 = v1 - v2;
	const CVec3 dv2 = v3 - v2;
	(*pNorm) = dv1 ^ dv2;
	Normalize( pNorm );
}

inline void GetBaryCoords( const float px, const float py, const float p0x, const float p0y,
													 const float p1x, const float p1y, const float p2x, const float p2y,
													 CVec2 *pBary )
{
	float d = ( p1x - p0x ) * ( p2y - p0y ) - ( p1y - p0y ) * ( p2x - p0x );
	if ( fabs(d) > DEF_EPS )
	{
		d = 1.0 / d;
		pBary->x = ( (px - p0x) * (p2y - p0y) - (py - p0y) * (p2x - p0x) ) * d;
		pBary->y = ( (py - p0y) * (p1x - p0x) - (px - p0x) * (p1y - p0y) ) * d;
	}
	else
	{
		pBary->x = -1.0f;
		pBary->y = -1.0f;
	}
}

inline void GetBaryCoords( const CVec3 &p, const CVec3 &p0, const CVec3 &p1, const CVec3 &p2, CVec2 *pBary )
{
	float d = ( p1.x - p0.x ) * ( p2.y - p0.y ) - ( p1.y - p0.y ) * ( p2.x - p0.x );
	if ( fabs(d) > DEF_EPS )
	{
		d = 1.0 / d;
		pBary->x = ( (p.x - p0.x) * (p2.y - p0.y) - (p.y - p0.y) * (p2.x - p0.x) ) * d;
		pBary->y = ( (p.y - p0.y) * (p1.x - p0.x) - (p.x - p0.x) * (p1.y - p0.y) ) * d;
	}
	else
	{
		d = ( p1.y - p0.y ) * ( p2.z - p0.z ) - ( p1.z - p0.z ) * ( p2.y - p0.y );
		if ( fabs(d) > DEF_EPS )
		{
			d = 1.0 / d;
			pBary->x = ( (p.y - p0.y) * (p2.z - p0.z) - (p.z - p0.z) * (p2.y - p0.y) ) * d;
			pBary->y = ( (p.z - p0.z) * (p1.y - p0.y) - (p.y - p0.y) * (p1.z - p0.z) ) * d;
		}
		else
		{
			d = ( p1.z - p0.z ) * ( p2.x - p0.x ) - ( p1.x - p0.x ) * ( p2.z - p0.z );
			if ( fabs(d) > DEF_EPS )
			{
				d = 1.0 / d;
				pBary->x = ( (p.z - p0.z) * (p2.x - p0.x) - (p.x - p0.x) * (p2.z - p0.z) ) * d;
				pBary->y = ( (p.x - p0.x) * (p1.z - p0.z) - (p.z - p0.z) * (p1.x - p0.x) ) * d;
			}
			else
				pBary->Set( -1, -1 );
		}
	}
}

inline void GetBaryCoords( const CVec3fEx &p, const CVec3 &p0, const CVec3 &p1, const CVec3 &p2, CVec2 *pBary )
{
	//GetBaryCoords( p.x, p.y, p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, pBary );
	GetBaryCoords( p.GetVec3() , p0, p1, p2, pBary );
}

inline void GetBaryCoords( const CVec3 &p, const CVec3fEx &p0, const CVec3fEx &p1, const CVec3fEx &p2, CVec2 *pBary )
{
	//GetBaryCoords( p.x, p.y, p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, pBary );
	GetBaryCoords( p, p0.GetVec3(), p1.GetVec3(), p2.GetVec3(), pBary );
}

//inline void GetBaryCoords( const CVec3 &p, const CVec3 &p0, const CVec3 &p1, const CVec3 &p2, CVec2 *pBary )
//{
//	GetBaryCoords( p.x, p.y, p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, pBary );
//}

inline void GetBaryCoords( const CVec2 &p, const CVec3 &p0, const CVec3 &p1, const CVec3 &p2, CVec2 *pBary )
{
	GetBaryCoords( p.x, p.y, p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, pBary );
}

inline void GetBaryCoords( const CVec2 &p, const CVec3fEx &p0, const CVec3fEx &p1, const CVec3fEx &p2, CVec2 *pBary )
{
	GetBaryCoords( p.x, p.y, p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, pBary );
}

inline void GetBaryCoords( const CVec3fEx &p, const CVec3dEx &p0, const CVec3dEx &p1, const CVec3dEx &p2, CVec2 *pBary )
{
	//GetBaryCoords( p.x, p.y, p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, pBary );
	GetBaryCoords( p.GetVec3(), p0.GetVec3(), p1.GetVec3(), p2.GetVec3(), pBary );
}

inline bool IsBaryIn( const CVec2 &v )
{
	return ( v.x > -EPS_VALUE ) && ( v.y > -EPS_VALUE ) && ( (v.x + v.y) < (1.0f + EPS_VALUE) );
}

inline float GetPointDistOnSegment( const CVec3 &p, const CVec3 &p1, const CVec3 &p2 )
{
	if ( fabs(p2.x - p1.x) > DEF_EPS )
		return ( p.x - p1.x ) / ( p2.x - p1.x );
	else
		if ( fabs(p2.y - p1.y) > DEF_EPS )
			return ( p.y - p1.y ) / ( p2.y - p1.y );
	return 0.0f;
}

// Calc normalized canonical line equation ax+by+c=0
inline void GetLineEq( const float x1, const float y1, const float x2, const float y2, float *a, float *b, float *c )
{
	const float ta = y1 - y2;
	const float tb = x2 - x1;
	const float tc = -x1*ta - y1*tb;
	const float fpMod = ta*ta + tb*tb;
	if ( fpMod < 1e-7 )
	{
		*a = 0.7071f;
		*b = -0.7071f;
		*c = 0;
		return;
	}
	const float sq = 1.0 / sqrt( fpMod );
	*a = ta * sq;
	*b = tb * sq;
	*c = tc * sq;
}

inline void GetLineEq( const double x1, const double y1, const double x2, const double y2, double *a, double *b, double *c )
{
	const double ta = y1 - y2;
	const double tb = x2 - x1;
	const double tc = -x1*ta - y1*tb;
	const double fpMod = ta*ta + tb*tb;
	if ( fpMod < 1e-7 )
	{
		*a = 0.7071;
		*b = -0.7071;
		*c = 0;
		return;
	}
	const double sq = 1.0 / sqrt( fpMod );
	*a = ta * sq;
	*b = tb * sq;
	*c = tc * sq;
}

class CTriangleEx
{
public:
	CVec3dEx points[3];
	CVec3dEx coeffs[3];
	//
	CTriangleEx() {};
	//
	CTriangleEx( const CVec3fEx &p1, const CVec3fEx &p2, const CVec3fEx &p3 )
	{
		points[0] = p1;
		points[1] = p2;
		points[2] = p3;
		GetLineEq( p1.x, p1.y, p2.x, p2.y, &(coeffs[0].x), &(coeffs[0].y), &(coeffs[0].z) );
		GetLineEq( p2.x, p2.y, p3.x, p3.y, &(coeffs[1].x), &(coeffs[1].y), &(coeffs[1].z) );
		GetLineEq( p3.x, p3.y, p1.x, p1.y, &(coeffs[2].x), &(coeffs[2].y), &(coeffs[2].z) );
	}
	//
	void Set( const CVec3fEx &p1, const CVec3fEx &p2, const CVec3fEx &p3 )
	{
		points[0] = p1;
		points[1] = p2;
		points[2] = p3;
		GetLineEq( p1.x, p1.y, p2.x, p2.y, &(coeffs[0].x), &(coeffs[0].y), &(coeffs[0].z) );
		GetLineEq( p2.x, p2.y, p3.x, p3.y, &(coeffs[1].x), &(coeffs[1].y), &(coeffs[1].z) );
		GetLineEq( p3.x, p3.y, p1.x, p1.y, &(coeffs[2].x), &(coeffs[2].y), &(coeffs[2].z) );
	}
};

inline float GetDistToSegment( const CVec2 &v, const CVec2 &v1, const CVec2 &v2 )
{
	const float fDist = fabs2( v2.x - v1.x ) + fabs2( v2.y - v1.y );
	if ( fDist < DEF_EPS )
		return 0.0f;
	const float t = ( (v2.x - v1.x) * (v.x - v1.x) + (v2.y - v1.y) * (v.y - v1.y) ) / fDist;
	if ( (t > -DEF_EPS) && (t < (1.0f + DEF_EPS)) )
	{
		const CVec2 p( v1.x + (v2.x - v1.x) * t, v1.y + (v2.y - v1.y) * t );
		return fabs2( v - p );
	}
	return min( fabs2(v - v1), fabs2(v - v2) );
}

// return error between source point and point on segment
inline float GetErrorDist( const CVec3fEx &v, const CVec3fEx &v1, const CVec3fEx &v2, float *pRes )
{
	if ( fabs(v2.x - v1.x) > DEF_EPS )
	{
		const float t = ( v.x - v1.x ) / ( v2.x - v1.x );
		(*pRes) = t;
		if ( (t > -DEF_EPS) && (t < (1.0 + DEF_EPS)) )
		{
			const float y = v1.y + ( v2.y - v1.y ) * t - v.y;
			return y * y;
		}
		return min( fabs2(v.x - v1.x) + fabs2(v.y - v1.y), fabs2(v.x - v2.x) + fabs2(v.y - v2.y) );
	}
	else
		if ( fabs(v2.y - v1.y) > DEF_EPS )
		{
			const float t = ( v.y - v1.y ) / ( v2.y - v1.y );
			(*pRes) = t;
			if ( (t > -DEF_EPS) && (t < (1.0 + DEF_EPS)) )
			{
				const float x = v1.x + ( v2.x - v1.x ) * t - v.x;
				return x * x;
			}
			return min( fabs2(v.x - v1.x) + fabs2(v.y - v1.y), fabs2(v.x - v2.x) + fabs2(v.y - v2.y) );
		}
	return FP_MAX_VALUE;
}

inline float GetErrorDist( const CVec3fEx &v, const CVec3 &v1, const CVec3 &v2, float *pRes )
{
	if ( fabs(v2.x - v1.x) > DEF_EPS )
	{
		const float t = ( v.x - v1.x ) / ( v2.x - v1.x );
		const float y = v1.y + ( v2.y - v1.y ) * t - v.y;
		(*pRes) = t;
		if ( (t > -DEF_EPS) && (t < (1.0 + DEF_EPS)) )
			return y * y;
		return min( (v.x - v1.x) * (v.x - v1.x) + (y - v1.y) * (y - v1.y),
								(v.x - v2.x) * (v.x - v2.x) + (y - v2.y) * (y - v2.y) );
	}
	else
		if ( fabs(v2.y - v1.y) > DEF_EPS )
		{
			const float t = ( v.y - v1.y ) / ( v2.y - v1.y );
			const float x = v1.x + ( v2.x - v1.x ) * t - v.x;
			(*pRes) = t;
			if ( (t > -DEF_EPS) && (t < (1.0 + DEF_EPS)) )
				return x * x;
			return min( (x - v1.x) * (x - v1.x) + (v.y - v1.y) * (v.y - v1.y),
									(x - v2.x) * (x - v2.x) + (v.y - v2.y) * (v.y - v2.y) );
		}
		return FP_MAX_VALUE;
}

inline bool IsInside( const CTriangleEx &trg, const CVec3dEx &vert/*, bool bIncludeBorders = false*/ )
{
	int nCount = 0;
	{
		const double d = trg.coeffs[0].x * vert.x + trg.coeffs[0].y * vert.y + trg.coeffs[0].z;
		if ( fabs(d) < DEF_EPS )
			return false;

		if ( d > 0 )
			++nCount;
		else
			--nCount;
	}
	{
		const double d = trg.coeffs[1].x * vert.x + trg.coeffs[1].y * vert.y + trg.coeffs[1].z;
		if ( fabs(d) < DEF_EPS )
			return false;

		if ( d > 0 )
			++nCount;
		else
			--nCount;
	}
	{
		const double d = trg.coeffs[2].x * vert.x + trg.coeffs[2].y * vert.y + trg.coeffs[2].z;
		if ( fabs(d) < DEF_EPS )
			return false;

		if ( d > 0 )
			++nCount;
		else
			--nCount;
	}
	return ( (nCount == 3) || (nCount == -3) );
}

inline bool IsInside( const std::vector<CVec3> &lineCoeffs, const CVec3 &vert )
{
	int nCountL = 0, nCountR = 0;
	for ( std::vector<CVec3>::const_iterator it = lineCoeffs.begin(); it != lineCoeffs.end(); ++it )
	{
		const float d = it->x * vert.x + it->y * vert.y + it->z;
		if ( fabs(d) < DEF_FLOAT_EPS )
		{
			++nCountL;
			++nCountR;
		}
		else
			if ( d < 0 )
				++nCountL;
			else
				++nCountR;
	}
	return ( (nCountL == lineCoeffs.size()) || (nCountR == lineCoeffs.size()) );
}

inline bool IsIntersect( const CVec3 &p1, const CVec3 &p2, const CVec3 &v1, const CVec3 &v2 )
{
	const double fDet = (double)( p2.x - p1.x ) * ( v2.y - v1.y ) - ( p2.y - p1.y ) * ( v2.x - v1.x );
	if ( fabs(fDet) > DEF_EPS )
	{
		const double fInvDet = 1.0 / fDet;
		const double t = ( (double)(v1.x - p1.x) * (v2.y - v1.y) - (v1.y - p1.y) * (v2.x - v1.x) ) * fInvDet;
		const double k = ( (double)(v1.x - p1.x) * (p2.y - p1.y) - (v1.y - p1.y) * (p2.x - p1.x) ) * fInvDet;
		if ( (t > DEF_EPS) && (t < (1.0 - DEF_EPS)) && (k > DEF_EPS) && (k < (1.0 - DEF_EPS)) )
			return true;
	}
	else
	{
		const double fDet = (double)( p2.y - p1.y ) * ( v2.z - v1.z ) - ( p2.z - p1.z ) * ( v2.y - v1.y );
		if ( fabs(fDet) > DEF_EPS )
		{
			const double fInvDet = 1.0 / fDet;
			const double t = ( (double)(v1.y - p1.y) * (v2.z - v1.z) - (v1.z - p1.z) * (v2.y - v1.y) ) * fInvDet;
			const double k = ( (double)(v1.y - p1.y) * (p2.z - p1.z) - (v1.z - p1.z) * (p2.y - p1.y) ) * fInvDet;
			if ( (t > DEF_EPS) && (t < (1.0 - DEF_EPS)) && (k > DEF_EPS) && (k < (1.0 - DEF_EPS)) )
				return true;
		}
		else
		{
			const double fDet = (double)( p2.x - p1.x ) * ( v2.z - v1.z ) - ( p2.z - p1.z ) * ( v2.x - v1.x );
			if ( fabs(fDet) > DEF_EPS )
			{
				const double fInvDet = 1.0 / fDet;
				const double t = ( (double)(v1.x - p1.x) * (v2.z - v1.z) - (v1.z - p1.z) * (v2.x - v1.x) ) * fInvDet;
				const double k = ( (double)(v1.x - p1.x) * (p2.z - p1.z) - (v1.z - p1.z) * (p2.x - p1.x) ) * fInvDet;
				if ( (t > DEF_EPS) && (t < (1.0 - DEF_EPS)) && (k > DEF_EPS) && (k < (1.0 - DEF_EPS)) )
					return true;
			}
		}
	}
	return false;
}

inline bool IsIntersect( const CVec3dEx &p1, const CVec3dEx &p2, const CVec3dEx &v1, const CVec3dEx &v2 )
{
	const double fDet = (double)( p2.x - p1.x ) * ( v2.y - v1.y ) - ( p2.y - p1.y ) * ( v2.x - v1.x );
	if ( fabs(fDet) > DEF_EPS )
	{
		const double fInvDet = 1.0 / fDet;

		const double t = ( (double)(v1.x - p1.x) * (v2.y - v1.y) - (v1.y - p1.y) * (v2.x - v1.x) ) * fInvDet;
		const double k = ( (double)(v1.x - p1.x) * (p2.y - p1.y) - (v1.y - p1.y) * (p2.x - p1.x) ) * fInvDet;
		if ( (t > DEF_EPS) && (t < (1.0 - DEF_EPS)) && (k > DEF_EPS) && (k < (1.0 - DEF_EPS)) )
			return true;
	}
	return false;
}

inline bool IsIntersectOneOf( const CVec3 &p1, const CVec3 &p2, const CVec3 &v1, const CVec3 &v2 )
{
	const double fDet = (double)( p2.x - p1.x ) * ( v2.y - v1.y ) - ( p2.y - p1.y ) * ( v2.x - v1.x );
	if ( fabs(fDet) > DEF_EPS )
	{
		const double fInvDet = 1.0 / fDet;
		const double t = ( (double)(v1.x - p1.x) * (v2.y - v1.y) - (v1.y - p1.y) * (v2.x - v1.x) ) * fInvDet;
		const double k = ( (double)(v1.x - p1.x) * (p2.y - p1.y) - (v1.y - p1.y) * (p2.x - p1.x) ) * fInvDet;
		if ( ((t > -DEF_EPS) && (t < (1.0f + DEF_EPS))) || ((k > -DEF_EPS) && (k < (1.0f + DEF_EPS))) )
			return true;
	}
	return false;
}

inline void AddIntersection( std::vector<CVec3> *pIntersection, const CVec3 &p1, const CVec3 &p2, const CVec3 &v1, const CVec3 &v2 )
{
	const float fDet = ( p2.x - p1.x ) * ( v2.y - v1.y ) - ( p2.y - p1.y ) * ( v2.x - v1.x );
	if ( fabs(fDet) > DEF_EPS )
	{
		const float fInvDet = 1.0 / fDet;
		const float t = ( (v1.x - p1.x) * (v2.y - v1.y) - (v1.y - p1.y) * (v2.x - v1.x)) * fInvDet;
		const float k = ( (v1.x - p1.x) * (p2.y - p1.y) - (v1.y - p1.y) * (p2.x - p1.x)) * fInvDet;
		if ( (t > -DEF_EPS) && (t < (1.0 + DEF_EPS)) && (k > -DEF_EPS) && (k < (1.0 + DEF_EPS)) )
		{
			pIntersection->push_back( CVec3(v1.x + (v2.x - v1.x) * k, v1.y + (v2.y - v1.y) * k, v1.z + (v2.z - v1.z) * k) );
		}
	}
}

inline void AddIntersectionOneSide( std::vector<SIntersectPoint> *pIntersection, const CVec3 &v1, const CVec3 &v2, const CVec3 &p1, const CVec3 &p2 )
{
	const float fDet = ( v2.x - v1.x ) * ( p2.y - p1.y ) - ( v2.y - v1.y ) * ( p2.x - p1.x );
	if ( fabs( fDet ) > DEF_EPS )
	{
		const float fInvDet = 1.0 / fDet;
		const float t = ( (p1.x - v1.x) * (p2.y - p1.y) - (p1.y - v1.y) * (p2.x - p1.x) ) * fInvDet;
		const float k = ( (v1.y - p1.y) * (v2.x - v1.x) - (v1.x - p1.x) * (v2.y - v1.y) ) * fInvDet;
		if ( (t > DEF_EPS) && (t < (1.0 - DEF_EPS)) && (k > -DEF_EPS) && (k < (1.0 + DEF_EPS)) )
			PushBackUnique( pIntersection, SIntersectPoint(p1 + (p2 - p1) * k, t) );
	}
}

inline void AddIntersection( std::vector<SIntersectPoint> *pIntersection, const CVec3 &v1, const CVec3 &v2, const CVec3 &p1, const CVec3 &p2 )
{
	const float fDet = ( v2.x - v1.x ) * ( p2.y - p1.y ) - ( v2.y - v1.y ) * ( p2.x - p1.x );
	if ( fabs(fDet) > DEF_EPS )
	{
		const float fInvDet = 1.0 / fDet;
		const float t = ( (p1.x - v1.x) * (p2.y - p1.y) - (p1.y - v1.y) * (p2.x - p1.x) ) * fInvDet;
		const float k = ( (v1.y - p1.y) * (v2.x - v1.x) - (v1.x - p1.x) * (v2.y - v1.y) ) * fInvDet;
		if ( (t > -DEF_EPS) && (t < (1.0 + DEF_EPS)) && (k > -DEF_EPS) && (k < (1.0 + DEF_EPS)) )
			PushBackUnique( pIntersection, SIntersectPoint( v1 + (v2 - v1) * t, t ) );
	}
}

// whether "rV" is inside "rPoly"
B2_M1_TERRAIN_EXPORT bool IsInside( const std::vector<CVec3dEx> &rPoly, const CVec3dEx &rV, bool bIncludeBorders = false );
B2_M1_TERRAIN_EXPORT bool IsOutside( const std::vector<CVec3dEx> &rPoly, const CVec3dEx &rV );
B2_M1_TERRAIN_EXPORT void GetIntersection( std::vector<CVec3dEx> *pIntersection, const std::vector<CVec3dEx> &rPoly, const CVec3dEx &rV1, const CVec3dEx &rV2 );
B2_M1_TERRAIN_EXPORT void GetIntersection( std::vector<SIntersectPoint> *pIntersection, const std::vector<CVec3fEx> &rPoly, const CVec3 &rV1, const CVec3 &rV2 );
B2_M1_TERRAIN_EXPORT void GetIntersection( std::vector<SIntersectPoint> *pIntersection, const std::vector<NDb::SVSOPoint> &rPoly, const CVec3 &rV1, const CVec3 &rV2 );
void GetBorderIntersection( std::vector<CVec3> *pBorderIntersection, const CTriangleEx &rTriangle1, const CTriangleEx &rTriangle2 );
void AttachIntersection( std::vector<NDb::SVSOPoint> *pIntersection, const std::vector<NDb::SVSOPoint> &rPoly, const bool bSetFlag );
B2_M1_TERRAIN_EXPORT bool IsIntersect( const std::vector<CVec3dEx> &rPoly, const CVec3dEx &rV1, const CVec3dEx &rV2 );
B2_M1_TERRAIN_EXPORT bool IsIntersect( const std::vector<STriangle> &rTriangles, const std::vector<CVec3dEx> &rVerts, const CVec3dEx &rV1, const CVec3dEx &rV2 );
B2_M1_TERRAIN_EXPORT void CreateConvexHull( std::vector<CVec3> *pResPoints, const std::vector<CVec3> &rSourcePoints );

void GetIntersectionTriangles( std::vector<CTriangleEx> *pIntersection, const CTriangleEx &rTriangle1, const CTriangleEx &rTriangle2 );
bool IsInsideTriangle( const CTriangleEx &rTriangle, const CVec3dEx &rPoint, bool bIncludeBorders = false );
bool AreTrianglesTakenUp( const CTriangleEx rOuterTriangle, const CTriangleEx rInnerTriangle, bool bIncludeBorders = true );

