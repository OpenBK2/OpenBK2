#include "StdAfx.h"
#include <d3d9.h>
#include <d3dx9.h>

#include "ImageDDS.h"

#include "ImageInternal.h"
#include "ImageConvertor.h"
#include "DDS.h"
#include "System/FilePath.h"
#include "GUnpackDXT.h"
#include "ImageMip.h"
#include "3dMotor/GfxInternal.h" // ePixelFormat->D3DFormat
#include "ImageScale.h"

extern "C" { 
#include "vendor/S3TC/s3tc.h"
}




#pragma comment( linker, "/NODEFAULTLIB:libc.lib" )


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
	if ( ( static_cast<DWORD>(x) & 0x80000000 ) != 0 )       \
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

bool RecognizeFormatDDS( CDataStream *pStream )
{
	DWORD dwSignature = 0;;

	if ( pStream->GetPosition() + 4 >= pStream->GetSize() )
		return false;

	pStream->Read( &dwSignature, 4 );
	pStream->Seek( pStream->GetPosition() - 4 );
	return dwSignature == SDDSFileHeader::SIGNATURE;
}

// ************************************************************************************************************************ //
// **
// ** ARGB subformats decoding
// **
// **
// **
// ************************************************************************************************************************ //

static void DecompressARGB( DWORD *pRes, const SDDSHeader &hdr, const BYTE *pCompBytes )
{
	SPixelConvertInfo pci( hdr.ddspf.dwABitMask, hdr.ddspf.dwRBitMask, hdr.ddspf.dwGBitMask, hdr.ddspf.dwBBitMask );
	//
	switch ( hdr.ddspf.dwRGBBitCount ) 
	{
		case 16:
			{
				WORD *pSrc = (WORD*)pCompBytes;
				for ( int i = 0; i < hdr.dwWidth * hdr.dwHeight; ++i )
					*pRes++ = pci.DecompColor( *pSrc++ );
			}
			break;
		case 24:
			for ( int i = 0; i < hdr.dwWidth * hdr.dwHeight; ++i )
				*pRes++ = ( pCompBytes[i*3 + 0] << 16 ) | ( pCompBytes[i*3 + 1] << 8 ) | ( pCompBytes[i*3 + 2] );
			break;
		case 32:
			NI_ASSERT( false, "better read it directly to image" );
	}
}

// ************************************************************************************************************************ //
// **
// ** main load function - load DDS image and unpack it to ARGB8888 format
// **
// **
// **
// ************************************************************************************************************************ //

bool LoadImageDDS( CArray2D<DWORD> *pRes, CDataStream *pStream )
{
	// skip signature
	pStream->Seek( 4 );
	// read header
	SDDSHeader hdr;
	pStream->Read( &hdr, sizeof(hdr) );
	// check for sub-formats
	// DXT#
	if ( hdr.ddspf.dwFlags & DDS_FOURCC ) 
	{
		int nCompSize = 0, nDxt = 0;
		switch ( hdr.ddspf.dwFourCC ) 
		{
			case MAKEFOURCC('D','X','T','1'):
				nDxt = 1;
				nCompSize = hdr.dwWidth * hdr.dwHeight / 2;	// 4 BPP
				break;
			case MAKEFOURCC('D','X','T','2'):
				nDxt = 2;
				nCompSize = hdr.dwWidth * hdr.dwHeight;	// 8 BPP
				break;
			case MAKEFOURCC('D','X','T','3'):
				nDxt = 3;
				nCompSize = hdr.dwWidth * hdr.dwHeight;	// 8 BPP
				break;
			case MAKEFOURCC('D','X','T','4'):
				nDxt = 4;
				nCompSize = hdr.dwWidth * hdr.dwHeight;	// 8 BPP
				break;
			case MAKEFOURCC('D','X','T','5'):
				nDxt = 5;
				nCompSize = hdr.dwWidth * hdr.dwHeight;	// 8 BPP
				break;
		}
		// decompress
		if ( nCompSize > 0 ) 
		{

			BYTE *buffer = new BYTE[nCompSize];
			pStream->Read( buffer, nCompSize );
			UnpackDXT( nDxt, hdr.dwWidth, hdr.dwHeight, buffer, pRes );
			delete []buffer;
		}
		else
			return false;
	}
	else if ( ((hdr.ddspf.dwFlags & DDS_ARGB) == DDS_ARGB) || ((hdr.ddspf.dwFlags & DDS_ARGB) == DDS_RGB) ) 
	{
		pRes->SetSizes( hdr.dwWidth, hdr.dwHeight );
		if ( hdr.ddspf.dwRGBBitCount == 32 )	// directly read to image - this is ARGB8888
			pStream->Read( &(*pRes)[0][0], hdr.dwWidth * hdr.dwHeight * 4 );
		else
		{
			int nCompSize = hdr.dwWidth * hdr.dwHeight * hdr.ddspf.dwRGBBitCount / 8;
			BYTE *buffer = new BYTE[nCompSize];
			pStream->Read( buffer, nCompSize );
			DecompressARGB( &(*pRes)[0][0], hdr, buffer );
			delete []buffer;
		}
		return true;
	}
	return false;
}

// ************************************************************************************************************************ //
// **
// ** DXT# and ARGB compression using S3TC compressor
// **
// **
// **
// ************************************************************************************************************************ //

static bool GetDDSPixelFormat( NGfx::EPixelFormat format, SDDSPixelFormat *pFormat )
{
	switch ( format ) 
	{
		case CF_DXT1:
			*pFormat = DDSPF_DXT1;
			break;
		case CF_DXT2:
			*pFormat = DDSPF_DXT2;
			break;
		case CF_DXT3:
			*pFormat = DDSPF_DXT3;
			break;
		case CF_DXT4:
			*pFormat = DDSPF_DXT4;
			break;
		case CF_DXT5:
			*pFormat = DDSPF_DXT5;
			break;
		case CF_A8R8G8B8:
			*pFormat = DDSPF_A8R8G8B8;
			break;
		case CF_A4R4G4B4:
			*pFormat = DDSPF_A4R4G4B4;
			break;
		case CF_A1R5G5B5:
			*pFormat = DDSPF_A1R5G5B5;
			break;
		case CF_R5G6B5:
			*pFormat = DDSPF_R5G6B5;
			break;
		default:
			return false;
	}
	return true;
}

static bool CompressDXTN( vector<BYTE> &compImage, const CArray2D<DWORD> &srcImage, EPixelFormat format )
{
	float fWeights[] = { 0.309f, 0.609f, 0.082f, 0, 0, 0, 0, 0 };
	DWORD dwEncodeType = 0;
	SDDSPixelFormat ddsformat;
	int nBPP = 8;
	// compose encoding type
	GetDDSPixelFormat( format, &ddsformat );
	switch ( format )
	{
		case CF_DXT1:
			dwEncodeType = S3TC_ENCODE_RGB_COLOR_KEY;
			nBPP = 4;
			break;
		case CF_DXT2:
			dwEncodeType = S3TC_ENCODE_RGB_ALPHA_COMPARE | S3TC_ENCODE_ALPHA_EXPLICIT;
			break;
		case CF_DXT3:
			dwEncodeType = S3TC_ENCODE_RGB_FULL | S3TC_ENCODE_ALPHA_EXPLICIT;
			break;
		case CF_DXT4:
			dwEncodeType = S3TC_ENCODE_RGB_ALPHA_COMPARE | S3TC_ENCODE_ALPHA_INTERPOLATED;
			break;
		case CF_DXT5:
			dwEncodeType = S3TC_ENCODE_RGB_FULL | S3TC_ENCODE_ALPHA_INTERPOLATED;
			break;
	}
	// compose in header
	DDSURFACEDESC ddsdIn;
	Zero( ddsdIn );
	ddsdIn.dwSize = sizeof( DDSURFACEDESC );

	ddsdIn.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_LINEARSIZE | DDSD_PIXELFORMAT | DDSD_LPSURFACE;
	ddsdIn.dwWidth = srcImage.GetSizeX();
	ddsdIn.dwHeight = srcImage.GetSizeY();
	ddsdIn.lPitch = srcImage.GetSizeX() * 4;
	ddsdIn.lpSurface = (LPVOID)( &srcImage[0][0] );

	ddsdIn.ddpfPixelFormat.dwSize = sizeof( DDPIXELFORMAT );
	ddsdIn.ddpfPixelFormat.dwRGBBitCount = 32;
	ddsdIn.ddpfPixelFormat.dwFlags = DDPF_ALPHAPIXELS | DDPF_RGB;
	ddsdIn.ddpfPixelFormat.dwRBitMask = 0x00FF0000;
	ddsdIn.ddpfPixelFormat.dwGBitMask = 0x0000FF00;
	ddsdIn.ddpfPixelFormat.dwBBitMask = 0x000000FF;
	ddsdIn.ddpfPixelFormat.dwRGBAlphaBitMask = 0xFF000000;
	// compose out header
	DDSURFACEDESC ddsdOut;
	Zero( ddsdOut );
	ddsdOut.dwSize = sizeof( DDSURFACEDESC );
	const int nNumCompressedBytes = S3TCgetEncodeSize( &ddsdIn, dwEncodeType );
	// 
	compImage.resize( nNumCompressedBytes );
	//
	S3TCsetAlphaReference( 0 );
	S3TCencode( &ddsdIn, 0, &ddsdOut, &(compImage[0]), dwEncodeType, fWeights );
	//
	return true;
}

static bool CompressRGBA( vector<BYTE> &compImage, const CArray2D<DWORD> &srcImage, NGfx::EPixelFormat format )
{
	const int nSizeX = srcImage.GetSizeX();
	const int nSizeY = srcImage.GetSizeY();
	int nBPP = 16;
	//
	SPixelConvertInfo pci;
	SDDSPixelFormat ddsformat;
	GetDDSPixelFormat( format, &ddsformat );
	switch ( format )
	{
		case CF_A8R8G8B8:
			pci.InitMaskInfo( 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff );
			nBPP = 32;
			break;
		case CF_A1R5G5B5:
			pci.InitMaskInfo( 0x00008000, 0x00007c00, 0x000003e0, 0x0000001f );
			break;
		case CF_A4R4G4B4:
			pci.InitMaskInfo( 0x0000f000, 0x00000f00, 0x000000f0, 0x0000000f );
			break;
		case CF_R5G6B5:
			pci.InitMaskInfo( 0x00000000, 0x0000f800, 0x000007e0, 0x0000001f );
			break;
		default:
			return false;
	}
	//
	compImage.resize( nSizeX * nSizeY * nBPP / 8 );

	const DWORD *pSrc = &( srcImage[0][0] );

	if ( nBPP == 16 )
	{
		WORD *pDst = reinterpret_cast<WORD*>( &( compImage[0] ) );
		for ( int i = 0; i < nSizeX*nSizeY; ++i, ++pDst )
			*pDst = pci.ComposeColorSlow( pSrc[i] );
		return true;
	}
	else if ( nBPP == 32 )
	{
		DWORD *pDst = reinterpret_cast<DWORD*>( &( compImage[0] ) );
		memcpy( pDst, pSrc, nSizeX*nSizeY*nBPP/8 );
		return true;
	}
	return 0;
}

static bool Compress( vector<BYTE> &compImage, const CArray2D<DWORD> &srcImage, EPixelFormat format )
{
	if ( (format >= CF_DXT1) && (format <= CF_DXT5) )
		return CompressDXTN( compImage, srcImage, format );
	else if ( (format >= CF_A8R8G8B8) || (format <= CF_R5G6B5) )
		return CompressRGBA( compImage, srcImage, format );
	else
		return false;
}

static void MakeDDSHeader( SDDSHeader *pHdr, int nWidth, int nHeight, NGfx::EPixelFormat ePixelFormat, int nNumMipLevels )
{
	// get BPP of the image
	int nBPP = 0;
	switch ( ePixelFormat ) 
	{
		case CF_DXT1:
			nBPP = 4;
			break;
		case CF_DXT2:
		case CF_DXT3:
		case CF_DXT4:
		case CF_DXT5:
			nBPP = 8;
			break;
		case CF_A8R8G8B8:
			nBPP = 32;
			break;
		case CF_A4R4G4B4:
		case CF_A1R5G5B5:
		case CF_R5G6B5:
			nBPP = 16;
			break;
	}	
	// build header
	GetDDSPixelFormat( ePixelFormat, &pHdr->ddspf );
	pHdr->dwWidth = nWidth;
	pHdr->dwHeight = nHeight;
	pHdr->dwMipMapCount = nNumMipLevels;
	pHdr->dwHeaderFlags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_MIPMAP;
	pHdr->dwSurfaceFlags = DDS_SURFACE_FLAGS_TEXTURE | DDS_SURFACE_FLAGS_MIPMAP;
	if ( pHdr->ddspf.dwFlags == DDS_FOURCC )
	{
		pHdr->dwHeaderFlags |= DDS_HEADER_FLAGS_LINEARSIZE;
		pHdr->dwPitchOrLinearSize = pHdr->dwWidth * pHdr->dwHeight * nBPP / 8;
	}
	else
	{
		pHdr->dwHeaderFlags |= DDS_HEADER_FLAGS_PITCH;
		pHdr->dwPitchOrLinearSize = pHdr->dwWidth * nBPP / 8;
	}
}

static int CalcNumMipLevels( int nWidth, int nHeight, NGfx::EPixelFormat ePixelFormat, int nNumMipLevels )
{
	const int nMaxPossible = GetMSB( Min(nWidth, nHeight) ) - ( (ePixelFormat >= CF_DXT1) && (ePixelFormat <= CF_DXT5) ? 2 : 0 );
	return nNumMipLevels <= 0 ? nMaxPossible : Min( nNumMipLevels, nMaxPossible );
}

// ************************************************************************************************************************ //
// **
// ** special image processing for improved DDS texture-oriented quality
// **
// **
// **
// ************************************************************************************************************************ //

//! Prepare image for compression in accordance with type
void PrepareImageForCompression( CArray2D<CVec4> *pSrc, EImageType eImageType, 
																bool bWrapX, bool bWrapY, float fMappingSize )
{
	switch ( eImageType )
	{
	case IMAGE_TYPE_PICTURE:
		break;
	case IMAGE_TYPE_BUMP:
		GenerateNormals( pSrc, CVec4(1, 0, 0, 0), fMappingSize, bWrapX, bWrapY );
		break;
	case IMAGE_TYPE_TRANSPARENT:
		for ( int y = 0; y < pSrc->GetSizeY(); ++y )
		{
			for ( int x = 0; x < pSrc->GetSizeX(); ++x )
			{
				CVec4 &v = (*pSrc)[y][x];
				v = CVec4( v.x * v.a, v.y * v.a, v.z * v.a, v.a );
			}
		}
		break;
	case IMAGE_TYPE_TRANSPARENT_ADD:
		for ( int y = 0; y < pSrc->GetSizeY(); ++y )
		{
			for ( int x = 0; x < pSrc->GetSizeX(); ++x )
			{
				CVec4 &v = (*pSrc)[y][x];
				v = CVec4( v.x * v.a, v.y * v.a, v.z * v.a, 0 );
			}
		}
		break;
	default:
		ASSERT(0);
		break;
	}
}

void GenerateMipLevelsAndPrepareForCompression( vector<CArray2D<DWORD> > *pMips, const CArray2D<CVec4> &srcImage, 
																								EImageType eImageType, NGfx::EPixelFormat ePixelFormat, int _nNumMipLevels, 
																								bool bWrapX, bool bWrapY, float fMappingSize )
{
	const int nNumMipLevels = CalcNumMipLevels( srcImage.GetSizeX(), srcImage.GetSizeY(), ePixelFormat, _nNumMipLevels );
	pMips->resize( nNumMipLevels );

	// generate mip-map levels and process it
	CArray2D<CVec4> mip( srcImage ), src( srcImage );
	//NHPTimer::STime tStart;
	for ( int i = 0; i < nNumMipLevels; ++i )
	{
		// process current mip-map level
		PrepareImageForCompression( &src, eImageType, bWrapX, bWrapY, fMappingSize );
		// convert to packed DWORD image and store to i-th mip image
		CArray2D<DWORD> &dst = (*pMips)[i];
		dst.SetSizes( src.GetSizeX(), src.GetSizeY() );
		Convert( &dst, src );

		// generate next mip level
		GenerateMipLevel( &src, mip, bWrapX, bWrapY );
		mip = src;
	}
}

// ************************************************************************************************************************ //
// **
// ** DX compression specific functions
// **
// **
// **
// ************************************************************************************************************************ //

static void WriteDDS( IDirect3DDevice9 *pDevice, const string &szFileName, NGfx::EPixelFormat ePixelFormat, 
	const vector<CArray2D<DWORD> > &mips )
{
	ASSERT( !mips.empty() );
	if ( mips.empty() )
		return;
	D3DFORMAT fmt = NGfx::PixelID2D3DFormat( ePixelFormat );
	if ( fmt == D3DFMT_A8R8G8B8 && ePixelFormat != NGfx::CF_A8R8G8B8 )
	{
		NI_ASSERT( 0, StrFmt("Wrong destination format, DXT conversion failed (\"%s\")", szFileName.c_str()) );
		return;
	}

	NWin32Helper::com_ptr<IDirect3DTexture9> pDstTexture;
	HRESULT hr = pDevice->CreateTexture( mips[0].GetSizeX(), mips[0].GetSizeY(), mips.size(), 0, fmt,
		                                   D3DPOOL_MANAGED, pDstTexture.GetAddr(), 0 );
	if ( FAILED(hr) )
	{
		NI_ASSERTHR( hr, StrFmt("Can't create DXT texture \"%s\", DXT conversion failed", szFileName.c_str()) );
		return;
	}

	for ( int nLevel = 0; nLevel < mips.size(); ++nLevel )
	{
		const CArray2D<DWORD> &image = mips[ nLevel ];
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
			NI_ASSERTHR( hr, StrFmt("Can't get %d level of texture \"%s\", conversion failed", nLevel, szFileName.c_str()) );
			continue;
		}
		hr = D3DXLoadSurfaceFromMemory( pSurfaceLevel, NULL, NULL, &(image[0][0]), D3DFMT_A8R8G8B8,
			                                      image.GetSizeX() * sizeof(DWORD), NULL, &rect, D3DX_FILTER_NONE, 0 );
		if ( FAILED(hr) ) 
		{
			NI_ASSERTHR( hr, StrFmt("Can't load %d level of texture \"%s\", conversion failed", nLevel, szFileName.c_str()) );
			continue;
		}
	}

	NFile::CreatePath( NFile::GetFilePath(szFileName) );
	hr = D3DXSaveTextureToFile( szFileName.c_str(), D3DXIFF_DDS, pDstTexture, NULL );
	if ( FAILED(hr) )
	{
		NI_ASSERTHR( hr, StrFmt("Can't write final DXT texture \"%s\", DXT conversion failed", szFileName.c_str()) );
	}
}

static void ConvertAndSaveAsDDS( IDirect3DDevice9 *pDevice, const string &szFileName, const CArray2D<CVec4> &srcImage, 
	EImageType eImageType, NGfx::EPixelFormat ePixelFormat, int _nNumMipLevels, bool bWrapX, bool bWrapY, float fMappingSize )
{
	vector<CArray2D<DWORD> > mips;
	GenerateMipLevelsAndPrepareForCompression( &mips, srcImage, eImageType, ePixelFormat, 
		                                         _nNumMipLevels, bWrapX, bWrapY, fMappingSize );

	WriteDDS( pDevice, szFileName, ePixelFormat, mips );
}

static void SaveAsDDSWithDX( IDirect3DDevice9 *pDevice, const string &szFileName, const CArray2D<DWORD> &srcImage, 
	NGfx::EPixelFormat ePixelFormat, int _nNumMipLevels )
{
	int nNumMipLevels = CalcNumMipLevels( srcImage.GetSizeX(), srcImage.GetSizeY(), ePixelFormat, _nNumMipLevels );

	vector<CArray2D<DWORD> > mips;
	mips.resize( nNumMipLevels );
	mips[0] = srcImage;

  for ( int nLevel = 1; nLevel < nNumMipLevels; ++nLevel )
  {
    const int nSizeX = srcImage.GetSizeX() >> nLevel;
    const int nSizeY = srcImage.GetSizeY() >> nLevel;
		CArray2D<DWORD> &image = mips[ nLevel ];
		image.SetSizes( nSizeX, nSizeY );
		Scale( &image, srcImage, IMAGE_SCALE_METHOD_LANCZOS3 );
  }
	WriteDDS( pDevice, szFileName, ePixelFormat, mips );
}

#define DEF_INV_255 ( 1.0f / 255 )
void ConvertAndSaveAsDDSWithDX( IDirect3DDevice9 * pDevice, const string &szFileName, const CArray2D<DWORD> &srcImage,
	EImageType eImageType, NGfx::EPixelFormat nSubFormat, int nNumMipLevels, bool bWrapX, bool bWrapY, float fMappingSize )
{
	if ( pDevice == NULL )
	{
		NI_ASSERT( pDevice != NULL, "D3DDevice is not ready, DXT conversion failed" );
		return;
	}

	if ( eImageType != IMAGE_TYPE_PICTURE_FASTMIP )
	{
		CArray2D<CVec4> trueImage;
		trueImage.SetSizes( srcImage.GetSizeX(), srcImage.GetSizeY() );
		Convert( &trueImage, srcImage );
		ConvertAndSaveAsDDS( pDevice, szFileName, trueImage, eImageType, nSubFormat, nNumMipLevels, bWrapX, bWrapY, fMappingSize );
		return;
	}

	switch ( eImageType )
  {
	case IMAGE_TYPE_PICTURE_FASTMIP:
		break;
	default:
		ASSERT(0);
		break;
  }

  SaveAsDDSWithDX( pDevice, szFileName, srcImage, nSubFormat, nNumMipLevels );
}

// ************************************************************************************************************************ //
// **
// ** S3TC specific compression functions
// **
// **
// **
// ************************************************************************************************************************ //

bool ConvertAndSaveAsDDS( CDataStream *pStream, const CArray2D<DWORD> &srcImage,
													EImageType eImageType, NGfx::EPixelFormat eSubFormat, int _nNumMipLevels, 
													bool bWrapX, bool bWrapY, float fMappingSize )
{
	vector<CArray2D<DWORD> > mips;
	{
		CArray2D<CVec4> srcv4;
		srcv4.SetSizes( srcImage.GetSizeX(), srcImage.GetSizeY() );
		Convert( &srcv4, srcImage );
		GenerateMipLevelsAndPrepareForCompression( &mips, srcv4, eImageType, eSubFormat,
			_nNumMipLevels, bWrapX, bWrapY, fMappingSize );
	}
	const int nNumMipLevels = mips.size();
	//
	SDDSFileHeader hdr;
	MakeDDSHeader( &hdr.header, srcImage.GetSizeX(), srcImage.GetSizeY(), eSubFormat, nNumMipLevels );
	pStream->Write( &hdr, sizeof(hdr) );
	//
	for ( int i = 0; i < mips.size(); ++i )
	{
		vector<BYTE> compImage;
		if ( Compress(compImage, mips[i], eSubFormat) )
			pStream->Write( &(compImage[0]), compImage.size() );
		else
		{
			NI_VERIFY( false, StrFmt("Can't compress image to %d format", eSubFormat), return false );
		}
	}
	return true;
}

}

