#pragma once

#include <cstdint>

namespace NRectPacker
{
struct SRect
{
	uint16_t nXShift, nYShift;
	uint16_t nXSize, nYSize;
};
void PackRects( std::vector<SRect> *pRes, CTPoint<int> *pSize );

}
