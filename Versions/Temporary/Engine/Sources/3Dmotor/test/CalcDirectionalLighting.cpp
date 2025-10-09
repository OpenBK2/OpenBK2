#include <gtest/gtest.h>

#include "3Dmotor/stdafx.h"
#include "3Dmotor/GLightPerVertex.cpp"

#include "random.h"
#include "original.h"

bool bIsSSEPresent;
unsigned char nCubicRoot[32768];

enum { iterations = 10000 };

TEST(CalcDirectionalLighting, CalcDirectionalLightingRandom) {
    for (size_t i = 0; i < iterations; ++i) {

        std::vector<WORD> posIndices;
        posIndices.emplace_back(random_uint16());
        std::vector<NGfx::SCompactVector> normals;
        NGfx::SCompactVector normal;
        normal.dw = random_uint32();
        normals.emplace_back(normal);
        NGScene::SPerVertexLightState ls;
        random_mmx_word(ls.ambient);
        random_mmx_word(ls.lightColor);
        random_mmx_word(ls.incidentShadowColor);
        random_mmx_word(ls.shadeColor);
        random_mmx_word(ls.dirLight);
        random_mmx_word(ls.shift);
        bool bTranslucent = random_bool();
        CVec3 vTranslucentColor(random_float(), random_float(), random_float());

        std::vector<DWORD> ResColors, RefColors;
        std::vector<DWORD> ResShadow, RefShadow;

        NGScene::CalcDirectionalLighting(posIndices, normals, ls, bTranslucent, vTranslucentColor, &ResColors, &ResShadow);
        original::CalcDirectionalLighting(posIndices, normals, ls, bTranslucent, vTranslucentColor, &RefColors, &RefShadow);

        EXPECT_EQ(ResColors, RefColors);
        EXPECT_EQ(ResShadow, RefShadow);
    }
}
