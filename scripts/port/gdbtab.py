"""Select a table in a running editor's Game Database Window, so the browser
tree populates on demand.

The database browser does not build its tree at startup. CDWGDBBrowser fills it
from OnTabSelected, which the message map reaches two ways:

    ON_MESSAGE( WM_GDB_BROWSER, OnTabSelected )
    ON_CBN_SELCHANGE( IDC_TREE_GDB_BROWSER, OnTabSelected )

and OnTabSelected calls CreateTree, which sets a 100 ms timer that adds a chunk
of items per tick and reschedules itself until the iterator ends. That is the
"something asynchronous" that a previous session watched start on its own after
two and a half minutes of idle and never start again in thirteen: nothing
schedules it, a selection does, and until one arrives the tree stays empty.

So this makes the population reproducible instead of waited for. It finds the
combo (IDC_TREE_GDB_BROWSER, 135), sets a selection and tells the parent the
selection changed, exactly as clicking the combo would.

    python gdbtab.py B2_MapEditor.exe                 # list the combos and their items
    python gdbtab.py B2_MapEditor.exe --select 1      # pick item 1 in every combo found
    python gdbtab.py B2_MapEditor.exe --select 1 --hwnd 0x12345
    python gdbtab.py B2_MapEditor.exe --poke          # send WM_GDB_BROWSER instead

Run it with the editor up, and with crashwatch.py watching if the point is to
catch what the population does.

Posted rather than sent: a send from outside blocks this script for as long as
the editor takes, and the editor is what is being watched. Posting puts the
notification in its queue and lets its own message loop take it, which is also
the only way the timer that follows can run.
"""
import argparse
import ctypes as C
import sys
from ctypes import wintypes

u = C.WinDLL('user32', use_last_error=True)
k = C.WinDLL('kernel32', use_last_error=True)

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

IDC_TREE_GDB_BROWSER = 135
WM_USER = 0x0400
WM_GDB_BROWSER = WM_USER + 2
WM_COMMAND = 0x0111
CB_GETCOUNT = 0x0146
CB_GETCURSEL = 0x0147
CB_SETCURSEL = 0x014E
CBN_SELCHANGE = 1
SMTO_ABORTIFHUNG = 0x0002


def ask(hwnd, msg, wparam=0, lparam=0, timeout=3000):
    """A message that returns a value, with a timeout, so a wedged editor does
    not wedge the probe with it."""
    out = C.c_size_t(0)
    if not u.SendMessageTimeoutW(hwnd, msg, wparam, lparam, SMTO_ABORTIFHUNG,
                                 timeout, C.byref(out)):
        return None
    return out.value


def class_name(hwnd):
    buf = C.create_unicode_buffer(256)
    u.GetClassNameW(hwnd, buf, 256)
    return buf.value


def pids_for(image):
    """Every pid whose main window belongs to a process with this image name."""
    import subprocess
    out = subprocess.run(["tasklist", "/FI", "IMAGENAME eq " + image, "/FO", "CSV", "/NH"],
                         capture_output=True, text=True).stdout
    pids = []
    for line in out.splitlines():
        parts = [p.strip('"') for p in line.split('","')]
        if len(parts) > 1 and parts[0].lower() == image.lower():
            pids.append(int(parts[1]))
    return pids


def find_combos(pids):
    """Every window in those processes carrying the browser combo's control id."""
    # A set, and a seen set for the walk: EnumChildWindows is recursive already,
    # so recursing into it as well reaches every descendant once per ancestor and
    # reports one combo as four.
    found, seen = [], set()

    def visit(hwnd, _):
        if hwnd in seen:
            return True
        seen.add(hwnd)
        pid = wintypes.DWORD()
        u.GetWindowThreadProcessId(hwnd, C.byref(pid))
        if pid.value in pids and u.GetDlgCtrlID(hwnd) == IDC_TREE_GDB_BROWSER:
            found.append(hwnd)
        u.EnumChildWindows(hwnd, WNDENUMPROC(visit), 0)
        return True

    u.EnumWindows(WNDENUMPROC(visit), 0)
    return found


def describe(hwnd):
    count = ask(hwnd, CB_GETCOUNT)
    cur = ask(hwnd, CB_GETCURSEL)
    parent = u.GetParent(hwnd)
    # CB_GETCOUNT answers CB_ERR (-1) on a window that is not a combo box, which
    # is worth saying rather than reporting as zero tables.
    if count in (None, 0xFFFFFFFF, -1):
        count = "not a combo box"
    if cur in (0xFFFFFFFF, -1):
        cur = "none"
    print("  hwnd=0x%X class=%s items=%s selected=%s parent=0x%X %s"
          % (hwnd, class_name(hwnd), count, cur, parent, class_name(parent)))
    return parent


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("image", nargs="?", default="B2_MapEditor.exe")
    ap.add_argument("--select", type=int, help="combo index to select")
    ap.add_argument("--hwnd", help="act on this combo only (hex or decimal)")
    ap.add_argument("--poke", action="store_true",
                    help="post WM_GDB_BROWSER to the parent instead of a selection")
    args = ap.parse_args()

    pids = pids_for(args.image)
    if not pids:
        sys.exit("no running %s" % args.image)
    print("%s pids: %s" % (args.image, ", ".join(str(p) for p in pids)))

    combos = find_combos(set(pids))
    if args.hwnd:
        want = int(args.hwnd, 0)
        combos = [h for h in combos if h == want]
    if not combos:
        sys.exit("no window with control id %d found" % IDC_TREE_GDB_BROWSER)

    print("combos (control id %d):" % IDC_TREE_GDB_BROWSER)
    parents = [describe(h) for h in combos]

    if args.poke:
        for parent in parents:
            print("posting WM_GDB_BROWSER to 0x%X" % parent)
            u.PostMessageW(parent, WM_GDB_BROWSER, 0, 0)
        return 0

    if args.select is None:
        print("\nnothing sent. --select N picks an item, --poke sends WM_GDB_BROWSER")
        return 0

    for hwnd, parent in zip(combos, parents):
        before = ask(hwnd, CB_GETCURSEL)
        ask(hwnd, CB_SETCURSEL, args.select)
        after = ask(hwnd, CB_GETCURSEL)
        # The control is set directly, which raises no notification, so the
        # parent has to be told the way the combo itself would have told it.
        u.PostMessageW(parent, WM_COMMAND,
                       (CBN_SELCHANGE << 16) | (IDC_TREE_GDB_BROWSER & 0xFFFF), hwnd)
        print("0x%X: selection %s -> %s, CBN_SELCHANGE posted to 0x%X"
              % (hwnd, before, after, parent))
    return 0


if __name__ == "__main__":
    sys.exit(main())
