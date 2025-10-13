#include <gtest/gtest.h>

#include "3Dmotor/stdafx.h"
#include "3Dmotor/GLightPerVertex.cpp"

#include "random.h"
#include "original.h"

#include <cstdint>

bool bIsSSEPresent;
unsigned char nCubicRoot[32768];

enum { iterations = 10000 };

TEST(ScaleColors, ScaleColorsRandom) {
    for (size_t i = 0; i < iterations; ++i) {

        std::vector<uint32_t> res, ref;
        res.resize(1);
        ref.resize(1);
        uint32_t src = random_uint32();
        int nSrcStride = random_int();
	    unsigned char scale = random_uint8();
        int nScaleMask = 1;
        std::vector<uint16_t> posIndices;
        posIndices.emplace_back(0);
        std::vector<NGfx::SCompactVector> transp;
        NGfx::SCompactVector transparent;
        transparent.x = random_uint8();
        transparent.y = random_uint8();
        transparent.z = random_uint8();
        transparent.w = random_uint8();
        transp.emplace_back(transparent);
	    bool bMultiplyOnTransparency = random_bool();

        NGScene::ScaleColors(&res, &src, nSrcStride, &scale, nScaleMask, posIndices, transp, bMultiplyOnTransparency);
        original::ScaleColors(&ref, &src, nSrcStride, &scale, nScaleMask, posIndices, transp, bMultiplyOnTransparency);

        EXPECT_EQ(res, ref);
    }
}
