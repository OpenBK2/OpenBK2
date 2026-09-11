"""Click a value cell in the editor's property tree and report what appeared.

The inline editors are real child windows created on demand, so clicking a row
and then listing the tree's children shows exactly which control the property
grid built and which font it was given.

Handles have to be found by walking the probe desktop in this process rather
than pasted in from an earlier dump: EnumChildWindows on a handle this process
has not reached through EnumDesktopWindows fails with ERROR_INVALID_WINDOW_HANDLE.

    python clickrow.py --title "Create New" --x 150 --y 33
"""
import argparse
import ctypes as C
import time
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
u.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
u.SendMessageTimeoutW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM,
                                  wintypes.LPARAM, wintypes.UINT, wintypes.UINT,
                                  C.POINTER(C.c_size_t)]

WM_LBUTTONDOWN, WM_LBUTTONUP, WM_GETFONT = 0x0201, 0x0202, 0x0031
CB_SHOWDROPDOWN = 0x014F
MK_LBUTTON, GWL_ID = 0x0001, -12


def name(fn, h, n=512):
    b = C.create_unicode_buffer(n)
    fn(h, b, n)
    return b.value


def font_of(h):
    out = C.c_size_t(0)
    if not u.SendMessageTimeoutW(h, WM_GETFONT, 0, 0, 0x0002, 300, C.byref(out)):
        return None
    return out.value


def describe(h, depth=0):
    r = wintypes.RECT()
    u.GetWindowRect(h, C.byref(r))
    return (depth, h, name(u.GetClassNameW, h, 256), name(u.GetWindowTextW, h)[:44],
            u.GetWindowLongPtrW(h, GWL_ID) & 0xffffffff,
            (r.left, r.top, r.right - r.left, r.bottom - r.top), font_of(h))


def walk(h, depth, rows):
    rows.append(describe(h, depth))

    @WNDENUMPROC
    def cb(c, _):
        walk(c, depth + 1, rows)
        return True

    u.EnumChildWindows(h, cb, 0)


def tree_of_desktop(desktop, title):
    desk = u.OpenDesktopW(desktop, 0, False, 0x0040 | 0x0001)
    if not desk:
        raise SystemExit(f'no desktop {desktop}')
    tops = []

    @WNDENUMPROC
    def top(h, _):
        tops.append(h)
        return True

    u.EnumDesktopWindows(desk, top, 0)
    rows = []
    for h in tops:
        walk(h, 0, rows)
    for i, row in enumerate(rows):
        if title.lower() in row[3].lower():
            # Everything nested under this window, i.e. until the depth drops back.
            out = [row]
            for later in rows[i + 1:]:
                if later[0] <= row[0]:
                    break
                out.append(later)
            return out
    raise SystemExit(f'no window titled {title!r}')


def show(rows, label):
    print(f'--- {label} ---')
    for depth, h, cls, txt, cid, rect, f in rows:
        tag = 'NO-FONT(system)' if f == 0 else ('no answer' if f is None else f'0x{f:x}')
        print(f'{tag:>18}  {"  " * depth}h=0x{h:x} {cls!r} id={cid} {rect} {txt!r}')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--desktop', default='bk2probe')
    ap.add_argument('--title', default='Create New')
    ap.add_argument('--x', type=int, default=150)
    ap.add_argument('--y', type=int, default=33)
    ap.add_argument('--drop', action='store_true', help='also drop the combo open')
    args = ap.parse_args()

    rows = tree_of_desktop(args.desktop, args.title)
    tree = next((r[1] for r in rows if r[2] == 'SysTreeView32'), 0)
    if not tree:
        show(rows, 'no tree found')
        return

    lp = (args.y << 16) | (args.x & 0xffff)
    u.PostMessageW(tree, WM_LBUTTONDOWN, MK_LBUTTON, lp)
    u.PostMessageW(tree, WM_LBUTTONUP, 0, lp)
    time.sleep(1.2)

    rows = tree_of_desktop(args.desktop, args.title)
    show(rows, f'dialog after click at tree client ({args.x},{args.y})')

    if args.drop:
        combo = next((r[1] for r in rows if r[2] == 'ComboBox'), 0)
        if combo:
            u.PostMessageW(combo, CB_SHOWDROPDOWN, 1, 0)
            time.sleep(1.0)
            # The dropped list is a top-level ComboLBox owned by the combo, so it
            # is not under the dialog and has to be found on the desktop instead.
            desk = u.OpenDesktopW(args.desktop, 0, False, 0x0040 | 0x0001)
            tops = []

            @WNDENUMPROC
            def top(h, _):
                tops.append(h)
                return True

            u.EnumDesktopWindows(desk, top, 0)
            lists = [describe(h) for h in tops
                     if name(u.GetClassNameW, h, 256) in ('ComboLBox', 'ListBox')]
            show(lists, 'dropped list')


if __name__ == '__main__':
    main()
