#ifndef __RECTPACKER_H__
#define __RECTPACKER_H__
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

namespace NRectPacker
{
struct SRect
{
	WORD nXShift, nYShift;
	WORD nXSize, nYSize;
};
void PackRects( vector<SRect> *pRes, CTPoint<int> *pSize );

}
#endif

