
#pragma once

struct IStatesFactory : public CAIObjectBase
{
public:
	virtual bool CanCommandBeExecuted( class CAICommand *pCommand ) = 0;

	virtual int operator&( IBinSaver &saver ) = 0;
	virtual struct IUnitState* ProduceState( class CQueueUnit *pUnit, class CAICommand *pCommand ) = 0;
	virtual struct IUnitState* ProduceRestState( class CQueueUnit *pUnit ) = 0;
};


