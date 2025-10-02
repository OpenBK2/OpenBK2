#include "StdAfx.h"

#include "Misc/Win32Random.h"
#include "PerlinNoise.h"

#define DEF_NOISE_EPS 0.00001f

#define B 0x100
#define BM 0xff

#define NP 12
#define N (2 << NP)

static int p[B + B + 2];
static float g3[B + B + 2][3];
static float g2[B + B + 2][2];
static float g1[B + B + 2];
static int nStart = 1;

static void InitNoiseParameters( void );

#define s_curve(t) ( t * t * (3.0f - 2.0f * t) )

#define lerp(t, a, b) ( a + t * (b - a) )

#define setup(i, b0, b1, r0, r1)\
	t = vec[i] + N;\
	b0 = ((int)t) & BM;\
	b1 = ( b0 + 1 ) & BM;\
	r0 = t - (int)t;\
	r1 = r0 - 1.;

inline static float GetNoise1D( float fArg )
{
	int bx0, bx1;
	float rx0, rx1, sx, t, u, v, vec[1];

	vec[0] = fArg;
	if ( nStart )
	{
		nStart = 0;
		InitNoiseParameters();
	}

	setup( 0, bx0, bx1, rx0, rx1 );

	sx = s_curve(rx0);

	u = rx0 * g1[p[bx0]];
	v = rx1 * g1[p[bx1]];

	return lerp( sx, u, v );
}

inline static float GetNoise2D( float fVecX, float fVecY )
{
	int bx0, bx1, by0, by1, b00, b10, b01, b11;
	float rx0, rx1, ry0, ry1, *q, sx, sy, a, b, t, u, v;
	float vec[2] = { fVecX, fVecY };
	int i, j;

	if ( nStart )
	{
		nStart = 0;
		InitNoiseParameters();
	}

	setup( 0, bx0,bx1, rx0,rx1 );
	setup( 1, by0,by1, ry0,ry1 );

	i = p[bx0];
	j = p[bx1];

	b00 = p[i + by0];
	b10 = p[j + by0];
	b01 = p[i + by1];
	b11 = p[j + by1];

	sx = s_curve(rx0);
	sy = s_curve(ry0);

#define at2( rx, ry ) ( rx * q[0] + ry * q[1] )

	q = g2[b00]; u = at2( rx0, ry0 );
	q = g2[b10]; v = at2( rx1, ry0 );
	a = lerp( sx, u, v );

	q = g2[b01]; u = at2( rx0, ry1 );
	q = g2[b11]; v = at2( rx1, ry1 );
	b = lerp( sx, u, v );

	return lerp( sy, a, b );
}

inline static void Normalize2( float v[2] )
{
	float s;

	s = sqrt( v[0] * v[0] + v[1] * v[1] );
	v[0] = v[0] / s;
	v[1] = v[1] / s;
}

static void InitNoiseParameters(void)
{
	int i, j, k;

	for (i = 0 ; i < B ; i++) {
		p[i] = i;

		g1[i] = (float)((rand() % (B + B)) - B) / B;

		for (j = 0 ; j < 2 ; j++)
			g2[i][j] = (float)((rand() % (B + B)) - B) / B;
		Normalize2(g2[i]);
	}

	while (--i) {
		k = p[i];
		p[i] = p[j = rand() % B];
		p[j] = k;
	}

	for (i = 0 ; i < B + 2 ; i++) {
		p[B + i] = p[i];
		g1[B + i] = g1[i];
		for (j = 0 ; j < 2 ; j++)
			g2[B + i][j] = g2[i][j];
		for (j = 0 ; j < 3 ; j++)
			g3[B + i][j] = g3[i][j];
	}
}

int CPerlinNoise::Create( const int nSizeX, const int nSizeY, const long nOctsNum, const float fPers, const float fScale )
{
	NI_ASSERT( ( nSizeX > 0 ) && ( nSizeY > 0 ) && ( nOctsNum > 0 ) && ( fScale > 0 ), "Wrong parameters" );

	if ( ( noise.GetSizeX() != nSizeX ) || ( noise.GetSizeY() != nSizeY ) )
		noise.SetSizes( nSizeX, nSizeY );

	float fXCoor, fYCoor, fFreq;
	float fMin = 1000000.0f, fMax = -1000000.0f;

	float fTexOffsX = NWin32Random::Random( 0.1f, 10000.0f );
	float fTexOffsY = NWin32Random::Random( 0.1f, 10000.0f );
	if ( ( fTexOffsX - int(fTexOffsX) ) < DEF_NOISE_EPS )
		fTexOffsX += 0.1f;
	if ( ( fTexOffsY - int(fTexOffsY) ) < DEF_NOISE_EPS )
		fTexOffsY += 0.1f;

	// create noise
	for ( int g = 0; g < noise.GetSizeY(); ++g )
	{
		for ( int i = 0; i < noise.GetSizeX(); ++i )
		{
			noise[g][i] = 0.0f;
			fXCoor = (float)i * fScale + fTexOffsX;
			fYCoor = (float)g * fScale + fTexOffsY;
			fFreq = 1.0f;

			for ( int k = 0; k < nOctsNum; ++k )
			{
				noise[g][i] += GetNoise2D( fXCoor, fYCoor ) * fFreq;
				fFreq *= fPers;
				fXCoor *= 2.0f;
				fYCoor *= 2.0f;
			}

			if ( noise[g][i] < fMin ) fMin = noise[g][i];
			if ( noise[g][i] > fMax ) fMax = noise[g][i];
		}
	}

	// normalize noise
	const float fInvDisp = fabs(fMax - fMin ) > DEF_NOISE_EPS ? ( 1.0f / ( fMax - fMin ) ) : 1.0f;
	for ( int g = 0; g < noise.GetSizeY(); ++g )
		for ( int i = 0; i < noise.GetSizeX(); ++i )
			noise[g][i] = ( noise[g][i] - fMin ) * fInvDisp;

	return true;
}

inline float CPerlinNoise::GetFullNoise( const int x, const int y, const int nOctsNum, const float fScale, const float fPers,
																				 const float fTexOffsX, const float fTexOffsY )
{
	float fRes = 0.0f;
	float fCurScale = fScale;
	float fFreq = 1.0f;

	for ( int k = 0; k < nOctsNum; ++k )
	{
		const float px = (float)x * fCurScale + fTexOffsX;
		const float py = (float)y * fCurScale + fTexOffsY;
		fRes += GetNoise2D( px, py );
		fFreq *= fPers;
		fCurScale *= 2.0f;
	}

	return fRes;
}

int CPerlinNoise::CreateTiled( const int nSizeX, const int nSizeY, const long nOctsNum, const float fPers, const float fScale )
{
	NI_ASSERT( ( nSizeX > 0 ) && ( nSizeY > 0 ) && ( nOctsNum > 0 ) && ( fScale > 0 ), "Wrong parameters" );

	if (( noise.GetSizeX() != nSizeX ) || ( noise.GetSizeY() != nSizeY ))
		noise.SetSizes( nSizeX, nSizeY );

	//float fFreq;
	float fMin = 1000000.0f, fMax = -1000000.0f;

	float fTexOffsX = NWin32Random::Random( 0.1f, 10000.0f );
	float fTexOffsY = NWin32Random::Random( 0.1f, 10000.0f );
	if ( ( fTexOffsX - int(fTexOffsX) ) < DEF_NOISE_EPS )
		fTexOffsX += 0.1f;
	if ( ( fTexOffsY - int(fTexOffsY) ) < DEF_NOISE_EPS )
		fTexOffsY += 0.1f;

	const float fInvSizesMult = 1.0f / ( noise.GetSizeX() * noise.GetSizeY() );

	//float fCurScale;

	// create noise
	for ( int g = 0; g < noise.GetSizeY(); ++g )
	{
		for ( int i = 0; i < noise.GetSizeX(); ++i )
		{
			noise[g][i] = (
				GetFullNoise( i, g, nOctsNum, fScale, fPers, fTexOffsX, fTexOffsY ) * ( noise.GetSizeX() - 1 - i ) * ( noise.GetSizeY() - 1 - g ) +
				GetFullNoise( i - noise.GetSizeX() + 1, g, nOctsNum, fScale, fPers, fTexOffsX, fTexOffsY ) * i * ( noise.GetSizeY() - 1 - g ) +
				GetFullNoise( i, g - noise.GetSizeY() + 1, nOctsNum, fScale, fPers, fTexOffsX, fTexOffsY ) * ( noise.GetSizeX() - 1 - i ) * g +
				GetFullNoise( i - noise.GetSizeX() + 1, g - noise.GetSizeY() + 1, nOctsNum, fScale, fPers, fTexOffsX, fTexOffsY ) * i * g ) * fInvSizesMult;

			/*noise[g][i] = 0.0f;
			fCurScale = fScale;
			fFreq = 1.0f;

			const float x = (float)i * fCurScale + fTexOffsX;
			const float y = (float)g * fCurScale + fTexOffsY;

			for ( int k = 0; k < nOctsNum; ++k )
			{
				noise[g][i] += (
					GetNoise2D( x, y ) * ( noise.GetSizeX() - 1 - i ) * ( noise.GetSizeY() - 1 - g ) +
					GetNoise2D( x - ( noise.GetSizeX() - 1 ) * fCurScale, y ) * i * ( noise.GetSizeY() - 1 - g ) +
					GetNoise2D( x - ( noise.GetSizeX() - 1 ) * fCurScale, y - ( noise.GetSizeY() - 1 ) * fCurScale ) * i * g +
					GetNoise2D( x, y - ( noise.GetSizeY() - 1 ) * fCurScale ) * ( noise.GetSizeX() - 1 - i ) * g ) *
					fInvSizesMult * fFreq;
				fFreq *= fPers;
				fCurScale *= 2.0f;
			}*/

			if ( noise[g][i] < fMin ) fMin = noise[g][i];
			if ( noise[g][i] > fMax ) fMax = noise[g][i];
		}
	}

	// normalize noise
	const float fInvDisp = fabs(fMax - fMin ) > DEF_NOISE_EPS ? ( 1.0f / ( fMax - fMin ) ) : 1.0f;
	for ( int g = 0; g < noise.GetSizeY(); ++g )
		for ( int i = 0; i < noise.GetSizeX(); ++i )
			noise[g][i] = ( noise[g][i] - fMin ) * fInvDisp;

	return true;
}


