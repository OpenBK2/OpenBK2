#pragma once

#include "UpdatableProcess.h"
#include "Stats_B2_M1/RPGStats.h"

class CIdleMechProcess : public IClientUpdatableProcess
{
	OBJECT_BASIC_METHODS( CIdleMechProcess )

	ZDATA
	int nID;
	std::vector<std::string> bones;
	std::vector<int> times;
	CDBPtr<NDb::SEffect> pEffect;
	bool bRandomLocator;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nID); f.Add(3,&bones); f.Add(4,&times); f.Add(5,&pEffect); f.Add(6,&bRandomLocator); return 0; }
private:
protected:
	CIdleMechProcess() : nID( 0 ), bRandomLocator( false ) {}
public:
	CIdleMechProcess( int nObjectID, const std::vector<std::string> &effectBones, const NDb::SComplexEffect *pComplexEffect, const bool _bRandomLocator = false );
	bool Update( const NTimer::STime &time );
};


