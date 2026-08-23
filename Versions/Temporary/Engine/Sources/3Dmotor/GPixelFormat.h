#pragma once

#include <cstdint>

namespace NGfx
{

// The underlying type is spelled out so the declarations in Image can be legal
// C++: an enum can only be declared without its definition when it is fixed.
// int is also what MSVC already picked here, where GCC picks unsigned for an
// unfixed enum whose enumerators are all non-negative, so this makes the two
// agree rather than changing either. CF_FORCE_DWORD below now says the same
// thing twice and is kept only because removing an enumerator is its own call.
enum EPixelFormat : int
{
	CF_DXT1     = 1,
	CF_DXT2     = 2,
	CF_DXT3     = 3,
	CF_DXT4     = 4,
	CF_DXT5     = 5,
	CF_A8R8G8B8 = 6,
	CF_A4R4G4B4 = 7,
  CF_R5G6B5   = 8,
  CF_A1R5G5B5 = 9,
	CF_R32F     = 10,
	CF_A32R32G32B32 = 11,

	CF_FORCE_DWORD = 0x7FFFFFFF
};
inline int GetBPP( EPixelFormat format )
{
	switch ( format )
	{
		case CF_DXT1: return 4;
		case CF_DXT2: return 8;
		case CF_DXT3: return 8;
		case CF_DXT4: return 8;
		case CF_DXT5: return 8;
		case CF_A8R8G8B8: return 32;
		case CF_A4R4G4B4: return 16;
  	case CF_R5G6B5:   return 16;
  	case CF_A1R5G5B5: return 16;
		case CF_A32R32G32B32: return 128;
		default: return 0;
	}
}

struct SPixelFloat
{
	enum { ID = CF_R32F, XSize = 1, YSize = 1 };
	float r;
	SPixelFloat() {}
	SPixelFloat( float _r ) : r( _r ) {}
};
struct SPixelFFFF
{
	enum { ID = CF_A32R32G32B32, XSize = 1, YSize = 1 };
	float b, g, r, a;
	SPixelFFFF() {}
	SPixelFFFF( float _r, float _g, float _b, float _a ) : r(_r), g(_g), b(_b), a(_a) {}
};
struct SPixel8888
{
	enum { ID = CF_A8R8G8B8, XSize = 1, YSize = 1 };
	union
	{
		uint32_t dwColor;
		struct
		{
			uint32_t b : 8;
			uint32_t g : 8;
			uint32_t r : 8;
			uint32_t a : 8;
		};
	};
	SPixel8888() {}
	SPixel8888( unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a = 0xFF )
		: b( _b ), g( _g ), r( _r ), a( _a ) {}
	SPixel8888( uint32_t _dwColor ) : dwColor(_dwColor ){}
};
inline bool operator==( const SPixel8888 &a, const SPixel8888 &b ) { return a.dwColor == b.dwColor; }

struct SPixel1555
{
	enum { ID = CF_A1R5G5B5, XSize = 1, YSize = 1 };
	union
	{
		uint16_t wColor;
		struct
		{
			uint16_t b : 5;
			uint16_t g : 5;
			uint16_t r : 5;
			uint16_t a : 1;
		};
	};
	SPixel1555() {}
	SPixel1555( unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a = 1 )
		: b( _b ), g( _g ), r( _r ), a( _a ) {}
};
struct SPixel565
{
	enum { ID = CF_R5G6B5, XSize = 1, YSize = 1 };
	union
	{
		uint16_t wColor;
		struct
		{
			uint16_t b : 5;
			uint16_t g : 6;
			uint16_t r : 5;
		};
	};
	SPixel565() {}
	SPixel565( unsigned char _r, unsigned char _g, unsigned char _b )
		: b( _b ), g( _g ), r( _r ) {}
};
struct SPixel4444
{
	enum { ID = CF_A4R4G4B4, XSize = 1, YSize = 1 };
	union
	{
		uint16_t wColor;
		struct
		{
			uint16_t b : 4;
			uint16_t g : 4;
			uint16_t r : 4;
			uint16_t a : 4;
		};
	};
	SPixel4444() {}
	SPixel4444( unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a = 0xF )
		: b( _b ), g( _g ), r( _r ), a( _a ) {}
};
struct SPixelDXT1
{
	enum { ID = CF_DXT1, XSize = 4, YSize = 4 };
	uint16_t color1, color2;
	uint32_t colors;
};
struct SPixelDXT2
{
	enum { ID = CF_DXT2, XSize = 4, YSize = 4 };
	uint32_t colors1, colors2, colors3, colors4;
};
struct SPixelDXT3
{
	enum { ID = CF_DXT3, XSize = 4, YSize = 4 };
	uint32_t colors1, colors2, colors3, colors4;
};
struct SPixelDXT4
{
	enum { ID = CF_DXT4, XSize = 4, YSize = 4 };
	uint32_t colors1, colors2, colors3, colors4;
};
struct SPixelDXT5
{
	enum { ID = CF_DXT5, XSize = 4, YSize = 4 };
	uint32_t colors1, colors2, colors3, colors4;
};

struct SShortTextureUV
{
	union
	{
		struct { short nU, nV; };
		uint32_t dw;
	};
};

// range [0, 255], from float [-1.0, 1.0], value 128 represents zero
struct SCompactVector
{
	union
	{
		struct { unsigned char z, y, x, w; };
		uint32_t dw;
	};
};

inline void CalcCompactVector( SCompactVector *pRes, const CVec3 &v )
{
	pRes->x = Clamp( Float2Int( v.x * 127 ) + 128, 0, 255 );
	pRes->y = Clamp( Float2Int( v.y * 127 ) + 128, 0, 255 );
	pRes->z = Clamp( Float2Int( v.z * 127 ) + 128, 0, 255 );
	pRes->w = 255;
}

inline CVec3 GetVector( const SCompactVector &a )
{
	return CVec3( ( ((int)a.x) - 128 ) / 127.0f, ( ((int)a.y) - 128 ) / 127.0f, ( ((int)a.z) - 128 ) / 127.0f );
}

inline uint32_t Get255Range( float f )
{
	return (std::max)( 0, (std::min)( 255, Float2Int( f * 256 ) ) );
}
inline uint32_t GetDWORDColor( const CVec4 &color )
{
	return Get255Range( color.z ) + (Get255Range( color.y ) << 8) +
		(Get255Range( color.x ) << 16) + (Get255Range( color.w ) << 24);
}
inline SPixel8888 Get8888Color( const CVec4 &color )
{
	return SPixel8888( Get255Range( color.r ), Get255Range( color.g ), Get255Range( color.b ), Get255Range( color.a ) );
}
inline CVec4 GetCVec4Color( uint32_t cr )
{
	return CVec4( cr >> 16 & 0xff, cr >> 8 & 0xff, cr & 0xff, cr >> 24 & 0xff ) / 255;
}

struct CInterpolateColor
{
	typedef uint32_t RET;
	uint32_t operator()( uint32_t a, uint32_t b, float f ) const
	{ 
		return
			Float2Int( (a&0xff) * (1-f) + (b&0xff) * f ) +
			( Float2Int( ((a>>8)&0xff) * (1-f) + ((b>>8)&0xff) * f ) << 8 ) +
			( Float2Int( ((a>>16)&0xff) * (1-f) + ((b>>16)&0xff) * f ) << 16 ) +
			( Float2Int( ((a>>24)&0xff) * (1-f) + ((b>>24)&0xff) * f ) << 24 );
	}
};

struct STriangleList
{
	const STriangle *pTri;
	int nTris;
	int nBaseIndex;
	int nOffset;

	STriangleList() : pTri(0), nTris(0), nBaseIndex(0), nOffset(0) {}
	STriangleList( const std::vector<STriangle> &t ) : pTri( &t[0] ), nTris( t.size() ), nBaseIndex(0), nOffset(0) {}
	STriangleList( const STriangle *_pTri, int _nTris, int _nBaseIndex = 0, int _nOffset = 0 ) : pTri(_pTri), nTris(_nTris), nBaseIndex(_nBaseIndex), nOffset(_nOffset) {}
};

struct SMMXWord
{
	short nZ, nY, nX, nW;
};
struct SCompactTransformer
{
	SMMXWord a, b, c;
};

}; // namespace NGfx


