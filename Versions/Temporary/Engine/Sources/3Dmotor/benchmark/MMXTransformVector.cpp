#include <benchmark/benchmark.h>

#include "3Dmotor/stdafx.h"
#include "3Dmotor/GSSEtransform.h"
#include "3Dmotor/GSSEtransform.cpp"
// Same MASM references the test uses, so the benchmark is no longer x86 only.
#include "3Dmotor/test/original/MMXTransformVector.h"
#include "3Dmotor/test/random.h"

#include <vector>

namespace {

// The inputs are built once, outside the timed loop. Generating a random rotation
// matrix costs far more than transforming a vector by one, so the previous version
// was mostly measuring randomizeMatrix and reported the two implementations as
// nearly equal no matter what they did.
struct SInputs
{
    std::vector<NGfx::SCompactVector> src;
    std::vector<SHMatrix> transform;
    std::vector<uint8_t> weight;

    SInputs()
    {
        enum { count = 1024 };
        src.resize( count );
        transform.resize( count );
        weight.resize( count );
        for ( int k = 0; k < count; ++k )
        {
            randomizeNormalVector( src[k] );
            randomizeMatrix( transform[k] );
            weight[k] = random_uint8();
        }
    }
};

const SInputs &Inputs()
{
    static const SInputs inputs;
    return inputs;
}

}

static void BM_original( benchmark::State &state )
{
    const SInputs &in = Inputs();
    size_t k = 0;
    for ( auto _ : state )
    {
        NGfx::SCompactVector res{};
        original::MMXTransformVector( res, in.src[k], in.transform[k] );
        benchmark::DoNotOptimize( res );
        k = ( k + 1 ) % in.src.size();
    }
}

static void BM_current( benchmark::State &state )
{
    const SInputs &in = Inputs();
    size_t k = 0;
    for ( auto _ : state )
    {
        NGfx::SCompactVector res{};
        MMXTransformVector( res, in.src[k], in.transform[k] );
        benchmark::DoNotOptimize( res );
        k = ( k + 1 ) % in.src.size();
    }
}

static void BM_original2( benchmark::State &state )
{
    const SInputs &in = Inputs();
    size_t k = 0;
    for ( auto _ : state )
    {
        NGfx::SCompactVector res{};
        const uint8_t w = in.weight[k];
        original::MMXTransformVector2( res, in.src[k],
            in.transform[k], w,
            in.transform[( k + 1 ) % in.transform.size()], static_cast<uint8_t>( 255 - w ) );
        benchmark::DoNotOptimize( res );
        k = ( k + 1 ) % in.src.size();
    }
}

static void BM_current2( benchmark::State &state )
{
    const SInputs &in = Inputs();
    size_t k = 0;
    for ( auto _ : state )
    {
        NGfx::SCompactVector res{};
        const uint8_t w = in.weight[k];
        MMXTransformVector2( res, in.src[k],
            in.transform[k], w,
            in.transform[( k + 1 ) % in.transform.size()], static_cast<uint8_t>( 255 - w ) );
        benchmark::DoNotOptimize( res );
        k = ( k + 1 ) % in.src.size();
    }
}

static void BM_original3( benchmark::State &state )
{
    const SInputs &in = Inputs();
    size_t k = 0;
    for ( auto _ : state )
    {
        NGfx::SCompactVector res{};
        const size_t n = in.transform.size();
        original::MMXTransformVector3( res, in.src[k],
            in.transform[k], 85, in.transform[( k + 1 ) % n], 85, in.transform[( k + 2 ) % n], 85 );
        benchmark::DoNotOptimize( res );
        k = ( k + 1 ) % in.src.size();
    }
}

static void BM_current3( benchmark::State &state )
{
    const SInputs &in = Inputs();
    size_t k = 0;
    for ( auto _ : state )
    {
        NGfx::SCompactVector res{};
        const size_t n = in.transform.size();
        MMXTransformVector3( res, in.src[k],
            in.transform[k], 85, in.transform[( k + 1 ) % n], 85, in.transform[( k + 2 ) % n], 85 );
        benchmark::DoNotOptimize( res );
        k = ( k + 1 ) % in.src.size();
    }
}

BENCHMARK( BM_original );
BENCHMARK( BM_current );
BENCHMARK( BM_original2 );
BENCHMARK( BM_current2 );
BENCHMARK( BM_original3 );
BENCHMARK( BM_current3 );

BENCHMARK_MAIN();
