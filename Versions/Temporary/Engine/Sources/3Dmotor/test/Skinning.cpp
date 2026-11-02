// Holds every position skinning kernel (GSkinning.h, test/SkinningVariants.h) and the
// frozen copy of the engine's SSESkinning (test/SkinningReference.h) to a double
// precision oracle, sum_i w_i * ( M_i * ( x, y, z, 1 ) ).
//
// The kernels feed rendering, culling bounds and shadows only, never the lockstep
// simulation, so they are not required to match SSESkinning bit for bit; each output
// component must be within a few float roundings of the oracle, measured against
// the size of the terms that were summed (a translation of 1000 and a position near
// the origin make an absolute error of 1e-4 perfectly normal). The largest error
// seen, in units of that bound and absolute, is printed for every kernel.
//
// Every input array ends exactly at an inaccessible page, so a kernel that reads or
// writes even one float past the end of positions, weights, results or bone columns
// crashes the test instead of passing by luck. Zero weight slots carry random bone
// indices (up to 255) and every matrix past the part's bone count is NaN, so a
// kernel that follows the bone index of a zero weight turns its result into NaN.

#include "3Dmotor/stdafx.h"

#include "3Dmotor/GLightPerVertexKernels.h"
// IsAVX2Present() lives beside the lighting kernels, which need the initialised
// nCubicRoot table; the 3Dmotor DLL does not export it on Windows.
#include "3Dmotor/GSSEtransform.cpp"
#include "3Dmotor/test/SkinningReference.h"
#include "3Dmotor/test/SkinningVariants.h"

#include <algorithm>
#include <cmath>
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

namespace
{

using NSkinningVariants::SBoneColumns;
using NSkinning::SSkinWeights;
using NSkinningVariants::SRigidCall;
using NSkinningVariants::SSkinCall;
using NSkinningVariants::SSkinRun;

// Up to 255 is what a byte bone index can name; the matrix array always has room
// for all of them so a stray index lands on a NaN matrix rather than outside memory.
const int N_MATRIX_SLOTS = 256;

// nCount elements of T whose last one ends where an inaccessible page begins.
template <class T>
class CGuardedArray
{
	size_t nPage = 0;
	size_t nTotal = 0;
	char *pBase = nullptr;
	T *pData = nullptr;

public:
	explicit CGuardedArray( size_t nCount )
	{
#if defined( _WIN32 )
		SYSTEM_INFO si;
		GetSystemInfo( &si );
		nPage = si.dwPageSize;
#else
		nPage = static_cast<size_t>( sysconf( _SC_PAGESIZE ) );
#endif
		const size_t nBytes = nCount * sizeof( T );
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
		pData = reinterpret_cast<T *>( pBase + nDataPages * nPage - nBytes );
	}

	~CGuardedArray()
	{
#if defined( _WIN32 )
		VirtualFree( pBase, 0, MEM_RELEASE );
#else
		munmap( pBase, nTotal );
#endif
	}

	CGuardedArray( const CGuardedArray & ) = delete;
	CGuardedArray &operator=( const CGuardedArray & ) = delete;

	T *data()
	{
		return pData;
	}

	T &operator[]( size_t i )
	{
		return pData[i];
	}

	bool IsValid() const
	{
		return pBase != nullptr;
	}
};

// A random affine bone: rotation from a random unit quaternion, a scale of 0.5..2
// per axis, translation up to 1000 units. The bottom row is junk on purpose; the
// engine never uses it for positions and neither may a kernel.
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

// One part's input. Influence counts are drawn per vertex; which slots carry the
// nonzero weights is random too (zero weights in the middle), and the bone indices of
// zero slots are random bytes. bNormalised makes the weights sum to 1; otherwise they
// are anything in 0..2. bAllowEmpty puts in the odd vertex with no weight at all.
struct SPart
{
	int nCount = 0;
	int nBones = 0;
	std::vector<float> src;
	std::vector<SSkinWeights> weights;
	std::vector<float> matrices; // N_MATRIX_SLOTS * 16, NaN past nBones
};

void MakePart(
		std::mt19937 &rng, SPart &part, int nCount, int nBones, int nMaxInfluences, bool bNormalised, bool bAllowEmpty )
{
	part.nCount = nCount;
	part.nBones = nBones;
	part.src.resize( 3 * nCount );
	std::uniform_real_distribution<float> pos( -50.0f, 50.0f );
	for ( float &v : part.src )
	{
		v = pos( rng );
	}
	part.matrices.assign( N_MATRIX_SLOTS * 16, std::numeric_limits<float>::quiet_NaN() );
	for ( int b = 0; b < nBones; ++b )
	{
		RandomMatrix( rng, &part.matrices[16 * b] );
	}
	std::uniform_int_distribution<int> infl( 1, nMaxInfluences );
	std::uniform_int_distribution<int> bone( 0, nBones - 1 );
	std::uniform_int_distribution<int> anyByte( 0, 255 );
	std::uniform_real_distribution<float> raw( 0.05f, 1.0f );
	std::uniform_real_distribution<float> loose( 0.0f, 2.0f );
	part.weights.assign( nCount, SSkinWeights{} );
	for ( int k = 0; k < nCount; ++k )
	{
		SSkinWeights &w = part.weights[k];
		int slots[4] = { 0, 1, 2, 3 };
		std::shuffle( slots, slots + 4, rng );
		const int nInfl = ( bAllowEmpty && anyByte( rng ) == 0 ) ? 0 : infl( rng );
		// Half the vertices keep the engine's usual packing (slots 0..n-1), the rest
		// put the influences in random slots.
		if ( anyByte( rng ) & 1 )
		{
			for ( int i = 0; i < 4; ++i )
			{
				slots[i] = i;
			}
		}
		for ( int i = 0; i < 4; ++i )
		{
			w.fWeights[i] = 0.0f;
			w.cBoneIndices[i] = static_cast<uint8_t>( anyByte( rng ) );
			w.nWeights[i] = 0;
		}
		float fSum = 0;
		for ( int j = 0; j < nInfl; ++j )
		{
			const float f = bNormalised ? raw( rng ) : loose( rng );
			// A weight of exactly zero would turn this into a different influence count.
			w.fWeights[slots[j]] = f == 0.0f ? 0.5f : f;
			w.cBoneIndices[slots[j]] = static_cast<uint8_t>( bone( rng ) );
			fSum += w.fWeights[slots[j]];
		}
		if ( bNormalised && nInfl > 0 )
		{
			for ( int j = 0; j < nInfl; ++j )
			{
				w.fWeights[slots[j]] /= fSum;
			}
			if ( nInfl == 1 )
			{
				w.fWeights[slots[0]] = 1.0f;
			}
		}
		for ( int i = 0; i < 4; ++i )
		{
			const float fByte = std::min( w.fWeights[i], 1.0f ) * 255.0f;
			w.nWeights[i] = static_cast<uint8_t>( std::lround( fByte ) );
		}
	}
}

// Double precision sum and the size of what was summed, per component.
void Oracle( const SPart &part, int k, double *pRes, double *pBound )
{
	const double x = part.src[3 * k], y = part.src[3 * k + 1], z = part.src[3 * k + 2];
	for ( int j = 0; j < 3; ++j )
	{
		pRes[j] = 0;
		pBound[j] = 0;
	}
	const SSkinWeights &w = part.weights[k];
	for ( int i = 0; i < 4; ++i )
	{
		if ( w.fWeights[i] == 0.0f )
		{
			continue;
		}
		const float *m = &part.matrices[16 * w.cBoneIndices[i]];
		const double f = w.fWeights[i];
		for ( int j = 0; j < 3; ++j )
		{
			const double a = m[4 * j] * x, b = m[4 * j + 1] * y, c = m[4 * j + 2] * z, t = m[4 * j + 3];
			pRes[j] += f * ( a + b + c + t );
			pBound[j] += std::fabs( f ) * ( std::fabs( a ) + std::fabs( b ) + std::fabs( c ) + std::fabs( t ) );
		}
	}
}

// Tolerance in units of FLT_EPSILON times the bound: blend or transform first, each
// output is at most four products, three adds and four weighted accumulations deep.
const double TOLERANCE_EPS = 8.0;

struct SErrorStats
{
	double fMaxRel = 0; // in FLT_EPSILON * bound
	double fMaxAbs = 0;
	long long nChecked = 0;
};

// Compares nCount results with the oracle; returns false (after a gtest failure) on
// the first one out of tolerance.
bool CheckResults( const char *pszName, const SPart &part, const float *pRes, SErrorStats &stats )
{
	const double eps = std::numeric_limits<float>::epsilon();
	for ( int k = 0; k < part.nCount; ++k )
	{
		double exact[3], bound[3];
		Oracle( part, k, exact, bound );
		for ( int j = 0; j < 3; ++j )
		{
			const double got = pRes[3 * k + j];
			const double err = std::fabs( got - exact[j] );
			const double rel = bound[j] > 0 ? err / ( eps * bound[j] ) : ( err == 0 ? 0 : 1e30 );
			if ( !( rel <= TOLERANCE_EPS ) )
			{
				ADD_FAILURE() << pszName << " vertex " << k << " of " << part.nCount << " component " << j << ": got " << got
											<< ", expected " << exact[j] << " (error " << rel << " eps of bound " << bound[j] << ")";
				return false;
			}
			stats.fMaxRel = std::max( stats.fMaxRel, rel );
			stats.fMaxAbs = std::max( stats.fMaxAbs, err );
			++stats.nChecked;
		}
	}
	return true;
}

using TSkinFunc = void ( * )( const SSkinCall & );

void RefSkin( const SSkinCall &c )
{
	NSkinningReference::SSESkinning( reinterpret_cast<const CVec3 *>( c.pSrc ),
	                                 reinterpret_cast<CVec3 *>( c.pDst ),
	                                 c.pWeights,
	                                 reinterpret_cast<const SHMatrix *>( c.pMatrices ),
	                                 c.nCount );
}

struct SSkinVariant
{
	const char *pszName;
	TSkinFunc pFunc;
	bool bNeedsAVX2;
};

const SSkinVariant skinVariants[] = {
	{ "Ref", &RefSkin, false },
	{ "Scalar", &NSkinningVariants::Scalar, false },
#if BK2_SKINNING_SSE2
	{ "ColsBlend", &NSkinningVariants::ColsBlend, false },
	{ "ColsXform", &NSkinningVariants::ColsXform, false },
	{ "ColsXformFast", &NSkinningVariants::ColsXformFast, false },
	{ "RowsBlend", &NSkinningVariants::RowsBlend, false },
	{ "RowsBlendFast", &NSkinningVariants::RowsBlendFast, false },
	{ "SoA4", &NSkinningVariants::SoA4, false },
	{ "RunsSSE", &NSkinningVariants::RunsSSE, false },
	{ "AVX2Gather", &NSkinningVariants::AVX2Gather, true },
	{ "RunsAVX2", &NSkinningVariants::RunsAVX2, true },
#endif
};

// Copies a part into guarded arrays, runs one variant, checks it.
bool RunSkinVariant( const SSkinVariant &v, const SPart &part, SErrorStats &stats )
{
	const int n = part.nCount;
	CGuardedArray<float> src( 3 * n ), dst( 3 * n );
	CGuardedArray<SSkinWeights> weights( n );
	CGuardedArray<float> matrices( N_MATRIX_SLOTS * 16 );
	CGuardedArray<SBoneColumns> scratch( N_MATRIX_SLOTS );
	if ( !src.IsValid() || !dst.IsValid() || !weights.IsValid() || !matrices.IsValid() || !scratch.IsValid() )
	{
		ADD_FAILURE() << "could not map guarded buffers";
		return false;
	}
	if ( n > 0 )
	{
		std::memcpy( src.data(), part.src.data(), sizeof( float ) * 3 * n );
		std::memcpy( weights.data(), part.weights.data(), sizeof( SSkinWeights ) * n );
		std::fill( dst.data(), dst.data() + 3 * n, -12345.0f );
	}
	std::memcpy( matrices.data(), part.matrices.data(), sizeof( float ) * 16 * N_MATRIX_SLOTS );
	// Columns past the part's bones are NaN, like the matrices, for the same reason.
	for ( int b = 0; b < N_MATRIX_SLOTS; ++b )
	{
		for ( auto &col : scratch[b].c )
		{
			for ( float &f : col )
			{
				f = std::numeric_limits<float>::quiet_NaN();
			}
		}
	}
	std::vector<SSkinRun> runs;
	NSkinningVariants::BuildRuns( weights.data(), n, runs );
	const SSkinCall call{ src.data(),
	                      dst.data(),
	                      weights.data(),
	                      n,
	                      matrices.data(),
	                      part.nBones,
	                      scratch.data(),
	                      runs.data(),
	                      static_cast<int>( runs.size() ) };
	v.pFunc( call );
	return CheckResults( v.pszName, part, dst.data(), stats );
}

void PrintStats( const char *pszName, const SErrorStats &s )
{
	std::printf( "  %-16s max error %.3f eps of bound, max abs %.3g, %lld components\n",
	             pszName,
	             s.fMaxRel,
	             s.fMaxAbs,
	             s.nChecked );
}

// Random parts of every size 0..40 and then random sizes up to 5000, so every
// remainder of the four- and eight-vertex loops is hit.
void CheckSkinAll( int nMaxInfluences, bool bNormalised, bool bSorted )
{
	std::mt19937 rng( std::random_device{}() );
	std::uniform_int_distribution<int> sizeDist( 0, 5000 );
	std::uniform_int_distribution<int> bonesDist( 1, 64 );
	SErrorStats stats[std::size( skinVariants )];
	const int nCases = 120;
	for ( int nCase = 0; nCase < nCases; ++nCase )
	{
		const int nCount = nCase <= 40 ? nCase : sizeDist( rng );
		const int nBones = nCase % 17 == 0 ? 241 : bonesDist( rng );
		SPart part;
		MakePart( rng, part, nCount, nBones, nMaxInfluences, bNormalised, !bSorted );
		if ( bSorted )
		{
			// What a load-time reorder would produce: vertices grouped by bone set, so
			// the Runs variants see long runs and their eight-vertex blocks.
			std::vector<int> order( nCount );
			for ( int k = 0; k < nCount; ++k )
			{
				order[k] = k;
			}
			std::stable_sort( order.begin(),
			                  order.end(),
			                  [&]( int a, int b )
			                  {
													const SSkinWeights &wa = part.weights[a], &wb = part.weights[b];
													const int ma = NSkinningVariants::WeightMask( wa ), mb = NSkinningVariants::WeightMask( wb );
													if ( ma != mb )
													{
														return ma < mb;
													}
													for ( int i = 0; i < 4; ++i )
													{
														const int ba = ( ma & ( 1 << i ) ) ? wa.cBoneIndices[i] : 0;
														const int bb = ( mb & ( 1 << i ) ) ? wb.cBoneIndices[i] : 0;
														if ( ba != bb )
														{
															return ba < bb;
														}
													}
													return false;
												} );
			SPart sorted = part;
			for ( int k = 0; k < nCount; ++k )
			{
				sorted.weights[k] = part.weights[order[k]];
				for ( int j = 0; j < 3; ++j )
				{
					sorted.src[3 * k + j] = part.src[3 * order[k] + j];
				}
			}
			part = sorted;
		}
		for ( size_t v = 0; v < std::size( skinVariants ); ++v )
		{
			if ( skinVariants[v].bNeedsAVX2 && !NGScene::IsAVX2Present() )
			{
				continue;
			}
			SCOPED_TRACE( skinVariants[v].pszName );
			if ( !RunSkinVariant( skinVariants[v], part, stats[v] ) )
			{
				return;
			}
		}
	}
	for ( size_t v = 0; v < std::size( skinVariants ); ++v )
	{
		PrintStats( skinVariants[v].pszName, stats[v] );
	}
}

TEST( Skinning, OneInfluenceNormalised )
{
	CheckSkinAll( 1, true, false );
}

TEST( Skinning, UpToFourInfluencesNormalised )
{
	CheckSkinAll( 4, true, false );
}

TEST( Skinning, UpToFourInfluencesUnnormalised )
{
	CheckSkinAll( 4, false, false );
}

TEST( Skinning, SortedRunsOneInfluence )
{
	CheckSkinAll( 1, true, true );
}

TEST( Skinning, SortedRunsUpToTwoInfluences )
{
	CheckSkinAll( 2, false, true );
}

// The guard page is what makes "no variant reads past an array" a checked claim, so
// check the guard itself: one float past the end must fault.
TEST( SkinningDeathTest, GuardPageCatchesOverRead )
{
	GTEST_FLAG_SET( death_test_style, "threadsafe" );
	EXPECT_DEATH(
			{
				CGuardedArray<float> a( 3 );
				volatile float f = a.data()[3];
				(void)f;
			},
			"" );
}

// The sentinel bone tests: a part where every vertex has its single weight in a
// random slot and three garbage bone indices pointing at NaN matrices.
TEST( Skinning, ZeroWeightBonesAreNeverRead )
{
	std::mt19937 rng( 4242 );
	SPart part;
	MakePart( rng, part, 1003, 3, 1, true, false );
	for ( SSkinWeights &w : part.weights )
	{
		for ( int i = 0; i < 4; ++i )
		{
			if ( w.fWeights[i] == 0.0f )
			{
				w.cBoneIndices[i] = static_cast<uint8_t>( 3 + rng() % 253 );
			}
		}
	}
	for ( const SSkinVariant &v : skinVariants )
	{
		if ( v.bNeedsAVX2 && !NGScene::IsAVX2Present() )
		{
			continue;
		}
		SErrorStats stats;
		SCOPED_TRACE( v.pszName );
		RunSkinVariant( v, part, stats );
	}
}

using TRigidFunc = void ( * )( const SRigidCall & );

void RefRigid( const SRigidCall &c )
{
	NSkinningReference::SSEBatchTransform( *reinterpret_cast<const SHMatrix *>( c.pMatrix ),
	                                       reinterpret_cast<const CVec3 *>( c.pSrc ),
	                                       reinterpret_cast<CVec3 *>( c.pDst ),
	                                       c.nCount );
}

struct SRigidVariant
{
	const char *pszName;
	TRigidFunc pFunc;
	bool bNeedsAVX2;
	bool bBounds;
};

const SRigidVariant rigidVariants[] = {
	{ "RigidRef", &RefRigid, false, false },
	{ "RigidScalar", &NSkinningVariants::RigidScalar, false, false },
#if BK2_SKINNING_SSE2
	{ "RigidSSE", &NSkinningVariants::RigidSSE, false, false },
	{ "RigidSSEThenBounds", &NSkinningVariants::RigidSSEThenBounds, false, true },
	{ "RigidSSEBounds", &NSkinningVariants::RigidSSEBounds, false, true },
	{ "RigidSoA4", &NSkinningVariants::RigidSoA4, false, false },
	{ "RigidAVX2", &NSkinningVariants::RigidAVX2, true, false },
	{ "RigidAVX2Bounds", &NSkinningVariants::RigidAVX2Bounds, true, true },
#endif
};

TEST( Skinning, RigidVariants )
{
	std::mt19937 rng( std::random_device{}() );
	std::uniform_int_distribution<int> sizeDist( 0, 5000 );
	SErrorStats stats[std::size( rigidVariants )];
	for ( int nCase = 0; nCase < 200; ++nCase )
	{
		const int nCount = nCase <= 40 ? nCase : sizeDist( rng );
		SPart part;
		// One bone, one influence of weight 1: the oracle is the plain transform.
		MakePart( rng, part, nCount, 1, 1, true, false );
		for ( SSkinWeights &w : part.weights )
		{
			w = SSkinWeights{ { 1.0f, 0, 0, 0 }, { 255, 0, 0, 0 }, { 0, 0, 0, 0 } };
		}
		for ( size_t v = 0; v < std::size( rigidVariants ); ++v )
		{
			const SRigidVariant &var = rigidVariants[v];
			if ( var.bNeedsAVX2 && !NGScene::IsAVX2Present() )
			{
				continue;
			}
			SCOPED_TRACE( var.pszName );
			CGuardedArray<float> src( 3 * nCount ), dst( 3 * nCount ), matrix( 16 );
			if ( nCount > 0 )
			{
				std::memcpy( src.data(), part.src.data(), sizeof( float ) * 3 * nCount );
			}
			std::memcpy( matrix.data(), part.matrices.data(), sizeof( float ) * 16 );
			float vMin[3] = { 777, 777, 777 }, vMax[3] = { -777, -777, -777 };
			var.pFunc( SRigidCall{ src.data(), dst.data(), nCount, matrix.data(), vMin, vMax } );
			ASSERT_TRUE( CheckResults( var.pszName, part, dst.data(), stats[v] ) ) << "size " << nCount;
			if ( var.bBounds && nCount > 0 )
			{
				// The box must be exactly that of the results the kernel wrote.
				float eMin[3], eMax[3];
				NSkinningVariants::ScalarBounds( dst.data(), nCount, eMin, eMax );
				for ( int j = 0; j < 3; ++j )
				{
					ASSERT_EQ( eMin[j], vMin[j] ) << "size " << nCount << " axis " << j;
					ASSERT_EQ( eMax[j], vMax[j] ) << "size " << nCount << " axis " << j;
				}
			}
			if ( var.bBounds && nCount == 0 )
			{
				ASSERT_EQ( 777.0f, vMin[0] ) << "an empty part must leave the box alone";
			}
		}
	}
	for ( size_t v = 0; v < std::size( rigidVariants ); ++v )
	{
		PrintStats( rigidVariants[v].pszName, stats[v] );
	}
}

} // namespace
