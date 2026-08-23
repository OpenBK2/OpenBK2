#include "3Dmotor/stdafx.h"
#include "3Dmotor/GLightPerVertexKernels.h"

#include <gtest/gtest.h>

#include "random.h"
#include "original.h"

bool bIsSSEPresent;
unsigned char nCubicRoot[32768];

enum { iterations = 1000 };

namespace {

// Every kernel set this CPU can actually execute. Entering avx2LightingKernels on a
// machine without AVX2 would fault, so it is only added when CPUID agrees.
std::vector<const NGScene::SLightingKernels *> AvailableKernels()
{
    std::vector<const NGScene::SLightingKernels *> kernels;
    kernels.push_back( &NGScene::refLightingKernels );
    if ( NGScene::IsSSE2Present() )
        kernels.push_back( &NGScene::sse2LightingKernels );
    if ( NGScene::IsAVX2Present() )
        kernels.push_back( &NGScene::avx2LightingKernels );
    return kernels;
}

}

TEST(SampleWarFogInt, SampleWarFogIntRandom) {

    for (const NGScene::SLightingKernels *pKernels : AvailableKernels()) {
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

            pKernels->pSampleWarFogInt(
                &intCoords[0], &fog[0][0], fog.GetSizeX(), &res[0], nVertices);
            original::SampleWarFogInt(intCoords, fog, &ref, nVertices);

            EXPECT_EQ(res, ref) << "kernel set: " << pKernels->pszName;
        }
    }
}
