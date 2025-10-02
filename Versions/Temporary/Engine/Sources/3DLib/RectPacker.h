#pragma once
namespace NRectPacker
{
struct SRect
{
	WORD nXShift, nYShift;
	WORD nXSize, nYSize;
};
void PackRects( vector<SRect> *pRes, CTPoint<int> *pSize );

}

