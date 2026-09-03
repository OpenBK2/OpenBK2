"""Read a running program's status bar panes: how many, how wide, what text.

The window dump says a status bar exists but not what is in it, and the pane bug
this was written for -- two SetPaneInfo calls landing on the same pane -- is
invisible from the outside until you ask for the parts.

SB_GETPARTS and SB_GETTEXT take a pointer to a buffer the control fills in, and
that pointer is interpreted in the *target* process: unlike WM_GETTEXT, the
window manager does not marshal these. Passing an address from this process
therefore does not fail, it makes the status bar scribble over whatever lives at
that address in the editor, which corrupted and then killed the editor the first
time this script was written that way. So the buffer is allocated in the target
with VirtualAllocEx and read back with ReadProcessMemory.

    python statusbar.py B2_MapEditor.exe
"""
import ctypes as C
import subprocess
import sys
from ctypes import wintypes

u = C.WinDLL('user32', use_last_error=True)
k = C.WinDLL('kernel32', use_last_error=True)

WNDENUMPROC = C.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
u.EnumWindows.argtypes = [WNDENUMPROC, wintypes.LPARAM]
u.EnumChildWindows.argtypes = [wintypes.HWND, WNDENUMPROC, wintypes.LPARAM]
u.GetClassNameW.argtypes = [wintypes.HWND, wintypes.LPWSTR, C.c_int]
u.GetWindowThreadProcessId.argtypes = [wintypes.HWND, C.POINTER(wintypes.DWORD)]
u.GetWindowLongPtrW.argtypes = [wintypes.HWND, C.c_int]
u.GetWindowLongPtrW.restype = C.c_ssize_t
u.GetWindowRect.argtypes = [wintypes.HWND, C.POINTER(wintypes.RECT)]
u.SendMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
u.SendMessageW.restype = C.c_ssize_t

k.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
k.OpenProcess.restype = wintypes.HANDLE
k.VirtualAllocEx.argtypes = [wintypes.HANDLE, wintypes.LPVOID, C.c_size_t,
                             wintypes.DWORD, wintypes.DWORD]
k.VirtualAllocEx.restype = wintypes.LPVOID
k.VirtualFreeEx.argtypes = [wintypes.HANDLE, wintypes.LPVOID, C.c_size_t, wintypes.DWORD]
k.ReadProcessMemory.argtypes = [wintypes.HANDLE, wintypes.LPCVOID, wintypes.LPVOID,
                                C.c_size_t, C.POINTER(C.c_size_t)]

GWL_ID = -12
SB_GETPARTS = 0x0406
SB_GETTEXTW = 0x040D

PROCESS_VM = 0x0008 | 0x0010 | 0x0020 | 0x0400  # READ | OPERATION | WRITE | QUERY_INFO
MEM_COMMIT, MEM_RESERVE, MEM_RELEASE = 0x1000, 0x2000, 0x8000
PAGE_READWRITE = 0x04
BUF = 4096


def pids_for(exe):
    out = subprocess.run(['tasklist', '/FI', f'IMAGENAME eq {exe}', '/FO', 'CSV', '/NH'],
                         capture_output=True, text=True).stdout
    return [int(line.split('","')[1]) for line in out.splitlines() if line.startswith('"')]


def find_status_bars(pids):
    """Every msctls_statusbar32 under a top-level window of those processes."""
    bars = []

    def visit(hwnd, _):
        buf = C.create_unicode_buffer(256)
        u.GetClassNameW(hwnd, buf, 256)
        if buf.value == 'msctls_statusbar32':
            pid = wintypes.DWORD()
            u.GetWindowThreadProcessId(hwnd, C.byref(pid))
            bars.append((hwnd, pid.value))
        return True

    def top(hwnd, _):
        pid = wintypes.DWORD()
        u.GetWindowThreadProcessId(hwnd, C.byref(pid))
        if pid.value in pids:
            visit(hwnd, 0)
            u.EnumChildWindows(hwnd, WNDENUMPROC(visit), 0)
        return True

    u.EnumWindows(WNDENUMPROC(top), 0)
    return bars


class Remote:
    """A scratch buffer inside another process, for messages that take a pointer."""

    def __init__(self, pid):
        self.h = k.OpenProcess(PROCESS_VM, False, pid)
        if not self.h:
            raise OSError(f'OpenProcess({pid}) failed: {C.get_last_error()}')
        self.addr = k.VirtualAllocEx(self.h, None, BUF, MEM_COMMIT | MEM_RESERVE,
                                     PAGE_READWRITE)
        if not self.addr:
            raise OSError(f'VirtualAllocEx failed: {C.get_last_error()}')

    def read(self, size):
        local = (C.c_char * size)()
        got = C.c_size_t()
        if not k.ReadProcessMemory(self.h, self.addr, local, size, C.byref(got)):
            raise OSError(f'ReadProcessMemory failed: {C.get_last_error()}')
        return bytes(local[:got.value])

    def close(self):
        if self.addr:
            k.VirtualFreeEx(self.h, self.addr, 0, MEM_RELEASE)
        if self.h:
            k.CloseHandle(self.h)


def main():
    exe = sys.argv[1] if len(sys.argv) > 1 else 'B2_MapEditor.exe'
    pids = pids_for(exe)
    if not pids:
        sys.exit(f'{exe} is not running')

    bars = find_status_bars(pids)
    if not bars:
        sys.exit('no msctls_statusbar32 found -- the status bar is not a CStatusBar')

    for hwnd, pid in bars:
        nid = u.GetWindowLongPtrW(hwnd, GWL_ID)
        rect = wintypes.RECT()
        u.GetWindowRect(hwnd, C.byref(rect))
        count = u.SendMessageW(hwnd, SB_GETPARTS, 0, 0)
        print(f'status bar 0x{hwnd:X} id={nid} '
              f'window={rect.right - rect.left}x{rect.bottom - rect.top} parts={count}')
        if count <= 0:
            continue

        mem = Remote(pid)
        try:
            u.SendMessageW(hwnd, SB_GETPARTS, count, mem.addr)
            edges = list(C.cast(mem.read(4 * count), C.POINTER(C.c_int))[:count])
            prev = 0
            for i in range(count):
                n = u.SendMessageW(hwnd, SB_GETTEXTW, i, mem.addr)
                text = mem.read(2 * ((n & 0xFFFF) + 1)).decode('utf-16-le', 'replace')
                text = text.split('\x00')[0]
                right = edges[i]
                width = 'stretch' if right == -1 else right - prev
                print(f'  pane {i}: right={right} width={width} text={text!r}')
                if right != -1:
                    prev = right
        finally:
            mem.close()


if __name__ == '__main__':
    main()
