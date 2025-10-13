#include <gtest/gtest.h>

#include "3Dmotor/stdafx.h"
#include "3Dmotor/GLightPerVertex.cpp"

#include "random.h"
#include "original.h"

#include <cstdint>

bool bIsSSEPresent;
unsigned char nCubicRoot[32768];

enum { iterations = 10000 };

TEST(CalcPointLightColors, CalcPointLightColorsRandom) {
    for (size_t i = 0; i < iterations; ++i) {

        std::vector<NGfx::SMMXWord> res, ref;
        res.resize(1);
        ref.resize(1);
        std::vector<NGfx::SMMXWord> attenuations;
        NGfx::SMMXWord attenuation;
        random_mmx_word(attenuation);
        attenuations.emplace_back(attenuation);
        NGScene::SUVInfo src;
        src.normal.x = random_uint8();
        src.normal.y = random_uint8();
        src.normal.z = random_uint8();
        src.normal.w = random_uint8();
        std::vector<uint16_t> posIndices;
        posIndices.emplace_back(0);
        std::vector<NGfx::SCompactVector> normals;
        NGfx::SCompactVector normal;
        normal.x = random_uint8();
        normal.y = random_uint8();
        normal.z = random_uint8();
        normal.w = random_uint8();
        normals.emplace_back(normal);
        CVec3 vColor{random_float(), random_float(), random_float()};

        NGScene::CalcPointLightColors(&res, attenuations, posIndices, normals, vColor);
        original::CalcPointLightColors(&ref, attenuations, posIndices, normals, vColor);

        EXPECT_EQ(res.size(), ref.size());
        for (size_t j = 0; j < res.size(); ++j) {
            EXPECT_TRUE(0 == memcmp(&ref[j], &res[j], sizeof(NGfx::SMMXWord)));
        }
    }
}


TEST(CalcPointLightColors, CalcPointLightColorsRandom2) {
    for (size_t i = 0; i < iterations; ++i) {

        std::vector<NGfx::SMMXWord> res, ref;
        res.resize(1);
        ref.resize(1);
        NGfx::SMMXWord attenuation;
        random_mmx_word(attenuation);
        NGScene::SUVInfo src;
        src.normal.x = random_uint8();
        src.normal.y = random_uint8();
        src.normal.z = random_uint8();
        src.normal.w = random_uint8();
        CVec3 vColor{random_float(), random_float(), random_float()};
        int nSize = 1;

        NGScene::CalcPointLightColors(&res, attenuation, &src, nSize, vColor);
        original::CalcPointLightColors(&ref, attenuation, &src, nSize, vColor);

        EXPECT_EQ(res.size(), ref.size());
        for (size_t j = 0; j < res.size(); ++j) {
            EXPECT_TRUE(0 == memcmp(&ref[j], &res[j], sizeof(NGfx::SMMXWord)));
        }
    }
}
