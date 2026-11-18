#pragma once

struct IAILogicCommandB2 : public CObjectBase
{
	virtual void Execute() = 0;

	// Live lockstep execution knows the originating map slot independently of the command payload.
	virtual void ExecuteFromPlayer( int nIssuingPlayer )
	{
		Execute();
	}

	// нужно ли сохранять в истории команд
	virtual bool NeedToBeStored() const = 0;

#ifndef _FINALRELEASE
	virtual std::string GetDebugInfo() const
	{
		return "\n";
	}
#endif
};
