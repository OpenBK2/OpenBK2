#pragma once

#include "..\3DMotor\GfxBuffers.h"

template<class T> class CPtrFuncBase;
namespace NGfx
{
	class CTexture;
}
namespace NGScene
{
	class CFileTexture;
}
namespace NDb
{
	struct STexture;
}

interface IUIVisitor
{
	virtual void ClipSet( const CTRect<float> &rClip ) = 0;
	virtual void ClipRestore() = 0;
	//
	virtual void SetLowerLevel() {}
	virtual void SetUpperLevel() {}
	//
	virtual void VisitZClearRect( const CRectLayout &sLayout, const CTPoint<float> &sPosition, const CTRect<float> &sClipWindow, float fZ=1.0f ) = 0;
	virtual void VisitUIRect( const NDb::STexture *pTexture, const int nShadingEffect, const class CRectLayout &rect ) = 0;
  virtual void VisitUIRect( const NDb::STexture *pTexture, const int nShadingEffect, 
		const CVec2 *pPos4, const NGfx::SPixel8888 *pColors4, const CTRect<float> &rectTexture ) = 0;
	virtual void VisitUITextureRect( CPtrFuncBase<NGfx::CTexture> *pTexture, const int nShadingEffect, const class CRectLayout &rects ) = 0;
	virtual void VisitUIText( interface IML *pML, const CTPoint<float> &sPosition, const CTRect<float> &sWindow ) = 0;
	//virtual void VisitUIText( interface IGFXText *pText, const CTRect<float> &rcRect, const int nY, const DWORD dwColor, const DWORD dwFlags ) = 0;
	//virtual void VisitStringW( const wstring &szString, const int nFont, const int nX, const int nY, const DWORD dwColor = 0xFFFFFFFF ) = 0;
	friend class CClipStore;
};

// helps to store clip
class CClipStore
{
	IUIVisitor *pVisitor;
public:
	CClipStore( IUIVisitor * _pVisitor, const CTRect<float> &rNewClip ) 
		: pVisitor( _pVisitor ) 
	{ 
		pVisitor->ClipSet( rNewClip ); 
	}
	~CClipStore() { pVisitor->ClipRestore(); }
};

