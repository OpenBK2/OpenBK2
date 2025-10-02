#pragma once

#include "ImageConvertor.h"

namespace NImage
{

// ************************************************************************************************************************ //
// **
// ** TGA file header/footer structs and types
// **
// **
// **
// ************************************************************************************************************************ //

enum ETGAImageType
{
	TGAIT_NOIMAGEDATA				= 0,
	TGAIT_COLOR_MAPPED			= 1,
	TGAIT_TRUE_COLOR				= 2,
	TGAIT_BLACK_WHITE				= 3,
	TGAIT_RLE_COLOR_MAPPED	= 9,
	TGAIT_RLE_TRUE_COLOR		= 10,
	TGAIT_RLE_BLACK_WHITE		= 11
};

#pragma pack( 1 )
// describe the color map (if any) used for the image
struct SColorMapSpecification 
{
	WORD wFirstEntryIndex;								// Index of the first color map entry. Index refers to the starting entry in loading the color map.
	WORD wColorMapLength;									// Total number of color map entries included
	BYTE cColorMapEntrySize;							// Establishes the number of bits per entry. Typically 15, 16, 24 or 32-bit values are used

};
//
struct SImageDescriptor
{
	BYTE cAlphaChannelBits : 4;						// the number of attribute bits per pixel
	BYTE cLeftToRightOrder : 1;						// left-to-right ordering 
	BYTE cTopToBottomOrder : 1;						// top-to-bottom ordering 
	BYTE cUnused           : 2;						// Must be zero to insure future compatibility
};
// describe the image screen location, size and pixel depth
struct SImageSpecification
{
	WORD wXOrigin;												// absolute horizontal coordinate for the lower left corner of the image as it is positioned on a display device having an origin at the lower left of the screen 
	WORD wYOrigin;												// absolute vertical coordinate for the lower left corner of the image as it is positioned on a display device having an origin at the lower left of the screen
	WORD wImageWidth;											// width of the image in pixels
	WORD wImageHeight;										// height of the image in pixels
	BYTE cPixelDepth;											// number of bits per pixel. This number includes the Attribute or Alpha channel bits. Common values are 8, 16, 24 and 32 but other pixel depths could be used.
	SImageDescriptor descriptor;					//
};
struct STGAFileHeader
{
	BYTE cIDLength;												// identifies the number of bytes contained in Field 6, the Image ID Field
	BYTE cColorMapType;										// indicates the type of color map (if any) included with the image
	BYTE cImageType;											// TGA File Format can be used to store Pseudo-Color, True-Color and Direct-Color images of various pixel depths
	SColorMapSpecification colormap;      //
	SImageSpecification imagespec;				// 
};
struct STGAFileFooter
{
	DWORD dwExtensionAreaOffset;					// offset from the beginning of the file to the start of the Extension  Area
	DWORD dwDeveloperDirectoryOffset;			// offset from the beginning of the file to the start of the Developer Directory
	BYTE cSignature[16];									// "TRUEVISION-XFILE"
	BYTE cReservedCharacter;							// must be '.'
	BYTE cBinaryZeroStringTerminator;			// '\0'
};
#pragma pack()

// ************************************************************************************************************************ //
// **
// ** Raw TGA loader
// **
// **
// **
// ************************************************************************************************************************ //

template <typename TInColor, typename TOutColor, class TConvertor>
class CTGARawLoader
{
	static bool LoadLocal( TOutColor *pColors, const int nNumElements, const TConvertor &convertor, CDataStream *pStream, SInt2Type<0> *pp )
	{
		vector<TInColor> buffer( nNumElements );
		const int nBytesToRead = nNumElements * sizeof( TInColor );
		pStream->Read( &(buffer[0]), nBytesToRead );
		for ( vector<TInColor>::const_iterator it = buffer.begin(); it != buffer.end(); ++it )
			*pColors++ = convertor( *it );
		return true;
	}
	static bool LoadLocal( TOutColor *pColors, const int nNumElements, const TConvertor &convertor, CDataStream *pStream, SInt2Type<1> *pp )
	{
		const int nBytesToRead = nNumElements * sizeof( TInColor );
		pStream->Read( pColors, nBytesToRead );
		return true;
	}
public:
	static bool Load( TOutColor *pColors, const STGAFileHeader &hdr, CDataStream *pStream )
	{
		TConvertor convertor( hdr.colormap.wColorMapLength, hdr.colormap.cColorMapEntrySize, pStream );
		if ( !convertor.IsReady() ) 
			return false;
		// second condition (!= 1) are to avoid pallettized image loading w/o conversion
		const int nColorsEqual = sizeof( TInColor ) == sizeof( TOutColor ) && sizeof( TInColor ) != 1;
		SInt2Type<nColorsEqual> separator;
		return LoadLocal( pColors, hdr.imagespec.wImageWidth * hdr.imagespec.wImageHeight, convertor, pStream, &separator );
	}
};

// ************************************************************************************************************************ //
// **
// ** RLE TGA loader
// ** RLE encoding:
// ** 1 byte - the Repetition Count field
// **          Run-length packet: bit7 = 1, other bits - run-length counter (up to 127)
// **          Raw-data packet: bit7 = 0,  other bits - number of pixel values (up to 127)
// ** next bytes (depent on pixel format)
// **          Run-lenght packet: single color value
// **          Raw-data packet: 'number of pixel values' color values
// ** NOTE: all counters must be increased to 1
// **
// ************************************************************************************************************************ //

template <typename TInColor, typename TOutColor, class TConvertor>
class CTGARLELoader
{
	static bool LoadRaw( TOutColor *pColors, const int nNumElements, const TConvertor &convertor, CDataStream *pStream, SInt2Type<0> *pp )
	{
		TInColor buffer[128];
		pStream->Read( &(buffer[0]), nNumElements * sizeof(TInColor) );
		for ( TInColor *pColor = buffer; pColor != buffer + nNumElements; ++pColor, ++pColors )
			*pColors = convertor( *pColor );
		return true;
	}
	static bool LoadRaw( TOutColor *pColors, const int nNumElements, const TConvertor &convertor, CDataStream *pStream, SInt2Type<1> *pp )
	{
		pStream->Read( pColors, nNumElements * sizeof(TInColor) );
		return true;
	}
public:
	static bool Load( TOutColor *pColors, const STGAFileHeader &hdr, CDataStream *pStream )
	{
		TConvertor convertor( hdr.colormap.wColorMapLength, hdr.colormap.cColorMapEntrySize, pStream );
		if ( !convertor.IsReady() ) 
			return false;
		const int nColorsEqual = sizeof( TInColor ) == sizeof( TOutColor );
		SInt2Type<nColorsEqual> separator;
		const int nBytesToRead = int(hdr.imagespec.wImageWidth) * int(hdr.imagespec.wImageHeight) * int(hdr.imagespec.cPixelDepth) / 8;
		int nReadBytes = 0;
		BYTE cRepetitionCounter = 0;
		TInColor pixelValue;
		while ( nReadBytes < nBytesToRead )
		{
			pStream->Read( &cRepetitionCounter, 1 );
			const int nNumElements = ( cRepetitionCounter & 0x7f ) + 1;
			if ( cRepetitionCounter & 0x80 )	// run-length packet
			{
				pStream->Read( &pixelValue, sizeof(TInColor) );
				const TOutColor color = convertor( pixelValue );
				for ( TOutColor *pColor = pColors; pColor != pColors + nNumElements; ++pColor )
					*pColor = color;
				//MemSetDWord( reinterpret_cast<DWORD*>(pColors), convertor(pixelValue), nNumElements );
			}
			else															// raw-data packet
				LoadRaw( pColors, nNumElements, convertor, pStream, &separator );
			pColors += nNumElements;
			nReadBytes += nNumElements * sizeof( TInColor );
		}
		NI_ASSERT( nReadBytes == nBytesToRead, StrFmt("Can't load image - not enough bytes %d != %d", nReadBytes, nBytesToRead) );
		return nReadBytes == nBytesToRead;
	}
};

// ************************************************************************************************************************ //
// **
// ** main loading functions block
// **
// **
// **
// ************************************************************************************************************************ //

inline void LoadTGAHeader( STGAFileHeader *pHdr, CDataStream *pStream )
{
	pStream->Seek( 0 );
	pStream->Read( pHdr, sizeof(*pHdr) );
	// skip image ID
	if ( pHdr->cIDLength != 0 )
		pStream->Seek( pStream->GetPosition() + pHdr->cIDLength );
}

template <typename TOutColor>
bool LoadTGAImageData( TOutColor *pColors, const STGAFileHeader &hdr, CDataStream *pStream )
{
	switch ( hdr.cImageType )
	{
		case TGAIT_TRUE_COLOR:
			switch ( hdr.imagespec.cPixelDepth )
			{
				case 24:
					return CTGARawLoader<SColor24, TOutColor, CRawColorConvertor<TOutColor> >::Load( pColors, hdr, pStream );
				case 32:
					return CTGARawLoader<DWORD, TOutColor, CRawColorConvertor<TOutColor> >::Load( pColors, hdr, pStream );
				default:
					NI_ASSERT( 0, StrFmt("unsupported bit depth (%d) - still not realized", hdr.imagespec.cPixelDepth) );
					return false;
			}
			break;

		case TGAIT_COLOR_MAPPED:
			return CTGARawLoader<BYTE, TOutColor, CPaletteConvertor<TOutColor> >::Load( pColors, hdr, pStream );

		case TGAIT_BLACK_WHITE:
			return CTGARawLoader<BYTE, TOutColor, CGrayConvertor<TOutColor> >::Load( pColors, hdr, pStream );

		case TGAIT_RLE_TRUE_COLOR:
			switch ( hdr.imagespec.cPixelDepth )
			{
				case 24:
					return CTGARLELoader<SColor24, TOutColor, CRawColorConvertor<TOutColor> >::Load( pColors, hdr, pStream );
				case 32:
					return CTGARLELoader<DWORD, TOutColor, CRawColorConvertor<TOutColor> >::Load( pColors, hdr, pStream );
				default:
					NI_ASSERT( 0, StrFmt("unsupported bit depth (%d) - still not realized", hdr.imagespec.cPixelDepth) );
					return false;
			}
			break;

		case TGAIT_RLE_COLOR_MAPPED:
			return CTGARLELoader<BYTE, TOutColor, CPaletteConvertor<TOutColor> >::Load( pColors, hdr, pStream );

		case TGAIT_RLE_BLACK_WHITE:
			return CTGARLELoader<BYTE, TOutColor, CGrayConvertor<TOutColor> >::Load( pColors, hdr, pStream );
	}
	return false;
}

template <typename TOutColor>
bool LoadTGAImage( CArray2D<TOutColor> &dst, CDataStream *pStream )
{
	STGAFileHeader hdr;
	LoadTGAHeader( &hdr, pStream );
	dst.SetSizes( hdr.imagespec.wImageWidth, hdr.imagespec.wImageHeight );
	if ( LoadTGAImageData(&(dst[0][0]), hdr, pStream) == false ) 
		return false;
	if ( hdr.imagespec.descriptor.cTopToBottomOrder == 0 )
		FlipY( dst );
	return true;
}

// return new Y position
template <typename TOutColor>
int LoadTGAImageAppendY( CArray2D<TOutColor> &dst, const int nY, CDataStream *pStream )
{
	STGAFileHeader hdr;
	LoadTGAHeader( &hdr, pStream );
	NI_ASSERT( (dst.GetSizeX() == int(hdr.imagespec.wImageWidth)) && (dst.GetSizeY() > nY + int(hdr.imagespec.wImageHeight)), "Image too small to append" );
	if ( (dst.GetSizeX() != int(hdr.imagespec.wImageWidth)) || (dst.GetSizeY() <= nY + int(hdr.imagespec.wImageHeight)) ) 
		return nY;
	if ( LoadTGAImageData(&(dst[nY][0]), hdr, pStream) == false ) 
		return nY;
	if ( hdr.imagespec.descriptor.cTopToBottomOrder == 0 )
		FlipY( dst, nY, nY + int(hdr.imagespec.wImageHeight) - 1 );
	return nY + int(hdr.imagespec.wImageHeight);
}

// ************************************************************************************************************************ //
// **
// ** Save: header forming
// **
// **
// **
// ************************************************************************************************************************ //

template <typename TColor>
inline void __fill_tga_header( STGAFileHeader *pHdr )
{
	NI_ASSERT( false, "Unknwon format" );
}
template <> 
inline void __fill_tga_header<DWORD>( STGAFileHeader *pHdr )
{
	pHdr->cImageType = TGAIT_TRUE_COLOR;
	pHdr->imagespec.descriptor.cAlphaChannelBits = 8;
}
template <> 
inline void __fill_tga_header<SColor24>( STGAFileHeader *pHdr )
{
	pHdr->cImageType = TGAIT_TRUE_COLOR;
}
template <> 
inline void __fill_tga_header<BYTE>( STGAFileHeader *pHdr )
{
	pHdr->cImageType = TGAIT_BLACK_WHITE;
}

// ************************************************************************************************************************ //
// **
// ** save functions
// **
// **
// **
// ************************************************************************************************************************ //

template <typename TColor>
bool SaveAsTGA( const CArray2D<TColor> &image, CDataStream *pStream )
{
	STGAFileHeader hdr;
	Zero( hdr );
	hdr.imagespec.wImageWidth = image.GetSizeX();
	hdr.imagespec.wImageHeight = image.GetSizeY();
	hdr.imagespec.descriptor.cTopToBottomOrder = 1;
	hdr.imagespec.cPixelDepth = 8 * sizeof( TColor );
	__fill_tga_header<TColor>( &hdr );

	pStream->Write( &hdr, sizeof(hdr) );

	// write color data
	const int nNumElements = int(hdr.imagespec.wImageWidth) * int(hdr.imagespec.wImageHeight);
	const int nWriteSizeInBytes = nNumElements * sizeof(TColor);
	NI_ASSERT( nWriteSizeInBytes > 0, StrFmt("image size %d : %d are too big (>2GB) - can't save it", int(hdr.imagespec.wImageWidth), int(hdr.imagespec.wImageHeight)) );
	pStream->Write( &(image[0][0]), nWriteSizeInBytes );
	// compose and write TGA file footer
	STGAFileFooter footer;
	Zero( footer );
	memcpy( footer.cSignature, "TRUEVISION-XFILE", 16 );
	footer.cReservedCharacter = '.';
	pStream->Write( &footer, sizeof(footer) );
	return true;
}

}
