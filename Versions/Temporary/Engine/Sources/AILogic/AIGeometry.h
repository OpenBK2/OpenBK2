#pragma once

//*******************************************************************
//*								  Geometry for AI : vectors, lines								*
//*******************************************************************

const CVec3 V3_CAMERA_HOR = CVec3( -FP_SQRT_2/2.0f, FP_SQRT_2/2.0f, 0.0f ); //V3_AXIS_Y - V3_AXIS_X

// только с достаточно близкими точками
inline long SquareOfDistance( const SVector &v1, const SVector &v2 )
{
	return square(long(v1.x-v2.x))+square(long(v1.y-v2.y));
}

inline float SquareOfDistance( const CVec2 &v1, const CVec2 &v2 )
{
	return square(v1.x-v2.x) + square(v1.y-v2.y);
}

inline CVec2 Project( const CVec2 &vec, const CVec2 &axis )
{
	if ( axis == VNULL2 )
		return VNULL2;
	else
		return axis*( (vec*axis) / fabs2(axis) );
}

inline const CVec2 Proj2D( const CVec3 &v3D )
{
	return CVec2( v3D.x, v3D.y );
}

inline float DoubleTrSquare( const CVec2 &side1, const CVec2 &side2 )
{
	return side1.x*side2.y - side2.x*side1.y;
}

//*******************************************************************
//*										  SLine																				*
//*******************************************************************

struct SLine
{
	int a, b, c;

	SLine() {  };
	SLine( int _a, int _b, int _c ) : a( _a ), b( _b ), c( _c ) {  }
	SLine( const SVector &ptStart, const SVector &ptFinish ) 
		: a( ptFinish.y - ptStart.y ), b( ptStart.x - ptFinish.x ), c( ptFinish.x*ptStart.y - ptStart.x*ptFinish.y ) {  }

	bool IsPointOnLine( const SVector &point )	const { return a*point.x + b*point.y + c == 0; }
	const int GetHPLineSign( const SVector &point ) const { return boost::math::sign( a*point.x + b*point.y + c ); }
	bool IsSegmIntersectLine( const SVector &ptStart, const SVector &ptFinish) const
	{
		const int t1 = boost::math::sign( GetHPLineSign( ptStart ) );
		const int t2 = boost::math::sign( GetHPLineSign( ptFinish ) );
		return (t1 >= 0) && (t2 <= 0) || (t1 <= 0) && (t2 >= 0);
	}
	const SLine GetPerpendicular( const SVector &point ) const { return SLine( -b, a, b*point.x - a*point.y ); }

	const SVector GetDirVector() const { return SVector( -b, a ); }
};

//*******************************************************************
//*															CBresZ															*
//*******************************************************************

class CBresZ
{
	int xerr, yerr;
	int xlen, ylen, len;
	int xinc, yinc, zinc, rinc;

	int intPart, dxy;
	
	SVector dir;
	int z, r;

public:
	void InitXY( const SVector &start, const SVector &finish )
	{ 
		dir = start; 
		
		xlen = finish.x - start.x;
		ylen = finish.y - start.y;

		xerr = 0;
		yerr = 0;
		
		xinc = boost::math::sign( xlen );
		yinc = boost::math::sign( ylen );

		xlen = abs( xlen ) + 1;
		ylen = abs( ylen ) + 1;
		len = (std::max)( xlen, ylen );

		NI_ASSERT( xlen != 0 || ylen != 0, "Wrong line" );
	}

	void InitZ( const int startZ, const int finishZ, const SVector &startPoint, const SVector &finishPoint )
	{
		NI_ASSERT( startPoint != finishPoint, "Wrong ray" );
		
		const int zlen = finishZ-startZ;
		dxy = (std::max)( abs( finishPoint.x-startPoint.x ), abs( finishPoint.y-startPoint.y ) );

		intPart = zlen / dxy;
		rinc = abs( zlen % dxy );
 		zinc = boost::math::sign( zlen );

		z = finishZ; 
		r = 0;
	}

	void InitZWithStep( const int startZ, const int finishZ, const SVector &startPoint, const SVector &finishPoint )
	{
		NI_ASSERT( startPoint != finishPoint, "Wrong ray" );
		
		const int zlen = finishZ-startZ;
		dxy = (std::max)( abs( finishPoint.x-startPoint.x ), abs( finishPoint.y-startPoint.y ) );
		
		intPart = zlen / dxy;
		rinc = abs( zlen % dxy );
		zinc = boost::math::sign( zlen );

		z = finishZ+intPart;
		r = rinc;
	}

	void MakeStep()
	{
		xerr += xlen;
		yerr += ylen;

		if ( xerr >= len )
			dir.x += xinc, xerr -= len;
		if ( yerr >= len )
			dir.y += yinc, yerr -= len;

		z += intPart;
		r += rinc;
		if ( r >= dxy )
			z += zinc, r -= dxy;
	}
	
	const SVector& GetPoint() const { return dir; }
	const int GetZ() const { return z; }
};

// минимальное расстояние от точки до отрезка
const float GetDistanceToSegment( const CVec2 &vSegmentStart, const CVec2 &vSegmentEnd, const CVec2 &vPoint );

void MakeQuatBySpeedAndNormale( CQuat *pQuat, const CVec3 &vSpeed, const CVec3 &vNormale );

const CVec2 MoveVectorByDirection( const CVec2 &vPoint, WORD wDir );


