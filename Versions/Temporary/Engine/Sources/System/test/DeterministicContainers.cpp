#include "../det_map.h"
#include "../det_set.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

namespace {

template <class Map>
std::vector<typename Map::key_type> Keys( const Map &map )
{
	std::vector<typename Map::key_type> keys;
	for ( const auto &entry : map )
		keys.push_back( entry.first );
	return keys;
}

template <class Set>
std::vector<typename Set::value_type> Values( const Set &set )
{
	return { set.begin(), set.end() };
}

struct ThrowOnCopy
{
	static bool bThrow;
	int nValue;

	explicit ThrowOnCopy( int n = 0 ) : nValue( n ) {}
	ThrowOnCopy( const ThrowOnCopy &other ) : nValue( other.nValue )
	{
		if ( bThrow )
			throw std::runtime_error( "copy failed" );
	}
	ThrowOnCopy( ThrowOnCopy && ) = default;
};

bool ThrowOnCopy::bThrow = false;

// A copy must preserve the existing functors, not default-construct new ones.
int nDefaultModulus = 10;
struct ModuloHash
{
	int nModulus = nDefaultModulus;
	std::size_t operator()( int n ) const { return std::hash<int>{}( n % nModulus ); }
};
struct ModuloEqual
{
	int nModulus = nDefaultModulus;
	bool operator()( int a, int b ) const { return a % nModulus == b % nModulus; }
};

} // namespace

TEST( DeterministicContainers, MapCopyOwnsItsLookupAndPreservesOrder )
{
	det_map<int, std::string> source{ { 8, "eight" }, { 2, "two" }, { 5, "five" } };
	auto copy = source;
	EXPECT_EQ( Keys( copy ), ( std::vector<int>{ 8, 2, 5 } ) );
	ASSERT_NE( copy.find( 8 ), copy.end() );
	EXPECT_EQ( std::addressof( *copy.find( 8 ) ), std::addressof( *copy.begin() ) );
	EXPECT_NE( std::addressof( copy.at( 8 ) ), std::addressof( source.at( 8 ) ) );
	copy[8] = "changed";
	EXPECT_EQ( source.at( 8 ), "eight" );
	EXPECT_EQ( copy.begin()->second, "changed" );
	ASSERT_TRUE( copy.erase( 2 ) );
	copy.erase( copy.find( 5 ) );
	EXPECT_EQ( Keys( copy ), ( std::vector<int>{ 8 } ) );
	EXPECT_EQ( Keys( source ), ( std::vector<int>{ 8, 2, 5 } ) );
	EXPECT_FALSE( copy.insert( { 8, "duplicate" } ).second );
	EXPECT_EQ( copy.at( 8 ), "changed" );
}

TEST( DeterministicContainers, SetCopyOwnsItsLookupAndPreservesOrder )
{
	det_set<int> source{ 8, 2, 5 };
	auto copy = source;
	EXPECT_EQ( Values( copy ), ( std::vector<int>{ 8, 2, 5 } ) );
	ASSERT_NE( copy.find( 8 ), copy.end() );
	EXPECT_EQ( std::addressof( *copy.find( 8 ) ), std::addressof( *copy.begin() ) );
	EXPECT_NE( std::addressof( *copy.find( 8 ) ), std::addressof( *source.find( 8 ) ) );
	ASSERT_TRUE( copy.erase( 2 ) );
	copy.erase( copy.find( 5 ) );
	EXPECT_EQ( Values( copy ), ( std::vector<int>{ 8 } ) );
	EXPECT_EQ( Values( source ), ( std::vector<int>{ 8, 2, 5 } ) );
	EXPECT_FALSE( copy.insert( 8 ).second );
}

TEST( DeterministicContainers, AssignedCopiesSurviveSourceDestruction )
{
	det_map<int, int> map{ { 99, 99 } };
	det_set<int> set{ 99 };
	{
		const det_map<int, int> sourceMap{ { 4, 40 }, { 1, 10 } };
		const det_set<int> sourceSet{ 4, 1 };
		map = sourceMap;
		set = sourceSet;
	}
	EXPECT_EQ( Keys( map ), ( std::vector<int>{ 4, 1 } ) );
	EXPECT_EQ( Values( set ), ( std::vector<int>{ 4, 1 } ) );
	EXPECT_EQ( map.at( 4 ), 40 );
	ASSERT_NE( set.find( 4 ), set.end() );
	EXPECT_EQ( *set.find( 4 ), 4 );
	EXPECT_FALSE( map.contains( 99 ) );
	EXPECT_FALSE( set.contains( 99 ) );
	EXPECT_TRUE( map.erase( 4 ) );
	EXPECT_TRUE( set.erase( 4 ) );
	const auto &sameMap = map;
	const auto &sameSet = set;
	map = sameMap;
	set = sameSet;
	EXPECT_EQ( map.at( 1 ), 10 );
	EXPECT_EQ( Values( set ), ( std::vector<int>{ 1 } ) );
	const det_map<int, int> emptyMap;
	const det_set<int> emptySet;
	map = emptyMap;
	set = emptySet;
	EXPECT_TRUE( map.empty() );
	EXPECT_TRUE( set.empty() );
	map[6] = 60;
	set.insert( 6 );
	EXPECT_EQ( map.at( 6 ), 60 );
	EXPECT_TRUE( set.contains( 6 ) );
}

TEST( DeterministicContainers, NestedCopiesAndVectorGrowthKeepIndependentIndices )
{
	using Nested = det_map<int, det_map<int, det_set<int>>>;
	std::vector<Nested> copies;
	{
		Nested source;
		source[7][3].insert( 12 );
		source[7][3].insert( 4 );
		copies.push_back( source );
		// Growth may copy or move existing containers depending on the STL.
		copies.reserve( copies.capacity() + 1 );
		source[7][3].erase( 12 );
		EXPECT_TRUE( copies.front().at( 7 ).at( 3 ).contains( 12 ) );
	}
	auto &values = copies.front().at( 7 ).at( 3 );
	EXPECT_EQ( Values( values ), ( std::vector<int>{ 12, 4 } ) );
	EXPECT_TRUE( values.erase( 12 ) );
	EXPECT_EQ( Values( values ), ( std::vector<int>{ 4 } ) );
}

TEST( DeterministicContainers, MovesTransferNodesWithoutCopyingValues )
{
	det_map<int, std::unique_ptr<int>> source;
	source.emplace( 3, std::make_unique<int>( 30 ) );
	const auto *pMapNode = std::addressof( *source.begin() );
	auto moved = std::move( source );
	EXPECT_EQ( std::addressof( *moved.find( 3 ) ), pMapNode );
	det_map<int, std::unique_ptr<int>> assigned;
	assigned.emplace( 8, std::make_unique<int>( 80 ) );
	assigned = std::move( moved );
	EXPECT_EQ( std::addressof( *assigned.find( 3 ) ), pMapNode );
	EXPECT_EQ( *assigned.at( 3 ), 30 );
	EXPECT_FALSE( assigned.contains( 8 ) );
	source.clear();
	source.emplace( 1, std::make_unique<int>( 10 ) );
	EXPECT_EQ( *source.at( 1 ), 10 );

	det_set<int> sourceSet{ 3, 1 };
	const auto *pSetNode = std::addressof( *sourceSet.begin() );
	auto movedSet = std::move( sourceSet );
	det_set<int> assignedSet{ 8 };
	assignedSet = std::move( movedSet );
	EXPECT_EQ( std::addressof( *assignedSet.find( 3 ) ), pSetNode );
	EXPECT_EQ( Values( assignedSet ), ( std::vector<int>{ 3, 1 } ) );
	EXPECT_TRUE( assignedSet.erase( 3 ) );
	sourceSet.clear();
	sourceSet.insert( 2 );
	EXPECT_TRUE( sourceSet.contains( 2 ) );
}

TEST( DeterministicContainers, CopyAssignmentKeepsDestinationWhenValueCopyThrows )
{
	det_map<int, ThrowOnCopy> source;
	source.emplace( 1, 10 );
	det_map<int, ThrowOnCopy> destination;
	destination.emplace( 2, 20 );
	ThrowOnCopy::bThrow = true;
	EXPECT_THROW( destination = source, std::runtime_error );
	ThrowOnCopy::bThrow = false;
	EXPECT_EQ( Keys( destination ), ( std::vector<int>{ 2 } ) );
	EXPECT_EQ( destination.at( 2 ).nValue, 20 );
	EXPECT_EQ( source.at( 1 ).nValue, 10 );
}

TEST( DeterministicContainers, CopiesPreserveStatefulHashEqualityAndLoadPolicy )
{
	nDefaultModulus = 10;
	det_map<int, int, ModuloHash, ModuloEqual> sourceMap{ { 1, 10 } };
	det_set<int, ModuloHash, ModuloEqual> sourceSet{ 1 };
	sourceMap.max_load_factor( 0.5f );
	sourceSet.max_load_factor( 0.5f );
	nDefaultModulus = 100;
	auto copyMap = sourceMap;
	auto copySet = sourceSet;
	decltype( sourceMap ) assignedMap;
	decltype( sourceSet ) assignedSet;
	assignedMap = sourceMap;
	assignedSet = sourceSet;
	nDefaultModulus = 10;
	EXPECT_EQ( copyMap.at( 11 ), 10 );
	EXPECT_EQ( assignedMap.at( 11 ), 10 );
	EXPECT_TRUE( copySet.contains( 11 ) );
	EXPECT_TRUE( assignedSet.contains( 11 ) );
	EXPECT_FLOAT_EQ( copyMap.max_load_factor(), 0.5f );
	EXPECT_FLOAT_EQ( copySet.max_load_factor(), 0.5f );
	EXPECT_FLOAT_EQ( assignedMap.max_load_factor(), 0.5f );
	EXPECT_FLOAT_EQ( assignedSet.max_load_factor(), 0.5f );
}
