"""Press a key in a window on the probe desktop with its modifiers stated, and
read an editor's selection.

    python simkey.py "Script Editor" key F --ctrl       # Ctrl+F: Find
    python simkey.py "Script Editor" key H --ctrl       # Ctrl+H: Replace
    python simkey.py "Script Editor" key F3             # find again
    python simkey.py "Script Editor" sel                # selection start and end

A posted WM_KEYDOWN reaches a window on another desktop but says nothing about
Ctrl: the editor asks GetAsyncKeyState or GetKeyState, and SendInput, the only
way to set those, is refused off the input desktop. So a window with shortcuts
worth testing handles the registered message OBK2.SimulatedKey and runs the
same dispatch its key handler runs, with the modifiers this sends. See
MapEditorLib/SimulatedKey.h for the message itself.

The title is matched as a substring of a visible top-level caption. The message
is sent, not posted, so the answer comes back -- which is safe for these
shortcuts because what they open is modeless; a shortcut that opened something
modal would hold the send until it closed, and the timeout covers that.
"""
import argparse
import ctypes as C
import sys
from ctypes import wintypes

u = C.WinDLL('user32', use_last_error=True)
WNDENUMPROC = C.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
u.OpenDesktopW.restype = wintypes.HANDLE
u.EnumDesktopWindows.argtypes = [wintypes.HANDLE, WNDENUMPROC, wintypes.LPARAM]
u.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, C.c_int]
u.RegisterWindowMessageW.argtypes = [wintypes.LPCWSTR]
u.RegisterWindowMessageW.restype = wintypes.UINT
u.SendMessageTimeoutW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM,
                                  wintypes.LPARAM, wintypes.UINT, wintypes.UINT,
                                  C.POINTER(C.c_size_t)]

OP_KEY, OP_SELECTION_START, OP_SELECTION_END = 1, 2, 3
MODIFIER_CONTROL, MODIFIER_SHIFT = 0x0001, 0x0002
SMTO_ABORTIFHUNG = 0x0002


def find_window(desktop, title):
    desk = u.OpenDesktopW(desktop, 0, False, 0x0040 | 0x0001)
    if not desk:
        raise SystemExit('cannot open desktop %s' % desktop)
    hits = []

    @WNDENUMPROC
    def top(h, _):
        if u.IsWindowVisible(h):
            buf = C.create_unicode_buffer(512)
            u.GetWindowTextW(h, buf, 512)
            if title in buf.value:
                hits.append((h, buf.value))
        return True

    u.EnumDesktopWindows(desk, top, 0)
    if not hits:
        raise SystemExit('no visible window with %r in its caption' % title)
    if len(hits) > 1:
        print('several match, using the first: %s' % [t for _, t in hits])
    return hits[0]


def virtual_key(name):
    if name.lower().startswith('0x'):
        return int(name, 16)
    if len(name) == 1 and name.isalnum():
        return ord(name.upper())
    if name.upper().startswith('F') and name[1:].isdigit() and 1 <= int(name[1:]) <= 24:
        return 0x70 + int(name[1:]) - 1
    raise SystemExit('unknown key %r: a letter or digit, F1..F24, or 0x hex' % name)


def send(hwnd, wparam, lparam, timeout=5000):
    msg = u.RegisterWindowMessageW('OBK2.SimulatedKey')
    out = C.c_size_t(0)
    if not u.SendMessageTimeoutW(hwnd, msg, wparam, lparam, SMTO_ABORTIFHUNG, timeout, C.byref(out)):
        raise SystemExit('send failed or timed out: %d' % C.get_last_error())
    return out.value


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('title')
    ap.add_argument('--desktop', default='bk2probe')
    sub = ap.add_subparsers(dest='op', required=True)
    k = sub.add_parser('key')
    k.add_argument('key')
    k.add_argument('--ctrl', action='store_true')
    k.add_argument('--shift', action='store_true')
    sub.add_parser('sel')
    args = ap.parse_args()

    hwnd, caption = find_window(args.desktop, args.title)
    if args.op == 'key':
        mods = (MODIFIER_CONTROL if args.ctrl else 0) | (MODIFIER_SHIFT if args.shift else 0)
        vk = virtual_key(args.key)
        handled = send(hwnd, OP_KEY, (mods << 16) | vk)
        print('%s%s%s -> %r: %s' % ('Ctrl+' if args.ctrl else '', 'Shift+' if args.shift else '',
                                    args.key, caption,
                                    'handled' if handled else 'not handled (no handler for the message)'))
        sys.exit(0 if handled else 1)
    start = send(hwnd, OP_SELECTION_START, 0)
    end = send(hwnd, OP_SELECTION_END, 0)
    print('selection %d..%d' % (start, end))


if __name__ == '__main__':
    main()
