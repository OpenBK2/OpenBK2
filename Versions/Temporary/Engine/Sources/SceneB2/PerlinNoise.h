#pragma once

#include "Misc/2DArray.h"

class CPerlinNoise
{
	CArray2D<float> noise;

	inline float GetFullNoise( const int x, const int y, const int nOctsNum, const float fScale, const float fPers,
		const float fTexOffsX, const float fTexOffsY );
public:
	// function creates 2D perlin noise
	int Create( const int nSizeX, const int nSizeY, const long nOctsNum, const float fPers, const float fScale );
	// function creates tiled 2D perlin noise
	int CreateTiled( const int nSizeX, const int nSizeY, const long nOctsNum, const float fPers, const float fScale );

	// function returns perlin noise array
	const CArray2D<float> &GetPerlinNoise() const { return noise; }
	// function returns size X of noise array
	unsigned int GetSizeX() const { return noise.GetSizeX(); }
	// function returns size Y of noise array
	unsigned int GetSizeY() const { return noise.GetSizeY(); }
};


