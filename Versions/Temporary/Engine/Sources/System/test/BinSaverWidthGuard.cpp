// Pins which types IBinSaver::Add will refuse to serialize.
//
// Its scalar path is DataChunk( id, p, sizeof( T ) ), so the width of the
// member is part of the save, the replay and the packet. Twelve members in this
// tree were declared long, unsigned long, uLong or size_t, which are four bytes
// on Windows and eight on Linux, and each of them made a file or a packet the
// other platform could not read. Every one was found by compiling the whole
// tree with this guard in place rather than by reading code, so the guard is
// what keeps the thirteenth from being written.
//
// These are static_asserts because the thing under test is a decision the
// compiler makes, and the cases that must be REFUSED are the point: a predicate
// that answered false to everything would pass every build in silence.
//
// The asymmetry is the interesting part. On MSVC, long is four bytes and a
// distinct type from int32_t and int64_t, so refusing it catches exactly the
// accidental uses. On LP64 Linux, long IS int64_t, so refusing it would reject
// every deliberate 64-bit member. The trait takes that as a parameter so both
// answers can be pinned here, on one platform.

#include "System/BinSaver.h"

#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{

template <class T>
constexpr bool Refused()
{
	return NBinSaverDetail::TIsWidthVarying<T>::value;
}

template <class T, bool bLongDiffers>
constexpr bool RefusedWhere()
{
	return NBinSaverDetail::TIsWidthVarying<T, bLongDiffers>::value;
}

// The fixed-width types are what a serialized member should be, on every
// platform.
static_assert( !Refused<int8_t>(), "int8_t is one byte everywhere" );
static_assert( !Refused<uint8_t>(), "uint8_t is one byte everywhere" );
static_assert( !Refused<int16_t>(), "int16_t is two bytes everywhere" );
static_assert( !Refused<uint16_t>(), "uint16_t is two bytes everywhere" );
static_assert( !Refused<int32_t>(), "int32_t is four bytes everywhere" );
static_assert( !Refused<uint32_t>(), "uint32_t is four bytes everywhere" );
static_assert( !Refused<int64_t>(), "int64_t is eight bytes everywhere" );
static_assert( !Refused<uint64_t>(), "uint64_t is eight bytes everywhere" );

// And so are the types the engine has always serialized and that do not vary
// across its targets: int is 32 bits, bool is one byte, float and double are
// IEEE 754 on x86 and x64 with every compiler here.
static_assert( !Refused<int>(), "int is four bytes on every target here" );
static_assert( !Refused<unsigned>(), "unsigned is four bytes on every target here" );
static_assert( !Refused<short>(), "short is two bytes on every target here" );
static_assert( !Refused<bool>(), "bool is one byte on every target here" );
static_assert( !Refused<float>(), "float is binary32 on every target here" );
static_assert( !Refused<double>(), "double is binary64 on every target here" );
static_assert( !Refused<char>(), "char is one byte by definition" );

// Class types go through their own operator& and are not the guard's business.
static_assert( !Refused<std::string>(), "a string has its own chunk form" );
static_assert( !Refused<std::wstring>(), "a wide string is converted to UTF-16LE, not written raw" );
static_assert( !Refused<std::vector<int>>(), "a vector is serialized element by element" );

// The refusals, which are why this file exists.
static_assert( Refused<wchar_t>(), "wchar_t is two bytes on Windows and four on Linux" );
static_assert( Refused<long double>(), "long double is eight, ten or sixteen bytes depending on the target" );

// cv-qualified spellings must answer the same way, since Add deduces T from the
// pointer it is handed.
static_assert( Refused<const wchar_t>(), "constness does not change the width" );
static_assert( Refused<volatile wchar_t>(), "volatility does not change the width" );
static_assert( !Refused<const int32_t>(), "constness does not make a fixed-width type suspect" );

// Both halves of the long question, pinned from one platform. The first is what
// the MSVC build sees and enforces; the second is what a Linux build has to
// accept, because there the spelling is int64_t.
static_assert( RefusedWhere<long, true>(), "on MSVC long is four bytes and must be refused" );
static_assert( RefusedWhere<unsigned long, true>(), "on MSVC unsigned long is four bytes and must be refused" );
static_assert( !RefusedWhere<long, false>(), "on LP64 long is int64_t and cannot be refused" );
static_assert( !RefusedWhere<unsigned long, false>(), "on LP64 unsigned long is uint64_t and cannot be refused" );
static_assert( RefusedWhere<wchar_t, false>(), "wchar_t is refused whatever long is" );

// And the knob must not change the answer for a type that is distinct from
// long on every target.
static_assert( !RefusedWhere<int32_t, true>(), "the long knob must not touch int32_t" );
static_assert( !RefusedWhere<int, true>(), "the long knob must not touch int" );

// int64_t is deliberately not asserted either way, and the reason is the whole
// point of the asymmetry: with the knob on, this compiler refuses int64_t too,
// because on LP64 int64_t and long are the same type and nothing can tell them
// apart. On MSVC, where int64_t is long long, it would be accepted. So the
// answer here depends on the platform the test runs on, which makes it the one
// thing in this file that cannot be pinned, and it is why the knob defaults to
// off everywhere except MSVC. An earlier draft of this file asserted that
// int64_t is accepted with the knob on and failed exactly this way.

TEST( BinSaverWidthGuard, TheTraitIsCheckedAtCompileTime )
{
	// The assertions above are the test. This case exists so the binary has
	// something to run and so a build that quietly stopped compiling the file
	// would show up as a missing test rather than as nothing at all.
	EXPECT_FALSE( Refused<uint32_t>() );
	EXPECT_TRUE( Refused<wchar_t>() );
}

} // namespace
