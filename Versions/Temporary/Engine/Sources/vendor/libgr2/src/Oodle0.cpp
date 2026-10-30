// Oodle0 decompression.
//
// PROVENANCE. A derived work, not an independent implementation. The algorithm
// comes from blendergranny's io_scene_gr2/gr2/decompress/oodle0.py, which is MIT
// licensed, Copyright (c) 2026 ciupix and contributors, and which carries an
// explicit clean-room policy in its docs/PROVENANCE.md: behaviour studied from
// private references, code written fresh, notes kept at the algorithm level. That
// policy is the reason this is the version to derive from rather than
// granny-ro-js's port of it, whose comments cite a leaked RAD source path.
//
//   https://github.com/ciupix/blendergranny  (io_scene_gr2/gr2/decompress/oodle0.py)
//
// Its Oodle0 was checked against the real DLL over 15,742 files with identical
// SHA-256 on every one, so it is a reference worth being faithful to.
//
// WHAT CHANGED from that reference, and why:
//
//  - Python integers are unbounded and C++ ones are not, so every place the
//    original relies on that is written out: the range coder's products need 64
//    bits, and the model's total bins are explicitly 16-bit lanes packed two to a
//    32-bit add, which is what the arithmetic in the reference is modelling.
//  - Every read and write is bounds checked and failure is a return value rather
//    than an exception, because this runs on whatever a .pak contains.
//  - The bit reader takes its buffer's length, so running off the end is a
//    refusal rather than reading zeros forever.
//
// HOW IT WORKS. Three layers.
//
// A VARIABLE BIT READER pulls little-endian 32-bit words and hands out bits from
// the low end.
//
// A RANGE CODER sits on that, with a 31-bit low, high and code. It is the
// unusual part: the code register is filled with bit-reversed input, whole
// nibbles at a time when it renormalises by eight, which is a detail no amount of
// reasoning recovers and which has to be copied exactly.
//
// An ADAPTIVE MODEL sits on that. Symbols are counted as they appear, and a
// symbol the model has never seen is coded as an escape at position zero followed
// by the raw value, which is why every read here is "ask the model, and if it
// escapes, read the value and teach it". Counts are kept in sixteen cumulative
// bins so the coder's scale is one lookup, and halved when they grow past 16,384
// so recent statistics dominate.
//
// The LZ LAYER above chooses per symbol between a literal and a back reference.
// A length of zero means a literal. Lengths come from one of 65 models selected
// by the previous length, so runs of similar lengths cost almost nothing, and the
// four highest codes mean 128, 192, 256 and 512. An offset is split at two bits,
// low part and high part, each from its own model.

#include "Oodle0.h"

#include "Trace.h"

// Branch hints. Every test that guards a division here is overwhelmingly one
// way, measured zero out of 628,690,344 over the corpus, and GCC lays the code
// out backwards without being told.
#if defined( __GNUC__ )
#	define OODLE0_LIKELY( x ) __builtin_expect( !!( x ), 1 )
#else
#	define OODLE0_LIKELY( x ) ( x )
#endif

#include <cstring>
#include <new>
#include <vector>

// SSE2 is used to pick one of the model's sixteen cumulative bins with two
// compares instead of a scan. It is architecturally guaranteed on x86-64 and on
// MSVC x86 (which defaults to /arch:SSE2), so no runtime dispatch is needed; the
// scalar fallback below exists only for a build that really has no SSE2.
#if defined( _M_X64 ) || defined( _M_AMD64 ) || defined( __x86_64__ ) || defined( __SSE2__ ) \
	|| ( defined( _M_IX86_FP ) && _M_IX86_FP >= 2 )
#define GR2_OODLE0_SSE2 1
#include <emmintrin.h>
#else
#define GR2_OODLE0_SSE2 0
#endif

#if defined( _MSC_VER )
#include <intrin.h>
#endif

#if defined( _MSC_VER )
// _byteswap_ulong and _BitScanReverse, both of which are intrinsics rather than
// library calls. Neither needs an instruction set beyond the baseline.
#include <intrin.h>
#endif

namespace NGr2
{

namespace
{

#if GR2_OODLE0_SSE2
//! Sixteen lane suffix masks for all sixteen bins, overlapped into one array.
//!
//! The window of sixteen entries starting at SUFFIX_ONES + 16 - b is all ones
//! from lane b upwards and zero below it, so this one 64-byte table serves every
//! bin and the bin index becomes an address offset rather than a branch.
alignas( 64 ) const uint16_t SUFFIX_ONES[32] = {
	0,      0,      0,      0,      0,      0,      0,      0,
	0,      0,      0,      0,      0,      0,      0,      0,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff };
#endif

//! Index of the lowest set bit. Undefined for zero, which no caller passes.
inline uint32_t LowestSetBit( uint32_t nMask )
{
#if defined( _MSC_VER )
	unsigned long nIndex = 0;
	_BitScanForward( &nIndex, nMask );
	return static_cast<uint32_t>( nIndex );
#else
	return static_cast<uint32_t>( __builtin_ctz( nMask ) );
#endif
}

//! A 64-by-32 unsigned division whose quotient is known to fit in 32 bits.
//!
//! Every division in this decoder has a 64-bit dividend, a divisor that fits in
//! 32 bits, and a quotient that fits in 32 bits (see DivNarrow for why). C++ has
//! no way to say that: `uint64_t / uint32_t` promotes both operands and the
//! compiler emits a full 64-bit divide, which on this hardware occupies the
//! divider for about 1.7 times as long. x86's DIV with a 32-bit operand already
//! takes a 64-bit dividend in edx:eax, so the narrow form is one instruction; it
//! simply cannot be reached from portable C++.
//!
//! MSVC x86 has no intrinsic for it, so it is not offered there and the guard
//! below compiles away: that build keeps exactly the code it had.
#if ( defined( __GNUC__ ) && ( defined( __x86_64__ ) || defined( __i386__ ) ) ) \
	|| ( defined( _MSC_VER ) && defined( _M_X64 ) )
#	define OODLE0_NARROW_DIV 1
#endif

#ifdef OODLE0_NARROW_DIV
inline uint32_t DivQuot64By32( uint64_t nDividend, uint32_t nDivisor )
{
#	if defined( __GNUC__ )
	uint32_t nQuotient;
	uint32_t nRemainder;
	__asm__( "divl %4"
	         : "=a"( nQuotient ), "=d"( nRemainder )
	         : "a"( static_cast<uint32_t>( nDividend ) ),
	           "d"( static_cast<uint32_t>( nDividend >> 32 ) ), "rm"( nDivisor )
	         : "cc" );
	return nQuotient;
#	else
	uint32_t nRemainder;
	return _udiv64( nDividend, nDivisor, &nRemainder );
#	endif
}
#endif

//! floor( nDividend / nDivisor ), exactly, for any inputs with nDivisor != 0.
//!
//! WHY THE TEST. x86's narrow DIV raises #DE rather than truncating when the
//! quotient will not fit in 32 bits, so it may only be issued when the high word
//! of the dividend is below the divisor, which is exactly that condition. The
//! invariants the coder maintains make it always true here: both dividends are
//! width * n with n at most the scale, so the quotient is at most the width,
//! which is at most 2^31. Measured over the 37,640 section corpus the fallback
//! is taken zero times out of 628,690,344 divisions. It exists so that a corrupt
//! stream that broke the invariant would get a wrong byte, as it already would,
//! rather than a hardware exception.
//!
//! The divisor is taken as 64 bits because one caller's can be 2^32: a broken
//! stream can leave low exactly one past high, and then high - low + 1 does not
//! fit. The truncation to 32 bits is zero in that case, no dividend can be below
//! it, and the fallback divides by the real value.
inline uint64_t DivNarrow( uint64_t nDividend, uint64_t nDivisor )
{
#ifdef OODLE0_NARROW_DIV
	const uint32_t nDivisor32 = static_cast<uint32_t>( nDivisor );
	if ( OODLE0_LIKELY( ( nDividend >> 32 ) < nDivisor32 ) )
	{
		return DivQuot64By32( nDividend, nDivisor32 );
	}
#endif
	return nDividend / nDivisor;
}

//! RECIPROCALS INSTEAD OF DIVIDES, for the two narrowings in Remove.
//!
//! They share a divisor, which is the coder's scale, and that divisor is small:
//! it is a sum of 16-bit counts that rescaling holds under 16,386, and measured
//! over the corpus it is below 16,384 on 99.90% of calls. Small enough to look
//! up, which is what makes this different from computing a reciprocal per
//! symbol: computing one needs a division, so it could never pay.
//!
//! THE IDENTITY, and why it is exact. For each divisor d >= 2 the table holds
//!
//!     M(d) = floor( ( 2^64 - 1 ) / d ) + 1,
//!
//! which is ceil( 2^64 / d ) for every d: writing 2^64 = q d + r, if r > 0 then
//! floor( ( 2^64 - 1 ) / d ) is q and the sum is q + 1, and if r == 0, which is
//! d a power of two, it is q - 1 and the sum is q. Written the other way,
//! M d = 2^64 + e with 0 <= e <= d - 1. Then for any dividend n,
//!
//!     n M / 2^64 = n / d + n e / ( d 2^64 ),
//!
//! so the multiply overshoots the true quotient by n e / ( d 2^64 ) and nothing
//! else. Let k = floor( n / d ). The real quotient n / d is at least k and at
//! most k + 1 - 1/d, because ( k + 1 ) d - n is a positive integer. So the
//! product's floor is k provided the overshoot is under 1/d, which is
//!
//!     n e < 2^64.
//!
//! Here e <= d - 1 < 16384 and the test below refuses any dividend at or above
//! 2^48, so n e < 2^48 * 2^14 = 2^62. Four bits of margin, and the real
//! dividends are smaller still: width is at most 2^31 and start + count is at
//! most the scale, so n < 2^45.
//!
//! So no correction step is needed, unlike a floating point reciprocal, which
//! rounds twice and lands below an integer quotient often enough to matter.
//! s9_recip_probe.cpp has the counts.
constexpr uint32_t RECIP_MAX = 16384;

#if defined( __SIZEOF_INT128__ ) || ( defined( _MSC_VER ) && defined( _M_X64 ) )
#	define OODLE0_MUL_HIGH 1
#endif

#ifdef OODLE0_MUL_HIGH
//! The high half of a 64 by 64 product. One instruction on x64.
inline uint64_t MulHigh( uint64_t nA, uint64_t nB )
{
#	if defined( __SIZEOF_INT128__ )
	return static_cast<uint64_t>( ( static_cast<unsigned __int128>( nA ) * nB ) >> 64 );
#	else
	return __umulh( nA, nB );
#	endif
}

struct SReciprocals
{
	//! Indexed by the divisor. Entries 0 and 1 are unused; the caller's test
	//! excludes them, since M(1) would be 2^64 and does not fit.
	uint64_t nMagic[RECIP_MAX];

	constexpr SReciprocals()
		: nMagic()
	{
		for ( uint32_t d = 2; d < RECIP_MAX; ++d )
		{
			nMagic[d] = 0xffffffffffffffffull / d + 1;
		}
	}
};

constexpr SReciprocals RECIPROCALS;
#endif

constexpr uint32_t HEADER_SIZE = 36;
constexpr uint32_t STAGE_COUNT = 3;
constexpr uint32_t OFFSET_SPLIT_SHIFT = 2;
constexpr uint32_t LOW_OFFSET_MASK = ( 1u << OFFSET_SPLIT_SHIFT ) - 1;
constexpr uint32_t MAX_LENGTHS = 64;
constexpr uint32_t MASK31 = 0x7fffffffu;

//! The four longest length codes, in place of a count.
constexpr uint32_t LONG_LENGTHS[4] = { MAX_LENGTHS * 2, MAX_LENGTHS * 3, MAX_LENGTHS * 4,
                                       MAX_LENGTHS * 8 };

//! Counts are halved once their total reaches this.
constexpr uint32_t RESCALE_TOTAL = 16384;

//! Byte-order swap, so the bit reversal below is one instruction plus three
//! SWAR steps rather than a loop.
inline uint32_t ByteSwap32( uint32_t nValue )
{
#if defined( _MSC_VER )
	return _byteswap_ulong( nValue );
#elif defined( __GNUC__ ) || defined( __clang__ )
	return __builtin_bswap32( nValue );
#else
	return ( nValue >> 24 ) | ( ( nValue >> 8 ) & 0x0000ff00u )
	       | ( ( nValue << 8 ) & 0x00ff0000u ) | ( nValue << 24 );
#endif
}

//! Leading zero count. Never called with zero, so bsr is enough and no LZCNT
//! feature bit is relied on.
inline uint32_t CountLeadingZeros32( uint32_t nValue )
{
#if defined( _MSC_VER )
	unsigned long nIndex = 0;
	_BitScanReverse( &nIndex, nValue );
	return 31u - nIndex;
#elif defined( __GNUC__ ) || defined( __clang__ )
	return static_cast<uint32_t>( __builtin_clz( nValue ) );
#else
	uint32_t nCount = 0;
	while ( ( nValue & 0x80000000u ) == 0 )
	{
		++nCount;
		nValue <<= 1;
	}
	return nCount;
#endif
}

//! Bits out of a little-endian word stream, earliest bit first.
//!
//! The original reader handed out bits from the low end of the word and the
//! range coder then reversed every nibble on the way into the code register,
//! because the code register is filled most significant bit first. Those two
//! reversals are the same reversal: the whole stream is one MSB-first bit
//! source, and the nibble swapping in the old renormalise loop was only there
//! because it pulled whole bytes out of an LSB-first reader.
//!
//! So the accumulator is kept pre-reversed. One word is reversed as it is read,
//! a byte swap and three SWAR steps for all thirty-two bits, where the old
//! renormalise paid two unrolled four-bit reversal loops, about twenty-four
//! operations, for every single byte. A read of n bits is then a shift.
//!
//! Word granularity, the zero padding and the failure threshold are unchanged
//! from the original, and so is the bit accounting: m_nBitLen evolves exactly as
//! it did, so a word is read at exactly the same point in the stream and
//! Failed() flips on exactly the same input. The behaviour is identical on
//! malformed streams, not merely on the ones that decode.
class CVarBits
{
public:
	CVarBits( const uint8_t *pData, uint32_t nSize, uint32_t nAt )
		: m_pData( pData )
		, m_nSize( nSize )
		, m_nAt( nAt )
	{
	}

	//! The next nBits bits, earliest bit in the highest position. nBits <= 31.
	uint32_t Get( uint32_t nBits )
	{
		if ( m_nBitLen < nBits )
		{
			// m_nBitLen is at most 30 here, so this shift is 2..32 and the
			// accumulator never holds more than 62 bits.
			m_Acc |= uint64_t( ReadWordReversed() ) << ( 32 - m_nBitLen );
			m_nBitLen += 32;
		}
		// Two shifts rather than one by 64 - nBits, which would be undefined
		// when nBits is zero, and zero bits is the common case.
		const uint32_t nValue = static_cast<uint32_t>( ( m_Acc >> ( 63 - nBits ) ) >> 1 );
		m_Acc <<= nBits;
		m_nBitLen -= nBits;
		return nValue;
	}

	bool Failed() const { return m_bFailed; }

private:
	//! The next word with all thirty-two bits reversed, zero padded past the
	//! end, which the format relies on.
	//!
	//! Reading past the end is normal at the tail and is not an error by itself.
	//! Reading far past it is, and that is what the limit catches.
	uint32_t ReadWordReversed();

	const uint8_t *m_pData = nullptr;
	uint32_t m_nSize = 0;
	uint32_t m_nAt = 0;
	//! Valid bits sit at the top, bit 63 being the next one out.
	uint64_t m_Acc = 0;
	uint32_t m_nBitLen = 0;
	bool m_bFailed = false;
};

uint32_t CVarBits::ReadWordReversed()
{
	// Eight words of slack, which is more than any renormalisation can consume
	// after the last real byte.
	if ( m_nAt > m_nSize + 32 )
	{
		m_bFailed = true;
		return 0;
	}

	uint8_t bytes[4] = { 0, 0, 0, 0 };
	for ( uint32_t i = 0; i < 4; ++i )
	{
		if ( m_nAt + i < m_nSize )
		{
			bytes[i] = m_pData[m_nAt + i];
		}
	}
	m_nAt += 4;

	uint32_t nWord = 0;
	memcpy( &nWord, bytes, sizeof( nWord ) );

	// Reverse all thirty-two bits: the byte order, then the bits within each
	// byte. The stream is bytes in order, each read from its low bit, so the
	// earliest bit of the earliest byte ends up at bit 31.
	nWord = ByteSwap32( nWord );
	nWord = ( ( nWord & 0xaaaaaaaau ) >> 1 ) | ( ( nWord & 0x55555555u ) << 1 );
	nWord = ( ( nWord & 0xccccccccu ) >> 2 ) | ( ( nWord & 0x33333333u ) << 2 );
	nWord = ( ( nWord & 0xf0f0f0f0u ) >> 4 ) | ( ( nWord & 0x0f0f0f0fu ) << 4 );
	return nWord;
}

//! The range coder.
class CArithBits
{
public:
	CArithBits( const uint8_t *pData, uint32_t nSize, uint32_t nAt )
		: m_Bits( pData, nSize, nAt )
	{
		m_nCode = m_Bits.Get( 31 );
	}

	//! Where the code sits within a range of nScale, before narrowing.
	uint32_t GetCount( uint32_t nScale ) const;

	//! Narrow to [nStart, nStart + nCount) of nScale, and refill.
	void Remove( uint32_t nStart, uint32_t nCount, uint32_t nScale );

	//! One exact value out of nScale, which is how a raw escape is read.
	uint32_t GetValue( uint32_t nScale );

	bool Failed() const { return m_Bits.Failed(); }

private:
	CVarBits m_Bits;
	uint32_t m_nHigh = MASK31;
	uint32_t m_nLow = 0;
	uint32_t m_nCode = 0;
};

uint32_t CArithBits::GetCount( uint32_t nScale ) const
{
	if ( nScale == 0 )
	{
		return 0;
	}
	// 64-bit because the product is up to 2^31 times the scale. This is the one
	// line where Python's unbounded integers were doing real work.
	const uint64_t nRange = uint64_t( m_nHigh - m_nLow ) + 1;
	const uint64_t nOffset = uint64_t( m_nCode - m_nLow ) + 1;
	return static_cast<uint32_t>( DivNarrow( nOffset * nScale - 1, nRange ) );
}

void CArithBits::Remove( uint32_t nStart, uint32_t nCount, uint32_t nScale )
{
	if ( nScale == 0 )
	{
		return;
	}

	uint32_t nHigh = m_nHigh;
	uint32_t nLow = m_nLow;

	const uint64_t nWidth = uint64_t( nHigh - nLow ) + 1;
	const uint64_t nNumHigh = nWidth * ( uint64_t( nStart ) + nCount );
	const uint64_t nNumLow = nWidth * nStart;
#ifdef OODLE0_MUL_HIGH
	// One test for both dividends, because the second is never the larger, and
	// one table load for both, because they share the divisor. Non short
	// circuited so this is one branch rather than two, and it is overwhelmingly
	// taken: 99.90% of calls by the scale, every call by the dividend.
	if ( OODLE0_LIKELY( ( nScale - 2u < RECIP_MAX - 2u )
	                    & ( ( nNumHigh >> 48 ) == 0 ) ) )
	{
		const uint64_t nMagic = RECIPROCALS.nMagic[nScale];
		nHigh = static_cast<uint32_t>( nLow + MulHigh( nNumHigh, nMagic ) - 1 );
		nLow = static_cast<uint32_t>( nLow + MulHigh( nNumLow, nMagic ) );
	}
	else
#endif
	{
		nHigh = static_cast<uint32_t>( nLow + DivNarrow( nNumHigh, nScale ) - 1 );
		nLow = static_cast<uint32_t>( nLow + DivNarrow( nNumLow, nScale ) );
	}

	// Renormalise, and absorb the underflow loop into the same shift.
	//
	// The original walked four data-dependent loops to find out how far to shift:
	// eight bits at a time, then one step of four, then one bit at a time, then
	// the underflow loop. Each of those trip counts is a leading bit count, so
	// all of it is two counts and one shift. proof_rev0.cpp checks the identities
	// below against the original loops over 11.5 million states, including the
	// degenerate ones where the narrowed range inverts.
	//
	// nZ, the renormalise distance, is the length of the agreeing prefix of
	// nHigh and nLow in bits 30..0. The 8-loop runs nZ >> 3 times, the 4-step
	// runs when bit 2 of nZ is set and the 1-loop runs nZ & 3 times, which is
	// 8 * (nZ >> 3) + 4 * ((nZ >> 2) & 1) + (nZ & 3) == nZ. The gate on bit 30 is
	// just nZ == 0, so there is nothing left to branch on. The (x << 1) | 1 keeps
	// the count defined when the two agree completely, where the answer is 31.
	const uint32_t nZ = CountLeadingZeros32( ( ( nHigh ^ nLow ) << 1 ) | 1u );

	// nU, the underflow distance, is the run of positions below the midpoint
	// where nLow is one and nHigh is zero, counted down from bit 29 after the
	// renormalise shift. Shifting the test value by nZ is the same as testing the
	// shifted registers, and the low nZ bits it brings in are zeros, which is
	// also why nZ + nU can never exceed 31: one reader call covers both.
	//
	// The shift by two does the masking: it pushes bits 29..0 up to 31..2 and
	// drops the two above them, and the two ones it leaves at the bottom are the
	// sentinel that keeps the count defined when the whole run is ones.
	const uint32_t nU = CountLeadingZeros32( ~( ( ( nLow & ~nHigh ) << nZ ) << 2 ) );

	// Both shifts at once. The per-iteration nCode ^= 0x20000000 telescopes into
	// a single flip of bit 30 because every later shift walks the earlier flips
	// out of the 31-bit window, and the underflow loop's masking of bit 30 only
	// applies when it ran at all, which is what nKeep and nFlip carry.
	const uint32_t nShift = nZ + nU;
	const uint32_t nFlip = nU != 0 ? 0x40000000u : 0u;
	const uint32_t nKeep = MASK31 ^ nFlip;
	const uint32_t nCode = m_nCode;

	// ~( ~nHigh << n ) is ( nHigh << n ) | ( ( 1 << n ) - 1 ) in one operation
	// fewer, and it has no shift-count edge case at n == 31.
	m_nLow = ( nLow << nShift ) & nKeep;
	m_nHigh = ( ~( ~nHigh << nShift ) & nKeep ) | nFlip;
	m_nCode = ( ( ( nCode << nShift ) | m_Bits.Get( nShift ) ) ^ nFlip ) & MASK31;
}

uint32_t CArithBits::GetValue( uint32_t nScale )
{
	uint32_t nValue = GetCount( nScale );
	if ( nScale != 0 && nValue >= nScale )
	{
		nValue = nScale - 1;
	}
	Remove( nValue, 1, nScale );
	return nValue;
}

//! Where a model's counts should be split across sixteen cumulative bins.
//!
//! Bin b covers indices [b << nShift, (b+1) << nShift) for b under fifteen, and
//! bin fifteen covers everything from nLastBinStart up. In the ordinary case
//! nLastBinStart is 15 << nShift, so the bins tile the table; in the degenerate
//! case below it is zero and every index lands in bin fifteen. FindPos gets both
//! from min( b << nShift, nLastBinStart ), which needs no branch.
struct SBinning
{
	uint32_t nShift = 15;
	uint32_t nLastBinStart = 0;
};

SBinning BestShift( uint32_t nValue )
{
	SBinning out;
	if ( nValue < 6 )
	{
		return out;
	}

	uint32_t nBestMax = 0xffffffffu;
	uint32_t nBestBin = 0;
	for ( uint32_t i = 0; i < 16; ++i )
	{
		const uint32_t nSize = 1u << i;
		uint32_t nBins = ( nValue + nSize - 1 ) / nSize;
		if ( nBins > 16 )
		{
			nBins = 16;
		}
		uint32_t nLast = nValue - nSize * ( nBins - 1 );
		if ( nLast < nSize )
		{
			nLast = nSize;
		}
		if ( nLast < nBestMax )
		{
			nBestBin = i;
			nBestMax = nLast;
		}
		if ( nSize > nValue )
		{
			break;
		}
	}

	out.nShift = nBestBin;
	out.nLastBinStart = 15u * ( 1u << nBestBin );
	return out;
}

//! What a model returned: a known value, or an escape and the slot to fill.
struct SSymbol
{
	uint32_t nValue = 0;
	uint32_t nEscapeIndex = 0;
	bool bEscape = false;
	bool bFailed = false;
};

//! An adaptive symbol model with an escape at position zero.
class CArithModel
{
public:
	explicit CArithModel( uint32_t nUniqueValues );

	SSymbol Decompress( CArithBits &bits );
	void SetEscaped( uint32_t nIndex, uint32_t nValue )
	{
		m_Values[nIndex] = static_cast<uint16_t>( nValue );
	}

private:
	//! Which entry a coder count falls in, and where that entry starts.
	bool FindPos( uint32_t nCount, uint32_t *pnPos, uint32_t *pnStart ) const;

	//! Add to the cumulative bins.
	//!
	//! nAmount carries two 16-bit lanes and is added as one 32-bit value across
	//! pairs of bins, which is what makes a single add update two running totals.
	void IncrementTotals( uint32_t nValue, uint32_t nAmount );
	void AddTotalPair( uint32_t nPairIndex, uint32_t nAmount );

	//! IncrementTotals for the case where both 16-bit lanes of nAmount are equal.
	//!
	//! Every hot caller is in that case, and it is much cheaper: with equal lanes
	//! there is nothing for the pair packing to do, because the carry it exists to
	//! compensate for cannot happen. No total ever reaches 65536 (the model
	//! rescales at 16384 and the largest total measured over the corpus is 16383),
	//! so each 16-bit lane add is independent and the whole update is "add nAmount
	//! to the bins from BinOf( nValue ) up", which is one masked vector add.
	void IncrementTotalsUniform( uint32_t nValue, uint32_t nAmount );

	void QuickIncrement( uint32_t nValue, uint32_t nAmount );
	void DecrementCounts( uint32_t nValue, uint32_t nAmount );
	void Rescale();

	uint32_t BinOf( uint32_t nIndex ) const
	{
		return nIndex < m_Binning.nLastBinStart ? ( nIndex >> m_Binning.nShift ) : 15u;
	}

	//! The sixteen cumulative bins. Eight zero slots sit in front of them.
	//!
	//! The padding earns its keep twice in FindPos. It makes both halves of the
	//! sixteen 16-byte aligned, and it makes Totals()[-1] a legal read holding
	//! zero, so the running sum in front of bin zero costs no branch. Nothing
	//! ever writes the padding.
	uint16_t *Totals() { return m_TotalsStore + TOTALS_PAD; }
	const uint16_t *Totals() const { return m_TotalsStore + TOTALS_PAD; }

	static constexpr uint32_t TOTALS_PAD = 8;

	//! A scalar mirror of Totals()[15], the sum of every count in the model.
	//!
	//! The sixteen bins are now written as two 128-bit stores, so reading one of
	//! them back as a 2-byte scalar is a store-to-load forwarding stall. Nothing
	//! outside this class needs an individual bin, and everything that used to
	//! read Totals()[15] wanted the total, so the total is kept here as well and
	//! the vector half is only ever touched as vectors.
	uint16_t m_nTotal = 0;

	uint32_t m_nUniqueValues = 0;
	alignas( 16 ) uint16_t m_TotalsStore[TOTALS_PAD + 16] = {};
	std::vector<uint16_t> m_Counts;
	std::vector<uint16_t> m_Values;
	uint32_t m_nNumber = 0;
	SBinning m_Binning;
};

CArithModel::CArithModel( uint32_t nUniqueValues )
	: m_nUniqueValues( nUniqueValues )
{
	// Rounded up with room to spare, which is what the encoder assumes.
	const uint32_t nCount = ( nUniqueValues + 5 ) & ~3u;
	m_Counts.assign( nCount, 0 );
	m_Values.assign( nCount, 0 );
	m_Binning = BestShift( nUniqueValues + 1 );
	// The escape starts with a weight of three, in both lanes.
	QuickIncrement( 0, 0x00030003u );
}

void CArithModel::AddTotalPair( uint32_t nPairIndex, uint32_t nAmount )
{
	uint16_t *pTotals = Totals();
	const uint32_t nIndex = nPairIndex * 2;
	const uint32_t nOld =
		uint32_t( pTotals[nIndex] ) | ( uint32_t( pTotals[nIndex + 1] ) << 16 );
	const uint32_t nNew = nOld + nAmount;
	pTotals[nIndex] = static_cast<uint16_t>( nNew & 0xffffu );
	pTotals[nIndex + 1] = static_cast<uint16_t>( nNew >> 16 );
}

void CArithModel::IncrementTotals( uint32_t nValue, uint32_t nAmount )
{
	uint16_t *pTotals = Totals();
	const uint16_t nLow = static_cast<uint16_t>( nAmount & 0xffffu );
	if ( nValue >= m_Binning.nLastBinStart )
	{
		pTotals[15] = static_cast<uint16_t>( pTotals[15] + nLow );
		m_nTotal = pTotals[15];
		return;
	}

	uint32_t nBin = nValue >> m_Binning.nShift;
	if ( ( nBin & 1u ) != 0 )
	{
		pTotals[nBin] = static_cast<uint16_t>( pTotals[nBin] + nLow );
		++nBin;
	}
	for ( uint32_t nPair = nBin >> 1; nPair < 8; ++nPair )
	{
		AddTotalPair( nPair, nAmount );
	}
	// Read the mirror back rather than reasoning about what the pair carry did to
	// the last bin. This path runs once per model at most.
	m_nTotal = pTotals[15];
}

void CArithModel::IncrementTotalsUniform( uint32_t nValue, uint32_t nAmount )
{
	const uint32_t nBin = BinOf( nValue );
	uint16_t *pTotals = Totals();
	// Bin 15 is in every suffix, so the total always moves by the whole amount.
	m_nTotal = static_cast<uint16_t>( m_nTotal + nAmount );
#if GR2_OODLE0_SSE2
	const __m128i vAmount = _mm_set1_epi16( static_cast<short>( nAmount ) );
	const __m128i vMaskLow =
		_mm_loadu_si128( reinterpret_cast<const __m128i *>( SUFFIX_ONES + 16 - nBin ) );
	const __m128i vMaskHigh =
		_mm_loadu_si128( reinterpret_cast<const __m128i *>( SUFFIX_ONES + 24 - nBin ) );
	__m128i *pLow = reinterpret_cast<__m128i *>( pTotals );
	__m128i *pHigh = pLow + 1;
	_mm_store_si128( pLow, _mm_add_epi16( _mm_load_si128( pLow ),
	                                      _mm_and_si128( vMaskLow, vAmount ) ) );
	_mm_store_si128( pHigh, _mm_add_epi16( _mm_load_si128( pHigh ),
	                                       _mm_and_si128( vMaskHigh, vAmount ) ) );
#else
	for ( uint32_t i = nBin; i < 16; ++i )
	{
		pTotals[i] = static_cast<uint16_t>( pTotals[i] + nAmount );
	}
#endif
}

void CArithModel::QuickIncrement( uint32_t nValue, uint32_t nAmount )
{
	// Every caller of this one passes equal lanes.
	IncrementTotalsUniform( nValue, nAmount & 0xffffu );
	m_Counts[nValue] = static_cast<uint16_t>( m_Counts[nValue] + ( nAmount & 0xffffu ) );
}

void CArithModel::DecrementCounts( uint32_t nValue, uint32_t nAmount )
{
	// The one asymmetric caller, which needs the pair-packed borrow, so it goes
	// through the general IncrementTotals rather than the uniform fast path.
	const uint32_t nNegative = 0u - nAmount;
	const uint32_t nPacked = ( ( nNegative - 1 ) << 16 ) | ( nNegative & 0xffffu );
	IncrementTotals( nValue, nPacked );
	m_Counts[nValue] = static_cast<uint16_t>( m_Counts[nValue] + ( nPacked & 0xffffu ) );
}

//! Which entry a coder count falls in, and where that entry starts.
//!
//! This used to be a plain scan from index zero, which over this game's data cost
//! 38.9 steps per call: the median was 2 but a fifth of the calls walked more than
//! 32 entries and those carried most of the work. The model already keeps the
//! structure needed to skip the walk and never used it. m_Totals is sixteen
//! cumulative bins over the same m_Counts, so one lookup names the bin the answer
//! is in and the running sum in front of it, and only that bin has to be scanned.
//! Measured over the whole corpus that is 4.44 steps per call instead of 38.90,
//! with the position and start identical on all 194,829,921 calls.
bool CArithModel::FindPos( uint32_t nCount, uint32_t *pnPos, uint32_t *pnStart ) const
{
	const uint16_t *pTotals = Totals();

	// Everything the model holds is in m_Totals[15], which is maintained as the
	// exact sum of m_Counts. So a count at or past it is precisely the refusal the
	// old scan used to discover by running off the end of the table. This test has
	// to come first for a second reason: on a malformed stream GetCount can return
	// any 32-bit value, and the 16-bit compares below are only meaningful once
	// nCount is known to be under the total, hence under 16384.
	if ( nCount >= m_nTotal )
	{
		return false;
	}

#if GR2_OODLE0_SSE2
	// m_Totals is non-decreasing, so the bins holding more than nCount are a
	// suffix and the first of them is the one the answer lies in. Two 128-bit
	// compares cover all sixteen with no branch and no dependent chain. The
	// signed 16-bit compare is safe because every value in play is under 16384;
	// the largest total seen over the corpus is 16383.
	const __m128i vCount = _mm_set1_epi16( static_cast<short>( nCount ) );
	const __m128i vLow = _mm_load_si128( reinterpret_cast<const __m128i *>( pTotals ) );
	const __m128i vHigh = _mm_load_si128( reinterpret_cast<const __m128i *>( pTotals + 8 ) );
	const uint32_t nMask =
		static_cast<uint32_t>( _mm_movemask_epi8( _mm_cmpgt_epi16( vLow, vCount ) ) )
		| ( static_cast<uint32_t>( _mm_movemask_epi8( _mm_cmpgt_epi16( vHigh, vCount ) ) )
		    << 16 );
	// Never zero: bin 15 holds the total, which the test above proved is greater.
	// Two mask bits per lane, so the lane is the bit index halved.
	const uint32_t nBin = LowestSetBit( nMask ) >> 1;
#else
	uint32_t nBin = 0;
	while ( nCount >= pTotals[nBin] )
	{
		++nBin;
	}
#endif

	// Bin b starts at b << nShift, except that bin 15 starts at nLastBinStart.
	// Clamping covers both: for b under 15 the shift is already below
	// nLastBinStart, and for b of 15 the clamp is the answer. That also handles
	// the degenerate binning, where nLastBinStart is zero and bin 15 is the whole
	// table. The slot in front of the bins is the zero padding, so the running sum
	// for bin zero needs no test either.
	uint32_t i = nBin << m_Binning.nShift;
	if ( i > m_Binning.nLastBinStart )
	{
		i = m_Binning.nLastBinStart;
	}
	uint32_t nStart = m_TotalsStore[TOTALS_PAD - 1 + nBin];
	for ( ; i < m_Counts.size(); ++i )
	{
		if ( nCount < nStart + m_Counts[i] )
		{
			*pnPos = i;
			*pnStart = nStart;
			return true;
		}
		nStart += m_Counts[i];
	}
	return false;
}

void CArithModel::Rescale()
{
	m_Binning = BestShift( m_nNumber + 1 );

	uint32_t Bins[16] = {};
	m_Counts[0] = static_cast<uint16_t>( m_Counts[0] >> 1 );
	Bins[BinOf( 0 )] += m_Counts[0];

	uint32_t nMaxCount = 0;
	uint32_t nMaxPos = 0;
	uint32_t nIndex = 1;
	bool bDone = false;
	while ( nIndex <= m_nNumber && !bDone )
	{
		// Drop everything that halved away, filling the gap from the end.
		while ( m_Counts[nIndex] <= 1 )
		{
			if ( nIndex < m_nNumber )
			{
				m_Counts[nIndex] = m_Counts[m_nNumber];
				m_Values[nIndex] = m_Values[m_nNumber];
				m_Counts[m_nNumber] = 0;
				--m_nNumber;
			}
			else
			{
				m_Counts[nIndex] = 0;
				--m_nNumber;
				bDone = true;
				break;
			}
		}
		if ( bDone )
		{
			break;
		}

		m_Counts[nIndex] = static_cast<uint16_t>( m_Counts[nIndex] >> 1 );
		if ( m_Counts[nIndex] > nMaxCount )
		{
			nMaxCount = m_Counts[nIndex];
			nMaxPos = nIndex;
		}
		Bins[BinOf( nIndex )] += m_Counts[nIndex];
		++nIndex;
	}

	if ( nMaxCount != 0 )
	{
		// The heaviest symbol moves to the front of the last bin, where the
		// search reaches it soonest.
		uint32_t nSwapPos = m_nNumber < m_Binning.nLastBinStart
		                        ? ( ( m_nNumber >> m_Binning.nShift ) << m_Binning.nShift )
		                        : m_Binning.nLastBinStart;
		if ( nSwapPos == 0 )
		{
			nSwapPos = 1;
		}
		if ( nMaxPos != nSwapPos )
		{
			const uint16_t nOldCount = m_Counts[nSwapPos];
			m_Counts[nSwapPos] = m_Counts[nMaxPos];
			Bins[BinOf( nSwapPos )] += uint32_t( m_Counts[nSwapPos] ) - nOldCount;
			Bins[BinOf( nMaxPos )] += uint32_t( nOldCount ) - m_Counts[nSwapPos];
			m_Counts[nMaxPos] = nOldCount;
			const uint16_t nSwapValue = m_Values[nSwapPos];
			m_Values[nSwapPos] = m_Values[nMaxPos];
			m_Values[nMaxPos] = nSwapValue;
		}
	}

	// The escape has to stay reachable while there are values still unseen.
	if ( m_nNumber != m_nUniqueValues && m_Counts[0] == 0 )
	{
		m_Counts[0] = static_cast<uint16_t>( m_Counts[0] + 2 );
		Bins[BinOf( 0 )] += 2;
	}

	uint16_t *pTotals = Totals();
	uint32_t nRunning = 0;
	for ( uint32_t i = 0; i < 16; ++i )
	{
		nRunning += Bins[i];
		pTotals[i] = static_cast<uint16_t>( nRunning );
	}
	m_nTotal = static_cast<uint16_t>( nRunning );
}

SSymbol CArithModel::Decompress( CArithBits &bits )
{
	SSymbol out;

	if ( m_nTotal >= RESCALE_TOTAL )
	{
		Rescale();
	}

	const uint32_t nScale = m_nTotal;
	const uint32_t nCount = bits.GetCount( nScale );

	uint32_t nPos = 0;
	uint32_t nStart = 0;
	if ( !FindPos( nCount, &nPos, &nStart ) )
	{
		out.bFailed = true;
		return out;
	}

	const uint16_t nOldCount = m_Counts[nPos];
	// The totals are bumped before the range narrows, so the scale Remove sees is
	// the total from before this symbol. Order matters and is not obvious. That is
	// exactly nScale: the bump adds one to the last bin unconditionally, so the old
	// "Totals()[15] - 1" was a longer way of writing the value already in hand.
	// The sequence is kept as it was anyway, because nothing is gained by moving it.
	IncrementTotalsUniform( nPos, 1u );
	bits.Remove( nStart, nOldCount, nScale );
	m_Counts[nPos] = static_cast<uint16_t>( m_Counts[nPos] + 1 );

	if ( nPos != 0 )
	{
		out.nValue = m_Values[nPos];
		return out;
	}

	// An escape: a value this model has not seen. The caller reads it raw and
	// gives it back through SetEscaped.
	++m_nNumber;
	if ( m_nNumber >= m_Counts.size() )
	{
		out.bFailed = true;
		return out;
	}
	QuickIncrement( m_nNumber, 0x00020002u );
	if ( m_nNumber == m_nUniqueValues )
	{
		// Every value is now known, so the escape can never be needed again.
		DecrementCounts( 0, m_Counts[0] );
	}

	out.bEscape = true;
	out.nEscapeIndex = m_nNumber;
	return out;
}

//! The twelve bytes of parameters in front of each of the three stages.
struct SParameters
{
	uint32_t nMaxByteValue = 0;
	uint32_t nMaxOffset = 0;
	uint32_t nUniqueByteValues = 0;
	uint32_t nUniqueOffsets = 0;
	uint8_t nUniqueLengths[4] = {};

	//! How many distinct lengths the model for a given previous length expects.
	uint32_t LengthUnique( uint32_t nIndex ) const
	{
		uint32_t nGroup = nIndex / ( MAX_LENGTHS / 4 );
		if ( nGroup > 3 )
		{
			nGroup = 3;
		}
		return nUniqueLengths[3 - nGroup];
	}
};

SParameters ReadParameters( const uint8_t *pBytes )
{
	uint32_t nWords[3] = {};
	memcpy( nWords, pBytes, sizeof( nWords ) );

	SParameters out;
	// The same packing Oodle1 uses for its first word: nine bits then twenty
	// three. The second word is split the same way here, where Oodle1 splits the
	// upper part again.
	out.nMaxByteValue = nWords[0] & 0x1ffu;
	out.nMaxOffset = nWords[0] >> 9;
	out.nUniqueByteValues = nWords[1] & 0x1ffu;
	out.nUniqueOffsets = nWords[1] >> 9;
	memcpy( out.nUniqueLengths, &nWords[2], sizeof( out.nUniqueLengths ) );
	return out;
}

//! Every model one stage needs.
class CStage
{
public:
	explicit CStage( const SParameters &params );
	~CStage();

	CStage( const CStage & ) = delete;
	CStage &operator=( const CStage & ) = delete;

	//! Decode into pOut until nStop bytes of output exist in total.
	//!
	//! \param nWritten how much of the output is already filled, in and out.
	bool Decode( CArithBits &bits, uint8_t *pOut, uint32_t nSize, uint32_t nStop,
	             uint32_t *pnWritten );

private:
	//! Ask a model, and read the value raw if the model escapes.
	static bool Draw( CArithModel &model, CArithBits &bits, uint32_t nEscapeScale,
	                  uint32_t *pnValue );

	//! Build the length model for nIndex. Out of line and cold: it runs at most
	//! once per index per stage, against 86.7 million draws over this corpus.
	CArithModel *BuildLength( uint32_t nIndex );

	uint32_t m_nMaxBytes = 0;
	uint32_t m_nMaxOffsets = 0;
	uint32_t m_nMaxOffsetLow = 0;
	uint32_t m_nBytesDecompressed = 0;
	uint32_t m_nLastLength = 0;

	CArithModel m_Bytes;
	CArithModel m_OffsetLow;
	CArithModel m_OffsetHigh;

	// LAZY LENGTH MODELS. The format declares one length model per previous
	// length, 65 of them, and a stage draws from only 22.7 of them on average:
	// over this corpus 3,364,725 length models were built and 1,177,449 (35.0%)
	// ever drawn from. Building one costs two heap allocations, a sixteen-way
	// BestShift search and a totals update, so two thirds of that was thrown
	// away. Here each slot is built on its first draw instead.
	//
	// The storage is in the stage rather than on the heap so a built model costs
	// no allocation of its own, and only the pointer array has to be cleared.
	uint8_t m_UniqueLengths[4] = {};
	CArithModel *m_pLengths[MAX_LENGTHS + 1] = {};
	alignas( CArithModel ) uint8_t m_LengthStorage[( MAX_LENGTHS + 1 ) * sizeof( CArithModel )];
};

CStage::CStage( const SParameters &params )
	: m_nMaxBytes( params.nMaxByteValue )
	, m_nMaxOffsets( params.nMaxOffset )
	, m_nMaxOffsetLow( params.nMaxOffset < LOW_OFFSET_MASK + 1 ? params.nMaxOffset
	                                                           : LOW_OFFSET_MASK + 1 )
	, m_Bytes( params.nUniqueByteValues )
	, m_OffsetLow( m_nMaxOffsetLow )
	, m_OffsetHigh( params.nUniqueOffsets )
{
	memcpy( m_UniqueLengths, params.nUniqueLengths, sizeof( m_UniqueLengths ) );
}

CStage::~CStage()
{
	for ( uint32_t i = 0; i <= MAX_LENGTHS; ++i )
	{
		if ( m_pLengths[i] != nullptr )
		{
			m_pLengths[i]->~CArithModel();
		}
	}
}

CArithModel *CStage::BuildLength( uint32_t nIndex )
{
	uint32_t nGroup = nIndex / ( MAX_LENGTHS / 4 );
	if ( nGroup > 3 )
	{
		nGroup = 3;
	}
	void *pSlot = m_LengthStorage + size_t( nIndex ) * sizeof( CArithModel );
	CArithModel *pModel = new ( pSlot ) CArithModel( m_UniqueLengths[3 - nGroup] );
	m_pLengths[nIndex] = pModel;
	return pModel;
}

bool CStage::Draw( CArithModel &model, CArithBits &bits, uint32_t nEscapeScale,
                   uint32_t *pnValue )
{
	const SSymbol symbol = model.Decompress( bits );
	if ( symbol.bFailed || bits.Failed() )
	{
		return false;
	}
	if ( !symbol.bEscape )
	{
		*pnValue = symbol.nValue;
		return true;
	}

	*pnValue = bits.GetValue( nEscapeScale );
	model.SetEscaped( symbol.nEscapeIndex, *pnValue );
	return !bits.Failed();
}

bool CStage::Decode( CArithBits &bits, uint8_t *pOut, uint32_t nSize, uint32_t nStop,
                     uint32_t *pnWritten )
{
	while ( *pnWritten < nStop )
	{
		uint32_t nLengthSymbol = 0;
		CArithModel *pLength = m_pLengths[m_nLastLength];
		if ( pLength == nullptr )
		{
			pLength = BuildLength( m_nLastLength );
		}
		if ( !Draw( *pLength, bits, MAX_LENGTHS + 1, &nLengthSymbol )
		     || nLengthSymbol > MAX_LENGTHS )
		{
			return false;
		}
		m_nLastLength = nLengthSymbol;

		if ( nLengthSymbol == 0 )
		{
			uint32_t nLiteral = 0;
			if ( !Draw( m_Bytes, bits, m_nMaxBytes, &nLiteral ) || nLiteral > 0xff
			     || *pnWritten >= nSize )
			{
				return false;
			}
			pOut[*pnWritten] = static_cast<uint8_t>( nLiteral );
			++*pnWritten;
			++m_nBytesDecompressed;
			continue;
		}

		const uint32_t nLength = nLengthSymbol >= MAX_LENGTHS - 3
		                             ? LONG_LENGTHS[nLengthSymbol - ( MAX_LENGTHS - 3 )]
		                             : nLengthSymbol + 1;

		uint32_t nLow = 0;
		if ( !Draw( m_OffsetLow, bits, m_nMaxOffsetLow, &nLow ) )
		{
			return false;
		}

		// The high part's range grows with what this stage has produced, so an
		// early match cannot name an offset that does not exist yet.
		const uint32_t nReach = m_nMaxOffsets < m_nBytesDecompressed ? m_nMaxOffsets
		                                                             : m_nBytesDecompressed;
		uint32_t nHigh = 0;
		if ( !Draw( m_OffsetHigh, bits, ( nReach >> OFFSET_SPLIT_SHIFT ) + 1, &nHigh ) )
		{
			return false;
		}

		const uint64_t nDistance =
			uint64_t( nLow ) + 1 + ( uint64_t( nHigh ) << OFFSET_SPLIT_SHIFT );
		if ( nDistance == 0 || nDistance > *pnWritten || nLength > nSize - *pnWritten )
		{
			// Before the start of the output, or past the end of it. The reference
			// checks the first and would run off the end on the second.
			return false;
		}

		const uint32_t nFrom = *pnWritten - static_cast<uint32_t>( nDistance );

		// EIGHT BYTES AT A TIME WHERE THAT IS LEGAL. Measured over this game's
		// data: 21.5 million matches averaging 9.2 bytes, 76% of them eight bytes
		// or shorter, and only 1.10% with the source overlapping the destination.
		// So the common case is one unaligned move rather than up to eight.
		//
		// Two conditions. nDistance >= nLength means the source and destination do
		// not overlap at all, so reading ahead cannot read a byte this copy is
		// about to write. And the padded length has to fit inside the output,
		// because the chunked form overshoots by up to seven bytes; those bytes are
		// always rewritten by the next symbol, except at the very end of a section,
		// which is what the slack test excludes.
		const uint32_t nPadded = ( nLength + 7u ) & ~7u;
		if ( nDistance >= nLength && nSize - *pnWritten >= nPadded )
		{
			uint8_t *pDst = pOut + *pnWritten;
			const uint8_t *pSrc = pOut + nFrom;
			uint32_t i = 0;
			do
			{
				memcpy( pDst + i, pSrc + i, 8 );
				i += 8;
			} while ( i < nLength );
		}
		else
		{
			// Byte at a time, because the source overlaps the destination whenever
			// the distance is shorter than the length, which is how a two byte
			// offset fills five hundred.
			for ( uint32_t i = 0; i < nLength; ++i )
			{
				pOut[*pnWritten + i] = pOut[nFrom + i];
			}
		}
		*pnWritten += nLength;
		m_nBytesDecompressed += nLength;
	}
	return true;
}

}

bool Oodle0Decompress( const uint8_t *pCompressed, uint32_t nCompressedSize, uint32_t nStop0,
                       uint32_t nStop1, uint8_t *pDecompressed, uint32_t nDecompressedSize )
{
	if ( pCompressed == nullptr || pDecompressed == nullptr )
	{
		return false;
	}
	if ( nCompressedSize < HEADER_SIZE )
	{
		return false;
	}

	// The stops are clamped rather than refused. first16Bit and first8Bit are
	// hints about where the 32, 16 and 8 bit data begin, and a file that puts
	// them out of order is still readable as long as the stages stay ordered.
	uint32_t nStops[STAGE_COUNT] = { nStop0, nStop1, nDecompressedSize };
	for ( uint32_t i = 0; i < STAGE_COUNT; ++i )
	{
		if ( nStops[i] > nDecompressedSize )
		{
			nStops[i] = nDecompressedSize;
		}
	}
	if ( nStops[1] < nStops[0] )
	{
		nStops[1] = nStops[0];
	}

	SParameters params[STAGE_COUNT];
	for ( uint32_t i = 0; i < STAGE_COUNT; ++i )
	{
		params[i] = ReadParameters( pCompressed + i * 12 );
	}

	CArithBits bits( pCompressed, nCompressedSize, HEADER_SIZE );

	uint32_t nWritten = 0;
	for ( uint32_t i = 0; i < STAGE_COUNT; ++i )
	{
		if ( nStops[i] <= nWritten )
		{
			// An empty stage, which the middle one always is in this game's data.
			continue;
		}
		CStage stage( params[i] );
		if ( !stage.Decode( bits, pDecompressed, nDecompressedSize, nStops[i], &nWritten ) )
		{
			return false;
		}
	}

	return nWritten == nDecompressedSize && !bits.Failed();
}

}
