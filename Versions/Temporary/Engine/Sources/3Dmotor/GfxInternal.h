#pragma once
// full description of buffers & textures for internal use & some internal data access

#include <D3D9.h>
#include "Misc/Win32Helper.h"
#include "GPixelFormat.h"

#include <cstdint>

namespace NGfx
{
	class CGeometry;
	class CTexture;
	class CCubeTexture;
	enum EFace : int;
	enum EWrap : int;
	extern NWin32Helper::com_ptr<IDirect3D9> pD3D;
	extern NWin32Helper::com_ptr<IDirect3DDevice9> pDevice;
	extern bool bNoTexture;
	extern int nCurrentFrame;
	extern bool bHardwareVP, bHardwarePixelShaders, bHardwarePixelShaders14, bHardwarePixelShaders20, bHardwarePixelShaders20a;
	extern bool bTnLDevice;
	extern bool bDoValidateDevice;
	extern bool bNVHackNP2, bBanNP2, bBan32BitIndices, bStaticNooverwrite;
	extern bool bNoCubeMapMipLevels;
	extern int nUseAnisotropy;
	extern D3DCAPS9 devCaps;
	extern int nTotalFrames;
	extern bool bInitOk;
	extern bool b16BitMode, bDXTSupported, b8888Supported, b16BitTextures;

// Vertex info

struct SGeomFormatInfo
{
	int nFormatID;
	int nSize;
	D3DVERTEXELEMENT9 *pdwVSD;
	uint32_t dwFVF;
};
extern SGeomFormatInfo geometryFormatInfo[6];

inline int GetGeomFormatSize( int nFormatID )
{
	ASSERT( nFormatID >= 0 && nFormatID < std::size( geometryFormatInfo ) );
	ASSERT( nFormatID == geometryFormatInfo[nFormatID].nFormatID );
	return geometryFormatInfo[nFormatID].nSize;
}

inline const D3DVERTEXELEMENT9* GetVertexLayout( int nFormatID )
{
	ASSERT( nFormatID >= 0 && nFormatID < std::size( geometryFormatInfo ) );
	ASSERT( nFormatID == geometryFormatInfo[nFormatID].nFormatID );
	return geometryFormatInfo[nFormatID].pdwVSD;
}

inline int D3DFormat2PixelID( D3DFORMAT format )
{
	switch ( format )
	{
  case D3DFMT_A8R8G8B8: return CF_A8R8G8B8;
  case D3DFMT_X8R8G8B8: return CF_A8R8G8B8;
  case D3DFMT_R5G6B5:   return CF_R5G6B5;
	case D3DFMT_X1R5G5B5: return CF_A1R5G5B5;
	case D3DFMT_A1R5G5B5: return CF_A1R5G5B5;
	case D3DFMT_A4R4G4B4: return CF_A4R4G4B4;
	case D3DFMT_X4R4G4B4: return CF_A4R4G4B4;
	case D3DFMT_DXT1: return CF_DXT1;
	case D3DFMT_DXT2: return CF_DXT2;
	case D3DFMT_DXT3: return CF_DXT3;
	case D3DFMT_DXT4: return CF_DXT4;
	case D3DFMT_DXT5: return CF_DXT5;
	case D3DFMT_R32F: return CF_R32F;
	case D3DFMT_A32B32G32R32F: return CF_A32R32G32B32;
	}
	ASSERT(0);
	return -1;
}

inline int D3DFormat2PixelBitSize( D3DFORMAT format )
{
	switch ( format )
	{
  case D3DFMT_A8R8G8B8: return 4*8;
  case D3DFMT_X8R8G8B8: return 4*8;
  case D3DFMT_R5G6B5:   return 2*8;
	case D3DFMT_X1R5G5B5: return 2*8;
	case D3DFMT_A1R5G5B5: return 2*8;
	case D3DFMT_A4R4G4B4: return 2*8;
	case D3DFMT_X4R4G4B4: return 2*8;
	case D3DFMT_DXT1: return 4; // ???
	case D3DFMT_DXT2: return 8;
	case D3DFMT_DXT3: return 8;
	case D3DFMT_DXT4: return 8;
	case D3DFMT_DXT5: return 8;
	case D3DFMT_R32F: return 32;
	}
	ASSERT(0);
	return -1;
}

inline D3DFORMAT PixelID2D3DFormat( int nPixelID )
{
	switch ( nPixelID )
	{
  case CF_A8R8G8B8: return D3DFMT_A8R8G8B8;
  case CF_R5G6B5:   return D3DFMT_R5G6B5;
	case CF_A1R5G5B5: return D3DFMT_A1R5G5B5;
	case CF_A4R4G4B4: return D3DFMT_A4R4G4B4;
	case CF_DXT1: return D3DFMT_DXT1;
	case CF_DXT2: return D3DFMT_DXT2;
	case CF_DXT3: return D3DFMT_DXT3;
	case CF_DXT4: return D3DFMT_DXT4;
	case CF_DXT5: return D3DFMT_DXT5;
	case CF_R32F: return D3DFMT_R32F;
	case CF_A32R32G32B32: return D3DFMT_A32B32G32R32F;
	}
	ASSERT( 0 );
	return D3DFMT_A8R8G8B8;
}

class CGeometry : virtual public CObjectBase
{
public:
	virtual void DoTouch() = 0;
	virtual void SetVertexStream() = 0;
	virtual void* GetVertexStream() = 0;
	virtual int GetVBStart() const = 0;
	virtual int GetVBSize() const = 0;
	virtual int GetGeometryFormatID() = 0;
};

struct S3DTriangle;
class CTriListWrapper;
class CTriList: virtual public CObjectBase
{
public:
	virtual const std::vector<S3DTriangle>& GetTris() const = 0;
	virtual int GetTrisNumber() const = 0;
	virtual void DrawPrimitive( int nVBStart, int nMinIndex, int nMaxIndex ) = 0;
	virtual CTriListWrapper* CreateWrapper( int nTris ) = 0;
};

void InitBuffers();
void NextFrameBuffes( bool bOnThrashing = false );
void ReduceHWLag();
void SetTexture( int nStage, CTexture *pTex );
void SetTexture( int nStage, CCubeTexture *pTex );
void DestroyLostableBuffers();
void DestroyManagedBuffers();
void InitRender();
bool InitZBuffer( D3DFORMAT format );
void DoneZBuffer();
void GetSurface( CTexture *pTexture, int nLevel, NWin32Helper::com_ptr<IDirect3DSurface9> *pRes );
void GetSurface( CCubeTexture *pTexture, EFace face, int nLevel, NWin32Helper::com_ptr<IDirect3DSurface9> *pRes );
EWrap GetWrap( CTexture *pTex );
void DoneRender();
CTexture* MakeRenderTarget( int nXSize, int nYSize, int nPixelID );
void AddPrimitiveGeometry( CGeometry *pGeom, CTriList *pTriList, int nStartVertex, int nVertices );
void AddPrimitiveGeometry( CGeometry *pGeom, CTriList *pTriList );
void AddPrimitiveGeometry( CGeometry *pGeom, const STriangleList *pTris, int nCount, unsigned nMask );
void FlushPrimitive();
void AddLineStrip( CGeometry *pGeom, const unsigned short *pIndices, int nLines );
bool CanOverwriteStatic();
int GetBpp( D3DFORMAT format );
int GetMaxBufferedFlipsNum();
void ForceTextureFilterSetup();

} // namespace


