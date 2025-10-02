#pragma once
#include "../System/DG.h"
#include "GPixelFormat.h"

namespace NGScene
{

class CSWRectLayout
{
public:
	enum ERectOrient
	{
		NORMAL,
		ROTATE_90,
		ROTATE_180,
		ROTATE_270
	};
	struct STextureCoord
	{
		ERectOrient eOrient;
		CTRect<short> rcTexRect;

		STextureCoord() {}
		STextureCoord( const CTRect<short> &_rcTexRect, ERectOrient _eOrient = NORMAL ): eOrient( _eOrient ), rcTexRect( _rcTexRect ) {}
		bool IsSwap() const { return eOrient == ROTATE_90 || eOrient == ROTATE_270; }
		int GetWidth() const { return IsSwap() ? rcTexRect.Height() : rcTexRect.Width(); }
		int GetHeight() const { return IsSwap() ? rcTexRect.Width() : rcTexRect.Height(); }
	};
	struct SRect
	{
		short nX, nY;
		STextureCoord sTex, sMask;
		NGfx::SPixel8888 sColor;

		SRect() {}
		SRect( short _nX, short _nY, const STextureCoord &_sTex, const STextureCoord &_sMask, const NGfx::SPixel8888 &_sColor )
			:nX(_nX), nY(_nY), sTex(_sTex), sMask(_sMask), sColor(_sColor) {}
	};
	//
	CTPoint<float> scale;
	vector<SRect> rects;
	//
	CSWRectLayout(): scale( 1, 1 ) {}
	void AddRect( short nScreenX, short nScreenY, const STextureCoord &sTex, const NGfx::SPixel8888 &color = NGfx::SPixel8888( 0xFF, 0xFF, 0xFF, 0xFF ) )
	{
		rects.push_back( SRect( nScreenX, nScreenY, sTex, sTex, color ) );
	}
	void AddRect( short nScreenX, short nScreenY, const STextureCoord &sTex, const STextureCoord &sMask, const NGfx::SPixel8888 &color = NGfx::SPixel8888( 0xFF, 0xFF, 0xFF, 0xFF ) )
	{
		rects.push_back( SRect( nScreenX, nScreenY, sTex, sMask, color ) );
	}
	int operator&( CStructureSaver &f )
	{
		f.Add( 1, &scale );
		f.Add( 2, &rects );
		return 0;
	}
};

// standard point order: ld, lu, ru, rd
template<class T>
inline void ApplyRectOrient( CSWRectLayout::ERectOrient eOrient, const T &ld, const T &ru, T *pResult )
{
	switch( eOrient )
	{
	case CSWRectLayout::NORMAL:
		pResult[0].x = ld.x;
		pResult[0].y = ld.y;
		pResult[1].x = ld.x;
		pResult[1].y = ru.y;
		pResult[2].x = ru.x;
		pResult[2].y = ru.y;
		pResult[3].x = ru.x;
		pResult[3].y = ld.y;
		break;
	case CSWRectLayout::ROTATE_90:
		pResult[0].x = ru.x;
		pResult[0].y = ld.y;
		pResult[1].x = ld.x;
		pResult[1].y = ld.y;
		pResult[2].x = ld.x;
		pResult[2].y = ru.y;
		pResult[3].x = ru.x;
		pResult[3].y = ru.y;
		break;
	case CSWRectLayout::ROTATE_180:
		pResult[0].x = ru.x;
		pResult[0].y = ru.y;
		pResult[1].x = ru.x;
		pResult[1].y = ld.y;
		pResult[2].x = ld.x;
		pResult[2].y = ld.y;
		pResult[3].x = ld.x;
		pResult[3].y = ru.y;
		break;
	case CSWRectLayout::ROTATE_270:
		pResult[0].x = ld.x;
		pResult[0].y = ru.y;
		pResult[1].x = ru.x;
		pResult[1].y = ru.y;
		pResult[2].x = ru.x;
		pResult[2].y = ld.y;
		pResult[3].x = ld.x;
		pResult[3].y = ld.y;
		break;
	default:
		ASSERT( 0 );
		break;
	}
}

template<class T, class TRes>
inline void ApplyRectOrient( CSWRectLayout::ERectOrient eOrient, const CTRect<T> &r, TRes *pResult )
{
	TRes ld, ru;
	ld.x = r.x1; ld.y = r.y1;
	ru.x = r.x2; ru.y = r.y2;
	ApplyRectOrient( eOrient, ld, ru, pResult );
}

DEFINE_DG_CONSTANT_NODE( CCSWRectLayout, CSWRectLayout );
#ifdef STUPID_VISUAL_ASSIST
class CCSWRectLayout;
#endif
}


