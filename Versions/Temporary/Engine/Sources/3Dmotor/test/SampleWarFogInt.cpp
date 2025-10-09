#include "3Dmotor/GLightPerVertex.cpp"

#include <gtest/gtest.h>

#include "random.h"
#include "original.h"

bool bIsSSEPresent;
unsigned char nCubicRoot[32768];

enum { iterations = 1000 };

TEST(SampleWarFogInt, SampleWarFogIntRandom) {

    for (size_t i = 0; i < iterations; i++) {

        std::vector<int> intCoords;
        intCoords.emplace_back(random_int());
        intCoords.emplace_back(random_int());
        CArray2D<unsigned char> fog;
        fog.SetSizes( 3, 3 );
        for (int u = 0; u < 3; u++) {
            for (int v = 0; v < 3; v++) {
                fog[u][v] = random_uint8();
            }
        }
        std::vector<unsigned char> res, ref;
        int nVertices = 1;

        res.resize(nVertices);
        ref.resize(nVertices);

        NGScene::SampleWarFogInt(intCoords, fog, &res, nVertices);
        original::SampleWarFogInt(intCoords, fog, &ref, nVertices);

        EXPECT_EQ(res, ref);
    }
}
