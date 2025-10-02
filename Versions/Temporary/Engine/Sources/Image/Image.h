#pragma once

template<class T> class CArray2D;

struct IDirect3DDevice9;
namespace NGfx
{
enum EPixelFormat;
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

bool Copy( const CArray2D<DWORD> &src, const CTRect<long> *pSrcRect, CArray2D<DWORD> &dst, const CTPoint<long> &dstPos = CTPoint<long>(0, 0) );
bool CopyAB( const CArray2D<DWORD> &src, const CTRect<long> *pSrcRect, CArray2D<DWORD> &dst, const CTPoint<long> &dstPos = CTPoint<long>(0, 0) );
//
bool LoadAnyImage( CArray2D<DWORD> *pRes, CDataStream *pStream );
void ConvertAndSaveAsDDSWithDX( IDirect3DDevice9 * pDevice, const string &szFileName, const CArray2D<DWORD> &srcImage,
	EImageType eImageType, NGfx::EPixelFormat nSubFormat, int nNumMipLevels, bool bWrapX, bool bWrapY, float fMappingSize );

struct SColor
{
  union
  {
    struct  
    {
      BYTE b, g, r, a;
    };
    DWORD dwColor;
  };
  //
  SColor() {  }
  SColor( const DWORD _dwColor ) : dwColor( _dwColor ) {  }
  SColor( const BYTE _a, const BYTE _r, const BYTE _g, const BYTE _b ) : a( _a ), r( _r ), g( _g ), b( _b ) {  }
  //
  operator DWORD() const { return dwColor; }
};
}

