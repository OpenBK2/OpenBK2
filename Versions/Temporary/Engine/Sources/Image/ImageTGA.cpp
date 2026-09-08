#include "stdafx.h"

#include "ImageTGA.h"

#include "Targa.h"

#include <cstdint>

namespace NImage
{

// ************************************************************************************************************************ //
// **
// ** recognize TGA file format
// **
// **
// **
// ************************************************************************************************************************ //

bool RecognizeFormatTGA( CDataStream *pStream )
{
	const int nOriginalPos = pStream->GetPosition();
	// Original TGA files can be smaller than the optional 26-byte footer.
	if ( pStream->GetSize() - nOriginalPos < sizeof(STGAFileHeader) )
		return false;
	if ( pStream->GetSize() >= sizeof(STGAFileFooter) )
	{
		pStream->Seek(pStream->GetSize() - sizeof(STGAFileFooter));
		STGAFileFooter footer = {};
		pStream->Read(&footer, sizeof(footer));
		pStream->Seek(nOriginalPos);
		if ( footer.cReservedCharacter == '.' &&
			memcmp(footer.cSignature, "TRUEVISION-XFILE", 16) == 0 )
			return true;
	}
	// check for the original
	STGAFileHeader hdr;
	pStream->Read( &hdr, sizeof(hdr) );
	pStream->Seek( nOriginalPos );
	// image type <=> color map type
	bool bCheck = false;
	switch ( hdr.cImageType )
	{
		case TGAIT_NOIMAGEDATA:
			bCheck = false;
			break;
		case TGAIT_COLOR_MAPPED:
			bCheck = hdr.cColorMapType == 1;
			break;
		case TGAIT_TRUE_COLOR:
			bCheck = hdr.cColorMapType == 0;
			break;
		case TGAIT_BLACK_WHITE:
			bCheck = hdr.cColorMapType == 1;
			break;
		case TGAIT_RLE_COLOR_MAPPED:
			bCheck = hdr.cColorMapType == 1;
			break;
		case TGAIT_RLE_TRUE_COLOR:
			bCheck = hdr.cColorMapType == 0;
			break;
		case TGAIT_RLE_BLACK_WHITE:
			bCheck = hdr.cColorMapType == 0;
			break;
	}
	if ( !bCheck )
		return false;
	// some fields valid values
	bCheck = hdr.imagespec.descriptor.cUnused == 0;
	if ( !bCheck )
		return false;

	return true;
}

// ************************************************************************************************************************ //
// **
// ** main loading function
// **
// **
// **
// ************************************************************************************************************************ //

bool LoadImageTGA( CArray2D<uint32_t> *pRes, CDataStream *pStream )
{
	STGAFileHeader hdr;
	LoadTGAHeader( &hdr, pStream );
	pRes->SetSizes( hdr.imagespec.wImageWidth, hdr.imagespec.wImageHeight );
	return LoadTGAImageData( &(*pRes)[0][0], hdr, pStream );
}

// ************************************************************************************************************************ //
// **
// ** main save function
// **
// **
// **
// ************************************************************************************************************************ //

bool SaveImageAsTGA( CDataStream *pStream, const CArray2D<uint32_t> &image )
{
	return SaveAsTGA( image, pStream );
}

}

