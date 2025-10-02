#pragma once


#include "PredictedAntiAviationFire.h"
class CTankShootEstimator;
class CAIUnit;
class CShootEstimatorSupportAAGun;

class CSupportAAGun : public CAIObjectBase
{
	OBJECT_BASIC_METHODS( CSupportAAGun );
	ZDATA
	CPredictedAntiAviationFire predictedFire;
	CPtr<CShootEstimatorSupportAAGun> pShootEstimator;
	NTimer::STime timeNextTargetSearch;
	DWORD dwAllowed;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&predictedFire); f.Add(3,&pShootEstimator); f.Add(4,&timeNextTargetSearch); f.Add(5,&dwAllowed); return 0; }

public:

	CSupportAAGun() : predictedFire( 0, 0 ) {  }
	CSupportAAGun( CAIUnit *_pUnit );
	
	void Segment();
	void AddGunNumber( const int nGun );
};


