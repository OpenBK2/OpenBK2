// Holds every implementation of the 16-bit to 32-bit index shift (GIndexShift.h and
// test/IndexShiftVariants.h) to the scalar reference, bit for bit.
//
// The engine calls it on a source that is only 2-byte aligned and a destination that
// is only 4-byte aligned, with sizes that are any multiple of three and offsets up to
// the size of the dynamic vertex buffer. So every case here draws a random size in
// 0..5000, random misalignments of both buffers, and an offset from a set that
// includes zero, the largest the engine could produce and values whose sum with a
// 16-bit index crosses the 2^24 and 2^31 boundaries.
//
// The destination is surrounded by guard values, so a variant that writes before
// pDst or past pDst + nSize fails as well as one that writes the wrong value.

#include "3Dmotor/stdafx.h"

#include "3Dmotor/GLightPerVertexKernels.h"
// IsAVX2Present() lives beside the lighting kernels, which need the initialised
// nCubicRoot table; the 3Dmotor DLL does not export it on Windows.
#include "3Dmotor/GSSEtransform.cpp"
#include "3Dmotor/test/IndexShiftVariants.h"

#include <cstdint>
#include <iterator>
#include <random>
#include <vector>

#include <gtest/gtest.h>

namespace
{

using TShiftFunc = void ( * )( const uint16_t *, int32_t *, int, int32_t );

struct SVariant
{
	const char *pszName;
	TShiftFunc pFunc;
	bool bNeedsAVX2;
};

// SSE4.1 is implied by AVX2; the SSE41 variant is gated on the AVX2 check so that
// only one CPUID probe exists.
const SVariant variants[] = {
	// Original is left out: its int addition overflows on the wrap-around offsets
	// below, which is undefined. ReferenceMatchesOriginalTransform covers it.
	{ "ShiftIndices", &NGfx::ShiftIndices, false },
	{ "RestrictIvdep", &NIndexShift::RestrictIvdep, false },
#if BK2_INDEX_SHIFT_AUTOVEC
	{ "AutoVec", &NIndexShift::AutoVec, false },
#endif
#if BK2_INDEX_SHIFT_SSE2
	{ "SSE2", &NGfx::ShiftIndicesSSE2, false },
	{ "SSE2x16", &NIndexShift::SSE2x16, false },
	{ "SSE2Unaligned", &NIndexShift::SSE2Unaligned, false },
	{ "SSE2Stream", &NIndexShift::SSE2Stream, false },
	{ "SSE41", &NIndexShift::SSE41, true },
	{ "AVX2", &NIndexShift::AVX2, true },
	{ "AVX2Aligned", &NIndexShift::AVX2Aligned, true },
	{ "AVX2Stream", &NIndexShift::AVX2Stream, true },
#endif
#if BK2_INDEX_SHIFT_STDX
	{ "Stdx", &NIndexShift::Stdx, false },
#endif
};

// Offsets the engine can produce (0 up to the ~14 million vertices the dynamic vertex
// buffer can hold) plus the wrap-around cases, where the unsigned reference and the
// SIMD paddd must still agree.
const int32_t shifts[] = {
	0, 1, 65535, 65536, 1234567, 14000000, 16777215 - 65535, 16777216, 0x7FFFFFFF - 65535, 0x7FFFFFFF, -1, -65536 };

const int32_t N_GUARD = 0x5A5A5A5A;
const int N_GUARD_INTS = 16;

void CheckVariant( const SVariant &variant, std::mt19937 &rng, int nCases )
{
	std::uniform_int_distribution<int> sizeDist( 0, 5000 );
	std::uniform_int_distribution<int> wordDist( 0, 65535 );
	std::uniform_int_distribution<int> srcMisalign( 0, 7 );
	std::uniform_int_distribution<int> dstMisalign( 0, 7 );
	std::uniform_int_distribution<int> shiftPick( 0, static_cast<int>( std::size( shifts ) ) );
	for ( int nCase = 0; nCase < nCases; ++nCase )
	{
		// Every size from 0 to 40 first, so each remainder path is hit, then random.
		const int nSize = nCase <= 40 ? nCase : sizeDist( rng );
		const int nSrcOff = srcMisalign( rng );
		const int nDstOff = dstMisalign( rng );
		const int nPick = shiftPick( rng );
		// One draw in thirteen is a fully random offset rather than one from the table.
		const int32_t nShift =
				nPick < static_cast<int>( std::size( shifts ) ) ? shifts[nPick] : static_cast<int32_t>( rng() );

		std::vector<uint16_t> src( nSrcOff + nSize );
		for ( int i = 0; i < nSize; ++i )
		{
			// Force the extremes in now and then; 0xFFFF plus an offset is where a
			// signed unpack would go wrong.
			const int nWord = wordDist( rng );
			src[nSrcOff + i] = static_cast<uint16_t>( ( nWord & 15 ) == 0 ? 0xFFFF : nWord );
		}
		std::vector<int32_t> expected( nSize );
		NGfx::ShiftIndicesRef( src.data() + nSrcOff, expected.data(), nSize, nShift );

		std::vector<int32_t> dst( N_GUARD_INTS + nDstOff + nSize + N_GUARD_INTS, N_GUARD );
		int32_t *pDst = dst.data() + N_GUARD_INTS + nDstOff;
		variant.pFunc( src.data() + nSrcOff, pDst, nSize, nShift );

		for ( int i = 0; i < nSize; ++i )
		{
			ASSERT_EQ( expected[i], pDst[i] ) << variant.pszName << " size " << nSize << " index " << i << " shift " << nShift
																				<< " src misalign " << nSrcOff << " dst misalign " << nDstOff;
		}
		for ( int i = 0; i < N_GUARD_INTS + nDstOff; ++i )
		{
			ASSERT_EQ( N_GUARD, dst[i] ) << variant.pszName << " wrote before the destination, size " << nSize;
		}
		for ( size_t i = N_GUARD_INTS + nDstOff + nSize; i < dst.size(); ++i )
		{
			ASSERT_EQ( N_GUARD, dst[i] ) << variant.pszName << " wrote past the destination, size " << nSize;
		}
	}
}

TEST( IndexShift, ReferenceMatchesOriginalTransform )
{
	// The reference is defined in unsigned arithmetic; check it against the code it
	// replaced over the range the engine actually uses, where int addition cannot
	// overflow.
	std::vector<uint16_t> src( 65536 );
	for ( int i = 0; i < 65536; ++i )
	{
		src[i] = static_cast<uint16_t>( i );
	}
	for ( const int32_t nShift : { 0, 1, 65536, 14000000 } )
	{
		std::vector<int32_t> a( src.size() ), b( src.size() );
		NIndexShift::Original( src.data(), a.data(), static_cast<int>( src.size() ), nShift );
		NGfx::ShiftIndicesRef( src.data(), b.data(), static_cast<int>( src.size() ), nShift );
		ASSERT_EQ( a, b ) << "shift " << nShift;
	}
}

TEST( IndexShift, AllVariantsMatchReference )
{
	std::mt19937 rng( std::random_device{}() );
	int nRun = 0;
	for ( const SVariant &variant : variants )
	{
		if ( variant.bNeedsAVX2 && !NGScene::IsAVX2Present() )
		{
			continue;
		}
		SCOPED_TRACE( variant.pszName );
		CheckVariant( variant, rng, 2000 );
		if ( HasFatalFailure() )
		{
			return;
		}
		++nRun;
	}
	EXPECT_GT( nRun, 0 );
}

} // namespace
