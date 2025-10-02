#ifndef __GFXBUFFERSINTERNAL_H_
#define __GFXBUFFERSINTERNAL_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "GfxInternal.h"
#include "Gfx.h"
#include "GfxCatchGPF.h"

namespace NGfx
{

enum ETrueBufferUsage
{
	TBU_STATIC,
	TBU_DYNAMIC,
	TBU_SOFTWARE
};
inline DWORD GetUsageFlags( ETrueBufferUsage eUsage )
{
	switch ( eUsage )
	{
	case TBU_STATIC:    return D3DUSAGE_WRITEONLY;
	case TBU_DYNAMIC:   return D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY;
	case TBU_SOFTWARE:  return D3DUSAGE_SOFTWAREPROCESSING | D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
	default:
		ASSERT( 0 );
	}
	return 0;
}
inline D3DPOOL GetPool( ETrueBufferUsage eUsage )
{
	switch ( eUsage )
	{
	case TBU_STATIC:    return D3DPOOL_DEFAULT;
	case TBU_DYNAMIC:   return D3DPOOL_DEFAULT;
	case TBU_SOFTWARE:  return D3DPOOL_SYSTEMMEM;
	default:
		ASSERT( 0 );
	}
	return D3DPOOL_DEFAULT;
}

class CLockable: public CObjectBase
{
public:
	virtual void Free() = 0;
};
extern bool bWasLinearBufferLock;
void FreeLinearBuffers();

// general actions on linear DX buffers
// dwFlags = bDiscard ? D3DLOCK_DISCARD: D3DLOCK_NOOVERWRITE
template <class TDXobj>
class CRBase: public CLockable
{
public:
	NWin32Helper::com_ptr< TDXobj > obj;
	int nLockCount;
	unsigned char *pLocked;
	int nSize;  // size in bytes

	CRBase(): nLockCount(0), pLocked(0) {}
	CRBase( int _nSize ): nSize(_nSize), nLockCount(0), pLocked(0) {}
	~CRBase() { Free(); }
	int GetSize() const { return nSize; }
	void Lock( DWORD dwFlags )
	{
		ASSERT( nLockCount == 0 || dwFlags == 0 || dwFlags == D3DLOCK_NOOVERWRITE );
		++nLockCount;
		if ( !pLocked )
		{
			//HRESULT hr = obj.obj->Lock( nOffset, nSize, &obj.pLocked, dwFlags );
			HRESULT hr = obj->Lock( 0, 0, (void**)&pLocked, dwFlags );
			ASSERT( hr == D3D_OK );
			bWasLinearBufferLock = true;
			AddIgnoreAccessViolationRegion( pLocked, nSize );
		}
	}
	void Unlock()
	{
		ASSERT( nLockCount > 0 );
		--nLockCount;
	}
	virtual void Free() 
	{
		ASSERT( nLockCount == 0 );
		if ( pLocked )
		{
			RemoveIgnoreAccessViolationRegion( pLocked, nSize );
			HRESULT hr = obj->Unlock();
			ASSERT( hr == D3D_OK );
			pLocked = 0;
		}
	}
};

template<class T>
inline void TryLockBuffer( T *p, ETrueBufferUsage eUsage )
{
	if ( eUsage == TBU_DYNAMIC )
		p->Lock( D3DLOCK_DISCARD );
	else
		p->Lock( 0 );
	p->Unlock();
}

// large vertex buffer
class CVB: public CRBase<IDirect3DVertexBuffer9>
{
	OBJECT_NOCOPY_METHODS( CVB );
public:
	CVB() {}
	// size in bytes
	CVB( int _nSize, ETrueBufferUsage eUsage ): CRBase<IDirect3DVertexBuffer9>(_nSize)
	{
		HRESULT hRes = pDevice->CreateVertexBuffer(
			nSize, 
			GetUsageFlags( eUsage ),
			0,
			GetPool( eUsage ), 
			obj.GetAddr(),
			0
			);
		D3DASSERT( hRes, "CreateVertexBuffer failed with SIZE: %d USAGE: %d", nSize, eUsage );
		bInitOk &= SUCCEEDED(hRes);

		if ( bInitOk )
			TryLockBuffer( this, eUsage );
	}
};

// large index buffer
template <int TFormat>
class CIBFast: public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CIBFast );
public:
	NWin32Helper::com_ptr< IDirect3DIndexBuffer9 > obj;
	unsigned char *pLocked;
	int nSize;  // size in bytes

	CIBFast() : pLocked(0) {}
	// size in bytes
	CIBFast( int _nSize, ETrueBufferUsage eUsage ) : nSize(_nSize)
	{
		pLocked = 0;
		HRESULT hRes = pDevice->CreateIndexBuffer(
			_nSize, 
			GetUsageFlags( eUsage ),
			(D3DFORMAT)TFormat,
			GetPool( eUsage ),
			obj.GetAddr(),
			0
			);
		D3DASSERT( hRes, "CreateIndexBuffer failed with SIZE: %d FORMAT: %d USAGE: %d", _nSize, TFormat, eUsage );
		bInitOk &= SUCCEEDED(hRes);

		if ( bInitOk )
			TryLockBuffer( this, eUsage );
	}
	void Lock( DWORD dwFlags )
	{
		ASSERT( !pLocked );
		HRESULT hr = obj->Lock( 0, 0, (void**)&pLocked, dwFlags );
		ASSERT( hr == D3D_OK );
		AddIgnoreAccessViolationRegion( pLocked, nSize );
	}
	void Unlock()
	{
		ASSERT( pLocked );
		RemoveIgnoreAccessViolationRegion( pLocked, nSize );
		HRESULT hr = obj->Unlock();
		ASSERT( hr == D3D_OK );
		pLocked = 0;
	}
};
typedef CIBFast<D3DFMT_INDEX32> CIB32Fast;
typedef CIBFast<D3DFMT_INDEX16> CIB16Fast;

// 16 bit index buffer
//class CIB16: public CRBase<IDirect3DIndexBuffer8>

}
#endif

