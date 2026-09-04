"""Dump a running MFC program's menus with the enabled state they would show.

Reading a menu without opening it tells you nothing here. Every item in
ED_B2_M1.rc's IDM_MAIN is authored GRAYED, and MFC computes the real state only
in CFrameWnd::OnInitMenuPopup, which runs when the user drops the popup: it
walks the items, builds a CCmdUI for each and routes it, and in this editor
CMainFrame::OnUpdateUserCommand then asks ICommandHandlerContainer::UpdateCommand
whether the command is enabled. So a dump taken cold reports the resource, not
the editor, and reports it identically for a working editor and a broken one.

This primes each popup with WM_INITMENUPOPUP first, which is the message the
menu sends when it opens, and only then reads GetMenuState. That is the same
path a mouse takes, minus the mouse.

    python menudump.py B2_MapEditor.exe                  # tree to stdout
    python menudump.py B2_MapEditor.exe --json out.json   # and/or JSON
    python menudump.py B2_MapEditor.exe --raw             # skip priming, read the resource

Two of these side by side is the point: ours against the shipped editor in
C:\\Games\\BK2-FoTR\\bin_, which answers "should this item be enabled" with
evidence instead of reasoning. Menu handles read out of another process work
across the bitness boundary, so the x86 original and this x64 port compare
directly.

Kill the shipped editor rather than closing it, as always: it writes
SaveBarState on a clean exit.
"""
import argparse
import ctypes as C
import json
import subprocess
import sys
from ctypes import wintypes

u = C.WinDLL('user32', use_last_error=True)

u.GetMenu.argtypes = [wintypes.HWND]
u.GetMenu.restype = wintypes.HMENU
u.GetMenuItemCount.argtypes = [wintypes.HMENU]
u.GetSubMenu.argtypes = [wintypes.HMENU, C.c_int]
u.GetSubMenu.restype = wintypes.HMENU
u.GetMenuStringW.argtypes = [wintypes.HMENU, wintypes.UINT, wintypes.LPWSTR, C.c_int, wintypes.UINT]
u.GetMenuItemID.argtypes = [wintypes.HMENU, C.c_int]
u.GetMenuState.argtypes = [wintypes.HMENU, wintypes.UINT, wintypes.UINT]
u.GetWindowThreadProcessId.argtypes = [wintypes.HWND, C.POINTER(wintypes.DWORD)]
u.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, C.c_int]
u.SendMessageTimeoutW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM,
                                  wintypes.LPARAM, wintypes.UINT, wintypes.UINT,
                                  C.POINTER(C.c_size_t)]
WNDENUMPROC = C.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
u.EnumWindows.argtypes = [WNDENUMPROC, wintypes.LPARAM]

MF_BYPOSITION = 0x0400
MF_GRAYED = 0x0001
MF_DISABLED = 0x0002
MF_CHECKED = 0x0008
MF_SEPARATOR = 0x0800
MF_POPUP = 0x0010
WM_INITMENU = 0x0116
WM_INITMENUPOPUP = 0x0117
SMTO_ABORTIFHUNG = 0x0002
# GetMenuState answers this for an index that is not there.
MENU_STATE_ERROR = 0xFFFFFFFF


def pids_for(image):
    out = subprocess.run(["tasklist", "/FI", "IMAGENAME eq " + image, "/FO", "CSV", "/NH"],
                         capture_output=True, text=True).stdout
    pids = []
    for line in out.splitlines():
        parts = [p.strip('"') for p in line.split('","')]
        if len(parts) > 1 and parts[0].lower() == image.lower():
            pids.append(int(parts[1]))
    return pids


def frames_with_menus(pids):
    """Top-level windows in those processes that carry a menu bar."""
    found = []

    def visit(hwnd, _):
        pid = wintypes.DWORD()
        u.GetWindowThreadProcessId(hwnd, C.byref(pid))
        if pid.value in pids:
            hmenu = u.GetMenu(hwnd)
            if hmenu:
                buf = C.create_unicode_buffer(256)
                u.GetWindowTextW(hwnd, buf, 256)
                found.append((hwnd, hmenu, buf.value))
        return True

    u.EnumWindows(WNDENUMPROC(visit), 0)
    return found


def prime(hwnd, hmenu, index):
    """Send the popup the message that makes MFC compute its state.

    lParam is the popup's index in the menu bar in the low word and, in the
    high word, whether it is a window menu. Both matter to CFrameWnd, which
    refuses to update a system menu.
    """
    out = C.c_size_t(0)
    u.SendMessageTimeoutW(hwnd, WM_INITMENUPOPUP, hmenu, index & 0xFFFF,
                          SMTO_ABORTIFHUNG, 4000, C.byref(out))


def read_menu(hwnd, hmenu, do_prime, depth=0, index_in_parent=0):
    if not hmenu or depth > 4:
        return []
    if do_prime:
        prime(hwnd, hmenu, index_in_parent)
    items = []
    for i in range(u.GetMenuItemCount(hmenu)):
        state = u.GetMenuState(hmenu, i, MF_BYPOSITION)
        if state == MENU_STATE_ERROR:
            continue
        buf = C.create_unicode_buffer(512)
        u.GetMenuStringW(hmenu, i, buf, 512, MF_BYPOSITION)
        sub = u.GetSubMenu(hmenu, i)
        item = {
            "text": buf.value,
            "id": u.GetMenuItemID(hmenu, i) & 0xFFFFFFFF if not sub else None,
            "separator": bool(state & MF_SEPARATOR),
            "popup": bool(sub),
            # A menu item is unusable if either bit is set; MFC's
            # CCmdUI::Enable sets and clears both together.
            "enabled": not (state & (MF_GRAYED | MF_DISABLED)),
            "checked": bool(state & MF_CHECKED),
        }
        if sub:
            item["items"] = read_menu(hwnd, sub, do_prime, depth + 1, i)
        items.append(item)
    return items


def render(items, indent=2):
    lines = []
    for it in items:
        if it["separator"]:
            lines.append(" " * indent + "-" * 20)
            continue
        mark = "   " if it["enabled"] else "[X]"
        check = " *" if it["checked"] else ""
        ident = "" if it["id"] is None else "  id=%d" % it["id"]
        lines.append("%s%s %s%s%s" % (" " * indent, mark, it["text"].replace("\t", "  "),
                                      ident, check))
        if it.get("items"):
            lines.extend(render(it["items"], indent + 4))
    return lines


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("image", nargs="?", default="B2_MapEditor.exe")
    ap.add_argument("--json", help="write the dump here as well")
    ap.add_argument("--raw", action="store_true",
                    help="do not send WM_INITMENUPOPUP; report the resource's own state")
    args = ap.parse_args()

    pids = pids_for(args.image)
    if not pids:
        sys.exit("no running %s" % args.image)

    frames = frames_with_menus(set(pids))
    if not frames:
        sys.exit("no window with a menu bar in %s (pids %s)"
                 % (args.image, ", ".join(str(p) for p in pids)))

    dump = []
    for hwnd, hmenu, title in frames:
        if not args.raw:
            # WM_INITMENU first, as the real sequence does, then each popup.
            out = C.c_size_t(0)
            u.SendMessageTimeoutW(hwnd, WM_INITMENU, hmenu, 0,
                                  SMTO_ABORTIFHUNG, 4000, C.byref(out))
        frame = {"hwnd": hwnd, "title": title, "primed": not args.raw,
                 "items": read_menu(hwnd, hmenu, not args.raw)}
        dump.append(frame)
        print("%s  hwnd=0x%X  %s" % (title or "(untitled)", hwnd,
                                     "" if args.raw else "(primed)"))
        print("\n".join(render(frame["items"])))
        enabled = []

        def count(items):
            for it in items:
                if not it["separator"] and not it["popup"]:
                    enabled.append(it["enabled"])
                count(it.get("items", []))

        count(frame["items"])
        print("\n  %d of %d commands enabled" % (sum(enabled), len(enabled)))

    if args.json:
        with open(args.json, "w", encoding="utf-8") as f:
            json.dump(dump, f, indent=2)
        print("\nwrote %s" % args.json)
    return 0


if __name__ == "__main__":
    sys.exit(main())
