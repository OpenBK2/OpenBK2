#include "3Dmotor/stdafx.h"
#include "3Dmotor/GSSEtransform.h"
#include "3Dmotor/GSSEtransform.cpp"

#include <gtest/gtest.h>

#include "original.h"
#include "random.h"

enum { iterations = 100000 };


static void compareVectors(const NGfx::SCompactVector & v1, const NGfx::SCompactVector & v2, int precision = 1) {

    EXPECT_NEAR(v1.x, v2.x, precision);
    EXPECT_NEAR(v1.y, v2.y, precision);
    EXPECT_NEAR(v1.z, v2.z, precision);
    EXPECT_NEAR(v1.w, v2.w, precision);
}

TEST(MMXEmulation, MMXTransformVector) {

    for (int i = 0; i < iterations; ++i) {
        NGfx::SCompactVector src{}, resExpected{}, resActual{};
        SHMatrix transform{};

        randomizeNormalVector(src);
        randomizeMatrix(transform);

        original::MMXTransformVector(resExpected, src, transform);
        MMXTransformVector(resActual, src, transform);

        compareVectors(resExpected, resActual);
    }
}

TEST(MMXEmulation, MMXTransformVector2) {

    for (int i = 0; i < iterations; ++i) {
        NGfx::SCompactVector src{}, resExpected{}, resActual{};
        SHMatrix transform1{}, transform2{};

        randomizeNormalVector(src);
        randomizeMatrix(transform1);
        randomizeMatrix(transform2);
        uint8_t w1 = random_uint8();
        uint8_t w2 = 255 - w1;

        original::MMXTransformVector2(resExpected, src, transform1, w1, transform2, w2);
        MMXTransformVector2(resActual, src, transform1,w1, transform2, w2);

        compareVectors(resExpected, resActual, 3);
    }
}

TEST(MMXEmulation, MMXTransformVector3) {

    for (int i = 0; i < iterations; ++i) {
        NGfx::SCompactVector src{}, resExpected{}, resActual{};
        SHMatrix transform1{}, transform2{}, transform3{};

        randomizeNormalVector(src);
        randomizeMatrix(transform1);
        randomizeMatrix(transform2);
        randomizeMatrix(transform3);
        uint8_t w1 = random_uint8() / 2;
        uint8_t w2 = random_uint8() / 2;
        uint8_t w3 = 255 - w1 - w2;

        original::MMXTransformVector3(resExpected, src, transform1, w1, transform2, w2, transform3, w3);
        MMXTransformVector3(resActual, src, transform1, w1, transform2, w2, transform3, w3);

        compareVectors(resExpected, resActual, 3);
    }
}
