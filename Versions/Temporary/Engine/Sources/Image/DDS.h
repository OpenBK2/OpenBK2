#pragma once

#include <cstdint>

// This header defines constants and structures that are useful when parsing 
// DDS files.  DDS files were originally designed to use several structures
// and constants that are native to DirectDraw and are defined in ddraw.h,
// such as DDSURFACEDESC2 and DDSCAPS2.  This file defines similar 
// (compatible) constants and structures so that one can use DDS files 
// without needing to include ddraw.h.

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
		((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) |   \
		((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24 ))
#endif // MAKEFOURCC

// ************************************************************************************************************************ //
// **
// ** pixel format
// **
// **
// **
// ************************************************************************************************************************ //

struct SDDSPixelFormat
{
  uint32_t dwSize;													// size of this structure
  uint32_t dwFlags;												// flags (RGB, ARGB, DXT#)
  uint32_t dwFourCC;												// fourCC code (for DXT# formats)
  uint32_t dwRGBBitCount;									// ARGB bit count (for ARGB formats)
  uint32_t dwRBitMask;											// R bit mask  (for ARGB formats)
  uint32_t dwGBitMask;											// G bit mask  (for ARGB formats)
  uint32_t dwBBitMask;											// B bit mask  (for ARGB formats)
  uint32_t dwABitMask;											// A bit mask  (for ARGB formats)
};
inline const bool operator==( const SDDSPixelFormat &pf1, const SDDSPixelFormat &pf2 ) { return memcmp( &pf1, &pf2, sizeof(pf1) ) == 0; }

// ************************************************************************************************************************ //
// **
// ** pixel format flags and pre-crafted pixel formats
// **
// **
// **
// ************************************************************************************************************************ //

const uint32_t DDS_FOURCC	= 0x00000004;		// DDPF_FOURCC
const uint32_t DDS_RGB			= 0x00000040;		// DDPF_RGB
const uint32_t DDS_ARGB		= 0x00000041;		// DDPF_RGB | DDPF_ALPHAPIXELS

// DXT# formats
const SDDSPixelFormat DDSPF_DXT1 = { sizeof(SDDSPixelFormat), DDS_FOURCC, MAKEFOURCC('D','X','T','1'), 0, 0, 0, 0, 0 };
const SDDSPixelFormat DDSPF_DXT2 = { sizeof(SDDSPixelFormat), DDS_FOURCC, MAKEFOURCC('D','X','T','2'), 0, 0, 0, 0, 0 };
const SDDSPixelFormat DDSPF_DXT3 = { sizeof(SDDSPixelFormat), DDS_FOURCC, MAKEFOURCC('D','X','T','3'), 0, 0, 0, 0, 0 };
const SDDSPixelFormat DDSPF_DXT4 = { sizeof(SDDSPixelFormat), DDS_FOURCC, MAKEFOURCC('D','X','T','4'), 0, 0, 0, 0, 0 };
const SDDSPixelFormat DDSPF_DXT5 = { sizeof(SDDSPixelFormat), DDS_FOURCC, MAKEFOURCC('D','X','T','5'), 0, 0, 0, 0, 0 };
// ARGB formats
const SDDSPixelFormat DDSPF_A8R8G8B8 = { sizeof(SDDSPixelFormat), DDS_ARGB, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 };
const SDDSPixelFormat DDSPF_A1R5G5B5 = { sizeof(SDDSPixelFormat), DDS_ARGB, 0, 16, 0x00007c00, 0x000003e0, 0x0000001f, 0x00008000 };
const SDDSPixelFormat DDSPF_A4R4G4B4 = { sizeof(SDDSPixelFormat), DDS_ARGB, 0, 16, 0x0000f000, 0x000000f0, 0x0000000f, 0x0000f000 };
const SDDSPixelFormat DDSPF_R5G6B5   = { sizeof(SDDSPixelFormat), DDS_RGB , 0, 16, 0x0000f800, 0x000007e0, 0x0000001f, 0x00000000 };
const SDDSPixelFormat DDSPF_R8G8B8   = { sizeof(SDDSPixelFormat), DDS_RGB , 0, 24, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 };

// ************************************************************************************************************************ //
// **
// ** DDS format flags
// **
// **
// **
// ************************************************************************************************************************ //

// additional flags
// header flags
const uint32_t DDS_HEADER_FLAGS_TEXTURE    = 0x00001007;	// DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT
const uint32_t DDS_HEADER_FLAGS_MIPMAP     = 0x00020000;	// DDSD_MIPMAPCOUNT
const uint32_t DDS_HEADER_FLAGS_VOLUME     = 0x00800000;	// DDSD_DEPTH
const uint32_t DDS_HEADER_FLAGS_PITCH      = 0x00000008;	// DDSD_PITCH
const uint32_t DDS_HEADER_FLAGS_LINEARSIZE = 0x00080000;	// DDSD_LINEARSIZE
// surface flags
const uint32_t DDS_SURFACE_FLAGS_TEXTURE		= 0x00001000;	// DDSCAPS_TEXTURE
const uint32_t DDS_SURFACE_FLAGS_MIPMAP		= 0x00400008;	// DDSCAPS_COMPLEX | DDSCAPS_MIPMAP
const uint32_t DDS_SURFACE_FLAGS_CUBEMAP		= 0x00000008;	// DDSCAPS_COMPLEX
// cube map flags
const uint32_t DDS_CUBEMAP_POSITIVEX				= 0x00000600;	// DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
const uint32_t DDS_CUBEMAP_NEGATIVEX				= 0x00000a00;	// DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
const uint32_t DDS_CUBEMAP_POSITIVEY				= 0x00001200;	// DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
const uint32_t DDS_CUBEMAP_NEGATIVEY				= 0x00002200;	// DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
const uint32_t DDS_CUBEMAP_POSITIVEZ				= 0x00004200;	// DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
const uint32_t DDS_CUBEMAP_NEGATIVEZ				= 0x00008200;	// DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

const uint32_t DDS_CUBEMAP_ALLFACES				= DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX |
																					DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY | 
																					DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ;
// volume flags...
const uint32_t DDS_FLAGS_VOLUME						= 0x00200000;	// DDSCAPS2_VOLUME

// ************************************************************************************************************************ //
// **
// ** DDS header and DDS file header
// **
// **
// **
// ************************************************************************************************************************ //

struct SDDSHeader
{
	uint32_t dwSize;													// size of the structure
	uint32_t dwHeaderFlags;									// header flags
	uint32_t dwHeight;												// height of the texture biggest mip level
	uint32_t dwWidth;												// width  of the texture biggest mip level
	uint32_t dwPitchOrLinearSize;						// pitch (for ARGB formats) of linear size (for DXT# formats) of the biggest mip level
	uint32_t dwDepth;												// only if DDS_HEADER_FLAGS_VOLUME is set in dwHeaderFlags
	uint32_t dwMipMapCount;									// number of mip-map levels
	uint32_t dwReserved1[11];								//
	SDDSPixelFormat ddspf;								// pixel format descriptor
	uint32_t dwSurfaceFlags;									// surface flags
	uint32_t dwCubemapFlags;									// cube map flags
	uint32_t dwReserved2[3];									//
	//
	SDDSHeader() { Zero(*this); dwSize = sizeof( *this ); }
};
struct SDDSFileHeader
{
	enum { SIGNATURE = 0x20534444 };
	uint32_t dwSignature;										// .dds file signature
	SDDSHeader header;										// header itself
	//
	SDDSFileHeader() { dwSignature = SDDSFileHeader::SIGNATURE; }
};


