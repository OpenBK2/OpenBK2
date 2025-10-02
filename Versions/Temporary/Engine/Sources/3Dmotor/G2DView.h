#pragma once

#include "3Dmotor_export.h"


#include "../System/DG.h"
#include "GPixelFormat.h"
#include "GRenderModes.h"

class CRectLayout;
namespace NGfx
{
	class CTexture;
}
namespace NDb
{
	struct STexture;
};

namespace NGScene
{

class CTextLocaleInfo;
struct STextureKey;

interface ILayoutFakeView
{
public:
	virtual void CreateDynamicRects( const NDb::STexture *pTexture, const CRectLayout &sLayout, const CTPoint<float> &sPosition, const CTRect<float> &sWindow ) = 0;
	virtual void CreateDynamicRects( const NDb::STexture *pTexture, 
		const CVec2 *pPos4, const NGfx::SPixel8888 *pColors4, const CTRect<float> &rectTexture ) = 0;
	virtual void CreateDynamicRects( CPtrFuncBase<NGfx::CTexture> *pTexture, const CRectLayout &sLayout, const CTPoint<float> &sPosition, const CTRect<float> &sWindow ) = 0;
};

class I2DGameView: public CObjectBase, public ILayoutFakeView 
{
public:
	virtual void CreateDynamicClearRects( const CRectLayout &sLayout, const CTPoint<float> &sPosition, const CTRect<float> &sClipWindow ) = 0;

	virtual CObjectBase* CreateTexture( const NGScene::STextureKey &key ) = 0;

	virtual CVec2 GetViewportSize() = 0;
	virtual CTextLocaleInfo* GetLocaleInfo() const = 0;

	virtual void StartNewFrame( NGfx::CTexture *pTarget ) = 0;
	virtual void StartNewFrame( EAlphaMode2D _AlphaMode2D ) = 0;
	virtual void Flush() = 0;
	virtual void SetWindowSize( const CVec2 &vSize ) = 0;
};

_3DMOTOR_EXPORT I2DGameView* CreateNew2DView();

_3DMOTOR_EXPORT bool Is3DActive();
_3DMOTOR_EXPORT void SetWireframe( bool bWire );
void SetShowSceneInfo( bool bShow );
_3DMOTOR_EXPORT void Flip();
_3DMOTOR_EXPORT void ClearScreen( const CVec3 &vColor );

}

