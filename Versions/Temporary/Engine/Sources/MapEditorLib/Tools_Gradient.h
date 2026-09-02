#pragma once

#include <cstdint>

struct SGradient
{
private:
	mutable float a;
	mutable float b;
	mutable float c;
	mutable int nPreviousIndex;

public:
	typedef std::vector<float> CHeightList;
	//
	CHeightList heightList;
	CTPoint<float> range;
	CTPoint<float> heightRange;

	SGradient() : a( 0.0f ), b( 0.0f ), c( 0.0f ), nPreviousIndex( -1 ), range( 0.0f, 0.0f ), heightRange( 0.0f, 0.0f ) {}
	SGradient( const CHeightList &rHeightList,	const CTPoint<float> &rRange, const CTPoint<float> &rHeightRange )
		: a( 0.0f ), b( 0.0f ), c( 0.0f ), nPreviousIndex( -1 ), heightList( rHeightList ), range( rRange ), heightRange( rHeightRange ) {}
	SGradient( const SGradient &rGradient )
		: a( 0.0f ), b( 0.0f ), c( 0.0f ), nPreviousIndex( -1 ), heightList( rGradient.heightList ), range( rGradient.range ), heightRange( rGradient.heightRange ) {}
	SGradient& operator=( const SGradient &rGradient )
	{
		if( &rGradient != this )
		{
			a = 0.0f;
			b = 0.0f;
			c = 0.0f;
			nPreviousIndex = ( -1 );
			heightList = rGradient.heightList;
			range = rGradient.range;
			heightRange = rGradient.heightRange;
		}
		return *this;
	}

	void UpdateHeightRanges();
	bool CreateFromImage( const CArray2D<uint32_t> &rImage, const CTPoint<float> &rRange, const CTPoint<float> &rHeightRange );
	bool Get( float *pfValue, float fPosition, bool bSquareInterpolated ) const;
	inline float operator()( float fPosition, bool bSquareInterpolated = false ) const { float fValue = 0.0f; Get( &fValue, fPosition, bSquareInterpolated ); return fValue; }
};


