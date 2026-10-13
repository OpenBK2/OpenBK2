#pragma once
#include "Executor.h"

class CExecutorSimpleEvent : public CExecutorEvent
{
public:
	CExecutorSimpleEvent( const SExecutorEventParam &param ) : CExecutorEvent( param ) {  }

	CExecutorSimpleEvent() { }
};
