#ifndef __GS2DCENE_H_
#define __GS2DCENE_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "..\System\DG.h"
#include "RectLayout.h"
namespace NGfx
{
	class CTexture;
	CVec2 GetScreenRect();
}
namespace NGScene
{

class I2DScene: public CObjectBase
{
public:
	virtual void CreateDynamicRects( CPtrFuncBase<NGfx::CTexture> *pTexture, const CRectLayout &sLayout, const CTPoint<float> &sPosition, const CTRect<float> &sClipWindow ) = 0;
	virtual void CreateDynamicRects( CPtrFuncBase<NGfx::CTexture> *pTexture, 
		const CVec2 *pPos4, const NGfx::SPixel8888 *pColors4, const CTRect<float> &rectTexture ) = 0;
	virtual void CreateDynamicClearRects( const CRectLayout &sLayout, const CTPoint<float> &sPosition, const CTRect<float> &sClipWindow ) = 0;

	virtual void StartNewFrame( NGfx::CTexture *pTarget, const CVec2 &vSize, enum EAlphaMode2D _AlphaMode2D ) = 0;
	virtual void Flush() = 0;
};

I2DScene* Make2DScene();

}

#endif

