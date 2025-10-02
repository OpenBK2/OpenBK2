#include "StdAfx.h"

#include "WindController.h"
#include "System/FastMath.h"

float CWindController::CoeffFunc( int nParam )
{
	const int nPoint1 = 0;
	const int nPoint2 = 100;
	const int nPoint3 = 300;
	const int nPoint4 = 1000;

	float fRes = 0.0f;
	float fParam = nParam / fWindIntensity * 0.5f;		// Multiplied by speed

	if ( fParam < nPoint1 ) 
		fRes = 0.0f;
	else if ( fParam < nPoint2 ) 
		fRes = fParam / nPoint2;
	else if ( fParam < nPoint3 ) 
		fRes = 1.0f;
	else if ( fParam < nPoint4 ) 
		fRes = 1.0f - ( fParam - nPoint3 ) / ( nPoint4 - nPoint3 );

	//elsewhere
	return fRes;
}

CWindController::CWindController( IGameTimer *_pTimer, const float _fWindDirection, const float _fWindIntensity ) :
pTimer( _pTimer )
{

	SetWindParams( _fWindDirection, _fWindIntensity );
}

void CWindController::SetWindParams( const float _fWindDirection, const float _fWindIntensity )
{
	fWindDirection = _fWindDirection;
	fWindIntensity = Clamp( _fWindIntensity, 0.0f, 10.0f );

	const float fAngleRad = _fWindDirection * PI / 180;
	fCoeffY = cos( fAngleRad );
	fCoeffX = sin( fAngleRad );

	// Cycle length: ~13 seconds with Intensity = 1, 3 seconds (continuous) with Intensity = 10
	nCycleLength = 9000 + fWindIntensity * 1200;
	// Speed coeff: ~0.2 with Intensity = 1, ~2 with Intensity = 10;
	fSpeedCoeff = fWindIntensity / 5.0f;
}

float CWindController::GetWindCoeff( const CVec2 &vPos )
{
	if ( fWindIntensity == 0.0f )				//No intensity = no wind
		return 0.0f;

	int nWindCoord = int ( ( vPos.y * fCoeffY + vPos.x * fCoeffX ) + pTimer->GetGameTime() * fSpeedCoeff ) % nCycleLength;
	int nWindWaveCoord = int ( NMath::Sin( ( vPos.x * fCoeffY - vPos.y * fCoeffX ) * 0.01f ) * 300 );

	return CoeffFunc( nWindCoord + nWindWaveCoord );
}

int CWindController::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pTimer );
	saver.Add( 3, &fCoeffX );
	saver.Add( 4, &fCoeffY );
	saver.Add( 5, &nCycleLength );
	saver.Add( 6, &fSpeedCoeff );
	saver.Add( 7, &fWindDirection );
	saver.Add( 8, &fWindIntensity );

	return 0;
}

REGISTER_SAVELOAD_CLASS( 0x30135500, CWindController );


