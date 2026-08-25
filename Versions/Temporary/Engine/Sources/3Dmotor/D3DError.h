#pragma once

// Text for a D3D9 HRESULT.
//
// This is what DxErr's DXGetErrorString and DXGetErrorDescription used to
// provide. DxErr shipped only with the DirectX SDK, was never carried into the
// Windows SDK, and DXVK has no equivalent, so the table lives here instead.
// Inline rather than compiled into 3Dmotor, because Image reads 3Dmotor headers
// without linking the library.

#include <d3d9.h>

// converts DirectX error code to the string
inline const char *D3DErrorToString( HRESULT hErrorCode )
{
	switch( hErrorCode )
	{
	case D3D_OK:
		return "No error occurred.";
	case D3DERR_CONFLICTINGRENDERSTATE:
		return "The currently set render states cannot be used together.";
	case D3DERR_CONFLICTINGTEXTUREFILTER:
		return "The current texture filters cannot be used together.";
	case D3DERR_CONFLICTINGTEXTUREPALETTE: 
		return "The current textures cannot be used simultaneously.\nThis generally occurs when a multitexture device requires that all palletized textures simultaneously enabled also share the same palette.";
	case D3DERR_DEVICELOST:
		return "The device is lost and cannot be restored at the current time, so rendering is not possible.";
	case D3DERR_DEVICENOTRESET:
		return "The device cannot be reset.";
	case D3DERR_DRIVERINTERNALERROR:
		return "Internal driver error.";
	case D3DERR_INVALIDCALL:
		return "The method call is invalid. For example, a method's parameter may have an invalid value.";
	case D3DERR_INVALIDDEVICE:
		return "The requested device type is not valid.";
	case D3DERR_MOREDATA:
		return "There is more data available than the specified buffer size can hold.";
	case D3DERR_NOTAVAILABLE:
		return "The queried technique is not supported by this device.";
	case D3DERR_NOTFOUND:
		return "The requested item was not found.";
	case D3DERR_OUTOFVIDEOMEMORY:
		return "Direct3D does not have enough display memory to perform the operation.";
	case D3DERR_TOOMANYOPERATIONS: 
		return "The application is requesting more texture-filtering operations than the device supports.";
	case D3DERR_UNSUPPORTEDALPHAARG:
		return "The device does not support a specified texture-blending arguments for the alpha channel.";
	case D3DERR_UNSUPPORTEDALPHAOPERATION:
		return "The device does not support a specified texture-blending operations for the alpha channel.";
	case D3DERR_UNSUPPORTEDCOLORARG:
		return "The device does not support a specified texture-blending arguments for color values.";
	case D3DERR_UNSUPPORTEDCOLOROPERATION:
		return "The device does not support a specified texture-blending operations for color values.";
	case D3DERR_UNSUPPORTEDFACTORVALUE:
		return "The specified texture factor value is not supported by the device.";
	case D3DERR_UNSUPPORTEDTEXTUREFILTER: 
		return "The specified texture filter is not supported by the device.";
	case D3DERR_WRONGTEXTUREFORMAT:
		return "The pixel format of the texture surface is not valid.";
	default:
		return "Unrecognized error value.";
	}
	return "Unrecognized error value.";
}
