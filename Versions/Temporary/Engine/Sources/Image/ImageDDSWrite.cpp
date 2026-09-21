#include "stdafx.h"

#include "ImageDDS.h"

#include "DDS.h"
#include "ImageInternal.h"		// SPixelConvertInfo
#include "ImageConvertor.h"	// Convert, between packed ARGB and CVec4
#include "ImageMip.h"				// GenerateMipLevel, GenerateNormals
#include "ImageScale.h"

#include "System/FilePath.h"
#include "System/Streams.h"

#include "3Dmotor/GPixelFormat.h"

#include <squish/squish.h>

#include <cstdint>
#include <vector>

#include <fmt/format.h>


using namespace NGfx;

namespace NImage
{

// ************************************************************************************************************************ //
// **
// ** DXT# and ARGB compression
// **
// ** This writer builds the DDS container itself and compresses through squish.
// ** It replaces one that went through D3DXLoadSurfaceFromMemory and
// ** D3DXSaveTextureToFile, which pinned texture export to Windows and made the
// ** editor create a D3DDEVTYPE_NULLREF device -- a device that renders nothing
// ** -- purely to have something to call CreateTexture on. Nothing here needs a
// ** GPU: the compressor is a pure function of the pixels and the header is 128
// ** bytes of it.
// **
// ************************************************************************************************************************ //

//! Colour error weights the original pipeline compressed with.
//! The S3TC path that shipped this game's textures passed exactly these three
//! numbers to S3TCencode as its colour metric, and squish takes them in the same
//! role and the same R,G,B order. Keeping them means the encoder carries the
//! perceptual bias the shipped data was built with instead of squish's own
//! Rec.709 default. squish takes the metric by non-const pointer, so this cannot
//! be const.
static float s_colourMetric[3] = { 0.309f, 0.609f, 0.082f };

static bool IsDXTFormat( NGfx::EPixelFormat format )
{
	return format >= CF_DXT1 && format <= CF_DXT5;
}

static bool GetDDSPixelFormat( NGfx::EPixelFormat format, SDDSPixelFormat *pFormat )
{
	switch ( format )
	{
		case CF_DXT1:     *pFormat = DDSPF_DXT1;     break;
		case CF_DXT2:     *pFormat = DDSPF_DXT2;     break;
		case CF_DXT3:     *pFormat = DDSPF_DXT3;     break;
		case CF_DXT4:     *pFormat = DDSPF_DXT4;     break;
		case CF_DXT5:     *pFormat = DDSPF_DXT5;     break;
		case CF_A8R8G8B8: *pFormat = DDSPF_A8R8G8B8; break;
		case CF_A4R4G4B4: *pFormat = DDSPF_A4R4G4B4; break;
		case CF_A1R5G5B5: *pFormat = DDSPF_A1R5G5B5; break;
		case CF_R5G6B5:   *pFormat = DDSPF_R5G6B5;   break;
		default:
			return false;
	}
	return true;
}

//! Compression flags for one of the DXT formats.
static int GetSquishMethod( NGfx::EPixelFormat format )
{
	switch ( format )
	{
		case CF_DXT1:
			return squish::kDxt1;
		// DXT2 and DXT4 differ from DXT3 and DXT5 only in that the colour is
		// taken to be premultiplied by alpha. That is a claim the caller makes
		// about the source pixels, not a different block layout, so they encode
		// the same way. Premultiplying here would alter pixels the caller did
		// not ask to have altered, and nothing in the shipped data uses either
		// format.
		case CF_DXT2:
		case CF_DXT3:
			return squish::kDxt3;
		case CF_DXT4:
		case CF_DXT5:
			return squish::kDxt5;
		default:
			return 0;
	}
}

//! squish reads pixels as R,G,B,A bytes; CArray2D<uint32_t> holds 0xAARRGGBB,
//! which is B,G,R,A in memory on a little-endian machine.
//!
//! squish has a kSourceBGRA flag for exactly this, and it does not work: its
//! FixFlags() rebuilds the flag word as method|fit|kWeightColourByAlpha before
//! CompressImage passes it to CopyRGBA, so the bit is always stripped before
//! anything reads it. Passing it would silently swap red and blue in every
//! exported texture. Hence the explicit conversion.
static void SwizzleToRGBA( std::vector<squish::u8> *pRes, const CArray2D<uint32_t> &image )
{
	const int nSizeX = image.GetSizeX();
	const int nSizeY = image.GetSizeY();
	pRes->resize( nSizeX * nSizeY * 4 );
	squish::u8 *pDst = &(*pRes)[0];
	for ( int y = 0; y < nSizeY; ++y )
	{
		const uint32_t *pSrc = &image[y][0];
		for ( int x = 0; x < nSizeX; ++x, ++pSrc, pDst += 4 )
		{
			pDst[0] = static_cast<squish::u8>( ( *pSrc >> 16 ) & 0xff );	// R
			pDst[1] = static_cast<squish::u8>( ( *pSrc >>  8 ) & 0xff );	// G
			pDst[2] = static_cast<squish::u8>( ( *pSrc       ) & 0xff );	// B
			pDst[3] = static_cast<squish::u8>( ( *pSrc >> 24 ) & 0xff );	// A
		}
	}
}

//! One mip level, from ARGB8888 to the destination format.
static bool CompressLevel( std::vector<uint8_t> *pRes, const CArray2D<uint32_t> &image, NGfx::EPixelFormat format )
{
	const int nSizeX = image.GetSizeX();
	const int nSizeY = image.GetSizeY();
	if ( nSizeX <= 0 || nSizeY <= 0 )
		return false;

	if ( IsDXTFormat( format ) )
	{
		// kColourClusterFit is squish's default and the quality level the
		// exporter wants: this runs once per texture from the editor, not per
		// frame. kWeightColourByAlpha is deliberately not set -- it biases the
		// fit towards opaque texels, which helps alpha-blended art and hurts
		// everything else, and the original compressor did not do it.
		const int nFlags = GetSquishMethod( format ) | squish::kColourClusterFit;
		std::vector<squish::u8> rgba;
		SwizzleToRGBA( &rgba, image );
		pRes->resize( squish::GetStorageRequirements( nSizeX, nSizeY, nFlags ) );
		squish::CompressImage( &rgba[0], nSizeX, nSizeY, &(*pRes)[0], nFlags, s_colourMetric );
		return true;
	}

	if ( format == CF_A8R8G8B8 )
	{
		// The in-memory layout is already A8R8G8B8 little-endian, which is what
		// the pixel format in the header describes.
		pRes->resize( nSizeX * nSizeY * 4 );
		memcpy( &(*pRes)[0], &image[0][0], pRes->size() );
		return true;
	}

	SDDSPixelFormat ddsformat;
	if ( !GetDDSPixelFormat( format, &ddsformat ) )
		return false;
	// The 16 bit formats quantise by truncation, as the removed S3TC path did.
	SPixelConvertInfo pci( ddsformat.dwABitMask, ddsformat.dwRBitMask, ddsformat.dwGBitMask, ddsformat.dwBBitMask );
	pRes->resize( nSizeX * nSizeY * 2 );
	uint16_t *pDst = reinterpret_cast<uint16_t *>( &(*pRes)[0] );
	for ( int y = 0; y < nSizeY; ++y )
	{
		const uint32_t *pSrc = &image[y][0];
		for ( int x = 0; x < nSizeX; ++x, ++pSrc, ++pDst )
			*pDst = static_cast<uint16_t>( pci.ComposeColorSlow( *pSrc ) );
	}
	return true;
}

//! The 128 byte container header.
//!
//! The flag and caps combinations here are the ones the shipped textures carry:
//! of 11896 .dds files under Versions/Current, the mipmapped ones are
//! dwHeaderFlags 0x00021007 with dwSurfaceFlags 0x00401008 and the single level
//! ones are 0x00001007 with 0x00001000. dwPitchOrLinearSize is 0 throughout,
//! and both reserved blocks are zero in every one of them, so there is no tool
//! watermark to reproduce.
static void MakeDDSHeader( SDDSHeader *pHdr, int nWidth, int nHeight, NGfx::EPixelFormat ePixelFormat, int nNumMipLevels )
{
	GetDDSPixelFormat( ePixelFormat, &pHdr->ddspf );
	pHdr->dwWidth = nWidth;
	pHdr->dwHeight = nHeight;
	pHdr->dwHeaderFlags = DDS_HEADER_FLAGS_TEXTURE;
	pHdr->dwSurfaceFlags = DDS_SURFACE_FLAGS_TEXTURE;
	// A texture with one level announces no mip chain at all rather than a
	// chain of length one, which is what the shipped files do.
	if ( nNumMipLevels > 1 )
	{
		pHdr->dwHeaderFlags |= DDS_HEADER_FLAGS_MIPMAP;
		pHdr->dwSurfaceFlags |= DDS_SURFACE_FLAGS_MIPMAP;
		pHdr->dwMipMapCount = nNumMipLevels;
	}
	// DDSCAPS_ALPHA, a DirectDraw-era flag the shipped uncompressed textures
	// with an alpha channel carry and the DXT ones do not, alpha or no alpha.
	// Readers ignore it; it is set so a re-export of an existing texture differs
	// from it in as few bytes as possible.
	if ( !IsDXTFormat( ePixelFormat ) && ( pHdr->ddspf.dwFlags & DDS_ARGB ) == DDS_ARGB )
		pHdr->dwSurfaceFlags |= DDS_SURFACE_ALPHA;
}

// ************************************************************************************************************************ //
// **
// ** Image type aware mip generation
// **
// ** The mip chain is not a plain resize. What a texel means decides how it may
// ** be averaged, and averaging it wrongly shows up as haloes on cut-outs,
// ** flattened bumps at distance, and colour bleeding out of transparent
// ** regions. This is the original pipeline's treatment, restored: it was lost
// ** when the writer moved to D3DX, which filtered every texture the same way.
// **
// ************************************************************************************************************************ //

//! Prepares one level for compression according to what the image represents.
//!
//! Called per mip level rather than once on the source, because each of these
//! is a property of the level being compressed: normals have to come from that
//! level's own height field, and premultiplication has to happen after the
//! averaging that produced the level, not before it.
static void PrepareImageForCompression( CArray2D<CVec4> *pSrc, EImageType eImageType,
	bool bWrapX, bool bWrapY, float fMappingSize )
{
	switch ( eImageType )
	{
		case IMAGE_TYPE_PICTURE_FASTMIP:
		case IMAGE_TYPE_PICTURE:
			// Plain colour: the average of the texels is the answer.
			break;
		case IMAGE_TYPE_BUMP:
			// The source is a height field, and the normals come from the
			// heights of the level being written. Deriving them once at full
			// resolution and averaging those instead shortens every normal and
			// flattens the surface as it recedes.
			GenerateNormals( pSrc, CVec4( 1, 0, 0, 0 ), fMappingSize, bWrapX, bWrapY );
			break;
		case IMAGE_TYPE_TRANSPARENT:
			// Stored premultiplied, which is a convention the renderer's blend
			// mode expects, not a filtering trick: this runs after the level has
			// already been averaged, and GenerateMipLevel averages each channel
			// on its own. Colour from fully transparent texels therefore still
			// reaches the lower levels, as it did in the original pipeline.
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
			// Additive blending takes its weight from the colour, so alpha goes
			// to zero once it has been folded into the colour.
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
			ASSERT( 0 );
			break;
	}
}

static int CalcNumMipLevels( int nWidth, int nHeight, NGfx::EPixelFormat ePixelFormat, int nNumMipLevels );

//! Builds the whole chain in floating point, preparing each level as it goes.
//!
//! Each level is halved from the level above it and not from the original, and
//! the halving reads the level's *unprepared* pixels: `mip` keeps them while
//! `src` is the copy PrepareImageForCompression works on. Downsampling the
//! prepared image instead would premultiply twice and would average normals
//! rather than heights.
static void GenerateMipLevelsAndPrepareForCompression( std::vector<CArray2D<uint32_t> > *pMips,
	const CArray2D<CVec4> &srcImage, EImageType eImageType, NGfx::EPixelFormat ePixelFormat,
	int _nNumMipLevels, bool bWrapX, bool bWrapY, float fMappingSize )
{
	const int nNumMipLevels = CalcNumMipLevels( srcImage.GetSizeX(), srcImage.GetSizeY(), ePixelFormat, _nNumMipLevels );
	pMips->resize( nNumMipLevels );

	CArray2D<CVec4> mip( srcImage ), src( srcImage );
	for ( int i = 0; i < nNumMipLevels; ++i )
	{
		PrepareImageForCompression( &src, eImageType, bWrapX, bWrapY, fMappingSize );

		CArray2D<uint32_t> &dst = (*pMips)[i];
		dst.SetSizes( src.GetSizeX(), src.GetSizeY() );
		Convert( &dst, src );

		// Halve the untouched copy, then keep it for the next round.
		GenerateMipLevel( &src, mip, bWrapX, bWrapY );
		mip = src;
	}
}

static int CalcNumMipLevels( int nWidth, int nHeight, NGfx::EPixelFormat ePixelFormat, int nNumMipLevels )
{
	// Include the base level and allow the tail down to 1x1, including DXT blocks.
	const int nMaxPossible = GetMSB( (std::max)(nWidth, nHeight) ) + 1;
	return nNumMipLevels <= 0 ? nMaxPossible : (std::min)( nNumMipLevels, nMaxPossible );
}

static bool WriteDDS( const std::string &szFileName, NGfx::EPixelFormat ePixelFormat,
	const std::vector<CArray2D<uint32_t> > &mips )
{
	ASSERT( !mips.empty() );
	if ( mips.empty() )
		return false;

	SDDSPixelFormat ddsformat;
	if ( !GetDDSPixelFormat( ePixelFormat, &ddsformat ) )
	{
		NI_ASSERT( 0, fmt::format( "Unsupported destination format {}, DDS conversion failed (\"{}\")",
			static_cast<int>( ePixelFormat ), szFileName ) );
		return false;
	}

	// Compress everything before the file is created, so a failure leaves no
	// truncated .dds behind for the exporter to pick up as a finished one.
	std::vector<std::vector<uint8_t> > levels( mips.size() );
	for ( size_t i = 0; i < mips.size(); ++i )
	{
		if ( !CompressLevel( &levels[i], mips[i], ePixelFormat ) )
		{
			NI_ASSERT( 0, fmt::format( "Can't compress level {} of texture \"{}\", DDS conversion failed", i, szFileName ) );
			return false;
		}
	}

	SDDSFileHeader hdr;
	MakeDDSHeader( &hdr.header, mips[0].GetSizeX(), mips[0].GetSizeY(), ePixelFormat, mips.size() );

	NFile::CreatePath( NFile::GetFilePath( szFileName ) );
	CFileStream stream( szFileName, CFileStream::WIN_CREATE );
	if ( !stream.IsOk() )
	{
		NI_ASSERT( 0, fmt::format( "Can't create DDS file \"{}\"", szFileName ) );
		return false;
	}
	stream.Write( &hdr, sizeof(hdr) );
	for ( size_t i = 0; i < levels.size(); ++i )
		stream.Write( &levels[i][0], levels[i].size() );
	return stream.IsOk();
}

bool ConvertAndSaveAsDDS( const std::string &szFileName, const CArray2D<uint32_t> &srcImage,
	EImageType eImageType, NGfx::EPixelFormat nSubFormat, int nNumMipLevels, bool bWrapX, bool bWrapY, float fMappingSize )
{
	if ( srcImage.GetSizeX() <= 0 || srcImage.GetSizeY() <= 0 )
		return false;

	std::vector<CArray2D<uint32_t> > mips;

	if ( eImageType == IMAGE_TYPE_PICTURE_FASTMIP )
	{
		// What the name says: no per-level work, and every level resampled
		// straight from the full resolution original rather than from the level
		// above it, so the error does not accumulate down the chain. This is
		// the path the D3DX writer took for every texture regardless of type.
		const int nMips = CalcNumMipLevels( srcImage.GetSizeX(), srcImage.GetSizeY(), nSubFormat, nNumMipLevels );
		mips.resize( nMips );
		mips[0] = srcImage;
		for ( int nLevel = 1; nLevel < nMips; ++nLevel )
		{
			const int nSizeX = (std::max)( 1, srcImage.GetSizeX() >> nLevel );
			const int nSizeY = (std::max)( 1, srcImage.GetSizeY() >> nLevel );
			CArray2D<uint32_t> &image = mips[ nLevel ];
			image.SetSizes( nSizeX, nSizeY );
			Scale( &image, srcImage, IMAGE_SCALE_METHOD_LANCZOS3 );
		}
	}
	else
	{
		// Everything else goes through the type aware chain, in floating point
		// so that repeated halving does not quantise at every level.
		CArray2D<CVec4> trueImage;
		trueImage.SetSizes( srcImage.GetSizeX(), srcImage.GetSizeY() );
		Convert( &trueImage, srcImage );
		GenerateMipLevelsAndPrepareForCompression( &mips, trueImage, eImageType, nSubFormat,
			nNumMipLevels, bWrapX, bWrapY, fMappingSize );
	}

	return WriteDDS( szFileName, nSubFormat, mips );
}

}
