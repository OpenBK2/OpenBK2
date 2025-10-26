#pragma once

#include <cmath>
#include <random>

#include <boost/math/constants/constants.hpp>

static uint64_t random_int() {
    static std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution dist;
    return dist(rng);
}

static uint64_t random_uint64() {
    static std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<uint64_t> dist;
    return dist(rng);
}

static uint16_t random_uint16() {
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution dist(-32768, 32767);
    return static_cast<uint16_t>(dist(rng));
}

static uint32_t random_uint32() {
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<uint32_t> dist;
    return static_cast<uint32_t>(dist(rng));
}

static uint8_t random_uint8() {
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution dist(0, 255);
    return static_cast<uint8_t>(dist(rng));
}

static bool random_bool() {
    static std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution dist(0, 1);
    return dist(rng) ? true :false;
}

static float random_float() {
    static std::mt19937_64 rng{std::random_device{}()};
    std::uniform_real_distribution dist(-1.0f, 1.0f);
    return dist(rng);
}

template<typename T>
static void random_mmx_word(T & word) {
    word.nX = random_uint16();
    word.nY = random_uint16();
    word.nZ = random_uint16();
    word.nW = random_uint16();
}

template<typename T>
static void randomizeVector(T &vec) {
    vec.x = random_uint8();
    vec.y = random_uint8();
    vec.z = random_uint8();
    vec.w = random_uint8();
}

template<typename T>
static void randomizeNormalVector(T &vec) {
    // Spherical coordinates - guaranteed non-zero
    const float theta = (random_float() + 1.0f) * boost::math::constants::pi<float>(); // [0, 2π]
    const float u = (random_float() + 1.0f) * 0.5f;
    const float phi = acosf(2.0f * u - 1.0f); // [0, π]

    const float x = sinf(phi) * cosf(theta);
    const float y = sinf(phi) * sinf(theta);
    const float z = cosf(phi);

    vec.x = static_cast<uint8_t>(std::clamp(x * 127.0f + 128.0f, 0.0f, 255.0f));
    vec.y = static_cast<uint8_t>(std::clamp(y * 127.0f + 128.0f, 0.0f, 255.0f));
    vec.z = static_cast<uint8_t>(std::clamp(z * 127.0f + 128.0f, 0.0f, 255.0f));
    vec.w = random_uint8();
}

template<typename T>
static void randomizeFixup(T &fix) {

    random_mmx_word(fix.normalFixup);
    random_mmx_word(fix.shiftedFixup);
}

template<typename T>
static void randomizeTransformer(T &t) {

    random_mmx_word(t.a);
    random_mmx_word(t.b);
    random_mmx_word(t.c);
    t.a.nW = t.b.nW = t.c.nW = 0;
}

template<typename T>
static void randomizeMatrix(T &m) {

    // Generate first random unit vector using spherical coordinates
    float theta1 = (random_float() + 1.0f) * boost::math::constants::pi<float>(); // [0, 2π]
    float u1 = (random_float() + 1.0f) * 0.5f; // [0, 1]
    float phi1 = acosf(2.0f * u1 - 1.0f); // [0, π]

    float x1 = sinf(phi1) * cosf(theta1);
    float y1 = sinf(phi1) * sinf(theta1);
    float z1 = cosf(phi1);

    // Generate second vector by rotating first vector 90 degrees around Z
    // then applying the same spherical coordinates but with offset
    float theta2 = theta1 + 1.57079633f; // +90 degrees
    float phi2 = phi1; // Same inclination

    float x2 = sinf(phi2) * cosf(theta2);
    float y2 = sinf(phi2) * sinf(theta2);
    float z2 = cosf(phi2);

    // Ensure orthogonality by Gram-Schmidt
    float dot = x1*x2 + y1*y2 + z1*z2;
    x2 -= dot * x1;
    y2 -= dot * y1;
    z2 -= dot * z1;

    // Normalize second vector
    float len2 = sqrtf(x2*x2 + y2*y2 + z2*z2);
    x2 /= len2; y2 /= len2; z2 /= len2;

    // Third vector is cross product
    float x3 = y1*z2 - z1*y2;
    float y3 = z1*x2 - x1*z2;
    float z3 = x1*y2 - y1*x2;

    // Build orthogonal matrix
    m._11 = x1; m._12 = y1; m._13 = z1; m._14 = 0;
    m._21 = x2; m._22 = y2; m._23 = z2; m._24 = 0;
    m._31 = x3; m._32 = y3; m._33 = z3; m._34 = 0;
    m._41 = 0;  m._42 = 0;  m._43 = 0;  m._44 = 1;
}
