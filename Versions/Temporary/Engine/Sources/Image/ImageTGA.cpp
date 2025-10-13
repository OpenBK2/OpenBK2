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
	// check for the new/original TGA file format
	pStream->Seek( pStream->GetSize() - 26 );
	STGAFileFooter footer;
	pStream->Read( &footer, sizeof(footer) );
	pStream->Seek( nOriginalPos );
	// check for the new
	char pszSignature[32];
	memcpy( pszSignature, footer.cSignature, 16 );
	pszSignature[16] = 0;
	bool bNewTGA = ( footer.cReservedCharacter == '.' ) && ( strcmp(pszSignature, "TRUEVISION-XFILE") == 0 );
	if ( bNewTGA )
		return true;
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

