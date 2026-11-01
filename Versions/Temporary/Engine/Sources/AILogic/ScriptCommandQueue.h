#pragma once

#include "Stats_B2_M1/AIUnitCmd.h"
#include "System/BinSaver.h"

#include <utility>
#include <vector>

// Simulation-derived orders are reproduced by Lua on every peer and during replay.
// Keep values and unit identities here, never a transient selection or Lua stack reference.
struct SQueuedScriptCommand
{
	std::vector<int> unitIDs;
	SAIUnitCmd command;
	bool bPlaceInQueue = false;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 2, &unitIDs );
		saver.Add( 3, &command );
		saver.Add( 4, &bPlaceInQueue );
		return 0;
	}
};

class CScriptCommandQueue
{
	std::vector<SQueuedScriptCommand> pending;

public:
	int operator&( IBinSaver &saver )
	{
		saver.Add( 2, &pending );
		return 0;
	}

	void Add( SQueuedScriptCommand command )
	{
		pending.push_back( std::move( command ) );
	}

	void Clear()
	{
		pending.clear();
	}

	template <class TExecute>
	void Drain( TExecute execute )
	{
		// Registered-command callbacks can enqueue more orders. They belong to the
		// next segment, so callbacks cannot recurse into the batch being executed.
		std::vector<SQueuedScriptCommand> batch;
		batch.swap( pending );
		for ( SQueuedScriptCommand &command : batch )
		{
			execute( command );
		}
	}
};
