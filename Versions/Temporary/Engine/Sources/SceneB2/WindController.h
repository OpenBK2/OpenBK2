#pragma once

#include "SceneB2_export.h"

#include "../system/time.h"
#include "../Main/GameTimer.h"

//
//		Creates waves of wind activity according to wind direction
//

class SCENEB2_EXPORT CWindController : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CWindController )

	CPtr<IGameTimer> pTimer;
	//Stored wind parameters
	float fWindDirection;
	float fWindIntensity;

	//Computed
	int nCycleLength;
	float fSpeedCoeff;
	float fCoeffX, fCoeffY;		//For turning

	float CoeffFunc( int nParam );			//Has positive values (up to 1.0) in [0, 1000], zero elsewhere.
public:
	CWindController() {}
	CWindController( IGameTimer *_pTimer, const float _fWindDirection, const float _fWindIntensity );

	void SetWindParams( const float _fWindDirection, const float _fWindIntensity );
	float GetWindDirection() const { return fWindDirection; }
	float GetWindIntensity() const { return fWindIntensity; }
	void GetDirCoeff( float *pCX, float *pCY ) const { *pCX = fCoeffX; *pCY = fCoeffY; }
	float GetWindCoeff( const CVec2 &vPos );

	int operator&( IBinSaver &saver );
};

