#include "stdafx.h"

#include "3Dmotor/RectLayout.h"
#include "BackgroundVectorImage.h"
#include "System/Streams.h"
#include "System/VFSOperations.h"
#include "UIVisitor.h"

#include <cmath>
#include <vector>

// The only translation unit that includes lunasvg, so replacing the library
// later touches this file alone.
#include <lunasvg.h>

REGISTER_SAVELOAD_CLASS( UI, 0x1F1A6C00, CBackgroundVectorImage )

// CBackgroundVectorImage::CRaster

const lunasvg::Document *CBackgroundVectorImage::CRaster::GetDocument()
{
	if ( bLoaded )
	{
		return pDocument.get();
	}
	// Tried once: a file that is missing or does not parse is reported once and
	// then drawn as nothing, not looked for again every frame.
	bLoaded = true;
	CFileStream stream( NVFS::GetMainVFS(), szFileName );
	if ( !stream.IsOk() || stream.GetSize() <= 0 )
	{
		csSystem << "CBackgroundVectorImage: cannot open " << szFileName << "\n";
		return nullptr;
	}
	std::vector<char> data( stream.GetSize() );
	stream.Read( data.data(), static_cast<int>( data.size() ) );
	pDocument = lunasvg::Document::loadFromData( data.data(), data.size() );
	if ( !pDocument || pDocument->width() <= 0 || pDocument->height() <= 0 )
	{
		csSystem << "CBackgroundVectorImage: cannot parse " << szFileName << "\n";
		pDocument.reset();
	}
	return pDocument.get();
}

float CBackgroundVectorImage::CRaster::GetAspect()
{
	const lunasvg::Document *pDoc = GetDocument();
	return pDoc ? pDoc->width() / pDoc->height() : 0.0f;
}

void CBackgroundVectorImage::CRaster::SetSize( int nX, int nY )
{
	if ( nX != nSizeX || nY != nSizeY )
	{
		nSizeX = nX;
		nSizeY = nY;
		bNeedUpdate = true;
	}
}

void CBackgroundVectorImage::CRaster::Recalc()
{
	bNeedUpdate = false;
	const lunasvg::Document *pDoc = GetDocument();
	const int nX = (std::max)( nSizeX, 1 );
	const int nY = (std::max)( nSizeY, 1 );
	// TEXTURE_2D goes into the shared 2D texture cache, which holds A8R8G8B8, or
	// A4R4G4B4 when the user asked for 16-bit textures (NGfx::MakeTexture asserts
	// which), so both are written below.
	const bool b16Bit = NGfx::Is16BitTextures();
	pValue =
			NGfx::MakeTexture( nX, nY, 1, b16Bit ? NGfx::CF_A4R4G4B4 : NGfx::CF_A8R8G8B8, NGfx::TEXTURE_2D, NGfx::CLAMP );
	if ( !IsValid( pValue ) )
	{
		return;
	}

	// Rendered onto transparency: where the image does not reach, the window
	// behind it shows through. A missing document leaves the texture transparent.
	lunasvg::Bitmap bitmap;
	if ( pDoc && nSizeX > 0 && nSizeY > 0 )
	{
		bitmap = pDoc->renderToBitmap( nX, nY, 0x00000000 );
	}
	const bool bHasImage = !bitmap.isNull() && bitmap.width() == nX && bitmap.height() == nY;

	// lunasvg's ARGB32 is a native 32-bit 0xAARRGGBB per pixel, the same value
	// SPixel8888::dwColor holds, but premultiplied by alpha, and the 2D renderer
	// blends straight alpha. Opaque pixels are unchanged; the antialiased edge
	// pixels have their colour divided back out, rounding to nearest.
	const auto StraightPixel = [&]( int x, int y ) -> NGfx::SPixel8888
	{
		if ( !bHasImage )
		{
			return NGfx::SPixel8888( 0, 0, 0, 0 );
		}
		const uint32_t dw =
				reinterpret_cast<const uint32_t *>( bitmap.data() + static_cast<size_t>( y ) * bitmap.stride() )[x];
		const uint32_t a = dw >> 24;
		uint32_t r = ( dw >> 16 ) & 0xFF;
		uint32_t g = ( dw >> 8 ) & 0xFF;
		uint32_t b = dw & 0xFF;
		if ( a != 0 && a != 0xFF )
		{
			r = (std::min)( 0xFFu, ( r * 255 + a / 2 ) / a );
			g = (std::min)( 0xFFu, ( g * 255 + a / 2 ) / a );
			b = (std::min)( 0xFFu, ( b * 255 + a / 2 ) / a );
		}
		return NGfx::SPixel8888( r, g, b, a );
	};

	if ( b16Bit )
	{
		NGfx::CTextureLock<NGfx::SPixel4444> lock( pValue, 0, NGfx::INPLACE );
		for ( int y = 0; y < nY; ++y )
		{
			for ( int x = 0; x < nX; ++x )
			{
				const NGfx::SPixel8888 p = StraightPixel( x, y );
				lock[y][x] = NGfx::SPixel4444( p.r >> 4, p.g >> 4, p.b >> 4, p.a >> 4 );
			}
		}
	}
	else
	{
		NGfx::CTextureLock<NGfx::SPixel8888> lock( pValue, 0, NGfx::INPLACE );
		for ( int y = 0; y < nY; ++y )
		{
			for ( int x = 0; x < nX; ++x )
			{
				lock[y][x] = StraightPixel( x, y );
			}
		}
	}
}

// CBackgroundVectorImage

void CBackgroundVectorImage::SetFileName( const std::string &_szFileName )
{
	if ( szFileName != _szFileName )
	{
		szFileName = _szFileName;
		// A different image: parsed and rasterized again on the next Visit.
		pRaster = nullptr;
	}
}

void CBackgroundVectorImage::Visit( IUIVisitor *pVisitor )
{
	if ( pos.IsEmpty() || szFileName.empty() )
	{
		return;
	}
	if ( !IsValid( pRaster ) )
	{
		pRaster = new CRaster( szFileName );
	}
	const float fAspect = pRaster->GetAspect();
	if ( fAspect <= 0 )
	{
		return;
	}

	// The window in screen pixels.
	CTRect<float> rcScreen;
	VirtualToScreen( pos, &rcScreen );
	const float fWidth = rcScreen.Width();
	const float fHeight = rcScreen.Height();
	if ( fWidth <= 0 || fHeight <= 0 )
	{
		return;
	}
	// Every image gets the same width, and its height follows from its aspect
	// ratio, so a list of flags lines up in a column however their proportions
	// differ. That width is the window's laid-out shape, from the virtual
	// screen, at the window's height on screen: what the window would be if
	// the UI were scaled by the same factor both ways. The UI stretches X and Y
	// independently, so the window on screen is usually wider than that, and
	// fitting the image to it instead gave a 1:2 flag its whole width and a 3:2
	// flag less. Only an image narrower than the window's laid-out shape, which
	// is 16:11 for the game room's 32x22 slot, is held to the window's height
	// and comes out narrower.
	const float fSlotX = (std::min)( fWidth, fHeight * pos.Width() / pos.Height() );
	// Centred, then snapped to whole pixels: the quad renderer subtracts the
	// half pixel itself (C2DQuadsRenderer::FillRect), so integer corners map
	// texels onto pixels exactly, and the texture is rendered at that same
	// integer size.
	float fImageX = fSlotX;
	float fImageY = fSlotX / fAspect;
	if ( fImageY > fHeight )
	{
		fImageY = fHeight;
		fImageX = fHeight * fAspect;
	}
	const int nSizeX = static_cast<int>( std::floor( fImageX + 0.5f ) );
	const int nSizeY = static_cast<int>( std::floor( fImageY + 0.5f ) );
	if ( nSizeX <= 0 || nSizeY <= 0 )
	{
		return;
	}
	const float fX = std::floor( rcScreen.x1 + ( fWidth - nSizeX ) * 0.5f + 0.5f );
	const float fY = std::floor( rcScreen.y1 + ( fHeight - nSizeY ) * 0.5f + 0.5f );
	pRaster->SetSize( nSizeX, nSizeY );

	// Already in screen pixels, so not passed through VirtualToScreen.
	CRectLayout rects;
	rects.AddRect(
			fX, fY, nSizeX, nSizeY, CTRect<float>( 0.0f, 0.0f, nSizeX, nSizeY ), FadeColor( 0xFFFFFFFF, GetFadeValue() ) );
	pVisitor->VisitUITextureRect( pRaster, 3, rects );
}

int CBackgroundVectorImage::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CBackground *>( this ) );
	saver.Add( 2, &szFileName );
	saver.Add( 3, &pReplaced );
	return 0;
}
