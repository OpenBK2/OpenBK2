#pragma once
// universal bilinear interpolation

template<class T, class TInterp> 
inline typename TInterp::RET GetBilinear( const CArray2D<T> &data, float x, float y, const TInterp &interp )
{
	x = Clamp( x, 0.0f, data.GetXSize() - 1.01f );
	y = Clamp( y, 0.0f, data.GetYSize() - 1.01f );
	int nX = x, nY = y;
	T a = interp( data[nY][nX], data[nY][nX+1], x - nX );
	T b = interp( data[nY+1][nX], data[nY+1][nX+1], x - nX );
	return interp( a, b, y - nY );
}

struct TLinearInterpolate
{
	typedef float RET;
	template<class T>
		float operator()( T a, T b, float f ) const { return (1-f) * a + f * b; }
};

struct THermitInterpolate
{
	template <class TValue>
		TValue operator() ( float fCoeff, TValue p0, TValue p1, TValue v0, TValue v1 )
		{
			if ( fCoeff <= 0 )
				return p0;
			else if ( fCoeff >= 1 )
				return p1;
			else
				return ( (2*p0 - 2*p1 + v0 + v1)*fCoeff*fCoeff*fCoeff + (-3*p0 + 3*p1 -2*v0 - v1)*fCoeff*fCoeff + v0*fCoeff + p0 );
		}
};


