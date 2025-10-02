
#pragma once

interface IStatesFactory : public CAIObjectBase
{
public:
	virtual bool CanCommandBeExecuted( class CAICommand *pCommand ) = 0;

	virtual int operator&( IBinSaver &saver ) = 0;
	virtual interface IUnitState* ProduceState( class CQueueUnit *pUnit, class CAICommand *pCommand ) = 0;
	virtual interface IUnitState* ProduceRestState( class CQueueUnit *pUnit ) = 0;
};


