#include "StdAfx.h"

#include "../Misc/2DArray.h"
#include "Tools_Image.h"

void NImage::Copy( CArray2D<DWORD> *pDestination, const CArray2D<DWORD> &rSource, const CTPoint<int> &rStartPoint )
{
	for ( int nXIndex = 0; ( nXIndex < pDestination->GetSizeX() ) && ( nXIndex < ( rSource.GetSizeX() - rStartPoint.x ) ); ++nXIndex )
	{
		for ( int nYIndex = 0; ( nYIndex < pDestination->GetSizeY() ) && ( nYIndex < ( rSource.GetSizeY() - rStartPoint.y ) ); ++nYIndex )
		{
			( *pDestination )[nYIndex][nXIndex] = rSource[nYIndex + rStartPoint.y][nXIndex + rStartPoint.x];
		}
	}
}


void NImage::Load2Bitmap( CBitmap *pBitmap, const CArray2D<DWORD> &rImage )
{
	Load2Bitmap( pBitmap, rImage, CTPoint<int>( rImage.GetSizeX(), rImage.GetSizeY() ) );
}


void NImage::Load2Bitmap( CBitmap *pBitmap, const CArray2D<DWORD> &rImage, const CTPoint<int> &rSize )
{
	try
	{
		CTPoint<int> size = rSize;
		if ( ( size.x < 0 ) || ( size.x > rImage.GetSizeX() ) )
		{
			size.x = rImage.GetSizeX();
		}
		if ( ( size.y < 0 ) || ( size.y > rImage.GetSizeY() ) )
		{
			size.y = rImage.GetSizeY();
		}
		pBitmap->DeleteObject();

		BITMAPINFO bmi;
		bmi.bmiHeader.biSize = sizeof( bmi.bmiHeader );
		bmi.bmiHeader.biWidth = size.x;
		bmi.bmiHeader.biHeight = -size.y;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;
		bmi.bmiHeader.biSizeImage = 0;
		bmi.bmiHeader.biClrUsed = 0;

		HDC hDC = ::GetDC( GetDesktopWindow() );
		HBITMAP hbm = CreateCompatibleBitmap( hDC, size.x, size.y );
		::SetDIBits( hDC, hbm, 0, size.y, &(rImage[0][0]), &bmi, DIB_RGB_COLORS );
		::ReleaseDC( GetDesktopWindow(), hDC );
		pBitmap->Attach( hbm );
	}
	catch ( ... ) 
	{
	}
}


// basement storage  


