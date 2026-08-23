#include "stdafx.h"
#include <D3D9.h>
#include "GfxBuffers.h"
#include "Cache.h"
#include "System/Commands.h"
#include "Misc/2Darray.h"
#include "GfxBuffersInternal.h"

#include "port/debugging.h"

#include <algorithm>
#include <cstdint>

#include <boost/config.hpp>

const int N_SYSMEM_TEXTURES = 2;
const int N_SYSMEM_TEXTURE_SIZE = 1024;
const int N_MAX_PRESENTS_IN_QUEUE = 4;

namespace NGfx
{
extern SRenderTargetsInfo rtInfo;
static void OnThrashing();

// INew2DTexAllocCallback

static std::vector<INew2DTexAllocCallback*> alloc2Dcallback;
INew2DTexAllocCallback::INew2DTexAllocCallback()
{
	alloc2Dcallback.push_back( this );
}

INew2DTexAllocCallback::~INew2DTexAllocCallback()
{
	// alloc2Dcallback.remove( this );
	auto this_pos = std::find(alloc2Dcallback.begin(), alloc2Dcallback.end(), this);
	if ( this_pos != alloc2Dcallback.end() )
		alloc2Dcallback.erase(this_pos);
}

static void InformNew2DTextureAlloc()
{
	for ( std::vector<INew2DTexAllocCallback*>::iterator i = alloc2Dcallback.begin(); i != alloc2Dcallback.end(); ++i )
		(*i)->NewTextureWasAllocated();
}


static std::vector<CPtr<CLockable> > lockableList;
bool bWasLinearBufferLock;

template <class T>
T* RegisterDXBuffer( T *p )
{
	lockableList.push_back( p );
	return p;
}

void FreeLinearBuffers()
{
	EraseInvalidRefs( &lockableList );
	for ( std::vector<CPtr<CLockable> >::iterator i = lockableList.begin(); i != lockableList.end(); )
	{
		CLockable *p = *i;
		if ( IsValid( p ) )
		{
			p->Free();
			++i;
		}
		else
			i = lockableList.erase( i );
	}
	bWasLinearBufferLock = false;
}

static void RealSetVertexStream( IDirect3DVertexBuffer9 *pBuf, int nStride );

template<class TDXBuffer, class TUserObject>
class CLinearBuffer : public CObjectBase
{
	OBJECT_NOCOPY_METHODS(CLinearBuffer);
	typedef NCache::CGatheringCache<NCache::CShortPtrAllocator, NCache::CFibElement, TUserObject> CCache;
	CObj<TDXBuffer> pDX;
	CObj<CCache> pCache;
	int nFormatID;
	int nStride;
	uint32_t dwNextLockFlags, dwLockFlags, dwFirstLockFlags;
	bool bFlushBufferOnNewFrame, bIsThrashing;
	bool bAccountQueueDepth;
	struct SBuffersPerFrame
	{
		std::vector<CObj<TUserObject> > data;
	};
	std::deque<SBuffersPerFrame> frames;
public:
	CLinearBuffer() {}
	CLinearBuffer( int _nSize, int _nStride, int _nFormatID, ETrueBufferUsage usage ) : nFormatID(_nFormatID), nStride(_nStride), bIsThrashing(false)
	{
		int nFibSize = NCache::GetMajorFib( _nSize );
		int nSize = NCache::fib(nFibSize);
		pDX = RegisterDXBuffer( new TDXBuffer( nSize * _nStride, usage ) );//DYNAMIC ) );
		pCache = new CCache( nCurrentFrame );
		NCache::CFibElement root;
		root.nSize = nFibSize;
		root.nShift = 0;
		pCache->AddRoot( root );
		frames.emplace_front();
		if ( usage == DYNAMIC )
		{
			//bFlushBufferOnNewFrame = true;
			//dwLockFlags = D3DLOCK_NOOVERWRITE;
			//dwFirstLockFlags = D3DLOCK_DISCARD;
			//bAccountQueueDepth = false;
			bFlushBufferOnNewFrame = false;
			dwLockFlags = D3DLOCK_NOOVERWRITE;
			dwFirstLockFlags = D3DLOCK_NOOVERWRITE;
			bAccountQueueDepth = true;
		}
		else
		{
			bFlushBufferOnNewFrame = false;
			if ( CanOverwriteStatic() )
			{
				dwLockFlags = D3DLOCK_NOOVERWRITE;
				bAccountQueueDepth = true;
			}
			else
			{
				dwLockFlags = 0;
				bAccountQueueDepth = false;
			}
			dwFirstLockFlags = dwLockFlags;
		}
		dwNextLockFlags = dwFirstLockFlags;
	}
	TDXBuffer* GetBuffer() const { return pDX; }
	int GetFormatID() const {	return nFormatID; }
	int GetStride() const { return nStride; }
	TUserObject* Alloc( int nSize )
	{
		ASSERT( IsValid( pCache ) );
		//ASSERT( nSize < 65536 );
		//NCache::MRU_TYPE nBestMRU = NCache::MRU_LAST;
		NCache::CFibElement el;
		el.nSize = NCache::GetMajorFib( nSize );
		CCache::SCachePlace best;
		if ( !pCache->GetPlace( el, &best ) )
		{
			ASSERT( 0 );
			return 0;
		}
		ASSERT( pCache->GetCurrentRU() == nCurrentFrame );
		if ( best.nMRU >= nCurrentFrame - GetMaxBufferedFlipsNum() )
		{
			bIsThrashing = true;
			OnThrashing();
			Flush();
			// recalc place after trouble happened
			if ( !pCache->GetPlace( el, &best ) )
			{
				ASSERT( 0 );
				return 0;
			}
		}
		TUserObject *pRes = new TUserObject( this );
		pCache->PerformAlloc( pRes, &best );
		pRes->nStart = best.resPlace.nShift;
		pRes->nBufSize = NCache::fib( best.resPlace.nSize );
		pRes->nSize = nSize;
		WasTouched( pRes );
		return pRes;
	}
	void Flush()
	{
		// wait till rendered
		HRESULT hr;
		void *pFake;
		hr = pDX->obj->Lock( 0, 0, &pFake, 0 );
		ASSERT( hr == D3D_OK );
		hr = pDX->obj->Unlock();
		ASSERT( hr == D3D_OK );
	}
	void NextFrame( bool bOnThrashing, bool *pbNeedFlush ) 
	{
		pCache->AdvanceFrameCounter();
		int nReserve = 10;
		if ( !frames.empty() )
			nReserve = frames.front().data.size() + 64;
		frames.emplace_front();
		frames.front().data.reserve(nReserve);
		// remove all elements for dynamic buffer
		if ( bFlushBufferOnNewFrame )
			pCache->Clear();
		int nKeepObjectsFrames = bAccountQueueDepth ? GetMaxBufferedFlipsNum() : 0;//N_MAX_PRESENTS_IN_QUEUE + 1;
		while ( frames.size() > nKeepObjectsFrames + 1 )
			frames.pop_back();
		if ( !bOnThrashing )
		{
			if ( bIsThrashing )
			{
				pCache->Clear();
				if ( pbNeedFlush )
					*pbNeedFlush = true;
				else
					ASSERT(0); // dx exec queue should be flushed in this case
			}
			bIsThrashing = false;
		}
		dwNextLockFlags = dwFirstLockFlags;
	}
	void WasTouched( TUserObject *p ) { frames.front().data.push_back( p ); }
	void CalcStats( NCache::SStats *pStats ) 
	{ 
		if ( IsValid( pCache ) )
			pCache->CalcStats( pStats ); 
		pStats->bThrashing |= bIsThrashing;
	}
	void DrawRU( CTexture *pTarget )
	{
		CTextureLock<SPixel8888> tl( pTarget, 0, INPLACE );
		std::vector<CCache::SStatePlace> places;
		pCache->GetState( &places );
		for ( int y = 0; y < tl.GetSizeY(); ++y )
			for ( int x = 0; x < tl.GetSizeX(); ++x )
				tl[y][x] = SPixel8888( 255,255,255 );
		for ( int k = 0; k < places.size(); ++k )
		{
			const CCache::SStatePlace &p = places[k];
			SPixel8888 color;
			if ( p.pUser )
			{
				switch ( pCache->GetCurrentRU() - p.nMRU )
				{
				case 0: color = SPixel8888( 0,0,255 ); break;
				case 1: color = SPixel8888( 0,0,200 ); break;
				case 2: color = SPixel8888( 0,0,100 ); break;
				default: color = SPixel8888( 0,0,50 ); break;
				}
			}
			else
			{
				ASSERT( p.nMRU == 0 );
				color = SPixel8888( 255,0,0 );
			}
			const NCache::CFibElement &fe = p.place;
			const int N_XSTEP = 1;
			const int N_YSTEP = 1;
			int nPerScanline = tl.GetSizeX() / N_XSTEP;
			for ( int i = 0; i < NCache::fib( fe.nSize ); ++i )
			{
				int n = fe.nShift + i, x1 = ( n & (nPerScanline-1) ) * N_XSTEP, y1 = (n/nPerScanline) * N_YSTEP;
				for ( int y = 0; y < N_YSTEP; ++y )
				{
					for ( int x = 0; x < N_XSTEP; ++x )
						tl[y1+y][x1+x] = color;
				}
			}
		}
	}

	unsigned char* Lock() { pDX->Lock( dwNextLockFlags ); dwNextLockFlags = dwLockFlags; return pDX->pLocked; }
	void Unlock() { pDX->Unlock(); }
};

template<class TBuffer, class TDXBuffer, class TUserObject>
class CLinearBufferElement : 
	public NCache::CGatherElementBase<NCache::CShortPtrAllocator,NCache::CFibElement, TUserObject>,
	public ILinearBuffer
{
	OBJECT_NOCOPY_METHODS(CLinearBufferElement);
public:
	CPtr<TBuffer> pBuffer;
	int nStart, nBufSize;
	int nSize;
	int nLocked;

	CLinearBufferElement( TBuffer *_p = 0 ) : pBuffer(_p), nLocked(0) {}
	~CLinearBufferElement() { while ( nLocked-- ) pBuffer->Unlock(); }
	virtual int GetFormatID() const { return pBuffer->GetFormatID(); }
	virtual void SetSize( int _nSize ) { ASSERT( _nSize <= nBufSize ); nSize = _nSize; }
	virtual int GetSize() const { return nSize; }
	virtual int GetBufSize() const { return nBufSize; }
	virtual void* Lock()
	{
		++nLocked;
		return pBuffer->Lock() + pBuffer->GetStride() * nStart;
	}
	virtual void Unlock() { if ( nLocked == 0 ) return; --nLocked; pBuffer->Unlock(); }
};

class CUserGeometry;
typedef CLinearBuffer<CVB, CUserGeometry> CGeometryBuffer;
class CUserGeometry : 
	public CLinearBufferElement<CGeometryBuffer, CVB, CUserGeometry>,
	public CGeometry
{
	OBJECT_NOCOPY_METHODS(CUserGeometry);
	typedef CLinearBufferElement<CGeometryBuffer, CVB, CUserGeometry> TParent;
public:
	CUserGeometry( CGeometryBuffer *_p = 0 ) : TParent(_p) {}
	virtual void* GetVertexStream() { return pBuffer; }
	virtual int GetVBStart() const { return nStart; }
	virtual int GetVBSize() const { return nSize; }
	virtual void DoTouch()
	{
		ASSERT( IsValid( this ) );
		if ( Touch() )
			pBuffer->WasTouched( this );
	}
	virtual void SetVertexStream() 
	{
		ASSERT( IsValid( this ) );
		RealSetVertexStream( pBuffer->GetBuffer()->obj, pBuffer->GetStride() );
	}
	virtual int GetGeometryFormatID() { return GetFormatID(); }
};


class CTriListWrapper;
class CTriListWrapperHandle
{
	int nSlowCheck;
	std::vector<CMObj<CTriListWrapper> > wrappers;
public:
	template<class T>
	CTriListWrapper* NewWrapper( T *pThis, int nTris )
	{
		if ( ( (++nSlowCheck) & 0xff ) == 0 && !wrappers.empty() )
			EraseInvalidRefs( &wrappers );
		ASSERT( nTris <= pThis->GetSize() );
		CTriListWrapper *pRes = new CTriListWrapper( pThis, nTris ); 
		wrappers.push_back( pRes ); 
		return pRes;  
	}
};

class CTriListCore: public ILinearBuffer, public CTriList, public CTriListWrapperHandle
{
	OBJECT_NOCOPY_METHODS(CTriListCore);
	std::vector<S3DTriangle> data;
	int nSize;
public:
	CTriListCore( int _nSize = 0 ) : data(_nSize), nSize(_nSize) {}
	virtual int GetFormatID() const { return S3DTriangle::ID; }
	virtual void SetSize( int _nSize ) { ASSERT( _nSize <= data.size() ); nSize = _nSize; }
	virtual int GetSize() const { return nSize; }
	virtual int GetBufSize() const { return data.size(); }
	virtual void* Lock() { return &data[0]; }
	virtual void Unlock() {}

	virtual const std::vector<S3DTriangle>& GetTris() const { return data; }
	virtual int GetTrisNumber() const { return GetSize(); }
	virtual CTriListWrapper* CreateWrapper( int nTris ) { return NewWrapper( this, nTris ); }
	void DrawPrimitive( int nVBStart, int nMinIndex, int nMaxIndex ) { ASSERT( 0 ); }
};


/*class CTriListCore16;
typedef CLinearBuffer<CIB16, CTriListCore16> CIndicesBuffer;
class CTriListCore16: 
	public CLinearBufferElement<CIndicesBuffer, CIB16, CTriListCore16>,
	public CTriList, public CTriListWrapperHandle
{
	OBJECT_NOCOPY_METHODS(CTriListCore16);
	typedef CLinearBufferElement<CIndicesBuffer, CIB16, CTriListCore16> TParent;
public:
	CTriListCore16( CIndicesBuffer *_pBuf = 0 ) : TParent( _pBuf ) {}
	virtual const vector<S3DTriangle>& GetTris() const { ASSERT(0); return *(vector<S3DTriangle>*)0; }
	virtual int GetTrisNumber() const { ASSERT(0); return 0; }
	virtual CTriListWrapper* CreateWrapper( int nTris ) { return NewWrapper( this, nTris ); }
	void RealDrawPrimitive( int nVBStart, int nMinIndex, int nMaxIndex, int _nTris )
	{
		if ( bWasLinearBufferLock )
			FreeLinearBuffers();
		if ( Touch() )
			pBuffer->WasTouched( this );
		renderStats.nVertices += nMaxIndex - nMinIndex;
		renderStats.nTris += _nTris;
		pDevice->SetIndices( pBuffer->GetBuffer()->obj, nVBStart );
		HRESULT hRes = pDevice->DrawIndexedPrimitive( 
			D3DPT_TRIANGLELIST, 
			0,
			nMinIndex, nMaxIndex - nMinIndex + 1,
			nStart * 3,
			_nTris
			);
		ASSERT( hRes == D3D_OK );
	}
	virtual void DrawPrimitive( int nVBStart, int nMinIndex, int nMaxIndex ) { RealDrawPrimitive( nVBStart, nMinIndex, nMaxIndex, nSize ); }
};*/

class CTriListWrapper: public CTriList, public ISomeBuffer
{
	OBJECT_BASIC_METHODS( CTriListWrapper );

	CPtr<ISomeBuffer> pParent;
	int nTris;
public:
	CTriListWrapper() {}
	CTriListWrapper( ISomeBuffer *_p, int _nTris ): pParent(_p), nTris(_nTris) {}
	virtual const std::vector<S3DTriangle>& GetTris() const { return dynamic_cast<CTriList*>(pParent.GetPtr())->GetTris(); }
	virtual int GetTrisNumber() const { return nTris; }
	int GetFormatID() const { ASSERT( IsValid( pParent ) ); return pParent->GetFormatID(); }
	void SetSize( int _nSize ) { ASSERT( IsValid( pParent ) && _nSize <= pParent->GetSize() ); nTris = _nSize; }
	int GetSize() const { return nTris; }
	int GetBufSize() const { ASSERT( IsValid( pParent ) ); return pParent->GetBufSize(); }
	virtual CTriListWrapper* CreateWrapper( int nTris ) { return dynamic_cast<CTriListCore*>(pParent.GetPtr())->CreateWrapper( nTris ); }
	virtual void DrawPrimitive( int nVBStart, int nMinIndex, int nMaxIndex ) { ASSERT(0); }//dynamic_cast<CTriListCore16*>(pParent.GetPtr())->RealDrawPrimitive( nVBStart, nMinIndex, nMaxIndex, nTris ); }
};

// TEXTURES support classes

class CTB: public CObjectBase
{
	OBJECT_BASIC_METHODS(CTB);
public:
	NWin32Helper::com_ptr<IDirect3DTexture9> obj;
	bool bIsDynamic;
	//
	CTB() : nLOD(0) {}
	CTB( int _nXSize, int _nYSize, int _nLevels, D3DFORMAT _format, ETextureUsage usage )
		:format(_format), nXSize(_nXSize), nYSize(_nYSize), nLevels(_nLevels), nLOD(0)
	{
		uint32_t dwUsage = 0;
		if ( usage == TARGET )
			dwUsage = D3DUSAGE_RENDERTARGET;
		if ( usage == DYNAMIC_TEXTURE )
			dwUsage |= D3DUSAGE_DYNAMIC;
		D3DPOOL pool = usage == REGULAR ? D3DPOOL_MANAGED : D3DPOOL_DEFAULT;

		HRESULT hRes = pDevice->CreateTexture( 
			_nXSize, 
			_nYSize, 
			_nLevels, 
			dwUsage,
			_format, 
			pool,
			obj.GetAddr(),
			0 );

		bIsDynamic = usage == DYNAMIC_TEXTURE;
		if ( is_debugger_present() )
		{
			if ( hRes == D3DERR_OUTOFVIDEOMEMORY )
				ASSERT( 0 );
			ASSERT( D3D_OK == hRes );
		}
		D3DASSERT( hRes, "CreateTexture failed with X: %d Y: %d Levels: %d Usage: %d Format: %d Pool: %d", _nXSize, _nYSize, _nLevels, dwUsage, fmt::underlying( _format ), fmt::underlying( pool ) );
		bInitOk &= SUCCEEDED(hRes);
	}
	
	int GetSizeX() const { return nXSize; }
	int GetSizeY() const { return nYSize; }
	D3DFORMAT GetFormat() const { return format; }
	int GetNumMipLevels() const { return nLevels; }
	int GetRawSize() const
	{
		int nRes = 0;
		for ( int k = 0; k < nLevels; ++k )
			nRes += nXSize * nYSize * D3DFormat2PixelBitSize( format ) >> (k*2);
		return nRes >> 3;
	}
	int GetRawSizeNotSetMip( int nMip ) const
	{
		if ( nMip != nLOD )
			return ( nXSize * nYSize * D3DFormat2PixelBitSize( format ) >> ( (std::min)( nMip, nLevels - 1 ) * 2 ) ) >> 3;
		return 0;
	}
	void SetLOD( int _nLOD )
	{
		const int nMinLOD = (std::min)( _nLOD, nLevels - 1 );
		if ( nMinLOD != nLOD )
		{
			nLOD = nMinLOD;
			obj->SetLOD( nLOD );
		}
	}
private:
	int nXSize, nYSize, nLevels;
	D3DFORMAT format;
	int nLOD;
};

// system memory texture for update operations
class CSysTexture: public CObjectBase
{
	OBJECT_BASIC_METHODS(CSysTexture);
	bool bBusy;
public:
	NWin32Helper::com_ptr<IDirect3DSurface9> pSurface;
	NWin32Helper::com_ptr<IDirect3DTexture9> pTexture;
	//
	CSysTexture() { bBusy = false; }
	CSysTexture( D3DFORMAT _format )
	{
		bBusy = false;
		HRESULT hRes = pDevice->CreateTexture( 
			N_SYSMEM_TEXTURE_SIZE,
			N_SYSMEM_TEXTURE_SIZE,
			1,
			0, 
			_format, 
			D3DPOOL_SYSTEMMEM,
			pTexture.GetAddr(),
			0 );
		ASSERT( D3D_OK == hRes ); // if this fails no need to run further
		hRes = pTexture->GetSurfaceLevel( 0, pSurface.GetAddr() );
		ASSERT( D3D_OK == hRes );
	}
	void MarkBusy() { bBusy = true; }
	void Free() { bBusy = false; }
	bool IsBusy() const { return bBusy; }
};

// class for ring usage of system memory buffers for texture update operations
class CSurfaceRing: public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CSurfaceRing );
public:
	std::vector< CObj<CSysTexture> > textures;
	int nCurrent;
	//
	CSurfaceRing() { ASSERT( 0 ); }
	CSurfaceRing( D3DFORMAT _format, int nSize );
	void Switch() { nCurrent = ( nCurrent + 1 ) % textures.size(); }
	CSysTexture* GetTexture();
};

CSurfaceRing::CSurfaceRing( D3DFORMAT _format, int nSize )
{
	textures.resize(0);
	textures.reserve( nSize );
	for ( int i = 0; i < nSize; i++ )
		textures.push_back( new CSysTexture( _format ) );
	nCurrent = 0;
}

CSysTexture* CSurfaceRing::GetTexture()
{
	CSysTexture *pRes;
	for(;;)
	{
		pRes = textures[nCurrent];
		if ( !pRes->IsBusy() )
			break;
		Switch();
	}
	Switch();
	return pRes;
}

// class responsible for texture updates
class CTextureLocker: public I2DBufferLock
{
	NWin32Helper::com_ptr<IDirect3DSurface9> pObj;
	EAccess access;
	CPtr<CSysTexture> pLocker;
	D3DLOCKED_RECT buf;
	CTRect<int> rect;
	NWin32Helper::com_ptr<IDirect3DTexture9> pTexture;
public:
	// rect is specified for level 0
	CTextureLocker( IDirect3DSurface9 *_pObj, const CTRect<int> &_rect, EAccess _access, D3DFORMAT format, IDirect3DTexture9 *_pTexture );
	~CTextureLocker();
	virtual void* GetBuffer() { return buf.pBits; }
	virtual int GetStride() { return buf.Pitch; }
};

struct SD3DFormatHash
{
	int operator()( D3DFORMAT f ) const { return (int)f; }
};
typedef std::unordered_map<D3DFORMAT, CPtr<CSurfaceRing>, SD3DFormatHash > CFormatRingMap;
static CFormatRingMap sysTextures;

CTextureLocker::CTextureLocker( IDirect3DSurface9 *_pObj, const CTRect<int> &_rect, EAccess _access, D3DFORMAT format, IDirect3DTexture9 *_pTexture )
: pObj(_pObj), rect(_rect), access( _access ), pTexture(_pTexture)
{
	static int nTroubleBuffer[1024];
	HRESULT hr;
	NWin32Helper::com_ptr<IDirect3DSurface9> pTempBuf;
	//
	uint32_t dwLockFlags = 0;//D3DLOCK_NO_DIRTY_UPDATE;
	CTRect<int> lockRect;
	if ( access != INPLACE && access != INPLACE_READONLY )
	{
		lockRect.SetRect( 0, 0, rect.Width(), rect.Height() );
		CFormatRingMap::iterator i = sysTextures.find( format );
		ASSERT( i != sysTextures.end() );
		pLocker = i->second->GetTexture();
		pLocker->MarkBusy();
		ASSERT( rect.Width() <= N_SYSMEM_TEXTURE_SIZE );
		ASSERT( rect.Height() <= N_SYSMEM_TEXTURE_SIZE );
		//
		pTempBuf = pLocker->pSurface;
		if ( pDevice && pObj && access != WRITEONLY )
		{
			ASSERT(0);
			// GetRenderTargetData()
			//hr = pDevice->CopyRects( pObj, (RECT*)&rect, 1, pTempBuf, 0 );
			//ASSERT( D3D_OK == hr );
		}
	}
	else
	{
		if ( access == INPLACE_READONLY )
			dwLockFlags |= D3DLOCK_READONLY;
		if ( pTexture )
			dwLockFlags |= D3DLOCK_NO_DIRTY_UPDATE;
		pTempBuf = pObj;
		lockRect = rect;
	}
	if ( pTempBuf )
	{
		hr = pTempBuf->LockRect( &buf, (RECT*)&lockRect, dwLockFlags );
		ASSERT( D3D_OK == hr );
	}
	else
	{
		ASSERT(0); // texture to be locked is unavailable
		buf.pBits = nTroubleBuffer;
		buf.Pitch = 0;
	}
}

CTextureLocker::~CTextureLocker()
{
	NWin32Helper::com_ptr<IDirect3DSurface9> pTempBuf;
	if ( access == INPLACE || access == INPLACE_READONLY )
	{
		pTempBuf = pObj;
		if ( pTempBuf )
		{
			pTempBuf->UnlockRect();
			// fix for a strange bug (adding dirty rects during lockrect for pool_default textures break)
			if ( access == INPLACE && pTexture ) 
				 pTexture->AddDirtyRect( (RECT*)&rect );
		}
	}
	else
	{
		ASSERT( pLocker != 0 );
		pLocker->Free();
		if ( IsValid( pLocker ) )
		{
			pTempBuf = pLocker->pSurface;
			pTempBuf->UnlockRect();
			if ( pDevice && pObj && access != READONLY )
			{
				CTRect<int> copyRect;
				copyRect.SetRect( 0, 0, rect.Width(), rect.Height() );
				POINT dst;
				dst.x = rect.x1;
				dst.y = rect.y1;
				HRESULT hRes = pDevice->UpdateSurface( 
					pTempBuf, 
					(RECT*)&copyRect,
					pObj,
					(POINT*)&dst );
				ASSERT( hRes == D3D_OK );
			}
		}
	}
}

class CTexture: public I2DBuffer, 
	public NCache::CGatherElementBase<NCache::CShortPtrAllocator,NCache::CQuadTreeElement, CTexture>
{
	typedef NCache::CGatherElementBase<NCache::CShortPtrAllocator,NCache::CQuadTreeElement, CTexture> CBase;
	OBJECT_BASIC_METHODS(CTexture);
	int nFrameUsed;
public:
	CPtr<CTB> pTB;
	CTRect<int> region;
	EWrap wrap;
	bool bPointFiltered;
	//
	bool Touch() { bool bRes = CBase::Touch(); nFrameUsed = nCurrentFrame; return bRes; }
	CTexture() : bPointFiltered(false) {}
	CTexture( CTB *_pTB, EWrap _wrap ): pTB(_pTB), nFrameUsed(0), wrap(_wrap), bPointFiltered(false) { region.SetRect(0, 0, _pTB->GetSizeX(), _pTB->GetSizeY() ); }
	//CTexture( int _nXSize, int _nYSize, int nLevels, D3DFORMAT _format, uint32_t dwUsage );
	virtual int GetPixelID() { return D3DFormat2PixelID( pTB->GetFormat() ); }
	virtual I2DBufferLock* Lock( int nLevel, EAccess access );
	virtual int GetSizeX() const { return region.Width(); }///pTB->GetSizeX(); }
	virtual int GetSizeY() const { return region.Height(); }//pTB->GetSizeY(); }
	virtual int GetNumMipLevels() const { return pTB->GetNumMipLevels(); }
	virtual int GetFrameMRU() const { return nFrameUsed; }
	virtual void UserTouch() { Touch(); }
	void ReplaceTextureSurface( int nLevel, IDirect3DSurface9 *pSrc )
	{
		//NWin32Helper::com_ptr<IDirect3DSurface9> pSurface;
		IDirect3DSurface9 *pDst;
		pTB->obj->GetSurfaceLevel( nLevel, &pDst );
		//HRESULT hr = pDevice->UpdateSurface( pSrc, NULL, pSurface, NULL );
		HRESULT hr = pDevice->StretchRect( pSrc, NULL, pDst, NULL, D3DTEXF_NONE );
		ASSERT( hr == D3D_OK );
		pDst->Release();
	}
};

I2DBufferLock* CTexture::Lock( int nLevel, EAccess access ) 
{
	NWin32Helper::com_ptr<IDirect3DSurface9> pSurface;
	pTB->obj->GetSurfaceLevel( nLevel, pSurface.GetAddr() );
	CTRect<int> rect = region;
	rect.x1 >>= nLevel;
	rect.y1 >>= nLevel;
	rect.x2 >>= nLevel;
	rect.y2 >>= nLevel;
	//pTB->obj->AddDirtyRect( (RECT*)&region );
	return new CTextureLocker( pSurface, rect, access, pTB->GetFormat(), pTB->bIsDynamic ? 0 : pTB->obj );
}

void ReplaceTextureSurface( CTexture *pTex, int nLevel, IDirect3DSurface9 *pSurf )
{
	pTex->ReplaceTextureSurface( nLevel, pSurf );
}

void GetSurface( CTexture *pTexture, int nLevel, NWin32Helper::com_ptr<IDirect3DSurface9> *pRes )
{
	ASSERT( IsValid( pTexture ) );
	if ( !IsValid(pTexture) || !IsValid(pTexture->pTB) || !pTexture->pTB->obj )
	{
		*pRes = 0;
		return;
	}
	pTexture->Touch();
	pTexture->pTB->obj->GetSurfaceLevel( nLevel, pRes->GetAddr() );
}

EWrap GetWrap( CTexture *pTex )
{
	return pTex->wrap;
}

class CCubeTB : public CObjectBase
{
	OBJECT_NOCOPY_METHODS(CCubeTB);
	int nSize, nMipLevels;
	D3DFORMAT format;
public:
	NWin32Helper::com_ptr<IDirect3DCubeTexture9> obj;
	//
	CCubeTB() {}
	CCubeTB( int _nSize, int _nMipLevels, D3DFORMAT _format, ETextureUsage usage )
		:format(_format), nSize(_nSize), nMipLevels(_nMipLevels)
	{
		if ( bNoCubeMapMipLevels )
			nMipLevels = 1;

		HRESULT hRes = pDevice->CreateCubeTexture( 
			_nSize,
			nMipLevels,
			usage == REGULAR ? 0 : D3DUSAGE_RENDERTARGET,
			_format, 
			usage == REGULAR ? D3DPOOL_MANAGED : D3DPOOL_DEFAULT,
			obj.GetAddr(),
			0 );
		if ( is_debugger_present() )
		{
			if ( hRes == D3DERR_OUTOFVIDEOMEMORY )
				ASSERT( 0 );
			ASSERT( D3D_OK == hRes );
		}
		bInitOk &= SUCCEEDED(hRes);
	}
	D3DFORMAT GetFormat() const { return format; }
	int GetSize() const { return nSize; }
	int GetNumMipLevels() const { return nMipLevels; }
};

class CCubeTexture : public ICubeBuffer
{
	OBJECT_NOCOPY_METHODS(CCubeTexture);
	CPtr<CCubeTB> pTB;
	int nFrameUsed;
public:
	CCubeTexture( CCubeTB *_p = 0 ) : pTB(_p), nFrameUsed(0) {}
	CCubeTB* GetTB() const { return pTB; }
	virtual int GetPixelID() { return D3DFormat2PixelID( pTB->GetFormat() ); }
	virtual I2DBufferLock* Lock( EFace face, int nLevel, EAccess access );
	virtual int GetSize() const { return pTB->GetSize(); }
	virtual int GetNumMipLevels() const { return pTB->GetNumMipLevels(); }
	void Touch() { nFrameUsed = nCurrentFrame; }
	int GetFrameMRU() const { return nFrameUsed; }
};

inline D3DCUBEMAP_FACES GetD3DFace( EFace face )
{
	switch ( face )
	{
	case POSITIVE_X: return D3DCUBEMAP_FACE_POSITIVE_X;
	case POSITIVE_Y: return D3DCUBEMAP_FACE_POSITIVE_Y;
	case POSITIVE_Z: return D3DCUBEMAP_FACE_POSITIVE_Z;
	case NEGATIVE_X: return D3DCUBEMAP_FACE_NEGATIVE_X;
	case NEGATIVE_Y: return D3DCUBEMAP_FACE_NEGATIVE_Y;
	case NEGATIVE_Z: return D3DCUBEMAP_FACE_NEGATIVE_Z;
	}
	ASSERT( 0 );
	return D3DCUBEMAP_FACE_POSITIVE_X;
}

I2DBufferLock* CCubeTexture::Lock( EFace face, int nLevel, EAccess access ) 
{
	NWin32Helper::com_ptr<IDirect3DSurface9> pSurface;
	pTB->obj->GetCubeMapSurface( GetD3DFace( face ), nLevel, pSurface.GetAddr() );
	int nSize = pTB->GetSize();
	CTRect<int> rect( 0, 0, nSize >> nLevel, nSize >> nLevel );
	return new CTextureLocker( pSurface, rect, access, pTB->GetFormat(), 0 ); 
}

void GetSurface( CCubeTexture *pTexture, EFace face, int nLevel, NWin32Helper::com_ptr<IDirect3DSurface9> *pRes )
{
	ASSERT( IsValid( pTexture ) );
	pTexture->Touch();
	pTexture->GetTB()->obj->GetCubeMapSurface( GetD3DFace( face ), nLevel, pRes->GetAddr() );
}


class CTextureCache
{
	typedef NCache::CGatheringCache<NCache::CShortPtrAllocator, NCache::CQuadTreeElement, CTexture> CCache;
	CObj<CCache> pCache;
	CPtr<CTB> pBuffer;
	CMObj<CTexture> pTexture;
	CObj<CTexture> pTexObj;

public:
	void Init( CTB *_pBuffer )
	{
		pBuffer = _pBuffer;
		if ( pBuffer )
			pTexture = new CTexture( pBuffer, CLAMP );
		else
			pTexture = 0;
		pTexObj = pTexture;
		pCache = 0;
		if ( !pBuffer )
			return;
		pCache = new CCache( nCurrentFrame );
		NCache::CQuadTreeElement root;
		root.nXSize = GetMSB( pBuffer->GetSizeX() - 1 ) + 1;
		root.nYSize = GetMSB( pBuffer->GetSizeY() - 1 ) + 1;
		root.nShiftX = 0;
		root.nShiftY = 0;
		pCache->AddRoot( root );
	}
	void NextFrame( bool bOnThrashing )
	{
		if ( IsValid( pCache ) )
			pCache->AdvanceFrameCounter();
	}
	CTB* GetTB() const { return pBuffer; }
	CTexture* GetTexture() const { return pTexture; }
	bool IsThrashing() const { return IsValid( pCache ) && pCache->IsThrashing(); }
	CTexture* Alloc( int nXSize, int nYSize )
	{
		if ( !IsValid(pCache) )
			return 0;
		NCache::CQuadTreeElement elem;
		elem.nXSize = GetMSB( nXSize - 1 ) + 1;
		elem.nYSize = GetMSB( nYSize - 1 ) + 1;
		CCache::SCachePlace place;
		if ( !pCache->GetPlace( elem, &place ) )
			return 0;
		CTexture *pRes = new CTexture( pBuffer, CLAMP );
		pCache->PerformAlloc( pRes, &place );
		pRes->region.SetRect( 
			place.resPlace.nShiftX, place.resPlace.nShiftY,
			place.resPlace.nShiftX + nXSize, place.resPlace.nShiftY + nYSize
			);
		//pRes->Touch();
		return pRes;
	}
	void DrawRU()
	{
		CTextureLock<SPixel8888> tl( pTexture, 0, INPLACE );
		std::vector<CCache::SStatePlace> places;
		pCache->GetState( &places );
		for ( int y = 0; y < tl.GetSizeY(); ++y )
			for ( int x = 0; x < tl.GetSizeX(); ++x )
				tl[y][x] = SPixel8888( 255,255,255 );
		for ( int k = 0; k < places.size(); ++k )
		{
			const CCache::SStatePlace &p = places[k];
			SPixel8888 color;
			if ( p.pUser )
			{
				switch ( pCache->GetCurrentRU() - p.nMRU )
				{
				case 0: color = SPixel8888( 0,0,255 ); break;
				case 1: color = SPixel8888( 0,0,200 ); break;
				case 2: color = SPixel8888( 0,0,100 ); break;
				default: color = SPixel8888( 0,0,50 ); break;
				}
			}
			else
			{
				ASSERT( p.nMRU == 0 );
				color = SPixel8888( 255,0,0 );
			}
			const NCache::CQuadTreeElement &te = p.place;
			if ( ( (te.nShiftX >> te.nXSize ) + (te.nShiftY >> te.nYSize ) ) & 1 )
				color.g = 30;
			for ( int y = te.nShiftY; y < te.nShiftY + (1<<te.nYSize); ++y )
			{
				for ( int x = te.nShiftX; x < te.nShiftX + (1<<te.nXSize); ++x )
					tl[y][x] = color;
			}
		}
	}
};

template<class TBuf, class THandle>
class CLRUBuffersSet
{
	struct STex
	{
		CPtr<TBuf> pTB;
		CMObj<THandle> pTexture;

		STex( TBuf *_pTB ): pTB(_pTB) {}
	};
	std::vector<STex> textures;
protected:
	void AddBuffer( TBuf *_pTB ) { textures.push_back( STex( _pTB ) ); }
	virtual THandle* CreateHandle( TBuf *p ) = 0;
public:
	void Clear() { textures.clear(); }
	void Walk()
	{
		for ( std::vector<STex>::iterator i = textures.begin(); i != textures.end(); ++i )
		{
			ASSERT( IsValid( i->pTB ) );
			if ( !IsValid( i->pTexture ) )
				i->pTexture = 0;
		}
	}
	THandle* Alloc()
	{
		// pick best
		NCache::MRU_TYPE nBest = nCurrentFrame - 1; //MRU_LAST;
		STex *pBest = 0;
		for ( std::vector<STex>::iterator i = textures.begin(); i != textures.end(); ++i )
		{
			if ( !IsValid( i->pTexture ) )
			{
				pBest = &(*i);
				break;
			}
			NCache::MRU_TYPE nTest = i->pTexture->GetFrameMRU();
			if ( nTest < nBest )
			{
				pBest = &(*i);
				nBest = nTest;
			}
		}
		if ( pBest == 0 )
			return 0;
		THandle *pRes = CreateHandle( pBest->pTB );
		pBest->pTexture = pRes;
		pRes->Touch();
		return pRes;
	}
};

class CTextureBuffersSet : public CLRUBuffersSet<CTB, CTexture>
{
public:
	void Init( const SRenderTargetDesc &desc, int nNumber )
	{
		int nSize = desc.nResolution;
		D3DFORMAT fmt = PixelID2D3DFormat( desc.nFormatID );
		for ( int i = 0; i < nNumber; ++i )
			AddBuffer( new CTB( nSize, nSize, 1, fmt, TARGET ) );
	}
	virtual CTexture* CreateHandle( CTB *pTB ) { return new CTexture( pTB, CLAMP ); }
};

class CCubemapBufferSet : public CLRUBuffersSet<CCubeTB, CCubeTexture>
{
public:
	void Init( const SRenderTargetDesc &desc, int nNumber )
	{
		int nSize = desc.nResolution;
		D3DFORMAT fmt = PixelID2D3DFormat( desc.nFormatID );
		for ( int i = 0; i < nNumber; ++i )
			AddBuffer( new CCubeTB( nSize, 1, fmt, TARGET ) );
	}
	virtual CCubeTexture* CreateHandle( CCubeTB *pTB ) { return new CCubeTexture( pTB ); }
};

static BOOST_FORCEINLINE void ReallyFastShiftingTransfer( const unsigned short *pSrc, int *pDst, int nSize, int nShift )
{
	std::transform(pSrc, pSrc + nSize, pDst, [nShift](uint16_t value) {
		return value + nShift;
	});
}

struct S32Triangle
{
	int n1, n2, n3;
};
const int N_TRIS_BUFFER_SIZE = 65536; // in bytes
template<class T, class TIB>
class CDynamicTrisBase
{
protected:
	enum EPrim
	{
		TRILIST,
		LINESTRIP
	};
	CObj<TIB> pDynamicTrisBuffer;
	int nLast, nBuf;
	int nStart; // start of queued triangles and number of them
public:
	void Clear() { pDynamicTrisBuffer = 0; nLast = 0; nBuf = 0;}
	CDynamicTrisBase() { Clear(); }
	virtual void Init()
	{
		int nBufSize = N_TRIS_BUFFER_SIZE;
		int n = nBufSize / T::N_TRIANGLE_SIZE;
		n = n & (~1);
		nLast = n;
		nBuf = n;
		pDynamicTrisBuffer = new TIB( nBufSize, TBU_DYNAMIC );
		nStart = n;
	}
	TIB* GetBuffer() { return pDynamicTrisBuffer; }
};

class CDynamicTrisIndices32 : public CDynamicTrisBase<CDynamicTrisIndices32, CIB32Fast>
{
	typedef CDynamicTrisBase<CDynamicTrisIndices32, CIB32Fast> TBase;
	int nMaxVBIndex; // fake to keep DX happy
	EPrim currentPrim;
	
public:
	enum EE
	{
		N_TRIANGLE_SIZE = 12,
		N_MIN_BATCH_SIZE = 500
	};
	virtual void Init()
	{
		TBase::Init();
		nMaxVBIndex = 0;
		currentPrim = TRILIST;
	}
	virtual int GetTriangleSize() const { return 12; }
	void AddPrimitiveGeometry( const S3DTriangle *pSrcTris, int nTris, int nVBStart, int nVBSize, EPrim _prim = TRILIST )
	{
		if ( _prim != currentPrim )
		{
			FlushPrimitive();
			currentPrim = _prim;
		}
		int nSrcStart = 0;
		nMaxVBIndex = (std::max)( nMaxVBIndex, nVBSize + nVBStart );
		while ( nTris > 0 )
		{
			// feed into current buffer until full
			//{
			int nToDraw = (std::min)( nBuf - nLast, nTris );
			uint32_t dwFlags = D3DLOCK_NOOVERWRITE;
			if ( nToDraw == 0 )
			{
				FlushPrimitive();
				nMaxVBIndex = nVBSize + nVBStart;
				nLast = 0;
				nStart = 0;
				dwFlags = D3DLOCK_DISCARD;
				nToDraw = (std::min)( nBuf - nLast, nTris );
			}

			if ( !pDynamicTrisBuffer->pLocked )
			{
				pDynamicTrisBuffer->Lock( dwFlags );
			}

			S32Triangle *pTri = (S32Triangle*)pDynamicTrisBuffer->pLocked;
			pTri += nLast;
			ReallyFastShiftingTransfer( (const unsigned short*)&pSrcTris[ nSrcStart ], (int*)pTri, 
				nToDraw* 3, 
				nVBStart );
			
			nLast += nToDraw;
			nSrcStart += nToDraw;
			nTris -= nToDraw;
		}
	}
	void AddLinestrip( int nVBStart, int nVBSize, const unsigned short *pIndices, int nLines )
	{
		ASSERT( (nLines % 3) == 0 );
		AddPrimitiveGeometry( (const S3DTriangle*)pIndices, nLines / 3 * 2, nVBStart, nVBSize, LINESTRIP );
	}
	void FlushPrimitive()
	{
		if ( pDynamicTrisBuffer->pLocked )
			pDynamicTrisBuffer->Unlock();
		
		if ( bWasLinearBufferLock )
			FreeLinearBuffers();
		if ( nLast - nStart > 0 )
		{
			HRESULT hr;
			switch ( currentPrim )
			{
			case TRILIST:
				hr = pDevice->DrawIndexedPrimitive( 
					D3DPT_TRIANGLELIST, 
					0,
					0, nMaxVBIndex,
					nStart * 3,
					nLast - nStart
					);

				ASSERT( hr == D3D_OK );
				renderStats.nVertices += (nLast - nStart) * 3;//nVBSize;
				renderStats.nTris += nLast - nStart;
				++renderStats.nDIPs;
				break;
			case LINESTRIP:
				hr = pDevice->DrawIndexedPrimitive( 
					D3DPT_LINELIST, 
					0,
					0, nMaxVBIndex,
					nStart * 3,
					( nLast - nStart ) * 3 / 2
					);
				ASSERT( hr == D3D_OK );
				break;
			default:
				ASSERT(0);
				break;
			}
		}
		nLast = ( nLast + 1 ) & ~1;
		if ( nBuf - nLast < N_MIN_BATCH_SIZE )
			nLast = nBuf;
		nMaxVBIndex = 0;
		nStart = nLast;
	}
	void NextFrame()
	{
		if ( bBan32BitIndices )
			return;
		FlushPrimitive();
		nLast = nBuf;
		nStart = nLast;
	}
};

class CDynamicTrisIndices16 : public CDynamicTrisBase<CDynamicTrisIndices16, CIB16Fast>
{
	void FlushPrimitive( int nVBStart, int nVBSize, int nMin, int nMax, EPrim _prim )
	{
		ASSERT( nMax > nMin && nMax < nVBSize );
		if ( bWasLinearBufferLock )
			FreeLinearBuffers();
		if ( nLast - nStart > 0 )
		{
			HRESULT hr;
			hr = pDevice->SetIndices( GetBuffer()->obj );
			ASSERT( D3D_OK == hr );
			switch ( _prim )
			{
			case TRILIST:
				hr = pDevice->DrawIndexedPrimitive( 
					D3DPT_TRIANGLELIST, 
					nVBStart,
					nMin, nMax - nMin + 1,
					nStart * 3,
					nLast - nStart
					);
				ASSERT( hr == D3D_OK );
				renderStats.nVertices += nMax - nMin + 1;//nVBSize;
				renderStats.nTris += nLast - nStart;
				++renderStats.nDIPs;
				break;
			case LINESTRIP:
				hr = pDevice->DrawIndexedPrimitive( 
					D3DPT_LINELIST, 
					nVBStart,
					nMin, nMax - nMin + 1,
					nStart * 3,
					( nLast - nStart ) * 3 / 2
					);
				ASSERT( hr == D3D_OK );
				renderStats.nVertices += nMax - nMin + 1;//nVBSize;
				renderStats.nTris += ( nLast - nStart ) * 3 / 2;
				++renderStats.nDIPs;
				break;
			default:
				ASSERT(0);
				break;
			}
		}
		nLast = ( nLast + 1 ) & ~1;
		if ( nBuf - nLast < N_MIN_BATCH_SIZE )
			nLast = nBuf;
		nStart = nLast;
	}
public:
	enum EE
	{
		N_TRIANGLE_SIZE = 6,
		N_MIN_BATCH_SIZE = 2048
	};
	void AddPrimitiveGeometry( const S3DTriangle *pSrcTris, int nTris, int nVBStart, int nVBSize, EPrim _prim = TRILIST )
	{
		int nSrcStart = 0;//, nSrcInfo = 0;
		while ( nTris > 0 )
		{
			// feed into current buffer until full
			int nToDraw = (std::min)( nBuf - nLast, nTris );
			uint32_t dwFlags = D3DLOCK_NOOVERWRITE;
			if ( nToDraw == 0 )
			{
				nLast = 0;
				nStart = 0;
				dwFlags = D3DLOCK_DISCARD;
				nToDraw = (std::min)( nBuf - nLast, nTris );
			}

			pDynamicTrisBuffer->Lock( dwFlags );
			S3DTriangle *pTri = (S3DTriangle*)pDynamicTrisBuffer->pLocked;
			pTri += nLast;
			// fill tris from source
			unsigned short nMin = 0xffff, nMax = 0;
			for ( int k = 0; k < nToDraw; ++k )
			{
				S3DTriangle &res = *pTri++;
				const S3DTriangle &src = pSrcTris[ nSrcStart + k ];
				res.i1 = src.i1;
				res.i2 = src.i2;
				res.i3 = src.i3;
				nMin = (std::min) ( nMin, src.i1 );
				nMin = (std::min) ( nMin, src.i2 );
				nMin = (std::min) ( nMin, src.i3 );
				nMax = (std::max) ( nMax, src.i1 );
				nMax = (std::max) ( nMax, src.i2 );
				nMax = (std::max) ( nMax, src.i3 );
			}
			pDynamicTrisBuffer->Unlock();

			nLast += nToDraw;
			nSrcStart += nToDraw;
			nTris -= nToDraw;
			FlushPrimitive( nVBStart, nVBSize, nMin, nMax, _prim );
		}
	}
	void AddLinestrip( int nVBStart, int nVBSize, const unsigned short *pIndices, int nLines )
	{
		ASSERT( (nLines % 3) == 0 );
		AddPrimitiveGeometry( (const S3DTriangle*)pIndices, nLines / 3 * 2, nVBStart, nVBSize, LINESTRIP );
	}
	void NextFrame()
	{
		nLast = nBuf;
		nStart = nLast;
	}
};

struct SGeometryType
{
	EBufferUsage usage;
	int nID;
	SGeometryType( EBufferUsage _usage, int _nID ) : usage(_usage), nID(_nID) {}
	bool operator==( const SGeometryType &a ) const { return usage == a.usage && nID == a.nID; }
};
struct SGeometryTypeHash
{
	int operator()( const SGeometryType &s ) const { return s.usage ^ s.nID; }
};



// all DX buffers
static std::vector< CMObj<CObjectBase> > lostable;
static std::vector< CMObj<CObjectBase> > managed;
typedef std::unordered_map<CPtr<CTB>, CObj<CTexture>> CTexContainerHash;
static CTexContainerHash texContainers;
typedef std::unordered_map<SRenderTargetDesc, CTextureBuffersSet, SRTDescHash> CRTCache;
static CRTCache rtCache;
typedef std::unordered_map<SRenderTargetDesc, CCubemapBufferSet, SRTDescHash> CCMCache;
static CCMCache cmCache;
static CTextureCache textureCache, transparentCache;
typedef std::unordered_map<SGeometryType, CObj<CGeometryBuffer>,SGeometryTypeHash > CGeometryCacheHash;
static CGeometryCacheHash geometries;
static CDynamicTrisIndices32 dynamicTris32;
static CDynamicTrisIndices16 dynamicTris16;
static NWin32Helper::com_ptr<IDirect3DVertexBuffer9> pCurrentVB;
static CObj<CTexture> pLinearBufferMRU;
int nCurrentFrame = NCache::N_START_RU;

static void OnThrashing()
{
	dynamicTris32.FlushPrimitive();
	for ( int k = 0; k < N_MAX_PRESENTS_IN_QUEUE + 1; ++k )
		NextFrameBuffes( true );
}

template <class T>
T* AddLostable( T *p )
{
	lostable.push_back( p );
	return p;
}

template <class T>
T* AddManaged( T *p )
{
	managed.push_back( p );
	return p;
}

void DestroyLostableBuffers()
{
	FreeLinearBuffers();
	pLinearBufferMRU = 0;
	lostable.clear();
	lockableList.clear();
	texContainers.clear();
	dynamicTris16.Clear();
	dynamicTris32.Clear();
	geometries.clear();
	pCurrentVB = 0;
	sysTextures.clear();
	rtCache.clear();
	cmCache.clear();
	pDevice->SetIndices( 0 );
	pDevice->SetStreamSource( 0, 0, 0, 4 );
	for ( int i = 0; i < 8; i++ )
		pDevice->SetTexture( i, 0 );
	transparentCache.Init( 0 );
	textureCache.Init( 0 );
}

void DestroyManagedBuffers()
{
	managed.clear();
}

static bool bFillTransp = false, bFill2d = false;
void NextFrameBuffes( bool bOnThrashing )
{
	// render caches RU
	if ( bFill2d )
		textureCache.DrawRU();
	if ( bFillTransp )
		transparentCache.DrawRU();

	++nCurrentFrame;
	EraseInvalidRefs( &lostable );
	EraseInvalidRefs( &managed );
	for ( CRTCache::iterator i = rtCache.begin(); i != rtCache.end(); ++i )
		i->second.Walk();
	for ( CCMCache::iterator i = cmCache.begin(); i != cmCache.end(); ++i )
		i->second.Walk();
	bool bNeedFlush = false;
	for ( CGeometryCacheHash::iterator i = geometries.begin(); i != geometries.end(); ++i )
		i->second->NextFrame( bOnThrashing, &bNeedFlush );
	textureCache.NextFrame( bOnThrashing );
	transparentCache.NextFrame( bOnThrashing );
	dynamicTris16.NextFrame();
	dynamicTris32.NextFrame();
	if ( bNeedFlush )
		geometries.begin()->second->Flush();
}

CTexture* GetTextureContainer( CTexture *pTex, STexturePlaceInfo *pPlace )
{
	if ( !IsValid( pTex ) )
		return 0;
	pTex->Touch();
	pPlace->place = pTex->region;
	pPlace->size.x = pTex->pTB->GetSizeX();
	pPlace->size.y = pTex->pTB->GetSizeY();
	if ( pPlace->IsWhole() )
		return pTex;
	CTexContainerHash::iterator i = texContainers.find( pTex->pTB );
	if ( i != texContainers.end() && IsValid( i->second ) )
		return i->second;
	CTexture *pRes = new CTexture( pTex->pTB, CLAMP );
	texContainers[pTex->pTB] = pRes;
	return pRes;
}

template<class T>
void AddGeometryCache( int nSize, EBufferUsage usage, ETrueBufferUsage trueUsage, T *p = 0 )
{
	geometries[SGeometryType(usage,T::ID)] = new CGeometryBuffer( nSize, sizeof(T), T::ID, trueUsage );
}
void InitBuffers()
{
	for ( SRenderTargetsInfo::CRTHash::iterator i = rtInfo.targets.begin(); i != rtInfo.targets.end(); ++i )
		rtCache[ i->first ].Init( i->first, i->second );
	for ( SRenderTargetsInfo::CRTHash::iterator i = rtInfo.cubeTargets.begin(); i != rtInfo.cubeTargets.end(); ++i )
		cmCache[ i->first ].Init( i->first, i->second );
	// buffers for dynamic textures
	sysTextures[D3DFMT_A8R8G8B8] = new CSurfaceRing( D3DFMT_A8R8G8B8, N_SYSMEM_TEXTURES );
	//sysTextures[D3DFMT_R5G6B5] = new CSurfaceRing( D3DFMT_R5G6B5, N_SYSMEM_TEXTURES );
	//trilists.Init( 220000, sizeof(S3DTriangle), S3DTriangle::ID, STATIC );
	int nGeomBufSize = 256 * 1024 * 3;
	if ( !bTnLDevice )
	{
		if ( bHardwareVP )
		{
			// придержим память под более полезные вещи типа текстур
			NWin32Helper::com_ptr<IDirect3DVertexBuffer9> pHold;
			for ( int nAmount = 16 * 1024 * 1024; nAmount > 4 * 1024 * 1024; nAmount -= 2 * 1024 * 1024 )
			{
				HRESULT hr;
				hr = pDevice->CreateVertexBuffer( nAmount, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, pHold.GetAddr(), 0 );
				if ( SUCCEEDED(hr) )
					break;
			}
			AddGeometryCache<SGeomVecFull>( nGeomBufSize, DYNAMIC, TBU_DYNAMIC );
			pHold = 0;
		}
		else
			AddGeometryCache<SGeomVecFull>( nGeomBufSize, DYNAMIC, TBU_DYNAMIC );
	}
	else
	{
		if ( bHardwareVP )
		{
			// придержим память под более полезные вещи типа текстур
			NWin32Helper::com_ptr<IDirect3DVertexBuffer9> pHold;
			for ( int nAmount = 16 * 1024 * 1024; nAmount > 4 * 1024 * 1024; nAmount -= 2 * 1024 * 1024 )
			{
				HRESULT hr;
				hr = pDevice->CreateVertexBuffer( nAmount, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, pHold.GetAddr(), 0 );
				if ( SUCCEEDED(hr) )
					break;
			}
			AddGeometryCache<SGeomVecT2C1>( nGeomBufSize, DYNAMIC, TBU_DYNAMIC );
			pHold = 0;
		}
		else
			AddGeometryCache<SGeomVecT2C1>( nGeomBufSize, DYNAMIC, TBU_DYNAMIC );
	}
	if ( bBan32BitIndices )
		dynamicTris16.Init();
	else
	{
		dynamicTris32.Init();
		pDevice->SetIndices( dynamicTris32.GetBuffer()->obj );
	}
	if ( b16BitTextures )
	{
		textureCache.Init( new CTB( 1024, 1024, 1, PixelID2D3DFormat(SPixel4444::ID), REGULAR ) );
		transparentCache.Init( new CTB( 1024, 1024, 4, PixelID2D3DFormat(SPixel4444::ID), REGULAR ) );
	}
	else
	{
		textureCache.Init( new CTB( 1024, 1024, 1, PixelID2D3DFormat(SPixel8888::ID), REGULAR ) );
		transparentCache.Init( new CTB( 1024, 1024, 4, PixelID2D3DFormat(SPixel8888::ID), REGULAR ) );
	}
}

static ILinearBuffer* MakeGeometry( int nFormatID, int nSize, EBufferUsage usage )
{
	//if ( !bHardwareVP )
	//	usage = STATIC;
	ASSERT( !bTnLDevice || nFormatID != SGeomVecFull::ID );
	ASSERT( bTnLDevice || nFormatID == SGeomVecFull::ID );
	CGeometryCacheHash::iterator i = geometries.find( SGeometryType(usage, nFormatID) );
	ASSERT( i != geometries.end() );
	return i->second->Alloc( nSize );
}

static ILinearBuffer* MakeTriList( int nSize )// int nSize, EBufferUsage eUsage )
{
	CTriListCore *pRes = new CTriListCore( nSize );
	return pRes;
}

ILinearBuffer* CreateBuffer( int nFormatID, int nSize, EBufferUsage usage )
{
	if ( nFormatID == S3DTriangle::ID )
		return MakeTriList( nSize );
	return MakeGeometry( nFormatID, nSize, usage );
}

CTriList* MakeWrapper( CTriList *pSrc, int nTris )
{
	if ( IsValid( pSrc ) )
		return pSrc->CreateWrapper( nTris );
	return 0;
}

CTexture* GetTextureCache()
{
	return textureCache.GetTexture();
}

CTexture* GetTransparentTextureCache()
{
	return transparentCache.GetTexture();
}

bool HasSameContainer( CTexture *p1, CTexture *p2 )
{
	return p1->pTB == p2->pTB;
}

CTexture* MakeTexture( int nXSize, int nYSize, int nMipLevels, int nPixelID, ETextureUsage eUsage, EWrap wrap )
{
	if ( eUsage == DYNAMIC_TEXTURE )
	{
		if ( devCaps.Caps2 & D3DCAPS2_DYNAMICTEXTURES )
		{
			ASSERT( GetNextPow2( nXSize ) == nXSize && GetNextPow2( nYSize ) == nYSize );
			CTB *pTB = new CTB( nXSize, nYSize, nMipLevels, PixelID2D3DFormat( nPixelID ), eUsage );
			return AddLostable( new CTexture( pTB, wrap ) );
		}
		eUsage = REGULAR;
	}
	if ( eUsage == REGULAR  )
	{
		ASSERT( GetNextPow2( nXSize ) == nXSize && GetNextPow2( nYSize ) == nYSize );
		CTB *pTB = new CTB( nXSize, nYSize, nMipLevels, PixelID2D3DFormat( nPixelID ), eUsage );
		return AddManaged( new CTexture( pTB, wrap ) );
	}
	if ( eUsage == TEXTURE_2D )
	{
		InformNew2DTextureAlloc();
		D3DFORMAT fmt = PixelID2D3DFormat( nPixelID );
		ASSERT( fmt == ( b16BitTextures ? D3DFMT_A4R4G4B4 : D3DFMT_A8R8G8B8 ) );
		return textureCache.Alloc( nXSize, nYSize );
	}
	if ( eUsage == TRANSPARENT_TEXTURE )
	{
		D3DFORMAT fmt = PixelID2D3DFormat( nPixelID );
		ASSERT( fmt == ( b16BitTextures ? D3DFMT_A4R4G4B4 : D3DFMT_A8R8G8B8 ) );
		return transparentCache.Alloc( nXSize, nYSize );
	}
	SRenderTargetDesc rtDesc( nXSize, nPixelID );
	ASSERT( wrap == CLAMP );
	ASSERT( eUsage == TARGET );
	ASSERT( nXSize == nYSize );
	ASSERT( nMipLevels == 1 );
	ASSERT( rtCache.find( rtDesc ) != rtCache.end() );
	CTexture *pRes = rtCache[rtDesc].Alloc();
	return pRes;
}

CTexture* MakeRenderTarget( int nXSize, int nYSize, int nPixelID )
{
	CTB *pTB = new CTB( nXSize, nYSize, 1, PixelID2D3DFormat( nPixelID ), TARGET );
	CTexture *pRes = new CTexture( pTB, CLAMP );
	return pRes;// pRes->Touch(); AddLostable( pRes );
}

CCubeTexture* MakeCubeTexture( int nSize, int nMipLevels, int nPixelID, ETextureUsage eUsage )
{
	if ( eUsage == REGULAR  )
	{
		ASSERT( GetNextPow2(nSize) == nSize );
		CCubeTB *pRes = new CCubeTB( nSize, nMipLevels, PixelID2D3DFormat( nPixelID ), eUsage );
		return AddManaged( new CCubeTexture( pRes ) );
	}
	SRenderTargetDesc rtDesc( nSize, nPixelID );
	ASSERT( eUsage == TARGET );
	ASSERT( nMipLevels == 1 );
	ASSERT( cmCache.find( rtDesc ) != cmCache.end() );
	CCubeTexture *pRes = cmCache[ rtDesc ].Alloc();
	return pRes;
}

static void RealSetVertexStream( IDirect3DVertexBuffer9 *pBuf, int nStride )
{
	if ( pCurrentVB != pBuf )
	{
		pCurrentVB = pBuf;
		pDevice->SetStreamSource( 0, pBuf, 0, nStride );
	}
}

static CObj<CTexture> pSmallTexture;
static bool bDrawMip;
struct STextureSize
{
	int nXSize, nYSize, nMips;
	STextureSize( int _nXSize, int _nYSize, int _nMips ) : nXSize(_nXSize), nYSize(_nYSize), nMips(_nMips) {}
	bool operator==( const STextureSize &a ) const { return nXSize == a.nXSize && nYSize == a.nYSize && nMips == a.nMips; }
};
struct STextureSizeHash
{
	int operator()( const STextureSize &a ) const { return ( a.nXSize ^ ( a.nYSize << 9 ) ) * a.nMips; }
};
typedef std::unordered_map<STextureSize, CObj<CTexture>, STextureSizeHash> CTextureMipHash;
static CTextureMipHash mipDrawTextures;
static CTexture *MakeMipDrawTexture( CTexture *pTex )
{
	STextureSize key( pTex->GetSizeX(), pTex->GetSizeY(), pTex->GetNumMipLevels() );
	CTextureMipHash::iterator i = mipDrawTextures.find( key );
	if ( i != mipDrawTextures.end() && IsValid( i->second ) )
		return i->second;
	CTexture *pRes = MakeTexture( key.nXSize, key.nYSize, key.nMips, NGfx::SPixel8888::ID, NGfx::REGULAR, NGfx::CLAMP );
	for ( int k = 0; k < pRes->GetNumMipLevels(); ++k )
	{
		NGfx::SPixel8888 color(0,0,0,0xff);
		switch ( k )
		{
		case 0: color = NGfx::SPixel8888(   0,   0, 255, 255); break;
		case 1: color = NGfx::SPixel8888(   0, 255,   0, 255); break;
		case 2: color = NGfx::SPixel8888( 255,   0,   0, 255); break;
		case 3: color = NGfx::SPixel8888( 255, 255,   0, 255); break;
		case 4: color = NGfx::SPixel8888( 255,   0, 255, 255); break;
		case 5: color = NGfx::SPixel8888(   0, 255, 255, 255); break;
		case 6: color = NGfx::SPixel8888(   0,   0,   0, 255); break;
		case 7: color = NGfx::SPixel8888(   0,   0,   0, 255); break;
		case 8: color = NGfx::SPixel8888(   0,   0,   0, 255); break;
		}
		CTextureLock<NGfx::SPixel8888> tex( pRes, k, INPLACE );
		for ( int y = 0; y < tex.GetSizeY(); ++y )
		{
			for ( int x = 0; x < tex.GetSizeX(); ++x )
				tex[y][x] = color;
		}
	}
	mipDrawTextures[key] = pRes;
	return pRes;
}
void SetTexture( int nStage, CTexture *pTex )
{
	if ( !bNoTexture )
	{
		if ( IsValid( pTex ) )
		{
			if ( bDrawMip )
				pTex = MakeMipDrawTexture( pTex );
			pTex->Touch();
			HRESULT hRes = pDevice->SetTexture( nStage, pTex->pTB->obj );
			ASSERT( hRes == D3D_OK );
		}
		else
		{
			HRESULT hRes = pDevice->SetTexture( nStage, 0 );
			ASSERT( hRes == D3D_OK );
		}
		return;
	}
	if ( !IsValid(pSmallTexture) )
		pSmallTexture = MakeTexture( 1,1, 1, NGfx::SPixel4444::ID, REGULAR, WRAP );
	pSmallTexture->Touch();
	HRESULT hRes = pDevice->SetTexture( nStage, pSmallTexture->pTB->obj );
	ASSERT( hRes == D3D_OK );
}

static CObj<CCubeTexture> pSmallCubeTexture;
void SetTexture( int nStage, CCubeTexture *pTex )
{
	if ( !bNoTexture )
	{
		if ( IsValid( pTex ) )
		{
			pTex->Touch();
			HRESULT hRes = pDevice->SetTexture( nStage, pTex->GetTB()->obj );
			ASSERT( hRes == D3D_OK );
		}
		else
		{
			HRESULT hRes = pDevice->SetTexture( nStage, 0 );
			ASSERT( hRes == D3D_OK );
		}
		return;
	}
	if ( !IsValid(pSmallCubeTexture) )
		pSmallCubeTexture = MakeCubeTexture( 2, 1, NGfx::SPixel8888::ID, REGULAR );
	pSmallCubeTexture->Touch();
	HRESULT hRes = pDevice->SetTexture( nStage, pSmallCubeTexture->GetTB()->obj );
	ASSERT( hRes == D3D_OK );
}

void AddPrimitiveGeometry( CGeometry *pGeom, CTriList *pTriList, int nBaseVertex, int nVertices )
{
	pGeom->DoTouch();
	ASSERT( nBaseVertex >= 0 && nVertices > 0 );
	ASSERT( nBaseVertex + nVertices <= pGeom->GetVBSize());
	const std::vector<S3DTriangle> &tris = pTriList->GetTris();
	int nTris = pTriList->GetTrisNumber();
	int nVBStart = pGeom->GetVBStart() + nBaseVertex, nVBSize = nVertices;
	if ( bBan32BitIndices )
	{
		pGeom->SetVertexStream();
		dynamicTris16.AddPrimitiveGeometry( &tris[0], nTris, nVBStart, nVBSize );
		//pTriList->DrawPrimitive( pGeom->GetVBStart() + nBaseVertex, 0, nVertices );
		return;
	}
	dynamicTris32.AddPrimitiveGeometry( &tris[0], nTris, nVBStart, nVBSize );
	renderStats.nVertices += nVBSize;
	renderStats.nTris += nTris;
}

void AddPrimitiveGeometry( CGeometry *pGeom, CTriList *pTriList )
{
	AddPrimitiveGeometry( pGeom, pTriList, 0, pGeom->GetVBSize() );
}

template<class T>
BOOST_FORCEINLINE void AddPrimitiveGeometry( T *pRes, CGeometry *pGeom, const STriangleList *pTris, int nCount, unsigned nMask )
{
	pGeom->DoTouch();
	int nVBStart = pGeom->GetVBStart(), nVBSize = pGeom->GetVBSize();
	S3DTriangle *pTriBufLast = 0;
	int nOffsetLast = 0, nTrisLast = 0;
	for ( int k = 0; k < nCount; ++k )
	{
		if ( ( nMask & (1<<k) ) == 0 )
			continue;
		const STriangleList &t = pTris[k];
		int nTris = t.nTris;
		//dynamicTris.AddPrimitiveGeometry( (S3DTriangle*)t.pTri, nTris, nVBStart + t.nBaseIndex, nVBSize - t.nBaseIndex );
		S3DTriangle *pAddTris = (S3DTriangle*)t.pTri;
		if ( pTriBufLast )
		{
			if ( nOffsetLast == t.nBaseIndex && pTriBufLast + nTrisLast == pAddTris )
			{
				nTrisLast += nTris;
				continue;
			}
			pRes->AddPrimitiveGeometry( pTriBufLast, nTrisLast, nVBStart + nOffsetLast, nVBSize - nOffsetLast );
		}
		pTriBufLast = pAddTris;
		nOffsetLast = t.nBaseIndex;
		nTrisLast = nTris;
	}
	if ( pTriBufLast )
		pRes->AddPrimitiveGeometry( pTriBufLast, nTrisLast, nVBStart + nOffsetLast, nVBSize - nOffsetLast );
}

void AddPrimitiveGeometry( CGeometry *pGeom, const STriangleList *pTris, int nCount, unsigned nMask )
{
	ASSERT( sizeof(S3DTriangle) == sizeof(STriangle) ); // is required for this function to work
	if ( bBan32BitIndices )
		AddPrimitiveGeometry( &dynamicTris16, pGeom, pTris, nCount, nMask );
	else
		AddPrimitiveGeometry( &dynamicTris32, pGeom, pTris, nCount, nMask );
}

void FlushPrimitive()
{
	// for 16 bit buffers FlushPrimitive done in AddPrimitiveGeometry
	if ( !bBan32BitIndices )
		dynamicTris32.FlushPrimitive();
}

void AddLineStrip( CGeometry *pGeom, const unsigned short *pIndices, int nLines )
{
	pGeom->DoTouch();
	int nVBGeomSize = pGeom->GetVBSize();
	int nVBGeomStart = pGeom->GetVBStart();
	if ( nVBGeomSize < 2 )
		return;
	if ( bBan32BitIndices )
		dynamicTris16.AddLinestrip( nVBGeomStart, nVBGeomSize, pIndices, nLines );
	else
		dynamicTris32.AddLinestrip( nVBGeomStart, nVBGeomSize, pIndices, nLines );
//	dynamicTris.FlushPrimitive();
}

void GetRenderTargetData( NGfx::CTexture *_pTarget, NGfx::CTexture *_pSrc )
{
	ASSERT( IsValid( _pSrc ) && IsValid( _pTarget ) );
	NWin32Helper::com_ptr<IDirect3DSurface9> pSrc, pBuf, pDst;
	GetSurface( _pSrc, 0, &pSrc );
	D3DSURFACE_DESC desc;
	pSrc->GetDesc( &desc );

	HRESULT hr;
	hr = pDevice->CreateOffscreenPlainSurface( desc.Width, desc.Height,
		desc.Format, D3DPOOL_SYSTEMMEM, pBuf.GetAddr(), 0 );
	ASSERT( D3D_OK == hr );
	hr = pDevice->GetRenderTargetData( pSrc, pBuf );
	ASSERT( D3D_OK == hr );
	//HRESULT hr = pDevice->CopyRects( pSrc, 0, 0, pDst, 0 );

	GetSurface( _pTarget, 0, &pDst );
#ifdef _DEBUG
	D3DSURFACE_DESC descTarget;
	pDst->GetDesc( &descTarget );
	ASSERT( desc.Format == descTarget.Format );
	ASSERT( desc.Width == descTarget.Width );
	ASSERT( desc.Height == descTarget.Height );
#endif
	D3DLOCKED_RECT data;
	hr = pBuf->LockRect( &data, 0, D3DLOCK_READONLY );
	ASSERT( D3D_OK == hr );
	D3DLOCKED_RECT dataDst;
	hr = pDst->LockRect( &dataDst, 0, 0 );
	ASSERT( D3D_OK == hr );
	char *pSrcData = (char*)data.pBits, *pDstData = (char*)dataDst.pBits;
	for ( int y = 0; y < desc.Height; ++y )
	{
		memcpy( pDstData, pSrcData, desc.Width * GetBpp( desc.Format ) / 8 );
		pSrcData += data.Pitch;
		pDstData += dataDst.Pitch;
	}
	hr = pBuf->UnlockRect();
	ASSERT( D3D_OK == hr );
	hr = pDst->UnlockRect();
	ASSERT( D3D_OK == hr );
}

void GetRenderTargetData( CArray2D<NGfx::SPixel8888> *pRes, NGfx::CTexture *_pSrc )
{
	ASSERT( IsValid( _pSrc ) );
	NWin32Helper::com_ptr<IDirect3DSurface9> pSrc, pDst;
	GetSurface( _pSrc, 0, &pSrc );
	D3DSURFACE_DESC desc;
	pSrc->GetDesc( &desc );
	HRESULT hr;
	hr = pDevice->CreateOffscreenPlainSurface( desc.Width, desc.Height,
		D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, pDst.GetAddr(), 0 );
	ASSERT( D3D_OK == hr );
	hr = pDevice->GetRenderTargetData( pSrc, pDst );
	ASSERT( D3D_OK == hr );
	D3DLOCKED_RECT data;
	hr = pDst->LockRect( &data, 0, D3DLOCK_READONLY );
	ASSERT( D3D_OK == hr );
	pRes->SetSizes( desc.Width, desc.Height );
	char *pData = (char*)data.pBits;
	for ( int y = 0; y < pRes->GetSizeY(); ++y, pData += data.Pitch )
	  memcpy( &(*pRes)[y][0], pData, sizeof((*pRes)[0][0]) * pRes->GetSizeX() );
	hr = pDst->UnlockRect();
	ASSERT( D3D_OK == hr );
}

int CalcTouchedTextureSize()
{
	int nRes = 0;
	for ( std::vector< CMObj<CObjectBase> >::iterator i = managed.begin(); i != managed.end(); ++i )
	{
		CDynamicCast<CTexture> pTexture( *i );
		if ( IsValid( pTexture ) && pTexture->GetFrameMRU() > nCurrentFrame - N_MAX_PRESENTS_IN_QUEUE - 2 )
			nRes += pTexture->pTB->GetRawSize();
	}
	return nRes;
}

int CalcTotalTextureSize( int *pnTexturesCount )
{
	int nRes = 0, nCount = 0;
	for ( std::vector< CMObj<CObjectBase> >::iterator i = managed.begin(); i != managed.end(); ++i )
	{
		CDynamicCast<CTexture> pTexture( *i );
		if ( IsValid( pTexture ) )
		{
			nRes += pTexture->pTB->GetRawSize();
			++nCount;
		}
	}
	if ( pnTexturesCount )
		*pnTexturesCount = nCount;
	return nRes;
}

int CalcTouchedTextureSizeNotSetMip( int nMip )
{
	int nRes = 0;
	for ( std::vector< CMObj<CObjectBase> >::iterator i = managed.begin(); i != managed.end(); ++i )
	{
		CDynamicCast<CTexture> pTexture( *i );
		if ( IsValid( pTexture ) && pTexture->GetFrameMRU() > nCurrentFrame - N_MAX_PRESENTS_IN_QUEUE - 2 )
			nRes += pTexture->pTB->GetRawSizeNotSetMip( nMip );
	}
	return nRes;
}

void SetLODToAllTextures( int nLOD )
{
	for ( std::vector< CMObj<CObjectBase> >::iterator i = managed.begin(); i != managed.end(); ++i )
	{
		CDynamicCast<CTexture> pTexture( *i );
		if ( IsValid( pTexture ) )
			pTexture->pTB->SetLOD( nLOD );
	}
}

template<class TBuf, class TElem>
static bool IsThrashing( CLinearBuffer<TBuf,TElem> *pLinearBuffer )
{
	NCache::SStats stats;
	pLinearBuffer->CalcStats( &stats );
	return stats.bThrashing;
}

bool IsStaticGeometryThrashing()
{
	bool bRes = false;
	for ( CGeometryCacheHash::iterator i = geometries.begin(); i != geometries.end(); ++i )
		bRes |= i->first.usage == STATIC && IsThrashing( i->second.GetPtr() );
	return bRes;
}

bool IsDynamicGeometryThrashing()
{
	bool bRes = false;
	for ( CGeometryCacheHash::iterator i = geometries.begin(); i != geometries.end(); ++i )
		bRes |= i->first.usage == DYNAMIC && IsThrashing( i->second.GetPtr() );
	return bRes;
}

bool Is2DTextureThrashing()
{
	return textureCache.IsThrashing();
}

bool IsTransparentThrashing()
{
	return transparentCache.IsThrashing();
}

bool CanStreamGeometry()
{
	return !bBan32BitIndices;
}

void FlushQueue()
{
	if ( geometries.empty() )
		return;
	geometries.begin()->second->Flush();
}

CTexture* GetLinearBufferMRU( EBufferUsage usage )
{
	if ( !IsValid( pLinearBufferMRU ) )
		pLinearBufferMRU = MakeTexture( 1024, 1024, 1, SPixel8888::ID, REGULAR, CLAMP );
	if ( geometries.find( SGeometryType(usage,SGeomVecFull::ID) ) == geometries.end() )
		return 0;
	geometries[SGeometryType(usage,SGeomVecFull::ID)]->DrawRU( pLinearBufferMRU );
	return pLinearBufferMRU;
}

template<class TBuf, class TElem>
void TypeStats( CLinearBuffer<TBuf,TElem> *pLinearBuffer )
{
	NCache::SStats stats;
	pLinearBuffer->CalcStats( &stats );
	csSystem << "size=" << stats.nFree + stats.nUsed << "; blocks=" << stats.nBlocks <<"; used=" << stats.nUsed << endl;
	csSystem << "frame=" << (int)stats.nCurrentRU << "; eldest=" << (int)stats.nEldestEntry;
	if ( stats.bThrashing )
		csSystem << " thrashing" << endl;
	else 
		csSystem << " ok" << endl;
}

static void ShowCacheStats( const std::string &szID, const std::vector<std::wstring> &szParams, void *pContext )
{
	for ( CGeometryCacheHash::iterator i = geometries.begin(); i != geometries.end(); ++i )
	{
		csSystem << "type " << i->first.nID;
		if ( i->first.usage == STATIC )
			csSystem << " static geometry" << endl;
		else
			csSystem << " dynamic geometry" << endl;
		TypeStats( i->second.GetPtr() );
	}
}
static void ShowFill2D( const std::string &szID, const std::vector<std::wstring> &szParams, void *pContext )
{
	bFill2d = !bFill2d;
}
static void ShowFillTransp( const std::string &szID, const std::vector<std::wstring> &szParams, void *pContext )
{
	bFillTransp = !bFillTransp;
}

START_REGISTER(GfxBuffers)
	REGISTER_CMD( "gfx_stats", ShowCacheStats )
	REGISTER_CMD( "gfx_fill_2d", ShowFill2D )
	REGISTER_CMD( "gfx_fill_transp", ShowFillTransp )
	REGISTER_VAR_EX( "gfx_draw_mip", NGlobal::VarBoolHandler, &bDrawMip, 0, STORAGE_NONE )
	REGISTER_CMD( "gfx_fill_transp", ShowFillTransp )
FINISH_REGISTER
}

using namespace NGfx;
BASIC_REGISTER_CLASS( _3DMOTOR, CTexture )
BASIC_REGISTER_CLASS( _3DMOTOR, CGeometry )
BASIC_REGISTER_CLASS( _3DMOTOR, CTriList )
BASIC_REGISTER_CLASS( _3DMOTOR, CCubeTexture )

