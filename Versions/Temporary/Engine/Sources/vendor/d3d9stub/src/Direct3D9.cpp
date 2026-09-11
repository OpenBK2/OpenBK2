#include "Device.h"
#include "Formats.h"
#include "Stubs.h"

#include <cstring>
#include <vector>

// The adapter, and the DLL's three exports.
//
// One adapter, the primary display, reporting the captured card's caps and the
// real desktop mode, and accepting any device type -- HAL for the viewport,
// NULLREF for Image's DDS export -- because nothing about the device depends
// on which. It says what it is in its identifier, so a log that prints the
// adapter description shows the stub was in use.

namespace ND3D9Stub
{
	void FillCaps( D3DCAPS9 *pCaps, D3DDEVTYPE eDeviceType )
	{
		D3DCAPS9 &caps = *pCaps;
		std::memset( &caps, 0, sizeof( caps ) );
#include "Caps.inc"
		caps.DeviceType = eDeviceType;
		caps.AdapterOrdinal = D3DADAPTER_DEFAULT;
	}


	D3DDISPLAYMODE DesktopMode()
	{
		DEVMODEA devMode = {};
		devMode.dmSize = sizeof( devMode );
		D3DDISPLAYMODE mode = {};
		if ( EnumDisplaySettingsA( nullptr, ENUM_CURRENT_SETTINGS, &devMode ) )
		{
			mode.Width = devMode.dmPelsWidth;
			mode.Height = devMode.dmPelsHeight;
			mode.RefreshRate = devMode.dmDisplayFrequency;
		}
		else
		{
			mode.Width = 1920;
			mode.Height = 1080;
			mode.RefreshRate = 60;
		}
		mode.Format = D3DFMT_X8R8G8B8;
		return mode;
	}


	bool IsDepthFormat( D3DFORMAT eFormat )
	{
		switch ( eFormat )
		{
		case D3DFMT_D16_LOCKABLE: case D3DFMT_D32: case D3DFMT_D15S1: case D3DFMT_D24S8: case D3DFMT_D24X8:
		case D3DFMT_D24X4S4: case D3DFMT_D16: case D3DFMT_D32F_LOCKABLE: case D3DFMT_D24FS8: case D3DFMT_D32_LOCKABLE:
			return true;
		default:
			return false;
		}
	}


	namespace
	{
		// The modes offered for a format: the common ones that fit the desktop,
		// and the desktop's own.
		std::vector<D3DDISPLAYMODE> Modes( D3DFORMAT eFormat )
		{
			std::vector<D3DDISPLAYMODE> modes;
			if ( eFormat != D3DFMT_X8R8G8B8 && eFormat != D3DFMT_R5G6B5 )
			{
				return modes;
			}
			const D3DDISPLAYMODE desktop = DesktopMode();
			const UINT sizes[][2] = { { 640, 480 }, { 800, 600 }, { 1024, 768 }, { 1280, 720 }, { 1280, 1024 },
																{ 1600, 900 }, { 1920, 1080 } };
			for ( const auto &size : sizes )
			{
				if ( size[0] <= desktop.Width && size[1] <= desktop.Height &&
						 !( size[0] == desktop.Width && size[1] == desktop.Height ) )
				{
					modes.push_back( { size[0], size[1], 60, eFormat } );
				}
			}
			modes.push_back( { desktop.Width, desktop.Height, desktop.RefreshRate, eFormat } );
			return modes;
		}


		class CDirect3D9 : public CDirect3D9ExStub
		{
		public:
			STDMETHOD_(UINT, GetAdapterCount)() override
			{
				D3D9_TRACE0( "IDirect3D9" );
				return 1;
			}
			STDMETHOD(GetAdapterIdentifier)( UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9 *pIdentifier ) override
			{
				D3D9_TRACE( "IDirect3D9", Adapter, Flags, pIdentifier );
				if ( Adapter != D3DADAPTER_DEFAULT || pIdentifier == nullptr )
				{
					return D3DERR_INVALIDCALL;
				}
				std::memset( pIdentifier, 0, sizeof( *pIdentifier ) );
				strcpy_s( pIdentifier->Driver, "d3d9.dll (OpenBK2 stub)" );
				strcpy_s( pIdentifier->Description, "OpenBK2 D3D9 stub -- nothing is rendered" );
				strcpy_s( pIdentifier->DeviceName, "\\\\.\\DISPLAY1" );
				// Vendor 0: neither NVIDIA's nor ATI's, so Gfx.cpp takes neither
				// of its vendor workarounds.
				return D3D_OK;
			}
			STDMETHOD_(UINT, GetAdapterModeCount)( UINT Adapter, D3DFORMAT Format ) override
			{
				D3D9_TRACE( "IDirect3D9", Adapter, Format );
				return Adapter == D3DADAPTER_DEFAULT ? static_cast<UINT>( Modes( Format ).size() ) : 0;
			}
			STDMETHOD(EnumAdapterModes)( UINT Adapter, D3DFORMAT Format, UINT Mode, D3DDISPLAYMODE *pMode ) override
			{
				D3D9_TRACE( "IDirect3D9", Adapter, Format, Mode, pMode );
				const std::vector<D3DDISPLAYMODE> modes = Modes( Format );
				if ( Adapter != D3DADAPTER_DEFAULT || pMode == nullptr || Mode >= modes.size() )
				{
					return D3DERR_INVALIDCALL;
				}
				*pMode = modes[Mode];
				return D3D_OK;
			}
			STDMETHOD(GetAdapterDisplayMode)( UINT Adapter, D3DDISPLAYMODE *pMode ) override
			{
				D3D9_TRACE( "IDirect3D9", Adapter, pMode );
				if ( Adapter != D3DADAPTER_DEFAULT || pMode == nullptr )
				{
					return D3DERR_INVALIDCALL;
				}
				*pMode = DesktopMode();
				return D3D_OK;
			}
			STDMETHOD(CheckDeviceType)( UINT Adapter, D3DDEVTYPE DevType, D3DFORMAT AdapterFormat,
																	D3DFORMAT BackBufferFormat, BOOL bWindowed ) override
			{
				D3D9_TRACE( "IDirect3D9", Adapter, DevType, AdapterFormat, BackBufferFormat, bWindowed );
				return Adapter == D3DADAPTER_DEFAULT ? D3D_OK : D3DERR_INVALIDCALL;
			}
			// Yes to every format there is a layout for, as a depth buffer only
			// for the depth formats, and no to the rest: an unknown FOURCC is a
			// vendor extension, and saying yes would send the engine down a
			// vendor's path.
			STDMETHOD(CheckDeviceFormat)( UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage,
																		D3DRESOURCETYPE RType, D3DFORMAT CheckFormat ) override
			{
				D3D9_TRACE( "IDirect3D9", Adapter, DeviceType, AdapterFormat, Usage, RType, CheckFormat );
				if ( Adapter != D3DADAPTER_DEFAULT )
				{
					return D3DERR_INVALIDCALL;
				}
				const bool bDepth = IsDepthFormat( CheckFormat );
				if ( ( Usage & D3DUSAGE_DEPTHSTENCIL ) != 0 )
				{
					return bDepth ? D3D_OK : D3DERR_NOTAVAILABLE;
				}
				if ( bDepth && ( Usage & D3DUSAGE_RENDERTARGET ) != 0 )
				{
					return D3DERR_NOTAVAILABLE;
				}
				bool bKnown = true;
				BitsPerPixel( CheckFormat, &bKnown );
				return ( bKnown || IsBlockCompressed( CheckFormat ) ) ? D3D_OK : D3DERR_NOTAVAILABLE;
			}
			// No multisampling: one quality level of none, nothing else.
			STDMETHOD(CheckDeviceMultiSampleType)( UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat,
																						 BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType,
																						 DWORD *pQualityLevels ) override
			{
				D3D9_TRACE( "IDirect3D9", Adapter, DeviceType, SurfaceFormat, Windowed, MultiSampleType, pQualityLevels );
				if ( MultiSampleType != D3DMULTISAMPLE_NONE )
				{
					return D3DERR_NOTAVAILABLE;
				}
				if ( pQualityLevels != nullptr )
				{
					*pQualityLevels = 1;
				}
				return D3D_OK;
			}
			STDMETHOD(CheckDepthStencilMatch)( UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat,
																				 D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat ) override
			{
				D3D9_TRACE( "IDirect3D9", Adapter, DeviceType, AdapterFormat, RenderTargetFormat, DepthStencilFormat );
				return IsDepthFormat( DepthStencilFormat ) ? D3D_OK : D3DERR_NOTAVAILABLE;
			}
			STDMETHOD(CheckDeviceFormatConversion)( UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SourceFormat,
																							D3DFORMAT TargetFormat ) override
			{
				D3D9_TRACE( "IDirect3D9", Adapter, DeviceType, SourceFormat, TargetFormat );
				return D3D_OK;
			}
			STDMETHOD(GetDeviceCaps)( UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS9 *pCaps ) override
			{
				D3D9_TRACE( "IDirect3D9", Adapter, DeviceType, pCaps );
				if ( Adapter != D3DADAPTER_DEFAULT || pCaps == nullptr )
				{
					return D3DERR_INVALIDCALL;
				}
				FillCaps( pCaps, DeviceType );
				return D3D_OK;
			}
			STDMETHOD_(HMONITOR, GetAdapterMonitor)( UINT Adapter ) override
			{
				D3D9_TRACE( "IDirect3D9", Adapter );
				return MonitorFromPoint( POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY );
			}
			STDMETHOD(CreateDevice)( UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags,
															 D3DPRESENT_PARAMETERS *pPresentationParameters,
															 IDirect3DDevice9 **ppReturnedDeviceInterface ) override
			{
				D3D9_TRACE( "IDirect3D9", Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters,
										ppReturnedDeviceInterface );
				if ( Adapter != D3DADAPTER_DEFAULT )
				{
					return D3DERR_INVALIDCALL;
				}
				if ( pPresentationParameters != nullptr )
				{
					const D3DPRESENT_PARAMETERS &rpp = *pPresentationParameters;
					Note( "CreateDevice: type %d, flags 0x%lX, %ux%u format %d x%u, windowed %d, depth %d format %d, "
								"swap %d, interval 0x%X", DeviceType, BehaviorFlags, rpp.BackBufferWidth, rpp.BackBufferHeight,
								rpp.BackBufferFormat, rpp.BackBufferCount, rpp.Windowed, rpp.EnableAutoDepthStencil,
								rpp.AutoDepthStencilFormat, rpp.SwapEffect, rpp.PresentationInterval );
				}
				return ND3D9Stub::CreateDevice( this, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters,
																				ppReturnedDeviceInterface );
			}
		};
	}
}


// The DLL's exports; d3d9.def gives them their undecorated names on x86.

extern "C" IDirect3D9* WINAPI Direct3DCreate9( UINT SDKVersion )
{
	::ND3D9Stub::Note( "Direct3DCreate9( SDKVersion=%u )", SDKVersion );
	return new ::ND3D9Stub::CDirect3D9();
}


// Not called by this engine, which imports Direct3DCreate9 alone, but part of
// what a d3d9.dll is. The object is the same one: it implements IDirect3D9Ex,
// and its Ex methods are stubs until something reaches them.
extern "C" HRESULT WINAPI Direct3DCreate9Ex( UINT SDKVersion, IDirect3D9Ex **ppD3D )
{
	::ND3D9Stub::Note( "Direct3DCreate9Ex( SDKVersion=%u )", SDKVersion );
	if ( ppD3D == nullptr )
	{
		return D3DERR_INVALIDCALL;
	}
	*ppD3D = new ::ND3D9Stub::CDirect3D9();
	return S_OK;
}


// d3dx9_43.dll looks this up in whatever d3d9.dll is loaded, to quieten the
// debug runtime. There is nothing to quieten.
extern "C" void WINAPI DebugSetMute()
{
}


BOOL WINAPI DllMain( HINSTANCE, DWORD nReason, LPVOID )
{
	if ( nReason == DLL_PROCESS_DETACH )
	{
		::ND3D9Stub::Shutdown();
	}
	return TRUE;
}
