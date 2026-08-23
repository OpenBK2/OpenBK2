#pragma once

#include "3Dmotor_export.h"

#include "GPixelFormat.h"

#include <fmt/printf.h>

namespace NGfx
{

enum EFS
{
	WINDOWED,
	FULL_SCREEN
};

struct SVideoMode
{
	int nXSize, nYSize, nBpp, nRefreshRate;
	EFS fullScreen;
	SVideoMode() { nXSize = 800, nYSize = 600; nBpp = 32; nRefreshRate = 0; fullScreen = WINDOWED; }
	SVideoMode( int _nXSize, int _nYSize, int _nBpp, EFS _fullScreen, int _nRefreshRate = 0 )
		:nXSize(_nXSize), nYSize(_nYSize), nBpp(_nBpp), nRefreshRate(_nRefreshRate), fullScreen(_fullScreen) {}
};

struct SSystemInfo
{
	int nDesktopWidth;
	int nDesktopHeight;
};

struct SRenderTargetDesc
{
	int nResolution, nFormatID;

	SRenderTargetDesc() : nResolution(-1), nFormatID(-1) {}
	SRenderTargetDesc( int _nResolution, int _nFormatID ) : nResolution(_nResolution), nFormatID(_nFormatID) {}
	bool operator==( const SRenderTargetDesc &a ) const { return nResolution == a.nResolution && nFormatID == a.nFormatID; }
};
struct SRTDescHash
{
	int operator()( const SRenderTargetDesc &a ) const { return a.nResolution ^ a.nFormatID; }
};
struct SRenderTargetsInfo
{
	typedef std::unordered_map<SRenderTargetDesc,int,SRTDescHash> CRTHash;
	CRTHash targets; // resolution to number
	CRTHash cubeTargets; // resolution to number
	int nRegisters;
	int nFloatRegisters;
	SRenderTargetsInfo() : nRegisters(0), nFloatRegisters(0) {}

	void Clear() { targets.clear(); cubeTargets.clear(); nRegisters = 0; nFloatRegisters = 0; }
	static void Add( CRTHash *pRes, int nResolution, int nFormatID, int nTargets ) 
	{ 
		SRenderTargetDesc desc( nResolution, nFormatID );
		if ( pRes->find( desc ) == pRes->end() )
			(*pRes)[ desc ] = nTargets;
		else
			(*pRes)[ desc ] += nTargets;
	}
	void AddTex( int nResolution, int nFormatID, int nTargets ) { Add( &targets, nResolution, nFormatID, nTargets ); }
	void AddCube( int nResolution, int nFormatID, int nTargets ) { Add( &cubeTargets, nResolution, nFormatID, nTargets ); }
	void Add( const SRenderTargetsInfo &a )
	{
		for ( CRTHash::const_iterator i = a.targets.begin(); i != a.targets.end(); ++i )
			Add( &targets, i->first.nResolution, i->first.nFormatID, i->second );
		for ( CRTHash::const_iterator i = a.cubeTargets.begin(); i != a.cubeTargets.end(); ++i )
			Add( &cubeTargets, i->first.nResolution, i->first.nFormatID, i->second );
		nRegisters += a.nRegisters;
		nFloatRegisters += a.nFloatRegisters;
	}
};

// general
_3DMOTOR_EXPORT bool Init3D( HWND hWnd );
_3DMOTOR_EXPORT void Done3D();
HWND GetHWND();
_3DMOTOR_EXPORT bool Is3DActive();
_3DMOTOR_EXPORT void SetGamma( bool bGamma );
_3DMOTOR_EXPORT void SetGammaRamp( const std::vector<NGfx::SPixel8888> &ramp );
bool SetMode( const SVideoMode &m_, const SRenderTargetsInfo &_rtInfo );
_3DMOTOR_EXPORT void GetModesList( std::vector<SVideoMode> *pRes, int nBpp = 32 );
int GetMaxAnisotropicLevel();
_3DMOTOR_EXPORT CVec2 GetScreenRect();
void Flip();
_3DMOTOR_EXPORT void MakeScreenShot( CArray2D<SPixel8888> *pRes, bool bCorrectGamma );
void MakeScreenShotHQ( CArray2D<SPixel8888> *pRes, bool bCorrectGamma );
void MakeFast32BitScreenShot( CArray2D<SPixel8888> *pRes, bool bCorrectGamma );
_3DMOTOR_EXPORT void CheckBackBufferSize();
int GetDeviceCreationID();
const SSystemInfo& GetSystemInfo();
bool Is16BitMode();
bool Is16BitDesktop();
bool IsDXTSupported();
bool Is8888FormatSupported();
const int GetAdapterToUse();

void D3DAssertFailed( HRESULT hRes, fmt::string_view fmtStr, fmt::printf_args args );

template < typename... TArgs >
void D3DASSERT( HRESULT hRes, fmt::string_view fmtStr, const TArgs &... args )
{
	// The success path formats nothing. D3DASSERT wraps calls made per draw, and the
	// old body reached its formatting only after this same test, so the cost stays
	// where it was. is_debugger_present() and the ASSERT it guards used to run ahead
	// of the test, but both are no-ops on success, so they move as well.
	//
	// S_OK rather than D3D_OK, which d3d9.h defines as S_OK: the test lives in this
	// header now, and Gfx.h is included by modules that have no reason to see d3d9.h.
	if ( hRes == S_OK )
		return;
	D3DAssertFailed( hRes, fmtStr, fmt::make_printf_args( args... ) );
}

struct SRenderStats
{
	int nVertices, nTris, nDIPs;
	SRenderStats(): nVertices(0), nTris(0), nDIPs(0) {}
	void Clear() { nVertices = 0; nTris = 0; nDIPs = 0;}
};
EXTERNVAR SRenderStats renderStats;
}

