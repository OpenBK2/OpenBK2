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
//
// What is deliberately not here yet: entry selection and the bit depths other
// than 8, 24 and 32. This is the extraction of what already existed, unchanged.
namespace NWinImage
{

// One decoded image. 32-bit ARGB, top-down, nWidth pixels per row, so
// pixels[y * nWidth + x] with no padding between rows.
struct SImage
{
	int nWidth;
	int nHeight;
	// From a .cur. The .ico form of the container puts other things in those two
	// fields, so an icon reports 0,0 and the caller should not read these.
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

// Decode the first directory entry of the ICO container at nBase.
//
// The entry is a device independent bitmap: a BITMAPINFOHEADER, an optional
// palette, a bottom-up colour bitmap of double the stated height, and then a
// 1bpp AND mask covering the other half. That mask carries the transparency,
// including at 32bpp, because these files hold no alpha channel of their own.
//
// False when the data is not that container, when the entry is compressed, or
// when its bit depth is not 8, 24 or 32.
SYSTEM_EXPORT bool DecodeFirstImage( SImage *pResult, const uint8_t *pData, size_t nSize, size_t nBase );

}
