#pragma once

// Hardware reference for the mmx:: emulation helpers, written with SSE2 intrinsics
// rather than inline assembly or MASM.
//
// test/mmx.h does the same job with __asm, which MSVC accepts on x86 only, so the
// MMXhelpers test could never run on x64. MASM would fix that but only for MSVC:
// a Linux or macOS build would need the same code again in NASM or GAS syntax. SSE2
// intrinsics compile everywhere x86 does, with every compiler, and are still a
// hardware reference rather than a software model of one.
//
// This is sound because SSE2 defines these integer operations exactly as MMX does.
// Every operation below is either lane-wise or shifts within a 64-bit lane, so
// running it on the low half of an __m128i with the high half zeroed reproduces the
// 64-bit MMX result bit for bit. The three that are not that simple - punpckhwd and
// the two packs - are noted where they are defined.
//
// Operand order follows the assembly: the first argument is the destination, which is
// the one the instruction reads and overwrites.

#include <emmintrin.h>

#include <cstdint>

namespace original
{

namespace detail
{

inline __m128i Load( uint64_t v )
{
    // High half zeroed, which the pack helpers below rely on.
    return _mm_loadl_epi64( reinterpret_cast<const __m128i*>( &v ) );
}

inline uint64_t Store( __m128i v )
{
    uint64_t out;
    _mm_storel_epi64( reinterpret_cast<__m128i*>( &out ), v );
    return out;
}

// MMX packs take four values from the destination and four from the source into one
// 64-bit result. The SSE2 pack takes eight from each into 128 bits, so its low half
// would be all destination. Concatenating the two 64-bit halves first puts the eight
// inputs where the SSE2 instruction expects them, and its low half is then the MMX
// answer.
inline __m128i Concat( uint64_t a, uint64_t b )
{
    return _mm_unpacklo_epi64( Load( a ), Load( b ) );
}

}

#define OBK2_MMX_UNARY( name, expr )                                    \
    static uint64_t name( uint64_t val )                                \
    {                                                                   \
        const __m128i x = detail::Load( val );                          \
        return detail::Store( expr );                                   \
    }

#define OBK2_MMX_BINARY( name, expr )                                   \
    static uint64_t name( uint64_t dest, uint64_t src )                 \
    {                                                                   \
        const __m128i a = detail::Load( dest );                         \
        const __m128i b = detail::Load( src );                          \
        return detail::Store( expr );                                   \
    }

OBK2_MMX_UNARY( psllw_1, _mm_slli_epi16( x, 1 ) )
OBK2_MMX_UNARY( psllw_3, _mm_slli_epi16( x, 3 ) )
OBK2_MMX_UNARY( psllw_5, _mm_slli_epi16( x, 5 ) )
OBK2_MMX_UNARY( psrlw_1, _mm_srli_epi16( x, 1 ) )
OBK2_MMX_UNARY( psrlw_2, _mm_srli_epi16( x, 2 ) )
OBK2_MMX_UNARY( psrad_14, _mm_srai_epi32( x, 14 ) )
OBK2_MMX_UNARY( psrad_15, _mm_srai_epi32( x, 15 ) )
OBK2_MMX_UNARY( psraw_4, _mm_srai_epi16( x, 4 ) )
OBK2_MMX_UNARY( psraw_16, _mm_srai_epi16( x, 16 ) )

// psllq and psrlq shift the whole 64-bit MMX register. The SSE2 forms shift each
// 64-bit lane on its own, so the low lane behaves identically.
OBK2_MMX_UNARY( psllq_16, _mm_slli_epi64( x, 16 ) )
OBK2_MMX_UNARY( psrlq_32, _mm_srli_epi64( x, 32 ) )

OBK2_MMX_BINARY( paddw, _mm_add_epi16( a, b ) )
OBK2_MMX_BINARY( paddd, _mm_add_epi32( a, b ) )
OBK2_MMX_BINARY( paddsw, _mm_adds_epi16( a, b ) )
OBK2_MMX_BINARY( psubw, _mm_sub_epi16( a, b ) )
OBK2_MMX_BINARY( pmulhw, _mm_mulhi_epi16( a, b ) )
OBK2_MMX_BINARY( pmullw, _mm_mullo_epi16( a, b ) )
OBK2_MMX_BINARY( pmaddwd, _mm_madd_epi16( a, b ) )
OBK2_MMX_BINARY( pand, _mm_and_si128( a, b ) )
OBK2_MMX_BINARY( pandn, _mm_andnot_si128( a, b ) )
OBK2_MMX_BINARY( por, _mm_or_si128( a, b ) )
OBK2_MMX_BINARY( pxor, _mm_xor_si128( a, b ) )
OBK2_MMX_BINARY( pcmpgtw, _mm_cmpgt_epi16( a, b ) )
OBK2_MMX_BINARY( pcmpeqw, _mm_cmpeq_epi16( a, b ) )

// The low halves of both operands hold everything, so unpacking low reproduces MMX.
OBK2_MMX_BINARY( punpcklbw, _mm_unpacklo_epi8( a, b ) )
OBK2_MMX_BINARY( punpcklwd, _mm_unpacklo_epi16( a, b ) )
OBK2_MMX_BINARY( punpckldq, _mm_unpacklo_epi32( a, b ) )

// punpckhwd is the exception: MMX takes the high 32 bits of each 64-bit operand, while
// the SSE2 high unpack takes the high 64 bits of each 128-bit one. Shifting each
// operand down by 4 bytes first moves the wanted halves into place.
OBK2_MMX_BINARY( punpckhwd,
    _mm_unpacklo_epi16( _mm_srli_si128( a, 4 ), _mm_srli_si128( b, 4 ) ) )

#undef OBK2_MMX_UNARY
#undef OBK2_MMX_BINARY

static uint64_t packssdw( uint64_t dest, uint64_t src )
{
    const __m128i both = detail::Concat( dest, src );
    return detail::Store( _mm_packs_epi32( both, both ) );
}

static uint64_t packuswb( uint64_t dest, uint64_t src )
{
    const __m128i both = detail::Concat( dest, src );
    return detail::Store( _mm_packus_epi16( both, both ) );
}

}
