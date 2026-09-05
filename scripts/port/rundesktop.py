"""Run the editor on a Win32 desktop nobody is looking at.

A desktop object has its own input queue, its own foreground window and its own
cursor. A program on a second desktop therefore cannot take the focus from, or
receive a click meant for, whoever is at the keyboard -- and neither can a probe
driving it. That is the point: the isolation is structural rather than a rule
the tooling has to remember, and the tooling forgot twice.

    python rundesktop.py                       # start the editor on bk2probe
    python rundesktop.py --list                # what is on that desktop now
    python rundesktop.py --kill

    powershell -File winshot.ps1 -Desktop bk2probe -Out shot.png
    python windump.py --desktop bk2probe

What works there and what does not, both measured rather than assumed:

    SendInput            fails, ERROR_ACCESS_DENIED. Real mouse and keyboard
                         input only ever reaches the desktop the hardware is
                         attached to, so dragbar.py cannot be used here.
    PostMessage          works. That covers sendcmd.py, treesel.py, treetext.py,
                         windump.py and every other probe here, and it covers
                         modifier clicks too: SEC_TREECLASS::OnLButtonDown reads
                         Ctrl and Shift out of wParam's MK_ bits rather than the
                         keyboard state, so a posted click carries them.

What is left needing the real desktop is the modal loops -- docking drags,
splitter resizes, menu tracking -- because CDockContext::Track asks
GetCursorPos() where the pointer is, and no posted message moves it.
"""
import argparse
import ctypes as C
import subprocess
import time
from ctypes import wintypes

u = C.WinDLL('user32', use_last_error=True)
k = C.WinDLL('kernel32', use_last_error=True)

GENERIC_ALL = 0x10000000
DESKTOP_CREATEWINDOW = 0x0002
STARTF_USESHOWWINDOW = 0x00000001
SW_SHOW = 5
CREATE_NEW_CONSOLE = 0x00000010

u.CreateDesktopW.restype = wintypes.HANDLE
u.CreateDesktopW.argtypes = [wintypes.LPCWSTR, wintypes.LPCWSTR, C.c_void_p,
                             wintypes.DWORD, wintypes.DWORD, C.c_void_p]
u.OpenDesktopW.restype = wintypes.HANDLE
u.OpenDesktopW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
u.SetThreadDesktop.argtypes = [wintypes.HANDLE]
u.GetThreadDesktop.restype = wintypes.HANDLE
u.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, C.c_int]
u.GetClassNameW.argtypes = [wintypes.HWND, wintypes.LPWSTR, C.c_int]
u.GetWindowThreadProcessId.argtypes = [wintypes.HWND, C.POINTER(wintypes.DWORD)]

WNDENUMPROC = C.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
u.EnumDesktopWindows.argtypes = [wintypes.HANDLE, WNDENUMPROC, wintypes.LPARAM]


class STARTUPINFOW(C.Structure):
    _fields_ = [('cb', wintypes.DWORD), ('lpReserved', wintypes.LPWSTR),
                ('lpDesktop', wintypes.LPWSTR), ('lpTitle', wintypes.LPWSTR),
                ('dwX', wintypes.DWORD), ('dwY', wintypes.DWORD),
                ('dwXSize', wintypes.DWORD), ('dwYSize', wintypes.DWORD),
                ('dwXCountChars', wintypes.DWORD), ('dwYCountChars', wintypes.DWORD),
                ('dwFillAttribute', wintypes.DWORD), ('dwFlags', wintypes.DWORD),
                ('wShowWindow', wintypes.WORD), ('cbReserved2', wintypes.WORD),
                ('lpReserved2', C.c_void_p), ('hStdInput', wintypes.HANDLE),
                ('hStdOutput', wintypes.HANDLE), ('hStdError', wintypes.HANDLE)]


class PROCESS_INFORMATION(C.Structure):
    _fields_ = [('hProcess', wintypes.HANDLE), ('hThread', wintypes.HANDLE),
                ('dwProcessId', wintypes.DWORD), ('dwThreadId', wintypes.DWORD)]


def open_or_create(name):
    """The desktop, made if it is not there yet.

    A desktop lives as long as something is using it, so one made by an earlier
    run is still there while the editor from that run is.
    """
    desk = u.OpenDesktopW(name, 0, False, GENERIC_ALL)
    if desk:
        return desk, False
    desk = u.CreateDesktopW(name, None, None, 0, GENERIC_ALL, None)
    if not desk:
        raise SystemExit('CreateDesktop(%s) failed: %d' % (name, C.get_last_error()))
    return desk, True


def launch(desktop, exe, cwd):
    si = STARTUPINFOW()
    si.cb = C.sizeof(si)
    # The one field that matters: the process starts on that desktop and every
    # window it makes belongs to it.
    si.lpDesktop = desktop
    si.dwFlags = STARTF_USESHOWWINDOW
    si.wShowWindow = SW_SHOW
    pi = PROCESS_INFORMATION()
    if not k.CreateProcessW(exe, None, None, None, False, CREATE_NEW_CONSOLE,
                            None, cwd, C.byref(si), C.byref(pi)):
        raise SystemExit('CreateProcess failed: %d' % C.get_last_error())
    k.CloseHandle(pi.hThread)
    k.CloseHandle(pi.hProcess)
    return pi.dwProcessId


def windows_on(desktop):
    desk = u.OpenDesktopW(desktop, 0, False, GENERIC_ALL)
    if not desk:
        return []
    found = []

    def cb(h, _):
        title = C.create_unicode_buffer(256)
        cls = C.create_unicode_buffer(128)
        u.GetWindowTextW(h, title, 256)
        u.GetClassNameW(h, cls, 128)
        pid = wintypes.DWORD()
        u.GetWindowThreadProcessId(h, C.byref(pid))
        found.append((h, pid.value, cls.value, title.value))
        return True

    proc = WNDENUMPROC(cb)
    u.EnumDesktopWindows(desk, proc, 0)
    return found


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--desktop', default='bk2probe')
    ap.add_argument('--exe', default=r'C:\Temp\edtest\bin\B2_MapEditor.exe')
    ap.add_argument('--cwd', default=r'C:\Games\bk2\bin',
                    help='the install the data is resolved against')
    ap.add_argument('--wait', type=int, default=30, help='seconds to let it start')
    ap.add_argument('--list', action='store_true', help='what is on that desktop now')
    ap.add_argument('--kill', action='store_true')
    args = ap.parse_args()

    if args.kill:
        subprocess.run(['taskkill', '/F', '/IM', 'B2_MapEditor.exe'], capture_output=True)
        print('killed')
        return 0

    if args.list:
        for h, pid, cls, title in windows_on(args.desktop):
            print('  0x%-10X pid=%-6d %-28s %s' % (h, pid, cls, title))
        return 0

    desk, made = open_or_create(args.desktop)
    print('desktop %s %s (handle 0x%X)' % (args.desktop,
                                           'created' if made else 'already there', desk))
    pid = launch(args.desktop, args.exe, args.cwd)
    print('started %s as pid %d on %s' % (args.exe, pid, args.desktop))
    print('the interactive desktop is untouched; nothing of this is on screen')
    for _ in range(args.wait):
        time.sleep(1)
        if any(cls.startswith('Afx') and title for _, _, cls, title in windows_on(args.desktop)):
            break
    for h, wpid, cls, title in windows_on(args.desktop):
        if title:
            print('  0x%-10X pid=%-6d %-28s %s' % (h, wpid, cls, title))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
