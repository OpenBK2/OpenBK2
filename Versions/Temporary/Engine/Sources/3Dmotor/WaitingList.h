#pragma once

#include "System/Basic.h"

#include <vector>

namespace NGScene
{

//! Walk a list of parts waiting on their geometry, hand over the ones that have
//! it, and drop them from the list. OnLoaded is called for each of those, in
//! order, before the list shrinks; a part that died while waiting is dropped
//! without it. Everything still waiting stays, in the order it was in.
//!
//! One pass, because the loop this replaces erased from the middle of the vector
//! as it walked, once per part it finished with. std::vector::erase shifts every
//! element behind the erased one down a slot, and these are vectors of CPtr,
//! whose assignment is an AddRef on the new target followed by a Release on the
//! old, so finishing k parts out of n cost O(n*k) refcount updates.
//! CGScene::LoadEverything calls CGScene::WalkNotLoadedObjects from a loop it
//! spins while the resource thread works, so the whole walk repeated on every
//! spin. On a map with many objects that came to 46 per cent of the time the
//! mission spent loading, measured with perf and with perforator separately, and
//! it grew with the square of the object count, which is why the largest maps
//! suffered worst.
//!
//! Compacting assigns each surviving entry at most once, so the cost is linear.
//!
//! The references the list drops are released when it shrinks rather than one at
//! a time as the walk passes them. That holds every object a little longer than
//! the erases did and never releases one sooner, so nothing can be reached after
//! its last reference has gone that could not be before.
//!
//! TPart needs HasLoadedObjectInfo(). The predicate is deliberately not a
//! parameter: this says what "finished waiting" means for every caller.
template <class TPart, class TOnLoaded>
void CompactWaitingList( std::vector<CPtr<TPart>> *pList, TOnLoaded OnLoaded )
{
	size_t nKept = 0;
	for ( size_t i = 0; i < pList->size(); ++i )
	{
		TPart *pPart = ( *pList )[i];
		if ( !IsValid( pPart ) )
		{
			continue; // died while waiting, so it leaves without being handed over
		}
		if ( pPart->HasLoadedObjectInfo() )
		{
			OnLoaded( pPart );
			continue; // finished, so it leaves the list
		}
		// Still waiting, so it stays. Assigning a slot to itself would be a
		// pointless AddRef and Release, and until something leaves there is
		// nothing to move anything down to.
		if ( nKept != i )
		{
			( *pList )[nKept] = ( *pList )[i];
		}
		++nKept;
	}
	// Everything from nKept on is either a stale copy of an entry now held
	// earlier in the vector or an entry that left, so dropping them releases
	// each reference exactly once.
	pList->resize( nKept );
}

} // namespace NGScene
