#pragma once

#include "3Dmotor_export.h"

namespace NGfx
{
struct SPerformanceInfo
{
	float fPSRate, fFillRate, fTriangleRate, fCPUclock; // in millions
};
void PerformBenchmark();
_3DMOTOR_EXPORT const SPerformanceInfo &GetPerformanceInfo();
}

