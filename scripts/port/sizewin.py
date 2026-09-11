"""Show, resize or raise a window on the probe desktop so it lays out and can be photographed.

    python sizewin.py 0x2008F6 240 420            # show it at 240x420
    python sizewin.py 0x2109CA --raise            # show and bring to the top of its siblings
    python sizewin.py 0x2008F6 240 420 --raise

A docking pane or palette the editor has not shown yet sits hidden at 0x0, and
deskshot.py (PrintWindow) then returns whatever is behind it. SetWindowPos and
ShowWindow are window-manager calls that work across processes and touch no
input: resizing makes the owning thread run its own WM_SIZE handler, so an MFC
shortcut bar or tab window lays its children out for real, and a wx host fits
its contents. Size the chain outside-in -- pane, shortcut bar, tab window --
and let each lay out the next; `uiact.py ancestry <hwnd>` prints the chain.
"""
import argparse
import ctypes as C
import sys
from ctypes import wintypes

import uiprobe as P

P.u.SetWindowPos.argtypes = [wintypes.HWND, wintypes.HWND, C.c_int, C.c_int, C.c_int, C.c_int, wintypes.UINT]
P.u.ShowWindow.argtypes = [wintypes.HWND, C.c_int]
SWP_NOSIZE, SWP_NOMOVE, SWP_NOZORDER, SWP_NOACTIVATE, SWP_SHOWWINDOW = 0x1, 0x2, 0x4, 0x10, 0x40
SW_SHOWNA, HWND_TOP = 8, 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('hwnd', type=lambda s: int(s, 16) if not s.lower().startswith('0x') else int(s, 0))
    ap.add_argument('width', type=int, nargs='?')
    ap.add_argument('height', type=int, nargs='?')
    ap.add_argument('--raise', dest='raise_', action='store_true')
    ap.add_argument('--desktop', default='bk2probe')
    a = ap.parse_args()
    P.attach(a.desktop)
    P.u.ShowWindow(a.hwnd, SW_SHOWNA)
    if a.width and a.height:
        P.u.SetWindowPos(a.hwnd, 0, 0, 0, a.width, a.height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW)
    if a.raise_:
        P.u.SetWindowPos(a.hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW)
    r = P.rect(a.hwnd)
    print('%s visible=%s at (%d,%d) %dx%d' % (hex(a.hwnd), bool(P.u.IsWindowVisible(a.hwnd)), r.left, r.top,
                                             r.right - r.left, r.bottom - r.top))
    return 0


if __name__ == '__main__':
    sys.exit(main())
