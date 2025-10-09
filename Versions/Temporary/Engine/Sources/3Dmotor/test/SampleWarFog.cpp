#include "3Dmotor/GLightPerVertex.cpp"

#include <gtest/gtest.h>

#include "random.h"
#include "original.h"

bool bIsSSEPresent;
unsigned char nCubicRoot[32768];

enum { iterations = 1000 };

TEST(SampleWarFog, SampleWarFogRandom) {

    for (size_t i = 0; i < iterations; i++) {

        std::vector<CVec3> srcPos;
        srcPos.emplace_back(CVec3{random_float(), random_float(), random_float()});
        float fScale = random_float();
        std::vector<unsigned char> res1, res2, ref1, ref2;
        CArray2D<unsigned char> fog1, fog2;
        fog1.SetSizes( 3, 3 );
        fog2.SetSizes( 3, 3 );
        for (int u = 0; u < 3; u++) {
            for (int v = 0; v < 3; v++) {
                fog1[u][v] = random_uint8();
                fog2[u][v] = random_uint8();
            }
        }

        NGScene::SampleWarFog(srcPos, fScale, &res1, fog1, &res2, fog2);
        original::SampleWarFog(srcPos, fScale, &ref1, fog1, &ref2, fog2);

        EXPECT_EQ(res1, ref1);
        EXPECT_EQ(res2, ref2);
    }
}
