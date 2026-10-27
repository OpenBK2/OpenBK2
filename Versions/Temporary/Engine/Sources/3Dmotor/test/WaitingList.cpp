#include "3Dmotor/stdafx.h"

#include "3Dmotor/WaitingList.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

// CompactWaitingList replaced a loop that erased from the middle of a vector of
// CPtr once per entry it finished with, which was quadratic in the number of
// entries and half the cost of loading a map. The replacement is a single
// compacting pass, and what has to hold across that change is the behaviour the
// erases had: which entries leave, which stay, in what order, how many times each
// one is handed over, and that each reference is released exactly once.
//
// Nothing here needs a scene or a renderer. CompactWaitingList asks only for
// HasLoadedObjectInfo, so the part is a stub that answers it and counts its own
// destruction.
namespace
{
int g_nLiveParts = 0;

class CTestPart : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CTestPart );

public:
	std::string szName;
	bool bLoaded = false;

	CTestPart()
	{
		++g_nLiveParts;
	}

	CTestPart( const std::string &_szName, bool _bLoaded )
		: szName( _szName )
		, bLoaded( _bLoaded )
	{
		++g_nLiveParts;
	}

	~CTestPart() override
	{
		--g_nLiveParts;
	}

	bool HasLoadedObjectInfo() const
	{
		return bLoaded;
	}
};

//! The names still in the list, in order, so a test can say what it expects
//! without reaching through CPtr.
std::vector<std::string> NamesOf( const std::vector<CPtr<CTestPart>> &list )
{
	std::vector<std::string> names;
	for ( size_t i = 0; i < list.size(); ++i )
	{
		names.push_back( list[i] ? list[i]->szName : std::string( "<null>" ) );
	}
	return names;
}

//! A list built from name/loaded pairs, holding the only references to its parts.
std::vector<CPtr<CTestPart>> MakeList( const std::vector<std::pair<std::string, bool>> &spec )
{
	std::vector<CPtr<CTestPart>> list;
	for ( size_t i = 0; i < spec.size(); ++i )
	{
		list.push_back( new CTestPart( spec[i].first, spec[i].second ) );
	}
	return list;
}

//! Collects what CompactWaitingList handed over, in the order it did.
struct SRecorder
{
	std::vector<std::string> handed;

	void operator()( CTestPart *pPart )
	{
		handed.push_back( pPart->szName );
	}
};
} // namespace

TEST( WaitingList, EmptyListStaysEmpty )
{
	std::vector<CPtr<CTestPart>> list;
	SRecorder recorder;
	NGScene::CompactWaitingList( &list, std::ref( recorder ) );
	EXPECT_TRUE( list.empty() );
	EXPECT_TRUE( recorder.handed.empty() );
}

TEST( WaitingList, NothingLoadedKeepsEveryEntryInOrder )
{
	std::vector<CPtr<CTestPart>> list = MakeList( { { "a", false }, { "b", false }, { "c", false } } );
	SRecorder recorder;
	NGScene::CompactWaitingList( &list, std::ref( recorder ) );
	EXPECT_EQ( NamesOf( list ), ( std::vector<std::string>{ "a", "b", "c" } ) );
	EXPECT_TRUE( recorder.handed.empty() );
}

TEST( WaitingList, EverythingLoadedEmptiesTheList )
{
	std::vector<CPtr<CTestPart>> list = MakeList( { { "a", true }, { "b", true }, { "c", true } } );
	SRecorder recorder;
	NGScene::CompactWaitingList( &list, std::ref( recorder ) );
	EXPECT_TRUE( list.empty() );
	// In order, and once each: the old loop walked front to back and handed over
	// every entry it erased.
	EXPECT_EQ( recorder.handed, ( std::vector<std::string>{ "a", "b", "c" } ) );
}

TEST( WaitingList, LoadedLeaveAndWaitingKeepTheirOrder )
{
	// Loaded entries at the front, the back and the middle, which is where an
	// off-by-one in the compaction would show.
	std::vector<CPtr<CTestPart>> list =
			MakeList( { { "a", true }, { "b", false }, { "c", true }, { "d", false }, { "e", false }, { "f", true } } );
	SRecorder recorder;
	NGScene::CompactWaitingList( &list, std::ref( recorder ) );
	EXPECT_EQ( NamesOf( list ), ( std::vector<std::string>{ "b", "d", "e" } ) );
	EXPECT_EQ( recorder.handed, ( std::vector<std::string>{ "a", "c", "f" } ) );
}

TEST( WaitingList, NullEntriesAreDroppedWithoutBeingHandedOver )
{
	std::vector<CPtr<CTestPart>> list = MakeList( { { "a", false }, { "b", true }, { "c", false } } );
	// An entry whose part has gone. IsValid is false for it, and the loop this
	// replaced erased such an entry without calling anything on it.
	list.insert( list.begin() + 1, CPtr<CTestPart>() );
	SRecorder recorder;
	NGScene::CompactWaitingList( &list, std::ref( recorder ) );
	EXPECT_EQ( NamesOf( list ), ( std::vector<std::string>{ "a", "c" } ) );
	EXPECT_EQ( recorder.handed, ( std::vector<std::string>{ "b" } ) );
}

TEST( WaitingList, EntriesThatLeaveAreReleasedExactlyOnce )
{
	// The list holds the only reference to each part, so a part that leaves it
	// has to be destroyed, and one that stays has to survive. A compaction that
	// released a reference twice would destroy a live part; one that released
	// none would leak every part the list ever held.
	const int nBefore = g_nLiveParts;
	{
		std::vector<CPtr<CTestPart>> list = MakeList( { { "a", true }, { "b", false }, { "c", true }, { "d", false } } );
		EXPECT_EQ( g_nLiveParts, nBefore + 4 );

		SRecorder recorder;
		NGScene::CompactWaitingList( &list, std::ref( recorder ) );

		// a and c left, so nothing holds them; b and d are still in the list.
		EXPECT_EQ( g_nLiveParts, nBefore + 2 );
		EXPECT_EQ( NamesOf( list ), ( std::vector<std::string>{ "b", "d" } ) );
	}
	EXPECT_EQ( g_nLiveParts, nBefore );
}

TEST( WaitingList, APartHeldElsewhereSurvivesLeavingTheList )
{
	// What PlaceToOctree does: the callback takes a reference of its own while
	// the entry leaves. Releasing the list's reference must not take the part
	// with it.
	const int nBefore = g_nLiveParts;
	CPtr<CTestPart> pKeeper;
	{
		std::vector<CPtr<CTestPart>> list = MakeList( { { "a", true } } );
		NGScene::CompactWaitingList( &list, [&pKeeper]( CTestPart *pPart ) { pKeeper = pPart; } );
		EXPECT_TRUE( list.empty() );
	}
	EXPECT_EQ( g_nLiveParts, nBefore + 1 );
	EXPECT_EQ( pKeeper->szName, "a" );
	pKeeper = 0;
	EXPECT_EQ( g_nLiveParts, nBefore );
}

TEST( WaitingList, RepeatedPassesConvergeTheWayTheWaitLoopNeeds )
{
	// CGScene::LoadEverything spins calling this until the list empties, with
	// entries becoming loaded in between. Each pass has to hand over exactly the
	// entries that became loaded since the last one, and never a second time.
	std::vector<CPtr<CTestPart>> list = MakeList( { { "a", false }, { "b", false }, { "c", false }, { "d", false } } );
	SRecorder recorder;

	NGScene::CompactWaitingList( &list, std::ref( recorder ) );
	EXPECT_EQ( list.size(), 4u );
	EXPECT_TRUE( recorder.handed.empty() );

	list[1]->bLoaded = true;
	NGScene::CompactWaitingList( &list, std::ref( recorder ) );
	EXPECT_EQ( NamesOf( list ), ( std::vector<std::string>{ "a", "c", "d" } ) );
	EXPECT_EQ( recorder.handed, ( std::vector<std::string>{ "b" } ) );

	list[0]->bLoaded = true;
	list[2]->bLoaded = true;
	NGScene::CompactWaitingList( &list, std::ref( recorder ) );
	EXPECT_EQ( NamesOf( list ), ( std::vector<std::string>{ "c" } ) );
	EXPECT_EQ( recorder.handed, ( std::vector<std::string>{ "b", "a", "d" } ) );

	list[0]->bLoaded = true;
	NGScene::CompactWaitingList( &list, std::ref( recorder ) );
	EXPECT_TRUE( list.empty() );
	EXPECT_EQ( recorder.handed, ( std::vector<std::string>{ "b", "a", "d", "c" } ) );
}
