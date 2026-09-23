#include "libdb/ArrayOperations.h"

#include <gtest/gtest.h>
#include <memory>
#include <string>

namespace
{
struct Area
{
	std::string name;
	std::wstring description;
	std::vector<std::string> children;
	std::shared_ptr<int> resource;
};
}

TEST( ArrayOperations, StringsSurviveGrowthAndInsertionBeforeExistingElements )
{
	std::vector<Area> areas;
	const auto *ops = NDb::NMetaInfo::ArrayOperations<Area>();
	// Include both inline and heap-allocated strings. Byte relocation of the
	// former caused the editor's invalid free when removing a script area.
	for ( int i = 0; i < 80; ++i )
	{
		ops->insert( &areas, 0, 1 );
		auto *area = reinterpret_cast<Area*>( ops->element( &areas, 0 ) );
		area->name = i % 2 ? std::string( 80, 'x' ) + std::to_string(i) : std::to_string(i);
		area->description = i % 2 ? std::wstring( 80, L'x' ) : L"abc";
		area->children = { "short", std::string( 100, 'y' ) };
	}
	ASSERT_EQ( ops->size( &areas ), 80 );
	for ( int i = 0; i < 80; ++i )
	{
		const int id = 79 - i;
		EXPECT_EQ( areas[i].name, id % 2 ? std::string( 80, 'x' ) + std::to_string(id) : std::to_string(id) );
		EXPECT_EQ( areas[i].description, id % 2 ? std::wstring( 80, L'x' ) : L"abc" );
		EXPECT_EQ( areas[i].children[1], std::string( 100, 'y' ) );
	}
	ops->remove( &areas, 0, 80 );
	EXPECT_TRUE( areas.empty() );
}

TEST( ArrayOperations, MiddleEraseDestroysOnlyRemovedRecords )
{
	std::vector<Area> areas( 5 );
	std::vector<std::weak_ptr<int>> references;
	for ( int i = 0; i < 5; ++i )
	{
		areas[i].name = std::to_string(i);
		areas[i].resource = std::make_shared<int>( i );
		references.push_back( areas[i].resource );
	}
	const auto *ops = NDb::NMetaInfo::ArrayOperations<Area>();
	ops->remove( &areas, 1, 3 );
	ASSERT_EQ( areas.size(), 2u );
	EXPECT_EQ( areas[0].name, "0" );
	EXPECT_EQ( areas[1].name, "4" );
	EXPECT_FALSE( references[0].expired() );
	EXPECT_FALSE( references[4].expired() );
	for ( int i = 1; i < 4; ++i ) EXPECT_TRUE( references[i].expired() );
	ops->remove( &areas, 0, 2 );
	for ( const auto &reference : references ) EXPECT_TRUE( reference.expired() );
}

TEST( ArrayOperations, SimpleStringArraysUseTheirActualElementType )
{
	std::vector<std::string> strings = { "one", "two", "three" };
	const auto *ops = NDb::NMetaInfo::ArrayOperations<std::string>();
	ops->insert( &strings, 1, 2 );
	*reinterpret_cast<std::string*>( ops->element( &strings, 1 ) ) = "inserted";
	ops->remove( &strings, 2, 2 );
	EXPECT_EQ( strings, (std::vector<std::string>{ "one", "inserted", "three" }) );
}
