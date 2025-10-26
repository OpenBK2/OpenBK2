#include "3Dmotor/stdafx.h"
#include "3Dmotor/GSSEtransform.h"
#include "3Dmotor/GSSEtransform.cpp"

#include <gtest/gtest.h>

#include "original.h"
#include "random.h"

enum { iterations = 100000 };

TEST(MMXEmulation, MMXTransformVector) {

    for (int i = 0; i < iterations; ++i) {
        NGfx::SCompactVector src{}, resExpected{}, resActual{};
        SHMatrix transform{};

        randomizeVector(src);
        randomizeMatrix(transform);

        original::MMXTransformVector(resExpected, src, transform);
        MMXTransformVector(resActual, src, transform);

        EXPECT_EQ(0, memcmp(&resExpected, &resActual, sizeof(NGfx::SCompactVector)));
    }
}

TEST(MMXEmulation, MMXTransformVector2) {

    for (int i = 0; i < iterations; ++i) {
        NGfx::SCompactVector src{}, resExpected{}, resActual{};
        SHMatrix transform1{}, transform2{};

        randomizeVector(src);
        randomizeMatrix(transform1);
        randomizeMatrix(transform2);
        uint8_t w1 = random_uint8();
        uint8_t w2 = random_uint8();

        original::MMXTransformVector2(resExpected, src, transform1, w1, transform2, w2);
        MMXTransformVector2(resActual, src, transform1,w1, transform2, w2);

        ASSERT_EQ(0, memcmp(&resExpected, &resActual, sizeof(NGfx::SCompactVector)));
    }
}

TEST(MMXEmulation, MMXTransformVector3) {

    for (int i = 0; i < iterations; ++i) {
        NGfx::SCompactVector src{}, resExpected{}, resActual{};
        SHMatrix transform1{}, transform2{}, transform3{};

        randomizeVector(src);
        randomizeMatrix(transform1);
        randomizeMatrix(transform2);
        randomizeMatrix(transform3);
        uint8_t w1 = random_uint8();
        uint8_t w2 = random_uint8();
        uint8_t w3 = random_uint8();

        original::MMXTransformVector3(resExpected, src, transform1, w1, transform2, w2, transform3, w3);
        MMXTransformVector3(resActual, src, transform1, w1, transform2, w2, transform3, w3);

        ASSERT_EQ(0, memcmp(&resExpected, &resActual, sizeof(NGfx::SCompactVector)));
    }
}
