#pragma once

#include "Misc/2Darray.h"

#include <cstdint>

#include <fmt/format.h>

#pragma pack ( 1 )
struct SColor24 
{ 
	uint8_t b;
	uint8_t g;
	uint8_t r;
};
#pragma pack()

namespace NImage
{
inline unsigned long FP_NORM_TO_BYTE2(float p)                                                 
{                                                                            
  float fpTmp = p + 1.0f;                                                      
  return ((*(unsigned *)&fpTmp) >> 15) & 0xFF;  
}

inline unsigned long FP_NORM_TO_BYTE3(float p)     
{
  float ftmp = p + 12582912.0f;                                                      
  return ((*(unsigned long *)&ftmp) & 0xFF);
}

inline uint8_t GetA( uint32_t val ) { return ( (val >> 24) & 0xff ); }
inline uint8_t GetR( uint32_t val ) { return ( (val >> 16) & 0xff ); }
inline uint8_t GetG( uint32_t val ) { return ( (val >>  8) & 0xff ); }
inline uint8_t GetB( uint32_t val ) { return ( (val      ) & 0xff ); }
inline uint8_t MakeGray( uint8_t r, uint8_t g, uint8_t b ) { return FP_NORM_TO_BYTE3( Clamp(0.3f*r + 0.59f*g + 0.11f*b, 0.0f, 255.0f) ); }
inline uint8_t MakeGray( float r, float g, float b ) { return FP_NORM_TO_BYTE2( Clamp(0.3f*r + 0.59f*g + 0.11f*b, 0.0f, 1.0f) ); }
inline uint32_t MakeComponent( float f ) { return uint32_t( FP_NORM_TO_BYTE3(Clamp(f*255.0f, 0.0f, 255.0f)) ); }

// ************************************************************************************************************************ //
// **
// ** gray convertor
// **
// **
// **
// ************************************************************************************************************************ //

template <class TOutColor>
class CGrayConvertor
{
};
template <> class CGrayConvertor<uint32_t>
{
public:
	CGrayConvertor( int nColorMapLength, int nColorMapEntrySize, CDataStream *pStream ) {  }
	uint32_t operator()( uint8_t input ) const { return 0xff000000 | (uint32_t(input) << 16) | (uint32_t(input) << 8) | uint32_t(input); }
	bool IsReady() const { return true; }
};
template <> class CGrayConvertor<SColor24>
{
public:
	CGrayConvertor( int nColorMapLength, int nColorMapEntrySize, CDataStream *pStream ) {  }
	SColor24 operator()( uint8_t input ) const { SColor24 color = { input, input, input }; return color; }
	bool IsReady() const { return true; }
};
template <> class CGrayConvertor<uint8_t>
{
public:
	CGrayConvertor( int nColorMapLength, int nColorMapEntrySize, CDataStream *pStream ) {  }
	uint8_t operator()( uint8_t input ) const { return input; }
	bool IsReady() const { return true; }
};
template <> class CGrayConvertor<CVec4>
{
public:
	CGrayConvertor( int nColorMapLength, int nColorMapEntrySize, CDataStream *pStream ) {  }
	CVec4 operator()( uint8_t input ) const { return CVec4( input / 255.0f, input / 255.0f, input / 255.0f, 1.0f ); }
	bool IsReady() const { return true; }
};

// ************************************************************************************************************************ //
// **
// ** palette convertor
// **
// **
// **
// ************************************************************************************************************************ //

template <class TOutColor>
class CPaletteConvertor
{
};
template <> class CPaletteConvertor<uint32_t>
{
	std::vector<uint32_t> palette;
public:
	CPaletteConvertor( int nColorMapLength, int nColorMapEntrySize, CDataStream *pStream )
	{
		switch ( nColorMapEntrySize ) 
		{
			case 32:
				palette.resize( nColorMapLength );
				pStream->Read( &(palette[0]), sizeof(uint32_t) * palette.size() );
				break;
			case 24:
				{
					std::vector<SColor24> palette24( nColorMapLength );
					pStream->Read( &(palette24[0]), sizeof(SColor24) * palette24.size() );
					palette.reserve( nColorMapLength );
					for ( std::vector<SColor24>::const_iterator it = palette24.begin(); it != palette24.end(); ++it )
						palette.push_back( 0xff000000 | (uint32_t(it->r) << 16) | (uint32_t(it->g) << 8) | uint32_t(it->b) );
				}
				break;
			default:
				NI_ASSERT( 0, fmt::format("unsupported bit depth ({}) - still not realized", nColorMapEntrySize) );
		}
	}
	//
	uint32_t operator()( const uint8_t input ) const { return palette[input]; }
	bool IsReady() const { return !palette.empty(); }
};
template <> class CPaletteConvertor<SColor24>
{
	std::vector<SColor24> palette;
public:
	CPaletteConvertor( int nColorMapLength, int nColorMapEntrySize, CDataStream *pStream )
	{
		switch ( nColorMapEntrySize ) 
		{
			case 32:
				{
					std::vector<uint32_t> palette32( nColorMapLength );
					pStream->Read( &(palette32[0]), sizeof(uint32_t) * palette32.size() );
					palette.resize( nColorMapLength );
					int i = 0;
					for ( std::vector<uint32_t>::const_iterator it = palette32.begin(); it != palette32.end(); ++it, ++i )
					{
						palette[i].r = GetR( *it );
						palette[i].g = GetG( *it );
						palette[i].b = GetB( *it );
					}
				}
				break;
			case 24:
				palette.resize( nColorMapLength );
				pStream->Read( &(palette[0]), sizeof(SColor24) * palette.size() );
				break;
			default:
				NI_ASSERT( 0, fmt::format("unsupported bit depth ({}) - still not realized", nColorMapEntrySize) );
		}
	}
	//
	SColor24 operator()( const uint8_t input ) const { return palette[input]; }
	bool IsReady() const { return !palette.empty(); }
};
template <> class CPaletteConvertor<uint8_t>
{
	std::vector<uint8_t> palette;
public:
	CPaletteConvertor( int nColorMapLength, int nColorMapEntrySize, CDataStream *pStream )
	{
		switch ( nColorMapEntrySize ) 
		{
			case 32:
				{
					std::vector<uint32_t> palette32( nColorMapLength );
					pStream->Read( &(palette32[0]), sizeof(uint32_t) * palette32.size() );
					palette.reserve( nColorMapLength );
					for ( std::vector<uint32_t>::const_iterator it = palette32.begin(); it != palette32.end(); ++it )
						palette.push_back( MakeGray(GetR(*it), GetG(*it), GetB(*it)) );
				}
				break;
			case 24:
				{
					std::vector<SColor24> palette24( nColorMapLength );
					pStream->Read( &(palette24[0]), sizeof(SColor24) * palette24.size() );
					palette.reserve( nColorMapLength );
					for ( std::vector<SColor24>::const_iterator it = palette24.begin(); it != palette24.end(); ++it )
						palette.push_back( MakeGray(it->r, it->g, it->b) );
				}
				break;
			default:
				NI_ASSERT( 0, fmt::format("unsupported bit depth ({}) - still not realized", nColorMapEntrySize) );
		}
	}
	//
	uint8_t operator()( const uint8_t input ) const { return palette[input]; }
	bool IsReady() const { return !palette.empty(); }
};
template <> class CPaletteConvertor<CVec4>
{
	std::vector<CVec4> palette;
public:
	CPaletteConvertor( int nColorMapLength, int nColorMapEntrySize, CDataStream *pStream )
	{
		switch ( nColorMapEntrySize ) 
		{
			case 32:
				{
					std::vector<uint32_t> palette32( nColorMapLength );
					pStream->Read( &(palette32[0]), sizeof(uint32_t) * palette32.size() );
					palette.reserve( nColorMapLength );
					for ( std::vector<uint32_t>::const_iterator it = palette32.begin(); it != palette32.end(); ++it )
						palette.push_back( CVec4(GetR(*it) / 255.0f, GetG(*it) / 255.0f, GetB(*it) / 255.0f, GetA(*it) / 255.0f) );
				}
				break;
			case 24:
				{
					std::vector<SColor24> palette24( nColorMapLength );
					pStream->Read( &(palette24[0]), sizeof(SColor24) * palette24.size() );
					palette.reserve( nColorMapLength );
					for ( std::vector<SColor24>::const_iterator it = palette24.begin(); it != palette24.end(); ++it )
						palette.push_back( CVec4(it->r / 255.0f, it->g / 255.0f, it->b / 255.0f, 1.0f) );
				}
				break;
			default:
				NI_ASSERT( 0, fmt::format("unsupported bit depth ({}) - still not realized", nColorMapEntrySize) );
		}
	}
	//
	CVec4 operator()( const uint8_t input ) const { return palette[input]; }
	bool IsReady() const { return !palette.empty(); }
};

// ************************************************************************************************************************ //
// **
// ** raw color convertor
// **
// **
// **
// ************************************************************************************************************************ //

template <class TOutColor>
class CRawColorConvertor
{
};
template <> class CRawColorConvertor<uint32_t>
{
public:
	CRawColorConvertor( int nColorMapLength, int nColorMapEntrySize, CDataStream *pStream ) {}
	uint32_t operator()( const uint32_t input ) const { return input; }
	uint32_t operator()( const SColor24 input ) const { return 0xff000000 | (uint32_t(input.r) << 16) | (uint32_t(input.g) << 8) | uint32_t(input.b); }
	uint32_t operator()( const CVec4 &input ) const { return (MakeComponent(input.w) << 24) | (MakeComponent(input.x) << 16) | (MakeComponent(input.y) << 8) | MakeComponent(input.z); }
	bool IsReady() const { return true; }
};
template <> class CRawColorConvertor<SColor24>
{
public:
	CRawColorConvertor( int nColorMapLength, int nColorMapEntrySize, CDataStream *pStream ) {}
	SColor24 operator()( const uint32_t input ) const { const SColor24 color = { GetB(input), GetG(input), GetR(input)	}; return color; }
	SColor24 operator()( const SColor24 input ) const { return input; }
	SColor24 operator()( const CVec4 &input ) const { const SColor24 color = { MakeComponent(input.z), MakeComponent(input.y), MakeComponent(input.x) }; return color; }
	bool IsReady() const { return true; }
};
template <> class CRawColorConvertor<uint8_t>
{
public:
	CRawColorConvertor( int nColorMapLength, int nColorMapEntrySize, CDataStream *pStream ) {}
	uint8_t operator()( const uint32_t input ) const { return MakeGray( GetR(input), GetG(input), GetB(input) ); }
	uint8_t operator()( const SColor24 input ) const { return MakeGray( input.r, input.g, input.b ); }
	uint8_t operator()( const CVec4 &input ) const { return MakeGray( input.r, input.g, input.b ); }
	bool IsReady() const { return true; }
};
template <> class CRawColorConvertor<CVec4>
{
public:
	CRawColorConvertor( int nColorMapLength, int nColorMapEntrySize, CDataStream *pStream ) {}
	CVec4 operator()( const uint32_t input ) const { return CVec4( GetR(input) / 255.0f, GetG(input) / 255.0f, GetB(input) / 255.0f, GetA(input) / 255.0f ); }
	CVec4 operator()( const SColor24 input ) const { return CVec4( input.r / 255.0f, input.g / 255.0f, input.b / 255.0f, 1.0f ); }
	CVec4 operator()( const CVec4 &input ) const { return input; }	
	bool IsReady() const { return true; }
};

// ************************************************************************************************************************ //
// **
// ** image conversion function
// **
// **
// **
// ************************************************************************************************************************ //

template <typename TOutColor, typename TInColor>
inline void Convert( CArray2D<TOutColor> *pDst, const CArray2D<TInColor> &src )
{
	NI_ASSERT( (src.GetSizeX() == pDst->GetSizeX()) && (src.GetSizeY() == pDst->GetSizeY()), "Wrong image dimension(s)!" );
	CRawColorConvertor<TOutColor> convertor(0,0,0);
	const TInColor *pInColor = &( src[0][0] );
	const TInColor *pEnd = pInColor + src.GetSizeX()*src.GetSizeY();
	TOutColor *pOutColor = &( (*pDst)[0][0] );
	for ( ; pInColor != pEnd; ++pInColor, ++pOutColor )
		*pOutColor = convertor( *pInColor );
}

template <typename TColor>
inline void FlipY( CArray2D<TColor> &data, int nBegin, int nEnd )
{
	while ( nEnd > nBegin ) 
	{
		for ( int x = 0; x < data.GetSizeX(); ++x )
		{
			std::swap( data[nEnd][x], data[nBegin][x] );
		}
		--nEnd;
		++nBegin;
	}
}
template <typename TColor>
inline void FlipY( CArray2D<TColor> &data )
{
	FlipY( data, 0, data.GetSizeY() - 1 );
}


template <typename TColor>
inline void FlipX( CArray2D<TColor> &data, int nBegin, int nEnd )
{
	while ( nEnd > nBegin ) 
	{
		for ( int y = 0; y < data.GetSizeY(); ++y )
		{
			std::swap( data[y][nEnd], data[y][nBegin] );
		}
		--nEnd;
		++nBegin;
	}
}
template <typename TColor>
inline void FlipX( CArray2D<TColor> &data )
{
	FlipX( data, 0, data.GetSizeX() - 1 );
}

}

