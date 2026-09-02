// Which Granny this claims to be.
//
// The answer is 2.11.8.0, because that is the ABI this library reproduces: the
// structure layouts in Structures.h are the 2.11 ones and the entry points match
// granny211.h's signatures. Saying anything else would be a lie to the one
// caller there is.
//
// granny211.h spells the same four numbers as GrannyProductMajorVersion,
// GrannyProductMinorVersion, GrannyProductBuildNumber and
// GrannyProductCustomization, and GrannyProductVersion as the string. They are
// repeated here rather than included, because this library does not compile
// against that header and a second copy is what lets the two be compared at all.
//
// The one caller is PrintGrannyVersions in ED_B2_M1Dll.cpp, which logs the
// string at editor startup and then evaluates the GrannyVersionsMatch macro,
// which is GrannyVersionsMatch_ over those four constants. Both were measured
// against granny2.dll: the string is exactly "2.11.8.0", and the predicate is
// equality on all four numbers, with no tolerance on the build number and no
// ordering. (2,11,8,0) is true and (2,11,0,8), (2,11,8,1), (2,11,9,0),
// (2,10,8,0) and (3,11,8,0) are all false.

#include <gr2/granny.h>

#include "Trace.h"

namespace
{

constexpr granny_int32x MAJOR_VERSION = 2;
constexpr granny_int32x MINOR_VERSION = 11;
constexpr granny_int32x BUILD_NUMBER = 8;
constexpr granny_int32x CUSTOMIZATION = 0;

const char VERSION_STRING[] = "2.11.8.0";

}

extern "C"
{

GR2_API( char const * ) GrannyGetVersionString( void )
{
	GR2_TRACE( "" );

	return VERSION_STRING;
}

GR2_API( bool ) GrannyVersionsMatch_( granny_int32x MajorVersion, granny_int32x MinorVersion,
                                      granny_int32x BuildNumber, granny_int32x Customization )
{
	GR2_TRACE( "MajorVersion={} MinorVersion={} BuildNumber={} Customization={}", MajorVersion,
	           MinorVersion, BuildNumber, Customization );

	return MajorVersion == MAJOR_VERSION && MinorVersion == MINOR_VERSION
	       && BuildNumber == BUILD_NUMBER && Customization == CUSTOMIZATION;
}

}
