#include "stdafx.h"
#include <d3d9.h>
#include <d3dx9.h>

#include "ImageDDS.h"

#include "System/FilePath.h"
#include "3Dmotor/GfxInternal.h" // ePixelFormat->D3DFormat
#include "ImageScale.h"

#include <cstdint>

#include <fmt/format.h>


// converts DirectX error code to the string
static const char* DXErrorToString( HRESULT hErrorCode )
{
	switch( hErrorCode )
	{
	case D3D_OK:
		return "No error occurred.";
	case D3DERR_CONFLICTINGRENDERSTATE:
		return "The currently set render states cannot be used together.";
	case D3DERR_CONFLICTINGTEXTUREFILTER:
		return "The current texture filters cannot be used together.";
	case D3DERR_CONFLICTINGTEXTUREPALETTE: 
		return "The current textures cannot be used simultaneously.\nThis generally occurs when a multitexture device requires that all palletized textures simultaneously enabled also share the same palette.";
	case D3DERR_DEVICELOST:
		return "The device is lost and cannot be restored at the current time, so rendering is not possible.";
	case D3DERR_DEVICENOTRESET:
		return "The device cannot be reset.";
	case D3DERR_DRIVERINTERNALERROR:
		return "Internal driver error.";
	case D3DERR_INVALIDCALL:
		return "The method call is invalid. For example, a method's parameter may have an invalid value.";
	case D3DERR_INVALIDDEVICE:
		return "The requested device type is not valid.";
	case D3DERR_MOREDATA:
		return "There is more data available than the specified buffer size can hold.";
	case D3DERR_NOTAVAILABLE:
		return "The queried technique is not supported by this device.";
	case D3DERR_NOTFOUND:
		return "The requested item was not found.";
	case D3DERR_OUTOFVIDEOMEMORY:
		return "Direct3D does not have enough display memory to perform the operation.";
	case D3DERR_TOOMANYOPERATIONS: 
		return "The application is requesting more texture-filtering operations than the device supports.";
	case D3DERR_UNSUPPORTEDALPHAARG:
		return "The device does not support a specified texture-blending arguments for the alpha channel.";
	case D3DERR_UNSUPPORTEDALPHAOPERATION:
		return "The device does not support a specified texture-blending operations for the alpha channel.";
	case D3DERR_UNSUPPORTEDCOLORARG:
		return "The device does not support a specified texture-blending arguments for color values.";
	case D3DERR_UNSUPPORTEDCOLOROPERATION:
		return "The device does not support a specified texture-blending operations for color values.";
	case D3DERR_UNSUPPORTEDFACTORVALUE:
		return "The specified texture factor value is not supported by the device.";
	case D3DERR_UNSUPPORTEDTEXTUREFILTER: 
		return "The specified texture filter is not supported by the device.";
	case D3DERR_WRONGTEXTUREFORMAT:
		return "The pixel format of the texture surface is not valid.";
	default:
		return "Unrecognized error value.";
	}
	return "Unrecognized error value.";
}

#if defined( _DO_ASSERT_SLOW )
#define NI_ASSERTHR( x, user_text )                        \
{                                                          \
	if ( ( static_cast<uint32_t>(x) & 0x80000000 ) != 0 )       \
	{                                                        \
		char buff[1024];                                       \
		sprintf( buff, "(0x%X) %s", x, DXErrorToString( x ) ); \
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
