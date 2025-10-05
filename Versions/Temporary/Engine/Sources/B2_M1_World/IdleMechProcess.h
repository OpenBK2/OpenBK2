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
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nID); f.Add(3,&bones); f.Add(4,&times); f.Add(5,&pEffect); return 0; }
private:
protected:
	CIdleMechProcess() {}
public:
	CIdleMechProcess( int nObjectID, const std::vector<std::string> &effectBones, const NDb::SComplexEffect *pComplexEffect );
	bool Update( const NTimer::STime &time );
};


