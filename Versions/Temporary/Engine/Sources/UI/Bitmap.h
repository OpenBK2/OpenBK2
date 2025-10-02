#ifndef __BITMAP_H__
#define __BITMAP_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "..\3DMotor\GfxBuffers.h"
#include "..\Misc\2Darray.h"
#include "..\System\DG.h"


class CBitmap :  public CPtrFuncBase< NGfx::CTexture >
{
	OBJECT_BASIC_METHODS(CBitmap);
	CArray2D<NGfx::SPixel8888> points;
	int nWidth, nHeight;
	bool bNeedUpdate;

protected:
	bool NeedUpdate() { return bNeedUpdate; }
	void Recalc();

public:
	void SetSize( const int nWidth, const int nHeight );
	int GetWidth() const { return nWidth; }
	int GetHeight() const { return nHeight; }
	void PutPixel( const int x, const int y, const NGfx::SPixel8888 &color );
	const NGfx::SPixel8888 GetPixel( const int x, const int y ) const;
	void Fill( const NGfx::SPixel8888 &color );

	int operator&( IBinSaver &saver );
};

class CBitmapWindow : public CObjectBase
{
	OBJECT_BASIC_METHODS(CBitmapWindow);
	CObj< CBitmap > pBitmap;
	CVec2 vPos;
	CVec2 vSize;

public:
	virtual void Visit( interface IUIVisitor * pVisitor );
	virtual int operator&( interface IBinSaver &saver );

	const CVec2 GetPos() const { return vPos; }
	void SetPos( const CVec2 &vPos );
	const CVec2 GetSize() const { return vSize; }
	void SetSize( const CVec2 &vSize );
	
	void PutPixel( const CVec2 &vPos, const NGfx::SPixel8888 &color );
	const NGfx::SPixel8888 GetPixel( const CVec2 &vPos ) const;
	void Fill( const NGfx::SPixel8888 &color );

};


#endif //__BITMAP_H__
