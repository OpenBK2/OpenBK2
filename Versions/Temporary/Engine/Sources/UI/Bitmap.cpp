#include "stdafx.h"
#include "Bitmap.h"
#include "3dMotor/RectLayout.h"
#include "UIVisitor.h"

REGISTER_SAVELOAD_CLASS(0x170A6B80, CBitmap);
REGISTER_SAVELOAD_CLASS(0x170A6B81, CBitmapWindow);


// CBitmap

void CBitmap::Recalc()
{
	if ( !IsValid( pValue ) )
		pValue = NGfx::MakeTexture( GetNextPow2( nWidth ), GetNextPow2( nHeight ), 1, NGfx::SPixel8888::ID, NGfx::DYNAMIC_TEXTURE, NGfx::CLAMP );

	NGfx::CTextureLock<NGfx::SPixel8888> lock( pValue, 0, NGfx::INPLACE );
	for ( int y = 0; y < nHeight; ++y )
	{
		for ( int x = 0; x < nWidth; ++x )
		{
			lock[y][x] = points[y][x];
		}
	}
	bNeedUpdate = false;
}

void CBitmap::SetSize( const int _nWidth, const int _nHeight )
{
	nWidth = _nWidth;
	nHeight = _nHeight;
	points.SetSizes( nWidth, nHeight );
	Fill( NGfx::SPixel8888( 0, 0, 0, 0 ) );
	pValue = 0;
	bNeedUpdate = true;
}

void CBitmap::PutPixel( const int x, const int y, const NGfx::SPixel8888 &color )
{
	points[y][x] = color;
	bNeedUpdate = true;
}

const NGfx::SPixel8888 CBitmap::GetPixel( const int x, const int y ) const
{
	if ( x >= 0 && y >= 0 && x < nWidth && y < nHeight )
		return points[y][x];
	else
    return NGfx::SPixel8888( 0, 0, 0, 0 );
}

void CBitmap::Fill( const NGfx::SPixel8888 &color )
{
	points.FillEvery( color );
}

int CBitmap::operator&( IBinSaver &saver )
{
	saver.Add( 1, &points );
	saver.Add( 2, &nWidth );
	saver.Add( 3, &nHeight );
	saver.Add( 4, &bNeedUpdate );
	return 0;
}

// CBitmapWindow

void CBitmapWindow::Visit( struct IUIVisitor * pVisitor )
{
	if ( !pBitmap ) 
		return;

	CRectLayout rects;
	const CTRect<float> rectTexture = CTRect<float>( 0.0f, 0.0f, vSize.x, vSize.y );
	rects.AddRect( vPos.x, vPos.y, vSize.x, vSize.y, rectTexture, 0xFFFFFFFF );
	VirtualToScreen( &rects );
	pVisitor->VisitUITextureRect( pBitmap, 3, rects );
}

int CBitmapWindow::operator&( struct IBinSaver &saver )
{
	saver.Add( 1, &pBitmap );
	saver.Add( 1, &vPos );
	saver.Add( 1, &vSize );
	return 0;
}

void CBitmapWindow::SetPos( const CVec2 &_vPos )
{
	vPos = _vPos;
}

void CBitmapWindow::SetSize( const CVec2 &_vSize )
{
	if ( !pBitmap )
	{
		vSize = _vSize;
		pBitmap = new CBitmap();
		pBitmap->SetSize( vSize.x, vSize.y );
	}
}

void CBitmapWindow::PutPixel( const CVec2 &vPos, const NGfx::SPixel8888 &color )
{
	pBitmap->PutPixel( vPos.x, vPos.y, color );
}

const NGfx::SPixel8888 CBitmapWindow::GetPixel( const CVec2 &vPos ) const
{
	return pBitmap->GetPixel( vPos.x, vPos.y );
}

void CBitmapWindow::Fill( const NGfx::SPixel8888 &color )
{
	pBitmap->Fill( color );
}


