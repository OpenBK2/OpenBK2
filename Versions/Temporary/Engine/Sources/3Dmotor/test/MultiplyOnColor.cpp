#include <gtest/gtest.h>

#include "3Dmotor/stdafx.h"
#include "3Dmotor/GLightPerVertex.cpp"

#include "random.h"
#include "original.h"

bool bIsSSEPresent;
unsigned char nCubicRoot[32768];

TEST(MultiplyOnColor, MultiplyOnColorRandom) {

    for (size_t i = 0; i < 10000; ++i) {
        std::vector<DWORD> res, ref, mult;

        auto c = random_uint32();
        res.push_back(c);
        ref.push_back(c);
        mult.push_back(random_uint32());

        NGScene::MultiplyOnColor(&res, mult);
        original::MultiplyOnColor(&ref, mult);

        ASSERT_EQ(res, ref);
    }
}
