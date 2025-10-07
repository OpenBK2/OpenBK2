#include "stdafx.h"

#include "Geom.h"

void CQuat::DecompEulerAngles( float *pfYaw, float *pfPitch, float *pfRoll )
{
	const float x2 = x*x;
	const float y2 = y*y;
	const float z2 = z*z;
	const float w2 = w*w;

	*pfYaw = atan2( 2*(w*z + x*y), w2 - z2 + x2 - y2 );
	*pfPitch = -asin( 2*(z*x - w*y) );
	*pfRoll = atan2( 2*(z*y + w*x), w2 + z2 - x2 - y2 );
}

const WORD GetDirectionByVector( float x, float y )
{
	if ( IsAlmostZero( x, y ) )
		return 0;
	
	float add = 49152.0f;

	if ( x <= 0 && y > 0 )
	{
		add = 0.0f;

		std::swap( x, y );
		y = -y;
	}
	else if ( y <= 0 && x < 0 )
	{
		add = 16384.0f;
		
		x = -x;
		y = -y;
	}
	else if ( x >= 0 && y < 0 )
	{
		add = 32768.0f;
		
		std::swap( x, y );
		x = -x;
	}
		
	//NI_VERIFY( (x >= 0) && (y >= 0), StrFmt("Wrong vector {%g, %g}", x, y), return 0 );
	if ( x < 0 || y < 0 )
		return 0;

	if ( x + y != 0 )
		return 16384.0f * y / ( x + y ) + add;
	else
		return 0;
}

const WORD GetDirectionByVector( const CVec2 &vec )
{
	return GetDirectionByVector( vec.x, vec.y );
}

const CVec2 GetVectorByDirection( const WORD dir )
{
	const float fDir = float(dir % 16384) / 16384.0f;
	CVec2 result( 1-fDir, fDir );

	if ( dir < 16384 )
	{
		result.y = -result.y;
		std::swap( result.x, result.y );
	}
	else if ( dir < 32768 )
	{
		result.x = -result.x;
		result.y = -result.y;
	}
	else if ( dir < 49152 )
	{
		result.x = -result.x;
		std::swap( result.x, result.y );
	}

	Normalize( &result );
	return result;
}

const WORD DirsDifference( const WORD dir1, const WORD dir2 )
{
	const	WORD clockWise = dir1-dir2;
	const	WORD antiClockWise = dir2-dir1;

	return (std::min)( clockWise, antiClockWise );
}

const int DifferenceSign( const WORD dir1, const WORD dir2 )
{
	const	WORD clockWise = dir1-dir2;
	const	WORD antiClockWise = dir2-dir1;

	return boost::math::sign(int(antiClockWise) - int(clockWise));
}

bool IsInTheAngle( const WORD dir, const WORD startAngleDir, const WORD finishAngleDir )
{
	return 
		WORD( dir - startAngleDir ) + WORD( finishAngleDir - dir ) == WORD( finishAngleDir - startAngleDir );
}

bool IsInTheMinAngle( const WORD dir, const WORD dir1, const WORD dir2 )
{
	return
		(int)DirsDifference( dir, dir1 ) + (int)DirsDifference( dir, dir2 ) == DirsDifference( dir1, dir2 );
}

const WORD GetZDirectionBy3DVector( const float x, const float y, const float z )
{
	return GetDirectionByVector( fabs( x, y ), z );
}

const WORD GetZDirectionBy3DVector( const CVec2 &vec, const float z )
{
	return GetZDirectionBy3DVector( vec.x, vec.y, z );
}

const WORD GetZAngle( const CVec2 &vec, const float z )
{
	return GetZAngle( vec.x, vec.y, z );
}

const WORD GetZAngle( const float x, const float y, const float z )
{
	const WORD wZDir = GetZDirectionBy3DVector( x, y, z );

	const WORD wZAngle = (std::min)( DirsDifference( wZDir, 16384 * 3 ), DirsDifference( wZDir, 16384 ) );
	return z >= 0.0f ? wZAngle : 65536 - wZAngle;
}

const WORD GetZAngle( const CVec3 &vPoint )
{
	return GetZAngle( vPoint.x, vPoint.y, vPoint.z );
}

//*******************************************************************
//*															SRect																*
//*******************************************************************

void SRect::InitRect( const CVec2 &_v1, const CVec2 &_v2, const CVec2 &_v3, const CVec2 &_v4 )
{
	v1 = _v1;
	v2 = _v2;
	v3 = _v3;
	v4 = _v4;
	center = ( v1 + v3 ) * 0.5;

	dir = v4 - v1;

	lengthBack = lengthAhead = fabs( dir );
	Normalize( &dir );

	dirPerp.x = -dir.y;
	dirPerp.y = dir.x;

	width = fabs( v2 - v1 ) * 0.5f;
}

void SRect::InitRect( const CVec2 &_center, const CVec2 &_dir, const float length, const float _width )
{
	center = _center;
	dir = _dir;
	Normalize( &dir );

	dirPerp.x = -dir.y;
	dirPerp.y = dir.x;

	lengthBack = lengthAhead = length;
	width = _width;

	const CVec2 pointBack = center - dir * length;
	const CVec2 pointForward = center + dir * length;

	v1 = pointBack - dirPerp * width;
	v2 = pointBack + dirPerp * width;
	v3 = pointForward + dirPerp * width;
	v4 = pointForward - dirPerp * width;
}

void SRect::InitRect( const CVec2 &_center, const CVec2 &_dir, const float _lengthAhead, const float _lengthBack, const float _width )
{
	center = _center;
	dir = _dir;
	Normalize( &dir );
	dirPerp.x = -dir.y;
	dirPerp.y = dir.x;

	lengthBack = _lengthBack;
	lengthAhead = _lengthAhead;
	width = _width;

	const CVec2 pointBack = center - dir * lengthBack;
	const CVec2 pointForward = center + dir * lengthAhead;

	v1 = pointBack - dirPerp * width;
	v2 = pointBack + dirPerp * width;
	v3 = pointForward + dirPerp * width;
	v4 = pointForward - dirPerp * width;
}

bool SRect::IsIntersectProject( const CVec2 &v1, const CVec2 &v2, const CVec2 &v3, const CVec2 &v4, const CVec2 &dir, const float min, const float max ) const
{
	const float proj1 = v1 * dir;
	const float proj2 = v2 * dir;
	const float proj3 = v3 * dir;
	const float proj4 = v4 * dir;

	const float min12 = (std::min)( proj1, proj2 );
	const float min34 = (std::min)( proj3, proj4 );
	const float max12 = (std::max)( proj1, proj2 );
	const float max34 = (std::max)( proj3, proj4 );

	return 
		!( (std::min)( min12, min34 ) >= max || (std::max)( max12, max34 ) <= min );
}

bool SRect::IsIntersected( const CSegment &segment ) const
{
	CVec2 vSegmetDir = segment.p2 - segment.p1;
	const float fSegmLength = fabs( vSegmetDir );
	vSegmetDir /= fSegmLength;
	
	const CVec2 vYDir( 0.0f, 1.0f );
	const CVec2 vSegmentDirPerp = vSegmetDir ^ vYDir;

	return 	
		IsIntersectProject( v1 - segment.p1, v2 - segment.p1, v3 - segment.p1, v4 - segment.p1, vSegmetDir, 0.0f, fSegmLength ) &&
		IsIntersectProject( v1 - segment.p1, v2 - segment.p1, v3 - segment.p1, v4 - segment.p1, vSegmentDirPerp, 0.0f, 0.0f );
}

bool SRect::IsIntersected( const SRect &rect ) const
{
	return 
		IsIntersectProject( v1 - rect.center, v2 - rect.center, v3 - rect.center, v4 - rect.center, rect.dir, -rect.lengthBack, rect.lengthAhead ) &&
		IsIntersectProject( v1 - rect.center, v2 - rect.center, v3 - rect.center, v4 - rect.center, rect.dirPerp, -rect.width, rect.width ) &&
		IsIntersectProject( rect.v1 - center, rect.v2 - center, rect.v3 - center, rect.v4 - center, dir, -lengthBack, lengthAhead ) &&
		IsIntersectProject( rect.v1 - center, rect.v2 - center, rect.v3 - center, rect.v4 - center, dirPerp, -width, width );
}

bool SRect::IsPointInside( const CVec2 &point ) const
{
	const CVec2 center( ( v1.x + v2.x + v3.x + v4.x ) / 4, ( v1.y + v2.y + v3.y + v4.y ) / 4 );
	const short int rightSign = boost::math::sign( TriangleAAA( v1, v2, center ) );

	// вырожденный прямоугольник
	if ( rightSign == 0 )
		return fabs2( point - center ) < 0.001f;

	return 
		boost::math::sign( TriangleAAA ( v1, v2, point ) ) == rightSign && boost::math::sign( TriangleAAA ( v2, v3, point ) ) == rightSign &&
		boost::math::sign( TriangleAAA ( v3, v4, point ) ) == rightSign && boost::math::sign( TriangleAAA ( v4, v1, point ) ) == rightSign;
}

bool SRect::IsIntersectCircle( const CVec2 &circleCenter, const float r ) const
{
	if ( IsPointInside( circleCenter ) )
		return true;

	const CVec2 vNewCenter = circleCenter - center;
	const CVec2 vLocalCoordCenter( vNewCenter.x * dir.x + vNewCenter.y * dir.y, -vNewCenter.x * dir.y + vNewCenter.y * dir.x );

	float fDist = 0;

	if ( vLocalCoordCenter.x < -lengthBack )
		fDist += sqr( vLocalCoordCenter.x - (-lengthBack) );
	else if ( vLocalCoordCenter.x > lengthAhead )
		fDist += sqr( vLocalCoordCenter.x - lengthAhead );

	if ( vLocalCoordCenter.y < -width )
		fDist += sqr( vLocalCoordCenter.y - (-width) );
	else if ( vLocalCoordCenter.y > width )
		fDist += sqr( vLocalCoordCenter.y - width );

	return fDist <= sqr( r );
}

bool SRect::IsIntersectTriangle( const CVec2 &vTr1, const CVec2 &vTr2, const CVec2 &vTr3 ) const
{
	if ( IsPointInsideTriangle( vTr1, vTr2, vTr3, v1 ) || IsPointInsideTriangle( vTr1, vTr2, vTr3, v2 ) ||
			 IsPointInsideTriangle( vTr1, vTr2, vTr3, v3 ) || IsPointInsideTriangle( vTr1, vTr2, vTr3, v4 ) )
		 return true;

	CSegment segment1( vTr1, vTr2 );
	if ( IsIntersected( segment1 ) )
		return true;

	CSegment segment2( vTr1, vTr3 );
	if ( IsIntersected( segment2 ) )
		return true;

	CSegment segment3( vTr2, vTr3 );
	if ( IsIntersected( segment3 ) )
		return true;

	return false;
}

const int SRect::GetSide( const WORD dirFromRectCenter ) const
{
	// разница по модулю 65536	
	const WORD diff = dirFromRectCenter - GetDirectionByVector( dir );

	if ( diff <= 8192 )
		return SIDE_FRONT;
	else if ( diff > 8192 && diff <= 24576 )
		return SIDE_LEFT;
	else if ( diff > 24576 && diff <= 40960 )
		return SIDE_BACK;
	else if ( diff > 40960 && diff <= 57344 )
		return SIDE_RIGHT;
	else
		return SIDE_FRONT;
}

const int SRect::GetSide( const CVec2 &point ) const
{
	return GetSide( GetDirectionByVector( point - center ) );
}

void SRect::Compress( const float fFactor )
{
	lengthAhead *= fFactor;
	lengthBack *= fFactor;
	width *= fFactor;

	InitRect( center, dir, lengthAhead, lengthBack, width );
}

const float fabs( const SRect rect1, const SRect rect2 )
{
	CVec2 dir( rect1.center - rect2.center );
	const float dist = fabs( dir );
	Normalize( &dir );

	const float f1_1 = ( rect1.v1 - rect1.center ) * dir + dist;
	const float f1_2 = ( rect1.v2 - rect1.center ) * dir + dist;
	const float f1_3 = ( rect1.v3 - rect1.center ) * dir + dist;
	const float f1_4 = ( rect1.v4 - rect1.center ) * dir + dist;

	const float f2_1 = ( rect2.v1 - rect2.center ) * dir;
	const float f2_2 = ( rect2.v2 - rect2.center ) * dir;
	const float f2_3 = ( rect2.v3 - rect2.center ) * dir;
	const float f2_4 = ( rect2.v4 - rect2.center ) * dir;

	const float segm1Min = (std::min)( (std::min)( f1_1, f1_2 ), (std::min)( f1_3, f1_4 ) );
	const float segm1Max = (std::max)( (std::max)( f1_1, f1_2 ), (std::max)( f1_3, f1_4 ) );

	const float segm2Min = (std::min)( (std::min)( f2_1, f2_2 ), (std::min)( f2_3, f2_4 ) );
	const float segm2Max = (std::max)( (std::max)( f2_1, f2_2 ), (std::max)( f2_3, f2_4 ) );

	// не пересекаются
	if ( segm1Max < segm2Min || segm2Max < segm1Min )
		return (std::min)( fabs( segm1Max - segm2Min ), fabs( segm2Max - segm1Min ) );
	else
		return 0;
}

const bool GetRectBeamIntersection( CVec2 *pvResult, const CVec2 &vPoint, const CVec2 &vDir, const SRect &rect )
{
	if ( pvResult )
		*pvResult = VNULL2;
  
	int nNearestPoint = 0;
	float fNearestDist2 = fabs2( vPoint - rect.v[0] );
	for ( int i = 1; i < 4; ++i )
	{
		const float fThisDist2 = fabs2( vPoint - rect.v[i] );
		if ( fNearestDist2 > fThisDist2 )
		{
			nNearestPoint = i;
			fNearestDist2 = fThisDist2;
		}
	}

	if ( fNearestDist2 == 0.0f )
	{
		if ( pvResult )
			*pvResult = rect.v[nNearestPoint];
		return true;
	}

	CVec2 vDirNorm( vDir );
	if ( !Normalize( &vDirNorm ) )
		return false;

	const int nPrevPoint = nNearestPoint == 0 ? 3 : nNearestPoint - 1;
	const int nNextPoint = nNearestPoint == 3 ? 0 : nNearestPoint + 1;

	const float dPrev = ( rect.v[nNearestPoint].x - rect.v[nPrevPoint].x )*vDirNorm.y - ( rect.v[nNearestPoint].y - rect.v[nPrevPoint].y )*vDirNorm.x;
	if ( dPrev != 0.0f )
	{
		const float tr = ( ( vPoint.x - rect.v[nPrevPoint].x )*vDirNorm.y - ( vPoint.y - rect.v[nPrevPoint].y )*vDirNorm.x )/dPrev;
		if ( tr >= 0.0f && tr <= 1.0f )
		{
			if ( pvResult )
				*pvResult = rect.v[nPrevPoint] + ( rect.v[nNearestPoint] - rect.v[nPrevPoint] )*tr;
			return true;
		}
	}

	const float dNext = ( rect.v[nNearestPoint].x - rect.v[nNextPoint].x )*vDirNorm.y - ( rect.v[nNearestPoint].y - rect.v[nNextPoint].y )*vDirNorm.x;
	if ( dNext != 0.0f )
	{
		const float tr = ( ( vPoint.x - rect.v[nNextPoint].x )*vDirNorm.y - ( vPoint.y - rect.v[nNextPoint].y )*vDirNorm.x )/dNext;
		if ( tr >= 0.0f && tr <= 1.0f )
		{
			if ( pvResult )
				*pvResult = rect.v[nNextPoint] + ( rect.v[nNearestPoint] - rect.v[nNextPoint] )*tr;
			return true;
		}
	}
	return false;
}

const WORD GetVisibleAngle( const CVec2 &point, const SRect rect )
{
	const WORD wAngle1 = GetDirectionByVector( rect.v1 - point );
	const WORD wAngle2 = GetDirectionByVector( rect.v2 - point );
	const WORD wAngle3 = GetDirectionByVector( rect.v3 - point );
	const WORD wAngle4 = GetDirectionByVector( rect.v4 - point );

	const WORD diff1 = DirsDifference( wAngle2, wAngle1 );
	const WORD diff2 = DirsDifference( wAngle3, wAngle1 );
	const WORD diff3 = DirsDifference( wAngle4, wAngle1 );
	const WORD diff4 = DirsDifference( wAngle3, wAngle2 );
	const WORD diff5 = DirsDifference( wAngle4, wAngle2 );
	const WORD diff6 = DirsDifference( wAngle4, wAngle3 );

	return (std::max)( (std::max) ( (std::max)( diff1, diff2 ), (std::max)( diff3, diff4 ) ), (std::max)( diff5, diff6 ) );
}


