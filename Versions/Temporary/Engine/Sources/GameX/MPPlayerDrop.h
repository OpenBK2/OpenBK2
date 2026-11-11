#pragma once

#include <algorithm>
#include <limits>
#include <vector>

namespace NMPPlayerDrop
{
enum EResult
{
	INVALID,
	UNCHANGED,
	SCHEDULED,
	TOO_LATE
};

inline int GetRequiredCommandPacket( int nCurrentCommonSegment, int nLatency, int nSegmentsPackSize )
{
	// Packet P is queued at P + pack size and executed another latency later.
	// Check the startup boundary before subtracting to avoid signed overflow.
	if ( nCurrentCommonSegment < 0 || nLatency < 0 || nSegmentsPackSize <= 0 || nCurrentCommonSegment < nLatency ||
	     nCurrentCommonSegment - nLatency < nSegmentsPackSize )
	{
		return -1;
	}
	return nCurrentCommonSegment - nLatency - nSegmentsPackSize;
}

class SPlayerDrop
{
	int nDepartingHostClientID = -1;
	int nCommandSegment = -1;
	int nDeferredSegment = -1;
	std::vector<int> correctionAuthorities;
	bool bApplied = false;
	bool bDisconnected = false;

public:
	static bool IsValidSegment( int nSegment, int nLatency )
	{
		return nSegment >= 0 && nLatency >= 0 && nSegment <= ( std::numeric_limits<int>::max )() - nLatency;
	}

	bool DeferSelfAnnouncement( int nSender, int nTargetClient, int nSegment, int nLatency )
	{
		if ( nSender < 0 || nSender != nTargetClient || !IsValidSegment( nSegment, nLatency ) )
		{
			return false;
		}
		// A future host's self-departure can arrive before the preceding handoff.
		// Retaining it grants no authority and does not schedule any removal.
		if ( nDeferredSegment < 0 || nSegment < nDeferredSegment )
		{
			nDeferredSegment = nSegment;
		}
		return true;
	}

	int TakeDeferredSegment()
	{
		const int nSegment = nDeferredSegment;
		nDeferredSegment = -1;
		return nSegment;
	}

	int GetDeferredSegment() const
	{
		return nDeferredSegment;
	}

	void RememberDisconnect()
	{
		// Backend notifications can precede this client's turn as control host.
		bDisconnected = true;
	}

	bool IsDisconnected() const
	{
		return bDisconnected;
	}

	void RememberHost( int nClientID )
	{
		if ( nClientID >= 0 )
		{
			nDepartingHostClientID = nClientID;
		}
	}

	void RememberCorrectionAuthority( int nClientID )
	{
		// A retiring host may still have corrections in flight for known drops.
		// Retaining that authority must not authorize any new unrelated departure.
		if ( nCommandSegment >= 0 && nClientID >= 0 &&
		     std::find( correctionAuthorities.begin(), correctionAuthorities.end(), nClientID ) ==
		         correctionAuthorities.end() )
		{
			correctionAuthorities.push_back( nClientID );
		}
	}

	bool IsAuthoritative( int nSender, int nCurrentHost, int nTargetClient, int nReplacementHost ) const
	{
		if ( nSender < 0 || nTargetClient < 0 )
		{
			return false;
		}
		if ( nSender == nCurrentHost )
		{
			return true;
		}
		if ( nTargetClient == nCurrentHost && nReplacementHost >= 0 && nSender == nReplacementHost )
		{
			return true;
		}
		if ( std::find( correctionAuthorities.begin(), correctionAuthorities.end(), nSender ) !=
		     correctionAuthorities.end() )
		{
			return true;
		}
		// Backend removal can change authority before the departing host's own
		// announcement arrives. That old authority extends only to its own slot.
		return nSender == nDepartingHostClientID && nTargetClient == nDepartingHostClientID;
	}

	EResult Schedule( int nSegment, int nCurrentCommonSegment, int nLatency )
	{
		if ( nCurrentCommonSegment < 0 || !IsValidSegment( nSegment, nLatency ) )
		{
			return INVALID;
		}
		// A relay of an already accepted or later cutoff cannot change the result,
		// including after application. Only an earlier cutoff needs reconciliation.
		if ( nCommandSegment >= 0 && nSegment >= nCommandSegment )
		{
			return UNCHANGED;
		}
		if ( bApplied || nSegment + nLatency < nCurrentCommonSegment )
		{
			// The simulation cannot undo an applied transfer or earlier commands.
			// The caller must stop on this result instead of applying at local now.
			return TOO_LATE;
		}
		nCommandSegment = nSegment;
		return SCHEDULED;
	}

	int GetSegment() const
	{
		return nCommandSegment;
	}

	int GetRemovalSegment( int nLatency ) const
	{
		if ( !IsValidSegment( nCommandSegment, nLatency ) )
		{
			return -1;
		}
		return nCommandSegment + nLatency;
	}

	bool IsPending() const
	{
		return nCommandSegment >= 0 && !bApplied;
	}

	bool RequiresPacket( int nCurrentCommonSegment, int nLatency, int nSegmentsPackSize, int nLastReceivedSegment ) const
	{
		const int nPacket = GetRequiredCommandPacket( nCurrentCommonSegment, nLatency, nSegmentsPackSize );
		// Releasing the network wait must not skip commands before the committed
		// removal. Ordered receipt through P proves that this prefix has arrived.
		return IsPending() && nPacket >= 0 && nPacket < nCommandSegment - nSegmentsPackSize &&
		       nLastReceivedSegment < nPacket;
	}

	void MarkApplied()
	{
		if ( nCommandSegment >= 0 )
		{
			bApplied = true;
		}
	}
};
} // namespace NMPPlayerDrop
