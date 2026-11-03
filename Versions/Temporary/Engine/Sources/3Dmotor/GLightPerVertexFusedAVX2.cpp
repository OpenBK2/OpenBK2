// AVX2 set of the fused per-vertex lighting kernels (GLightPerVertexFused.h, which
// holds the exactness argument). Eight vertices per block: the 32 bytes of eight
// packed normals expand in-lane to two registers of four 64-bit MMX operands each,
// so every lane-local instruction of the original sequence runs on four vertices,
// and the packs come out in vertex order with no lane crossing.
//
// Compiled with /arch:AVX2 (-mavx2) and entered only through avx2FusedLightKernels
// after IsAVX2Present(). It sees only the types of the header
// (BK2_LIGHTFUSED_DECLARATIONS_ONLY), and every helper is in the anonymous namespace,
// so no AVX2-compiled inline function with external linkage leaves this file.

#define BK2_LIGHTFUSED_DECLARATIONS_ONLY
#include "GLightPerVertexFused.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <immintrin.h>

// GCC at -O2 left the block helpers out of line, passing the __m256i results through
// memory; every helper below the loops is forced inline.
#if defined( _MSC_VER )
#define BK2_LF_INLINE __forceinline
#else
#define BK2_LF_INLINE inline __attribute__( ( always_inline ) )
#endif

namespace NLightFused
{
namespace
{

	struct SConsts8
	{
		__m256i shift, dirLight, ambient, lightColor, incidentShadowColor, shadeColor, translucentShade;
		// pshufb control: bytes 2, 3 of each qword (the high word of the dot product
		// after the shift left by one) into all four words of that qword.
		__m256i broadcastF;
	};

	// One 64-bit MMX operand broadcast to all four quarters.
	BK2_LF_INLINE __m256i LoadWord( const int16_t *p )
	{
		long long v;
		std::memcpy( &v, p, sizeof( v ) );
		return _mm256_set1_epi64x( v );
	}

	BK2_LF_INLINE SConsts8 LoadConsts( const SDirectionalConsts &c )
	{
		SConsts8 r;
		r.shift = LoadWord( c.shift );
		r.dirLight = LoadWord( c.dirLight );
		r.ambient = LoadWord( c.ambient );
		r.lightColor = LoadWord( c.lightColor );
		r.incidentShadowColor = LoadWord( c.incidentShadowColor );
		r.shadeColor = LoadWord( c.shadeColor );
		r.translucentShade = LoadWord( c.translucentShade );
		r.broadcastF = _mm256_setr_epi8( 2,
		                                 3,
		                                 2,
		                                 3,
		                                 2,
		                                 3,
		                                 2,
		                                 3,
		                                 10,
		                                 11,
		                                 10,
		                                 11,
		                                 10,
		                                 11,
		                                 10,
		                                 11,
		                                 2,
		                                 3,
		                                 2,
		                                 3,
		                                 2,
		                                 3,
		                                 2,
		                                 3,
		                                 10,
		                                 11,
		                                 10,
		                                 11,
		                                 10,
		                                 11,
		                                 10,
		                                 11 );
		return r;
	}

	inline int CountZeroPrefix( const uint32_t *pNormals, int nCount )
	{
		int n = 0;
		while ( n < nCount && pNormals[n] == 0 )
		{
			++n;
		}
		return n;
	}

	// The original MMX sequence on four normals (one per 64-bit quarter), already
	// expanded to words. Returns colour and shadow words after psraw 4.
	//
	// f is bits 15..30 of the 32-bit dot product, which the original extracts with
	// psrad 15 and two unpacks; here a shift left by one puts those bits in the high
	// word of the dword and one pshufb copies it to all four words.
	BK2_LF_INLINE void Directional4( __m256i normals, const SConsts8 &k, __m256i *pColor, __m256i *pShadow )
	{
		__m256i t = _mm256_sub_epi16( normals, k.shift );
		t = _mm256_madd_epi16( t, k.dirLight );
		t = _mm256_add_epi32( t, _mm256_srli_epi64( t, 32 ) );
		t = _mm256_slli_epi32( t, 1 );
		t = _mm256_shuffle_epi8( t, k.broadcastF );
		const __m256i zero = _mm256_setzero_si256();
		const __m256i fPos = _mm256_max_epi16( t, zero );
		const __m256i fNeg = _mm256_xor_si256( _mm256_min_epi16( t, zero ), _mm256_cmpeq_epi16( zero, zero ) );
		__m256i c = _mm256_add_epi16( k.ambient, _mm256_mulhi_epi16( k.lightColor, fPos ) );
		__m256i s = _mm256_add_epi16( k.ambient, _mm256_mulhi_epi16( k.incidentShadowColor, fPos ) );
		c = _mm256_add_epi16( c, _mm256_mulhi_epi16( k.translucentShade, fNeg ) );
		s = _mm256_add_epi16( s, _mm256_mulhi_epi16( k.shadeColor, fNeg ) );
		*pColor = _mm256_srai_epi16( c, 4 );
		*pShadow = _mm256_srai_epi16( s, 4 );
	}

	// Eight packed normals to eight packed colours and shadows, vertex j in dword j.
	// unpacklo takes normals 0, 1 | 4, 5 and unpackhi 2, 3 | 6, 7; packus puts them
	// back as 0 1 2 3 | 4 5 6 7.
	BK2_LF_INLINE void Directional8( __m256i n, const SConsts8 &k, __m256i *pColor, __m256i *pShadow )
	{
		__m256i cA, sA, cB, sB;
		Directional4( _mm256_unpacklo_epi8( n, n ), k, &cA, &sA );
		Directional4( _mm256_unpackhi_epi8( n, n ), k, &cB, &sB );
		*pColor = _mm256_packus_epi16( cA, cB );
		*pShadow = _mm256_packus_epi16( sA, sB );
	}

	// Per byte ( a * b ) >> 8, as pmulhuw( a << 8, b ).
	BK2_LF_INLINE __m256i Multiply8( __m256i a, __m256i b )
	{
		const __m256i zero = _mm256_setzero_si256();
		const __m256i lo = _mm256_mulhi_epu16( _mm256_unpacklo_epi8( zero, a ), _mm256_unpacklo_epi8( b, zero ) );
		const __m256i hi = _mm256_mulhi_epu16( _mm256_unpackhi_epi8( zero, a ), _mm256_unpackhi_epi8( b, zero ) );
		return _mm256_packus_epi16( lo, hi );
	}

	BK2_LF_INLINE __m256i LoadU( const void *p )
	{
		return _mm256_loadu_si256( static_cast<const __m256i *>( p ) );
	}

	BK2_LF_INLINE void StoreU( void *p, __m256i v )
	{
		_mm256_storeu_si256( static_cast<__m256i *>( p ), v );
	}

	// Eight vertices' colours: lit, multiplied, zero prefix applied.
	template <bool bVertexColor>
	BK2_LF_INLINE void LightBlock8( const uint32_t *pNormals,
	                                const uint32_t *pVertexColor,
	                                int nZeroLeft,
	                                const SConsts8 &k,
	                                __m256i *pColor,
	                                __m256i *pShadow )
	{
		__m256i c, s;
		Directional8( LoadU( pNormals ), k, &c, &s );
		if ( bVertexColor )
		{
			const __m256i vc = LoadU( pVertexColor );
			c = Multiply8( c, vc );
			s = Multiply8( s, vc );
		}
		// Only for the first block of a part whose first normal is 0.
		if ( nZeroLeft > 0 )
		{
			const __m256i m =
					_mm256_cmpgt_epi32( _mm256_set1_epi32( nZeroLeft ), _mm256_setr_epi32( 0, 1, 2, 3, 4, 5, 6, 7 ) );
			c = _mm256_andnot_si256( m, c );
			s = _mm256_andnot_si256( m, s );
		}
		*pColor = c;
		*pShadow = s;
	}

	BK2_LF_INLINE uint32_t Fog4( const uint8_t *pFog, const int *pIdx )
	{
		return pFog[pIdx[0]] | ( pFog[pIdx[1]] << 8 ) | ( pFog[pIdx[2]] << 16 ) |
		       ( static_cast<uint32_t>( pFog[pIdx[3]] ) << 24 );
	}

	BK2_LF_INLINE __m128i Load32V( const void *p )
	{
		uint32_t v;
		std::memcpy( &v, p, sizeof( v ) );
		return _mm_cvtsi32_si128( static_cast<int>( v ) );
	}

	// pos of position nIndex in dwords 0..2, the normal in dword 3. A 16-byte load
	// reads the next position's x, which the blend drops; for the last position
	// there is no next one, so it is assembled from an 8- and a 4-byte load.
	BK2_LF_INLINE __m128i PosNormal( const SVertexCall &call, int nIndex, const uint32_t *pNormal )
	{
		const float *p = call.pPositions + 3 * nIndex;
		const __m128i n = _mm_castps_si128( _mm_broadcast_ss( reinterpret_cast<const float *>( pNormal ) ) );
		__m128i pos;
		if ( nIndex + 1 < call.nPositions )
		{
			pos = _mm_loadu_si128( reinterpret_cast<const __m128i *>( p ) );
		}
		else
		{
			pos = _mm_unpacklo_epi64( _mm_loadl_epi64( reinterpret_cast<const __m128i *>( p ) ), Load32V( p + 2 ) );
		}
		return _mm_blend_epi32( pos, n, 0x8 );
	}

	BK2_LF_INLINE __m128i TexPair( const SVertexCall &call, int k )
	{
		return _mm_loadl_epi64(
				reinterpret_cast<const __m128i *>( call.pUV + static_cast<ptrdiff_t>( call.nUVStride ) * k ) );
	}

	// Where the vertices go: pRes only, pStream only, or both.
	enum EDest
	{
		DEST_PLAIN,
		DEST_STREAM,
		DEST_BOTH
	};

	EDest GetDest( const SVertexCall &call )
	{
		return call.pStream == nullptr ? DEST_PLAIN : ( call.pRes == nullptr ? DEST_STREAM : DEST_BOTH );
	}

	template <EDest eDest>
	BK2_LF_INLINE void StoreVertex( const SVertexCall &call, int k, __m128i lo, __m128i hi )
	{
		if ( eDest != DEST_STREAM )
		{
			_mm_storeu_si128( reinterpret_cast<__m128i *>( &call.pRes[k] ), lo );
			_mm_storeu_si128( reinterpret_cast<__m128i *>( &call.pRes[k].tex ), hi );
		}
		if ( eDest != DEST_PLAIN )
		{
			_mm_stream_si128( reinterpret_cast<__m128i *>( &call.pStream[k] ), lo );
			_mm_stream_si128( reinterpret_cast<__m128i *>( &call.pStream[k].tex ), hi );
		}
	}

	// Eight whole vertices from k with the colours given.
	template <bool bFogIndexed, EDest eDest>
	BK2_LF_INLINE void AssembleBlock8( const SVertexCall &call,
	                                   int k,
	                                   __m256i c,
	                                   __m256i s,
	                                   const __m256i &fogOldConst,
	                                   const __m256i &fogNewConst,
	                                   bool bGather )
	{
		int idx[8];
		for ( int j = 0; j < 8; ++j )
		{
			idx[j] = call.pPosIndices[k + j];
		}
		__m256i fo = fogOldConst, fn = fogNewConst;
		// Gather: four bytes from pFog + idx for each vertex, of which the first is
		// the fog byte; shifted left by 24 the other three fall off. Those three may lie
		// past the array for the last three positions (the fog arrays hold at least
		// nPositions bytes), so a block that touches one takes the byte loads below.
		const __m256i vIdx =
				_mm256_cvtepu16_epi32( _mm_loadu_si128( reinterpret_cast<const __m128i *>( call.pPosIndices + k ) ) );
		if ( bFogIndexed && bGather &&
		     _mm256_testz_si256( _mm256_cmpgt_epi32( vIdx, _mm256_set1_epi32( call.nPositions - 4 ) ),
		                         _mm256_set1_epi32( -1 ) ) )
		{
			fo = _mm256_slli_epi32( _mm256_i32gather_epi32( reinterpret_cast<const int *>( call.pFogOld ), vIdx, 1 ), 24 );
			fn = _mm256_slli_epi32( _mm256_i32gather_epi32( reinterpret_cast<const int *>( call.pFogNew ), vIdx, 1 ), 24 );
		}
		else if ( bFogIndexed )
		{
			// Fog byte << 24 for eight vertices: four bytes per dword, widened.
			const __m128i bytes = _mm_setr_epi32( static_cast<int>( Fog4( call.pFogOld, idx ) ),
			                                      static_cast<int>( Fog4( call.pFogOld, idx + 4 ) ),
			                                      static_cast<int>( Fog4( call.pFogNew, idx ) ),
			                                      static_cast<int>( Fog4( call.pFogNew, idx + 4 ) ) );
			fo = _mm256_slli_epi32( _mm256_cvtepu8_epi32( bytes ), 24 );
			fn = _mm256_slli_epi32( _mm256_cvtepu8_epi32( _mm_unpackhi_epi64( bytes, bytes ) ), 24 );
		}
		const __m256i low24 = _mm256_set1_epi32( 0xffffff );
		const __m256i u = _mm256_or_si256( _mm256_and_si256( s, low24 ), fo );
		const __m256i v = _mm256_or_si256( _mm256_and_si256( c, low24 ), fn );
		// u v pairs: lo = 0 1 | 4 5, hi = 2 3 | 6 7.
		const __m256i pLo = _mm256_unpacklo_epi32( u, v );
		const __m256i pHi = _mm256_unpackhi_epi32( u, v );
		const __m128i uv01 = _mm256_castsi256_si128( pLo );
		const __m128i uv23 = _mm256_castsi256_si128( pHi );
		const __m128i uv45 = _mm256_extracti128_si256( pLo, 1 );
		const __m128i uv67 = _mm256_extracti128_si256( pHi, 1 );
		const uint32_t *pN = call.pNormals + k;
		// Even vertices take the low pair of their register, odd ones the high pair,
		// which a blend keeps in place.
		StoreVertex<eDest>( call,
		                    k + 0,
		                    PosNormal( call, call.pPosIndices[k + 0], pN + 0 ),
		                    _mm_unpacklo_epi64( TexPair( call, k + 0 ), uv01 ) );
		StoreVertex<eDest>( call,
		                    k + 1,
		                    PosNormal( call, call.pPosIndices[k + 1], pN + 1 ),
		                    _mm_blend_epi32( uv01, TexPair( call, k + 1 ), 0x3 ) );
		StoreVertex<eDest>( call,
		                    k + 2,
		                    PosNormal( call, call.pPosIndices[k + 2], pN + 2 ),
		                    _mm_unpacklo_epi64( TexPair( call, k + 2 ), uv23 ) );
		StoreVertex<eDest>( call,
		                    k + 3,
		                    PosNormal( call, call.pPosIndices[k + 3], pN + 3 ),
		                    _mm_blend_epi32( uv23, TexPair( call, k + 3 ), 0x3 ) );
		StoreVertex<eDest>( call,
		                    k + 4,
		                    PosNormal( call, call.pPosIndices[k + 4], pN + 4 ),
		                    _mm_unpacklo_epi64( TexPair( call, k + 4 ), uv45 ) );
		StoreVertex<eDest>( call,
		                    k + 5,
		                    PosNormal( call, call.pPosIndices[k + 5], pN + 5 ),
		                    _mm_blend_epi32( uv45, TexPair( call, k + 5 ), 0x3 ) );
		StoreVertex<eDest>( call,
		                    k + 6,
		                    PosNormal( call, call.pPosIndices[k + 6], pN + 6 ),
		                    _mm_unpacklo_epi64( TexPair( call, k + 6 ), uv67 ) );
		StoreVertex<eDest>( call,
		                    k + 7,
		                    PosNormal( call, call.pPosIndices[k + 7], pN + 7 ),
		                    _mm_blend_epi32( uv67, TexPair( call, k + 7 ), 0x3 ) );
	}

	// The whole-block part of a part: [ nFrom, nFrom + 8 * blocks ).
	template <bool bLight, bool bVertexColor, bool bFogIndexed, EDest eDest>
	void RunBlocks( const SVertexCall &callIn,
	                int nBlocks,
	                const SConsts8 &k,
	                int nZero,
	                const uint32_t *pVertexColor,
	                const uint32_t *pColorsIn,
	                const uint32_t *pShadowIn,
	                uint32_t *pColorsOut,
	                uint32_t *pShadowOut,
	                bool bGather )
	{
		// A local copy: the vertex stores go through may_alias vector pointers, so
		// with the caller's struct the compiler reloaded every pointer after each one.
		const SVertexCall call = callIn;
		__m256i fogOld = _mm256_setzero_si256(), fogNew = _mm256_setzero_si256();
		if ( !bFogIndexed )
		{
			fogOld = _mm256_set1_epi32( static_cast<int>( static_cast<uint32_t>( call.pFogOld[0] ) << 24 ) );
			fogNew = _mm256_set1_epi32( static_cast<int>( static_cast<uint32_t>( call.pFogNew[0] ) << 24 ) );
		}
		for ( int b = 0; b < nBlocks; ++b )
		{
			const int n = 8 * b;
			__m256i c, s;
			if ( bLight )
			{
				LightBlock8<bVertexColor>( call.pNormals + n, bVertexColor ? pVertexColor + n : nullptr, nZero - n, k, &c, &s );
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
					const __m256i vc = LoadU( pVertexColor + n );
					c = Multiply8( c, vc );
					s = Multiply8( s, vc );
				}
			}
			AssembleBlock8<bFogIndexed, eDest>( call, n, c, s, fogOld, fogNew, bGather );
		}
	}

	template <bool bLight, bool bVertexColor, bool bFogIndexed>
	void RunDest( const SVertexCall &call,
	              int nBlocks,
	              const SConsts8 &k,
	              int nZero,
	              const uint32_t *pVertexColor,
	              const uint32_t *pColorsIn,
	              const uint32_t *pShadowIn,
	              uint32_t *pColorsOut,
	              uint32_t *pShadowOut,
	              bool bGather )
	{
		switch ( GetDest( call ) )
		{
			case DEST_PLAIN:
				RunBlocks<bLight, bVertexColor, bFogIndexed, DEST_PLAIN>(
						call, nBlocks, k, nZero, pVertexColor, pColorsIn, pShadowIn, pColorsOut, pShadowOut, bGather );
				break;
			case DEST_STREAM:
				RunBlocks<bLight, bVertexColor, bFogIndexed, DEST_STREAM>(
						call, nBlocks, k, nZero, pVertexColor, pColorsIn, pShadowIn, pColorsOut, pShadowOut, bGather );
				break;
			case DEST_BOTH:
				RunBlocks<bLight, bVertexColor, bFogIndexed, DEST_BOTH>(
						call, nBlocks, k, nZero, pVertexColor, pColorsIn, pShadowIn, pColorsOut, pShadowOut, bGather );
				break;
		}
	}

	template <bool bLight>
	void RunAll( const SVertexCall &call,
	             int nBlocks,
	             const SConsts8 &k,
	             int nZero,
	             const uint32_t *pVertexColor,
	             const uint32_t *pColorsIn,
	             const uint32_t *pShadowIn,
	             uint32_t *pColorsOut,
	             uint32_t *pShadowOut,
	             bool bGather )
	{
		const bool bIndexed = call.nFogMask != 0;
		if ( pVertexColor )
		{
			if ( bIndexed )
			{
				RunDest<bLight, true, true>(
						call, nBlocks, k, nZero, pVertexColor, pColorsIn, pShadowIn, pColorsOut, pShadowOut, bGather );
			}
			else
			{
				RunDest<bLight, true, false>(
						call, nBlocks, k, nZero, pVertexColor, pColorsIn, pShadowIn, pColorsOut, pShadowOut, bGather );
			}
		}
		else
		{
			if ( bIndexed )
			{
				RunDest<bLight, false, true>(
						call, nBlocks, k, nZero, pVertexColor, pColorsIn, pShadowIn, pColorsOut, pShadowOut, bGather );
			}
			else
			{
				RunDest<bLight, false, false>(
						call, nBlocks, k, nZero, pVertexColor, pColorsIn, pShadowIn, pColorsOut, pShadowOut, bGather );
			}
		}
	}

	// LightAndAssemble (bLight) and AssembleVertices. Whole blocks in place; the last
	// 1..7 vertices go through the same block code on padded copies of their inputs,
	// and only the real vertices are copied out, so nothing is read or written past
	// any array.
	template <bool bLight>
	void Run( const SVertexCall &call,
	          const SDirectionalConsts *pConsts,
	          const uint32_t *pVertexColor,
	          const uint32_t *pColorsIn,
	          const uint32_t *pShadowIn,
	          uint32_t *pColorsOut,
	          uint32_t *pShadowOut,
	          bool bGather )
	{
		if ( call.nCount <= 0 )
		{
			return;
		}
		SConsts8 k{};
		int nZero = 0;
		if ( bLight )
		{
			k = LoadConsts( *pConsts );
			nZero = CountZeroPrefix( call.pNormals, call.nCount );
		}
		const int nBlocks = call.nCount / 8;
		RunAll<bLight>( call, nBlocks, k, nZero, pVertexColor, pColorsIn, pShadowIn, pColorsOut, pShadowOut, bGather );

		const int nDone = nBlocks * 8;
		const int nLeft = call.nCount - nDone;
		if ( nLeft == 0 )
		{
			return;
		}
		// Padding: normals 0 and colours 0 (computed, then discarded), index of the
		// first tail vertex (valid, so the position and fog reads stay in bounds).
		alignas( 32 ) uint32_t normals[8] = {}, vertexColor[8] = {}, colorsIn[8] = {}, shadowIn[8] = {};
		alignas( 32 ) uint32_t colorsOut[8], shadowOut[8];
		uint16_t indices[8];
		uint8_t uv[8 * 8] = {};
		alignas( 32 ) SVertexFull res[8];
		for ( int j = 0; j < 8; ++j )
		{
			const int src = nDone + ( j < nLeft ? j : 0 );
			indices[j] = call.pPosIndices[src];
			if ( j < nLeft )
			{
				normals[j] = call.pNormals[src];
				std::memcpy( uv + 8 * j, call.pUV + static_cast<ptrdiff_t>( call.nUVStride ) * src, 8 );
				if ( pVertexColor )
				{
					vertexColor[j] = pVertexColor[src];
				}
				if ( !bLight )
				{
					colorsIn[j] = pColorsIn[src];
					shadowIn[j] = pShadowIn[src];
				}
			}
		}
		SVertexCall tail = call;
		tail.pRes = res;
		tail.pStream = nullptr;
		tail.pPosIndices = indices;
		tail.pUV = uv;
		tail.nUVStride = 8;
		tail.pNormals = normals;
		tail.nCount = 8;
		RunAll<bLight>( tail,
		                1,
		                k,
		                nZero - nDone,
		                pVertexColor ? vertexColor : nullptr,
		                colorsIn,
		                shadowIn,
		                pColorsOut ? colorsOut : nullptr,
		                shadowOut,
		                bGather );
		if ( call.pRes )
		{
			std::memcpy( call.pRes + nDone, res, sizeof( SVertexFull ) * nLeft );
		}
		if ( call.pStream )
		{
			for ( int j = 0; j < nLeft; ++j )
			{
				const __m128i *p = reinterpret_cast<const __m128i *>( &res[j] );
				_mm_stream_si128( reinterpret_cast<__m128i *>( &call.pStream[nDone + j] ), _mm_load_si128( p ) );
				_mm_stream_si128( reinterpret_cast<__m128i *>( &call.pStream[nDone + j].tex ), _mm_load_si128( p + 1 ) );
			}
		}
		if ( pColorsOut )
		{
			std::memcpy( pColorsOut + nDone, colorsOut, sizeof( uint32_t ) * nLeft );
			std::memcpy( pShadowOut + nDone, shadowOut, sizeof( uint32_t ) * nLeft );
		}
	}

	// Directional (with the multiply if pVertexColor) into arrays, padded tail.
	template <bool bVertexColor>
	void DirectionalArrays( const uint32_t *pNormals,
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
		const SConsts8 k = LoadConsts( c );
		const int nZero = CountZeroPrefix( pNormals, nCount );
		int n = 0;
		for ( ; n + 8 <= nCount; n += 8 )
		{
			__m256i col, sh;
			LightBlock8<bVertexColor>( pNormals + n, bVertexColor ? pVertexColor + n : nullptr, nZero - n, k, &col, &sh );
			StoreU( pColors + n, col );
			StoreU( pShadow + n, sh );
		}
		const int nLeft = nCount - n;
		if ( nLeft > 0 )
		{
			alignas( 32 ) uint32_t normals[8] = {}, vertexColor[8] = {}, colors[8], shadow[8];
			std::memcpy( normals, pNormals + n, sizeof( uint32_t ) * nLeft );
			if ( bVertexColor )
			{
				std::memcpy( vertexColor, pVertexColor + n, sizeof( uint32_t ) * nLeft );
			}
			__m256i col, sh;
			LightBlock8<bVertexColor>( normals, vertexColor, nZero - n, k, &col, &sh );
			StoreU( colors, col );
			StoreU( shadow, sh );
			std::memcpy( pColors + n, colors, sizeof( uint32_t ) * nLeft );
			std::memcpy( pShadow + n, shadow, sizeof( uint32_t ) * nLeft );
		}
	}

	void DirectionalLightingAVX2(
			const uint32_t *pNormals, int nCount, const SDirectionalConsts &c, uint32_t *pColors, uint32_t *pShadow )
	{
		DirectionalArrays<false>( pNormals, nCount, c, nullptr, pColors, pShadow );
	}

	void MultiplyOnColorAVX2( uint32_t *pDst, const uint32_t *pMul, int nCount )
	{
		int n = 0;
		for ( ; n + 8 <= nCount; n += 8 )
		{
			StoreU( pDst + n, Multiply8( LoadU( pDst + n ), LoadU( pMul + n ) ) );
		}
		const int nLeft = nCount - n;
		if ( nLeft > 0 )
		{
			alignas( 32 ) uint32_t dst[8] = {}, mul[8] = {};
			std::memcpy( dst, pDst + n, sizeof( uint32_t ) * nLeft );
			std::memcpy( mul, pMul + n, sizeof( uint32_t ) * nLeft );
			StoreU( dst, Multiply8( LoadU( dst ), LoadU( mul ) ) );
			std::memcpy( pDst + n, dst, sizeof( uint32_t ) * nLeft );
		}
	}

	void DirectionalMultiplyAVX2( const uint32_t *pNormals,
	                              int nCount,
	                              const SDirectionalConsts &c,
	                              const uint32_t *pVertexColor,
	                              uint32_t *pColors,
	                              uint32_t *pShadow )
	{
		if ( pVertexColor )
		{
			DirectionalArrays<true>( pNormals, nCount, c, pVertexColor, pColors, pShadow );
		}
		else
		{
			DirectionalArrays<false>( pNormals, nCount, c, nullptr, pColors, pShadow );
		}
	}

	void AssembleVerticesAVX2( const SVertexCall &call,
	                           const uint32_t *pVertexColor,
	                           const uint32_t *pColors,
	                           const uint32_t *pShadow )
	{
		Run<false>( call, nullptr, pVertexColor, pColors, pShadow, nullptr, nullptr, true );
	}

	void AssembleVerticesAVX2NoGather( const SVertexCall &call,
	                                   const uint32_t *pVertexColor,
	                                   const uint32_t *pColors,
	                                   const uint32_t *pShadow )
	{
		Run<false>( call, nullptr, pVertexColor, pColors, pShadow, nullptr, nullptr, false );
	}

	void LightAndAssembleAVX2( const SVertexCall &call,
	                           const SDirectionalConsts &c,
	                           const uint32_t *pVertexColor,
	                           uint32_t *pColorsOut,
	                           uint32_t *pShadowOut )
	{
		Run<true>( call, &c, pVertexColor, nullptr, nullptr, pColorsOut, pShadowOut, true );
	}

	void LightAndAssembleAVX2NoGather( const SVertexCall &call,
	                                   const SDirectionalConsts &c,
	                                   const uint32_t *pVertexColor,
	                                   uint32_t *pColorsOut,
	                                   uint32_t *pShadowOut )
	{
		Run<true>( call, &c, pVertexColor, nullptr, nullptr, pColorsOut, pShadowOut, false );
	}

} // namespace

extern const SFusedLightKernels avx2FusedLightKernels = { "avx2",
                                                          &DirectionalLightingAVX2,
                                                          &MultiplyOnColorAVX2,
                                                          &DirectionalMultiplyAVX2,
                                                          &AssembleVerticesAVX2,
                                                          &LightAndAssembleAVX2 };

// The same without vpgatherdd: the fog bytes are loaded one by one. For CPUs where
// gathers are slow (AMD before Zen 3) or microcode-slowed (Intel's Gather Data
// Sampling mitigation on Skylake through Tiger Lake).
extern const SFusedLightKernels avx2NoGatherFusedLightKernels = { "avx2-nogather",
                                                                  &DirectionalLightingAVX2,
                                                                  &MultiplyOnColorAVX2,
                                                                  &DirectionalMultiplyAVX2,
                                                                  &AssembleVerticesAVX2NoGather,
                                                                  &LightAndAssembleAVX2NoGather };

} // namespace NLightFused
