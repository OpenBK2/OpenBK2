#include "stdafx.h"
#include <d3d9.h>
#include <d3dx9.h>

#include "ImageDDS.h"

#include "System/FilePath.h"
#include "3Dmotor/GfxInternal.h" // ePixelFormat->D3DFormat
#include "3Dmotor/D3DError.h"
#include "ImageScale.h"

#include <cstdint>

#include <fmt/format.h>


#if defined( _DO_ASSERT_SLOW )
#define NI_ASSERTHR( x, user_text )                        \
{                                                          \
	if ( ( static_cast<uint32_t>(x) & 0x80000000 ) != 0 )       \
	{                                                        \
		char buff[1024];                                       \
		sprintf( buff, "(0x%X) %s", x, D3DErrorToString( x ) ); \
		NI_FORCE_ASSERT( 0, buff, user_text );                 \
	}                                                        \
}
#else
#define NI_ASSERTHR( x, user_text ) ((void)0);
#endif

using namespace NGfx;

namespace NImage
{

// ************************************************************************************************************************ //
// **
// ** DXT# and ARGB compression using S3TC compressor
// **
// **
// **
// ************************************************************************************************************************ //

static int CalcNumMipLevels( int nWidth, int nHeight, NGfx::EPixelFormat ePixelFormat, int nNumMipLevels )
{
	const int nMaxPossible = GetMSB( (std::min)(nWidth, nHeight) ) - ( (ePixelFormat >= CF_DXT1) && (ePixelFormat <= CF_DXT5) ? 2 : 0 );
	return nNumMipLevels <= 0 ? nMaxPossible : (std::min)( nNumMipLevels, nMaxPossible );
}

// ************************************************************************************************************************ //
// **
// ** DX compression specific functions
// **
// **
// **
// ************************************************************************************************************************ //

static void WriteDDS( IDirect3DDevice9 *pDevice, const std::string &szFileName, NGfx::EPixelFormat ePixelFormat,
	const std::vector<CArray2D<uint32_t> > &mips )
{
	ASSERT( !mips.empty() );
	if ( mips.empty() )
		return;
	D3DFORMAT fmt = NGfx::PixelID2D3DFormat( ePixelFormat );
	if ( fmt == D3DFMT_A8R8G8B8 && ePixelFormat != NGfx::CF_A8R8G8B8 )
	{
		NI_ASSERT( 0, fmt::format("Wrong destination format, DXT conversion failed (\"{}\")", szFileName) );
		return;
	}

	NWin32Helper::com_ptr<IDirect3DTexture9> pDstTexture;
	HRESULT hr = pDevice->CreateTexture( mips[0].GetSizeX(), mips[0].GetSizeY(), mips.size(), 0, fmt,
		                                   D3DPOOL_MANAGED, pDstTexture.GetAddr(), 0 );
	if ( FAILED(hr) )
	{
		NI_ASSERTHR( hr, fmt::format("Can't create DXT texture \"{}\", DXT conversion failed", szFileName) );
		return;
	}

	for ( int nLevel = 0; nLevel < mips.size(); ++nLevel )
	{
		const CArray2D<uint32_t> &image = mips[ nLevel ];
		RECT rect;
		const int nSizeX = image.GetSizeX();
		const int nSizeY = image.GetSizeY();

		rect.left = 0;
		rect.top = 0;
		rect.right = image.GetSizeX();
		rect.bottom = image.GetSizeY();

		NWin32Helper::com_ptr<IDirect3DSurface9> pSurfaceLevel;
		HRESULT hr = pDstTexture->GetSurfaceLevel( nLevel, pSurfaceLevel.GetAddr() );
		if ( FAILED(hr) ) 
		{
			NI_ASSERTHR( hr, fmt::format("Can't get {} level of texture \"{}\", conversion failed", nLevel, szFileName) );
			continue;
		}
		hr = D3DXLoadSurfaceFromMemory( pSurfaceLevel, NULL, NULL, &(image[0][0]), D3DFMT_A8R8G8B8,
			                                      image.GetSizeX() * sizeof(uint32_t), NULL, &rect, D3DX_FILTER_NONE, 0 );
		if ( FAILED(hr) ) 
		{
			NI_ASSERTHR( hr, fmt::format("Can't load {} level of texture \"{}\", conversion failed", nLevel, szFileName) );
			continue;
		}
	}

	NFile::CreatePath( NFile::GetFilePath(szFileName) );
	hr = D3DXSaveTextureToFile( szFileName.c_str(), D3DXIFF_DDS, pDstTexture, NULL );
	if ( FAILED(hr) )
	{
		NI_ASSERTHR( hr, fmt::format("Can't write final DXT texture \"{}\", DXT conversion failed", szFileName) );
	}
}

static void SaveAsDDSWithDX( IDirect3DDevice9 *pDevice, const std::string &szFileName, const CArray2D<uint32_t> &srcImage,
	NGfx::EPixelFormat ePixelFormat, int _nNumMipLevels )
{
	int nNumMipLevels = CalcNumMipLevels( srcImage.GetSizeX(), srcImage.GetSizeY(), ePixelFormat, _nNumMipLevels );

	std::vector<CArray2D<uint32_t> > mips;
	mips.resize( nNumMipLevels );
	mips[0] = srcImage;

  for ( int nLevel = 1; nLevel < nNumMipLevels; ++nLevel )
  {
    const int nSizeX = srcImage.GetSizeX() >> nLevel;
    const int nSizeY = srcImage.GetSizeY() >> nLevel;
		CArray2D<uint32_t> &image = mips[ nLevel ];
		image.SetSizes( nSizeX, nSizeY );
		Scale( &image, srcImage, IMAGE_SCALE_METHOD_LANCZOS3 );
  }
	WriteDDS( pDevice, szFileName, ePixelFormat, mips );
}

#define DEF_INV_255 ( 1.0f / 255 )
void ConvertAndSaveAsDDSWithDX( IDirect3DDevice9 * pDevice, const std::string &szFileName, const CArray2D<uint32_t> &srcImage,
	EImageType eImageType, NGfx::EPixelFormat nSubFormat, int nNumMipLevels, bool bWrapX, bool bWrapY, float fMappingSize )
{
	if ( pDevice == NULL )
	{
		NI_ASSERT( pDevice != NULL, "D3DDevice is not ready, DXT conversion failed" );
		return;
	}
  SaveAsDDSWithDX( pDevice, szFileName, srcImage, nSubFormat, nNumMipLevels );
}

}
