#include <benchmark/benchmark.h>

#include "3Dmotor/stdafx.h"
#include "3Dmotor/GSSEtransform.h"
#include "3Dmotor/GSSEtransform.cpp"
#include "3Dmotor/test/original.h"
#include "3Dmotor/test/random.h"

static void BM_original(benchmark::State& state) {
    for (auto _ : state) {

        NGfx::SCompactVector src{}, res{};
        SHMatrix transform{};

        randomizeVector(src);
        randomizeMatrix(transform);

        original::MMXTransformVector(res, src, transform);
    }
}

static void BM_current(benchmark::State& state) {
    for (auto _ : state) {
        NGfx::SCompactVector src{}, res{};
        SHMatrix transform{};

        randomizeVector(src);
        randomizeMatrix(transform);

        MMXTransformVector(res, src, transform);
    }
}

BENCHMARK(BM_original);
BENCHMARK(BM_current);

BENCHMARK_MAIN();
