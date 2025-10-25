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
        SMMXFixups fixups{};
        NGfx::SCompactTransformer trans{};

        randomizeVector(src);
        randomizeTransformer(trans);

        original::MMXTransformVector(&resExpected, &src, &trans);
        MMXTransformVector(&resActual, &src, &trans);

        EXPECT_EQ(0, memcmp(&resExpected, &resActual, sizeof(NGfx::SCompactVector)));
    }
}

TEST(MMXEmulation, MMXTransformVector2) {

    for (int i = 0; i < iterations; ++i) {
        NGfx::SCompactVector src{}, resExpected{}, resActual{};
        NGfx::SCompactTransformer trans{}, trans2{};

        randomizeVector(src);
        randomizeTransformer(trans);
        randomizeTransformer(trans2);
        char w1 = random_uint8();
        char w2 = random_uint8();

        original::MMXTransformVector2(&resExpected, &src, &trans, w1, &trans2, w2);
        MMXTransformVector2(&resActual, &src, &trans,w1, &trans2, w2);

        ASSERT_EQ(0, memcmp(&resExpected, &resActual, sizeof(NGfx::SCompactVector)));
    }
}

TEST(MMXEmulation, MMXTransformVector3) {

    for (int i = 0; i < iterations; ++i) {
        NGfx::SCompactVector src{}, resExpected{}, resActual{};
        NGfx::SCompactTransformer trans{}, trans2{}, trans3{};

        randomizeVector(src);
        randomizeTransformer(trans);
        randomizeTransformer(trans2);
        randomizeTransformer(trans3);
        char w1 = random_uint8();
        char w2 = random_uint8();
        char w3 = random_uint8();

        original::MMXTransformVector3(&resExpected, &src, &trans, w1, &trans2, w2, &trans3, w3);
        MMXTransformVector3(&resActual, &src, &trans, w1, &trans2, w2, &trans3, w3);

        ASSERT_EQ(0, memcmp(&resExpected, &resActual, sizeof(NGfx::SCompactVector)));
    }
}
