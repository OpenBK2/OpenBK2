
#pragma once

struct IStatesFactory : public CAIObjectBase
{
public:
	virtual bool CanCommandBeExecuted( class CAICommand *pCommand ) = 0;

	int operator&( IBinSaver &saver ) override = 0;
	virtual struct IUnitState *ProduceState( class CQueueUnit *pUnit, class CAICommand *pCommand ) = 0;
	virtual struct IUnitState *ProduceRestState( class CQueueUnit *pUnit ) = 0;
};
