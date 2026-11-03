// Holds the fused war fog kernels of GWarFogSample.h to the shipped kernel sets bit
// for bit: sse2LightingKernels and avx2LightingKernels, called the way
// GLightPerVertex.cpp's SampleWarFog calls them (pSampleWarFogCoords, then
// pSampleWarFogInt once per grid).
//
// Three layers, so that a mismatch says where it is:
//
// - Integer stage: every one of the 2^28 x and y fractions (sampled in _DEBUG),
//   with random cells including negative and far out-of-grid ones, on a random grid
//   and on two extreme ones (all 255, a 0/255 checkerboard), against the shipped
//   pSampleWarFogInt of both sets and the reference transcription.
// - Coordinate stage: every one of the 2^32 float bit patterns as x (and, rotated,
//   as y), NaN, infinities and denormals included, at the scale the game uses and at
//   other ones, against the shipped pSampleWarFogCoords of both sets. Compared as
//   integers, not as fog bytes, which would hide a coordinate off by one. The scalar
//   reference set is compared too, and the lanes where it differs are counted and
//   required to be the ones lrintf leaves undefined.
// - The fused kernels end to end on realistic parts and on hostile positions (cell
//   borders and the floats beside them, rounding ties, negative, out of grid, NaN),
//   every count from 0 to 40 plus random ones up to 3000, one grid and two, every
//   realistic grid size and some unrealistic ones, with the positions, both results
//   and the grid each ending at an inaccessible page and the grid also starting
//   right after one, so a read or write outside them crashes the test.

#include "3Dmotor/stdafx.h"

#include "3Dmotor/GLightPerVertexKernels.h"
#include "3Dmotor/GSSEtransform.h"
// Brings the initialised nCubicRoot the lighting kernel sources reference.
#include "3Dmotor/GSSEtransform.cpp"
#include "3Dmotor/GWarFogSample.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <random>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#if defined( _WIN32 )
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

// The kernels read packed xyz triples where the engine has CVec3.
static_assert( sizeof( CVec3 ) == 12, "a position is three packed floats" );

namespace
{

// Fixed seed: a failure has to be reproducible to be bisected.
std::mt19937 rng( 0xF06F06u );

// The scale InterfaceMissionInternal.cpp passes: one over the 2.75 world unit cell.
const float F_GAME_SCALE = 1.0f / 2.75f;

#if defined( _DEBUG )
// The debug CRT and unoptimised kernels make the exhaustive sweeps take hours.
const int N_SWEEP_STEP = 97;
#else
const int N_SWEEP_STEP = 1;
#endif

// nBytes bytes surrounded by inaccessible pages; either the first byte sits right
// after the front guard or the last byte right before the back guard.
class CGuardedBytes
{
	size_t nPage = 0;
	size_t nTotal = 0;
	char *pBase = nullptr;
	uint8_t *pData = nullptr;

	static void Protect( char *p, size_t n )
	{
#if defined( _WIN32 )
		DWORD nOld = 0;
		VirtualProtect( p, n, PAGE_NOACCESS, &nOld );
#else
		mprotect( p, n, PROT_NONE );
#endif
	}

public:
	CGuardedBytes( size_t nBytes, bool bFlushToEnd )
	{
#if defined( _WIN32 )
		SYSTEM_INFO si;
		GetSystemInfo( &si );
		nPage = si.dwPageSize;
#else
		nPage = static_cast<size_t>( sysconf( _SC_PAGESIZE ) );
#endif
		const size_t nDataPages = ( nBytes + nPage - 1 ) / nPage + ( nBytes == 0 ? 1 : 0 );
		nTotal = ( nDataPages + 2 ) * nPage;
#if defined( _WIN32 )
		pBase = static_cast<char *>( VirtualAlloc( nullptr, nTotal, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE ) );
#else
		void *p = mmap( nullptr, nTotal, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0 );
		pBase = p == MAP_FAILED ? nullptr : static_cast<char *>( p );
#endif
		if ( pBase )
		{
			Protect( pBase, nPage );
			Protect( pBase + ( nDataPages + 1 ) * nPage, nPage );
			char *pFirst = pBase + nPage;
			pData = reinterpret_cast<uint8_t *>( bFlushToEnd ? pFirst + nDataPages * nPage - nBytes : pFirst );
		}
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

float BitsToFloat( uint32_t n )
{
	float f;
	std::memcpy( &f, &n, sizeof( f ) );
	return f;
}

// A fused kernel under test.
typedef void ( *TFused )( const float *, int, float, int, const uint8_t *, uint8_t *, const uint8_t *, uint8_t * );

struct SFused
{
	const char *pszName;
	TFused pFunc;
	bool bNeedsAVX2;
};

std::vector<SFused> FusedKernels()
{
	std::vector<SFused> v;
	v.push_back( { "Scalar", &NWarFog::SampleWarFogScalar, false } );
	v.push_back( { "SSE2", &NWarFog::SampleWarFogSSE2, false } );
	if ( NGScene::IsAVX2Present() )
	{
		v.push_back( { "AVX2", &NWarFog::SampleWarFogAVX2, true } );
		v.push_back( { "AVX2Insert", &NWarFog::SampleWarFogAVX2T<NWarFog::EAVX2Load::Insert>, true } );
		v.push_back( { "AVX2BlockCheck", &NWarFog::SampleWarFogAVX2T<NWarFog::EAVX2Load::GatherBlockCheck>, true } );
		v.push_back( { "AVX2Uniform", &NWarFog::SampleWarFogAVX2Uniform, true } );
	}
	return v;
}

// The shipped kernel sets the dispatcher can hand out on this CPU.
std::vector<const NGScene::SLightingKernels *> ShippedSets()
{
	std::vector<const NGScene::SLightingKernels *> v;
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

// Random bytes, all 255, or a 0/255 checkerboard: the last two put the weights at
// their extremes (a full 255 * 0x4000 sum, and maximal differences).
void FillGrid( uint8_t *pGrid, int nSize, int nKind )
{
	for ( int y = 0; y < nSize; ++y )
	{
		for ( int x = 0; x < nSize; ++x )
		{
			uint8_t v;
			if ( nKind == 0 )
			{
				v = static_cast<uint8_t>( rng() );
			}
			else if ( nKind == 1 )
			{
				v = 255;
			}
			else if ( nKind == 3 )
			{
				// What the game's grid looks like: seen (255) and unseen (a low value)
				// areas, with the odd differing byte, so the uniform shortcut both hits
				// and narrowly misses.
				v = ( x < nSize / 3 && y < nSize / 2 ) ? 40 : 255;
				if ( rng() % 64 == 0 )
				{
					v = static_cast<uint8_t>( v - 1 );
				}
			}
			else
			{
				v = ( ( x + y ) & 1 ) ? 255 : 0;
			}
			pGrid[y * nSize + x] = v;
		}
	}
}

} // namespace

// Integer stage: all 2^14 x fractions times all 2^14 y fractions. Each row of 16384
// coordinates gets random integer parts, including negative and huge ones, so the
// "& ( nFogSize - 2 )" wrap is exercised everywhere.
TEST( WarFogSample, IntegerStageAllFractions )
{
	const int nSize = 9;
	std::vector<uint8_t> grid( nSize * nSize );
	std::vector<int32_t> coords( 2 * 16384 );
	std::vector<uint8_t> expect( 16384 ), got( 16384 );
	uint64_t nCompared = 0, nMismatches = 0;
	uint64_t nState = 0x9E3779B97F4A7C15ull;
	for ( int nKind = 0; nKind < 3; ++nKind )
	{
		FillGrid( &grid[0], nSize, nKind );
		for ( int fy = 0; fy < 16384; fy += N_SWEEP_STEP )
		{
			for ( int fx = 0; fx < 16384; ++fx )
			{
				// xorshift64: 2^28 mt19937 draws would cost more than the kernels do.
				nState ^= nState << 13;
				nState ^= nState >> 7;
				nState ^= nState << 17;
				const uint32_t nHighX = static_cast<uint32_t>( nState ) << 14;
				const uint32_t nHighY = static_cast<uint32_t>( nState >> 32 ) << 14;
				coords[2 * fx] = static_cast<int32_t>( nHighX | static_cast<uint32_t>( fx ) );
				coords[2 * fx + 1] = static_cast<int32_t>( nHighY | static_cast<uint32_t>( fy ) );
			}
			// The reference transcription first, then everything against it.
			NGScene::refLightingKernels.pSampleWarFogInt( &coords[0], &grid[0], nSize, &expect[0], 16384 );
			auto check = [&]( const char *pszName )
			{
				for ( int k = 0; k < 16384; ++k )
				{
					if ( got[k] != expect[k] )
					{
						if ( ++nMismatches < 10 )
						{
							ADD_FAILURE() << pszName << " x " << coords[2 * k] << " y " << coords[2 * k + 1] << ": " << int( got[k] )
														<< " expected " << int( expect[k] );
						}
					}
				}
				nCompared += 16384;
			};
			for ( const NGScene::SLightingKernels *pSet : ShippedSets() )
			{
				pSet->pSampleWarFogInt( &coords[0], &grid[0], nSize, &got[0], 16384 );
				check( pSet->pszName );
			}
			for ( int k = 0; k < 16384; ++k )
			{
				got[k] = NWarFog::NDetail::SampleInt( coords[2 * k], coords[2 * k + 1], &grid[0], nSize );
			}
			check( "SampleInt" );
			NWarFog::SampleWarFogIntSSE2( &coords[0], &grid[0], nSize, &got[0], 16384 );
			check( "IntSSE2" );
			if ( NGScene::IsAVX2Present() )
			{
				NWarFog::SampleWarFogIntAVX2( &coords[0], &grid[0], nSize, &got[0], 16384 );
				check( "IntAVX2" );
			}
		}
	}
	std::printf( "integer stage: %llu samples compared, %llu mismatches\n",
	             static_cast<unsigned long long>( nCompared ),
	             static_cast<unsigned long long>( nMismatches ) );
	EXPECT_EQ( nMismatches, 0u );
}

// Coordinate stage: every float bit pattern as x, and the same pattern rotated by 16
// bits as y, at the game's scale and at three others (a power of two, which makes
// the product exact, a large one that pushes ordinary positions out of int range,
// and a tiny one).
TEST( WarFogSample, CoordinateStageAllFloats )
{
	const float scales[] = { F_GAME_SCALE, 0.25f, 3.0e5f, 1.0e-30f };
	const int N_CHUNK = 1 << 16;
	std::vector<CVec3> pos( N_CHUNK );
	std::vector<int32_t> expect( 2 * N_CHUNK ), got( 2 * N_CHUNK ), ref( 2 * N_CHUNK );
	uint64_t nCompared = 0, nMismatches = 0, nRefDiffers = 0, nRefDiffersInRange = 0;
#if defined( _DEBUG )
	const uint64_t nChunkStep = 997;
#else
	const uint64_t nChunkStep = 1;
#endif
	for ( float fScale : scales )
	{
		// The game's scale sweeps all 2^32 patterns in Release; the other three take
		// every 64th chunk, which keeps the test near half a minute.
		const uint64_t nStep = fScale == F_GAME_SCALE ? nChunkStep : nChunkStep * 64;
		const float fFixed = fScale * 0x4000;
		for ( uint64_t nChunk = 0; nChunk < ( uint64_t( 1 ) << 32 ) / N_CHUNK; nChunk += nStep )
		{
			for ( int k = 0; k < N_CHUNK; ++k )
			{
				const uint32_t n = static_cast<uint32_t>( nChunk * N_CHUNK + k );
				pos[k].x = BitsToFloat( n );
				pos[k].y = BitsToFloat( ( n << 16 ) | ( n >> 16 ) );
				pos[k].z = BitsToFloat( n ^ 0x5a5a5a5a );
			}
			const float *pPos = &pos[0].x;
			NGScene::refLightingKernels.pSampleWarFogCoords( &pos[0], N_CHUNK, fScale, &ref[0] );
			bool bFirst = true;
			for ( const NGScene::SLightingKernels *pSet : ShippedSets() )
			{
				pSet->pSampleWarFogCoords( &pos[0], N_CHUNK, fScale, bFirst ? &expect[0] : &got[0] );
				if ( !bFirst )
				{
					// The two shipped SIMD sets against each other.
					nMismatches += std::memcmp( &expect[0], &got[0], expect.size() * sizeof( int32_t ) ) != 0;
				}
				bFirst = false;
			}
			auto check = [&]( const char *pszName )
			{
				for ( int k = 0; k < 2 * N_CHUNK; ++k )
				{
					if ( got[k] != expect[k] )
					{
						if ( ++nMismatches < 10 )
						{
							ADD_FAILURE() << pszName << " coordinate " << k << " of chunk " << nChunk << ": " << got[k]
														<< " expected " << expect[k];
						}
					}
				}
				nCompared += 2 * N_CHUNK;
			};
			NWarFog::SampleWarFogCoordsSSE2( pPos, N_CHUNK, fScale, &got[0] );
			check( "CoordsSSE2" );
			if ( NGScene::IsAVX2Present() )
			{
				NWarFog::SampleWarFogCoordsAVX2( pPos, N_CHUNK, fScale, &got[0] );
				check( "CoordsAVX2" );
			}
			// The scalar reference may differ only where the product does not fit an
			// int (lrintf is undefined there) or is NaN.
			for ( int k = 0; k < 2 * N_CHUNK; ++k )
			{
				if ( ref[k] != expect[k] )
				{
					++nRefDiffers;
					const float fIn = ( k & 1 ) ? pos[k / 2].y : pos[k / 2].x;
					const float fProduct = fIn * fFixed;
					if ( fProduct >= -2147483648.0f && fProduct < 2147483648.0f )
					{
						++nRefDiffersInRange;
					}
				}
			}
		}
	}
	std::printf( "coordinate stage: %llu coordinates compared, %llu mismatches; the reference set differs "
	             "from SSE2/AVX2 at %llu, of them %llu with the product in int range\n",
	             static_cast<unsigned long long>( nCompared ),
	             static_cast<unsigned long long>( nMismatches ),
	             static_cast<unsigned long long>( nRefDiffers ),
	             static_cast<unsigned long long>( nRefDiffersInRange ) );
	EXPECT_EQ( nMismatches, 0u );
	EXPECT_EQ( nRefDiffersInRange, 0u );
}

namespace
{

// Hostile positions: exact cell borders, the floats beside them, products that land
// on a half (rounding ties), negative, beyond the grid, beyond int range, NaN, inf.
float HostileCoordinate( int nGridSize )
{
	const float fCell = 2.75f;
	const int nCell = static_cast<int>( rng() % ( 3 * nGridSize ) ) - nGridSize;
	switch ( rng() % 9 )
	{
		case 0:
			return nCell * fCell;
		case 1:
			return std::nextafter( nCell * fCell, -1e30f );
		case 2:
			return std::nextafter( nCell * fCell, 1e30f );
		case 3:
		{
			// x * fixed scale == n + 0.5 as nearly as a float allows.
			const float fFixed = F_GAME_SCALE * 0x4000;
			const float fTie = ( static_cast<float>( static_cast<int32_t>( rng() % 4000000 ) - 2000000 ) + 0.5f ) / fFixed;
			const int nUlps = static_cast<int>( rng() % 5 ) - 2;
			float f = fTie;
			for ( int i = 0; i < nUlps; ++i )
			{
				f = std::nextafter( f, 1e30f );
			}
			for ( int i = 0; i > nUlps; --i )
			{
				f = std::nextafter( f, -1e30f );
			}
			return f;
		}
		case 4:
			return std::uniform_real_distribution<float>( -1e6f, 1e6f )( rng );
		case 5:
			return std::uniform_real_distribution<float>( -1e10f, 1e10f )( rng );
		case 6:
		{
			const float special[] = { std::numeric_limits<float>::quiet_NaN(),
			                          std::numeric_limits<float>::infinity(),
			                          -std::numeric_limits<float>::infinity(),
			                          std::numeric_limits<float>::denorm_min(),
			                          -0.0f,
			                          0.0f,
			                          std::numeric_limits<float>::max(),
			                          -std::numeric_limits<float>::max() };
			return special[rng() % 8];
		}
		default:
			return std::uniform_real_distribution<float>( 0.0f, ( nGridSize - 1 ) * fCell )( rng );
	}
}

// A realistic part: positions within a few world units of a point on the map.
void RealisticPart( float *pPos, int nCount, int nGridSize )
{
	const float fMap = ( nGridSize - 1 ) * 2.75f;
	const float fCX = std::uniform_real_distribution<float>( 0.0f, fMap )( rng );
	const float fCY = std::uniform_real_distribution<float>( 0.0f, fMap )( rng );
	const float fExtent = std::uniform_real_distribution<float>( 0.2f, 8.0f )( rng );
	std::uniform_real_distribution<float> offset( -0.5f * fExtent, 0.5f * fExtent );
	for ( int k = 0; k < nCount; ++k )
	{
		pPos[3 * k] = fCX + offset( rng );
		pPos[3 * k + 1] = fCY + offset( rng );
		pPos[3 * k + 2] = offset( rng );
	}
}

} // namespace

TEST( WarFogSample, FusedKernelsInsideGuardPages )
{
	// 129, 257, 513: the grids real maps give (16 cells per patch, rounded up to a
	// power of two, plus one). 2, 3, 5: the smallest. 6, 100: not a power of two plus
	// one, which the engine asserts against but the shipped kernels still accept.
	const int sizes[] = { 2, 3, 5, 9, 33, 129, 257, 513, 6, 100 };
	const std::vector<SFused> fused = FusedKernels();
	const std::vector<const NGScene::SLightingKernels *> shipped = ShippedSets();
	ASSERT_FALSE( shipped.empty() );
	uint64_t nCompared = 0, nMismatches = 0;
	std::vector<int32_t> coords;
	std::vector<uint8_t> expectA, expectB, gotShipped;
	for ( int nSize : sizes )
	{
		for ( int nRound = 0; nRound < 2; ++nRound )
		{
			// Round 0: the grids end at a guard page; round 1: they start after one.
			const bool bFlushToEnd = nRound == 0;
			CGuardedBytes gridA( size_t( nSize ) * nSize, bFlushToEnd ), gridB( size_t( nSize ) * nSize, bFlushToEnd );
			ASSERT_TRUE( gridA.IsValid() && gridB.IsValid() );
			FillGrid( gridA.data(), nSize, nRound == 0 ? 0 : 3 );
			FillGrid( gridB.data(), nSize, nRound == 0 ? static_cast<int>( rng() % 3 ) : 3 );
			std::vector<int> counts;
			for ( int n = 0; n <= 40; ++n )
			{
				counts.push_back( n );
			}
			for ( int i = 0; i < 40; ++i )
			{
				counts.push_back( static_cast<int>( rng() % 3000 ) );
			}
			for ( int nCount : counts )
			{
				for ( int nData = 0; nData < 2; ++nData )
				{
					CGuardedBytes posBytes( sizeof( float ) * 3 * nCount, true );
					ASSERT_TRUE( posBytes.IsValid() );
					float *pPos = reinterpret_cast<float *>( posBytes.data() );
					if ( nData == 0 )
					{
						RealisticPart( pPos, nCount, nSize );
					}
					else
					{
						for ( int k = 0; k < 3 * nCount; ++k )
						{
							pPos[k] = HostileCoordinate( nSize );
						}
					}
					const float fScale =
							( rng() % 4 ) ? F_GAME_SCALE : std::uniform_real_distribution<float>( 0.01f, 2.0f )( rng );
					for ( const NGScene::SLightingKernels *pSet : shipped )
					{
						// The shipped path, exactly as GLightPerVertex.cpp's SampleWarFog runs it.
						coords.assign( 2 * size_t( nCount ) + 2, 0 );
						expectA.assign( nCount + 1, 0 );
						expectB.assign( nCount + 1, 0 );
						if ( nCount > 0 )
						{
							pSet->pSampleWarFogCoords( reinterpret_cast<const CVec3 *>( pPos ), nCount, fScale, &coords[0] );
							pSet->pSampleWarFogInt( &coords[0], gridA.data(), nSize, &expectA[0], nCount );
							pSet->pSampleWarFogInt( &coords[0], gridB.data(), nSize, &expectB[0], nCount );
						}
						for ( const SFused &f : fused )
						{
							for ( int nGrids = 1; nGrids <= 2; ++nGrids )
							{
								CGuardedBytes resA( nCount, true ), resB( nCount, true );
								ASSERT_TRUE( resA.IsValid() && resB.IsValid() );
								f.pFunc( pPos,
								         nCount,
								         fScale,
								         nSize,
								         gridA.data(),
								         resA.data(),
								         nGrids == 2 ? gridB.data() : nullptr,
								         nGrids == 2 ? resB.data() : nullptr );
								for ( int k = 0; k < nCount; ++k )
								{
									bool bBad = resA.data()[k] != expectA[k];
									if ( nGrids == 2 )
									{
										bBad |= resB.data()[k] != expectB[k];
									}
									if ( bBad && ++nMismatches < 10 )
									{
										ADD_FAILURE() << f.pszName << " vs " << pSet->pszName << ", grid " << nSize << ", count " << nCount
																	<< ", position " << k << " (" << pPos[3 * k] << ", " << pPos[3 * k + 1] << ")";
									}
								}
								nCompared += uint64_t( nCount ) * nGrids;
							}
						}
					}
				}
			}
		}
	}
	std::printf( "fused kernels: %llu samples compared, %llu mismatches\n",
	             static_cast<unsigned long long>( nCompared ),
	             static_cast<unsigned long long>( nMismatches ) );
	EXPECT_EQ( nMismatches, 0u );
}

// The uniform fog shortcut: whenever FindUniformFog says a part's samples are all
// one value, the shipped path must produce exactly that value at every position, on
// grids where it often holds and often narrowly fails (a differing byte just beside
// the block). Both outcomes have to occur, or the test proves nothing.
TEST( WarFogSample, UniformShortcutIsExact )
{
	const int sizes[] = { 2, 3, 5, 33, 129, 257, 6, 100 };
	const std::vector<const NGScene::SLightingKernels *> shipped = ShippedSets();
	ASSERT_FALSE( shipped.empty() );
	uint64_t nHits = 0, nMisses = 0, nCompared = 0, nMismatches = 0;
	std::vector<float> pos;
	std::vector<int32_t> coords;
	std::vector<uint8_t> resA, resB;
	for ( int nSize : sizes )
	{
		std::vector<uint8_t> gridA( size_t( nSize ) * nSize ), gridB( size_t( nSize ) * nSize );
		for ( int nRound = 0; nRound < 2000; ++nRound )
		{
			FillGrid( &gridA[0], nSize, nRound % 7 == 0 ? 1 : 3 );
			FillGrid( &gridB[0], nSize, nRound % 5 == 0 ? 1 : 3 );
			const int nCount = 1 + static_cast<int>( rng() % 300 );
			pos.resize( 3 * size_t( nCount ) );
			// Mostly small parts, some straddling the grid edge; one in eight with a
			// hostile position mixed in.
			const float fMap = ( nSize - 1 ) * 2.75f;
			const float fExtent = std::uniform_real_distribution<float>( 0.05f, 4.0f )( rng );
			const float fCX = std::uniform_real_distribution<float>( -2.0f, fMap + 2.0f )( rng );
			const float fCY = std::uniform_real_distribution<float>( -2.0f, fMap + 2.0f )( rng );
			std::uniform_real_distribution<float> offset( -0.5f * fExtent, 0.5f * fExtent );
			for ( int k = 0; k < nCount; ++k )
			{
				pos[3 * k] = fCX + offset( rng );
				pos[3 * k + 1] = fCY + offset( rng );
				pos[3 * k + 2] = 0;
			}
			if ( nRound % 8 == 0 )
			{
				pos[3 * ( rng() % nCount ) + ( rng() % 2 )] = HostileCoordinate( nSize );
			}
			// A negative scale reverses which extreme gives the smaller cell.
			const float fScale = ( nRound % 3 ) ? F_GAME_SCALE : -F_GAME_SCALE;
			const bool bTwo = nRound % 2 == 0;
			uint8_t nValueA = 0, nValueB = 0;
			const bool bHit = NWarFog::FindUniformFog(
					&pos[0], nCount, fScale, nSize, &gridA[0], nValueA, bTwo ? &gridB[0] : nullptr, nValueB );
			nHits += bHit;
			nMisses += !bHit;
			if ( !bHit )
			{
				continue;
			}
			for ( const NGScene::SLightingKernels *pSet : shipped )
			{
				coords.assign( 2 * size_t( nCount ), 0 );
				resA.assign( nCount, 0 );
				resB.assign( nCount, 0 );
				pSet->pSampleWarFogCoords( reinterpret_cast<const CVec3 *>( &pos[0] ), nCount, fScale, &coords[0] );
				pSet->pSampleWarFogInt( &coords[0], &gridA[0], nSize, &resA[0], nCount );
				pSet->pSampleWarFogInt( &coords[0], &gridB[0], nSize, &resB[0], nCount );
				for ( int k = 0; k < nCount; ++k )
				{
					bool bBad = resA[k] != nValueA;
					if ( bTwo )
					{
						bBad |= resB[k] != nValueB;
					}
					if ( bBad && ++nMismatches < 10 )
					{
						ADD_FAILURE() << pSet->pszName << ", grid " << nSize << ", position " << k << ": shortcut said "
													<< int( nValueA ) << "/" << int( nValueB ) << ", sampled " << int( resA[k] ) << "/"
													<< int( resB[k] );
					}
				}
				nCompared += uint64_t( nCount ) * ( bTwo ? 2 : 1 );
			}
		}
	}
	std::printf( "uniform shortcut: %llu hits, %llu misses, %llu samples compared, %llu mismatches\n",
	             static_cast<unsigned long long>( nHits ),
	             static_cast<unsigned long long>( nMisses ),
	             static_cast<unsigned long long>( nCompared ),
	             static_cast<unsigned long long>( nMismatches ) );
	EXPECT_GT( nHits, 100u );
	EXPECT_GT( nMisses, 100u );
	EXPECT_EQ( nMismatches, 0u );
}

// FindUniformFog reads a block row with one 8-byte load when that stays inside the
// grid. Every block position of a uniform grid that ends at a guard page, the
// bottom-right corner included, must be found uniform without reading past it.
TEST( WarFogSample, UniformShortcutAtGridEnd )
{
	const int sizes[] = { 2, 3, 5, 9, 17, 33 };
	int nHits = 0, nTried = 0;
	for ( int nSize : sizes )
	{
		CGuardedBytes grid( size_t( nSize ) * nSize, true );
		ASSERT_TRUE( grid.IsValid() );
		FillGrid( grid.data(), nSize, 1 );
		const int nCells = nSize - 1;
		for ( int nCY = 0; nCY < nCells; ++nCY )
		{
			for ( int nCX = 0; nCX < nCells; ++nCX )
			{
				for ( int nSpan = 0; nSpan < 4 && nCX + nSpan < nCells; ++nSpan )
				{
					// Two positions: inside cell (nCX, nCY) and inside cell (nCX + nSpan, nCY).
					const float pos[6] = { ( nCX + 0.5f ) * 2.75f,
					                       ( nCY + 0.5f ) * 2.75f,
					                       0,
					                       ( nCX + nSpan + 0.5f ) * 2.75f,
					                       ( nCY + 0.5f ) * 2.75f,
					                       0 };
					uint8_t nValueA = 0, nValueB = 0;
					const bool bHit =
							NWarFog::FindUniformFog( pos, 2, F_GAME_SCALE, nSize, grid.data(), nValueA, grid.data(), nValueB );
					++nTried;
					nHits += bHit;
					EXPECT_TRUE( bHit && nValueA == 255 && nValueB == 255 )
							<< "grid " << nSize << ", cell " << nCX << ", " << nCY;
				}
			}
		}
	}
	std::printf( "uniform shortcut at the grid end: %d of %d blocks found uniform\n", nHits, nTried );
}

// The guard pages must actually catch a stray access, or the test above proves
// nothing about bounds.
TEST( WarFogSampleDeathTest, GuardPageCatchesOverRead )
{
	EXPECT_DEATH(
			{
				CGuardedBytes a( 5, true );
				volatile uint8_t v = a.data()[5];
				(void)v;
			},
			"" );
	EXPECT_DEATH(
			{
				CGuardedBytes a( 5, false );
				volatile uint8_t v = *( a.data() - 1 );
				(void)v;
			},
			"" );
}
