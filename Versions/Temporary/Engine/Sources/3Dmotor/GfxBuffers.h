#pragma once
#include "3dmotor_export.h"


#include "GPixelFormat.h"

struct IDirect3DSurface9;

namespace NGfx
{
	enum ETextureUsage
	{
		REGULAR,
		TARGET,
		TEXTURE_2D,
		TRANSPARENT_TEXTURE,
		DYNAMIC_TEXTURE
	};
	enum EAccess
	{
		READWRITE,
		WRITEONLY,
		READONLY,
		INPLACE,
		INPLACE_READONLY
	};
	enum EFace
	{
		POSITIVE_X,
		POSITIVE_Y,
		POSITIVE_Z,
		NEGATIVE_X,
		NEGATIVE_Y,
		NEGATIVE_Z
	};
	enum EWrap
	{
		CLAMP = 0,
		WRAP_X = 1,
		WRAP_Y = 2,
		WRAP = 3,
	};
	enum EBufferUsage
	{
		STATIC,
		DYNAMIC
	};

// VERTEX/INDEX formats

struct S3DTriangle
{
	enum { ID = 100 };
	WORD i1, i2, i3;

	S3DTriangle() {}
	S3DTriangle( WORD _i1, WORD _i2, WORD _i3 ): i1(_i1), i2(_i2), i3(_i3) {}
};

//struct SGeomVecT1C1
//{
//	enum { ID = 2 };
//	CVec3 pos;
//	SPixel8888 color;
//	CVec2 tex;
//};

struct SGeomVecT2C1
{
	enum { ID = 4 };
	CVec3 pos;
	SPixel8888 color;
	CVec2 tex1, tex2;
};
//struct SGeomVec
//{
//	enum { ID = 0 };
//	CVec3 pos;
//};

const int N_VEC_FULL_TEX_SIZE = 2048;
inline void CalcTexCoords( SShortTextureUV *pRes, float fU, float fV ) 
{
	pRes->nU = Float2Int( fU * N_VEC_FULL_TEX_SIZE );
	pRes->nV = Float2Int( fV * N_VEC_FULL_TEX_SIZE );
}
inline void CalcLMCoords( SShortTextureUV *pRes, float fU, float fV ) 
{
	pRes->nU = Min( Float2Int( fU * 65536 ), 65535 ) - 32768;
	pRes->nV = Min( Float2Int( fV * 65536 ), 65535 ) - 32768;
}
inline CVec2 GetTexCoords( const SShortTextureUV &src )
{
	return CVec2( src.nU * (1.0f / N_VEC_FULL_TEX_SIZE), src.nV * (1.0f / N_VEC_FULL_TEX_SIZE) );
}
inline void MakeLMToScreenMatrix( SHMatrix *pM, int nXTargetSize, int nYTargetSize )
{
	pM->x = CVec4( 2.0f / 65536, 0, 0, 1 - 1 - 1.0f / nXTargetSize ); // N_RENDER_TARGET_SIZE
	pM->y = CVec4( 0,-2.0f / 65536, 0, -1 + 1 + 1.0f / nYTargetSize );
	pM->z = CVec4( 0, 0, 1, 0.5f ); // since texture is expanded to (u,v,0,1) this works and matrix has inverse
	pM->w = CVec4( 0, 0, 0, 1 );
}
struct SGeomVecFull
{
	enum { ID = 5 };
	CVec3 pos;
	SCompactVector normal;
	SShortTextureUV tex, texLM;
	SCompactVector texU, texV;
};

// linear buffer locks

class ISomeBuffer : virtual public CObjectBase
{
public:
	virtual int GetFormatID() const = 0;
	virtual void SetSize( int nSize ) = 0;
	virtual int GetSize() const = 0;
	virtual int GetBufSize() const = 0;
};

class ILinearBuffer : public ISomeBuffer
{
public:
	virtual void* Lock() = 0;
	virtual void Unlock() = 0;
};

ILinearBuffer* CreateBuffer( int nFormatID, int nSize, EBufferUsage usage ); // for internal use in BufferLock<>
template<class TElement>
class CBufferLock
{
	CPtr<ILinearBuffer> pObj;
	TElement *pStart;
	typedef TElement Element;
public:
	template<class T>
	CBufferLock( CObj<T> *pRes, int nSize, EBufferUsage usage = DYNAMIC )
	{
		*pRes = 0;
		pObj = CreateBuffer( TElement::ID, nSize, usage );
		*pRes = CastToUserObject( pObj, pRes->GetPtr() );
		ASSERT( IsValid( *pRes ) );
		ASSERT( pObj->GetFormatID() == Element::ID );
		pStart = (TElement*)pObj->Lock();
	}
	~CBufferLock() { pObj->Unlock(); }
	//
	void SetSize( int nSize ) { pObj->SetSize( nSize ); }
	int GetSize() const { return pObj->GetSize(); }
	int GetStride() const { return sizeof( TElement ); }
	TElement& operator[]( int n ) const
	{
		ASSERT( n >= 0 && n < pObj->GetBufSize() );
		return ((TElement*)pStart)[n];
	}
};

// 2D buffer locks

class I2DBufferLock
{
public:
	virtual ~I2DBufferLock() {}
	virtual void* GetBuffer() = 0;
	virtual int GetStride() = 0;
};

class I2DBuffer : virtual public CObjectBase
{
public:
	virtual int GetPixelID() = 0;
	virtual I2DBufferLock* Lock( int nLevel, EAccess access ) = 0;
	virtual int GetSizeX() const = 0;
	virtual int GetSizeY() const = 0;
	virtual int GetNumMipLevels() const = 0;
	virtual int GetFrameMRU() const = 0;
	virtual void UserTouch() = 0;
	bool IsNP2() const { return GetNextPow2(GetSizeX()) != GetSizeX() || GetNextPow2(GetSizeY()) != GetSizeY(); }
};

class ICubeBuffer : public CObjectBase
{
public:
	virtual int GetPixelID() = 0;
	virtual I2DBufferLock* Lock( EFace face, int nLevel, EAccess access ) = 0;
	virtual int GetSize() const = 0;
	virtual int GetNumMipLevels() const = 0;
};

template<class TPixel>
class CTextureLock
{
	I2DBufferLock *pLock;
	NGfx::EAccess access;
	vector<void*> raws;
	int nXSize, nYSize, nStride;
	typedef TPixel Element;
	
	CTextureLock( const CTextureLock &a ) { ASSERT( 0 ); }
	CTextureLock& operator=( const CTextureLock & a ) { ASSERT( 0 ); return *this; }
	void InitAccess()
	{
		char *pStart = (char*)pLock->GetBuffer();
		raws.resize( Max( 1, nYSize ) );
		vector<void*>::iterator tek = raws.begin(), fin = raws.end();
		while ( tek != fin )
		{
			*tek++ = pStart;
			pStart += nStride;
		}
	}
public:
	CTextureLock( const CDynamicCast<I2DBuffer> &pObj, int nLevel, EAccess access )
	{
		ASSERT( IsValid( pObj ) );
		ASSERT( pObj->GetPixelID() == Element::ID );
		nXSize = (pObj->GetSizeX() >> nLevel) / TPixel::XSize;
		nYSize = (pObj->GetSizeY() >> nLevel) / TPixel::YSize;
		pLock = pObj->Lock( nLevel, access );
		nStride = pLock->GetStride();
		InitAccess();
	}
	CTextureLock( const CDynamicCast<ICubeBuffer> &pObj, EFace face, int nLevel, EAccess access )
	{
		ASSERT( IsValid( pObj ) );
		ASSERT( pObj->GetPixelID() == Element::ID );
		nXSize = (pObj->GetSize() >> nLevel) / TPixel::XSize;
		nYSize = (pObj->GetSize() >> nLevel) / TPixel::YSize;
		pLock = pObj->Lock( face, nLevel, access );
		nStride = pLock->GetStride();
		InitAccess();
	}
	~CTextureLock() { delete pLock; }
	// get physical size (in physical pixels, for DXT real size / 4 f.e.
	int GetSizeX() const { return nXSize; } 
	int GetSizeY() const { return nYSize; }
	int GetStride() const { return nStride; }
	TPixel* operator[]( int nY ) { return (TPixel*)raws[nY]; }
};

// place for texture
struct STexturePlaceInfo
{
	CTRect<int> place;
	CTPoint<int> size;

	bool IsWhole() const { return place.x1 == 0 && place.y1 == 0 && place.x2 == size.x && place.y2 == size.y; }
	bool IsHolderAPow2Texture() const { return GetNextPow2(size.x) == size.x && GetNextPow2(size.y) == size.y; }
};

// could not be copied
struct _3DMOTOR_EXPORT INew2DTexAllocCallback
{
	INew2DTexAllocCallback();
	~INew2DTexAllocCallback();
	virtual void NewTextureWasAllocated() = 0;
};

// buffers handling
class CGeometry;
class CTriList;
class CTexture;
class CCubeTexture;
CTriList* MakeWrapper( CTriList *pSrc, int nTris );
_3DMOTOR_EXPORT CTexture* MakeTexture( int nXSize, int nYSize, int nMipLevels, int nPixelID, ETextureUsage eUsage, EWrap bWrap );
CCubeTexture* MakeCubeTexture( int nSize, int nMipLevels, int nPixelID, ETextureUsage eUsage );
CTexture* GetTextureCache();
CTexture* GetTransparentTextureCache();
CTexture* GetTextureContainer( CTexture *pTex, STexturePlaceInfo *pPlace );
CTexture* GetLinearBufferMRU( EBufferUsage usage );
bool HasSameContainer( CTexture *p1, CTexture *p2 );
void GetRenderTargetData( NGfx::CTexture *pTarget, NGfx::CTexture *pSrc );
void GetRenderTargetData( CArray2D<NGfx::SPixel8888> *pRes, NGfx::CTexture *pSrc );
int CalcTouchedTextureSize();
int CalcTouchedTextureSizeNotSetMip( int nMip );
int CalcTotalTextureSize( int *pnTexturesCount );
void SetLODToAllTextures( int nLOD );
bool IsStaticGeometryThrashing();
bool IsDynamicGeometryThrashing();
bool Is2DTextureThrashing();
bool IsTransparentThrashing();
bool CanStreamGeometry();
_3DMOTOR_EXPORT bool Is16BitTextures();
void ReplaceTextureSurface( CTexture *pTex, int nLevel, IDirect3DSurface9 *pSurf );
void FlushQueue();

}


