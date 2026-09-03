"""Toggle a running MFC frame's control bars, so a window dump can be taken of
bars a saved layout left hidden.

The editor's toolbars and shortcut bars carry ids in MFC's control bar range,
AFX_IDW_CONTROLBAR_FIRST..LAST (0xE800..0xE8FF). CFrameWnd maps that whole range
to OnBarCheck with ON_COMMAND_EX_RANGE, so posting WM_COMMAND with a bar's id
shows or hides that bar exactly as the View menu would. Nothing here is specific
to this editor beyond the list of ids to try.

    python winbars.py B2_MapEditor.exe 59397 59398 ...
    python winbars.py B2_MapEditor.exe --all-toolbars

Killing the process afterwards, rather than closing it, is deliberate when the
saved layout matters: MFC writes SaveBarState on a clean exit and would persist
whatever this turned on.
"""
import ctypes as C
import sys
from ctypes import wintypes

u = C.WinDLL('user32', use_last_error=True)
k = C.WinDLL('kernel32', use_last_error=True)

WNDENUMPROC = C.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
u.EnumWindows.argtypes = [WNDENUMPROC, wintypes.LPARAM]
u.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, C.c_int]
u.GetWindowThreadProcessId.argtypes = [wintypes.HWND, C.POINTER(wintypes.DWORD)]
u.SendMessageTimeoutW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM,
                                  wintypes.LPARAM, wintypes.UINT, wintypes.UINT,
                                  C.POINTER(C.c_size_t)]

WM_COMMAND = 0x0111
SMTO_ABORTIFHUNG = 0x0002

# The nine toolbars and the three shortcut bar / movies bar containers the
# startup trace names, plus the top dock bar they live on.
TOOLBARS = [59392, 59396, 59397, 59398, 59399, 59400, 59401, 59402, 59403]
DOCKING = [59420, 59421, 59422]


def pids_for(exe):
    """Every process id whose image name matches, without opening the process."""
    import subprocess
    out = subprocess.run(['tasklist', '/FI', f'IMAGENAME eq {exe}', '/FO', 'CSV', '/NH'],
                         capture_output=True, text=True).stdout
    return [int(line.split('","')[1]) for line in out.splitlines() if line.startswith('"')]


def top_windows(pids):
    """Visible top-level windows belonging to those processes."""
    found = []

    def cb(hwnd, _):
        pid = wintypes.DWORD()
        u.GetWindowThreadProcessId(hwnd, C.byref(pid))
        if pid.value in pids and u.IsWindowVisible(hwnd):
            buf = C.create_unicode_buffer(512)
            u.GetWindowTextW(hwnd, buf, 512)
            if buf.value:
                found.append((hwnd, buf.value))
        return True

    u.EnumWindows(WNDENUMPROC(cb), 0)
    return found


def command(hwnd, wparam):
    """WM_COMMAND with a timeout, so a busy or modal editor cannot hang this."""
    result = C.c_size_t()
    ok = u.SendMessageTimeoutW(hwnd, WM_COMMAND, wparam, 0, SMTO_ABORTIFHUNG, 5000,
                               C.byref(result))
    return bool(ok)


def main():
    exe = sys.argv[1]
    rest = sys.argv[2:]
    ids = TOOLBARS + DOCKING if (not rest or rest[0] == '--all-toolbars') else [int(a) for a in rest]

    pids = pids_for(exe)
    if not pids:
        sys.exit(f'{exe} is not running')
    wins = top_windows(pids)
    if not wins:
        sys.exit(f'{exe} has no visible top-level window yet')

    hwnd, title = wins[0]
    print(f'frame 0x{hwnd:X} {title!r}')
    for nid in ids:
        print(f'  WM_COMMAND {nid}: {"sent" if command(hwnd, nid) else "TIMED OUT"}')


if __name__ == '__main__':
    main()
