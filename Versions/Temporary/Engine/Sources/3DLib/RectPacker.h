#pragma once
namespace NRectPacker
{
struct SRect
{
	WORD nXShift, nYShift;
	WORD nXSize, nYSize;
};
void PackRects( std::vector<SRect> *pRes, CTPoint<int> *pSize );

}

