#include "StdAfx.h"
#include "GfxBuffers.h"
#include "2DSceneSW.h"
#include "SWTexture.h"
#include "Render.h"
#include "../System/BasicShare.h"
#include "DBScene.h"
#include "RectLayout.h"
#include "GTexture.h"

namespace NGScene
{
	static CBasicShare<CDBPtr<NDb::STexture>, CSWTexture, SDBPtrHash > shareSWTextures(112);
CSWTexture* GetSWTex( const NDb::STexture *pTex ) 
{
	return shareSWTextures.Get( pTex );
}

// Structures

template<class T, class TElement>
class CSomeRender: public CArrayRasterizer<T, TElement>
{
public:
	CSomeRender(): fScale(1) {}
	bool DoRenderBackface() const { return true; }
	void SetMaskMapping( const CVec4 &_ptDU, const CVec4 &_ptDV )
	{
		maskMapping.SetGradient( _ptDU, _ptDV );
	}
	void SetTexture( CSWTextureData &tex )
	{
		pTexture = &tex;
		bDoMask = 0;
	}
	void SetMask() { bDoMask = true; }
	void SetScale( float _fScale ) { fScale = _fScale; }
	float GetScale() const { return fScale; }
	void SetColor( const NGfx::SPixel8888 &_color ) { multColor = _color; }
protected:
	STextureMapping maskMapping;
	CSWTextureData *pTexture;
	bool bDoMask;
	float fScale;
	NGfx::SPixel8888 multColor;
};

template<class T>
struct STextureIterator
{
	unsigned int nU, nDU, nV, nDV, nVShift;
	int nUMask, nVMask;
	int nMip;
	const T *pMip;

	void Init( float fU, float fV, float fDU, float fDV, const vector< CArray2D<T> > &tex )
	{
		int nXSize = tex[0].GetSizeX(), nYSize = tex[0].GetSizeY();
		nMip = 0;
		while ( ( fabs(fDU) > 1.9 || fabs(fDV) > 1.9 ) && nMip < tex.size() - 1 )
		{
			fDU /= 2; fDV /= 2; fU /= 2; fV /= 2; nMip++; nXSize /= 2; nYSize /= 2;
		}
		nU = Float2Int( fU * 0x10000 );
		nV = Float2Int( fV * 0x10000 );
		nDU = Float2Int( fDU * 0x10000 );
		nDV = Float2Int( fDV * 0x10000 );
		int nXBits = GetMSB( nXSize );
		nVShift = 16 - nXBits;
		nUMask = nXSize - 1;
		nVMask = (nYSize - 1 ) << nXBits;
		pMip = &tex[nMip][0][0];
	}
	__forceinline const T& Fetch()
	{
		const T &r = pMip[((nV>>nVShift)&nVMask) + ((nU>>16)&nUMask) ];
		nU += nDU;
		nV += nDV;
		return r;
	}
	// slow variant for special rare case
	template<class TT>
	__forceinline const TT& Fetch( const vector< CArray2D<TT> > &tex )
	{
		const TT *pData = &tex[nMip][0][0];
		const TT &r = pData[((nV>>nVShift)&nVMask) + ((nU>>16)&nUMask) ];
		nU += nDU;
		nV += nDV;
		return r;
	}
};
class CTrueColorRender;
class CTrueColorRender: public CSomeRender<CTrueColorRender, NGfx::SPixel8888>
{
public:
private:
	void RasterSpan( int nY, int nLeft, int nRight, float fZ, float fDZ, int nBackface )
	{
		STextureIterator<NGfx::SPixel8888> tex;
		float fU = texMapping.ptDU.w + nLeft * texMapping.ptDU.x + nY * texMapping.ptDU.y;
		float fV = texMapping.ptDV.w + nLeft * texMapping.ptDV.x + nY * texMapping.ptDV.y;
		tex.Init( fU, fV, texMapping.ptDU.x, texMapping.ptDV.x, pTexture->mips );
		NGfx::SPixel8888 *pRow = &res[nY][0];
		NGfx::SPixel8888 *pDst = pRow + nLeft, *pFinish = pRow + nRight;
		if ( bDoMask )
		{
			ASSERT( multColor.dwColor == 0xffffffff );
			_asm
			{
				pxor mm2, mm2
			}
			for ( ; pDst < pFinish; ++pDst )
			{
				DWORD color = tex.Fetch().dwColor;
				__asm
				{
					mov esi, pDst
					movd mm0, color
					movd mm1, [esi]
					movq mm3, mm0
					punpcklbw mm3, mm3
					punpcklbw mm0, mm2
					punpckhwd mm3, mm3
					punpcklbw mm1, mm2
					punpckhdq mm3, mm3
					paddw mm0, mm1
					psllw mm1, 1
					psrlw mm3, 1
					pmulhw mm1, mm3
					psubw mm0, mm1
					packuswb mm0, mm0
					movd [esi], mm0
				}
				//const NGfx::SPixel8888 &color = tex.Fetch();
				//NGfx::SPixel8888 &dst = *pDst;
				//int a = color.a;
				//dst.r = color.r + ( ( dst.r * ( 256 - a ) ) >> 8 );
				//dst.g = color.g + ( ( dst.g * ( 256 - a ) ) >> 8 );
				//dst.b = color.b + ( ( dst.b * ( 256 - a ) ) >> 8 );
				//dst.a = color.a + ( ( dst.a * ( 256 - a ) ) >> 8 );
			}
			__asm emms
		}
		else
		{
			if ( multColor.dwColor != 0xffffffff )
			{
				DWORD tempColor = multColor.dwColor;
				_asm
				{
					pxor mm2, mm2
					movd mm1, tempColor
					punpcklbw mm1, mm1
					psrlw mm1, 1
				}
				for ( ; pDst < pFinish; ++pDst )
				{
					DWORD color = tex.Fetch().dwColor;
					__asm
					{
						mov esi, pDst
						movd mm0, color
						punpcklbw mm0, mm2
						psllw mm0, 1
						pmulhw mm0, mm1
						packuswb mm0, mm0
						movd [esi], mm0
					}
				}
				__asm emms
			}
			else
			{
				for ( ; pDst < pFinish; ++pDst )
					*pDst = tex.Fetch();
			}
		}
	}
	friend class CRasterizer<CTrueColorRender>;
};

class CBumpRender;
class CBumpRender : public CSomeRender<CBumpRender, SBumpPixel>
{
private:
	void RasterSpan( int nY, int nLeft, int nRight, float fZ, float fDZ, int nBackface )
	{
		pTexture->PrepareBump();
		const float fDiv = 1.0f / 256;
		STextureIterator<SBumpPixel> tex;
		float fU = texMapping.ptDU.w + nLeft * texMapping.ptDU.x + nY * texMapping.ptDU.y;
		float fV = texMapping.ptDV.w + nLeft * texMapping.ptDV.x + nY * texMapping.ptDV.y;
		tex.Init( fU, fV, texMapping.ptDU.x, texMapping.ptDV.x, pTexture->bumpMips );
		if ( bDoMask )
		{
			for ( int x = nLeft; x < nRight; ++x )
			{
				const NGfx::SPixel8888 &fc = tex.Fetch( pTexture->mips );
				const SBumpPixel &color = tex.Fetch();
				SBumpPixel &dst = res[nY][x];
				int a = fc.a;
				dst.fDU = ( color.fDU * a + dst.fDU * ( 256 - a ) ) * fDiv;
				dst.fDV = ( color.fDV * a + dst.fDV * ( 256 - a ) ) * fDiv;
			}
		}
		else
		{
			for ( int x = nLeft; x < nRight; ++x )
			{
				const SBumpPixel &color = tex.Fetch();
				res[nY][x] = color;
			}
		}
	}
	friend class CRasterizer<CBumpRender>;
};

class CFogRender;
class CFogRender: public CSomeRender<CFogRender, int>
{
private:
	void RasterSpan( int nY, int nLeft, int nRight, float fZ, float fDZ, int nBackface )
	{
		const float fDiv = 1.0f / 255;
		STextureIterator<NGfx::SPixel8888> tex;
		float fU = texMapping.ptDU.w + nLeft * texMapping.ptDU.x + nY * texMapping.ptDU.y;
		float fV = texMapping.ptDV.w + nLeft * texMapping.ptDV.x + nY * texMapping.ptDV.y;
		tex.Init( fU, fV, texMapping.ptDU.x, texMapping.ptDV.x, pTexture->mips );

		for ( int x = nLeft; x < nRight; ++x )
		{
			const NGfx::SPixel8888 &color = tex.Fetch();
			res[nY][x] = Min( res[nY][x] , (int)( 0xFF - color.r ) );// ( ( res[nY][x] * color.r ) >> 8 ); 
		}
	}
	friend class CRasterizer<CFogRender>;
};

// SW Rects

class ISWRects: public CObjectBase
{
public:
	virtual void Draw( CTrueColorRender *pRes ) = 0;
	virtual void Draw( CBumpRender *pRes ) = 0;
	virtual void Draw( CFogRender *pRes ) = 0;
};

// CSWRects

static void FillTextureRect( CVec3 *vTextureRect, const CTRect<float> &r )
{
	vTextureRect[0] = CVec3( r.x1, r.y1, 0 );
	vTextureRect[1] = CVec3( r.x1, r.y2, 0 );
	vTextureRect[2] = CVec3( r.x2, r.y2, 0 );
	vTextureRect[3] = CVec3( r.x2, r.y1, 0 );
}

class CSWRects: public ISWRects
{
	OBJECT_BASIC_METHODS(CSWRects);
	ZDATA
	CDGPtr<CPtrFuncBase<CSWTextureData> > pTexture;
	CRectLayout layout;
	//float fGain;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTexture); f.Add(3,&layout); return 0; }

	template<class T, class TElement>
		void DrawRect( CSomeRender<T, TElement> *pRes )
	{
		CSWTextureData *pTextureData;
		pTexture.Refresh();
		pTextureData = pTexture->GetValue();
		for ( int nTemp = 0; nTemp < layout.rects.size(); nTemp++ )
		{
			const CRectLayout::SRect &r = layout.rects[nTemp];
			CVec3 vGeom[4], vMaskRect[4], vTextureRect[4];
			FillTextureRect( vTextureRect, r.sTex.rcTexRect );
			float fWidth = r.fSizeX;
			float fHeight = r.fSizeY;
			vGeom[0] = CVec3( r.fX, r.fY, 0 );
			vGeom[1] = CVec3( r.fX, r.fY + fHeight, 0 );
			vGeom[2] = CVec3( r.fX + fWidth, r.fY + fHeight, 0 );
			vGeom[3] = CVec3( r.fX + fWidth, r.fY, 0 );
			for ( int i = 0; i < 4; ++i )
				vGeom[i] *= pRes->GetScale();
			SGradientMatrix gm;
			PrepareGradientMatrix( &gm, vGeom[0], vGeom[1], vGeom[2] );
			CVec4 ptDU, ptDV;
			CalcGradient( gm, &ptDU, vTextureRect[0].x, vTextureRect[1].x, vTextureRect[2].x );
			CalcGradient( gm, &ptDV, vTextureRect[0].y, vTextureRect[1].y, vTextureRect[2].y );
			pRes->SetTextureMapping( ptDU, ptDV );
			pRes->SetTexture( *pTextureData );
			pRes->SetColor( r.sColor );
			pRes->RasterNoClip( vGeom[0], vGeom[1], vGeom[2] );
			pRes->RasterNoClip( vGeom[0], vGeom[2], vGeom[3] );
		}
	}
public:
	CSWRects() {}
	CSWRects( const CRectLayout &_layout, CPtrFuncBase<CSWTextureData> *_pTexture )
		: layout(_layout), pTexture(_pTexture) {}
	void Draw( CTrueColorRender *pRes ) { DrawRect( pRes ); }
	void Draw( CBumpRender *pRes ) { DrawRect( pRes ); }
	void Draw( CFogRender *pRes ) { DrawRect( pRes ); }
};

// CSWSpot

class CSWSpot: public ISWRects
{
	OBJECT_BASIC_METHODS(CSWSpot);
	ZDATA
	CDGPtr<CPtrFuncBase<CSWTextureData> > pTexture;
	float fAngle;
	CVec2 ptPos, ptSize;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTexture); f.Add(3,&fAngle); f.Add(4,&ptPos); f.Add(5,&ptSize); return 0; }

	template<class T, class TElement>
		void DrawSpot( CSomeRender<T, TElement> *pRes )
	{
		CSWTextureData *pTextureData;
		pTexture.Refresh();
		pTextureData = pTexture->GetValue();

		CVec3 vGeom[4];
		float fCos = cos( fAngle ), fSin = sin( fAngle );
		CVec3 ptRight(  ptSize.x * fCos, ptSize.x * fSin, 0 );
		CVec3 ptUp   ( -ptSize.y * fSin, ptSize.y * fCos, 0 );
		CVec3 ptCenter( ptPos, 0 );
		vGeom[0] = ptCenter;
		vGeom[1] = ptCenter + ptUp;
		vGeom[2] = ptCenter + ptUp + ptRight;
		vGeom[3] = ptCenter + ptRight;
		for ( int i = 0; i < 4; ++i )
			vGeom[i] *= pRes->GetScale();
		SGradientMatrix gm;
		PrepareGradientMatrix( &gm, vGeom[0], vGeom[1], vGeom[2] );
		CVec4 ptDU, ptDV;
		int nTexXSize = pTextureData->GetSizeX();
		int nTexYSize = pTextureData->GetSizeY();
		CalcGradient( gm, &ptDU, 0, 0, nTexXSize );
		CalcGradient( gm, &ptDV, 0, nTexYSize, nTexYSize );
		pRes->SetTextureMapping( ptDU, ptDV );
		pRes->SetTexture( *pTextureData );
		pRes->SetColor( NGfx::SPixel8888(0xff,0xff,0xff,0xff) );
		pRes->SetMask();
		pRes->RasterNoClip( vGeom[0], vGeom[1], vGeom[2] );
		pRes->RasterNoClip( vGeom[0], vGeom[2], vGeom[3] );
	}
public:
	CSWSpot() {}
	CSWSpot( const CVec2 &_ptPos, const CVec2 &_ptSize, float _fAngle,
		CPtrFuncBase<CSWTextureData> *_pTexture )
		: ptPos(_ptPos), ptSize(_ptSize), fAngle(_fAngle), pTexture(_pTexture) {}
	void Draw( CTrueColorRender *pRes ) { DrawSpot( pRes ); }
	void Draw( CBumpRender *pRes ) { DrawSpot( pRes ); }
	void Draw( CFogRender *pRes ) { DrawSpot( pRes ); }
};

class C2DPostFilter : public ISWRects
{
	OBJECT_NOCOPY_METHODS(C2DPostFilter);
	ZDATA
	CDGPtr<CPtrFuncBase<CSWTextureData> > pMask;
	int nCenterX, nCenterY;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pMask); f.Add(3,&nCenterX); f.Add(4,&nCenterY); return 0; }
	C2DPostFilter() {}
	C2DPostFilter( CPtrFuncBase<CSWTextureData> *_pMask, int _nCenterX, int _nCenterY ) : pMask(_pMask), nCenterX(_nCenterX), nCenterY(_nCenterY) {}
	void Draw( CTrueColorRender *pRes )
	{
		CSWTextureData *pTextureData;
		pMask.Refresh();
		pTextureData = pMask->GetValue();
		if ( !pTextureData || pTextureData->mips.empty() )
			return;

		const CArray2D<NGfx::SPixel8888> &mask = pTextureData->mips[0];
		if ( mask.GetSizeX() == 0 || mask.GetSizeY() == 0 )
			return;

		CVec4 vMaskCenter = NGfx::GetCVec4Color( mask[ mask.GetSizeY() / 2 ][ mask.GetSizeX() / 2 ].dwColor );

		CArray2D<NGfx::SPixel8888> &res = pRes->res;
		int nXSize = res.GetSizeX(), nYSize = res.GetSizeY();
		CArray2D<int> priorities;
		priorities.SetSizes( nXSize, nYSize );
		const int N_FILLED = 0x7fffffff;
		for ( int y = 0; y < nYSize; ++y )
		{
			for ( int x = 0; x < nXSize; ++x )
			{
				if ( res[y][x].a == 0 )
          priorities[y][x] = 0;
				else
				{
					priorities[y][x] = N_FILLED;
					if ( res[y][x].a != 255 )
					{
						CVec4 vIcon = NGfx::GetCVec4Color( res[y][x].dwColor );
						CVec4 vRes(
							vIcon.x + ( 1 - vIcon.w ) * vMaskCenter.x,
							vIcon.y + ( 1 - vIcon.w ) * vMaskCenter.y,
							vIcon.z + ( 1 - vIcon.w ) * vMaskCenter.z,
							vIcon.w + vMaskCenter.w - vIcon.w * vMaskCenter.w );
						res[y][x] = NGfx::Get8888Color( vRes );
					}
				}
			}
		}
		// filtering itself
		for ( int y = 0; y < nYSize; ++y )
		{
			for ( int x = 0; x < nXSize; ++x )
			{
				if ( priorities[y][x] == N_FILLED )
				{
					for ( int dy = 0; dy < mask.GetSizeY(); ++dy )
					{
						for ( int dx = 0; dx < mask.GetSizeX(); ++dx )
						{
							int nResX = x + dx - nCenterX, nResY = y + dy - nCenterY;
							if ( nResX < 0 || nResX >= nXSize || nResY < 0 || nResY >= nYSize )
								continue;
							int nPriority = N_FILLED - 1 - ( square( dx - nCenterX ) + square( dy - nCenterY ) );
							if ( nPriority <= priorities[ nResY ][ nResX ] )
								continue;
							priorities[ nResY ][ nResX ] = nPriority;
							res[ nResY ][ nResX ] = mask[dy][dx];
						}
					}
				}
			}
		}
	}
	void Draw( CBumpRender *pRes ) { ASSERT(0); }
	void Draw( CFogRender *pRes ) { ASSERT(0); }
};

class CGrayingFilter : public ISWRects
{
	OBJECT_NOCOPY_METHODS(CGrayingFilter);
	ZDATA
	CVec4 vConvolution;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&vConvolution); return 0; }
	CGrayingFilter() {}
	CGrayingFilter( const CVec4 &_vConv ) : vConvolution(_vConv) {}
	void Draw( CTrueColorRender *pRes )
	{
		CArray2D<NGfx::SPixel8888> &res = pRes->res;
		int nXSize = res.GetSizeX(), nYSize = res.GetSizeY();
		for ( int y = 0; y < nYSize; ++y )
		{
			for ( int x = 0; x < nXSize; ++x )
			{
				CVec4 vSrc = NGfx::GetCVec4Color( res[y][x].dwColor );
				float fAlpha = vSrc.x * vConvolution.x + vSrc.y * vConvolution.y + vSrc.z * vConvolution.z + vSrc.w * vConvolution.w;
				CVec4 vRes( fAlpha, fAlpha, fAlpha, vSrc.a );
				res[y][x] = NGfx::Get8888Color( vRes );
			}
		}
	}
	void Draw( CBumpRender *pRes ) { ASSERT(0); }
	void Draw( CFogRender *pRes ) { ASSERT(0); }
};

// Software 2D Scene

class CSW2DScene: public ISW2DScene
{
	OBJECT_BASIC_METHODS(CSW2DScene);
private:
	ZDATA
	list< CPtr<ISWRects> > rects;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&rects); return 0; }

	template<class T, class TSet>
	void DrawSet( T *pRes, TSet *pSet )
	{
		for ( TSet::iterator iTemp = pSet->begin(); iTemp != pSet->end(); )
		{
			if ( IsValid( *iTemp ) )
			{
				(*iTemp)->Draw( pRes );
				++iTemp;
			}
			else
				iTemp = pSet->erase( iTemp );
		}
	}
	template<class T>
		void InitScene( T *pRes, NGfx::CTexture *pTarget, const CTPoint<int> &vViewport )
	{
		CDynamicCast<NGfx::I2DBuffer> p2DBuffer( pTarget );
		float fScale = 1;
		int nXSize = vViewport.x, nYSize = vViewport.y;
		while ( nXSize > p2DBuffer->GetSizeX() )
		{
			nXSize /= 2; nYSize /= 2; fScale *= 0.5f;
		}
		CTRect<int> dstRegion( 0, 0, nXSize, nYSize );
		pRes->SetRegion( dstRegion );
		pRes->SetScale( fScale );
	}
public:
	CObjectBase* CreateRects( CPtrFuncBase<CSWTextureData> *pTexture, const CRectLayout &layout );
	CObjectBase* CreateSpot( CPtrFuncBase<CSWTextureData> *pTexture, const CVec2 &_ptPos, const CVec2 &_ptSize, float _fAngle );
	void AddPostFilter( CPtrFuncBase<CSWTextureData> *pTexture, int _nCenterX, int _nCenterY );
	void AddGrayingFilter( const CVec4 &vConvolution );
	void Draw( NGfx::CTexture *pTarget, const CTPoint<int> &vViewport, bool bClear );
	void DrawFog( CArray2D<int> *pFogMap, const CTPoint<int> &vViewport );
	void DrawBump( NGfx::CTexture *pTarget, const CTPoint<int> &vViewport, float fScale );
};

// Create scene

ISW2DScene* Make2DSWScene()
{
	return new CSW2DScene;
}

// CSW2DScene

CObjectBase* CSW2DScene::CreateRects( CPtrFuncBase<CSWTextureData> *pTexture, const CRectLayout &layout )
{
	if ( !pTexture )
		return 0;
	CSWRects *pRes = new CSWRects( layout, pTexture );
	rects.push_back( pRes );
	return pRes;
}

CObjectBase* CSW2DScene::CreateSpot( CPtrFuncBase<CSWTextureData> *pTexture,
	const CVec2 &_ptPos, const CVec2 &_ptSize, float _fAngle )
{
	if ( !pTexture )
		return 0;
	CSWSpot *pSpot = new CSWSpot( _ptPos, _ptSize, _fAngle, pTexture );
	rects.push_back( pSpot );
	return pSpot;
}

void CSW2DScene::AddPostFilter( CPtrFuncBase<CSWTextureData> *pTexture, int _nCenterX, int _nCenterY )
{
	if ( !pTexture )
		return;
	rects.push_back( new C2DPostFilter( pTexture, _nCenterX, _nCenterY ) );
}

void CSW2DScene::AddGrayingFilter( const CVec4 &vConvolution )
{
	rects.push_back( new CGrayingFilter( vConvolution ) );
}

#pragma warning( disable : 4799 )
static void StartHiColorConvert4444()
{
	DWORD mask1 = 0x00f000f0;
	DWORD mask2 = 0xf000f000;
	DWORD rAdd = 0x08080808;
	_asm
	{
		movd mm4, mask1
		movd mm5, mask2
		movd mm7, rAdd
		punpckldq mm4, mm4
		punpckldq mm5, mm5
		punpckldq mm7, mm7
	}
}

static void ConvertToHiColor4444( void *pDst, void *pSrc, int nSize )
{
	ASSERT( ( nSize & 1 ) == 0 && nSize > 0 );
	nSize /= 2;
	_asm
	{
		mov ecx, nSize
		mov esi, pSrc
		mov edi, pDst
lp:
		movq mm0, [esi]
		paddusb mm0, mm7
		movq mm2, mm0
		movq mm1, mm0
		pand mm2, mm4
		pand mm1, mm5
		psrlq mm2, 4
		psrlq mm1, 8
		por mm2, mm1
		packuswb mm2, mm2
		movd [edi], mm2
		add esi, 8
		add edi, 4
		dec ecx
		jnz lp
	}
}

static void CalcMip( CArray2D<NGfx::SPixel8888> *pRes )
{
	CArray2D<NGfx::SPixel8888> src = *pRes;
	int nXSize = pRes->GetSizeX() / 2;
	int nYSize = pRes->GetSizeY() / 2;
	pRes->SetSizes( nXSize, nYSize );
	for ( int y = 0; y < nYSize; ++y )
	{
		for ( int x = 0; x < nXSize; ++x )
		{
			const NGfx::SPixel8888 &s = src[y*2][x*2];
			const NGfx::SPixel8888 &s1 = src[y*2+1][x*2];
			const NGfx::SPixel8888 &s2 = src[y*2][x*2+1];
			const NGfx::SPixel8888 &s3 = src[y*2+1][x*2+1];
			(*pRes)[y][x] = NGfx::SPixel8888( 
				(int(s.r) + s1.r + s2.r + s3.r)>>2,
				(int(s.g) + s1.g + s2.g + s3.g)>>2,
				(int(s.b) + s1.b + s2.b + s3.b)>>2, 0 );
		}
	}
}

void CSW2DScene::Draw( NGfx::CTexture *pTarget, const CTPoint<int> &vViewport, bool bClear )
{
	CTrueColorRender r;
	InitScene( &r, pTarget, vViewport );
	if ( bClear )
		r.res.FillZero();
	DrawSet( &r, &rects );
	//
	CDynamicCast<NGfx::I2DBuffer> p2DBuffer( pTarget );
	for ( int nMip = 0; nMip < p2DBuffer->GetNumMipLevels(); ++nMip )
	{
		if ( p2DBuffer->GetPixelID() == NGfx::SPixel8888::ID )
		{
			NGfx::CTextureLock<NGfx::SPixel8888> lock( pTarget, nMip, NGfx::INPLACE );
			for ( int y = 0; y < lock.GetSizeY(); ++y )
			{
				for ( int x = 0; x < lock.GetSizeX(); ++x )
					lock[y][x] = r.res[y][x];
			}
		}
		else
		{
			NGfx::CTextureLock<NGfx::SPixel4444> lock( pTarget, nMip, NGfx::INPLACE );
			StartHiColorConvert4444();
			int nSizeX = lock.GetSizeX();
			vector<NGfx::SPixel4444> colorBuf;
			vector<NGfx::SPixel8888> srcColorBuf;
			for ( int y = 0; y < lock.GetSizeY(); ++y )
			{
				if ( nSizeX & 1 )
				{
					colorBuf.resize( nSizeX + 1 );
					srcColorBuf.resize( nSizeX + 1 );
					memcpy( &srcColorBuf[0], &r.res[y][0], nSizeX * sizeof(srcColorBuf[0]) );
					ConvertToHiColor4444( &colorBuf[0], &srcColorBuf[0], nSizeX + 1 );
					memcpy( &lock[y][0], &colorBuf[0], nSizeX * sizeof(colorBuf[0]) );
				}
				else
					ConvertToHiColor4444( &lock[y][0], &r.res[y][0], lock.GetSizeX() );
			}
			_asm emms
		}
		CalcMip( &r.res );
	}
}

void CSW2DScene::DrawFog( CArray2D<int> *pFogMap, const CTPoint<int> &vViewport )
{
	CFogRender r;

	float fScale = 1;
	CTRect<int> dstRegion( 0, 0, pFogMap->GetSizeX(), pFogMap->GetSizeY() );
	r.SetRegion( dstRegion );
	r.SetScale( fScale );
	r.res = (*pFogMap);
	DrawSet( &r, &rects );

	for ( int y = 0; y < pFogMap->GetSizeY(); ++y )
	{
		for ( int x = 0; x < pFogMap->GetSizeX(); ++x )
			(*pFogMap)[y][x] = r.res[y][x];
	}
}

static void CalcMip( CArray2D<SBumpPixel> *pRes )
{
	CArray2D<SBumpPixel> src = *pRes;
	int nXSize = pRes->GetSizeX() / 2;
	int nYSize = pRes->GetSizeY() / 2;
	pRes->SetSizes( nXSize, nYSize );
	for ( int y = 0; y < nYSize; ++y )
	{
		for ( int x = 0; x < nXSize; ++x )
		{
			(*pRes)[y][x].fDU = 0.25f * ( src[y*2][x*2].fDU + src[y*2+1][x*2].fDU + src[y*2][x*2+1].fDU + src[y*2+1][x*2+1].fDU );
			(*pRes)[y][x].fDV = 0.25f * ( src[y*2][x*2].fDV + src[y*2+1][x*2].fDV + src[y*2][x*2+1].fDV + src[y*2+1][x*2+1].fDV );
		}
	}
}

static void CalcNormals( const CArray2D<SBumpPixel> &m, NGfx::CTexture *pTarget, int nMip, float fScale )
{
	CDynamicCast<NGfx::I2DBuffer> p2DBuffer( pTarget );
	if ( p2DBuffer->GetPixelID() == NGfx::SPixel8888::ID )
	{
		NGfx::CTextureLock<NGfx::SPixel8888> lock( pTarget, nMip, NGfx::INPLACE );
		ASSERT( m.GetSizeX() == lock.GetSizeX() && m.GetSizeY() == lock.GetSizeY() );
		for ( int y = 0; y < m.GetSizeY(); ++y )
		{
			for ( int x = 0; x < m.GetSizeX(); ++x )
			{
				CVec3 vNormal( m[y][x].fDU, m[y][x].fDV, 1 );
				Normalize( &vNormal );
				lock[y][x] = NGfx::SPixel8888( 
					Float2Int( vNormal.x * 127 + 128 ), 
					Float2Int( vNormal.y * 127 + 128 ), 
					Float2Int( vNormal.z * 127 + 128 ), 255 );
			}
		}
	}
	else
	{
		NGfx::CTextureLock<NGfx::SPixel565> lock( pTarget, nMip, NGfx::INPLACE );
		ASSERT( m.GetSizeX() == lock.GetSizeX() && m.GetSizeY() == lock.GetSizeY() );
		for ( int y = 0; y < m.GetSizeY(); ++y )
		{
			for ( int x = 0; x < m.GetSizeX(); ++x )
			{
				CVec3 vNormal( m[y][x].fDU, m[y][x].fDV, 1 );
				Normalize( &vNormal );
				lock[y][x] = NGfx::SPixel565( 
					Float2Int( vNormal.x * 127 + 128 ) >> 3, 
					Float2Int( vNormal.y * 127 + 128 ) >> 2, 
					Float2Int( vNormal.z * 127 + 128 ) >> 3 );
			}
		}
	}
}

void CSW2DScene::DrawBump( NGfx::CTexture *pTarget, const CTPoint<int> &vViewport, float _fBumpScale )
{
	float fBumpScale = _fBumpScale;
	CBumpRender r;
	InitScene( &r, pTarget, vViewport );
	DrawSet( &r, &rects );
	fBumpScale *= r.GetScale();
	//
	CDynamicCast<NGfx::I2DBuffer> p2DBuffer( pTarget );
	for ( int nMip = 0; nMip < p2DBuffer->GetNumMipLevels(); ++nMip )
	{
		CalcNormals( r.res, pTarget, nMip, fBumpScale );
		CalcMip( &r.res );
		fBumpScale /= 2;
	}
}

} // NAMESPACE

using namespace NGScene;
BASIC_REGISTER_CLASS( ISWRects );
REGISTER_SAVELOAD_CLASS( 0xF3005171, CSWRects )
REGISTER_SAVELOAD_CLASS( 0xF3005172, CSW2DScene )
REGISTER_SAVELOAD_CLASS( 0xF3005173, CSWSpot )
REGISTER_SAVELOAD_CLASS( 0x20146400, C2DPostFilter )
REGISTER_SAVELOAD_CLASS( 0x20172C80, CGrayingFilter )

