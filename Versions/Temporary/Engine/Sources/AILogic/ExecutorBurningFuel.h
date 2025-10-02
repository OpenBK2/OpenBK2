#pragma once
#include "executor.h"

namespace NDb
{
	struct SBurningFuel;
}

class CExecutorBurningFuel : public CExecutor
{
	OBJECT_BASIC_METHODS(CExecutorBurningFuel)
	ZDATA_(CExecutor)
	CVec3 vPos;
	CDBPtr<NDb::SBurningFuel> pStats;
	NTimer::STime timeBurn;
public: 
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CExecutor*)this); f.Add(2,&vPos); f.Add(3,&pStats); f.Add(4,&timeBurn); return 0; }
public:
	CExecutorBurningFuel( const CVec3 &_vPos, const NDb::SBurningFuel *_pStats );
	CExecutorBurningFuel() {  }
	virtual int Segment();
	virtual bool NotifyEvent( const CExecutorEvent &event ) { return false; }
	virtual bool IsExecutorValid() const { return true; }
};

