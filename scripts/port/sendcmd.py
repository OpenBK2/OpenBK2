"""Post a menu command to a running editor, the way choosing the menu item does.

Every command the editor's menus carry is a WM_COMMAND with the item's id, and
CMainFrame routes it through ON_COMMAND_RANGE to
ICommandHandlerContainer::HandleCommand. Posting one from outside reaches the
same handler, so the editor can be driven without a mouse. menudump.py lists the
ids and says which of them the editor would currently accept; this sends them.

    python sendcmd.py 1041                # File -> Open
    python sendcmd.py 1047                # File -> Select Tables
    python sendcmd.py 0x411 --hex
    python sendcmd.py 1047 --image B2_MapEditor.exe --wait 5

Posted, never sent. Several of these open a modal dialog, and a modal dialog
does not return until it is dismissed: SendMessage would block this script for
as long as the dialog is up, and worse, it would block it inside the editor's
own message handling. Posting puts the command in the queue and lets the
editor's loop run it, which is the only way a dialog can appear at all.

That also means this returns before the command has finished. --wait gives the
editor a few seconds to get somewhere before the next probe looks, which is
usually what a caller wants between a send and a windump.
"""
import argparse
import ctypes as C
import subprocess
import sys
import time
from ctypes import wintypes

u = C.WinDLL('user32', use_last_error=True)
u.GetMenu.argtypes = [wintypes.HWND]
u.GetMenu.restype = wintypes.HMENU
u.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, C.c_int]
u.GetWindowThreadProcessId.argtypes = [wintypes.HWND, C.POINTER(wintypes.DWORD)]
u.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
u.IsWindowVisible.argtypes = [wintypes.HWND]
WNDENUMPROC = C.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
u.EnumWindows.argtypes = [WNDENUMPROC, wintypes.LPARAM]

WM_COMMAND = 0x0111


def pids_for(image):
    out = subprocess.run(["tasklist", "/FI", "IMAGENAME eq " + image, "/FO", "CSV", "/NH"],
                         capture_output=True, text=True).stdout
    pids = []
    for line in out.splitlines():
        parts = [p.strip('"') for p in line.split('","')]
        if len(parts) > 1 and parts[0].lower() == image.lower():
            pids.append(int(parts[1]))
    return pids


def main_frame(pids):
    """The top-level window carrying the menu bar, which is where the commands
    are routed from."""
    found = []

    def visit(hwnd, _):
        pid = wintypes.DWORD()
        u.GetWindowThreadProcessId(hwnd, C.byref(pid))
        if pid.value in pids and u.GetMenu(hwnd) and u.IsWindowVisible(hwnd):
            buf = C.create_unicode_buffer(256)
            u.GetWindowTextW(hwnd, buf, 256)
            found.append((hwnd, buf.value))
        return True

    u.EnumWindows(WNDENUMPROC(visit), 0)
    return found


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("ids", nargs="+", help="command ids to post, in order")
    ap.add_argument("--image", default="B2_MapEditor.exe")
    ap.add_argument("--wait", type=float, default=0.0,
                    help="seconds to wait after each command")
    args = ap.parse_args()

    pids = pids_for(args.image)
    if not pids:
        sys.exit("no running %s" % args.image)
    frames = main_frame(set(pids))
    if not frames:
        sys.exit("no visible window with a menu bar in %s" % args.image)
    hwnd, title = frames[0]
    print("frame 0x%X  %s" % (hwnd, title))

    for raw in args.ids:
        nID = int(raw, 0)
        # The high word is the notification code, zero for a menu command, and
        # lParam is zero because a menu command has no control behind it.
        ok = u.PostMessageW(hwnd, WM_COMMAND, nID & 0xFFFF, 0)
        print("  WM_COMMAND %d (0x%X) posted: %s" % (nID, nID, "yes" if ok else "FAILED"))
        if args.wait:
            time.sleep(args.wait)
    return 0


if __name__ == "__main__":
    sys.exit(main())
