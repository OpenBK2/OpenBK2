#include "StdAfx.h"

#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "..\stats_b2_m1\iconsset.h"
#include "MissionWarFog.h"
//#include "..\SceneB2\PerlinNoise.h"

static int CLOUDS_NOISE_OCTS_NUM = 2;
static float CLOUDS_NOISE_PERS = 1.0f;
static float CLOUDS_NOISE_SCALE = 0.04f;
static float CLOUDS_NOISE_POWER = 1.0f;

static int CLOUDS_NOISE_KEYS_TIME = 10;
static float CLOUDS_NOISE_SHARPNESS = 0.5f;
static float CLOUDS_NOISE_DENSITY = 0.5f;

static float CLOUDS_NOISE_SPEED_X = 2.0f;
static float CLOUDS_NOISE_SPEED_Y = 1.0f;
/*

SInterfaceMissionWarFogInfo::SInterfaceMissionWarFogInfo()
{
	bIsWarFogOn = false;
	bIsNoiseOn = false;
	baseNoiseTime1 = baseNoiseTime2 = 0;
	vNoisePos = VNULL2;
	noiseTime = 0;

	int nDensity = Clamp( (int)(CLOUDS_NOISE_DENSITY * 255.0f), 0, 255 );
	float fSharpness = CLOUDS_NOISE_SHARPNESS;
	for ( int i = 0; i < 256; ++i )
	{
		int c = i - nDensity;
		if ( c < 0 )
			c = 0;
		cloudDensities[i] = 1.0f - pow( fSharpness, c );
	}
}

void SInterfaceMissionWarFogInfo::Reset( const int _nSizeX, const int _nSizeY, const float _fScale )
{
	nSizeX = _nSizeX;
	nSizeY = _nSizeY;
	fScale = _fScale;
	bIsWarFog = false;
	bIsNoise = false;
	bIsNoiseOn = NGlobal::GetVar( "clouds_shadow", 0 ) != 0;
}

void SInterfaceMissionWarFogInfo::SetWarFog( const CArray2D<BYTE> &warFog, float _fScale, NTimer::STime nGameTime )
{
	if ( !bIsWarFog )
	{
		bIsWarFog = true;
		warFogTime1 = nGameTime;
		warFogTime2 = nGameTime + Singleton<IAILogic>()->WAR_FOG_FULL_UPDATE();
		warFog1.SetSizes( nSizeX, nSizeY );
		warFog2.SetSizes( nSizeX, nSizeY );
		for ( int x = 0; x < nSizeX; ++x )
		{
			for ( int y = 0; y < nSizeY; ++y )
			{
				float f = (float)warFog[y][x] / 255.0f;
				warFog1[y][x] = f;
				warFog2[y][x] = f;
			}
		}
	}
	else
	{
		warFogTime1 = nGameTime;
		warFogTime2 = nGameTime + Singleton<IAILogic>()->WAR_FOG_FULL_UPDATE();
		warFog1 = warFog2;
		for ( int x = 0; x < nSizeX; ++x )
		{
			for ( int y = 0; y < nSizeY; ++y )
			{
				float f = (float)warFog[y][x] / 255.0f;
				warFog2[y][x] = f;
			}
		}
	}
}

void SInterfaceMissionWarFogInfo::Update( NTimer::STime nGameTime )
{
	if ( !bIsWarFog && !bIsNoise )
		return;

	// сгенерим новый фог
	CArray2D<float> fog;
	fog.SetSizes( nSizeX, nSizeY );
	for ( int x = 0; x < nSizeX; ++x )
	{
		for ( int y = 0; y < nSizeY; ++y )
		{
			fog[y][x] = 1.0f;
		}
	}

	if ( bIsWarFogOn )
	{
		CArray2D<float> warFog;
		GetWarFog( &warFog, nGameTime );
		for ( int x = 0; x < nSizeX; ++x )
		{
			for ( int y = 0; y < nSizeY; ++y )
			{
				fog[y][x] *= warFog[y][x];
			}
		}
	}

	if ( bIsNoiseOn )
	{
		CArray2D<float> noise;
		CArray2D<float> noise2;
		GetNoise( &noise, nGameTime );

		float deltaTime = (float)(nGameTime - noiseTime) / 1000.f;
		noiseTime = nGameTime;
		vNoisePos.x += CLOUDS_NOISE_SPEED_X * deltaTime;
		vNoisePos.y += CLOUDS_NOISE_SPEED_Y * deltaTime;

		CycleMove( &noise2, noise, vNoisePos );
		for ( int x = 0; x < nSizeX; ++x )
		{
			for ( int y = 0; y < nSizeY; ++y )
			{
				fog[y][x] *= (1.0f - CLOUDS_NOISE_POWER) + CLOUDS_NOISE_POWER * noise2[y][x];
			}
		}
	}

	SceneSetFarFog( fog );
}

inline void Convert2BYTE( CArray2D<BYTE> *pDst, const CArray2D<float> &src )
{
	pDst->SetSizes( src.GetSizeX(), src.GetSizeY() );
	for ( int g = 0; g < src.GetSizeY(); ++g )
	{
		for ( int i = 0; i < src.GetSizeX(); ++i )
		{
			(*pDst)[g][i] = Clamp( int( src[g][i] * 255.0f ), 0, 255 );
		}
	}
}

void SInterfaceMissionWarFogInfo::GetNoise( CArray2D<float> *pNoise, NTimer::STime time )
{
	if ( !bIsNoise || time > baseNoiseTime2 + CLOUDS_NOISE_KEYS_TIME * 1000 )
	{
		bIsNoise = true;
		noiseTime = time;
		// сгенерим произвольные опорные шумы
		CPerlinNoise perlin;
		perlin.CreateTiled( nSizeX, nSizeY, CLOUDS_NOISE_OCTS_NUM, CLOUDS_NOISE_PERS, CLOUDS_NOISE_SCALE );
		baseNoise1 = perlin.GetPerlinNoise();
		perlin.CreateTiled( nSizeX, nSizeY, CLOUDS_NOISE_OCTS_NUM, CLOUDS_NOISE_PERS, CLOUDS_NOISE_SCALE );
		baseNoise2 = perlin.GetPerlinNoise();



		baseNoiseTime1 = time;
		baseNoiseTime2 = baseNoiseTime1 + CLOUDS_NOISE_KEYS_TIME * 1000;
	}
	else if ( time >= baseNoiseTime2 )
	{
		NI_ASSERT( time >= baseNoiseTime1, "invalid time for noise" );
		baseNoiseTime1 = time;
		baseNoise1 = baseNoise2;

		CPerlinNoise perlin;
		perlin.CreateTiled( nSizeX, nSizeY, CLOUDS_NOISE_OCTS_NUM, CLOUDS_NOISE_PERS, CLOUDS_NOISE_SCALE );
		baseNoise2 = perlin.GetPerlinNoise();

		baseNoiseTime2 = time + CLOUDS_NOISE_KEYS_TIME * 1000;
	}
	else
	{
		NI_ASSERT( time >= baseNoiseTime1, "invalid time for noise" );
	}
	float fDelta = Clamp( (float)(time - baseNoiseTime1) / (float)(baseNoiseTime2 - baseNoiseTime1), 0.0f, 1.0f );
	CArray2D<float> noise;
	Blend( &noise, baseNoise1, baseNoise2, fDelta );
	ApplyCloudDensity( &noise );
	Blur( pNoise, noise );
}

void SInterfaceMissionWarFogInfo::Blend( CArray2D<float> *pDst, 
																				const CArray2D<float> &src1, const CArray2D<float> &src2, float fDelta )
{
	pDst->SetSizes( nSizeX, nSizeY );
	for ( int x = 0; x < nSizeX; ++x )
	{
		for ( int y = 0; y < nSizeY; ++y )
		{
			(*pDst)[y][x] = src1[y][x] * (1.0f - fDelta) + src2[y][x] * fDelta;
		}
	}
}

void SInterfaceMissionWarFogInfo::CycleMove( CArray2D<float> *pDst, const CArray2D<float> &src, const CVec2 &_vPos )
{
	CVec2 vPos = _vPos;
	if ( vPos.x >= 0.0f )
		vPos.x = fmod( vPos.x, nSizeX );
	else
		vPos.x = nSizeX + fmod( vPos.x, nSizeX );
	if ( vPos.y >= 0.0f )
		vPos.y = fmod( vPos.y, nSizeY );
	else
		vPos.y = nSizeY + fmod( vPos.y, nSizeY );

	// считаем площадь сегментов сдвинутой клетки
	const float fDX = vPos.x - floorf( vPos.x );
	const float fDY = vPos.y - floorf( vPos.y );
	const float f11 = (1.0f - fDX) * (1.0f - fDY);
	const float f12 = fDX * (1.0f - fDY);
	const float f21 = (1.0f - fDX) * fDY;
	const float f22 = fDX * fDY;

	const int nStartX = (int)floorf( vPos.x );
	const int nStartY = (int)floorf( vPos.y );

	pDst->SetSizes( nSizeX, nSizeY );
	pDst->FillZero();
	int dx = nStartX;
	for ( int x = 0; x < nSizeX; ++x, ++dx )
	{
		int dy = nStartY;
		for ( int y = 0; y < nSizeY; ++y, ++dy )
		{
			if ( dx >= nSizeX )
				dx -= nSizeX;
			if ( dy >= nSizeY )
				dy -= nSizeY;
			int dx2 = dx + 1;
			int dy2 = dy + 1;
			if ( dx2 >= nSizeX )
				dx2 -= nSizeX;
			if ( dy2 >= nSizeY )
				dy2 -= nSizeY;
			(*pDst)[dy][dx] += src[y][x] * f11;
			(*pDst)[dy][dx2] += src[y][x] * f12;
			(*pDst)[dy2][dx] += src[y][x] * f21;
			(*pDst)[dy2][dx2] += src[y][x] * f22;
		}
	}
}

void SInterfaceMissionWarFogInfo::SceneSetFarFog( const CArray2D<float> &src )
{
	CArray2D<BYTE> sceneWarFog;
	Convert2BYTE( &sceneWarFog, src );
	Scene()->SetWarFog( sceneWarFog, fScale );
	//	Scene()->SetWarFogBlend( 1.0f );
}

void SInterfaceMissionWarFogInfo::ApplyCloudDensity( CArray2D<float> *pNoise )
{
	if ( CLOUDS_NOISE_SHARPNESS == 0.0f )
		return;
	for ( int x = 0; x < nSizeX; ++x )
	{
		for ( int y = 0; y < nSizeY; ++y )
		{
			BYTE b = (BYTE)((*pNoise)[y][x] * 255.0f);
			(*pNoise)[y][x] = cloudDensities[b];
		}
	}
}

void SInterfaceMissionWarFogInfo::Blur( CArray2D<float> *pDst, const CArray2D<float> &src )
{
	const int n = 1;
	pDst->SetSizes( src.GetSizeX(), src.GetSizeY() );
	for ( int y = 0; y < src.GetSizeY(); ++y )
	{
		int y1 = Max( 0, y - n );
		int y2 = Min( y + n, src.GetSizeY() );
		for ( int x = 0; x < src.GetSizeX(); ++x )
		{
			int x1 = Max( 0, x - n );
			int x2 = Min( x + n, src.GetSizeX() );
			float value = 0.0f;
			float sum = 0.0f;
			for ( int yy = y1; yy < y2; ++yy )
			{
				for ( int xx = x1; xx < x2; ++xx )
				{
					float coeff = 1.0f / (float)((abs( xx - x ) + 1) * (abs( yy - y ) + 1));
					value += src[yy][xx] * coeff;
					sum += coeff;
				}
			}
			(*pDst)[y][x] = value / sum;
		}
	}
}

START_REGISTER(MissionWarFogVars)
REGISTER_VAR_EX( "clouds_shadow_octs_num", NGlobal::VarIntHandler, &CLOUDS_NOISE_OCTS_NUM, 2, STORAGE_NONE );
REGISTER_VAR_EX( "clouds_shadow_pers", NGlobal::VarFloatHandler, &CLOUDS_NOISE_PERS, 1.0f, STORAGE_NONE );
REGISTER_VAR_EX( "clouds_shadow_scale", NGlobal::VarFloatHandler, &CLOUDS_NOISE_SCALE, 0.04f, STORAGE_NONE );
REGISTER_VAR_EX( "clouds_shadow_power", NGlobal::VarFloatHandler, &CLOUDS_NOISE_POWER, 1.0f, STORAGE_NONE );
REGISTER_VAR_EX( "clouds_shadow_keys_time", NGlobal::VarIntHandler, &CLOUDS_NOISE_KEYS_TIME, 10, STORAGE_NONE );
REGISTER_VAR_EX( "clouds_shadow_sharpness", NGlobal::VarFloatHandler, &CLOUDS_NOISE_SHARPNESS, 0.5f, STORAGE_NONE );
REGISTER_VAR_EX( "clouds_shadow_density", NGlobal::VarFloatHandler, &CLOUDS_NOISE_DENSITY, 0.5f, STORAGE_NONE );
REGISTER_VAR_EX( "clouds_shadow_speed_x", NGlobal::VarFloatHandler, &CLOUDS_NOISE_SPEED_X, 2.0f, STORAGE_NONE );
REGISTER_VAR_EX( "clouds_shadow_speed_y", NGlobal::VarFloatHandler, &CLOUDS_NOISE_SPEED_Y, 1.0f, STORAGE_NONE );
FINISH_REGISTER
*/

