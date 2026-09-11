"""Will Direct3D 9 give a device on this desktop? The questions Init3D asks, one by one.

    python d3d9check.py                    # the desktop this runs on
    python d3d9check.py --desktop bk2probe

The editor only logs "Failed to initialize Direct3D9". This asks what
3Dmotor's InitD3D asks -- Direct3DCreate9, the adapter count and display mode,
CheckDeviceType, then a windowed HAL device on a hidden 64x64 window -- and
says which step answers what. Measured 2026-09-07: on bk2probe everything up
to CheckDeviceType succeeds and CheckDeviceType and CreateDevice answer
D3DERR_NOTAVAILABLE (0x8876086A), because D3D9 refuses a process on a desktop
that is not the input desktop. That is why the probe runs use the stub
d3d9.dll (rundesktop.py --d3d9stub).

Loads whatever d3d9.dll the loader finds first for python.exe -- System32's,
unless a copy sits beside it.
"""
import argparse
import ctypes as C
import sys
from ctypes import wintypes

D3D_SDK_VERSION, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL = 32, 0, 1
D3DFMT_X8R8G8B8, D3DFMT_UNKNOWN = 22, 0
D3DCREATE_SOFTWARE_VERTEXPROCESSING, D3DSWAPEFFECT_DISCARD = 0x20, 1


class D3DDISPLAYMODE(C.Structure):
    _fields_ = [('Width', wintypes.UINT), ('Height', wintypes.UINT), ('RefreshRate', wintypes.UINT),
                ('Format', C.c_int)]


class D3DPRESENT_PARAMETERS(C.Structure):
    _fields_ = [('BackBufferWidth', wintypes.UINT), ('BackBufferHeight', wintypes.UINT),
                ('BackBufferFormat', C.c_int), ('BackBufferCount', wintypes.UINT),
                ('MultiSampleType', C.c_int), ('MultiSampleQuality', wintypes.DWORD),
                ('SwapEffect', C.c_int), ('hDeviceWindow', wintypes.HWND), ('Windowed', wintypes.BOOL),
                ('EnableAutoDepthStencil', wintypes.BOOL), ('AutoDepthStencilFormat', C.c_int),
                ('Flags', wintypes.DWORD), ('FullScreen_RefreshRateInHz', wintypes.UINT),
                ('PresentationInterval', wintypes.UINT)]


def method(obj, index, restype, argtypes):
    vtable = C.cast(obj, C.POINTER(C.POINTER(C.c_void_p)))[0]
    return C.WINFUNCTYPE(restype, *argtypes)(vtable[index])


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--desktop', help='run on this desktop (before any window exists)')
    a = ap.parse_args()
    u = C.WinDLL('user32', use_last_error=True)
    if a.desktop:
        u.OpenDesktopW.restype = wintypes.HANDLE
        u.OpenDesktopW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
        u.SetThreadDesktop.argtypes = [wintypes.HANDLE]
        desk = u.OpenDesktopW(a.desktop, 0, False, 0x10000000)
        if not desk or not u.SetThreadDesktop(desk):
            raise SystemExit('cannot move to desktop %s: %d' % (a.desktop, C.get_last_error()))
    d3d9 = C.WinDLL('d3d9')
    d3d9.Direct3DCreate9.argtypes = [wintypes.UINT]
    d3d9.Direct3DCreate9.restype = C.c_void_p
    p = d3d9.Direct3DCreate9(D3D_SDK_VERSION)
    print('Direct3DCreate9        -> %s' % ('0x%X' % p if p else 'NULL'))
    if not p:
        return 1
    print('GetAdapterCount        -> %d' % method(p, 4, wintypes.UINT, [C.c_void_p])(p))
    mode = D3DDISPLAYMODE()
    hr = method(p, 8, C.c_long, [C.c_void_p, wintypes.UINT, C.POINTER(D3DDISPLAYMODE)])(p, 0, C.byref(mode))
    print('GetAdapterDisplayMode  -> 0x%08X %dx%d format %d' % (hr & 0xFFFFFFFF, mode.Width, mode.Height, mode.Format))
    # CheckDeviceType takes no window, which is what pins a failure on the
    # desktop rather than on anything about the window.
    hr = method(p, 9, C.c_long, [C.c_void_p, wintypes.UINT, C.c_int, C.c_int, C.c_int, wintypes.BOOL])(
        p, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DFMT_X8R8G8B8, True)
    print('CheckDeviceType        -> 0x%08X' % (hr & 0xFFFFFFFF))
    u.CreateWindowExW.restype = wintypes.HWND
    u.CreateWindowExW.argtypes = [wintypes.DWORD, wintypes.LPCWSTR, wintypes.LPCWSTR, wintypes.DWORD] + \
        [C.c_int] * 4 + [wintypes.HWND, wintypes.HMENU, wintypes.HINSTANCE, C.c_void_p]
    hwnd = u.CreateWindowExW(0, 'STATIC', 'd3d9check', 0, 0, 0, 64, 64, None, None, None, None)
    pp = D3DPRESENT_PARAMETERS(BackBufferWidth=64, BackBufferHeight=64, BackBufferFormat=D3DFMT_UNKNOWN,
                               BackBufferCount=1, SwapEffect=D3DSWAPEFFECT_DISCARD, hDeviceWindow=hwnd, Windowed=1)
    dev = C.c_void_p()
    hr = method(p, 16, C.c_long, [C.c_void_p, wintypes.UINT, C.c_int, wintypes.HWND, wintypes.DWORD,
                                   C.POINTER(D3DPRESENT_PARAMETERS), C.POINTER(C.c_void_p)])(
        p, 0, D3DDEVTYPE_HAL, hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, C.byref(pp), C.byref(dev))
    print('CreateDevice (HAL)     -> 0x%08X device=%s' % (hr & 0xFFFFFFFF, '0x%X' % dev.value if dev.value else 'NULL'))
    if dev.value:
        method(dev.value, 2, wintypes.ULONG, [C.c_void_p])(dev.value)
    method(p, 2, wintypes.ULONG, [C.c_void_p])(p)
    return 0 if dev.value else 1


if __name__ == '__main__':
    sys.exit(main())
