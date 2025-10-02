#include "StdAfx.h"
#include "GParticlesRain.h"
#include "GParticleFilter.h"
#include "4dCalcs.h"

namespace NGScene
{

// time period of one particle falling
const float FP_DROP_RAIN_CYCLE = 200;//60;
// speed of falling in meters per segment
const float FP_RAIN_DROP_SPEED = 0.5f;
// number of drops per meter
const float FP_DROP_DENSITY = 2;
const float FP_DROP_DENSITY_1 = 1 / FP_DROP_DENSITY;
// Z coord dispersion of drops
const float FP_DROP_Z_DISP = 10;
// number of particle squares in one side
const int N_PARTICLES_S_NUMBER = 40;
// average Z coord of one drop
const float FP_DROP_RAIN_MIDDLE_Z = FP_DROP_RAIN_CYCLE * FP_RAIN_DROP_SPEED / 2 - FP_DROP_Z_DISP;
// sizes of particles
const float FP_LIMIT_DISTANCE = N_PARTICLES_S_NUMBER / FP_DROP_DENSITY + FP_DROP_Z_DISP;
const int N_WEATHER_POWER_GRADATIONS = 64;

static int nRandomTable[32] = {
	17, 43, 456, 942,  32, 234, 865, 95,  321, 47, 909, 284,  543, 396, 193, 120,
	98, 784, 633, 10,  259, 77, 118, 66,  921, 849, 356, 80,  475, 235, 213, 826
};
void CRainAnimator::Recalc()
{
	if ( !IsValid( pValue ) )
	{
		CRainParticleEffect *pRealValue = new CRainParticleEffect;
		pValue = pRealValue;
		pRealValue->textures = textureIDs;
	}
	CDynamicCast<CRainParticleEffect> pRealValue(pValue);

	if ( tStart == 0 )
		tStart = pTime->GetValue();
	float T = pTime->GetValue() / 1024.0f * 100;

	float fStart = ( pTime->GetValue() - tStart ) / 1024 * FP_PI;
	int nPowerCheck = Float2Int( N_WEATHER_POWER_GRADATIONS * ( fStart > FP_PI ? 1 : 0.5f - cos( fStart ) * 0.5f ) );

	CVec3 vRainWind( 0.1f, 0.2f, -FP_RAIN_DROP_SPEED );
	CVec3 vParticleDir = -vRainWind / FP_RAIN_DROP_SPEED;

	CVec3 vCameraPos = SafeUnhomogen( pCamera->GetValue() );//pWorld->GetCamera()->GetCP();
	float fShiftAmount = ( vCameraPos.z - FP_DROP_RAIN_MIDDLE_Z ) / vRainWind.z;
	int nCameraX = Float2Int( ( vCameraPos.x - vRainWind.x * fShiftAmount ) * FP_DROP_DENSITY );
	int nCameraY = Float2Int( ( vCameraPos.y - vRainWind.y * fShiftAmount ) * FP_DROP_DENSITY );

	//float fpMinH, fpMaxH;
	//fpMinH = Max( 0.0f, vCameraPos.z - FP_LIMIT_DISTANCE );
	//fpMaxH = vCameraPos.z + FP_LIMIT_DISTANCE;
	float fWrapCenterZ = vCameraPos.z;
	int nBufSpace = sqr( N_PARTICLES_S_NUMBER * 2 + 1 );
	vector<char> faces( nBufSpace );
	vector<CVec3> positions( nBufSpace );
	vector<CVec3> directions( nBufSpace );
	int nTotalParticles = 0;
	for ( int y = -N_PARTICLES_S_NUMBER; y <= N_PARTICLES_S_NUMBER; ++y )
	{
		for ( int x = -N_PARTICLES_S_NUMBER; x <= N_PARTICLES_S_NUMBER; ++x )
		{
			float fX, fY, fZ;
			int nX = nCameraX + x;
			int nY = nCameraY + y;
			int nStep;
			float fpTime = T + (( nX * 172 + nY * 13 + nRandomTable[ ( nX * nX + nY ) & 31 ] ) & 0xffff );
			nStep = Float2Int( fpTime / FP_DROP_RAIN_CYCLE );
			if ( (nStep&(N_WEATHER_POWER_GRADATIONS-1)) >= nPowerCheck )
				continue;
			float fpTimeStep = ( fpTime - nStep * FP_DROP_RAIN_CYCLE );
			fZ = FP_DROP_RAIN_MIDDLE_Z + fpTimeStep * vRainWind.z;
			fZ += Float2Int( ( fWrapCenterZ - fZ ) * (1 / (2 * FP_LIMIT_DISTANCE)) ) * (2 * FP_LIMIT_DISTANCE);
			//while ( fZ > fpMaxH )
			//	fZ -= 2 * FP_LIMIT_DISTANCE;
			//while ( fZ < fpMinH )
			//	fZ += 2 * FP_LIMIT_DISTANCE;
			fX = float( nX ) * FP_DROP_DENSITY_1 + fpTimeStep * vRainWind.x;
			fY = float( nY ) * FP_DROP_DENSITY_1 + fpTimeStep * vRainWind.y;
			fX += ( ( ( nStep * 197  ) & 255 ) - 128 ) / 128.0f * FP_DROP_DENSITY_1;
			fY += ( ( ( nStep * 67 ) & 255 ) - 128 ) / 128.0f * FP_DROP_DENSITY_1;
			fZ += ( ( ( nStep * 129 ) & 255 ) - 128 ) / 128.0f * FP_DROP_Z_DISP;
			faces[nTotalParticles] = nStep & 3;
			positions[nTotalParticles] = CVec3(fX, fY, fZ );
			float fDeltaX = ( ( nRandomTable[(nX*nX)&31         ]&31) / 31.0f ) * 0.2f - 0.1f;
			float fDeltaY = ( ( nRandomTable[(nX*nY + nY*nY)&31 ]&31) / 31.0f ) * 0.2f - 0.1f;
			directions[nTotalParticles] = vParticleDir + CVec3( fDeltaX, fDeltaY, 0 );
			++nTotalParticles;
		}
	}
	faces.resize( nTotalParticles );
	positions.resize( nTotalParticles );
	directions.resize( nTotalParticles );
	if ( IsValid(pFilter) )
	{
		vector<char> fake, filter;
		pFilter->FilterParticles( positions, fake, &filter );
		pRealValue->faces.resize( nTotalParticles );
		pRealValue->positions.resize( nTotalParticles );
		pRealValue->directions.resize( nTotalParticles );
		int nRes = 0;
		for ( int k = 0; k < filter.size(); ++k )
		{
			if ( !filter[k] )
			{
				pRealValue->faces[nRes] = faces[k];
				pRealValue->positions[nRes] = positions[k];
				pRealValue->directions[nRes] = directions[k];
				++nRes;
			}
		}
		pRealValue->faces.resize( nRes );
		pRealValue->positions.resize( nRes );
		pRealValue->directions.resize( nRes );
	}
	else
	{
		pRealValue->faces.swap( faces );
		pRealValue->positions.swap( positions );
		pRealValue->directions.swap( directions );
	}
}
}
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x01063120, CRainAnimator )

