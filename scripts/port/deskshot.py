"""Screenshot a window on another Win32 desktop, unattended.

winshot.ps1 cannot do this and never will: SetThreadDesktop refuses a thread
that already owns a window, and PowerShell's host thread owns several, so it
fails with ERROR_BUSY (170) before it starts. A fresh Python process owns none,
which is the only reason this works.

PrintWindow is what makes it possible at all. It asks the window to paint itself
into a DC, so it does not care whether the window is on a screen anyone is
looking at -- which is exactly the case on a desktop created by rundesktop.py.

    python deskshot.py --out shot.png                    # the editor's frame
    python deskshot.py --id 400 --out tree.png           # one control by id
    python deskshot.py --hwnd 700F62 --out frame.png

PW_RENDERFULLCONTENT (flag 2) is passed, which is what makes layered and
DirectComposition content come out rather than black. It does not help a D3D
swap chain: the viewport is presented straight to the screen and there is
nothing for the window to paint. See --probe-d3d.
"""
import argparse
import ctypes as C
from ctypes import wintypes

from PIL import Image

u = C.WinDLL('user32', use_last_error=True)
g = C.WinDLL('gdi32', use_last_error=True)
k = C.WinDLL('kernel32', use_last_error=True)

GENERIC_ALL = 0x10000000
PW_RENDERFULLCONTENT = 0x00000002
BI_RGB = 0
DIB_RGB_COLORS = 0

u.OpenDesktopW.restype = wintypes.HANDLE
u.OpenDesktopW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
u.SetThreadDesktop.argtypes = [wintypes.HANDLE]
u.GetWindowRect.argtypes = [wintypes.HWND, C.POINTER(wintypes.RECT)]
u.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, C.c_int]
u.GetClassNameW.argtypes = [wintypes.HWND, wintypes.LPWSTR, C.c_int]
u.GetWindowThreadProcessId.argtypes = [wintypes.HWND, C.POINTER(wintypes.DWORD)]
u.GetDlgCtrlID.argtypes = [wintypes.HWND]
# argtypes as well as restype throughout: a GDI handle in a 64-bit process does
# not fit the int ctypes assumes for an unprototyped argument, and the call dies
# with "int too long to convert".
u.GetWindowDC.restype = wintypes.HDC
u.GetWindowDC.argtypes = [wintypes.HWND]
u.ReleaseDC.argtypes = [wintypes.HWND, wintypes.HDC]
u.PrintWindow.argtypes = [wintypes.HWND, wintypes.HDC, wintypes.UINT]
g.CreateCompatibleDC.restype = wintypes.HDC
g.CreateCompatibleDC.argtypes = [wintypes.HDC]
g.CreateCompatibleBitmap.restype = wintypes.HBITMAP
g.CreateCompatibleBitmap.argtypes = [wintypes.HDC, C.c_int, C.c_int]
g.SelectObject.restype = wintypes.HGDIOBJ
g.SelectObject.argtypes = [wintypes.HDC, wintypes.HGDIOBJ]
g.DeleteObject.argtypes = [wintypes.HGDIOBJ]
g.DeleteDC.argtypes = [wintypes.HDC]
g.GetDIBits.argtypes = [wintypes.HDC, wintypes.HBITMAP, wintypes.UINT, wintypes.UINT,
                        C.c_void_p, C.c_void_p, wintypes.UINT]

WNDENUMPROC = C.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
u.EnumDesktopWindows.argtypes = [wintypes.HANDLE, WNDENUMPROC, wintypes.LPARAM]
u.EnumChildWindows.argtypes = [wintypes.HWND, WNDENUMPROC, wintypes.LPARAM]


class BITMAPINFOHEADER(C.Structure):
    _fields_ = [('biSize', wintypes.DWORD), ('biWidth', wintypes.LONG),
                ('biHeight', wintypes.LONG), ('biPlanes', wintypes.WORD),
                ('biBitCount', wintypes.WORD), ('biCompression', wintypes.DWORD),
                ('biSizeImage', wintypes.DWORD), ('biXPelsPerMeter', wintypes.LONG),
                ('biYPelsPerMeter', wintypes.LONG), ('biClrUsed', wintypes.DWORD),
                ('biClrImportant', wintypes.DWORD)]


def join(desktop):
    """Move this thread to that desktop. Must happen before any window exists."""
    desk = u.OpenDesktopW(desktop, 0, False, GENERIC_ALL)
    if not desk:
        raise SystemExit('no desktop %r (err %d)' % (desktop, C.get_last_error()))
    if not u.SetThreadDesktop(desk):
        raise SystemExit('SetThreadDesktop(%r) failed: %d' % (desktop, C.get_last_error()))
    return desk


def find_frame(image_pids):
    found = []

    def cb(h, _):
        pid = wintypes.DWORD()
        u.GetWindowThreadProcessId(h, C.byref(pid))
        if pid.value in image_pids:
            cls = C.create_unicode_buffer(160)
            title = C.create_unicode_buffer(256)
            u.GetClassNameW(h, cls, 160)
            u.GetWindowTextW(h, title, 256)
            if cls.value.startswith('Afx') and title.value:
                found.append(h)
                return False
        return True

    proc = WNDENUMPROC(cb)
    u.EnumDesktopWindows(None, proc, 0)
    return found[0] if found else None


def find_child(hFrame, ctrl_id):
    found = []

    def cb(h, _):
        if u.GetDlgCtrlID(h) == ctrl_id:
            found.append(h)
            return False
        return True

    proc = WNDENUMPROC(cb)
    u.EnumChildWindows(hFrame, proc, 0)
    return found[0] if found else None


def shoot(hwnd, path):
    rect = wintypes.RECT()
    if not u.GetWindowRect(hwnd, C.byref(rect)):
        raise SystemExit('GetWindowRect failed: %d' % C.get_last_error())
    w = rect.right - rect.left
    h = rect.bottom - rect.top
    if w <= 0 or h <= 0:
        raise SystemExit('window 0x%X has no size' % hwnd)

    hdcWin = u.GetWindowDC(hwnd)
    hdcMem = g.CreateCompatibleDC(hdcWin)
    hbm = g.CreateCompatibleBitmap(hdcWin, w, h)
    old = g.SelectObject(hdcMem, hbm)
    ok = u.PrintWindow(hwnd, hdcMem, PW_RENDERFULLCONTENT)

    bi = BITMAPINFOHEADER()
    bi.biSize = C.sizeof(bi)
    bi.biWidth = w
    bi.biHeight = -h            # negative, so the rows come back top down
    bi.biPlanes = 1
    bi.biBitCount = 32
    bi.biCompression = BI_RGB
    buf = (C.c_char * (w * h * 4))()
    g.GetDIBits(hdcMem, hbm, 0, h, buf, C.byref(bi), DIB_RGB_COLORS)

    g.SelectObject(hdcMem, old)
    g.DeleteObject(hbm)
    g.DeleteDC(hdcMem)
    u.ReleaseDC(hwnd, hdcWin)

    img = Image.frombuffer('RGBA', (w, h), bytes(buf), 'raw', 'BGRA', 0, 1)
    img.convert('RGB').save(path)
    return w, h, bool(ok)


def pids_for(image):
    import subprocess
    txt = subprocess.run(['tasklist', '/FI', 'IMAGENAME eq ' + image, '/FO', 'CSV', '/NH'],
                         capture_output=True, text=True).stdout
    return {int(l.split('","')[1]) for l in txt.splitlines() if l.startswith('"')}


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--desktop', default='bk2probe')
    ap.add_argument('--image', default='B2_MapEditor.exe')
    ap.add_argument('--hwnd', help='a window handle in hex')
    ap.add_argument('--id', type=int, help='a control id under the frame')
    ap.add_argument('--out', default='deskshot.png')
    args = ap.parse_args()

    join(args.desktop)
    if args.hwnd:
        hwnd = int(args.hwnd, 16)
    else:
        hwnd = find_frame(pids_for(args.image))
        if hwnd is None:
            raise SystemExit('%s has no frame on %s' % (args.image, args.desktop))
        if args.id is not None:
            hwnd = find_child(hwnd, args.id)
            if hwnd is None:
                raise SystemExit('no control %d under the frame' % args.id)
    w, h, ok = shoot(hwnd, args.out)
    print('%s %dx%d from 0x%X (PrintWindow returned %s)' % (args.out, w, h, hwnd, ok))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
