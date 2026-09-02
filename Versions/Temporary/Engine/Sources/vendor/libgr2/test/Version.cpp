// The version pair, against what granny2.dll was measured to say.
//
// Both were read off the DLL through ctypes: GrannyGetVersionString returns
// exactly "2.11.8.0", and GrannyVersionsMatch_ is equality on all four numbers.
// The numbers below are granny211.h's GrannyProductMajorVersion,
// GrannyProductMinorVersion, GrannyProductBuildNumber and
// GrannyProductCustomization, which is what the GrannyVersionsMatch macro in
// the map editor passes.

#include <gr2/granny.h>

#include <cstring>
#include <string>

#include <gtest/gtest.h>

TEST( Version, StringIsTheProductVersion )
{
	// Not a version of libgr2's own. The engine and the editor were compiled
	// against granny211.h and this is the ABI that answers them.
	ASSERT_NE( nullptr, GrannyGetVersionString() );
	EXPECT_EQ( std::string( "2.11.8.0" ), std::string( GrannyGetVersionString() ) );
}

TEST( Version, StringIsStable )
{
	// It is returned by pointer and the caller does not own it, so two calls
	// have to hand back the same storage rather than a rebuilt buffer.
	EXPECT_EQ( GrannyGetVersionString(), GrannyGetVersionString() );
}

TEST( Version, MatchesTheHeaderTheEditorCompiledAgainst )
{
	// GrannyVersionsMatch expands to exactly this call.
	EXPECT_TRUE( GrannyVersionsMatch_( 2, 11, 8, 0 ) );
}

TEST( Version, EveryFieldHasToAgree )
{
	// No tolerance on the build number, and no ordering: a newer build is not a
	// match either. Each of these was false against the DLL.
	EXPECT_FALSE( GrannyVersionsMatch_( 3, 11, 8, 0 ) );
	EXPECT_FALSE( GrannyVersionsMatch_( 2, 10, 8, 0 ) );
	EXPECT_FALSE( GrannyVersionsMatch_( 2, 11, 9, 0 ) );
	EXPECT_FALSE( GrannyVersionsMatch_( 2, 11, 8, 1 ) );
	EXPECT_FALSE( GrannyVersionsMatch_( 0, 0, 0, 0 ) );
}

TEST( Version, BuildAndCustomizationAreNotInterchangeable )
{
	// The parameter order is Major, Minor, Build, Customization, so 2.11.8.0
	// passed as 2, 11, 0, 8 is a different version and does not match. This is
	// what catches the two being swapped.
	EXPECT_FALSE( GrannyVersionsMatch_( 2, 11, 0, 8 ) );
}
