// Covers the reference scan that replaced XDBWatcher.
//
// The watcher was a C# tray application that indexed every .xdb in the data
// directory and answered "what points at this object" over a TCP socket through
// .NET Remoting, behind a COM shim. It has not existed on any machine this port
// has run on, which meant renaming an object in the editor silently did nothing
// and the References dialog always came back empty.
//
// The half worth testing is the resolution, because it has to agree exactly
// with BindProcessorSaveLoad's LoadRefFromNode. If it does not, this answers a
// different question from "what breaks if I rename this", which is the only
// reason anyone asks. The production code calls the same MakeFullPath and
// NormalizePath that the loader does, in the same order, so the agreement holds
// by construction; what these check is the part around it -- which attributes
// count as object references at all, and that a malformed file cannot hang the
// scan or read off the end of the buffer.

#include <list>
#include <set>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// FilePath.h names std::list and IBinSaver without including either, expecting
// a stdafx to have got there first. This is that prelude, in the order
// libdb/stdafx.h uses it.
#include "Misc/Asserts.h"
#include "System/System.h"
#include "Misc/Tools.h"
#include "System/Basic.h"
#include "System/Streams.h"
#include "System/BinSaver.h"
#include "System/FilePath.h"
#include "libdb/DBReferenceScan.h"

#include <gtest/gtest.h>

namespace {

std::vector<std::string> Scan( const std::string &szText, const std::string &szOwner )
{
	std::vector<std::string> res;
	NDb::CollectObjectReferences( &res, szText.data(), szText.data() + szText.size(), szOwner );
	return res;
}

//! Compares with the separator convention the engine normalises to, so the test
//! does not care whether that is a slash or a backslash.
std::string Norm( const std::string &sz )
{
	std::string res;
	NFile::NormalizePath( &res, sz );
	return res;
}

const char szOwner[] = "Bridges/Africa/bridge.xdb";

} // namespace

// The common shape in the shipped data: an absolute reference, which
// MakeFullPath turns into a name with the leading separator dropped.
TEST( DBReferenceScan, AbsoluteReference )
{
	const std::vector<std::string> refs = Scan(
		"<Bridge><RPGStats href=\"/Bridges/Africa/bridgerpgstats.xdb#xpointer(/BridgeRPGStats)\"/></Bridge>",
		szOwner );
	ASSERT_EQ( 1u, refs.size() );
	EXPECT_EQ( Norm( "Bridges/Africa/bridgerpgstats.xdb" ), refs[0] );
}

// SaveRefToNode writes a bare file name when the target sits beside its owner.
TEST( DBReferenceScan, ReferenceRelativeToItsOwner )
{
	const std::vector<std::string> refs = Scan(
		"<Bridge><RPGStats href=\"bridgerpgstats.xdb#xpointer(/BridgeRPGStats)\"/></Bridge>",
		szOwner );
	ASSERT_EQ( 1u, refs.size() );
	EXPECT_EQ( Norm( "Bridges/Africa/bridgerpgstats.xdb" ), refs[0] );
}

// The distinction the whole scan rests on. A file path field is written with no
// fragment, an object reference always carries #xpointer(/Class), and only the
// second kind is a database reference. Counting the .tga would make a texture
// look like a database object.
TEST( DBReferenceScan, PlainFilePathsAreNotObjectReferences )
{
	const std::vector<std::string> refs = Scan(
		"<Obj>"
		"<Texture href=\"/Bridges/AfricaBridge/c1.tga\"/>"
		"<Model href=\"/Bridges/AfricaBridge/c1.mb\"/>"
		"<Name href=\"/Bridges/Africa/name.txt\"/>"
		"<Stats href=\"/Bridges/Africa/bridgerpgstats.xdb#xpointer(/BridgeRPGStats)\"/>"
		"</Obj>",
		szOwner );
	ASSERT_EQ( 1u, refs.size() );
	EXPECT_EQ( Norm( "Bridges/Africa/bridgerpgstats.xdb" ), refs[0] );
}

// An unset reference field is written as an empty href, which is neither an
// object reference nor an error.
TEST( DBReferenceScan, EmptyHRefIsIgnored )
{
	EXPECT_TRUE( Scan( "<Obj><Ref href=\"\"/></Obj>", szOwner ).empty() );
	EXPECT_TRUE( Scan( "<Obj/>", szOwner ).empty() );
	EXPECT_TRUE( Scan( "", szOwner ).empty() );
}

TEST( DBReferenceScan, SeveralReferencesInOneFile )
{
	const std::vector<std::string> refs = Scan(
		"<Obj>"
		"<A href=\"/a/one.xdb#xpointer(/One)\"/>"
		"<B href=\"two.xdb#xpointer(/Two)\"/>"
		"<C href=\"/a/b/three.xdb#xpointer(/Three)\"/>"
		"</Obj>",
		szOwner );
	ASSERT_EQ( 3u, refs.size() );
	EXPECT_EQ( Norm( "a/one.xdb" ), refs[0] );
	EXPECT_EQ( Norm( "Bridges/Africa/two.xdb" ), refs[1] );
	EXPECT_EQ( Norm( "a/b/three.xdb" ), refs[2] );
}

// The scan reads files it did not write -- a half saved .xdb, or one being
// rewritten while the scan runs -- so an unterminated attribute has to end the
// scan rather than run past the end of the buffer or spin.
TEST( DBReferenceScan, TruncatedInputTerminates )
{
	EXPECT_TRUE( Scan( "<Obj><Ref href=\"/a/one.xdb#xpointer(/One)", szOwner ).empty() );
	EXPECT_TRUE( Scan( "<Obj><Ref href=", szOwner ).empty() );
	EXPECT_TRUE( Scan( "href=\"", szOwner ).empty() );

	// A good reference before the truncation is still reported.
	const std::vector<std::string> refs = Scan(
		"<Obj><A href=\"/a/one.xdb#xpointer(/One)\"/><B href=\"/a/two.xdb", szOwner );
	ASSERT_EQ( 1u, refs.size() );
	EXPECT_EQ( Norm( "a/one.xdb" ), refs[0] );
}

// The owner's name is what a relative reference resolves against, so a name
// with no directory in it must not walk off the front of the string.
TEST( DBReferenceScan, OwnerWithNoDirectory )
{
	const std::vector<std::string> refs = Scan(
		"<Obj><A href=\"two.xdb#xpointer(/Two)\"/></Obj>", "one.xdb" );
	ASSERT_EQ( 1u, refs.size() );
	EXPECT_EQ( Norm( "two.xdb" ), refs[0] );
}

// Backslashes appear in these files as often as slashes, and the database
// treats the two as the same separator.
TEST( DBReferenceScan, BackslashSeparators )
{
	const std::vector<std::string> refs = Scan(
		"<Obj><A href=\"\\Bridges\\Africa\\bridgerpgstats.xdb#xpointer(/BridgeRPGStats)\"/></Obj>",
		"Bridges\\Africa\\bridge.xdb" );
	ASSERT_EQ( 1u, refs.size() );
	EXPECT_EQ( Norm( "Bridges/Africa/bridgerpgstats.xdb" ), refs[0] );
}
