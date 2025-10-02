#pragma once

#include "Executor.h"
#include "Mine.h"

class CTimerChargeExecutor :	public CExecutor
{
	OBJECT_BASIC_METHODS( CTimerChargeExecutor );
	ZDATA_(CExecutor)
	CPtr<CMineStaticObject> pCharge;  
	int nTimeToExplode;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CExecutor*)this); f.Add(2,&pCharge); f.Add(3,&nTimeToExplode); return 0; }

public:
	CTimerChargeExecutor( CMineStaticObject *_pObj, int nOffset );
	CTimerChargeExecutor() {  }

	virtual int Segment();
	virtual bool NotifyEvent( const CExecutorEvent &event ) { return false; }
	virtual bool IsExecutorValid() const;
};


