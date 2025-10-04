#pragma once
#include "Misc/2Darray.h"

#pragma pack ( 1 )
struct SColor24 
{ 
	BYTE b;
	BYTE g;
	BYTE r; 
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

inline BYTE GetA( DWORD val ) { return ( (val >> 24) & 0xff ); }
inline BYTE GetR( DWORD val ) { return ( (val >> 16) & 0xff ); }
inline BYTE GetG( DWORD val ) { return ( (val >>  8) & 0xff ); }
inline BYTE GetB( DWORD val ) { return ( (val      ) & 0xff ); }
inline BYTE MakeGray( BYTE r, BYTE g, BYTE b ) { return FP_NORM_TO_BYTE3( Clamp(0.3f*r + 0.59f*g + 0.11f*b, 0.0f, 255.0f) ); }
inline BYTE MakeGray( float r, float g, float b ) { return FP_NORM_TO_BYTE2( Clamp(0.3f*r + 0.59f*g + 0.11f*b, 0.0f, 1.0f) ); }
inline DWORD MakeComponent( float f ) { return DWORD( FP_NORM_TO_BYTE3(Clamp(f*255.0f, 0.0f, 255.0f)) ); }

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
template <> class CGrayConvertor<DWORD>
{
public:
	CGrayConvertor( int nColorMapLength, int nColorMapEntrySize, CDataStream *pStream ) {  }
	DWORD operator()( BYTE input ) const { return 0xff000000 | (DWORD(input) << 16) | (DWORD(input) << 8) | DWORD(input); }
	bool IsReady() const { return true; }
};
template <> class CGrayConvertor<SColor24>
{
public:
	CGrayConvertor( int nColorMapLength, int nColorMapEntrySize, CDataStream *pStream ) {  }
	SColor24 operator()( BYTE input ) const { SColor24 color = { input, input, input }; return color; }
	bool IsReady() const { return true; }
};
template <> class CGrayConvertor<BYTE>
{
public:
	CGrayConvertor( int nColorMapLength, int nColorMapEntrySize, CDataStream *pStream ) {  }
	BYTE operator()( BYTE input ) const { return input; }
	bool IsReady() const { return true; }
};
template <> class CGrayConvertor<CVec4>
{
public:
	CGrayConvertor( int nColorMapLength, int nColorMapEntrySize, CDataStream *pStream ) {  }
	CVec4 operator()( BYTE input ) const { return CVec4( input / 255.0f, input / 255.0f, input / 255.0f, 1.0f ); }
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
template <> class CPaletteConvertor<DWORD>
{
	vector<DWORD> palette;
public:
	CPaletteConvertor( int nColorMapLength, int nColorMapEntrySize, CDataStream *pStream )
	{
		switch ( nColorMapEntrySize ) 
		{
			case 32:
				palette.resize( nColorMapLength );
				pStream->Read( &(palette[0]), sizeof(DWORD) * palette.size() );
				break;
			case 24:
				{
					vector<SColor24> palette24( nColorMapLength );
					pStream->Read( &(palette24[0]), sizeof(SColor24) * palette24.size() );
					palette.reserve( nColorMapLength );
					for ( vector<SColor24>::const_iterator it = palette24.begin(); it != palette24.end(); ++it )
						palette.push_back( 0xff000000 | (DWORD(it->r) << 16) | (DWORD(it->g) << 8) | DWORD(it->b) );
				}
				break;
			default:
				NI_ASSERT( 0, StrFmt("unsupported bit depth (%d) - still not realized", nColorMapEntrySize) );
		}
	}
	//
	DWORD operator()( const BYTE input ) const { return palette[input]; }
	bool IsReady() const { return !palette.empty(); }
};
template <> class CPaletteConvertor<SColor24>
{
	vector<SColor24> palette;
public:
	CPaletteConvertor( int nColorMapLength, int nColorMapEntrySize, CDataStream *pStream )
	{
		switch ( nColorMapEntrySize ) 
		{
			case 32:
				{
					vector<DWORD> palette32( nColorMapLength );
					pStream->Read( &(palette32[0]), sizeof(DWORD) * palette32.size() );
					palette.resize( nColorMapLength );
					int i = 0;
					for ( vector<DWORD>::const_iterator it = palette32.begin(); it != palette32.end(); ++it, ++i )
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
				NI_ASSERT( 0, StrFmt("unsupported bit depth (%d) - still not realized", nColorMapEntrySize) );
		}
	}
	//
	SColor24 operator()( const BYTE input ) const { return palette[input]; }
	bool IsReady() const { return !palette.empty(); }
};
template <> class CPaletteConvertor<BYTE>
{
	vector<BYTE> palette;
public:
	CPaletteConvertor( int nColorMapLength, int nColorMapEntrySize, CDataStream *pStream )
	{
		switch ( nColorMapEntrySize ) 
		{
			case 32:
				{
					vector<DWORD> palette32( nColorMapLength );
					pStream->Read( &(palette32[0]), sizeof(DWORD) * palette32.size() );
					palette.reserve( nColorMapLength );
					for ( vector<DWORD>::const_iterator it = palette32.begin(); it != palette32.end(); ++it )
						palette.push_back( MakeGray(GetR(*it), GetG(*it), GetB(*it)) );
				}
				break;
			case 24:
				{
					vector<SColor24> palette24( nColorMapLength );
					pStream->Read( &(palette24[0]), sizeof(SColor24) * palette24.size() );
					palette.reserve( nColorMapLength );
					for ( vector<SColor24>::const_iterator it = palette24.begin(); it != palette24.end(); ++it )
						palette.push_back( MakeGray(it->r, it->g, it->b) );
				}
				break;
			default:
				NI_ASSERT( 0, StrFmt("unsupported bit depth (%d) - still not realized", nColorMapEntrySize) );
		}
	}
	//
	BYTE operator()( const BYTE input ) const { return palette[input]; }
	bool IsReady() const { return !palette.empty(); }
};
template <> class CPaletteConvertor<CVec4>
{
	vector<CVec4> palette;
public:
	CPaletteConvertor( int nColorMapLength, int nColorMapEntrySize, CDataStream *pStream )
	{
		switch ( nColorMapEntrySize ) 
		{
			case 32:
				{
					vector<DWORD> palette32( nColorMapLength );
					pStream->Read( &(palette32[0]), sizeof(DWORD) * palette32.size() );
					palette.reserve( nColorMapLength );
					for ( vector<DWORD>::const_iterator it = palette32.begin(); it != palette32.end(); ++it )
						palette.push_back( CVec4(GetR(*it) / 255.0f, GetG(*it) / 255.0f, GetB(*it) / 255.0f, GetA(*it) / 255.0f) );
				}
				break;
			case 24:
				{
					vector<SColor24> palette24( nColorMapLength );
					pStream->Read( &(palette24[0]), sizeof(SColor24) * palette24.size() );
					palette.reserve( nColorMapLength );
					for ( vector<SColor24>::const_iterator it = palette24.begin(); it != palette24.end(); ++it )
						palette.push_back( CVec4(it->r / 255.0f, it->g / 255.0f, it->b / 255.0f, 1.0f) );
				}
				break;
			default:
				NI_ASSERT( 0, StrFmt("unsupported bit depth (%d) - still not realized", nColorMapEntrySize) );
		}
	}
	//
	CVec4 operator()( const BYTE input ) const { return palette[input]; }
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
template <> class CRawColorConvertor<DWORD>
{
public:
	CRawColorConvertor( int nColorMapLength, int nColorMapEntrySize, CDataStream *pStream ) {}
	DWORD operator()( const DWORD input ) const { return input; }
	DWORD operator()( const SColor24 input ) const { return 0xff000000 | (DWORD(input.r) << 16) | (DWORD(input.g) << 8) | DWORD(input.b); }
	DWORD operator()( const CVec4 &input ) const { return (MakeComponent(input.w) << 24) | (MakeComponent(input.x) << 16) | (MakeComponent(input.y) << 8) | MakeComponent(input.z); }
	bool IsReady() const { return true; }
};
template <> class CRawColorConvertor<SColor24>
{
public:
	CRawColorConvertor( int nColorMapLength, int nColorMapEntrySize, CDataStream *pStream ) {}
	SColor24 operator()( const DWORD input ) const { const SColor24 color = { GetB(input), GetG(input), GetR(input)	}; return color; }
	SColor24 operator()( const SColor24 input ) const { return input; }
	SColor24 operator()( const CVec4 &input ) const { const SColor24 color = { MakeComponent(input.z), MakeComponent(input.y), MakeComponent(input.x) }; return color; }
	bool IsReady() const { return true; }
};
template <> class CRawColorConvertor<BYTE>
{
public:
	CRawColorConvertor( int nColorMapLength, int nColorMapEntrySize, CDataStream *pStream ) {}
	BYTE operator()( const DWORD input ) const { return MakeGray( GetR(input), GetG(input), GetB(input) ); }
	BYTE operator()( const SColor24 input ) const { return MakeGray( input.r, input.g, input.b ); }
	BYTE operator()( const CVec4 &input ) const { return MakeGray( input.r, input.g, input.b ); }	
	bool IsReady() const { return true; }
};
template <> class CRawColorConvertor<CVec4>
{
public:
	CRawColorConvertor( int nColorMapLength, int nColorMapEntrySize, CDataStream *pStream ) {}
	CVec4 operator()( const DWORD input ) const { return CVec4( GetR(input) / 255.0f, GetG(input) / 255.0f, GetB(input) / 255.0f, GetA(input) / 255.0f ); }
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
			swap( data[nEnd][x], data[nBegin][x] );
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
			swap( data[y][nEnd], data[y][nBegin] );
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

