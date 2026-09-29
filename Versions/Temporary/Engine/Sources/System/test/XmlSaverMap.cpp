// How the XML saver writes a std::map: in key order, and in the Item/Key/Data
// layout it has always used for a hash map, so that types.xml, whose attribute
// lists are maps, comes out the same whichever standard library wrote it and
// still reads into either container.

// The standard headers the engine's stdafx.h prelude would supply, which
// XmlSaver.h relies on without including
#include <cmath>
#include <cstdint>
#include <cstring>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Misc/Asserts.h"
#include "Misc/Tools.h"
#include "System/System.h"
#include "System/Basic.h"
#include "Misc/Geom.h"
#include "System/Streams.h"
#include "System/BinSaver.h"
#include "System/XmlResource.h"
#include "System/XmlSaver.h"

#include <gtest/gtest.h>

namespace {

template <class TContainer>
std::string Save( TContainer container )
{
	CMemoryStream stream;
	{
		CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_WRITE );
		pSaver->Add( "Map", &container );
	}
	return std::string( stream.GetBuffer(), stream.GetBuffer() + stream.GetSize() );
}

template <class TContainer>
TContainer Load( const std::string &szText )
{
	CMemoryStream stream;
	stream.Write( szText.data(), static_cast<int>( szText.size() ) );
	stream.Seek( 0 );
	TContainer container;
	{
		CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_READ );
		pSaver->Add( "Map", &container );
	}
	return container;
}

// The keys in the order they have to appear in the text.
std::vector<std::string> KeysInText( const std::string &szText )
{
	std::vector<std::string> keys;
	for ( size_t nPos = szText.find( "<Key>" ); nPos != std::string::npos; nPos = szText.find( "<Key>", nPos + 1 ) )
	{
		const size_t nStart = nPos + 5;
		keys.push_back( szText.substr( nStart, szText.find( "</Key>", nStart ) - nStart ) );
	}
	return keys;
}

using TMap = std::map<std::string, int>;
using THashMap = std::unordered_map<std::string, int>;

const TMap attributes = {
	{ "typeRename", 1 }, { "typePrefix", 2 }, { "hidden", 3 }, { "export", 4 }, { "chunkID", 5 } };

TEST( XmlSaverMap, WritesInKeyOrder )
{
	const std::vector<std::string> expected = { "chunkID", "export", "hidden", "typePrefix", "typeRename" };
	EXPECT_EQ( KeysInText( Save( attributes ) ), expected );
}

// The order does not depend on the order the keys went in.
TEST( XmlSaverMap, SameContentSavesToSameText )
{
	std::map<std::string, int> reversed;
	for ( auto it = attributes.rbegin(); it != attributes.rend(); ++it )
	{
		reversed.insert( *it );
	}
	EXPECT_EQ( Save( reversed ), Save( attributes ) );
}

TEST( XmlSaverMap, LoadsBack )
{
	EXPECT_EQ( Load<TMap>( Save( attributes ) ), attributes );
}

// types.xml files written while the attributes were a hash map still load, and
// the other way round.
TEST( XmlSaverMap, SharesItsLayoutWithHashMaps )
{
	const std::unordered_map<std::string, int> hashed( attributes.begin(), attributes.end() );
	EXPECT_EQ( Load<TMap>( Save( hashed ) ), attributes );
	EXPECT_EQ( Load<THashMap>( Save( attributes ) ), hashed );
}

} // namespace
