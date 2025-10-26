#pragma once

#include <random>

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
    vec.z = random_uint8();
    vec.y = random_uint8();
    vec.x = random_uint8();
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
static void randomizeMatrix(T &t) {

    t.xx = random_float();
    t.xy = random_float();
    t.xz = random_float();
    t.xw = 0.0f;

    t.yx = random_float();
    t.yy = random_float();
    t.yz = random_float();
    t.yw = 0.0f;

    t.zx = random_float();
    t.zy = random_float();
    t.zz = random_float();
    t.zw = 0.0f;

    t.wx = 0.0f;
    t.wy = 0.0f;
    t.wz = 0.0f;
    t.ww = 0.0f;
}
