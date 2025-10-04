#include "stdafx.h"
#include "GLMLightState.h"
#include "RandomGen.h"
#include "GScene.h"
#include "GLightPerVertex.h"

// radius / F_POINT_LIGHT_FALLOFF - nominal brightness distance
const float F_POINT_LIGHT_FALLOFF = 8;
const float F_MIN_COLOR_SQUARED = sqr( sqr( 0.02f ) );

namespace NGScene
{

// CLightState

static float Halton( int b, int i )
{
	float x = 0, fBInv = 1.0f / b, f = fBInv;
	while ( i )
	{
		x += f * ( i % b );
		i /= b;
		f *= fBInv;
	}
	return x;
}

static void GenerateRandomSphereVector( CVec3 *pRes )
{
	for(;;)
	{
		CVec3 v( random.GetFloat(-1,1), random.GetFloat(-1,1), random.GetFloat(-1,1) );
		float f = fabs2( v );
		if ( f == 0 || f > 1 )
			continue;
		*pRes = v / sqrt( f );
		return;
	}
}

const float F_POINT_RADIUS = 16;
const float F_POINT_STRENGTH = 0.5f * 4 * 3.14f * F_POINT_RADIUS * F_POINT_RADIUS / F_POINT_LIGHT_FALLOFF / F_POINT_LIGHT_FALLOFF;
void CLightState::AddRay( const CVec3 &vFrom, const CVec3 &vDir, const CVec3 &_vColor )
{
	if ( !IsValid(pVis) )
		return;
	if ( random.GetFloat( 0, 1 ) < 0.3f ) // absorbtion
		return;
	CRay r;
	r.ptOrigin = vFrom;
	r.ptDir = vDir;
	r.ptDir *= 1000;
	CVec3 vPoint, vNormal, vReflectColor(1,1,1);
	float fT;
	if ( !pVis->TraceScene( MakeSelectAll(), r, &fT, &vNormal, SPS_STATIC ) ) // &vReflectColor
		return;
	vPoint = r.Get( fT );// + vNormal * 0.01f;
	float fRadius = F_POINT_RADIUS;
	//vReflectColor *= 2;
	ASSERT( vReflectColor.x <= 1 );
	ASSERT( vReflectColor.y <= 1 );
	ASSERT( vReflectColor.z <= 1 );
	//vReflectColor.Minimize( CVec3(1,1,1) );
	CVec3 vRefColor( _vColor.r * vReflectColor.r, _vColor.g * vReflectColor.g, _vColor.b * vReflectColor.b );
	CVec3 vColor(vRefColor);
	if ( fabs2(vColor) == 0 )
		return;
	while ( fabs2(vColor) < F_MIN_COLOR_SQUARED )
	{
		vColor *= 2;
		fRadius /= 1.41f;
	}
	if ( fRadius < 1 )
		return;
	semiPoints.push_back( SSemiPointLight( vColor, vPoint, vNormal, fRadius ) );
	CVec3 vReflect;
	GenerateRandomSphereVector( &vReflect );
	if ( vReflect * vNormal < 0 )
		vReflect -= ( 2 * ( vReflect * vNormal ) ) * vNormal;
	AddRay( vPoint + vReflect * 0.01f, vReflect, vRefColor * 0.9f );
}

void CLightState::AddParallel( const SSphere &_bound, const CVec3 &vDir, const CVec3 &_vColor )
{
	CVec3 vColor = _vColor;
	if ( fabs2( vColor ) < 1e-16f )
		return;
	//if ( bDoRender )
	//	parallel.push_back( SParallelLight( vColor, vDir ) );
	if ( !pVis )
		return;
	CVec3 vCenter = _bound.ptCenter;
	float fWidth = _bound.fRadius * 2;
	float fTest = random.GetFloat( 0, F_POINT_STRENGTH );
	float fStrength = fWidth * fWidth;
	while ( fabs2( vColor ) > F_MIN_COLOR_SQUARED )
	{
		vColor = vColor * 0.5f;
		fStrength = fStrength * 2;
	}
	while ( fabs2( vColor ) < F_MIN_COLOR_SQUARED / 2.5 )
	{
		vColor = vColor * 2;
		fStrength = fStrength * 0.5f;
	}
	CVec3 vRight = CVec3(0,0,1) ^ vDir;
	if ( fabs2( vRight ) < 0.001f )
		vRight = CVec3(0,1,0) ^ vDir;
	Normalize( &vRight );
	CVec3 vUp = vRight ^ vDir;
	static int nFake = 0;
	for ( ; fTest < fStrength; fTest += F_POINT_STRENGTH )
	{
		++nFake;
		CVec3 vShifted = vCenter +
			vRight * ( Halton( 5, nFake ) * fWidth - fWidth / 2 ) +
			vUp    * ( Halton( 7, nFake ) * fWidth - fWidth / 2 );
		vShifted -= vDir * 100;
		AddRay( vShifted, vDir, vColor );
	}
}

void CLightState::AddPoint( const CVec3 &vCenter, float fRadius, const CVec3 &_vColor )
{
	//if ( bDoRender )
	points.push_back( SPointLight( _vColor, vCenter, fRadius ) );
	if ( !pVis )
		return;
	float fTest = random.GetFloat( 0, F_POINT_STRENGTH );
	float fStrength = sqr( fRadius ) / sqr( F_POINT_RADIUS ) * F_POINT_STRENGTH;
	CVec3 vColor(_vColor);
	while ( fabs2( vColor ) > F_MIN_COLOR_SQUARED )
	{
		vColor = vColor * 0.5f;
		fStrength = fStrength * 2;
	}
	while ( fabs2( vColor ) < F_MIN_COLOR_SQUARED / 2.5 )
	{
		vColor = vColor * 2;
		fStrength = fStrength * 0.5f;
	}
	for ( ; fTest < fStrength; fTest += F_POINT_STRENGTH )
	{
		CVec3 vDir;
		GenerateRandomSphereVector( &vDir );
		AddRay( vCenter, vDir, vColor );
	}
}

CVec3 CLightState::GenerateSkyDir( SLightStateCalcSeed *pSeed, int nSkyDirs )
{
	CVec3 vSky;
	int &nSeed = pSeed->nSeed;
	int nDirBeta = nSkyDirs;
	for (;;)
	{
		CVec3 vAmbDir;
		float fTeta = 1 - Halton( 2, nSeed ) * 0.7f; // never less then 0.3f;//
		fTeta = acos( fTeta );
		float fOmega = Halton( nDirBeta, nSeed ) * FP_2PI;
		vSky = CVec3( cos(fOmega) * sin(fTeta), sin(fOmega) * sin(fTeta), -cos(fTeta) );
		++nSeed;
		if ( nSeed == 100000 )//12 )//
			nSeed = 0;
		if ( vSky.z <= -0.3f )
			break;
		ASSERT( 0 );
	}
	return vSky;
}

inline float GetLinear( float f ) { return pow( f, 2.4f ); }
inline CVec3 GetLinearColor( const CVec3 &_vColor )
{
	return CVec3( GetLinear( _vColor.x ), GetLinear( _vColor.y ), GetLinear( _vColor.z ) );
}

void CLightState::CreateScattered( SLightStateCalcSeed *pSeed, const SSphere &_bounds, 
	const SPerVertexLightState &l, IGScene *_pVis, int nSkyDirs )
{
	pVis = _pVis;
	CVec3 vLinearAmbient = GetLinearColor( l.vAmbientColor );
	vAmbientColor = vLinearAmbient;
	skyDirections.resize( nSkyDirs );
	for ( int i = 0; i < skyDirections.size(); ++i )
	{
		skyDirections[i] = GenerateSkyDir( pSeed, nSkyDirs );
		AddParallel( _bounds, skyDirections[i], vLinearAmbient * F_SKY_SINGLE_STRENGTH_MUL / nSkyDirs );
	}
	// sun is treated as parallel light source, usually single in scene
	CVec3 vSunColor = GetLinearColor( l.vLightColor + l.vAmbientColor ) - vLinearAmbient;
	vSunColor.Maximize( CVec3(0,0,0) );
	AddParallel( _bounds, l.vSunDir, vSunColor );

	for ( int k = 0; k < l.staticPointLights.size(); ++k )
	{
		const SPerVertexLightState::SPointLightInfo &p = l.staticPointLights[k];
		AddPoint( p.vCenter, p.fRadius, GetLinearColor( p.vColor ) );
	}
}

void CLightState::CreateSimple( SLightStateCalcSeed *pSeed, const SSphere &_bounds, const SPerVertexLightState &l, int nSkyDirs )
{
	CreateScattered( pSeed, _bounds, l, 0, nSkyDirs );
}
}

