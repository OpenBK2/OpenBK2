"""Report which controls of a running Win32 program actually have a font set.

WM_GETFONT answers with an HFONT, and a GDI handle belongs to the process that
made it, so its LOGFONT cannot be read from here. The handle *value* still
answers the only question that matters: NULL means the control was never sent
WM_SETFONT and is drawing in the stock system font, and two controls sharing a
value are drawing in the same font object. That separates "the font is wrong"
from "there is no font".

    python fontdump.py --desktop bk2probe [--filter Combo]
"""
import argparse
import ctypes as C
from ctypes import wintypes

u = C.WinDLL('user32', use_last_error=True)

WNDENUMPROC = C.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
u.EnumDesktopWindows.argtypes = [wintypes.HANDLE, WNDENUMPROC, wintypes.LPARAM]
u.EnumChildWindows.argtypes = [wintypes.HWND, WNDENUMPROC, wintypes.LPARAM]
u.OpenDesktopW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
u.OpenDesktopW.restype = wintypes.HANDLE
u.GetClassNameW.argtypes = [wintypes.HWND, wintypes.LPWSTR, C.c_int]
u.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, C.c_int]
u.GetWindowRect.argtypes = [wintypes.HWND, C.POINTER(wintypes.RECT)]
u.GetWindowLongPtrW.argtypes = [wintypes.HWND, C.c_int]
u.GetWindowLongPtrW.restype = C.c_ssize_t
u.SendMessageTimeoutW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM,
                                  wintypes.LPARAM, wintypes.UINT, wintypes.UINT,
                                  C.POINTER(C.c_size_t)]

WM_GETFONT = 0x0031
SMTO_ABORTIFHUNG = 0x0002
GWL_STYLE, GWL_ID = -16, -12
DESKTOP_ENUMERATE, DESKTOP_READOBJECTS = 0x0040, 0x0001


def text(fn, hwnd, n=512):
    buf = C.create_unicode_buffer(n)
    fn(hwnd, buf, n)
    return buf.value


def font_of(hwnd):
    out = C.c_size_t(0)
    if not u.SendMessageTimeoutW(hwnd, WM_GETFONT, 0, 0, SMTO_ABORTIFHUNG, 300,
                                 C.byref(out)):
        return None          # the window did not answer in time
    return out.value


def walk(hwnd, depth, rows):
    r = wintypes.RECT()
    u.GetWindowRect(hwnd, C.byref(r))
    rows.append({
        'depth': depth,
        'hwnd': hwnd,
        'cls': text(u.GetClassNameW, hwnd, 256),
        'txt': text(u.GetWindowTextW, hwnd)[:48],
        'id': u.GetWindowLongPtrW(hwnd, GWL_ID) & 0xffffffff,
        'vis': bool(u.GetWindowLongPtrW(hwnd, GWL_STYLE) & 0x10000000),
        'rect': (r.left, r.top, r.right - r.left, r.bottom - r.top),
        'font': font_of(hwnd),
    })

    @WNDENUMPROC
    def child(h, _):
        walk(h, depth + 1, rows)
        return True

    u.EnumChildWindows(hwnd, child, 0)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--desktop', default='bk2probe')
    ap.add_argument('--filter', default='', help='only print rows whose class or text matches')
    args = ap.parse_args()

    desk = u.OpenDesktopW(args.desktop, 0, False,
                          DESKTOP_ENUMERATE | DESKTOP_READOBJECTS)
    if not desk:
        raise SystemExit(f'no desktop {args.desktop}: {C.get_last_error()}')

    tops = []

    @WNDENUMPROC
    def top(h, _):
        tops.append(h)
        return True

    u.EnumDesktopWindows(desk, top, 0)

    rows = []
    for h in tops:
        walk(h, 0, rows)

    # A font value seen on many controls is the dialog's; name the common ones so
    # the odd control out is obvious at a glance.
    counts = {}
    for row in rows:
        if row['font']:
            counts[row['font']] = counts.get(row['font'], 0) + 1
    names = {f: f'F{i}' for i, (f, _) in
             enumerate(sorted(counts.items(), key=lambda kv: -kv[1]))}
    for f, n in sorted(counts.items(), key=lambda kv: -kv[1]):
        print(f'{names[f]} = 0x{f:x}  used by {n} controls')
    print()

    for row in rows:
        line = (f'{"  " * row["depth"]}h=0x{row["hwnd"]:x} {row["cls"]!r} id={row["id"]} '
                f'{"vis" if row["vis"] else "HID"} {row["rect"]} {row["txt"]!r}')
        if args.filter and args.filter.lower() not in line.lower():
            continue
        f = row['font']
        tag = 'NO-FONT(system)' if f == 0 else ('no answer' if f is None else names[f])
        print(f'{tag:>16}  {line}')


if __name__ == '__main__':
    main()
