#include <gtest/gtest.h>

#include "3Dmotor/stdafx.h"
#include "3Dmotor/GLightPerVertex.cpp"
// GSSEtransform is one filling nCubicRoot array
#include "3Dmotor/GSSEtransform.h"
#include "3Dmotor/GSSEtransform.cpp"

#include "random.h"
#include "original.h"

#include <cstdint>

enum { iterations = 10000 };

TEST(AddColors, AddColorsRandom) {
    for (size_t i = 0; i < iterations; ++i) {

        size_t count = 1;
        std::vector<uint32_t> res, ref;
        res.resize(count);
        ref.resize(count);
        std::vector<uint32_t> src;
        std::vector<NGfx::SMMXWord> add;
        for (size_t j = 0; j < count; ++j) {

            src.emplace_back(random_uint32());

            NGfx::SMMXWord color;
            random_mmx_word(color);
            add.emplace_back(color);
        }

        NGScene::AddColors(&res, src, add);
        original::AddColors(&ref, src, add);

        if (res[0] != ref[0]) {
            int x = 0;
        }

        EXPECT_EQ(res, ref);
    }
}
