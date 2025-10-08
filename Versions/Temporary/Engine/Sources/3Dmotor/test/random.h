#pragma once

#include <random>

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
