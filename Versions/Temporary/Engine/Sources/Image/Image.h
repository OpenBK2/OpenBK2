#pragma once

#include <cstdint>

template<class T> class CArray2D;

struct IDirect3DDevice9;
namespace NGfx
{
enum EPixelFormat : int;
}
namespace NImage
{

enum EImageType
{
	IMAGE_TYPE_PICTURE_FASTMIP,
	IMAGE_TYPE_PICTURE,
	IMAGE_TYPE_BUMP,
	IMAGE_TYPE_TRANSPARENT,
	IMAGE_TYPE_TRANSPARENT_ADD,
};

bool Copy( const CArray2D<uint32_t> &src, const CTRect<long> *pSrcRect, CArray2D<uint32_t> &dst, const CTPoint<long> &dstPos = CTPoint<long>(0, 0) );
bool CopyAB( const CArray2D<uint32_t> &src, const CTRect<long> *pSrcRect, CArray2D<uint32_t> &dst, const CTPoint<long> &dstPos = CTPoint<long>(0, 0) );
//
bool LoadAnyImage( CArray2D<uint32_t> *pRes, CDataStream *pStream );
void ConvertAndSaveAsDDSWithDX( IDirect3DDevice9 * pDevice, const std::string &szFileName, const CArray2D<uint32_t> &srcImage,
	EImageType eImageType, NGfx::EPixelFormat nSubFormat, int nNumMipLevels, bool bWrapX, bool bWrapY, float fMappingSize );

struct SColor
{
  union
  {
    struct  
    {
      uint8_t b, g, r, a;
    };
    uint32_t dwColor;
  };
  //
  SColor() {  }
  SColor( const uint32_t _dwColor ) : dwColor( _dwColor ) {  }
  SColor( const uint8_t _a, const uint8_t _r, const uint8_t _g, const uint8_t _b ) : a( _a ), r( _r ), g( _g ), b( _b ) {  }
  //
  operator uint32_t() const { return dwColor; }
};
}

