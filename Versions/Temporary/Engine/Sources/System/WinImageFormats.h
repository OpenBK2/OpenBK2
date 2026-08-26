#pragma once

#include "System_export.h"

#include <cstddef>
#include <cstdint>
#include <vector>

// Readers for the Windows binary image containers the game's data is stored in:
// RIFF, and the ICO container that .cur, .ani and .ico all share.
//
// These live here rather than inside the one caller that needed them first,
// because the formats are not a cursor concern. The same container holds the
// window icon, and a reader that only ever decodes cursors is a reader nobody
// looks in when they want an icon.
//
// Nothing here touches SDL or windows.h. Bytes in, pixels out; what the caller
// does with them is its own business. Every field is assembled a byte at a time
// rather than read through a cast, because these formats are little-endian
// whatever the host is and nothing in them is aligned.
namespace NWinImage
{

// What one directory entry holds, before deciding whether to decode it.
//
// The container is a list of the same image at several sizes and colour depths,
// which is the point of it: Win32 has both hIcon and hIconSmall for the same
// reason, that a 16x16 slot wants the 16x16 drawing rather than a 48x48 one
// squeezed down, and a large slot wants the large drawing rather than a small
// one blown up. Read the list, pick, then decode the one you picked.
struct SImageInfo
{
	int nWidth;
	int nHeight;
	// 1, 4 and 8 are palettes; 24 and 32 are direct colour. Anything else, and
	// any entry holding a PNG rather than a bitmap, reports bSupported false.
	int nBpp;
	// From a .cur. The .ico form of the container puts other things in those two
	// fields, so an icon reports 0,0 and the caller should not read these.
	int nHotX;
	int nHotY;
	// false when DecodeImage would refuse this entry. The entry is still listed,
	// because knowing an image is there and unreadable is worth more than a gap.
	bool bSupported;

	SImageInfo() : nWidth( 0 ), nHeight( 0 ), nBpp( 0 ), nHotX( 0 ), nHotY( 0 ), bSupported( false ) { }
};

// One decoded image. 32-bit ARGB, top-down, nWidth pixels per row, so
// pixels[y * nWidth + x] with no padding between rows.
struct SImage
{
	int nWidth;
	int nHeight;
	int nHotX;
	int nHotY;
	std::vector<uint32_t> pixels;

	SImage() : nWidth( 0 ), nHeight( 0 ), nHotX( 0 ), nHotY( 0 ) { }
};

// The offset of the first complete cursor inside a .ani.
//
// A .ani is a RIFF ACON whose "fram" list holds one "icon" chunk per frame, and
// each of those chunks is a whole .cur. Only the first frame is reported: it is
// what a caller with no animation to drive can use, and finding the rest is a
// separate piece of work.
//
// False when the data is not a RIFF ACON, or has no frame list, in which case
// the caller should treat the file as a plain .cur or .ico from offset zero.
SYSTEM_EXPORT bool FindFirstAniFrame( size_t *pnOffset, const uint8_t *pData, size_t nSize );

// Read the directory of the ICO container at nBase, without decoding anything.
// False when the data is not that container.
SYSTEM_EXPORT bool GetImages( std::vector<SImageInfo> *pImages, const uint8_t *pData,
                              size_t nSize, size_t nBase );

// Which entry to draw at nDesiredSize pixels, or -1 when none can be decoded.
//
// An exact match wins. Otherwise the smallest entry larger than asked for, since
// giving a scaler more detail than it needs beats asking it to invent some.
// Otherwise the largest there is. Ties go to the higher colour depth, which is
// how a 48x48 at 8bpp is preferred over the 48x48 at 4bpp beside it.
SYSTEM_EXPORT int SelectImage( const std::vector<SImageInfo> &images, int nDesiredSize );

// Decode one entry of the container at nBase.
//
// The entry is a device independent bitmap: a BITMAPINFOHEADER, an optional
// palette, a bottom-up colour bitmap of double the stated height, and then a
// 1bpp AND mask covering the other half.
//
// That mask is the transparency. At 32bpp the bitmap may also carry a real alpha
// channel, and where it does that is used instead, because a file with alpha
// leaves the mask zeroed and honouring the mask alone would make it opaque.
//
// False for an index that is out of range, an entry that is compressed, one
// holding a PNG, or one at a bit depth this does not read.
SYSTEM_EXPORT bool DecodeImage( SImage *pResult, const uint8_t *pData, size_t nSize,
                                size_t nBase, int nIndex );

}
