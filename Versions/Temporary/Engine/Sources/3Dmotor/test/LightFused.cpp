// Holds every kernel of GLightPerVertexFused.h (scalar, SSE2, AVX2) to what the
// engine computes today, bit for bit:
//
//   directional  refLightingKernels (the mmx:: transcription of the 2005 MMX), and the
//                shipped sse2LightingKernels / avx2LightingKernels, over all 2^24
//                x, y, z byte patterns of the packed normal (w random) through
//                several light states, then normal streams with runs of repeated
//                normals and with leading zero normals (the original's one quirk)
//   multiply     a frozen copy of GLightPerVertex.cpp's MultiplyOnColor, every
//                (a, b) byte pair in every byte lane; also proves its min( 255, ... )
//                never fires
//   assembly     a frozen copy of CalcPerVertexLight's SGeomVecFull output loop fed
//                by the shipped directional kernel and the frozen multiply: every
//                part size 0..40 and random ones up to 3000, vertex colour present
//                or not, fog indexed or not, plain, streaming and both destinations,
//                colour outputs or not. Every input and output array ends exactly at
//                an inaccessible page, so a kernel touching a byte past one crashes.
//
// The frozen copies are what the engine source said when this file was written. If
// GLightPerVertex.cpp's loops change, these copies must change with them.

#include "3Dmotor/stdafx.h"

#include "3DLib/GGeometry.h"
#include "3Dmotor/GfxBuffers.h"
#include "3Dmotor/GLightPerVertex.h"
#include "3Dmotor/GLightPerVertexFused.h"
#include "3Dmotor/GLightPerVertexKernels.h"
// The initialised nCubicRoot table the lighting kernel sources reference.
#include "3Dmotor/GSSEtransform.cpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#if defined( _WIN32 )
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

// The header's raw layouts must be the engine's.
static_assert( sizeof( NLightFused::SVertexFull ) == sizeof( NGfx::SGeomVecFull ), "vertex size" );
static_assert( offsetof( NGfx::SGeomVecFull, pos ) == offsetof( NLightFused::SVertexFull, pos ), "pos" );
static_assert( offsetof( NGfx::SGeomVecFull, normal ) == offsetof( NLightFused::SVertexFull, normal ), "normal" );
static_assert( offsetof( NGfx::SGeomVecFull, tex ) == offsetof( NLightFused::SVertexFull, tex ), "tex" );
static_assert( offsetof( NGfx::SGeomVecFull, texLM ) == offsetof( NLightFused::SVertexFull, texLM ), "texLM" );
static_assert( offsetof( NGfx::SGeomVecFull, texU ) == offsetof( NLightFused::SVertexFull, texU ), "texU" );
static_assert( offsetof( NGfx::SGeomVecFull, texV ) == offsetof( NLightFused::SVertexFull, texV ), "texV" );
static_assert( sizeof( NGScene::SUVInfo ) == 20 && offsetof( NGScene::SUVInfo, tex ) == 0 &&
                   offsetof( NGScene::SUVInfo, texLM ) == 4,
               "tex and texLM where SVertexCall reads them" );
static_assert( sizeof( CVec3 ) == 12, "three floats per position" );
static_assert( sizeof( NGfx::SMMXWord ) == 8 && offsetof( NGfx::SMMXWord, nZ ) == 0 &&
                   offsetof( NGfx::SMMXWord, nW ) == 6,
               "SMMXWord is z, y, x, w shorts like SDirectionalConsts' rows" );

namespace
{

using NLightFused::SDirectionalConsts;
using NLightFused::SFusedLightKernels;
using NLightFused::SVertexCall;
using NLightFused::SVertexFull;

// ---------------------------------------------------------------------------------
// Frozen copies of the engine code being replaced.

// GLightPerVertex.cpp, MultiplyOnColor, verbatim apart from the name.
void FrozenMultiplyOnColor( std::vector<uint32_t> *pRes, const std::vector<uint32_t> &mult )
{
	if ( mult.empty() )
	{
		return;
	}
	uint32_t *pDst = &( *pRes )[0], *pDstEnd = pDst + pRes->size();
	const uint32_t *pSrc = &mult[0];
	for ( ; pDst < pDstEnd; ++pDst, ++pSrc )
	{
		NGfx::SPixel8888 src = *pSrc;
		NGfx::SPixel8888 dst = *pDst;

		dst.r = std::min<int>( 255, ( src.r * dst.r ) >> 8 );
		dst.g = std::min<int>( 255, ( src.g * dst.g ) >> 8 );
		dst.b = std::min<int>( 255, ( src.b * dst.b ) >> 8 );
		dst.a = std::min<int>( 255, ( src.a * dst.a ) >> 8 );

		*pDst = dst.dwColor;
	}
}

// GLightPerVertex.cpp, CalcPerVertexLight( SGeomVecFull * ), the lit branch of the
// output loop, with SPVLightCalcer's members as parameters.
void FrozenOutputLoop( NGfx::SGeomVecFull *pRes,
                       const std::vector<CVec3> &srcPos,
                       const NGScene::SUVInfo *pSrc,
                       const std::vector<uint16_t> &posIndices,
                       const std::vector<NGfx::SCompactVector> &_normals,
                       const std::vector<uint32_t> &colors,
                       const std::vector<uint32_t> &shadowColors,
                       const unsigned char *pWarFogOld,
                       const unsigned char *pWarFogNew,
                       int nWarFogMask )
{
	int k = 0;
	for ( const NGScene::SUVInfo *pEnd = pSrc + posIndices.size(); pSrc < pEnd; ++pSrc, ++pRes, ++k )
	{
		NGfx::SGeomVecFull &res = *pRes;
		const CVec3 &pos = srcPos[posIndices[k]];
		res.pos = pos;
		res.normal = _normals[k];
		res.tex = pSrc->tex;
		res.texLM = pSrc->texLM;
		int nWarFogIndex = posIndices[k] & nWarFogMask;
		res.texU.dw = ( shadowColors[k] & 0xffffff ) | ( pWarFogOld[nWarFogIndex] << 24 );
		res.texV.dw = ( colors[k] & 0xffffff ) | ( pWarFogNew[nWarFogIndex] << 24 );
	}
}

// ---------------------------------------------------------------------------------

std::mt19937 rng( 0x11F05EDu );

uint32_t Rand32()
{
	return static_cast<uint32_t>( rng() );
}

NGfx::SMMXWord RandomWord()
{
	NGfx::SMMXWord w;
	w.nX = static_cast<short>( Rand32() );
	w.nY = static_cast<short>( Rand32() );
	w.nZ = static_cast<short>( Rand32() );
	w.nW = static_cast<short>( Rand32() );
	return w;
}

// A light state plus the translucent shade, as the engine hands them to the kernels,
// and the same seven words for the fused kernels.
struct SLight
{
	const char *pszName;
	NGScene::SPerVertexLightState ls;
	NGfx::SMMXWord translucent;
	SDirectionalConsts consts;
};

void FillConsts( SLight *p )
{
	const NGfx::SMMXWord *src[7] = { &p->ls.shift,
	                                 &p->ls.dirLight,
	                                 &p->ls.ambient,
	                                 &p->ls.lightColor,
	                                 &p->ls.incidentShadowColor,
	                                 &p->ls.shadeColor,
	                                 &p->translucent };
	int16_t *dst[7] = { p->consts.shift,
	                    p->consts.dirLight,
	                    p->consts.ambient,
	                    p->consts.lightColor,
	                    p->consts.incidentShadowColor,
	                    p->consts.shadeColor,
	                    p->consts.translucentShade };
	for ( int i = 0; i < 7; ++i )
	{
		std::memcpy( dst[i], src[i], 8 );
	}
}

// Realistic states through the engine's own SetDirectional (as the game sets the sun),
// then fully random words, which exercise every saturation and the wrapping madd.
std::vector<SLight> TestLights()
{
	std::vector<SLight> v;
	{
		SLight l{ "sun, opaque" };
		l.ls.SetDirectional( CVec3( 0.35f, 0.33f, 0.3f ),
		                     CVec3( 0.9f, 0.85f, 0.7f ),
		                     CVec3( 0.2f, 0.22f, 0.3f ),
		                     CVec3( 0.5f, 0.45f, 0.4f ),
		                     CVec3( 0.48f, -0.6f, -0.64f ),
		                     1,
		                     CVec3( 1, 1, 1 ) );
		l.translucent = l.ls.shadeColor;
		v.push_back( l );
	}
	{
		SLight l{ "bright sun, translucent" };
		l.ls.SetDirectional( CVec3( 0.9f, 0.9f, 0.9f ),
		                     CVec3( 1.9f, 1.8f, 1.7f ),
		                     CVec3( 0.6f, 0.62f, 0.7f ),
		                     CVec3( 1.5f, 1.45f, 1.4f ),
		                     CVec3( -0.8f, 0.0f, -0.6f ),
		                     2,
		                     CVec3( 1, 1, 1 ) );
		l.translucent.nX = 9000;
		l.translucent.nY = 12000;
		l.translucent.nZ = 3000;
		l.translucent.nW = 0;
		v.push_back( l );
	}
	for ( const char *psz : { "random words 1", "random words 2", "random words 3" } )
	{
		SLight l{ psz };
		l.ls.ambient = RandomWord();
		l.ls.lightColor = RandomWord();
		l.ls.incidentShadowColor = RandomWord();
		l.ls.shadeColor = RandomWord();
		l.ls.dirLight = RandomWord();
		l.ls.shift = RandomWord();
		l.translucent = RandomWord();
		v.push_back( l );
	}
	{
		// -32768 everywhere: the one input on which pmaddwd's sum wraps.
		SLight l{ "all -32768" };
		NGfx::SMMXWord m;
		m.nX = m.nY = m.nZ = m.nW = -32768;
		l.ls.ambient = l.ls.lightColor = l.ls.incidentShadowColor = l.ls.shadeColor = m;
		l.ls.dirLight = m;
		l.ls.shift = NGfx::SMMXWord();
		l.translucent = m;
		v.push_back( l );
	}
	for ( SLight &l : v )
	{
		FillConsts( &l );
	}
	return v;
}

// The fused sets this CPU can run.
std::vector<const SFusedLightKernels *> FusedSets()
{
	std::vector<const SFusedLightKernels *> v = { &NLightFused::scalarFusedLightKernels,
	                                              &NLightFused::sse2FusedLightKernels };
	if ( NGScene::IsAVX2Present() )
	{
		v.push_back( &NLightFused::avx2FusedLightKernels );
		v.push_back( &NLightFused::avx2NoGatherFusedLightKernels );
	}
	return v;
}

// The shipped directional kernels this CPU can run, the reference first.
std::vector<const NGScene::SLightingKernels *> ShippedSets()
{
	std::vector<const NGScene::SLightingKernels *> v = { &NGScene::refLightingKernels };
	if ( NGScene::IsSSE2Present() )
	{
		v.push_back( &NGScene::sse2LightingKernels );
	}
	if ( NGScene::IsAVX2Present() )
	{
		v.push_back( &NGScene::avx2LightingKernels );
	}
	return v;
}

// The exhaustive sweep is quick in optimised builds; the debug CRT samples every 61st
// pattern, the same compromise test/NormalTransform.cpp makes for CI's time limit.
#if defined( _DEBUG )
const int nPatternStep = 61;
#else
const int nPatternStep = 1;
#endif

// nBytes bytes whose last one ends where an inaccessible page begins (as in
// test/NormalTransform.cpp). Start alignment is whatever that leaves, so callers that
// need an aligned start round nBytes up.
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

	template <class T>
	T *As()
	{
		return reinterpret_cast<T *>( pData );
	}
};

// Normals with runs: each vertex repeats the previous normal with probability
// fRepeat, as flat-shaded meshes do, after nZeroPrefix leading zero normals.
std::vector<uint32_t> NormalStream( int n, double fRepeat, int nZeroPrefix )
{
	std::vector<uint32_t> v( n );
	std::bernoulli_distribution repeat( fRepeat );
	for ( int k = 0; k < n; ++k )
	{
		if ( k < nZeroPrefix )
		{
			v[k] = 0;
		}
		else if ( k > 0 && repeat( rng ) )
		{
			v[k] = v[k - 1];
		}
		else
		{
			// Sometimes 0 again after the prefix: computed there, unlike in the prefix.
			v[k] = ( Rand32() % 16 ) == 0 ? 0 : Rand32();
		}
	}
	return v;
}

} // namespace

// ---------------------------------------------------------------------------------

// 255 * 255 >> 8 is 254, so MultiplyOnColor's clamp is dead and dropping it is exact.
TEST( LightFused, MultiplyClampNeverFires )
{
	int nMax = 0;
	for ( int a = 0; a < 256; ++a )
	{
		for ( int b = 0; b < 256; ++b )
		{
			nMax = (std::max)( nMax, ( a * b ) >> 8 );
		}
	}
	EXPECT_EQ( 254, nMax );
	EXPECT_LT( nMax, 255 );
}

// Every (a, b) byte pair in every byte lane, against the frozen engine loop.
TEST( LightFused, MultiplyOnColorAllPairs )
{
	const int N = 65536;
	std::vector<uint32_t> dst( N ), mul( N );
	for ( int i = 0; i < N; ++i )
	{
		const uint32_t a = i & 0xff, b = i >> 8;
		// Lane pairs (a, b), (b, a), (b, ~a), (a, ~b): each covers all 65536 pairs.
		dst[i] = a | ( b << 8 ) | ( b << 16 ) | ( a << 24 );
		mul[i] = b | ( a << 8 ) | ( ( ~a & 0xff ) << 16 ) | ( ( ~b & 0xff ) << 24 );
	}
	std::vector<uint32_t> expected = dst;
	FrozenMultiplyOnColor( &expected, mul );
	for ( const SFusedLightKernels *pSet : FusedSets() )
	{
		// Every count's tail too: the arrays are walked at offsets 0..16.
		for ( int nOff = 0; nOff < 17; ++nOff )
		{
			std::vector<uint32_t> got( dst.begin() + nOff, dst.end() );
			pSet->pMultiplyOnColor( got.data(), mul.data() + nOff, N - nOff );
			long long nDiff = 0;
			for ( int i = 0; i < N - nOff; ++i )
			{
				nDiff += got[i] != expected[i + nOff];
			}
			EXPECT_EQ( 0, nDiff ) << pSet->pszName << " offset " << nOff;
		}
		std::printf( "%-13s multiply: %d pairs x 4 lanes x 17 offsets compared\n", pSet->pszName, N );
	}
}

// All 2^24 x, y, z patterns, w random, through every light state: the fused
// directional kernels against the reference and the shipped kernels, and the shipped
// ones against the reference as well.
TEST( LightFused, DirectionalAllPatterns )
{
	const int N = ( ( 1 << 24 ) + nPatternStep - 1 ) / nPatternStep;
	std::vector<uint32_t> normals( N );
	for ( int k = 0; k < N; ++k )
	{
		normals[k] = static_cast<uint32_t>( k ) * nPatternStep | ( ( Rand32() & 0xffu ) << 24 );
	}
	// Starts on a nonzero normal: the zero-prefix quirk is the stream test's subject;
	// here every pattern, 0x000000 with its w included, has to be computed.
	normals[0] |= 0x01000000u;
	std::vector<uint32_t> refC( N ), refS( N ), gotC( N ), gotS( N );
	const std::vector<const NGScene::SLightingKernels *> shipped = ShippedSets();
	const std::vector<const SFusedLightKernels *> fused = FusedSets();
	std::vector<long long> nShippedDiff( shipped.size(), 0 ), nFusedDiff( fused.size(), 0 );
	long long nCompared = 0;
	for ( const SLight &l : TestLights() )
	{
		SCOPED_TRACE( l.pszName );
		const NGfx::SCompactVector *pN = reinterpret_cast<const NGfx::SCompactVector *>( normals.data() );
		NGScene::refLightingKernels.pCalcDirectionalLighting( pN, N, l.ls, l.translucent, refC.data(), refS.data() );
		for ( size_t s = 1; s < shipped.size(); ++s )
		{
			shipped[s]->pCalcDirectionalLighting( pN, N, l.ls, l.translucent, gotC.data(), gotS.data() );
			long long nDiff = 0;
			for ( int k = 0; k < N; ++k )
			{
				nDiff += ( gotC[k] != refC[k] ) + ( gotS[k] != refS[k] );
			}
			nShippedDiff[s] += nDiff;
			EXPECT_EQ( 0, nDiff ) << "shipped " << shipped[s]->pszName;
		}
		for ( size_t f = 0; f < fused.size(); ++f )
		{
			std::fill( gotC.begin(), gotC.end(), 0x5a5a5a5au );
			std::fill( gotS.begin(), gotS.end(), 0x5a5a5a5au );
			fused[f]->pDirectionalLighting( normals.data(), N, l.consts, gotC.data(), gotS.data() );
			long long nDiff = 0;
			int nFirst = -1;
			for ( int k = 0; k < N; ++k )
			{
				if ( gotC[k] != refC[k] || gotS[k] != refS[k] )
				{
					nFirst = nFirst < 0 ? k : nFirst;
					++nDiff;
				}
			}
			nFusedDiff[f] += nDiff;
			EXPECT_EQ( 0, nDiff ) << fused[f]->pszName << ": first at normal " << std::hex
														<< normals[nFirst < 0 ? 0 : nFirst];
		}
		nCompared += N;
	}
	for ( size_t s = 1; s < shipped.size(); ++s )
	{
		std::printf( "shipped %-6s directional vs ref: %lld normals, %lld mismatches\n",
		             shipped[s]->pszName,
		             nCompared,
		             nShippedDiff[s] );
	}
	for ( size_t f = 0; f < fused.size(); ++f )
	{
		std::printf( "fused   %-13s directional vs ref: %lld normals, %lld mismatches\n",
		             fused[f]->pszName,
		             nCompared,
		             nFusedDiff[f] );
	}
}

// Streams with repeated normals and leading zero normals, every count 0..40 and some
// long ones: the original's reuse of the previous result must be invisible, the
// zero prefix must come out 0, and the fused directional + multiply must equal the
// shipped directional followed by the frozen MultiplyOnColor.
TEST( LightFused, DirectionalStreams )
{
	std::vector<int> counts;
	for ( int n = 0; n <= 40; ++n )
	{
		counts.push_back( n );
	}
	for ( int n : { 63, 64, 65, 80, 257, 600, 1000 } )
	{
		counts.push_back( n );
	}
	long long nCompared = 0, nDiff = 0;
	for ( const SLight &l : TestLights() )
	{
		for ( int n : counts )
		{
			for ( double fRepeat : { 0.0, 0.3, 0.7, 1.0 } )
			{
				for ( int nZero : { 0, 1, 3, 7, 8, 9, 17, 1000 } )
				{
					const std::vector<uint32_t> normals = NormalStream( n, fRepeat, (std::min)( nZero, n ) );
					std::vector<uint32_t> vc( n );
					for ( uint32_t &c : vc )
					{
						c = Rand32();
					}
					std::vector<uint32_t> refC( n ), refS( n );
					const NGfx::SCompactVector *pN = reinterpret_cast<const NGfx::SCompactVector *>( normals.data() );
					// The engine adapter's resize-then-call, so n == 0 is not called.
					if ( n > 0 )
					{
						NGScene::refLightingKernels.pCalcDirectionalLighting(
								pN, n, l.ls, l.translucent, refC.data(), refS.data() );
					}
					for ( const NGScene::SLightingKernels *pShipped : ShippedSets() )
					{
						std::vector<uint32_t> c( n ), s( n );
						if ( n > 0 )
						{
							pShipped->pCalcDirectionalLighting( pN, n, l.ls, l.translucent, c.data(), s.data() );
						}
						nDiff += c != refC || s != refS;
						EXPECT_TRUE( c == refC && s == refS ) << "shipped " << pShipped->pszName << " n=" << n;
					}
					std::vector<uint32_t> mulC = refC, mulS = refS;
					FrozenMultiplyOnColor( &mulC, vc );
					FrozenMultiplyOnColor( &mulS, vc );
					for ( const SFusedLightKernels *pSet : FusedSets() )
					{
						CGuardedBytes gN( 4 * n ), gVC( 4 * n ), gC( 4 * n ), gS( 4 * n );
						std::memcpy( gN.As<uint32_t>(), normals.data(), 4 * n );
						std::memcpy( gVC.As<uint32_t>(), vc.data(), 4 * n );
						pSet->pDirectionalLighting( gN.As<uint32_t>(), n, l.consts, gC.As<uint32_t>(), gS.As<uint32_t>() );
						const bool bDir = n == 0 || ( std::memcmp( gC.As<uint32_t>(), refC.data(), 4 * n ) == 0 &&
						                              std::memcmp( gS.As<uint32_t>(), refS.data(), 4 * n ) == 0 );
						pSet->pDirectionalMultiply(
								gN.As<uint32_t>(), n, l.consts, gVC.As<uint32_t>(), gC.As<uint32_t>(), gS.As<uint32_t>() );
						const bool bMul = n == 0 || ( std::memcmp( gC.As<uint32_t>(), mulC.data(), 4 * n ) == 0 &&
						                              std::memcmp( gS.As<uint32_t>(), mulS.data(), 4 * n ) == 0 );
						pSet->pDirectionalMultiply( gN.As<uint32_t>(), n, l.consts, nullptr, gC.As<uint32_t>(), gS.As<uint32_t>() );
						const bool bNoVC = n == 0 || ( std::memcmp( gC.As<uint32_t>(), refC.data(), 4 * n ) == 0 &&
						                               std::memcmp( gS.As<uint32_t>(), refS.data(), 4 * n ) == 0 );
						nDiff += !bDir + !bMul + !bNoVC;
						EXPECT_TRUE( bDir && bMul && bNoVC )
								<< pSet->pszName << " " << l.pszName << " n=" << n << " repeat=" << fRepeat << " zero prefix=" << nZero;
					}
					nCompared += n;
				}
			}
		}
	}
	std::printf( "directional streams: %lld vertices per kernel, %lld mismatching streams\n", nCompared, nDiff );
}

// The fused assembly against the engine pipeline: shipped directional, frozen
// MultiplyOnColor twice, frozen output loop.
TEST( LightFused, AssembleMatchesPipeline )
{
	const NGScene::SLightingKernels &shipped =
			NGScene::IsAVX2Present() ? NGScene::avx2LightingKernels : NGScene::sse2LightingKernels;
	std::vector<int> counts;
	for ( int n = 0; n <= 40; ++n )
	{
		counts.push_back( n );
	}
	std::uniform_int_distribution<int> big( 41, 3000 );
	for ( int i = 0; i < 40; ++i )
	{
		counts.push_back( big( rng ) );
	}
	const std::vector<SLight> lights = TestLights();
	long long nCalls = 0, nVertices = 0, nBad = 0;
	for ( int n : counts )
	{
		for ( int nVariant = 0; nVariant < 16; ++nVariant )
		{
			const SLight &l = lights[( n + nVariant ) % lights.size()];
			const bool bVC = ( nVariant & 1 ) != 0;
			const bool bIndexedFog = ( nVariant & 2 ) != 0;
			const double fRepeat = ( nVariant & 4 ) ? 0.5 : 0.0;
			const int nZero = ( nVariant & 8 ) ? 1 + static_cast<int>( Rand32() % 12 ) : 0;
			// Positions: fewer than vertices, as a mesh shares them; at least one.
			const int nPos = (std::max)( 1, n / 2 + static_cast<int>( Rand32() % 5 ) );

			std::vector<CVec3> srcPos( nPos );
			for ( CVec3 &p : srcPos )
			{
				p = CVec3( static_cast<float>( Rand32() % 2000 ) - 1000.5f,
				           static_cast<float>( Rand32() % 2000 ) * 0.25f,
				           static_cast<float>( Rand32() ) * 1e-6f );
			}
			std::vector<uint16_t> posIndices( n );
			for ( int k = 0; k < n; ++k )
			{
				// The last position used often: the 16-byte position load must not
				// reach past it.
				posIndices[k] = static_cast<uint16_t>( ( Rand32() % 4 ) == 0 ? nPos - 1 : Rand32() % nPos );
			}
			std::vector<NGScene::SUVInfo> uv( n );
			for ( NGScene::SUVInfo &u : uv )
			{
				u.tex.nU = static_cast<short>( Rand32() );
				u.tex.nV = static_cast<short>( Rand32() );
				u.texLM.nU = static_cast<short>( Rand32() );
				u.texLM.nV = static_cast<short>( Rand32() );
				u.normal.dw = Rand32();
				u.texU.dw = Rand32();
				u.texV.dw = Rand32();
			}
			const std::vector<uint32_t> normalsRaw = NormalStream( n, fRepeat, (std::min)( nZero, n ) );
			std::vector<NGfx::SCompactVector> normals( n );
			std::memcpy( normals.data(), normalsRaw.data(), 4 * n );
			std::vector<uint32_t> vc;
			if ( bVC )
			{
				vc.resize( n );
				for ( uint32_t &c : vc )
				{
					c = Rand32();
				}
			}
			// Indexed fog is one byte per position; unindexed, the engine points both at
			// a single byte (cFogHold) or a one-element sample.
			const int nFog = bIndexedFog ? nPos : 1;
			std::vector<uint8_t> fogOld( nFog ), fogNew( nFog );
			for ( int i = 0; i < nFog; ++i )
			{
				fogOld[i] = static_cast<uint8_t>( Rand32() );
				fogNew[i] = static_cast<uint8_t>( Rand32() );
			}
			const int nMask = bIndexedFog ? -1 : 0;

			// The engine pipeline.
			std::vector<uint32_t> colors( n ), shadow( n );
			if ( n > 0 )
			{
				shipped.pCalcDirectionalLighting( normals.data(), n, l.ls, l.translucent, colors.data(), shadow.data() );
			}
			FrozenMultiplyOnColor( &shadow, vc );
			FrozenMultiplyOnColor( &colors, vc );
			std::vector<NGfx::SGeomVecFull> expected( n );
			FrozenOutputLoop( expected.data(),
			                  srcPos,
			                  uv.data(),
			                  posIndices,
			                  normals,
			                  colors,
			                  shadow,
			                  fogOld.data(),
			                  fogNew.data(),
			                  nMask );

			// Inputs and outputs each ending at a guard page.
			CGuardedBytes gPos( sizeof( CVec3 ) * nPos ), gIdx( 2 * n ), gUV( sizeof( NGScene::SUVInfo ) * n ), gN( 4 * n ),
					gVC( 4 * n ), gFo( nFog ), gFn( nFog ), gRes( 32 * n ), gStream( 32 * n ), gC( 4 * n ), gS( 4 * n ),
					gCin( 4 * n ), gSin( 4 * n );
			std::memcpy( gPos.As<void>(), srcPos.data(), sizeof( CVec3 ) * nPos );
			std::memcpy( gIdx.As<void>(), posIndices.data(), 2 * n );
			std::memcpy( gUV.As<void>(), uv.data(), sizeof( NGScene::SUVInfo ) * n );
			std::memcpy( gN.As<void>(), normalsRaw.data(), 4 * n );
			if ( bVC )
			{
				std::memcpy( gVC.As<void>(), vc.data(), 4 * n );
			}
			std::memcpy( gFo.As<void>(), fogOld.data(), nFog );
			std::memcpy( gFn.As<void>(), fogNew.data(), nFog );

			for ( const SFusedLightKernels *pSet : FusedSets() )
			{
				for ( int nDest = 0; nDest < 3; ++nDest )
				{
					// 0: plain stores only, 1: streaming only, 2: both.
					SVertexCall call;
					call.pRes = nDest != 1 ? gRes.As<SVertexFull>() : nullptr;
					call.pStream = nDest != 0 ? gStream.As<SVertexFull>() : nullptr;
					call.pPositions = gPos.As<float>();
					call.nPositions = nPos;
					call.pPosIndices = gIdx.As<uint16_t>();
					call.pUV = gUV.As<uint8_t>();
					call.nUVStride = sizeof( NGScene::SUVInfo );
					call.pNormals = gN.As<uint32_t>();
					call.pFogOld = gFo.As<uint8_t>();
					call.pFogNew = gFn.As<uint8_t>();
					call.nFogMask = static_cast<uint32_t>( nMask );
					call.nCount = n;
					const uint32_t *pVC = bVC ? gVC.As<uint32_t>() : nullptr;
					const bool bColorsOut = ( nDest + n ) % 2 == 0;

					auto check = [&]( const char *pszWhat, bool bCheckColors )
					{
						bool bOk = true;
						if ( call.pRes )
						{
							bOk &= n == 0 || std::memcmp( gRes.As<void>(), expected.data(), 32 * n ) == 0;
						}
						if ( call.pStream )
						{
							bOk &= n == 0 || std::memcmp( gStream.As<void>(), expected.data(), 32 * n ) == 0;
						}
						if ( bCheckColors )
						{
							bOk &= n == 0 || ( std::memcmp( gC.As<void>(), colors.data(), 4 * n ) == 0 &&
							                   std::memcmp( gS.As<void>(), shadow.data(), 4 * n ) == 0 );
						}
						nBad += !bOk;
						EXPECT_TRUE( bOk ) << pSet->pszName << " " << pszWhat << " n=" << n << " variant=" << nVariant
															 << " dest=" << nDest;
					};

					// Light, multiply and assemble in one pass.
					std::memset( gRes.As<void>(), 0xcd, 32 * n );
					std::memset( gStream.As<void>(), 0xcd, 32 * n );
					std::memset( gC.As<void>(), 0xcd, 4 * n );
					std::memset( gS.As<void>(), 0xcd, 4 * n );
					pSet->pLightAndAssemble(
							call, l.consts, pVC, bColorsOut ? gC.As<uint32_t>() : nullptr, bColorsOut ? gS.As<uint32_t>() : nullptr );
					NLightFused::StreamFence();
					check( "LightAndAssemble", bColorsOut );

					// Assembly from the multiplied colours (the cache-hit path).
					std::memcpy( gCin.As<void>(), colors.data(), 4 * n );
					std::memcpy( gSin.As<void>(), shadow.data(), 4 * n );
					std::memset( gRes.As<void>(), 0xcd, 32 * n );
					std::memset( gStream.As<void>(), 0xcd, 32 * n );
					pSet->pAssembleVertices( call, nullptr, gCin.As<uint32_t>(), gSin.As<uint32_t>() );
					NLightFused::StreamFence();
					check( "AssembleVertices", false );

					// Multiply and assembly from the unmultiplied directional colours.
					std::vector<uint32_t> rawC( n ), rawS( n );
					if ( n > 0 )
					{
						shipped.pCalcDirectionalLighting( normals.data(), n, l.ls, l.translucent, rawC.data(), rawS.data() );
					}
					std::memcpy( gCin.As<void>(), rawC.data(), 4 * n );
					std::memcpy( gSin.As<void>(), rawS.data(), 4 * n );
					std::memset( gRes.As<void>(), 0xcd, 32 * n );
					std::memset( gStream.As<void>(), 0xcd, 32 * n );
					pSet->pAssembleVertices( call, pVC, gCin.As<uint32_t>(), gSin.As<uint32_t>() );
					NLightFused::StreamFence();
					check( "multiply + AssembleVertices", false );
					++nCalls;
				}
			}
			nVertices += n;
		}
	}
	std::printf( "assembly: %lld part calls (x3 entry points), %lld distinct part vertices, %lld mismatching calls\n",
	             nCalls,
	             nVertices,
	             nBad );
}
