#pragma once
#include "Executor.h"

class CAIUnit;

class CExecutorTransportHealInfantry : public CExecutor
{
	OBJECT_BASIC_METHODS( CExecutorTransportHealInfantry )
	ZDATA_( CExecutor )
	CPtr<CAIUnit> pUnit;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CExecutor *)this );
		f.Add( 2, &pUnit );
		return 0;
	}

	bool IsExecutorValidInternal() const;

public:
	CExecutorTransportHealInfantry( CAIUnit *_pUnit )
		: CExecutor( TID_TRANSPORT_HEAL_INFANTRY, SConsts::BEH_UPDATE_DURATION / SConsts::AI_SEGMENT_DURATION )
		, pUnit( _pUnit )
	{
	}

	CExecutorTransportHealInfantry() {}

	int Segment() override;
	bool NotifyEvent( const CExecutorEvent &event ) override;

	bool IsExecutorValid() const override
	{
		return IsExecutorValidInternal();
	}
};
