// Holds every rigid normal transform kernel (GNormalTransform.h,
// test/NormalTransformVariants.h) to the engine's SSE2 MMXTransformVector bit for bit.
//
// Two baselines. ReferenceTransform in the variants header is a frozen copy of
// MMXTransformVector, so the kernels keep their target when GSSEtransform.h changes;
// the live MMXTransformVector is checked against that copy as well, so the day the
// two part this test says so instead of quietly comparing against stale code.
//
// Coverage: every one of the 2^24 x, y, z byte patterns, sampled in _DEBUG builds
// (w random, it must come through unchanged), through several realistic matrices
// (rotation with per-axis scale and a large translation, the bottom row junk because
// nothing may read it)
// and several degenerate ones (zero, singular, huge, tiny, infinite and NaN
// translation), so the zero vector's inf/NaN path and every saturation are
// compared too. Then every part size from 0 to 40 and random sizes up to 5000 at the
// strides the engine uses, with each source and result array ending exactly at an
// inaccessible page, so a kernel that touches a byte past either crashes the test.

#include "3Dmotor/stdafx.h"

// The live MMXTransformVector (static, in the header) and the tables the
// GSSEtransform.cpp beside it defines; IsAVX2Present() comes with the lighting
// kernel sources the CMake target adds.
#include "3DLib/GGeometry.h"
#include "3Dmotor/GLightPerVertexKernels.h"
#include "3Dmotor/GSSEtransform.cpp"
#include "3Dmotor/GSSEtransform.h"
#include "3Dmotor/test/NormalTransformVariants.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#if defined( _WIN32 )
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

// The engine passes &pUV[0].normal with a stride of sizeof( SUVInfo ); the kernel
// reads four bytes at each, which only works if the normal is where it is assumed.
static_assert( sizeof( NGfx::SCompactVector ) == 4, "a normal is one dword" );
static_assert( sizeof( NGScene::SUVInfo ) == 20, "the stride the engine call passes" );
static_assert( offsetof( NGScene::SUVInfo, normal ) == 8, "the normal inside SUVInfo" );
static_assert( sizeof( SHMatrix ) == 64, "sixteen packed floats" );

namespace
{

using NNormalVariants::SNormalCall;
using NNormalVariants::SNormalVariant;

// nBytes bytes whose last one ends where an inaccessible page begins. The same
// guard as test/Skinning.cpp, in bytes because the source is a strided struct array.
class CGuardedBytes
{
	size_t nPage = 0;
	size_t nTotal = 0;
	char *pBase = nullptr;
	uint8_t *pData = nullptr;

public:
	explicit CGuardedBytes( size_t nBytes )
	{
#if defined( _WIN32 )
		SYSTEM_INFO si;
		GetSystemInfo( &si );
		nPage = si.dwPageSize;
#else
		nPage = static_cast<size_t>( sysconf( _SC_PAGESIZE ) );
#endif
		const size_t nDataPages = ( nBytes + nPage - 1 ) / nPage;
		nTotal = ( nDataPages + 1 ) * nPage;
#if defined( _WIN32 )
		pBase = static_cast<char *>( VirtualAlloc( nullptr, nTotal, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE ) );
		DWORD nOld = 0;
		VirtualProtect( pBase + nDataPages * nPage, nPage, PAGE_NOACCESS, &nOld );
#else
		void *p = mmap( nullptr, nTotal, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0 );
		pBase = p == MAP_FAILED ? nullptr : static_cast<char *>( p );
		if ( pBase )
		{
			mprotect( pBase + nDataPages * nPage, nPage, PROT_NONE );
		}
#endif
		pData = reinterpret_cast<uint8_t *>( pBase + nDataPages * nPage - nBytes );
	}

	~CGuardedBytes()
	{
#if defined( _WIN32 )
		VirtualFree( pBase, 0, MEM_RELEASE );
#else
		munmap( pBase, nTotal );
#endif
	}

	CGuardedBytes( const CGuardedBytes & ) = delete;
	CGuardedBytes &operator=( const CGuardedBytes & ) = delete;

	uint8_t *data()
	{
		return pData;
	}

	bool IsValid() const
	{
		return pBase != nullptr;
	}
};

// A bone the way the engine's normal matrices look: rotation from a random unit
// quaternion, a per-axis scale of 0.5..2 (scaled parts exist, and the transform
// renormalises), translation up to 1000 (the normal's w input is 0, so it may only
// contribute the signed zero), and a junk bottom row.
void RandomMatrix( std::mt19937 &rng, float *m )
{
	std::normal_distribution<double> gauss;
	std::uniform_real_distribution<double> scale( 0.5, 2.0 );
	std::uniform_real_distribution<double> trans( -1000.0, 1000.0 );
	double q[4];
	double len = 0;
	for ( double &v : q )
	{
		v = gauss( rng );
		len += v * v;
	}
	len = std::sqrt( len );
	const double w = q[0] / len, x = q[1] / len, y = q[2] / len, z = q[3] / len;
	const double r[3][3] = { { 1 - 2 * ( y * y + z * z ), 2 * ( x * y - z * w ), 2 * ( x * z + y * w ) },
	                         { 2 * ( x * y + z * w ), 1 - 2 * ( x * x + z * z ), 2 * ( y * z - x * w ) },
	                         { 2 * ( x * z - y * w ), 2 * ( y * z + x * w ), 1 - 2 * ( x * x + y * y ) } };
	const double s[3] = { scale( rng ), scale( rng ), scale( rng ) };
	for ( int row = 0; row < 3; ++row )
	{
		for ( int col = 0; col < 3; ++col )
		{
			m[4 * row + col] = static_cast<float>( r[row][col] * s[col] );
		}
		m[4 * row + 3] = static_cast<float>( trans( rng ) );
	}
	std::uniform_real_distribution<float> junk( -5.0f, 5.0f );
	for ( int col = 0; col < 4; ++col )
	{
		m[12 + col] = junk( rng );
	}
}

struct SNamedMatrix
{
	const char *pszName;
	float m[16];
};

// The matrices the exhaustive test runs every byte pattern through.
std::vector<SNamedMatrix> TestMatrices()
{
	std::vector<SNamedMatrix> v;
	const float inf = std::numeric_limits<float>::infinity();
	const float nan = std::numeric_limits<float>::quiet_NaN();
	v.push_back( { "identity", { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 } } );
	std::mt19937 rng( 20261102 );
	for ( const char *psz :
	      { "random rotation and scale 1", "random rotation and scale 2", "random rotation and scale 3" } )
	{
		SNamedMatrix n{ psz, {} };
		RandomMatrix( rng, n.m );
		v.push_back( n );
	}
	// Negative translation: m[i][3] * 0 is -0 there, so the kernels' constant has to
	// carry the sign.
	v.push_back( { "mirror, negative translation", { -1, 0, 0, -500, 0, 1, 0, -3, 0, 0, -1, -0.5f, 9, 9, 9, 9 } } );
	v.push_back( { "zero", { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } );
	// Rank one: almost every normal maps to a multiple of one axis, many to zero.
	v.push_back( { "singular", { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } } );
	// len2 overflows to inf (1/inf = 0) and underflows to 0 or a denormal.
	v.push_back( { "huge", { 3e19f, 0, 0, 0, 0, 3e19f, 0, 0, 0, 0, 3e19f, 0, 0, 0, 0, 1 } } );
	v.push_back( { "tiny", { 1e-20f, 0, 0, 0, 0, 1e-21f, 0, 0, 0, 0, 1e-22f, 0, 0, 0, 0, 1 } } );
	v.push_back( { "infinite translation", { 1, 0, 0, inf, 0, 1, 0, -inf, 0, 0, 1, 0, 0, 0, 0, 1 } } );
	v.push_back( { "NaN translation", { 0.6f, 0.8f, 0, nan, -0.8f, 0.6f, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 } } );
	return v;
}

int AvailableVariants( const SNormalVariant **ppVariants )
{
	int nCount = 0;
	*ppVariants = NNormalVariants::GetNormalVariants( &nCount );
	return nCount;
}

bool IsRunnable( const SNormalVariant &v )
{
	return !v.bNeedsAVX2 || NGScene::IsAVX2Present();
}

// The sweep over all 2^24 patterns took twelve minutes against the debug CRT, past
// the 600 s ctest gives a test in CI, so builds with _DEBUG take every 61st pattern.
// The Linux jobs and every other optimised build stay exhaustive. An odd step visits
// all values of each byte.
#if defined( _DEBUG )
const int nPatternStep = 61;
#else
const int nPatternStep = 1;
#endif

// Sampling must not lose the zero vector, whose inf/NaN path is the reason for half
// of the degenerate matrices, so a sampled sweep ends on it.
const uint32_t nZeroVectorPattern = 0x808080u;

int PatternCount()
{
	return ( ( 1 << 24 ) - 1 ) / nPatternStep + 1 + ( nPatternStep > 1 ? 1 : 0 );
}

uint32_t Pattern( const int k )
{
	const uint32_t nPattern = static_cast<uint32_t>( k ) * static_cast<uint32_t>( nPatternStep );
	return nPattern < ( 1u << 24 ) ? nPattern : nZeroVectorPattern;
}

#if BK2_NORMALS_SSE2

// The decode table must be the division's bits, byte for byte; the kernels that use
// it are only exact if it is.
TEST( NormalTransform, DecodeTableIsTheDivision )
{
	const float *lut = NNormalVariants::DecodeTable();
	for ( int b = 0; b < 256; ++b )
	{
		const float f = NNormals::NDetail::DecodeScalar( static_cast<uint32_t>( b ) );
		ASSERT_EQ( 0, std::memcmp( &f, &lut[b], sizeof( f ) ) ) << "byte " << b;
	}
	// The double-precision decode's claim, that rounding ( b - 128 ) * ( 1 / 127 )
	// from double to float gives the correctly rounded quotient, for all 256 bytes.
	for ( int b = 0; b < 256; b += 4 )
	{
		alignas( 16 ) float f[4];
		_mm_store_ps( f, NNormalVariants::Decode4Double( _mm_setr_epi32( b, b + 1, b + 2, b + 3 ) ) );
		ASSERT_EQ( 0, std::memcmp( f, &lut[b], sizeof( f ) ) ) << "bytes " << b << ".." << b + 3;
	}
}

// Every x, y, z byte pattern, every matrix, every variant: the bytes must be the
// reference's.
TEST( NormalTransform, AllByteCombinationsBitExact )
{
	const int N = PatternCount();
	std::vector<uint32_t> src( N ), expected( N ), live( N ), got( N );
	std::mt19937 rng( 7 );
	for ( int k = 0; k < N; ++k )
	{
		src[k] = Pattern( k ) | ( ( rng() & 0xffu ) << 24 );
	}
	const SNormalVariant *pVariants = nullptr;
	const int nVariants = AvailableVariants( &pVariants );
	std::vector<long long> mismatches( nVariants, 0 );
	long long nCompared = 0;
	for ( const SNamedMatrix &nm : TestMatrices() )
	{
		SCOPED_TRACE( nm.pszName );
		NNormalVariants::Ref( SNormalCall{ src.data(), 4, expected.data(), N, nm.m } );

		// The frozen copy against the code the engine actually compiles today.
		SHMatrix matrix;
		std::memcpy( &matrix, nm.m, sizeof( matrix ) );
		const NGfx::SCompactVector *pSrc = reinterpret_cast<const NGfx::SCompactVector *>( src.data() );
		NGfx::SCompactVector *pLive = reinterpret_cast<NGfx::SCompactVector *>( live.data() );
		for ( int k = 0; k < N; ++k )
		{
			MMXTransformVector( pLive[k], pSrc[k], matrix );
		}
		long long nLiveDiff = 0;
		for ( int k = 0; k < N; ++k )
		{
			nLiveDiff += live[k] != expected[k];
		}
		EXPECT_EQ( 0, nLiveDiff ) << "the frozen reference no longer matches GSSEtransform.h's MMXTransformVector";

		for ( int v = 0; v < nVariants; ++v )
		{
			if ( !IsRunnable( pVariants[v] ) )
			{
				continue;
			}
			std::fill( got.begin(), got.end(), 0x5a5a5a5au );
			pVariants[v].pFunc( SNormalCall{ src.data(), 4, got.data(), N, nm.m } );
			long long nDiff = 0;
			int nFirst = -1;
			for ( int k = 0; k < N; ++k )
			{
				if ( got[k] != expected[k] )
				{
					if ( nFirst < 0 )
					{
						nFirst = k;
					}
					++nDiff;
				}
			}
			mismatches[v] += nDiff;
			EXPECT_EQ( 0, nDiff ) << pVariants[v].pszName << ": first at src " << std::hex << src[nFirst >= 0 ? nFirst : 0]
														<< " expected " << expected[nFirst >= 0 ? nFirst : 0] << " got "
														<< got[nFirst >= 0 ? nFirst : 0];
		}
		nCompared += N;
	}
	for ( int v = 0; v < nVariants; ++v )
	{
		std::printf( "%-16s %s  %lld normals compared, %lld mismatches\n",
		             pVariants[v].pszName,
		             IsRunnable( pVariants[v] ) ? "ran    " : "skipped",
		             IsRunnable( pVariants[v] ) ? nCompared : 0,
		             mismatches[v] );
	}
}

#endif

// TransformNormalBlend2/3 (and the scalar TransformBlendScalar) against the live glm
// MMXTransformVector2/3 in GSSEtransform.h; there is no frozen copy, these are
// compared with whatever the engine compiles. Every byte pattern through a few fixed
// bone pairs and triples, then random normals, bones and weights (zero weights
// included).
struct SBlendStats
{
	long long nCompared = 0, nSSE = 0, nScalar = 0;
};

void CheckBlend(
		uint32_t src, const float *m1, int w1, const float *m2, int w2, const float *m3, int w3, SBlendStats &st )
{
	SHMatrix a, b, c;
	std::memcpy( &a, m1, sizeof( a ) );
	std::memcpy( &b, m2, sizeof( b ) );
	NGfx::SCompactVector in, out;
	in.dw = src;
	uint32_t nSSE;
	if ( m3 )
	{
		std::memcpy( &c, m3, sizeof( c ) );
		MMXTransformVector3(
				out, in, a, static_cast<uint8_t>( w1 ), b, static_cast<uint8_t>( w2 ), c, static_cast<uint8_t>( w3 ) );
		nSSE = NNormals::TransformNormalBlend3( src, m1, w1, m2, w2, m3, w3 );
	}
	else
	{
		MMXTransformVector2( out, in, a, static_cast<uint8_t>( w1 ), b, static_cast<uint8_t>( w2 ) );
		nSSE = NNormals::TransformNormalBlend2( src, m1, w1, m2, w2 );
	}
	const uint32_t nScalar = NNormals::NDetail::TransformBlendScalar( src, m1, w1, m2, w2, m3, w3 );
	++st.nCompared;
	st.nSSE += nSSE != out.dw;
	st.nScalar += nScalar != out.dw;
}

TEST( NormalTransform, BlendMatchesGlm )
{
	std::mt19937 rng( 31337 );
	float m[3][16];
	for ( auto &mm : m )
	{
		RandomMatrix( rng, mm );
	}
	SBlendStats st;
	// Every byte pattern, w random, through fixed pairs and triples.
	const int fixed[][3] = { { 128, 127, 0 }, { 200, 55, 0 }, { 100, 80, 75 }, { 85, 85, 85 } };
	for ( const auto &w : fixed )
	{
		const int nPatterns = PatternCount();
		for ( int k = 0; k < nPatterns; ++k )
		{
			const uint32_t src = Pattern( k ) | ( ( rng() & 0xffu ) << 24 );
			CheckBlend( src, m[0], w[0], m[1], w[1], w[2] ? m[2] : nullptr, w[2], st );
		}
	}
	// Random everything, now and then one of the degenerate matrices of the rigid test.
	const std::vector<SNamedMatrix> special = TestMatrices();
	std::uniform_int_distribution<int> byte( 0, 255 );
	for ( int n = 0; n < 4000000; ++n )
	{
		if ( n % 1024 == 0 )
		{
			for ( auto &mm : m )
			{
				if ( byte( rng ) < 32 )
				{
					std::memcpy( mm, special[rng() % special.size()].m, sizeof( mm ) );
				}
				else
				{
					RandomMatrix( rng, mm );
				}
			}
		}
		const bool bThree = ( rng() & 1 ) != 0;
		CheckBlend( static_cast<uint32_t>( rng() ),
		            m[0],
		            byte( rng ),
		            m[1],
		            byte( rng ),
		            bThree ? m[2] : nullptr,
		            bThree ? byte( rng ) : 0,
		            st );
	}
	std::printf( "blend: %lld normals compared, SSE2 %lld mismatches, scalar %lld mismatches\n",
	             st.nCompared,
	             st.nSSE,
	             st.nScalar );
	EXPECT_EQ( 0, st.nSSE );
	EXPECT_EQ( 0, st.nScalar );
}

// One part: nCount normals at nStride, the source ending at the guard page right
// after the last normal's four bytes and the result right after its last dword. The
// dword before the result holds a sentinel that must survive.
void CheckPart( const SNormalVariant &v, std::mt19937 &rng, int nCount, int nStride, const float *pMatrix )
{
	const size_t nSrcBytes = nCount > 0 ? size_t( nCount - 1 ) * nStride + 4 : 0;
	CGuardedBytes src( nSrcBytes ), dst( 4 * size_t( nCount + 1 ) );
	ASSERT_TRUE( src.IsValid() && dst.IsValid() );
	for ( size_t b = 0; b < nSrcBytes; ++b )
	{
		src.data()[b] = static_cast<uint8_t>( rng() );
	}
	uint32_t *pDst = reinterpret_cast<uint32_t *>( dst.data() ) + 1;
	for ( int k = -1; k < nCount; ++k )
	{
		const uint32_t nSentinel = 0xdeadbeefu;
		std::memcpy( pDst + k, &nSentinel, 4 );
	}
	v.pFunc( SNormalCall{ src.data(), nStride, pDst, nCount, pMatrix } );
	uint32_t nBefore;
	std::memcpy( &nBefore, pDst - 1, 4 );
	ASSERT_EQ( 0xdeadbeefu, nBefore ) << "wrote before the result array, size " << nCount;
	for ( int k = 0; k < nCount; ++k )
	{
		uint32_t nSrc, nGot;
		std::memcpy( &nSrc, src.data() + size_t( k ) * nStride, 4 );
		std::memcpy( &nGot, pDst + k, 4 );
#if BK2_NORMALS_SSE2
		const uint32_t nExpected = NNormalVariants::ReferenceTransform( nSrc, pMatrix );
#else
		uint32_t nExpected;
		NNormals::TransformNormalsRigidScalar( &nSrc, 4, &nExpected, 1, pMatrix );
#endif
		ASSERT_EQ( nExpected, nGot ) << "size " << nCount << " stride " << nStride << " normal " << k;
	}
}

// Every size up to 40 (all the tail shapes of the four- and eight-wide loops, and
// the small paths), then random sizes up to 5000, at the SUVInfo stride the engine
// passes and at others.
TEST( NormalTransform, PartSizesAndStridesInsideGuardPages )
{
	std::mt19937 rng( std::random_device{}() );
	std::uniform_int_distribution<int> sizeDist( 0, 5000 );
	const int strides[] = { 4, 8, static_cast<int>( sizeof( NGScene::SUVInfo ) ), 24, 36 };
	const SNormalVariant *pVariants = nullptr;
	const int nVariants = AvailableVariants( &pVariants );
	for ( int nCase = 0; nCase < 300; ++nCase )
	{
		const int nCount = nCase <= 40 ? nCase : sizeDist( rng );
		float m[16];
		RandomMatrix( rng, m );
		for ( const int nStride : strides )
		{
			for ( int v = 0; v < nVariants; ++v )
			{
				if ( !IsRunnable( pVariants[v] ) )
				{
					continue;
				}
				SCOPED_TRACE( pVariants[v].pszName );
				CheckPart( pVariants[v], rng, nCount, nStride, m );
				if ( HasFatalFailure() )
				{
					return;
				}
			}
		}
	}
}

// A matrix that is itself the last thing before a guard page: a kernel reading more
// than the sixteen floats of an SHMatrix crashes.
TEST( NormalTransform, MatrixReadStaysInSixteenFloats )
{
	std::mt19937 rng( 99 );
	CGuardedBytes matrix( 64 );
	ASSERT_TRUE( matrix.IsValid() );
	float m[16];
	RandomMatrix( rng, m );
	std::memcpy( matrix.data(), m, sizeof( m ) );
	const SNormalVariant *pVariants = nullptr;
	const int nVariants = AvailableVariants( &pVariants );
	for ( int v = 0; v < nVariants; ++v )
	{
		if ( IsRunnable( pVariants[v] ) )
		{
			SCOPED_TRACE( pVariants[v].pszName );
			CheckPart( pVariants[v], rng, 37, 20, reinterpret_cast<const float *>( matrix.data() ) );
		}
	}
}

// The guard page is what makes "no variant reads past an array" a checked claim, so
// check the guard itself: one byte past the end must fault.
TEST( NormalTransformDeathTest, GuardPageCatchesOverRead )
{
	GTEST_FLAG_SET( death_test_style, "threadsafe" );
	EXPECT_DEATH(
			{
				CGuardedBytes a( 5 );
				volatile uint8_t b = a.data()[5];
				(void)b;
			},
			"" );
}

} // namespace
