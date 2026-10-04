#pragma once

// Defensive member defaults; explicit constructor values still take precedence.
#include <cstdint>

// Offline model processing can combine several GLTF meshes and exceed 16-bit indices.
struct SModelTriangle
{
	uint32_t i1 = 0, i2 = 0, i3 = 0;
	SModelTriangle(uint32_t a, uint32_t b, uint32_t c) : i1(a), i2(b), i3(c) {}
};
