"""Select an item in a running program's tree control, and read back what is
selected.

The database browser's trees are how the editor is told which object to act on:
the Open dialog enables its OK button from the tree's selection, and the
property browser fills itself from it. Selecting from outside is therefore the
step that turns "the tree has items in it" into "the editor was asked to open
one", which is the difference between a control that works and an editor that
does.

    python treeselect.py --id 400                 # what is in there, what is selected
    python treeselect.py --id 400 --first         # select the first root item
    python treeselect.py --id 400 --index 3       # select the fourth root item
    python treeselect.py --id 400 --first --then-ok

Everything here uses messages whose answer is the message result rather than
something written through a caller's pointer: TVM_GETNEXTITEM and TVM_SELECTITEM
both return an HTREEITEM or a BOOL. That is what makes them safe to send into
another process with nothing allocated in it. Reading an item's *text* is the
other kind -- TVM_GETITEM takes a TVITEM the control writes into -- and needs
VirtualAllocEx in the target, which is why this reports handles and counts and
not labels.
"""
import argparse
import ctypes as C
import subprocess
import sys
from ctypes import wintypes

u = C.WinDLL('user32', use_last_error=True)
WNDENUMPROC = C.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
u.EnumWindows.argtypes = [WNDENUMPROC, wintypes.LPARAM]
u.EnumChildWindows.argtypes = [wintypes.HWND, WNDENUMPROC, wintypes.LPARAM]
u.GetClassNameW.argtypes = [wintypes.HWND, wintypes.LPWSTR, C.c_int]
u.GetDlgCtrlID.argtypes = [wintypes.HWND]
u.GetParent.argtypes = [wintypes.HWND]
u.GetWindowThreadProcessId.argtypes = [wintypes.HWND, C.POINTER(wintypes.DWORD)]
u.SendMessageTimeoutW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM,
                                  wintypes.LPARAM, wintypes.UINT, wintypes.UINT,
                                  C.POINTER(C.c_size_t)]
u.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]

TV_FIRST = 0x1100
TVM_GETCOUNT = TV_FIRST + 5
TVM_GETNEXTITEM = TV_FIRST + 10
TVM_SELECTITEM = TV_FIRST + 11
TVM_ENSUREVISIBLE = TV_FIRST + 20
TVGN_ROOT = 0x0000
TVGN_NEXT = 0x0001
TVGN_CARET = 0x0009
WM_COMMAND = 0x0111
IDOK = 1
SMTO_ABORTIFHUNG = 0x0002


def ask(hwnd, msg, wparam=0, lparam=0, timeout=5000):
    out = C.c_size_t(0)
    if not u.SendMessageTimeoutW(hwnd, msg, wparam, lparam, SMTO_ABORTIFHUNG,
                                 timeout, C.byref(out)):
        return None
    return out.value


def pids_for(image):
    out = subprocess.run(["tasklist", "/FI", "IMAGENAME eq " + image, "/FO", "CSV", "/NH"],
                         capture_output=True, text=True).stdout
    pids = []
    for line in out.splitlines():
        parts = [p.strip('"') for p in line.split('","')]
        if len(parts) > 1 and parts[0].lower() == image.lower():
            pids.append(int(parts[1]))
    return pids


def find(pids, ctrl_id):
    found, seen = [], set()

    def visit(hwnd, _):
        if hwnd in seen:
            return True
        seen.add(hwnd)
        pid = wintypes.DWORD()
        u.GetWindowThreadProcessId(hwnd, C.byref(pid))
        if pid.value in pids:
            buf = C.create_unicode_buffer(64)
            u.GetClassNameW(hwnd, buf, 64)
            if buf.value == "SysTreeView32" and u.GetDlgCtrlID(hwnd) == ctrl_id:
                found.append(hwnd)
        u.EnumChildWindows(hwnd, WNDENUMPROC(visit), 0)
        return True

    u.EnumWindows(WNDENUMPROC(visit), 0)
    return found


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--image", default="B2_MapEditor.exe")
    ap.add_argument("--id", type=int, required=True, help="tree control id")
    ap.add_argument("--first", action="store_true", help="select the first root item")
    ap.add_argument("--index", type=int, help="select this root item, zero based")
    ap.add_argument("--then-ok", action="store_true",
                    help="post IDOK to the tree's dialog afterwards")
    args = ap.parse_args()

    pids = pids_for(args.image)
    if not pids:
        sys.exit("no running %s" % args.image)
    trees = find(set(pids), args.id)
    if not trees:
        sys.exit("no SysTreeView32 with id %d" % args.id)

    for hwnd in trees:
        count = ask(hwnd, TVM_GETCOUNT)
        selected = ask(hwnd, TVM_GETNEXTITEM, TVGN_CARET, 0)
        print("tree 0x%X id=%d items=%s selected=%s"
              % (hwnd, args.id, count, hex(selected) if selected else "none"))

        want = args.index if args.index is not None else (0 if args.first else None)
        if want is None:
            continue

        item = ask(hwnd, TVM_GETNEXTITEM, TVGN_ROOT, 0)
        for _ in range(want):
            if not item:
                break
            item = ask(hwnd, TVM_GETNEXTITEM, TVGN_NEXT, item)
        if not item:
            print("  no root item at index %d" % want)
            continue

        ask(hwnd, TVM_ENSUREVISIBLE, 0, item)
        ok = ask(hwnd, TVM_SELECTITEM, TVGN_CARET, item)
        now = ask(hwnd, TVM_GETNEXTITEM, TVGN_CARET, 0)
        print("  selected item 0x%X (result %s), caret now %s"
              % (item, ok, hex(now) if now else "none"))

        if args.then_ok:
            # The dialog, not the tree: IDOK belongs to whatever owns the
            # buttons, which is the tree's parent.
            dlg = u.GetParent(hwnd)
            u.PostMessageW(dlg, WM_COMMAND, IDOK, 0)
            print("  posted IDOK to 0x%X" % dlg)
    return 0


if __name__ == "__main__":
    sys.exit(main())
