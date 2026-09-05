"""Read the *text* out of a running editor's tree and list controls.

Every other probe here counts items or moves a selection, because those answer
in the message result and so are safe to send into another process with nothing
allocated in it. Text is the other kind: TVM_GETITEM takes a TVITEM the control
writes through, so the struct and the buffer have to live in the target. This
does that, the way statusbar.py does it for SB_GETTEXT.

It reads each item twice, which is the point of it rather than a detail:

    -W  asks the control for UTF-16, which is what the control actually holds.
    -A  asks for the same text converted to the *process* ANSI code page, which
        is what an ANSI build like this editor sees through every ...A call.

Comparing the two says where an encoding problem is. If the W text is right and
the A text is mangled, the loss is on the way out to an ANSI caller. If the W
text is already mangled, the text was wrong before it ever reached the control,
and the usual cause is narrow UTF-8 bytes handed to a ...A function while the
process code page is something else.

    python treetext.py --id 400
    python treetext.py --id 400 --max 40
    python treetext.py --lists
"""
import argparse
import ctypes as C
import subprocess
from ctypes import wintypes

u = C.WinDLL('user32', use_last_error=True)
k = C.WinDLL('kernel32', use_last_error=True)

WNDENUMPROC = C.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
u.EnumWindows.argtypes = [WNDENUMPROC, wintypes.LPARAM]
u.EnumChildWindows.argtypes = [wintypes.HWND, WNDENUMPROC, wintypes.LPARAM]
u.GetClassNameW.argtypes = [wintypes.HWND, wintypes.LPWSTR, C.c_int]
u.GetDlgCtrlID.argtypes = [wintypes.HWND]
u.GetWindowThreadProcessId.argtypes = [wintypes.HWND, C.POINTER(wintypes.DWORD)]
u.SendMessageTimeoutW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM,
                                  wintypes.LPARAM, wintypes.UINT, wintypes.UINT,
                                  C.POINTER(C.c_size_t)]
k.OpenProcess.restype = wintypes.HANDLE
k.VirtualAllocEx.restype = wintypes.LPVOID
k.VirtualAllocEx.argtypes = [wintypes.HANDLE, wintypes.LPVOID, C.c_size_t,
                             wintypes.DWORD, wintypes.DWORD]
k.VirtualFreeEx.argtypes = [wintypes.HANDLE, wintypes.LPVOID, C.c_size_t, wintypes.DWORD]
k.WriteProcessMemory.argtypes = [wintypes.HANDLE, wintypes.LPVOID, wintypes.LPCVOID,
                                 C.c_size_t, C.POINTER(C.c_size_t)]
k.ReadProcessMemory.argtypes = [wintypes.HANDLE, wintypes.LPCVOID, wintypes.LPVOID,
                                C.c_size_t, C.POINTER(C.c_size_t)]

TV_FIRST = 0x1100
TVM_GETNEXTITEM = TV_FIRST + 10
TVM_GETITEMA = TV_FIRST + 12
TVM_GETITEMW = TV_FIRST + 62
TVGN_ROOT = 0
TVGN_NEXT = 1
TVGN_CHILD = 4
TVIF_TEXT = 0x0001
TVIF_HANDLE = 0x0010

LVM_FIRST = 0x1000
LVM_GETITEMCOUNT = LVM_FIRST + 4
LVM_GETITEMTEXTA = LVM_FIRST + 45
LVM_GETITEMTEXTW = LVM_FIRST + 115
LVIF_TEXT = 0x0001

SMTO_ABORTIFHUNG = 0x0002
PROCESS_ALL = 0x1F0FFF
MEM_COMMIT = 0x1000
MEM_RELEASE = 0x8000
PAGE_READWRITE = 4
TEXT_CHARS = 512


class TVITEM(C.Structure):
    _fields_ = [('mask', wintypes.UINT), ('hItem', C.c_void_p),
                ('state', wintypes.UINT), ('stateMask', wintypes.UINT),
                ('pszText', C.c_void_p), ('cchTextMax', C.c_int),
                ('iImage', C.c_int), ('iSelectedImage', C.c_int),
                ('cChildren', C.c_int), ('lParam', wintypes.LPARAM)]


class LVITEM(C.Structure):
    _fields_ = [('mask', wintypes.UINT), ('iItem', C.c_int), ('iSubItem', C.c_int),
                ('state', wintypes.UINT), ('stateMask', wintypes.UINT),
                ('pszText', C.c_void_p), ('cchTextMax', C.c_int),
                ('iImage', C.c_int), ('lParam', wintypes.LPARAM),
                ('iIndent', C.c_int), ('iGroupId', C.c_int),
                ('cColumns', wintypes.UINT), ('puColumns', C.c_void_p),
                ('piColFmt', C.c_void_p), ('iGroup', C.c_int)]


def ask(hwnd, msg, wparam=0, lparam=0, timeout=5000):
    out = C.c_size_t(0)
    if not u.SendMessageTimeoutW(hwnd, msg, wparam, lparam, SMTO_ABORTIFHUNG,
                                 timeout, C.byref(out)):
        return None
    return out.value


def pids_for(image):
    txt = subprocess.run(['tasklist', '/FI', 'IMAGENAME eq ' + image, '/FO', 'CSV', '/NH'],
                         capture_output=True, text=True).stdout
    return {int(l.split('","')[1]) for l in txt.splitlines() if l.startswith('"')}


def find(image, want_class, ctrl_id):
    want = pids_for(image)
    hits = []

    def child(h, _):
        cls = C.create_unicode_buffer(64)
        u.GetClassNameW(h, cls, 64)
        if cls.value == want_class and (ctrl_id is None or u.GetDlgCtrlID(h) == ctrl_id):
            hits.append(h)
        return True

    def top(h, _):
        pid = wintypes.DWORD()
        u.GetWindowThreadProcessId(h, C.byref(pid))
        if pid.value in want:
            u.EnumChildWindows(h, WNDENUMPROC(child), 0)
        return True

    u.EnumWindows(WNDENUMPROC(top), 0)
    return hits


class Remote(object):
    """A scratch block in the target: the struct, then the text buffer."""

    def __init__(self, hwnd, struct_size):
        pid = wintypes.DWORD()
        u.GetWindowThreadProcessId(hwnd, C.byref(pid))
        self.proc = k.OpenProcess(PROCESS_ALL, False, pid.value)
        if not self.proc:
            raise SystemExit('OpenProcess failed: %d' % C.get_last_error())
        self.size = struct_size + 8 + TEXT_CHARS * 2
        self.base = k.VirtualAllocEx(self.proc, None, self.size, MEM_COMMIT, PAGE_READWRITE)
        if not self.base:
            raise SystemExit('VirtualAllocEx failed: %d' % C.get_last_error())
        self.text = self.base + struct_size + 8

    def write(self, struct):
        n = C.c_size_t(0)
        k.WriteProcessMemory(self.proc, self.base, C.byref(struct),
                             C.sizeof(struct), C.byref(n))

    def read_text(self, wide):
        buf = (C.c_char * (TEXT_CHARS * 2))()
        n = C.c_size_t(0)
        k.ReadProcessMemory(self.proc, self.text, buf, len(buf), C.byref(n))
        raw = bytes(buf)
        if wide:
            end = raw.find(b'\x00\x00')
            while end > 0 and end % 2:
                end = raw.find(b'\x00\x00', end + 1)
            cut = raw[:max(end, 0)]
            return cut, cut.decode('utf-16-le', 'replace'), 'utf-16'
        end = raw.find(b'\x00')
        raw = raw[:end if end >= 0 else len(raw)]
        # These bytes are in the *target's* code page, which is not this
        # process's: the editor declares UTF-8 in its manifest and this script
        # runs under whatever the system says. Decoding them as mbcs here would
        # report the probe's code page as if it were the editor's, so UTF-8 is
        # tried first and the fallback is named.
        try:
            return raw, raw.decode('utf-8'), 'utf-8'
        except UnicodeDecodeError:
            return raw, raw.decode('mbcs', 'replace'), 'mbcs'

    def close(self):
        if self.base:
            k.VirtualFreeEx(self.proc, self.base, 0, MEM_RELEASE)
        self.base = None


def describe(raw, text):
    """Show the text, and the bytes too when the text is not plain ASCII.

    Judged on the decoded text rather than on the bytes: a UTF-16 read is full
    of nulls whatever it says, so testing the bytes marks every wide read as
    interesting and hides the ones that are.
    """
    if all(' ' <= ch <= '~' for ch in text):
        return repr(text)
    return '%-40s  %s' % (repr(text), raw.hex(' '))


def walk_tree(hwnd, limit):
    items, stack = [], []
    item = ask(hwnd, TVM_GETNEXTITEM, TVGN_ROOT, 0)
    while item and len(items) < limit:
        items.append(item)
        child = ask(hwnd, TVM_GETNEXTITEM, TVGN_CHILD, item)
        nxt = ask(hwnd, TVM_GETNEXTITEM, TVGN_NEXT, item)
        if nxt:
            stack.append(nxt)
        if child:
            item = child
        elif stack:
            item = stack.pop()
        else:
            item = None
    return items


def dump_tree(hwnd, limit):
    print('== SysTreeView32 0x%X id=%d' % (hwnd, u.GetDlgCtrlID(hwnd)))
    mem = Remote(hwnd, C.sizeof(TVITEM))
    try:
        for hItem in walk_tree(hwnd, limit):
            row = []
            for wide, msg in ((True, TVM_GETITEMW), (False, TVM_GETITEMA)):
                item = TVITEM()
                item.mask = TVIF_TEXT | TVIF_HANDLE
                item.hItem = hItem
                item.pszText = mem.text
                item.cchTextMax = TEXT_CHARS
                mem.write(item)
                ask(hwnd, msg, 0, mem.base)
                row.append(mem.read_text(wide))
            (rawW, txtW, _), (rawA, txtA, encA) = row
            flag = '  <-- differs' if txtW != txtA else ''
            print('   W       %s' % describe(rawW, txtW))
            print('   A %-5s %s%s' % (encA, describe(rawA, txtA), flag))
    finally:
        mem.close()


def dump_list(hwnd, limit):
    count = ask(hwnd, LVM_GETITEMCOUNT) or 0
    if count == 0:
        return
    print('== SysListView32 0x%X id=%d items=%d' % (hwnd, u.GetDlgCtrlID(hwnd), count))
    mem = Remote(hwnd, C.sizeof(LVITEM))
    try:
        for i in range(min(count, limit)):
            row = []
            for wide, msg in ((True, LVM_GETITEMTEXTW), (False, LVM_GETITEMTEXTA)):
                item = LVITEM()
                item.mask = LVIF_TEXT
                item.iItem = i
                item.iSubItem = 0
                item.pszText = mem.text
                item.cchTextMax = TEXT_CHARS
                mem.write(item)
                ask(hwnd, msg, i, mem.base)
                row.append(mem.read_text(wide))
            (rawW, txtW, _), (rawA, txtA, encA) = row
            flag = '  <-- differs' if txtW != txtA else ''
            print('   W       %s' % describe(rawW, txtW))
            print('   A %-5s %s%s' % (encA, describe(rawA, txtA), flag))
    finally:
        mem.close()


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--image', default='B2_MapEditor.exe')
    ap.add_argument('--id', type=int, help='one control id; every tree if omitted')
    ap.add_argument('--max', type=int, default=25, help='items per control')
    ap.add_argument('--lists', action='store_true', help='list views as well as trees')
    args = ap.parse_args()

    for hwnd in find(args.image, 'SysTreeView32', args.id):
        dump_tree(hwnd, args.max)
    if args.lists:
        for hwnd in find(args.image, 'SysListView32', args.id):
            dump_list(hwnd, args.max)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
