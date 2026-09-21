// Covers the DDS writer, which replaced a D3DX one that only existed on Windows.
//
// Two things are worth guarding here and they are guarded differently.
//
// The container is exactly reproducible, so it is asserted byte for byte. The
// flag and caps combinations come from the shipped data rather than from the
// DDS documentation: of the 11896 .dds files under Versions/Current, the
// mipmapped ones carry dwHeaderFlags 0x00021007 with dwSurfaceFlags 0x00401008,
// the single level ones 0x00001007 with 0x00001000, uncompressed formats with
// an alpha channel add DDSCAPS_ALPHA and the DXT ones never do, and
// dwPitchOrLinearSize is 0 throughout. ShippedHeadersMatch puts that to the
// real files instead of restating it.
//
// The compressed payload is not reproducible and cannot be asserted that way.
// BC1/BC2/BC3 decoding is exactly specified, but choosing two endpoints and
// sixteen indices per block is a search with no canonical answer: every encoder
// differs, and this one differs from both the S3TC library the original
// pipeline used and the D3DX call that replaced it. So the payload is held to a
// decoded error bound and to the structural properties that are exact -- sizes,
// the lossless path, and punch-through alpha surviving.
//
// The decoder used to measure all of that is squish's, not the engine's
// UnpackDXT; see DecodeLevel0 for why that distinction cost a wrong result
// before it was noticed.

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <list>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Neither 2Darray.h nor GPixelFormat.h is self-contained: the first names
// IBinSaver and the second Float2Int and CVec4, and both expect a stdafx to
// have supplied them. This is that prelude, in the order Image/stdafx.h uses.
#include "Misc/Asserts.h"
#include "System/System.h"
#include "Misc/Tools.h"
#include "System/Basic.h"
#include "Misc/Geom.h"
#include "System/Streams.h"
#include "System/BinSaver.h"
#include "Misc/2Darray.h"
#include "Image/DDS.h"
#include "Image/Image.h"
#include "Image/ImageDDS.h"
#include "Image/GUnpackDXT.h"
#include <squish/squish.h>
#include "3Dmotor/GPixelFormat.h"

#include <gtest/gtest.h>

namespace {

std::filesystem::path TempFile( const char *pszName )
{
	return std::filesystem::temp_directory_path() / pszName;
}

//! A deterministic image with enough structure that a block compressor has to
//! work: smooth gradients, a hard edge and some noise.
CArray2D<uint32_t> MakeImage( int nSizeX, int nSizeY, bool bWithAlpha )
{
	CArray2D<uint32_t> image;
	image.SetSizes( nSizeX, nSizeY );
	for ( int y = 0; y < nSizeY; ++y )
	{
		for ( int x = 0; x < nSizeX; ++x )
		{
			const uint32_t r = static_cast<uint32_t>( x * 255 / (std::max)( 1, nSizeX - 1 ) );
			const uint32_t g = static_cast<uint32_t>( y * 255 / (std::max)( 1, nSizeY - 1 ) );
			const uint32_t b = static_cast<uint32_t>( ( ( x ^ y ) & 0x1f ) * 8 );
			const uint32_t a = bWithAlpha ? static_cast<uint32_t>( ( x + y ) & 0xff ) : 0xffu;
			image[y][x] = ( a << 24 ) | ( r << 16 ) | ( g << 8 ) | b;
		}
	}
	return image;
}

std::vector<uint8_t> ReadFile( const std::filesystem::path &path )
{
	std::vector<uint8_t> res;
	FILE *pFile = fopen( path.string().c_str(), "rb" );
	if ( !pFile )
		return res;
	fseek( pFile, 0, SEEK_END );
	res.resize( static_cast<size_t>( ftell( pFile ) ) );
	fseek( pFile, 0, SEEK_SET );
	if ( !res.empty() )
	{
		if ( fread( &res[0], 1, res.size(), pFile ) != res.size() )
			res.clear();
	}
	fclose( pFile );
	return res;
}

const SDDSHeader &HeaderOf( const std::vector<uint8_t> &bytes )
{
	return reinterpret_cast<const SDDSFileHeader *>( &bytes[0] )->header;
}

int BlockBytes( NGfx::EPixelFormat format )
{
	return format == NGfx::CF_DXT1 ? 8 : 16;
}

size_t ExpectedSize( int nSizeX, int nSizeY, NGfx::EPixelFormat format, int nMips )
{
	size_t nTotal = 0;
	for ( int i = 0; i < nMips; ++i )
	{
		const int w = (std::max)( 1, nSizeX >> i );
		const int h = (std::max)( 1, nSizeY >> i );
		if ( format >= NGfx::CF_DXT1 && format <= NGfx::CF_DXT5 )
			nTotal += static_cast<size_t>( ( w + 3 ) / 4 ) * ( ( h + 3 ) / 4 ) * BlockBytes( format );
		else
			nTotal += static_cast<size_t>( w ) * h * ( NGfx::GetBPP( format ) / 8 );
	}
	return nTotal;
}

//! Root mean square error per channel between two ARGB images.
double RMSE( const CArray2D<uint32_t> &a, const CArray2D<uint32_t> &b )
{
	EXPECT_EQ( a.GetSizeX(), b.GetSizeX() );
	EXPECT_EQ( a.GetSizeY(), b.GetSizeY() );
	double fSum = 0;
	int nCount = 0;
	for ( int y = 0; y < a.GetSizeY(); ++y )
	{
		for ( int x = 0; x < a.GetSizeX(); ++x )
		{
			for ( int nShift = 0; nShift <= 16; nShift += 8 )
			{
				const double d = static_cast<double>( ( a[y][x] >> nShift ) & 0xff )
				               - static_cast<double>( ( b[y][x] >> nShift ) & 0xff );
				fSum += d * d;
				++nCount;
			}
		}
	}
	return std::sqrt( fSum / (std::max)( 1, nCount ) );
}

//! Writes the image through the writer under test and reads the file back.
std::vector<uint8_t> WriteAndRead( const char *pszName, const CArray2D<uint32_t> &image,
	NGfx::EPixelFormat format, int nMips )
{
	const std::filesystem::path path = TempFile( pszName );
	std::error_code ec;
	std::filesystem::remove( path, ec );
	EXPECT_TRUE( NImage::ConvertAndSaveAsDDS( path.string(), image, NImage::IMAGE_TYPE_PICTURE,
		format, nMips, true, true, 1024.0f ) );
	std::vector<uint8_t> bytes = ReadFile( path );
	std::filesystem::remove( path, ec );
	return bytes;
}

//! Decodes mip 0 with squish rather than with the engine's own UnpackDXT.
//!
//! UnpackDXT cannot be used to measure an encoder, because it does not decode
//! DXT correctly: it widens the 5 and 6 bit endpoints to 8 bits by shifting
//! left (`col_0->r <<= 3`) instead of replicating the high bits into the low
//! ones, so the top of each channel's range is unreachable and 0xffff -- pure
//! white -- comes back as 248,252,248. Measuring against it charges this
//! encoder for that error: re-encoding a shipped white texture scored 6.0 RMSE
//! through UnpackDXT and the whole of it was the decoder. BC1/BC2/BC3 decoding
//! is exactly specified and squish implements the specified form, so it is the
//! reference here. UnpackDXT's own behaviour is a separate bug.
void DecodeLevel0( CArray2D<uint32_t> *pRes, const std::vector<uint8_t> &bytes, NGfx::EPixelFormat format )
{
	const SDDSHeader &hdr = HeaderOf( bytes );
	const int nWidth = static_cast<int>( hdr.dwWidth );
	const int nHeight = static_cast<int>( hdr.dwHeight );
	const int nFlags = ( format == NGfx::CF_DXT1 ) ? squish::kDxt1
	                 : ( format == NGfx::CF_DXT3 ) ? squish::kDxt3 : squish::kDxt5;

	std::vector<squish::u8> rgba( static_cast<size_t>( nWidth ) * nHeight * 4 );
	squish::DecompressImage( &rgba[0], nWidth, nHeight, &bytes[sizeof(SDDSFileHeader)], nFlags );

	pRes->SetSizes( nWidth, nHeight );
	for ( int y = 0; y < nHeight; ++y )
	{
		for ( int x = 0; x < nWidth; ++x )
		{
			const squish::u8 *p = &rgba[( static_cast<size_t>( y ) * nWidth + x ) * 4];
			(*pRes)[y][x] = ( static_cast<uint32_t>( p[3] ) << 24 ) | ( static_cast<uint32_t>( p[0] ) << 16 )
			              | ( static_cast<uint32_t>( p[1] ) << 8 ) | static_cast<uint32_t>( p[2] );
		}
	}
}

} // namespace

TEST( DDSWrite, Signature )
{
	const std::vector<uint8_t> bytes = WriteAndRead( "obk2_dds_sig.dds", MakeImage( 32, 32, false ), NGfx::CF_DXT1, 1 );
	ASSERT_GE( bytes.size(), sizeof(SDDSFileHeader) );
	EXPECT_EQ( 0x20534444u, reinterpret_cast<const SDDSFileHeader *>( &bytes[0] )->dwSignature );
	EXPECT_EQ( 124u, HeaderOf( bytes ).dwSize );
	EXPECT_EQ( 32u, sizeof(SDDSPixelFormat) );
	EXPECT_EQ( 128u, sizeof(SDDSFileHeader) );
}

// A single level texture announces no mip chain at all rather than a chain of
// length one, which is what the shipped single level files do.
TEST( DDSWrite, HeaderSingleLevel )
{
	const std::vector<uint8_t> bytes = WriteAndRead( "obk2_dds_one.dds", MakeImage( 64, 64, false ), NGfx::CF_DXT1, 1 );
	ASSERT_GE( bytes.size(), sizeof(SDDSFileHeader) );
	const SDDSHeader &hdr = HeaderOf( bytes );
	EXPECT_EQ( 0x00001007u, hdr.dwHeaderFlags );
	EXPECT_EQ( 0x00001000u, hdr.dwSurfaceFlags );
	EXPECT_EQ( 0u, hdr.dwMipMapCount );
	EXPECT_EQ( 0u, hdr.dwPitchOrLinearSize );
	EXPECT_EQ( 0u, hdr.dwCubemapFlags );
}

TEST( DDSWrite, HeaderMipped )
{
	const std::vector<uint8_t> bytes = WriteAndRead( "obk2_dds_mip.dds", MakeImage( 64, 64, false ), NGfx::CF_DXT1, 4 );
	ASSERT_GE( bytes.size(), sizeof(SDDSFileHeader) );
	const SDDSHeader &hdr = HeaderOf( bytes );
	EXPECT_EQ( 0x00021007u, hdr.dwHeaderFlags );
	EXPECT_EQ( 0x00401008u, hdr.dwSurfaceFlags );
	EXPECT_EQ( 4u, hdr.dwMipMapCount );
	EXPECT_EQ( 0u, hdr.dwPitchOrLinearSize );
	EXPECT_EQ( MAKEFOURCC('D','X','T','1'), hdr.ddspf.dwFourCC );
}

// DDSCAPS_ALPHA goes on uncompressed formats that carry alpha and on no DXT
// format, alpha or no alpha. Readers ignore it; it is set so that re-exporting
// an existing texture differs from it in as few bytes as possible.
TEST( DDSWrite, HeaderAlphaCapsOnlyForUncompressed )
{
	const std::vector<uint8_t> argb = WriteAndRead( "obk2_dds_argb.dds", MakeImage( 32, 32, true ), NGfx::CF_A8R8G8B8, 1 );
	ASSERT_GE( argb.size(), sizeof(SDDSFileHeader) );
	EXPECT_EQ( 0x00001002u, HeaderOf( argb ).dwSurfaceFlags );

	// DXT3 has an alpha channel and still does not get the flag.
	const std::vector<uint8_t> dxt3 = WriteAndRead( "obk2_dds_dxt3.dds", MakeImage( 32, 32, true ), NGfx::CF_DXT3, 1 );
	ASSERT_GE( dxt3.size(), sizeof(SDDSFileHeader) );
	EXPECT_EQ( 0x00001000u, HeaderOf( dxt3 ).dwSurfaceFlags );
}

TEST( DDSWrite, MipChainSizes )
{
	struct { int w, h; NGfx::EPixelFormat fmt; int mips; } cases[] = {
		{  64,  64, NGfx::CF_DXT1,     4 },
		{  64,  64, NGfx::CF_DXT3,     4 },
		{ 256, 128, NGfx::CF_DXT1,     9 },
		{  32,  32, NGfx::CF_A8R8G8B8, 1 },
		{  16,  16, NGfx::CF_R5G6B5,   5 },
	};
	for ( const auto &c : cases )
	{
		const std::vector<uint8_t> bytes = WriteAndRead( "obk2_dds_chain.dds", MakeImage( c.w, c.h, true ), c.fmt, c.mips );
		EXPECT_EQ( sizeof(SDDSFileHeader) + ExpectedSize( c.w, c.h, c.fmt, c.mips ), bytes.size() )
			<< c.w << "x" << c.h << " format " << static_cast<int>( c.fmt ) << " mips " << c.mips;
	}
}

// A8R8G8B8 is a copy, not a compression, so it has to come back bit for bit.
TEST( DDSWrite, A8R8G8B8IsLossless )
{
	const CArray2D<uint32_t> src = MakeImage( 37, 23, true );
	const std::vector<uint8_t> bytes = WriteAndRead( "obk2_dds_lossless.dds", src, NGfx::CF_A8R8G8B8, 1 );
	ASSERT_EQ( sizeof(SDDSFileHeader) + 37u * 23u * 4u, bytes.size() );
	const uint32_t *pPixels = reinterpret_cast<const uint32_t *>( &bytes[sizeof(SDDSFileHeader)] );
	for ( int y = 0; y < src.GetSizeY(); ++y )
	{
		for ( int x = 0; x < src.GetSizeX(); ++x )
			ASSERT_EQ( src[y][x], pPixels[y * src.GetSizeX() + x] ) << "at " << x << "," << y;
	}
}

TEST( DDSWrite, Dxt1RoundTripQuality )
{
	const CArray2D<uint32_t> src = MakeImage( 64, 64, false );
	const std::vector<uint8_t> bytes = WriteAndRead( "obk2_dds_q1.dds", src, NGfx::CF_DXT1, 1 );
	ASSERT_EQ( sizeof(SDDSFileHeader) + ExpectedSize( 64, 64, NGfx::CF_DXT1, 1 ), bytes.size() );
	CArray2D<uint32_t> decoded;
	DecodeLevel0( &decoded, bytes, NGfx::CF_DXT1 );
	// 4 bits per pixel over a deliberately awkward image. The bound is loose
	// enough not to pin the encoder's exact choices and tight enough that a
	// swapped channel or a misplaced endpoint blows straight through it.
	EXPECT_LT( RMSE( src, decoded ), 12.0 );
}

TEST( DDSWrite, Dxt3RoundTripQuality )
{
	const CArray2D<uint32_t> src = MakeImage( 64, 64, true );
	const std::vector<uint8_t> bytes = WriteAndRead( "obk2_dds_q3.dds", src, NGfx::CF_DXT3, 1 );
	CArray2D<uint32_t> decoded;
	DecodeLevel0( &decoded, bytes, NGfx::CF_DXT3 );
	EXPECT_LT( RMSE( src, decoded ), 12.0 );
}

// Red and blue swapped is the failure this catches, and it is the one the
// library invites: squish reads R,G,B,A bytes while CArray2D<uint32_t> is
// B,G,R,A, and squish's own kSourceBGRA flag does not work -- FixFlags() drops
// it before anything reads it. A flat colour leaves the compressor no freedom,
// so the channels have to come back where they went in.
TEST( DDSWrite, ChannelOrderSurvives )
{
	CArray2D<uint32_t> src;
	src.SetSizes( 16, 16 );
	for ( int y = 0; y < 16; ++y )
	{
		for ( int x = 0; x < 16; ++x )
			src[y][x] = 0xffc03010u;	// strongly red, mid green, little blue
	}
	const std::vector<uint8_t> bytes = WriteAndRead( "obk2_dds_chan.dds", src, NGfx::CF_DXT1, 1 );
	CArray2D<uint32_t> decoded;
	DecodeLevel0( &decoded, bytes, NGfx::CF_DXT1 );
	const uint32_t nPixel = decoded[8][8];
	const int r = ( nPixel >> 16 ) & 0xff;
	const int g = ( nPixel >> 8 ) & 0xff;
	const int b = nPixel & 0xff;
	EXPECT_GT( r, 0xa0 ) << "red lost";
	EXPECT_GT( g, 0x18 ) << "green lost";
	EXPECT_LT( g, 0x48 );
	EXPECT_LT( b, 0x30 ) << "blue too high, channels are probably swapped";
	EXPECT_GT( r, b ) << "red and blue swapped";
}

// DXT1 carries one bit of alpha in its three-colour block mode, and the shipped
// data uses it: 0.9% of the blocks sampled across 1500 shipped DXT1 files hold
// at least one transparent texel. An encoder that only ever emits four-colour
// blocks would compile, pass every other test here, and quietly turn cut-outs
// opaque.
TEST( DDSWrite, Dxt1PunchThroughAlpha )
{
	CArray2D<uint32_t> src;
	src.SetSizes( 16, 16 );
	for ( int y = 0; y < 16; ++y )
	{
		for ( int x = 0; x < 16; ++x )
			src[y][x] = ( x < 8 ) ? 0x00000000u : 0xff3080c0u;
	}
	const std::vector<uint8_t> bytes = WriteAndRead( "obk2_dds_punch.dds", src, NGfx::CF_DXT1, 1 );
	ASSERT_GE( bytes.size(), sizeof(SDDSFileHeader) + 8u );

	bool bAnyThreeColour = false;
	const uint8_t *pBlocks = &bytes[sizeof(SDDSFileHeader)];
	const size_t nBlocks = ( bytes.size() - sizeof(SDDSFileHeader) ) / 8;
	for ( size_t i = 0; i < nBlocks; ++i )
	{
		const uint16_t c0 = static_cast<uint16_t>( pBlocks[i*8+0] | ( pBlocks[i*8+1] << 8 ) );
		const uint16_t c1 = static_cast<uint16_t>( pBlocks[i*8+2] | ( pBlocks[i*8+3] << 8 ) );
		if ( c0 <= c1 )
			bAnyThreeColour = true;
	}
	EXPECT_TRUE( bAnyThreeColour ) << "no three-colour block emitted for a half transparent image";

	CArray2D<uint32_t> decoded;
	DecodeLevel0( &decoded, bytes, NGfx::CF_DXT1 );
	EXPECT_EQ( 0u, decoded[8][2] >> 24 ) << "transparent half came back opaque";
	EXPECT_NE( 0u, decoded[8][13] >> 24 ) << "opaque half came back transparent";
}

// Quality, measured against the encoder that actually produced this game's
// textures rather than against a number someone picked.
//
// The trick is that a DXT-decoded image is exactly representable in DXT: every
// block holds at most four colours drawn from two endpoints, which is precisely
// what a block can encode. So decoding a shipped file and compressing the
// result again should come back almost unchanged, and how far it misses is how
// much worse this encoder is at finding a fit than the S3TC library Nival used.
// A bound of 1.0 RMSE, against a channel range of 255, leaves room for the odd
// block the cluster fit rounds differently and none at all for an encoder that
// is simply worse.
TEST( DDSWrite, ReEncodingShippedIsNearLossless )
{
	const std::filesystem::path root = std::filesystem::path( OBK2_DATA_DIR );
	if ( !std::filesystem::exists( root ) )
		GTEST_SKIP() << "no game data at " << root.string();

	int nChecked = 0;
	double fWorst = 0;
	std::string szWorst;
	std::error_code ec;
	for ( std::filesystem::recursive_directory_iterator it( root, ec ), end; it != end && nChecked < 40; it.increment( ec ) )
	{
		if ( ec || !it->is_regular_file( ec ) )
			continue;
		const std::filesystem::path &p = it->path();
		if ( p.extension() != ".dds" && p.extension() != ".DDS" )
			continue;
		const std::vector<uint8_t> bytes = ReadFile( p );
		if ( bytes.size() < sizeof(SDDSFileHeader) )
			continue;
		const SDDSHeader &hdr = HeaderOf( bytes );
		if ( hdr.dwWidth > 128 || hdr.dwHeight > 128 || !( hdr.ddspf.dwFlags & DDS_FOURCC ) )
			continue;
		NGfx::EPixelFormat format;
		if ( hdr.ddspf.dwFourCC == MAKEFOURCC('D','X','T','1') )
			format = NGfx::CF_DXT1;
		else if ( hdr.ddspf.dwFourCC == MAKEFOURCC('D','X','T','3') )
			format = NGfx::CF_DXT3;
		else
			continue;

		// What the original encoder's blocks decode to.
		CArray2D<uint32_t> original;
		DecodeLevel0( &original, bytes, format );

		// Compress that again and decode it back.
		const std::vector<uint8_t> ours = WriteAndRead( "obk2_dds_requant.dds", original, format, 1 );
		ASSERT_GE( ours.size(), sizeof(SDDSFileHeader) ) << p.string();
		CArray2D<uint32_t> again;
		DecodeLevel0( &again, ours, format );

		const double fError = RMSE( original, again );
		if ( fError > fWorst )
		{
			fWorst = fError;
			szWorst = p.filename().string();
		}
		EXPECT_LT( fError, 1.0 ) << "re-encoding " << p.string() << " lost more than rounding";
		++nChecked;
	}
	EXPECT_GT( nChecked, 0 ) << "no shipped DXT files were examined";
	std::printf( "[          ] %d files, worst RMSE %.4f (%s)\n", nChecked, fWorst, szWorst.c_str() );
}

// The strongest form of the check above, on the simplest possible input: a flat
// texture leaves the compressor no freedom, so re-encoding one has to land on
// the very block the original S3TC encoder chose. It does -- c0 0xffff,
// c1 0xf7ff, every index 2 -- which is worth asserting rather than just
// measuring, because it says this encoder agrees with the one that built the
// game's textures and not merely that it is close.
TEST( DDSWrite, ReEncodingAFlatTextureReproducesTheOriginalBlock )
{
	const std::filesystem::path p = std::filesystem::path( OBK2_DATA_DIR ) / "aidebuginfo" / "white.dds";
	if ( !std::filesystem::exists( p ) )
		GTEST_SKIP() << "no " << p.string();
	const std::vector<uint8_t> bytes = ReadFile( p );
	ASSERT_GT( bytes.size(), sizeof(SDDSFileHeader) );
	CArray2D<uint32_t> original;
	DecodeLevel0( &original, bytes, NGfx::CF_DXT1 );

	const std::vector<uint8_t> ours = WriteAndRead( "obk2_dds_flat.dds", original, NGfx::CF_DXT1, 1 );
	ASSERT_GE( ours.size(), sizeof(SDDSFileHeader) + 8u );

	// Byte for byte against the shipped block, which is the whole point.
	EXPECT_EQ( 0, memcmp( &bytes[sizeof(SDDSFileHeader)], &ours[sizeof(SDDSFileHeader)], 8 ) )
		<< "flat block differs from the one S3TC chose";

	CArray2D<uint32_t> again;
	DecodeLevel0( &again, ours, NGfx::CF_DXT1 );
	EXPECT_DOUBLE_EQ( 0.0, RMSE( original, again ) );
}

// The engine's own decoder, against the reference, on the real textures.
//
// UnpackDXT had two faults and both are fixed: it widened the 5 and 6 bit
// endpoints by shifting rather than replicating the high bits, so pure white
// decoded to 248,252,248 and every DXT image came back short of contrast; and
// it applied DXT1's punch-through rule to DXT3 and DXT5, which have no such
// mode, so index 2 got the midpoint instead of the two thirds point and index 3
// went black. The second one bit flat blocks, where the two endpoints are equal
// and so "not greater than", which is 23% of the blocks in the shipped DXT3
// textures.
//
// Both decoders now use the same interpolation and the same 4 bit alpha
// expansion, so agreement is exact rather than approximate, and asserting
// equality catches a regression in either direction.
TEST( DDSWrite, UnpackDXTMatchesReference )
{
	const std::filesystem::path root = std::filesystem::path( OBK2_DATA_DIR );
	if ( !std::filesystem::exists( root ) )
		GTEST_SKIP() << "no game data at " << root.string();

	int nDxt1 = 0;
	int nDxt3 = 0;
	std::error_code ec;
	for ( std::filesystem::recursive_directory_iterator it( root, ec ), end;
		it != end && ( nDxt1 < 60 || nDxt3 < 60 ); it.increment( ec ) )
	{
		if ( ec || !it->is_regular_file( ec ) )
			continue;
		const std::filesystem::path &p = it->path();
		if ( p.extension() != ".dds" && p.extension() != ".DDS" )
			continue;
		const std::vector<uint8_t> bytes = ReadFile( p );
		if ( bytes.size() < sizeof(SDDSFileHeader) )
			continue;
		const SDDSHeader &hdr = HeaderOf( bytes );
		if ( !( hdr.ddspf.dwFlags & DDS_FOURCC ) || hdr.dwWidth > 256 || hdr.dwHeight > 256 )
			continue;
		// UnpackDXT walks whole 4x4 blocks and has no edge handling.
		if ( ( hdr.dwWidth % 4 ) != 0 || ( hdr.dwHeight % 4 ) != 0 )
			continue;

		NGfx::EPixelFormat format;
		if ( hdr.ddspf.dwFourCC == MAKEFOURCC('D','X','T','1') && nDxt1 < 60 )
		{
			format = NGfx::CF_DXT1;
			++nDxt1;
		}
		else if ( hdr.ddspf.dwFourCC == MAKEFOURCC('D','X','T','3') && nDxt3 < 60 )
		{
			format = NGfx::CF_DXT3;
			++nDxt3;
		}
		else
		{
			continue;
		}

		CArray2D<uint32_t> reference;
		DecodeLevel0( &reference, bytes, format );

		CArray2D<uint32_t> engine;
		NImage::UnpackDXT( static_cast<int>( format ), hdr.dwWidth, hdr.dwHeight,
			&bytes[sizeof(SDDSFileHeader)], &engine );

		ASSERT_EQ( reference.GetSizeX(), engine.GetSizeX() ) << p.string();
		ASSERT_EQ( reference.GetSizeY(), engine.GetSizeY() ) << p.string();
		for ( int y = 0; y < reference.GetSizeY(); ++y )
		{
			for ( int x = 0; x < reference.GetSizeX(); ++x )
			{
				ASSERT_EQ( reference[y][x], engine[y][x] )
					<< p.string() << " at " << x << "," << y;
			}
		}
	}
	EXPECT_GT( nDxt1, 0 ) << "no DXT1 files examined";
	EXPECT_GT( nDxt3, 0 ) << "no DXT3 files examined";
}

// Pure white is the value the old shift could not reach: 0xffff decoded to
// 248,252,248 rather than to white. Named separately from the sweep above
// because it is the one case anyone can check by eye.
TEST( DDSWrite, UnpackDXTReachesFullWhite )
{
	CArray2D<uint32_t> src;
	src.SetSizes( 4, 4 );
	for ( int y = 0; y < 4; ++y )
	{
		for ( int x = 0; x < 4; ++x )
			src[y][x] = 0xffffffffu;
	}
	const std::vector<uint8_t> bytes = WriteAndRead( "obk2_dds_white.dds", src, NGfx::CF_DXT1, 1 );
	ASSERT_GE( bytes.size(), sizeof(SDDSFileHeader) + 8u );
	CArray2D<uint32_t> engine;
	NImage::UnpackDXT( 1, 4, 4, &bytes[sizeof(SDDSFileHeader)], &engine );
	EXPECT_EQ( 0xffu, ( engine[0][0] >> 16 ) & 0xff ) << "red short of full range";
	EXPECT_EQ( 0xffu, ( engine[0][0] >> 8 ) & 0xff ) << "green short of full range";
	EXPECT_EQ( 0xffu, engine[0][0] & 0xff ) << "blue short of full range";
}

// The header rule above is taken from the shipped data, so it is checked
// against the shipped data. Every file whose header is in that dialect --
// 11642 of 11896 at the time of writing, the rest being a handful written by
// other tools with PITCH or LINEARSIZE set -- must be reproduced exactly by
// what this writer emits for the same size, format and mip count.
TEST( DDSWrite, ShippedHeadersMatch )
{
	const std::filesystem::path root = std::filesystem::path( OBK2_DATA_DIR );
	if ( !std::filesystem::exists( root ) )
		GTEST_SKIP() << "no game data at " << root.string();

	int nChecked = 0;
	int nSkippedDialect = 0;
	std::error_code ec;
	for ( std::filesystem::recursive_directory_iterator it( root, ec ), end; it != end && nChecked < 150; it.increment( ec ) )
	{
		if ( ec || !it->is_regular_file( ec ) )
			continue;
		const std::filesystem::path &p = it->path();
		if ( p.extension() != ".dds" && p.extension() != ".DDS" )
			continue;
		const std::vector<uint8_t> bytes = ReadFile( p );
		if ( bytes.size() < sizeof(SDDSFileHeader) )
			continue;
		const SDDSHeader &hdr = HeaderOf( bytes );
		// Keep the files to something a block compressor can chew through
		// quickly; the header rule does not vary with size.
		if ( hdr.dwWidth > 64 || hdr.dwHeight > 64 )
			continue;
		// The minority dialects set PITCH or LINEARSIZE. They are not what this
		// writer emits and not what the bulk of the data looks like.
		if ( ( hdr.dwHeaderFlags & ( DDS_HEADER_FLAGS_PITCH | DDS_HEADER_FLAGS_LINEARSIZE ) ) != 0 )
		{
			++nSkippedDialect;
			continue;
		}

		NGfx::EPixelFormat format = NGfx::CF_A8R8G8B8;
		if ( hdr.ddspf.dwFlags & DDS_FOURCC )
		{
			if ( hdr.ddspf.dwFourCC == MAKEFOURCC('D','X','T','1') )
				format = NGfx::CF_DXT1;
			else if ( hdr.ddspf.dwFourCC == MAKEFOURCC('D','X','T','3') )
				format = NGfx::CF_DXT3;
			else
				continue;
		}
		else if ( hdr.ddspf.dwRGBBitCount != 32 )
		{
			continue;
		}

		const int nMips = (std::max)( 1u, hdr.dwMipMapCount );
		const std::vector<uint8_t> ours = WriteAndRead( "obk2_dds_corpus.dds",
			MakeImage( hdr.dwWidth, hdr.dwHeight, true ), format, nMips );
		ASSERT_GE( ours.size(), sizeof(SDDSFileHeader) ) << p.string();
		EXPECT_EQ( 0, memcmp( &bytes[0], &ours[0], sizeof(SDDSFileHeader) ) )
			<< "header differs from shipped " << p.string();
		EXPECT_EQ( bytes.size(), ours.size() ) << "payload length differs from shipped " << p.string();
		++nChecked;
	}
	EXPECT_GT( nChecked, 0 ) << "no shipped .dds were examined";
}
