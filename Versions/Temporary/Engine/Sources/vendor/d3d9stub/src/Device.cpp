#include "Device.h"
#include "Resources.h"

#include <map>
#include <vector>

// The device. It draws nothing and answers everything the engine asks as a
// device with nothing to draw would: resources are made and can be locked,
// state is kept and can be read back, drawing and presenting succeed.
//
// What is here is what 3Dmotor, SceneB2, Image and D3DX were seen or expected
// to call; everything else is still the generated stub, which fails and says
// so in the trace. When the trace shows a stub being reached, it moves here.

namespace ND3D9Stub
{
	namespace
	{
		// A reference held on something bound to the device -- a texture, a
		// stream, a render target -- as D3D9 holds one, so that what the engine
		// has bound and released stays valid until it is unbound.
		template<class T>
		class TBound
		{
			T *p = nullptr;

		public:
			TBound() = default;
			TBound( const TBound& ) = delete;
			TBound& operator=( const TBound& ) = delete;
			~TBound()
			{
				Set( nullptr );
			}

			void Set( T *pNew )
			{
				if ( pNew != nullptr )
				{
					pNew->AddRef();
				}
				T *pOld = p;
				p = pNew;
				if ( pOld != nullptr )
				{
					pOld->Release();
				}
			}

			T* Get() const
			{
				return p;
			}

			// For a Get* method: the bound object, with a reference for the caller.
			void CopyTo( T **pp ) const
			{
				*pp = p;
				if ( p != nullptr )
				{
					p->AddRef();
				}
			}
		};

		// Samplers 0-15 and the four vertex texture samplers, which D3D9 numbers
		// D3DVERTEXTEXTURESAMPLER0 (257) and up.
		const unsigned SAMPLERS = 20;
		const unsigned TEXTURE_STAGES = 8;
		const unsigned STREAMS = 16;
		const unsigned RENDER_TARGETS = 4;

		int SamplerIndex( DWORD nSampler )
		{
			if ( nSampler < 16 )
			{
				return static_cast<int>( nSampler );
			}
			if ( nSampler >= D3DVERTEXTEXTURESAMPLER0 && nSampler <= D3DVERTEXTEXTURESAMPLER3 )
			{
				return static_cast<int>( 16 + nSampler - D3DVERTEXTEXTURESAMPLER0 );
			}
			return -1;
		}
	}


	class CDevice : public CDirect3DDevice9ExStub
	{
		IDirect3D9 *pD3D;
		D3DDEVICE_CREATION_PARAMETERS creation;
		D3DPRESENT_PARAMETERS pp;
		D3DCAPS9 caps;
		// References from the resources this made; see Resources.h.
		std::atomic<ULONG> nInternal{ 0 };

		// The swap chain's surfaces and the automatic depth buffer. The device
		// owns one reference on each and they hold none on it.
		std::vector<CSurface*> backBuffers;
		CSurface *pAutoDepth = nullptr;

		DWORD renderStates[256] = {};
		DWORD textureStageStates[TEXTURE_STAGES][D3DTSS_CONSTANT + 1] = {};
		DWORD samplerStates[SAMPLERS][D3DSAMP_DMAPOFFSET + 1] = {};
		std::map<D3DTRANSFORMSTATETYPE, D3DMATRIX> transforms;
		D3DVIEWPORT9 viewport = {};
		D3DMATERIAL9 material = {};
		std::map<DWORD, D3DLIGHT9> lights;
		std::map<DWORD, BOOL> lightsEnabled;
		float clipPlanes[6][4] = {};
		RECT scissor = {};
		D3DGAMMARAMP gamma = {};
		BOOL bSoftwareVP = FALSE;
		DWORD nFVF = 0;

		TBound<IDirect3DBaseTexture9> textures[SAMPLERS];
		TBound<IDirect3DSurface9> renderTargets[RENDER_TARGETS];
		TBound<IDirect3DSurface9> depthStencil;
		TBound<IDirect3DVertexBuffer9> streams[STREAMS];
		UINT streamOffsets[STREAMS] = {};
		UINT streamStrides[STREAMS] = {};
		UINT streamFrequencies[STREAMS] = {};
		TBound<IDirect3DIndexBuffer9> indices;
		TBound<IDirect3DVertexDeclaration9> declaration;
		TBound<IDirect3DVertexShader9> vertexShader;
		TBound<IDirect3DPixelShader9> pixelShader;

		std::vector<float> vertexConstantsF;
		std::vector<float> pixelConstantsF;
		int vertexConstantsI[16 * 4] = {};
		int pixelConstantsI[16 * 4] = {};
		BOOL vertexConstantsB[16] = {};
		BOOL pixelConstantsB[16] = {};

		// A windowed device with no size given takes its window's client area,
		// and an unknown format the desktop's, as D3D9 does -- and writes both
		// back into the caller's parameters, as D3D9 does.
		void CompletePresentParameters( D3DPRESENT_PARAMETERS *pParams )
		{
			if ( pParams->Windowed )
			{
				HWND hWindow = pParams->hDeviceWindow != nullptr ? pParams->hDeviceWindow : creation.hFocusWindow;
				RECT client = {};
				if ( hWindow != nullptr )
				{
					GetClientRect( hWindow, &client );
				}
				if ( pParams->BackBufferWidth == 0 )
				{
					pParams->BackBufferWidth = ( std::max )( 1L, client.right - client.left );
				}
				if ( pParams->BackBufferHeight == 0 )
				{
					pParams->BackBufferHeight = ( std::max )( 1L, client.bottom - client.top );
				}
				if ( pParams->BackBufferFormat == D3DFMT_UNKNOWN )
				{
					pParams->BackBufferFormat = DesktopMode().Format;
				}
			}
			if ( pParams->BackBufferCount == 0 )
			{
				pParams->BackBufferCount = 1;
			}
		}

		void ReleaseImplicitSurfaces()
		{
			for ( CSurface *pSurface : backBuffers )
			{
				pSurface->Release();
			}
			backBuffers.clear();
			if ( pAutoDepth != nullptr )
			{
				pAutoDepth->Release();
				pAutoDepth = nullptr;
			}
		}

		// The back buffers and depth buffer the parameters ask for, bound as
		// render target 0 and the depth buffer, with the viewport over them.
		void CreateImplicitSurfaces()
		{
			D3DSURFACE_DESC desc = {};
			desc.Format = pp.BackBufferFormat;
			desc.Type = D3DRTYPE_SURFACE;
			desc.Usage = D3DUSAGE_RENDERTARGET;
			desc.Pool = D3DPOOL_DEFAULT;
			desc.MultiSampleType = pp.MultiSampleType;
			desc.MultiSampleQuality = pp.MultiSampleQuality;
			desc.Width = pp.BackBufferWidth;
			desc.Height = pp.BackBufferHeight;
			for ( UINT n = 0; n < pp.BackBufferCount; ++n )
			{
				backBuffers.push_back( new CSurface( this, desc, nullptr, false ) );
			}
			if ( pp.EnableAutoDepthStencil )
			{
				desc.Format = pp.AutoDepthStencilFormat;
				desc.Usage = D3DUSAGE_DEPTHSTENCIL;
				pAutoDepth = new CSurface( this, desc, nullptr, false );
			}
			renderTargets[0].Set( backBuffers[0] );
			for ( unsigned n = 1; n < RENDER_TARGETS; ++n )
			{
				renderTargets[n].Set( nullptr );
			}
			depthStencil.Set( pAutoDepth );
			SetFullViewport( pp.BackBufferWidth, pp.BackBufferHeight );
			Note( "device surfaces: %u back buffer(s) %ux%u format %d, depth %s format %d", pp.BackBufferCount,
						pp.BackBufferWidth, pp.BackBufferHeight, pp.BackBufferFormat, pAutoDepth != nullptr ? "yes" : "no",
						pp.AutoDepthStencilFormat );
		}

		void SetFullViewport( DWORD nWidth, DWORD nHeight )
		{
			viewport.X = 0;
			viewport.Y = 0;
			viewport.Width = nWidth;
			viewport.Height = nHeight;
			viewport.MinZ = 0.0f;
			viewport.MaxZ = 1.0f;
			scissor.left = 0;
			scissor.top = 0;
			scissor.right = static_cast<LONG>( nWidth );
			scissor.bottom = static_cast<LONG>( nHeight );
		}

		void ReleaseBindings()
		{
			for ( auto &texture : textures )
			{
				texture.Set( nullptr );
			}
			for ( auto &target : renderTargets )
			{
				target.Set( nullptr );
			}
			depthStencil.Set( nullptr );
			for ( auto &stream : streams )
			{
				stream.Set( nullptr );
			}
			indices.Set( nullptr );
			declaration.Set( nullptr );
			vertexShader.Set( nullptr );
			pixelShader.Set( nullptr );
		}

		// Copies a range of shader constants in or out, refusing one that runs
		// past the end rather than writing there.
		template<class T>
		HRESULT Constants( T *pStore, size_t nStoreCount, UINT nStart, const T *pIn, T *pOut, UINT nCount )
		{
			if ( static_cast<size_t>( nStart ) + nCount > nStoreCount || ( pIn == nullptr && pOut == nullptr ) )
			{
				Note( "shader constants %u+%u past the %u there are", nStart, nCount, static_cast<unsigned>( nStoreCount ) );
				return D3DERR_INVALIDCALL;
			}
			if ( pIn != nullptr )
			{
				std::memcpy( pStore + nStart, pIn, nCount * sizeof( T ) );
			}
			else
			{
				std::memcpy( pOut, pStore + nStart, nCount * sizeof( T ) );
			}
			return D3D_OK;
		}

		HRESULT MakeSurface( UINT nWidth, UINT nHeight, D3DFORMAT eFormat, DWORD nUsage, D3DPOOL ePool,
												 D3DMULTISAMPLE_TYPE eMultiSample, DWORD nQuality, IDirect3DSurface9 **ppSurface )
		{
			if ( ppSurface == nullptr || nWidth == 0 || nHeight == 0 )
			{
				return D3DERR_INVALIDCALL;
			}
			D3DSURFACE_DESC desc = {};
			desc.Format = eFormat;
			desc.Type = D3DRTYPE_SURFACE;
			desc.Usage = nUsage;
			desc.Pool = ePool;
			desc.MultiSampleType = eMultiSample;
			desc.MultiSampleQuality = nQuality;
			desc.Width = nWidth;
			desc.Height = nHeight;
			*ppSurface = new CSurface( this, desc, nullptr, true );
			return D3D_OK;
		}

	public:
		CDevice( IDirect3D9 *_pD3D, D3DDEVTYPE eDeviceType, HWND hFocusWindow, DWORD nBehaviorFlags,
						 const D3DPRESENT_PARAMETERS &rParams )
			: pD3D( _pD3D ), pp( rParams )
		{
			pD3D->AddRef();
			creation.AdapterOrdinal = D3DADAPTER_DEFAULT;
			creation.DeviceType = eDeviceType;
			creation.hFocusWindow = hFocusWindow;
			creation.BehaviorFlags = nBehaviorFlags;
			FillCaps( &caps, eDeviceType );
			vertexConstantsF.resize( 4 * static_cast<size_t>( ( std::max )( caps.MaxVertexShaderConst, 256ul ) ) );
			pixelConstantsF.resize( 4 * 256 );
			for ( auto &frequency : streamFrequencies )
			{
				frequency = 1;
			}
			bSoftwareVP = ( nBehaviorFlags & D3DCREATE_SOFTWARE_VERTEXPROCESSING ) != 0;
			CompletePresentParameters( &pp );
			CreateImplicitSurfaces();
		}

		~CDevice() override
		{
			ReleaseBindings();
			ReleaseImplicitSurfaces();
			pD3D->Release();
		}

		const D3DPRESENT_PARAMETERS& PresentParameters() const
		{
			return pp;
		}

		// A device outlives its last public reference while anything it made
		// still exists; see Resources.h.
		STDMETHOD_(ULONG, AddRef)() override
		{
			return ++nRefs;
		}
		STDMETHOD_(ULONG, Release)() override
		{
			const ULONG nLeft = --nRefs;
			if ( nLeft == 0 )
			{
				// Held across the unbinding, which can take the last resource
				// with it and so the last internal reference.
				InternalAddRef();
				ReleaseBindings();
				InternalRelease();
			}
			return nLeft;
		}
		void InternalAddRef()
		{
			++nInternal;
		}
		void InternalRelease()
		{
			if ( --nInternal == 0 && nRefs == 0 )
			{
				delete this;
			}
		}

		//	Device, swap chain, presentation
		STDMETHOD(TestCooperativeLevel)() override
		{
			D3D9_TRACE0( "IDirect3DDevice9" );
			return D3D_OK;
		}
		// Plenty: Gfx.cpp turns hardware vertex processing off below 45 MB.
		STDMETHOD_(UINT, GetAvailableTextureMem)() override
		{
			D3D9_TRACE0( "IDirect3DDevice9" );
			return 1024u * 1024u * 1024u;
		}
		STDMETHOD(EvictManagedResources)() override
		{
			D3D9_TRACE0( "IDirect3DDevice9" );
			return D3D_OK;
		}
		STDMETHOD(GetDirect3D)( IDirect3D9 **ppD3D9 ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", ppD3D9 );
			if ( ppD3D9 == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			*ppD3D9 = pD3D;
			pD3D->AddRef();
			return D3D_OK;
		}
		STDMETHOD(GetDeviceCaps)( D3DCAPS9 *pCaps ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", pCaps );
			if ( pCaps == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			*pCaps = caps;
			return D3D_OK;
		}
		STDMETHOD(GetDisplayMode)( UINT iSwapChain, D3DDISPLAYMODE *pMode ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", iSwapChain, pMode );
			if ( pMode == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			*pMode = DesktopMode();
			return D3D_OK;
		}
		STDMETHOD(GetCreationParameters)( D3DDEVICE_CREATION_PARAMETERS *pParameters ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", pParameters );
			if ( pParameters == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			*pParameters = creation;
			return D3D_OK;
		}
		STDMETHOD_(UINT, GetNumberOfSwapChains)() override
		{
			D3D9_TRACE0( "IDirect3DDevice9" );
			return 1;
		}
		STDMETHOD(Reset)( D3DPRESENT_PARAMETERS *pPresentationParameters ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", pPresentationParameters );
			if ( pPresentationParameters == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			CompletePresentParameters( pPresentationParameters );
			pp = *pPresentationParameters;
			renderTargets[0].Set( nullptr );
			depthStencil.Set( nullptr );
			ReleaseImplicitSurfaces();
			CreateImplicitSurfaces();
			return D3D_OK;
		}
		STDMETHOD(Present)( const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride,
												const RGNDATA *pDirtyRegion ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion );
			return D3D_OK;
		}
		STDMETHOD(GetBackBuffer)( UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type,
															IDirect3DSurface9 **ppBackBuffer ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", iSwapChain, iBackBuffer, Type, ppBackBuffer );
			if ( ppBackBuffer == nullptr || iSwapChain != 0 || iBackBuffer >= backBuffers.size() )
			{
				return D3DERR_INVALIDCALL;
			}
			*ppBackBuffer = backBuffers[iBackBuffer];
			backBuffers[iBackBuffer]->AddRef();
			return D3D_OK;
		}
		STDMETHOD(GetRasterStatus)( UINT iSwapChain, D3DRASTER_STATUS *pRasterStatus ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", iSwapChain, pRasterStatus );
			if ( pRasterStatus == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			pRasterStatus->InVBlank = FALSE;
			pRasterStatus->ScanLine = 0;
			return D3D_OK;
		}
		STDMETHOD(SetDialogBoxMode)( BOOL bEnableDialogs ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", bEnableDialogs );
			return D3D_OK;
		}
		STDMETHOD_(void, SetGammaRamp)( UINT iSwapChain, DWORD Flags, const D3DGAMMARAMP *pRamp ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", iSwapChain, Flags, pRamp );
			if ( pRamp != nullptr )
			{
				gamma = *pRamp;
			}
		}
		STDMETHOD_(void, GetGammaRamp)( UINT iSwapChain, D3DGAMMARAMP *pRamp ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", iSwapChain, pRamp );
			if ( pRamp != nullptr )
			{
				*pRamp = gamma;
			}
		}

		//	Resource creation
		STDMETHOD(CreateTexture)( UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
															IDirect3DTexture9 **ppTexture, HANDLE *pSharedHandle ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", Width, Height, Levels, Usage, Format, Pool, ppTexture, pSharedHandle );
			if ( ppTexture == nullptr || Width == 0 || Height == 0 )
			{
				return D3DERR_INVALIDCALL;
			}
			// Levels 0 asks for the whole chain. An autogenerated one is a full
			// chain underneath that shows one level.
			const DWORD nLevels = ( Levels == 0 || ( Usage & D3DUSAGE_AUTOGENMIPMAP ) != 0 ) ? FullChain( Width, Height )
																																											: Levels;
			D3DSURFACE_DESC top = {};
			top.Format = Format;
			top.Type = D3DRTYPE_SURFACE;
			top.Usage = Usage;
			top.Pool = Pool;
			top.Width = Width;
			top.Height = Height;
			*ppTexture = new CTexture( this, top, Usage, nLevels );
			return D3D_OK;
		}
		STDMETHOD(CreateCubeTexture)( UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
																	IDirect3DCubeTexture9 **ppCubeTexture, HANDLE *pSharedHandle ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", EdgeLength, Levels, Usage, Format, Pool, ppCubeTexture, pSharedHandle );
			if ( ppCubeTexture == nullptr || EdgeLength == 0 )
			{
				return D3DERR_INVALIDCALL;
			}
			const DWORD nLevels = ( Levels == 0 || ( Usage & D3DUSAGE_AUTOGENMIPMAP ) != 0 )
															? FullChain( EdgeLength, EdgeLength ) : Levels;
			D3DSURFACE_DESC top = {};
			top.Format = Format;
			top.Type = D3DRTYPE_SURFACE;
			top.Usage = Usage;
			top.Pool = Pool;
			top.Width = EdgeLength;
			top.Height = EdgeLength;
			*ppCubeTexture = new CCubeTexture( this, top, Usage, nLevels );
			return D3D_OK;
		}
		STDMETHOD(CreateVertexBuffer)( UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool,
																	 IDirect3DVertexBuffer9 **ppVertexBuffer, HANDLE *pSharedHandle ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", Length, Usage, FVF, Pool, ppVertexBuffer, pSharedHandle );
			if ( ppVertexBuffer == nullptr || Length == 0 )
			{
				return D3DERR_INVALIDCALL;
			}
			D3DVERTEXBUFFER_DESC desc = {};
			desc.Format = D3DFMT_VERTEXDATA;
			desc.Type = D3DRTYPE_VERTEXBUFFER;
			desc.Usage = Usage;
			desc.Pool = Pool;
			desc.Size = Length;
			desc.FVF = FVF;
			*ppVertexBuffer = new CVertexBuffer( this, desc );
			return D3D_OK;
		}
		STDMETHOD(CreateIndexBuffer)( UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
																	IDirect3DIndexBuffer9 **ppIndexBuffer, HANDLE *pSharedHandle ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", Length, Usage, Format, Pool, ppIndexBuffer, pSharedHandle );
			if ( ppIndexBuffer == nullptr || Length == 0 )
			{
				return D3DERR_INVALIDCALL;
			}
			D3DINDEXBUFFER_DESC desc = {};
			desc.Format = Format;
			desc.Type = D3DRTYPE_INDEXBUFFER;
			desc.Usage = Usage;
			desc.Pool = Pool;
			desc.Size = Length;
			*ppIndexBuffer = new CIndexBuffer( this, desc );
			return D3D_OK;
		}
		STDMETHOD(CreateRenderTarget)( UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample,
																	 DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9 **ppSurface,
																	 HANDLE *pSharedHandle ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", Width, Height, Format, MultiSample, MultisampleQuality, Lockable, ppSurface,
									pSharedHandle );
			return MakeSurface( Width, Height, Format, D3DUSAGE_RENDERTARGET, D3DPOOL_DEFAULT, MultiSample,
													MultisampleQuality, ppSurface );
		}
		STDMETHOD(CreateDepthStencilSurface)( UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample,
																					DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9 **ppSurface,
																					HANDLE *pSharedHandle ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", Width, Height, Format, MultiSample, MultisampleQuality, Discard, ppSurface,
									pSharedHandle );
			return MakeSurface( Width, Height, Format, D3DUSAGE_DEPTHSTENCIL, D3DPOOL_DEFAULT, MultiSample,
													MultisampleQuality, ppSurface );
		}
		STDMETHOD(CreateOffscreenPlainSurface)( UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool,
																						IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", Width, Height, Format, Pool, ppSurface, pSharedHandle );
			return MakeSurface( Width, Height, Format, 0, Pool, D3DMULTISAMPLE_NONE, 0, ppSurface );
		}
		STDMETHOD(CreateVertexDeclaration)( const D3DVERTEXELEMENT9 *pVertexElements,
																				IDirect3DVertexDeclaration9 **ppDecl ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", pVertexElements, ppDecl );
			if ( pVertexElements == nullptr || ppDecl == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			*ppDecl = new CVertexDeclaration( this, pVertexElements );
			return D3D_OK;
		}
		STDMETHOD(CreateVertexShader)( const DWORD *pFunction, IDirect3DVertexShader9 **ppShader ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", pFunction, ppShader );
			if ( pFunction == nullptr || ppShader == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			*ppShader = new CVertexShader( this, pFunction );
			return D3D_OK;
		}
		STDMETHOD(CreatePixelShader)( const DWORD *pFunction, IDirect3DPixelShader9 **ppShader ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", pFunction, ppShader );
			if ( pFunction == nullptr || ppShader == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			*ppShader = new CPixelShader( this, pFunction );
			return D3D_OK;
		}
		// A null out pointer asks whether the type is supported. The vertex
		// cache query is not, as on most cards; Gfx.cpp waits in a loop for one
		// that is to report, and then reads a pattern out of it.
		STDMETHOD(CreateQuery)( D3DQUERYTYPE Type, IDirect3DQuery9 **ppQuery ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", Type, ppQuery );
			if ( Type == D3DQUERYTYPE_VCACHE )
			{
				return D3DERR_NOTAVAILABLE;
			}
			if ( ppQuery == nullptr )
			{
				return D3D_OK;
			}
			*ppQuery = new CQuery( this, Type );
			return D3D_OK;
		}
		STDMETHOD(CreateStateBlock)( D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9 **ppSB ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", Type, ppSB );
			if ( ppSB == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			*ppSB = new CStateBlock( this );
			return D3D_OK;
		}
		STDMETHOD(BeginStateBlock)() override
		{
			D3D9_TRACE0( "IDirect3DDevice9" );
			return D3D_OK;
		}
		STDMETHOD(EndStateBlock)( IDirect3DStateBlock9 **ppSB ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", ppSB );
			if ( ppSB == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			*ppSB = new CStateBlock( this );
			return D3D_OK;
		}

		//	Copies between surfaces: accepted, and nothing moves, since nothing
		//	was drawn to move.
		STDMETHOD(UpdateSurface)( IDirect3DSurface9 *pSourceSurface, const RECT *pSourceRect,
															IDirect3DSurface9 *pDestinationSurface, const POINT *pDestPoint ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", pSourceSurface, pSourceRect, pDestinationSurface, pDestPoint );
			return D3D_OK;
		}
		STDMETHOD(UpdateTexture)( IDirect3DBaseTexture9 *pSourceTexture, IDirect3DBaseTexture9 *pDestinationTexture ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", pSourceTexture, pDestinationTexture );
			return D3D_OK;
		}
		STDMETHOD(GetRenderTargetData)( IDirect3DSurface9 *pRenderTarget, IDirect3DSurface9 *pDestSurface ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", pRenderTarget, pDestSurface );
			return D3D_OK;
		}
		STDMETHOD(GetFrontBufferData)( UINT iSwapChain, IDirect3DSurface9 *pDestSurface ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", iSwapChain, pDestSurface );
			return D3D_OK;
		}
		STDMETHOD(StretchRect)( IDirect3DSurface9 *pSourceSurface, const RECT *pSourceRect, IDirect3DSurface9 *pDestSurface,
														const RECT *pDestRect, D3DTEXTUREFILTERTYPE Filter ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", pSourceSurface, pSourceRect, pDestSurface, pDestRect, Filter );
			return D3D_OK;
		}
		STDMETHOD(ColorFill)( IDirect3DSurface9 *pSurface, const RECT *pRect, D3DCOLOR color ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", pSurface, pRect, color );
			return D3D_OK;
		}

		//	Render targets
		STDMETHOD(SetRenderTarget)( DWORD RenderTargetIndex, IDirect3DSurface9 *pRenderTarget ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", RenderTargetIndex, pRenderTarget );
			if ( RenderTargetIndex >= RENDER_TARGETS || ( RenderTargetIndex == 0 && pRenderTarget == nullptr ) )
			{
				return D3DERR_INVALIDCALL;
			}
			renderTargets[RenderTargetIndex].Set( pRenderTarget );
			// Setting target 0 resets the viewport to cover it, in D3D9.
			if ( RenderTargetIndex == 0 )
			{
				D3DSURFACE_DESC desc = {};
				pRenderTarget->GetDesc( &desc );
				SetFullViewport( desc.Width, desc.Height );
			}
			return D3D_OK;
		}
		STDMETHOD(GetRenderTarget)( DWORD RenderTargetIndex, IDirect3DSurface9 **ppRenderTarget ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", RenderTargetIndex, ppRenderTarget );
			if ( RenderTargetIndex >= RENDER_TARGETS || ppRenderTarget == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			renderTargets[RenderTargetIndex].CopyTo( ppRenderTarget );
			return *ppRenderTarget != nullptr ? D3D_OK : D3DERR_NOTFOUND;
		}
		STDMETHOD(SetDepthStencilSurface)( IDirect3DSurface9 *pNewZStencil ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", pNewZStencil );
			depthStencil.Set( pNewZStencil );
			return D3D_OK;
		}
		STDMETHOD(GetDepthStencilSurface)( IDirect3DSurface9 **ppZStencilSurface ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", ppZStencilSurface );
			if ( ppZStencilSurface == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			depthStencil.CopyTo( ppZStencilSurface );
			return *ppZStencilSurface != nullptr ? D3D_OK : D3DERR_NOTFOUND;
		}

		//	Drawing: all of it succeeds and none of it happens.
		STDMETHOD(BeginScene)() override
		{
			D3D9_TRACE0( "IDirect3DDevice9" );
			return D3D_OK;
		}
		STDMETHOD(EndScene)() override
		{
			D3D9_TRACE0( "IDirect3DDevice9" );
			return D3D_OK;
		}
		STDMETHOD(Clear)( DWORD Count, const D3DRECT *pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", Count, pRects, Flags, Color, Z, Stencil );
			return D3D_OK;
		}
		STDMETHOD(DrawPrimitive)( D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", PrimitiveType, StartVertex, PrimitiveCount );
			return D3D_OK;
		}
		STDMETHOD(DrawIndexedPrimitive)( D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinVertexIndex,
																		 UINT NumVertices, UINT startIndex, UINT primCount ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", Type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount );
			return D3D_OK;
		}
		STDMETHOD(DrawPrimitiveUP)( D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void *pVertexStreamZeroData,
																UINT VertexStreamZeroStride ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride );
			// The UP draws unbind stream 0 and the indices, in D3D9.
			streams[0].Set( nullptr );
			return D3D_OK;
		}
		STDMETHOD(DrawIndexedPrimitiveUP)( D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices,
																			 UINT PrimitiveCount, const void *pIndexData, D3DFORMAT IndexDataFormat,
																			 const void *pVertexStreamZeroData, UINT VertexStreamZeroStride ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount, pIndexData,
									IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride );
			streams[0].Set( nullptr );
			indices.Set( nullptr );
			return D3D_OK;
		}
		STDMETHOD(ValidateDevice)( DWORD *pNumPasses ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", pNumPasses );
			if ( pNumPasses != nullptr )
			{
				*pNumPasses = 1;
			}
			return D3D_OK;
		}

		//	Fixed-function state
		STDMETHOD(SetTransform)( D3DTRANSFORMSTATETYPE State, const D3DMATRIX *pMatrix ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", State, pMatrix );
			if ( pMatrix == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			transforms[State] = *pMatrix;
			return D3D_OK;
		}
		STDMETHOD(GetTransform)( D3DTRANSFORMSTATETYPE State, D3DMATRIX *pMatrix ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", State, pMatrix );
			if ( pMatrix == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			const auto it = transforms.find( State );
			if ( it != transforms.end() )
			{
				*pMatrix = it->second;
			}
			else
			{
				*pMatrix = {};
				pMatrix->_11 = pMatrix->_22 = pMatrix->_33 = pMatrix->_44 = 1.0f;
			}
			return D3D_OK;
		}
		STDMETHOD(SetViewport)( const D3DVIEWPORT9 *pViewport ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", pViewport );
			if ( pViewport == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			viewport = *pViewport;
			return D3D_OK;
		}
		STDMETHOD(GetViewport)( D3DVIEWPORT9 *pViewport ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", pViewport );
			if ( pViewport == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			*pViewport = viewport;
			return D3D_OK;
		}
		STDMETHOD(SetMaterial)( const D3DMATERIAL9 *pMaterial ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", pMaterial );
			if ( pMaterial == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			material = *pMaterial;
			return D3D_OK;
		}
		STDMETHOD(GetMaterial)( D3DMATERIAL9 *pMaterial ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", pMaterial );
			if ( pMaterial == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			*pMaterial = material;
			return D3D_OK;
		}
		STDMETHOD(SetLight)( DWORD Index, const D3DLIGHT9 *arg1 ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", Index, arg1 );
			if ( arg1 == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			lights[Index] = *arg1;
			return D3D_OK;
		}
		STDMETHOD(GetLight)( DWORD Index, D3DLIGHT9 *arg1 ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", Index, arg1 );
			const auto it = lights.find( Index );
			if ( arg1 == nullptr || it == lights.end() )
			{
				return D3DERR_INVALIDCALL;
			}
			*arg1 = it->second;
			return D3D_OK;
		}
		STDMETHOD(LightEnable)( DWORD Index, BOOL Enable ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", Index, Enable );
			lightsEnabled[Index] = Enable;
			return D3D_OK;
		}
		STDMETHOD(GetLightEnable)( DWORD Index, BOOL *pEnable ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", Index, pEnable );
			const auto it = lightsEnabled.find( Index );
			if ( pEnable == nullptr || it == lightsEnabled.end() )
			{
				return D3DERR_INVALIDCALL;
			}
			*pEnable = it->second;
			return D3D_OK;
		}
		STDMETHOD(SetClipPlane)( DWORD Index, const float *pPlane ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", Index, pPlane );
			if ( Index >= 6 || pPlane == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			std::memcpy( clipPlanes[Index], pPlane, sizeof( clipPlanes[Index] ) );
			return D3D_OK;
		}
		STDMETHOD(GetClipPlane)( DWORD Index, float *pPlane ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", Index, pPlane );
			if ( Index >= 6 || pPlane == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			std::memcpy( pPlane, clipPlanes[Index], sizeof( clipPlanes[Index] ) );
			return D3D_OK;
		}
		STDMETHOD(SetRenderState)( D3DRENDERSTATETYPE State, DWORD Value ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", State, Value );
			if ( static_cast<unsigned>( State ) >= 256 )
			{
				return D3DERR_INVALIDCALL;
			}
			renderStates[State] = Value;
			return D3D_OK;
		}
		STDMETHOD(GetRenderState)( D3DRENDERSTATETYPE State, DWORD *pValue ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", State, pValue );
			if ( static_cast<unsigned>( State ) >= 256 || pValue == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			*pValue = renderStates[State];
			return D3D_OK;
		}
		STDMETHOD(SetScissorRect)( const RECT *pRect ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", pRect );
			if ( pRect == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			scissor = *pRect;
			return D3D_OK;
		}
		STDMETHOD(GetScissorRect)( RECT *pRect ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", pRect );
			if ( pRect == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			*pRect = scissor;
			return D3D_OK;
		}
		STDMETHOD(SetSoftwareVertexProcessing)( BOOL bSoftware ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", bSoftware );
			bSoftwareVP = bSoftware;
			return D3D_OK;
		}
		STDMETHOD_(BOOL, GetSoftwareVertexProcessing)() override
		{
			D3D9_TRACE0( "IDirect3DDevice9" );
			return bSoftwareVP;
		}

		//	Textures and samplers
		STDMETHOD(SetTexture)( DWORD Stage, IDirect3DBaseTexture9 *pTexture ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", Stage, pTexture );
			const int nIndex = SamplerIndex( Stage );
			if ( nIndex < 0 )
			{
				return D3DERR_INVALIDCALL;
			}
			textures[nIndex].Set( pTexture );
			return D3D_OK;
		}
		STDMETHOD(GetTexture)( DWORD Stage, IDirect3DBaseTexture9 **ppTexture ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", Stage, ppTexture );
			const int nIndex = SamplerIndex( Stage );
			if ( nIndex < 0 || ppTexture == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			textures[nIndex].CopyTo( ppTexture );
			return D3D_OK;
		}
		STDMETHOD(SetTextureStageState)( DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", Stage, Type, Value );
			if ( Stage >= TEXTURE_STAGES || static_cast<unsigned>( Type ) > D3DTSS_CONSTANT )
			{
				return D3DERR_INVALIDCALL;
			}
			textureStageStates[Stage][Type] = Value;
			return D3D_OK;
		}
		STDMETHOD(GetTextureStageState)( DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD *pValue ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", Stage, Type, pValue );
			if ( Stage >= TEXTURE_STAGES || static_cast<unsigned>( Type ) > D3DTSS_CONSTANT || pValue == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			*pValue = textureStageStates[Stage][Type];
			return D3D_OK;
		}
		STDMETHOD(SetSamplerState)( DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", Sampler, Type, Value );
			const int nIndex = SamplerIndex( Sampler );
			if ( nIndex < 0 || static_cast<unsigned>( Type ) > D3DSAMP_DMAPOFFSET )
			{
				return D3DERR_INVALIDCALL;
			}
			samplerStates[nIndex][Type] = Value;
			return D3D_OK;
		}
		STDMETHOD(GetSamplerState)( DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD *pValue ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", Sampler, Type, pValue );
			const int nIndex = SamplerIndex( Sampler );
			if ( nIndex < 0 || static_cast<unsigned>( Type ) > D3DSAMP_DMAPOFFSET || pValue == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			*pValue = samplerStates[nIndex][Type];
			return D3D_OK;
		}

		//	Vertex input
		STDMETHOD(SetVertexDeclaration)( IDirect3DVertexDeclaration9 *pDecl ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", pDecl );
			declaration.Set( pDecl );
			return D3D_OK;
		}
		STDMETHOD(GetVertexDeclaration)( IDirect3DVertexDeclaration9 **ppDecl ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", ppDecl );
			if ( ppDecl == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			declaration.CopyTo( ppDecl );
			return D3D_OK;
		}
		STDMETHOD(SetFVF)( DWORD FVF ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", FVF );
			nFVF = FVF;
			return D3D_OK;
		}
		STDMETHOD(GetFVF)( DWORD *pFVF ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", pFVF );
			if ( pFVF == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			*pFVF = nFVF;
			return D3D_OK;
		}
		STDMETHOD(SetStreamSource)( UINT StreamNumber, IDirect3DVertexBuffer9 *pStreamData, UINT OffsetInBytes,
																UINT Stride ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", StreamNumber, pStreamData, OffsetInBytes, Stride );
			if ( StreamNumber >= STREAMS )
			{
				return D3DERR_INVALIDCALL;
			}
			streams[StreamNumber].Set( pStreamData );
			streamOffsets[StreamNumber] = OffsetInBytes;
			streamStrides[StreamNumber] = Stride;
			return D3D_OK;
		}
		STDMETHOD(GetStreamSource)( UINT StreamNumber, IDirect3DVertexBuffer9 **ppStreamData, UINT *pOffsetInBytes,
																UINT *pStride ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", StreamNumber, ppStreamData, pOffsetInBytes, pStride );
			if ( StreamNumber >= STREAMS || ppStreamData == nullptr || pOffsetInBytes == nullptr || pStride == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			streams[StreamNumber].CopyTo( ppStreamData );
			*pOffsetInBytes = streamOffsets[StreamNumber];
			*pStride = streamStrides[StreamNumber];
			return D3D_OK;
		}
		STDMETHOD(SetStreamSourceFreq)( UINT StreamNumber, UINT Setting ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", StreamNumber, Setting );
			if ( StreamNumber >= STREAMS )
			{
				return D3DERR_INVALIDCALL;
			}
			streamFrequencies[StreamNumber] = Setting;
			return D3D_OK;
		}
		STDMETHOD(GetStreamSourceFreq)( UINT StreamNumber, UINT *pSetting ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", StreamNumber, pSetting );
			if ( StreamNumber >= STREAMS || pSetting == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			*pSetting = streamFrequencies[StreamNumber];
			return D3D_OK;
		}
		STDMETHOD(SetIndices)( IDirect3DIndexBuffer9 *pIndexData ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", pIndexData );
			indices.Set( pIndexData );
			return D3D_OK;
		}
		STDMETHOD(GetIndices)( IDirect3DIndexBuffer9 **ppIndexData ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", ppIndexData );
			if ( ppIndexData == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			indices.CopyTo( ppIndexData );
			return D3D_OK;
		}

		//	Shaders and their constants
		STDMETHOD(SetVertexShader)( IDirect3DVertexShader9 *pShader ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", pShader );
			vertexShader.Set( pShader );
			return D3D_OK;
		}
		STDMETHOD(GetVertexShader)( IDirect3DVertexShader9 **ppShader ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", ppShader );
			if ( ppShader == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			vertexShader.CopyTo( ppShader );
			return D3D_OK;
		}
		STDMETHOD(SetPixelShader)( IDirect3DPixelShader9 *pShader ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", pShader );
			pixelShader.Set( pShader );
			return D3D_OK;
		}
		STDMETHOD(GetPixelShader)( IDirect3DPixelShader9 **ppShader ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", ppShader );
			if ( ppShader == nullptr )
			{
				return D3DERR_INVALIDCALL;
			}
			pixelShader.CopyTo( ppShader );
			return D3D_OK;
		}
		STDMETHOD(SetVertexShaderConstantF)( UINT StartRegister, const float *pConstantData, UINT Vector4fCount ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", StartRegister, pConstantData, Vector4fCount );
			return Constants<float>( vertexConstantsF.data(), vertexConstantsF.size(), StartRegister * 4, pConstantData,
															 nullptr, Vector4fCount * 4 );
		}
		STDMETHOD(GetVertexShaderConstantF)( UINT StartRegister, float *pConstantData, UINT Vector4fCount ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", StartRegister, pConstantData, Vector4fCount );
			return Constants<float>( vertexConstantsF.data(), vertexConstantsF.size(), StartRegister * 4, nullptr,
															 pConstantData, Vector4fCount * 4 );
		}
		STDMETHOD(SetVertexShaderConstantI)( UINT StartRegister, const int *pConstantData, UINT Vector4iCount ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", StartRegister, pConstantData, Vector4iCount );
			return Constants<int>( vertexConstantsI, 16 * 4, StartRegister * 4, pConstantData, nullptr, Vector4iCount * 4 );
		}
		STDMETHOD(GetVertexShaderConstantI)( UINT StartRegister, int *pConstantData, UINT Vector4iCount ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", StartRegister, pConstantData, Vector4iCount );
			return Constants<int>( vertexConstantsI, 16 * 4, StartRegister * 4, nullptr, pConstantData, Vector4iCount * 4 );
		}
		STDMETHOD(SetVertexShaderConstantB)( UINT StartRegister, const BOOL *pConstantData, UINT BoolCount ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", StartRegister, pConstantData, BoolCount );
			return Constants<BOOL>( vertexConstantsB, 16, StartRegister, pConstantData, nullptr, BoolCount );
		}
		STDMETHOD(GetVertexShaderConstantB)( UINT StartRegister, BOOL *pConstantData, UINT BoolCount ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", StartRegister, pConstantData, BoolCount );
			return Constants<BOOL>( vertexConstantsB, 16, StartRegister, nullptr, pConstantData, BoolCount );
		}
		STDMETHOD(SetPixelShaderConstantF)( UINT StartRegister, const float *pConstantData, UINT Vector4fCount ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", StartRegister, pConstantData, Vector4fCount );
			return Constants<float>( pixelConstantsF.data(), pixelConstantsF.size(), StartRegister * 4, pConstantData,
															 nullptr, Vector4fCount * 4 );
		}
		STDMETHOD(GetPixelShaderConstantF)( UINT StartRegister, float *pConstantData, UINT Vector4fCount ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", StartRegister, pConstantData, Vector4fCount );
			return Constants<float>( pixelConstantsF.data(), pixelConstantsF.size(), StartRegister * 4, nullptr,
															 pConstantData, Vector4fCount * 4 );
		}
		STDMETHOD(SetPixelShaderConstantI)( UINT StartRegister, const int *pConstantData, UINT Vector4iCount ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", StartRegister, pConstantData, Vector4iCount );
			return Constants<int>( pixelConstantsI, 16 * 4, StartRegister * 4, pConstantData, nullptr, Vector4iCount * 4 );
		}
		STDMETHOD(GetPixelShaderConstantI)( UINT StartRegister, int *pConstantData, UINT Vector4iCount ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", StartRegister, pConstantData, Vector4iCount );
			return Constants<int>( pixelConstantsI, 16 * 4, StartRegister * 4, nullptr, pConstantData, Vector4iCount * 4 );
		}
		STDMETHOD(SetPixelShaderConstantB)( UINT StartRegister, const BOOL *pConstantData, UINT BoolCount ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", StartRegister, pConstantData, BoolCount );
			return Constants<BOOL>( pixelConstantsB, 16, StartRegister, pConstantData, nullptr, BoolCount );
		}
		STDMETHOD(GetPixelShaderConstantB)( UINT StartRegister, BOOL *pConstantData, UINT BoolCount ) override
		{
			D3D9_TRACE( "IDirect3DDevice9", StartRegister, pConstantData, BoolCount );
			return Constants<BOOL>( pixelConstantsB, 16, StartRegister, nullptr, pConstantData, BoolCount );
		}
	};


	void DeviceInternalAddRef( CDevice *pDevice )
	{
		pDevice->InternalAddRef();
	}


	void DeviceInternalRelease( CDevice *pDevice )
	{
		pDevice->InternalRelease();
	}


	IDirect3DDevice9* DeviceInterface( CDevice *pDevice )
	{
		return pDevice;
	}


	HRESULT CreateDevice( IDirect3D9 *pD3D, D3DDEVTYPE eDeviceType, HWND hFocusWindow, DWORD nBehaviorFlags,
												D3DPRESENT_PARAMETERS *pPresentationParameters, IDirect3DDevice9 **ppDevice )
	{
		if ( pPresentationParameters == nullptr || ppDevice == nullptr )
		{
			return D3DERR_INVALIDCALL;
		}
		CDevice *pDevice = new CDevice( pD3D, eDeviceType, hFocusWindow, nBehaviorFlags, *pPresentationParameters );
		// Written back, as D3D9 writes back what it filled in.
		*pPresentationParameters = pDevice->PresentParameters();
		*ppDevice = pDevice;
		return D3D_OK;
	}
}
