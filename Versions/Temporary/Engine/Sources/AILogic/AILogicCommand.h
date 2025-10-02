#pragma once

interface IAILogicCommandB2 : public CObjectBase
{
	virtual void Execute() = 0;
	// нужно ли сохранять в истории команд
	virtual bool NeedToBeStored() const = 0;

#ifndef _FINALRELEASE
	virtual string GetDebugInfo() const { return "\n"; }
#endif
};

