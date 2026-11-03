#pragma once
// Directional per-vertex lighting fused with the vertex colour multiply and the
// SGeomVecFull output assembly. These replace, for the common case, three passes of
// GLightPerVertex.cpp:
//
//   CalcDirectionalLighting   lit and shadowed colour of every vertex from its
//                             transformed packed normal (the 2005 MMX arithmetic)
//   MultiplyOnColor (twice)   both colours times the mesh's own vertex colour
//   CalcPerVertexLight's      pos from srcPos[posIndices[k]], normal, tex, texLM,
//   output loop               texU = shadow | fog old << 24, texV = lit | fog new << 24
//
// On a replay with dozens of moving tanks those three are about a fifth of the main
// thread, and every vertex of every moving part goes through all of them each frame
// (a transform change also invalidates the directional cache).
//
// EVERY KERNEL HERE IS BIT-EXACT against the shipped kernels for every input,
// including light states no engine code produces (test/LightFused.cpp holds them to
// refLightingKernels, sse2LightingKernels, avx2LightingKernels and frozen copies of
// MultiplyOnColor and the output loop). The arguments for the three places where the
// code looks different from what it replaces:
//
// 1. No repeated-normal shortcut. The original computes a vertex only when its packed
//    normal differs from the previous vertex's, and otherwise reuses the last result.
//    The colour is a pure function of the normal, so the reuse returns exactly what
//    computing again would, with one exception: the state starts as dwPrevNormal = 0
//    and colour = shadow = 0, so a leading run of vertices whose packed normal is 0
//    (all four bytes, w included) gets colour 0 without ever being computed. Here every
//    vertex is computed, branch free, and the vertices of that leading run
//    (CountZeroNormalPrefix) are forced to 0 afterwards. Everything after the first
//    nonzero normal is F( normal ), as in the original.
//
// 2. The f >= 0 / f < 0 split. The original forms sign = f >> 15, f+ = f & ~sign and
//    f- = ~( f & sign ). f & ~sign is max( f, 0 ) and f & sign is min( f, 0 ) for
//    every 16-bit f, so pmaxsw / pminsw give the same words in one instruction less.
//    The AVX2 kernels also broadcast f (bits 15..30 of the 32-bit dot product) with
//    one pshufb after a left shift by one, which selects the same sixteen bits the
//    original's psrad 15 + punpcklwd + punpckldq kept.
//
// 3. MultiplyOnColor's min( 255, ( src * dst ) >> 8 ) never clamps: both factors are
//    bytes, so src * dst <= 255 * 255 = 65025 and 65025 >> 8 = 254 < 255. The product
//    is formed as pmulhuw( dst << 8, src ) = ( dst * 256 * src ) >> 16, which is
//    ( dst * src ) >> 8 exactly because the shift by 8 then 16 drops the same bits.
//
// 16-bit additions wrap in both the original (paddw) and here, so their order does
// not matter. All arithmetic is integer: there is no floating point anywhere in the
// colour path, so the compiler's float settings cannot affect exactness.
//
// The header knows no engine types, so it can be compiled on its own (Compiler
// Explorer checks it under MSVC x86 and x64) and by GLightPerVertexFusedAVX2.cpp
// without the engine's inline functions. The caller converts:
//   SDirectionalConsts  the seven NGfx::SMMXWord of the light (z, y, x, w shorts, the
//                       same order); memcpy them from SPerVertexLightState
//   SVertexFull         NGfx::SGeomVecFull (pos, normal, tex, texLM, texU, texV)
//   pUV / nUVStride     &SUVInfo[0] and sizeof( SUVInfo ): tex at +0, texLM at +4
//   pPositions          &srcPos[0].x, three floats per position, nPositions of them
//
// The non-temporal second destination (pStream) is for writing straight into a locked
// dynamic vertex buffer, which may be write-combined GPU memory: it is written only
// with sequential 16-byte streaming stores, every byte of every 32-byte vertex, and
// never read back. Reading write-combined memory is uncached and catastrophically
// slow, which is why the kernels never touch it except to store. Streaming stores are
// weakly ordered: the caller must issue _mm_sfence() (StreamFence()) before the buffer
// is unlocked and handed to the GPU.
//
// SSE2 only in this header: the x86-64 baseline and what MSVC x86 targets by
// default. Without SSE2 at compile time the scalar versions take the SSE2 names.
// The AVX2 sets are defined in GLightPerVertexFusedAVX2.cpp, the only file compiled
// with /arch:AVX2 (-mavx2); reach them only after NGScene::IsAVX2Present(). There are
// two, differing only in how the fog bytes are fetched: avx2FusedLightKernels uses
// vpgatherdd, avx2NoGatherFusedLightKernels byte loads. The gather is about 10% faster
// on a hot Alder Lake core and no faster cold, and gathers are slow on AMD before
// Zen 3 and slowed by the Gather Data Sampling microcode on Intel Skylake through
// Tiger Lake, so the no-gather set is the safer default.

#include <cstddef>
#include <cstdint>
#include <cstring>

#if defined( __SSE2__ ) || defined( _M_X64 ) || ( defined( _M_IX86_FP ) && _M_IX86_FP >= 2 )
#define BK2_LIGHTFUSED_SSE2 1
#include <emmintrin.h>
#else
#define BK2_LIGHTFUSED_SSE2 0
#endif

// GCC at -O2 keeps small helpers that are called from several places out of line,
// which costs a call per block; same reason as BK2_NORMALS_INLINE.
#if defined( _MSC_VER )
#define BK2_LIGHTFUSED_INLINE __forceinline
#else
#define BK2_LIGHTFUSED_INLINE inline __attribute__( ( always_inline ) )
#endif

namespace NLightFused
{

// The seven 64-bit operands of the original MMX code, each four shorts in the order
// z, y, x, w (NGfx::SMMXWord's member order, and the byte order of a packed normal).
// translucentShade is ls.shadeColor for an opaque part, the translucent colour
// converted with ConvertColor otherwise, exactly as CalcDirectionalLighting forms it.
struct SDirectionalConsts
{
	int16_t shift[4];
	int16_t dirLight[4];
	int16_t ambient[4];
	int16_t lightColor[4];
	int16_t incidentShadowColor[4];
	int16_t shadeColor[4];
	int16_t translucentShade[4];
};

// NGfx::SGeomVecFull, 32 bytes. test/LightFused.cpp pins the offsets against it.
struct SVertexFull
{
	float pos[3];
	uint32_t normal;
	uint32_t tex;
	uint32_t texLM;
	uint32_t texU;
	uint32_t texV;
};

static_assert( sizeof( SVertexFull ) == 32, "one vertex is 32 bytes" );

// Everything the output assembly reads and writes. One call is one render part.
struct SVertexCall
{
	// Destination written with ordinary stores, nCount vertices: the part's gfxData,
	// or the locked vertex buffer itself. Whole 32-byte vertices, strictly in order,
	// never read. nullptr to skip (then pStream must be set).
	SVertexFull *pRes;
	// Destination written with streaming stores (movntdq) only, same order, never
	// read: the locked vertex buffer when it may be write-combined. Must be 16-byte
	// aligned. nullptr to skip. Both may be set: gfxData plus the vertex buffer.
	SVertexFull *pStream;
	// srcPos as floats, three per position; posIndices maps vertex to position.
	const float *pPositions;
	int nPositions;
	const uint16_t *pPosIndices;
	// SUVInfo array: tex at byte 0 and texLM at byte 4 of every nUVStride-byte element.
	const uint8_t *pUV;
	int nUVStride;
	// Transformed normals: copied to the output and lit.
	const uint32_t *pNormals;
	// War fog bytes, indexed by posIndices[k] & nFogMask (nFogMask is 0 or ~0u). With
	// nFogMask ~0u both arrays must hold at least nPositions bytes (SampleWarFog sizes
	// them to srcPos.size() or more); with 0, one byte each is enough.
	const uint8_t *pFogOld;
	const uint8_t *pFogNew;
	uint32_t nFogMask;
	int nCount;
};

// The kernel set. Each entry is one whole-part transform over raw pointers, no
// allocation, and any count from 0 up.
struct SFusedLightKernels
{
	const char *pszName;

	// Drop-in for SLightingKernels::pCalcDirectionalLighting: nCount colours and
	// shadow colours from nCount packed normals.
	void ( *pDirectionalLighting )(
			const uint32_t *pNormals, int nCount, const SDirectionalConsts &c, uint32_t *pColors, uint32_t *pShadow );

	// MultiplyOnColor: pDst[k] = per byte ( pDst[k] * pMul[k] ) >> 8.
	void ( *pMultiplyOnColor )( uint32_t *pDst, const uint32_t *pMul, int nCount );

	// The two above fused: directional, then times pVertexColor (nullptr: no vertex
	// colour, the multiply is skipped as MultiplyOnColor skips an empty array).
	void ( *pDirectionalMultiply )( const uint32_t *pNormals,
	                                int nCount,
	                                const SDirectionalConsts &c,
	                                const uint32_t *pVertexColor,
	                                uint32_t *pColors,
	                                uint32_t *pShadow );

	// The output loop alone, colours from arrays: a directional cache hit, or the
	// colours after point lights were added. The engine caches colours already
	// multiplied by the vertex colour, so it passes pVertexColor = nullptr; non-null
	// multiplies the given colours first (for a cache that keeps them unmultiplied).
	void ( *pAssembleVertices )( const SVertexCall &call,
	                             const uint32_t *pVertexColor,
	                             const uint32_t *pColors,
	                             const uint32_t *pShadow );

	// All three fused: light, multiply, assemble. pColorsOut / pShadowOut (both or
	// neither) also receive the colours, for the directional cache.
	void ( *pLightAndAssemble )( const SVertexCall &call,
	                             const SDirectionalConsts &c,
	                             const uint32_t *pVertexColor,
	                             uint32_t *pColorsOut,
	                             uint32_t *pShadowOut );
};

// The AVX2 set, defined in GLightPerVertexFusedAVX2.cpp. Only for a CPU where
// NGScene::IsAVX2Present() holds.
extern const SFusedLightKernels avx2FusedLightKernels;
// The AVX2 set with the fog bytes loaded one by one instead of with vpgatherdd.
extern const SFusedLightKernels avx2NoGatherFusedLightKernels;

// The AVX2 translation unit defines this before including the header: it needs the
// types but must not compile any of the inline functions below with /arch:AVX2. An
// AVX2 copy of an inline function with external linkage is a COMDAT the linker may
// pick over the baseline copy, which would then run AVX2 instructions on a CPU
// without them. So everything from here on is invisible to that file, which carries
// its own helpers in an anonymous namespace.
#if !defined( BK2_LIGHTFUSED_DECLARATIONS_ONLY )

namespace NDetail
{

	BK2_LIGHTFUSED_INLINE uint32_t Load32( const void *p )
	{
		uint32_t v;
		std::memcpy( &v, p, sizeof( v ) );
		return v;
	}

	// pmulhw: the high half of the signed 32-bit product.
	BK2_LIGHTFUSED_INLINE int16_t MulHi( int16_t a, int16_t b )
	{
		return static_cast<int16_t>( ( static_cast<int32_t>( a ) * b ) >> 16 );
	}

	BK2_LIGHTFUSED_INLINE int16_t Add16( int16_t a, int16_t b )
	{
		return static_cast<int16_t>( static_cast<uint16_t>( a ) + static_cast<uint16_t>( b ) );
	}

	BK2_LIGHTFUSED_INLINE uint32_t PackUS( int16_t v )
	{
		return static_cast<uint32_t>( v < 0 ? 0 : ( v > 255 ? 255 : v ) );
	}

} // namespace NDetail

// Number of leading vertices whose packed normal is 0. The original never computes
// them (point 1 above), so they keep the initial colour 0.
inline int CountZeroNormalPrefix( const uint32_t *pNormals, int nCount )
{
	int n = 0;
	while ( n < nCount && pNormals[n] == 0 )
	{
		++n;
	}
	return n;
}

// The original arithmetic for one normal, written out in plain integers: the
// specification the SIMD kernels implement. Colour in the low 32 bits of the result,
// shadow colour in the high 32.
inline uint64_t DirectionalScalar( uint32_t dwNormal, const SDirectionalConsts &c )
{
	using namespace NDetail;
	int16_t s[4];
	for ( int i = 0; i < 4; ++i )
	{
		// punpcklbw( n, n ) makes byte b the word b * 257; psubw wraps.
		const uint32_t b = ( dwNormal >> ( 8 * i ) ) & 0xff;
		s[i] = static_cast<int16_t>( static_cast<uint16_t>( b * 257u - static_cast<uint16_t>( c.shift[i] ) ) );
	}
	// pmaddwd, then the two dword halves added. Unsigned so the one wrapping case
	// ( -32768 * -32768 twice ) wraps as the instruction does instead of overflowing.
	const uint32_t d0 = static_cast<uint32_t>( s[0] * c.dirLight[0] ) + static_cast<uint32_t>( s[1] * c.dirLight[1] );
	const uint32_t d1 = static_cast<uint32_t>( s[2] * c.dirLight[2] ) + static_cast<uint32_t>( s[3] * c.dirLight[3] );
	const int16_t f = static_cast<int16_t>( static_cast<int32_t>( d0 + d1 ) >> 15 );
	const int16_t fPos = f > 0 ? f : 0;
	const int16_t fNeg = static_cast<int16_t>( ~( f < 0 ? f : 0 ) );
	uint32_t dwColor = 0, dwShadow = 0;
	for ( int i = 0; i < 4; ++i )
	{
		const int16_t col =
				Add16( Add16( c.ambient[i], MulHi( c.lightColor[i], fPos ) ), MulHi( c.translucentShade[i], fNeg ) );
		const int16_t sh =
				Add16( Add16( c.ambient[i], MulHi( c.incidentShadowColor[i], fPos ) ), MulHi( c.shadeColor[i], fNeg ) );
		dwColor |= PackUS( static_cast<int16_t>( col >> 4 ) ) << ( 8 * i );
		dwShadow |= PackUS( static_cast<int16_t>( sh >> 4 ) ) << ( 8 * i );
	}
	return dwColor | ( static_cast<uint64_t>( dwShadow ) << 32 );
}

// MultiplyOnColor for one pixel. ( a * b ) >> 8 of two bytes is at most 254, so the
// original's min( 255, ... ) is dropped.
inline uint32_t MultiplyScalar( uint32_t dst, uint32_t mul )
{
	uint32_t r = 0;
	for ( int i = 0; i < 32; i += 8 )
	{
		r |= ( ( ( ( dst >> i ) & 0xff ) * ( ( mul >> i ) & 0xff ) ) >> 8 ) << i;
	}
	return r;
}

// One output vertex, the original loop body.
BK2_LIGHTFUSED_INLINE void AssembleScalarOne( const SVertexCall &call, int k, uint32_t dwColor, uint32_t dwShadow )
{
	SVertexFull v;
	const int nIndex = call.pPosIndices[k];
	std::memcpy( v.pos, call.pPositions + 3 * nIndex, sizeof( v.pos ) );
	v.normal = call.pNormals[k];
	const uint8_t *pUV = call.pUV + static_cast<ptrdiff_t>( call.nUVStride ) * k;
	v.tex = NDetail::Load32( pUV );
	v.texLM = NDetail::Load32( pUV + 4 );
	const int nFog = static_cast<int>( nIndex & call.nFogMask );
	v.texU = ( dwShadow & 0xffffff ) | ( static_cast<uint32_t>( call.pFogOld[nFog] ) << 24 );
	v.texV = ( dwColor & 0xffffff ) | ( static_cast<uint32_t>( call.pFogNew[nFog] ) << 24 );
	if ( call.pRes )
	{
		std::memcpy( call.pRes + k, &v, sizeof( v ) );
	}
	if ( call.pStream )
	{
		std::memcpy( call.pStream + k, &v, sizeof( v ) );
	}
}

// Portable versions, one vertex at a time. Exact everywhere (integers only).
inline void DirectionalLightingScalar(
		const uint32_t *pNormals, int nCount, const SDirectionalConsts &c, uint32_t *pColors, uint32_t *pShadow )
{
	const int nZero = CountZeroNormalPrefix( pNormals, nCount );
	for ( int k = 0; k < nCount; ++k )
	{
		const uint64_t r = k < nZero ? 0 : DirectionalScalar( pNormals[k], c );
		pColors[k] = static_cast<uint32_t>( r );
		pShadow[k] = static_cast<uint32_t>( r >> 32 );
	}
}

inline void MultiplyOnColorScalar( uint32_t *pDst, const uint32_t *pMul, int nCount )
{
	for ( int k = 0; k < nCount; ++k )
	{
		pDst[k] = MultiplyScalar( pDst[k], pMul[k] );
	}
}

inline void DirectionalMultiplyScalar( const uint32_t *pNormals,
                                       int nCount,
                                       const SDirectionalConsts &c,
                                       const uint32_t *pVertexColor,
                                       uint32_t *pColors,
                                       uint32_t *pShadow )
{
	DirectionalLightingScalar( pNormals, nCount, c, pColors, pShadow );
	if ( pVertexColor )
	{
		MultiplyOnColorScalar( pColors, pVertexColor, nCount );
		MultiplyOnColorScalar( pShadow, pVertexColor, nCount );
	}
}

inline void AssembleVerticesScalar( const SVertexCall &call,
                                    const uint32_t *pVertexColor,
                                    const uint32_t *pColors,
                                    const uint32_t *pShadow )
{
	for ( int k = 0; k < call.nCount; ++k )
	{
		if ( pVertexColor )
		{
			AssembleScalarOne(
					call, k, MultiplyScalar( pColors[k], pVertexColor[k] ), MultiplyScalar( pShadow[k], pVertexColor[k] ) );
		}
		else
		{
			AssembleScalarOne( call, k, pColors[k], pShadow[k] );
		}
	}
}

inline void LightAndAssembleScalar( const SVertexCall &call,
                                    const SDirectionalConsts &c,
                                    const uint32_t *pVertexColor,
                                    uint32_t *pColorsOut,
                                    uint32_t *pShadowOut )
{
	const int nZero = CountZeroNormalPrefix( call.pNormals, call.nCount );
	for ( int k = 0; k < call.nCount; ++k )
	{
		const uint64_t r = k < nZero ? 0 : DirectionalScalar( call.pNormals[k], c );
		uint32_t dwColor = static_cast<uint32_t>( r ), dwShadow = static_cast<uint32_t>( r >> 32 );
		if ( pVertexColor )
		{
			dwColor = MultiplyScalar( dwColor, pVertexColor[k] );
			dwShadow = MultiplyScalar( dwShadow, pVertexColor[k] );
		}
		if ( pColorsOut )
		{
			pColorsOut[k] = dwColor;
			pShadowOut[k] = dwShadow;
		}
		AssembleScalarOne( call, k, dwColor, dwShadow );
	}
}

// Orders the streaming stores before the vertex buffer is unlocked.
inline void StreamFence()
{
#if BK2_LIGHTFUSED_SSE2
	_mm_sfence();
#endif
}

#if BK2_LIGHTFUSED_SSE2

namespace NDetailSSE2
{

	// The light's words, each 64-bit operand broadcast to both halves.
	struct SConsts
	{
		__m128i shift, dirLight, ambient, lightColor, incidentShadowColor, shadeColor, translucentShade;
	};

	BK2_LIGHTFUSED_INLINE __m128i LoadWord( const int16_t *p )
	{
		const __m128i w = _mm_loadl_epi64( reinterpret_cast<const __m128i *>( p ) );
		return _mm_unpacklo_epi64( w, w );
	}

	BK2_LIGHTFUSED_INLINE SConsts LoadConsts( const SDirectionalConsts &c )
	{
		SConsts r;
		r.shift = LoadWord( c.shift );
		r.dirLight = LoadWord( c.dirLight );
		r.ambient = LoadWord( c.ambient );
		r.lightColor = LoadWord( c.lightColor );
		r.incidentShadowColor = LoadWord( c.incidentShadowColor );
		r.shadeColor = LoadWord( c.shadeColor );
		r.translucentShade = LoadWord( c.translucentShade );
		return r;
	}

	// The original MMX sequence for two normals, one per 64-bit half, already
	// expanded to words (punpcklbw n, n). Leaves the colour and shadow words after
	// the final psraw 4, before packuswb.
	BK2_LIGHTFUSED_INLINE void Directional2( __m128i normals, const SConsts &k, __m128i *pColor, __m128i *pShadow )
	{
		__m128i t = _mm_sub_epi16( normals, k.shift );
		t = _mm_madd_epi16( t, k.dirLight );
		t = _mm_add_epi32( t, _mm_srli_epi64( t, 32 ) );
		t = _mm_srai_epi32( t, 15 );
		t = _mm_shufflelo_epi16( t, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		t = _mm_shufflehi_epi16( t, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		// max( f, 0 ) is f & ~sign, ~min( f, 0 ) is ~( f & sign ): see point 2.
		const __m128i zero = _mm_setzero_si128();
		const __m128i fPos = _mm_max_epi16( t, zero );
		const __m128i fNeg = _mm_xor_si128( _mm_min_epi16( t, zero ), _mm_cmpeq_epi16( zero, zero ) );
		__m128i c = _mm_add_epi16( k.ambient, _mm_mulhi_epi16( k.lightColor, fPos ) );
		__m128i s = _mm_add_epi16( k.ambient, _mm_mulhi_epi16( k.incidentShadowColor, fPos ) );
		c = _mm_add_epi16( c, _mm_mulhi_epi16( k.translucentShade, fNeg ) );
		s = _mm_add_epi16( s, _mm_mulhi_epi16( k.shadeColor, fNeg ) );
		*pColor = _mm_srai_epi16( c, 4 );
		*pShadow = _mm_srai_epi16( s, 4 );
	}

	// Four packed normals in, four packed colours and shadows out (vertex j in dword j).
	BK2_LIGHTFUSED_INLINE void Directional4( __m128i n, const SConsts &k, __m128i *pColor, __m128i *pShadow )
	{
		__m128i c01, s01, c23, s23;
		Directional2( _mm_unpacklo_epi8( n, n ), k, &c01, &s01 );
		Directional2( _mm_unpackhi_epi8( n, n ), k, &c23, &s23 );
		*pColor = _mm_packus_epi16( c01, c23 );
		*pShadow = _mm_packus_epi16( s01, s23 );
	}

	// Four pixels times four pixels, per byte ( a * b ) >> 8 (point 3).
	BK2_LIGHTFUSED_INLINE __m128i Multiply4( __m128i a, __m128i b )
	{
		const __m128i zero = _mm_setzero_si128();
		const __m128i lo = _mm_mulhi_epu16( _mm_unpacklo_epi8( zero, a ), _mm_unpacklo_epi8( b, zero ) );
		const __m128i hi = _mm_mulhi_epu16( _mm_unpackhi_epi8( zero, a ), _mm_unpackhi_epi8( b, zero ) );
		return _mm_packus_epi16( lo, hi );
	}

	// Lanes j with j < nZeroLeft set: the vertices still inside the zero-normal prefix.
	BK2_LIGHTFUSED_INLINE __m128i ZeroPrefixMask4( int nZeroLeft )
	{
		return _mm_cmpgt_epi32( _mm_set1_epi32( nZeroLeft ), _mm_setr_epi32( 0, 1, 2, 3 ) );
	}

	BK2_LIGHTFUSED_INLINE __m128i LoadU( const void *p )
	{
		return _mm_loadu_si128( static_cast<const __m128i *>( p ) );
	}

	BK2_LIGHTFUSED_INLINE void StoreU( void *p, __m128i v )
	{
		_mm_storeu_si128( static_cast<__m128i *>( p ), v );
	}

	BK2_LIGHTFUSED_INLINE __m128i Load32V( const void *p )
	{
		return _mm_cvtsi32_si128( static_cast<int>( NDetail::Load32( p ) ) );
	}

	// Colours of four vertices: lit, multiplied, and the zero prefix applied.
	template <bool bVertexColor>
	BK2_LIGHTFUSED_INLINE void LightBlock4( const uint32_t *pNormals,
	                                        const uint32_t *pVertexColor,
	                                        int nZeroLeft,
	                                        const SConsts &k,
	                                        __m128i *pColor,
	                                        __m128i *pShadow )
	{
		__m128i c, s;
		Directional4( LoadU( pNormals ), k, &c, &s );
		if ( bVertexColor )
		{
			const __m128i vc = LoadU( pVertexColor );
			c = Multiply4( c, vc );
			s = Multiply4( s, vc );
		}
		// Taken only for the first block or two of a part whose first normal is 0.
		if ( nZeroLeft > 0 )
		{
			const __m128i m = ZeroPrefixMask4( nZeroLeft );
			c = _mm_andnot_si128( m, c );
			s = _mm_andnot_si128( m, s );
		}
		*pColor = c;
		*pShadow = s;
	}

	// The bytes pFog[idx[j]] of four vertices, as dwords byte << 24.
	BK2_LIGHTFUSED_INLINE void Fog4( const SVertexCall &call, const int *pIdx, __m128i *pOld, __m128i *pNew )
	{
		const uint32_t fo = call.pFogOld[pIdx[0]] | ( call.pFogOld[pIdx[1]] << 8 ) | ( call.pFogOld[pIdx[2]] << 16 ) |
		                    ( static_cast<uint32_t>( call.pFogOld[pIdx[3]] ) << 24 );
		const uint32_t fn = call.pFogNew[pIdx[0]] | ( call.pFogNew[pIdx[1]] << 8 ) | ( call.pFogNew[pIdx[2]] << 16 ) |
		                    ( static_cast<uint32_t>( call.pFogNew[pIdx[3]] ) << 24 );
		const __m128i zero = _mm_setzero_si128();
		const __m128i w = _mm_unpacklo_epi8( zero,
		                                     _mm_unpacklo_epi32( _mm_cvtsi32_si128( static_cast<int>( fo ) ),
		                                                         _mm_cvtsi32_si128( static_cast<int>( fn ) ) ) );
		*pOld = _mm_unpacklo_epi16( zero, w );
		*pNew = _mm_unpackhi_epi16( zero, w );
	}

	// Where the vertices go: pRes only, pStream only, or both. A template parameter
	// rather than two tests per vertex.
	enum EDest
	{
		DEST_PLAIN,
		DEST_STREAM,
		DEST_BOTH
	};

	BK2_LIGHTFUSED_INLINE EDest GetDest( const SVertexCall &call )
	{
		return call.pStream == nullptr ? DEST_PLAIN : ( call.pRes == nullptr ? DEST_STREAM : DEST_BOTH );
	}

	// Writes vertex k: lo = pos, normal; hi = tex, texLM, texU, texV.
	template <EDest eDest>
	BK2_LIGHTFUSED_INLINE void StoreVertex( const SVertexCall &call, int k, __m128i lo, __m128i hi )
	{
		if ( eDest != DEST_STREAM )
		{
			StoreU( &call.pRes[k], lo );
			StoreU( &call.pRes[k].tex, hi );
		}
		if ( eDest != DEST_PLAIN )
		{
			_mm_stream_si128( reinterpret_cast<__m128i *>( &call.pStream[k] ), lo );
			_mm_stream_si128( reinterpret_cast<__m128i *>( &call.pStream[k].tex ), hi );
		}
	}

	// pos of position nIndex and the normal: x, y from one 8-byte load, z and the
	// normal paired, so nothing past the position is read.
	BK2_LIGHTFUSED_INLINE __m128i PosNormal( const SVertexCall &call, int nIndex, int k )
	{
		const float *p = call.pPositions + 3 * nIndex;
		const __m128i xy = _mm_loadl_epi64( reinterpret_cast<const __m128i *>( p ) );
		const __m128i zn = _mm_unpacklo_epi32( Load32V( p + 2 ), Load32V( call.pNormals + k ) );
		return _mm_unpacklo_epi64( xy, zn );
	}

	BK2_LIGHTFUSED_INLINE __m128i TexPair( const SVertexCall &call, int k )
	{
		return _mm_loadl_epi64(
				reinterpret_cast<const __m128i *>( call.pUV + static_cast<ptrdiff_t>( call.nUVStride ) * k ) );
	}

	// Four whole vertices from k, colours given. bFogIndexed false: nFogMask == 0, so
	// every vertex takes fog byte 0 and fogOld / fogNew are the precomputed vectors.
	template <bool bFogIndexed, EDest eDest>
	BK2_LIGHTFUSED_INLINE void AssembleBlock4(
			const SVertexCall &call, int k, __m128i c, __m128i s, const __m128i &fogOldConst, const __m128i &fogNewConst )
	{
		int idx[4];
		for ( int j = 0; j < 4; ++j )
		{
			idx[j] = call.pPosIndices[k + j];
		}
		__m128i fo = fogOldConst, fn = fogNewConst;
		if ( bFogIndexed )
		{
			Fog4( call, idx, &fo, &fn );
		}
		const __m128i low24 = _mm_set1_epi32( 0xffffff );
		const __m128i u = _mm_or_si128( _mm_and_si128( s, low24 ), fo );
		const __m128i v = _mm_or_si128( _mm_and_si128( c, low24 ), fn );
		// u0 v0 u1 v1 and u2 v2 u3 v3: the last eight bytes of vertices 0..3.
		const __m128i uv01 = _mm_unpacklo_epi32( u, v );
		const __m128i uv23 = _mm_unpackhi_epi32( u, v );
		// Vertex 0 and 2 take the low pair, 1 and 3 the high one (movsd keeps it).
		StoreVertex<eDest>( call, k, PosNormal( call, idx[0], k ), _mm_unpacklo_epi64( TexPair( call, k ), uv01 ) );
		StoreVertex<eDest>(
				call,
				k + 1,
				PosNormal( call, idx[1], k + 1 ),
				_mm_castpd_si128( _mm_move_sd( _mm_castsi128_pd( uv01 ), _mm_castsi128_pd( TexPair( call, k + 1 ) ) ) ) );
		StoreVertex<eDest>(
				call, k + 2, PosNormal( call, idx[2], k + 2 ), _mm_unpacklo_epi64( TexPair( call, k + 2 ), uv23 ) );
		StoreVertex<eDest>(
				call,
				k + 3,
				PosNormal( call, idx[3], k + 3 ),
				_mm_castpd_si128( _mm_move_sd( _mm_castsi128_pd( uv23 ), _mm_castsi128_pd( TexPair( call, k + 3 ) ) ) ) );
	}

	BK2_LIGHTFUSED_INLINE void FogConsts( const SVertexCall &call, __m128i *pOld, __m128i *pNew )
	{
		*pOld = _mm_set1_epi32( static_cast<int>( static_cast<uint32_t>( call.pFogOld[0] ) << 24 ) );
		*pNew = _mm_set1_epi32( static_cast<int>( static_cast<uint32_t>( call.pFogNew[0] ) << 24 ) );
	}

	// The full blocks of LightAndAssemble / AssembleVertices; returns where the tail
	// starts. bLight false: colours come from pColorsIn / pShadowIn.
	template <bool bLight, bool bVertexColor, bool bFogIndexed, EDest eDest>
	inline int Run4( const SVertexCall &callIn,
	                 const SConsts &k,
	                 int nZero,
	                 const uint32_t *pVertexColor,
	                 const uint32_t *pColorsIn,
	                 const uint32_t *pShadowIn,
	                 uint32_t *pColorsOut,
	                 uint32_t *pShadowOut )
	{
		// A local copy: the vertex stores go through may_alias vector pointers, so
		// with the caller's struct the compiler reloads every pointer after each one.
		const SVertexCall call = callIn;
		__m128i fogOld = _mm_setzero_si128(), fogNew = _mm_setzero_si128();
		if ( !bFogIndexed )
		{
			FogConsts( call, &fogOld, &fogNew );
		}
		int n = 0;
		for ( ; n + 4 <= call.nCount; n += 4 )
		{
			__m128i c, s;
			if ( bLight )
			{
				LightBlock4<bVertexColor>( call.pNormals + n, bVertexColor ? pVertexColor + n : nullptr, nZero - n, k, &c, &s );
				if ( pColorsOut )
				{
					StoreU( pColorsOut + n, c );
					StoreU( pShadowOut + n, s );
				}
			}
			else
			{
				c = LoadU( pColorsIn + n );
				s = LoadU( pShadowIn + n );
				if ( bVertexColor )
				{
					const __m128i vc = LoadU( pVertexColor + n );
					c = Multiply4( c, vc );
					s = Multiply4( s, vc );
				}
			}
			AssembleBlock4<bFogIndexed, eDest>( call, n, c, s, fogOld, fogNew );
		}
		return n;
	}

	template <bool bLight, bool bVertexColor, bool bFogIndexed>
	inline int RunDest( const SVertexCall &call,
	                    const SConsts &k,
	                    int nZero,
	                    const uint32_t *pVertexColor,
	                    const uint32_t *pColorsIn,
	                    const uint32_t *pShadowIn,
	                    uint32_t *pColorsOut,
	                    uint32_t *pShadowOut )
	{
		switch ( GetDest( call ) )
		{
			case DEST_PLAIN:
				return Run4<bLight, bVertexColor, bFogIndexed, DEST_PLAIN>(
						call, k, nZero, pVertexColor, pColorsIn, pShadowIn, pColorsOut, pShadowOut );
			case DEST_STREAM:
				return Run4<bLight, bVertexColor, bFogIndexed, DEST_STREAM>(
						call, k, nZero, pVertexColor, pColorsIn, pShadowIn, pColorsOut, pShadowOut );
			default:
				return Run4<bLight, bVertexColor, bFogIndexed, DEST_BOTH>(
						call, k, nZero, pVertexColor, pColorsIn, pShadowIn, pColorsOut, pShadowOut );
		}
	}

	template <bool bLight>
	inline int RunVertexColor( const SVertexCall &call,
	                           const SConsts &k,
	                           int nZero,
	                           const uint32_t *pVertexColor,
	                           const uint32_t *pColorsIn,
	                           const uint32_t *pShadowIn,
	                           uint32_t *pColorsOut,
	                           uint32_t *pShadowOut )
	{
		const bool bIndexed = call.nFogMask != 0;
		if ( pVertexColor )
		{
			return bIndexed ? RunDest<bLight, true, true>(
														call, k, nZero, pVertexColor, pColorsIn, pShadowIn, pColorsOut, pShadowOut )
			                : RunDest<bLight, true, false>(
														call, k, nZero, pVertexColor, pColorsIn, pShadowIn, pColorsOut, pShadowOut );
		}
		return bIndexed ? RunDest<bLight, false, true>(
													call, k, nZero, pVertexColor, pColorsIn, pShadowIn, pColorsOut, pShadowOut )
		                : RunDest<bLight, false, false>(
													call, k, nZero, pVertexColor, pColorsIn, pShadowIn, pColorsOut, pShadowOut );
	}

	template <bool bLight>
	inline void RunDispatch( const SVertexCall &call,
	                         const SDirectionalConsts *pConsts,
	                         const uint32_t *pVertexColor,
	                         const uint32_t *pColorsIn,
	                         const uint32_t *pShadowIn,
	                         uint32_t *pColorsOut,
	                         uint32_t *pShadowOut )
	{
		if ( call.nCount <= 0 )
		{
			return;
		}
		SConsts k{};
		int nZero = 0;
		if ( bLight )
		{
			k = LoadConsts( *pConsts );
			nZero = CountZeroNormalPrefix( call.pNormals, call.nCount );
		}
		const int n = RunVertexColor<bLight>( call, k, nZero, pVertexColor, pColorsIn, pShadowIn, pColorsOut, pShadowOut );
		// Fewer than four left: one at a time with the scalar arithmetic.
		for ( int t = n; t < call.nCount; ++t )
		{
			uint32_t dwColor, dwShadow;
			if ( bLight )
			{
				const uint64_t r = t < nZero ? 0 : DirectionalScalar( call.pNormals[t], *pConsts );
				dwColor = static_cast<uint32_t>( r );
				dwShadow = static_cast<uint32_t>( r >> 32 );
				if ( pVertexColor )
				{
					dwColor = MultiplyScalar( dwColor, pVertexColor[t] );
					dwShadow = MultiplyScalar( dwShadow, pVertexColor[t] );
				}
				if ( pColorsOut )
				{
					pColorsOut[t] = dwColor;
					pShadowOut[t] = dwShadow;
				}
			}
			else
			{
				dwColor = pColorsIn[t];
				dwShadow = pShadowIn[t];
				if ( pVertexColor )
				{
					dwColor = MultiplyScalar( dwColor, pVertexColor[t] );
					dwShadow = MultiplyScalar( dwShadow, pVertexColor[t] );
				}
			}
			AssembleScalarOne( call, t, dwColor, dwShadow );
		}
	}

	// Directional (and multiply) into arrays, four at a time.
	template <bool bVertexColor>
	inline void DirectionalArrays( const uint32_t *pNormals,
	                               int nCount,
	                               const SDirectionalConsts &c,
	                               const uint32_t *pVertexColor,
	                               uint32_t *pColors,
	                               uint32_t *pShadow )
	{
		if ( nCount <= 0 )
		{
			return;
		}
		const SConsts k = LoadConsts( c );
		const int nZero = CountZeroNormalPrefix( pNormals, nCount );
		int n = 0;
		for ( ; n + 4 <= nCount; n += 4 )
		{
			__m128i col, sh;
			LightBlock4<bVertexColor>( pNormals + n, bVertexColor ? pVertexColor + n : nullptr, nZero - n, k, &col, &sh );
			StoreU( pColors + n, col );
			StoreU( pShadow + n, sh );
		}
		for ( ; n < nCount; ++n )
		{
			const uint64_t r = n < nZero ? 0 : DirectionalScalar( pNormals[n], c );
			uint32_t dwColor = static_cast<uint32_t>( r ), dwShadow = static_cast<uint32_t>( r >> 32 );
			if ( bVertexColor )
			{
				dwColor = MultiplyScalar( dwColor, pVertexColor[n] );
				dwShadow = MultiplyScalar( dwShadow, pVertexColor[n] );
			}
			pColors[n] = dwColor;
			pShadow[n] = dwShadow;
		}
	}

} // namespace NDetailSSE2

inline void DirectionalLightingSSE2(
		const uint32_t *pNormals, int nCount, const SDirectionalConsts &c, uint32_t *pColors, uint32_t *pShadow )
{
	NDetailSSE2::DirectionalArrays<false>( pNormals, nCount, c, nullptr, pColors, pShadow );
}

inline void MultiplyOnColorSSE2( uint32_t *pDst, const uint32_t *pMul, int nCount )
{
	int n = 0;
	for ( ; n + 4 <= nCount; n += 4 )
	{
		NDetailSSE2::StoreU( pDst + n,
		                     NDetailSSE2::Multiply4( NDetailSSE2::LoadU( pDst + n ), NDetailSSE2::LoadU( pMul + n ) ) );
	}
	for ( ; n < nCount; ++n )
	{
		pDst[n] = MultiplyScalar( pDst[n], pMul[n] );
	}
}

inline void DirectionalMultiplySSE2( const uint32_t *pNormals,
                                     int nCount,
                                     const SDirectionalConsts &c,
                                     const uint32_t *pVertexColor,
                                     uint32_t *pColors,
                                     uint32_t *pShadow )
{
	if ( pVertexColor )
	{
		NDetailSSE2::DirectionalArrays<true>( pNormals, nCount, c, pVertexColor, pColors, pShadow );
	}
	else
	{
		NDetailSSE2::DirectionalArrays<false>( pNormals, nCount, c, nullptr, pColors, pShadow );
	}
}

inline void AssembleVerticesSSE2( const SVertexCall &call,
                                  const uint32_t *pVertexColor,
                                  const uint32_t *pColors,
                                  const uint32_t *pShadow )
{
	NDetailSSE2::RunDispatch<false>( call, nullptr, pVertexColor, pColors, pShadow, nullptr, nullptr );
}

inline void LightAndAssembleSSE2( const SVertexCall &call,
                                  const SDirectionalConsts &c,
                                  const uint32_t *pVertexColor,
                                  uint32_t *pColorsOut,
                                  uint32_t *pShadowOut )
{
	NDetailSSE2::RunDispatch<true>( call, &c, pVertexColor, nullptr, nullptr, pColorsOut, pShadowOut );
}

#else // !BK2_LIGHTFUSED_SSE2

// No SSE2 at compile time (MSVC x86 /arch:IA32): the scalar versions, which are
// exact everywhere, under the SSE2 names.
inline void DirectionalLightingSSE2(
		const uint32_t *pNormals, int nCount, const SDirectionalConsts &c, uint32_t *pColors, uint32_t *pShadow )
{
	DirectionalLightingScalar( pNormals, nCount, c, pColors, pShadow );
}

inline void MultiplyOnColorSSE2( uint32_t *pDst, const uint32_t *pMul, int nCount )
{
	MultiplyOnColorScalar( pDst, pMul, nCount );
}

inline void DirectionalMultiplySSE2( const uint32_t *pNormals,
                                     int nCount,
                                     const SDirectionalConsts &c,
                                     const uint32_t *pVertexColor,
                                     uint32_t *pColors,
                                     uint32_t *pShadow )
{
	DirectionalMultiplyScalar( pNormals, nCount, c, pVertexColor, pColors, pShadow );
}

inline void AssembleVerticesSSE2( const SVertexCall &call,
                                  const uint32_t *pVertexColor,
                                  const uint32_t *pColors,
                                  const uint32_t *pShadow )
{
	AssembleVerticesScalar( call, pVertexColor, pColors, pShadow );
}

inline void LightAndAssembleSSE2( const SVertexCall &call,
                                  const SDirectionalConsts &c,
                                  const uint32_t *pVertexColor,
                                  uint32_t *pColorsOut,
                                  uint32_t *pShadowOut )
{
	LightAndAssembleScalar( call, c, pVertexColor, pColorsOut, pShadowOut );
}

#endif // BK2_LIGHTFUSED_SSE2

// The baseline set. C++17 inline variable: one definition across the program.
inline const SFusedLightKernels sse2FusedLightKernels = { "sse2",
                                                          &DirectionalLightingSSE2,
                                                          &MultiplyOnColorSSE2,
                                                          &DirectionalMultiplySSE2,
                                                          &AssembleVerticesSSE2,
                                                          &LightAndAssembleSSE2 };

inline const SFusedLightKernels scalarFusedLightKernels = { "scalar",
                                                            &DirectionalLightingScalar,
                                                            &MultiplyOnColorScalar,
                                                            &DirectionalMultiplyScalar,
                                                            &AssembleVerticesScalar,
                                                            &LightAndAssembleScalar };

#endif // !BK2_LIGHTFUSED_DECLARATIONS_ONLY

} // namespace NLightFused
