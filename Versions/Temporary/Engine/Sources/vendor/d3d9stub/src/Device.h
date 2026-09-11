#pragma once

// What Direct3D9.cpp and Device.cpp need of each other.

#include <d3d9.h>

namespace ND3D9Stub
{
	// IDirect3D9::CreateDevice, once the adapter has been accepted.
	HRESULT CreateDevice( IDirect3D9 *pD3D, D3DDEVTYPE eDeviceType, HWND hFocusWindow, DWORD nBehaviorFlags,
												D3DPRESENT_PARAMETERS *pPresentationParameters, IDirect3DDevice9 **ppDevice );

	// The captured card's caps, in Caps.inc, for the adapter and the device.
	void FillCaps( D3DCAPS9 *pCaps, D3DDEVTYPE eDeviceType );

	// The real desktop's mode, so a windowed device matches the screen it is on.
	D3DDISPLAYMODE DesktopMode();

	bool IsDepthFormat( D3DFORMAT eFormat );
}
