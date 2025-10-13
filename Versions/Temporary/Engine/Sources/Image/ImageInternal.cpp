#include "stdafx.h"
#include "ImageInternal.h"

#include <cstdint>

// ************************************************************************************************************************ //
// **
// ** pixel format conversion functions
// **
// **
// **
// ************************************************************************************************************************ //

bool SPixelConvertInfo::InitMaskInfo( uint32_t dwABitMask, uint32_t dwRBitMask, uint32_t dwGBitMask, uint32_t dwBBitMask )
{
	uint32_t dwMask, dwBitShift, dwBitCount;
	//
	memset( this, 0, sizeof(*this) );
	// Get Alpha Mask info
	dwMask = dwABitMask;
	dwBitShift = dwBitCount = 0;
	if ( dwMask )
	{
		for ( ; (dwMask & 0x1) == 0; dwMask >>= 1 )
			dwBitShift++;
		for ( ; (dwMask & 0x1) == 1; dwMask >>= 1 )
			dwBitCount++;
	}
	dwAMask  = dwABitMask;
	dwABits  = dwBitCount;
	dwAShift = dwBitShift;
	// Get Red Mask info
	dwMask = dwRBitMask;
	dwBitShift = dwBitCount = 0;
	if ( dwMask )
	{
		for ( ; (dwMask & 0x1) == 0; dwMask >>= 1 )
			dwBitShift++;
		for ( ; (dwMask & 0x1) == 1; dwMask >>= 1 )
			dwBitCount++;
	}
	dwRMask  = dwRBitMask;
	dwRBits  = dwBitCount;
	dwRShift = dwBitShift;
	// Get Green Mask info
	dwMask = dwGBitMask;
	dwBitShift = dwBitCount = 0;
	if ( dwMask )
	{
		for ( ; (dwMask & 0x1) == 0; dwMask >>= 1 )
			dwBitShift++;
		for ( ; (dwMask & 0x1) == 1; dwMask >>= 1 )
			dwBitCount++;
	}
	dwGMask  = dwGBitMask;
	dwGBits  = dwBitCount;
	dwGShift = dwBitShift;
	// Get Blue Mask info
	dwMask = dwBBitMask;
	dwBitShift = dwBitCount = 0;
	if ( dwMask )
	{
		for ( ; (dwMask & 0x1) == 0; dwMask >>= 1 )
			dwBitShift++;
		for ( ; (dwMask & 0x1) == 1; dwMask >>= 1 )
			dwBitCount++;
	}
	dwBMask  = dwBBitMask;
	dwBBits  = dwBitCount;
	dwBShift = dwBitShift;
	//
	return true;
}

// convert color from (ARGB = 8888) format to specified by this pci
uint32_t SPixelConvertInfo::ComposeColor( uint32_t dwColor ) const
{
	// Convert Alpha component
	uint32_t a = (dwColor >> 24) & 0xFF;
	a >>= (8 - dwABits);
	a <<= dwAShift;
	// Convert Red component
	uint32_t r = (dwColor >> 16) & 0xFF;			// Convert to uint8_t
	r >>= (8 - dwRBits);		              // throw away low precision bits
	r <<= dwRShift;					              // move to new position
	// Convert Green component
	uint32_t g = (dwColor >> 8) & 0xFF;
	g >>= (8 - dwGBits);
	g <<= dwGShift;
	// Convert Blue component
	uint32_t b = dwColor & 0xFF;
	b >>= (8 - dwBBits);
	b <<= dwBShift;
	// Return converted color
	return (r | g | b | a);
}

// more precise convert color from (ARGB = 8888) format to specified by this pci
uint32_t SPixelConvertInfo::ComposeColorSlow( uint32_t dwColor ) const
{
	// Convert Alpha component
	uint32_t a = uint32_t( double( ( dwColor >> 24 ) & 0xFF ) / double( 1 << (8 - dwABits) ) ) << dwAShift;
	// Convert Red component
	uint32_t r = uint32_t( double( ( dwColor >> 16 ) & 0xFF ) / double( 1 << (8 - dwRBits) ) ) << dwRShift;
	// Convert Green component
	uint32_t g = uint32_t( double( ( dwColor >>  8 ) & 0xFF ) / double( 1 << (8 - dwGBits) ) ) << dwGShift;
	// Convert Blue component
	uint32_t b = uint32_t( double( ( dwColor       ) & 0xFF ) / double( 1 << (8 - dwBBits) ) ) << dwBShift;
	// Return converted color
	return (r | g | b | a);
}

// convert color from current (specified by this pci) to (ARGB = 8888) format
uint32_t SPixelConvertInfo::DecompColor( uint32_t dwColor ) const
{
	// Convert Alpha component
	uint32_t a = ((dwColor & dwAMask) >> dwAShift);
	a <<= (8 - dwABits);
	a <<= 24;
	// Convert Red component
	uint32_t r = ((dwColor & dwRMask) >> dwRShift);
	r <<= (8 - dwRBits);
	r <<= 16;
	// Convert Green component
	uint32_t g = ((dwColor & dwGMask) >> dwGShift);
	g <<= (8 - dwGBits);
	g <<= 8;
	// Convert Blue component
	uint32_t b = ((dwColor & dwBMask) >> dwBShift);
	b <<= (8 - dwBBits);
	// Return converted color
	return (r | g | b | a);
}


