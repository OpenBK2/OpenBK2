#pragma once

// Everything a device creates. None of it holds anything a GPU could use; each
// holds what the engine and D3DX read back -- a description, and memory the
// right size and layout for a Lock to hand out -- and a real reference count.
//
// Reference counting follows D3D9 where it matters:
//
//   * every object made by a device holds a private reference on it, so a
//     device whose last public reference goes stays alive until its resources
//     do (CDevice::InternalAddRef). Surfaces the device owns itself, the back
//     buffer and the automatic depth buffer, do not, or it could never go;
//   * a texture's level surfaces are part of it: AddRef and Release on one
//     count the texture, as D3D9 does, so a level handed out by
//     GetSurfaceLevel keeps its texture alive and cannot outlive it.

#include "Formats.h"
#include "Stubs.h"

#include <malloc.h>

#include <cstring>
#include <vector>

namespace ND3D9Stub
{
	class CDevice;

	// Defined in Device.cpp; declared here so the resources need not see the
	// whole device.
	void DeviceInternalAddRef( CDevice *pDevice );
	void DeviceInternalRelease( CDevice *pDevice );
	IDirect3DDevice9* DeviceInterface( CDevice *pDevice );

	// Memory a Lock hands out: zeroed, 64-byte aligned -- more than any SIMD
	// store the engine or D3DX makes needs -- and made on the first Lock, so a
	// render target nobody ever locks costs nothing.
	class CLockMemory
	{
		void *pData = nullptr;
		size_t nSize = 0;

	public:
		CLockMemory() = default;
		CLockMemory( const CLockMemory& ) = delete;
		CLockMemory& operator=( const CLockMemory& ) = delete;
		~CLockMemory()
		{
			_aligned_free( pData );
		}

		void SetSize( size_t _nSize )
		{
			nSize = _nSize;
		}

		size_t GetSize() const
		{
			return nSize;
		}

		uint8_t* Get()
		{
			if ( pData == nullptr )
			{
				const size_t nAllocate = nSize > 0 ? nSize : 16;
				pData = _aligned_malloc( nAllocate, 64 );
				if ( pData != nullptr )
				{
					std::memset( pData, 0, nAllocate );
				}
			}
			return static_cast<uint8_t*>( pData );
		}
	};


	// A device's child: GetDevice, and the private reference on the device.
	template<class TStub>
	class TDeviceChild : public TStub
	{
	protected:
		CDevice *pDevice;
		bool bHoldsDevice;

	public:
		TDeviceChild( CDevice *_pDevice, bool _bHoldsDevice )
			: pDevice( _pDevice ), bHoldsDevice( _bHoldsDevice )
		{
			if ( bHoldsDevice )
			{
				DeviceInternalAddRef( pDevice );
			}
		}

		virtual ~TDeviceChild()
		{
			if ( bHoldsDevice )
			{
				DeviceInternalRelease( pDevice );
			}
		}

		STDMETHOD(GetDevice)( IDirect3DDevice9 **ppDevice ) override
		{
			D3D9_TRACE( "IDirect3DResource9", ppDevice );
			if ( ppDevice == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			*ppDevice = DeviceInterface( pDevice );
			( *ppDevice )->AddRef();
			return D3D_OK;
		}
	};


	// What every resource has: private data nobody reads, a priority nobody
	// uses, and the pool and usage it was made with.
	template<class TStub>
	class TResource : public TDeviceChild<TStub>
	{
	protected:
		DWORD nPriority = 0;

	public:
		TResource( CDevice *pDevice, bool bHoldsDevice ) : TDeviceChild<TStub>( pDevice, bHoldsDevice ) {}

		// Accepted and dropped: the engine does not read it back, and a stub
		// that failed here would fail resource creation in D3DX.
		STDMETHOD(SetPrivateData)( REFGUID refguid, const void *pData, DWORD SizeOfData, DWORD Flags ) override
		{
			D3D9_TRACE( "IDirect3DResource9", refguid, pData, SizeOfData, Flags );
			return D3D_OK;
		}
		STDMETHOD(GetPrivateData)( REFGUID refguid, void *pData, DWORD *pSizeOfData ) override
		{
			D3D9_TRACE( "IDirect3DResource9", refguid, pData, pSizeOfData );
			return D3DERR_NOTFOUND;
		}
		STDMETHOD(FreePrivateData)( REFGUID refguid ) override
		{
			D3D9_TRACE( "IDirect3DResource9", refguid );
			return D3D_OK;
		}
		STDMETHOD_(DWORD, SetPriority)( DWORD PriorityNew ) override
		{
			D3D9_TRACE( "IDirect3DResource9", PriorityNew );
			const DWORD nOld = nPriority;
			nPriority = PriorityNew;
			return nOld;
		}
		STDMETHOD_(DWORD, GetPriority)() override
		{
			D3D9_TRACE0( "IDirect3DResource9" );
			return nPriority;
		}
		STDMETHOD_(void, PreLoad)() override
		{
			D3D9_TRACE0( "IDirect3DResource9" );
		}
	};


	class CSurface : public TResource<CDirect3DSurface9Stub>
	{
		D3DSURFACE_DESC desc;
		size_t nPitch;
		CLockMemory memory;
		// The texture this is a level of, which it counts references on; null
		// for a surface of its own.
		IUnknown *pContainer;
		bool bLocked = false;

	public:
		CSurface( CDevice *pDevice, const D3DSURFACE_DESC &rDesc, IUnknown *_pContainer, bool bHoldsDevice )
			: TResource<CDirect3DSurface9Stub>( pDevice, bHoldsDevice ), desc( rDesc ),
			nPitch( Pitch( rDesc.Format, rDesc.Width ) ), pContainer( _pContainer )
		{
			bool bKnown = true;
			BitsPerPixel( rDesc.Format, &bKnown );
			if ( !bKnown && !IsBlockCompressed( rDesc.Format ) )
			{
				Note( "surface format %d (0x%X) is not in Formats.h; laid out as 32 bits a pixel", rDesc.Format,
							rDesc.Format );
			}
			memory.SetSize( nPitch * Rows( rDesc.Format, rDesc.Height ) );
		}

		const D3DSURFACE_DESC& Desc() const
		{
			return desc;
		}

		// A level's references are its texture's.
		STDMETHOD_(ULONG, AddRef)() override
		{
			return pContainer != nullptr ? pContainer->AddRef() : CDirect3DSurface9Stub::AddRef();
		}
		STDMETHOD_(ULONG, Release)() override
		{
			return pContainer != nullptr ? pContainer->Release() : CDirect3DSurface9Stub::Release();
		}

		STDMETHOD_(D3DRESOURCETYPE, GetType)() override
		{
			D3D9_TRACE0( "IDirect3DResource9" );
			return D3DRTYPE_SURFACE;
		}
		STDMETHOD(GetContainer)( REFIID riid, void **ppContainer ) override
		{
			D3D9_TRACE( "IDirect3DSurface9", riid, ppContainer );
			if ( pContainer != nullptr )
			{
				return pContainer->QueryInterface( riid, ppContainer );
			}
			// A surface of its own is contained by its device.
			return DeviceInterface( pDevice )->QueryInterface( riid, ppContainer );
		}
		STDMETHOD(GetDesc)( D3DSURFACE_DESC *pDesc ) override
		{
			D3D9_TRACE( "IDirect3DSurface9", pDesc );
			if ( pDesc == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			*pDesc = desc;
			return D3D_OK;
		}
		STDMETHOD(LockRect)( D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags ) override
		{
			D3D9_TRACE( "IDirect3DSurface9", pLockedRect, pRect, Flags );
			return Lock( pLockedRect, pRect );
		}
		STDMETHOD(UnlockRect)() override
		{
			D3D9_TRACE0( "IDirect3DSurface9" );
			return Unlock();
		}

		// The texture's LockRect comes here without going through the trace a
		// second time.
		HRESULT Lock( D3DLOCKED_RECT *pLockedRect, const RECT *pRect )
		{
			if ( pLockedRect == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			uint8_t *pBits = memory.Get();
			if ( pBits == nullptr )
			{
				Note( "LockRect: out of memory for %u bytes", static_cast<unsigned>( memory.GetSize() ) );
				return E_OUTOFMEMORY;
			}
			size_t nOffset = 0;
			if ( pRect != nullptr )
			{
				if ( pRect->left < 0 || pRect->top < 0 || pRect->right > static_cast<LONG>( desc.Width ) ||
						 pRect->bottom > static_cast<LONG>( desc.Height ) || pRect->left >= pRect->right ||
						 pRect->top >= pRect->bottom )
				{
					Note( "LockRect: rect (%ld,%ld)-(%ld,%ld) outside a %ux%u surface", pRect->left, pRect->top,
								pRect->right, pRect->bottom, desc.Width, desc.Height );
					return D3DERR_INVALIDCALL;
				}
				nOffset = Offset( desc.Format, nPitch, pRect->left, pRect->top );
			}
			pLockedRect->Pitch = static_cast<INT>( nPitch );
			pLockedRect->pBits = pBits + nOffset;
			bLocked = true;
			return D3D_OK;
		}

		HRESULT Unlock()
		{
			bLocked = false;
			return D3D_OK;
		}
	};


	// Texture, cube texture: the level bookkeeping both share.
	template<class TStub>
	class TBaseTexture : public TResource<TStub>
	{
	protected:
		DWORD nUsage;
		DWORD nLevels;
		DWORD nLOD = 0;
		D3DTEXTUREFILTERTYPE eAutoGenFilter = D3DTEXF_LINEAR;

	public:
		TBaseTexture( CDevice *pDevice, DWORD _nUsage, DWORD _nLevels )
			: TResource<TStub>( pDevice, true ), nUsage( _nUsage ), nLevels( _nLevels ) {}

		STDMETHOD_(DWORD, SetLOD)( DWORD LODNew ) override
		{
			D3D9_TRACE( "IDirect3DBaseTexture9", LODNew );
			const DWORD nOld = nLOD;
			nLOD = LODNew;
			return nOld;
		}
		STDMETHOD_(DWORD, GetLOD)() override
		{
			D3D9_TRACE0( "IDirect3DBaseTexture9" );
			return nLOD;
		}
		// An autogenerated chain shows one level, as D3D9's does.
		STDMETHOD_(DWORD, GetLevelCount)() override
		{
			D3D9_TRACE0( "IDirect3DBaseTexture9" );
			return ( nUsage & D3DUSAGE_AUTOGENMIPMAP ) != 0 ? 1 : nLevels;
		}
		STDMETHOD(SetAutoGenFilterType)( D3DTEXTUREFILTERTYPE FilterType ) override
		{
			D3D9_TRACE( "IDirect3DBaseTexture9", FilterType );
			eAutoGenFilter = FilterType;
			return D3D_OK;
		}
		STDMETHOD_(D3DTEXTUREFILTERTYPE, GetAutoGenFilterType)() override
		{
			D3D9_TRACE0( "IDirect3DBaseTexture9" );
			return eAutoGenFilter;
		}
		STDMETHOD_(void, GenerateMipSubLevels)() override
		{
			D3D9_TRACE0( "IDirect3DBaseTexture9" );
		}
	};


	// The description of level nLevel of a chain whose top is rTop.
	inline D3DSURFACE_DESC LevelDesc( const D3DSURFACE_DESC &rTop, DWORD nLevel )
	{
		D3DSURFACE_DESC desc = rTop;
		desc.Width = ( std::max )( 1u, rTop.Width >> nLevel );
		desc.Height = ( std::max )( 1u, rTop.Height >> nLevel );
		return desc;
	}


	class CTexture : public TBaseTexture<CDirect3DTexture9Stub>
	{
		std::vector<CSurface*> levels;

	public:
		CTexture( CDevice *pDevice, const D3DSURFACE_DESC &rTop, DWORD nUsage, DWORD nLevels )
			: TBaseTexture<CDirect3DTexture9Stub>( pDevice, nUsage, nLevels )
		{
			for ( DWORD nLevel = 0; nLevel < nLevels; ++nLevel )
			{
				levels.push_back( new CSurface( pDevice, LevelDesc( rTop, nLevel ), this, false ) );
			}
		}

		// The levels are parts of this, not separately counted objects.
		~CTexture() override
		{
			for ( CSurface *pLevel : levels )
			{
				delete pLevel;
			}
		}

		STDMETHOD_(D3DRESOURCETYPE, GetType)() override
		{
			D3D9_TRACE0( "IDirect3DResource9" );
			return D3DRTYPE_TEXTURE;
		}
		STDMETHOD(GetLevelDesc)( UINT Level, D3DSURFACE_DESC *pDesc ) override
		{
			D3D9_TRACE( "IDirect3DTexture9", Level, pDesc );
			if ( Level >= levels.size() || pDesc == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			*pDesc = levels[Level]->Desc();
			return D3D_OK;
		}
		STDMETHOD(GetSurfaceLevel)( UINT Level, IDirect3DSurface9 **ppSurfaceLevel ) override
		{
			D3D9_TRACE( "IDirect3DTexture9", Level, ppSurfaceLevel );
			if ( Level >= levels.size() || ppSurfaceLevel == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			*ppSurfaceLevel = levels[Level];
			levels[Level]->AddRef();
			return D3D_OK;
		}
		STDMETHOD(LockRect)( UINT Level, D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags ) override
		{
			D3D9_TRACE( "IDirect3DTexture9", Level, pLockedRect, pRect, Flags );
			if ( Level >= levels.size() )
			{
				return D3DERR_INVALIDCALL;
			}
			return levels[Level]->Lock( pLockedRect, pRect );
		}
		STDMETHOD(UnlockRect)( UINT Level ) override
		{
			D3D9_TRACE( "IDirect3DTexture9", Level );
			if ( Level >= levels.size() )
			{
				return D3DERR_INVALIDCALL;
			}
			return levels[Level]->Unlock();
		}
		STDMETHOD(AddDirtyRect)( const RECT *pDirtyRect ) override
		{
			D3D9_TRACE( "IDirect3DTexture9", pDirtyRect );
			return D3D_OK;
		}
	};


	class CCubeTexture : public TBaseTexture<CDirect3DCubeTexture9Stub>
	{
		// Face by face, each a chain of levels.
		std::vector<CSurface*> faces[6];

	public:
		CCubeTexture( CDevice *pDevice, const D3DSURFACE_DESC &rTop, DWORD nUsage, DWORD nLevels )
			: TBaseTexture<CDirect3DCubeTexture9Stub>( pDevice, nUsage, nLevels )
		{
			for ( auto &face : faces )
			{
				for ( DWORD nLevel = 0; nLevel < nLevels; ++nLevel )
				{
					face.push_back( new CSurface( pDevice, LevelDesc( rTop, nLevel ), this, false ) );
				}
			}
		}

		~CCubeTexture() override
		{
			for ( auto &face : faces )
			{
				for ( CSurface *pLevel : face )
				{
					delete pLevel;
				}
			}
		}

		CSurface* Level( D3DCUBEMAP_FACES eFace, UINT nLevel )
		{
			if ( static_cast<unsigned>( eFace ) >= 6 || nLevel >= faces[0].size() )
			{
				return nullptr;
			}
			return faces[eFace][nLevel];
		}

		STDMETHOD_(D3DRESOURCETYPE, GetType)() override
		{
			D3D9_TRACE0( "IDirect3DResource9" );
			return D3DRTYPE_CUBETEXTURE;
		}
		STDMETHOD(GetLevelDesc)( UINT Level, D3DSURFACE_DESC *pDesc ) override
		{
			D3D9_TRACE( "IDirect3DCubeTexture9", Level, pDesc );
			CSurface *pLevel = Level < faces[0].size() ? faces[0][Level] : nullptr;
			if ( pLevel == nullptr || pDesc == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			*pDesc = pLevel->Desc();
			return D3D_OK;
		}
		STDMETHOD(GetCubeMapSurface)( D3DCUBEMAP_FACES FaceType, UINT Level, IDirect3DSurface9 **ppCubeMapSurface ) override
		{
			D3D9_TRACE( "IDirect3DCubeTexture9", FaceType, Level, ppCubeMapSurface );
			CSurface *pLevel = this->Level( FaceType, Level );
			if ( pLevel == nullptr || ppCubeMapSurface == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			*ppCubeMapSurface = pLevel;
			pLevel->AddRef();
			return D3D_OK;
		}
		STDMETHOD(LockRect)( D3DCUBEMAP_FACES FaceType, UINT Level, D3DLOCKED_RECT *pLockedRect, const RECT *pRect,
												 DWORD Flags ) override
		{
			D3D9_TRACE( "IDirect3DCubeTexture9", FaceType, Level, pLockedRect, pRect, Flags );
			CSurface *pLevel = this->Level( FaceType, Level );
			return pLevel != nullptr ? pLevel->Lock( pLockedRect, pRect ) : D3DERR_INVALIDCALL;
		}
		STDMETHOD(UnlockRect)( D3DCUBEMAP_FACES FaceType, UINT Level ) override
		{
			D3D9_TRACE( "IDirect3DCubeTexture9", FaceType, Level );
			CSurface *pLevel = this->Level( FaceType, Level );
			return pLevel != nullptr ? pLevel->Unlock() : D3DERR_INVALIDCALL;
		}
		STDMETHOD(AddDirtyRect)( D3DCUBEMAP_FACES FaceType, const RECT *pDirtyRect ) override
		{
			D3D9_TRACE( "IDirect3DCubeTexture9", FaceType, pDirtyRect );
			return D3D_OK;
		}
	};


	// Vertex and index buffers: a size, a description, and memory to lock.
	template<class TStub, class TDesc, D3DRESOURCETYPE TYPE>
	class TBuffer : public TResource<TStub>
	{
	protected:
		TDesc desc;
		CLockMemory memory;

	public:
		TBuffer( CDevice *pDevice, const TDesc &rDesc ) : TResource<TStub>( pDevice, true ), desc( rDesc )
		{
			memory.SetSize( rDesc.Size );
		}

		STDMETHOD_(D3DRESOURCETYPE, GetType)() override
		{
			D3D9_TRACE0( "IDirect3DResource9" );
			return TYPE;
		}
		// Size 0 with offset 0 is the whole buffer, as in D3D9.
		STDMETHOD(Lock)( UINT OffsetToLock, UINT SizeToLock, void **ppbData, DWORD Flags ) override
		{
			D3D9_TRACE( TYPE == D3DRTYPE_VERTEXBUFFER ? "IDirect3DVertexBuffer9" : "IDirect3DIndexBuffer9",
									OffsetToLock, SizeToLock, ppbData, Flags );
			if ( ppbData == nullptr || OffsetToLock > desc.Size || SizeToLock > desc.Size - OffsetToLock )
			{
				Note( "Lock: %u bytes at %u of a %u-byte buffer", SizeToLock, OffsetToLock, desc.Size );
				return D3DERR_INVALIDCALL;
			}
			uint8_t *pData = memory.Get();
			if ( pData == nullptr )
			{
				return E_OUTOFMEMORY;
			}
			*ppbData = pData + OffsetToLock;
			return D3D_OK;
		}
		STDMETHOD(Unlock)() override
		{
			D3D9_TRACE0( TYPE == D3DRTYPE_VERTEXBUFFER ? "IDirect3DVertexBuffer9" : "IDirect3DIndexBuffer9" );
			return D3D_OK;
		}
		STDMETHOD(GetDesc)( TDesc *pDesc ) override
		{
			D3D9_TRACE( TYPE == D3DRTYPE_VERTEXBUFFER ? "IDirect3DVertexBuffer9" : "IDirect3DIndexBuffer9", pDesc );
			if ( pDesc == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			*pDesc = desc;
			return D3D_OK;
		}
	};

	typedef TBuffer<CDirect3DVertexBuffer9Stub, D3DVERTEXBUFFER_DESC, D3DRTYPE_VERTEXBUFFER> CVertexBuffer;
	typedef TBuffer<CDirect3DIndexBuffer9Stub, D3DINDEXBUFFER_DESC, D3DRTYPE_INDEXBUFFER> CIndexBuffer;


	class CVertexDeclaration : public TDeviceChild<CDirect3DVertexDeclaration9Stub>
	{
		std::vector<D3DVERTEXELEMENT9> elements;

	public:
		// pElements runs to and includes the D3DDECL_END entry.
		CVertexDeclaration( CDevice *pDevice, const D3DVERTEXELEMENT9 *pElements )
			: TDeviceChild<CDirect3DVertexDeclaration9Stub>( pDevice, true )
		{
			for ( const D3DVERTEXELEMENT9 *p = pElements; ; ++p )
			{
				elements.push_back( *p );
				if ( p->Stream == 0xFF )
				{
					break;
				}
			}
		}

		STDMETHOD(GetDeclaration)( D3DVERTEXELEMENT9 *pElement, UINT *pNumElements ) override
		{
			D3D9_TRACE( "IDirect3DVertexDeclaration9", pElement, pNumElements );
			if ( pNumElements == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			*pNumElements = static_cast<UINT>( elements.size() );
			if ( pElement != nullptr )
			{
				std::memcpy( pElement, elements.data(), elements.size() * sizeof( D3DVERTEXELEMENT9 ) );
			}
			return D3D_OK;
		}
	};


	// Vertex and pixel shaders: the bytecode they were made from, for
	// GetFunction, and nothing else.
	template<class TStub>
	class TShader : public TDeviceChild<TStub>
	{
		std::vector<uint8_t> function;

	public:
		// Shader bytecode is DWORD tokens ending in D3DSIO_END, 0x0000FFFF.
		TShader( CDevice *pDevice, const DWORD *pFunction ) : TDeviceChild<TStub>( pDevice, true )
		{
			const DWORD *p = pFunction;
			while ( *p != 0x0000FFFF )
			{
				++p;
			}
			function.assign( reinterpret_cast<const uint8_t*>( pFunction ), reinterpret_cast<const uint8_t*>( p + 1 ) );
		}

		STDMETHOD(GetFunction)( void *pData, UINT *pSizeOfData ) override
		{
			// Parenthesised: the comma in the template arguments would split the
			// macro's arguments.
			D3D9_TRACE( ( std::is_same_v<TStub, CDirect3DVertexShader9Stub> ? "IDirect3DVertexShader9"
																																	: "IDirect3DPixelShader9" ),
									pData, pSizeOfData );
			if ( pSizeOfData == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			if ( pData != nullptr )
			{
				if ( *pSizeOfData < function.size() )
				{
					return D3DERR_INVALIDCALL;
				}
				std::memcpy( pData, function.data(), function.size() );
			}
			*pSizeOfData = static_cast<UINT>( function.size() );
			return D3D_OK;
		}
	};

	typedef TShader<CDirect3DVertexShader9Stub> CVertexShader;
	typedef TShader<CDirect3DPixelShader9Stub> CPixelShader;


	// A query that has always already finished, with nothing to report: its
	// data is zeros of the size the type has.
	class CQuery : public TDeviceChild<CDirect3DQuery9Stub>
	{
		D3DQUERYTYPE eType;

	public:
		CQuery( CDevice *pDevice, D3DQUERYTYPE _eType ) : TDeviceChild<CDirect3DQuery9Stub>( pDevice, true ), eType( _eType ) {}

		static DWORD DataSize( D3DQUERYTYPE eType )
		{
			switch ( eType )
			{
			case D3DQUERYTYPE_EVENT: return sizeof( BOOL );
			case D3DQUERYTYPE_OCCLUSION: return sizeof( DWORD );
			case D3DQUERYTYPE_TIMESTAMP: return sizeof( UINT64 );
			case D3DQUERYTYPE_TIMESTAMPDISJOINT: return sizeof( BOOL );
			case D3DQUERYTYPE_TIMESTAMPFREQ: return sizeof( UINT64 );
			default: return 0;
			}
		}

		STDMETHOD_(D3DQUERYTYPE, GetType)() override
		{
			D3D9_TRACE0( "IDirect3DQuery9" );
			return eType;
		}
		STDMETHOD_(DWORD, GetDataSize)() override
		{
			D3D9_TRACE0( "IDirect3DQuery9" );
			return DataSize( eType );
		}
		STDMETHOD(Issue)( DWORD dwIssueFlags ) override
		{
			D3D9_TRACE( "IDirect3DQuery9", dwIssueFlags );
			return D3D_OK;
		}
		STDMETHOD(GetData)( void *pData, DWORD dwSize, DWORD dwGetDataFlags ) override
		{
			D3D9_TRACE( "IDirect3DQuery9", pData, dwSize, dwGetDataFlags );
			if ( pData != nullptr && dwSize > 0 )
			{
				std::memset( pData, 0, dwSize );
				// An event is a fence; zeros would say it never passed.
				if ( eType == D3DQUERYTYPE_EVENT && dwSize >= sizeof( BOOL ) )
				{
					*static_cast<BOOL*>( pData ) = TRUE;
				}
			}
			return S_OK;
		}
	};


	// Records nothing and restores nothing; there is no state to get wrong.
	class CStateBlock : public TDeviceChild<CDirect3DStateBlock9Stub>
	{
	public:
		explicit CStateBlock( CDevice *pDevice ) : TDeviceChild<CDirect3DStateBlock9Stub>( pDevice, true ) {}

		STDMETHOD(Capture)() override
		{
			D3D9_TRACE0( "IDirect3DStateBlock9" );
			return D3D_OK;
		}
		STDMETHOD(Apply)() override
		{
			D3D9_TRACE0( "IDirect3DStateBlock9" );
			return D3D_OK;
		}
	};
}
