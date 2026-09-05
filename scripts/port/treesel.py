"""Which items a running editor's tree has selected, and which one is the caret.

Multiple selection in this port is kept as TVIS_SELECTED on the items, which is
also what the editor's own code reads -- CSortTreeControl::IsTopSelection walks
the parents asking exactly that. So the state bit is the thing to check, and a
screenshot of a 259x110 pane is not: two adjacent highlighted rows and one
highlighted row with a focus rectangle look the same at that size.

    python treesel.py --id 400
    python treesel.py                 # every tree in the editor that has one

TVM_GETITEMSTATE answers in the message result, so unlike treetext.py this needs
nothing allocated in the target. The text does, so it is read the same way
treetext.py reads it, and this borrows that.
"""
import argparse
import ctypes as C

import treetext as T

u = T.u

TVM_GETITEMSTATE = T.TV_FIRST + 39
TVGN_CARET = 9

TVIS_SELECTED = 0x0002
TVIS_EXPANDED = 0x0020
TVIS_BOLD = 0x0010
TVIS_CUT = 0x0004
TVIS_DROPHILITED = 0x0008

STATE_NAMES = [(TVIS_SELECTED, 'SELECTED'), (TVIS_EXPANDED, 'EXPANDED'),
               (TVIS_BOLD, 'BOLD'), (TVIS_CUT, 'CUT'),
               (TVIS_DROPHILITED, 'DROPHILITED')]
STATE_MASK = TVIS_SELECTED | TVIS_EXPANDED | TVIS_BOLD | TVIS_CUT | TVIS_DROPHILITED


def item_text(hwnd, mem, hItem):
    item = T.TVITEM()
    item.mask = T.TVIF_TEXT | T.TVIF_HANDLE
    item.hItem = hItem
    item.pszText = mem.text
    item.cchTextMax = T.TEXT_CHARS
    mem.write(item)
    T.ask(hwnd, T.TVM_GETITEMW, 0, mem.base)
    return mem.read_text(True)[1]


def dump(hwnd, limit):
    items = T.walk_tree(hwnd, limit)
    caret = T.ask(hwnd, T.TVM_GETNEXTITEM, TVGN_CARET, 0) or 0
    mem = T.Remote(hwnd, C.sizeof(T.TVITEM))
    selected = 0
    try:
        print('== SysTreeView32 0x%X id=%d  %d items, caret 0x%X'
              % (hwnd, u.GetDlgCtrlID(hwnd), len(items), caret))
        for hItem in items:
            state = T.ask(hwnd, TVM_GETITEMSTATE, hItem, STATE_MASK) or 0
            names = [name for bit, name in STATE_NAMES if state & bit]
            if state & TVIS_SELECTED:
                selected += 1
            print('   %-18s 0x%-14X %-8s %s'
                  % (item_text(hwnd, mem, hItem), hItem,
                     '<-- caret' if hItem == caret else '',
                     ' '.join(names)))
    finally:
        mem.close()
    print('   %d selected' % selected)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--image', default='B2_MapEditor.exe')
    ap.add_argument('--id', type=int, help='one control id; every tree if omitted')
    ap.add_argument('--max', type=int, default=60, help='items per control')
    args = ap.parse_args()

    for hwnd in T.find(args.image, 'SysTreeView32', args.id):
        dump(hwnd, args.max)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
