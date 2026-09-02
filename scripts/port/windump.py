"""Dump a running Win32 program's window tree and menus, as JSON and as a tree.

Spy++ in a hundred lines, minus the GUI. Nothing to build and nothing to attach:
EnumWindows and EnumChildWindows read another process's window tree fine, since
windows are kernel objects and their class, text, styles and rectangles are all
readable without opening the process.

    python windump.py B2_MapEditor.exe [out.json]
"""
import ctypes as C
import json
import sys
from ctypes import wintypes

u = C.WinDLL('user32', use_last_error=True)
k = C.WinDLL('kernel32', use_last_error=True)

WNDENUMPROC = C.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
u.EnumWindows.argtypes = [WNDENUMPROC, wintypes.LPARAM]
u.EnumChildWindows.argtypes = [wintypes.HWND, WNDENUMPROC, wintypes.LPARAM]
u.GetClassNameW.argtypes = [wintypes.HWND, wintypes.LPWSTR, C.c_int]
u.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, C.c_int]
u.GetWindowLongPtrW.argtypes = [wintypes.HWND, C.c_int]
u.GetWindowLongPtrW.restype = C.c_ssize_t
u.GetWindowRect.argtypes = [wintypes.HWND, C.POINTER(wintypes.RECT)]
u.GetWindowThreadProcessId.argtypes = [wintypes.HWND, C.POINTER(wintypes.DWORD)]
u.GetMenu.argtypes = [wintypes.HWND]
u.GetMenu.restype = wintypes.HMENU
u.GetMenuItemCount.argtypes = [wintypes.HMENU]
u.GetSubMenu.argtypes = [wintypes.HMENU, C.c_int]
u.GetSubMenu.restype = wintypes.HMENU
u.GetMenuStringW.argtypes = [wintypes.HMENU, wintypes.UINT, wintypes.LPWSTR, C.c_int, wintypes.UINT]
u.GetMenuItemID.argtypes = [wintypes.HMENU, C.c_int]

GWL_STYLE, GWL_EXSTYLE, GWL_ID = -16, -20, -12
MF_BYPOSITION = 0x400

WS = [(0x80000000, 'POPUP'), (0x40000000, 'CHILD'), (0x20000000, 'MINIMIZE'),
      (0x10000000, 'VISIBLE'), (0x08000000, 'DISABLED'), (0x04000000, 'CLIPSIBLINGS'),
      (0x02000000, 'CLIPCHILDREN'), (0x01000000, 'MAXIMIZE'), (0x00800000, 'BORDER'),
      (0x00400000, 'DLGFRAME'), (0x00200000, 'VSCROLL'), (0x00100000, 'HSCROLL'),
      (0x00080000, 'SYSMENU'), (0x00040000, 'THICKFRAME'), (0x00020000, 'GROUP'),
      (0x00010000, 'TABSTOP')]

# The low word of a control bar's style is MFC's, not Windows'.
CBRS = [(0x0001, 'ALIGN_LEFT'), (0x0002, 'ALIGN_TOP'), (0x0004, 'ALIGN_RIGHT'),
        (0x0008, 'ALIGN_BOTTOM'), (0x0040, 'BORDER_TOP'), (0x0080, 'BORDER_BOTTOM'),
        (0x0100, 'BORDER_LEFT'), (0x0200, 'BORDER_RIGHT'), (0x0400, 'TOOLTIPS'),
        (0x0800, 'FLYBY'), (0x1000, 'FLOAT_MULTI'), (0x2000, 'BORDER_3D'),
        (0x4000, 'HIDE_INPLACE'), (0x8000, 'SIZE_DYNAMIC'), (0x10000, 'SIZE_FIXED'),
        (0x20000, 'FLOATING')]


def names(value, table):
    return [n for bit, n in table if value & bit]


def text(fn, hwnd, size=512):
    buf = C.create_unicode_buffer(size)
    fn(hwnd, buf, size)
    return buf.value


def describe(hwnd):
    rect = wintypes.RECT()
    u.GetWindowRect(hwnd, C.byref(rect))
    style = u.GetWindowLongPtrW(hwnd, GWL_STYLE) & 0xFFFFFFFF
    cls = text(u.GetClassNameW, hwnd, 256)
    out = {
        'hwnd': '0x%X' % hwnd,
        'class': cls,
        'text': text(u.GetWindowTextW, hwnd),
        'id': u.GetWindowLongPtrW(hwnd, GWL_ID) & 0xFFFFFFFF,
        'rect': [rect.left, rect.top, rect.right, rect.bottom],
        'size': [rect.right - rect.left, rect.bottom - rect.top],
        'style': '0x%08X' % style,
        'styles': names(style, WS),
        'exstyle': '0x%08X' % (u.GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & 0xFFFFFFFF),
        'children': [],
    }
    # An MFC control bar keeps CBRS_ flags in m_dwStyle, not in the window, but
    # the low word is worth decoding for anything that looks like one.
    if 'Afx' in cls or 'ControlBar' in cls:
        out['low_word_as_cbrs'] = names(style & 0xFFFF, CBRS)
    return out


def menu(hmenu, depth=0):
    if not hmenu or depth > 3:
        return []
    items = []
    for i in range(u.GetMenuItemCount(hmenu)):
        buf = C.create_unicode_buffer(256)
        u.GetMenuStringW(hmenu, i, buf, 256, MF_BYPOSITION)
        sub = u.GetSubMenu(hmenu, i)
        items.append({'text': buf.value, 'id': u.GetMenuItemID(hmenu, i) & 0xFFFFFFFF,
                      'items': menu(sub, depth + 1)})
    return items


def children(hwnd):
    found = []

    @WNDENUMPROC
    def cb(child, _):
        found.append(child)
        return True

    u.EnumChildWindows(hwnd, cb, 0)
    return found


def tree(hwnd, seen):
    node = describe(hwnd)
    seen.add(hwnd)
    for child in children(hwnd):
        if child in seen:
            continue
        # only direct children; EnumChildWindows is recursive
        if u.GetAncestor(child, 1) != hwnd:
            continue
        node['children'].append(tree(child, seen))
    return node


def main():
    want = (sys.argv[1] if len(sys.argv) > 1 else 'B2_MapEditor.exe').lower()
    out_path = sys.argv[2] if len(sys.argv) > 2 else None

    u.GetAncestor.argtypes = [wintypes.HWND, wintypes.UINT]
    u.GetAncestor.restype = wintypes.HWND

    import subprocess
    pids = set()
    csv = subprocess.run(['tasklist', '/FO', 'CSV', '/NH'], capture_output=True, text=True).stdout
    for line in csv.splitlines():
        parts = [p.strip('"') for p in line.split('","')]
        if len(parts) >= 2 and parts[0].lower() == want:
            pids.add(int(parts[1]))
    if not pids:
        print('%s is not running' % want)
        return 1
    print('pid(s): %s' % sorted(pids))

    tops = []

    @WNDENUMPROC
    def cb(hwnd, _):
        pid = wintypes.DWORD()
        u.GetWindowThreadProcessId(hwnd, C.byref(pid))
        if pid.value in pids:
            tops.append(hwnd)
        return True

    u.EnumWindows(cb, 0)

    roots = []
    for hwnd in tops:
        node = tree(hwnd, set())
        node['menu'] = menu(u.GetMenu(hwnd))
        roots.append(node)

    def show(node, depth=0):
        flags = ','.join(node['styles'][:4])
        print('%s%-30s id=%-6s %4dx%-4d %-28s %s'
              % ('  ' * depth, node['class'][:30], node['id'],
                 node['size'][0], node['size'][1], flags, node['text'][:34]))
        for c in node['children']:
            show(c, depth + 1)

    for r in roots:
        show(r)
        if r['menu']:
            print('  MENU: %s' % ' | '.join(i['text'].replace('\n', ' ') or '(sep)'
                                            for i in r['menu']))
        else:
            print('  MENU: none')
    if out_path:
        json.dump(roots, open(out_path, 'w', encoding='utf-8'), indent=1)
        print('\nwrote %s' % out_path)
    return 0


sys.exit(main())
