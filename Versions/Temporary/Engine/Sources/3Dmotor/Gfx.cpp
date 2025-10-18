#include "stdafx.h"

#include "3DLib/3DLib_export.h"

#include <D3D9.h>
#include <ddraw.h>
#include <dxerr.h>
#include "Misc/2Darray.h"
#include "System/Commands.h"
#include "Gfx.h"
#include "GfxInternal.h"
#include "Misc/HPTimer.h"
#include "GfxBuffers.h"

namespace NGfx
{


struct SVideoModeInfo
{
	D3DDISPLAYMODE mode;
	SVideoMode info;
	
	SVideoModeInfo() {}
	SVideoModeInfo( D3DDISPLAYMODE &_m, SVideoMode &_info ) : mode(_m), info(_info) {}
};

NWin32Helper::com_ptr<IDirect3D9> pD3D;
NWin32Helper::com_ptr<IDirect3DDevice9> pDevice;
bool bNoTexture = false;
SRenderStats renderStats;
bool bHardwareVP, bHardwarePixelShaders, bHardwarePixelShaders14, bHardwarePixelShaders20, bHardwarePixelShaders20a;
bool bTnLDevice = false;
static bool bForbidPS = false, bForceSWVP = false, bGammaIsSet = false;
bool bNVHackNP2Cfg = false, bNVHackNP2, bBanNP2 = false, bStaticNooverwrite = true;
bool bBan32BitIndices = true;
bool bNoCubeMapMipLevels = false;
bool bInitOk = true;
bool b16BitMode = false, bDXTSupported = true, b8888Supported = true, b16BitTextures = false;
static bool b16BitModeOnly = false, bVSync = false;
int nUseAnisotropy = 1;
_3DLIB_EXPORT EXTERNVAR int nVCacheSize;
static SVideoMode videoMode;
static D3DPRESENT_PARAMETERS pp;
static unsigned char nGammaCorrectionR[256], nGammaCorrectionG[256], nGammaCorrectionB[256];
D3DCAPS9 devCaps;
SRenderTargetsInfo rtInfo;
static HWND hWnd;
static std::vector<SVideoModeInfo> videoModes;
static int nDeviceCreationID = 1;
static SSystemInfo systemInfo;
static bool bIsDebugRuntime;
static bool bCanDoFastScreenshot = false;
static DWORD nFSAA, nMaxFSAA;
static int nWinXPos = 0;
static int nAdapterToUse = D3DADAPTER_DEFAULT;

HWND GetHWND() { return hWnd; }

// forward declarations
static D3DFORMAT GetZBufferFormat( D3DFORMAT rTarget );

static void DestroyLostableDXObjects()
{
	DoneRender();
	DestroyLostableBuffers();
	DoneZBuffer();
}
static void DestroyManagedDXObjects()
{
	DestroyManagedBuffers();
}
static HRESULT InitDXObjects()
{
	bInitOk = true;
	++nDeviceCreationID;
	// init itself
	if ( !InitZBuffer( GetZBufferFormat( pp.BackBufferFormat ) ) )
		bInitOk = false;
	InitBuffers();
	InitRender();
	return bInitOk ? D3D_OK : D3DERR_NOTAVAILABLE;
}
int GetDeviceCreationID()
{
	return nDeviceCreationID;
}
const SSystemInfo& GetSystemInfo() { return systemInfo; }
bool CanOverwriteStatic() { return bStaticNooverwrite & !bIsDebugRuntime & bHardwareVP; }
bool Is16BitMode() { return b16BitMode; }
bool IsDXTSupported() { return bDXTSupported; }
bool Is8888FormatSupported() { return b8888Supported; }
bool Is16BitTextures() { return b16BitTextures; }



static bool bDeviceCreated = false;
const _D3DDEVTYPE DEVICE_TYPE = D3DDEVTYPE_HAL;//D3DDEVTYPE_REF;//

const int GetAdapterToUse()
{
	return nAdapterToUse;
}

static HRESULT CreateDevice( NWin32Helper::com_ptr<IDirect3DDevice9> *pRes, D3DPRESENT_PARAMETERS *pPP, bool bUseSWVP )
{
	HRESULT hr;
	*pRes = 0;
	hr = -1;
	bool bUseHWVP = !bUseSWVP;
	if ( !bTnLDevice && (devCaps.VertexShaderVersion & 0xFFFF) == 0 )
		bUseHWVP = false;
	if ( ( devCaps.DevCaps & D3DDEVCAPS_PUREDEVICE ) == 0 )
		bUseHWVP = false;
	//
	if ( bUseHWVP )
	{
	    // Peter Popov - NVPerfHUD profiling	
			// Set default settings 
		  D3DDEVTYPE deviceType = DEVICE_TYPE;

			if ( NGlobal::GetVar( "gfx_nvperfhud_enable", 0 ) != 0 )
			{
				for ( int nAdapter = 0; nAdapter < pD3D->GetAdapterCount(); ++nAdapter ) 
				{
					D3DADAPTER_IDENTIFIER9 identifier; 
					HRESULT hRes = pD3D->GetAdapterIdentifier( nAdapter, 0, &identifier );
					if ( SUCCEEDED(hRes) && strcmp(identifier.Description, "NVIDIA NVPerfHUD") == 0 )
					{ 
						nAdapterToUse = nAdapter; 
						deviceType = D3DDEVTYPE_REF; 
						csSystem << "NVperfHUD found" << endl;
						break; 
					}
				}
			}

		bHardwareVP = true;
		hr = pD3D->CreateDevice(
			GetAdapterToUse(), 
			deviceType,
			hWnd,
#if defined(_DEBUG) && !defined(FAST_DEBUG)
			D3DCREATE_HARDWARE_VERTEXPROCESSING,
#else
			D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE,
#endif
			pPP,
			pRes->GetAddr() );
		D3DASSERT( hr, "CreateDevice DEVICE_TYPE: %d VERTEXPROCESSING: %d", DEVICE_TYPE, D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE );
	}
	if ( FAILED( hr ) )
	{
		bHardwareVP = false;
		hr = pD3D->CreateDevice( 
			GetAdapterToUse(), 
			DEVICE_TYPE,
			hWnd, 
			D3DCREATE_SOFTWARE_VERTEXPROCESSING,
			pPP,
			pRes->GetAddr() );
		D3DASSERT( hr, "CreateDevice DEVICE_TYPE: %d VERTEXPROCESSING: %d", DEVICE_TYPE, D3DCREATE_SOFTWARE_VERTEXPROCESSING );
	}
	return hr;
}

static HRESULT ResetDevice()
{
	HRESULT hr;
	if ( !bDeviceCreated )
		hr = CreateDevice( &pDevice, &pp, !bHardwareVP );
	else
	{
		DestroyLostableDXObjects();
		hr = pDevice->EvictManagedResources();
		D3DASSERT( hr, "EvictManagedResources failed." );
		hr = pDevice->Reset( &pp );
		D3DASSERT( hr, "Device reset failed." );
	}
	{
		D3DDEVINFO_VCACHE vcache;
		Zero( vcache );
		IDirect3DQuery9 *pQ;
		HRESULT hr = pDevice->CreateQuery( D3DQUERYTYPE_VCACHE, &pQ );
		if ( SUCCEEDED(hr) )
		{
			pQ->Issue( D3DISSUE_BEGIN );
			while ( pQ->GetData( &vcache, sizeof(vcache), D3DGETDATA_FLUSH ) != S_OK )
				Sleep(0);
			pQ->Release();
			if ( vcache.Pattern == 0x48434143 )
				nVCacheSize = vcache.CacheSize;
		}
	}
	bGammaIsSet = false;
	SetWindowPos(
		hWnd, 
		HWND_NOTOPMOST, 
		nWinXPos, 0, pp.BackBufferWidth, pp.BackBufferHeight, 
		SWP_SHOWWINDOW );
	if ( hr == D3D_OK )
	{
		hr = InitDXObjects();
		bDeviceCreated = true;
	}
	return hr;
}

static void DeviceFinalRelease()
{
	if ( pDevice )
	{
		DestroyLostableDXObjects();
		DestroyManagedDXObjects();
		pDevice = 0;
	}
	pD3D = 0;
	bDeviceCreated = false;
}



// Utility functions

int GetBpp( D3DFORMAT format )
{
	switch ( format )
	{
		case D3DFMT_R8G8B8:   return 24;
    case D3DFMT_A8R8G8B8: return 32;
    case D3DFMT_X8R8G8B8: return 32;
    case D3DFMT_R5G6B5:   return 16;
    case D3DFMT_X1R5G5B5: return 16;
    case D3DFMT_A1R5G5B5: return 16;
    case D3DFMT_A4R4G4B4: return 16;
    case D3DFMT_R3G3B2:   return 8;
    case D3DFMT_A8:       return 8;
    case D3DFMT_A8R3G3B2: return 16;
    case D3DFMT_X4R4G4B4: return 16;

    case D3DFMT_A8P8:     return 16;
    case D3DFMT_P8:       return 8;

    case D3DFMT_L8:       return 8;
    case D3DFMT_A8L8:     return 16;
    case D3DFMT_A4L4:     return 8;

    case D3DFMT_V8U8:     return 16;
    case D3DFMT_L6V5U5:   return 16;
    case D3DFMT_X8L8V8U8: return 32;
	}
	return 0;
}

static int GetZBpp( D3DFORMAT format )
{
	switch ( format )
	{
		case D3DFMT_D16_LOCKABLE: return 16;
    case D3DFMT_D32:          return 32;
    case D3DFMT_D15S1:        return 16;
    case D3DFMT_D24S8:        return 32;
    case D3DFMT_D16:          return 16;
    case D3DFMT_D24X8:        return 32;
    case D3DFMT_D24X4S4:      return 32;
	}
	return 0;
}



// DX initialisation/finalisation

static bool TestZBufferFormat( D3DFORMAT screen, D3DFORMAT rTarget, D3DFORMAT zBuf )
{
	if ( D3D_OK != pD3D->CheckDeviceFormat( 
		GetAdapterToUse(), 
		DEVICE_TYPE,
		screen, 
		D3DUSAGE_DEPTHSTENCIL,
		D3DRTYPE_SURFACE,
		zBuf ) )
		return false;
	return D3D_OK == pD3D->CheckDepthStencilMatch( 
		GetAdapterToUse(), 
		DEVICE_TYPE,
		screen, 
		rTarget, 
		zBuf );
}

static bool TestRTargetFormat( D3DFORMAT screen, D3DFORMAT rTarget )
{
	if ( D3D_OK != pD3D->CheckDeviceFormat( 
		GetAdapterToUse(), 
		DEVICE_TYPE,
		screen,
		D3DUSAGE_RENDERTARGET,
		D3DRTYPE_TEXTURE,
		rTarget ) )
		return false;
	return true;
}

static D3DFORMAT GetZBufferFormat( D3DFORMAT screen, D3DFORMAT rTarget )
{
	if ( GetBpp( rTarget ) > 16 )
	{
		if ( TestZBufferFormat( screen, rTarget, D3DFMT_D24S8 ) )
			return D3DFMT_D24S8;
		if ( TestZBufferFormat( screen, rTarget, D3DFMT_D24X4S4 ) )
			return D3DFMT_D24X4S4;
		if ( TestZBufferFormat( screen, rTarget, D3DFMT_D32 ) )
			return D3DFMT_D32;
		if ( TestZBufferFormat( screen, rTarget, D3DFMT_D24X8 ) )
			return D3DFMT_D24X8;
	}
	if ( TestZBufferFormat( screen, rTarget, D3DFMT_D16 ) )
		return D3DFMT_D16;
	ASSERT(0);
	return D3DFMT_UNKNOWN;
}

static D3DFORMAT GetZBufferFormat( D3DFORMAT rTarget )
{
	return GetZBufferFormat( pp.BackBufferFormat, rTarget );
}

// to track rescaling
static void GetBackBufferSize()
{
  RECT windowPos;
  GetClientRect( pp.hDeviceWindow, &windowPos );
  pp.BackBufferWidth = windowPos.right;
  pp.BackBufferHeight = windowPos.bottom;
}

void CheckBackBufferSize()
{
	RECT windowPos;
	GetClientRect( pp.hDeviceWindow, &windowPos );

	if ( !IsWindowVisible( pp.hDeviceWindow ) )
		return;
	if ( windowPos.bottom == 0 || windowPos.right == 0 )
		return;
	if ( pp.BackBufferHeight != windowPos.bottom || pp.BackBufferWidth != windowPos.right )
	{
		pp.BackBufferWidth = windowPos.right;
		pp.BackBufferHeight = windowPos.bottom;
		ResetDevice();
	}
}

static void FillFSAA( D3DPRESENT_PARAMETERS *pRes )
{
	if ( nFSAA > 0 && nMaxFSAA > 0 )
	{
		DWORD nFront, nDepth;
		if ( 
			SUCCEEDED( pD3D->CheckDeviceMultiSampleType( GetAdapterToUse(), DEVICE_TYPE,
				pRes->BackBufferFormat, pRes->Windowed, D3DMULTISAMPLE_NONMASKABLE, &nFront ) ) &&
			SUCCEEDED( pD3D->CheckDeviceMultiSampleType( GetAdapterToUse(), DEVICE_TYPE,
				pRes->AutoDepthStencilFormat, pRes->Windowed, D3DMULTISAMPLE_NONMASKABLE, &nDepth ) ) )
		{
			pRes->MultiSampleType = D3DMULTISAMPLE_NONMASKABLE;
			pRes->MultiSampleQuality = (std::min)( (std::min)( nFront, nDepth ), nFSAA );
			if ( pRes->MultiSampleQuality > 0 )
				--pRes->MultiSampleQuality;
		}
	}
}

bool Is16BitDesktop()
{
	HRESULT hr;
	D3DDISPLAYMODE desktop;
	hr = pD3D->GetAdapterDisplayMode( GetAdapterToUse(), &desktop );
	if ( FAILED(hr) )
		return false;
	return GetBpp( desktop.Format ) == 16;
}

static bool FillPresent( const SVideoMode &m, D3DPRESENT_PARAMETERS *pRes )
{
	HRESULT hr;
	D3DPRESENT_PARAMETERS &pp = *pRes;
	memset( &pp, 0, sizeof(pp) );
	// Get the current desktop display mode of the current adapters
	D3DDISPLAYMODE desktop;
  hr = pD3D->GetAdapterDisplayMode( GetAdapterToUse(), &desktop );
	if ( FAILED(hr) )
		return false;

	pp.MultiSampleType = D3DMULTISAMPLE_NONE;
	pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	if ( bVSync )
		pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	else
		pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	if ( FULL_SCREEN == m.fullScreen )
	{
		// search through modes to find fitting
		bool bFound = false;
		D3DDISPLAYMODE best;
		Zero( best );
		// search for suitable mode
		for ( int i = 0; i < videoModes.size(); ++i )
		{
			if ( videoModes[i].info.nXSize == m.nXSize && videoModes[i].info.nYSize == m.nYSize && videoModes[i].info.nBpp == m.nBpp )
			{
				bFound = true;
				best = videoModes[i].mode;
				break;
			}
		}
		if ( !bFound )
			return false;
		// fill structure
		pp.BackBufferWidth = m.nXSize;
		pp.BackBufferHeight = m.nYSize;
		pp.BackBufferFormat = best.Format;
		pp.BackBufferCount = 1;

		pp.hDeviceWindow = hWnd;
		pp.Windowed = FALSE;
		pp.EnableAutoDepthStencil =	TRUE;
		pp.AutoDepthStencilFormat = GetZBufferFormat( pp.BackBufferFormat, pp.BackBufferFormat );
		//pp.EnableAutoDepthStencil =	FALSE;

		pp.FullScreen_RefreshRateInHz = best.RefreshRate;
		FillFSAA( &pp );
		return true;
	}
	//
	// Windowed mode
	// ensure that our back buffer bit depth is the same as that of 
  // windowed display depth to work in windowed mode
  if ( GetBpp( desktop.Format ) != m.nBpp )
		return false;
	pp.BackBufferWidth = m.nXSize;
	pp.BackBufferHeight = m.nYSize;
	pp.BackBufferFormat = desktop.Format;//D3DFMT_UNKNOWN;//D3DFMT_X8R8G8B8;// D3DFMT_UNKNOWN;//D3DFMT_UNKNOWN_D24S8;
	pp.BackBufferCount = 1;

	pp.hDeviceWindow = hWnd;
	pp.Windowed = TRUE;
	pp.EnableAutoDepthStencil =	TRUE;
	pp.AutoDepthStencilFormat = GetZBufferFormat( pp.BackBufferFormat, pp.BackBufferFormat );
	//pp.EnableAutoDepthStencil =	FALSE;
	FillFSAA( &pp );
	return true;
}

static void DetectModes( D3DFORMAT format, int nBpp )
{
	// Get the current desktop display mode of the current adapters
	D3DDISPLAYMODE desktop;
	HRESULT hr = pD3D->GetAdapterDisplayMode( GetAdapterToUse(), &desktop );

	//bool bFound = false, bMatchScreenRefresh = false;
	for ( int i = 0; i < pD3D->GetAdapterModeCount( GetAdapterToUse(), format ); i++ )
	{
		D3DDISPLAYMODE mode;
		pD3D->EnumAdapterModes( GetAdapterToUse(), format, i, &mode );
		if ( mode.Height > desktop.Height || mode.RefreshRate != desktop.RefreshRate )
			mode.RefreshRate = D3DPRESENT_RATE_DEFAULT;
		SVideoMode info( mode.Width, mode.Height, nBpp, FULL_SCREEN, mode.RefreshRate );
		// replace with better mode
		bool bFound = false;
		for ( int k = 0; k < videoModes.size(); ++k )
		{
			SVideoModeInfo &v = videoModes[k];
			if ( v.mode.Width == mode.Width && v.mode.Height == mode.Height && v.mode.Format == mode.Format )
			{
				bFound = true;
				if ( mode.RefreshRate > v.mode.RefreshRate )
				{
					v.mode = mode;
					v.info = info;
				}
				break;
			}
		}
		if ( !bFound )
			videoModes.push_back( SVideoModeInfo( mode, info ) );
	}
}

static void SelectDeviceType()
{
	int nForceTnLDevice = Float2Int( NGlobal::GetVar( "gfx_tnl_mode", 0 ).GetFloat() );
	bTnLDevice = true;
	if ( nForceTnLDevice == 1 || (devCaps.VertexShaderVersion & 0xFFFF) == 0 || (devCaps.PixelShaderVersion & 0xFFFF) == 0 )
		bTnLDevice = true;
	else
		bTnLDevice = false;
}

static void DetectModes()
{
	videoModes.clear();
	DetectModes( D3DFMT_X8R8G8B8, 32 );
	DetectModes( D3DFMT_R5G6B5, 16 );
}

static bool CheckDeviceCaps()
{
	HRESULT hr;
	pD3D->GetDeviceCaps( GetAdapterToUse(), DEVICE_TYPE, &devCaps );
	DetectModes();
	bHardwarePixelShaders = false;
	bHardwarePixelShaders14 = false;
	bHardwarePixelShaders20 = false;
	bHardwarePixelShaders20a = false;
	bNVHackNP2 = bNVHackNP2Cfg;
	nMaxFSAA = 0;
	if ( devCaps.MaxTextureWidth < 1024 || devCaps.MaxTextureHeight < 1024 )
		return false;

	if ( FAILED( pD3D->CheckDeviceType( GetAdapterToUse(), DEVICE_TYPE, D3DFMT_X8R8G8B8, D3DFMT_X8R8G8B8, false ) ) )
	{
		b16BitModeOnly = true;
		devCaps.VertexShaderVersion = 0;
		devCaps.PixelShaderVersion = 0;
	}

	{
		// CRAP detect nVidia & clear np2 workaround flag if not found
		D3DADAPTER_IDENTIFIER9 id;
		hr = pD3D->GetAdapterIdentifier( GetAdapterToUse(), 0, &id );
		ASSERT( SUCCEEDED( hr ) );
		if ( id.VendorId == 0x10de )
		{
			if ( ( id.DeviceId & 0xf00 ) == 0x100 )
				devCaps.VertexShaderVersion = 0; // ban hwvp on 4mxs
			if ( ( id.DeviceId & 0xf00 ) != 0x200 )
				bNVHackNP2 = false;
		}
		else
		{
			bNVHackNP2 = false;
		}
		bCanDoFastScreenshot = id.VendorId == 0x1002;
	}
	if ( bForbidPS )
		devCaps.PixelShaderVersion = 0;
	if ( (devCaps.PixelShaderVersion & 0xFFFF ) >= 0x0101 )
		bHardwarePixelShaders = true;
	if ( (devCaps.PixelShaderVersion & 0xFFFF ) >= 0x0104 )
		bHardwarePixelShaders14 = true;
	if ( (devCaps.PixelShaderVersion & 0xFFFF ) >= 0x0200 )
		bHardwarePixelShaders20 = true;
	if ( (devCaps.PixelShaderVersion & 0xFFFF ) >= 0x0201 )
		bHardwarePixelShaders20a = true;
	if ( (devCaps.TextureCaps & D3DPTEXTURECAPS_MIPCUBEMAP ) == 0 )
		bNoCubeMapMipLevels = true;

	// determine device class
	SelectDeviceType();

	D3DDISPLAYMODE desktop;
	hr = pD3D->GetAdapterDisplayMode( GetAdapterToUse(), &desktop );
	D3DPRESENT_PARAMETERS tpp;
	SVideoMode tm;
	if ( b16BitModeOnly )
		tm = SVideoMode( 640, 480, D3DFMT_R5G6B5, FULL_SCREEN );
	else
		tm = SVideoMode( 640, 480, GetBpp( desktop.Format ), WINDOWED );
	if ( !FillPresent( tm, &tpp ) )
	{
		ASSERT( 0 );
		return false;
	}
	NWin32Helper::com_ptr<IDirect3DDevice9> pTestDevice;
	CreateDevice( &pTestDevice, &tpp, bForceSWVP );
	if ( !pTestDevice )
	{
		ASSERT( 0 );
		return false;
	}

	if ( pTestDevice->GetAvailableTextureMem() < 45000000 )
		bHardwareVP = false;

	// detect D3D debug dll
	NWin32Helper::com_ptr<IDirect3DVertexBuffer9> pTestBuffer;
	pTestDevice->CreateVertexBuffer( 1024, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, pTestBuffer.GetAddr(), 0 );
	void *pTest;
	hr = pTestBuffer->Lock( 0, 1024, &pTest, D3DLOCK_NOOVERWRITE );
	bIsDebugRuntime = FAILED(hr);
	if ( SUCCEEDED(hr) )
		pTestBuffer->Unlock();

	systemInfo.nDesktopWidth = desktop.Width;
	systemInfo.nDesktopHeight = desktop.Height;

	if ( !bHardwareVP )
	{
		bTnLDevice = true;
		bHardwarePixelShaders = false;
		bHardwarePixelShaders14 = false;
		bHardwarePixelShaders20 = false;
		bHardwarePixelShaders20a = false;
		bNVHackNP2 = false;
	}
	bBan32BitIndices = devCaps.MaxVertexIndex < 1000000 || !bHardwareVP; // ban streaming actually

	// check textures support
	bool bRequiredTextureFormatsAreSupported = 
		SUCCEEDED( pD3D->CheckDeviceFormat( GetAdapterToUse(), DEVICE_TYPE, desktop.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_R5G6B5 ) ) &&
		SUCCEEDED( pD3D->CheckDeviceFormat( GetAdapterToUse(), DEVICE_TYPE, desktop.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_A1R5G5B5 ) ) &&
		SUCCEEDED( pD3D->CheckDeviceFormat( GetAdapterToUse(), DEVICE_TYPE, desktop.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_A4R4G4B4 ) );
	if ( !bRequiredTextureFormatsAreSupported )
		return false;
	bDXTSupported =
		SUCCEEDED( pD3D->CheckDeviceFormat( GetAdapterToUse(), DEVICE_TYPE, desktop.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_DXT1 ) ) &&
		SUCCEEDED( pD3D->CheckDeviceFormat( GetAdapterToUse(), DEVICE_TYPE, desktop.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_DXT2 ) ) &&
		SUCCEEDED( pD3D->CheckDeviceFormat( GetAdapterToUse(), DEVICE_TYPE, desktop.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_DXT3 ) ) &&
		SUCCEEDED( pD3D->CheckDeviceFormat( GetAdapterToUse(), DEVICE_TYPE, desktop.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_DXT4 ) ) &&
		SUCCEEDED( pD3D->CheckDeviceFormat( GetAdapterToUse(), DEVICE_TYPE, desktop.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_DXT5 ) );
	b8888Supported = 
		SUCCEEDED( pD3D->CheckDeviceFormat( GetAdapterToUse(), DEVICE_TYPE, desktop.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8 ) );

	DWORD nFrontFSAA, nDepthFSAA;
	if ( 
		SUCCEEDED( pD3D->CheckDeviceMultiSampleType( GetAdapterToUse(), DEVICE_TYPE, D3DFMT_X8R8G8B8,
			FALSE, D3DMULTISAMPLE_NONMASKABLE, &nFrontFSAA ) ) &&
		SUCCEEDED( pD3D->CheckDeviceMultiSampleType( GetAdapterToUse(), DEVICE_TYPE, D3DFMT_D24S8,
			FALSE, D3DMULTISAMPLE_NONMASKABLE, &nDepthFSAA ) )
		)
		nMaxFSAA = (std::min)( nFrontFSAA, nDepthFSAA );

	return true;
}

static bool InitD3D()
{
	pD3D.Create( Direct3DCreate9( D3D_SDK_VERSION ) );
	if ( pD3D == 0 )
	{
		ASSERT( 0 );
		return false;
	}
	if ( !CheckDeviceCaps() )
		return false;
	return true;
}

bool SetMode( const SVideoMode &m, const SRenderTargetsInfo &_rtInfo )
{
	if ( b16BitModeOnly && m.nBpp != 16 )
	{
		SVideoMode m16( m );
		m16.nBpp = 16;
		return SetMode( m16, _rtInfo );
	}
	D3DPRESENT_PARAMETERS newPP;
	if ( !FillPresent( m, &newPP ) )
		return false;
	pp = newPP;
	rtInfo = _rtInfo;
	videoMode = m;
	b16BitMode = m.nBpp == 16;
	HRESULT hr = ResetDevice();
	return D3D_OK == hr;
}

void GetModesList( std::list<SVideoMode> *pRes, int nBpp )
{
	if ( b16BitModeOnly )
		nBpp = 16;
	if ( videoModes.empty() )
		return;
	pRes->clear();
	for ( int k = 0; k < videoModes.size(); ++k )
	{
		if ( videoModes[k].info.nBpp == nBpp )
			pRes->push_back( videoModes[k].info );
	}
}

int GetMaxAnisotropicLevel()
{
	return devCaps.MaxAnisotropy;
}

CVec2 GetScreenRect()
{
	return CVec2( pp.BackBufferWidth, pp.BackBufferHeight );
}

static void CorrectGamma( CArray2D<SPixel8888> *pRes )
{
	for ( int y = 0; y < pRes->GetSizeY(); ++y )
	{
		for ( int x = 0; x < pRes->GetSizeX(); ++x )
		{
			SPixel8888 &c = (*pRes)[y][x];
			c.r = nGammaCorrectionR[ c.r ];
			c.g = nGammaCorrectionG[ c.g ];
			c.b = nGammaCorrectionB[ c.b ];
		}
	}
}

void MakeScreenShot( CArray2D<SPixel8888> *pRes, bool bCorrectGamma )
{
	HRESULT hr;
	NWin32Helper::com_ptr<IDirect3DSurface9> pSurface;
	if ( pp.BackBufferWidth == 0 || pp.BackBufferHeight == 0 )
	{
		pRes->SetSizes( 1, 1 );
		return;
	}
	D3DDISPLAYMODE desktop;
  hr = pD3D->GetAdapterDisplayMode( GetAdapterToUse(), &desktop );
	ASSERT( D3D_OK == hr );
	hr = pDevice->CreateOffscreenPlainSurface( desktop.Width, desktop.Height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, pSurface.GetAddr(), 0 );
	ASSERT( D3D_OK == hr );
	hr = pDevice->GetFrontBufferData( 0, pSurface );
	ASSERT( D3D_OK == hr );
	D3DLOCKED_RECT lr;
	hr = pSurface->LockRect( &lr, 0, D3DLOCK_READONLY );
	ASSERT( D3D_OK == hr );
	pRes->SetSizes( pp.BackBufferWidth, pp.BackBufferHeight );
	const char *pSrc = (const char*) lr.pBits;
	for ( int y = 0; y < pRes->GetSizeY(); ++y )
	{
		memcpy( &((*pRes)[y][0]), pSrc, 4 * pRes->GetSizeX() );
		pSrc += lr.Pitch;
	}
	if ( bCorrectGamma )
		CorrectGamma( pRes );
}

void MakeFast32BitScreenShot( CArray2D<SPixel8888> *pRes, bool bCorrectGamma )
{
	if ( !bCanDoFastScreenshot )
	{
		MakeScreenShot( pRes, bCorrectGamma );
		return;
	}
	HRESULT hr;
	NWin32Helper::com_ptr<IDirect3DSurface9> pSurface, pBackBuffer;
	if ( pp.BackBufferWidth == 0 || pp.BackBufferHeight == 0 )
	{
		pRes->SetSizes( 1, 1 );
		return;
	}
	hr = pDevice->CreateOffscreenPlainSurface( pp.BackBufferWidth, pp.BackBufferHeight, pp.BackBufferFormat, D3DPOOL_SYSTEMMEM, pSurface.GetAddr(), 0 );
	ASSERT( D3D_OK == hr );
	hr = pDevice->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, pBackBuffer.GetAddr() );
	ASSERT( D3D_OK == hr );
	hr = pDevice->GetRenderTargetData( pBackBuffer, pSurface );
	ASSERT( D3D_OK == hr );
	D3DLOCKED_RECT lr;
	hr = pSurface->LockRect( &lr, 0, D3DLOCK_READONLY );
	ASSERT( D3D_OK == hr );
	pRes->SetSizes( pp.BackBufferWidth, pp.BackBufferHeight );
	const char *pSrc = (const char*) lr.pBits;
	for ( int y = 0; y < pRes->GetSizeY(); ++y )
	{
		memcpy( &((*pRes)[y][0]), pSrc, 4 * pRes->GetSizeX() );
		pSrc += lr.Pitch;
	}
	hr = pSurface->UnlockRect();
	ASSERT( D3D_OK == hr );
	if ( bCorrectGamma )
		CorrectGamma( pRes );
}

bool bOutputFPS = false;
static float fTotalFrameTime = 0;
int nTotalFrames = 0;
const int N_SLOW_FPS_TYPE = 31;//1023;//
NHPTimer::STime timeFrameStart;

static int nPrevTexLOD = 0;
const int N_MIN_TEX_MEM = 10000000;
const int N_TEX_CHECK_FRAMES = 5;
static int nLastTexCheckFrame = 0;
void Flip()
{
	HRESULT hr = pDevice->EndScene();
	ASSERT( hr == D3D_OK );
	//
	hr = pDevice->Present( 0, 0, hWnd, 0 );
	//
	double fFrameTime = NHPTimer::GetTimePassed( &timeFrameStart );
	++nTotalFrames;
	if ( bOutputFPS )
	{
		static int nSlowOutCounter;
		fTotalFrameTime += fFrameTime;
		if ( (nTotalFrames&N_SLOW_FPS_TYPE) == 0 )
		{
			float fFPS = N_SLOW_FPS_TYPE / fTotalFrameTime;
			char szBuf[1024];
			sprintf( szBuf, "FPS = %f\n", fFPS );
			OutputDebugString( szBuf );
			nTotalFrames = 0;
			fTotalFrameTime = 0;
		}
	}
	//
	NextFrameBuffes();
	ReduceHWLag();

	//DebugTrace( "TexMem available: %d", pDevice->GetAvailableTextureMem() );

	if ( nLastTexCheckFrame >= N_TEX_CHECK_FRAMES )
	{
		const unsigned nCurMem = pDevice->GetAvailableTextureMem();
		if ( nCurMem < N_MIN_TEX_MEM )
		{
			++nPrevTexLOD;
			if ( nPrevTexLOD <= 4 )
			{
				SetLODToAllTextures( nPrevTexLOD );
				DebugTrace( "Reduce to %d mip-level of textures", nPrevTexLOD );
			}
		}
		else if ( nPrevTexLOD > 0 )
		{
			if ( ( nCurMem - CalcTouchedTextureSizeNotSetMip( nPrevTexLOD - 1 ) ) >= N_MIN_TEX_MEM )
			{
				--nPrevTexLOD;
				SetLODToAllTextures( nPrevTexLOD );
				DebugTrace( "Raise to %d mip-level of textures", nPrevTexLOD );
			}
		}
		nLastTexCheckFrame = 0;
	}
	else
		++nLastTexCheckFrame;
  //
	hr = pDevice->BeginScene();
	ASSERT( D3D_OK == hr );
	renderStats.Clear();
}

// test cooperative level

static float fGamma = 1.0f;
static std::vector<NGfx::SPixel8888> gammaRamp;
static float fLastGamma = -1;
static bool bChangedGammaRamp;
static D3DGAMMARAMP keptGamma;
void SetGamma( bool bGamma )
{
	if ( !pDevice )
		return;
	if ( ( bGammaIsSet == bGamma ) && ( fGamma == fLastGamma ) && !bChangedGammaRamp )
		return;

	// set gamma
	D3DGAMMARAMP gamma;
	if ( bGamma )
	{
		pDevice->GetGammaRamp( 0, &keptGamma );
		for ( int k = 0; k < 256; ++k )
		{
			float f = k / 256.0f;
			//if ( f < 0.0031308f ) f = f * 12.92f; else 	f = 1.055f * exp( log( f ) / 2.4f ) - 0.055f;
			//if ( f < 0.026175f ) f = f * 4; else 	f = 1.1466f * exp( log( f ) / 2.4f ) - 0.1466f;
			f = exp( log( f ) / fGamma );
			WORD wRes = Float2Int( f * 65535 );
			if ( !gammaRamp.empty() )
			{
				const NGfx::SPixel8888 &c = gammaRamp[ wRes >> 8 ];
				gamma.red[k] = c.r << 8;
				gamma.green[k] = c.g << 8;
				gamma.blue[k] = c.b << 8;
			}
			else
			{
				gamma.red[k] = wRes;
				gamma.green[k] = wRes;
				gamma.blue[k] = wRes;
			}
			nGammaCorrectionR[k] = gamma.red[k] >> 8;
			nGammaCorrectionG[k] = gamma.green[k] >> 8;
			nGammaCorrectionB[k] = gamma.blue[k] >> 8;
		}
	}
	else
	{
		int nMax = 0;
		for ( int k = 0; k < 256; ++k )
		{
			nMax = (std::max)( nMax, (int)keptGamma.red[k] );
			nMax = (std::max)( nMax, (int)keptGamma.green[k] );
			nMax = (std::max)( nMax, (int)keptGamma.blue[k] );
		}
		if ( nMax < 120 )
		{
			// suspicious gamma was returned better set to default
			for ( int k = 0; k < 256; ++k )
			{
				WORD wRes = k << 8;
				gamma.red[k] = wRes;
				gamma.green[k] = wRes;
				gamma.blue[k] = wRes;
			}
		}
		else
		{
			// for some reason gamma returned by GetGammaRamp is in different range then it is expected in SetGammaRamp
			int nShift = 0;
			while ( (nMax<<nShift) < 32768 )
				++nShift;
			for ( int k = 0; k < 256; ++k )
			{
				WORD wRes = k << 8;
				gamma.red[k] = keptGamma.red[k] << nShift;
				gamma.green[k] = keptGamma.green[k] << nShift;
				gamma.blue[k] = keptGamma.blue[k] << nShift;
			}
		}
	}
	pDevice->SetGammaRamp( 0, D3DSGR_NO_CALIBRATION, &gamma );
	bGammaIsSet = bGamma;
	fLastGamma = fGamma;
	bChangedGammaRamp = false;
}

void SetGammaRamp( const std::vector<NGfx::SPixel8888> &ramp )
{
	gammaRamp = ramp;
	bChangedGammaRamp = true;
}

bool Is3DActive()
{
	HRESULT hr = pDevice->TestCooperativeLevel();
	if ( hr == D3DERR_DEVICELOST )
		return false; 
	if ( hr == D3D_OK )
		return true;
	if ( pp.Windowed )
	{
		GetBackBufferSize();
		if ( !FillPresent( videoMode, &pp ) )
			return false;
	}
	ResetDevice();
	hr = pDevice->TestCooperativeLevel();
	if ( hr == D3DERR_DEVICELOST )
		return false;
	ASSERT( hr == D3D_OK );
	return hr == D3D_OK;
}

bool Init3D( HWND _hWnd )
{
	hWnd = _hWnd;
	if ( !InitD3D() )
	{
		ASSERT(0);
		return false;
	}
	return true;
}

void Done3D()
{
	DeviceFinalRelease();
}

void D3DASSERT( HRESULT hRes, const char *pDescr, ... )
{
	if ( IsDebuggerPresent() )
		ASSERT( hRes == D3D_OK );
	////
	if ( hRes == D3D_OK )
		return;
	////
	static char buff[256] = { '\0' };
	va_list va;
	va_start( va, pDescr );
	_vsnprintf( buff, 255, pDescr, va );
	va_end( va );
	////
	csSystem << "ERROR: " << buff << ". ";
	csSystem << "CODE: " << DXGetErrorString( hRes ) << ". ";
	csSystem << "DESCRIPTION: " << DXGetErrorDescription( hRes ) << endl;
}

// Commands/Vars


START_REGISTER(Gfx)
	// need restart to change
	REGISTER_VAR( "gfx_tnl_mode", 0, 0, STORAGE_USER )
	REGISTER_VAR_EX( "gfx_nopixelshaders", NGlobal::VarBoolHandler, &bForbidPS, 0, STORAGE_USER )
	REGISTER_VAR_EX( "gfx_swvertexprocess", NGlobal::VarBoolHandler, &bForceSWVP, 0, STORAGE_USER )
	REGISTER_VAR_EX( "gfx_fix_nv_np2_hack", NGlobal::VarBoolHandler, &bNVHackNP2Cfg, 0, STORAGE_USER )
	////
	REGISTER_VAR_EX( "gfx_gamma", NGlobal::VarFloatHandler, &fGamma, 1.0f, STORAGE_USER )
	REGISTER_VAR_EX( "gfx_validate", NGlobal::VarBoolHandler, &bDoValidateDevice, 0, STORAGE_NONE )
	REGISTER_VAR_EX( "gfx_anisotropic_filter", NGlobal::VarIntHandler, &nUseAnisotropy, 1, STORAGE_USER )
	REGISTER_VAR_EX( "gfx_fix_ban_np2", NGlobal::VarBoolHandler, &bBanNP2, 0, STORAGE_USER )
	REGISTER_VAR_EX( "gfx_static_nooverwrite", NGlobal::VarBoolHandler, &bStaticNooverwrite, 1, STORAGE_USER )
	REGISTER_VAR_EX( "gfx_notexture", NGlobal::VarBoolHandler, &bNoTexture, 0, STORAGE_NONE )
	REGISTER_VAR_EX( "gfx_16bit_textures", NGlobal::VarBoolHandler, &b16BitTextures, 0, STORAGE_USER )
	REGISTER_VAR_EX( "gfx_fsaa", NGlobal::VarIntHandler, &nFSAA, 0, STORAGE_USER )
	REGISTER_VAR_EX( "gfx_vsync", NGlobal::VarBoolHandler, &bVSync, 0, STORAGE_USER )

	REGISTER_VAR_EX( "special_win_x_pos", NGlobal::VarIntHandler, &nWinXPos, 0, STORAGE_USER );
	REGISTER_VAR_EX( "adapter_to_use", NGlobal::VarIntHandler, &nAdapterToUse, D3DADAPTER_DEFAULT, STORAGE_USER );
FINISH_REGISTER

}

