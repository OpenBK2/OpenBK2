"""Count the items in a running program's tree and list controls.

The window dump proves a SysTreeView32 exists; this proves whether anything
landed in it, which is the question that decides whether forwarding
SEC_TREECLASS to the common control actually works.

TVM_GETCOUNT and LVM_GETITEMCOUNT return their answer as the message result
rather than through a caller's buffer, so unlike SB_GETTEXT they are safe to
send across a process boundary with nothing allocated in the target.

    python treecount.py B2_MapEditor.exe
"""
import ctypes as C
import subprocess
import sys
from ctypes import wintypes

u = C.WinDLL('user32', use_last_error=True)

WNDENUMPROC = C.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
u.EnumWindows.argtypes = [WNDENUMPROC, wintypes.LPARAM]
u.EnumChildWindows.argtypes = [wintypes.HWND, WNDENUMPROC, wintypes.LPARAM]
u.GetClassNameW.argtypes = [wintypes.HWND, wintypes.LPWSTR, C.c_int]
u.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, C.c_int]
u.GetWindowThreadProcessId.argtypes = [wintypes.HWND, C.POINTER(wintypes.DWORD)]
u.GetWindowLongPtrW.argtypes = [wintypes.HWND, C.c_int]
u.GetWindowLongPtrW.restype = C.c_ssize_t
u.GetWindowRect.argtypes = [wintypes.HWND, C.POINTER(wintypes.RECT)]
u.SendMessageTimeoutW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM,
                                  wintypes.LPARAM, wintypes.UINT, wintypes.UINT,
                                  C.POINTER(C.c_size_t)]

GWL_ID = -12
TVM_GETCOUNT = 0x1100 + 5
TVM_GETVISIBLECOUNT = 0x1100 + 16
LVM_GETITEMCOUNT = 0x1000 + 4
SMTO_ABORTIFHUNG = 0x0002

WANTED = {'SysTreeView32': [('items', TVM_GETCOUNT), ('visible', TVM_GETVISIBLECOUNT)],
          'SysListView32': [('items', LVM_GETITEMCOUNT)]}


def pids_for(exe):
    out = subprocess.run(['tasklist', '/FI', f'IMAGENAME eq {exe}', '/FO', 'CSV', '/NH'],
                         capture_output=True, text=True).stdout
    return [int(line.split('","')[1]) for line in out.splitlines() if line.startswith('"')]


def ask(hwnd, msg):
    """Message result only, with a timeout so a busy editor cannot hang this."""
    out = C.c_size_t()
    if not u.SendMessageTimeoutW(hwnd, msg, 0, 0, SMTO_ABORTIFHUNG, 5000, C.byref(out)):
        return None
    return C.c_ssize_t(out.value).value


def main():
    exe = sys.argv[1] if len(sys.argv) > 1 else 'B2_MapEditor.exe'
    pids = pids_for(exe)
    if not pids:
        sys.exit(f'{exe} is not running')

    found = []

    def visit(hwnd, _):
        cls = C.create_unicode_buffer(256)
        u.GetClassNameW(hwnd, cls, 256)
        if cls.value in WANTED:
            found.append((hwnd, cls.value))
        return True

    def top(hwnd, _):
        pid = wintypes.DWORD()
        u.GetWindowThreadProcessId(hwnd, C.byref(pid))
        if pid.value in pids:
            u.EnumChildWindows(hwnd, WNDENUMPROC(visit), 0)
        return True

    u.EnumWindows(WNDENUMPROC(top), 0)
    if not found:
        sys.exit('no tree or list controls found')

    total = 0
    for hwnd, cls in found:
        nid = u.GetWindowLongPtrW(hwnd, GWL_ID)
        title = C.create_unicode_buffer(256)
        u.GetWindowTextW(hwnd, title, 256)
        rect = wintypes.RECT()
        u.GetWindowRect(hwnd, C.byref(rect))
        counts = ' '.join(f'{name}={ask(hwnd, msg)}' for name, msg in WANTED[cls])
        first = ask(hwnd, WANTED[cls][0][1])
        total += first if isinstance(first, int) and first > 0 else 0
        print(f'{cls:<14} id={nid:<6} '
              f'{rect.right - rect.left}x{rect.bottom - rect.top:<5} {counts} {title.value!r}')
    print(f'-- {len(found)} controls, {total} items in total')


if __name__ == '__main__':
    main()
