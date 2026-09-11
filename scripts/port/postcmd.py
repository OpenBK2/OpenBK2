"""Post WM_COMMAND ids to a window on another desktop.

sendcmd.py finds the main frame by looking for a window with a menu bar, and
this editor's menu is a Stingray SECMenuBar toolbar rather than an HMENU, so
that search comes up empty. Address the frame by its title instead.

    python postcmd.py --desktop bk2probe --title "Blitzkrieg 2 Editor" 1040
    python postcmd.py --hwnd 0x1234 --msg 0x14F --wparam 1   # CB_SHOWDROPDOWN
"""
import argparse
import ctypes as C
import time
from ctypes import wintypes

u = C.WinDLL('user32', use_last_error=True)
WNDENUMPROC = C.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
u.EnumDesktopWindows.argtypes = [wintypes.HANDLE, WNDENUMPROC, wintypes.LPARAM]
u.OpenDesktopW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
u.OpenDesktopW.restype = wintypes.HANDLE
u.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, C.c_int]
u.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]

WM_COMMAND = 0x0111


def find(desktop, title):
    desk = u.OpenDesktopW(desktop, 0, False, 0x0040 | 0x0001)
    if not desk:
        raise SystemExit(f'no desktop {desktop}')
    hits = []

    @WNDENUMPROC
    def top(h, _):
        buf = C.create_unicode_buffer(512)
        u.GetWindowTextW(h, buf, 512)
        if title.lower() in buf.value.lower():
            hits.append(h)
        return True

    u.EnumDesktopWindows(desk, top, 0)
    return hits


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('ids', nargs='*', type=lambda s: int(s, 0))
    ap.add_argument('--desktop', default='bk2probe')
    ap.add_argument('--title', default='Blitzkrieg 2 Editor')
    ap.add_argument('--hwnd', type=lambda s: int(s, 0), default=0)
    ap.add_argument('--msg', type=lambda s: int(s, 0), default=WM_COMMAND)
    ap.add_argument('--wparam', type=lambda s: int(s, 0))
    ap.add_argument('--lparam', type=lambda s: int(s, 0), default=0)
    ap.add_argument('--wait', type=float, default=1.5)
    args = ap.parse_args()

    if args.hwnd:
        targets = [args.hwnd]
    else:
        targets = find(args.desktop, args.title)
        if not targets:
            raise SystemExit(f'no window titled {args.title!r} on {args.desktop}')
        print('frame(s):', ' '.join(hex(h) for h in targets))
        targets = targets[:1]

    if args.wparam is not None:
        u.PostMessageW(targets[0], args.msg, args.wparam, args.lparam)
        print(f'posted msg 0x{args.msg:x} wparam=0x{args.wparam:x} to 0x{targets[0]:x}')
        time.sleep(args.wait)
        return

    for cid in args.ids:
        u.PostMessageW(targets[0], WM_COMMAND, cid, 0)
        print(f'posted WM_COMMAND {cid} to 0x{targets[0]:x}')
        time.sleep(args.wait)


if __name__ == '__main__':
    main()
