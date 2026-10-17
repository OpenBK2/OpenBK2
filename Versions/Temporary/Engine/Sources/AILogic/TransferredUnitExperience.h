#pragma once

#include "System/BinSaver.h"

// Levels are zero based, as in the scenario tracker. Ordinary units keep following their owner's category.
class CTransferredUnitExperience
{
	int nLevel = -1;
	int nOwnerLevel = -1;

public:
	int operator&( IBinSaver &saver )
	{
		saver.Add( 2, &nLevel );
		saver.Add( 3, &nOwnerLevel );
		return 0;
	}

	bool IsTransferred() const
	{
		return nLevel >= 0;
	}

	int GetLevel( int nCurrentOwnerLevel ) const
	{
		return IsTransferred() ? nLevel : nCurrentOwnerLevel;
	}

	void Transfer( int nUnitLevel, int nNewOwnerLevel )
	{
		nLevel = nUnitLevel;
		nOwnerLevel = nNewOwnerLevel;
	}

	bool UpdateOwnerLevel( int nNewOwnerLevel )
	{
		if ( !IsTransferred() )
		{
			return false;
		}
		// A donation never catches up immediately, even when its recipient is already at the maximum.
		const bool bAdvance = nNewOwnerLevel > nOwnerLevel && nNewOwnerLevel > nLevel;
		nOwnerLevel = nNewOwnerLevel;
		if ( bAdvance )
		{
			nLevel = nNewOwnerLevel;
		}
		return bAdvance;
	}
};
