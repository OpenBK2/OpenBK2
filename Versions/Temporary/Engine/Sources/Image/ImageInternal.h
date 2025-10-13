#pragma once

#include <cstdint>

// ************************************************************************************************************************ //
// **
// ** pixel format conversion class
// **
// **
// **
// ************************************************************************************************************************ //

struct SPixelConvertInfo
{
public:
	uint32_t dwBitDepth;                     // bit depth of this format
	// Alpha channel info
	uint32_t dwAMask;		                    // bit mask
	uint32_t dwABits;		                    // # of bits in mask
	uint32_t dwAShift;		                    // # of bits to shift down to canonical position
	// Red channel info
	uint32_t dwRMask;		                    // bit mask
	uint32_t dwRBits;		                    // # of bits in mask
	uint32_t dwRShift;		                    // # of bits to shift down to canonical position
	// Green channel info
	uint32_t dwGMask;		                    // bit mask
	uint32_t dwGBits;		                    // # of bits in mask
	uint32_t dwGShift;		                    // # of bits to shift down to canonical position
	// Blue channel Info
	uint32_t dwBMask;		                    // bit mask
	uint32_t dwBBits;		                    // # of bits in mask
	uint32_t dwBShift;		                    // # of bits to shift down to canonical position
public:
	SPixelConvertInfo() {  }
	SPixelConvertInfo( uint32_t dwABitMask, uint32_t dwRBitMask, uint32_t dwGBitMask, uint32_t dwBBitMask ) { InitMaskInfo( dwABitMask, dwRBitMask, dwGBitMask, dwBBitMask ); }
	// initialization
	bool InitMaskInfo( uint32_t dwABitMask, uint32_t dwRBitMask, uint32_t dwGBitMask, uint32_t dwBBitMask );
	// color composition/decomposition (from ARGB, to ARGB)
	uint32_t ComposeColor( uint32_t dwColor ) const;
	uint32_t ComposeColorSlow( uint32_t dwColor ) const;
	uint32_t DecompColor( uint32_t dwColor ) const;
};


