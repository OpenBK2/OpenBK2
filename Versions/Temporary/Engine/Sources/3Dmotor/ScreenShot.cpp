#include "stdafx.h"
#include "Gfx.h"
#include "GfxBuffers.h"
#include "ScreenShot.h"

namespace NGScene
{

// CScreenshotTexture

CScreenshotTexture::CScreenshotTexture():
	eMode( COLOR ), vCoeff( 1, 1, 1, 1 )
{
}

void CScreenshotTexture::Get( CArray2D<NGfx::SPixel8888> *pScreenShot )
{
	*pScreenShot = sScreenShot;
}

void CScreenshotTexture::Set( const CArray2D<NGfx::SPixel8888> &_sScreenShot )
{
	sScreenShot = _sScreenShot;
}

void CScreenshotTexture::Set( const CScreenshotTexture &_sScreenShot )
{
	sScreenShot = _sScreenShot.sScreenShot;
}

void CScreenshotTexture::Generate( bool bMakeItSafe )
{
	if ( !bMakeItSafe )
		NGfx::MakeFast32BitScreenShot( &sScreenShot, false );
	else
		NGfx::MakeScreenShot( &sScreenShot, false );
}

void CScreenshotTexture::GetSize( CTPoint<int> *pSize )
{
	pSize->x = sScreenShot.GetSizeX();
	pSize->y = sScreenShot.GetSizeY();
}

void CScreenshotTexture::SetMode( EMode _eMode, const CVec4 &_vCoeff )
{
	eMode = _eMode;
	vCoeff = _vCoeff;

	Updated();
}

void CScreenshotTexture::Recalc()
{
	CTPoint<int> sSize( GetNextPow2( sScreenShot.GetSizeX() ), GetNextPow2( sScreenShot.GetSizeY() ) );
	pValue = NGfx::MakeTexture( sSize.x, sSize.y, 1, NGfx::SPixel8888::ID, NGfx::REGULAR, NGfx::CLAMP );

	if ( !IsValid( pValue ) )
		return;

	if ( eMode == COLOR )
	{
		NGfx::CTextureLock<NGfx::SPixel8888> sLock( pValue, 0, NGfx::INPLACE );

		for ( int nTempY = 0; nTempY < sScreenShot.GetSizeY(); nTempY++ )
		{
			for ( int nTempX = 0; nTempX < sScreenShot.GetSizeX(); nTempX++ )
			{
				NGfx::SPixel8888 &sDst = sLock[nTempY][nTempX];
				const NGfx::SPixel8888 &sSrc = sScreenShot[nTempY][nTempX];

				sDst.a = Float2Int( 0xFF * vCoeff.a );
				sDst.r = Float2Int( sSrc.r * vCoeff.r );
				sDst.g = Float2Int( sSrc.g * vCoeff.g );
				sDst.b = Float2Int( sSrc.b * vCoeff.b );
			}
		}
	}
	else if ( eMode == BLACKANDWHITE )
	{
		NGfx::CTextureLock<NGfx::SPixel8888> sLock( pValue, 0, NGfx::INPLACE );

		for ( int nTempY = 0; nTempY < sScreenShot.GetSizeY(); nTempY++ )
		{
			for ( int nTempX = 0; nTempX < sScreenShot.GetSizeX(); nTempX++ )
			{
				NGfx::SPixel8888 &sDst = sLock[nTempY][nTempX];
				const NGfx::SPixel8888 &sSrc = sScreenShot[nTempY][nTempX];

				float fVal = Float2Int( ( ( sSrc.r * 0.3086f ) + ( sSrc.g * 0.6094f ) + ( sSrc.b * 0.0820f ) ) / 2 );
				sDst.a = Float2Int( 0xFF * vCoeff.a );
				sDst.r = Float2Int( fVal * vCoeff.r );
				sDst.g = Float2Int( fVal * vCoeff.g );
				sDst.b = Float2Int( fVal * vCoeff.b );
			}
		}
	}

	sSize.x = sScreenShot.GetSizeX();
	sSize.y = sScreenShot.GetSizeY();

	Updated();
}

} // namespace
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0xB2030101, CScreenshotTexture );

