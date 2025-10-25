#include <benchmark/benchmark.h>

#include "3Dmotor/stdafx.h"
#include "3Dmotor/GSSEtransform.h"
#include "3Dmotor/GSSEtransform.cpp"
#include "3Dmotor/test/original.h"
#include "3Dmotor/test/random.h"

static void BM_original(benchmark::State& state) {
    for (auto _ : state) {

        NGfx::SCompactVector src{}, res{};
        NGfx::SCompactTransformer trans{};

        randomizeVector(src);
        randomizeTransformer(trans);

        original::MMXTransformVector(&res, &src, &trans);
    }
}

static void BM_current(benchmark::State& state) {
    for (auto _ : state) {
        NGfx::SCompactVector src{}, res{};
        NGfx::SCompactTransformer trans{};

        randomizeVector(src);
        randomizeTransformer(trans);

        MMXTransformVector(&res, &src, &trans);
    }
}

BENCHMARK(BM_original);
BENCHMARK(BM_current);

BENCHMARK_MAIN();
