#ifndef __RECTLAYOUT_H_
#define __RECTLAYOUT_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "GPixelFormat.h"


class CRectLayout
{
public:
	struct STextureCoord
	{
		ZDATA
		CTRect<float> rcTexRect;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&rcTexRect); return 0; }

		STextureCoord() {}
		STextureCoord( const CTRect<float> &_rcTexRect ): rcTexRect( _rcTexRect ) {}
		STextureCoord( float fx, float fy, float fw, float fh )
		{
			rcTexRect.x1 = fx;
			rcTexRect.y1 = fy;
			rcTexRect.x2 = fx + fw;
			rcTexRect.y2 = fy + fh;
		}
		int GetWidth() const { return rcTexRect.Width(); }
		int GetHeight() const { return rcTexRect.Height(); }
	};
	struct SRect
	{
		ZDATA
		float fX, fY;
		float fSizeX, fSizeY;
		STextureCoord sTex;
		NGfx::SPixel8888 sColor;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&fX); f.Add(3,&fY); f.Add(4,&fSizeX); f.Add(5,&fSizeY); f.Add(6,&sTex); f.Add(7,&sColor); return 0; }

		SRect() {}
		SRect( float _fX, float _fY, float _fSizeX, float _fSizeY, const STextureCoord &_sTex, const NGfx::SPixel8888 &_sColor )
			:fX(_fX), fY(_fY), fSizeX( _fSizeX ), fSizeY( _fSizeY ), sTex(_sTex), sColor(_sColor) {}
	};
	ZDATA
	vector<SRect> rects;
	float fZ;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&rects); f.Add(3,&fZ); return 0; }
	//
	CRectLayout() :fZ(0) {}
	CRectLayout ( float fScrX, float fScrY, float fScrW, float fScrH, float fTxtX, float fTxtY, float fTxtW, float fTxtH, DWORD dwColor )
	{
		AddRect( fScrX, fScrY, fScrW, fScrH, STextureCoord( fTxtX, fTxtY, fTxtW, fTxtH ), dwColor );
		fZ = 0;
	}
	void AddRect( float fX, float fY, float fSizeX, float fSizeY, const STextureCoord &sTex = CTRect<float>( 0, 0, 0, 0 ), const NGfx::SPixel8888 &color = NGfx::SPixel8888( 0xFF, 0xFF, 0xFF, 0xFF ) )
	{
		rects.push_back( SRect( fX, fY, fSizeX, fSizeY, sTex, color ) );
	}
	void AddRect( const SRect &rect )
	{
		rects.push_back( rect );
	}
	void RemoveAllRects()
	{
		rects.clear();
	}
};

inline CRectLayout MakeLayout( float fScrX, float fScrY, float fScrW, float fScrH, float fTxtX, float fTxtY, float fTxtW, float fTxtH, DWORD dwColor = 0xFFFFFFFF )
{
	return CRectLayout( fScrX, fScrY, fScrW, fScrH, fTxtX, fTxtY, fTxtW, fTxtH, dwColor );
}

#endif
