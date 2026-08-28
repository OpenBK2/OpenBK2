#include "3Dmotor/MMXhelpers.h"

#include "random.h"
// The reference used to be test/mmx.h, which is inline __asm and so x86 only.
// original/MMXPrimitives.h is the same reference in SSE2 intrinsics: still real
// hardware rather than a model of it, but it builds on x64, and on any compiler
// rather than only MSVC, which matters for the Linux and macOS ports.
#include "original/MMXPrimitives.h"

#include <gtest/gtest.h>

enum { iterations = 10000 };

TEST(MMXEmulation, PSLLW) {

    for (size_t i = 0; i < iterations; ++i) {
        uint64_t value = random_uint64();
        uint64_t expected = original::psllw_5(value);
        uint64_t actual = mmx::psllw(value, 5);
        ASSERT_EQ(actual, expected);

        actual = original::psllw_3(value);
        expected = mmx::psllw(value, 3);
        ASSERT_EQ(actual, expected);

        actual = original::psllw_1(value);
        expected = mmx::psllw(value, 1);
        ASSERT_EQ(actual, expected);
    }
}

TEST(MMXEmulation, PSLLQ) {

    for (size_t i = 0; i < iterations; ++i) {
        uint64_t value = random_uint64();
        uint64_t actual = original::psllq_16(value);
        uint64_t expected = mmx::psllq(value, 16);
        EXPECT_EQ(actual, expected);
    }
}

TEST(MMXEmulation, PSRLQ) {

    for (size_t i = 0; i < iterations; ++i) {
        uint64_t value = random_uint64();
        uint64_t actual =  original::psrlq_32(value);
        uint64_t expected = mmx::psrlq(value, 32);
        EXPECT_EQ(actual, expected);
    }
}

TEST(MMXEmulation, PSRLW) {

    for (size_t i = 0; i < iterations; ++i) {
        uint64_t value = random_uint64();
        uint64_t actual =  original::psrlw_1(value);
        uint64_t expected = mmx::psrlw(value, 1);
        EXPECT_EQ(actual, expected);

        actual =  original::psrlw_2(value);
        expected = mmx::psrlw(value, 2);
        EXPECT_EQ(actual, expected);
    }
}

TEST(MMXEmulation, PUNPCKLBW) {

    for (size_t i = 0; i < iterations; ++i) {

        uint64_t low = random_uint64();
        uint64_t high = random_uint64();

        uint64_t actual = mmx::punpcklbw(low, high);
        uint64_t expected = original::punpcklbw(low, high);
        ASSERT_EQ(actual, expected);
    }
}

TEST(MMXEmulation, PUNPCKLWD) {

    for (size_t i = 0; i < iterations; ++i) {

        uint64_t low = random_uint64();
        uint64_t high = random_uint64();

        uint64_t actual = mmx::punpcklwd(low, high);
        uint64_t expected = original::punpcklwd(low, high);
        ASSERT_EQ(actual, expected);
    }
}

TEST(MMXEmulation, PUNPCKHWD) {

    for (size_t i = 0; i < iterations; ++i) {

        uint64_t low = random_uint64();
        uint64_t high = random_uint64();

        uint64_t actual = mmx::punpckhwd(low, high);
        uint64_t expected = original::punpckhwd(low, high);
        ASSERT_EQ(actual, expected);
    }
}

TEST(MMXEmulation, PUNPCKLDQ) {

    for (size_t i = 0; i < iterations; ++i) {

        uint64_t low = random_uint64();
        uint64_t high = random_uint64();

        uint64_t actual = mmx::punpckldq(low, high);
        uint64_t expected = original::punpckldq(low, high);
        ASSERT_EQ(actual, expected);
    }
}

TEST(MMXEmulation, PACKSSDW) {

    for (size_t i = 0; i < iterations; ++i) {

        uint64_t low = random_uint64();
        uint64_t high = random_uint64();

        uint64_t actual = mmx::packssdw(low, high);
        uint64_t expected = original::packssdw(low, high);
        ASSERT_EQ(actual, expected);
    }
}

TEST(MMXEmulation, PACKUSWB) {

    for (size_t i = 0; i < iterations; ++i) {

        uint64_t low = random_uint64();
        uint64_t high = random_uint64();

        uint64_t actual = mmx::packuswb(low, high);
        uint64_t expected = original::packuswb(low, high);
        ASSERT_EQ(actual, expected);
    }
}

TEST(MMXEmulation, PADDSW) {

    for (size_t i = 0; i < iterations; ++i) {
        uint64_t a = random_uint64();
        uint64_t b = random_uint64();

        uint64_t actual = mmx::paddsw(a, b);
        uint64_t expected = original::paddsw(a, b);
        ASSERT_EQ(actual, expected);
    }
}

TEST(MMXEmulation, PADDW) {

    for (size_t i = 0; i < iterations; ++i) {
        uint64_t a = random_uint64();
        uint64_t b = random_uint64();

        uint64_t expected = original::paddw(a, b);
        uint64_t actual = mmx::paddw(a, b);
        EXPECT_EQ(actual, expected);
    }
}

TEST(MMXEmulation, PSUBW) {

    for (size_t i = 0; i < iterations; ++i) {
        uint64_t a = random_uint64();
        uint64_t b = random_uint64();

        uint64_t expected = original::psubw(a, b);
        uint64_t actual = mmx::psubw(a, b);
        EXPECT_EQ(actual, expected);
    }
}

TEST(MMXEmulation, PMADDWD) {

    for (size_t i = 0; i < iterations; ++i) {
        uint64_t a = random_uint64();
        uint64_t b = random_uint64();

        uint64_t expected = original::pmaddwd(a, b);
        uint64_t actual = mmx::pmaddwd(a, b);
        EXPECT_EQ(actual, expected);
    }
}

TEST(MMXEmulation, PMULHW) {

    for (size_t i = 0; i < iterations; ++i) {
        uint64_t a = random_uint64();
        uint64_t b = random_uint64();
        uint64_t expected = original::pmulhw(a, b);
        uint64_t actual = mmx::pmulhw(a, b);
        ASSERT_EQ(actual, expected);
    }
}

TEST(MMXEmulation, PMULLW) {

    for (size_t i = 0; i < iterations; ++i) {
        uint64_t a = random_uint64();
        uint64_t b = random_uint64();
        uint64_t expected = original::pmullw(a, b);
        uint64_t actual = mmx::pmullw(a, b);
        ASSERT_EQ(actual, expected);
    }
}

TEST(MMXEmulation, PADDD) {

    for (size_t i = 0; i < iterations; ++i) {
        uint64_t a = random_uint64();
        uint64_t b = random_uint64();
        uint64_t expected = original::paddd(a, b);
        uint64_t actual = mmx::paddd(a, b);
        ASSERT_EQ(actual, expected);
    }
}

TEST(MMXEmulation, PSRAD) {

    for (size_t i = 0; i < iterations; ++i) {
        uint64_t a = random_uint64();

        uint64_t expected = original::psrad_14(a);
        uint64_t actual = mmx::psrad(a, 14);
        ASSERT_EQ(actual, expected);

        expected = original::psrad_15(a);
        actual = mmx::psrad(a, 15);
        ASSERT_EQ(actual, expected);
    }
}

TEST(MMXEmulation, PSRAW) {

    for (size_t i = 0; i < iterations; ++i) {
        uint64_t a = random_uint64();
        uint64_t expected = original::psraw_16(a);
        uint64_t actual = mmx::psraw(a, 16);
        ASSERT_EQ(actual, expected);

        expected = original::psraw_4(a);
        actual = mmx::psraw(a, 4);
        ASSERT_EQ(actual, expected);
    }
}

TEST(MMXEmulation, PAND) {

    for (size_t i = 0; i < iterations; ++i) {
        uint64_t a = random_uint64();
        uint64_t b = random_uint64();
        uint64_t expected = original::pand(a, b);
        uint64_t actual = mmx::pand(a, b);
        ASSERT_EQ(actual, expected);
    }
}

TEST(MMXEmulation, PANDN) {

    for (size_t i = 0; i < iterations; ++i) {
        uint64_t a = random_uint64();
        uint64_t b = random_uint64();
        uint64_t expected = original::pandn(a, b);
        uint64_t actual = mmx::pandn(a, b);
        ASSERT_EQ(actual, expected);
    }
}

TEST(MMXEmulation, PXOR) {

    for (size_t i = 0; i < iterations; ++i) {
        uint64_t a = random_uint64();
        uint64_t b = random_uint64();
        uint64_t expected = original::pxor(a, b);
        uint64_t actual = mmx::pxor(a, b);
        ASSERT_EQ(actual, expected);
    }
}

TEST(MMXEmulation, POR) {

    for (size_t i = 0; i < iterations; ++i) {
        uint64_t a = random_uint64();
        uint64_t b = random_uint64();
        uint64_t expected = original::por(a, b);
        uint64_t actual = mmx::por(a, b);
        ASSERT_EQ(actual, expected);
    }
}

TEST(MMXEmulation, PCMPGTW) {

    for (size_t i = 0; i < iterations; ++i) {
        uint64_t a = random_uint64();
        uint64_t b = random_uint64();
        uint64_t expected = original::pcmpgtw(a, b);
        uint64_t actual = mmx::pcmpgtw(a, b);
        ASSERT_EQ(actual, expected);
    }
}

TEST(MMXEmulation, PCMPEQW) {

    for (size_t i = 0; i < iterations; ++i) {
        uint64_t a = random_uint64();
        uint64_t b = random_uint64();
        uint64_t expected = original::pcmpeqw(a, b);
        uint64_t actual = mmx::pcmpeqw(a, b);
        ASSERT_EQ(actual, expected);
    }
}
