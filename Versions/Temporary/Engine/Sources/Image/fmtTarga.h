#pragma once

#include <cstdint>

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
	uint16_t wFirstEntryIndex;								// Index of the first color map entry. Index refers to the starting entry in loading the color map.
	uint16_t wColorMapLength;									// Total number of color map entries included
	uint8_t cColorMapEntrySize;							// Establishes the number of bits per entry. Typically 15, 16, 24 or 32-bit values are used

};
//
struct SImageDescriptor
{
	uint8_t cAlphaChannelBits : 4;						// the number of attribute bits per pixel
	uint8_t cLeftToRightOrder : 1;						// left-to-right ordering
	uint8_t cTopToBottomOrder : 1;						// top-to-bottom ordering
	uint8_t cUnused           : 2;						// Must be zero to insure future compatibility
};
// describe the image screen location, size and pixel depth
struct SImageSpecification
{
	uint16_t wXOrigin;												// absolute horizontal coordinate for the lower left corner of the image as it is positioned on a display device having an origin at the lower left of the screen
	uint16_t wYOrigin;												// absolute vertical coordinate for the lower left corner of the image as it is positioned on a display device having an origin at the lower left of the screen
	uint16_t wImageWidth;											// width of the image in pixels
	uint16_t wImageHeight;										// height of the image in pixels
	uint8_t cPixelDepth;											// number of bits per pixel. This number includes the Attribute or Alpha channel bits. Common values are 8, 16, 24 and 32 but other pixel depths could be used.
	SImageDescriptor descriptor;					//
};
struct STGAFileHeader
{
	uint8_t cIDLength;												// identifies the number of bytes contained in Field 6, the Image ID Field
	uint8_t cColorMapType;										// indicates the type of color map (if any) included with the image
	uint8_t cImageType;											// TGA File Format can be used to store Pseudo-Color, True-Color and Direct-Color images of various pixel depths
	SColorMapSpecification colormap;      //
	SImageSpecification imagespec;				// 
};
struct STGAFileFooter
{
	uint32_t dwExtensionAreaOffset;					// offset from the beginning of the file to the start of the Extension  Area
	uint32_t dwDeveloperDirectoryOffset;			// offset from the beginning of the file to the start of the Developer Directory
	uint8_t cSignature[16];									// "TRUEVISION-XFILE"
	uint8_t cReservedCharacter;							// must be '.'
	uint8_t cBinaryZeroStringTerminator;			// '\0'
};
#pragma pack()

