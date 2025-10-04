#pragma once
#include "Executor.h"

class CAIUnit;

class CExecutorTransportHealInfantry :	public CExecutor
{
	OBJECT_BASIC_METHODS(CExecutorTransportHealInfantry)
		ZDATA_(CExecutor)
	CPtr<CAIUnit> pUnit;  

	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CExecutor*)this); f.Add(2,&pUnit); return 0; }
	bool IsExecutorValidInternal() const;
public:
	CExecutorTransportHealInfantry( CAIUnit *_pUnit )
		: CExecutor(TID_TRANSPORT_HEAL_INFANTRY, SConsts::BEH_UPDATE_DURATION/SConsts::AI_SEGMENT_DURATION), pUnit( _pUnit ) { }
	CExecutorTransportHealInfantry() {  }
	virtual int Segment();
	virtual bool NotifyEvent( const CExecutorEvent &event );
	virtual bool IsExecutorValid() const { return IsExecutorValidInternal(); }

};



