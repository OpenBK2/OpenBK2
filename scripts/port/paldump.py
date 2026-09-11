"""Dump what the visible shortcut-bar palette shows, in a form two toolkits diff on.

    python paldump.py              # the palette on the visible tab
    python paldump.py --tab 2      # click tab 2 of the visible tab strip first
    python paldump.py --tabs       # how many tabs the visible strip has

Every visible control under the visible tab page, however deep -- the wx
palettes sit under a scrolled window the MFC ones do not have -- reduced to its
content: captions and check states, combo entries and selection, list rows in
every column with selection and check, edit and static text, enabled state.
Positions, ids and z-order are left out and the lines are sorted, so a layout
difference is not a difference and a content difference is.

One thing it cannot read is an owner-drawn button's state (wx bitmap toggles
are BS_OWNERDRAW): those show as "ownerdrawn button" and need a screenshot.
"""
import argparse
import sys
import time

import uiprobe as P


def strip_and_page():
    """The visible SysTabControl32 of a CDefault3DTabWindow, and its visible page."""
    for h in P.all_windows():
        if P.cls(h) == 'SysTabControl32' and P.u.IsWindowVisible(h):
            pages = [k for k in P.children(P.u.GetParent(h)) if k != h and P.u.IsWindowVisible(k)]
            if pages:
                return h, pages[0]
    raise SystemExit('no visible tab strip -- is a document open?')


def dump(out=print):
    strip, page = strip_and_page()
    out('tab %s of %s' % (P.send(strip, P.TCM_GETCURSEL), P.send(strip, P.TCM_GETITEMCOUNT)))
    for line in P.content(page):
        out('  ' + line)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--desktop', default='bk2probe')
    ap.add_argument('--tab', type=int)
    ap.add_argument('--tabs', action='store_true')
    ap.add_argument('--settle', type=float, default=4.0, help='seconds after a tab click')
    a = ap.parse_args()
    P.attach(a.desktop)
    if a.tabs:
        print(P.send(strip_and_page()[0], P.TCM_GETITEMCOUNT))
        return 0
    if a.tab is not None:
        P.click_tab(strip_and_page()[0], a.tab)
        time.sleep(a.settle)
    dump()
    return 0


if __name__ == '__main__':
    sys.exit(main())
